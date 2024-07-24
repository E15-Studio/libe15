/**
 * @file st7789.h
 * @author simakeng (simakeng@outlook.com)
 * @brief lcd driver for ST7789 in RGB565 mode
 * @version 0.1
 * @date 2023-07-19
 *
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
 * directory of this project named "LICENSE" file.
 *
 * https://www.gnu.org/licenses/gpl-3.0.html
 *
 * *****************************************************************************
 *
 */

/******************************************************************************/
/*                               INCLUDE FILES                                */
/******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include <color/color.h>

#include <error_codes.h>

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/

#ifndef _ST7789_H_
#define _ST7789_H_

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/

enum
{
    ST7789_BUS_MODE_UNKNOW = 0,
    ST7789_BUS_MODE_SPI = 1,
    ST7789_BUS_MODE_8080 = 2
};

typedef enum
{
    ST7789_ASYNC_STATE_IDLE = 0,
    ST7789_ASYNC_STATE_BUFFER_LOADED,
    ST7789_ASYNC_STATE_BUFFER_RELOADED,
    ST7789_ASYNC_STATE_TRANSFERING,
} st7789_async_state_t;

/******************************************************************************/
/*                              TYPE DEFINITIONS                              */
/******************************************************************************/

#ifndef RECT_TYPE_DEF
#define RECT_TYPE_DEF
typedef struct
{
    int32_t top, bottom, left, right;
} rect_t;
#endif

struct __tag_st7789_device_t;
typedef error_t (*st7789_transfer_cplt_handler_t)(struct __tag_st7789_device_t *device, void *pargs);

typedef struct
{
    /**
     * @brief write the CS pin state
     * @param gpio_state the new state of gpio
     */
    error_t (*gpio_cs_set)(int gpio_state);
    /**
     * @brief write the D/C pin state
     * @param gpio_state the new state of gpio
     */
    error_t (*gpio_dc_set)(int gpio_state);
    /**
     * @brief write the reset pin state
     * @param gpio_state the new state of gpio
     */
    error_t (*gpio_rst_set)(int gpio_state);
    /**
     * @brief write data to spi
     * @param size the size of data
     * @param data the data to write
     */
    error_t (*write)(uint32_t size, const void *data);

    /**
     * @brief write data to spi in async mode
     * @param blockid the id of this data block
     * @param size the size of data
     * @param data the data to write
     * @note user should implement this function if
     *       st7789_write_gram_async_start is called.
     *       and should call st7789_write_gram_async_completeed
     *       when one transfer is completed.
     */
    error_t (*write_async_start)(uint32_t size, const void *data);

} st7789_device_spi_op_t;

typedef struct
{
    /**
     * @brief read data from ST7789
     *
     * @param size the size of data
     * @param data the buffer to store data
     *
     * @note this driver takes care of endianess-conversion,
     *       please pass the data as it is.
     *
     * @note the size of data should always be multiple of 2
     */
    error_t (*data_read)(uint32_t size, void *data);

    /**
     * @brief write data to ST7789
     *
     * @param size the size of data
     * @param data the data to write
     *
     * @note this driver takes care of endianess-conversion,
     *       please pass the data as it is.
     *
     * @note the size of data should always be multiple of 2
     */
    error_t (*data_write)(uint32_t size, const void *data);

    /**
     * @brief write command to ST7789
     *
     * @param size the size of data
     * @param data the command to write
     *
     * @note this driver takes care of endianess-conversion,
     *       please pass the data as it is.
     *
     * @note the size of data should always be equal to 2
     */
    error_t (*command_write)(uint32_t size, const void *data);

    /**
     * @brief write a 16bit data into ST7789 for multiple times
     *
     * @param ndata the number of data to write
     * @param data the data to write
     *
     * @note this driver takes care of endianess-conversion,
     *       please pass the data as it is.
     */
    error_t (*data_set)(uint32_t ndata, uint16_t data);

    /**
     * @brief start a data transfer to ST7789 in async mode.
     *
     *       the implementation of this function should
     *       prepare the hardware context and start the transfer,
     *       than return immediately.
     *
     *
     * @param size the size of data
     * @param data the data to write
     *
     * @note this driver takes care of endianess-conversion,
     *       please pass the data as it is.
     *
     * @note the size of data should always be multiple of 2
     *
     * @note this function should be implemented if
     *       st7789_write_gram_async_start is called.
     *       and the user should call
     *       st7789_async_completed_notify to notify the driver
     *       when the transfer is completed.
     */
    error_t (*data_write_async_start)(uint32_t size, const void *data);
} st7789_device_80_op_t;

typedef struct
{
    unsigned bus_mode : 2;
    unsigned host_is_big_endian : 1;
    union
    {
        st7789_device_spi_op_t spi;
        st7789_device_80_op_t bus80;
    };
    /**
     * @brief change backlight duty
     * @param duty the duty of backlight, 10000 is full duty
     */
    error_t (*pwm_change_duty)(uint16_t duty);
    /**
     * @brief aquire the bus, this function is called by driver
     * when a sequence of operations is needed
     * @note if the bus is shared with other devices, this
     * function should be implemented
     */
    error_t (*bus_aquire)(void);
    /**
     * @brief aquire the bus, this function is called by driver
     * when a sequence of operations is ended and the bus
     * can be released.
     * @see bus_aquire
     * @note if the bus is shared with other devices, this
     * function should be implemented
     */
    error_t (*bus_release)(void);
} st7789_device_op_t;

typedef struct
{
    st7789_device_op_t device_op;
    struct
    {
        uint16_t x, y;
    } resolution;
} st7789_device_init_t;
typedef struct __tag_st7789_device_t
{
    st7789_device_op_t device_op;
    st7789_transfer_cplt_handler_t handler;
    void *handler_params;
    rect_t gram_rect;
    st7789_async_state_t async_state;
    struct
    {
        uint16_t x, y;
    } resolution;
    rgb565_t *gram_tx_buf;
    uint32_t gram_tx_buf_size;
} st7789_device_t;

/******************************************************************************/
/*                         PUBLIC FUNCTION DEFINITIONS                        */
/******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif // ! #ifdef __cplusplus

    /**
     * @brief initialize the st7789 device
     *
     * @param device the device to initialize
     * @param init the init parameters
     * @return error_t
     *
     * @note This function will take around 140ms
     */
    error_t st7789_init(st7789_device_t *device, st7789_device_init_t *init);

    error_t st7789_display_on(st7789_device_t *device);

    error_t st7789_display_off(st7789_device_t *device);

    error_t st7789_display_clear_gram(st7789_device_t *device, rgb565_t color);

    error_t st7789_display_clear_gram_async(st7789_device_t *device, rgb565_t color);

    error_t st7789_display_set_window(st7789_device_t *device, rect_t rect);

    error_t st7789_append_gram(st7789_device_t *device, const rgb565_t *w_data, uint32_t npixel);

    error_t st7789_async_completed_notify(st7789_device_t *device);

    error_t st7789_update_gram_set_buff(st7789_device_t *device, uint32_t buffer_size, rgb565_t *pbuf);

    error_t st7789_update_gram_stream_start(
        st7789_device_t *device,
        st7789_transfer_cplt_handler_t handler,
        void *params);

    error_t st7789_wait_async_complete(st7789_device_t *device, uint32_t timeout);

    error_t st7789_read_gram(st7789_device_t *device, uint32_t npixel, rgb565_t *pbuf, bool _continue);

    error_t st7789_read_gram_end(st7789_device_t *device);

#ifdef __cplusplus
}
#endif // ! #ifdef __cplusplus

/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/

#endif //! #ifndef _ST7789_H_
