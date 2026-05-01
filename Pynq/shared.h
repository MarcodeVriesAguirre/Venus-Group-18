#include <pthread.h>

#ifndef SHARED_H
#define SHARED_H

typedef struct {
    int distanceD;
    int colorD;
    int temperatureD;
    int algorithmD;
    int actuatorD;
    int running; //The condition for it to be running
    pthread_mutex_t lock;
} sdata;

#endif