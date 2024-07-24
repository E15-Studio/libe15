/**
 * @file template.h
 * @author simakeng (simakeng@outlook.com)
 * @brief This file is too show how code is organized in this project.
 * @version 0.1
 * @date 2024-06-05
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

#include <stdint.h>

/******************************************************************************/
/*                              MACRO DEFINITIONS                             */
/******************************************************************************/

#ifndef __COLOR_H__
#define __COLOR_H__

/******************************************************************************/
/*                            CONSTANT DEFINITIONS                            */
/******************************************************************************/

/******************************************************************************/
/*                              TYPE DEFINITIONS                              */
/******************************************************************************/

typedef uint16_t rgb565_t;

#pragma pack(push, 1)

typedef struct
{
    uint8_t r, g, b;
} rgb888_t;

#pragma pack(pop)

/******************************************************************************/
/*                           PUBLIC DATA DEFINITIONS                          */
/******************************************************************************/

/******************************************************************************/
/*                         PUBLIC FUNCTION DEFINITIONS                        */
/******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif //! #ifdef __cplusplus

#ifdef __cplusplus
}
#endif //! #ifdef __cplusplus
/******************************************************************************/
/*                                 END OF FILE                                */
/******************************************************************************/

#endif //! #ifndef __COLOR_H__
