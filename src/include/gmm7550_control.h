/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025, 2026 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "task.h"

#ifndef _GMM7550_CONTROL_H_
#define _GMM7550_CONTROL_H_

#define GMM7550_CONTROL_VERSION "0.6.2"

#define GREEN_LED_PIN 0 /* GPIO 25 on Pico-based prototype, 0 on a final h/w */

#define GMM7550_UART 0
#define GMM7550_UART_TX_PIN 12
#define GMM7550_UART_RX_PIN 13
#define GMM7550_UART_DEFAULT_BAUDRATE 115200

/* main.c */

/* serial.c */
#include "tusb.h"

#define SERIAL_BUFFER_SIZE 64
#define GMM7550_UART_READ_WAIT_US 5000
extern void serial_init(void *params);
extern void serial_set_line_coding(cdc_line_coding_t const* p_line_coding);
extern void serial_putc(const uint8_t c);
extern int  serial_getc_us(void);

/* usb.c */
#define CDC_SERIAL 0
#define CDC_CLI    1
#define CDC_SPI    2

extern void usb_task(void *params);

/* cli.c */
extern void cli_task(void *params);
extern volatile bool cli_connected;

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
extern bool i2c_gpio_initialized;

/* spi.c */
#define GMM7550_SPI           1
#define GMM7550_SPI_MOSI_PIN 11
#define GMM7550_SPI_MISO_PIN  8
#define GMM7550_SPI_SCK_PIN  10
#define GMM7550_SPI_NCS_PIN   9
extern void gmm7550_spi_init(void);
extern volatile bool spi_connected;

#endif
