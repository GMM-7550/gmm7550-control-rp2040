#include "pico/stdlib.h"
#include "gmm7550_control.h"
#include "FreeRTOS_CLI.h"

#include "string.h"
#include "hex.h"

#define CDCE6214_ADDR 0x68

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

static char *uint16_to_hex(char *p, const uint16_t d)
{
  *p++ = '0'; *p++ = 'x';
  *p++ = hex_digit((d >> 12) & 0xf);
  *p++ = hex_digit((d >>  8) & 0xf);
  *p++ = hex_digit((d >>  4) & 0xf);
  *p++ = hex_digit((d >>  0) & 0xf);
  return p;
}

/* Factory configuration page (to be skipped) */
#define EEPROM_F_PAGE_SIZE 16
#define EEPROM_PAGE_SIZE   24

/* Should be greater than 8 ms */
#define EEPROM_WRITE_DELAY_MS 12

/* Page 0 -- factory default */
static uint16_t page0_0[EEPROM_PAGE_SIZE] = {
  0xa801, 0x00ec, 0x4000, 0x2470, 0x0200, 0x0060, 0x0000, 0x0000,
  0x0000, 0x0a22, 0x1800, 0x00d8, 0x8000, 0x0c40, 0x0000, 0x0c48,
  0x0000, 0x0d08, 0x0000, 0x1008, 0x0000, 0x0000, 0x0000, 0x1000
};

/* Page 0 -- GMM-7550 default (Out 2 disabled) */
static uint16_t page0_a[EEPROM_PAGE_SIZE] = {
  0x2020, 0x0446, 0x4000, 0x1c60, 0x0200, 0x0060, 0x0000, 0x0000,
  0x0000, 0x0a22, 0x1800, 0x00d8, 0x0000, 0x0c10, 0x0200, 0x0c00,
  0x0000, 0xc000, 0x0003, 0x0080, 0x0020, 0x0000, 0x0000, 0x1000
};

/* Page 0 -- Out 2 LVDS 24 MHz for USB3340 (ULPI) reference clock */
static uint16_t page0_b[EEPROM_PAGE_SIZE] = {
  0x2020, 0x0446, 0x4000, 0x1c60, 0x0200, 0x0060, 0x0000, 0x0000,
  0x0000, 0x0a22, 0x1800, 0x00d8, 0x0000, 0x3210, 0x0000, 0x0c10,
  0x0000, 0xc000, 0x0003, 0x0080, 0x0020, 0x0000, 0x0000, 0x1000
};

static void pll_program_page(const uint16_t *eeprom_data)
{
  const uint16_t *d = eeprom_data;

  gmm7550_sreset(1);
  uint8_t io = pca_read_reg(2);
  io |= 0x20; /* Drive HW_SW_CTRL pin High -- select Page 1 */
  pca_write_reg(2, io);
  gmm7550_sreset(0);

  vTaskDelay(EEPROM_WRITE_DELAY_MS / portTICK_PERIOD_MS);
  pll_write_reg(0x000f, 0x5020); /* Unlock EEPROM */
  vTaskDelay(EEPROM_WRITE_DELAY_MS / portTICK_PERIOD_MS);
  pll_write_reg(0x000d, EEPROM_F_PAGE_SIZE); /* Skip factory config. page, start address of Page 0 */
  for (int i=0; i<EEPROM_PAGE_SIZE; i++) {
    /* Write data word, address is auto-incremented */
    pll_write_reg(0x000e, *d++);
    vTaskDelay(EEPROM_WRITE_DELAY_MS / portTICK_PERIOD_MS);
  }
  /* TODO: CRC Update ? */
  pll_write_reg(0x000f, 0xA020); /* Lock EEPROM */
  vTaskDelay(EEPROM_WRITE_DELAY_MS / portTICK_PERIOD_MS);

  gmm7550_sreset(1);
  io = pca_read_reg(2);
  io &= ~0x20; /* Drive HW_SW_CTRL pin Low -- select Page 0, default */
  pca_write_reg(2, io);
  gmm7550_sreset(0);
}

static BaseType_t cli_pll(char *pcWriteBuffer,
                          size_t xWriteBufferLen,
                          const char *pcCmd)
{
  static int line = 0;
  char *p;
  uint16_t reg;
  BaseType_t p_len;

  p = (char *)FreeRTOS_CLIGetParameter(pcCmd, 1, &p_len);

  if (p) {
    if (p_len == 1) {
      switch (*p) {
      case 'c':
        /* Initialize CDCE6214 Out 2 on the GMM-7550 Module */
        pll_write_reg(0x3e, 0x0019); /* R62, PSA, CH2 Divider == 25 */
        pll_write_reg(0x3f, 0x0000); /* R63 -- disable Out2 LP-HCSL */
        pll_write_reg(0x41, 0x0800); /* R65 -- enable Out2 LVDS */
        pll_write_reg(0x42, 0x0006); /* R66 -- standard Out2 LVDS swing */
        strncpy(pcWriteBuffer,
                "PLL Out 2 is enabled\n",
                xWriteBufferLen);
        return pdFALSE;

      case 'r':
        if (line == 0) {
          strncpy(pcWriteBuffer,
                  "CDCE6214 registers:\n",
                  xWriteBufferLen);
          line++;
          return pdTRUE;
        } else if (line <= 10) {
          reg = 8 * (line - 1);
          p = pcWriteBuffer;
          *p++ = ' ';
          *p++ = ' ';
          for (int i=0; i<8; i++) {
            p = uint16_to_hex(p, pll_read_reg(reg++));
            *p++ = ',';
            *p++ = ' ';
          }
          *p++ = '\n'; *p++ = '\0';
          line++;
          return pdTRUE;
        } else { /* last line, just 6 registers (R80..R85) */
          reg = 8 * (line - 1);
          p = pcWriteBuffer;
          *p++ = ' ';
          *p++ = ' ';
          for (int i=0; i<6; i++) {
            p = uint16_to_hex(p, pll_read_reg(reg++));
            *p++ = ',';
            *p++ = ' ';
          }
          p -= 2; /* remove trailing comma and space */
          *p++ = '\n'; *p++ = '\0';
          line = 0;
          return pdFALSE;
        }

      case 'e':
        if (line == 0) {
          pll_write_reg(0x000b, 0); /* Set initial EEPROM address */
          strncpy(pcWriteBuffer,
                  "CDCE6214 EEPROM:\n",
                  xWriteBufferLen);
          line++;
          return pdTRUE;
        } else {
          reg = 8 * (line - 1);
          p = pcWriteBuffer;
          *p++ = ' ';
          *p++ = ' ';
          for (int i=0; i<8; i++) {
            /* read EEPROM data from register, address is auto-incremented */
            p = uint16_to_hex(p, pll_read_reg(0x000c));
            *p++ = ',';
            *p++ = ' ';
          }
          *p++ = '\n'; *p++ = '\0';

          if (line < 8) {
            line++;
            return pdTRUE;
          } else { /* line == 8 (last EEPROM line) */
            line = 0;
            return pdFALSE;
          }
        }

      default:
        strncpy(pcWriteBuffer,
                "Unknown PLL command\n",
                xWriteBufferLen);
        return pdFALSE;
      }
    } else {
      if (p_len == 2) {
        if (*p == 'P') {
          if (*(p+1) == '0') {
            strncpy(pcWriteBuffer,
                    "Program PLL EEPROM Page 0 with factory (TI) default configuraton\n",
                    xWriteBufferLen);
            pll_program_page(page0_0);
          } else if (*(p+1) == 'a') {
            strncpy(pcWriteBuffer,
                    "Program PLL EEPROM Page 0 with default GMM-7550 configuration\n",
                    xWriteBufferLen);
            pll_program_page(page0_a);
          } else if (*(p+1) == 'b') {
            strncpy(pcWriteBuffer,
                    "Program PLL EEPROM Page 0 with Out 2 (LVDS, 24 MHz) configuration\n",
                    xWriteBufferLen);
            pll_program_page(page0_b);
          } else {
            strncpy(pcWriteBuffer,
                    "Unknown EEPROM configuration (should be either '0', 'a' or 'b')\n",
                    xWriteBufferLen);
          }
        } else {
          strncpy(pcWriteBuffer,
                  "Unknown PLL command\n",
                  xWriteBufferLen);
        }
      } else {
        strncpy(pcWriteBuffer,
                "Wrong PLL command agrument length (should be 1 or 2 characters)\n",
                xWriteBufferLen);
      }
    }
    return pdFALSE;
  } else { /* no argument -- print help */
    switch (line) {
    case 0:
      strncpy(pcWriteBuffer,
              "CDCE6214 control: pll c|r|e|P0|Pa|Pb\n",
              xWriteBufferLen);
      line++;
      break;
    case 1:
      strncpy(pcWriteBuffer,
              "  c  -- configure Out 2 (LVDS, 24 MHz)\n",
              xWriteBufferLen);
      line++;
      break;
    case 2:
      strncpy(pcWriteBuffer,
              "  r  -- print registers\n",
              xWriteBufferLen);
      line++;
      break;
    case 3:
      strncpy(pcWriteBuffer,
              "  e  -- print EEPROM\n",
              xWriteBufferLen);
      line++;
      break;
    case 4:
      strncpy(pcWriteBuffer,
              "  P0 -- boot from Page 1 and program Page 0 with factory (TI) default configuration\n",
              xWriteBufferLen);
      line++;
      break;
    case 5:
      strncpy(pcWriteBuffer,
              "  Pa -- boot from Page 1 and program Page 0 with default configuration (Out 2 disabled)\n",
              xWriteBufferLen);
      line++;
      break;
    case 6:
      strncpy(pcWriteBuffer,
              "  Pb -- boot from Page 1 and program Page 0 with Out 2 enabled (LVDS, 24 MHz)\n",
              xWriteBufferLen);
      line++;
      break;
    default:
      *pcWriteBuffer = '\0';
      line = 0;
      return pdFALSE;
    }
    return pdTRUE;
  }
}

static const CLI_Command_Definition_t pll_cmd = {
  "pll",
  "pll [c|r|e|P0|Pa|Pb]\n"
  "  PLL (CDCE6214) control (print help without an argument)\n\n",
  cli_pll,
  -1
};

void cli_register_pll(void)
{
  FreeRTOS_CLIRegisterCommand(&pll_cmd);
}
