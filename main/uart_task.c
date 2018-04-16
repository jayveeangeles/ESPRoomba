#include "uart_task.h"

static const char *UART_TAG = "UART_handler";
static const char *TX_TAG = "UART_TX";
static const char *RX_TAG = "UART_RX";
static const char *EVENT_TAG = "UART_EVENT";

// static PRIVILEGED_DATA portMUX_TYPE xTaskQueueMutex = portMUX_INITIALIZER_UNLOCKED;

static const char startOp[1] = { START_OP };
static const char stopOp[1] = { STOP_OP };
static const char safeOp[1] = { SAFE_OP };
static const char resetOp[1] = { RESET_OP };
static const char pauseStream[2] = { 150,  };
static const char streamOp[] = { 
  // STREAM_OP,  
  QUERY_LIST_OP,
  11, // number of data packets
  8,  // wall
  14, // wheels overcurrent
  15, // dirt detect
  19, // distance travelled
  22, // voltage
  23, // current
  24, // temp
  25, // battery charge
  26, // battery capacity
  34, // battery charge present?
  35  // OI mode
};

char* sensorData = NULL;
static const uint8_t ASCII_MAP[10] = {
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57
};

static BaseType_t _uartInit() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };
  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  return uart_driver_install(UART_NUM_2, RD_BUF_SIZE * 4, RD_BUF_SIZE * 4, 20, &xRSensorsQueue, 0);
};

static void _initSNTP(void) {
  ESP_LOGI(UART_TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();
};

static void _obtainTime(void) {
  _initSNTP();

  time_t now = 0;
  struct tm timeinfo = { 0 };
  int retry = 0;
  const int retry_count = 10;
  while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
    ESP_LOGI(UART_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(pdMS_TO_TICKS(1000));
    time(&now);
    localtime_r(&now, &timeinfo);
  }

  if (retry_count <= retry) {
    ESP_LOGW(UART_TAG, "cannot set date on roomba");
  } else {
    time_t now;
    struct tm ti;

    time(&now);

    setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &ti);

    ESP_LOGI(UART_TAG, "weekday->%d hour->%d minute->%d", ti.tm_wday, ti.tm_hour, ti.tm_min);

    const char dateTimeOp[] = { SET_DAY_TIME_OP, ti.tm_wday, ti.tm_hour, ti.tm_min };

    uart_write_bytes(UART_NUM_2, dateTimeOp, sizeof(dateTimeOp));
  }
};

void vDisplayBattPercentage(void *p) {
  uint16_t batteryCapacity = 0;
  uint32_t batteryCharge = 0;
  uint16_t quotient = 0;
  char battPercentageOp[] = {DIGITAL_LEDS_ASCII_OP, 45, 45, 45, 45};

  uart_write_bytes(UART_NUM_2, (const char *)battPercentageOp, sizeof(battPercentageOp));
  
  while (batteryCapacity == 0) {
    vTaskDelay(pdMS_TO_TICKS(100));
    batteryCapacity = ((uint8_t)sensorData[13] & 0xFF) | ((uint8_t)sensorData[12] << 8);
  }

  for(;;) {
    if (sensorData[15] > 1) {
      batteryCharge = (((uint8_t)sensorData[11] & 0xFF) | ((uint8_t)sensorData[10] << 8)) * 100;
      quotient = batteryCharge / batteryCapacity;

      if (quotient == 100) {
        quotient = 99;
      }

      battPercentageOp[1] = 32;
      battPercentageOp[2] = 32;
      battPercentageOp[3] = ASCII_MAP[quotient / 10];
      battPercentageOp[4] = ASCII_MAP[quotient % 10];

      uart_write_bytes(UART_NUM_2, (const char *)battPercentageOp, sizeof(battPercentageOp));
    }

    vTaskDelay(pdMS_TO_TICKS(20000));
  }

  vTaskDelete(NULL);
}

void vReadRoomba(void *p) {
  uart_event_t event;
  size_t buffered_size;
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(100);
  sensorData = (char *) pvPortMalloc(RD_BUF_SIZE);
  memset( sensorData, 0, RD_BUF_SIZE );

  for(;;) {
    uart_write_bytes(UART_NUM_2, streamOp, sizeof(streamOp));
	  //Waiting for UART event.
	  if(xQueueReceive( xRSensorsQueue, (void * )&event, pdMS_TO_TICKS(15))) {
      // ESP_LOGI(EVENT_TAG, "uart[%d] event:", UART_NUM_2);
      switch(event.type) {
        case UART_DATA:
          // uart_get_buffered_data_len(UART_NUM_2, &buffered_size);
          ESP_LOGI(EVENT_TAG, "data, len: %d;", event.size);
          uart_read_bytes(UART_NUM_2, (unsigned char*) sensorData, event.size, pdMS_TO_TICKS(15));
          // ESP_LOGI(RX_TAG, "[UART DATA]: %d opcode", sensorData[0]);

          vTaskEnterCritical(&xTaskQueueMutex);
          globalResource->dirty = pdTRUE;
          vTaskExitCritical(&xTaskQueueMutex);
          break;
        case UART_FIFO_OVF:
          ESP_LOGI(EVENT_TAG, "hw fifo overflow");
          uart_flush_input(UART_NUM_2);
          xQueueReset(xRSensorsQueue);
          break;
        case UART_BUFFER_FULL:
          ESP_LOGI(EVENT_TAG, "ring buffer full");
          uart_flush_input(UART_NUM_2);
          xQueueReset(xRSensorsQueue);
          break;
        case UART_BREAK:
          ESP_LOGI(EVENT_TAG, "uart rx break");
          break;
        case UART_PARITY_ERR:
          ESP_LOGI(EVENT_TAG, "uart parity error");
          break;
        case UART_FRAME_ERR:
          ESP_LOGI(EVENT_TAG, "uart frame error");
          break;
        default:
          ESP_LOGI(EVENT_TAG, "uart event type: %d", event.type);
          break;
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }

  free(sensorData);
  sensorData = NULL;
  vTaskDelete(NULL);
};

void vUartHandle(void *p) {
	uint32_t ulNotifiedValue;
	xTaskNotifyWait( 0x00,      			 /* Don't clear any notification bits on entry. */
	                 0x00,				 /* Reset the notification value to 0 on exit. */
	                 &ulNotifiedValue, /* Notified value pass out in
	                                      ulNotifiedValue. */
	                 pdMS_TO_TICKS(10000) );  /* Block for 10 seconds. */

	if (( ulNotifiedValue & NOT_CONNECTED_INET ) == 1) {
    ESP_LOGE(UART_TAG, "WiFi: Not connected!");
    vTaskDelete(NULL);
    return;
	}

	ESP_LOGI(UART_TAG, "Connected to AP");

	if (_uartInit() == ESP_FAIL) {
    ESP_LOGE(UART_TAG, "cannot install UART driver");
    vTaskDelete(NULL);
    return;
  }

  /* exit from OI mode */
  uart_write_bytes(UART_NUM_2, stopOp, 1);
  vTaskDelay(pdMS_TO_TICKS(30));

  /* put roomba in OI mode */
  uart_write_bytes(UART_NUM_2, startOp, 1);
  vTaskDelay(pdMS_TO_TICKS(30));

  /* ask roomba to start streaming */
  // uart_write_bytes(UART_NUM_2, streamOp, sizeof(streamOp));
  // vTaskDelay(pdMS_TO_TICKS(30));

  _obtainTime();

  xTaskCreatePinnedToCore(vReadRoomba, "read_roomb", 1024*4, NULL, configMAX_PRIORITIES, &xRoombaReadTask, 1);
  xTaskCreatePinnedToCore(vDisplayBattPercentage, "display_batt", 1024*2, NULL, tskIDLE_PRIORITY, NULL, 0);

	char *pxMessage = (char* )pvPortMalloc(20);
	const size_t len = 20;
	int8_t txBytes = -1;

	memset( pxMessage, 0, len );

  esp_log_level_set(TX_TAG, ESP_LOG_INFO);

  for (;;) {
  	if( xQueueReceive( xCommandsQueue, ( pxMessage ), pdMS_TO_TICKS(1000) ) ) {
      // ESP_LOGI(TX_TAG, "pxMessage->%d", pxMessage[0]);

      txBytes = uart_write_bytes(UART_NUM_2, (const char *)pxMessage, len);

  		while(txBytes == -1) {
        vTaskDelay(pdMS_TO_TICKS(60));
        txBytes = uart_write_bytes(UART_NUM_2, (const char *)pxMessage, len);
      }

      memset( pxMessage, 0, len );

  		// ESP_LOGI(TX_TAG, "Wrote %d bytes", txBytes);
  	}
  }

  free(pxMessage);

};