#include "pico/stdlib.h"
#include "gmm7550_control.h"
#include "FreeRTOS_CLI.h"
#include "hardware/adc.h"

#include "string.h"
#include "hex.h"

#define ADC_SHORT_HELP "adc [v|i|t]\n"

static const float adc_coef = GMM7550_ADC_VREF / ((float)(1<<12));
static const float i_conv = adc_coef * 1000.f; /* 1V   -> 1000 mA */
static const float v_conv = adc_coef * 2.2f;   /* 2.5V -> 5.5V (120kOhm / 100kOhm divider) */

static char *uint16_to_hex(char *p, const uint16_t d)
{
  *p++ = '0'; *p++ = 'x';
  *p++ = hex_digit((d >> 12) & 0xf);
  *p++ = hex_digit((d >>  8) & 0xf);
  *p++ = hex_digit((d >>  4) & 0xf);
  *p++ = hex_digit((d >>  0) & 0xf);
  return p;
}

static BaseType_t cli_adc(char *pcWriteBuffer,
                          size_t xWriteBufferLen,
                          const char *pcCmd)
{
  static int line = 0;
  char *p;
  uint16_t reg;
  float val;
  BaseType_t p_len;

  p = (char *)FreeRTOS_CLIGetParameter(pcCmd, 1, &p_len);

  if (p) {
    if (p_len == 1) {
      switch (*p) {
      case 'v':
        adc_select_input(GMM7550_ADC_INPUT(GMM7550_ADC_V_PIN));
        reg = adc_read();
        val = reg * v_conv;
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "V = %f V (0x%03x)\n", val, reg);
        break;
      case 'i':
        adc_select_input(GMM7550_ADC_INPUT(GMM7550_ADC_I_PIN));
        reg = adc_read();
        val = reg * i_conv;
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "I = %f mA (0x%03x)\n", val, reg);
        break;
      case 't':
        adc_select_input(GMM7550_ADC_INPUT_T);
        reg = adc_read();
        val = reg * adc_coef;
        val = 27. - (val - 0.706) / 0.001721;
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "T = %f (0x%03x)\n", val, reg);
        break;
      default:
        strncpy(pcWriteBuffer, "ADC command argument should be 'v', 'i' or 't'", xWriteBufferLen);
      }
      return pdFALSE;
    } else {
      strncpy(pcWriteBuffer, "ADC command argument should be 1 char", xWriteBufferLen);
      return pdFALSE;
    }
  } else { /* no argument -- print help */
    switch (line) {
    case 0:
      strncpy(pcWriteBuffer,
              ADC_SHORT_HELP,
              xWriteBufferLen);
      line++;
      break;
    case 1:
      strncpy(pcWriteBuffer,
              "  v -- Voltage\n",
              xWriteBufferLen);
      line++;
      break;
    case 2:
      strncpy(pcWriteBuffer,
              "  i -- Current\n",
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

static const CLI_Command_Definition_t adc_cmd = {
  "adc",
  ADC_SHORT_HELP
  "  ADC control (print help without an argument)\n\n",
  cli_adc,
  -1
};

void cli_register_adc(void)
{
  adc_init();
  adc_gpio_init(GMM7550_ADC_V_PIN);
  adc_gpio_init(GMM7550_ADC_I_PIN);
  adc_set_temp_sensor_enabled(true);
  FreeRTOS_CLIRegisterCommand(&adc_cmd);
}
