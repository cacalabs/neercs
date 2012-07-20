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

#if !defined _WIN32

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <pwd.h>

#include <errno.h>
#include <caca.h>

#include "neercs.h"

static void server_init(struct screen_list *screen_list);
static void refresh_screen(struct screen_list *screen_list, int refresh);
static int handle_key(struct screen_list *screen_list, unsigned int c,
                      int refresh);
static int handle_attach(struct screen_list *screen_list, char *buf);

static int send_to_client(const char *msg, int size,
                          struct screen_list *screen_list)
{
    int ret;
    if (!screen_list->comm.socket[SOCK_CLIENT])
        connect_socket(screen_list, SOCK_CLIENT);
    if (!screen_list->comm.socket[SOCK_CLIENT])
        ret = -1;
    else
        ret = write(screen_list->comm.socket[SOCK_CLIENT], msg, size);
    if (ret < 0 && errno != EAGAIN)
    {
        fprintf(stderr, "Failed to send message to client: %s\n",
                strerror(errno));
        if (screen_list->comm.attached)
            detach(screen_list);
    }
    return ret;
}

static int set_title(struct screen_list *screen_list)
{
    char buf[1024];
    int bytes;
    char *title = NULL;

    if (screen_list->comm.attached)
    {
        if (screen_list->pty < screen_list->count &&
            screen_list->screen[screen_list->pty]->title)
            title = screen_list->screen[screen_list->pty]->title;
        else
            title = PACKAGE_STRING;
    }

    if (screen_list->title)
    {
        if (!strcmp(screen_list->title, title))
            return 0;
        free(screen_list->title);
    }

    screen_list->title = strdup(title);

    bytes = snprintf(buf, sizeof(buf) - 1, "TITLE %s", title);
    buf[bytes] = '\0';
    return send_to_client(buf, strlen(buf), screen_list);
}

static int set_cursor(int state, struct screen_list *screen_list)
{
    char buf[16];
    int bytes;

    bytes = snprintf(buf, sizeof(buf) - 1, "CURSOR %d", state);
    buf[bytes] = '\0';

    return send_to_client(buf, strlen(buf), screen_list);
}

static int request_refresh(struct screen_list *screen_list)
{
    int ndirty = caca_get_dirty_rect_count(screen_list->cv);
    if (!ndirty)
        return 0;
    if (!screen_list->comm.socket[SOCK_CLIENT])
        connect_socket(screen_list, SOCK_CLIENT);
    if (screen_list->comm.socket[SOCK_CLIENT])
    {
        size_t bufsize, towrite;
        ssize_t written, ret;
        socklen_t optlen = sizeof(bufsize);
        size_t bytes;
        void *buf;
        char buf2[32];
        int x, y, i;

        getsockopt(screen_list->comm.socket[SOCK_CLIENT], SOL_SOCKET,
                   SO_SNDBUF, &bufsize, &optlen);
        bufsize /= 2;
        debug("bufsize=%d", bufsize);

        for (i = 0; i < ndirty; i++)
        {
            int w, h;
            caca_get_dirty_rect(screen_list->cv, i, &x, &y, &w, &h);
            debug("dirty @%d,%d %dx%d [%dx%d]", x, y, w, h,
                  caca_get_canvas_width(screen_list->cv),
                  caca_get_canvas_height(screen_list->cv));
            buf =
                caca_export_area_to_memory(screen_list->cv, x, y, w, h, "caca",
                                           &bytes);
            debug("Requesting refresh for %d", bytes);
            towrite = bytes;
            written = 0;
            sprintf(buf2, "UPDATE %10d %10d", x, y);
            ret = send_to_client(buf2, strlen(buf2) + 1, screen_list);
            if (ret < 29)
            {
                free(buf);
                return -1;
            }
            /* Block to write the end of the message */
            fcntl(screen_list->comm.socket[SOCK_CLIENT], F_SETFL, 0);
            while (towrite > 0)
            {
                ssize_t n;
                debug("Wrote %d, %d remaining", written, towrite);
                n = send_to_client((char *)buf + written,
                                   towrite > bufsize ? bufsize : towrite,
                                   screen_list);
                if (n < 0)
                {
                    debug("Can't refresh (%s), with %d bytes (out of %d)",
                          strerror(errno),
                          towrite > bufsize ? bufsize : towrite, towrite);
                    return -1;
                }
                written += n;
                towrite -= n;
            }
            fcntl(screen_list->comm.socket[SOCK_CLIENT], F_SETFL, O_NONBLOCK);
            free(buf);
        }
        sprintf(buf2, "REFRESH %10d %10d", caca_get_cursor_x(screen_list->cv),
                caca_get_cursor_y(screen_list->cv));
        /* FIXME check value of r */
        int r = send_to_client(buf2, strlen(buf2) + 1, screen_list);
        (void)r;
        caca_clear_dirty_rect_list(screen_list->cv);
    }
    return 0;
}

int detach(struct screen_list *screen_list)
{
    screen_list->comm.attached = 0;
    if (screen_list->lock.lock_on_detach)
        screen_list->lock.locked = 1;
    if (screen_list->comm.socket[SOCK_CLIENT])
    {
        send_to_client("DETACH", 6, screen_list);
        close(screen_list->comm.socket[SOCK_CLIENT]);
        screen_list->comm.socket[SOCK_CLIENT] = 0;
    }
    return 0;
}

static int server_iteration(struct screen_list *screen_list)
{
    int i;
    int eof = 0, refresh;

    int quit = 0;
    ssize_t n;
    char buf[128];

    /* Read program output */
    refresh = update_screens_contents(screen_list);

    /* Check if we got something from the client */
    while (screen_list->comm.socket[SOCK_SERVER]
           && (n =
               read(screen_list->comm.socket[SOCK_SERVER], buf,
                    sizeof(buf) - 1)) > 0)
    {
        buf[n] = 0;
        debug("Received command %s", buf);
        if (!strncmp("ATTACH ", buf, 7))
        {
            refresh |= handle_attach(screen_list, buf);
        }
        else if (!strncmp("QUIT", buf, 4))
        {
            quit = 1;
        }
        else if (!strncmp("DELAY ", buf, 6))
        {
            /* FIXME check the length before calling atoi */
            screen_list->delay = atoi(buf + 6);
        }
        else if (!strncmp("RESIZE ", buf, 7))
        {
            caca_free_canvas(screen_list->cv);
            /* FIXME check the length before calling atoi */
            screen_list->cv =
                caca_create_canvas(atoi(buf + 7), atoi(buf + 18));
            screen_list->changed = 1;
            refresh = 1;
        }
        else if (!strncmp("KEY ", buf, 4))
        {
            unsigned int c = atoi(buf + 4);
            refresh |= handle_key(screen_list, c, refresh);
        }
        else if (!strncmp("MOUSEP ", buf, 6))
        {
            debug("Got mouse press '%s'\n", buf);
            int x, y, b;
            x = 1 + atoi(buf + 7) - screen_list->screen[screen_list->pty]->x;
            y = 1 + atoi(buf + 18) - screen_list->screen[screen_list->pty]->y;
            b = atoi(buf + 28);

            switch (screen_list->screen[screen_list->pty]->report_mouse)
            {
            case MOUSE_VT200:
            case MOUSE_VT200_HIGHLIGHT:
            case MOUSE_BTN_EVENT:
            case MOUSE_ANY_EVENT:
                sprintf(buf, "\x1b[M%c%c%c", 32 + (b - 1), x + 32, y + 32);
                debug("mousea send ESC[M %d %d %d", (b - 1), x + 32, y + 32);
                send_ansi_sequence(screen_list, buf);
                break;
            case MOUSE_X10:
                sprintf(buf, "\x1b[M%c%c%c", 32 + (b - 1), 32 + x, 32 + y);
                debug("mousex send ESC[M %d %d %d", 32 + (b - 1), 32 + x,
                      32 + y);
                send_ansi_sequence(screen_list, buf);
                break;
            case MOUSE_NONE:
                break;

            }
        }
        else if (!strncmp("MOUSER ", buf, 6))
        {
            debug("Got mouse release '%s'\n", buf);
            int x, y, b;
            x = 1 + atoi(buf + 7) - screen_list->screen[screen_list->pty]->x;
            y = 1 + atoi(buf + 18) - screen_list->screen[screen_list->pty]->y;
            b = atoi(buf + 28);

            switch (screen_list->screen[screen_list->pty]->report_mouse)
            {
            case MOUSE_VT200:
            case MOUSE_VT200_HIGHLIGHT:
            case MOUSE_BTN_EVENT:
            case MOUSE_ANY_EVENT:
                sprintf(buf, "\x1b[M%c%c%c", 32 + 3, x + 32, y + 32);
                send_ansi_sequence(screen_list, buf);
                break;
            case MOUSE_X10:
                sprintf(buf, "\x1b[M%c%c%c", 32 + 3, 32 + x, 32 + y);
                send_ansi_sequence(screen_list, buf);
                break;
            case MOUSE_NONE:
                break;
            }
        }

        else if (!strncmp("MOUSEM ", buf, 6))
        {
            debug("Got mouse motion '%s'\n", buf);
            int x, y, b;
            x = 1 + atoi(buf + 7) - screen_list->screen[screen_list->pty]->x;
            y = 1 + atoi(buf + 18) - screen_list->screen[screen_list->pty]->y;
            b = atoi(buf + 28);

            switch (screen_list->screen[screen_list->pty]->report_mouse)
            {
            case MOUSE_X10:    // X10 reports mouse clicks only
                if (b)
                {
                    sprintf(buf, "\x1b[M%c%c%c", 32 + (b - 1), x + 32, y + 32);
                    send_ansi_sequence(screen_list, buf);
                }
                break;
            case MOUSE_VT200:
            case MOUSE_VT200_HIGHLIGHT:
            case MOUSE_BTN_EVENT:
                if (b)
                {
                    sprintf(buf, "\x1b[M%c%c%c", 32 + (b - 1), x + 32, y + 32);
                    send_ansi_sequence(screen_list, buf);
                }
            case MOUSE_ANY_EVENT:
                sprintf(buf, "\x1b[M%c%c%c", 32 + (b - 1), x + 32, y + 32);
                send_ansi_sequence(screen_list, buf);
                break;
            case MOUSE_NONE:
                break;
            }
        }
        else
        {
            fprintf(stderr, "Unknown command received: %s\n", buf);
        }
    }

    /* No more screens, exit */
    if (!screen_list->count)
        return -1;

    /* User requested to exit */
    if (quit)
        return -2;

    /* Update each screen canvas */
    refresh |= update_terms(screen_list);

    /* Launch recurrents if any */
    refresh |= handle_recurrents(screen_list);

    /* Refresh screen */
    refresh_screen(screen_list, refresh);

    eof = 1;
    for (i = 0; i < screen_list->count; i++)
        if (screen_list->screen[i]->fd >= 0)
            eof = 0;
    if (eof)
        return -3;

    return 0;
}

static void server_main(struct screen_list *screen_list)
{
    screen_list->last_key_time = 0;
    screen_list->comm.attached = 0;
    screen_list->command = 0;
    screen_list->was_in_bell = 0;
    screen_list->last_refresh_time = 0;

    server_init(screen_list);
#ifdef USE_PYTHON
    python_init(screen_list);
#endif

    for (;;)
    {
        if (server_iteration(screen_list))
            break;
    }

    detach(screen_list);

    free_screen_list(screen_list);

#ifdef USE_PYTHON
    python_close(screen_list);
#endif

    exit(0);
}

static void refresh_screen(struct screen_list *screen_list, int refresh)
{
    if (!screen_list->comm.attached)
    {
        /* No need to refresh Don't use the CPU too much Would be better to
           select on terms + socket */
        sleep(1);
    }
    else
    {
        long long unsigned int current_time = get_us();
        long long int tdiff =
            (current_time - screen_list->last_refresh_time) / 1000;

        refresh |= screen_list->need_refresh;

        if (screen_list->force_refresh)
        {
            wm_refresh(screen_list);
            refresh = 1;
        }

        /* Draw lock window */
        if (screen_list->lock.locked)
        {
            /* FIXME don't redraw it each iteration */
            draw_lock(screen_list);
            refresh = 1;
        }
#ifdef USE_LOCK
        else if ((current_time - screen_list->last_key_time >=
                  screen_list->lock.autolock_timeout))
        {
            screen_list->lock.locked = 1;
            refresh = 1;
        }
#endif
        else if ((current_time - screen_list->last_key_time >=
                  screen_list->screensaver.timeout))
        {
            if (!screen_list->screensaver.in_screensaver)
            {
                screensaver_init(screen_list);
                screen_list->screensaver.in_screensaver = 1;
                set_cursor(0, screen_list);
            }
            draw_screensaver(screen_list);
            refresh = 1;
        }
        else if (refresh || screen_list->was_in_bell)
        {
            if (tdiff >= screen_list->delay)
            {
                refresh_screens(screen_list);
                set_title(screen_list);
                refresh = 1;
            }
        }
        if (refresh)
        {
            if (tdiff >= screen_list->delay)
            {
                request_refresh(screen_list);
                screen_list->last_refresh_time = current_time;
                screen_list->need_refresh = 0;
            }
            else
            {
                debug("Skipping refresh (%lld < %d)", tdiff,
                      screen_list->delay);
                screen_list->need_refresh = 1;
            }
        }
    }
}

static void server_init(struct screen_list *screen_list)
{
    int i;
    debug("Screen list at %p\n", screen_list);

    /* Create socket and bind it */
    create_socket(screen_list, SOCK_SERVER);

    /* Connect to the client */
    connect_socket(screen_list, SOCK_CLIENT);

    screen_list->width = screen_list->height = 80;

    /* Create main canvas */
    screen_list->cv = caca_create_canvas(screen_list->width,
                                         screen_list->height
                                         + screen_list->modals.mini * 6
                                         + screen_list->modals.status);

    if (!screen_list->sys.to_grab && !screen_list->sys.to_start)
    {
        add_screen(screen_list,
                   create_screen(screen_list->width,
                                 screen_list->height,
                                 screen_list->sys.default_shell));
    }

    /* Attach processes */
    if (screen_list->sys.to_grab)
    {
        for (i = 0; screen_list->sys.to_grab[i]; i++)
        {
            add_screen(screen_list,
                       create_screen_grab(screen_list->width,
                                          screen_list->height,
                                          screen_list->sys.to_grab[i]));
        }
        free(screen_list->sys.to_grab);
        screen_list->sys.to_grab = NULL;
    }

    /* Launch command line processes */
    if (screen_list->sys.to_start)
    {
        for (i = 0; screen_list->sys.to_start[i]; i++)
        {
            add_screen(screen_list,
                       create_screen(screen_list->width,
                                     screen_list->height,
                                     screen_list->sys.to_start[i]));
            free(screen_list->sys.to_start[i]);
        }
        free(screen_list->sys.to_start);
        screen_list->sys.to_start = NULL;
    }

    screen_list->last_key_time = get_us();
}

static int handle_attach(struct screen_list *screen_list, char *buf)
{
    /* If we were attached to someone else, detach first */
    if (screen_list->comm.attached)
        detach(screen_list);
    screen_list->comm.attached = 1;
    caca_free_canvas(screen_list->cv);
    screen_list->cv = caca_create_canvas(atoi(buf + 7), atoi(buf + 18));
    screen_list->delay = atoi(buf + 29);
    screen_list->changed = 1;
    return 1;
}

static int handle_key(struct screen_list *screen_list, unsigned int c,
                      int refresh)
{
    char *str = NULL;
    int size = 0;

    if (screen_list->modals.help)
    {
        return help_handle_key(screen_list, c);
    }
#ifdef USE_PYTHON
    if (screen_list->modals.python_command)
    {
        return python_command_handle_key(screen_list, c);
    }
#endif

    /* CTRL-A has been pressed before, handle this as a command, except that
       CTRL-A a sends literal CTRL-A */
    if (screen_list->command && (c != 'a'))
    {
        screen_list->command = 0;
        refresh |= handle_command_input(screen_list, c);
    }
    else
    {
        /* Not in command mode */
        screen_list->last_key_time = get_us();
        set_cursor(1, screen_list);

        /* Kill screensaver */
        if (screen_list->screensaver.in_screensaver)
        {
            screensaver_kill(screen_list);
            screen_list->screensaver.in_screensaver = 0;
            screen_list->changed = 1;
            refresh = 1;
            return refresh;
        }
        /* Handle lock window */
        if (screen_list->lock.locked)
        {
            refresh |= update_lock(c, screen_list);
            screen_list->changed = 1;
        }
        else if (screen_list->modals.window_list)
        {
            refresh |= update_window_list(c, screen_list);
            screen_list->changed = 1;
        }
        else
        {
            switch (c)
            {
            case 0x01:         // CACA_KEY_CTRL_A:
                screen_list->command = 1;
                break;
            default:
                /* CTRL-A a sends literal CTRL-A */
                if (screen_list->command && (c == 'a'))
                {
                    c = 0x01;
                }
                /* Normal key, convert it if needed */
                str = convert_input_ansi(&c, &size);
                /* FIXME check value of r */
                int r = write(screen_list->screen[screen_list->pty]->fd, str,
                              size);
                (void)r;
                break;
            }
        }
    }
    return refresh;
}

int send_ansi_sequence(struct screen_list *screen_list, char *str)
{
    debug("Sending ansi '%s'\n", str);
    return write(screen_list->screen[screen_list->pty]->fd, str, strlen(str));
}


int install_fds(struct screen_list *screen_list)
{
    int fd;
    close(0);
    close(1);
    close(2);
    fd = open("/dev/null", O_RDWR, 0);
    if (fd < 0)
    {
        perror("Failed to open /dev/null");
        return -2;
    }
    dup2(fd, 0);
#ifndef DEBUG
    dup2(fd, 1);
    dup2(fd, 2);
    if (fd > 2)
        close(fd);
#else
    if (fd != 0)
        close(fd);
    screen_list->outfd =
        open("/tmp/neercs-debug.txt", O_TRUNC | O_RDWR | O_CREAT,
             S_IRUSR | S_IWUSR);
    dup2(screen_list->outfd, 1);
    dup2(screen_list->outfd, 2);
    if (screen_list->outfd > 2)
        close(screen_list->outfd);
#endif
    return 0;
}

int start_server(struct screen_list *screen_list)
{
    pid_t pid;

    pid = fork();
    if (pid < 0)
    {
        perror("Failed to create child process");
        return -1;
    }
    if (pid == 0)
    {
        int r = install_fds(screen_list);
        if (r)
            return r;
        setsid();

        server_main(screen_list);
        /* Never returns */
    }

    return 0;
}

long long get_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * (1000000) + tv.tv_usec);
}

#endif
