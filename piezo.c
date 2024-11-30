#include "piezo.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

#include "led.h"
#include "pico/time.h"

#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Initialize the Piezo sensor
void setupPiezoSensor() {
    gpio_init(PIEZO_SENSOR_PIN);
    gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR_PIN);
}
bool isPillDispensed() {
    const int check_count = 40000;            // Lisää tarkistuksia pidemmälle ajalle
    const int spike_threshold = 1;           // Herkkä piikkien tunnistus
    const int low_signal_threshold = 1;      // Maksimiherkkyys LOW-signaaleille
    const int integrated_signal_threshold = 30; // Alennettu alueen kynnys
    const int low_signal_weight = 30;        // Lisää painotusta LOW-signaaleille

    int high_count = 0;
    int low_count = 0;
    int spike_detected = 0;
    int low_integrated_signal = 0;
    int buffer[500] = {0};
    int buffer_index = 0;
    int sum = 0;

    memset(buffer, 0, sizeof(buffer)); // Nollaa puskuri

    bool last_state = gpio_get(PIEZO_SENSOR_PIN);

    for (int i = 0; i < check_count; i++) {
        bool current_state = gpio_get(PIEZO_SENSOR_PIN);

        sum -= buffer[buffer_index];
        buffer[buffer_index] = current_state ? 1 : -1;
        sum += buffer[buffer_index];
        buffer_index = (buffer_index + 1) % 500;

        if (current_state) {
            high_count++;
        } else {
            low_count++;
            low_integrated_signal += low_signal_weight;
        }

        if (last_state != current_state) {
            spike_detected++;
        }

        last_state = current_state;
        sleep_us(1);
    }

    int filtered_average = sum / 500;

    bool result = (low_count >= low_signal_threshold ||
                   spike_detected >= spike_threshold ||
                   low_integrated_signal >= integrated_signal_threshold ||
                   filtered_average < -2);

    if (result) {
        DEBUG_PRINT("Pill detected! High count = %d, Low count = %d, Spikes = %d, Integrated LOW Signal = %d, Filtered Avg = %d, Result = %d",
                    high_count, low_count, spike_detected, low_integrated_signal, filtered_average, result);
    } else {
        DEBUG_PRINT("No pill detected. High count = %d, Low count = %d, Spikes = %d, Integrated LOW Signal = %d, Filtered Avg = %d, Result = %d",
                    high_count, low_count, spike_detected, low_integrated_signal, filtered_average, result);
    }

    return result;
}
void blinkError(int times) {
    for (int i = 0; i < times; i++) {
        allLedsOn();
        sleep_ms(200);
        allLedsOff();
        sleep_ms(200);
    }
}