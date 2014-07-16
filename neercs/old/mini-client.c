/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2011 Sam Hocevar <sam@hocevar.net>
 *                2008-2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *                2008-2010 Pascal Terjan <pterjan@linuxfr.org>
 *                All Rights Reserved
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://www.wtfpl.net/ for more details.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h> /* BUFSIZ */
#include <string.h> /* strncmp() */

#include <caca.h>

#include "mini-neercs.h"
#include "mini-socket.h"

static caca_display_t *dp;
static caca_canvas_t *cv;
static nrx_socket_t *insock, *outsock;

void client_init(void)
{
    int i, usec = 10000;

    cv = caca_create_canvas(0, 0);
    dp = caca_create_display(cv);
    caca_set_display_title(dp, "Press Esc to quit");

    insock = socket_open("/tmp/neercs.sock.client", 1);

    for (i = 0; i < 10; i++)
    {
        outsock = socket_open("/tmp/neercs.sock", 0);
        if (outsock)
            break;
        usleep(usec);
        usec += usec;
    }

    socket_puts(outsock, "CONNECT /tmp/neercs.sock.client");
}

int client_step(void)
{
    caca_event_t ev;
    int ret;

    /* Handle client sockets */
    ret = socket_select(insock, 1000);
    if (ret > 0)
    {
        char buf[BUFSIZ];
        ptrdiff_t bytes = socket_read(insock, buf, BUFSIZ);
        if (bytes <= 0)
            return 1;

        /* Parse message */
        if (!strncmp(buf, "OK", strlen("OK")))
        {
            fprintf(stderr, "neercs: connection established\n");
socket_puts(insock, "TEST insock");
        }
    }

    /* Handle libcaca events */
    if(caca_get_event(dp, CACA_EVENT_KEY_PRESS, &ev, 1000)
        && caca_get_event_key_ch(&ev) == CACA_KEY_ESCAPE)
        return 0;

    return 1;
}

void client_fini(void)
{
    socket_puts(outsock, "QUIT /tmp/neercs.sock.client");

    caca_free_display(dp);
    caca_free_canvas(cv);
    if (insock)
        socket_close(insock);
    if (outsock)
        socket_close(outsock);
}

