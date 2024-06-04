/**
 * @file SSD1315.c
 * @author simakeng (simakeng@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-06-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <string.h>
#include <stddef.h>
#include <libe15-devop.h>
#include "SSD1315.h"

static error_t SSD1315_write_command_sequence(SSD1315_device_t *device, const void *cmd_seq, uint32_t size)
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

    return ALL_OK;
}

error_t SSD1315_init(SSD1315_device_t *device, SSD1315_Init_t *init)
{
    memset(device, 0, sizeof(SSD1315_device_t));

    // init pointers
    device->colum_offset = 0;
    device->row_offset = 0;

    // check all arguments

    ARGUMENT_CHECK(device);
    ARGUMENT_CHECK(init);

    ARGUMENT_CHECK(init->devop.gpio_cs_set);
    ARGUMENT_CHECK(init->devop.gpio_dc_set);
    ARGUMENT_CHECK(init->devop.spi_write);

    // init gpio state
    CALL_NULLABLE_WITH_ERROR(init->devop.gpio_rst_set, 1);

    // reset the device
    CALL_NULLABLE_WITH_ERROR(init->devop.gpio_rst_set, 0);
    // TODO: add timer based delay here
    CALL_NULLABLE_WITH_ERROR(init->devop.gpio_rst_set, 1);

    // copy the device maniuplation functions
    device->device_op = init->devop;

    // check the flags
    uint8_t seg_mapping = 0;
    uint8_t com_scan_dir = 0;

    if (init->flags & SSD1315_INIT_FLAG_LR_SWAP)
        seg_mapping = 0xA0;
    else
        seg_mapping = 0xA1;

    if (init->flags & SSD1315_INIT_FLAG_UD_SWAP)
        com_scan_dir = 0xC0;
    else
        com_scan_dir = 0xC8;

    uint8_t SSD1315_init_args[] = {
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
                      //
        0xB0,         //--  set row address
        0x10,         //--  set column address[7:4]
        0x00,         //--  set column address[3:0]
    };

    return SSD1315_write_command_sequence(
        device, SSD1315_init_args,
        sizeof(SSD1315_init_args));
}

error_t SSD1315_display_on(SSD1315_device_t *device)
{
    const uint8_t cmd_seq[] = {
        0x8D, // enable charge pump regulator
        0x14, // turn on charge pump
        0xAF, // enable screen
    };

    return SSD1315_write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

error_t SSD1315_display_off(SSD1315_device_t *device)
{
    const uint8_t cmd_seq[] = {
        0x8D, // enable charge pump regulator
        0x10, // turn off charge pump
        0xAE, // disable screen
    };

    return SSD1315_write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

error_t SSD1315_set_offset(SSD1315_device_t *device, uint32_t col_off, uint32_t row_off)
{
    // argument sanity check
    ARGUMENT_CHECK(device);

    if (col_off >= 128 || row_off >= 8)
    {
        error("invalid argument: col_off=%d, row_off=%d", col_off, row_off);
        return E_MEMORY_OUT_OF_BOUND;
    }

    // change the index
    device->colum_offset = col_off;
    device->row_offset = row_off;

    // assemble the command sequence
    const uint8_t cmd_seq[] = {
        // set the row id
        0xB0 | row_off,
        // set the col id
        0x10 | ((col_off & 0xF0) >> 4),
        0x00 | (col_off & 0x0F),
    };

    return SSD1315_write_command_sequence(
        device, cmd_seq,
        sizeof(cmd_seq));
}

error_t SSD1315_set_mem_offset(SSD1315_device_t *device, uint32_t mem_off)
{
    // argument sanity check
    ARGUMENT_CHECK(device);

    if (mem_off >= 1024)
    {
        error("invalid argument: mem_off=%d", mem_off);
        return E_MEMORY_OUT_OF_BOUND;
    }

    uint32_t col_off = mem_off % SSD1315_GRAM_LINE_WIDTH;
    uint32_t row_off = mem_off / SSD1315_GRAM_LINE_WIDTH;

    return SSD1315_set_offset(device, col_off, row_off);
}

static error_t SSD1315_write_data_to_bus(SSD1315_device_t *device, const void *w_data, uint32_t w_size)
{
    // enter active state
    CALL_WITH_ERROR(device->device_op.gpio_cs_set, 0);

    // send the display data
    CALL_WITH_ERROR(device->device_op.spi_write, w_data, w_size);

    // reset gpio state
    CALL_WITH_ERROR(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR(device->device_op.gpio_dc_set, 1);

    return ALL_OK;
}

error_t SSD1315_append_gram(SSD1315_device_t *device, const void *w_data, uint32_t w_size)
{
    // argument sanity check
    ARGUMENT_CHECK(device);
    ARGUMENT_CHECK(w_data);

    uint32_t currnet_mem_off =
        device->colum_offset +
        device->row_offset * SSD1315_GRAM_LINE_WIDTH;

    // check the size
    if ((currnet_mem_off + w_size) > SSD1315_GRAM_SIZE)
    {
        error("invalid argument: w_size=%d", w_size);
        return E_MEMORY_NOT_ENOUTH;
    }

    // aquire the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_aquire);

    // init gpio state
    CALL_WITH_ERROR(device->device_op.gpio_cs_set, 1);
    CALL_WITH_ERROR(device->device_op.gpio_dc_set, 1);

    uint32_t buf_wr_left = w_size;
    const uint8_t *buf_wr_ptr = w_data;
    while (buf_wr_left != 0)
    {
        // how many space are left in this line
        uint32_t line_space_left =
            SSD1315_GRAM_LINE_WIDTH - (currnet_mem_off % SSD1315_GRAM_LINE_WIDTH);
        // no need to write more than the buffer size
        if (line_space_left > buf_wr_left)
            line_space_left = buf_wr_left;

        // do the trick
        CALL_WITH_ERROR(SSD1315_write_data_to_bus, device, buf_wr_ptr, line_space_left);

        // update ptr
        buf_wr_ptr += line_space_left;
        buf_wr_left -= line_space_left;

        device->colum_offset += line_space_left;

        // check if new line needed
        if (buf_wr_left > 0)
        {
            // change to next line
            device->colum_offset = 0;
            device->row_offset += 1;
            SSD1315_set_offset(device, device->colum_offset, device->row_offset);
        }
    }

    // release the spi bus
    CALL_NULLABLE_WITH_ERROR(device->device_op.spi_release);

    uint32_t new_col_off = (currnet_mem_off + w_size) % SSD1315_GRAM_LINE_WIDTH;
    uint32_t new_row_off = (currnet_mem_off + w_size) / SSD1315_GRAM_LINE_WIDTH;

    device->colum_offset = new_col_off;
    device->row_offset = new_row_off;
    return ALL_OK;
}

error_t SSD1315_write_gram(SSD1315_device_t *device, uint32_t mem_off, const void *w_data, uint32_t w_size)
{
    // argument sanity check
    ARGUMENT_CHECK(device);
    ARGUMENT_CHECK(w_data);

    // check the size
    if ((mem_off + w_size) > SSD1315_GRAM_SIZE)
    {
        error("invalid argument: w_size=%d", w_size);
        return E_MEMORY_NOT_ENOUTH;
    }

    uint32_t col_off = (mem_off) % SSD1315_GRAM_LINE_WIDTH;
    uint32_t row_off = (mem_off) / SSD1315_GRAM_LINE_WIDTH;

    CALL_WITH_ERROR(SSD1315_set_offset, device, col_off, row_off);

    CALL_WITH_ERROR(SSD1315_append_gram, device, w_data, w_size);

    return ALL_OK;
}

error_t SSD1315_clear_gram(SSD1315_device_t *device)
{
    // argument sanity check
    ARGUMENT_CHECK(device);
    uint32_t zeros[4] = {0, 0, 0, 0};
    for (int i = 0; i < 8; i++)
    {
        CALL_WITH_ERROR(SSD1315_set_offset, device, 0, i);
        for (int j = 0; j < 8; j++)
            CALL_WITH_ERROR(SSD1315_write_data_to_bus, device, zeros, 16);
    }
    return ALL_OK;
}
