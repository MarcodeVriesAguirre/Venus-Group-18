#include "shared.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <libpynq.h>
#include <stepper.h>
#include <iic.h>
#include "vl53l0x.h"

/* ---------- distance sensor ---------- */
#define TOF_ADDR 0x29
#define STOP_DISTANCE_MM 50
#define NORMAL_SPEED 12000

/* ---------- turn / drive calibration ---------- */
#define TURN_SPEED     20000
#define STEPS_90        625   /* steps for a 90-deg turn (spin=2500 steps=360) */
#define DRIVE_SPEED    15000
#define STEPS_PER_CM   229    /* 1600 steps = 7 cm  ->  ~228.6 steps/cm */

vl53x tof_sensor;

/* ---------- color sensor (TCS3200) ---------- */
#define PIN_S0   IO_AR4
#define PIN_S1   IO_AR5
#define PIN_S2   IO_AR6
#define PIN_S3   IO_AR7
#define PIN_OUT  IO_AR8

/* ---------- infrared line sensor (digital, from Infrared/main.c) ---------- */
#define PIN_IR          IO_AR9
#define IR_BLACK_LEVEL  GPIO_LEVEL_HIGH   /* flip to GPIO_LEVEL_LOW if it reads inverted */

#define SETTLE_US        3000
#define TIMEOUT_US       100000
#define SAMPLES_PER_CH   5

#define R_MIN 55
#define R_MAX 90
#define G_MIN 100
#define G_MAX 180
#define B_MIN 100
#define B_MAX 180

#define NO_OBJECT_CLEAR_US 120
#define RAW_DOM_MARGIN_US 8

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

typedef struct {
    int r;
    int g;
    int b;
} rgb_t;

typedef enum {
    COLOR_UNKNOWN   = 0,
    COLOR_NO_OBJECT = 1,
    COLOR_BLACK     = 2,
    COLOR_WHITE     = 3,
    COLOR_RED       = 4,
    COLOR_GREEN     = 5,
    COLOR_BLUE      = 6
} color_t;

typedef struct {
    raw_reading_t raw;
    rgb_t rgb;
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
static void set_scaling_20(void) {
    gpio_set_level(PIN_S0, GPIO_LEVEL_HIGH);
    gpio_set_level(PIN_S1, GPIO_LEVEL_LOW);
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

static int map_to_255(uint32_t x, uint32_t in_min, uint32_t in_max) {
    if (in_max <= in_min) return 0;
    if (x < in_min) x = in_min;
    if (x > in_max) x = in_max;
    long y = 255L - ((long)(x - in_min) * 255L) / (long)(in_max - in_min);
    if (y < 0) y = 0;
    if (y > 255) y = 255;
    return (int)y;
}

static rgb_t raw_to_rgb(raw_reading_t d) {
    rgb_t rgb;
    rgb.r = map_to_255(d.red_us,   R_MIN, R_MAX);
    rgb.g = map_to_255(d.green_us, G_MIN, G_MAX);
    rgb.b = map_to_255(d.blue_us,  B_MIN, B_MAX);
    return rgb;
}

static color_t guess_color(raw_reading_t raw, rgb_t rgb) {
    if (raw.red_us == 0 || raw.green_us == 0 || raw.blue_us == 0 || raw.clear_us == 0) {
        return COLOR_UNKNOWN;
    }
    if (raw.clear_us > NO_OBJECT_CLEAR_US) {
        return COLOR_NO_OBJECT;
    }

    int max_rgb = rgb.r;
    if (rgb.g > max_rgb) max_rgb = rgb.g;
    if (rgb.b > max_rgb) max_rgb = rgb.b;

    int min_rgb = rgb.r;
    if (rgb.g < min_rgb) min_rgb = rgb.g;
    if (rgb.b < min_rgb) min_rgb = rgb.b;

    if (max_rgb < 50) return COLOR_BLACK;
    if (max_rgb > 140 && min_rgb > 90 && (max_rgb - min_rgb) < 90) return COLOR_WHITE;

    if (raw.red_us + RAW_DOM_MARGIN_US < raw.green_us &&
        raw.red_us + RAW_DOM_MARGIN_US < raw.blue_us) return COLOR_RED;
    if (raw.green_us + RAW_DOM_MARGIN_US < raw.red_us &&
        raw.green_us + RAW_DOM_MARGIN_US < raw.blue_us) return COLOR_GREEN;
    if (raw.blue_us + RAW_DOM_MARGIN_US < raw.red_us &&
        raw.blue_us + RAW_DOM_MARGIN_US < raw.green_us) return COLOR_BLUE;

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

    set_scaling_20();
    set_filter(FILTER_CLEAR);
}

color_result_t color_sensor_read(void) {
    color_result_t result;
    result.raw = read_raw_rgbc();
    result.rgb = raw_to_rgb(result.raw);
    result.color = guess_color(result.raw, result.rgb);
    return result;
}

bool distance_sensor_init(void) {
    switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
    switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
    iic_init(IIC0);

    if (tofPing(IIC0, TOF_ADDR) != 0) {
        printf("VL53L0X ping failed\n");
        return false;
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

void infrared_init(void) {
    switchbox_set_pin(PIN_IR, SWB_GPIO);
    gpio_set_direction(PIN_IR, GPIO_DIR_INPUT);
}

int infrared(void) {
    return gpio_get_level(PIN_IR) == IR_BLACK_LEVEL;  // 1 = black, 0 = not black
}

/* ---------- pose updates ---------- */
void posup(int d) {
    pthread_mutex_lock(&global_state.lock);
    switch (global_state.dir) {
        case 1: global_state.y += d; break;
        case 2: global_state.x += d; break;
        case 3: global_state.y -= d; break;
        case 4: global_state.x -= d; break;
    }
    pthread_mutex_unlock(&global_state.lock);
}

void dirup(int turn) {
    pthread_mutex_lock(&global_state.lock);
    if (global_state.dir == 4 && turn == 1) {
        global_state.dir = 1;
    } else if (global_state.dir == 1 && turn == -1) {
        global_state.dir = 4;
    } else {
        global_state.dir += turn;
    }
    pthread_mutex_unlock(&global_state.lock);
}

/* ---------- movement helpers ---------- */
static void wait_until_done(void) {
    while (!stepper_steps_done()) {
        sleep_msec(50);
    }
}

void forward(uint16_t speed, int steps) {
    stepper_set_speed(speed, speed);
    stepper_steps(steps, steps);
    wait_until_done();
    posup((steps / 1600) * 7);
}

/* turn 90 degrees to the left in place (left wheel +, right wheel -) */
void turn_left_90(void) {
    stepper_set_speed(TURN_SPEED, TURN_SPEED);
    stepper_steps(STEPS_90, -STEPS_90);
    wait_until_done();
    dirup(-1);
}

/* drive forward a given distance in centimeters */
void forward_cm(int cm) {
    forward(DRIVE_SPEED, cm * STEPS_PER_CM);
}

/* ---------- main ---------- */
int main(void) {
    pynq_init();
    stepper_init();
    stepper_enable();

    setbuf(stdout, NULL);
    color_sensor_init();
    infrared_init();
    if (!distance_sensor_init()) {
        printf("aborting: distance sensor unavailable\n");
        iic_destroy(IIC0);
        stepper_destroy();
        pynq_destroy();
        return 1;
    }
    printf("robot starting\n");

    int speed = 3075;

    while (1) {
        // latest_color_result = color_sensor_read();

        stepper_set_speed(speed, speed);
        stepper_steps(10, 10);

        if (infrared()) {            // IR sensor sees black
            wait_until_done();
            printf("boundary detected, turning around it\n");
            turn_left_90();          // turn 90 deg left
            forward_cm(5);           // go forward 5 cm
            turn_left_90();          // turn 90 deg left again
            continue;                // resume driving forward
        }
        
        if (getDistance() < STOP_DISTANCE_MM) {   // safety: obstacle ahead
            wait_until_done();
            printf("obstacle detected, stopped\n");
            break;
        }
    }

    iic_destroy(IIC0);
    stepper_destroy();
    pynq_destroy();
    return 0;
}
