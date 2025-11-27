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
}

static void gmm7550_hreset(uint rst)
{
  switch (rst) {
  case 0:  /* deassert */
    gpio_put(GMM7550_MR_PIN, 0);
    break;
  case 1:  /* assert */
    gpio_put(GMM7550_MR_PIN, 1);
    break;
  default: /* pulse */
    gpio_put(GMM7550_MR_PIN, 1);
    vTaskDelay(GMM7550_MR_TIME_MS / portTICK_PERIOD_MS);
    gpio_put(GMM7550_MR_PIN, 0);
  }
}

static BaseType_t cli_gmm7550_on(char *pcWriteBuffer,
                                 size_t xWriteBufferLen,
                                 const char *pcCmd)
{
  gmm7550_on();
  gmm7550_hreset(0);

  *pcWriteBuffer = '\0';
  return pdFALSE;
}

static const CLI_Command_Definition_t on_cmd = {
  "on",
  "on\n  Turn the GMM-7550 module ON",
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
  "off\n  Turn the GMM-7550 module OFF",
  cli_gmm7550_off,
  0
};

void cli_register_gpio(void)
{
  gmm7550_gpio_init();

  FreeRTOS_CLIRegisterCommand(&on_cmd);
  FreeRTOS_CLIRegisterCommand(&off_cmd);
}
