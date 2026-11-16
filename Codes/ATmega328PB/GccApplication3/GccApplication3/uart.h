/*
 * uart.h
 *
 * Created: 15.11.2026 14:11:30
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void usart0_init(uint16_t ubrr);
void usart0_transmit(uint8_t data);
void usart0_print(const char *text);
void usart0_print_P(const char *flash_text);
uint8_t usart0_read_char(char *data);

#endif