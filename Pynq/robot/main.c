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
    return 
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

void detectCell(color, distance, width)
{

}

int main(){

}