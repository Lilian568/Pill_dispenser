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
    const int check_count = 100000;            // Amount of signal checkings (40000 = 0,04s)
    const int spike_threshold = 1;           // Sensitivity for detecting spikes in signal
    const int low_signal_threshold = 1;      // Max sensitivity for low signal
    const int integrated_signal_threshold = 30; // Alennettu alueen kynnys
    const int low_signal_weight = 30;        // More weight on LOW-signals

    // Initializing the variables
    int high_count = 0;     // Counter for HIGH signals
    int low_count = 0;      // Counter for LOW signals
    int spike_detected = 0; // Counter for signal state changes
    int low_integrated_signal = 0;  // Sum of weighted LOW signals
    int buffer[500] = {0};  // Circular buffer for signal filtering
    int buffer_index = 0;   // Current index in the buffer
    int sum = 0;            // Sum of buffer values for filtering

    memset(buffer, 0, sizeof(buffer)); // Initialize the buffer with zeros

    bool last_state = gpio_get(PIEZO_SENSOR_PIN);

    for (int i = 0; i < check_count; i++) {
        bool current_state = gpio_get(PIEZO_SENSOR_PIN);

        sum -= buffer[buffer_index];
        buffer[buffer_index] = current_state ? 1 : -1;
        sum += buffer[buffer_index];
        buffer_index = (buffer_index + 1) % 500; // Increment buffer index with wrap-around


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

    // Calculate the filtered average from the buffer
    int filtered_average = sum / 500;

    // Determine if a pill has been detected based on thresholds
    bool result = (low_count >= low_signal_threshold || // Check if LOW signal count is sufficient
                   spike_detected >= spike_threshold || // Check if spike count meets threshold
                   low_integrated_signal >= integrated_signal_threshold || // Check integrated signal
                   filtered_average < -2); // Check if filtered average indicates strong LOW signal

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