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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "neercs.h"


struct config_line *get_config(const char *name);
int set_window_manager(const char *argv, struct screen_list *screen_list);
int set_cube_duration(const char *argv, struct screen_list *screen_list);
int set_thumbnails(const char *argv, struct screen_list *screen_list);
int set_status_bar(const char *argv, struct screen_list *screen_list);
int set_screensaver_timeout(const char *argv, struct screen_list *screen_list);
int set_autolock_timeout(const char *argv, struct screen_list *screen_list);
int set_lock_on_detach(const char *argv, struct screen_list *screen_list);
int set_socket_dir(const char *argv, struct screen_list *screen_list);
int set_delay(const char *argv, struct screen_list *screen_list);
int set_eyecandy(const char *argv, struct screen_list *screen_list);
int set_border(const char *argv, struct screen_list *screen_list);
char *get_window_manager(struct screen_list *screen_list);
char *get_cube_duration(struct screen_list *screen_list);
char *get_thumbnails(struct screen_list *screen_list);
char *get_status_bar(struct screen_list *screen_list);
char *get_screensaver_timeout(struct screen_list *screen_list);
char *get_autolock_timeout(struct screen_list *screen_list);
char *get_lock_on_detach(struct screen_list *screen_list);
char *get_socket_dir(struct screen_list *screen_list);
char *get_delay(struct screen_list *screen_list);
char *get_eyecandy(struct screen_list *screen_list);
char *get_border(struct screen_list *screen_list);

/* Options definition and associated function pointer */
struct config_line config_option[] = {
    {.name = "window_manager",.set = set_window_manager,.get =
     get_window_manager},
    {.name = "eyecandy",.set = set_eyecandy,.get = get_eyecandy},
    {.name = "borders",.set = set_border,.get = get_border},
    {.name = "cube_duration",.set = set_cube_duration,.get =
     get_cube_duration},
    {.name = "thumbnails",.set = set_thumbnails,.get = get_thumbnails},
    {.name = "status_bar",.set = set_status_bar,.get = get_status_bar},
    {.name = "screensaver_timeout",.set = set_screensaver_timeout,.get =
     get_screensaver_timeout},
    {.name = "autolock_timeout",.set = set_autolock_timeout,.get =
     get_autolock_timeout},
    {.name = "lock_on_detach",.set = set_lock_on_detach,.get =
     get_lock_on_detach},
    {.name = "socket_dir",.set = set_socket_dir,.get = get_socket_dir},
    {.name = "delay",.set = set_delay,.get = get_delay},

    {.name = "last",.set = NULL},
};



int read_configuration_file(char *filename, struct screen_list *screen_list)
{
    FILE *fp;
    struct stat st;
    int size = 0, i = 0, total = 0, offset = 0, l = 1;
    char *buffer = NULL;

    screen_list->config = NULL;

    /* Check if file exist */
    if (stat(filename, &st) < 0)
    {
        return -1;
    }
    /* Get its size */
    size = st.st_size;
    if (!size)
    {
        fprintf(stderr, "File too short\n");
        return -1;
    }

    /* Open it */
    fp = fopen(filename, "r");
    if (!fp)
    {
        return -1;
    }

    buffer = malloc(size + 1);
    if (!buffer)
    {
        fclose(fp);
        fprintf(stderr, "Can't allocate memory at %s:%d\n", __FUNCTION__,
                __LINE__);
        return -1;
    }
    /* Read it */
    while ((i = fread(buffer + total, 1, size, fp)) > 0)
    {
        total += i;
    }
    buffer[total] = '\n';

    fclose(fp);

    /* Parse it */
    while ((i =
            parse_conf_line(buffer + offset, total - offset, screen_list)) > 0)
    {
        offset += i;
        l++;
    }

    free(buffer);

    /* Fill neercs configuration with it */
    fill_config(screen_list);

    return 1;
}

struct config_line *get_config_option(void)
{
    return config_option;
}

int parse_conf_line(char *buf, int size, struct screen_list *screen_list)
{
    int i, s = 0, c = 0;
    char *line = NULL;
    int l = 0;
    int in_quote = 0, end_spaces = 0;
    static struct option_t *prev = NULL;

    if (size <= 0)
        return -1;

    /* Find EOL */
    for (i = 0; i < size; i++)
    {
        if (buf[i] == '\n')
        {
            s = i + 1;
            break;
        }
    }

    /* Strip comments and trailing spaces */
    for (i = 0; i < s; i++)
    {
        if (buf[i] == ';' && !in_quote)
        {
            break;
        }
        else if (buf[i] == '\n')
        {
            break;
        }
        else if (buf[i] == ' ' && !c)
        {
        }
        else
        {
            if (line == NULL)
            {
                line = malloc(2);
                if (!line)
                {
                    fprintf(stderr, "Can't allocate memory at %s:%d\n",
                            __FUNCTION__, __LINE__);
                    return -1;
                }
            }
            else
            {
                line = realloc(line, l + 2);
                if (!line)
                {
                    fprintf(stderr, "Can't allocate memory at %s:%d\n",
                            __FUNCTION__, __LINE__);
                    return -1;
                }
            }
            if (buf[i] == '"')
                in_quote = !in_quote;
            if (buf[i] == ' ')
                end_spaces++;
            else
                end_spaces = 0;

            line[l] = buf[i];
            line[l + 1] = 0;
            l++;
            c = 1;
        }
    }

    if (c == 0)
    {
        /* This line is empty, do nothing */
    }
    else
    {
        struct option_t *option = malloc(sizeof(struct option_t));
        if (!option)
        {
            fprintf(stderr, "Can't allocate memory at %s:%d\n",
                    __FUNCTION__, __LINE__);
            return -1;
        }
        option->next = NULL;
        l -= end_spaces;
        line = realloc(line, l + 1);
        line[l] = 0;

        get_key_value(line, option);

        if (!screen_list->config)
            screen_list->config = option;

        if (prev)
            prev->next = option;

        prev = option;
    }
    free(line);
    return s;
}

int get_key_value(char *line, struct option_t *option)
{
    unsigned int i, o = 0, b = 0, end_spaces = 0;
    char *cur = NULL;
    option->value = NULL;
    option->key = NULL;

    /* Line is a section delimiter */
    if (line[0] == '[')
    {
        option->value = malloc(strlen(line) - 1);
        if (!option->value)
        {
            fprintf(stderr, "Can't allocate memory at %s:%d\n",
                    __FUNCTION__, __LINE__);
            return -1;
        }
        memcpy(option->value, line + 1, strlen(line) - 1);
        option->value[strlen(line) - 2] = 0;
        return 0;
    }

    cur = malloc(1);
    if (!cur)
    {
        fprintf(stderr, "Can't allocate memory at %s:%d\n",
                __FUNCTION__, __LINE__);
        return -1;
    }
    cur[0] = 0;

    for (i = 0; i < strlen(line); i++)
    {
        if (line[i] == ' ' && !b)
            continue;


        if (line[i] == '=')
        {
            b = 0;
            cur[o - end_spaces] = 0;
            cur = realloc(cur, (o - end_spaces) + 1);
            if (!cur)
            {
                fprintf(stderr, "Can't allocate memory at %s:%d\n",
                        __FUNCTION__, __LINE__);
                return -1;
            }
            o = 0;
            option->key = cur;
            cur = malloc(1);
        }
        else
        {
            if (line[i] == ' ')
                end_spaces++;
            else
                end_spaces = 0;

            cur = realloc(cur, o + 2);
            if (!cur)
            {
                fprintf(stderr, "Can't allocate memory at %s:%d\n",
                        __FUNCTION__, __LINE__);
                return -1;
            }
            cur[o] = line[i];
            o++;
            b = 1;

        }
    }
    cur[o] = 0;
    option->value = cur;
    return 0;
}



struct config_line *get_config(const char *name)
{
    int i = 0;

    debug("Looking for '%s'\n", name);

    while (strncmp(config_option[i].name, "last", strlen("last")))
    {
        debug("%d Testing against '%s'\n", i, config_option[i].name);
        if (!strncmp(name, config_option[i].name, strlen(name)))
        {
            debug("Found\n");
            return &config_option[i];
        }
        i++;
    }
    return NULL;
}



int fill_config(struct screen_list *screen_list)
{
    int i = 0;
    struct option_t *option = screen_list->config;

    while (option)
    {
        if (option->key == NULL)
        {
            option = option->next;
            continue;
        }

        struct config_line *c = get_config(option->key);
        if (c)
        {
            c->set((const char *)option->value, screen_list);
        }
        option = option->next;
    }

    return i;
}



/*
 * Options setters
 */

#define IS_OPTION(t) (!strncmp(argv, t, strlen(argv)))
#define IS_OPTION_TRUE (IS_OPTION("true") || IS_OPTION("True") || IS_OPTION("1"))

int set_window_manager(const char *argv, struct screen_list *screen_list)
{
    if (IS_OPTION("full"))
        screen_list->wm_type = WM_FULL;
    else if (IS_OPTION("hsplit"))
        screen_list->wm_type = WM_HSPLIT;
    else if (IS_OPTION("vsplit"))
        screen_list->wm_type = WM_VSPLIT;
    else if (IS_OPTION("card"))
        screen_list->wm_type = WM_CARD;
    else
    {
        fprintf(stderr, "Unknown window manager '%s'\n", argv);
        return -1;
    }
    return 0;
}

int set_cube_duration(const char *argv, struct screen_list *screen_list)
{
    screen_list->cube.duration = atoi(argv) * 1000000;
    return 0;
}

int set_thumbnails(const char *argv, struct screen_list *screen_list)
{
    if (IS_OPTION_TRUE)
        screen_list->modals.mini = 1;
    else
        screen_list->modals.mini = 0;
    return 0;

}

int set_status_bar(const char *argv, struct screen_list *screen_list)
{
    if (IS_OPTION_TRUE)
        screen_list->modals.status = 1;
    else
        screen_list->modals.status = 0;
    return 0;
}

int set_screensaver_timeout(const char *argv, struct screen_list *screen_list)
{
    screen_list->screensaver.timeout = atoi(argv) * 1000000;
    /* if timeout is 0, set it to 0xFFFFFFFFFFFFFFFF */
    if (!screen_list->screensaver.timeout)
        screen_list->screensaver.timeout -= 1;
    return 0;
}

int set_autolock_timeout(const char *argv, struct screen_list *screen_list)
{
    if (screen_list->lock.autolock_timeout == 0 ||
        screen_list->lock.autolock_timeout == ((long long unsigned int)0) - 1)
    {
        screen_list->lock.autolock_timeout = atoi(argv) * 1000000;
        /* if timeout is 0, set it to 0xFFFFFFFFFFFFFFFF */
        if (!screen_list->lock.autolock_timeout)
            screen_list->lock.autolock_timeout -= 1;
    }
    return 0;
}

int set_lock_on_detach(const char *argv, struct screen_list *screen_list)
{
    if (IS_OPTION_TRUE)
        screen_list->lock.lock_on_detach = 1;
    else
        screen_list->lock.lock_on_detach = 0;
    return 0;
}

int set_eyecandy(const char *argv, struct screen_list *screen_list)
{
    if (IS_OPTION_TRUE)
        screen_list->eyecandy = 1;
    else
        screen_list->eyecandy = 0;
    return 0;
}

int set_border(const char *argv, struct screen_list *screen_list)
{
    if (IS_OPTION_TRUE)
        screen_list->border_size = 1;
    else
        screen_list->border_size = 0;
    return 0;
}

int set_socket_dir(const char *argv, struct screen_list *screen_list)
{
    screen_list->comm.socket_dir = strdup(argv);
    return 0;
}

int set_delay(const char *argv, struct screen_list *screen_list)
{
    screen_list->requested_delay = atoi(argv);
    screen_list->delay = atoi(argv);
    return 0;
}

char *get_window_manager(struct screen_list *screen_list)
{
    debug("Window manager is %d", screen_list->wm_type);
    switch (screen_list->wm_type)
    {
    case WM_FULL:
        return "full";
    case WM_CARD:
        return "card";
    case WM_VSPLIT:
        return "vsplit";
    case WM_HSPLIT:
        return "hsplit";
    default:
        return "invalid window manager";
    }
    return NULL;                /* Not reached */
}

char *get_cube_duration(struct screen_list *screen_list)
{
    char *r = malloc(100);
    sprintf(r, "%f", (float)screen_list->cube.duration / 1000000.0f);
    return r;
}

char *get_thumbnails(struct screen_list *screen_list)
{
    if (screen_list->modals.mini)
        return "true";
    return "false";
}

char *get_status_bar(struct screen_list *screen_list)
{
    if (screen_list->modals.status)
        return "true";
    return "false";
}

char *get_eyecandy(struct screen_list *screen_list)
{
    if (screen_list->eyecandy)
        return "true";
    return "false";
}

char *get_border(struct screen_list *screen_list)
{
    if (screen_list->border_size)
        return "true";
    return "false";
}

char *get_screensaver_timeout(struct screen_list *screen_list)
{
    char *r = malloc(100);
    sprintf(r, "%f", (float)screen_list->screensaver.timeout / 1000000.0f);
    return r;
}

char *get_autolock_timeout(struct screen_list *screen_list)
{
    char *r = malloc(100);
    sprintf(r, "%f", (float)screen_list->lock.autolock_timeout / 1000000.0f);
    return r;
}

char *get_lock_on_detach(struct screen_list *screen_list)
{
    if (screen_list->lock.lock_on_detach)
        return "true";
    else
        return "false";
}

char *get_socket_dir(struct screen_list *screen_list)
{
    return screen_list->comm.socket_dir;
}

char *get_delay(struct screen_list *screen_list)
{
    char *r = malloc(100);
    sprintf(r, "%d", screen_list->requested_delay);
    return r;
}

#endif

