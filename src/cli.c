#include "FreeRTOS.h"
#include "gmm7550_control.h"
#include "tusb.h"

void cli_task(__unused void *params)
{
  uint8_t buf[64];
  uint32_t count;

  while(1) {
    if (tud_cdc_n_connected(CDC_CLI)) {
      set_blink_interval_ms(BLINK_INTERVAL_CLI_CONNECTED);
      while (tud_cdc_n_available(CDC_CLI)) {
        count = tud_cdc_n_read(CDC_CLI, buf, sizeof(buf));
        tud_cdc_n_write(CDC_CLI, buf, count);
      }
      tud_cdc_n_write_flush(CDC_CLI);
    } else {
      set_blink_interval_ms(BLINK_INTERVAL_DEFAULT);
    }
    vTaskDelay(1);
  }
}
