/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

#if 0
#  define PICO_DEFAULT_UART 1
#  define PICO_DEFAULT_UART_TX_PIN 8
#  define PICO_DEFAULT_UART_RX_PIN 9
#endif

#define GREEN_LED_PIN 25 /* GPIO 25 on Pico-based prototype, 0 on a final h/w */

#define GMM7550_UART 0
#define GMM7550_UART_TX_PIN 12
#define GMM7550_UART_RX_PIN 13

#include <boards/pico.h>
