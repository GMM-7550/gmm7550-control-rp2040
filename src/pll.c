#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gmm7550_control.h"
#include "FreeRTOS_CLI.h"

#include "string.h"
#include "hex.h"

#define CDCE6214_ADDR 0x68

/* I2C controller is already initialized in the i2c.c */
static i2c_inst_t *i2c = I2C_INSTANCE(GMM7550_I2C);

static inline void pll_write_reg(const uint16_t reg, const uint16_t data)
{
  uint8_t buf[4];
  buf[0] = reg >> 8;
  buf[1] = reg & 0xff;
  buf[2] = data >> 8;
  buf[3] = data & 0xff;
  (void) i2c_write_blocking(i2c, CDCE6214_ADDR, buf, 4, false);
}

static inline uint16_t pll_read_reg(const uint16_t reg)
{
  uint8_t tx[2];
  uint8_t rx[2] = {0};

  tx[0] = reg >> 8;
  tx[1] = reg & 0xff;

  if (i2c_write_blocking(i2c, CDCE6214_ADDR, tx, 2, true) == 2) {
    (void) i2c_read_blocking(i2c, CDCE6214_ADDR, rx, 2, false);
  }
  return (rx[0] << 8) | rx[1];
}

static BaseType_t cli_pll(char *pcWriteBuffer,
                          size_t xWriteBufferLen,
                          const char *pcCmd)
{
  char *p;

  /* Initialize CDCE6214 Out 2 on the GMM-7550 Module */
  pll_write_reg(0x3e, 0x0019); /* R62, PSA, CH2 Divider == 25 */
  pll_write_reg(0x3f, 0x0000); /* R63 -- disable Out2 LP-HCSL */
  pll_write_reg(0x41, 0x0800); /* R65 -- enable Out2 LVDS */
  pll_write_reg(0x42, 0x0006); /* R66 -- standard Out2 LVDS swing */

  strncpy(pcWriteBuffer,
          "CDCE6214 Out 2 is eanbled\n",
          xWriteBufferLen);
  return pdFALSE;
}

static const CLI_Command_Definition_t pll_cmd = {
  "pll",
  "pll\n  Control CDCE6214 PLL\n\n",
  cli_pll,
  0
};

void cli_register_pll(void)
{
  FreeRTOS_CLIRegisterCommand(&pll_cmd);
}
