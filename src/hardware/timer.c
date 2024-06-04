/**
 * @file libe15-timer.c
 * @author Simakeng (simakeng@outlook.com)
 * @brief Timer Utilities for CMSIS SysTick Register
 * @version 0.1
 * @date 2022-04-26
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "timer.h"
#include <string.h>

// big ending:      01 23 45 67 | 89 ab cd ef
// little ending:   ef cd ab 89 | 67 45 23 01

// big ending:      89 ab cd ef
// little ending:   ef cd ab 89

static volatile struct
{
    union
    {
        uint64_t u64cnt;
        struct
        {
            uint32_t cnt;
            uint32_t cnt_hi;
        };
    };

} system_clk;

static struct
{
    uint32_t s;
    uint32_t ms;
    uint32_t us;
} systick_duration;

// void systick_init()
// {
//     // disable systick
//     SysTick->CTRL &= ~SysTick_CTRL_ENABLE;

//     // clear perivous settings
//     SysTick->CTRL &= ~(SysTick_CTRL_CLKSOURCE_MASK);

//     // reset all status
//     systick_status.tick_cnt = 0;
//     systick_status.tick_cnt_hi = 0;

//     // set control register
//     SysTick->CTRL |= SysTick_CTRL_CLKSOURCE(1);

//     // calculate reload value

//     // set reload value
//     SysTick->LOAD = SYSTICK_RELOAD_VALUE;

//     // clear current value
//     SysTick->VAL = 0UL;

//     // re-enable systick
//     SysTick->CTRL |= SysTick_CTRL_ENABLE;

//     return;
// }

void systick_init_as_is(uint32_t sys_clk)
{
    uint32_t cfg = SysTick->CTRL;

    // if not using hclk clock source
    if (!(cfg & SysTick_CTRL_CLKSOURCE_Msk))
    {
        sys_clk /= 8;
    }

    // update flags
    systick_duration.us = sys_clk / 1000000UL;
    systick_duration.ms = sys_clk / 1000UL;
    systick_duration.s = sys_clk;

    // force data sync
    __DSB();
}

/**
 * @brief Systick IRQ handler
 */
inline void systick_timer_isr()
{
    if (++system_clk.cnt == 0)
        system_clk.cnt_hi++;
}

void sys_delay_us(uint32_t us)
{
    // too short, no need for calculate
    if (us < 15)
    {
        while (us--)
        {
            uint32_t t = systick_duration.us >> 3;
            __NOP();
            __NOP();
            while (t--)
            {
                __NOP();
                __NOP();
            }
        }
        return;
    }

    // too long, use ms delay
    if(us > 100000)
    {
        sys_delay_ms(us / 1000);
        return;
    }

    // retrive current tick count
    volatile uint32_t timer_cnt_old = SysTick->VAL;
    volatile uint32_t tick_cnt_old = system_clk.cnt;
    volatile uint32_t tick_cnt_hi_old = system_clk.cnt_hi;

    uint32_t reload_cnt = SysTick->LOAD + 1;

    // calculate target tick count
    uint32_t timer_cnt = systick_duration.us * us;
    uint32_t tick_cnt_us = timer_cnt / reload_cnt;
    uint32_t timer_cnt_us = timer_cnt % reload_cnt;

    uint32_t target_timer_cnt = timer_cnt_old - timer_cnt_us;
    uint32_t target_tick_cnt_hi = tick_cnt_hi_old;
    uint32_t target_tick_cnt = tick_cnt_old + tick_cnt_us;

    // if underflow, add to high counter
    if (target_timer_cnt > reload_cnt)
    {
        target_timer_cnt += reload_cnt;
        target_tick_cnt++;
    }

    // if overflow, add to high counter
    if (target_tick_cnt < tick_cnt_old)
    {
        target_tick_cnt_hi++;
    }

    while (1)
    {
        __NOP();
        __NOP();
        __NOP();

        if (system_clk.cnt_hi > target_tick_cnt_hi)
        {
            break;
        }
        else if (system_clk.cnt_hi == target_tick_cnt_hi)
        {
            if (system_clk.cnt > target_tick_cnt)
            {
                break;
            }
            else if (system_clk.cnt == target_tick_cnt)
            {
                if (SysTick->VAL < target_timer_cnt)
                {
                    break;
                }
            }
        }
        __NOP();
    }
    __NOP();
}

void sys_delay_ms(uint32_t ms)
{
    // if (ms < 20)
    // {
    //     sys_delay_us(ms * 1000);
    //     return;
    // }

    volatile uint32_t tick_cnt_old = system_clk.cnt;
    volatile uint32_t tick_cnt_hi_old = system_clk.cnt_hi;

    uint32_t target_tick_cnt = tick_cnt_old + ms;
    uint32_t target_tick_cnt_hi = tick_cnt_hi_old;

    if (target_tick_cnt < tick_cnt_old)
    {
        target_tick_cnt_hi++;
    }

    while (1)
    {
        __NOP();
        if (system_clk.cnt_hi > target_tick_cnt_hi)
        {
            break;
        }
        else if (system_clk.cnt_hi == target_tick_cnt_hi)
        {
            if (system_clk.cnt > target_tick_cnt)
            {
                break;
            }
        }
        __NOP();
    }
}

uint32_t sys_get_tick()
{
    return system_clk.cnt;
}

uint64_t sys_get_ticku64()
{
    return system_clk.u64cnt;
}

#define CONFIG_OVERWRITE_CUBEMX_SYSTICK_SYSTEM

#ifdef CONFIG_OVERWRITE_CUBEMX_SYSTICK_SYSTEM
void HAL_IncTick(void)
{
    systick_timer_isr();
}

/**
  * @brief Provides a tick value in millisecond.
  * @note This function is declared as __weak to be overwritten in case of other 
  *       implementations in user file.
  * @retval tick value
  */
uint32_t HAL_GetTick(void)
{
  return system_clk.cnt;
}

#endif
