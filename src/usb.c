#include "gmm7550_control.h"
#include "tusb.h"

static void usb_init(__unused void *params)
{
  tusb_rhport_init_t dev_init = {
    .role  = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(0, &dev_init);
}

static void auto_start(void)
{
  gmm7550_on();
  gmm7550_hreset(0);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gmm7550_i2c_gpio_init();
  gmm7550_sreset(1);
  pca_write_reg(3, 0x40); /* Connect UART Rx/Tx signals, Configuration Mode = 0 (SPI Active) */
  vTaskDelay(GMM7550_MR_TIME_MS / portTICK_PERIOD_MS);
  gmm7550_sreset(0);
}

void usb_task(__unused void *params)
{
  usb_init(NULL);
  static bool auto_start_done = false;

  xTaskCreate(cli_task, "CLI",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 2UL),
              NULL
              );

  while(1) {
    uint8_t buf[SERIAL_BUFFER_SIZE];
    uint32_t count;
    int c;

    tud_task();

    if (tud_cdc_n_connected(CDC_SERIAL)) {
      if (!cli_was_connected && !auto_start_done) {
        auto_start();
        auto_start_done = true;
      };
      if (tud_cdc_n_available(CDC_SERIAL)) {
        count = tud_cdc_n_read(CDC_SERIAL, buf, sizeof(buf));
        for(int i=0; i<count; i++) {
          serial_putc(buf[i]);
        }
      }
      count = 0;
      for (int i=0; i<SERIAL_BUFFER_SIZE; i++) {
        if ((c = serial_getc_us()) > 0) {
          buf[i] = c;
          count++;
        } else {
          break;
        }
      }

      if (count) {
        tud_cdc_n_write(CDC_SERIAL, buf, count);
        tud_cdc_n_write_flush(CDC_SERIAL);
      }
    }
    vTaskDelay(1);
  }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
  if (CDC_SERIAL == itf) {
    serial_set_line_coding(p_line_coding);
  } else if (CDC_SPI == itf) {
    gmm7550_spi_set_baudrate(p_line_coding->bit_rate);
  }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  if (CDC_SPI == itf) {
    gmm7550_spi_set_cs(rts);
  }
}
