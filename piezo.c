#include "piezo.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include "led.h"
#include "pico/time.h"
#include "motor.h"


#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Global variables for tracking signals in interrupts
volatile int falling_edges = 0;
volatile int spikes = 0;
volatile bool last_state = false;

// Parameters for signal analysis
#define SPIKE_THRESHOLD 1
#define LOW_SIGNAL_THRESHOLD 1
#define PILL_DETECTION_MS 100 // Analysis duration in milliseconds

// Interrupt handler for the Piezo sensor
void piezo_interrupt_handler(uint gpio, uint32_t events) {
    bool current_state = gpio_get(PIEZO_SENSOR_PIN);

    if (!current_state) { // LOW signal detected
        falling_edges++;
    }

    if (current_state != last_state) { // Spike detection
        spikes++;
    }

    last_state = current_state;
}

// Initializing piezo sensor and setting up the interrupts
void setupPiezoSensor() {
    gpio_init(PIEZO_SENSOR_PIN);
    gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR_PIN);

    // Interrupts with falling edges
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_FALL, true, &piezo_interrupt_handler);
}

void enablePiezoInterrupt() {
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_FALL, true);
}

void disablePiezoInterrupt() {
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_FALL, false);
}

// Pill detection analysis and dispensing
bool dispensingAndDetecting() {
    falling_edges = 0;
    spikes = 0;

    enablePiezoInterrupt();
    rotate_steps_512(); // Motor rotating function


    sleep_ms(PILL_DETECTION_MS);  // Enough time for the pill to hit the sensor
    disablePiezoInterrupt();

    bool result = (falling_edges >= LOW_SIGNAL_THRESHOLD ||
                   spikes >= SPIKE_THRESHOLD);

    return result;
}

// Blink LEDs
void blinkError(int times) {
    for (int i = 0; i < times; i++) {
        allLedsOn();
        sleep_ms(200);
        allLedsOff();
        sleep_ms(200);
    }
}
