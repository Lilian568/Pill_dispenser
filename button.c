#include "button.h"
#include "hardware/gpio.h"
#include "pico/time.h"


// Initialize buttons
void buttonsInit() {
    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0); // Use pull-up resistors

    gpio_init(SW_2);
    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2); // Use pull-up resistors
}

// Check if a button is pressed with debounce logic
bool isButtonPressed(int button_pin) {
    static uint32_t last_debounce_time = 0; // Last debounce timestamp
    static bool last_state = false;        // Previous button state
    static bool initialized = false;       // Initialization check

    const uint32_t debounce_delay = 2;     // 2 ms debounce delay

    bool current_state = gpio_get(button_pin);
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (!initialized) {
        last_state = current_state;
        initialized = true;
        return false; // Avoid registering a press during initialization
    }

    if (current_state != last_state && (current_time - last_debounce_time > debounce_delay)) {
        last_debounce_time = current_time;

        if (current_state) {
            last_state = current_state;
            return true; // Return true once per button press
        }
    }

    last_state = current_state;
    return false;
}