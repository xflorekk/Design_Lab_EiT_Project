/*
 * sd_card.h
 *
 * Created: 15.11.2026 14:16:41
 *  Authors : Daniel Florek, Jakub W¹sowicz, Filip Dudek
 */ 


#ifndef SD_CARD_H_
#define SD_CARD_H_

#include <stdint.h>

#define SD_BLOCK_SIZE 512U

typedef enum
{
	SD_CARD_NONE = 0,
	SD_CARD_SDSC,
	SD_CARD_SDHC

} sd_card_type_t;

uint8_t sd_init(void);

uint8_t sd_read_block(uint32_t block_address,
uint8_t *buffer);

uint8_t sd_write_block(uint32_t block_address,
const uint8_t *buffer);

sd_card_type_t sd_get_type(void);

#endif