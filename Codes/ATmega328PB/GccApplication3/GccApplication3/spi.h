/*
 * spi.h
 *
 * Created: 15.11.2026 14:13:40
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

void spi_init(void);
void spi_set_slow_speed(void);
void spi_set_fast_speed(void);

void spi_select(void);
void spi_deselect(void);

uint8_t spi_transfer(uint8_t tx_data, uint8_t *rx_data);

#endif