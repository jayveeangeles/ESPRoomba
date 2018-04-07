#ifndef COAP_TASK_H
#define COAP_TASK_H

#include "globals.h"
#include <string.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "coap.h"

// #include "esp_heap_trace.h"

#define COAP_DEFAULT_TIME_SEC 	0
#define COAP_DEFAULT_TIME_USEC 	30000

void vCoapServer(void *p);

#endif