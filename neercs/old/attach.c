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

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <caca.h>

#include "config.h"
#include "neercs.h"

char *build_socket_path(char *socket_dir, char *session_name,
                        enum socket_type socktype)
{
    char *path, *dir;
    path = (char *)malloc(PATH_MAX + 1);
    dir = socket_dir;
    if (!dir)
        dir = getenv("NEERCSDIR");
    if (!dir)
        dir = getenv("TMPDIR");
    if (!dir)
        dir = "/tmp";
    if (path)
        snprintf(path, PATH_MAX + 1, "%s/neercs.%s%s.sock", dir, session_name,
                 socktype ? "" : ".srv");
    return path;
}

static char *socket_to_session(char const *sockpath)
{
    char *p, *s;
    p = strrchr(sockpath, '/');
    if (!p)
    {
        debug("Invalid socket path %s", sockpath);
        return NULL;
    }
    p += 8;                     /* skip neercs. */
    s = strdup(p);
    p = strrchr(s, '.');
    *p = '\0';                  /* drop .sock */
    p = strrchr(s, '.');
    *p = '\0';                  /* drop .srv */
    p = strdup(s);
    free(s);
    return p;
}

int create_socket(struct screen_list *screen_list, enum socket_type socktype)
{
    int sock;
    struct sockaddr_un myaddr;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("create_socket:socket");
        return errno;
    }

    memset(&myaddr, 0, sizeof(struct sockaddr_un));

    myaddr.sun_family = AF_UNIX;
    strncpy(myaddr.sun_path, screen_list->comm.socket_path[socktype],
            sizeof(myaddr.sun_path) - 1);

    unlink(screen_list->comm.socket_path[socktype]);

    if (bind(sock, (struct sockaddr *)&myaddr, sizeof(struct sockaddr_un)) < 0)
    {
        free(screen_list->comm.socket_path[socktype]);
        screen_list->comm.socket_path[socktype] = NULL;
        close(sock);
        perror("create_socket:bind");
        return errno;
    }
    fcntl(sock, F_SETFL, O_NONBLOCK);

    debug("Listening on %s (%d)", screen_list->comm.socket_path[socktype], sock);

    screen_list->comm.socket[socktype] = sock;

    return 0;
}

char **list_sockets(char *socket_dir, char *session_name)
{
    char *pattern, *dir;
    glob_t globbuf;

    globbuf.gl_pathv = NULL;

    pattern = (char *)malloc(PATH_MAX + 1);

    dir = socket_dir;
    if (!dir)
        dir = getenv("NEERCSDIR");
    if (!dir)
        dir = getenv("TMPDIR");
    if (!dir)
        dir = "/tmp";

    if (!pattern)
        return globbuf.gl_pathv;

    if (session_name && strlen(session_name) + strlen(dir) + 13 < PATH_MAX)
        sprintf(pattern, "%s/neercs.%s.srv.sock", dir, session_name);
    else
        snprintf(pattern, PATH_MAX, "%s/neercs.*.srv.sock", dir);
    pattern[PATH_MAX] = '\0';

    debug("Looking for sockets in the form %s", pattern);

    glob(pattern, 0, NULL, &globbuf);

    free(pattern);

    return globbuf.gl_pathv;
}

char *connect_socket(struct screen_list *screen_list,
                     enum socket_type socktype)
{
    int sock;
    struct sockaddr_un addr;

    debug("Connecting to %s", screen_list->comm.socket_path[socktype]);

    /* Open the socket */
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        debug("Failed to create socket\n");
        perror("connect_server:socket");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, screen_list->comm.socket_path[socktype]);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        debug("Failed to connect to %s: %s",
              screen_list->comm.socket_path[socktype], strerror(errno));
        close(sock);
        return NULL;
    }
    fcntl(sock, F_SETFL, O_NONBLOCK);

    screen_list->comm.socket[socktype] = sock;

    if (socktype == SOCK_SERVER)
    {
        return socket_to_session(screen_list->comm.socket_path[socktype]);
    }
    else
        return NULL;
}

int request_attach(struct screen_list *screen_list)
{
    char buf[41];
    int bytes;

    bytes = snprintf(buf, sizeof(buf) - 1, "ATTACH %10d %10d %10d",
                     caca_get_canvas_width(screen_list->cv),
                     caca_get_canvas_height(screen_list->cv),
                     screen_list->delay);
    buf[bytes] = '\0';
    debug("Requesting attach: %s", buf);
    return write(screen_list->comm.socket[SOCK_SERVER], buf, strlen(buf)) <= 0;
}

static char *select_socket(struct screen_list *screen_list)
{
    char **sockets = NULL, **usable_sockets = NULL;
    int i, sock, nb_usable_sockets = 0;
    char *ret = NULL;

    sockets = list_sockets(screen_list->comm.socket_dir, screen_list->comm.session_name);
    if (sockets)
    {
        for (i = 0; sockets[i]; i++);

        /* Return the socket or NULL if there is not more than one match */
        if (i <= 1)
        {
            if (sockets[0])
                ret = strdup(sockets[0]);
            goto end;
        }

        /* Else ask the user to chose one */
        usable_sockets = malloc(i * sizeof(char *));
        for (i = 0; sockets[i]; i++)
        {
            struct sockaddr_un addr;
            if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
            {
                perror("select_socket:socket");
                goto end;
            }
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, sockets[i]);
            if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                switch (errno)
                {
                case EACCES:
                    debug("Connection refused on %s", sockets[i]);
                    break;
                case ECONNREFUSED:
                    fprintf(stderr, "%s is dead\n", sockets[i]);
                    break;
                default:
                    fprintf(stderr, "Unknown error on %s:%s\n", sockets[i],
                            strerror(errno));
                }
            }
            else
            {
                usable_sockets[nb_usable_sockets] = strdup(sockets[i]);
                debug("%s is usable", usable_sockets[nb_usable_sockets]);
                nb_usable_sockets++;
                close(sock);
            }
        }
        if (!nb_usable_sockets)
            goto end;
        if (nb_usable_sockets == 1)
        {
            ret = strdup(usable_sockets[0]);
            goto end;
        }
        else
        {
            caca_event_t ev;
            enum caca_event_type t;
            int current_line = 1;
            int refresh = 1;
            screen_list->cv = caca_create_canvas(0, 0);
            screen_list->dp = caca_create_display(screen_list->cv);
            if (!screen_list->dp)
                goto end;
            caca_set_cursor(screen_list->dp, 0);
            caca_set_display_title(screen_list->dp, PACKAGE_STRING);
            while (1)
            {
                if (refresh)
                {
                    caca_set_color_ansi(screen_list->cv, CACA_BLUE, CACA_BLUE);
                    caca_fill_box(screen_list->cv,
                                  0, 0,
                                  caca_get_canvas_width(screen_list->cv),
                                  caca_get_canvas_height(screen_list->cv),
                                  '#');
                    caca_set_color_ansi(screen_list->cv, CACA_DEFAULT,
                                        CACA_BLUE);
                    caca_draw_cp437_box(screen_list->cv, 0, 0,
                                        caca_get_canvas_width(screen_list->cv),
                                        caca_get_canvas_height(screen_list->
                                                               cv));
                    caca_printf(screen_list->cv, 2, 2,
                                "Please select a session to reconnect:");
                    for (i = 0; i < nb_usable_sockets; i++)
                    {
                        if (i == current_line - 1)
                        {
                            caca_set_attr(screen_list->cv, CACA_BOLD);
                            caca_set_color_ansi(screen_list->cv, CACA_GREEN,
                                                CACA_BLUE);
                            caca_put_char(screen_list->cv, 1, i + 3, '>');
                        }
                        else
                        {
                            caca_set_attr(screen_list->cv, 0);
                            caca_set_color_ansi(screen_list->cv,
                                                CACA_LIGHTGRAY, CACA_BLUE);
                            caca_put_char(screen_list->cv, 1, i + 3, ' ');
                        }
                        caca_printf(screen_list->cv,
                                    3, i + 3,
                                    "%s",
                                    socket_to_session(usable_sockets[i]));
                    }
                    caca_refresh_display(screen_list->dp);
                    refresh = 0;
                }

                if (!caca_get_event(screen_list->dp,
                                    CACA_EVENT_KEY_PRESS
                                    | CACA_EVENT_RESIZE
                                    | CACA_EVENT_QUIT, &ev, 10000))
                    continue;

                t = caca_get_event_type(&ev);

                if (t & CACA_EVENT_KEY_PRESS)
                {
                    unsigned int c = caca_get_event_key_ch(&ev);
                    switch (c)
                    {
                    case CACA_KEY_UP:
                        if (current_line > 1)
                            current_line--;
                        break;
                    case CACA_KEY_DOWN:
                        if (current_line < nb_usable_sockets)
                            current_line++;
                        break;
                    case CACA_KEY_RETURN:
                        ret = strdup(usable_sockets[current_line - 1]);
                        goto end;
                        break;
                    case CACA_KEY_ESCAPE:
                        goto end;
                        break;
                    default:
                        break;
                    }
                    refresh = 1;
                }
                else if (t & CACA_EVENT_RESIZE)
                {
                    refresh = 1;
                }
                else if (t & CACA_EVENT_QUIT)
                    goto end;
            }
        }
    }

  end:
    if (sockets)
    {
        for (i = 0; sockets[i]; i++)
            free(sockets[i]);
        free(sockets);
    }
    if (usable_sockets)
    {
        for (i = 0; i < nb_usable_sockets; i++)
            free(usable_sockets[i]);
        free(usable_sockets);
    }
    if (screen_list->dp)
    {
        caca_free_display(screen_list->dp);
        screen_list->dp = NULL;
    }
    if (screen_list->cv)
    {
        caca_free_canvas(screen_list->cv);
        screen_list->cv = NULL;
    }
    return ret;
}

void attach(struct screen_list *screen_list)
{
    screen_list->comm.socket_path[SOCK_SERVER] = select_socket(screen_list);

    if (screen_list->comm.socket_path[SOCK_SERVER])
    {
        char *session;
        session = connect_socket(screen_list, SOCK_SERVER);
        if (session)
        {
            debug("Connected to session %s", session);
            /* Create main canvas and associated caca window */
            screen_list->cv = caca_create_canvas(0, 0);
            screen_list->dp = caca_create_display(screen_list->cv);
            if (!screen_list->dp)
                return;
            caca_set_display_time(screen_list->dp, screen_list->delay * 1000);
            caca_set_cursor(screen_list->dp, 1);

            screen_list->comm.socket_path[SOCK_CLIENT] =
                build_socket_path(screen_list->comm.socket_dir, session,
                                  SOCK_CLIENT);
            create_socket(screen_list, SOCK_CLIENT);
            request_attach(screen_list);
            if (screen_list->comm.session_name)
                free(screen_list->comm.session_name);
            screen_list->comm.session_name = session;
        }
        else
        {
            fprintf(stderr, "Failed to attach!\n");
            free(screen_list->comm.socket_path[SOCK_SERVER]);
            screen_list->comm.socket_path[SOCK_SERVER] = NULL;
            screen_list->sys.attach = 0;
        }
    }
    else
    {
        fprintf(stderr, "No socket found!\n");
        screen_list->sys.attach = 0;
    }
}

