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
 *  http://www.wtfpl.net/ for more details.
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

#if !defined HAVE_GETOPT_LONG
#   include "mygetopt.h"
#elif defined HAVE_GETOPT_H
#   include <getopt.h>
#endif
#if defined HAVE_GETOPT_LONG
#   define mygetopt getopt_long
#   define myoptind optind
#   define myoptarg optarg
#   define myoption option
#endif
#include <errno.h>
#include <caca.h>

#include "neercs.h"


void version(void)
{
    printf("%s\n", PACKAGE_STRING);
    printf("Copyright (C) 2006, 2008 Sam Hocevar <sam@zoy.org>\n");
    printf
        ("                         Jean-Yves Lamoureux <jylam@lnxscene.org>\n\n");
    printf
        ("This is free software.  You may redistribute copies of it under the\n");
    printf
        ("terms of the Do What The Fuck You Want To Public License, Version 2\n");
    printf("<http://www.wtfpl.net/>.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n");
    printf("\n");
    printf
        ("For more informations, visit http://libcaca.zoy.org/wiki/neercs\n");
}

void usage(int argc, char **argv)
{
    printf("%s\n", PACKAGE_STRING);
    printf("Usage : %s [command1] [command2] ... [commandN]\n", argv[0]);
    printf("Example : %s zsh top \n\n", argv[0]);
    printf("Options :\n");
    printf("\t--config\t-c <file>\t\tuse given config file\n");
    printf("\t--pid\t\t-P [pid]\t\tgrab process\n");
    printf("\t\t\t-r [session]\t\treattach to a detached neercs\n");
    printf
        ("\t\t\t-R [session]\t\treattach if possible, otherwise start a new session\n");
    printf("\t\t\t-S <name>\t\tname this session <name> instead of <pid>\n");
    printf("\t--lock-after\t-l [n]\t\t\tlock screen after n seconds\n");
    printf("\t--version\t-v \t\t\tdisplay version and exit\n");
    printf("\t--help\t\t-h \t\t\tthis help\n");
}

struct screen_list *init_neercs(int argc, char **argv)
{
    struct screen_list *screen_list = NULL;
    int args;

    int mainret = -1;

    screen_list = create_screen_list();
    screen_list->sys.default_shell = getenv("SHELL");

    args = argc - 1;
    if (screen_list->sys.default_shell == NULL && args <= 0)
    {
        fprintf(stderr,
                "Environment variable SHELL not set and no arguments given. kthxbye.\n");
        free_screen_list(screen_list);
        return NULL;
    }

    if (handle_command_line(argc, argv, screen_list) < 0)
    {
        free_screen_list(screen_list);
        return NULL;
    }

    /* Read global configuration first */
    read_configuration_file("/etc/neercsrc", screen_list);

    /* Then local one */
    if (screen_list->sys.user_path)
    {
        read_configuration_file(screen_list->sys.user_path, screen_list);
        free(screen_list->sys.user_path);
    }

    if (screen_list->sys.attach)
    {
        if (screen_list->sys.nb_to_grab || screen_list->sys.to_start)
        {
            fprintf(stderr,
                    "-R can not be associated with commands or pids!\n");
            free_screen_list(screen_list);
            return NULL;
        }

        attach(screen_list);

        if (screen_list->sys.forceattach && !screen_list->sys.attach)
        {
            free_screen_list(screen_list);
            return NULL;
        }
    }

    /* Build default session name */
    if (!screen_list->comm.session_name)
    {
        char mypid[32];         /* FIXME Compute the length of PID_MAX ? */
        snprintf(mypid, 31, "%d", getpid());
        mypid[31] = '\0';
        screen_list->comm.session_name = strdup(mypid);
        if (!screen_list->comm.session_name)
        {
            fprintf(stderr, "Can't allocate memory at %s:%d\n", __FUNCTION__,
                    __LINE__);
            free_screen_list(screen_list);
            return NULL;
        }
    }
    if (!screen_list->comm.socket_path[SOCK_CLIENT])
        screen_list->comm.socket_path[SOCK_CLIENT] =
            build_socket_path(screen_list->comm.socket_dir,
                              screen_list->comm.session_name, SOCK_CLIENT);

    if (!screen_list->comm.socket_path[SOCK_SERVER])
        screen_list->comm.socket_path[SOCK_SERVER] =
            build_socket_path(screen_list->comm.socket_dir,
                              screen_list->comm.session_name, SOCK_SERVER);

    /* Fork the server if needed */
    if (!screen_list->sys.attach)
    {
        debug("Spawning a new server");
        if (start_server(screen_list))
        {
            free_screen_list(screen_list);
            return NULL;
        }
        if (start_client(screen_list))
        {
            free_screen_list(screen_list);
            return NULL;
        }
    }

    return screen_list;
}

int handle_command_line(int argc, char *argv[],
                        struct screen_list *screen_list)
{
    int s = 0, i;
    for (;;)
    {
        int option_index = 0;
        int pidopt;
        static struct myoption long_options[] = {
            {"config", 1, NULL, 'c'},
#if defined USE_GRAB
            {"pid", 0, NULL, 'P'},
#endif
            {"lock-after", 1, NULL, 'l'},
            {"help", 0, NULL, 'h'},
            {"version", 0, NULL, 'v'},
            {NULL, 0, NULL, 0},
        };
#if defined USE_GRAB
        int c =
            mygetopt(argc, argv, "c:S:R::l::r::P::hv", long_options,
                     &option_index);
#else
        int c =
            mygetopt(argc, argv, "c:S:R::l::r::hv", long_options,
                     &option_index);
#endif
        if (c == -1)
            break;

        switch (c)
        {
        case 'c':              /* --config */
            if (screen_list->sys.user_path)
                free(screen_list->sys.user_path);
            screen_list->sys.user_path = strdup(myoptarg);
            s += 2;
            break;
        case 'S':
            if (!screen_list->comm.session_name)
                screen_list->comm.session_name = strdup(myoptarg);
            s += 2;
            break;
        case 'P':              /* --pid */
            if (myoptarg)
            {
                pidopt = atoi(myoptarg);
                if (pidopt <= 0)
                {
                    fprintf(stderr, "Invalid pid %d\n", pidopt);
                    if (screen_list->sys.to_grab)
                        free(screen_list->sys.to_grab);
                    return -1;
                }
            }
            else
                pidopt = select_process(screen_list);
            if (pidopt <= 0)
            {
                s += 1;
                break;
            }
            if (!screen_list->sys.to_grab)
            {
                /* At most argc-1-s times -P <pid> + final 0 */
                screen_list->sys.to_grab =
                    (int *)malloc(((argc - 1 - s) / 2 + 1) * sizeof(int));
                if (!screen_list->sys.to_grab)
                {
                    fprintf(stderr, "Can't allocate memory at %s:%d\n",
                            __FUNCTION__, __LINE__);
                    return -1;
                }
            }
            screen_list->sys.to_grab[screen_list->sys.nb_to_grab++] = pidopt;
            screen_list->sys.to_grab[screen_list->sys.nb_to_grab] = 0;
            s += 2;
            break;
        case 'l':
            screen_list->lock.autolock_timeout = atoi(myoptarg) * 1000000;
            if (screen_list->lock.autolock_timeout == 0)
                screen_list->lock.autolock_timeout -= 1;
            break;
        case 'r':
            screen_list->sys.forceattach = 1;
        case 'R':
            if (screen_list->sys.attach)
            {
                fprintf(stderr, "Attaching can only be requested once\n");
                return -1;
            }
            if (myoptarg)
            {
                if (screen_list->comm.session_name)
                    free(screen_list->comm.session_name);
                screen_list->comm.session_name = strdup(myoptarg);
                s += 1;
            }
            screen_list->sys.attach = 1;
            s += 1;
            break;
        case 'h':              /* --help */
            usage(argc, argv);
            return -1;
            break;
        case 'v':              /* --version */
            version();
            return -1;
            break;
        case -2:
            return -1;
        default:
            fprintf(stderr, "Unknown argument #%d\n", myoptind);
            return -1;
            break;
        }
    }
    if (s >= 0 && s < argc - 1)
    {
        screen_list->sys.to_start = (char **)malloc((argc - s) * sizeof(char *));
        if (!screen_list->sys.to_start)
        {
            fprintf(stderr, "Can't allocate memory at %s:%d\n", __FUNCTION__,
                    __LINE__);
            return -1;
        }
        for (i = 0; i < (argc - 1) - s; i++)
        {
            screen_list->sys.to_start[i] = strdup(argv[i + s + 1]);
        }
        screen_list->sys.to_start[argc - 1 - s] = NULL;
    }
    return s;
}

#endif

