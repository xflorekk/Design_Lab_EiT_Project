/*
 * 7seg.h
 *
 * Created: 15.11.2026 14:03:39
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

void display_init(void);

void display_show_number(uint16_t number);
void display_show_time(uint32_t seconds);
void display_show_temperature(int32_t temperature_centi_c);
void display_show_pressure(uint32_t pressure_pa);

void display_show_error(void);
void display_clear(void);

#endif