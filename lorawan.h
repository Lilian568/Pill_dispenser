#ifndef LORAWAN
#define LORAWAN

#include <stdio.h>
#include <stdbool.h>

#define UART_ID uart1
#define TX_PIN 4
#define RX_PIN 5
#define BAUD_RATE 9600
#define TIMEOUT_US 500000
#define MAX_RETRIES 5

#define STD_WAITING_TIME 500
#define LONG_TIMEOUT_US 20000
#define VERY_LONG_TIMEOUT_US 10000000000
#define MSG_WAITING_TIME 10000

#define STRLEN 128

typedef struct lorawan_item_ {
    char command[STRLEN];
    uint sleep_time;
} lorawan_item;

void send_uart_command(const char *command);
bool read_uart_response(char *response, size_t response_len);
bool check_module_connection();
bool loraInit();
bool loraMessage(const char *message, size_t msg_size, char *return_message);
void loraWanInit();
bool process_join_response(const char *initial_response);
#endif