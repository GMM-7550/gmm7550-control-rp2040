/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

#define GREEN_LED_PIN 25 /* GPIO 25 on Pico-based prototype, 0 on a final h/w */

#define GMM7550_UART 0
#define GMM7550_UART_TX_PIN 12
#define GMM7550_UART_RX_PIN 13

/* main.c */
#define BLINK_ON_TIME 100
#define BLINK_INTERVAL_DEFAULT 1900
#define BLINK_INTERVAL_CLI_CONNECTED 900

extern void set_blink_interval_ms(uint32_t ms);

/* serial.c */
#include "queue.h"
extern QueueHandle_t serial_rxQueue, serial_txQueue;
extern void serial_init(void *params);

/* usb.c */
#define CDC_SERIAL 0
#define CDC_CLI    1

extern void usb_task(void *params);

/* cli.c */
extern void cli_task(void *params);

/* gpio.c */
#define GMM7550_MR_PIN     15
#define GMM7550_EN_PIN      1
#define GMM7550_OFF_PIN    14

#define GMM7550_MR_TIME_MS 10
extern void cli_register_gpio(void);

/* i2c.c */
#define GMM7550_I2C         1
#define GMM7550_I2C_SDA_PIN 2
#define GMM7550_I2C_SCL_PIN 3
extern void cli_register_i2c(void);
