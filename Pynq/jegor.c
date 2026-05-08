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

void spin(void){

    stepper_set_speed(10000, 10000);
    stepper_steps(1400, -1400);
    wait_until_done();

}

void look_around(void){

    stepper_set_speed(10000, 10000);
    stepper_steps(175, -175);
    wait_until_done();

}

void forward(void){
    int speed;

    stepper_set_speed(8000, 8000);
        stepper_steps(1600, 1600);

    for(speed = 8000; speed <= 18000; speed +=2000){
        stepper_set_speed(speed, speed);
        stepper_steps(200, 200);
    }

    wait_until_done();
    

}

int main(void) {
    int speed;
    int steps; 
    pynq_init();
    
    stepper_init();
    stepper_enable();

    forward();
    forward();

    stepper_destroy();
    pynq_destroy();

    return 0;
}
