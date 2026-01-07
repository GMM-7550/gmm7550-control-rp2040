/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025, 2026 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

#include "pico/stdlib.h"
#include "gmm7550_control.h"

#define BLINK_ON_TIME 100
#define BLINK_INTERVAL_DEFAULT 2900
#define BLINK_INTERVAL_CLI_CONNECTED 900
#define BLINK_INTERVAL_SPI_CONNECTED 100

volatile bool cli_connected;
volatile bool spi_connected;

void blink_task(__unused void *params)
{
  uint32_t blink_interval;

  gpio_init(GREEN_LED_PIN);
  gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

  while(1) {
    gpio_put(GREEN_LED_PIN, 1);
    vTaskDelay(BLINK_ON_TIME / portTICK_PERIOD_MS);
    gpio_put(GREEN_LED_PIN, 0);
    if (spi_connected) {
      blink_interval = BLINK_INTERVAL_SPI_CONNECTED / portTICK_PERIOD_MS;
    } else if (cli_connected) {
      blink_interval = BLINK_INTERVAL_CLI_CONNECTED / portTICK_PERIOD_MS;
    } else {
      blink_interval = BLINK_INTERVAL_DEFAULT / portTICK_PERIOD_MS;
    }
    vTaskDelay(blink_interval);
  }
}

int main(void)
{
  cli_connected = false;
  spi_connected = false;

  serial_init(NULL);
  gmm7550_spi_init();

  xTaskCreate(blink_task, "Blink",
              configMINIMAL_STACK_SIZE, /* stack size */
              NULL,                     /* */
              (tskIDLE_PRIORITY + 1UL), /* priority */
              NULL                      /* */
              );
  xTaskCreate(usb_task, "USB",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 2UL),
              NULL
              );

  vTaskStartScheduler();
  return 0;
}
