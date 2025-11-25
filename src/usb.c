
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
    uint8_t buf[64];
    uint32_t count;
    tud_task();
    if (tud_cdc_connected()) {
      while (tud_cdc_available()) {
        count = tud_cdc_read(buf, sizeof(buf));
        for(int i=0; i<count; i++) {
          (void) xQueueSend(serial_txQueue, (void *) &buf[i], 0);
        }
      }
      if ((count = uxQueueMessagesWaiting(serial_rxQueue))) {
        for (int i=0; i<count; i++) {
          (void) xQueueReceive(serial_rxQueue, (void *) &buf[i], 0);
        }
        tud_cdc_write(buf, count);
      }
      tud_cdc_write_flush();
    }
    vTaskDelay(1);
  }
}
