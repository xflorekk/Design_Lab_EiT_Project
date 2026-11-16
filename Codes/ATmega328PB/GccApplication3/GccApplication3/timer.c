/*
 * timer.c
 *
 * Created: 15.11.2026 14:01:29
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "timer.h"

static volatile uint32_t seconds_counter = 0;
static volatile uint8_t timer_run = 0;
static volatile uint8_t tick_pending = 0;

void timer1_init(void)
{
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	// Mode CTC, interrupt every second
	OCR1A = 62499;

	TCCR1B = (1 << WGM12) | (1 << CS12);
	TIMSK1 |= (1 << OCIE1A);
}

void timer_start(void)
{
	timer_run = 1;
}

void timer_stop(void)
{
	timer_run = 0;
}

void timer_toggle(void)
{
	timer_run = !timer_run;
}

void timer_reset(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		seconds_counter = 0;
	}
}

uint8_t timer_is_running(void)
{
	return timer_run;
}

uint32_t timer_get_seconds(void)
{
	uint32_t seconds;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		seconds = seconds_counter;
	}

	return seconds;
}

uint8_t timer_take_tick(void)
{
	uint8_t result = 0;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (tick_pending > 0)
		{
			tick_pending--;
			result = 1;
		}
	}

	return result;
}

ISR(TIMER1_COMPA_vect)
{
	if (timer_run)
	{
		seconds_counter++;
	}

	if (tick_pending < 255)
	{
		tick_pending++;
	}
}
