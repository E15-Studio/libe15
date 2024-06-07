"""
This is a script to build fonts for literals scaned from the source code.

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

import sys
import os
import os.path as path
import regex as re
import argparse
import numpy as np
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw

from shape_providers import ssd1306

ENCODING = "utf-8"


def parse_args():
    """Parse command line arguments.

    Returns:
        argparse.Namespace -- The parsed arguments.
    """
    parser = argparse.ArgumentParser(
        description="Build fonts for literals" + "scaned from the source code."
    )

    parser.add_argument(
        "-f",
        "--font",
        type=str,
        metavar="font_name",
        help="Path to the font file.",
        default="SimSun.ttf",
    )

    parser.add_argument(
        "-W",
        "--width",
        type=int,
        metavar="font_width",
        help="Width of the characters.",
        default=8,
    )

    parser.add_argument(
        "-H",
        "--height",
        type=int,
        metavar="font_height",
        help="Height of the characters.",
        default=16,
    )

    parser.add_argument(
        "-O",
        "--output",
        type=str,
        metavar="output_file",
        help="Output file name.",
        required=True,
    )

    parser.add_argument(
        "-A",
        "--generate-all",
        action="store_true",
        help="Generate all ascii characters.",
    )

    parser.add_argument(
        "-C",
        "--encoding",
        type=str,
        metavar="encoding",
        help="Encoding of the source code.",
        default="utf-8",
    )

    parser.add_argument("source", type=str, nargs="*", help="source code file")

    args = parser.parse_args()
    return args


def search_literals(source):
    """Search literals from the source code.

    Arguments:
        source List[str] -- The source code.

    Returns:
        list -- A list of literals.
    """
    result = set()
    # iterate through the source code
    for source_file in source:

        # check if the source file exists
        if not path.isfile(source_file):
            print("File not found: ", source_file)
            sys.exit(1)

        # load lines from the source file
        with open(source_file, "r", encoding=ENCODING) as f:
            lines = f.readlines()

        # iterate through the lines
        for line in lines:
            # search for literals
            literals = re.findall(r'".*?"', line)

            if len(literals) == 0:
                continue

            for literal in literals:
                result.add(literal)

    return list(result)


def extract_characters(literals):
    """Extract characters from literals.

    Arguments:
        literals List[str] -- A list of literals.

    Returns:
        list -- A list of characters.
    """
    result = set()
    for literal in literals:
        for c in literal:
            result.add(c)

    result = list(result)

    # sort the characters
    result.sort()

    return result


def get_char_img_data(char, font_file, size_w, size_h):
    """Get the image data of a character.

    Arguments:
        char str -- The character.
        size_w int -- The width of the character.
        size_h int -- The height of the character.

    Returns:
        numpy.ndarray -- The image data of the character.
    """
    image = Image.new("1", (size_w, size_h), color=0)
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype(font_file, size_h)
    draw.text((0, 0), char, font=font, fill=1)
    arr = np.array(image)

    return arr


# check if all low ascii characters are used
def can_lut_skip_low_ascii(chars):
    for i in range(32, 127):
        if chr(i) not in chars:
            return False
    return True


def has_non_ascii(chars):
    for c in chars:
        if ord(c) > 127:
            return True
    return False


def lut_get_commandline():
    cmd = " ".join(sys.argv)
    cmd = "python " + cmd

    # split for each 77 characters
    list = [cmd[i : i + 77] for i in range(0, len(cmd), 77)]
    return "\n * ".join(list)


def get_code_point_hex_string(char):
    # figure out the width of the code point value
    code_point = ord(char)
    if code_point > 0x10000:
        code_point = f"0x{code_point:08X}"
    elif code_point > 0x100:
        code_point = f"0x{code_point:04X}"
    else:
        code_point = f"0x{code_point:02X}"
    return code_point


def replace_name_to_var(name: str):
    out = []
    for ch in name:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append("_")
    return "".join(out)


def get_glyph_bin_file_name(lut_file_name):
    return lut_file_name + "_glyph.bin"


def generate_lut_header(chars, font_file, size_w, size_h, lut_file_name):

    lut_header_template = """\
/*******************************************************************************
 * This file is generated by fontbuild.py
 * Do not modify this file manually.
 *
 * Parameters used for generating this file:
 * Font: $(font)
 * Width: $(width)
 * Height: $(height)
 *
 * Command:
 * $(command)
 ******************************************************************************/

/* user include */$(user_include)/* user include end */
 
#ifndef __$(file_name_upper)_H__
#define __$(file_name_upper)_H__

extern const bmfont_t $(font_var_name);

/* user code */$(user_code)/* user code end */

#endif // ! #ifndef __$(file_name_upper)_H__
"""
    # get file name
    file_name = lut_file_name + ".h"

    print("generating", file_name)

    user_include = "\n#include <font/bitmap/bm_font.h>\n"
    user_code = "\n\n"

    # load user part if exists
    if path.isfile(file_name):
        with open(file_name, "r", encoding=ENCODING) as f:
            content = f.read()

        # get user include
        m = re.search(
            r"/\* user include \*/(.*?)/\* user include end \*/", content, re.DOTALL
        )
        if m:
            user_include = m.group(1)

        # get user code
        m = re.search(
            r"/\* user code \*/(.*?)/\* user code end \*/", content, re.DOTALL
        )
        if m:
            user_code = m.group(1)

    # get font variable name
    font_var_name = "emb_font_" + replace_name_to_var(lut_file_name)

    # get upper base filename(no path)
    file_name_upper = replace_name_to_var(path.basename(lut_file_name))
    file_name_upper = file_name_upper.upper()

    command = lut_get_commandline()
    file_new_content = lut_header_template
    file_new_content = file_new_content.replace("$(font)", font_file)
    file_new_content = file_new_content.replace("$(width)", str(size_w))
    file_new_content = file_new_content.replace("$(height)", str(size_h))
    file_new_content = file_new_content.replace("$(command)", command)
    file_new_content = file_new_content.replace("$(font_var_name)", font_var_name)
    file_new_content = file_new_content.replace("$(file_name_upper)", file_name_upper)
    file_new_content = file_new_content.replace("$(user_include)", user_include)
    file_new_content = file_new_content.replace("$(user_code)", user_code)

    with open(file_name, "w", encoding=ENCODING) as f:
        f.write(file_new_content)


def generate_lut_list(chars):
    lut_data = []

    for c in chars:

        code_point = get_code_point_hex_string(c)

        # add comma to the end of the line
        code_point = code_point + ","

        # pad code point's right side to make it 20 characters long
        code_point = code_point.ljust(20)

        # add comment and format
        code_point = " " * 4 + code_point + f" /** --> {c} <-- */\n"

        lut_data.append(code_point)

    return "".join(lut_data)


def generate_lut_source(chars, font_file, size_w, size_h, lut_file_name):
    lut_source_template = """\
/*******************************************************************************
 * This file is generated by fontbuild.py
 * Do not modify this file manually.
 *
 * Parameters used for generating this file:
 * Font: $(font)
 * Width: $(width)
 * Height: $(height)
 *
 * Command:
 * $(command)
 ******************************************************************************/

#include "$(header_file_name)"

/* user include */$(user_include)/* user include end */

/**
 * @brief this is a list of unicode characters.
 * 
 * The index of the character in this list corresponds to the index of the glyph
 * in the `$(font_glyph_bin_file)` file.
 */
unicode_char_t emb_font_$(font_var_name)_lut_code_points[] = {
$(lut_data)}; 

/**
 * @brief this is the empty glyph data.
 * @see $(font_var_name)_empty_data
 */
const uint8_t $(font_var_name)_empty_glyph_data[] = {
    $(empty_data)
};

/**
 * @brief this is a symbol for an empty glyph.
 * if the font does not contain a glyph for a particular code point,
 * this empty glyph will be used.
 * 
 * It usually looks like this -> ▩ <-
 */
const bmfont_data_t $(font_var_name)_empty_data = {
    .data = $(font_var_name)_empty_glyph_data,
};

/**
 * @brief lookup table
 * if the table only contains ascii characters, the
 * `code_points` can be NULL.
 */
const bmfont_lut_t $(font_var_name)_lut = {
    .lut_size = $(lut_size),
    .code_points = $(code_points),
    .start = $(lut_start),
    .end = $(lut_end),
};

/**
 * @brief This code segment below is left for you to load the bin file
 * or do something you want.
 */
 
/* user code 0 */$(user_code_0)/* user code 0 end */


const bmfont_t $(font_var_name) = {
    .family = "$(font_c_literal)",
    .width = $(width),
    .height = $(height),

    /* user code 1 */$(user_code_1)/* user code end 1 */

    .lut = &$(font_var_name)_lut,
    .empty_glyph = &$(font_var_name)_empty_data,
};

const bmfont_t $(font_var_name);

/* user code 2 */$(user_code_2)/* user code end 2 */

"""
    print("generating " + lut_file_name + ".c")

    # figure out header file name
    header_file_name = lut_file_name + ".h"
    header_file_name = path.basename(header_file_name)

    lut_file_basename = path.basename(lut_file_name)
    lut_file_var = replace_name_to_var(lut_file_basename)

    # get font variable name
    font_var_name = "emb_font_" + lut_file_var

    # get glyph bin file name
    font_glyph_bin_file = get_glyph_bin_file_name(lut_file_name)
    font_glyph_bin_file = path.basename(font_glyph_bin_file)

    # place a dummy data for empty character
    # need to implement this later
    empty_data = "0x00"

    # get upper base filename(no path)
    file_name_upper = lut_file_var
    file_name_upper = file_name_upper.upper()

    # generate lut list
    lut_data = generate_lut_list(chars)

    # get lut size
    lut_size = str(len(chars))

    # get lut begin, end
    lut_start = get_code_point_hex_string(chars[0])
    lut_end = get_code_point_hex_string(chars[-1])

    # get code points
    code_points = f"emb_font_{font_var_name}_lut_code_points"

    user_include = "\n\n"
    user_code_0 = "\n\n// TODO: add code here.\n\n"
    user_code_1 = """
    
    // TODO: please insert the pointer to embedded bin file
    // * this should be a constant provide by your linker settings.
    .datas = NULL,

    """
    user_code_2 = "\n\n"

    # load user code if exists
    if path.exists(lut_file_name + ".c"):
        with open(lut_file_name + ".c", "r", encoding=ENCODING) as f:
            content = f.read()

        # get user include
        m = re.search(
            r"/\* user include \*/(.*?)/\* user include end \*/", content, re.DOTALL
        )
        if m:
            user_include = m.group(1)

        # get user code 0
        m = re.search(
            r"/\* user code 0 \*/(.*?)/\* user code 0 end \*/", content, re.DOTALL
        )
        if m:
            user_code_0 = m.group(1)

        # get user code 1
        m = re.search(
            r"/\* user code 1 \*/(.*?)/\* user code end 1 \*/", content, re.DOTALL
        )
        if m:
            user_code_1 = m.group(1)

        # get user code 2
        m = re.search(
            r"/\* user code 2 \*/(.*?)/\* user code end 2 \*/", content, re.DOTALL
        )
        if m:
            user_code_2 = m.group(1)

    # get font_c_literal
    font_c_literal = font_file.replace("\\", "\\\\").replace('"', '\\"')

    # generate source content
    command = lut_get_commandline()
    file_new_content = lut_source_template
    file_new_content = file_new_content.replace("$(font)", font_file)
    file_new_content = file_new_content.replace("$(font_c_literal)", font_c_literal)
    file_new_content = file_new_content.replace("$(width)", str(size_w))
    file_new_content = file_new_content.replace("$(height)", str(size_h))
    file_new_content = file_new_content.replace("$(command)", command)
    file_new_content = file_new_content.replace("$(lut_data)", lut_data)
    file_new_content = file_new_content.replace("$(lut_size)", lut_size)
    file_new_content = file_new_content.replace("$(lut_start)", lut_start)
    file_new_content = file_new_content.replace("$(lut_end)", lut_end)
    file_new_content = file_new_content.replace("$(code_points)", code_points)
    file_new_content = file_new_content.replace(
        "$(font_glyph_bin_file)", font_glyph_bin_file
    )
    file_new_content = file_new_content.replace("$(header_file_name)", header_file_name)
    file_new_content = file_new_content.replace("$(font_var_name)", font_var_name)
    file_new_content = file_new_content.replace("$(file_name_upper)", file_name_upper)
    file_new_content = file_new_content.replace("$(user_include)", user_include)
    file_new_content = file_new_content.replace("$(user_code_0)", user_code_0)
    file_new_content = file_new_content.replace("$(user_code_1)", user_code_1)
    file_new_content = file_new_content.replace("$(user_code_2)", user_code_2)
    file_new_content = file_new_content.replace("$(empty_data)", empty_data)

    # write to file
    with open(lut_file_name + ".c", "w", encoding=ENCODING) as f:
        f.write(file_new_content)


def generate_lut(chars, font_file, size_w, size_h, lut_file_name):
    file_name = lut_file_name + "_lut"
    generate_lut_header(chars, font_file, size_w, size_h, file_name)
    generate_lut_source(chars, font_file, size_w, size_h, file_name)


def generate_glyph_bin(chars, font_file, size_w, size_h, lut_file_name):
    glyphs = []
    for ch in chars:
        img_data = get_char_img_data(ch, font_file, size_w, size_h)
        glyph = ssd1306.convert_font_glyph_to_binary(img_data)
        glyphs.append(glyph)

    glyphs = b"".join(glyphs)
    with open(get_glyph_bin_file_name(lut_file_name), "wb") as f:
        f.write(glyphs)


def main():
    # parse args
    args = parse_args()

    # overwrite encoding
    global ENCODING
    ENCODING = args.encoding

    if len(args.source) == 0 and not args.generate_all:
        print("No source code file specified.")
        sys.exit(1)

    # search literals
    literals = search_literals(args.source)

    # extract characters from literals
    chars = extract_characters(literals)

    # generate all ascii characters
    if args.generate_all:
        chars = chars + [chr(i) for i in range(32, 127)]
        chars = list(set(chars))
        chars.sort()

    # remove " "(\x20) from chars (why you need a blank character?)
    if " " in chars:
        chars.remove(" ")

    # get output name
    lut_file_name = args.output

    # generate glyph bin file
    generate_glyph_bin(chars, args.font, args.width, args.height, lut_file_name)

    # generate lookup table
    generate_lut(chars, args.font, args.width, args.height, lut_file_name)


if __name__ == "__main__":
    main()
