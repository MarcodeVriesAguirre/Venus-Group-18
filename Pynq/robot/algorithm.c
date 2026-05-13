#include "shared.h"
#include <stdio.h>
#include <unistd.h>
#include "jegor.c"

bool IsItJulius; //Julius is yes,no is Javier
int turn; //side its turning

typedef struct{
int dir;
int x;
int y;
pthread_mutex_t lock;
} robpos;



void posup(robpos *state, int distance){
    pthread_mutex_lock(&state->lock);
    switch(state->dir){
        case 1: state->y += distance; break; //north 
        case 2: state->x += distance; break; //east
        case 3: state->y -= distance; break; //south
        case 4: state->x -= distance; break; //west
    }
    pthread_mutex_unlock(&state->lock);
}

void dirup(robpos *state, int turn){
    pthread_mutex_lock(&state->lock);
    //turn=1 for right
    //turn=-1 for left
    if (state->dir==4 && turn==1){
        state->dir = 1;
    } else if (state->dir==1 && turn==-1){
        state->dir = 4;
    } else {
        state->dir = (state->dir + turn);
    }
    pthread_mutex_unlock(&state->lock);
}

void colorstop() //start moving and stop after color sensor send info
{
        while(1/*no info is sent from color sensor*/)
        {
            //move forward
        }

    //make sensors take info
}

void walkaound() // maybe obstacle and 
{


    //turn 90 deg.
    //walk a bit
    //turn and check if it's still there
    //WHAT DO I MEAN //if no continue PREROOMBA, if yes loop
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