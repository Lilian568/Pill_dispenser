#include "piezo.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include "led.h"
#include "pico/time.h"

#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Globaaleja muuttujia signaalien seuraamiseen keskeytyksissä
volatile int high_count = 0;
volatile int low_count = 0;
volatile int spike_count = 0;
volatile int low_integrated_signal = 0;
volatile bool last_state = false;

// Analyysin parametrit
#define LOW_SIGNAL_WEIGHT 30
#define SPIKE_THRESHOLD 1
#define LOW_SIGNAL_THRESHOLD 1
#define INTEGRATED_SIGNAL_THRESHOLD 30
#define ANALYSIS_DURATION_MS 100 // 5 sekuntia analyysille

// Piezo-anturin keskeytyksen käsittelijä
void piezo_interrupt_handler(uint gpio, uint32_t events) {
    bool current_state = gpio_get(PIEZO_SENSOR_PIN);

    if (current_state) {
        high_count++;
    } else {
        low_count++;
        low_integrated_signal += LOW_SIGNAL_WEIGHT;
    }

    if (current_state != last_state) {
        spike_count++;
    }

    last_state = current_state;
}

// Piezo-anturin alustaminen ja keskeytysten asettaminen
void setupPiezoSensor() {
    gpio_init(PIEZO_SENSOR_PIN);
    gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR_PIN);

    // Rekisteröidään keskeytys GPIO:lle
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &piezo_interrupt_handler);

    DEBUG_PRINT("Piezo sensor initialized and interrupt set up.\n");
}

// Pillerin tunnistuksen analyysi
bool isPillDispensed() {
    // Nollaa laskurit ennen analyysin aloittamista
    high_count = 0;
    low_count = 0;
    spike_count = 0;
    low_integrated_signal = 0;

    // Analysoidaan signaaleja määritetyn ajan
    absolute_time_t start_time = get_absolute_time();
    while (absolute_time_diff_us(start_time, get_absolute_time()) < ANALYSIS_DURATION_MS * 1000) {
        // Odotetaan keskeytysten täyttävän laskurit
        tight_loop_contents();
    }

    // Analyysin tulokset
    bool result = (low_count >= LOW_SIGNAL_THRESHOLD ||
                   spike_count >= SPIKE_THRESHOLD ||
                   low_integrated_signal >= INTEGRATED_SIGNAL_THRESHOLD);

    if (result) {
        DEBUG_PRINT("Pill detected! High count = %d, Low count = %d, Spikes = %d, Integrated LOW Signal = %d, Filtered Avg = %d, Result = %d",
                    high_count, low_count,  low_integrated_signal, result);
    } else {
        DEBUG_PRINT("No pill detected. High count = %d, Low count = %d, Spikes = %d, Integrated LOW Signal = %d, Filtered Avg = %d, Result = %d",
                    high_count, low_count, low_integrated_signal, result);
    }

    return result;
}
