#include "queue.h"
extern QueueHandle_t serial_rxQueue, serial_txQueue;

extern void serial_init(__unused void *params);
extern void usb_task(__unused void *params);
