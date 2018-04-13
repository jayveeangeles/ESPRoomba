/**
* @main.c
* @brief this file contains main application.
* Main application to handle COAP-controlled Roomba
*
* @author John Vincent Angeles
*
* @date 03/28/2018
*/

#include "globals.h"
#include "uart_task.h"
#include "coap_task.h"
#include "gpio_task.h"

/**
 * @brief callback function for WiFi events.
 * Handle WiFi events by executing callbacks
 *
 * @param ctx unused
 * @param event variable to handle WiFi events
 *
 * @return always return ESP_OK to indicate function returned properly
 */
static esp_err_t wifiEventHandler(void *ctx, system_event_t *event) {
  switch(event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;

  case SYSTEM_EVENT_STA_GOT_IP:
    xTaskNotify(  xCoapServerTask,    // task handle
                  CONNECTED_INTERNET, // event bit
                  eSetBits            // set bit
                );
    xTaskNotify( xUartHandleTask, CONNECTED_INTERNET, eSetBits );
    break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    /* This is a workaround as ESP32 WiFi libs don't currently
       auto-reassociate. */
    esp_wifi_connect();
    xTaskNotify( xCoapServerTask, NOT_CONNECTED_INET, eSetBits );
    xTaskNotify( xUartHandleTask, NOT_CONNECTED_INET, eSetBits );
    break;

  default:
    break;
  }
  return ESP_OK;
};

/**
 * @brief function to init WiFi connection.
 * Initialize WiFi connection using menuconfig
 * 
 */
static void wifiConnInit(void) {
  tcpip_adapter_init();
  ESP_ERROR_CHECK( esp_event_loop_init(wifiEventHandler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASS,
    },
  };
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK( esp_wifi_start() );
};

 
void app_main(void) {
  ESP_ERROR_CHECK( nvs_flash_init() );
  wifiConnInit();

  xCommandsQueue = xQueueCreate( 10, sizeof(char) * 20 );

  if( xCommandsQueue == NULL ) {
    return;
  }

  xTaskCreatePinnedToCore(vPulseRoomb, "pulse_room", 1024*2, NULL, tskIDLE_PRIORITY, &xGPIOPulseRoomb, 1);
  xTaskCreatePinnedToCore(vCoapServer, "start_coap", 1024*4, NULL, configMAX_PRIORITIES - 10, &xCoapServerTask, 0);
  xTaskCreatePinnedToCore(vUartHandle, "start_uart", 1024*4, NULL, configMAX_PRIORITIES - 10, &xUartHandleTask, 1);
}

