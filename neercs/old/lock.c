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
#include <stdlib.h>
#include <string.h>
#include <caca.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>

#if defined USE_LOCK
#if defined HAVE_PAM_PAM_MISC_H
#   include <pam/pam_appl.h>
#   include <pam/pam_misc.h>
#else
#   include <security/pam_appl.h>
#   include <security/pam_misc.h>
#endif
#   include <pwd.h>
#endif

#include "neercs.h"

#if defined USE_LOCK
static int convpam(int num_msg, const struct pam_message **msg,
                   struct pam_response **resp, void *appdata_ptr);
#endif

int update_lock(int c, struct screen_list *screen_list)
{
    int refresh = 0;

#if defined USE_LOCK
    if (!screen_list->lock.locked)
        return 0;

    if (c == 0x08)              // BACKSPACE
    {
        if (screen_list->lock.lock_offset)
        {
            screen_list->lock.lockpass[screen_list->lock.lock_offset - 1] = 0;
            screen_list->lock.lock_offset--;
        }
    }
    else if (c == 0x0d)         // RETURN
    {
        memset(screen_list->lock.lockmsg, 0, 1024);
        if (validate_lock(screen_list, getenv("USER"), screen_list->lock.lockpass))
        {
            memset(screen_list->lock.lockpass, 0, 1024);
            screen_list->lock.locked = 0;
            screen_list->lock.lock_offset = 0;
            refresh = 1;
        }
        else
        {
            memset(screen_list->lock.lockpass, 0, 1024);
            screen_list->lock.lock_offset = 0;
            refresh = 1;
        }
    }
    else
    {
        if (screen_list->lock.lock_offset < 1023)
        {
            screen_list->lock.lockpass[screen_list->lock.lock_offset++] = c;
            screen_list->lock.lockpass[screen_list->lock.lock_offset] = 0;
        }
    }
#endif

    return refresh;
}

void draw_lock(struct screen_list *screen_list)
{
#if defined USE_LOCK
    unsigned int i;
    char buffer[1024];
    caca_canvas_t *cv = screen_list->cv;

    gethostname(buffer, sizeof(buffer) - 1);

    int w = 65, h = 20;
    int x = (caca_get_canvas_width(cv) - w) / 2;
    int y = (caca_get_canvas_height(cv) - h) / 2;


    caca_set_color_ansi(cv, CACA_BLUE, CACA_BLUE);
    caca_fill_box(cv, x, y, w, h, '#');
    caca_set_color_ansi(cv, CACA_DEFAULT, CACA_BLUE);
    caca_draw_cp437_box(cv, x, y, w, h);

    x += 2;
    y++;
    caca_printf(cv,
                (caca_get_canvas_width(cv) -
                 strlen(PACKAGE_STRING " locked")) / 2, y - 1,
                PACKAGE_STRING " locked");

    caca_printf(cv, x, y++, "Please type in your password for %s@%s :",
                getenv("USER"), buffer);
    y += 2;

    x = (caca_get_canvas_width(cv) / 2) -
        ((strlen(screen_list->lock.lockpass) / 2) + strlen("Password : "));
    caca_printf(cv, x, y, "Password : ");
    x += strlen("Password : ");
    for (i = 0; i < strlen(screen_list->lock.lockpass); i++)
    {
        caca_put_str(cv, x, y, "*");
        x++;
    }


    if (strlen(screen_list->lock.lockmsg))
    {
        x = ((caca_get_canvas_width(cv) - w) / 2) +
            (strlen(screen_list->lock.lockmsg));
        y += 2;
        caca_set_color_ansi(cv, CACA_RED, CACA_BLUE);
        caca_printf(cv, x, y, "Error : %s", screen_list->lock.lockmsg);
    }
#endif
}


#if defined USE_LOCK

/* FIXME, handle this without assuming this is a password auth */
static int convpam(int num_msg, const struct pam_message **msg,
                   struct pam_response **resp, void *appdata_ptr)
{

    struct pam_response *aresp;
    int i;
    aresp = calloc(num_msg, sizeof(*aresp));

    for (i = 0; i < num_msg; ++i)
    {
        switch (msg[i]->msg_style)
        {
        case PAM_PROMPT_ECHO_ON:
        case PAM_PROMPT_ECHO_OFF:
            aresp[i].resp = strdup(appdata_ptr);
            aresp[i].resp_retcode = 0;
            break;
        case PAM_ERROR_MSG:
            break;
        default:
            printf("Unknow message type from PAM\n");
            break;
        }
    }

    *resp = aresp;
    return (PAM_SUCCESS);
}
#endif

int validate_lock(struct screen_list *screen_list, char *user, char *pass)
{
#if USE_LOCK
    int ret;
    pam_handle_t *pamh = NULL;
    char buffer[100];
    const char *service = "neercs";
    struct pam_conv conv = {
        convpam,
        pass,
    };

    ret = pam_start(service, user, &conv, &pamh);
    if (ret != PAM_SUCCESS)
        return 0;
    pam_set_item(pamh, PAM_RUSER, user);

    ret = gethostname(buffer, sizeof(buffer) - 1);
    if (ret)
    {
        perror("failed to look up hostname");
        ret = pam_end(pamh, PAM_ABORT);
        sprintf(screen_list->lock.lockmsg, "Can't get hostname");
        pam_end(pamh, PAM_SUCCESS);
        return 0;
    }

    ret = pam_set_item(pamh, PAM_RHOST, buffer);
    if (ret != PAM_SUCCESS)
    {
        sprintf(screen_list->lock.lockmsg, "Can't set hostname");
        pam_end(pamh, PAM_SUCCESS);
        return 0;
    }

    ret = pam_authenticate(pamh, 0);
    if (ret != PAM_SUCCESS)
    {
        sprintf(screen_list->lock.lockmsg, "Can't authenticate");
        pam_end(pamh, PAM_SUCCESS);
        return 0;
    }

    ret = pam_end(pamh, PAM_SUCCESS);
#endif

    return 1;
}

#endif
