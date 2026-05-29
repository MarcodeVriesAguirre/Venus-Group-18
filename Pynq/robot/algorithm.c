#include "shared.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <libpynq.h>
#include <stepper.h>
#include <iic.h>
#include "vl53l0x.h"

#define TOF_ADDR 0x29
#define STOP_DISTANCE_MM 50  // 5 cm
#define NORMAL_SPEED 12000

vl53x tof_sensor;

robpos global_state = {0, 0, 1, PTHREAD_MUTEX_INITIALIZER};

uint32_t distance(void) {
    return tofReadDistance(&tof_sensor);
}

int infrared(void) {
    return 0;  // TODO: wire up IR sensor
}

void posup(int d) {
    pthread_mutex_lock(&global_state.lock);
    switch (global_state.dir) {
        case 1: global_state.y += d; break;  // north
        case 2: global_state.x += d; break;  // east
        case 3: global_state.y -= d; break;  // south
        case 4: global_state.x -= d; break;  // west
    }
    pthread_mutex_unlock(&global_state.lock);
}

void dirup(int turn) {
    pthread_mutex_lock(&global_state.lock);
    // turn=1 right, turn=-1 left
    if (global_state.dir == 4 && turn == 1) {
        global_state.dir = 1;
    } else if (global_state.dir == 1 && turn == -1) {
        global_state.dir = 4;
    } else {
        global_state.dir += turn;
    }
    pthread_mutex_unlock(&global_state.lock);
}

static void wait_until_done(void) {
    while (!stepper_steps_done()) {
        sleep_msec(50);
    }
}

static void forward(uint16_t speed, int steps) {
    stepper_set_speed(speed, speed);
    stepper_steps(steps, steps);
    wait_until_done();
    posup((steps / 1600) * 7);
}

int main(void) {
    pynq_init();
    stepper_init();
    stepper_enable();

    switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
    switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
    iic_init(IIC0);

    if (tofPing(IIC0, TOF_ADDR) != 0) {
        printf("VL53L0X ping failed\n");
        return 1;
    }
    if (tofInit(&tof_sensor, IIC0, TOF_ADDR, 0) != 0) {
        printf("VL53L0X init failed\n");
        return 1;
    }

    while (1) {
        uint32_t d = distance();
        printf("distance=%u mm\n", d);
        if (d <= STOP_DISTANCE_MM || infrared()) {
            print "obstacle detected, stopping\n");
        } else {
            forward(NORMAL_SPEED, 200);
        }
        
    }

    printf("stopped\n");
    iic_destroy(IIC0);
    stepper_destroy();
    pynq_destroy();
    return 0;
}
