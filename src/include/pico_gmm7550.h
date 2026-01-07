/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025, 2026 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _BOARDS_PICO_GMM7550_H
#define _BOARDS_PICO_GMM7550_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

#define PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (2 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#endif
