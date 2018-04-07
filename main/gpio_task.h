#ifndef GPIO_TASK_H
#define GPIO_TASK_H

#include "globals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define GPIO_OUTPUT_IO_0    	GPIO_NUM_18
#define GPIO_OUTPUT_PIN_SEL  	(1ULL<<GPIO_OUTPUT_IO_0)

void vPulseRoomb(void *p);

#endif