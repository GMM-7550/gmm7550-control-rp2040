
#include "FreeRTOS.h"
#include "tusb.h"
#include "usb.h"

#include "serial.h"

static void usb_init(__unused void *params)
{
  tusb_rhport_init_t dev_init = {
    .role  = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_FULL
  };
  tusb_init(0, &dev_init);
}

void usb_task(__unused void *params)
{
  usb_init(NULL);

  while(1) {
    tud_task();
    if (tud_cdc_connected()) {
      while (tud_cdc_available()) {
        uint8_t buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        tud_cdc_write(buf, count);
      }
      tud_cdc_write_flush();
    }
    vTaskDelay(1);
  }
}
