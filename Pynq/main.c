#include <pthread.h>
#include "shared.h"

extern void* distanceR(void *arg); // Tells he compiler that the files are elsewhere for these functions.
extern void* colorR(void *arg);
extern void* temperatureR(void *arg);
extern void* algorithmR(void *arg);
extern void* actuationR(void *arg);

int main(){

sdata shared= {0}; //initializes the data in my struct
pthread_mutex_init(&shared.lock, NULL); // turns on the mutex (prevents threads from crashing)

pthread_t distance, temperature, color, algorithm, actuation;

pthread_create(&distance, NULL, distanceR, &shared); //creating the threads
pthread_create(&color, NULL, colorR, &shared);
pthread_create(&temperature, NULL, temperatureR, &shared);
pthread_create(&algorithm, NULL, algorithmR, &shared);
pthread_create(&actuation, NULL, actuationR, &shared);

pthread_join(distance, NULL);
pthread_join(color, NULL);
pthread_join(temperature, NULL);
pthread_join(algorithm, NULL);
pthread_join(actuation, NULL);

pthread_mutex_destroy(&shared.lock);
printf("system stopped");
return 0;
}