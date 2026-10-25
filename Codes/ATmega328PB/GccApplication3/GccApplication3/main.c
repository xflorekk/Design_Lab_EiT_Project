/*
 * GccApplication3.c
 *
 * Created: 10/24/2026 19:33:08
 * Author : Daniel Florek, Jakub Wasowicz, Filip Dudek
 */ 
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/wdt.h>

#define BAUDRATE 9600
#define MYUBR ((F_CPU / (16UL * BAUDRATE)) - 1)

volatile uint8_t timerflag = 0;
volatile uint32_t seconds_counter = 0;

volatile char rx_data = 0;
volatile uint8_t rx_flag = 0;
volatile uint8_t timer_run = 0;
volatile uint8_t timer_res = 0;
volatile uint8_t auto_count = 0;

void sw0_init(void) {
	DDRB &= ~(1 << PB7);
	PORTB |= (1 << PB7);
}

void performHardRes(void) {
	usart0_print("Hardware reset called\r\n");
	cli();
	wdt_enable(WDTO_15MS);
	while(1);
}

void usart0_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart0_transmit(unsigned char data) {
	while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = data;
}

void usart0_print(const char *str) {
	while (*str) {
		usart0_transmit(*str++);
	}
}

void usart0_control() {
	
}

void timer1_init(void) {
	TCCR1B |= (1 << WGM12);
	OCR1A = 62499;
	TIMSK1 |= (1 << OCIE1A);
	TCCR1B |= (1 << CS12);
}

ISR(USART0_RX_vect) {
	rx_data = UDR0;
	rx_flag = 1;
}

ISR(TIMER1_COMPA_vect) {
	if (timer_run) {
		seconds_counter++;
		timerflag = 1;
	}
}
int main(void)
{
	MCUSR = 0;
	wdt_disable();
	
    char buffer[50];
	
	usart0_init(MYUBR);
	timer1_init();
	sei();
	sw0_init();
	
	usart0_print("SYSTEM STARTED!\r\n");
	usart0_print("Control: \r\n");
	usart0_print("Start/Stop: 's'\r\n");
	usart0_print("Reset counter: 'r'\r\n");
	usart0_print("Show current state of counter: 'p'\r\n");
	usart0_print("Auto counting mode: 'm'\r\n");
	
    while (1){
		if(!(PINB & (1 << PB7))) {
			performHardRes();
		}
		
		if (timerflag) {
			timerflag = 0;
			if (auto_count) {
				snprintf(buffer, sizeof(buffer), "Time from the start: %lu s\r\n", (unsigned long)seconds_counter);
				usart0_print(buffer);
			}
		}
		
		if (rx_flag) {
			rx_flag = 0;
			switch(rx_data) {
				case 'm':
				case 'M':
					auto_count = !auto_count;
					if (auto_count) {
						usart0_print("Auto counting has been enabled\r\n");
					} else {
						usart0_print("Counting in background enabled, to show current state of counter press 'p'\r\n");
					}
					break;
					
				case 's':
				case 'S':
					timer_run = !timer_run;
					if(timer_run) {
						usart0_print("Timer started\r\n");
					} else {
						usart0_print("Timer stopped\r\n");
					}
					break;
					
				case 'r':
				case 'R':
					seconds_counter = 0;
					usart0_print("Counter value now is 0 s\r\n");
					break;
					
				case 'p':
				case 'P':
					snprintf(buffer, sizeof(buffer), "Time: %lu s\r\n", (unsigned long)seconds_counter);
					usart0_print(buffer);
					break;
					
				default:
					snprintf(buffer, sizeof(buffer), "Unkown command: '%c'\r\n", rx_data);
					usart0_print(buffer);
					break;
			}
			
		}
    }
}

