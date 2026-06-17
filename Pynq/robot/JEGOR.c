







#include <libpynq.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <platform.h>
#include <stepper.h>



#define PIN_S0   IO_AR4
#define PIN_S1   IO_AR5
#define PIN_S2   IO_AR6
#define PIN_S3   IO_AR7
#define PIN_OUT  IO_AR8

#define SETTLE_US        3000
#define TIMEOUT_US       100000
#define SAMPLES_PER_CH   5

/* Calibration values - adjust later if needed */
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

/* Final color result for the rest of the robot */
typedef enum {
    COLOR_UNKNOWN   = 0, /* 000 */
    COLOR_NO_OBJECT = 1, /* 001 */
    COLOR_BLACK     = 2, /* 010 */
    COLOR_WHITE     = 3, /* 011 */
    COLOR_RED       = 4, /* 100 */
    COLOR_GREEN     = 5, /* 101 */
    COLOR_BLUE      = 6  /* 110 */
} color_t;

typedef struct {
    raw_reading_t raw;
    rgb_t rgb;
    color_t color;
} color_result_t;

/* Shared result variable for other submodules */
color_result_t latest_color_result;
[20.05.2026 16:39] Jegor: static uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000ULL);
}

static void delay_us(uint32_t us) {
    uint64_t start = now_us();
    while ((now_us() - start) < us) {
        /* busy wait */
    }
}

static bool wait_for_level(io_t pin, gpio_level_t target, uint32_t timeout_us) {
    uint64_t start = now_us();

    while ((now_us() - start) < timeout_us) {
        if (gpio_get_level(pin) == target) {
            return true;
        }
    }
    return false;
}

/* Set output scaling to 20% */
static void set_scaling_20(void) {
    gpio_set_level(PIN_S0, GPIO_LEVEL_HIGH);
    gpio_set_level(PIN_S1, GPIO_LEVEL_LOW);
    delay_us(SETTLE_US);
}

/* Select the active photodiode group */
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

/* Measure LOW pulse width on OUT */
static uint32_t pulse_in_low_us(io_t pin, uint32_t timeout_us) {
    uint64_t t0, t1;

    if (!wait_for_level(pin, GPIO_LEVEL_HIGH, timeout_us)) return 0;
    if (!wait_for_level(pin, GPIO_LEVEL_LOW, timeout_us)) return 0;

    t0 = now_us();

    if (!wait_for_level(pin, GPIO_LEVEL_HIGH, timeout_us)) return 0;

    t1 = now_us();

    return (uint32_t)(t1 - t0);
}

/* Read one channel and average several samples */
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

/* Map pulse width to 0..255 */
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

/* Same color logic as before, now returning an enum */
static color_t guess_color(raw_reading_t raw, rgb_t rgb) {
    if (raw.red_us == 0  raw.green_us == 0  raw.blue_us == 0 || raw.clear_us == 0) {
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

    if (max_rgb < 50) {
        return COLOR_BLACK;
    }

    if (max_rgb > 140 && min_rgb > 90 && (max_rgb - min_rgb) < 90) {
        return COLOR_WHITE;
    }
[20.05.2026 16:39] Jegor: /* Smaller pulse width = stronger response */
    if (raw.red_us + RAW_DOM_MARGIN_US < raw.green_us &&
        raw.red_us + RAW_DOM_MARGIN_US < raw.blue_us) {
        return COLOR_RED;
    }

    if (raw.green_us + RAW_DOM_MARGIN_US < raw.red_us &&
        raw.green_us + RAW_DOM_MARGIN_US < raw.blue_us) {
        return COLOR_GREEN;
    }

    if (raw.blue_us + RAW_DOM_MARGIN_US < raw.red_us &&
        raw.blue_us + RAW_DOM_MARGIN_US < raw.green_us) {
        return COLOR_BLUE;
    }

    return COLOR_UNKNOWN;
}

const char *color_to_string(color_t color) {
    switch (color) {
        case COLOR_NO_OBJECT: return "NO OBJECT";
        case COLOR_BLACK:     return "BLACK";
        case COLOR_WHITE:     return "WHITE";
        case COLOR_RED:       return "RED";
        case COLOR_GREEN:     return "GREEN";
        case COLOR_BLUE:      return "BLUE";
        default:              return "UNKNOWN";
    }
}

/* This is the code you can send over UART/MQTT as one byte */
uint8_t color_to_code(color_t color) {
    return (uint8_t)color;
}

void color_sensor_init(void) {
    pynq_init();

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

/* This is the main function the rest of the robot can call */
color_result_t color_sensor_read(void) {
    color_result_t result;

    result.raw = read_raw_rgbc();
    result.rgb = raw_to_rgb(result.raw);
    result.color = guess_color(result.raw, result.rgb);

    return result;
}

void wait_until_done(void) {
    while (!stepper_steps_done()) {
        sleep_msec(50);
    }
}
void set_speed(int *speed_ptr, int new_speed) {
    *speed_ptr = new_speed;
    stepper_set_speed(*speed_ptr, *speed_ptr);
}

void deceleration(int *speed_ptr){
    int increase = 1000;
    for (int s = *speed_ptr; s <= 18000; s += increase) {
        set_speed(speed_ptr, s);
        stepper_steps(400, 400);
        sleep_msec(200);
        increase = increase + 200;
    }
    wait_until_done();
}

void turn_left(void){

    stepper_set_speed(20000, 20000);
    stepper_steps(625, -625);

    
    wait_until_done();
}

void turn_right(void){

    stepper_set_speed(20000, 20000);
    stepper_steps(-625, 625);

  
    wait_until_done();
}

void spin(void){

    stepper_set_speed(25000, 25000);
    stepper_steps(2500, -2500);
    wait_until_done();

}
void turn(void){

    stepper_set_speed(25000, 25000);
    stepper_steps(1250, -1250);
    wait_until_done();

}

void look_around(void){

    stepper_set_speed(30000, 30000);
    stepper_steps(400, -400);
    wait_until_done();

    stepper_set_speed(30000, 30000);
    stepper_steps(-800, 800);
    wait_until_done();

    stepper_set_speed(30000, 30000);
    stepper_steps(400, -400);
    wait_until_done();

}

void forward(int *speed_ptr, int steps){

    stepper_set_speed(*speed_ptr, *speed_ptr);
    stepper_steps(steps, steps);

    
}


void obstacle(void){
    int distance = 60;
    turn_left();
    look_around();

    if(distance >> 30){
        while(1){
            forward(15000, 400);
            turn_right();

            if(distance >> 30){
                return;
            }
            turn_left();
        }
    }

    if(distance << 30){
        turn();
        if(distance >> 30){
            while(1){
                forward(15000, 400);
                turn_left();

                if(distance >> 30){
                    return;
                }
                turn_right();
            }
        }
        if(distance << 30){
            turn_right();
            return;
        }
    }
}


int main(void) {

    

    pynq_init();
    
    stepper_init();
    stepper_enable();

   int speed =15000;
    
    int a = 0;
    while(a < 5){

    
    for(int i =0; i < 6; i++){
        forward(&speed, 1600);
        a = a+1;
    }

    } 
    while(1){
    stepper_set_speed(speed, speed);
    stepper_steps(400, 400);
    
    }
    

    stepper_destroy();
    pynq_destroy();

    return 0;
}

int main(void) {
    pynq_init();
    stepper_init();
    stepper_enable();
    printf("Starting TCS3200 program...\n");
    fflush(stdout);

    int speed =3075;

    color_sensor_init();

    printf("TCS3200 test started\n");
    printf("Pins: S0=AR4 S1=AR5 S2=AR6 S3=AR7 OUT=AR8\n");
    printf("20%% scaling, OE tied to GND\n");
    printf("NO_OBJECT threshold (clear): %d us\n", NO_OBJECT_CLEAR_US);
    printf("RAW dominance margin: %d us\n\n", RAW_DOM_MARGIN_US);
    fflush(stdout);

    while (1) {
        latest_color_result = color_sensor_read();


        fflush(stdout);

        stepper_set_speed(speed, speed);
        stepper_steps(10, 10);
        
        if(latest_color_result.color != COLOR_WHITE && latest_color_result.color != COLOR_UNKNOWN){
            wait_until_done();
            stepper_set_speed(10000, 10000);
            stepper_steps(0, -1250);
            wait_until_done();
            break;
        }
            
    
    }

    stepper_destroy();
    pynq_destroy();
    return 0;
}

        stepper_set_speed(speed, speed);
        stepper_steps(10, 10);

        if(black tape){
            wait_until_done();
        }
        if(distance < 30){

        }
        
        char m_or_c(void){
            if(latest_color_result.color == COLOR_UNKNOWN || latest_color_result.color == COLOR_WHITE){
                return M;
            }
            
            else{
                rerturn c;
            }
        }



