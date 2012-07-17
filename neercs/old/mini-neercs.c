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

#include <stdlib.h>
#include <stdio.h> /* perror() */
#include <unistd.h> /* fork() */

#include "mini-neercs.h"

int main(void)
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid > 0)
    {
        client_init();
        while(client_step()) ;
        client_fini();
    }
    else
    {
        server_init();
        while(server_step()) ;
        server_fini();
    }

    return EXIT_SUCCESS;
}

