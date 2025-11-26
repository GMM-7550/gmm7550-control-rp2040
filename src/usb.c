
#include "FreeRTOS.h"
#include "tusb.h"
#include "usb.h"

#include "serial.h"

static void usb_init(__unused void *params)
{
  tusb_rhport_init_t dev_init = {
    .role  = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(0, &dev_init);
}

void cli_task(__unused void *params)
{
  uint8_t buf[64];
  uint32_t count;

  while(1) {
    if (tud_cdc_n_connected(1)) {
      while (tud_cdc_n_available(1)) {
        count = tud_cdc_n_read(1, buf, sizeof(buf));
        tud_cdc_n_write(1, buf, count);
      }
      tud_cdc_n_write_flush(1);
    }
    vTaskDelay(1);
  }
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
    if (tud_cdc_n_connected(0)) {
      while (tud_cdc_n_available(0)) {
        count = tud_cdc_n_read(0, buf, sizeof(buf));
        for(int i=0; i<count; i++) {
          (void) xQueueSend(serial_txQueue, (void *) &buf[i], 0);
        }
      }
      if ((count = uxQueueMessagesWaiting(serial_rxQueue))) {
        for (int i=0; i<count; i++) {
          (void) xQueueReceive(serial_rxQueue, (void *) &buf[i], 0);
        }
        tud_cdc_n_write(0, buf, count);
      }
      tud_cdc_n_write_flush(0);
    }
    vTaskDelay(1);
  }
}
