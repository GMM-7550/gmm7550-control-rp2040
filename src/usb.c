
#include "FreeRTOS.h"
#include "tusb.h"

#include "gmm7550_control.h"

static void usb_init(__unused void *params)
{
  tusb_rhport_init_t dev_init = {
    .role  = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(0, &dev_init);
}

void usb_task(__unused void *params)
{
  usb_init(NULL);

  xTaskCreate(cli_task, "CLI",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 2UL),
              NULL
              );

  while(1) {
    uint8_t buf[64];
    uint32_t count;
    tud_task();
    if (tud_cdc_n_connected(CDC_SERIAL)) {
      while (tud_cdc_n_available(CDC_SERIAL)) {
        count = tud_cdc_n_read(CDC_SERIAL, buf, sizeof(buf));
        for(int i=0; i<count; i++) {
          (void) xQueueSend(serial_txQueue, (void *) &buf[i], 0);
        }
      }
      if ((count = uxQueueMessagesWaiting(serial_rxQueue))) {
        for (int i=0; i<count; i++) {
          (void) xQueueReceive(serial_rxQueue, (void *) &buf[i], 0);
        }
        tud_cdc_n_write(CDC_SERIAL, buf, count);
      }
      tud_cdc_n_write_flush(CDC_SERIAL);
    }
    vTaskDelay(1);
  }
}
