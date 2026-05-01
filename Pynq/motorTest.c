/**
 * robotMovement.c
 * Calibration constants:
 *   - 1600 steps  = 1 full wheel rotation
 *   -  625 steps differential = ~90 degrees rotation
 *   - 1280 steps differential = ~180 degrees rotation
 *
 * Speed value is the time (in microseconds units) between step pulses.
 */

#include <stdio.h>
#include <stdint.h>
#include <libpynq.h>
#include <stepper.h>

/* ---- Movement tuning constants ---- */
#define DEFAULT_SPEED       10000   /* time between pulses */
#define STEPS_PER_FORWARD    800    /* one "tile" / forward unit */
#define STEPS_90_DEG         625    /* differential steps for 90 deg turn */
#define STEPS_180_DEG       1280    /* differential steps for 180 deg turn */

/* ---- Rotation directions ---- */
typedef enum {
    ROTATE_LEFT  = 0,
    ROTATE_RIGHT = 1,
    ROTATE_180   = 2
} rotate_dir_t;

/**
 * Block until the current stepper command is finished.
 */

static void wait_until_done(void) {
    int timeout_ms = 10000;  // 10 giây tối đa
    while (!stepper_steps_done() && timeout_ms > 0) {
        sleep_msec(10);
        timeout_ms -= 10;
    }
    sleep_msec(300);  // buffer thêm 300ms cho motor settle hẳn
}

/**
 * Move the robot forward by STEPS_PER_FORWARD steps.
 * Both wheels turn the same direction at the same speed -> straight line.
 */
void moveForward(void) {
    printf("[robot] move forward\n");
    stepper_set_speed(DEFAULT_SPEED, DEFAULT_SPEED);
    stepper_steps(STEPS_PER_FORWARD, STEPS_PER_FORWARD);
    wait_until_done();
}

void moveBackward(void) {
    printf("[robot] move backward\n");
    stepper_set_speed(DEFAULT_SPEED, DEFAULT_SPEED);
    stepper_steps(-STEPS_PER_FORWARD, -STEPS_PER_FORWARD);
    wait_until_done();
}

/**
 * Rotate the robot in place.
 * @param dir  ROTATE_LEFT, ROTATE_RIGHT, or ROTATE_180
 *
 * To rotate, wheels turn in opposite directions.
 */
void rotate(rotate_dir_t dir) {
    stepper_set_speed(DEFAULT_SPEED, DEFAULT_SPEED);

    switch (dir) {
        case ROTATE_RIGHT:
            printf("[robot] rotate right 90\n");
            stepper_steps(STEPS_90_DEG, -STEPS_90_DEG);
            break;

        case ROTATE_LEFT:
            printf("[robot] rotate left 90\n");
            stepper_steps(-STEPS_90_DEG, +STEPS_90_DEG);
            break;

        case ROTATE_180:
            printf("[robot] rotate 180\n");
            stepper_steps(-STEPS_180_DEG, STEPS_180_DEG);
            break;

        default:
            printf("[robot] unknown rotation direction\n");
            return;
    }

    wait_until_done();
}

/**
 * Demo / test main: shows each movement once.
 */
int main(void) {
    /* Init PYNQ + stepper hardware */
    pynq_init();
    stepper_init();
    stepper_enable();

    printf("=== Robot movement demo ===\n");

    moveForward();
    moveBackward();
    rotate(ROTATE_RIGHT);
    rotate(ROTATE_LEFT);
    rotate(ROTATE_180);

    /* Cleanup */
    stepper_disable();
    stepper_destroy();
    pynq_destroy();

    printf("=== Done ===\n");
    return 0;
}
