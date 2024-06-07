"""
This script is a part of fontbuild.

it eat a font image which is a 16 pixel height image and convert it to a
binary array. The array is suitable for SSD1306 OLED display.

Usage: python fontbuild.py [options] [souce code file]

@copyright Copyright (C) E15 Studio 2024

This program is FREE software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 3 as published by the 
Free Software Foundation.
This program is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
675 Mass Ave, Cambridge, MA 02139, USA. Or you can visit the link below to 
read the license online, or you can find a copy of the license in the root 
directory of this project named "LICENSE" file.

https://www.gnu.org/licenses/gpl-3.0.html

"""

import numpy as np

# generate image radix
radix = 2 ** np.arange(16)


def convert_font_glyph_to_binary(img):
    """
    Convert a font image to a binary array.
    """

    # check if the image is 16 pixel height
    if img.shape[0] != 16:
        raise ValueError("SSD1306 font image must be 16 pixel height")

    # tasnpose to put each vertical line in a row
    img = img.T

    # convert to uint32
    img = img.astype(np.uint32)

    # multiply by radix
    img = np.dot(img, radix)

    # convert back to uint16
    img = img.astype(np.uint16)

    # get binary array
    img = img.tobytes()

    return img
