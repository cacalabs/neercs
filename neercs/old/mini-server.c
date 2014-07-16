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

#include "mini-neercs.h"
#include "mini-socket.h"

static nrx_socket_t *insock, *outsock;

void server_init(void)
{
while (!insock)
    insock = socket_open("/tmp/neercs.sock", 1);
}

int server_step(void)
{
    char buf[BUFSIZ];
    ptrdiff_t bytes;
    int ret;

    if (outsock)
    {
        ret = socket_select(outsock, 1000);
        if (ret <= 0)
            goto nothing;

        bytes = socket_read(outsock, buf, BUFSIZ);
        if (bytes <= 0)
            goto nothing;
    }
nothing:

    ret = socket_select(insock, 1000);
    if (ret <= 0)
        return 1;

    bytes = socket_read(insock, buf, BUFSIZ);
    if (bytes <= 0)
        return 1;

    /* Parse message */
    if (!strncmp(buf, "CONNECT ", strlen("CONNECT ")))
    {
        outsock = socket_open(buf + strlen("CONNECT "), 0);
        socket_puts(outsock, "OK");
    }
    else if (!strncmp(buf, "QUIT ", strlen("QUIT ")))
    {
        return 0;
    }

    return 1;
}

void server_fini(void)
{
    if (insock)
        socket_close(insock);
    if (outsock)
        socket_close(outsock);
}

