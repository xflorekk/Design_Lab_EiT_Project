/*
 * spi.c
 *
 * Created: 15.11.2026 14:13:48
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#include <avr/io.h>
#include <stdint.h>
#include "spi.h"

#define SPI_CS   PB2
#define SPI_MOSI PB3
#define SPI_MISO PB4
#define SPI_SCK  PB5

#define SPI_TIMEOUT 60000U

// SPI0 control register bit positions
#define SPI_ENABLE_BIT  6
#define SPI_MASTER_BIT  4
#define SPI_SPR1_BIT    1
#define SPI_SPR0_BIT    0

// SPI0 status register bit position
#define SPI_COMPLETE_BIT 7

void spi_init(void)
{
	// Set chip select high before configuring the pin
	PORTB |= (1 << SPI_CS);

	// Configure CS, MOSI and SCK as outputs
	DDRB |= (1 << SPI_CS) |
	(1 << SPI_MOSI) |
	(1 << SPI_SCK);

	// Configure MISO as input
	DDRB &= ~(1 << SPI_MISO);

	// Disable internal pull-up on MISO
	PORTB &= ~(1 << SPI_MISO);

	// Enable SPI master in mode 0 at F_CPU / 64
	SPCR0 = (1 << SPI_ENABLE_BIT) |
	(1 << SPI_MASTER_BIT) |
	(1 << SPI_SPR1_BIT);

	SPSR0 = 0;
}

void spi_set_slow_speed(void)
{
	// Set SPI clock to F_CPU / 64
	SPCR0 = (1 << SPI_ENABLE_BIT) |
	(1 << SPI_MASTER_BIT) |
	(1 << SPI_SPR1_BIT);

	SPSR0 = 0;
}

void spi_set_fast_speed(void)
{
	// Set SPI clock to F_CPU / 16
	SPCR0 = (1 << SPI_ENABLE_BIT) |
	(1 << SPI_MASTER_BIT) |
	(1 << SPI_SPR0_BIT);

	SPSR0 = 0;
}

void spi_select(void)
{
	// Enable SD card communication
	PORTB &= ~(1 << SPI_CS);
}

void spi_deselect(void)
{
	// Disable SD card communication
	PORTB |= (1 << SPI_CS);
}

uint8_t spi_transfer(uint8_t tx_data, uint8_t *rx_data)
{
	uint16_t timeout = SPI_TIMEOUT;

	if (rx_data == 0)
	{
		return 0;
	}

	// Start SPI transmission
	SPDR0 = tx_data;

	// Wait for transmission to complete
	while (!(SPSR0 & (1 << SPI_COMPLETE_BIT)))
	{
		if (--timeout == 0)
		{
			return 0;
		}
	}

	// Read received data
	*rx_data = SPDR0;

	return 1;
}

