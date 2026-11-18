/*
 * i2c.h
 *
 * Created: 15.11.2026 14:07:30
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

void i2c_init(void);

uint8_t i2c_write_register(uint8_t address,
uint8_t reg,
uint8_t value);

uint8_t i2c_read_register(uint8_t address,
uint8_t reg,
uint8_t *value);

uint8_t i2c_read_registers(uint8_t address,
uint8_t reg,
uint8_t *buffer,
uint8_t length);

#endif 