#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "shared.h"

extern color_result_t latest_color_result;

void color_sensor_init(void);
color_result_t color_sensor_read(void);
const char *color_to_string(color_t color);
uint8_t color_to_code(color_t color);
void *color_sensor_thread(void *arg);

#endif
