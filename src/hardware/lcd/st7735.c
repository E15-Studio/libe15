/**
 * @file st7735.c
 * @author simakeng (simakeng@outlook.com)
 * @brief lcd driver for ST7735 in RGB565 mode
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

#include "st7735.h"

#include <hardware/devop.h>

#include <string.h>
#include <stdlib.h>

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/

/**
 * config registers of ST7735
 */
enum
{
    SWRESET = 0x01,
    SLEEP_OUT = 0x11,
    INVOFF = 0x20,
    INVON = 0x21,
    DISPOFF = 0x28,
    DISPON = 0x29,
    CASET = 0x2A,
    RASET = 0x2B,
    RAMWR = 0x2C,
    RAMRD = 0x2E,
    FRMCTR1 = 0xB1,
    FRMCTR2 = 0xB2,
    FRMCTR3 = 0xB3,
    INVCTR = 0xB4,
    PWCTR1 = 0xC0,
    PWCTR2 = 0xC1,
    PWCTR3 = 0xC2,
    PWCTR4 = 0xC3,
    PWCTR5 = 0xC4,
    VMCTR1 = 0xC5,
    GMCTRP1 = 0xE0,
    GMCTRN1 = 0xE1,
    GCV = 0xFC,
    COLMOD = 0x3A,
    MADCTL = 0x36,
};

/******************************************************************************/
/*                              TYPE DEFINITIONS                              */
/******************************************************************************/

static struct
{
    rgb565_t clear_color;
    uint16_t lines_left;
} st7735_gram_clear_config;

typedef struct
{
    rgb565_t *buf;
    uint16_t buf_size;
} st7735_gram_clear_args_t;

/******************************************************************************/
/*                        PRIVATE FUNCTION DEFINITIONS                        */
/******************************************************************************/

static error_t st7735_write_command(st7735_device_t *device, uint8_t command, const void *pargs, uint32_t nargs);
static error_t st7735_write_pixel_data(st7735_device_t *device, const rgb565_t *pdata, uint32_t ndata);
static error_t st7735_clear_gram_cplt_handler(st7735_device_t *device, void *pargs);

/******************************************************************************/
/*                      PUBLIC FUNCTION IMPLEMENTATIONS                       */
/******************************************************************************/

error_t st7735_init(st7735_device_t *device, st7735_device_init_t *init)
{
    // argument sanity check
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(init);

    if (init->device_op.bus_mode == ST7735_BUS_MODE_UNKNOW)
    {
        print(ERROR, "bus_mode is unknow, please configure a correct bus_mode.\n");
        return E_INVALID_ARGUMENT;
    }
    else if (init->device_op.bus_mode == ST7735_BUS_MODE_SPI)
    {
        PARAM_NOT_NULL(init->device_op.spi.write);
        PARAM_NOT_NULL(init->device_op.spi.gpio_cs_set);
        PARAM_NOT_NULL(init->device_op.spi.gpio_dc_set);
    }
    else if (init->device_op.bus_mode == ST7735_BUS_MODE_8080)
    {
        PARAM_NOT_NULL(init->device_op.bus80.command_write);
        PARAM_NOT_NULL(init->device_op.bus80.data_write);
    }

    PARAM_NOT_NULL(init->device_op.delay);

    uint32_t width = init->display_area.right - init->display_area.left;
    uint32_t height = init->display_area.bottom - init->display_area.top;

    if (width > 161)
        print(WARN,
              "resolution.x (%d) is larger than 161, this may cause unexpected behavior.\n",
              width);
    if (height > 161)
        print(WARN,
              "resolution.y (%d) is larger than 320, this may cause unexpected behavior.\n",
              height);

    memset(device, 0, sizeof(st7735_device_t));

    // copy all the init data
    device->device_op = init->device_op;
    device->display_area = init->display_area;

    // init gpio_state (only for spi mode)
    if (init->device_op.bus_mode == ST7735_BUS_MODE_SPI)
    {
        CALL_NULLABLE_WITH_ERROR(init->device_op.spi.gpio_cs_set, 1);
        CALL_NULLABLE_WITH_ERROR(init->device_op.spi.gpio_rst_set, 1);
        CALL_WITH_ERROR_RETURN(init->device_op.spi.gpio_dc_set, 0);
    }

    // init backlight
    CALL_NULLABLE_WITH_ERROR(init->device_op.pwm_change_duty, 0);

    // * 0. Reset the LCD controller * //

    // soft reset
    // Frame memory contents are unaffected by this command.
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, SWRESET, NULL, 0);

    // wait 120ms
    CALL_WITH_ERROR_RETURN(device->device_op.delay, 120);

    // sleep out
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, SLEEP_OUT, NULL, 0);

    // wait 120ms
    CALL_WITH_ERROR_RETURN(device->device_op.delay, 120);

    // * 1. Setup framerate. * //

    // Frame rate=fosc/((RTN[A-D] x 2 + 40) x (LINE + FP[A-D] + BP[A-D] +2))
    // fosc = 850kHz

    // Frame Rate Control (In normal mode/ Full colors)
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, FRMCTR1,
                           "\x05"  // RTNA=5
                           "\x3C"  // FPA=60
                           "\x3C", // BPA=60
                           3);

    // Frame Rate Control (In Idle mode/ 8-colors)
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, FRMCTR2,
                           "\x05"  // RTNB=5
                           "\x3C"  // FPA=60
                           "\x3C", // BPA=60

                           3);

    // Frame Rate Control (In Idle mode/ 8-colors)
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, FRMCTR3,
                           "\x05"  // RTNC=5
                           "\x3C"  // FPC=60
                           "\x3C"  // BPC=60
                           "\x05"  // RTND=5
                           "\x3C"  // FPD=60
                           "\x3C", // BPD=60
                           6);

    // * 2. Power Settings * //

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, PWCTR1,
                           "\xAB"  // AVDD=5V, GVDD=4.15V
                           "\x0B"  // GVCL=-4.15V
                           "\x04", // MODE=2X (?)
                           3);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, PWCTR2,
                           "\xC5", // V25=2.4 VGHBT=3*AVDD-0.5 VGL=-7.5
                           1);

    // Power Control 3 (in Normal mode/ Full colors)
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, PWCTR3,
                           "\x0D"
                           "\x00",
                           2);

    // PWCTR4 (C3h): Power Control 4 (in Idle mode/ 8-colors)
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, PWCTR4,
                           "\x8D"
                           "\x6A",
                           2);

    // PWCTR5 (C4h): Power Control 5 (in Partial mode/ full-colors)
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, PWCTR5,
                           "\x8D"
                           "\xEE",
                           2);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, VMCTR1,
                           "\x0F", // VCOM=-0.8
                           1);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, VMCTR1,
                           "\x0F", // VCOM=-0.8
                           1);

    // * 3. Color settings * //

    //  GMCTRP1 (E0h): Gamma (‘+’polarity) Correction Characteristics Setting
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, GMCTRP1,
                           "\x07\x0E\x08\x07\x10\x07\x02\x07"
                           "\x09\x0F\x25\x36\x00\x08\x04\x10",
                           16);

    // GMCTRN1 (E1h): Gamma ‘-’polarity Correction Characteristics Setting
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, GMCTRN1,
                           "\x0A\x0D\x08\x07\x0F\x07\x02\x07"
                           "\x09\x0F\x25\x35\x00\x09\x04\x10",
                           16);

    // Gate Pump Clock Frequency Variable
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, GCV,
                           "\x80", // Clk_Variable=Small, GCV_Enable=disbale
                           1);

    // * 4. Interface Settings * //

    // COLMOD (3Ah): Interface Pixel Format
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, COLMOD,
                           "\x05", // 16-bit/pixel
                           1);

    // MADCTL (36h): Memory Data Access Control
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, MADCTL,
                           "\x08", // MX=0 MY=0 MV=0 ML=0 BGR=1 MH=0
                           1);

    // * 5. Display Settings * //

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, INVON, NULL, 0);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, DISPOFF, NULL, 0);

    CALL_WITH_ERROR_RETURN(st7735_display_set_window, device,
                           device->display_area);

    return ALL_OK;
}

error_t st7735_display_on(st7735_device_t *device)
{
    PARAM_NOT_NULL(device);
    st7735_device_op_t *device_op = &device->device_op;

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, DISPON, NULL, 0);
    CALL_NULLABLE_WITH_ERROR(device_op->pwm_change_duty, 10000);

    return ALL_OK;
}

error_t st7735_display_off(st7735_device_t *device)
{
    PARAM_NOT_NULL(device);
    st7735_device_op_t *device_op = &device->device_op;

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, DISPOFF, NULL, 0);
    CALL_NULLABLE_WITH_ERROR(device_op->pwm_change_duty, 0);

    return ALL_OK;
}

error_t st7735ex_set_lcd_brightness(st7735_device_t *device,
                                    uint32_t brightness)
{
    PARAM_NOT_NULL(device);

    if (brightness > 10000)
    {
        print(WARN, "brightness (%d) is larger than 10000, this may cause "
                    "unexpected behavior.\n",
              brightness);
    }

    st7735_device_op_t *device_op = &device->device_op;

    if (device_op->pwm_change_duty == NULL)
    {
        print(WARN, "pwm_change_duty is NULL, adjust brightness will not "
                    "work.\n");
    }

    CALL_NULLABLE_WITH_ERROR(device_op->pwm_change_duty, brightness);

    return ALL_OK;
}

error_t st7735_display_set_window(st7735_device_t *device, rect_t rect)
{
    PARAM_NOT_NULL(device);

    PARAM_CHECK(rect.top, >= 0);
    PARAM_CHECK(rect.bottom, >= 0);
    PARAM_CHECK(rect.left, >= 0);
    PARAM_CHECK(rect.right, >= 0);
    PARAM_CHECK(rect.top, <= 162);
    PARAM_CHECK(rect.bottom, <= 162);
    PARAM_CHECK(rect.left, <= 162);
    PARAM_CHECK(rect.right, <= 162);

    rect.bottom -= 1;
    rect.right -= 1;

    struct
    {
        uint16_t begin;
        uint16_t end;
    } args;

    args.begin = U16ECV(rect.left);
    args.end = U16ECV(rect.right);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, CASET, &args, 4);

    args.begin = U16ECV(rect.top);
    args.end = U16ECV(rect.bottom);
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, RASET, &args, 4);

    return ALL_OK;
}

error_t st7735_append_gram(st7735_device_t *device, const rgb565_t *w_data,
                           uint32_t npixel)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(w_data);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, RAMWR, NULL, 0);

    CALL_WITH_ERROR_RETURN(st7735_write_pixel_data, device, w_data, npixel);

    return ALL_OK;
}

error_t st7735_display_clear_gram(st7735_device_t *device, rgb565_t color)
{
    PARAM_NOT_NULL(device);
    st7735_device_op_t *device_op = &device->device_op;

    CALL_WITH_ERROR_RETURN(st7735_display_set_window, device,
                           device->display_area);

    CALL_WITH_ERROR_RETURN(st7735_write_command, device, RAMWR, NULL, 0);

    error_t err = ALL_OK;

    uint32_t width = device->display_area.right - device->display_area.left;
    uint32_t height = device->display_area.bottom - device->display_area.top;
    uint32_t npixels = width * height;

    if (device_op->host_is_big_endian != true)
    {
        color = U16ECV(color);
    }

    if (device->device_op.bus_mode == ST7735_BUS_MODE_SPI)
    {
        CALL_WITH_CODE_GOTO(err, exit, device_op->spi.gpio_dc_set, 1);

        CALL_WITH_CODE_GOTO(err, exit, device_op->spi.gpio_cs_set, 0);
        for (uint32_t i = 0; i < npixels; i++)
        {
            CALL_WITH_CODE_GOTO(err, exit, device_op->spi.write, 2, &color);
        }
        CALL_WITH_CODE_GOTO(err, exit, device_op->spi.gpio_dc_set, 1);
        CALL_WITH_CODE_GOTO(err, exit, device_op->spi.gpio_cs_set, 1);
    }
    else if (device->device_op.bus_mode == ST7735_BUS_MODE_8080)
    {
        CALL_WITH_CODE_GOTO(err, exit, device_op->bus80.data_set, npixels, color);
    }
    else
    {
        err = E_INVALID_ARGUMENT;
        goto exit;
    }
exit:
    return err;
}

error_t st7735_clear_gram_set_buf(st7735_device_t *device, st7735_gram_clear_args_t *args)
{

    uint32_t width = device->display_area.right - device->display_area.left;
    uint32_t lines_can_write = args->buf_size / width;
    uint32_t lines_to_write = st7735_gram_clear_config.lines_left;

    if (lines_to_write > lines_can_write)
        lines_to_write = lines_can_write;

    uint32_t npixels = width * lines_to_write;

    CALL_WITH_ERROR_RETURN(st7735_update_gram_set_buff, device,
                           npixels, args->buf);

    st7735_gram_clear_config.lines_left -= lines_to_write;

    return ALL_OK;
}

error_t st7735_display_clear_gram_async(st7735_device_t *device, rgb565_t color)
{
    PARAM_NOT_NULL(device);

    const int width = device->display_area.right - device->display_area.left;
    const int height = device->display_area.bottom - device->display_area.top;

    // alloc buffer
    const int cache_line_count = 5;
    const int cache_buffer_size = cache_line_count * width;
    rgb565_t *pbuf = NULL;
    pbuf = malloc(cache_buffer_size * sizeof(rgb565_t));

    st7735_gram_clear_args_t *args = malloc(sizeof(st7735_gram_clear_args_t));
    args->buf_size = cache_buffer_size;
    args->buf = pbuf;

    if (pbuf == NULL || args == NULL)
    {
        print(ERROR, "malloc failed, can not allocate buffer "
                     "for gram clear.\n");
        return E_MEMORY_ALLOC_FAILED;
    }

    // prepare the buffer data
    for (int i = 0; i < cache_buffer_size; i++)
        pbuf[i] = U16ECV(color);

    // prepare update window
    CALL_WITH_ERROR_RETURN(st7735_display_set_window, device,
                           device->display_area);

    // update flags
    st7735_gram_clear_config.clear_color = color;
    st7735_gram_clear_config.lines_left = height;

    /**
     * This function serves as a example of how to use async gram update
     * in the function, we will clear the gram with a specific color
     *
     * the `st7735_clear_gram_cplt_handler` will set the buffer and call
     * st7735_update_gram_stream_start to start the transfer, we call it
     * to bootstrap the first transfer.
     */
    st7735_clear_gram_cplt_handler(device, args);

    return ALL_OK;
}

error_t st7735_update_gram_set_buff(
    st7735_device_t *device,
    uint32_t buffer_size,
    rgb565_t *pbuf)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(pbuf);

    switch (device->async_state)
    {
    case ST7735_ASYNC_STATE_BUFFER_LOADED:
    case ST7735_ASYNC_STATE_IDLE:
        device->async_state = ST7735_ASYNC_STATE_BUFFER_LOADED;
        break;
    case ST7735_ASYNC_STATE_BUFFER_RELOADED:
    case ST7735_ASYNC_STATE_TRANSFERING:
        device->async_state = ST7735_ASYNC_STATE_BUFFER_RELOADED;
        break;
    default:
        print(ERROR, "The buffer can only be accessed when "
                     "idle or transfering.\n");
        return E_INVALID_OPERATION;
    }

    device->gram_tx_buf_size = buffer_size;
    device->gram_tx_buf = pbuf;
    return ALL_OK;
}

error_t st7735_update_gram_stream_start(
    st7735_device_t *device,
    st7735_transfer_cplt_handler_t handler,
    void *params)
{
    PARAM_NOT_NULL(device);

    if (device->async_state == ST7735_ASYNC_STATE_TRANSFERING)
    {
        print(ERROR, "There is a transfering operation ongoing.\n");
        return E_INVALID_OPERATION;
    }

    if (device->async_state != ST7735_ASYNC_STATE_BUFFER_LOADED &&
        device->async_state != ST7735_ASYNC_STATE_BUFFER_RELOADED)
    {
        print(ERROR, "There is no data in buffer, please call"
                     " lock() and unlock() first.\n");
        return E_INVALID_OPERATION;
    }

    device->handler = handler;
    device->handler_params = params;

    error_t err = ALL_OK;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);

    // check if is the first transfer
    // in this case, we need to send mem write command
    // other wise, we just need to send data
    if (device->async_state != ST7735_ASYNC_STATE_BUFFER_RELOADED)
    {
        CALL_WITH_CODE_GOTO(err, error_exit, st7735_write_command,
                            device, RAMWR, NULL, 0);

        st7735_device_op_t *device_op = &device->device_op;
        if (device->device_op.bus_mode == ST7735_BUS_MODE_SPI)
        {
            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_dc_set, 1);

            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_cs_set, 0);
        }
        else if (device->device_op.bus_mode == ST7735_BUS_MODE_8080)
        {
            // nothing need to be done here.
        }
        else
        {
            err = E_INVALID_ARGUMENT;
            goto error_exit;
        }
    }

    device->async_state = ST7735_ASYNC_STATE_TRANSFERING;

    // send the data
    if (device->device_op.bus_mode == ST7735_BUS_MODE_SPI)
    {
        CALL_WITH_CODE_GOTO(err, error_exit,
                            device->device_op.spi.write_async_start,
                            device->gram_tx_buf_size,
                            device->gram_tx_buf);
    }
    else if (device->device_op.bus_mode == ST7735_BUS_MODE_8080)
    {
        CALL_WITH_CODE_GOTO(err, error_exit,
                            device->device_op.bus80.data_write_async_start,
                            device->gram_tx_buf_size,
                            device->gram_tx_buf);
    }
    else
    {
        err = E_INVALID_ARGUMENT;
        goto error_exit;
    }
    return ALL_OK;

error_exit:
    device->async_state = ST7735_ASYNC_STATE_IDLE;
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

error_t st7735_async_completed_notify(st7735_device_t *device)
{
    PARAM_NOT_NULL(device);

    if (device->async_state != ST7735_ASYNC_STATE_TRANSFERING)
    {
        print(ERROR, "There is no transfering operation\n");
        return E_INVALID_OPERATION;
    }

    // reset the buffer size
    device->gram_tx_buf_size = 0;

    error_t err = ALL_OK;

    // call callback, this may change the buffer
    CALL_NULLABLE_WITH_ERROR_EXIT(err, error_exit, device->handler, device, device->handler_params);

    // which means callback changed the buffer
    if (device->gram_tx_buf_size != 0)
    {
        ; // do nothing
    }
    else
    {
        // mark as last transfer, send ending sequence
        st7735_device_op_t *device_op = &device->device_op;
        if (device->device_op.bus_mode == ST7735_BUS_MODE_SPI)
        {
            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_dc_set, 1);

            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_cs_set, 1);
        }
        else if (device->device_op.bus_mode == ST7735_BUS_MODE_8080)
        {
            // nothing need to be done here.
            // 80 bus does not need a ending sequence
        }
        else
        {
            err = E_INVALID_ARGUMENT;
            goto error_exit;
        }

        device->async_state = ST7735_ASYNC_STATE_IDLE;
    }

    return ALL_OK;

error_exit:
    device->async_state = ST7735_ASYNC_STATE_IDLE;
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

error_t st7735_wait_async_complete(st7735_device_t *device, uint32_t timeout)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(device->device_op.sys_get_tick_ms);

    uint32_t ms = device->device_op.sys_get_tick_ms();
    while (device->async_state != ST7735_ASYNC_STATE_IDLE)
    {
        uint32_t delta = device->device_op.sys_get_tick_ms() - ms;
        if (delta > timeout)
            return E_HARDWARE_TIMEOUT;
    }

    return ALL_OK;
}

error_t st7735_read_gram(st7735_device_t *device, uint32_t npixel,
                         rgb565_t *pbuf, bool _continue)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(pbuf);

    if (!_continue)
    {
        // setting pixel format: 16bit RGB 565
        CALL_WITH_ERROR_RETURN(st7735_write_command, device, COLMOD, "\x06", 1);
        CALL_WITH_ERROR_RETURN(st7735_write_command, device, RAMRD, NULL, 0);
    }

    error_t err = ALL_OK;
    st7735_device_op_t device_op = device->device_op;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);
    uint32_t num_8bit_reads = npixel * 3;
    uint32_t num_16bit_reads = num_8bit_reads / 2;
    if (device_op.bus_mode == ST7735_BUS_MODE_8080)
    {
        if (device_op.host_is_big_endian == true)
            CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_read,
                                num_8bit_reads, pbuf);
        else
            for (int i = 0; i < num_16bit_reads; i++)
            {
                rgb565_t temp = 0;
                CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_read,
                                    2, &temp);
                pbuf[i] = temp; // U16ECV(temp);
            }
    }
    else // wtf?
    {
        return E_INVALID_ARGUMENT;
    }

exit:
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

error_t st7735_read_gram_end(st7735_device_t *device)
{
    CALL_WITH_ERROR_RETURN(st7735_write_command, device, COLMOD, "\x05", 1);
    return ALL_OK;
}

/******************************************************************************/
/*                     PRIVATE FUNCTION IMPLEMENTATIONS                       */
/******************************************************************************/

static error_t st7735_write_command(st7735_device_t *device, uint8_t command,
                                    const void *pargs, uint32_t nargs)
{
    PARAM_NOT_NULL(device);
    error_t err = ALL_OK;
    st7735_device_op_t device_op = device->device_op;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);

    if (device_op.bus_mode == ST7735_BUS_MODE_SPI)
    {

        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_dc_set, 0);
        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_cs_set, 0);
        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.write, 1, &command);
        if (nargs != 0)
        {
            CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_dc_set, 1);
            CALL_WITH_CODE_GOTO(err, exit, device_op.spi.write, nargs, pargs);
            CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_cs_set, 1);
        }
        else
        {
            CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_cs_set, 1);
            CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_dc_set, 1);
        }
    }
    else if (device_op.bus_mode == ST7735_BUS_MODE_8080)
    {
        uint16_t data = command;
        if (device_op.host_is_big_endian == false)
            data = U16ECV(data);

        CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.command_write, 2, &data);
        CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_write, nargs, pargs);
    }
    else // wtf?
    {
        err = E_INVALID_ARGUMENT;
        goto exit;
    }

exit:
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

static error_t st7735_write_pixel_data(st7735_device_t *device,
                                       const rgb565_t *pdata, uint32_t ndata)
{
    PARAM_NOT_NULL(device);
    error_t err = ALL_OK;
    st7735_device_op_t device_op = device->device_op;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);

    if (device_op.bus_mode == ST7735_BUS_MODE_SPI)
    {

        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_dc_set, 1);
        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_cs_set, 0);

        for (uint32_t i = 0; i < ndata; i++)
        {
            rgb565_t data = U16ECV(pdata[i]);
            CALL_WITH_CODE_GOTO(err, exit, device_op.spi.write, 2, &data);
        }

        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_dc_set, 1);
        CALL_WITH_CODE_GOTO(err, exit, device_op.spi.gpio_cs_set, 1);
    }
    else if (device_op.bus_mode == ST7735_BUS_MODE_8080)
    {
        if (device_op.host_is_big_endian == true)
            CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_write, ndata * 2, pdata);

        for (int i = 0; i < ndata; i++)
        {
            rgb565_t data = U16ECV(pdata[i]);
            CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_write, 2, &data);
        }
    }
    else // wtf?
    {
        return E_INVALID_ARGUMENT;
    }

exit:
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

/**
 * @brief set the buffer and start the transfer
 *
 * @param pargs
 * @return error_t
 */
static error_t st7735_clear_gram_cplt_handler(st7735_device_t *device, void *pargs)
{
    st7735_gram_clear_args_t *args = pargs;

    PARAM_NOT_NULL(device);

    if (st7735_gram_clear_config.lines_left > 0)
    {
        CALL_WITH_ERROR_RETURN(st7735_clear_gram_set_buf, device, args);

        CALL_WITH_ERROR_RETURN(st7735_update_gram_stream_start, device,
                               st7735_clear_gram_cplt_handler,
                               pargs);
    }
    else
    {
        // release the buffer
        free(args->buf);
        free(args);
    }

    return ALL_OK;
}

/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/
