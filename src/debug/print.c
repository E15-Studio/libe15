/**
 * @file libe15-dbg.c
 * @author Simakeng (simakeng@outlook.com)
 * @brief Debugging functions.
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
 * 
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "print.h"

#ifdef CONFIG_USE_LIBE15_LOG_PRINT

#ifndef unused
#define unused(X) ((void)X)
#endif // !#ifndef unused


static int32_t dbg_default_dev_puts(const char *s);
static int32_t dbg_default_dev_putc(int ch);

static volatile dbg_low_level_io_ops_t dbg_dev_ops = {
    .putc = dbg_default_dev_putc,
    .puts = dbg_default_dev_puts};

static volatile int32_t last_char = 0x00;

void dbg_print_init(dbg_low_level_io_ops_t *ops)
{
    dbg_dev_ops.putc = dbg_default_dev_putc;
    dbg_dev_ops.puts = dbg_default_dev_puts;

    if (ops == NULL)
        return;

    if (ops->putc != NULL)
        dbg_dev_ops.putc = ops->putc;

    if (ops->puts != NULL)
        dbg_dev_ops.puts = ops->puts;

    return;
}

static int32_t dbg_default_dev_putc(int ch)
{
    return EOF;
}

static int32_t dbg_default_dev_puts(const char *s)
{
    return EOF;
}

int32_t dbg_putc_warper(int ch)
{
    last_char = ch;
    return dbg_dev_ops.putc(ch);
}

int32_t dbg_dev_puts(const char *s)
{
    int32_t i = 0;
    int32_t cnt = 0;
    while (*s != 0)
    {
        i = dbg_putc_warper(*s);
        if (i == EOF)
            return EOF;
        s++;
        cnt++;
    }
    return cnt;
}

int32_t dbg_puts_warper(const char *s)
{
    const char *ch = s;

    while (*ch != 0)
        last_char = *ch++;

    int32_t cnt = dbg_dev_ops.puts(s);
    return cnt;
}

const char *translate_level(int32_t level)
{
    switch (level)
    {
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO ";
    case WARN:
        return "WARN ";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    default:
        return "UNKNO";
    }
}

int is_fmt_specifier(int ch)
{
    switch (ch)
    {
    case 'd':
    case 'i':
    case 'c':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    case 'F':
    case 'f':
    case 'E':
    case 'e':
    case 'G':
    case 'g':
    case 'A':
    case 'a':
    case 's':
        return 1;
    default:
        return 0;
    }
}

int is_specifier_attr(int ch)
{
    switch (ch)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
    case '+':
    case '-':
    case '#':
    case '*':
        return 1;
    default:
        return 0;
    }
}

int get_fmt_string(const char *start, const char **end_out)
{
    while (*start != 0)
    {
        if (is_specifier_attr(*start))
            start++;
        else if (is_fmt_specifier(*start))
        {
            *end_out = start + 1;
            return 1;
        }
        else
            return 0;
    }
    return 0;
}

#undef print
void dbg_print(int32_t level, const char *location, const char *function, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);

    unused(location);
    unused(function);
    unused(level);

    if (last_char == '\n' || last_char == '\0')
    {

#ifdef CONFIG_DEBUG_COLOR
        switch (level)
        {
        case ERROR:
            dbg_dev_puts(COLOR_MAGENTA);
            break;
        case FATAL:
            dbg_dev_puts(COLOR_RED);
            break;
        case WARN:
            dbg_dev_puts(COLOR_YELLOW);
            break;
        case INFO:
            dbg_dev_puts(COLOR_BEIGE);
            break;
        default:
            dbg_dev_puts(COLOR_RESET);
            break;
        }
#endif

#ifndef CONFIG_OMIT_LEVEL
        dbg_dev_puts("[");
        dbg_dev_puts(translate_level(level));
        dbg_dev_puts("] ");
#endif

#ifndef CONFIG_OMIT_LOCATION
        dbg_dev_puts("[");
#ifdef CONFIG_LOCATION_FULLNAME
        dbg_dev_puts(location);
#else
        {
            const char *last_spliter_nix = strrchr(location, '/');
            const char *last_spliter_win = strrchr(location, '\\');
            const char *last_spliter;
            if (last_spliter_nix > last_spliter_win)
                last_spliter = last_spliter_nix;
            else
                last_spliter = last_spliter_win;
            if (last_spliter == NULL)
                last_spliter = location;
            else
                last_spliter += 1;
            dbg_dev_puts(last_spliter);
        }
#endif

        dbg_dev_puts("] ");
#endif

#ifndef CONFIG_OMIT_FUNCTION_NAME
        dbg_dev_puts("[");
        dbg_dev_puts(function);
        dbg_dev_puts("] ");
#endif
    }

    const char *ch = msg;
    while (*ch != 0)
    {
        if (*ch == '%')
        {
            const char *ch_end = NULL;
            if (get_fmt_string(ch + 1, &ch_end))
            {
                int32_t fmt_specifier = *(ch_end - 1);
                int32_t fmt_len = ch_end - ch;
                if (fmt_len > 32)
                    goto not_a_valid_fmt;

                char fmt[32] = {'%', 0};
                memcpy(fmt + 1, ch + 1, fmt_len - 1);
                char buf[64] = {0};
                char *output = buf;

                switch (fmt_specifier)
                {
                case 'd':
                case 'i':
                case 'c':
                {
                    int32_t d = va_arg(args, int32_t);
                    snprintf(buf, 32, fmt, d);
                    break;
                }

                case 'u':
                case 'o':
                case 'x':
                case 'X':
                {
                    int32_t u = va_arg(args, uint32_t);
                    snprintf(buf, 32, fmt, u);
                    break;
                }

                case 'F':
                case 'f':
                case 'E':
                case 'e':
                case 'G':
                case 'g':
                case 'A':
                case 'a':
                {
                    double f = va_arg(args, double);
                    snprintf(buf, 32, fmt, f);
                    break;
                }

                case 's':
                {
                    output = va_arg(args, char *);
                    break;
                }

                case '%':
                {
                    buf[0] = '%';
                    buf[1] = 0;
                    break;
                }
                default:
                    buf[0] = '%';
                    buf[1] = fmt_specifier;
                    buf[2] = 0;
                    break;
                }
                dbg_dev_puts(output);
                ch = ch_end;
                continue;
            }
            else
            {
            not_a_valid_fmt:
                dbg_putc_warper(*ch);
            }
        }
        // The intention is to add a '\r' character before the '\n' character.
        // If the 'ch' pointer is pointing to the first character of the msg
        // string, or the previous character is not '\r', then we add it.
        else if (*ch == '\n')
        {
            // Because '||' is a short-circuit operator, if the first condition
            // is true, the second one will not be evaluated. and if the first
            // condition is false, then we can safely access the previous
            // character.
            if (ch == msg || *(ch - 1) != '\r')
                dbg_putc_warper('\r');
            dbg_putc_warper(*ch);
        }
        else
            dbg_putc_warper(*ch);

        ++ch;
    }

    va_end(args);
}

#endif // !#ifdef CONFIG_USE_LIBE15_LOG_PRINT
