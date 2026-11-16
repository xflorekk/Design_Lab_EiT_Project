/*
 * 7seg.c
 *
 * Created: 15.11.2026 14:03:54
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>
#include "7seg.h"

#define DISPLAY_SEGMENTS_ACTIVE_LOW 1
#define DISPLAY_DIGITS_ACTIVE_LOW   1

#define SEGMENT_D_MASK 0xFC
#define SEGMENT_B_MASK 0x03
#define DIGIT_MASK     0x0F

#define SEGMENT_DP     0x80

#define SYMBOL_BLANK   10
#define SYMBOL_DASH    11

// Segment patterns: A, B, C, D, E, F, G
static const uint8_t segment_patterns[12] =
{
	0x3F, // 0
	0x06, // 1
	0x5B, // 2
	0x4F, // 3
	0x66, // 4
	0x6D, // 5
	0x7D, // 6
	0x07, // 7
	0x7F, // 8
	0x6F, // 9
	0x00, // Blank
	0x40  // Dash
};

static volatile uint8_t display_buffer[4] = {0};
static volatile uint8_t current_digit = 0;

static void display_disable_digits(void)
{
	#if DISPLAY_DIGITS_ACTIVE_LOW
	PORTC |= DIGIT_MASK;
	#else
	PORTC &= ~DIGIT_MASK;
	#endif
}

static void display_enable_digit(uint8_t digit)
{
	uint8_t mask = (1 << digit);

	#if DISPLAY_DIGITS_ACTIVE_LOW
	PORTC &= ~mask;
	#else
	PORTC |= mask;
	#endif
}

static void display_set_segments(uint8_t pattern)
{
	uint8_t port_d_data =
	(pattern & 0x3F) << 2;

	uint8_t port_b_data =
	(pattern >> 6) & 0x03;

	#if DISPLAY_SEGMENTS_ACTIVE_LOW
	port_d_data ^= SEGMENT_D_MASK;
	port_b_data ^= SEGMENT_B_MASK;
	#endif

	// Preserve UART pins PD0 and PD1
	PORTD = (PORTD & ~SEGMENT_D_MASK) |
	port_d_data;

	// Preserve SPI pins PB2-PB5
	PORTB = (PORTB & ~SEGMENT_B_MASK) |
	port_b_data;
}

static void display_set_buffer(const uint8_t *patterns)
{
	// Update all four digits atomically
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		for (uint8_t index = 0; index < 4; index++)
		{
			display_buffer[index] = patterns[index];
		}
	}
}

void display_init(void)
{
	// Disable all digits before enabling outputs
	#if DISPLAY_DIGITS_ACTIVE_LOW
	PORTC |= DIGIT_MASK;
	#else
	PORTC &= ~DIGIT_MASK;
	#endif

	// Configure digit selection pins PC0-PC3
	DDRC |= DIGIT_MASK;

	// Configure segment pins PD2-PD7
	DDRD |= SEGMENT_D_MASK;

	// Configure segment pins PB0-PB1
	DDRB |= SEGMENT_B_MASK;

	display_disable_digits();

	// Switch all segments off
	display_set_segments(0x00);

	display_show_number(0);

	// Configure Timer0 in CTC mode
	TCCR0A = (1 << WGM01);
	TCCR0B = 0;

	TCNT0 = 0;

	// Generate an interrupt every 1 ms at 16 MHz
	OCR0A = 249;

	// Enable compare match interrupt
	TIMSK0 |= (1 << OCIE0A);

	// Start Timer0 with prescaler 64
	TCCR0B = (1 << CS01) | (1 << CS00);
}

ISR(TIMER0_COMPA_vect)
{
	// Disable all digits before changing segments
	display_disable_digits();

	// Set segments for the current digit
	display_set_segments(display_buffer[current_digit]);

	// Enable the selected digit
	display_enable_digit(current_digit);

	// Select the next digit
	current_digit++;

	if (current_digit >= 4)
	{
		current_digit = 0;
	}
}

void display_show_number(uint16_t number)
{
	uint8_t patterns[4];

	number %= 10000U;

	// Convert decimal number to four digits
	for (int8_t index = 3; index >= 0; index--)
	{
		patterns[index] =
		segment_patterns[number % 10U];

		number /= 10U;
	}

	display_set_buffer(patterns);
}

void display_show_time(uint32_t seconds)
{
	// Display MMSS format
	uint16_t minutes =
	(uint16_t)((seconds / 60UL) % 100UL);

	uint8_t remaining_seconds =
	(uint8_t)(seconds % 60UL);

	uint16_t value =
	minutes * 100U + remaining_seconds;

	display_show_number(value);
}

void display_show_temperature(int32_t temperature_centi_c)
{
	uint8_t patterns[4];

	uint8_t negative =
	(temperature_centi_c < 0);

	uint32_t absolute_temperature;

	if (negative)
	{
		absolute_temperature =
		(uint32_t)(-(temperature_centi_c + 1)) + 1UL;
	}
	else
	{
		absolute_temperature =
		(uint32_t)temperature_centi_c;
	}

	// Round temperature to one decimal place
	uint32_t temperature_tenths =
	(absolute_temperature + 5UL) / 10UL;

	// Check display range
	if (temperature_tenths > 9999UL ||
	(negative && temperature_tenths > 999UL))
	{
		display_show_error();
		return;
	}

	// Generate four decimal digits
	for (int8_t index = 3; index >= 0; index--)
	{
		patterns[index] =
		segment_patterns[temperature_tenths % 10UL];

		temperature_tenths /= 10UL;
	}

	// Add decimal point before the last digit
	patterns[2] |= SEGMENT_DP;

	// Remove unnecessary leading zeros
	if (absolute_temperature < 10000UL)
	{
		patterns[0] = segment_patterns[SYMBOL_BLANK];
	}

	if (absolute_temperature < 1000UL)
	{
		patterns[1] = segment_patterns[SYMBOL_BLANK];
	}

	// Add negative sign if required
	if (negative)
	{
		if (absolute_temperature < 1000UL)
		{
			patterns[1] = segment_patterns[SYMBOL_DASH];
		}
		else
		{
			patterns[0] = segment_patterns[SYMBOL_DASH];
		}
	}

	display_set_buffer(patterns);
}

void display_show_pressure(uint32_t pressure_pa)
{
	// Convert pascals to rounded hectopascals
	uint32_t pressure_hpa =
	(pressure_pa + 50UL) / 100UL;

	if (pressure_hpa > 9999UL)
	{
		display_show_error();
		return;
	}

	display_show_number((uint16_t)pressure_hpa);
}

void display_show_error(void)
{
	uint8_t patterns[4] =
	{
		0x40, 0x40, 0x40, 0x40
	};

	display_set_buffer(patterns);
}

void display_clear(void)
{
	uint8_t patterns[4] = {0, 0, 0, 0};

	display_set_buffer(patterns);
}