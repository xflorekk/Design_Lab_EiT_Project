/*
 * main.c
 *
 * Authors: Daniel Florek, Jakub Wasowicz, Filip Dudek
 * ATmega328PB - Environmental Data Logger
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "uart.h"
#include "timer.h"
#include "7seg.h"
#include "keyboard.h"
#include "i2c.h"
#include "bmp280.h"
#include "storage.h"

#define BAUDRATE 9600UL
#define MYUBR ((F_CPU / (16UL * BAUDRATE)) - 1UL)

// Disable this feature if PB7 is used by the external crystal.
#define RESET_BUTTON_ENABLED 1
#define DEVICE_RETRY_INTERVAL 10UL
#define BMP280_MAX_ERRORS 3U

typedef enum
{
    DISPLAY_TIME = 0,
    DISPLAY_TEMPERATURE,
    DISPLAY_PRESSURE
} display_mode_t;

static display_mode_t display_mode = DISPLAY_TIME;
static bmp280_data_t measurement = {0};

static uint8_t sensor_ready = 0;
static uint8_t measurement_valid = 0;
static uint8_t sensor_error_count = 0;
static uint8_t sd_ready = 0;
static uint8_t auto_count = 0;
static uint8_t auto_log = 1;

static uint32_t system_seconds = 0;
static uint32_t next_sensor_retry = DEVICE_RETRY_INTERVAL;
static uint32_t next_sd_retry = DEVICE_RETRY_INTERVAL;

// Send an unsigned decimal number without snprintf() or a large buffer.
static void uart_print_u32(uint32_t value)
{
    char digits[10];
    uint8_t count = 0;

    do
    {
        digits[count++] = (char)('0' + (value % 10UL));
        value /= 10UL;
    }
    while (value != 0);

    while (count > 0)
    {
        usart0_transmit((uint8_t)digits[--count]);
    }
}

static void uart_print_two_digits(uint8_t value)
{
    usart0_transmit((uint8_t)('0' + value / 10U));
    usart0_transmit((uint8_t)('0' + value % 10U));
}

static void uart_print_fixed_centi(int32_t value)
{
    uint32_t magnitude;

    if (value < 0)
    {
        usart0_transmit('-');
        magnitude = (uint32_t)(-(value + 1)) + 1UL;
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    uart_print_u32(magnitude / 100UL);
    usart0_transmit('.');
    uart_print_two_digits((uint8_t)(magnitude % 100UL));
}

static void uart_print_time(void)
{
    usart0_print_P(PSTR("Time: "));
    uart_print_u32(timer_get_seconds());
    usart0_print_P(PSTR(" s\r\n"));
}

static void uart_print_measurement(void)
{
    if (!measurement_valid)
    {
        usart0_print_P(PSTR("Sensor data unavailable\r\n"));
        return;
    }

    usart0_print_P(PSTR("Temp: "));
    uart_print_fixed_centi(measurement.temperature_centi_c);
    usart0_print_P(PSTR(" C, Pressure: "));
    uart_print_u32(measurement.pressure_pa / 100UL);
    usart0_transmit('.');
    uart_print_two_digits((uint8_t)(measurement.pressure_pa % 100UL));
    usart0_print_P(PSTR(" hPa\r\n"));
}

static void uart_print_status(void)
{
    usart0_print_P(PSTR("BMP280: "));
    usart0_print_P(sensor_ready ? PSTR("OK") : PSTR("ERROR"));
    usart0_print_P(PSTR(", SD: "));
    usart0_print_P(sd_ready ? PSTR("OK") : PSTR("ERROR"));
    usart0_print_P(PSTR(", Auto log: "));
    usart0_print_P(auto_log ? PSTR("ON\r\n") : PSTR("OFF\r\n"));
}

static void uart_print_menu(void)
{
    usart0_print_P(PSTR("\r\nSYSTEM STARTED!\r\n"));
    usart0_print_P(PSTR("Control:\r\n"));
    usart0_print_P(PSTR("Start/Stop: 's'\r\n"));
    usart0_print_P(PSTR("Reset counter: 'r'\r\n"));
    usart0_print_P(PSTR("Show counter: 'p'\r\n"));
    usart0_print_P(PSTR("Auto counting mode: 'm'\r\n"));
    usart0_print_P(PSTR("Show sensor data: 'd'\r\n"));
    usart0_print_P(PSTR("Show device status: 'i'\r\n"));
    usart0_print_P(PSTR("Toggle SD auto logging: 'l'\r\n"));
}

#if RESET_BUTTON_ENABLED
static void reset_button_init(void)
{
    DDRB &= ~(1 << PB7);
    PORTB |= (1 << PB7);
}

static void perform_hardware_reset(void)
{
    if (storage_is_ready())
    {
        storage_close();
    }

    // Clear the transmission-complete flag before the final message.
    UCSR0A = (1 << TXC0);
    usart0_print_P(PSTR("Hardware reset called\r\n"));

    while (!(UCSR0A & (1 << TXC0)))
    {
    }

    cli();
    wdt_enable(WDTO_15MS);

    while (1)
    {
    }
}
#endif

static void display_update(void)
{
    switch (display_mode)
    {
        case DISPLAY_TIME:
            display_show_time(timer_get_seconds());
            break;

        case DISPLAY_TEMPERATURE:
            if (measurement_valid)
            {
                display_show_temperature(measurement.temperature_centi_c);
            }
            else
            {
                display_show_error();
            }
            break;

        case DISPLAY_PRESSURE:
            if (measurement_valid)
            {
                display_show_pressure(measurement.pressure_pa);
            }
            else
            {
                display_show_error();
            }
            break;

        default:
            display_show_error();
            break;
    }
}

static void sensor_initialize(void)
{
    sensor_ready = bmp280_init();
    measurement_valid = 0;
    sensor_error_count = 0;

    if (sensor_ready)
    {
        usart0_print_P(PSTR("BMP280 initialized\r\n"));
    }
    else
    {
        next_sensor_retry = system_seconds + DEVICE_RETRY_INTERVAL;
        usart0_print_P(PSTR("BMP280 initialization failed\r\n"));
    }
}

static void sensor_update(void)
{
    bmp280_data_t new_measurement;
    uint8_t read_ok;

    if (!sensor_ready)
    {
        measurement_valid = 0;
        return;
    }

    read_ok = bmp280_read(&new_measurement);

    // Do not accept measurements outside BMP280's operating ranges.
    if (read_ok &&
        new_measurement.temperature_centi_c >= -4000L &&
        new_measurement.temperature_centi_c <= 8500L &&
        new_measurement.pressure_pa >= 30000UL &&
        new_measurement.pressure_pa <= 110000UL)
    {
        measurement = new_measurement;
        measurement_valid = 1;

        if (sensor_error_count != 0)
        {
            usart0_print_P(PSTR("BMP280 communication recovered\r\n"));
        }

        sensor_error_count = 0;
        return;
    }

    measurement_valid = 0;
    sensor_error_count++;

    if (sensor_error_count == 1)
    {
        usart0_print_P(PSTR("BMP280 read invalid\r\n"));
    }

    if (sensor_error_count >= BMP280_MAX_ERRORS)
    {
        sensor_ready = 0;
        sensor_error_count = 0;
        next_sensor_retry = system_seconds + DEVICE_RETRY_INTERVAL;
        usart0_print_P(PSTR("BMP280 offline; retry scheduled\r\n"));
    }
}

static void sd_initialize(void)
{
    sd_ready = storage_init();

    if (sd_ready)
    {
        usart0_print_P(PSTR("SD card initialized\r\n"));
    }
    else
    {
        next_sd_retry = system_seconds + DEVICE_RETRY_INTERVAL;
        usart0_print_P(PSTR("SD initialization failed: "));
        uart_print_u32((uint8_t)storage_get_last_error());
        usart0_print_P(PSTR("\r\n"));
    }
}

static uint8_t save_measurement(void)
{
    FRESULT error;

    if (!measurement_valid || !sd_ready)
    {
        return 0;
    }

    if (storage_append_measurement(timer_get_seconds(),
                                   measurement.temperature_centi_c,
                                   measurement.pressure_pa))
    {
        return 1;
    }

    error = storage_get_last_error();
    sd_ready = 0;
    storage_close();
    next_sd_retry = system_seconds + DEVICE_RETRY_INTERVAL;

    usart0_print_P(PSTR("SD write error: "));
    uart_print_u32((uint8_t)error);
    usart0_print_P(PSTR("\r\n"));
    return 0;
}

static void keyboard_process(void)
{
    keyboard_key_t key = keyboard_get_key();

    switch (key)
    {
        case KEY_1:
            display_mode = DISPLAY_TIME;
            display_update();
            usart0_print_P(PSTR("Display: Timer\r\n"));
            break;

        case KEY_2:
            display_mode = DISPLAY_TEMPERATURE;
            display_update();
            usart0_print_P(PSTR("Display: Temperature\r\n"));
            break;

        case KEY_3:
            display_mode = DISPLAY_PRESSURE;
            display_update();
            usart0_print_P(PSTR("Display: Pressure\r\n"));
            break;

        case KEY_4:
            // Request a fresh measurement before manual logging.
            if (!sensor_ready)
            {
                sensor_initialize();
            }

            if (sensor_ready)
            {
                sensor_update();
            }

            if (!sd_ready)
            {
                sd_initialize();
            }

            if (save_measurement())
            {
                usart0_print_P(PSTR("Measurement saved to SD\r\n"));
            }
            else
            {
                usart0_print_P(PSTR("Measurement save failed\r\n"));
            }

            display_update();
            break;

        default:
            break;
    }
}

static void uart_process(void)
{
    char received_command;

    while (usart0_read_char(&received_command))
    {
        switch (received_command)
        {
            case 's':
            case 'S':
                timer_toggle();
                usart0_print_P(timer_is_running() ?
                    PSTR("Timer started\r\n") : PSTR("Timer stopped\r\n"));
                display_update();
                break;

            case 'r':
            case 'R':
                timer_reset();
                usart0_print_P(PSTR("Counter value now is 0 s\r\n"));
                display_update();
                break;

            case 'p':
            case 'P':
                uart_print_time();
                break;

            case 'm':
            case 'M':
                auto_count = !auto_count;
                usart0_print_P(auto_count ?
                    PSTR("Auto counting enabled\r\n") :
                    PSTR("Auto counting disabled\r\n"));
                break;

            case 'd':
            case 'D':
                // Attempt a fresh read, including recovery if needed.
                if (!sensor_ready)
                {
                    sensor_initialize();
                }

                if (sensor_ready)
                {
                    sensor_update();
                }

                uart_print_measurement();
                display_update();
                break;

            case 'i':
            case 'I':
                uart_print_status();
                break;

            case 'l':
            case 'L':
                auto_log = !auto_log;
                usart0_print_P(auto_log ?
                    PSTR("SD auto logging enabled\r\n") :
                    PSTR("SD auto logging disabled\r\n"));
                break;

            case '\r':
            case '\n':
                break;

            default:
                usart0_print_P(PSTR("Unknown command: '"));
                usart0_transmit((uint8_t)received_command);
                usart0_print_P(PSTR("'\r\n"));
                break;
        }
    }
}

static void periodic_tasks(void)
{
    if (!sensor_ready && system_seconds >= next_sensor_retry)
    {
        sensor_initialize();
    }

    if (sensor_ready)
    {
        sensor_update();
    }

    if (!sd_ready && system_seconds >= next_sd_retry)
    {
        sd_initialize();
    }

    if (auto_log && sd_ready && measurement_valid)
    {
        save_measurement();
    }

    if (auto_count)
    {
        usart0_print_P(PSTR("Time from the start: "));
        uart_print_u32(timer_get_seconds());
        usart0_print_P(PSTR(" s\r\n"));
    }

    display_update();
}

int main(void)
{
    MCUSR = 0;
    wdt_disable();

    usart0_init((uint16_t)MYUBR);
    timer1_init();
    display_init();
    keyboard_init();
    i2c_init();

#if RESET_BUTTON_ENABLED
    reset_button_init();
#endif

    sei();

    sensor_initialize();
    if (sensor_ready)
    {
        sensor_update();
    }
    sd_initialize();
	uart_print_menu();
    display_update();

    while (1)
    {
#if RESET_BUTTON_ENABLED
        if (!(PINB & (1 << PB7)))
        {
            perform_hardware_reset();
        }
#endif

        uart_process();
        keyboard_process();

        // Collect elapsed ticks, then perform one periodic measurement.
        uint8_t tick_received = 0;

        while (timer_take_tick())
        {
            system_seconds++;
            tick_received = 1;
        }

        if (tick_received)
        {
            periodic_tasks();
        }
    }
}
