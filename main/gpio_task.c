#include "gpio_task.h"

void vPulseRoomb(void *p) {
  gpio_config_t io_conf;
  //disable interrupt
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  //disable pull-down mode
  io_conf.pull_down_en = 0;
  //enable pull-up mode
  io_conf.pull_up_en = 0;
  //configure GPIO with the given settings
  if (gpio_config(&io_conf) != ESP_OK) {
  	vTaskDelete(NULL);
  	return;
  }

  for (;;) {
  	gpio_set_level(GPIO_OUTPUT_IO_0, 0);
  	vTaskDelay(pdMS_TO_TICKS(1000));
  	gpio_set_level(GPIO_OUTPUT_IO_0, 1);
  	vTaskDelay(pdMS_TO_TICKS(60000));
  }

  vTaskDelete(NULL);
}