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
 *  http://www.wtfpl.net/ for more details.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#if !defined _WIN32

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include <caca.h>

#include "neercs.h"

struct screen *create_screen_grab(int w, int h, int pid)
{
    struct screen *s = (struct screen *)malloc(sizeof(struct screen));

    s->cv = caca_create_canvas(w, h);
    caca_set_color_ansi(s->cv, CACA_BLACK, CACA_BLACK);
    caca_clear_canvas(s->cv);
    s->init = 0;

    s->buf = NULL;
    s->title = NULL;
    s->total = 0;
    s->w = w;
    s->h = h;
    s->bell = 0;
    s->report_mouse = MOUSE_NONE;

    s->fd = create_pty_grab(pid, w, h, &s->pid);

    if (s->fd < 0)
    {
        caca_free_canvas(s->cv);
        free(s);
        return NULL;
    }
    return s;
}

struct screen *create_screen(int w, int h, char *command)
{
    struct screen *s = (struct screen *)malloc(sizeof(struct screen));

    s->cv = caca_create_canvas(w, h);
    caca_set_color_ansi(s->cv, CACA_BLACK, CACA_BLACK);
    caca_clear_canvas(s->cv);
    s->init = 0;

    s->buf = NULL;
    s->title = NULL;
    s->total = 0;
    s->w = w + 1;
    s->h = h + 1;
    s->bell = 0;
    s->visible = 1;
    s->scroll = 0;
    s->report_mouse = MOUSE_NONE;
    s->fd = create_pty(command, w, h, &s->pid);

    s->orig_w = s->w;
    s->orig_h = s->h;
    s->orig_x = s->x;
    s->orig_y = s->y;


    if (s->fd < 0)
    {
        caca_free_canvas(s->cv);
        free(s);
        return NULL;
    }
    return s;
}

int destroy_screen(struct screen *s)
{
    if (s->fd >= 0)
        close(s->fd);
    if (s->buf)
        free(s->buf);
    if (s->title)
        free(s->title);
    s->buf = NULL;
    if (s->cv)
        caca_free_canvas(s->cv);
    s->cv = NULL;
    free(s);
    return 1;
}

int add_screen(struct screen_list *list, struct screen *s)
{
    if (list == NULL || s == NULL)
        return -1;

    else
    {
        list->screen = (struct screen **)realloc(list->screen,
                                                 sizeof(sizeof
                                                        (struct screen *)) *
                                                 (list->count + 1));
        if (!list->screen)
            fprintf(stderr, "Can't allocate memory at %s:%d\n", __FUNCTION__,
                    __LINE__);
        list->screen[list->count] = s;
        list->count++;
    }

    list->changed = 1;

    return list->count - 1;
}

int remove_screen(struct screen_list *list, int n, int please_kill)
{

    if (n >= list->count)
        return -1;

    if (please_kill)
    {
        int status = 0;
        int ret = 0;
        /* FIXME */
        close(list->screen[n]->fd);
        list->screen[n]->fd = -1;
        kill(list->screen[n]->pid, SIGINT);
        ret = waitpid(list->screen[n]->pid, &status,
                      WNOHANG | WUNTRACED | WCONTINUED);
        if (!ret)
            kill(list->screen[n]->pid, SIGQUIT);
        ret = waitpid(list->screen[n]->pid, &status,
                      WNOHANG | WUNTRACED | WCONTINUED);
        if (!ret)
            kill(list->screen[n]->pid, SIGABRT);
        ret = waitpid(list->screen[n]->pid, &status,
                      WNOHANG | WUNTRACED | WCONTINUED);
        if (!ret)
            kill(list->screen[n]->pid, SIGKILL);

    }
    destroy_screen(list->screen[n]);

    memmove(&list->screen[n],
            &list->screen[n + 1],
            sizeof(struct screen *) * (list->count - (n + 1)));

    list->screen = (struct screen **)realloc(list->screen,
                                             sizeof(sizeof(struct screen *))
                                             * (list->count));
    if (!list->screen)
        fprintf(stderr, "Can't allocate memory at %s:%d\n", __FUNCTION__,
                __LINE__);



    list->count--;
    list->changed = 1;
    return 1;
}



void refresh_screens(struct screen_list *screen_list)
{
    int i;

    if (!screen_list->count)
        return;


    screen_list->width = caca_get_canvas_width(screen_list->cv);
    screen_list->height =
        caca_get_canvas_height(screen_list->cv) - (screen_list->modals.mini * 6) -
        screen_list->modals.status;

    if (!screen_list->dont_update_coords)
    {
        update_windows_props(screen_list);
    }

    if (screen_list->changed)
    {
        caca_set_color_ansi(screen_list->cv, CACA_DEFAULT, CACA_DEFAULT);
        caca_clear_canvas(screen_list->cv);
    }

    wm_refresh(screen_list);

    caca_gotoxy(screen_list->cv,
                screen_list->screen[screen_list->pty]->x +
                caca_get_cursor_x(screen_list->screen[screen_list->pty]->cv),
                screen_list->screen[screen_list->pty]->y +
                caca_get_cursor_y(screen_list->screen[screen_list->pty]->cv));


    if (screen_list->modals.mini)
    {
        draw_thumbnails(screen_list);
    }
    if (screen_list->modals.status)
    {
        draw_status(screen_list);
    }
    if (screen_list->modals.help)
    {
        draw_help(screen_list);
    }
    if (screen_list->modals.window_list)
    {
        draw_list(screen_list);
    }
#ifdef USE_PYTHON
    debug("py : command is %d (at %p)\n", screen_list->modals.python_command, &screen_list->modals.python_command);
    if(screen_list->modals.python_command)
    {
         draw_python_command(screen_list);
    }
#endif
    screen_list->changed = 0;
    for (i = screen_list->count - 1; i >= 0; i--)
        screen_list->screen[i]->changed = 0;
}


int update_screens_contents(struct screen_list *screen_list)
{
    int i, refresh = 0;
    int maxfd = 0;
    struct timeval tv;
    fd_set fdset;
    int ret;

    /* Read data, if any */
    FD_ZERO(&fdset);
    for (i = 0; i < screen_list->count; i++)
    {
        if (screen_list->screen[i]->fd >= 0)
            FD_SET(screen_list->screen[i]->fd, &fdset);
        if (screen_list->screen[i]->fd > maxfd)
            maxfd = screen_list->screen[i]->fd;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    ret = select(maxfd + 1, &fdset, NULL, NULL, &tv);

    if (ret < 0)
    {
        if (errno == EINTR)
            ;                   /* We probably got a SIGWINCH, ignore it */
        else
        {
            /* FIXME: Useless since break will mean that we return 0 But I
               don't know what was the purpose of this code */
            for (i = 0; i < screen_list->count; i++)
                if (screen_list->screen[i]->total)
                    break;
            if (i == screen_list->count)
                return 0;
        }
    }
    else if (ret)
    {

        for (i = 0; i < screen_list->count; i++)
        {
            /* FIXME: try a new strategy: read all filedescriptors until each
               of them starved at least once. */

            if (screen_list->screen[i]->fd < 0 ||
                !FD_ISSET(screen_list->screen[i]->fd, &fdset))
                continue;

            for (;;)
            {
                ssize_t nr;

                screen_list->screen[i]->buf =
                    realloc(screen_list->screen[i]->buf,
                            screen_list->screen[i]->total + 1024);
                if (!screen_list->screen[i]->buf)
                    fprintf(stderr, "Can't allocate memory at %s:%d\n",
                            __FUNCTION__, __LINE__);

                nr = read(screen_list->screen[i]->fd,
                          screen_list->screen[i]->buf +
                          screen_list->screen[i]->total, 1024);

                if (nr > 0)
                {
                    screen_list->screen[i]->total += nr;
                    continue;
                }

                if (nr == 0 || errno != EWOULDBLOCK)
                {
                    remove_screen(screen_list, i, 0);
                    if (i < screen_list->prevpty)
                        screen_list->prevpty--;
                    if (i == screen_list->pty)
                    {
                        screen_list->pty = screen_list->prevpty;
                        screen_list->prevpty = 0;
                    }
                    if (i < (screen_list->pty))
                        (screen_list->pty)--;
                    /* Don't skip the element which is now at position i */
                    i--;
                    refresh = 1;
                }

                break;
            }
        }
    }
    return refresh;
}

#endif

