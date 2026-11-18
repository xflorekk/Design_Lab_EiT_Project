/*
 * bmp280.c
 *
 * Created: 15.11.2026 14:09:08
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdint.h>
#include <util/delay.h>
#include "bmp280.h"
#include "i2c.h"

#define BMP280_ADDRESS_1  0x76
#define BMP280_ADDRESS_2  0x77

#define BMP280_CHIP_ID    0x58

#define BMP280_REG_ID        0xD0
#define BMP280_REG_CALIB     0x88
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG    0xF5
#define BMP280_REG_PRESS     0xF7

typedef struct
{
	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;

	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;

} bmp280_calibration_t;

static bmp280_calibration_t calibration;
static uint8_t sensor_address = 0;
static uint8_t sensor_initialized = 0;
static int32_t fine_temperature = 0;

static uint16_t bmp280_get_u16(const uint8_t *data)
{
	return (uint16_t)data[0] |
	((uint16_t)data[1] << 8);
}

static int16_t bmp280_get_s16(const uint8_t *data)
{
	return (int16_t)bmp280_get_u16(data);
}

static uint8_t bmp280_read_calibration(void)
{
	uint8_t data[24];

	// Read factory calibration coefficients
	if (!i2c_read_registers(sensor_address,
	BMP280_REG_CALIB,
	data, 24))
	{
		return 0;
	}

	calibration.dig_T1 = bmp280_get_u16(&data[0]);
	calibration.dig_T2 = bmp280_get_s16(&data[2]);
	calibration.dig_T3 = bmp280_get_s16(&data[4]);

	calibration.dig_P1 = bmp280_get_u16(&data[6]);
	calibration.dig_P2 = bmp280_get_s16(&data[8]);
	calibration.dig_P3 = bmp280_get_s16(&data[10]);
	calibration.dig_P4 = bmp280_get_s16(&data[12]);
	calibration.dig_P5 = bmp280_get_s16(&data[14]);
	calibration.dig_P6 = bmp280_get_s16(&data[16]);
	calibration.dig_P7 = bmp280_get_s16(&data[18]);
	calibration.dig_P8 = bmp280_get_s16(&data[20]);
	calibration.dig_P9 = bmp280_get_s16(&data[22]);

	if (calibration.dig_T1 == 0 ||
	calibration.dig_P1 == 0)
	{
		return 0;
	}

	return 1;
}

uint8_t bmp280_init(void)
{
	uint8_t chip_id = 0;

	sensor_initialized = 0;
	sensor_address = 0;

	// Detect the sensor I2C address
	if (i2c_read_register(BMP280_ADDRESS_1,
	BMP280_REG_ID,
	&chip_id) &&
	chip_id == BMP280_CHIP_ID)
	{
		sensor_address = BMP280_ADDRESS_1;
	}
	else if (i2c_read_register(BMP280_ADDRESS_2,
	BMP280_REG_ID,
	&chip_id) &&
	chip_id == BMP280_CHIP_ID)
	{
		sensor_address = BMP280_ADDRESS_2;
	}
	else
	{
		return 0;
	}

	// Read calibration coefficients
	if (!bmp280_read_calibration())
	{
		return 0;
	}

	// Set standby time to 1000 ms
	if (!i2c_write_register(sensor_address,
	BMP280_REG_CONFIG,
	0xA0))
	{
		return 0;
	}

	// Temperature x1, pressure x1, normal mode
	if (!i2c_write_register(sensor_address,
	BMP280_REG_CTRL_MEAS,
	0x27))
	{
		return 0;
	}

	_delay_ms(10);

	sensor_initialized = 1;

	return 1;
}

static int32_t bmp280_compensate_temperature(
int32_t raw_temperature)
{
	int32_t var1;
	int32_t var2;

	var1 = ((((raw_temperature >> 3) -
	((int32_t)calibration.dig_T1 << 1))) *
	((int32_t)calibration.dig_T2)) >> 11;

	var2 = (((((raw_temperature >> 4) -
	((int32_t)calibration.dig_T1)) *
	((raw_temperature >> 4) -
	((int32_t)calibration.dig_T1))) >> 12) *
	((int32_t)calibration.dig_T3)) >> 14;

	fine_temperature = var1 + var2;

	return (fine_temperature * 5 + 128) >> 8;
}

static uint32_t bmp280_compensate_pressure(
int32_t raw_pressure)
{
	int32_t var1;
	int32_t var2;
	uint32_t pressure;

	var1 = (fine_temperature >> 1) - 64000;

	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) *
	((int32_t)calibration.dig_P6);

	var2 += (var1 * ((int32_t)calibration.dig_P5)) * 2;

	var2 = (var2 >> 2) +
	((int32_t)calibration.dig_P4 * 65536L);

	var1 = (((calibration.dig_P3 *
	(((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
	+ (((int32_t)calibration.dig_P2 * var1) >> 1))
	>> 18;

	var1 = ((32768L + var1) *
	((int32_t)calibration.dig_P1)) >> 15;

	// Prevent division by zero
	if (var1 == 0)
	{
		return 0;
	}

	pressure = ((uint32_t)(1048576L -
	raw_pressure - (var2 >> 12))) * 3125UL;

	if (pressure < 0x80000000UL)
	{
		pressure = (pressure << 1) / (uint32_t)var1;
	}
	else
	{
		pressure = (pressure / (uint32_t)var1) * 2;
	}

	var1 = ((int32_t)calibration.dig_P9 *
	(int32_t)(((pressure >> 3) *
	(pressure >> 3)) >> 13)) >> 12;

	var2 = ((int32_t)(pressure >> 2) *
	(int32_t)calibration.dig_P8) >> 13;

	pressure = (uint32_t)((int32_t)pressure +
	((var1 + var2 +
	calibration.dig_P7) >> 4));

	return pressure;
}

uint8_t bmp280_read(bmp280_data_t *measurement)
{
	uint8_t data[6];
	int32_t raw_temperature;
	int32_t raw_pressure;
	uint32_t pressure;

	if (!sensor_initialized || measurement == 0)
	{
		return 0;
	}

	// Read pressure and temperature registers
	if (!i2c_read_registers(sensor_address,
	BMP280_REG_PRESS,
	data, 6))
	{
		return 0;
	}

	raw_pressure =
	((int32_t)data[0] << 12) |
	((int32_t)data[1] << 4) |
	((int32_t)data[2] >> 4);

	raw_temperature =
	((int32_t)data[3] << 12) |
	((int32_t)data[4] << 4) |
	((int32_t)data[5] >> 4);

	// Check for skipped measurements
	if (raw_temperature == 0x80000L ||
	raw_pressure == 0x80000L)
	{
		return 0;
	}

	// Apply temperature compensation first
	measurement->temperature_centi_c =
	bmp280_compensate_temperature(raw_temperature);

	// Apply pressure compensation
	pressure = bmp280_compensate_pressure(raw_pressure);

	if (pressure == 0)
	{
		return 0;
	}

	measurement->pressure_pa = pressure;

	return 1;
}
