#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "shared.h"

extern void* distanceR(void *arg); // Tells he compiler that the files are elsewhere for these functions.
extern void* colorR(void *arg);
extern void* temperatureR(void *arg);
extern void* algorithmR(void *arg);
extern void* actuationR(void *arg);


#define gridSize 150
#define blockSize 3

bool IsItJulius; //Julius is yes,no is Javier
int turn; //side its turning

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
    realPos=round(round(pos)/blockSize)+(gridSize/blockSize);
    return realPos;
}

void sendmap(xcell, ycell)
{


}

void detectCell(color, distance, width)
{
    //call this function when an obstacle is detected
    //detects which obstacle it is
    int cellNum;
    switch(color)
    {
        case 000://unknown
            cellNum=12; //hill
        break;
        case 001://no object
        break;
        case 010://black
            if (distance</*placeholder*/ && width</*placeholder*/)
            {
                cellNum=4; //small black cube
            } 
            else if (distance</*placeholder*/ && width>/*placeholder*/)
            {
                cellNum=9; //big black cube (BBC)
            }
            else if (distance>/*placeholder*/)
            {
                cellNum=11; //hole/border
            }
        break;
        case 011://white
            if (width</*placeholder*/)
            {
                cellNum=5; //small white cube
            } 
            else if (width>/*placeholder*/)
            {
                cellNum=10; //big white cube
            }
        break;
        case 100://red
            if (width</*placeholder*/)
            {
                cellNum=1; //small red cube
            } 
            else if (width>/*placeholder*/)
            {
                cellNum=6; //big red cube
            }
        break;
        case 101://green
            if (width</*placeholder*/)
            {
                cellNum=2; //small green cube
            } 
            else if (width>/*placeholder*/)
            {
                cellNum=7; //big green cube
            }
        break;
        case 110://blue
            if (width</*placeholder*/)
            {
                cellNum=3; //small blue cube
            } 
            else if (width>/*placeholder*/)
            {
                cellNum=8; //big blue cube
            }
        break;
        case 111://unused
        break;
        
        int xcell = coordTranslate(global_state.x); //gets the current position on the grid
        int ycell = coordTranslate(global_state.y);

        grid[xcell][ycell]=cellNum; //updates the grid
        sendmap(xcell, ycell); //calls the function for sending data to computer
    }

}


int main(){

sdata shared= {0}; //initializes the data in my struct
pthread_mutex_init(&shared.lock, NULL); // turns on the mutex (prevents threads from crashing)

pthread_t distance, temperature, color, algorithm, actuation;

pthread_create(&distance, NULL, distanceR, &shared); //creating the threads
pthread_create(&color, NULL, colorR, &shared);
pthread_create(&temperature, NULL, temperatureR, &shared);
pthread_create(&algorithm, NULL, algorithmR, &shared);
pthread_create(&actuation, NULL, actuationR, &shared);

pthread_join(distance, NULL); //waits for them all to start running
pthread_join(color, NULL);
pthread_join(temperature, NULL);
pthread_join(actuation, NULL);

    int blocks = 0;

    //main while - stops after all blocks are seen
    while(blocks < 5)
{        //get to the corners to start ROOMBA - PREROOMBA
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


pthread_mutex_destroy(&shared.lock);
printf("system stopped");
return 0;
}