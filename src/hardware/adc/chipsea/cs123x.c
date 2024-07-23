/**
 * @file cs123x.c
 * @author simakeng (simakeng@outlook.com)
 * @brief Driver for the Chipsea CS1237/CS1238 ADC
 * @version 0.1
 * @date 2024-07-17
 *
 * @see
 * CS1238: https://www.chipsea.com/product/details/?id=1156
 * CS1237: https://www.chipsea.com/product/details/?id=1155
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
 */

/******************************************************************************/
/*                               INCLUDE FILES                                */
/******************************************************************************/

#include "cs123x.h"
#include <hardware/devop.h>

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/
#define CS123X_READ 0x56
#define CS123X_WRITE 0x65

#define CS123X_REF_OUT_EN_BIT (1 << 6)
#define CS123X_ADC_SPEED_OFFSET 4
#define CS123X_ADC_SPEED_MASK (0x3 << CS123X_ADC_SPEED_OFFSET)
#define CS123X_PGA_GAIN_OFFSET 2
#define CS123X_PGA_GAIN_MASK (0x3 << CS123X_PGA_GAIN_OFFSET)
#define CS123X_CHANNEL_SELECT_OFFSET 0
#define CS123X_CHANNEL_SELECT_MASK (0x3 << CS123X_CHANNEL_SELECT_OFFSET)

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
static error_t cs123x_set_sdio_mode(cs123x_device_t *dev,
                                    cs123x_sdio_pin_mode_e mode);

static error_t cs123x_reset_bus(cs123x_device_t *dev);

static error_t cs123x_bitbang_transfer(cs123x_device_t *dev);

static error_t cs123x_bitbang_read_data(cs123x_device_t *dev);

static bool is_timeout(uint32_t start, uint32_t now, uint32_t timeout_ms);

static error_t cs123x_bitbang_bit_read(cs123x_device_t *dev, int32_t *bit_state);

static error_t cs123x_bitbang_bit_write(cs123x_device_t *dev, int32_t bit_state);

static error_t cs123x_bitbang_skip_bits(cs123x_device_t *dev, int32_t bits);

static error_t cs123x_bitbang_write_command(cs123x_device_t *dev, uint8_t cmd);

static inline uint8_t cs123x_generate_cfg_byte(cs123x_device_t *dev);

static error_t cs123x_bigbang_write_byte(cs123x_device_t *dev, uint8_t byte);

/******************************************************************************/
/*                      PUBLIC FUNCTION IMPLEMENTATIONS                       */
/******************************************************************************/
error_t cs123x_init(cs123x_device_t *dev,
                    cs123x_hal_t *hal,
                    cs123x_config_t *cfg)
{
    // sanity check
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(hal);
    PARAM_NOT_NULL(cfg);

    PARAM_NOT_NULL(hal->gpio_sclk_set);
    PARAM_NOT_NULL(hal->gpio_sdio_set);
    PARAM_NOT_NULL(hal->gpio_sdio_get);
    PARAM_NOT_NULL(hal->gpio_sdio_reconfig);

    // copy the HAL and config
    dev->hal = *hal;

    // set initial flags.
    dev->cfg = *cfg;
    dev->config_changed = true;

    // reset the bus
    CALL_WITH_ERROR_RETURN(cs123x_reset_bus, dev);

    // exit power down mode
    CALL_WITH_ERROR_RETURN(cs123x_exit_power_down, dev);

    return ALL_OK;
}

error_t cs123x_exit_power_down(cs123x_device_t *dev)
{
    PARAM_NOT_NULL(dev);

    // reset the bus
    CALL_WITH_ERROR_RETURN(cs123x_reset_bus, dev);

    // set the SDIO pin to input mode
    CALL_WITH_ERROR_RETURN(cs123x_set_sdio_mode, dev,
                           CS123X_SDIO_PIN_MODE_INPUT_HZ);

    // set the SCLK pin to low
    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 0);

    return ALL_OK;
}

error_t cs123x_set_config(cs123x_device_t *dev, cs123x_config_t *cfg)
{
    // sanity check
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(cfg);

    dev->cfg = *cfg;
    dev->config_changed = true;
    dev->config_pushed = false;
}

error_t cs123x_enter_power_down(cs123x_device_t *dev)
{
    PARAM_NOT_NULL(dev);

    // reset the bus
    CALL_WITH_ERROR_RETURN(cs123x_reset_bus, dev);

    // set the SDIO pin to input mode
    CALL_WITH_ERROR_RETURN(cs123x_set_sdio_mode, dev,
                           CS123X_SDIO_PIN_MODE_INPUT_HZ);

    // set the SCLK pin to high
    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 1);

    return ALL_OK;
}

error_t cs123x_is_data_ready(cs123x_device_t *dev, bool *ready)
{
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(ready);

    // set the SDIO pin to input mode
    CALL_WITH_ERROR_RETURN(cs123x_set_sdio_mode, dev,
                           CS123X_SDIO_PIN_MODE_INPUT_HZ);

    int32_t state = 0;

    // get the state of the SDIO pin
    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sdio_get, &state);

    *ready = (state == 0);

    return ALL_OK;
}

error_t cs123x_wait_data_ready(cs123x_device_t *dev, uint32_t timeout_ms)
{
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(dev->hal.get_time_ms);

    bool ready = false;
    uint32_t now, start = dev->hal.get_time_ms();
    do
    {
        CALL_WITH_ERROR_RETURN(cs123x_is_data_ready, dev, &ready);

        if (ready)
            return ALL_OK;

        CALL_NULLABLE(dev->hal.delay, 10);
        now = dev->hal.get_time_ms();

    } while (!is_timeout(start, now, timeout_ms));
    return E_HARDWARE_TIMEOUT;
}

error_t cs123x_wait_and_read_data(cs123x_device_t *dev,
                                  int32_t *data,
                                  uint32_t timeout_ms)
{
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(data);

    // wait for the data to be ready
    CALL_WITH_ERROR_RETURN(cs123x_wait_data_ready, dev, timeout_ms);

    // read the data
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_transfer, dev);

    // return the data
    CALL_WITH_ERROR_RETURN(cs123x_get_last_data, dev, data);

    return ALL_OK;
}

error_t cs123x_get_last_data(cs123x_device_t *dev, int32_t *data)
{
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(data);

    *data = dev->last_data;

    return ALL_OK;
}

error_t cs123x_register_data_ready_callback(cs123x_device_t *dev,
                                            data_ready_callback_t callback)
{
    PARAM_NOT_NULL(dev);
    PARAM_NOT_NULL(callback);

    dev->callback = callback;

    return ALL_OK;
}

error_t cs123x_exti_triggered(cs123x_device_t *dev)
{
    PARAM_NOT_NULL(dev);
    CALL_NULLABLE_WITH_ERROR(dev->callback, dev);
    return ALL_OK;
}

/******************************************************************************/
/*                     PRIVATE FUNCTION IMPLEMENTATIONS                       */
/******************************************************************************/

static error_t cs123x_set_sdio_mode(cs123x_device_t *dev,
                                    cs123x_sdio_pin_mode_e mode)
{
    if (dev->current_mode == mode)
        return ALL_OK;

    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sdio_reconfig, mode);
    dev->current_mode = mode;
    return ALL_OK;
}

static error_t cs123x_reset_bus(cs123x_device_t *dev)
{
    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 0);
    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sdio_set, 0);
    return ALL_OK;
}

static error_t cs123x_bitbang_transfer(cs123x_device_t *dev)
{
    // check if device is in the correct mode
    if (dev->current_mode != CS123X_SDIO_PIN_MODE_INPUT_HZ)
    {
        dev_err("Driver is not in the correct mode!\n");
        return E_INVALID_OPERATION;
    }

    // check if chip is ready
    bool ready = false;
    CALL_WITH_ERROR_RETURN(cs123x_is_data_ready, dev, &ready);

    if (!ready)
    {
        dev_err("Should not start a transfer when device is not ready!\n");
        return E_INVALID_OPERATION;
    }

    // read ADC data
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_read_data, dev);

    // read update bit
    int32_t update_bit = 0;
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_bit_read, dev, &update_bit);

    // check if last config is pushed to the device
    if (update_bit == 1 && dev->config_pushed == true)
    {
        dev->config_pushed = false;
        dev->config_changed = false;
    }

    // skip 2 bits
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_skip_bits, dev, 2);

    // no need for update the device cfg
    if (!dev->config_changed)
        return ALL_OK;

    // assemble the configuration byte
    uint8_t cfg_byte = cs123x_generate_cfg_byte(dev);

    // skip 2 bits
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_skip_bits, dev, 2);

    // write the configuration command
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_write_command, dev, CS123X_WRITE);

    // skip 1 bit
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_skip_bits, dev, 1);

    // write the configuration byte
    CALL_WITH_ERROR_RETURN(cs123x_bigbang_write_byte, dev, cfg_byte);

    // skip 1 bit
    CALL_WITH_ERROR_RETURN(cs123x_bitbang_skip_bits, dev, 1);

    dev->config_pushed = true;

    return ALL_OK;
}

static inline uint8_t cs123x_generate_cfg_byte(cs123x_device_t *dev)
{
    uint8_t cfg_byte = 0x00;

    if (dev->cfg.ref_out_enable)
        cfg_byte |= 0x40;

    cfg_byte |= (dev->cfg.speed << CS123X_ADC_SPEED_OFFSET);

    cfg_byte |= (dev->cfg.pga_gain << CS123X_PGA_GAIN_OFFSET);

    cfg_byte |= (dev->cfg.channel << CS123X_CHANNEL_SELECT_OFFSET);

    return cfg_byte;
}

static error_t cs123x_bigbang_write_byte(cs123x_device_t *dev, uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        int32_t bit = (byte >> (7 - i)) & 0x01;
        CALL_WITH_ERROR_RETURN(cs123x_bitbang_bit_write, dev, bit);
    }

    return ALL_OK;
}

static error_t cs123x_bitbang_write_command(cs123x_device_t *dev, uint8_t cmd)
{
    for (int i = 0; i < 7; i++)
    {
        int32_t bit = (cmd >> (6 - i)) & 0x01;
        CALL_WITH_ERROR_RETURN(cs123x_bitbang_bit_write, dev, bit);
    }

    return ALL_OK;
}

static error_t cs123x_bitbang_skip_bits(cs123x_device_t *dev, int32_t bits)
{
    for (int i = 0; i < bits; i++)
    { /*                                                   (ignored) */
        CALL_WITH_ERROR_RETURN(cs123x_bitbang_bit_read, dev, NULL);
    }

    return ALL_OK;
}

static error_t cs123x_bitbang_bit_read(cs123x_device_t *dev, int32_t *bit_state)
{
    CALL_WITH_ERROR_RETURN(cs123x_set_sdio_mode, dev,
                           CS123X_SDIO_PIN_MODE_INPUT_HZ);

    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 1);
    CALL_NULLABLE(dev->hal.delay, 1);

    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 0);

    if (bit_state != NULL)
        CALL_WITH_ERROR_RETURN(dev->hal.gpio_sdio_get, bit_state);

    CALL_NULLABLE(dev->hal.delay, 1);

    return ALL_OK;
}

static error_t cs123x_bitbang_bit_write(cs123x_device_t *dev, int32_t bit_state)
{
    CALL_WITH_ERROR_RETURN(cs123x_set_sdio_mode, dev,
                           CS123X_SDIO_PIN_MODE_OUTPUT_PP);

    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 1);
    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sdio_set, bit_state);
    CALL_NULLABLE(dev->hal.delay, 1);

    CALL_WITH_ERROR_RETURN(dev->hal.gpio_sclk_set, 0);
    CALL_NULLABLE(dev->hal.delay, 1);

    return ALL_OK;
}

static error_t cs123x_bitbang_read_data(cs123x_device_t *dev)
{
    // read first 24-bit ADC data
    uint32_t raw_data = 0;
    for (int i = 0; i < 24; i++)
    {
        int32_t state = 0;
        CALL_WITH_ERROR_RETURN(cs123x_bitbang_bit_read, dev, &state);
        raw_data |= (state << (23 - i));
    }

    // convert to signed integer
    int32_t data = raw_data << 8;
    data >>= 8;
    dev->last_data = data;

    return ALL_OK;
}

static bool is_timeout(uint32_t start, uint32_t now, uint32_t timeout_ms)

{
    if (start <= now)
    {
        return ((now - start) >= timeout_ms);
    }
    else // overflow happen
    {
        return (((0xFFFFFFFF - start) + now) >= timeout_ms);
    }
}

/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/