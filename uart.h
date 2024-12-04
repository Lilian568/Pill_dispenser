//
// Created by lina on 12/2/2024.
//
//this file includes 2 ring buffer tx and rx to handle transmitted and received data, defined in ring_buffer.h
#ifndef UART_H
#define UART_H

#endif //UART_H
#ifndef UART_IRQ_UART_H
#define UART_IRQ_UART_H

#include "ring_buffer.h"

void uart_setup(int uart_nr, int tx_pin, int rx_pin, int speed); // Configures UART with parameters
int uart_read(int uart_nr, uint8_t *buffer, int size); //Reads data from the UART rx
int uart_write(int uart_nr, const uint8_t *buffer, int size); //writes data to the UART tx
int uart_send(int uart_nr, const char *str); //sends a string of data via UART

typedef struct {
    ring_buffer tx;
    ring_buffer rx;
    uart_inst_t *uart;
    int irqn;
    irq_handler_t handler;
} uart_t;
uart_t *uart_get_handle(int uart_nr);

#endif
