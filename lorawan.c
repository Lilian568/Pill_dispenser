
#include <stdio.h>
#include <string.h>
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "uart.h"
#include "lorawan.h"

#ifdef DEBUG_PRINT
#define DEBUG_PRINT(f_,...) printf((f_), ##__VA_ARGS__)
#else
#define DEBUG_PRINT(f_, ...)
#endif

static const int uart_nr = UART_NR;
static lorawan_item lorawan[] = {{"AT\r\n", "+AT: OK\r\n", STD_WAITING_TIME},
                                 {"AT+MODE=LWOTAA\r\n", "+MODE: LWOTAA\r\n", STD_WAITING_TIME},
                                 {"AT+KEY=APPKEY,\"307fb94b705bd61559329b239686f653\"\r\n", "+KEY: 307fb94b705bd61559329b239686f653\r\n", STD_WAITING_TIME},  // Linh
                                 //{"AT+KEY=APPKEY,\"075c56f402aef60abcf4a9a5e943aa81\"\r\n", "+KEY: APPKEY 075c56f402aef60abcf4a9a5e943aa81\r\n", STD_WAITING_TIME},  // Vipe
                                 //{"AT+KEY=APPKEY,\"ef63470b1d60c8afb0b98261e1383392\"\r\n", "+KEY: APPKEY ef63470b1d60c8afb0b98261e1383392\r\n", STD_WAITING_TIME},  // Tomi
                                 {"AT+CLASS=A\r\n", "+CLASS: A\r\n", STD_WAITING_TIME},
                                 {"AT+PORT=8\r\n", "+PORT: 8\r\n", STD_WAITING_TIME},
                                 {"AT+JOIN\r\n", "Network joined\r\n", MSG_WAITING_TIME}};
//initialises the uart and set up llorawan communication
bool loraInit() {
    char return_message[STRLEN];
    int lorawanState = 0;
    uart_setup(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE);


    while(true) {
        for(;lorawanState < (sizeof(lorawan)/sizeof(lorawan[0])-1);lorawanState++) {
            if (false ==retvalChecker(lorawanState)) {
                return false;
            }
        }
        if (true == loraCommunication(lorawan[lorawanState].command,lorawan[lorawanState].sleep_time, return_message)) {
            if(strstr(return_message, lorawan[lorawanState].retval) != NULL) {
                return true;
            }
        }
        return false;
    }
}


//sends a command via UART and waits for a response.
bool loraCommunication(const char* command, const uint sleep_time, char* str) {
    uart_send(uart_nr, command);
    sleep_ms(sleep_time);
    int pos = uart_read(UART_NR, (uint8_t *) str, STRLEN - 1);
    if (pos > 0) {
        str[pos] = '\0';
        return true;
    }
    return false;
}

//Send a custom message using the LoRaWAN device.
bool loraMessage(const char* message, size_t msg_size, char* return_message) {
    const char start_tag[] = "AT+MSG=\"";
    const char end_tag[] = "\"\r\n";
    char lorawan_message[STRLEN];

    if (msg_size > STRLEN-strlen(start_tag)-strlen(end_tag)-1) {
        return false;
    }
    strcpy(lorawan_message, start_tag);
    strncpy(&lorawan_message[strlen(start_tag)], message, STRLEN - strlen(start_tag)- strlen(end_tag)-1);
    strcat(lorawan_message, end_tag);
    lorawan_message[STRLEN-1] = '\0';
    if(true == loraCommunication(lorawan_message, MSG_WAITING_TIME, return_message)) {
        return true;
    } else {
        return false;
    }
}
//
bool retvalChecker(const int index) {
    char return_message[STRLEN];

    if(true == loraCommunication(lorawan[index].command, lorawan[index].sleep_time, return_message)) {
        if(strcmp(lorawan[index].retval, return_message) == 0) {
            DEBUG_PRINT("Commparison same for: %s\n", return_message)
            return true;
        } else {
            DEBUG_PRINT("Comparison doesn't match, return message: %s lorawan [%d].retval: %s\n", return_message, index, lorawan[index].retval)
            DEBUG_PRINT("Exit lora communication.\n");
            return false;
        }
    } else {
        DEBUG_PRINT("[%d]Command failed, exit lora communication.\n", index);
        return false;
    }
}

