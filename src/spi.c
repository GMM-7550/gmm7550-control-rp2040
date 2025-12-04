#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "gmm7550_control.h"

static spi_inst_t *spi = SPI_INSTANCE(GMM7550_SPI);

#define SPI_BUFFER_SIZE 32
static uint8_t spi_tx_buf[SPI_BUFFER_SIZE];
static uint8_t spi_rx_buf[SPI_BUFFER_SIZE];

static void spi_task()
{
  uint32_t len;

  while(1) {
    if ((spi_connected = tud_cdc_n_connected(CDC_SPI))) {
      gpio_put(GMM7550_SPI_NCS_PIN, 0);
      if(tud_cdc_n_available(CDC_SPI)) {
        len = tud_cdc_n_read(CDC_SPI, spi_tx_buf, SPI_BUFFER_SIZE);
        if (len) {
          spi_write_read_blocking(spi, spi_tx_buf, spi_rx_buf, len);
          tud_cdc_n_write(CDC_SPI, spi_rx_buf, len);
          tud_cdc_n_write_flush(CDC_SPI);
        }
      }
    } else {
      gpio_put(GMM7550_SPI_NCS_PIN, 1);
    }
    vTaskDelay(1);
  }
}

void gmm7550_spi_init(void)
{
  spi_init(spi, 20*1000*1000);

  gpio_set_function(GMM7550_SPI_MISO_PIN, GPIO_FUNC_SPI);
  gpio_set_function(GMM7550_SPI_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(GMM7550_SPI_SCK_PIN,  GPIO_FUNC_SPI);

  gpio_init(GMM7550_SPI_NCS_PIN);
  gpio_set_dir(GMM7550_SPI_NCS_PIN, GPIO_OUT);
  gpio_put(GMM7550_SPI_NCS_PIN, 1);

  xTaskCreate(spi_task, "SPI",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 2UL),
              NULL
              );
}
