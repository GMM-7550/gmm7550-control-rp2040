/*
 * Raspberry Pi RP2040 Control firmware for GMM-7550 module
 * https://www.gmm7550.dev/doc/rp2040.html
 *
 * Copyright (c) 2025, 2026 Anton Kuzmin <ak@gmm7550.dev>
 *
 * SPDX-License-Identifier: MIT
 */

/* Dirty-JTAG implementation based on
 * https://github.com/phdussud/pico-dirtyJtag.git
 *
 * Copyright (c) 2020-2025 Patrick Dussud
 * Copyright (c) 2021 jeanthom
 * Copyright (c) 2023 David Williams (davidthings)
 * Copyright (c) 2023 Chandler Klüser
 * Copyright (c) 2024 DangerousPrototypes
 * Copyright (c) 2024 DESKTOP-M9CCUTI\ian
 */

#include "pico/stdlib.h"
#include "tusb.h"
#include "pio_jtag.h"
#include "gmm7550_control.h"

#include "cmd.h"

pio_jtag_inst_t jtag = {
  .pio = pio0,
  .sm = 0
};

static void djtag_init(void)
{
  gpio_set_function_masked((1<<GMM7550_JTAG_TDI_PIN) |
                           (1<<GMM7550_JTAG_TCK_PIN) |
                           (1<<GMM7550_JTAG_TDO_PIN) |
                           (1<<GMM7550_JTAG_TMS_PIN),
                           GPIO_FUNC_PIO0);
  init_jtag(&jtag, 1000,
            GMM7550_JTAG_TCK_PIN,
            GMM7550_JTAG_TDI_PIN,
            GMM7550_JTAG_TDO_PIN,
            GMM7550_JTAG_TMS_PIN,
            255, 255);
}

typedef uint8_t cmd_buffer[64];
static cmd_buffer tx_buf;

static uint wr_buffer_number = 0;
static uint rd_buffer_number = 0;

typedef struct buffer_info
{
  volatile uint8_t count;
  volatile uint8_t busy;
  cmd_buffer buffer;
} buffer_info;

#define N_BUFFERS (4)
buffer_info buffer_infos[N_BUFFERS];

static void jtag_task(__unused void *params)
{
  while(1) {
    if ((buffer_infos[wr_buffer_number].busy == false)) {
      //If tud_task() is called and tud_vendor_read isn't called immediately (i.e before calling tud_task again)
      //after there is data available, there is a risk that data from 2 BULK OUT transaction will be (partially) combined into one
      //The DJTAG protocol does not tolerate this.
      tud_task();// tinyusb device task
      if (tud_vendor_available()) {
        uint bnum = wr_buffer_number;
        uint count = tud_vendor_read(buffer_infos[wr_buffer_number].buffer, 64);
        if (count != 0) {
          buffer_infos[bnum].count = count;
          buffer_infos[bnum].busy = true;
          wr_buffer_number = wr_buffer_number + 1; //switch buffer
          if (wr_buffer_number == N_BUFFERS) {
            wr_buffer_number = 0;
          }
        }
      }
    }
    if (buffer_infos[rd_buffer_number].busy) {
      cmd_handle(&jtag, buffer_infos[rd_buffer_number].buffer, buffer_infos[rd_buffer_number].count, tx_buf);
      buffer_infos[rd_buffer_number].busy = false;
      rd_buffer_number++; //switch buffer
      if (rd_buffer_number == N_BUFFERS) {
        rd_buffer_number = 0;
      }
    }
    vTaskDelay(1);
  }
}

//this is to work around the fact that tinyUSB does not handle setup request automatically
//Hence this boiler plate code
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  if (stage != CONTROL_STAGE_SETUP) return true;
  return false;
}

void gmm7550_jtag_init(void)
{
  djtag_init();

  xTaskCreate(jtag_task, "JTAG",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 3UL),
              NULL
              );
}
