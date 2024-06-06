/**
 * @file error_codes.h
 * @author Simakeng (simakeng@outlook.com)
 * @brief Error codes for libe15
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
 * directory of this project named "LICENSE" file.
 * 
 * https://www.gnu.org/licenses/gpl-3.0.html
 * 
 * *****************************************************************************
 */

#include "generated-conf.h"

#include <stdint.h>
#include <stdlib.h>

#ifndef ERROR_TYPE
typedef int32_t error_t;
#define ERROR_TYPE error_t
#endif

#define ALL_OK 0

/** the dev is too lazy to implement this part.
 * if you see this error, bombard the dev with
 * emails would be a good idea.
 */
#define E_NOT_IMPLEMENTED               -(10001)

#define E_INVALID_ARGUMENT              -(90001)
#define E_INVALID_ADDRESS               -(90002)
#define E_INVALID_OPERATION             -(90003)


#define E_HARDWARE_ERROR                -(60001)

/** operation try to access hardware waits too long */
#define E_HARDWARE_TIMEOUT              -(60500)
/** device is not available now */
#define E_HARDWARE_RESOURCE_BUSY        -(60304)
/** device is not found */
#define E_HARDWARE_NOTFOUND             -(60404)


#define E_MEMORY_ERROR                  -(70001)

#define E_MEMORY_ALLOC_FAILED           -(70002)

#define E_MEMORY_BUFFER_INUSE           -(70005)

#define E_MEMORY_OUT_OF_BOUND           -(70100)

#define FAILED(res) ((res) != ALL_OK)