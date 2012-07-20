/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2010 Sam Hocevar <sam@hocevar.net>
 *                2008-2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *                2008-2011 Pascal Terjan <pterjan@linuxfr.org>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <pwd.h>

#include <errno.h>
#include <caca.h>

#include "neercs.h"

#define NEERCS_RECV_BUFSIZE 128*1024


int start_client(struct screen_list *screen_list)
{
    char *sess = NULL;
    create_socket(screen_list, SOCK_CLIENT);
    while ((sess = connect_socket(screen_list, SOCK_SERVER)) == NULL)
        usleep(100);
    free(sess);

    /* Create main canvas and associated caca window */
    screen_list->cv = caca_create_canvas(0, 0);
    screen_list->dp = caca_create_display(screen_list->cv);
    screen_list->mouse_button = 0;

    if (!screen_list->dp)
        return -3;

    caca_set_display_time(screen_list->dp, screen_list->delay * 1000);
    caca_set_cursor(screen_list->dp, 1);

    request_attach(screen_list);

    return 0;
}

int send_event(caca_event_t ev, struct screen_list *screen_list)
{
    enum caca_event_type t;
    char buf[64];
    int bytes = 0;

    t = caca_get_event_type(&ev);

    if (t & CACA_EVENT_KEY_PRESS)
    {
        bytes =  snprintf(buf, sizeof(buf) - 1, "KEY %d",
                          caca_get_event_key_ch(&ev));
        debug("Sending key press to server: %s", buf);
    }
    else if (t & CACA_EVENT_RESIZE)
    {
        bytes = snprintf(buf, sizeof(buf) - 1, "RESIZE %10d %10d",
                         caca_get_event_resize_width(&ev),
                         caca_get_event_resize_height(&ev));
    }
    else if (t & CACA_EVENT_MOUSE_PRESS)
    {
        screen_list->mouse_button = caca_get_event_mouse_button(&ev);
        bytes = snprintf(buf, sizeof(buf) - 1, "MOUSEP %10d %10d %10d",
                         caca_get_mouse_x(screen_list->dp),
                         caca_get_mouse_y(screen_list->dp),
                         screen_list->mouse_button);
    }
    else if (t & CACA_EVENT_MOUSE_RELEASE)
    {
        bytes = snprintf(buf, sizeof(buf) - 1, "MOUSER %10d %10d %10d",
                         caca_get_mouse_x(screen_list->dp),
                         caca_get_mouse_y(screen_list->dp),
                         screen_list->mouse_button);
        screen_list->mouse_button = 0;
    }
    else if (t & CACA_EVENT_MOUSE_MOTION)
    {
        int x = caca_get_mouse_x(screen_list->dp);
        int y = caca_get_mouse_y(screen_list->dp);
        int b = screen_list->mouse_button;
        debug("Mouse motion, button %d", b);
        if (x != screen_list->old_x || y != screen_list->old_y)
        {
            screen_list->old_x = caca_get_mouse_x(screen_list->dp);
            screen_list->old_y = caca_get_mouse_y(screen_list->dp);

            bytes = snprintf(buf, sizeof(buf) - 1, "MOUSEM %10d %10d %10d",
                             caca_get_mouse_x(screen_list->dp),
                             caca_get_mouse_y(screen_list->dp),
                             b>=0?b:0);
        }
    }
    else if (t & CACA_EVENT_QUIT)
    {
        bytes = snprintf(buf, sizeof(buf) - 1, "QUIT");
    }
    if (bytes)
    {
        ssize_t r;
        buf[bytes] = '\0';
        debug("Sending '%s', %d bytes", buf, bytes);
        r = write(screen_list->comm.socket[SOCK_SERVER], buf, bytes+1);
        while (r < 0 && errno == EAGAIN)
        {
            usleep(20);
            r = write(screen_list->comm.socket[SOCK_SERVER], buf, bytes+1);
        }
        return r < 0;
    }
    return 0;
}

int send_delay(struct screen_list *screen_list)
{
    debug("Sending DELAY\n");
    char buf[18];
    int bytes;
    bytes = snprintf(buf, sizeof(buf) - 1, "DELAY %10d", screen_list->delay);
    buf[bytes] = '\0';
    return write(screen_list->comm.socket[SOCK_SERVER], buf, strlen(buf)) <= 0;
}

/** \brief Main client loop.
 *
 * This is the main client loop.
 *
 * Repeat forever:
 *  - if data is available on the client socket, read it and interpret it:
 *    - "DETACH": exit the loop
 *    - "UPDATE": update screen with the given canvas data
 *    - "REFRESH": refresh the whole display
 *    - "CURSOR": set cursor position
 *    - "TITLE": set window or display title
 *  - wait for an input event with a 10ms timeout
 */
void mainloop(struct screen_list *screen_list)
{
    char *buf = NULL;
    screen_list->last_key_time = get_us();

    while (mainloop_tick(&buf, screen_list))
        ;

    free(buf);
}

int mainloop_tick(char **pbuf, struct screen_list *screen_list)
{
    caca_event_t ev;
    int ret = 0;
    ssize_t n;
    if (!screen_list)
        return 0;
    if (!*pbuf)
        *pbuf = malloc(NEERCS_RECV_BUFSIZE);
    if (!*pbuf)
    {
        debug("Failed to allocate memory");
        return 0;
    }
    if (screen_list->comm.socket[SOCK_CLIENT]
        && (n =
            read(screen_list->comm.socket[SOCK_CLIENT], *pbuf,
                 NEERCS_RECV_BUFSIZE - 1)) > 0)
    {
        *pbuf[n] = 0;
        debug("Received from server: '%s' (%d bytes)", *pbuf, n);
        if (!strncmp("DETACH", *pbuf, 6))
        {
            /* ret = 1; Not used */
            return 1;
        }
        else if (!strncmp("UPDATE ", *pbuf, 7))
        {
            int x, y;
            ssize_t l2 = 0, lb = 0;
            char *buf2;
            size_t l = 0;
            /* FIXME check the length before calling atoi */
            x = atoi(*pbuf + 8);
            y = atoi(*pbuf + 19);

            /* 0 means we have valid data but incomplete, so read the rest
             */
            while (l == 0)
            {
                buf2 = realloc(*pbuf, l2 + NEERCS_RECV_BUFSIZE);
                if (!buf2)
                {
                    debug("Failed to allocate memory");
                    return 0;
                }
                *pbuf = buf2;
                fcntl(screen_list->comm.socket[SOCK_CLIENT], F_SETFL, 0);
                lb = read(screen_list->comm.socket[SOCK_CLIENT], *pbuf + l2,
                          NEERCS_RECV_BUFSIZE - 1);
                if (lb < 0)
                {
                    debug
                        ("Failed to read the end of the refresh message (%s)",
                         strerror(errno));
                    l = -1;
                }
                else
                {
                    l2 += lb;
                    l = caca_import_area_from_memory(screen_list->cv, x, y,
                                                     *pbuf, l2, "caca");
                }
            }
            fcntl(screen_list->comm.socket[SOCK_CLIENT], F_SETFL,
                  O_NONBLOCK);
        }
        else if (!strncmp("REFRESH ", *pbuf, 8))
        {
            int dt, x, y;
            /* FIXME check the length before calling atoi */
            x = atoi(*pbuf + 8);
            y = atoi(*pbuf + 19);
            caca_gotoxy(screen_list->cv, x, y);
            caca_refresh_display(screen_list->dp);
            dt = caca_get_display_time(screen_list->dp);

            /* Adjust refresh delay so that the server do not compute
               useless things */
            if (dt > 2 * 1000 * screen_list->delay
                && screen_list->delay <= 100)
            {
                screen_list->delay *= 2;
                send_delay(screen_list);
            }
            else if (dt < screen_list->delay * 1000 * 1.2 &&
                     screen_list->delay >=
                     3 * screen_list->requested_delay / 2)
            {
                screen_list->delay = 2 * screen_list->delay / 3;
                send_delay(screen_list);
            }
            screen_list->comm.attached = 1;
        }
        else if (!strncmp("CURSOR ", *pbuf, 7))
        {
            caca_set_cursor(screen_list->dp, atoi(*pbuf + 7));
        }
        else if (!strncmp("TITLE ", *pbuf, 6))
        {
            caca_set_display_title(screen_list->dp, *pbuf + 6);
            caca_refresh_display(screen_list->dp);
        }
        else
        {
            debug("Unknown message received from server: %s", *pbuf);
        }
    }

    /* Wait to have finished attaching before handling events */
    if (screen_list->comm.attached)
    {
        ret = caca_get_event(screen_list->dp,
                             CACA_EVENT_KEY_PRESS
                             | CACA_EVENT_MOUSE_PRESS
                             | CACA_EVENT_MOUSE_RELEASE
                             | CACA_EVENT_MOUSE_MOTION
                             | CACA_EVENT_RESIZE
                             | CACA_EVENT_QUIT, &ev, 10000);
        if (ret)
            ret = send_event(ev, screen_list);
        if (ret)
        {
            debug("Failed send event, quitting: %s\n", strerror(errno));
            return 1;
        }
    }

    return 1;
}

#endif

