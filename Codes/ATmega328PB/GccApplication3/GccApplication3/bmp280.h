/*
 * bmp280.h
 *
 * Created: 15.11.2026 14:08:47
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef BMP280_H_
#define BMP280_H_

#include <stdint.h>

typedef struct
{
	int32_t temperature_centi_c;
	uint32_t pressure_pa;
} bmp280_data_t;

uint8_t bmp280_init(void);
uint8_t bmp280_read(bmp280_data_t *measurement);

#endif