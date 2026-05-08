#include <libpynq.h>
#include <platform.h>
#include <stepper.h>

void wait_until_done(void) {
    while (!stepper_steps_done()) {
        sleep_msec(50);
    }
}

void deceleration(int speed){

    for(int s = speed; s <=18000; s -=2000){
        stepper_set_speed(s, s);
        stepper_steps(320,320);
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

void look_around(void){

    stepper_set_speed(30000, 30000);
    stepper_steps(312, -312);
    wait_until_done();

    stepper_set_speed(30000, 30000);
    stepper_steps(-625, 625);
    wait_until_done();

    stepper_set_speed(30000, 30000);
    stepper_steps(312, -312);
    wait_until_done();

}

void forward(int speed, int steps){

    stepper_set_speed(speed, speed);
    stepper_steps(steps, steps);

    

    wait_until_done();
}

void obstacle


int main(void) {

    

    pynq_init();
    
    stepper_init();
    stepper_enable();


    

    stepper_destroy();
    pynq_destroy();

    return 0;
}

