/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2013 Sam Hocevar <sam@hocevar.net>
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

#if defined HAVE_FORKPTY
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

#include "../neercs.h"

Pty::Pty()
  : m_fd(-1),
    m_pid(-1),
    m_eof(false),
    m_unread_data(0),
    m_unread_len(0),
    m_size(-1, -1)
{
    ;
}

Pty::~Pty()
{
#if defined HAVE_FORKPTY
    delete[] m_unread_data;

    if (m_fd >= 0)
    {
        close((int)m_fd);
    }
#endif
}

void Pty::Run(char const *command, ivec2 size)
{
#if defined HAVE_FORKPTY
    int fd;
    pid_t pid;

    m_pid = -1;
    m_fd = -1;

    pid = forkpty(&fd, NULL, NULL, NULL);
    if (pid < 0)
    {
        fprintf(stderr, "forkpty() error\n");
        return;
    }
    else if (pid == 0)
    {
        SetWindowSize(size, 0);

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

bool Pty::IsEof() const
{
    return m_eof;
}

/* Read data from the PTY. We only perform one read() call so that the
 * caller can decide whether to ask for more data or not. This lets us
 * prioritise data in some way. */
size_t Pty::ReadData(char *data, size_t maxlen)
{
#if defined HAVE_FORKPTY
    size_t sent = 0;

    /* Do we have data from previous call? */
    if (m_unread_len)
    {
        size_t tocopy = min(maxlen, m_unread_len);

        memcpy(data, m_unread_data, tocopy);

        data += tocopy;
        sent += tocopy;
        maxlen -= tocopy;

        if (tocopy < m_unread_len)
        {
            m_unread_len -= tocopy;
            memmove(m_unread_data, m_unread_data + tocopy, m_unread_len);
        }
        else
        {
            delete[] m_unread_data;
            m_unread_data = 0;
            m_unread_len = 0;
        }
    }

    fd_set fdset;
    int maxfd = -1;

    FD_ZERO(&fdset);
    if (m_fd >= 0)
    {
        FD_SET((int)m_fd, &fdset);
        maxfd = std::max(maxfd, (int)m_fd);
    }

    if (maxfd >= 0)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        int ret = select(maxfd + 1, &fdset, NULL, NULL, &tv);

        if (ret < 0)
        {
            Log::Error("cannot read from PTY\n");
            m_eof = true;
            return 0;
        }
        else if (ret)
        {
            if (FD_ISSET((int)m_fd, &fdset))
            {
                ssize_t nr = read((int)m_fd, data, maxlen);

                /* Data available but zero-length read: EOF */
                if (nr <= 0)
                    m_eof = true;
                else
                    sent += nr;

                if (sent >= 0)
                    return sent;
            }
        }
    }
#endif

    return 0;
}

void Pty::UnreadData(char *data, size_t len)
{
#if defined HAVE_FORKPTY
    /* Prepare unread buffer */
    if (m_unread_data)
    {
        char *tmp = new char[m_unread_len + len];
        memcpy(tmp + len, m_unread_data, m_unread_len);
        delete[] m_unread_data;
        m_unread_data = tmp;
        m_unread_len += len;
    }
    else
    {
        m_unread_data = new char[len];
        m_unread_len = len;
    }

    /* Copy data to the unread buffer */
    memcpy(m_unread_data, data, len);
#endif
}

size_t Pty::WriteData(char const *data, size_t len)
{
#if defined HAVE_FORKPTY
    /* FIXME: can we be more naive than that? */
    return write((int)m_fd, data, len);
#endif

    return 0;
}

void Pty::SetWindowSize(ivec2 size, int64_t fd /* = -1 */)
{
#if defined HAVE_FORKPTY
    if (m_size == size)
        return;

    if (fd < 0)
        fd = m_fd;

    m_size = size;

    struct winsize ws;

    memset(&ws, 0, sizeof(ws));
    ws.ws_row = size.y;
    ws.ws_col = size.x;
    ioctl((int)fd, TIOCSWINSZ, (char *)&ws);
#endif
}
