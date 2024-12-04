#include "piezo.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include "led.h"
#include "pico/time.h"
#include "motor.h"

#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Global variables for tracking signals in interrupts
volatile int high_count = 0;
volatile int low_count = 0;
volatile int spike_count = 0;
volatile int low_integrated_signal = 0;
volatile bool last_state = false;

// Parameters for signal analysis
#define LOW_SIGNAL_WEIGHT 30
#define SPIKE_THRESHOLD 1
#define LOW_SIGNAL_THRESHOLD 1
#define INTEGRATED_SIGNAL_THRESHOLD 30
#define DETECTION_WINDOW_MS 500 // Analysis duration in milliseconds

// Interrupt handler for the Piezo sensor
void piezo_interrupt_handler(uint gpio, uint32_t events) {
    bool current_state = gpio_get(PIEZO_SENSOR_PIN);

    if (!current_state) { // LOW signal detected
        low_count++;
        low_integrated_signal += LOW_SIGNAL_WEIGHT;
    }

    if (current_state != last_state) { // Spike detection (state change)
        spike_count++;
    }

    last_state = current_state;
}

// Initialize the Piezo sensor and set up interrupts
void setupPiezoSensor() {
    gpio_init(PIEZO_SENSOR_PIN);
    gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR_PIN);

    // Register interrupt for FALLING EDGE (LOW signal)
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_FALL, true, &piezo_interrupt_handler);

    DEBUG_PRINT("Piezo sensor initialized with FALLING EDGE interrupt only.\n");
}

// Enable Piezo sensor interrupts
void enablePiezoInterrupt() {
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_FALL, true);
    DEBUG_PRINT("Piezo interrupt enabled.\n");
}

// Disable Piezo sensor interrupts
void disablePiezoInterrupt() {
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_FALL, false);
    DEBUG_PRINT("Piezo interrupt disabled.\n");
}

// Pill detection analysis without polling
bool dispenseAndDetectPill() {
    high_count = 0;
    low_count = 0;
    spike_count = 0;
    low_integrated_signal = 0;

    enablePiezoInterrupt();

    DEBUG_PRINT("Starting motor rotation...\n");
    rotate_steps_512(); // Start motor rotation and wait for the pill to drop

    // Wait for interrupts to accumulate data
    sleep_ms(DETECTION_WINDOW_MS);

    disablePiezoInterrupt();

    // Analyze collected data
    bool result = (low_count >= LOW_SIGNAL_THRESHOLD ||
                   spike_count >= SPIKE_THRESHOLD ||
                   low_integrated_signal >= INTEGRATED_SIGNAL_THRESHOLD);

    if (result) {
        DEBUG_PRINT("Pill detected! Low count = %d, Spikes = %d, Integrated LOW Signal = %d",
                    low_count, spike_count, low_integrated_signal);
    } else {
        DEBUG_PRINT("No pill detected. Low count = %d, Spikes = %d, Integrated LOW Signal = %d",
                    low_count, spike_count, low_integrated_signal);
    }

    return result;
}

// Blink error indication with LEDs
void blinkError(int times) {
    for (int i = 0; i < times; i++) {
        allLedsOn();
        sleep_ms(200);
        allLedsOff();
        sleep_ms(200);
    }
}