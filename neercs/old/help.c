/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2010 Sam Hocevar <sam@hocevar.net>
 *                2008-2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *                All Rights Reserved
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <caca.h>

#include "neercs.h"

int help_handle_key(struct screen_list *screen_list, unsigned int c)
{
    if (c == CACA_KEY_ESCAPE || c == '?')
    {
        screen_list->modals.help = 0;
        screen_list->changed = 1;
        return 1;
    }
    else
    {
        return 0;
    }
}

void draw_help(struct screen_list *screen_list)
{
    int w = 65, h = 20;
    int x = (caca_get_canvas_width(screen_list->cv) - w) / 2;
    int y = (caca_get_canvas_height(screen_list->cv) - h) / 2;


    caca_set_color_ansi(screen_list->cv, CACA_BLUE, CACA_BLUE);
    caca_fill_box(screen_list->cv, x, y, w, h, '#');
    caca_set_color_ansi(screen_list->cv, CACA_DEFAULT, CACA_BLUE);
    caca_draw_cp437_box(screen_list->cv, x, y, w, h);

    x += 2;
    y++;
    caca_printf(screen_list->cv,
                (caca_get_canvas_width(screen_list->cv) -
                 strlen(PACKAGE_STRING)) / 2, y - 1, PACKAGE_STRING);
    caca_printf(screen_list->cv, x, y++, "Copyright (c) 2006-2010");
    caca_printf(screen_list->cv, x, y++, "              Sam Hocevar <sam@zoy.org>");
    caca_printf(screen_list->cv, x, y++, "              Jean-Yves Lamoureux <jylam@lnxscene.org>");
    caca_printf(screen_list->cv, x, y++, "              Pascal Terjan <pterjan@linuxfr.org>");
    caca_printf(screen_list->cv, x, y++, "");
    caca_printf(screen_list->cv, x, y++, "");
    caca_printf(screen_list->cv, x, y++, "All shortcuts are in format 'ctrl-a-X' where X is :");
    caca_printf(screen_list->cv, x, y++, "n:        Next window");
    caca_printf(screen_list->cv, x, y++, "p:        Previous window");
    caca_printf(screen_list->cv, x, y++, "w:        Switch window manager");
    caca_printf(screen_list->cv, x, y++, "c:        Create new window");
    caca_printf(screen_list->cv, x, y++, "m:        Thumbnails");
    caca_printf(screen_list->cv, x, y++, "d:        Detach");
    caca_printf(screen_list->cv, x, y++, "k:        Close window and kill associated process");
    caca_printf(screen_list->cv, x, y++, "h:        Dump screen into a file");
    caca_printf(screen_list->cv, x, y++, "?:        This help");
    caca_printf(screen_list->cv, x, y++, "");
    caca_printf(screen_list->cv, x, y++, "");
    caca_printf(screen_list->cv, x, y, "See http://caca.zoy.org/wiki/neercs for more informations");
}
