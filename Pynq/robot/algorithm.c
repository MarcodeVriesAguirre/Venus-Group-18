#include "shared.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <libpynq.h>
#include <stepper.h>
#include <iic.h>
#include <sys/select.h>
#include <math.h>
#include "vl53l0x.h"

/* ---------- distance sensor ---------- */
#define TOF_ADDR 0x29
#define STOP_DISTANCE_MM 38

/* ---------- turn / drive calibration ---------- */
#define TURN_SPEED     20000
#define STEPS_90        625   /* steps for a 90-deg turn (spin=2500 steps=360) */
#define DRIVE_SPEED    15000
#define STEPS_PER_CM   229    /* 1600 steps = 7 cm  ->  ~228.6 steps/cm */

/* ---------- cube size measurement (angular sweep) ---------- */
/* radians turned per in-place turn-step, derived from the turn calibration:
 * STEPS_90 steps == 90 deg == pi/2 rad */
#define RAD_PER_TURN_STEP   (1.57079633f / (float)STEPS_90)  /* (pi/2) / STEPS_90 */
#define CUBE_LOST_MARGIN_MM 20   /* edge reached once distance exceeds center+this */
#define SWEEP_STEP_CHUNK    10   /* turn-steps per sweep iteration */
#define MAX_SWEEP_STEPS     400  /* safety cap per side (~57 deg) */
#define SIZE_THRESHOLD_MM   25   /* width >= this -> large cube. PROVISIONAL: big
                                  * blue measures ~37 mm; set to midpoint between
                                  * small and large once both widths are known. */
#define MOUNTAIN_WIDTH_MM   50   /* width >= this -> not a cube at all, it's the
                                  * cardboard "mountain". Set above the largest
                                  * real cube width (big blue ~37 mm). */

vl53x tof_sensor;

/* ---------- color sensor (TCS3200) ---------- */
#define PIN_S0   IO_AR4
#define PIN_S1   IO_AR5
#define PIN_S2   IO_AR6
#define PIN_S3   IO_AR7
#define PIN_OUT  IO_AR8

/* ---------- mapping / position ---------- */
#define radperstep 0.00251327   /* rad per turn-step for dirup(): (pi/2)/STEPS_90,
                                  * == 2*pi/2500 (2500 steps = full 360 spin) */

/* ---------- infrared line sensor (digital, from Infrared/main.c) ---------- */
#define PIN_IR          IO_AR9
#define IR_BLACK_LEVEL  GPIO_LEVEL_HIGH   /* flip to GPIO_LEVEL_LOW if it reads inverted */

#define SETTLE_US        3000
#define TIMEOUT_US       100000
#define SAMPLES_PER_CH   15

/* white-balance references: each channel's raw period on a WHITE target.
 * These hold at 2% output scaling (set_scaling_2). Re-measure if scaling,
 * sensor, or read distance changes. */
#define R_MIN 171
#define G_MIN 255
#define B_MIN 241

#define NO_OBJECT_CLEAR_US 2000   /* clear > this -> almost no light -> unknown */
#define WHITE_SPREAD_PCT   13     /* low-chroma & bright & no blue-tilt -> WHITE */
#define CHROMA_SPREAD_PCT  20     /* spread at/above this -> a saturated RED/BLUE.
                                   * Kept above black's residual tilt (~19% after
                                   * white-balance) so black is NOT called a color. */
#define GREEN_SPREAD_PCT    9     /* desaturated green: blue most-absorbed (bn==mx)
                                   * with at least this much spread. The green cube
                                   * is so weak its RED channel is brightest, so it
                                   * can't be found by "greenest channel"; the only
                                   * stable cue is suppressed blue. */
/* brown/cardboard rejection: real cubes read the same color every time, brown
 * does not. Read COLOR_VOTE_SAMPLES times; need COLOR_VOTE_REQUIRED to agree,
 * else the object is reported UNKNOWN. Raise REQUIRED toward SAMPLES to reject
 * brown harder; lower it if real cubes get wrongly rejected at distance. */
#define COLOR_VOTE_SAMPLES   5
#define COLOR_VOTE_REQUIRED  4

#define BLACK_MIN_CLEAR_US 350    /* low-chroma AND clear > this -> BLACK. Sits
                                   * between the colors' clear (~120-240) and
                                   * black's (~600) at the gripper read distance.
                                   * Grows with distance: read black at the
                                   * operating distance and re-calibrate. */

typedef enum {
    FILTER_RED,
    FILTER_GREEN,
    FILTER_BLUE,
    FILTER_CLEAR
} filter_t;

typedef struct {
    uint32_t red_us;
    uint32_t green_us;
    uint32_t blue_us;
    uint32_t clear_us;
} raw_reading_t;

typedef enum {
    COLOR_UNKNOWN   = 0, /* nothing useful / off-color (e.g. cardboard wall) */
    COLOR_BLACK     = 1,
    COLOR_WHITE     = 2,
    COLOR_RED       = 4,
    COLOR_GREEN     = 5,
    COLOR_BLUE      = 6
} color_t;

typedef struct {
    raw_reading_t raw;
    color_t color;
} color_result_t;

color_result_t latest_color_result;

/* ---------- robot pose ---------- */
robpos global_state = {0, 0, 1, PTHREAD_MUTEX_INITIALIZER};

/* ---------- timing helpers ---------- */
static uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000ULL);
}

static void delay_us(uint32_t us) {
    uint64_t start = now_us();
    while ((now_us() - start) < us) { /* busy wait */ }
}

static bool wait_for_level(io_t pin, gpio_level_t target, uint32_t timeout_us) {
    uint64_t start = now_us();
    while ((now_us() - start) < timeout_us) {
        if (gpio_get_level(pin) == target) return true;
    }
    return false;
}

/* ---------- color sensor internals ---------- */
/* Set output scaling to 2%. Lower scaling -> lower output frequency -> ~10x
 * longer pulse periods than 20%, which gives far more timing resolution so
 * blue/black/green separate reliably. White-balance refs above assume this. */
static void set_scaling_2(void) {
    gpio_set_level(PIN_S0, GPIO_LEVEL_LOW);
    gpio_set_level(PIN_S1, GPIO_LEVEL_HIGH);
    delay_us(SETTLE_US);
}

static void set_filter(filter_t f) {
    switch (f) {
        case FILTER_RED:
            gpio_set_level(PIN_S2, GPIO_LEVEL_LOW);
            gpio_set_level(PIN_S3, GPIO_LEVEL_LOW);
            break;
        case FILTER_BLUE:
            gpio_set_level(PIN_S2, GPIO_LEVEL_LOW);
            gpio_set_level(PIN_S3, GPIO_LEVEL_HIGH);
            break;
        case FILTER_CLEAR:
            gpio_set_level(PIN_S2, GPIO_LEVEL_HIGH);
            gpio_set_level(PIN_S3, GPIO_LEVEL_LOW);
            break;
        case FILTER_GREEN:
            gpio_set_level(PIN_S2, GPIO_LEVEL_HIGH);
            gpio_set_level(PIN_S3, GPIO_LEVEL_HIGH);
            break;
    }
    delay_us(SETTLE_US);
}

static uint32_t pulse_in_low_us(io_t pin, uint32_t timeout_us) {
    uint64_t t0, t1;
    if (!wait_for_level(pin, GPIO_LEVEL_HIGH, timeout_us)) return 0;
    if (!wait_for_level(pin, GPIO_LEVEL_LOW, timeout_us)) return 0;
    t0 = now_us();
    if (!wait_for_level(pin, GPIO_LEVEL_HIGH, timeout_us)) return 0;
    t1 = now_us();
    return (uint32_t)(t1 - t0);
}

static uint32_t read_channel_us(filter_t f) {
    uint64_t sum = 0;
    int valid = 0;
    set_filter(f);
    for (int i = 0; i < SAMPLES_PER_CH; i++) {
        uint32_t p = pulse_in_low_us(PIN_OUT, TIMEOUT_US);
        if (p > 0) {
            sum += p;
            valid++;
        }
        delay_us(500);
    }
    if (valid == 0) return 0;
    return (uint32_t)(sum / valid);
}

static raw_reading_t read_raw_rgbc(void) {
    raw_reading_t d;
    d.red_us   = read_channel_us(FILTER_RED);
    d.green_us = read_channel_us(FILTER_GREEN);
    d.blue_us  = read_channel_us(FILTER_BLUE);
    d.clear_us = read_channel_us(FILTER_CLEAR);
    return d;
}

static color_t guess_color(raw_reading_t raw) {
    if (raw.red_us == 0 || raw.green_us == 0 || raw.blue_us == 0 || raw.clear_us == 0) {
        return COLOR_UNKNOWN;
    }

    /* Almost no light on any channel -> nothing useful in front -> unknown. */
    if (raw.clear_us > NO_OBJECT_CLEAR_US) {
        return COLOR_UNKNOWN;
    }

    /* White-balance the channels. The sensor's red diodes respond faster than
     * green/blue, so dividing each channel by its period on a WHITE target
     * (R_MIN/G_MIN/B_MIN) makes equal light give equal values. Scaled by 1000
     * to stay in integer math; smaller == more of that color. */
    uint32_t rn = raw.red_us   * 1000u / R_MIN;
    uint32_t gn = raw.green_us * 1000u / G_MIN;
    uint32_t bn = raw.blue_us  * 1000u / B_MIN;

    uint32_t mn = rn, mx = rn;
    if (gn < mn) mn = gn;
    if (bn < mn) mn = bn;
    if (gn > mx) mx = gn;
    if (bn > mx) mx = bn;

    uint32_t spread = (mx - mn) * 100u;   /* compare against mn * <pct> */

    /* A channel strongly dominates -> a saturated color (red/blue). The smallest
     * white-balanced channel got the most light. Threshold is set above black's
     * residual ~19% tilt so a black cube is not mistaken for a faint red. */
    if (spread >= mn * CHROMA_SPREAD_PCT) {
        if (rn == mn) return COLOR_RED;
        if (gn == mn) return COLOR_GREEN;
        return COLOR_BLUE;
    }

    /* Weak / achromatic from here down. Black first: it absorbs nearly all light,
     * so its clear period is far longer than any color's at the read distance. */
    if (raw.clear_us > BLACK_MIN_CLEAR_US) {
        return COLOR_BLACK;
    }

    /* Desaturated green: too washed out to win the chroma test (its red channel
     * is actually the brightest), but it absorbs blue more than red/green, so the
     * blue channel is the most-absorbed (largest white-balanced) one. That tilt,
     * plus enough spread, separates it from neutral white. */
    if (bn == mx && spread >= mn * GREEN_SPREAD_PCT) {
        return COLOR_GREEN;
    }

    /* Bright and essentially neutral -> white; anything else is an off-tint we
     * don't recognize (e.g. a cardboard wall) -> unknown. */
    if (spread < mn * WHITE_SPREAD_PCT) {
        return COLOR_WHITE;
    }
    return COLOR_UNKNOWN;
}

/* ---------- public sensor API ---------- */
void color_sensor_init(void) {
    switchbox_set_pin(PIN_S0, SWB_GPIO);
    switchbox_set_pin(PIN_S1, SWB_GPIO);
    switchbox_set_pin(PIN_S2, SWB_GPIO);
    switchbox_set_pin(PIN_S3, SWB_GPIO);
    switchbox_set_pin(PIN_OUT, SWB_GPIO);

    gpio_set_direction(PIN_S0, GPIO_DIR_OUTPUT);
    gpio_set_direction(PIN_S1, GPIO_DIR_OUTPUT);
    gpio_set_direction(PIN_S2, GPIO_DIR_OUTPUT);
    gpio_set_direction(PIN_S3, GPIO_DIR_OUTPUT);
    gpio_set_direction(PIN_OUT, GPIO_DIR_INPUT);

    set_scaling_2();
    set_filter(FILTER_CLEAR);
}

color_result_t color_sensor_read(void) {
    color_result_t result;
    result.raw = read_raw_rgbc();
    result.color = guess_color(result.raw);
    /* DEBUG: raw channel periods (us) for calibrating new colors like brown */
    printf("RAW  R=%u  G=%u  B=%u  C=%u\n",
           result.raw.red_us, result.raw.green_us,
           result.raw.blue_us, result.raw.clear_us);
    return result;
}

/* Brown / cardboard reads a different color almost every time, while real cubes
 * read the same color consistently. Sample several times and only trust a color
 * if at least COLOR_VOTE_REQUIRED of the reads agree; otherwise call it UNKNOWN
 * so the caller treats it as brown (sidestep and retry). */
static color_t color_sensor_read_voted(void) {
    int votes[COLOR_BLUE + 1] = {0};   /* color_t values run 0..COLOR_BLUE */

    for (int i = 0; i < COLOR_VOTE_SAMPLES; i++) {
        color_t c = color_sensor_read().color;
        votes[c]++;
    }

    color_t best = COLOR_UNKNOWN;
    int best_votes = 0;
    for (int c = 0; c <= COLOR_BLUE; c++) {
        if (votes[c] > best_votes) { best_votes = votes[c]; best = (color_t)c; }
    }

    if (best != COLOR_UNKNOWN && best_votes >= COLOR_VOTE_REQUIRED) {
        return best;
    }
    return COLOR_UNKNOWN;   /* inconsistent -> treat as brown/unknown */
}

bool distance_sensor_init(void) {
    switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
    switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
    iic_init(IIC0);

    sleep_msec(100);   // let the I2C bus + sensor settle after power-up

    /* the VL53L0X can miss the first ping(s) right after power; retry a few times */
    int tries = 0;
    while (tofPing(IIC0, TOF_ADDR) != 0) {
        if (++tries >= 10) {
            printf("VL53L0X ping failed\n");
            return false;
        }
        sleep_msec(100);
    }

    if (tofInit(&tof_sensor, IIC0, TOF_ADDR, 0) != 0) {
        printf("VL53L0X init failed\n");
        return false;
    }
    printf("VL53L0X ready\n");
    return true;
}


uint32_t getDistance(void) {
    return tofReadDistance(&tof_sensor);
}

/* median of 3 reads -> rejects the occasional VL53L0X glitch/stale value that
 * would otherwise abort a sweep on a single bad sample */
static uint32_t getDistanceStable(void) {
    uint32_t a = getDistance();
    uint32_t b = getDistance();
    uint32_t c = getDistance();
    if (a > b) { uint32_t t = a; a = b; b = t; }
    if (b > c) { uint32_t t = b; b = c; c = t; }
    if (a > b) { uint32_t t = a; a = b; b = t; }
    return b;
}

void infrared_init(void) {
    switchbox_set_pin(PIN_IR, SWB_GPIO);
    gpio_set_direction(PIN_IR, GPIO_DIR_INPUT);
}

int infrared(void) {
    return gpio_get_level(PIN_IR) == IR_BLACK_LEVEL;  // 1 = black, 0 = not black
}

/* ---------- temperature sensor (NTC thermistor on ADC0) ---------- */
#define TEMP_BETA   4050.0    /* thermistor beta coefficient */
#define TEMP_V_REF  3.3       /* ADC reference voltage */
#define TEMP_R2     1000.0    /* 1k fixed resistor */
#define TEMP_R0     10000.0   /* 10k NTC nominal resistance */
#define TEMP_T0     298.15    /* 25C in Kelvin */

/* convert thermistor resistance (ohms) to temperature (Celsius) */
double r_to_t(double r_t) {
    double T = 1.0 / ((1.0 / TEMP_T0) + (1.0 / TEMP_BETA) * log(r_t / TEMP_R0));
    return T - 273.15;
}

/* read ADC0, convert the divider voltage to resistance, then to Celsius.
 * Returns -999.0 on a bad / zero reading. */
double read_temperature(void) {
    double v_out = adc_read_channel(ADC0);
    if (v_out <= 0.0) return -999.0;
    double r_t = (TEMP_V_REF - v_out) * TEMP_R2 / v_out;
    return r_to_t(r_t);
}

/* ---------- movement helpers ---------- */
void stepper_steps_pos(int x, float *pos, float *angle);
void stepper_steps_dir(int x, int y, float *angle);
void posup(int steps, float *pos, float *angle);
void dirup(int x, int y, float *angle);

static void wait_until_done(void) {
    while (!stepper_steps_done()) {
        sleep_msec(50);
    }
}

void forward(uint16_t speed, int steps, float *pos, float *angle) {
    stepper_set_speed(speed, speed);
    stepper_steps_pos(steps, pos, angle);
    wait_until_done();
}

/* turn 90 degrees to the left in place (left wheel +, right wheel -) */
void turn_left_90(float *angle) {
    stepper_set_speed(TURN_SPEED, TURN_SPEED);
    stepper_steps_dir(STEPS_90, -STEPS_90, angle);
    wait_until_done();
}

/* turn 90 degrees to the right in place (left wheel -, right wheel +) */
void turn_right_90(float *angle) {
    stepper_set_speed(TURN_SPEED, TURN_SPEED);
    stepper_steps_dir(-STEPS_90, STEPS_90, angle);
    wait_until_done();
}

/* drive forward a given distance in centimeters */
void forward_cm(int cm, float *pos, float *angle) {
    forward(DRIVE_SPEED, cm * STEPS_PER_CM, pos, angle);
}

/* Turn around an obstacle (the arena boundary or a mountain): swing 90, step
 * forward, swing 90 the same way. *direction (0=left, 1=right) picks the side
 * and alternates each call. Used for both IR-boundary and mountain handling so
 * they behave identically. */
static void avoid_obstacle(float *pos, float *angle, int *direction) {
    if (*direction == 0) {
        turn_left_90(angle);
        forward_cm(3, pos, angle);
        turn_left_90(angle);
    } else {
        turn_right_90(angle);
        forward_cm(3, pos, angle);
        turn_right_90(angle);
    }
    *direction = (*direction + 1) % 2;
}

void stepper_steps_pos(int x, float *pos, float *angle) {
    stepper_steps(x, x);
    posup(x, pos, angle);
}

void stepper_steps_dir(int x, int y, float *angle) {
    stepper_steps(x, y);
    dirup(x, y, angle);
}

/* ---------- mapping ---------- */

void posup(int steps, float *pos, float *angle){ //function for updating x and y coordinates
    pos[0]+=((float)steps/STEPS_PER_CM)*cos(*angle);
    pos[1]+=((float)steps/STEPS_PER_CM)*sin(*angle);
    // printf("Coordinates: (%f, %f) \n", pos[0], pos[1]);
}

void dirup(int x, int y, float *angle){ //function for updating the direction
    if (x>y){ //left
        *angle+=radperstep*abs(x);
    } else { //right
        *angle-=radperstep*abs(x);
    }
    printf("Angle: %f \n", *angle);
}

/* Single-char report code for a classified cube.
 * Upper = large, lower = small. Blue uses E/e because B/b are taken by black.
 *   red   R/r     green G/g     white W/w     black B/b     blue E/e
 *   '?' = object of unknown/off color (e.g. cardboard)   'N' = nothing in range */
static char cube_code(color_t color, bool large) {
    switch (color) {
        case COLOR_RED:   return large ? 'R' : 'r';
        case COLOR_GREEN: return large ? 'G' : 'g';
        case COLOR_WHITE: return large ? 'W' : 'w';
        case COLOR_BLACK: return large ? 'B' : 'b';
        case COLOR_BLUE:  return large ? 'E' : 'e';
        default:          return '?';
    }
}

/* Human-readable label for a report code (for printing / logging). */
static const char *cube_to_string(char code) {
    switch (code) {
        case 'R': return "large RED cube";
        case 'r': return "small RED cube";
        case 'G': return "large GREEN cube";
        case 'g': return "small GREEN cube";
        case 'W': return "large WHITE cube";
        case 'w': return "small WHITE cube";
        case 'B': return "large BLACK cube";
        case 'b': return "small BLACK cube";
        case 'E': return "large BLUE cube";
        case 'e': return "small BLUE cube";
        case 'M': return "mountain";
        case 'N': return "nothing in range";
        case '?': return "unknown object";
        default:  return "unrecognized";
    }
}

char detection_cube(float *angle){
    int speed = 3075;
    printf("detecting\n");

    if (getDistance() >= 45) {
        return 'N';   /* nothing close enough in front */
    }

    /* ---- 1. read color while still centered on the cube ---- */
    /* voted read: inconsistent (brown/cardboard) comes back UNKNOWN -> '?' */
    color_t color = color_sensor_read_voted();
    latest_color_result.color = color;

    /* center distance is the most reliable (sensor points straight at face) */
    uint32_t center_dist = getDistanceStable();
    uint32_t lost = center_dist + CUBE_LOST_MARGIN_MM;

    /* ---- 2. sweep LEFT (left wheel +, right wheel -) until the edge drops off ---- */
    int steps_l = 0;
    stepper_set_speed(speed, speed);
    while (getDistanceStable() < lost && steps_l < MAX_SWEEP_STEPS) {
        stepper_steps_dir(SWEEP_STEP_CHUNK, -SWEEP_STEP_CHUNK, angle);
        wait_until_done();
        steps_l += SWEEP_STEP_CHUNK;
    }
    /* return to center, then let the ToF settle before the next sweep */
    stepper_steps_dir(-steps_l, steps_l, angle);
    wait_until_done();
    delay_us(50000);

    /* ---- 3. sweep RIGHT until the edge drops off ---- */
    int steps_r = 0;
    while (getDistanceStable() < lost && steps_r < MAX_SWEEP_STEPS) {
        stepper_steps_dir(-SWEEP_STEP_CHUNK, SWEEP_STEP_CHUNK, angle);
        wait_until_done();
        steps_r += SWEEP_STEP_CHUNK;
    }
    /* return to center */
    stepper_steps_dir(steps_r, -steps_r, angle);
    wait_until_done();

    /* ---- 4. width = arc subtended by the cube at its distance ---- */
    int total_steps = steps_l + steps_r;
    float angular_width = (float)total_steps * RAD_PER_TURN_STEP;  /* radians */
    float width_mm = (float)center_dist * angular_width;           /* chord ~ arc */

    /* Too wide to be a cube -> it's the cardboard "mountain". Either the sweep
     * never found an edge (maxed out) or the measured width exceeds any cube.
     * Brown cardboard is optically identical to white to this sensor, so size is
     * the only reliable way to tell them apart. */
    if (steps_l >= MAX_SWEEP_STEPS || steps_r >= MAX_SWEEP_STEPS ||
        width_mm >= MOUNTAIN_WIDTH_MM) {
        printf("width %.1f mm (steps L=%d R=%d) -> mountain, code 'M'\n",
               width_mm, steps_l, steps_r);
        return 'M';
    }

    bool large = width_mm >= SIZE_THRESHOLD_MM;
    char code = cube_code(color, large);

    printf("detected %s (code '%c')  center=%u mm  steps L=%d R=%d  width=%.1f mm\n",
           cube_to_string(code), code, center_dist, steps_l, steps_r, width_mm);

    return code;
}

void sendmap(float *position, char *input, double temperature)
{
    static int uart_ready = 0;

    if (position == NULL || input == NULL)
    {
        fprintf(stderr, "sendmap error: NULL pointer\n");
        return;
    }

    float xcell = position[0];
    float ycell = position[1];
    char block_char = *input;

    if (!uart_ready)
    {
        switchbox_set_pin(IO_AR0, SWB_UART0_RX);
        switchbox_set_pin(IO_AR1, SWB_UART0_TX);

        uart_init(UART0);
        uart_reset_fifos(UART0);

        uart_ready = 1;
    }

    char buffer[200];

    /* w
       MQTT payload.
       This sends xcell, ycell, the char input, and the cube temperature (C).
       Example:
       4.0,7.0,R,23.5
    */
    snprintf(
        buffer,
        sizeof(buffer),
        "%f,%f,%c,%f",
        xcell,
        ycell,
        block_char,
        temperature
    );

    uint32_t len = strlen(buffer);

    uart_send(UART0, len & 0xFF);
    uart_send(UART0, (len >> 8) & 0xFF);
    uart_send(UART0, (len >> 16) & 0xFF);
    uart_send(UART0, (len >> 24) & 0xFF);

    for (uint32_t i = 0; i < len; i++)
    {
        uart_send(UART0, buffer[i]);
    }

    fprintf(stderr, "Sent map MQTT payload: %s", buffer);
    fflush(stderr);
}

/* ---------- main ---------- */
int main(void) {
    pynq_init();
    stepper_init();
    stepper_enable();
    float angle_first=0.0;
    float *angle=&angle_first;
    float pos_first[]={0.0, 0.0};
    float *pos=pos_first;
    char cube;

    setbuf(stdout, NULL);
    adc_init();
    color_sensor_init();
    infrared_init();
    
    if (!distance_sensor_init()) {
        printf("aborting: distance sensor unavailable\n");
        iic_destroy(IIC0);
        adc_destroy();
        stepper_destroy();
        pynq_destroy();
        return 1;
    }
    printf("robot starting\n");

    int speed = 3075;

    int direction = 0; // 0 = left, 1 = right (for boundary avoidance)

    while (1) {
        // latest_color_result = color_sensor_read();

        stepper_set_speed(speed, speed);
        stepper_steps_pos(10, pos, angle);


        if (infrared()) {            // IR sensor sees black
            wait_until_done();
            printf("boundary detected, turning around it\n");
            avoid_obstacle(pos, angle, &direction);
            continue;                // resume driving forward
        }
        
        if (getDistance() < STOP_DISTANCE_MM) {   // obstacle ahead
        wait_until_done();
        cube = detection_cube(angle);

            // read the cube's temperature while it's still in front of the
            // sensor, before we maneuver away from it.
            double cube_temp = read_temperature();
            printf("cube temperature: %.1f C\n", cube_temp);

            // mountain: report it, then turn around it exactly like the IR
            // boundary (same avoid_obstacle maneuver), then resume.
            if (cube == 'M') {
                sendmap(pos_first, &cube, cube_temp);
                printf("mountain reported, turning around it\n");
                avoid_obstacle(pos, angle, &direction);
                continue;
            }

            // real cube: report to the computer, then go around it
            sendmap(pos_first, &cube, cube_temp);
            printf("obstacle detected: %s (code '%c')\n",
                   cube_to_string(cube), cube);

            // steer around the obstacle so we don't re-detect it in place
            turn_left_90(angle);
            forward_cm(5, pos, angle);
            turn_right_90(angle);
            forward_cm(7, pos, angle);
            turn_right_90(angle);
            forward_cm(5, pos, angle);
            turn_left_90(angle);
        }
    }

    iic_destroy(IIC0);
    adc_destroy();
    stepper_destroy();
    pynq_destroy();
    return 0;
}



 
