/**
 * @file SSD1315.h
 * @author simakeng (simakeng@outlook.com)
 * @brief OLED display driver for SSD1315
 * @version 0.1
 * @date 2023-06-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdint.h>
#include <libe15-errors.h>

#define SSD1315_INIT_FLAG_LR_NORMAL 0x00
#define SSD1315_INIT_FLAG_LR_SWAP 0x01

#define SSD1315_INIT_FLAG_UD_NORMAL 0x00
#define SSD1315_INIT_FLAG_UD_SWAP 0x02

typedef struct
{
    /**
     * @brief write the cs pin state
     * @param gpio_state the new state of gpio
     */
    error_t (*gpio_cs_set)(int gpio_state);
    /**
     * @brief write the cs pin state
     * @param gpio_state the new state of gpio
     */
    error_t (*gpio_dc_set)(int gpio_state);

    /**
     * @brief write the reset pin state
     * @param gpio_state the new state of gpio
     * @note if this pin is not used, this function can be null.
     */
    error_t (*gpio_rst_set)(int gpio_state);
    /**
     * @brief write data to spi
     * @param size the size of data
     * @param data the data to write
     */
    error_t (*spi_write)(const void *data, uint32_t size);
    /**
     * @brief aquire the spi bus, this function is called by driver
     * when a sequence of spi operations is needed
     * @note if the spi bus is shared with other devices, this
     * function should be implemented
     */
    error_t (*spi_aquire)(void);
    /**
     * @brief aquire the spi bus, this function is called by driver
     * when a sequence of spi operations is ended and the spi bus
     * can be released.
     * @see spi_aquire
     * @note if the spi bus is shared with other devices, this
     * function should be implemented
     */
    error_t (*spi_release)(void);
} SSD1315_device_op_t;

typedef struct
{
    SSD1315_device_op_t devop;
    uint8_t flags;
} SSD1315_Init_t;

typedef struct
{
    SSD1315_device_op_t device_op;
    uint8_t colum_offset;
    uint8_t row_offset;
} SSD1315_device_t;

/**
 * @brief Initialize SSD1315 device
 *
 * @param device
 * @param init
 * @return error_t
 */
error_t SSD1315_init(SSD1315_device_t *device, SSD1315_Init_t *init);

/**
 * @brief Turn on display
 *
 * @param device
 * @return error_t
 */
error_t SSD1315_display_on(SSD1315_device_t *device);

/**
 * @brief Turn off display
 *
 * @param device
 * @return error_t
 */
error_t SSD1315_display_off(SSD1315_device_t *device);

/**
 * @brief clear the display
 *
 * @param device
 * @return error_t
 */
error_t SSD1315_clear_gram(SSD1315_device_t *device);

/**
 * @brief set the write ptr offset.
 * this will affect the next SSD1315_append_gram
 * and SSD1315_write_gram write function call
 *
 * @param device
 * @param col_off column offset 0..63
 * @param row_off row offset 0..7
 * @return error_t
 */
error_t SSD1315_set_offset(SSD1315_device_t *device, uint32_t col_off, uint32_t row_off);

/**
 * @brief set the write ptr offset.
 * this will affect the next SSD1315_append_gram
 * and SSD1315_write_gram write function call
 * @param device 
 * @param mem_off 
 * @return error_t 
 */
error_t SSD1315_set_mem_offset(SSD1315_device_t *device, uint32_t mem_off);

/**
 * @brief append data to gram and automatically increase the offset
 * @see SSD1315_set_offset
 * @param device
 * @param w_data
 * @param w_size
 * @return error_t
 */
error_t SSD1315_append_gram(SSD1315_device_t *device, const void *w_data, uint32_t w_size);

/**
 * @brief write block of data to gram
 *
 * @param device
 * @param mem_off
 * @param w_data
 * @param w_size
 * @return error_t
 */
error_t SSD1315_write_gram(SSD1315_device_t *device, uint32_t mem_off, const void *w_data, uint32_t w_size);

// error_t SSD1315_set_start_line(SSD1315_device_t *device, uint32_t off);
// error_t SSD1315_set_mux_ratio(SSD1315_device_t *device, uint32_t ratio);


#define SSD1315_GRAM_LINE_WIDTH 128
#define SSD1315_GRAM_LINE_HEIGHT 64

#define SSD1315_GRAM_SIZE (SSD1315_GRAM_LINE_WIDTH * SSD1315_GRAM_LINE_HEIGHT / 8)

