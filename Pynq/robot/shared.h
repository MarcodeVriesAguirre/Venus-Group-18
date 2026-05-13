#include <pthread.h>

#ifndef SHARED_H
#define SHARED_H

typedef struct {
    int distanceD;
    int colorD;
    int temperatureD;
    int algorithmD;
    int movementD;
    int running; //The condition for it to be running
    pthread_mutex_t lock;
} sdata;

typedef struct{ //variables for position tracking
    int dir;
    int x;
    int y;
    pthread_mutex_t lock;
} robpos;

extern robpos global_state;

#endif