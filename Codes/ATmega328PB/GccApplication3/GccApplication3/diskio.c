/*
 * diskio.c
 *
 * Created: 15.11.2026 14:25:38
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#include <stdint.h>
#include <stddef.h>

#include "ff.h"
#include "diskio.h"
#include "sd_card.h"

#define SD_DRIVE 0

static DSTATUS drive_status = STA_NOINIT;

DSTATUS disk_status(BYTE physical_drive)
{
	// Verify the physical drive number
	if (physical_drive != SD_DRIVE)
	{
		return STA_NOINIT;
	}

	return drive_status;
}

DSTATUS disk_initialize(BYTE physical_drive)
{
	// Verify the physical drive number
	if (physical_drive != SD_DRIVE)
	{
		return STA_NOINIT;
	}

	// Initialize the SD card
	if (sd_init())
	{
		drive_status = 0;
	}
	else
	{
		drive_status = STA_NOINIT;
	}

	return drive_status;
}

DRESULT disk_read(BYTE physical_drive,
BYTE *buffer,
LBA_t sector,
UINT count)
{
	if (physical_drive != SD_DRIVE ||
	buffer == NULL ||
	count == 0)
	{
		return RES_PARERR;
	}

	if (drive_status & STA_NOINIT)
	{
		return RES_NOTRDY;
	}

	// Read the requested sectors
	for (UINT index = 0; index < count; index++)
	{
		if (!sd_read_block((uint32_t)sector + index,
		buffer))
		{
			return RES_ERROR;
		}

		buffer += SD_BLOCK_SIZE;
	}

	return RES_OK;
}

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE physical_drive,
const BYTE *buffer,
LBA_t sector,
UINT count)
{
	if (physical_drive != SD_DRIVE ||
	buffer == NULL ||
	count == 0)
	{
		return RES_PARERR;
	}

	if (drive_status & STA_NOINIT)
	{
		return RES_NOTRDY;
	}

	// Write the requested sectors
	for (UINT index = 0; index < count; index++)
	{
		if (!sd_write_block((uint32_t)sector + index,
		buffer))
		{
			return RES_ERROR;
		}

		buffer += SD_BLOCK_SIZE;
	}

	return RES_OK;
}

#endif

DRESULT disk_ioctl(BYTE physical_drive,
BYTE command,
void *buffer)
{
	if (physical_drive != SD_DRIVE)
	{
		return RES_PARERR;
	}

	if (drive_status & STA_NOINIT)
	{
		return RES_NOTRDY;
	}

	switch (command)
	{
		case CTRL_SYNC:
		// Sector writes are completed synchronously
		return RES_OK;

		case GET_SECTOR_SIZE:
		if (buffer == NULL)
		{
			return RES_PARERR;
		}

		*(WORD *)buffer = SD_BLOCK_SIZE;
		return RES_OK;

		case GET_BLOCK_SIZE:
		if (buffer == NULL)
		{
			return RES_PARERR;
		}

		// Erase block size is not determined
		*(DWORD *)buffer = 1;
		return RES_OK;

		case GET_SECTOR_COUNT:
		// Card capacity detection is not implemented
		return RES_PARERR;

		default:
		return RES_PARERR;
	}
}
