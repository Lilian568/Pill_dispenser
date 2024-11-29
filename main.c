#include "led.h"
#include "button.h"
#include "motor.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

bool isButtonPressed(int button_pin) {
    static uint32_t last_debounce_time = 0;  // Viimeisin debounce-aika
    static bool last_state = false;         // Tallentaa painikkeen edellisen tilan
    static bool initialized = false;        // Tarkistaa, onko tila alustettu

    const uint32_t debounce_delay = 10;     // 10 ms debounce

    // Lue painikkeen nykyinen tila
    bool current_state = gpio_get(button_pin);

    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Alusta tila ensimmäisellä kutsulla
    if (!initialized) {
        last_state = current_state;  // Aseta painikkeen tila käynnistystilaan
        initialized = true;
        DEBUG_PRINT("Button initialized: pin=%d, state=%d", button_pin, current_state);
        return false;  // Vältä painalluksen rekisteröinti käynnistyksessä
    }

    // Tarkista, onko tila muuttunut ja debounce-viive on kulunut
    if (current_state != last_state && (current_time - last_debounce_time > debounce_delay)) {
        last_debounce_time = current_time;

        // Painalluksen rekisteröinti "false" -> "true"
        if (current_state) {
            DEBUG_PRINT("Button pressed: pin=%d", button_pin);
            last_state = current_state;
            return true; // Palauta "true" kerran per painallus
        }
    }

    // Päivitä painikkeen tila
    last_state = current_state;

    return false;
}

int main() {
    timer_hw->dbgpause = 0;
    stdio_init_all();
    ledsInit();
    pwmInit();
    buttonsInit();
    setup();

    int state = 1;
    absolute_time_t last_time_sw2_pressed = nil_time;  // Alustetaan tyhjällä ajalla
    const uint32_t step_delay_ms = 5000;  // Testausta varten 5 sekuntia
    int portion_count = 0;
    const int max_portion = 7;

    bool state1_logged = false;
    bool state2_logged = false;

    while (true) {
        absolute_time_t current_time = get_absolute_time();

        switch (state) {
            case 1:
                if (!state1_logged) {
                    DEBUG_PRINT("State 1: Waiting for SW_0 button to start calibration.");
                    state1_logged = true;
                }

                blink();

                // Tarkista, onko SW_0-painiketta painettu
                if (isButtonPressed(SW_0)) {
                    DEBUG_PRINT("SW_0 button pressed, starting calibration...");
                    calibrate();
                    state = 2; // Siirrytään tilaan 2 vasta painalluksen jälkeen
                    state1_logged = false;
                    state2_logged = false;
                    DEBUG_PRINT("Calibration complete, moving to state 2.");
                    last_time_sw2_pressed = nil_time;  // Nollataan viimeisin painallusaika
                }
                break;

            case 2:
                if (!state2_logged) {
                    DEBUG_PRINT("State 2: Monitoring SW_2 for motor activation.");
                    state2_logged = true;
                }

                uint64_t time_since_last_press = (last_time_sw2_pressed == nil_time)
                                                     ? 0
                                                     : absolute_time_diff_us(last_time_sw2_pressed, current_time) / 1000;

                // Tarkista, onko painiketta SW_2 painettu
                if (isButtonPressed(SW_2)) {
                    DEBUG_PRINT("SW_2 button pressed. Immediate motor activation...");
                    rotate_steps_512();
                    portion_count++;
                    last_time_sw2_pressed = current_time;  // Päivitä viimeisin aika heti painalluksen yhteydessä
                    DEBUG_PRINT("Portion count incremented to %d after SW_2 press.", portion_count);
                }

                // Tarkista, onko aika täyttynyt moottorin pyöritykselle
                else if (time_since_last_press >= step_delay_ms && last_time_sw2_pressed != nil_time) {
                    DEBUG_PRINT("Step delay met, rotating motor...");
                    rotate_steps_512();
                    portion_count++;
                    last_time_sw2_pressed = current_time;  // Päivitä viimeisin aika
                    DEBUG_PRINT("Portion count incremented to %d", portion_count);
                }

                if (portion_count >= max_portion) {
                    DEBUG_PRINT("Max portion count reached. Resetting portion count and turning off LEDs.");
                    portion_count = 0;
                    allLedsOff();
                    state = 1; // Palautetaan tila alkuun
                    state1_logged = false; // Nollaa logiviestit seuraavaa sykliä varten
                    state2_logged = false;
                }
                break;

            default:
                DEBUG_PRINT("Unknown state encountered: %d", state);
                break;
        }

        sleep_ms(100); // Pieni viive pääsilmukassa
    }

    return 0;
}