#ifndef GLOBALS_H
#define GLOBALS_H

#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_attr.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "coap.h"

#include "esp_log.h"

#include "lwip/err.h"
#include "apps/sntp/sntp.h"

#define WIFI_SSID           CONFIG_WIFI_SSID
#define WIFI_PASS          	CONFIG_WIFI_PASSWORD

#define RD_BUF_SIZE 				(256)

#define CONNECTED_INTERNET 	BIT0
#define NOT_CONNECTED_INET 	BIT1

// opcodes
#define RESET_OP								7
#define START_OP								128
#define	SAFE_OP									131
#define FULL_OP									132
#define	SPOT_OP									134
#define CLEAN_OP								135
#define MAX_OP									136
#define DRIVE_OP								137
#define SENSORS_OP							142
#define SEEK_DOCK_OP						143
#define DRIVE_DIRECT_OP					145
#define DRIVE_PWM_OP						146
#define STREAM_OP								148
#define PAUSE_RESUME_STREAM_OP	150
#define SCHEDULING_LED_OP				162
#define DIGITAL_LEDS_ASCII_OP		164
#define SCHEDULE_OP							167
#define SET_DAY_TIME_OP					168
#define STOP_OP									173

TaskHandle_t xCoapServerTask;
TaskHandle_t xUartHandleTask;
TaskHandle_t xRoombaReadTask;
TaskHandle_t xGPIOPulseRoomb;

QueueHandle_t xCommandsQueue;

extern char* sensorData;
// extern BaseType_t setDirtyBit;
extern struct coap_resource_t* globalResource;
extern PRIVILEGED_DATA portMUX_TYPE xTaskQueueMutex;

#endif