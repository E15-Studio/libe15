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
 * directory of this project named "COPYING" file.
 * 
 * https://www.gnu.org/licenses/gpl-3.0.html
 * 
 * *****************************************************************************
 * 
 */

#include <stddef.h>
#include <hardware/devop.h>

#include "ssd1306.h"

#ifdef CONFIG_OLED_SSD1306

static error_t SSD1306_write_command_sequence(SSD1306_Device_t *device, const void *cmd_seq, uint32_t size)
{
    // aquire the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_aquire);

    // init gpio state
    CALL_WITH_ERROR(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR(device->device_op.gpio_dc_set, 0);

    // send commands byte by byte
    for (int i = 0; i < size; i++)
    {
        uint8_t cmd = ((uint8_t *)cmd_seq)[i];
        CALL_WITH_ERROR(device->device_op.gpio_cs_set, 0);
        CALL_WITH_ERROR(device->device_op.spi_write, &cmd, 1);
        CALL_WITH_ERROR(device->device_op.gpio_cs_set, 1);
    }

    // reset gpio state
    CALL_WITH_ERROR(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR(device->device_op.gpio_dc_set, 1);

    // release the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_release);
}

error_t SSD1306_Init(SSD1306_Device_t *device, SSD1306_Init_t *init)
{
    // check all arguments

    ARGUMENT_CHECK(device);
    ARGUMENT_CHECK(init);

    ARGUMENT_CHECK(init->devop.gpio_cs_set);
    ARGUMENT_CHECK(init->devop.gpio_dc_set);
    ARGUMENT_CHECK(init->devop.spi_write);

    // copy the device maniuplation functions
    device->device_op = init->devop;

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
        0x20,         //--  Set Page Addressing Mode (0x00/0x01/0x02)
        0x02,         //
        0x8D,         //--  set Charge Pump enable/disable
        0x14,         //--  set(0x10) disable
        0xA4,         //--  Disable Entire Display On (0xa4/0xa5)
        0xA6,         //--  Disable Inverse Display On (0xa6/a7)
    };

    return SSD1306_write_command_sequence(
        device, ssd1306_init_args,
        sizeof(ssd1306_init_args));
}

error_t SSD1306_display_on(SSD1306_Device_t *device)
{
    const uint8_t cmd_seq[] = {
        0x8D, // enable charge pump regulator
        0x14, // turn on charge pump
        0xAF, // enable screen
    };

    return SSD1306_write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

error_t SSD1306_display_off(SSD1306_Device_t *device)
{
    const uint8_t cmd_seq[] = {
        0x8D, // enable charge pump regulator
        0x10, // turn off charge pump
        0xAE, // disable screen
    };

    return SSD1306_write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

#endif // !#ifdef CONFIG_OLED_SSD1306