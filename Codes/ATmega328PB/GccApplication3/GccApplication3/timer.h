/*
 * timer.h
 *
 * Created: 15.11.2026 14:02:05
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

void timer1_init(void);

void timer_start(void);
void timer_stop(void);
void timer_toggle(void);
void timer_reset(void);

uint8_t timer_is_running(void);
uint32_t timer_get_seconds(void);
uint8_t timer_take_tick(void);

#endif