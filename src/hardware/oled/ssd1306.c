/**
 * @file ssd1306.c
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

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <hardware/devop.h>

#include "ssd1306.h"

#ifdef CONFIG_OLED_SSD1306

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/
#define SSD1306_GRAM_SIZE 1024
#define SSD1306_GRAM_LINE_WIDTH 128
#define SSD1306_GRAM_LINE_COUNT 8
/******************************************************************************/
/*                              TYPE DEFINITIONS                              */
/******************************************************************************/

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/

/******************************************************************************/
/*                          PUBLIC DATA DEFINITIONS                           */
/******************************************************************************/

/******************************************************************************/
/*                          PRIVATE DATA DEFINITIONS                          */
/******************************************************************************/

/******************************************************************************/
/*                        PRIVATE FUNCTION DEFINITIONS                        */
/******************************************************************************/

static error_t write_command_sequence(ssd1306_device_t *device,
                                      const void *cmd_seq,
                                      uint32_t size);

static error_t write_data(ssd1306_device_t *device,
                          const void *data,
                          uint32_t size);

/******************************************************************************/
/*                      PUBLIC FUNCTION IMPLEMENTATIONS                       */
/******************************************************************************/

error_t ssd1306_init(ssd1306_device_t *device, ssd1306_Init_t *init)
{
    // check all arguments

    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(init);

    PARAM_NOT_NULL(init->devop.gpio_cs_set);
    PARAM_NOT_NULL(init->devop.gpio_dc_set);
    PARAM_NOT_NULL(init->devop.spi_write);

    // fill the device structure with zeros
    memset(device, 0, sizeof(ssd1306_device_t));

    // copy the device maniuplation functions
    device->device_op = init->devop;

    // init gpio state
    CALL_NULLABLE_WITH_ERROR(init->devop.gpio_rst_set, 1);

    // reset the device
    CALL_NULLABLE_WITH_ERROR(init->devop.gpio_rst_set, 0);
    // TODO: add timer based delay here
    CALL_NULLABLE_WITH_ERROR(init->devop.gpio_rst_set, 1);

    // check the flags
    uint8_t seg_mapping = 0;
    uint8_t com_scan_dir = 0;

    if (init->lr_flip)
        seg_mapping = 0xA0;
    else
        seg_mapping = 0xA1;

    if (init->ud_flip)
        com_scan_dir = 0xC0;
    else
        com_scan_dir = 0xC8;

    uint8_t ssd1306_init_args[] = {
        0xAE,         //--  turn off oled panel
        0x00,         //--  set low column address
        0x10,         //--  set high column address
        0x40,         //--  set start line address
                      //          Set Mapping RAM Display Start Line (0x00~0x3F)
        0x81,         //--  set contrast control register
        0xCF,         //--  set SEG Output Current Brightness
        seg_mapping,  //--  Set SEG/Column Mapping
        com_scan_dir, //--  Set COM/Row Scan Direction
        0xA6,         //--  set normal display
        0xA8,         //--  set multiplex ratio(1 to 64)
        0x3f,         //--  1/64 duty
        0xD3,         //--  set display offset
                      //          Shift Mapping RAM Counter (0x00~0x3F)
        0x00,         //--  not offset
        0xd5,         //--  set display clock divide ratio/oscillator frequency
        0x80,         //--  set divide ratio, Set Clock as 100 Frames/Sec
        0xD9,         //--  set pre-charge period
        0xF1,         //--  Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
        0xDA,         //--  set com pins hardware configuration
        0x12,         //
        0xDB,         //--  set vcomh
        0x40,         //--  Set VCOM Deselect Level

        0x20, //--  Set Page Addressing Mode (0x00/0x01/0x02)
        0x02, //

        0x8D, //--  set Charge Pump enable/disable
        0x10, //--  set(0x10) disable
        0xA4, //--  Disable Entire Display On (0xa4/0xa5)
        0xA6, //--  Disable Inverse Display On (0xa6/a7)
    };

    return write_command_sequence(
        device, ssd1306_init_args,
        sizeof(ssd1306_init_args));
}

error_t ssd1306_display_on(ssd1306_device_t *device)
{
    const uint8_t cmd_seq[] = {
        0x8D, // enable charge pump regulator
        0x14, // turn on charge pump
        0xAF, // enable screen
    };

    return write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

error_t ssd1306_display_off(ssd1306_device_t *device)
{
    const uint8_t cmd_seq[] = {
        0x8D, // enable charge pump regulator
        0x10, // turn off charge pump
        0xAE, // disable screen
    };

    return write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

error_t ssd1306_set_offset_by_addr(ssd1306_device_t *device, uint32_t off)
{
    PARAM_NOT_NULL(device);
    PARAM_CHECK_CODE(off, < SSD1306_GRAM_SIZE, E_MEMORY_OUT_OF_BOUND);

    uint32_t col_off = off % SSD1306_GRAM_LINE_WIDTH;
    uint32_t row_off = off / SSD1306_GRAM_LINE_WIDTH;

    return ssd1306_set_offset(device, col_off, row_off);
}

error_t ssd1306_set_offset(ssd1306_device_t *device,
                           uint32_t col_off, uint32_t row_off)
{
    // argument sanity check
    PARAM_NOT_NULL(device);

    PARAM_CHECK_CODE(col_off, < 128, E_MEMORY_OUT_OF_BOUND);
    PARAM_CHECK_CODE(row_off, < 8, E_MEMORY_OUT_OF_BOUND);

    // assemble the command sequence
    const uint8_t cmd_seq[] = {
        // set the row id
        0xB0 | row_off,
        // set the col id
        0x10 | ((col_off & 0xF0) >> 4),
        0x00 | (col_off & 0x0F),
    };

    error_t ret = write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));

    if (FAILED(ret))
        return ret;

    // update the offset
    uint32_t offset = col_off + row_off * SSD1306_GRAM_LINE_WIDTH;
    device->write_offset = offset;

    return ALL_OK;
}

error_t ssd1306_append_gram(ssd1306_device_t *device,
                            const void *w_data, uint32_t w_size)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(w_data);

    uint32_t current_mem_off =
        device->write_offset;

    if ((current_mem_off + w_size) > SSD1306_GRAM_SIZE)
    {
        dev_err("invalid argument: w_size=%d", w_size);
        return E_MEMORY_OUT_OF_BOUND;
    }

    uint32_t row_pos = current_mem_off / SSD1306_GRAM_LINE_WIDTH;
    uint32_t col_pos = current_mem_off % SSD1306_GRAM_LINE_WIDTH;

    uint32_t data_left = w_size;
    uint8_t *data_ptr = (uint8_t *)w_data;

    uint32_t col_left = SSD1306_GRAM_LINE_WIDTH - col_pos;

    if (col_left == 0)
    {
        col_pos = 0;
        row_pos++;
        col_left = SSD1306_GRAM_LINE_WIDTH;
        CALL_WITH_ERROR_RETURN(ssd1306_set_offset, device, col_pos, row_pos);
    }

    do
    {
        uint32_t write_size = data_left > col_left ? col_left : data_left;
        CALL_WITH_ERROR_RETURN(write_data, device, data_ptr, write_size);

        data_ptr += write_size;
        data_left -= write_size;

        if (data_left > 0)
        {
            row_pos++;
            col_pos = 0;

            CALL_WITH_ERROR_RETURN(ssd1306_set_offset, device, col_pos, row_pos);
        }

    } while (data_left > 0);

    // update the offset
    device->write_offset += w_size;

    return ALL_OK;
}

error_t SSD1306_write_gram(ssd1306_device_t *device,
                           uint32_t mem_off, const void *w_data,
                           uint32_t w_size)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(w_data);

    if (mem_off >= SSD1306_GRAM_SIZE)
    {
        dev_err("invalid argument: mem_off=%d", mem_off);
        return E_MEMORY_OUT_OF_BOUND;
    }

    uint32_t col_off = mem_off % SSD1306_GRAM_LINE_WIDTH;
    uint32_t row_off = mem_off / SSD1306_GRAM_LINE_WIDTH;

    error_t ret = ssd1306_set_offset(device, col_off, row_off);
    if (FAILED(ret))
        return ret;

    return ssd1306_append_gram(device, w_data, w_size);
}

error_t ssd1306_clear_gram(ssd1306_device_t *device, uint8_t fill_data)
{
    CALL_WITH_ERROR_RETURN(ssd1306_set_offset_by_addr, device, 0);

    uint32_t dummy_data[4] = {0};
    memset(dummy_data, fill_data, sizeof(dummy_data));

    const uint32_t fill_count = SSD1306_GRAM_LINE_WIDTH / sizeof(dummy_data);

    for (int i = 0; i < SSD1306_GRAM_LINE_COUNT; i++)
    {
        CALL_WITH_ERROR_RETURN(ssd1306_set_offset, device, 0, i);

        for (int j = 0; j < fill_count; j++)
        {
            CALL_WITH_ERROR_RETURN(write_data, device, dummy_data,
                            sizeof(dummy_data));
        }
    }

    // restore the offset
    CALL_WITH_ERROR_RETURN(ssd1306_set_offset_by_addr, device, device->write_offset);

    return ALL_OK;
}

/******************************************************************************/
/*                     PRIVATE FUNCTION IMPLEMENTATIONS                       */
/******************************************************************************/

static error_t write_command_sequence(ssd1306_device_t *device,
                                      const void *cmd_seq,
                                      uint32_t size)
{
    // aquire the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_aquire);

    // init gpio state
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_dc_set, 0);

    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 0);

    // send commands
    CALL_WITH_ERROR_RETURN(device->device_op.spi_write, cmd_seq, size);

    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 1);

    // reset gpio state
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_dc_set, 1);

    // release the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_release);

    return ALL_OK;
}

static error_t write_data(ssd1306_device_t *device,
                          const void *data,
                          uint32_t size)
{
    // aquire the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_aquire);

    // init gpio state
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_dc_set, 1);

    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 0);

    CALL_WITH_ERROR_RETURN(device->device_op.spi_write, data, size);

    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 1);

    // reset gpio state
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR_RETURN(device->device_op.gpio_dc_set, 1);

    // release the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_release);

    return ALL_OK;
}

/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/

#endif // !#ifdef CONFIG_OLED_SSD1306