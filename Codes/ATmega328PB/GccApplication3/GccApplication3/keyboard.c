/*
 * keyboard.c
 *
 * Created: 15.11.2026 18:05:38
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "keyboard.h"

#define KEY_1_PIN PE0
#define KEY_2_PIN PE1
#define KEY_3_PIN PE2
#define KEY_4_PIN PE3

#define KEYBOARD_MASK 0x0F
#define DEBOUNCE_DELAY_MS 20

static uint8_t key_locked = 0;

void keyboard_init(void)
{
	// Configure PE0-PE3 as inputs
	DDRE &= ~KEYBOARD_MASK;

	// Enable internal pull-up resistors
	PORTE |= KEYBOARD_MASK;

	// Reset keyboard state
	key_locked = 0;
}

static uint8_t keyboard_read_pins(void)
{
	// Read active-low buttons
	return (uint8_t)(~PINE) & KEYBOARD_MASK;
}

keyboard_key_t keyboard_get_key(void)
{
	uint8_t pressed_keys = keyboard_read_pins();

	// Wait for all buttons to be released
	if (key_locked)
	{
		if (pressed_keys == 0)
		{
			// Debounce button release
			_delay_ms(DEBOUNCE_DELAY_MS);

			if (keyboard_read_pins() == 0)
			{
				key_locked = 0;
			}
		}

		return KEY_NONE;
	}

	// No button pressed
	if (pressed_keys == 0)
	{
		return KEY_NONE;
	}

	// Debounce button press
	_delay_ms(DEBOUNCE_DELAY_MS);

	pressed_keys = keyboard_read_pins();

	if (pressed_keys == 0)
	{
		return KEY_NONE;
	}

	// Prevent repeated events while holding a button
	key_locked = 1;

	// Identify the pressed button
	if (pressed_keys & (1 << KEY_1_PIN))
	{
		return KEY_1;
	}

	if (pressed_keys & (1 << KEY_2_PIN))
	{
		return KEY_2;
	}

	if (pressed_keys & (1 << KEY_3_PIN))
	{
		return KEY_3;
	}

	if (pressed_keys & (1 << KEY_4_PIN))
	{
		return KEY_4;
	}

	return KEY_NONE;
}


