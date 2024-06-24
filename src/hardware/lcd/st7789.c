/**
 * @file st7789.c
 * @author simakeng (simakeng@outlook.com)
 * @brief lcd driver for ST7789 in RGB565 mode
 * @version 0.1
 * @date 2023-07-19
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "st7789.h"

#include <libe15-hw/devop.h>
#include <libe15-hw/timer.h>

#include <string.h>
#include <stdlib.h>

#include <io_adapter.h>
/**
 * config registers of ST7789
 */
enum
{
    SWRESET = 0x01,
    SLEEP_OUT = 0x11,
    INVON = 0x21,
    DISPOFF = 0x28,
    DISPON = 0x29,
    MADCTL = 0x36,
    COLMOD = 0x3A,
    FRCTRL1 = 0xB3,
    FRCTRL2 = 0xC6,
    PORCTRL = 0xB2,
    GCTRL = 0xB7,
    VCOMS = 0xBB,
    LCMCTRL = 0xC0,
    VDVVRHEN = 0xC2,
    VRHS = 0xC3,
    VDVSET = 0xC4,
    PWCTRL1 = 0xD0,
    PVGAMCTRL = 0xE0,
    NVGAMCTRL = 0xE1,
    CASET = 0x2A,
    RASET = 0x2B,
    RAMWR = 0x2C,
    TEON = 0x35,
};

error_t st7789_write_command(st7789_device_t *device, uint8_t command, const void *pargs, uint32_t nargs)
{
    PARAM_NOT_NULL(device);
    error_t err = ALL_OK;
    st7789_device_op_t device_op = device->device_op;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);

    if (device_op.bus_mode == ST7789_BUS_MODE_SPI)
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
    else if (device_op.bus_mode == ST7789_BUS_MODE_8080)
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

error_t st7789_write_pixel_data(st7789_device_t *device, const rgb565_t *pdata, uint32_t ndata)
{
    PARAM_NOT_NULL(device);
    error_t err = ALL_OK;
    st7789_device_op_t device_op = device->device_op;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);

    if (device_op.bus_mode == ST7789_BUS_MODE_SPI)
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
    else if (device_op.bus_mode == ST7789_BUS_MODE_8080)
    {
        if (device_op.host_is_big_endian == true)
            CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_write, ndata * 2, pdata);

        // we need to convert the data to big endian
        const int max_cache_size = 1024;
        uint32_t num_transtfer = 0;
        uint32_t cache_size = ndata > max_cache_size ? max_cache_size : ndata;
        rgb565_t *gram_buf = malloc(cache_size * sizeof(rgb565_t));
        if (gram_buf != NULL)
        {
            // malloc success, use heap buffer to speed up the transfer
            num_transtfer = (ndata + (cache_size - 1)) / cache_size;
            for (int i = 0; i < num_transtfer; i++)
            {
                uint32_t byte_transfer = cache_size;
                if (i == num_transtfer - 1)
                    byte_transfer = ndata - (num_transtfer - 1) * cache_size;
                uint32_t offset = i * cache_size;

                MEMCOPY_FUNCMAP(gram_buf, pdata + offset, byte_transfer, rgb565_t, U16ECV);

                CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_write,
                                     byte_transfer, gram_buf);
            }
            free(gram_buf);
        }
        else
        {
            // malloc failed, do it one by one
            num_transtfer = ndata;
            for (int i = 0; i < num_transtfer; i++)
            {
                rgb565_t data = U16ECV(pdata[i]);
                CALL_WITH_CODE_GOTO(err, exit, device_op.bus80.data_write, 2, &data);
            }
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

error_t st7789_init(st7789_device_t *device, st7789_device_init_t *init)
{
    // argument sanity check
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(init);

    if (init->device_op.bus_mode == ST7789_BUS_MODE_UNKNOW)
    {
        print(LERROR, "bus_mode is unknow, please configure a correct bus_mode.\n");
        return E_INVALID_ARGUMENT;
    }
    else if (init->device_op.bus_mode == ST7789_BUS_MODE_SPI)
    {
        PARAM_NOT_NULL(init->device_op.spi.write);
        PARAM_NOT_NULL(init->device_op.spi.gpio_cs_set);
        PARAM_NOT_NULL(init->device_op.spi.gpio_dc_set);
    }
    else if (init->device_op.bus_mode == ST7789_BUS_MODE_8080)
    {
        PARAM_NOT_NULL(init->device_op.bus80.command_write);
        PARAM_NOT_NULL(init->device_op.bus80.data_write);
    }

    if (init->resolution.x > 240)
        print(LWARN,
              "resolution.x (%d) is larger than 240, this may cause unexpected behavior.\n",
              init->resolution.x);
    if (init->resolution.y > 320)
        print(LWARN,
              "resolution.y (%d) is larger than 320, this may cause unexpected behavior.\n",
              init->resolution.y);

    memset(device, 0, sizeof(st7789_device_t));

    // copy all the init data
    device->device_op = init->device_op;
    device->resolution.x = init->resolution.x;
    device->resolution.y = init->resolution.y;

    // init gpio_state (only for spi mode)
    if (init->device_op.bus_mode == ST7789_BUS_MODE_SPI)
    {
        CALL_NULLABLE_WITH_ERROR(init->device_op.spi.gpio_cs_set, 1);
        CALL_NULLABLE_WITH_ERROR(init->device_op.spi.gpio_rst_set, 1);
        CALL_WITH_ERROR_RETURN(init->device_op.spi.gpio_dc_set, 0);
    }

    // init backlight
    CALL_NULLABLE_WITH_ERROR(init->device_op.pwm_change_duty, 0);

    // soft reset
    // Frame memory contents are unaffected by this command.
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, SWRESET, NULL, 0);

    // wait 5ms
    sys_delay_ms(120);

    // sleep out
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, SLEEP_OUT, NULL, 0);

    // wait 5ms
    sys_delay_ms(5);

    // setting gram mapping
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, MADCTL, "\x00", 1);

    // setting pixel format: 16bit RGB 565
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, COLMOD, "\x05", 1);

    // porch control:
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, PORCTRL,
                    "\x03" // BPA[6:0] Back porch: 3 pclk
                    "\x03" // FPA[6:0] Front porch: 3 pclk
                    "\x00" // PSEN: separate porch control: disabled
                    "\x33" // BPB[3:0] FPB[3:0] Porch Setting in idle mode
                    "\x33" // BPC[3:0] FPC[3:0] Porch Setting in partial mode
                    ,
                    5);

    // frame rate control: partial and idle mode use normal mode settings
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, FRCTRL1, "\x00\x0F\x0F", 3);

    // frame rate control: fps = 10M / (250 + BPA + FPA) * (250 + RTNA[4:0])
    // in this config fps = 29.9706
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, FRCTRL2,
                    "\x0F" // RTNA[4:0] : 15 pclk
                    ,
                    1);

    // gate voltage control: set to VGH = 13.26V VGL = -10.43V
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, GCTRL, "\x35", 1);

    // VCOM Setting control: set to VCOM=1.35V
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, VCOMS, "\x19", 1);

    // LCM Control
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, LCMCTRL, "\x2C", 1);

    // VDV and VRH Command Enable
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, VDVVRHEN, "\x01\xFF", 2);

    // VRH = 4.6 + (vcom + vcom offset + vdv)
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, VRHS, "\x12", 1);

    // VDV = 0V
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, VDVSET, "\x20", 1);

    // Power Control 1 : AVDD = 6.8V, AVDD = -4.8V, VDS = 2.3V
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, PWCTRL1, "\xA4\xA1", 2);

    // Positive Voltage Gamma Control
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, PVGAMCTRL,
                    "\xD0\x04\x0D\x11\x13\x2B\x3F\x54\x4C\x18\x0D\x0B\x1F\x23",
                    14);

    // Negative Voltage Gamma Control
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, NVGAMCTRL,
                    "\xD0\x04\x0C\x11\x13\x2C\x3F\x44\x51\x2F\x1F\x1F\x20\x23",
                    14);

    // display inversion on
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, INVON, NULL, 0);

    // TE signal output on
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, TEON, "\x00", 1);

    // sleep out
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, SLEEP_OUT, NULL, 0);

    // delay 120ms
    sys_delay_ms(120);

    return ALL_OK;
}

error_t st7789_display_on(st7789_device_t *device)
{
    PARAM_NOT_NULL(device);
    st7789_device_op_t *device_op = &device->device_op;

    CALL_WITH_ERROR_RETURN(st7789_write_command, device, DISPON, NULL, 0);
    CALL_NULLABLE_WITH_ERROR(device_op->pwm_change_duty, 10000);

    return ALL_OK;
}

error_t st7789_display_off(st7789_device_t *device)
{
    PARAM_NOT_NULL(device);
    st7789_device_op_t *device_op = &device->device_op;

    CALL_WITH_ERROR_RETURN(st7789_write_command, device, DISPOFF, NULL, 0);
    CALL_NULLABLE_WITH_ERROR(device_op->pwm_change_duty, 0);

    return ALL_OK;
}

error_t st7789_display_set_window(st7789_device_t *device, rect_t rect)
{
    PARAM_NOT_NULL(device);

    PARAM_CHECK(rect.top, >= 0);
    PARAM_CHECK(rect.bottom, >= 0);
    PARAM_CHECK(rect.left, >= 0);
    PARAM_CHECK(rect.right, >= 0);
    PARAM_CHECK(rect.top, <= 320);
    PARAM_CHECK(rect.bottom, <= 320);
    PARAM_CHECK(rect.left, <= 240);
    PARAM_CHECK(rect.right, <= 240);

    rect.bottom -= 1;
    rect.right -= 1;

    struct
    {
        uint16_t begin;
        uint16_t end;
    } args;

    args.begin = U16ECV(rect.left);
    args.end = U16ECV(rect.right);

    CALL_WITH_ERROR_RETURN(st7789_write_command, device, CASET, &args, 4);

    args.begin = U16ECV(rect.top);
    args.end = U16ECV(rect.bottom);
    CALL_WITH_ERROR_RETURN(st7789_write_command, device, RASET, &args, 4);

    return ALL_OK;
}

error_t st7789_append_gram(st7789_device_t *device, const rgb565_t *w_data, uint32_t npixel)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(w_data);

    CALL_WITH_ERROR_RETURN(st7789_write_command, device, RAMWR, NULL, 0);

    CALL_WITH_ERROR_RETURN(st7789_write_pixel_data, device, w_data, npixel);

    return ALL_OK;
}

error_t st7789_display_clear_gram(st7789_device_t *device, rgb565_t color)
{
    PARAM_NOT_NULL(device);
    st7789_device_op_t *device_op = &device->device_op;

    rect_t rect = {
        .top = 0,
        .left = 0,
        .bottom = device->resolution.y,
        .right = device->resolution.x,
    };

    CALL_WITH_ERROR_RETURN(st7789_display_set_window, device, rect);

    CALL_WITH_ERROR_RETURN(st7789_write_command, device, RAMWR, NULL, 0);

    error_t err = ALL_OK;

    uint32_t npixels = device->resolution.x * device->resolution.y;

    if (device_op->host_is_big_endian != true)
    {
        color = U16ECV(color);
    }

    if (device->device_op.bus_mode == ST7789_BUS_MODE_SPI)
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
    else if (device->device_op.bus_mode == ST7789_BUS_MODE_8080)
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

static struct
{
    rgb565_t clear_color;
    uint16_t lines_left;
} st7789_gram_clear_config;

typedef struct
{
    rgb565_t *buf;
    uint16_t buf_size;
} st7789_gram_clear_args_t;

error_t st7789_clear_gram_set_buf(st7789_device_t *device, st7789_gram_clear_args_t *args)
{

    uint32_t lines_can_write = args->buf_size / device->resolution.x;
    uint32_t lines_to_write = st7789_gram_clear_config.lines_left;

    if (lines_to_write > lines_can_write)
        lines_to_write = lines_can_write;

    uint32_t npixels = device->resolution.x * lines_to_write;

    CALL_WITH_ERROR_RETURN(st7789_update_gram_set_buff, device,
                    npixels, args->buf);

    st7789_gram_clear_config.lines_left -= lines_to_write;

    return ALL_OK;
}

/**
 * @brief set the buffer and start the transfer
 *
 * @param pargs
 * @return error_t
 */
error_t st7789_clear_gram_cplt_handler(st7789_device_t *device, void *pargs)
{
    st7789_gram_clear_args_t *args = pargs;

    PARAM_NOT_NULL(device);

    if (st7789_gram_clear_config.lines_left > 0)
    {
        CALL_WITH_ERROR_RETURN(st7789_clear_gram_set_buf, device, args);

        CALL_WITH_ERROR_RETURN(st7789_update_gram_stream_start, device,
                        st7789_clear_gram_cplt_handler,
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

error_t st7789_display_clear_gram_async(st7789_device_t *device, rgb565_t color)
{
    PARAM_NOT_NULL(device);

    rect_t rect = {
        .top = 0,
        .left = 0,
        .bottom = device->resolution.y,
        .right = device->resolution.x,
    };
    // alloc buffer
    const int cache_line_count = 5;
    const int cache_buffer_size = cache_line_count * device->resolution.x;
    rgb565_t *pbuf = NULL;
    pbuf = malloc(cache_buffer_size * sizeof(rgb565_t));

    st7789_gram_clear_args_t *args = malloc(sizeof(st7789_gram_clear_args_t));
    args->buf_size = cache_buffer_size;
    args->buf = pbuf;

    if (pbuf == NULL || args == NULL)
    {
        print(LERROR, "malloc failed, can not allocate buffer for gram clear.\n");
        return E_MEMORY_ALLOC_FAILED;
    }

    // prepare the buffer data
    for (int i = 0; i < cache_buffer_size; i++)
        pbuf[i] = U16ECV(color);

    // prepare update window
    CALL_WITH_ERROR_RETURN(st7789_display_set_window, device, rect);

    // update flags
    st7789_gram_clear_config.clear_color = color;
    st7789_gram_clear_config.lines_left = device->resolution.y;

    /**
     * This function serves as a example of how to use async gram update
     * in the function, we will clear the gram with a specific color
     *
     * the `st7789_clear_gram_cplt_handler` will set the buffer and call
     * st7789_update_gram_stream_start to start the transfer, we call it
     * to bootstrap the first transfer.
     */
    st7789_clear_gram_cplt_handler(device, args);

    return ALL_OK;
}

error_t st7789_update_gram_set_buff(
    st7789_device_t *device,
    uint32_t buffer_size,
    rgb565_t *pbuf)
{
    PARAM_NOT_NULL(device);
    PARAM_NOT_NULL(pbuf);

    switch (device->async_state)
    {
    case ST7789_ASYNC_STATE_IDLE:
        device->async_state = ST7789_ASYNC_STATE_BUFFER_LOADED;
        break;
    case ST7789_ASYNC_STATE_TRANSFERING:
        device->async_state = ST7789_ASYNC_STATE_BUFFER_RELOADED;
        break;
    default:
        print(LERROR, "The buffer can only be accessed when idle or transfering.\n");
        return E_INVALID_OPERATION;
    }

    device->gram_tx_buf_size = buffer_size;
    device->gram_tx_buf = pbuf;
    return ALL_OK;
}

error_t st7789_update_gram_stream_start(
    st7789_device_t *device,
    st7789_transfer_cplt_handler_t handler,
    void *params)
{
    PARAM_NOT_NULL(device);

    if (device->async_state == ST7789_ASYNC_STATE_TRANSFERING)
    {
        print(LERROR, "There is a transfering operation ongoing.\n");
        return E_INVALID_OPERATION;
    }

    if (device->async_state != ST7789_ASYNC_STATE_BUFFER_LOADED && device->async_state != ST7789_ASYNC_STATE_BUFFER_RELOADED)
    {
        print(LERROR, "There is no data in buffer, please call lock() and unlock() first.\n");
        return E_INVALID_OPERATION;
    }

    device->handler = handler;
    device->handler_params = params;

    error_t err = ALL_OK;

    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_aquire);

    // check if is the first transfer
    // in this case, we need to send mem write command
    // other wise, we just need to send data
    if (device->async_state != ST7789_ASYNC_STATE_BUFFER_RELOADED)
    {
        CALL_WITH_CODE_GOTO(err, error_exit, st7789_write_command, device, RAMWR, NULL, 0);

        st7789_device_op_t *device_op = &device->device_op;
        if (device->device_op.bus_mode == ST7789_BUS_MODE_SPI)
        {
            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_dc_set, 1);

            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_cs_set, 0);
        }
        else if (device->device_op.bus_mode == ST7789_BUS_MODE_8080)
        {
            // nothing need to be done here.
        }
        else
        {
            err = E_INVALID_ARGUMENT;
            goto error_exit;
        }
    }

    device->async_state = ST7789_ASYNC_STATE_TRANSFERING;

    // send the data
    if (device->device_op.bus_mode == ST7789_BUS_MODE_SPI)
    {
        CALL_WITH_CODE_GOTO(err, error_exit,
                             device->device_op.spi.write_async_start,
                             device->gram_tx_buf_size,
                             device->gram_tx_buf);
    }
    else if (device->device_op.bus_mode == ST7789_BUS_MODE_8080)
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
    device->async_state = ST7789_ASYNC_STATE_IDLE;
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

error_t st7789_async_completed_notify(st7789_device_t *device)
{
    PARAM_NOT_NULL(device);

    if (device->async_state != ST7789_ASYNC_STATE_TRANSFERING)
    {
        print(LERROR, "There is no transfering operation\n");
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
        st7789_device_op_t *device_op = &device->device_op;
        if (device->device_op.bus_mode == ST7789_BUS_MODE_SPI)
        {
            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_dc_set, 1);

            CALL_WITH_CODE_GOTO(err, error_exit, device_op->spi.gpio_cs_set, 1);
        }
        else if (device->device_op.bus_mode == ST7789_BUS_MODE_8080)
        {
            // nothing need to be done here.
            // 80 bus does not need a ending sequence
        }
        else
        {
            err = E_INVALID_ARGUMENT;
            goto error_exit;
        }

        device->async_state = ST7789_ASYNC_STATE_IDLE;
    }

    return ALL_OK;

error_exit:
    device->async_state = ST7789_ASYNC_STATE_IDLE;
    CALL_NULLABLE_WITH_ERROR(device->device_op.bus_release);
    return err;
}

error_t st7789_wait_async_complete(st7789_device_t *device, uint32_t timeout)
{
    PARAM_NOT_NULL(device);

    uint32_t ms = sys_get_tick();
    while (device->async_state != ST7789_ASYNC_STATE_IDLE)
    {
        uint32_t delta = sys_get_tick() - ms;
        if (delta > timeout)
            return E_HARDWARE_TIMEOUT;
    }

    return ALL_OK;
}