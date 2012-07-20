/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2010 Sam Hocevar <sam@hocevar.net>
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

#if !defined _WIN32

#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#if defined HAVE_PTY_H
#   include <pty.h>             /* for openpty and forkpty */
#elif defined HAVE_UTIL_H
#   include <util.h>            /* for OS X, OpenBSD and NetBSD */
#elif defined HAVE_LIBUTIL_H
#   include <libutil.h>         /* for FreeBSD */
#endif
#include <unistd.h>
#include <fcntl.h>

#include <caca.h>

#include "neercs.h"


int create_pty(char *cmd, unsigned int w, unsigned int h, int *cpid)
{
    char **argv;
    int fd;
    pid_t pid;

    pid = forkpty(&fd, NULL, NULL, NULL);
    if (pid < 0)
    {
        fprintf(stderr, "forkpty() error\n");
        return -1;
    }
    else if (pid == 0)
    {
        set_tty_size(0, w, h);
        putenv("CACA_DRIVER=slang");
        putenv("TERM=xterm");
        argv = malloc(2 * sizeof(char *));
        if (!argv)
        {
            fprintf(stderr, "Can't allocate memory at %s:%d\n", __FUNCTION__,
                    __LINE__);
            return -1;
        }
        argv[0] = cmd;
        argv[1] = NULL;
        execvp(cmd, argv);
        fprintf(stderr, "execvp() error\n");
        return -1;
    }

    *cpid = pid;

    fcntl(fd, F_SETFL, O_NDELAY);
    return fd;
}

int create_pty_grab(long pid, unsigned int w, unsigned int h, int *newpid)
{
    int fdm, fds;

    int ret = openpty(&fdm, &fds, NULL, NULL, NULL);

    if (ret < 0)
    {
        fprintf(stderr, "open() error\n");
        return -1;
    }

    set_tty_size(0, w, h);
    grab_process(pid, ptsname(fdm), fds, newpid);

    fcntl(fdm, F_SETFL, O_NDELAY);
    return fdm;
}

int set_tty_size(int fd, unsigned int w, unsigned int h)
{
    struct winsize ws;

    memset(&ws, 0, sizeof(ws));
    ws.ws_row = h;
    ws.ws_col = w;
    ioctl(fd, TIOCSWINSZ, (char *)&ws);

    return 0;
}



int update_terms(struct screen_list *screen_list)
{
    int i, refresh = 0;
    for (i = 0; i < screen_list->count; i++)
    {
        if (screen_list->screen[i]->total && !screen_list->dont_update_coords)
        {
            unsigned long int bytes;

            bytes = import_term(screen_list,
                                screen_list->screen[i],
                                screen_list->screen[i]->buf,
                                screen_list->screen[i]->total);

            if (bytes > 0)
            {
                screen_list->screen[i]->total -= bytes;
                memmove(screen_list->screen[i]->buf,
                        screen_list->screen[i]->buf + bytes,
                        screen_list->screen[i]->total);
                if (screen_list->screen[i]->visible || screen_list->modals.mini)
                    refresh = 1;
            }
        }
    }
    return refresh;
}

#endif

