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

#include "serial.h"

void blink_task(__unused void *params)
{
  int on = 1;

  gpio_init(GREEN_LED_PIN);
  gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

  while(1) {
    gpio_put(GREEN_LED_PIN, on);
    on = 1 - on;
    sleep_ms(500);
  }
}

int main(void)
{
  xTaskCreate(blink_task, "Blink",
              configMINIMAL_STACK_SIZE, /* stack size */
              NULL,                     /* */
              (tskIDLE_PRIORITY + 1UL), /* priority */
              NULL                      /* */
              );
  xTaskCreate(serial_task, "UART",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 1UL),
              NULL
              );

  vTaskStartScheduler();
  return 0;
}
