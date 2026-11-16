/*
 * uart.c
 *
 * ATmega328PB UART0 driver with a small interrupt-driven RX buffer.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "uart.h"

#define UART_RX_BUFFER_SIZE 16U
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1U)

#if (UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK) != 0
#error "UART_RX_BUFFER_SIZE must be a power of two"
#endif

static volatile char rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

void usart0_init(uint16_t ubrr)
{
    rx_head = 0;
    rx_tail = 0;

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Normal-speed UART, 8 data bits, no parity, 1 stop bit
    UCSR0A = 0;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart0_transmit(uint8_t data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

void usart0_print(const char *text)
{
    // Keep RAM-string support for existing SD and sensor modules
    while (*text != '\0')
    {
        usart0_transmit((uint8_t)*text++);
    }
}

void usart0_print_P(const char *flash_text)
{
    // Read constant messages directly from program memory
    uint8_t character;

    while ((character = pgm_read_byte(flash_text++)) != 0)
    {
        usart0_transmit(character);
    }
}

uint8_t usart0_read_char(char *data)
{
    if (data == 0 || rx_head == rx_tail)
    {
        return 0;
    }

    *data = rx_buffer[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1U) & UART_RX_BUFFER_MASK);

    return 1;
}

ISR(USART0_RX_vect)
{
    char received_char = (char)UDR0;
    uint8_t next_head = (uint8_t)((rx_head + 1U) & UART_RX_BUFFER_MASK);

    // Drop incoming bytes when the receive buffer is full
    if (next_head != rx_tail)
    {
        rx_buffer[rx_head] = received_char;
        rx_head = next_head;
    }
}
