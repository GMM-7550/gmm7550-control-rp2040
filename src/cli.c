#include "FreeRTOS.h"
#include "gmm7550_control.h"
#include "tusb.h"

#define CMD_MAX_SIZE 10

static inline uint8_t cdc_getc(const uint cdc)
{
  uint8_t c;
  while(!tud_cdc_n_available(cdc)) {
    if (tud_cdc_n_connected(cdc)) {
      set_blink_interval_ms(BLINK_INTERVAL_CLI_CONNECTED);
    } else {
      set_blink_interval_ms(BLINK_INTERVAL_DEFAULT);
    }
    vTaskDelay(1);
  }
  tud_cdc_n_read(cdc, &c, 1);
  return c;
}

static inline void cdc_putc(const uint cdc, const uint8_t c)
{
  uint8_t b = c;
  tud_cdc_n_write(cdc, &b, 1);
  tud_cdc_n_write_flush(cdc);
}

static void cdc_puts(const uint cdc, uint8_t *s)
{
  uint8_t *p = s;
  uint8_t c;

  while ((c = *p++)) {
    cdc_putc(cdc, c);
    if (c == '\n') cdc_putc(cdc, '\r');
  }
}

static uint8_t *cdc_get_line(const uint cdc)
{
  static uint8_t cmd[CMD_MAX_SIZE];
  uint cnt=0;
  uint8_t c;
  uint8_t *p = cmd;

  while((c=cdc_getc(cdc)) != '\r') {
    if (c >= ' ' && c <= '~') {
      if (cnt < CMD_MAX_SIZE-1) {
        cdc_putc(cdc, *p++ = c);
        cnt++;
      }
    } else {
      if ((c == '\b' || c == 0x7f) && cnt > 0) {
        cnt--; p--;
        cdc_puts(cdc, "\b \b");
      }
    }
  }
  *p = '\0';
  return cmd;
}

#define getc()  cdc_getc(CDC_CLI)
#define putc(c) cdc_putc(CDC_CLI, (c))
#define puts(s) cdc_puts(CDC_CLI, (s))
#define get_line() cdc_get_line(CDC_CLI)

void process_command(const uint8_t* cmd)
{
}

void cli_task(__unused void *params)
{
  uint8_t *l;
  while(1) {
    puts("\n> ");
    l = get_line();
    puts("\n- ");
    puts(l);
   }
}
