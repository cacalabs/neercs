/*
 *  neercs        console-based window manager
 *  Copyright (c) 2008-2010 Pascal Terjan <pterjan@linuxfr.org>
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

#define _XOPEN_SOURCE 500       /* getsid() */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#if defined HAVE_LINUX_KDEV_T_H
#   include <linux/kdev_t.h>
#   include <linux/major.h>
#endif

#include "neercs.h"
#include "mytrace.h"

int grab_process(long pid, char *ptyname, int ptyfd, int *newpid)
{
#if defined HAVE_LINUX_KDEV_T_H
    char fdstr[1024];
    struct mytrace *parent, *child;
    int i = 0, fd = 0, ret;
    char to_open[128];
    int mode[128];
    int fds[128];
    struct stat stat_buf;
    struct termios tos;
    int validtos = 0;
    DIR *fddir;
    struct dirent *fddirent;

    debug("pty is %s", ptyname);

    parent = mytrace_attach(pid);
    if (!parent)
    {
        fprintf(stderr, "Cannot access process %ld\n", pid);
        return -1;
    }

    child = mytrace_fork(parent);

    snprintf(fdstr, sizeof(fdstr), "/proc/%ld/fd", pid);
    fddir = opendir(fdstr);

    /* Look for file descriptors that are PTYs */
    while ((fddirent = readdir(fddir)) && i < (int)sizeof(to_open) - 1)
    {
        fd = atoi(fddirent->d_name);
        fds[i] = fd;
        to_open[i] = 0;
        lstat(fdstr, &stat_buf);
        if ((stat_buf.st_mode & S_IRUSR) && (stat_buf.st_mode & S_IWUSR))
            mode[i] = O_RDWR;
        else if (stat_buf.st_mode & S_IWUSR)
            mode[i] = O_WRONLY;
        else
            mode[i] = O_RDONLY;

        snprintf(fdstr, sizeof(fdstr), "/proc/%ld/fd/%s", pid,
                 fddirent->d_name);

        if (stat(fdstr, &stat_buf) < 0)
            continue;

        if (!S_ISCHR(stat_buf.st_mode)
            || MAJOR(stat_buf.st_rdev) != UNIX98_PTY_SLAVE_MAJOR)
            continue;

        debug("found pty %d for pid %d", fd, pid);

        if (!validtos)
        {
            ret = mytrace_tcgets(child, fd, &tos);
            if (ret < 0)
            {
                perror("mytrace_tcgets");
            }
            else
            {
                validtos = 1;
            }
        }
        to_open[i] = 1;
        i++;
    }
    closedir(fddir);

    if (i >= (int)sizeof(to_open) - 1)
    {
        fprintf(stderr, "too many open pty\n");
        mytrace_detach(child);
        return -1;
    }

    ret = mytrace_exec(parent, "/usr/bin/reset");
    if (ret < 0)
        mytrace_exit(parent, 0);
    mytrace_detach(parent);
    waitpid(pid, NULL, 0);      /* Wait for reset to finish before displaying */
    mytrace_write(child, 2, "\033[H\033[2J", 7);
    mytrace_write(child, 2, "\n[Process stolen by neercs]\r\n\n", 30);

    pid = mytrace_getpid(child);
    *newpid = pid;

    /* Set the process's session ID */
    debug("Running setsid on process %ld (sid=%d)", pid, getsid(pid));

    ret = mytrace_setpgid(child, 0, getsid(pid));
    if (ret < 0)
    {
        fprintf(stderr, "syscall setpgid failed\n");
        mytrace_detach(child);
        return -1;
    }

    if (ret != 0)
    {
        fprintf(stderr, "setpgid returned %d\n", ret);
        mytrace_detach(child);
        return -1;
    }

    ret = mytrace_setsid(child);
    if (ret < 0)
    {
        fprintf(stderr, "syscall setsid failed\n");
        mytrace_detach(child);
        return -1;
    }

    debug("pid %ld has now sid %d", pid, getsid(pid));

    /* Reopen PTY file descriptors */
    for (; i >= 0; i--)
    {
        if (!to_open[i])
            continue;
        ret = mytrace_close(child, fds[i]);
        if (ret < 0)
        {
            perror("mytrace_close");
            continue;
        }
        fd = mytrace_open(child, ptyname, mode[i]);
        if (fd < 0)
        {
            perror("mytrace_open");
            continue;
        }

        /* FIXME Only needed once */
        mytrace_sctty(child, fd);

        if (validtos)
        {
            ret = mytrace_tcsets(child, fd, &tos);
            if (ret < 0)
            {
                perror("mytrace_tcsets");
            }
            validtos = 0;
        }
        ret = mytrace_dup2(child, fd, fds[i]);
        if (ret < 0)
        {
            perror("mytrace_dup2");
        }
    }

    kill(pid, SIGWINCH);
    mytrace_detach(child);

    close(ptyfd);
    return 0;
#else
    errno = ENOSYS;
    return -1;
#endif
}

struct process
{
    long pid;
    char *cmdline;
};

static int list_process(struct process **process_list)
{
    glob_t pglob;
    unsigned int i, n = 0;
    glob("/proc/[0-9]*", GLOB_NOSORT, NULL, &pglob);
    *process_list = malloc(pglob.gl_pathc * sizeof(struct process));
    for (i = 0; i < pglob.gl_pathc; i++)
    {
        glob_t pglob2;
        unsigned int j;
        char *fds;
        (*process_list)[n].pid = atoi(basename(pglob.gl_pathv[i]));
        /* Don't allow grabbing ourselves */
        if ((*process_list)[n].pid == getpid())
            continue;
        /* FIXME check value of r */
        int r = asprintf(&fds, "%s/fd/*", pglob.gl_pathv[i]);
        (void) r;
        glob(fds, GLOB_NOSORT, NULL, &pglob2);
        free(fds);
        for (j = 0; j < pglob2.gl_pathc; j++)
        {
            char path[4096];
            ssize_t l = readlink(pglob2.gl_pathv[j], path, sizeof(path));
            if (l <= 0)
                continue;
            path[l] = '\0';
            if (strstr(path, "/dev/pt"))
            {
                char *cmdfile;
                int fd;
                /* FIXME check value of r */
                r = asprintf(&cmdfile, "%s/cmdline", pglob.gl_pathv[i]);
                (void) r;
                fd = open(cmdfile, O_RDONLY);
                free(cmdfile);
                if (fd)
                {
                    /* FIXME check value of r */
                    r = read(fd, path, sizeof(path));
                    (void) r;
                    (*process_list)[n].cmdline = strdup(path);
                    close(fd);
                    n++;
                    break;
                }
            }
        }
        globfree(&pglob2);
    }
    globfree(&pglob);
    return n;
}

long select_process(struct screen_list *screen_list)
{
    caca_event_t ev;
    enum caca_event_type t;
    int current_line = 1;
    int refresh = 1;
    int nb_process, i;
    int ret = 0;
    int start = 0;
    struct process *process_list;

    nb_process = list_process(&process_list);

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
                          caca_get_canvas_height(screen_list->cv), '#');
            caca_set_color_ansi(screen_list->cv, CACA_DEFAULT, CACA_BLUE);
            caca_draw_cp437_box(screen_list->cv,
                                0, 0,
                                caca_get_canvas_width(screen_list->cv),
                                caca_get_canvas_height(screen_list->cv));
            caca_printf(screen_list->cv, 2, 2,
                        "Please select a process to grab:");
            for (i = 0;
                 i < nb_process
                 && i < caca_get_canvas_height(screen_list->cv) - 4; i++)
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
                    caca_set_color_ansi(screen_list->cv, CACA_LIGHTGRAY,
                                        CACA_BLUE);
                    caca_put_char(screen_list->cv, 1, i + 3, ' ');
                }
                caca_printf(screen_list->cv,
                            3, i + 3,
                            "%5d %s",
                            process_list[i + start].pid,
                            process_list[i + start].cmdline);
            }
            caca_refresh_display(screen_list->dp);
            refresh = 0;
        }

        if (!caca_get_event(screen_list->dp,
                            CACA_EVENT_KEY_PRESS
                            | CACA_EVENT_RESIZE | CACA_EVENT_QUIT, &ev, 10000))
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
                if (current_line < start && start > 0)
                    start--;
                break;
            case CACA_KEY_DOWN:
                if (current_line < nb_process)
                    current_line++;
                if (current_line >
                    start + caca_get_canvas_height(screen_list->cv) - 3)
                    start++;
                break;
            case CACA_KEY_RETURN:
                ret = process_list[current_line - 1].pid;
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

  end:
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
    if (nb_process > 0)
    {
        for (i = 0; i < nb_process; i++)
        {
            free(process_list[i].cmdline);
        }
        free(process_list);
    }
    return ret;
}

#endif

