/**
 * @file libe15-dbg.h
 * @author Simakeng (simakeng@outlook.com)
 * @brief Debugging functions
 * @version 0.1
 * @date 2023-04-10
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

#include "generated-conf.h"

#include <stdio.h>
#include <stdint.h>

#ifndef __LIBE15_DBG_H__
#define __LIBE15_DBG_H__

/**************************************
 * Color defs
 **************************************/

#ifdef CONFIG_DEBUG_COLOR

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_DARKGREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_GRAY "\033[37m"
#define COLOR_PINK "\033[91m"
#define COLOR_GREEN "\033[92m"
#define COLOR_BEIGE "\033[93m"
#define COLOR_ROYALBLUE "\033[94m"
#define COLOR_PURPLE "\033[95m"
#define COLOR_TEAL "\033[96m"
#define COLOR_WHITE "\033[97m"

#endif //! #ifdef CONFIG_DEBUG_COLOR

/**************************************
 * Debug levels
 **************************************/

#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3
#define FATAL 4

#if defined(CONFIG_USE_LIBE15_LOG_PRINT)

#ifdef __cplusplus
extern "C"
{
#endif // ! #ifdef __cplusplus

    typedef struct
    {
        /**
         * @brief put a char into an output device
         * @param ch ASCII char code
         * @return int32_t ch: success, EOF: failure
         */
        int32_t (*putc)(int32_t ch);
        /**
         * @brief put a string into an output device
         * default behaviour is call dev_putc on each char
         * @param s The string to print
         * @return int32_t num of char puted, EOF: failure
         * @note The string is encoded in UTF-8
         */
        int32_t (*puts)(const char *s);
    } dbg_low_level_io_ops_t;

    /**
     * @brief Initialize the debug print function
     *
     * @param ops low level io functions
     */
    void dbg_print_init(dbg_low_level_io_ops_t *ops);

    /**
     * @brief Print an error message
     * @param level The level of the message
     * @param location The location where this function is called
     * @param function The function name of the caller function
     * @param msg The message to print
     * @param ... The arguments to the message
     */
    void dbg_print(int32_t level, const char *location, const char *function, const char *msg, ...);

#ifdef __cplusplus
}
#endif // ! #ifdef __cplusplus

#define DBG_TRANSLATE_LINE(line) #line

#define DBG_TRANSLATE_LOCATION(file, line) file ":" DBG_TRANSLATE_LINE(line)


/************************************
 * calculate levels
 ************************************/
#if !defined(CONFIG_LOG_LEVEL_INFO) && defined(CONFIG_LOG_LEVEL_DEBUG)
#define CONFIG_LOG_LEVEL_INFO
#endif

#if !defined(CONFIG_LOG_LEVEL_WARN) && defined(CONFIG_LOG_LEVEL_INFO)
#define CONFIG_LOG_LEVEL_WARN
#endif

#if !defined(CONFIG_LOG_LEVEL_ERROR) && defined(CONFIG_LOG_LEVEL_WARN)
#define CONFIG_LOG_LEVEL_ERROR
#endif

#if !defined(CONFIG_LOG_LEVEL_FATAL) && defined(CONFIG_LOG_LEVEL_ERROR)
#define CONFIG_LOG_LEVEL_FATAL
#endif

/************************************
 * @brief Debug level debug message
 ************************************/
#if defined(CONFIG_LOG_LEVEL_DEBUG)
#define dbg_print_LDEBUG(level, location, function, msg, ...) dbg_print(level, location, function, msg, ##__VA_ARGS__)
#else
#define dbg_print_LDEBUG(level, location, function, msg, ...) ((void)0)
#endif //! #if defined(CONFIG_LOG_LEVEL_DEBUG)

/************************************
 * @brief Info level debug message
 ************************************/
#if defined(CONFIG_LOG_LEVEL_INFO)
#define dbg_print_LINFO(level, location, function, msg, ...) dbg_print(level, location, function, msg, ##__VA_ARGS__)
#else
#define dbg_print_LINFO(level, location, function, msg, ...) ((void)0)
#endif //! #if defined(CONFIG_LOG_LEVEL_INFO)

/************************************
 * @brief Info level debug message
 ************************************/
#if defined(CONFIG_LOG_LEVEL_WARN)
#define dbg_print_LWARN(level, location, function, msg, ...) dbg_print(level, location, function, msg, ##__VA_ARGS__)
#else
#define dbg_print_LWARN(level, location, function, msg, ...) ((void)0)
#endif //! #if defined(CONFIG_LOG_LEVEL_WARN)

/************************************
 * @brief Info level debug message
 ************************************/
#if defined(CONFIG_LOG_LEVEL_ERROR)
#define dbg_print_LERROR(level, location, function, msg, ...) dbg_print(level, location, function, msg, ##__VA_ARGS__)
#else
#define dbg_print_LERROR(level, location, function, msg, ...) ((void)0)
#endif //! #if defined(CONFIG_LOG_LEVEL_ERROR)

/************************************
 * @brief Fatal level debug message
 ************************************/
#if defined(CONFIG_LOG_LEVEL_FATAL)
#define dbg_print_LFATAL(level, location, function, msg, ...) dbg_print(level, location, function, msg, ##__VA_ARGS__)
#else
#define dbg_print_LFATAL(level, location, function, msg, ...) ((void)0)
#endif //! #if defined(CONFIG_LOG_LEVEL_FATAL)


#define print(level, ...) dbg_print_L##level(level, DBG_TRANSLATE_LOCATION(__FILE__, __LINE__), __FUNCTION__, __VA_ARGS__)


#elif defined(CONFIG_USE_CUSTOM_LOG_PRINT)

#error Please insert your #include and func here and comment out this error.
// #define print(level, ...) your_function_to_print(level, __VA_ARGS__)

#else
#define print(level, ...) ((void)0)
#endif

#endif // ! #ifndef __LIBE15_DBG_H__
