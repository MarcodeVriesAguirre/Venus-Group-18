#include "shared.h"
#include <stdio.h>
#include <unistd.h>

#define gridSize 150
#define blockSize 3

robpos global_state = {0, 0, 1, PTHREAD_MUTEX_INITIALIZER};

void posup(int distance){ //function for updating x and y coordinates
    //This function are called in movement file when told to move forwards or backwards
    pthread_mutex_lock(&global_state.lock);
    switch(global_state.dir){
        case 1: global_state.y += distance; break; //north 
        case 2: global_state.x += distance; break; //east
        case 3: global_state.y -= distance; break; //south
        case 4: global_state.x -= distance; break; //west
    }
    pthread_mutex_unlock(&global_state.lock);
}

void dirup(int turn){ //function for updating the direction
    //This function are called in movement file when told to turn left or right
    pthread_mutex_lock(&global_state.lock);
    //turn=1 for right
    //turn=-1 for left
    if (global_state.dir==4 && turn==1){
        global_state.dir = 1;
    } else if (global_state.dir==1 && turn==-1){
        global_state.dir = 4;
    } else {
        global_state.dir = (global_state.dir + turn);
    }
    pthread_mutex_unlock(&global_state.lock);
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
    realPos=pos+(gridSize/blockSize);
}

void updateMap(color, distance)
{
    
}

void sendmap()
{


}

int main()
{
    int blocks = 0;

    //main while - stops after all blocks are seen
    while(blocks < 5)
    {
        //get to the corners to start ROOMBA - PREROOMBA
        while(1/*PREROOMBA*/)
        {
            //walk forward
            colorstop();
            walkaround();
        }

        //send it to mapping
        sendmap();
    
        //turn 90 deg. left for the left one and right for the right one
    
        //walk a bit forward and check 
        
    }
    return 0;
}