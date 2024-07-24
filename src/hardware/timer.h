/**
 * @file libe15-timer.h
 * @author Simakeng (simakeng@outlook.com)
 * @brief Timer Utilities for CMSIS SysTick Register
 * @version 0.1
 * @date 2022-04-26
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdint.h>

/**
 * @brief Initialize SysTick Timer acording to current SysTick settings.
 * @brief this can be used with other libraries that use SysTick, like 
 * STM32Cube HAL.
 */
void systick_init_as_is(uint32_t sys_clk);

typedef struct {
    uint32_t sys_clk;
    uint32_t irq_freq;
} systick_config_t;

/**
 * @brief Initialize SysTick Timer with custom settings.
 * 
 * @param config  the custom settings
 */
 void systick_init(systick_config_t *config);

/**
 * @brief Delay for a number of microseconds
 * 
 * @param us 
 */
void sys_delay_us(uint32_t us);

/**
 * @brief Delay for a number of milliseconds
 * 
 * @param ms 
 */
void sys_delay_ms(uint32_t ms);

/**
 * @brief Get the time from this module inited in ms
 * 
 * @return uint32_t 
 */
uint32_t sys_get_tick();

/**
 * @brief Get the time from this module inited in ms
 * 
 * @return uint64_t 
 */
uint64_t sys_get_ticku64();

void systick_timer_isr(void);
