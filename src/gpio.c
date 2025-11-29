#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "gmm7550_control.h"

static void gmm7550_gpio_init(void)
{
  gpio_init(GMM7550_MR_PIN);
  gpio_set_dir(GMM7550_MR_PIN, GPIO_OUT);
  gpio_init(GMM7550_EN_PIN);
  gpio_set_dir(GMM7550_EN_PIN, GPIO_OUT);
  gpio_init(GMM7550_OFF_PIN);
  gpio_set_dir(GMM7550_OFF_PIN, GPIO_OUT);

  gpio_put(GMM7550_EN_PIN, 0);
  gpio_put(GMM7550_OFF_PIN, 0);
  gpio_put(GMM7550_MR_PIN, 1);
}

static void gmm7550_on(void)
{
  gpio_put(GMM7550_EN_PIN, 1);
}

static void gmm7550_off(void)
{
  gpio_put(GMM7550_EN_PIN, 0);
  i2c_gpio_initialized = false;
}

static void gmm7550_hreset(uint rst)
{
  switch (rst) {
  case 0:  /* deassert */
    gpio_put(GMM7550_MR_PIN, 0);
    break;
  case 1:  /* assert */
    gpio_put(GMM7550_MR_PIN, 1);
    i2c_gpio_initialized = false;
    break;
  default: /* pulse */
    gpio_put(GMM7550_MR_PIN, 1);
    i2c_gpio_initialized = false;
    vTaskDelay(GMM7550_MR_TIME_MS / portTICK_PERIOD_MS);
    gpio_put(GMM7550_MR_PIN, 0);
  }
}

static BaseType_t cli_gmm7550_on(char *pcWriteBuffer,
                                 size_t xWriteBufferLen,
                                 const char *pcCmd)
{
  gmm7550_on();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gmm7550_hreset(0);

  *pcWriteBuffer = '\0';
  return pdFALSE;
}

static const CLI_Command_Definition_t on_cmd = {
  "on",
  "on\n  Turn the GMM-7550 module ON\n\n",
  cli_gmm7550_on,
  0
};

static BaseType_t cli_gmm7550_off(char *pcWriteBuffer,
                                  size_t xWriteBufferLen,
                                  const char *pcCmd)
{
  gmm7550_hreset(1);
  gmm7550_off();

  *pcWriteBuffer = '\0';
  return pdFALSE;
}

static const CLI_Command_Definition_t off_cmd = {
  "off",
  "off\n  Turn the GMM-7550 module OFF\n\n",
  cli_gmm7550_off,
  0
};

static BaseType_t cli_gmm7550_hrst(char *pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char *pcCmd)
{
  char *p;
  BaseType_t p_len, ret;

  p = (char *)FreeRTOS_CLIGetParameter(pcCmd, 1, &p_len);
  if (p) {
    if (p_len == 1) {
      if (*p == '0') {
        gmm7550_hreset(0);
      } else if (*p == '1') {
        gmm7550_hreset(1);
      }
    }
  } else {
    gmm7550_hreset(2);
  }

  *pcWriteBuffer = '\0';
  return pdFALSE;
}

static const CLI_Command_Definition_t hrst_cmd = {
  "hrst",
  "hrst [0|1]\n  Hard reset GMM-7550 module\n  no argument - reset pulse\n  0 - de-assert reset\n  1 - assert reset\n\n",
  cli_gmm7550_hrst,
  -1
};

void cli_register_gpio(void)
{
  gmm7550_gpio_init();

  FreeRTOS_CLIRegisterCommand(&on_cmd);
  FreeRTOS_CLIRegisterCommand(&off_cmd);
  FreeRTOS_CLIRegisterCommand(&hrst_cmd);
}
