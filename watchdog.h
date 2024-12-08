#include "pico.h"
#include "hardware/watchdog.h"

void watchdog_init(uint32_t timeout_ms);
void watchdogFeed();