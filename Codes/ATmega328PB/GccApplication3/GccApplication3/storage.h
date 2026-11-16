/*
 * storage.h
 *
 * Created: 15.11.2026 14:27:32
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 

#ifndef STORAGE_H_
#define STORAGE_H_

#include <stdint.h>
#include "ff.h"

uint8_t storage_init(void);

uint8_t storage_append_measurement(uint32_t seconds,
int32_t temperature_centi_c,
uint32_t pressure_pa);

uint8_t storage_is_ready(void);
FRESULT storage_get_last_error(void);
uint8_t storage_close(void);

#endif
