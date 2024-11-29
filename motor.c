#include <stdio.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include <string.h>
#include "motor.h"

#define OPTOFORK_PIN 28
#define MOTOR_PIN1 2
#define MOTOR_PIN2 3
#define MOTOR_PIN3 6
#define MOTOR_PIN4 13

int steps_per_revolution = -1; // Kalibroidut askeleet per kierros
int steps_per_drop = -1;       // Kalibroidut askeleet per lääkkeen annostelu
bool is_calibrated = false;    // Kalibroinnin tila

// Moottorin 8-vaiheinen askelsarja
void setSteps(int step) {
    bool steps[8][4] = {
        {1, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 1},
        {1, 0, 0, 1}
    };
    gpio_put(MOTOR_PIN1, steps[step][0]);
    gpio_put(MOTOR_PIN2, steps[step][1]);
    gpio_put(MOTOR_PIN3, steps[step][2]);
    gpio_put(MOTOR_PIN4, steps[step][3]);
    sleep_ms(1);
}

void rotate(int step_count) {
    static int current_position = 0; // Seuraa moottorin tarkkaa askelpaikkaa
    for (int i = 0; i < step_count; i++) {
        current_position = (current_position + 1) % 8;
        setSteps(current_position);
    }
}
// Hienosäätää moottorin liikettä pienellä määrällä askeleita
void fine_tune_position() {
    // Pyöritetään moottoria 1/16 kierrosta, mikä on pieni liike
    rotate(steps_per_revolution / 12);  // Pienempi liike, 1/16 kierrosta
}

void setup() {
    gpio_init(OPTOFORK_PIN);
    gpio_set_dir(OPTOFORK_PIN, GPIO_IN);
    gpio_pull_up(OPTOFORK_PIN);

    gpio_init(MOTOR_PIN1);
    gpio_set_dir(MOTOR_PIN1, GPIO_OUT);
    gpio_init(MOTOR_PIN2);
    gpio_set_dir(MOTOR_PIN2, GPIO_OUT);
    gpio_init(MOTOR_PIN3);
    gpio_set_dir(MOTOR_PIN3, GPIO_OUT);
    gpio_init(MOTOR_PIN4);
    gpio_set_dir(MOTOR_PIN4, GPIO_OUT);
}

void calibrate() {
    printf("Starting calibration...\n");

    int step_count = 0;
    int revolutions_total = 0;
    const int valid_iterations = 1;
    const int actual_iterations = valid_iterations + 1;

    for (int i = 0; i < actual_iterations; i++) {
        step_count = 0;

        // Käännetään kunnes sensori ei enää laukea
        while (gpio_get(OPTOFORK_PIN)) {
            rotate(1);
            step_count++;
        }
        sleep_ms(100);

        // Käännetään kunnes sensori laukeaa uudelleen
        while (!gpio_get(OPTOFORK_PIN)) {
            rotate(1);
            step_count++;
        }

        // Käännetään kunnes sensori ei enää laukea
        while (gpio_get(OPTOFORK_PIN)) {
            rotate(1);
            step_count++;
        }

        if (i > 0) { // Hylätään ensimmäinen kierros
            revolutions_total += step_count;
            printf("Valid revolution %d: %d steps\n", i, step_count);
        } else {
            printf("Ignored revolution %d: %d steps\n", i + 1, step_count);
        }

        sleep_ms(100);
    }

    // Lasketaan askeleet per kierros ja askeleet per lääkkeen annostelu
    steps_per_revolution = revolutions_total / valid_iterations;
    steps_per_drop = steps_per_revolution / 8; // Yksi pilleri vastaa 1/8 kierrosta
    is_calibrated = true;

    printf("Calibration complete. Average steps per revolution: %d\n", steps_per_revolution);
    printf("Steps per drop: %d\n", steps_per_drop);

    // Kohdistetaan moottori tarkasti lähtökohtaan
    while (gpio_get(OPTOFORK_PIN)) {
        rotate(1);
    }
    while (!gpio_get(OPTOFORK_PIN)) {
        rotate(1);
    }
    fine_tune_position();

    // Fine-tuning alignment after calibration
    /*for (int i = 0; i < 1; i++) {  // Adjust this value if necessary
        rotate(-1);  // Slight backward rotation to fine-tune position
        sleep_ms(100);
    }*/

    printf("Motor aligned to start position.\n");
}

void rotate_steps_512() {
    if (!is_calibrated) {
        printf("Motor is not calibrated. Cannot rotate.\n");
        return;
    }
    printf("Rotating %d steps (1 drop)...\n", steps_per_drop);
    rotate(steps_per_drop);
}