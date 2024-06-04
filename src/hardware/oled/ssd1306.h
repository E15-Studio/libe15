/**
 * @file ssd1306.h
 * @author simakeng (simakeng@outlook.com)
 * @brief OLED display driver for SSD1306
 * @version 0.1
 * @date 2023-06-13
 * *****************************************************************************
 * @copyright Copyright (C) E15 Studio 2024
 *
 * This program is FREE software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation.
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA. Or you can visit the link below to
 * read the license online, or you can find a copy of the license in the root
 * directory of this project named "COPYING" file.
 *
 * https://www.gnu.org/licenses/gpl-3.0.html
 *
 * *****************************************************************************
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include <error_codes.h>
#include <generated-conf.h>

#ifndef __SSD1306_H__
#define __SSD1306_H__

#ifdef CONFIG_OLED_SSD1306

#ifdef __cplusplus
extern "C"
{
#endif //! #ifdef __cplusplus

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
         * @brief write data to spi
         * @param size the size of data
         * @param data the data to write
         */
        error_t (*spi_write)(uint32_t size, const void *data);
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
    } SSD1306_device_op_t;

    /**
     * @brief Initialization structure for SSD1306
     */
    typedef struct
    {
        /**
         * @brief Function pointer to device operations
         */
        SSD1306_device_op_t devop;

        /**
         * @brief display pixel order flip in vertical direction
         */
        bool ud_flip;

        /**
         * @brief display pixel order flip in horizontal direction
         */
        bool lr_flip;
    } SSD1306_Init_t;

    /**
     * @brief Device structure for SSD1306
     */
    typedef struct
    {
        SSD1306_device_op_t device_op;
    } SSD1306_Device_t;

    /**
     * @brief Initialize SSD1306 device
     *
     * @param device
     * @param init
     * @return error_t
     */
    error_t SSD1306_Init(SSD1306_Device_t *device, SSD1306_Init_t *init);

    /**
     * @brief Turn on display
     *
     * @param device
     * @return error_t
     */
    error_t SSD1306_display_on(SSD1306_Device_t *device);

    /**
     * @brief Turn off display
     *
     * @param device
     * @return error_t
     */
    error_t SSD1306_display_off(SSD1306_Device_t *device);

    /**
     * @brief clear the display
     *
     * @param device
     * @return error_t
     */
    error_t SSD1306_clear_gram(SSD1306_Device_t *device);

    /**
     * @brief set the write ptr offset.
     * this will affect the next SSD1306_append_gram write
     *
     * @param device
     * @param off
     * @return error_t
     */
    error_t SSD1306_set_offset(SSD1306_Device_t *device, uint32_t off);

    /**
     * @brief append data to gram and automatically increase the offset
     * @see SSD1306_set_offset
     * @param device
     * @param w_data
     * @param w_size
     * @return error_t
     */
    error_t SSD1306_append_gram(SSD1306_Device_t *device,
                                const void *w_data, uint32_t w_size);

    /**
     * @brief write block of data to gram
     *
     * @param device
     * @param mem_off
     * @param w_data
     * @param w_size
     * @return error_t
     */
    error_t SSD1306_write_gram(SSD1306_Device_t *device,
                               uint32_t mem_off, const void *w_data,
                               uint32_t w_size);

#ifdef __cplusplus
}
#endif //! #ifdef __cplusplus

#endif // !#ifdef CONFIG_OLED_SSD1306

#endif //! #ifndef __SSD1306_H__
