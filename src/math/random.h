/**
 * @file random.h
 * @author simakeng (simakeng@outlook.com)
 * @brief Functions for random number generator
 * @version 0.1
 * @date 2024-06-06
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
 * directory of this project named "COPYING" file.
 *
 * https://www.gnu.org/licenses/gpl-3.0.html
 *
 * *****************************************************************************
 *
 */
/******************************************************************************/
/*                               INCLUDE FILES                                */
/******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/
#ifndef __RANDOM_H__
#define __RANDOM_H__

#define LCG_DEFAULT_INIT(_seed) \
    {                           \
        .seed = (_seed),        \
        .multiplier = 48271,    \
        .increment = 0,         \
        .modulus = 2147483647,  \
    }

/******************************************************************************/
/*                              TYPE DEFINITIONS                              */
/******************************************************************************/

/**
 * @brief States of a linear congruential generator.
 * 
 * @see https://en.wikipedia.org/wiki/Linear_congruential_generator
 * 
 * @example
 *  rand_lcg_t lcg = LCG_DEFAULT_INIT(12345678);
 *  uint32_t rand = rand_lcg_next(&lcg);
 */
typedef struct
{
    uint32_t seed;
    uint32_t multiplier;
    uint32_t increment;
    uint32_t modulus;
} linear_congruential_generator_t;

typedef linear_congruential_generator_t rand_lcg_t;

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif //! #ifdef __cplusplus

#ifdef __cplusplus
}
#endif //! #ifdef __cplusplus
/******************************************************************************/
/*                           PUBLIC DATA DEFINITIONS                          */
/******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif //! #ifdef __cplusplus

#ifdef __cplusplus
}
#endif //! #ifdef __cplusplus
/******************************************************************************/
/*                         PUBLIC FUNCTION DEFINITIONS                        */
/******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif //! #ifdef __cplusplus

    /**
     * @brief Get the next random number from the LCG.
     * 
     * @see linear_congruential_generator_t
     * 
     * @param lcg 
     * @return uint32_t 
     */
    uint32_t rand_lcg_next(rand_lcg_t *lcg);

#ifdef __cplusplus
}
#endif //! #ifdef __cplusplus
/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/

#endif //! #ifndef __RANDOM_H__
