#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "gmm7550_control.h"

#include "string.h"
#include "hex.h"

#define PCA9539A_ADDR 0x74

static i2c_inst_t *i2c = I2C_INSTANCE(GMM7550_I2C);
bool i2c_gpio_initialized = false;

static void gmm7550_i2c_init(void)
{
  i2c_init(i2c, 400000);
  gpio_set_function(GMM7550_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(GMM7550_I2C_SCL_PIN, GPIO_FUNC_I2C);
}

static inline void pca_write_reg(const uint8_t reg, const uint8_t data)
{
  uint8_t buf[2];
  buf[0] = reg;
  buf[1] = data;
  (void) i2c_write_blocking(i2c, PCA9539A_ADDR, buf, 2, false);
}

static inline uint8_t pca_read_reg(const uint8_t reg)
{
  uint8_t tx = reg;
  uint8_t rx = 0;

  if (i2c_write_blocking(i2c, PCA9539A_ADDR, &tx, 1, true) == 1) {
    (void) i2c_read_blocking(i2c, PCA9539A_ADDR, &rx, 1, false);
  }
  return rx;
}

static bool config_is_safe(uint8_t b)
{
  const uint8_t mux = (b >> 4) & 0x0f;
  const uint8_t cfg = b * 0x0f;

  /* SPI Active mode and SPI_SEL0 == 1 is an usafe combo */
  if (((cfg & 0xC) == 0) && (mux & 0x1)) return false;

  return true;
}

static void gmm7550_i2c_gpio_init(void)
{
  if (!i2c_gpio_initialized) {
    /* output port 0 */
    /* hardware default values */
    pca_write_reg(2, 0xcf);

    /* output port 1 */
    /* SPI mux and Configuration mode -- hardware default values */
    pca_write_reg(3, 0);

    /* polarity inversion 0 */
    /* CFG_FAILED_N bit */
    pca_write_reg(4, 0x04);

    /* configuration register 0 */
    pca_write_reg(6, 0x0c);

    /* configuration register 1 */
    /* all pins are outputs */
    pca_write_reg(7, 0);

    i2c_gpio_initialized = true;
  }
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

static BaseType_t cli_pca(char *pcWriteBuffer,
                          size_t xWriteBufferLen,
                          const char *pcCmd)
{
  static uint line = 0;
  int ret;
  char *p;
  uint8_t wr_byte, rd_byte;

  if (line == 0) {
    strncpy(pcWriteBuffer,
            "PCA9539A registers:\n",
            xWriteBufferLen);
    line += 1;
    return pdTRUE;
  } else if (line <= 8) {
    p = pcWriteBuffer;
    *p++ = ' ';
    *p++ = ' ';
    *p++ = hex_digit(line-1);
    *p++ = ':';
    *p++ = ' ';

    wr_byte = line - 1; /* register index */
    if (i2c_write_blocking(i2c, PCA9539A_ADDR, &wr_byte, 1, true) == 1) {
      if (i2c_read_blocking(i2c, PCA9539A_ADDR, &rd_byte, 1, false) == 1) {
        *p++ = hex_digit(rd_byte >> 4);
        *p++ = hex_digit(rd_byte);
      } else {
        *p++ = '-';
        *p++ = 'r';
      }
    } else {
      *p++ = '-';
      *p++ = 'w';
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

static const CLI_Command_Definition_t pca_cmd = {
  "pca",
  "pca\n  Dump PCA9539A (I2C GPIO) registers\n\n",
  cli_pca,
  0
};

static void gmm7550_sreset(uint rst)
{
  uint8_t data;

  if (!i2c_gpio_initialized) {gmm7550_i2c_gpio_init();}

  switch (rst) {
  case 0:  /* deassert */
    data = pca_read_reg(2);
    data |= 0x01;
    pca_write_reg(2, data);
    break;
  case 1:  /* assert */
    data = pca_read_reg(2);
    data &= ~0x01;
    pca_write_reg(2, data);
    break;
  default: /* pulse */
    data = pca_read_reg(2);
    pca_write_reg(2, data & ~0x01);
    vTaskDelay(GMM7550_MR_TIME_MS / portTICK_PERIOD_MS);
    pca_write_reg(2, data |  0x01);
  }
}

static BaseType_t cli_mux(char *pcWriteBuffer,
                          size_t xWriteBufferLen,
                          const char *pcCmd)
{
  BaseType_t p_len;
  uint8_t srst;
  uint8_t mux;
  uint8_t data;

  char *p = (char *)FreeRTOS_CLIGetParameter(pcCmd, 1, &p_len);

  if (p && (p_len == 1) && is_hex_digit(*p)) {

    mux = char2hex(*p);

    if (!i2c_gpio_initialized) {gmm7550_i2c_gpio_init();}

    data = pca_read_reg(3); /* output port 1 */
    data &= 0x0f; /* keep configuration mode settings */
    data |= mux << 4;

    if (config_is_safe(data)) {
      srst = pca_read_reg(2) & 0x01; /* save state of the reset signal */
      gmm7550_sreset(1);      /* assert reset signal to the FPGA */

      pca_write_reg(3, data); /* set new SPI Mux/Configuration Mode */

      data = pca_read_reg(2); /* reset bit is known to be 0 (reset active) at this point */
      data |= srst;           /* restore reset state */
      pca_write_reg(2, data);

      p = stpncpy(pcWriteBuffer, "SPI Mux configuration is ", xWriteBufferLen);
      *p++ = hex_digit(mux);
      *p++ = '\n';
      *p++ = '\0';
    } else {
      strncpy(pcWriteBuffer, "Error: unsafe SPI Mux and Configuration Mode combination\n", xWriteBufferLen);
    }
  } else {
    strncpy(pcWriteBuffer, "Error: argument should be a single hex digit (0..f|0..F)\n", xWriteBufferLen);
  }
  return pdFALSE;
}

static const CLI_Command_Definition_t mux_cmd = {
  "mux",
  "mux n\n  Set SPI multiplexors. Useful settings:\n    1 -- FPGA SPI\n    2 -- NOR FLASH\n    4 -- UART (SPI D2/D3)\n\n",
  cli_mux,
  1
};

static BaseType_t cli_cfg(char *pcWriteBuffer,
                          size_t xWriteBufferLen,
                          const char *pcCmd)
{
  BaseType_t p_len;
  uint8_t srst;
  uint8_t cfg;
  uint8_t data;

  char *p = (char *)FreeRTOS_CLIGetParameter(pcCmd, 1, &p_len);

  if (p && (p_len == 1) && is_hex_digit(*p)) {

    cfg = char2hex(*p);

    if (!i2c_gpio_initialized) {gmm7550_i2c_gpio_init();}

    data = pca_read_reg(3); /* output port 1 */
    data &= 0xf0; /* keep SPI multiplexor settings */
    data |= cfg;

    if (config_is_safe(data)) {
      srst = pca_read_reg(2) & 0x01; /* save state of the reset signal */
      gmm7550_sreset(1);      /* assert reset signal to the FPGA */

      pca_write_reg(3, data); /* set new SPI Mux/Configuration Mode */

      data = pca_read_reg(2); /* reset bit is known to be 0 (reset active) at this point */
      data |= srst;           /* restore reset state */
      pca_write_reg(2, data);

      p = stpncpy(pcWriteBuffer, "Configuration mode is ", xWriteBufferLen);
      *p++ = hex_digit(cfg);
      *p++ = '\n';
      *p++ = '\0';
    } else {
      strncpy(pcWriteBuffer, "Error: unsafe SPI Mux and Configuration Mode combination\n", xWriteBufferLen);
    }
  } else {
    strncpy(pcWriteBuffer, "Error: argument should be a single hex digit (0..f|0..F)\n", xWriteBufferLen);
  }
  return pdFALSE;
}

static const CLI_Command_Definition_t cfg_cmd = {
  "cfg",
  "cfg n\n  Set Configuration mode. Useful settings:\n    0 -- SPI Active\n    4 -- SPI Passive\n    c -- JTAG\n\n",
  cli_cfg,
  1
};

static BaseType_t cli_gmm7550_rst(char *pcWriteBuffer,
                                  size_t xWriteBufferLen,
                                  const char *pcCmd)
{
  char *p;
  BaseType_t p_len, ret;

  p = (char *)FreeRTOS_CLIGetParameter(pcCmd, 1, &p_len);
  if (p) {
    if (p_len == 1) {
      if (*p == '0') {
        gmm7550_sreset(0);
      } else if (*p == '1') {
        gmm7550_sreset(1);
      }
    }
  } else {
    gmm7550_sreset(2);
  }

  *pcWriteBuffer = '\0';
  return pdFALSE;
}

static const CLI_Command_Definition_t rst_cmd = {
  "rst",
  "rst [0|1]\n  Soft reset GMM-7550 module\n  no argument - reset pulse\n  0 - de-assert reset\n  1 - assert reset\n\n",
  cli_gmm7550_rst,
  -1
};

void cli_register_i2c(void)
{
  gmm7550_i2c_init();
  FreeRTOS_CLIRegisterCommand(&scan_cmd);
  FreeRTOS_CLIRegisterCommand(&pca_cmd);
  FreeRTOS_CLIRegisterCommand(&mux_cmd);
  FreeRTOS_CLIRegisterCommand(&cfg_cmd);
  FreeRTOS_CLIRegisterCommand(&rst_cmd);
}
