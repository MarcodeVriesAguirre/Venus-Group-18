#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "shared.h"

#define gridSize 150
#define blockSize 3

robpos global_state = {0, 0, 1, PTHREAD_MUTEX_INITIALIZER};

void posup(int distance){ //function for updating x and y coordinates

}

void dirup(int turn){ //function for updating the direction
}

void colorstop()
{
    //start moving and stop after color sensor send info
        while(1/*no info is sent from color sensor*/)
        {
            //move forward
        }

    //make sensors take info
}

void walkaound()
{
    //turn 90 deg.

    //walk a bit

    //turn and check if it's still there

    //if no continue PREROOMBA, if yes loop
}

void createMap(void)
{
    //Assumptions: The map is going to be 1.5x1.5 meters, each grid block will be 3cmx3cm.
    // the created map needs to be double the size of the theoretical map
    int grid[gridSize/blockSize][gridSize/blockSize]={0};
}

int coordTranslate(pos)
{
    //The grid is represented as a matrix with dimensions [gridSize/blockSize]x[gridSize/blockSize]
    //and the 0, 0 isn't going to be 0, 0 on the matrix. coordinates therefore need to be translated.
    int realPos; //this is the position translated to the matrix.
    realPos=round(round(pos)/blockSize)+(gridSize/blockSize);
    return realPos;
}

void sendmap(xcell, ycell) 
{


}

void detectCell(color, distance, width)
{
}


int main(){
    
}