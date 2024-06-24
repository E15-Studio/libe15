/**
 * @file libe15-devop.h
 * @author simakeng (simakeng@outlook.com)
 * @brief hardware related helper functions or macros
 * @version 0.1
 * @date 2023-06-14
 *
 * @copyright Copyright (c) 2023
 *******************************************************************************
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

#include <error_codes.h>
#include <debug/print.h>

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/

#ifndef __LIBE15_DEVOP_H__
#define __LIBE15_DEVOP_H__

#ifndef __CALLBACK_TYPE_DEF__
#define __CALLBACK_TYPE_DEF__
/**
 * @brief callback function type
 */
typedef void (*callback_t)(void);

/**
 * @brief callback function type with arguments
 */
typedef void (*callback_arg_t)(void *parg);
#endif // ! #ifdef __CALLBACK_TYPE_DEF__

#ifdef CONFIG_USE_DEVOP_ERROR_PRINT
#define dev_err(...) print(ERROR, __VA_ARGS__)
#else
#define dev_err(...) ((void)(0))
#endif // ! #ifdef CONFIG_USE_DEVOP_ERROR_PRINT

#ifndef CALL_NULLABLE
/**
 * @brief call a function and check if it returns an error.
 * the function can be null, in which case it will be ignored.
 */
#define CALL_NULLABLE(func, ...)   \
    do                             \
    {                              \
        if ((func) == ((void *)0)) \
            break;                 \
        else                       \
            (func)(__VA_ARGS__);   \
    } while (0u)
#endif // ! CALL_NULLABLE

#ifndef CALL_WITH_ERROR_RETURN
/**
 * @brief call a function and check if it returns an error.
 * the function **must not** be null, but we don't check it.
 * if the function returns an error, the error will be logged and returned.
 */
#define CALL_WITH_ERROR_RETURN(func, ...)                         \
    do                                                     \
    {                                                      \
        error_t err = (func)(__VA_ARGS__);                 \
        if (FAILED(err))                                   \
        {                                                  \
            dev_err("'" #func "()' "                   \
                        "failed with error code 0x%08X\n", \
                        err);                              \
            return err;                                    \
        }                                                  \
    } while (0u)
#endif // ! CALL_WITH_ERROR_RETURN

#ifndef CALL_WITH_CODE_GOTO
/**
 * @brief call a function and check if it returns an error.
 * the function **must not** be null, but we don't check it.
 * if the function returns an error, the error will be logged
 * and the error code will be set to the given symbol, then
 * goto the given label.
 */
#define CALL_WITH_CODE_GOTO(hr, lab, func, ...)           \
    do                                                     \
    {                                                      \
        hr = (func)(__VA_ARGS__);                          \
        if (FAILED(hr))                                    \
        {                                                  \
            dev_err("'" #func "()' "                   \
                        "failed with error code 0x%08X\n", \
                        err);                              \
            goto lab;                                      \
        }                                                  \
    } while (0u)
#endif // ! CALL_WITH_CODE_GOTO

#ifndef CALL_NULLABLE_WITH_ERROR
/**
 * @brief call a function and check if it returns an error.
 * the function can be null, in which case it will be ignored.
 * if the function returns an error, the error will be logged and returned.
 */
#define CALL_NULLABLE_WITH_ERROR(func, ...)                    \
    do                                                         \
    {                                                          \
        if ((func) == ((void *)0))                             \
            break;                                             \
        else                                                   \
        {                                                      \
            error_t err = (func)(__VA_ARGS__);                 \
            if (FAILED(err))                                   \
            {                                                  \
                dev_err("'" #func "()' "                   \
                            "failed with error code 0x%08X\n", \
                            err);                              \
                return err;                                    \
            }                                                  \
        }                                                      \
    } while (0u)
#endif // ! CALL_NULLABLE_WITH_ERROR

#ifndef CALL_NULLABLE_WITH_ERROR_EXIT
/**
 * @brief call a function and check if it returns an error.
 * the function can be null, in which case it will be ignored.
 * if the function returns an error, the error will be logged and returned.
 */
#define CALL_NULLABLE_WITH_ERROR_EXIT(hr, lab, func, ...)      \
    do                                                         \
    {                                                          \
        if ((func) == ((void *)0))                             \
            break;                                             \
        else                                                   \
        {                                                      \
            hr = (func)(__VA_ARGS__);                          \
            if (FAILED(err))                                   \
            {                                                  \
                dev_err("'" #func "()' "                   \
                            "failed with error code 0x%08X\n", \
                            err);                              \
                goto lab;                                      \
            }                                                  \
        }                                                      \
    } while (0u)
#endif // ! CALL_NULLABLE_WITH_ERROR_EXIT

#ifndef PARAM_CHECK
#define PARAM_CHECK(param, condition)                                  \
    do                                                                 \
    {                                                                  \
        if (!(param condition))                                        \
        {                                                              \
            dev_err("'" #param "' "                                \
                        "not meeting requirement '" #condition "'\n"); \
            return E_INVALID_ARGUMENT;                                 \
        }                                                              \
    } while (0U)
#endif // ! PARAM_CHECK

#ifndef PARAM_CHECK_CODE
#define PARAM_CHECK_CODE(param, condition, code)                       \
    do                                                                 \
    {                                                                  \
        if (!(param condition))                                        \
        {                                                              \
            dev_err("'" #param "' "                                \
                        "not meeting requirement '" #condition "'\n"); \
            return code;                                               \
        }                                                              \
    } while (0U)
#endif // ! PARAM_CHECK_CODE

#ifndef PARAM_NOT_NULL
#define PARAM_NOT_NULL(param)                              \
    do                                                     \
    {                                                      \
        if (param == NULL)                                 \
        {                                                  \
            dev_err("'" #param "' cannot be NULL.\n"); \
            return E_INVALID_ARGUMENT;                     \
        }                                                  \
    } while (0U)

#endif // ! PARAM_NOT_NULL

#ifndef U16ECV
/**
 * @brief convert a 16-bit value's endianness
 * if the value is 0x1234, the result will be 0x3412
 * @param value the value to convert
 */
// #define U16ECV(value)                      \
//     (((((uint16_t)value) & 0x00FF) << 8) | \
//      ((((uint16_t)value) & 0xFF00) >> 8))

#define U16ECV(value) __REV16(value)
#endif // ! U16ECV

#ifndef U32ECV
/**
 * @brief convert a 32-bit value's endianness
 * if the value is 0x12345678, the result will be 0x78563412
 * @param value the value to convert
 */
// #define U32ECV(value)                           \
//     (((((uint32_t)value) & 0x000000FF) << 24) | \
//      ((((uint32_t)value) & 0x0000FF00) << 8) |  \
//      ((((uint32_t)value) & 0x00FF0000) >> 8) |  \
//      ((((uint32_t)value) & 0xFF000000) >> 24))

#define U32ECV(value) __REV(value)
#endif // ! U32ECV

#ifndef MEMCOPY_FUNCMAP
/**
 * @brief map a function to a memory copy operation
 *
 * @param dst the destination address
 * @param src the source address
 * @param ntransfer the number of bytes to transfer
 * @param type the type of the data to transfer
 * @param func the function to map
 */
#define MEMCOPY_FUNCMAP(dst, src, ntransfer, type, func) \
    do                                                   \
    {                                                    \
        uint32_t transfer_left = (ntransfer);            \
        type *src_ptr = (type *)(src);                   \
        type *dst_ptr = (type *)(dst);                   \
        type temp;                                       \
        do                                               \
        {                                                \
            switch (transfer_left)                       \
            {                                            \
            case 0:                                      \
                break;                                   \
            default:                                     \
            case 8:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 7:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 6:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 5:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 4:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 3:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 2:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
            case 1:                                      \
                temp = *dst_ptr++;                       \
                *src_ptr++ = func(temp);                 \
                transfer_left -= 8;                      \
                break;                                   \
            }                                            \
        } while (transfer_left > 0);                     \
    } while (0u)
#endif // ! MEMCOPY_FUNCMAP

#endif // ! __LIBE15_DEVOP_H__