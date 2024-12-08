#include <stdio.h>
#include <string.h>
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "lorawan.h"

#define DEBUG_PRINT(f_, ...) printf((f_), ##__VA_ARGS__)

// LoRaWAN-komentojen määrittely

static lorawan_item lorawan[] = {
    {"AT\r\n", STD_WAITING_TIME},
    {"AT+MODE=LWOTAA\r\n", STD_WAITING_TIME},
    {"AT+KEY=APPKEY,\"075c56f402aef60abcf4a9a5e943aa81\"\r\n", MSG_WAITING_TIME},
    {"AT+CLASS=A\r\n", STD_WAITING_TIME},
    {"AT+PORT=8\r\n", STD_WAITING_TIME},
    {"AT+JOIN\r\n", STD_WAITING_TIME}
};

//{"AT+KEY=APPKEY,\"307fb94b705bd61559329b239686f653\"\r\n", "+KEY: 307fb94b705bd61559329b239686f653\r\n", STD_WAITING_TIME},  // Linh
//{"AT+KEY=APPKEY,\"075c56f402aef60abcf4a9a5e943aa81\"\r\n", "+KEY: APPKEY 075c56f402aef60abcf4a9a5e943aa81\r\n", STD_WAITING_TIME},  // Vipe
//{"AT+KEY=APPKEY,\"ef63470b1d60c8afb0b98261e1383392\"\r\n", "+KEY: APPKEY ef63470b1d60c8afb0b98261e1383392\r\n", STD_WAITING_TIME},  // Tomi

void loraWanInit() {
    // Alusta UART-yhteys LoRaWAN-moduulille
    uart_init(UART_ID, BAUD_RATE);

    // Määritä UART TX- ja RX-pinnit
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    // Aseta pull-up vastukset (ei pakollinen UART:lle, mutta turvallinen vaihtoehto)
    gpio_pull_up(TX_PIN);
    gpio_pull_up(RX_PIN);

    // Tulosta debug-viesti
    printf("LoRaWAN UART initialized: TX=%d, RX=%d, Baud=%d\n", TX_PIN, RX_PIN, BAUD_RATE);
}
// Lähetä UART-komento
void send_uart_command(const char *command) {
    char cmd_with_newline[512];
    snprintf(cmd_with_newline, sizeof(cmd_with_newline), "%s\n", command);
    uart_puts(UART_ID, cmd_with_newline);
    printf("Sent command: %s", cmd_with_newline);

    for (size_t i = 0; i < strlen(cmd_with_newline); i++) {
        //printf("Sent char[%zu]: 0x%x\n", i, cmd_with_newline[i]);
    }
}

// Lue UART-vastaus
bool read_uart_response(char *response, size_t response_len) {
    size_t index = 0;
    absolute_time_t global_timeout = make_timeout_time_us(VERY_LONG_TIMEOUT_US); // Kokonaisaika vastaukselle
    absolute_time_t char_timeout = make_timeout_time_us(MSG_WAITING_TIME); // Aika yhden merkin saapumiselle

    printf("Waiting for response...\n");

    while (true) {
        // Tarkista, onko kokonaisaika ylittynyt
        if (absolute_time_diff_us(get_absolute_time(), global_timeout) <= 0) {
            printf("Response timeout. Partial response: %s\n", response);
            return false;
        }

        // Tarkista, onko UART:issa luettavaa
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            //printf("Received char: 0x%x (%c)\n", c, c);

            if (c == '\r' || c == '\n') {
                if (index > 0) { // Lopeta, kun vastaus on valmis
                    response[index] = '\0';
                    printf("Complete response received: %s\n", response);
                    return true;
                }
            } else if (index < response_len - 1) {
                response[index++] = c; // Lisää merkki bufferiin
                char_timeout = make_timeout_time_us(TIMEOUT_US); // Päivitä merkkikohtainen timeout
            }
        }

        // Tarkista, onko merkkikohtainen timeout ylittynyt
        if (absolute_time_diff_us(get_absolute_time(), char_timeout) <= 0) {
            if (index > 0) { // Jos olemme saaneet osittaisen vastauksen, hyväksy se
                response[index] = '\0';
                printf("Partial response timeout. Received so far: %s\n", response);
                return true;
            }
        }
    }
}
// Tarkista LoRaWAN-yhteys
bool check_module_connection() {
    char response[128];
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        printf("Sending 'AT' command (attempt %d)...\n", attempt);
        send_uart_command("AT\r\n");

        if (read_uart_response(response, sizeof(response))) {
            if (strstr(response, "OK")) {
                printf("Connected to LoRa module.\n");
                return true;
            } else {
                printf("Unexpected response: %s\n", response);
            }
        }
        sleep_ms(1000);
    }
    printf("Module not responding. Returning to waiting.\n");
    return false;
}

// LoRaWAN Initialisation
bool loraInit() {
    char response[128];
    int attempts = 0;

    // Yritetään yhteyttä moduuliin
    while (attempts < MAX_RETRIES) {
        printf("Attempting to connect to LoRa module (attempt %d)...\n", attempts + 1);

        if (check_module_connection()) {
            printf("LoRa module connection successful.\n");

            // Käydään läpi kaikki LoRaWAN-komennot
            for (int i = 0; i < sizeof(lorawan) / sizeof(lorawan[0]); i++) {
                printf("Sending command: %s\n", lorawan[i].command);
                send_uart_command(lorawan[i].command);
                sleep_ms(lorawan[i].sleep_time);

                // Tarkistetaan vastaus
                if (!read_uart_response(response, sizeof(response))) {
                    printf("Command failed: %s\n", lorawan[i].command);
                    return false;
                }

                printf("Received response: %s\n", response);

                // Erityiskäsittely `AT+JOIN`-komennolle
                if (strcmp(lorawan[i].command, "AT+JOIN\r\n") == 0) {
                    if (!process_join_response(response)) {
                        printf("Join failed. Retrying...\n");
                        break; // Keskeytä ja yritä uudelleen
                    }
                } else {
                    printf("Command successful: %s\n", lorawan[i].command);
                }
            }

            // Jos kaikki komennot onnistuivat, palautetaan true
            return true;
        }

        attempts++;
        printf("Retrying LoRaWAN initialization...\n");
        sleep_ms(5000);
    }

    printf("LoRaWAN initialization failed after %d attempts.\n", MAX_RETRIES);
    return false;
}
bool process_join_response(const char *initial_response) {
    char response[128];

    // Tarkista ensimmäinen vastaus
    if (strstr(initial_response, "+JOIN: NORMAL")) {
        printf("Join normal. Waiting for further confirmation...\n");
    } else if (strstr(initial_response, "+JOIN: Start")) {
        printf("Join process started. Waiting for further responses...\n");
    } else {
        printf("Unexpected join response: %s\n", initial_response);
        return false;
    }

    // Jatka odottamista liittymisprosessin valmistumiseksi
    absolute_time_t timeout = make_timeout_time_ms(MSG_WAITING_TIME);
    while (absolute_time_diff_us(get_absolute_time(), timeout) > 0) {
        if (read_uart_response(response, sizeof(response))) {
            printf("Received response: %s\n", response);

            // Tarkista liittymisen hyväksyntä
            if (strstr(response, "+JOIN: Done")) {
                printf("Join successful. Network ready.\n");
                return true; // Liittyminen onnistui
            }

            // Tarkista liittymisen epäonnistuminen
            if (strstr(response, "+JOIN: FAIL")) {
                printf("Join failed. Retrying...\n");
                return false; // Liittyminen epäonnistui
            }
        }
    }

    printf("Join process timed out.\n");
    return false; // Aikakatkaisu
}



// Lähetä viesti LoRaWAN-moduulilla
bool loraMessage(const char *message, size_t msg_size, char *return_message) {
    char command[128];
    snprintf(command, sizeof(command), "AT+MSG=\"%s\"\r\n", message);

    send_uart_command(command);
    if (!read_uart_response(return_message, 128)) {
        printf("Failed to send message.\n");
        return false;
    }

    printf("Message sent successfully: %s\n", return_message);
    return true;
}