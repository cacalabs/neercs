/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2012 Sam Hocevar <sam@hocevar.net>
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

#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
#   define _XOPEN_SOURCE
#   include <stdlib.h>
#   include <stdio.h>
#   include <string.h>
#   include <sys/ioctl.h>
#   include <sys/types.h>
#   include <termios.h>
#   if defined HAVE_PTY_H
#       include <pty.h>             /* for openpty and forkpty */
#   elif defined HAVE_UTIL_H
#       include <util.h>            /* for OS X, OpenBSD and NetBSD */
#   elif defined HAVE_LIBUTIL_H
#       include <libutil.h>         /* for FreeBSD */
#   endif
#   include <unistd.h>
#   include <fcntl.h>
#endif

#include "core.h"
#include "loldebug.h"

using namespace std;
using namespace lol;

#include "neercs.h"

Pty::Pty(ivec2 size)
  : m_size(size)
{
    ;
}

Pty::~Pty()
{

}

void Pty::Run(char const *command)
{
#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    int fd;
    pid_t pid;

    m_pid = 0;
    m_fd = 0;

    pid = forkpty(&fd, NULL, NULL, NULL);
    if (pid < 0)
    {
        fprintf(stderr, "forkpty() error\n");
        return;
    }
    else if (pid == 0)
    {
        SetWindowSize(m_size);

        /* putenv() eats the string, they need to be writable */
        static char tmp1[] = "CACA_DRIVER=slang";
        static char tmp2[] = "TERM=xterm";
        putenv(tmp1);
        putenv(tmp2);

        m_argv[0] = command;
        m_argv[1] = NULL;
        /* The following const cast is valid. The Open Group Base
         * Specification guarantees that the objects are completely
         * constant. */
        execvp(command, const_cast<char * const *>(m_argv));
        fprintf(stderr, "execvp() error\n");
        return;
    }

    fcntl(fd, F_SETFL, O_NDELAY);

    m_pid = pid;
    m_fd = fd;
#endif
}

void Pty::SetWindowSize(ivec2 size)
{
#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    m_size = size;

    struct winsize ws;

    memset(&ws, 0, sizeof(ws));
    ws.ws_row = size.y;
    ws.ws_col = size.x;
    ioctl((int)m_fd, TIOCSWINSZ, (char *)&ws);
#endif
}

#if 0
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

