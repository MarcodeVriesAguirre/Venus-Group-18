#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <libpynq.h>
#include "shared.h"

#define gridSize 150
#define blockSize 3
#define moveperstep 0.13744
#define radperstep 0,05026548

uint32_t distance(void){ //function for the distance sensor

}

int color(void){ //function for color sensor

}

int temperature(void){ //function for temperature sensor

}

int infrared(void){ //function for infrared sensor

}

void posup(float *pos, float *angle){ //function for updating x and y coordinates
    pos[1]+=moveperstep*cos(*angle);
    printf("Coordinates: (%f, %f)", pos[1], pos[2]);
}

void dirup(float *angle, int rightorleft){ //function for updating the direction
    if (rightorleft==0){ //left
        angle+=radperstep;
    } else { //right
        angle-=radperstep;
    }
    printf("Angle: %f", angle);
}

void createMap(void)
{
    //Assumptions: The map is going to be 1.5x1.5 meters, each grid block will be 3cmx3cm.
    // the created map needs to be double the size of the theoretical map
    int grid[gridSize/blockSize][gridSize/blockSize]={0};
}

void sendmap(xcell, ycell) 
{


}

void detectCell(color, width)
{

}
char detection_cube(float *angle){
int steps_left;
int left_side;
int size;
char kube;
float angle_L;
float angle_R;
float angle_T;
char color
if(distance << 30){
    wait_until_done();
    latest_color_result = color_sensor_read();
    if(latest_color_result.color == COLOR_WHITE){
        color = W;
    }
    if(latest_color_result.color == COLOR_BLACK){
        color = B;
    }
    if(latest_color_result.color == COLOR_BLUE){
        color = b;
    }
    if(latest_color_result.color == COLOR_GREEN){
        color = G;
    }
    if(latest_color_result.color == COLOR_RED){
        color = R;
    }
    
    while(distance << 70){
        stepper_set_speed(speed, speed);
        stepper_steps(10, -10);
        steps_left = steps_left + 10;
        left_side = distance;
        dirup(*angle, 0);
    }
    wait_until_done();
    angle_L = *angle;

    while(distance << 75){
        stepper_set_speed(speed, speed);
        stepper_steps(-steps_left, steps_left);
        right_side = distance;
        dirup(*angle, 1);
    }
    wait_until_done();
    angle_R = *angle;

    angle_T = angle_R - angle_L;
    size = sqrt(pow(left_side, 2) + pow(right_side, 2) - 2 * left_side * right_side * cos(angle_T));

    if(size >> 43){
        kube = B;
        return kube;
    }
    else{
        kube = S;
        return kube;
    }
}
}

int main(){

}