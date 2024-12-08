//
// Created by lily on 12/3/2024.
//
#include "watchdog.h"
//initialize watchdog
void watchdog_init(uint32_t timeout_ms) {
    watchdog_enable(timeout_ms,true);
}
//updates the watchdog
void watchdogFeed() {
    watchdog_update();
}