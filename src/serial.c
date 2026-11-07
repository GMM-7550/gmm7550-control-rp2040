#include "pico/stdlib.h"
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
  /* CDC: stop_bits 0 = 1, 1 = 1.5, 2 = 2; parity 0 none, 1 odd, 2 even, 3 mark, 4 space */
  uint stop_bits = (p->stop_bits == 2) ? 2 : 1;
  uint data_bits = (p->data_bits >= 5 && p->data_bits <= 8) ? p->data_bits : 8;
  uart_parity_t parity = (p->parity == 1) ? UART_PARITY_ODD :
                         (p->parity == 2) ? UART_PARITY_EVEN : UART_PARITY_NONE;

  uart_tx_wait_blocking(uart);
  (void) uart_set_baudrate(uart, p->bit_rate);
  uart_set_format(uart, data_bits, stop_bits, parity);
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
