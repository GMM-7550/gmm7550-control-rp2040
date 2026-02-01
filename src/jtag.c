#include "pico/stdlib.h"
#include "tusb.h"
#include "gmm7550_control.h"

static void jtag_task()
{
  while(1) {
    vTaskDelay(1);
  }
}

void gmm7550_jtag_init(void)
{
  gpio_set_function_masked((1<<GMM7550_JTAG_TDI_PIN) |
                           (1<<GMM7550_JTAG_TCK_PIN) |
                           (1<<GMM7550_JTAG_TDO_PIN) |
                           (1<<GMM7550_JTAG_TMS_PIN),
                           GPIO_FUNC_PIO0);

  xTaskCreate(jtag_task, "JTAG",
              configMINIMAL_STACK_SIZE,
              NULL,
              (tskIDLE_PRIORITY + 2UL),
              NULL
              );
}
