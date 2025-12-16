#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "gmm7550_control.h"

static uart_inst_t *uart = UART_INSTANCE(GMM7550_UART);

inline void serial_putc(const uint8_t c)
{
  uart_putc(uart, c);
}

inline int serial_getc_us(void)
{
  /* if (uart_is_readable_within_us(uart, GMM7550_UART_READ_WAIT_US)) { */
  if (uart_is_readable(uart)) {
    return uart_getc(uart);
  } else {
    return -1;
  }
}

void serial_set_line_coding(cdc_line_coding_t const* p)
{
  uart_tx_wait_blocking(uart);
  (void) uart_set_baudrate(uart, p->bit_rate);
  uart_set_format(uart, p->data_bits, p->stop_bits, p->parity);
}

void serial_init(__unused void *params)
{
  gpio_set_function(GMM7550_UART_TX_PIN,
                    UART_FUNCSEL_NUM(uart, GMM7550_UART_TX_PIN));

  gpio_set_function(GMM7550_UART_RX_PIN,
                    UART_FUNCSEL_NUM(uart, GMM7550_UART_RX_PIN));

  uart_init(uart, GMM7550_UART_DEFAULT_BAUDRATE);
  uart_set_fifo_enabled(uart, true);
  /*
  uart_set_translate_crlf(uart, true);
  */
}
