#include <pthread.h>

#ifndef SHARED_H
#define SHARED_H

typedef struct {
    int distance;
    int color;
    int temperature;
    int mapping;
    int algorithm;
    pthread_mutex_t lock;
} sdata;

#endif