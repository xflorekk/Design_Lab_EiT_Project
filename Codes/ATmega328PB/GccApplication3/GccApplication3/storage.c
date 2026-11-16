/*
 * storage.c
 *
 * Created: 15.11.2026 14:27:23
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#include <stdint.h>
#include <stdio.h>

#include "ff.h"
#include "storage.h"

#define STORAGE_DRIVE "0:"
#define STORAGE_FILENAME "0:/LOG.CSV"
#define STORAGE_BUFFER_SIZE 64

static FATFS file_system;
static FIL log_file;

static uint8_t storage_mounted = 0;
static uint8_t storage_open = 0;
static FRESULT last_error = FR_OK;

static uint8_t storage_write_data(const char *data,
UINT length)
{
	UINT bytes_written = 0;

	// Write data to the currently opened file
	last_error = f_write(&log_file,
	data,
	length,
	&bytes_written);

	if (last_error != FR_OK)
	{
		return 0;
	}

	// Verify that all bytes were written
	if (bytes_written != length)
	{
		last_error = FR_DISK_ERR;
		return 0;
	}

	return 1;
}

uint8_t storage_init(void)
{
	static const char csv_header[] =
	"Time_s;Temperature_C;Pressure_hPa\r\n";

	if (storage_open)
	{
		return 1;
	}

	// Mount the FAT file system
	last_error = f_mount(&file_system,
	STORAGE_DRIVE,
	1);

	if (last_error != FR_OK)
	{
		return 0;
	}

	storage_mounted = 1;

	// Open the existing file or create a new one
	last_error = f_open(&log_file,
	STORAGE_FILENAME,
	FA_WRITE | FA_OPEN_ALWAYS);

	if (last_error != FR_OK)
	{
		goto init_error;
	}

	storage_open = 1;

	// Write CSV header only if the file is empty
	if (f_size(&log_file) == 0)
	{
		if (!storage_write_data(
		csv_header,
		(UINT)(sizeof(csv_header) - 1)))
		{
			goto init_error;
		}

		last_error = f_sync(&log_file);

		if (last_error != FR_OK)
		{
			goto init_error;
		}
	}

	// Move the file pointer to the end
	last_error = f_lseek(&log_file,
	f_size(&log_file));

	if (last_error != FR_OK)
	{
		goto init_error;
	}

	return 1;

	init_error:
	{
		FRESULT saved_error = last_error;

		if (storage_open)
		{
			f_close(&log_file);
			storage_open = 0;
		}

		if (storage_mounted)
		{
			f_mount(NULL, STORAGE_DRIVE, 0);
			storage_mounted = 0;
		}

		last_error = saved_error;
	}

	return 0;
}

uint8_t storage_append_measurement(uint32_t seconds,
int32_t temperature_centi_c,
uint32_t pressure_pa)
{
	char buffer[STORAGE_BUFFER_SIZE];
	int length;

	if (!storage_open)
	{
		last_error = FR_NOT_READY;
		return 0;
	}

	// Convert signed temperature to absolute value
	const char *temperature_sign =
	temperature_centi_c < 0 ? "-" : "";

	uint32_t absolute_temperature;

	if (temperature_centi_c < 0)
	{
		absolute_temperature =
		(uint32_t)(-(temperature_centi_c + 1)) + 1UL;
	}
	else
	{
		absolute_temperature =
		(uint32_t)temperature_centi_c;
	}

	// Format CSV record without floating-point operations
	length = snprintf(
	buffer,
	sizeof(buffer),
	"%lu;%s%lu.%02lu;%lu.%02lu\r\n",
	(unsigned long)seconds,
	temperature_sign,
	(unsigned long)(absolute_temperature / 100UL),
	(unsigned long)(absolute_temperature % 100UL),
	(unsigned long)(pressure_pa / 100UL),
	(unsigned long)(pressure_pa % 100UL)
	);

	if (length < 0 ||
	length >= (int)sizeof(buffer))
	{
		last_error = FR_INT_ERR;
		return 0;
	}

	// Append measurement to the CSV file
	if (!storage_write_data(buffer, (UINT)length))
	{
		return 0;
	}

	// Synchronize file data with the SD card
	last_error = f_sync(&log_file);

	if (last_error != FR_OK)
	{
		return 0;
	}

	return 1;
}

uint8_t storage_is_ready(void)
{
	return storage_open;
}

FRESULT storage_get_last_error(void)
{
	return last_error;
}

uint8_t storage_close(void)
{
	FRESULT close_result = FR_OK;
	FRESULT unmount_result = FR_OK;

	// Close the currently opened file
	if (storage_open)
	{
		close_result = f_close(&log_file);
		storage_open = 0;
	}

	// Unmount the filesystem
	if (storage_mounted)
	{
		unmount_result = f_mount(NULL,
		STORAGE_DRIVE,
		0);

		storage_mounted = 0;
	}

	last_error = close_result != FR_OK ?
	close_result : unmount_result;

	return last_error == FR_OK;
}
