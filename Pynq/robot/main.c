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
}

int coordTranslate(pos)
{
}

void sendmap(xcell, ycell) 
{


}

void detectCell(color, distance, width)
{
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