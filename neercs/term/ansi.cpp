/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2010 Sam Hocevar <sam@hocevar.net>
 *                2008-2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *                2008-2010 Pascal Terjan <pterjan@linuxfr.org>
 *                All Rights Reserved
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <string.h>

#include "core.h"

using namespace std;
using namespace lol;

#include "../neercs.h"
#include "term.h"

/* DEC ACS with common extensions */
static uint32_t dec_acs(uint32_t uc)
{
    switch (uc)
    {
    case '+':
        return 0x2192;          /* RIGHTWARDS ARROW */
    case ',':
        return 0x2190;          /* LEFTWARDS ARROW */
    case '-':
        return 0x2191;          /* UPWARDS ARROW */
    case '.':
        return 0x2193;          /* DOWNWARDS ARROW */
    case '0':
        return 0x25AE;          /* BLACK VERTICAL RECTANGLE */
    case '_':
        return 0x25AE;          /* BLACK VERTICAL RECTANGLE */
    case '`':
        return 0x25C6;          /* BLACK DIAMOND */
    case 'a':
        return 0x2592;          /* MEDIUM SHADE */
    case 'b':
        return 0x2409;          /* SYMBOL FOR HORIZONTAL TABULATION */
    case 'c':
        return 0x240C;          /* SYMBOL FOR FORM FEED */
    case 'd':
        return 0x240D;          /* SYMBOL FOR CARRIAGE RETURN */
    case 'e':
        return 0x240A;          /* SYMBOL FOR LINE FEED */
    case 'f':
        return 0x00B0;          /* DEGREE SIGN */
    case 'g':
        return 0x00B1;          /* PLUS-MINUS SIGN */
    case 'h':
        return 0x2424;          /* SYMBOL FOR NEWLINE */
    case 'i':
        return 0x240B;          /* SYMBOL FOR VERTICAL TABULATION */
    case 'j':
        return 0x2518;          /* BOX DRAWINGS LIGHT UP AND LEFT */
    case 'k':
        return 0x2510;          /* BOX DRAWINGS LIGHT DOWN AND LEFT */
    case 'l':
        return 0x250C;          /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    case 'm':
        return 0x2514;          /* BOX DRAWINGS LIGHT UP AND RIGHT */
    case 'n':
        return 0x253C;          /* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    case 'o':
        return 0x23BA;          /* HORIZONTAL SCAN LINE-1 */
    case 'p':
        return 0x23BB;          /* HORIZONTAL SCAN LINE-3 */
    case 'q':
        return 0x2500;          /* BOX DRAWINGS LIGHT HORIZONTAL */
    case 'r':
        return 0x23BC;          /* HORIZONTAL SCAN LINE-7 */
    case 's':
        return 0x23BD;          /* HORIZONTAL SCAN LINE-9 */
    case 't':
        return 0x251C;          /* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    case 'u':
        return 0x2524;          /* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    case 'v':
        return 0x2534;          /* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    case 'w':
        return 0x252C;          /* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    case 'x':
        return 0x2502;          /* BOX DRAWINGS LIGHT VERTICAL */
    case 'y':
        return 0x2264;          /* LESS-THAN OR EQUAL TO */
    case 'z':
        return 0x2265;          /* GREATER-THAN OR EQUAL TO */
    case '{':
        return 0x03C0;          /* GREEK SMALL LETTER PI */
    case '|':
        return 0x2260;          /* NOT EQUAL TO */
    case '}':
        return 0x00A3;          /* POUND SIGN */
    case '~':
        return 0x00B7;          /* MIDDLE DOT */
    default:
        return uc;
    }
};

#define LITERAL2CHAR(i0,i1) (((i0) << 8) | (i1))

#define LITERAL3CHAR(i0,i1,i2) LITERAL2CHAR(LITERAL2CHAR(i0, i1), i2)

inline int Term::ReadChar(unsigned char c, int *x, int *y)
{
    if (c == '\r')
    {
        *x = 0;
    }

    else if (c == '\n')
    {
        *x = 0;
        *y = *y + 1;
    }
    else if (c == '\a')
    {
        /* FIXME TODO: in_bell ? */
//        if (!m_bell)
//            screen_list->in_bell = 10;
        m_bell = 1;
    }

    else if (c == '\t')
    {
        *x = (*x + 7) & ~7;
    }

    else if (c == '\x08')
    {
        if (*x > 0)
            *x = *x - 1;
    }
    else if (c == '\x0b')
    {
        /* Vertical tab */
        /* Not sure about the real meaning of it, just y++ for now */
        if (*y < caca_get_canvas_height(m_caca))
            *y = *y + 1;
    }
    else if (c == '\x0e')
    {
        /* Shift Out (Ctrl-N) -> Switch to Alternate Character Set: invokes
           the G1 character set. */
        m_conv_state.glr[0] = 1;
    }

    else if (c == '\x0f')
    {
        /* Shift In (Ctrl-O) -> Switch to Standard Character Set: invokes the
           G0 character set. */
        m_conv_state.glr[0] = 0;
    }
    else
    {
        return 1;
    }
    return 0;
}

inline int Term::ReadDuplet(unsigned char const *buffer, unsigned int *skip,
                          int top, int bottom, int width, int height)
{
    int i = 0, j, k;
    unsigned int dummy = 0;

    /* Single Shift Select of G2 Character Set (SS2: 0x8e): affects next
       character only */
    if (buffer[i] == '\033' && buffer[i + 1] == 'N')
    {
        m_conv_state.ss = 2;
        *skip += 1;
    }
    /* Reverse Index (RI) go up one line, reverse scroll if necessary */
    else if (buffer[i] == '\033' && buffer[i + 1] == 'M')
    {
        /* FIXME : not sure about the meaning of 'go up one line' and 'if
           necessary' words. Implemented as a scroller only. */
        for (j = bottom - 1; j > top; j--)
        {
            for (k = 0; k < width; k++)
            {
                caca_put_char(m_caca, k, j, caca_get_char(m_caca, k, j - 1));
                caca_put_attr(m_caca, k, j, caca_get_attr(m_caca, k, j - 1));
            }
        }
        caca_draw_line(m_caca, 0, top - 1, width - 1, top - 1, ' ');
        *skip += 1;
    }

    /* Single Shift Select of G3 Character Set (SS2: 0x8f): affects next
       character only */
    else if (buffer[i] == '\033' && buffer[i + 1] == 'O')
    {
        m_conv_state.ss = 3;
        *skip += 1;
    }

    /* LOCKING-SHIFT TWO (LS2), ISO 2022, ECMA-48 (1986), ISO 6429 : 1988 */
    else if (buffer[i] == '\033' && buffer[i + 1] == 'n')
    {
        m_conv_state.glr[0] = 2;
        *skip += 1;
    }

    /* LOCKING-SHIFT THREE (LS3) ISO 2022, ECMA-48 (1986), ISO 6429 : 1988 */
    else if (buffer[i] == '\033' && buffer[i + 1] == 'o')
    {
        m_conv_state.glr[0] = 3;
        *skip += 1;
    }

    /* RESET TO INITIAL STATE (RIS), ECMA-48 (1986), ISO 6429 : 1988 */
    else if (buffer[i] == '\033' && buffer[i + 1] == 'c')
    {
        m_dfg = CACA_DEFAULT;
        m_dbg = CACA_DEFAULT;

        caca_set_color_ansi(m_caca, m_dfg, m_dbg);
        m_clearattr = caca_get_attr(m_caca, -1, -1);
        ReadGrcm(1, &dummy);

        m_conv_state.Reset();
        *skip += 1;
    }

    /* Coding Method Delimiter (CMD), ECMA-48 (1991), ISO/IEC 6429:1992 (ISO
       IR 189) */
    else if (buffer[i] == '\033' && buffer[i + 1] == 'd')
    {
        m_conv_state.Reset();
        *skip += 1;
    }
    else
    {
        return 1;
    }

    return 0;
}

size_t Term::ReadAnsi(void const *data, size_t size)
{
    unsigned char const *buffer = (unsigned char const *)data;
    unsigned int i, j, k, skip, dummy = 0;
    unsigned int width, height, top, bottom;
    uint32_t savedattr;
    int x = 0, y = 0, save_x = 0, save_y = 0;
    char b[100];

    debug("ansi : import_term\n");

    width = caca_get_canvas_width(m_caca);
    height = caca_get_canvas_height(m_caca);
    x = caca_get_cursor_x(m_caca);
    y = caca_get_cursor_y(m_caca);
    top = 1;
    bottom = height;

    if (!m_init)
    {
        m_dfg = CACA_LIGHTGRAY;
        m_dbg = CACA_BLACK;

        caca_set_color_ansi(m_caca, m_dfg, m_dbg);
        m_clearattr = caca_get_attr(m_caca, -1, -1);

        ReadGrcm(1, &dummy);

        m_conv_state.Reset();

        m_init = 1;
    }

    for (i = 0; i < size; i += skip)
    {
        uint32_t ch = 0;
        int wch = 0;

        skip = 1;

        /* Control codes (ASCII < \x20) */
        if (!ReadChar(buffer[i], &x, &y))
        {
        }

        /* If there are not enough characters to parse the escape sequence,
           wait until the next try. We require 3. */

        else if (buffer[i] == '\033' && i + 2 >= size)
            break;


        else if (!ReadDuplet(&buffer[i], &skip, top, bottom, width, height))
        {

        }


        /* GZDM4, G0-Designators, multi, 94^n chars [grandfathered short form
           from ISO 2022:1986] */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && (buffer[i + 2] >= '@') && (buffer[i + 2] <= 'C'))
        {
            m_conv_state.gn[0] = LITERAL2CHAR('$', buffer[i + 2]);
            skip += 2;
        }

        /* GnDMx Gn-Designators, 9x^n chars; need one more char to distinguish
           these */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && (i + 3 >= size))
            break;

        /* GZD4 G0-Designator, 94 chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '(')
        {
            m_conv_state.gn[0] = buffer[i + 2];
            skip += 2;
        }

        /* G1D4 G1-Designator, 94 chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == ')')
        {
            m_conv_state.gn[1] = buffer[i + 2];
            skip += 2;
        }

        /* G2D4 G2-Designator, 94 chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '*')
        {
            m_conv_state.gn[2] = buffer[i + 2];
            skip += 2;
        }

        /* G3D4 G3-Designator, 94 chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '+')
        {
            m_conv_state.gn[3] = buffer[i + 2];
            skip += 2;
        }

        /* G2D6 G2-Designator, 96 chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '.')
        {
            m_conv_state.gn[2] = LITERAL2CHAR('.', buffer[i + 2]);
            skip += 2;
        }

        /* G3D6 G3-Designator, 96 chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '/')
        {
            m_conv_state.gn[3] = LITERAL2CHAR('.', buffer[i + 2]);
            skip += 2;
        }

        /* GZDM4 G0-Designator, 94^n chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && buffer[i + 2] == '(')
        {
            m_conv_state.gn[0] = LITERAL2CHAR('$', buffer[i + 3]);
            skip += 3;
        }

        /* G1DM4 G1-Designator, 94^n chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && buffer[i + 2] == ')')
        {
            m_conv_state.gn[1] = LITERAL2CHAR('$', buffer[i + 3]);
            skip += 3;
        }

        /* G2DM4 G2-Designator, 94^n chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && buffer[i + 2] == '*')
        {
            m_conv_state.gn[2] = LITERAL2CHAR('$', buffer[i + 3]);
            skip += 3;
        }

        /* G3DM4 G3-Designator, 94^n chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && buffer[i + 2] == '+')
        {
            m_conv_state.gn[3] = LITERAL2CHAR('$', buffer[i + 3]);
            skip += 3;
        }

        /* G2DM6 G2-Designator, 96^n chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && buffer[i + 2] == '.')
        {
            m_conv_state.gn[2] = LITERAL3CHAR('$', '.', buffer[i + 3]);
            skip += 3;
        }

        /* G3DM6 G3-Designator, 96^n chars */
        else if (buffer[i] == '\033' && buffer[i + 1] == '$'
                 && buffer[i + 2] == '/')
        {
            m_conv_state.gn[3] = LITERAL3CHAR('$', '.', buffer[i + 3]);
            skip += 3;
        }
        else if (buffer[i] == '\033' && buffer[i + 1] == '#')
        {
            debug("ansi private '#' sequence\n");

            switch (buffer[i + 2])
            {
            case '8':          /* DECALN Fills the entire screen area with
                                   uppercase Es for screen focus and
                                   alignment. */
                for (j = 0; j < height; j++)
                {
                    for (k = 0; k < width; k++)
                    {
                        caca_put_char(m_caca, k, j, 'E');
                    }
                }
                x = 0;
                y = 0;
                skip += 2;
                break;

            default:
                debug("Unknow private sequence 'ESC#%c'\n", buffer[i + 2]);
                continue;
            }

        }
        /* Interpret escape commands, as per Standard ECMA-48 "Control
           Functions for Coded Character Sets", 5.4. Control sequences. */
        else if (buffer[i] == '\033' && buffer[i + 1] == '[')
        {
            unsigned int argc = 0, argv[101];
            unsigned int param, inter, junk, final;

            if (buffer[i + 2] == '?')
            {
                debug("CSI? %c%c%c%c%c\n",
                      buffer[i + 3], buffer[i + 4], buffer[i + 5],
                      buffer[i + 6], buffer[i + 7]);
            }

            /* Compute offsets to parameter bytes, intermediate bytes and to
               the final byte. Only the final byte is mandatory, there can be
               zero of the others. 0 param=2 inter final final+1
               +-----+------------------+---------------------+-----------------+
               | CSI | parameter bytes | intermediate bytes | final byte | | |
               0x30 - 0x3f | 0x20 - 0x2f | 0x40 - 0x7e | | ^[[ | 0123456789:;<=>?
               | SPC !"#$%&'()*+,-./ | azAZ@[\]^_`{|}~ |
               +-----+------------------+---------------------+-----------------+ */
            param = 2;

            /* vttest use to interleave control characters (\014 CR or \010
               BS) into CSI sequences, either directly after ESC[ or after
               param. Can't find anything related to this in any documentation
               nor XTerm sources, thought. */

            for (junk = param; i + junk < size; junk++)
                if (buffer[i + junk] < 0x20)
                {
                    ReadChar(buffer[i + junk], &x, &y);
                }
                else
                {
                    break;
                }

            /* Intermediate offset */
            for (inter = junk; i + inter < size; inter++)
                if (buffer[i + inter] < 0x30 || buffer[i + inter] > 0x3f)
                {
                    break;
                }
            /* Interleaved character */
            for (junk = inter; i + junk < size; junk++)
                if (buffer[i + junk] < 0x20)
                {
                    ReadChar(buffer[i + junk], &x, &y);
                }
                else
                {
                    break;
                }

            /* Final Byte offset */
            for (final = junk; i + final < size; final++)
                if (buffer[i + final] < 0x20 || buffer[i + final] > 0x2f)
                {
                    break;
                }
            if (i + final >= size
                || buffer[i + final] < 0x40 || buffer[i + final] > 0x7e)
            {
                debug("ansi Invalid Final Byte (%d %c)\n", buffer[i + final],
                      buffer[i + final]);
                break;          /* Invalid Final Byte */
            }

            skip += final;

            /* Sanity checks */
            if (param < inter && buffer[i + param] >= 0x3c)
            {
                /* Private sequence, only parse what we know */
                debug("ansi import: private sequence \"^[[%.*s\"",
                      final - param + 1, buffer + i + param);
                /* FIXME better parsing */
                if (buffer[i + 2] == '?')
                {
                    char arg[5], *end;
                    int a = 0;
                    int c, p, Pm;
                    for (p = 0; p < 4; p++)
                    {
                        if (buffer[i + 3 + p] >= '0'
                            && buffer[i + 3 + p] <= '9')
                        {
                            arg[a] = buffer[i + 3 + p];
                            arg[a + 1] = 0;
                            a++;
                            debug("private a now '%s'\n", arg);
                        }
                        else
                        {
                            break;
                        }
                    }
                    Pm = strtol(arg, &end, 10);

                    c = buffer[i + 3 + (end - arg)];

                    debug("ansi private mouse : command %c, arg %d", c, Pm);
                    if (c == 'h')       /* DECSET DEC Private Mode Set */
                    {

                        switch (Pm)
                        {
                            /* FIXME Handle different modes */
                        case 9:
                            debug("mouse : X10 mode\n");
                            m_report_mouse = MOUSE_X10;
                            break;
                        case 1000:     /* Send Mouse X & Y on button press
                                           and release.  */
                                debug("mouse : VT200 mode\n");
                            m_report_mouse = MOUSE_VT200;
                            break;
                        case 1001:     /* Use Hilite Mouse Tracking.  */
                                debug("mouse : VT200_HIGHLIGHT mode\n");
                            m_report_mouse = MOUSE_VT200_HIGHLIGHT;
                            break;
                        case 1002:     /* Use Cell Motion Mouse Tracking. */
                                debug("mouse : BTN mode\n");
                            m_report_mouse = MOUSE_BTN_EVENT;
                            break;
                        case 1003:     /* Use All Motion Mouse Tracking.  */
                                debug("mouse : ANY mode\n");
                            m_report_mouse = MOUSE_ANY_EVENT;
                            break;
                        default:
                            break;
                        }
                    }
                    else if (c == 'l')  /* DECRST DEC Private Mode Reset */
                    {
                        Pm = atoi(arg);
                        switch (Pm)
                        {
                            /* FIXME Handle different modes */
                        case 9:
                        case 1000:     /* Send Mouse X & Y on button press
                                           and release.  */
                        case 1001:     /* Use Hilite Mouse Tracking.  */
                        case 1002:     /* Use Cell Motion Mouse Tracking. */
                        case 1003:     /* Use All Motion Mouse Tracking.  */
                            m_report_mouse = MOUSE_NONE;
                            debug("ansi private mouse : NOT reporting mouse");
                            break;
                        default:
                            break;
                        }
                    }
                }
                continue;       /* Private sequence, skip it entirely */
            }

            if (final - param > 100)
                continue;       /* Suspiciously long sequence, skip it */

            /* Parse parameter bytes as per ECMA-48 5.4.2: Parameter string
               format */
            if (param < inter)
            {
                argv[0] = 0;
                for (j = param; j < inter; j++)
                {
                    if (buffer[i + j] == ';')
                        argv[++argc] = 0;
                    else if (buffer[i + j] >= '0' && buffer[i + j] <= '9')
                        argv[argc] = 10 * argv[argc] + (buffer[i + j] - '0');
                }
                argc++;
            }

            /* Interpret final byte. The code representations are given in
               ECMA-48 5.4: Control sequences, and the code definitions are
               given in ECMA-48 8.3: Definition of control functions. */
            debug("ansi import: command '%c'", buffer[i + final]);
            switch (buffer[i + final])
            {
            case 'A':          /* CUU (0x41) - Cursor Up */
                y -= argc ? argv[0] : 1;
                if (y < 0)
                    y = 0;
                break;
            case 'B':          /* CUD (0x42) - Cursor Down */
                y += argc ? argv[0] : 1;
                break;
            case 'C':          /* CUF (0x43) - Cursor Right */
                x += argc ? argv[0] : 1;
                break;
            case 'D':          /* CUB (0x44) - Cursor Left */
                x -= argc ? argv[0] : 1;
                if (x < 0)
                    x = 0;
                break;
            case 'G':          /* CHA (0x47) - Cursor Character Absolute */
                x = (argc && argv[0] > 0) ? argv[0] - 1 : 0;
                break;
            case 'H':          /* CUP (0x48) - Cursor Position */
                x = (argc > 1 && argv[1] > 0) ? argv[1] - 1 : 0;
                y = (argc > 0 && argv[0] > 0) ? argv[0] - 1 : 0;
                debug("ansi CUP : Cursor at %dx%d\n", x, y);
                break;
            case 'J':          /* ED (0x4a) - Erase In Page */
                savedattr = caca_get_attr(m_caca, -1, -1);
                caca_set_attr(m_caca, m_clearattr);
                if (!argc || argv[0] == 0)
                {
                    caca_draw_line(m_caca, x, y, width, y, ' ');
                    caca_fill_box(m_caca, 0, y + 1, width, height - 1, ' ');
                }
                else if (argv[0] == 1)
                {
                    caca_fill_box(m_caca, 0, 0, width, y, ' ');
                    caca_draw_line(m_caca, 0, y, x, y, ' ');
                }
                else if (argv[0] == 2)
                {
                    // x = y = 0;
                    caca_fill_box(m_caca, 0, 0, width, height, ' ');
                }
                caca_set_attr(m_caca, savedattr);
                break;
            case 'K':          /* EL (0x4b) - Erase In Line */
                debug("ansi EL : cursor at %dx%d\n", x, y);
                if (!argc || argv[0] == 0)
                {
                    caca_draw_line(m_caca, x, y, width, y, ' ');
                }
                else if (argv[0] == 1)
                {
                    caca_draw_line(m_caca, 0, y, x, y, ' ');
                }
                else if (argv[0] == 2)
                {
                    caca_draw_line(m_caca, 0, y, width, y, ' ');
                }
                break;
            case 'L':          /* IL - Insert line */
                {
                    unsigned int nb_lines = argc ? argv[0] : 1;
                    for (j = bottom - 1; j >= (unsigned int)y + nb_lines; j--)
                    {
                        for (k = 0; k < width; k++)
                        {
                            caca_put_char(m_caca, k, j,
                                          caca_get_char(m_caca, k,
                                                        j - nb_lines));
                            caca_put_attr(m_caca, k, j,
                                          caca_get_attr(m_caca, k,
                                                        j - nb_lines));
                        }
                        caca_draw_line(m_caca, 0, j - nb_lines, width,
                                       j - nb_lines, ' ');
                    }
                }
                break;
            case 'P':          /* DCH (0x50) - Delete Character */
                if (!argc || argv[0] == 0)
                    argv[0] = 1;        /* echo -ne 'foobar\r\e[0P\n' */

                for (j = x; (unsigned int)(j + argv[0]) < width; j++)
                {
                    caca_put_char(m_caca, j, y,
                                  caca_get_char(m_caca, j + argv[0], y));
                    caca_put_attr(m_caca, j, y,
                                  caca_get_attr(m_caca, j + argv[0], y));
                }
                break;
#if 0
                savedattr = caca_get_attr(m_caca, -1, -1);
                caca_set_attr(m_caca, m_clearattr);
                for (; (unsigned int)j < width; j++)
                    caca_put_char(m_caca, j, y, ' ');
                caca_set_attr(m_caca, savedattr);
#endif
            case 'X':          /* ECH (0x58) - Erase Character */
                if (argc && argv[0])
                {
                    savedattr = caca_get_attr(m_caca, -1, -1);
                    caca_set_attr(m_caca, m_clearattr);
                    caca_draw_line(m_caca, x, y, x + argv[0] - 1, y, ' ');
                    caca_set_attr(m_caca, savedattr);
                }
            case 'c':          /* DA -- Device Attributes */
                /*
                   0 Base VT100, no options 1 Processor options (STP) 2
                   Advanced video option (AVO) 3 AVO and STP 4 Graphics
                   processor option (GPO) 5 GPO and STP 6 GPO and AVO 7 GPO,
                   STP, and AVO */
                /* Warning, argument is Pn */
                debug("ansi Got command c, argc %d, argv[0] (%d)\n", argc,
                      argv[0], argv[0]);
                if (!argc || argv[0] == 0)
                {
                    SendAnsi("\x1b[?1;0c");
                }
                else
                {
                    switch (argv[0])
                    {
                    case 1:
                        SendAnsi("\x1b[?\x1;\x1c");
                        break;
                    case 2:
                        SendAnsi("\x1b[?\x1;\x2c");
                        break;
                    case 3:
                        SendAnsi("\x1b[?\x1;\x3c");
                        break;
                    case 4:
                        SendAnsi("\x1b[?\x1;\x4c");
                        break;
                    case 5:
                        SendAnsi("\x1b[?\x1;\x5c");
                        break;
                    case 6:
                        SendAnsi("\x1b[?\x1;\x6c");
                        break;
                    case 7:
                        SendAnsi("\x1b[?\x1;\x7c");
                        break;
                    default:
                        debug("Unsupported DA option '%d'\n", argv[0]);
                        break;
                    }
                }
                break;
            case 'd':          /* VPA (0x64) - Line Position Absolute */
                y = (argc && argv[0] > 0) ? argv[0] - 1 : 0;
                break;
            case 'f':          /* HVP (0x66) - Character And Line Position */
                x = (argc > 1 && argv[1] > 0) ? argv[1] - 1 : 0;
                y = (argc > 0 && argv[0] > 0) ? argv[0] - 1 : 0;
                break;
            case 'g':          /* TBC -- Tabulation Clear */
                break;
            case 'r':          /* FIXME */
                if (argc == 2)  /* DCSTBM - Set top and bottom margin */
                {
                    debug("DCSTBM %d %d", argv[0], argv[1]);
                    top = argv[0];
                    bottom = argv[1];
                }
                else
                    debug("ansi import: command r with %d params", argc);
                break;
            case 'h':          /* SM (0x68) - FIXME */
                debug("ansi import: set mode %i", argc ? (int)argv[0] : -1);
                break;
            case 'l':          /* RM (0x6c) - FIXME */
                debug("ansi import: reset mode %i", argc ? (int)argv[0] : -1);
                break;
            case 'm':          /* SGR (0x6d) - Select Graphic Rendition */
                if (argc)
                    ReadGrcm(argc, argv);
                else
                    ReadGrcm(1, &dummy);
                break;
            case 'n':
                debug("ansi command n, argc %d, argv[0] %d\n", argc, argv[0]);
                if (!argc)
                    break;

                switch (argv[0])
                {
                case 5:
                    /* Term ok */
                    SendAnsi("\x1b[0n");
                    break;
                case 6:
                    /* Cursor Position */
                    sprintf(b, "\x1b[%d;%dR", y + 1, x + 1);
                    SendAnsi(b);
                    break;
                }

                break;
            case 's':          /* Private (save cursor position) */
                save_x = x;
                save_y = y;
                break;
            case 'u':          /* Private (reload cursor position) */
                x = save_x;
                y = save_y;
                break;
            default:
                debug("ansi import: unknown command \"^[%.*s\"",
                      final - param + 1, buffer + i + param);
                break;
            }
        }

        /* Parse OSC stuff. */
        else if (buffer[i] == '\033' && buffer[i + 1] == ']')
        {
            char *string;
            unsigned int command = 0;
            unsigned int mode = 2, semicolon, final;

            for (semicolon = mode; i + semicolon < size; semicolon++)
            {
                if (buffer[i + semicolon] < '0' || buffer[i + semicolon] > '9')
                    break;
                command = 10 * command + (buffer[i + semicolon] - '0');
            }

            if (i + semicolon >= size || buffer[i + semicolon] != ';')
                break;          /* Invalid Mode */

            for (final = semicolon + 1; i + final < size; final++)
                if (buffer[i + final] < 0x20)
                    break;

            if (i + final >= size || buffer[i + final] != '\a')
                break;          /* Not enough data or no bell found */
            /* FIXME: XTerm also reacts to <ESC><backslash> and <ST> */
            /* FIXME: differenciate between not enough data (try again) and
               invalid data (print shit) */

            skip += final;

            string = (char *)malloc(final - (semicolon + 1) + 1);
            memcpy(string, buffer + i + (semicolon + 1),
                   final - (semicolon + 1));
            string[final - (semicolon + 1)] = '\0';
            debug("ansi import: got OSC command %i string '%s'", command,
                  string);
            if (command == 0 || command == 2)
            {
                if (m_title)
                    free(m_title);
                m_title = string;
            }
            else
                free(string);
        }

        /* Get the character we’re going to paste */
        else
        {
            size_t bytes;

            if (i + 6 < size)
            {
                ch = caca_utf8_to_utf32((char const *)(buffer + i), &bytes);
            }
            else
            {
                /* Add a trailing zero to what we're going to read */
                char tmp[7];
                memcpy(tmp, buffer + i, size - i);
                tmp[size - i] = '\0';
                ch = caca_utf8_to_utf32(tmp, &bytes);
            }

            if (!bytes)
            {
                /* If the Unicode is invalid, assume it was latin1. */
                ch = buffer[i];
                bytes = 1;
            }

            /* very incomplete ISO-2022 implementation tailored to DEC ACS */
            if (m_conv_state.cs == '@')
            {
                if (((ch > ' ') && (ch <= '~'))
                    &&
                    (m_conv_state.
                     gn[m_conv_state.ss ? m_conv_state.
                        gn[m_conv_state.ss] : m_conv_state.glr[0]] == '0'))
                {
                    ch = dec_acs(ch);
                }
                else if (((ch > 0x80) && (ch < 0xff))
                         && (m_conv_state.gn[m_conv_state.glr[1]] == '0'))
                {
                    ch = dec_acs(ch + ' ' - 0x80);
                }
            }
            m_conv_state.ss = 0;      /* no single-shift (GL) */

            wch = caca_utf32_is_fullwidth(ch) ? 2 : 1;

            skip += bytes - 1;
        }

        /* Wrap long lines or grow horizontally */
        while ((unsigned int)x + wch > width)
        {
            x -= width;
            y++;
        }

        /* Scroll or grow vertically */
        if ((unsigned int)y >= bottom)
        {
            int lines = (y - bottom) + 1;

            savedattr = caca_get_attr(m_caca, -1, -1);

            for (j = top - 1; j + lines < bottom; j++)
            {
                for (k = 0; k < width; k++)
                {
                    caca_put_char(m_caca, k, j,
                                  caca_get_char(m_caca, k, j + lines));
                    caca_put_attr(m_caca, k, j,
                                  caca_get_attr(m_caca, k, j + lines));
                }
            }
            caca_set_attr(m_caca, m_clearattr);
            caca_fill_box(m_caca, 0, bottom - lines, width, bottom - 1, ' ');
            y -= lines;
            caca_set_attr(m_caca, savedattr);
        }

        /* Now paste our character, if any */
        if (wch)
        {
            caca_put_char(m_caca, x, y, ch);
            x += wch;
        }
    }

    caca_gotoxy(m_caca, x, y);

    if (i)
        m_changed = 1;
    return i;
}

size_t Term::SendAnsi(char const *str)
{
    size_t ret = strlen(str);

    /* FIXME TODO: not implemented */
    Log::Debug("Sending %d-character ANSI sequence\n", (int)ret);

    return ret;
}

/* Coding Method Delimiter (CMD), ECMA-48 (1991), ISO/IEC 6429:1992 (ISO IR
   189) */
void Iso2022Conversion::Reset()
{
    cs = '@';    /* ISO-2022 coding system */
    cn[0] = '@'; /* ISO 646 C0 control charset */
    cn[1] = 'C'; /* ISO 6429-1983 C1 control charset */
    glr[0] = 0;  /* G0 in GL */
    glr[1] = 2;  /* G2 in GR */
    gn[0] = 'B'; /* US-ASCII G0 charset */
    gn[1] = '0'; /* DEC ACS G1 charset */
    gn[2] = LITERAL2CHAR('.', 'A'); /* ISO 8859-1 G2 charset */
    gn[3] = LITERAL2CHAR('.', 'A'); /* ISO 8859-1 G3 charset */
    ss = 0;      /* no single-shift (GL) */
    ctrl8bit = 1;
}

/* XXX : ANSI loader helper */

void Term::ReadGrcm(unsigned int argc, unsigned int const *argv)
{
    static uint8_t const ansi2caca[] = {
        CACA_BLACK, CACA_RED, CACA_GREEN, CACA_BROWN,
        CACA_BLUE, CACA_MAGENTA, CACA_CYAN, CACA_LIGHTGRAY
    };

    unsigned int j;
    uint8_t efg, ebg;           /* Effective (libcaca) fg/bg */

    for (j = 0; j < argc; j++)
    {
        /* Defined in ECMA-48 8.3.117: SGR - SELECT GRAPHIC RENDITION */
        if (argv[j] >= 30 && argv[j] <= 37)
            m_fg = ansi2caca[argv[j] - 30];
        else if (argv[j] >= 40 && argv[j] <= 47)
            m_bg = ansi2caca[argv[j] - 40];
        else if (argv[j] >= 90 && argv[j] <= 97)
            m_fg = ansi2caca[argv[j] - 90] + 8;
        else if (argv[j] >= 100 && argv[j] <= 107)
            m_bg = ansi2caca[argv[j] - 100] + 8;
        else
            switch (argv[j])
            {
            case 0:            /* default rendition */
                m_fg = m_dfg;
                m_bg = m_dbg;
                m_bold = m_blink = m_italics = m_negative
                       = m_concealed = m_underline = m_faint = m_strike
                       = m_proportional = 0;
                break;
            case 1:            /* bold or increased intensity */
                m_bold = 1;
                break;
            case 2:            /* faint, decreased intensity or second colour
                                 */
                m_faint = 1;
                break;
            case 3:            /* italicized */
                m_italics = 1;
                break;
            case 4:            /* singly underlined */
                m_underline = 1;
                break;
            case 5:            /* slowly blinking (less then 150 per minute) */
            case 6:            /* rapidly blinking (150 per minute or more) */
                m_blink = 1;
                break;
            case 7:            /* negative image */
                m_negative = 1;
                break;
            case 8:            /* concealed characters */
                m_concealed = 1;
                break;
            case 9:            /* crossed-out (characters still legible but
                                   marked as to be deleted */
                m_strike = 1;
                break;
            case 21:           /* doubly underlined */
                m_underline = 1;
                break;
            case 22:           /* normal colour or normal intensity (neither
                                   bold nor faint) */
                m_bold = m_faint = 0;
                break;
            case 23:           /* not italicized, not fraktur */
                m_italics = 0;
                break;
            case 24:           /* not underlined (neither singly nor doubly) */
                m_underline = 0;
                break;
            case 25:           /* steady (not blinking) */
                m_blink = 0;
                break;
            case 26:           /* (reserved for proportional spacing as
                                   specified in CCITT Recommendation T.61) */
                m_proportional = 1;
                break;
            case 27:           /* positive image */
                m_negative = 0;
                break;
            case 28:           /* revealed characters */
                m_concealed = 0;
                break;
            case 29:           /* not crossed out */
                m_strike = 0;
                break;
            case 38:           /* (reserved for future standardization,
                                   intended for setting character foreground
                                   colour as specified in ISO 8613-6 [CCITT
                                   Recommendation T.416]) */
                break;
            case 39:           /* default display colour
                                   (implementation-defined) */
                m_fg = m_dfg;
                break;
            case 48:           /* (reserved for future standardization,
                                   intended for setting character background
                                   colour as specified in ISO 8613-6 [CCITT
                                   Recommendation T.416]) */
                break;
            case 49:           /* default background colour
                                   (implementation-defined) */
                m_bg = m_dbg;
                break;
            case 50:           /* (reserved for cancelling the effect of the
                                   rendering aspect established by parameter
                                   value 26) */
                m_proportional = 0;
                break;
            default:
                debug("ansi import: unknown sgr %i", argv[j]);
                break;
            }
    }

    if (m_concealed)
    {
        efg = ebg = CACA_TRANSPARENT;
    }
    else
    {
        efg = m_negative ? m_bg : m_fg;
        ebg = m_negative ? m_fg : m_bg;

        if (m_bold)
        {
            if (efg < 8)
                efg += 8;
            else if (efg == CACA_DEFAULT)
                efg = CACA_WHITE;
        }
    }

    caca_set_color_ansi(m_caca, efg, ebg);
}
