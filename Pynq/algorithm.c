#include "shared.h"
#include <stdio.h>
#include <unistd.h>

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