#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "serial.h"

static uart_inst_t *uart = UART_INSTANCE(GMM7550_UART);
static QueueHandle_t rxQueue, txQueue;

void serial_rx_task()
{
  char c;
  while(1) {
    if(uart_is_readable(uart)) {
      c = uart_getc(uart);
      (void) xQueueSend(rxQueue, (void *) &c, 0);
    }
  }
}

void serial_tx_task()
{
  BaseType_t ret;
  char c;
  while(1) {
    ret = xQueueReceive(txQueue, (void *) &c, 0);
    if (pdPASS == ret) {
      uart_putc(uart, c);
    }
  }
}

void serial_init(__unused void *params)
{
  gpio_set_function(GMM7550_UART_TX_PIN,
                    UART_FUNCSEL_NUM(uart, GMM7550_UART_TX_PIN));

  gpio_set_function(GMM7550_UART_RX_PIN,
                    UART_FUNCSEL_NUM(uart, GMM7550_UART_RX_PIN));

  uart_init(uart, 115200);
  /*
  uart_set_fifo_enabled(uart, true);
  uart_set_translate_crlf(uart, true);
  */

  rxQueue = xQueueCreate(16, sizeof(char));
  txQueue = xQueueCreate(16, sizeof(char));
}

void serial_task(__unused void *params)
{
  BaseType_t ret;

  xTaskCreate(serial_rx_task, "UART Rx",
              configMINIMAL_STACK_SIZE, /* stack size */
              NULL,                     /* */
              (tskIDLE_PRIORITY + 1UL), /* priority */
              NULL                      /* */
              );

  xTaskCreate(serial_tx_task, "UART Tx",
              configMINIMAL_STACK_SIZE, /* stack size */
              NULL,                     /* */
              (tskIDLE_PRIORITY + 1UL), /* priority */
              NULL                      /* */
              );

  while(1) {
    char c;
    ret = xQueueReceive(rxQueue, (void *) &c, 0);
    if (pdPASS == ret) {
      xQueueSend(txQueue, (void *) &c, 0);
    }
  }
}
