#include <libpynq.h>
#include <platform.h>
#include <stepper.h>
#include "shared.h"
#define NORMAL_SPEED 12000

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

    for (int s = *speed_ptr; s <= 18000; s += 1000) {
        set_speed(speed_ptr, s);
        stepper_steps(160, 160);
    }
    wait_until_done();
}

void turn_left(void){
    stepper_set_speed(20000, 20000);
    stepper_steps(625, -625);
    dirup(-1) //update direction
    wait_until_done();
}

void turn_right(void){
    stepper_set_speed(20000, 20000);
    stepper_steps(-625, 625);
    dirup(1) //update direction
    wait_until_done();
}

void spin(void){

    stepper_set_speed(25000, 25000);
    stepper_steps(2500, -2500);
    wait_until_done();

}
void 180(void){

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

    stepper_set_speed(speed_ptr, speed_ptr);
    stepper_steps(steps, steps);

    posup((steps/1600)*7);
    
}


void obstacle(void){

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
        180();
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

    int speed = 13500;

    forward(&speed, 1600);
    deceleration(&speed);

    stepper_destroy();
    pynq_destroy();

    return 0;
}

