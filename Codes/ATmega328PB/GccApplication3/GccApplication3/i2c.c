/*
 * i2c.c
 *
 * Created: 15.11.2026 14:07:11
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/twi.h>
#include "i2c.h"

#define I2C_FREQUENCY 50000UL
#define I2C_TIMEOUT 30000U

static uint8_t i2c_wait(void)
{
	for (uint16_t attempt = 0; attempt < I2C_TIMEOUT; attempt++)
	{
		if (TWCR0 & (1 << TWINT))
		{
			return 1;
		}
	}

	return 0;
}

static uint8_t i2c_status(void)
{
	return TWSR0 & 0xF8;
}

void i2c_init(void)
{
	// Configure SDA and SCL as inputs
	DDRC &= ~((1 << PC4) | (1 << PC5));

	// Disable internal pull-up resistors
	PORTC &= ~((1 << PC4) | (1 << PC5));

	// Set prescaler to 1
	TWSR0 = 0;

	// Set I2C clock frequency to 100 kHz
	TWBR0 = (uint8_t)((F_CPU / I2C_FREQUENCY - 16UL) / 2UL);

	// Enable TWI interface
	TWCR0 = (1 << TWEN);
}

static uint8_t i2c_start(uint8_t address, uint8_t read)
{
	// Send START condition
	TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	if (!i2c_wait())
	{
		return 0;
	}

	uint8_t status = i2c_status();

	if (status != TW_START && status != TW_REP_START)
	{
		return 0;
	}

	// Send device address and read/write bit
	TWDR0 = (address << 1) | (read ? 1 : 0);

	TWCR0 = (1 << TWINT) | (1 << TWEN);

	if (!i2c_wait())
	{
		return 0;
	}

	status = i2c_status();

	if (read)
	{
		return status == TW_MR_SLA_ACK;
	}

	return status == TW_MT_SLA_ACK;
}

static void i2c_stop(void)
{
	// Send STOP condition
	TWCR0 = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

	// Wait until STOP condition is completed
	for (uint16_t attempt = 0; attempt < I2C_TIMEOUT; attempt++)
	{
		if (!(TWCR0 & (1 << TWSTO)))
		{
			break;
		}
	}
}

static uint8_t i2c_write_byte(uint8_t data)
{
	TWDR0 = data;

	TWCR0 = (1 << TWINT) | (1 << TWEN);

	if (!i2c_wait())
	{
		return 0;
	}

	return i2c_status() == TW_MT_DATA_ACK;
}

static uint8_t i2c_read_byte(uint8_t *data, uint8_t acknowledge)
{
	if (acknowledge)
	{
		TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	}
	else
	{
		TWCR0 = (1 << TWINT) | (1 << TWEN);
	}

	if (!i2c_wait())
	{
		return 0;
	}

	uint8_t expected_status = acknowledge ?
	TW_MR_DATA_ACK :
	TW_MR_DATA_NACK;

	if (i2c_status() != expected_status)
	{
		return 0;
	}

	*data = TWDR0;

	return 1;
}

uint8_t i2c_write_register(uint8_t address,
uint8_t reg,
uint8_t value)
{
	uint8_t success = 0;

	if (!i2c_start(address, 0))
	{
		goto finish;
	}

	if (!i2c_write_byte(reg))
	{
		goto finish;
	}

	if (!i2c_write_byte(value))
	{
		goto finish;
	}

	success = 1;

	finish:
	i2c_stop();
	return success;
}

uint8_t i2c_read_registers(uint8_t address,
uint8_t reg,
uint8_t *buffer,
uint8_t length)
{
	uint8_t success = 0;

	if (buffer == 0 || length == 0)
	{
		return 0;
	}

	// Select the register address
	if (!i2c_start(address, 0))
	{
		goto finish;
	}

	if (!i2c_write_byte(reg))
	{
		goto finish;
	}

	// Send repeated START for reading
	if (!i2c_start(address, 1))
	{
		goto finish;
	}

	for (uint8_t index = 0; index < length; index++)
	{
		uint8_t acknowledge = (index < length - 1);

		if (!i2c_read_byte(&buffer[index], acknowledge))
		{
			goto finish;
		}
	}

	success = 1;

	finish:
	i2c_stop();
	return success;
}

uint8_t i2c_read_register(uint8_t address,
uint8_t reg,
uint8_t *value)
{
	return i2c_read_registers(address, reg, value, 1);
}

