/*
 * sd_card.c
 *
 * Created: 15.11.2026 14:16:32
 * Authors: Daniel Florek, Jakub Wasowicz, Filip Dudek
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdint.h>
#include <util/delay.h>

#include "spi.h"
#include "sd_card.h"

#define SD_CMD0   0U
#define SD_CMD8   8U
#define SD_CMD16  16U
#define SD_CMD17  17U
#define SD_CMD24  24U
#define SD_CMD41  41U
#define SD_CMD55  55U
#define SD_CMD58  58U

#define SD_IDLE_STATE       0x01U
#define SD_READY_STATE      0x00U
#define SD_DATA_START       0xFEU
#define SD_DATA_ACCEPTED    0x05U

#define SD_INIT_RETRIES     1000U
#define SD_READY_TIMEOUT   60000U
#define SD_DATA_TIMEOUT    20000U

static sd_card_type_t card_type = SD_CARD_NONE;

static void sd_release(void)
{
    uint8_t ignored;
    spi_deselect();
    (void)spi_transfer(0xFF, &ignored);
}

static uint8_t sd_wait_ready(uint16_t timeout)
{
    uint8_t response;

    while (timeout-- > 0U)
    {
        if (!spi_transfer(0xFF, &response))
        {
            return 0;
        }
        if (response == 0xFF)
        {
            return 1;
        }
    }
    return 0;
}

// A successful command keeps CS low for possible response data.
static uint8_t sd_send_command(uint8_t command,
                               uint32_t argument,
                               uint8_t crc,
                               uint8_t *response)
{
    uint8_t received;
    uint8_t frame[6];

    if (response == 0)
    {
        return 0;
    }
    *response = 0xFF;

    sd_release();
    spi_select();

    if (!sd_wait_ready(1000U))
    {
        goto command_error;
    }

    frame[0] = (uint8_t)(0x40U | command);
    frame[1] = (uint8_t)(argument >> 24);
    frame[2] = (uint8_t)(argument >> 16);
    frame[3] = (uint8_t)(argument >> 8);
    frame[4] = (uint8_t)argument;
    frame[5] = crc;

    for (uint8_t index = 0; index < 6U; index++)
    {
        if (!spi_transfer(frame[index], &received))
        {
            goto command_error;
        }
    }

    for (uint8_t attempt = 0; attempt < 10U; attempt++)
    {
        if (!spi_transfer(0xFF, &received))
        {
            goto command_error;
        }
        if ((received & 0x80U) == 0U)
        {
            *response = received;
            return 1;
        }
    }

command_error:
    sd_release();
    return 0;
}

sd_card_type_t sd_get_type(void)
{
    return card_type;
}

uint8_t sd_init(void)
{
    uint8_t response = 0xFF;
    uint8_t ocr[4];
    uint8_t version2 = 0;
    uint8_t initialized = 0;

    card_type = SD_CARD_NONE;

    spi_init();
    spi_set_slow_speed();
    spi_deselect();
    _delay_ms(10);

    // Provide at least 80 clocks while CS is high.
    for (uint8_t index = 0; index < 10U; index++)
    {
        if (!spi_transfer(0xFF, &response))
        {
            return 0;
        }
    }

    for (uint8_t attempt = 0; attempt < 10U; attempt++)
    {
        if (sd_send_command(SD_CMD0, 0, 0x95, &response) &&
            response == SD_IDLE_STATE)
        {
            initialized = 1;
            sd_release();
            break;
        }
        sd_release();
        _delay_ms(10);
    }

    if (!initialized)
    {
        return 0;
    }

    if (!sd_send_command(SD_CMD8, 0x000001AAUL, 0x87, &response))
    {
        return 0;
    }

    if (response == SD_IDLE_STATE)
    {
        for (uint8_t index = 0; index < 4U; index++)
        {
            if (!spi_transfer(0xFF, &ocr[index]))
            {
                sd_release();
                return 0;
            }
        }
        sd_release();

        if (ocr[2] != 0x01U || ocr[3] != 0xAAU)
        {
            return 0;
        }
        version2 = 1;
    }
    else if (response == 0x05U)
    {
        sd_release();
    }
    else
    {
        sd_release();
        return 0;
    }

    initialized = 0;
    for (uint16_t attempt = 0; attempt < SD_INIT_RETRIES; attempt++)
    {
        if (!sd_send_command(SD_CMD55, 0, 0x01, &response))
        {
            return 0;
        }
        sd_release();

        if (response > SD_IDLE_STATE)
        {
            return 0;
        }

        uint32_t argument = version2 ? 0x40000000UL : 0UL;
        if (!sd_send_command(SD_CMD41, argument, 0x01, &response))
        {
            return 0;
        }
        sd_release();

        if (response == SD_READY_STATE)
        {
            initialized = 1;
            break;
        }
        if (response != SD_IDLE_STATE)
        {
            return 0;
        }
        _delay_ms(1);
    }

    if (!initialized)
    {
        return 0;
    }

    if (!sd_send_command(SD_CMD58, 0, 0x01, &response))
    {
        return 0;
    }
    if (response != SD_READY_STATE)
    {
        sd_release();
        return 0;
    }

    for (uint8_t index = 0; index < 4U; index++)
    {
        if (!spi_transfer(0xFF, &ocr[index]))
        {
            sd_release();
            return 0;
        }
    }
    sd_release();

    if (version2 && (ocr[0] & 0x40U))
    {
        card_type = SD_CARD_SDHC;
    }
    else
    {
        if (!sd_send_command(SD_CMD16, SD_BLOCK_SIZE, 0x01, &response))
        {
            return 0;
        }
        sd_release();
        if (response != SD_READY_STATE)
        {
            return 0;
        }
        card_type = SD_CARD_SDSC;
    }

    spi_set_fast_speed();
    return 1;
}

static uint8_t sd_get_address(uint32_t block_address,
                              uint32_t *address)
{
    if (card_type == SD_CARD_NONE || address == 0)
    {
        return 0;
    }
    if (card_type == SD_CARD_SDSC)
    {
        if (block_address > UINT32_MAX / SD_BLOCK_SIZE)
        {
            return 0;
        }
        *address = block_address * SD_BLOCK_SIZE;
    }
    else
    {
        *address = block_address;
    }
    return 1;
}

uint8_t sd_read_block(uint32_t block_address, uint8_t *buffer)
{
    uint8_t response;
    uint8_t token = 0xFF;
    uint32_t address;
    uint8_t found_token = 0;

    if (buffer == 0 || !sd_get_address(block_address, &address))
    {
        return 0;
    }
    if (!sd_send_command(SD_CMD17, address, 0x01, &response))
    {
        return 0;
    }
    if (response != SD_READY_STATE)
    {
        sd_release();
        return 0;
    }

    for (uint16_t attempt = 0; attempt < SD_DATA_TIMEOUT; attempt++)
    {
        if (!spi_transfer(0xFF, &token))
        {
            sd_release();
            return 0;
        }
        if (token == SD_DATA_START)
        {
            found_token = 1;
            break;
        }
        if (token != 0xFF)
        {
            break;
        }
    }
    if (!found_token)
    {
        sd_release();
        return 0;
    }

    for (uint16_t index = 0; index < SD_BLOCK_SIZE; index++)
    {
        if (!spi_transfer(0xFF, &buffer[index]))
        {
            sd_release();
            return 0;
        }
    }

    // Discard the two CRC bytes (CRC checking is disabled).
    for (uint8_t index = 0; index < 2U; index++)
    {
        if (!spi_transfer(0xFF, &response))
        {
            sd_release();
            return 0;
        }
    }
    sd_release();
    return 1;
}

uint8_t sd_write_block(uint32_t block_address, const uint8_t *buffer)
{
    uint8_t response;
    uint32_t address;

    if (buffer == 0 || !sd_get_address(block_address, &address))
    {
        return 0;
    }
    if (!sd_send_command(SD_CMD24, address, 0x01, &response))
    {
        return 0;
    }
    if (response != SD_READY_STATE)
    {
        sd_release();
        return 0;
    }

    if (!spi_transfer(0xFF, &response) ||
        !spi_transfer(SD_DATA_START, &response))
    {
        sd_release();
        return 0;
    }

    for (uint16_t index = 0; index < SD_BLOCK_SIZE; index++)
    {
        if (!spi_transfer(buffer[index], &response))
        {
            sd_release();
            return 0;
        }
    }

    if (!spi_transfer(0xFF, &response) ||
        !spi_transfer(0xFF, &response))
    {
        sd_release();
        return 0;
    }

    // The card sends a data response token after the CRC bytes.
    if (!spi_transfer(0xFF, &response))
    {
        sd_release();
        return 0;
    }
    if ((response & 0x1FU) != SD_DATA_ACCEPTED)
    {
        sd_release();
        return 0;
    }
    if (!sd_wait_ready(SD_READY_TIMEOUT))
    {
        sd_release();
        return 0;
    }
    sd_release();
    return 1;
}
