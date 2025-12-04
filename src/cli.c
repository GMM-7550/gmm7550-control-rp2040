#include "pico/bootrom.h"
#include "FreeRTOS.h"
#include "gmm7550_control.h"
#include "tusb.h"

#define CMD_MAX_SIZE 40

#include "FreeRTOS_CLI.h"

static bool was_disconnected;

static inline uint8_t cdc_getc(const uint cdc)
{
  uint8_t c;
  while(1) {
    if ((cli_connected = tud_cdc_n_connected(cdc))) {
      if (tud_cdc_n_available(cdc)) {
        tud_cdc_n_read(cdc, &c, 1);
        return c;
      }
    } else {
      was_disconnected = true;
    }
    vTaskDelay(1);
  }
}

static inline void cdc_putc(const uint cdc, const uint8_t c)
{
  while(!tud_cdc_n_write_available(cdc)) vTaskDelay(1);
  tud_cdc_n_write_char(cdc, c);
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
  p = cmd;
  while(*p && (*p == ' ')) p++; /* skip leading spaces */
  return p;
}

#define getc()  cdc_getc(CDC_CLI)
#define putc(c) cdc_putc(CDC_CLI, (c))
#define puts(s) cdc_puts(CDC_CLI, (s))
#define get_line() cdc_get_line(CDC_CLI)

static BaseType_t cli_bootsel(char *pcWriteBuffer,
                              size_t xWriteBufferLen,
                              const char *pcCmd)
{
  uint8_t c;
  puts("\n\n*** Reboot to BOOTSEL mode ***\nAre you sure? [y/N]? ");
  c = getc();
  if ((c == 'y') || (c == 'Y')) {
    putc(c); putc('\r'); putc('\n');
    vTaskDelay(500 / portTICK_PERIOD_MS);
    rom_reset_usb_boot(0, 0);
  }
  *pcWriteBuffer = '\0';
  return pdFALSE;
}

static const CLI_Command_Definition_t bootsel_cmd = {
  "bootsel",
  "bootsel\n  Reboot RP2040 into BOOTSEL mode (uf2 firmware update)\n\n",
  cli_bootsel,
  0
};

void process_command(const uint8_t* cmd)
{
  BaseType_t ret;
  uint8_t *out = FreeRTOS_CLIGetOutputBuffer();

  do {
    ret = FreeRTOS_CLIProcessCommand(cmd, out,
                                     configCOMMAND_INT_MAX_OUTPUT_SIZE);
    puts(out);
  } while(pdTRUE == ret);
}

void cli_task(__unused void *params)
{
  uint8_t *l;
  was_disconnected = true;

  cli_register_gpio();
  cli_register_i2c();
  FreeRTOS_CLIRegisterCommand(&bootsel_cmd);

  while(1) {
    if (was_disconnected) {
      puts("\nGMM-7550 Control CLI");
      was_disconnected = false;
    }
    puts("\n> ");
    l = get_line();
    puts("\n");
    if (!was_disconnected && l) process_command(l);
  }
}
