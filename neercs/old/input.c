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
#include "config.h"
#include <caca.h>
#include <string.h>
#include "neercs.h"

struct kconv
{
    unsigned int key;
    char *val;
    int size;
};

struct kconv kconv[] = {
    {CACA_KEY_UP, "\033OA", 3},
    {CACA_KEY_DOWN, "\033OB", 3},
    {CACA_KEY_RIGHT, "\033OC", 3},
    {CACA_KEY_LEFT, "\033OD", 3},
    {CACA_KEY_PAGEUP, "\033[5~", 4},
    {CACA_KEY_PAGEDOWN, "\033[6~", 4},
    {CACA_KEY_HOME, "\033[1~", 4},
    {CACA_KEY_INSERT, "\033[2~", 4},
    {CACA_KEY_DELETE, "\033[3~", 4},
    {CACA_KEY_END, "\033[4~", 4},
    {CACA_KEY_F1, "\033[11~", 5},
    {CACA_KEY_F2, "\033[12~", 5},
    {CACA_KEY_F3, "\033[13~", 5},
    {CACA_KEY_F4, "\033[14~", 5},
    {CACA_KEY_F5, "\033[15~", 5},
    {CACA_KEY_F6, "\033[17~", 5},
    {CACA_KEY_F7, "\033[18~", 5},
    {CACA_KEY_F8, "\033[19~", 5},
    {CACA_KEY_F9, "\033[20~", 5},
    {CACA_KEY_F10, "\033[21~", 5},
    {CACA_KEY_F11, "\033[23~", 5},
    {CACA_KEY_F12, "\033[24~", 5},
};



void *convert_input_ansi(unsigned int *c, int *size)
{
    unsigned int i;
    for (i = 0; i < sizeof(kconv) / sizeof(struct kconv); i++)
    {
        if (*c == kconv[i].key)
        {
            *size = kconv[i].size;
            return kconv[i].val;
        }
    }

    *size = 1;
    return c;
}



int handle_command_input(struct screen_list *screen_list, unsigned int c)
{
    int refresh = 0;

    debug("Key %x\n", c);
    screen_list->changed = 1;

    if (c >= '0' && c <= '9')
    {
        int n = c - 49;
        if (n < screen_list->count)
        {
            screen_list->prevpty = screen_list->pty;
            screen_list->pty = n == -1 ? 10 : n;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    switch (c)
    {
    case 0x01:                 // CACA_KEY_CTRL_A:
        screen_list->pty ^= screen_list->prevpty;
        screen_list->prevpty ^= screen_list->pty;
        screen_list->pty ^= screen_list->prevpty;
        refresh = 1;
        break;
    case 'm':
    case 0x0d:                 // CACA_KEY_CTRL_M:
        screen_list->modals.mini = !screen_list->modals.mini;
        refresh = 1;
        break;
    case 'n':
    case ' ':
    case '\0':
    case 0x0e:                 // CACA_KEY_CTRL_N:
        screen_list->prevpty = screen_list->pty;
        screen_list->pty = (screen_list->pty + 1) % screen_list->count;
        if (screen_list->pty != screen_list->prevpty)
        {
            screen_list->last_switch = get_us();
            screen_list->cube.in_switch = 1;
            screen_list->cube.side = 0;
        }
        refresh = 1;
        break;
    case 'p':
    case 0x10:                 // CACA_KEY_CTRL_P:
        screen_list->prevpty = screen_list->pty;
        screen_list->pty =
            (screen_list->pty + screen_list->count - 1) % screen_list->count;
        if (screen_list->pty != screen_list->prevpty)
        {
            screen_list->last_switch = get_us();
            screen_list->cube.in_switch = 1;
            screen_list->cube.side = 1;
        }
        refresh = 1;
        break;
    case 'c':
    case 0x03:                 // CACA_KEY_CTRL_C:
        screen_list->prevpty = screen_list->pty;
        screen_list->pty =
            add_screen(screen_list,
                       create_screen(screen_list->width,
                                     screen_list->height,
                                     screen_list->sys.default_shell));
        refresh = 1;
        break;
    case 'w':
    case 0x17:                 // CACA_KEY_CTRL_W:
        screen_list->wm_type = (screen_list->wm_type == (WM_MAX - 1) ?
                                screen_list->wm_type = 0 :
                                screen_list->wm_type + 1);
        refresh = 1;
        break;
    case 'k':
    case 0x0b:                 // CACA_KEY_CTRL_K:
        add_recurrent(screen_list->recurrent_list, close_screen_recurrent,
                      screen_list->cv);
        refresh = 1;
        break;
    case 'x':
    case 0x18:                 // CACA_KEY_CTRL_X:
        memset(screen_list->lock.lockpass, 0, 1024);
        screen_list->lock.locked = 1;
        screen_list->lock.lock_offset = 0;
        refresh = 1;
        break;
    case 'h':
    case 0x08:                 // CACA_KEY_CTRL_H:
        dump_to_file(screen_list);
        break;
    case '?':
        screen_list->modals.help = !screen_list->modals.help;
        refresh = 1;
        break;
    case '"':
    case 0x34:                 // CTRL+"
        screen_list->modals.cur_in_list = screen_list->pty;
        screen_list->modals.window_list = !screen_list->modals.window_list;
        refresh = 1;
        break;
    case 'd':
    case 0x04:                 // CACA_KEY_CTRL_D:
        detach(screen_list);
        break;
#ifdef USE_PYTHON
    case 'e':
    case 0x05:
        debug("py : command is %d, setting to 1 (at %p)\n", screen_list->modals.python_command, &screen_list->modals.python_command);
        screen_list->modals.python_command = 1;
        refresh = 1;
        break;
#endif
    }
    return refresh;

}
