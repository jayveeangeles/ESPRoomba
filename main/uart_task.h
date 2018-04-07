#ifndef UART_TASK_H
#define UART_TASK_H

#include "globals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "string.h"

#define BUF_SIZE (1024)

#define TXD_PIN (GPIO_NUM_17)   // native UART1 IO17
#define RXD_PIN (GPIO_NUM_16)   // native UART1 IO16

QueueHandle_t xRSensorsQueue;

void vUartHandle(void *p);
void vReadRoomba(void *p);

#endif