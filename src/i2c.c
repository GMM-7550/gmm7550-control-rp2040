#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "gmm7550_control.h"

#include "string.h"
#include "hex.h"

static i2c_inst_t *i2c = I2C_INSTANCE(GMM7550_I2C);

static void gmm7550_i2c_init(void)
{
  i2c_init(i2c, 400000);
  gpio_set_function(GMM7550_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(GMM7550_I2C_SCL_PIN, GPIO_FUNC_I2C);
}

static BaseType_t cli_i2c_scan(char *pcWriteBuffer,
                               size_t xWriteBufferLen,
                               const char *pcCmd)
{
  static uint line = 0;
  char *p;
  uint8_t addr, byte;

  if (line == 0) {
    strncpy(pcWriteBuffer,
            "   x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF\n",
            xWriteBufferLen);
    line += 1;
    return pdTRUE;
  } else if (line <= 8) {
    p = pcWriteBuffer;
    *p++ = hex_digit(line - 1);
    *p++ = 'x';
    *p++ = ' ';
    for (int i=0; i<0x10; i++) {
      addr = (((line-1) << 4) + i);
      if (i2c_write_blocking(i2c, addr, &byte, 1, false) < 0) {
        *p++ = '-';
        *p++ = '-';
      } else {
        *p++ = hex_digit(line-1);
        *p++ = hex_digit(i);
      }
      *p++ = ' ';
    }
    *p++ = '\n';
    *p++ = '\0';
    if (line == 8) {
      line = 0;
      return pdFALSE;
    } else {
      line += 1;
      return pdTRUE;
    }
  }
}

static const CLI_Command_Definition_t scan_cmd = {
  "scan",
  "scan\n  Scan I2C bus for available devices\n\n",
  cli_i2c_scan,
  0
};

void cli_register_i2c(void)
{
  gmm7550_i2c_init();
  FreeRTOS_CLIRegisterCommand(&scan_cmd);
}
