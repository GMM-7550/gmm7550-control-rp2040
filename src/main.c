/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "gmm7550_control.h"

static uint32_t blink_interval;

void set_blink_interval_ms(uint32_t ms)
{
  blink_interval = ms / portTICK_PERIOD_MS;
}

void blink_task(__unused void *params)
{
  gpio_init(GREEN_LED_PIN);
  gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

  while(1) {
    gpio_put(GREEN_LED_PIN, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_put(GREEN_LED_PIN, 0);
    vTaskDelay(blink_interval);
  }
}

int main(void)
{
  set_blink_interval_ms(1900);
  serial_init(NULL);

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
