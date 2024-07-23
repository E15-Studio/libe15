/**
 * @file cs123x.h
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

#include <stdint.h>
#include <stdbool.h>

#include <hardware/devop.h>

#include <generated-conf.h>

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/

#ifndef __CS123X_H__
#define __CS123X_H__

#ifdef CONFIG_ADC_CS123X

/*******************************************************************************
 *! Driver Notice:
 **  1. This chip is a 24bit sigma-delta ADC, but it uses a weird SCLK/SDIO way
 **     to communicate with the host. and the "data ready" is also reused with
 **     the "SDIO" signal. And all data is NOT aligned with the byte boundary
 **     except the 24bit data, so no SPI bus support, only GPIO bit-banging.
 **
 **  2. Due to the above reason, user should provide a function that can provide
 **     at least 0.5us of delay. In some MCU maybe the function call overhead is
 **     enough for this, so you can just fill that with NULL or generate some
 **     small delay use NOP instruction.
 ******************************************************************************/

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/

typedef enum
{
    CS123X_ADC_SPEED_10Hz = 0,
    CS123X_ADC_SPEED_40Hz = 1,
    CS123X_ADC_SPEED_640Hz = 2,
    CS123X_ADC_SPEED_1280Hz = 3,
} cs123x_adc_speed_e;

typedef enum
{
    CS123X_PGA_GAIN_1 = 0,
    CS123X_PGA_GAIN_2 = 1,
    CS123X_PGA_GAIN_64 = 2,
    CS123X_PGA_GAIN_128 = 3,
} cs123x_pga_gain_e;

typedef enum
{
    CS123X_CH_A = 0,
    CS123X_CH_B = 1,
    CS123X_CH_TEMP = 2,
    CS123X_CH_SHORT = 3,
} cs123x_ch_sel_e;

typedef enum
{
    /// @brief The pin mode is unknown.
    CS123X_SDIO_PIN_MODE_UNKONW,

    /// @brief The pin mode is set to output push-pull
    CS123X_SDIO_PIN_MODE_OUTPUT_PP,

    // @brief The pin mode is set to input high impedance
    CS123X_SDIO_PIN_MODE_INPUT_HZ,

    /// @brief The pin mode is set to externel interrupt with negative edge
    /// trigger.
    CS123X_SDIO_PIN_MODE_EXTI_NEGEDGE,
} cs123x_sdio_pin_mode_e;

/******************************************************************************/
/*                              TYPE DEFINITIONS                              */
/******************************************************************************/

/**
 * @brief User provided HAL functions for the CS123x driver.
 */
typedef struct
{
    /**
     * @brief Set the SCLK pin state.
     * This pin is uesd to generate the clock signal.
     *
     * @param state The state of the pin, 0 for low, 1 for high.
     *
     * @return error_t < 0 for error, 0 for success.
     */
    error_t (*gpio_sclk_set)(int state);

    /**
     * @brief Set the SDIO pin state.
     * This pin is also used as nDRDY pin.
     *
     * @param state The state of the pin, 0 for low, 1 for high.
     *
     * @return error_t < 0 for error, 0 for success.
     */
    error_t (*gpio_sdio_set)(int state);

    /**
     * @brief Read the SDIO pin state.
     *
     * @param state The state of the pin, 0 for low, 1 for high.
     *
     * @return error_t < 0 for error, 0 for success.
     */
    error_t (*gpio_sdio_get)(int *state);

    /**
     * @brief Reconfigure the SDIO pin mode.
     * The chip uses sigle pin for both read and write.
     * @see document in this file for more information.
     *
     * @param mode The mode to set.
     *
     * @return error_t < 0 for error, 0 for success.
     */
    error_t (*gpio_sdio_reconfig)(cs123x_sdio_pin_mode_e mode);

    /**
     * @brief user provided delay function,
     *
     * @param us The delay time in us.
     *
     * @note can be NULL if the MCU is slow enough.
     * @see document in this file for more information.
     */
    void (*delay)(uint32_t us);

    /**
     * @brief user provided time base function.
     *
     * @note can be NULL if the USER dont call cs123x_wait_data_ready.
     */
    uint32_t (*get_time_ms)(void);

} cs123x_hal_t;

typedef struct
{
    /// @brief The reference voltage enable.
    bool ref_out_enable;

    /// @brief The ADC speed to set.
    cs123x_adc_speed_e speed;

    /// @brief The PGA gain to set..
    cs123x_pga_gain_e pga_gain;

    /// @brief The channel to select.
    cs123x_ch_sel_e channel;

} cs123x_config_t;

/**
 * @brief The callback for the chip's data ready event.
 * @note user should set flags to let thread code do the data reading.
 * It is not recommended to read data in the callback.
 */
typedef error_t (*data_ready_callback_t)(void *dev);

/**
 * @brief The CS123x device structure.
 */
typedef struct
{
    bool initialized;
    cs123x_hal_t hal;
    cs123x_config_t cfg;
    bool power_down;

    int32_t last_data;
    bool config_changed;
    bool config_pushed;
    data_ready_callback_t callback;

    cs123x_sdio_pin_mode_e current_mode;
} cs123x_device_t;

/******************************************************************************/
/*                         PUBLIC FUNCTION DEFINITIONS                        */
/******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif //! #ifdef __cplusplus

    /**
     * @brief Initialize the CS123x device.
     *
     * @param dev The device to initialize.
     * @param hal The HAL functions for the device.
     * @param cfg The configuration for the device.
     *
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_init(cs123x_device_t *dev,
                        cs123x_hal_t *hal,
                        cs123x_config_t *cfg);

    /**
     * @brief Set a new config to CS123X chip.
     * 
     * @param dev 
     * @param cfg  The configuration for the device.
     * @return error_t 
     */
    error_t cs123x_set_config(cs123x_device_t *dev, cs123x_config_t *cfg);

    /**
     * @brief Enter the power down mode.
     *
     * @param dev
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_enter_power_down(cs123x_device_t *dev);

    /**
     * @brief Exit the power down mode.
     *
     * @param dev
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_exit_power_down(cs123x_device_t *dev);

    /**
     * @brief Check if the data is ready.
     *
     * @param dev
     * @param ready out
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_is_data_ready(cs123x_device_t *dev, bool *ready);

    /**
     * @brief Wait for the data ready.
     *
     * @param dev
     * @param timeout_ms
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_wait_data_ready(cs123x_device_t *dev, uint32_t timeout_ms);

    /**
     * @brief Read the last data from the device.
     *
     * @param dev
     * @param data out
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_get_last_data(cs123x_device_t *dev, int32_t *data);

    /**
     * @brief Read the data from device.
     * @note If the application is sensitive to any delay, use callback model.
     * @param dev The device to read data.
     * @param data The data read from the device.
     * @param timeout_ms The timeout in ms.
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_wait_and_read_data(cs123x_device_t *dev,
                                      int32_t *data,
                                      uint32_t timeout_ms);

    /**
     * @brief Register the data ready callback.
     *
     * @param dev The device to register the callback.
     * @param callback The callback function.
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_register_data_ready_callback(cs123x_device_t *dev,
                                                data_ready_callback_t callback);

    /**
     * @brief Inform the driver that the EXTI is triggered.
     * @warning This function should only be called in the EXTI IRQ handler.
     * @param dev
     * @return error_t < 0 for error, 0 for success.
     */
    error_t cs123x_exti_triggered(cs123x_device_t *dev);

#ifdef __cplusplus
}
#endif //! #ifdef __cplusplus
/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/

#endif // CONFIG_ADC_CS123X

#endif // __CS123X_H__
