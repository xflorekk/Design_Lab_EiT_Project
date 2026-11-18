/*
 * keyboard.h
 *
 * Created: 15.11.2026 18:05:51
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stdint.h>

typedef enum
{
	KEY_NONE = 0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4
} keyboard_key_t;

void keyboard_init(void);
keyboard_key_t keyboard_get_key(void);

#endif