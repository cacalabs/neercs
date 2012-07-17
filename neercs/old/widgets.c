/*
 *  neercs        console-based window manager
 *  Copyright (c) 2009-2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *                All Rights Reserved
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */


#include "widgets.h"

static void widget_ibox_add_char(struct input_box *box, unsigned int c);
static void widget_ibox_del_char(struct input_box *box);

struct input_box *widget_ibox_init(caca_canvas_t *cv, int w, int h)
{
    struct input_box *box = malloc(sizeof(struct input_box));
    if (!box)
        return NULL;
    box->cv = cv;
    box->w = w;
    box->h = h;
    box->command = NULL;
    box->output_err = NULL;
    box->output_res = NULL;

    return box;
}


int widget_ibox_draw(struct input_box *box)
{
    int x = (caca_get_canvas_width(box->cv) - box->w) / 2;
    int y = (caca_get_canvas_height(box->cv) - box->h) / 2;

    caca_set_color_ansi(box->cv, CACA_BLUE, CACA_BLUE);
    caca_fill_box(box->cv, x, y, box->w, box->h, '#');
    caca_set_color_ansi(box->cv, CACA_DEFAULT, CACA_BLUE);
    caca_draw_cp437_box(box->cv, x, y, box->w, box->h);
    caca_printf(box->cv, x, y, "Mini-command");

    caca_printf(box->cv, x + 2, y + 2,
                "[___________________________________________________________]");

    if (box->command)
    {
        caca_printf(box->cv, x + 3, y + 2, "%s", box->command);
        caca_gotoxy(box->cv, x + 3 + box->x, y + 2);
    }
    else
    {
        caca_gotoxy(box->cv, x + 3, y + 2);
    }

    if (box->output_err)
    {
        caca_set_color_ansi(box->cv, CACA_RED, CACA_BLUE);
        caca_printf(box->cv, x + 2, y + 4, box->output_err);
    }
    if (box->output_res)
    {
        caca_set_color_ansi(box->cv, CACA_LIGHTGREEN, CACA_BLUE);
        caca_printf(box->cv, x + 2, y + 4, box->output_res);
    }

    return 0;
}

char *widget_ibox_get_text(struct input_box *box)
{
    return box->command;
}

void widget_ibox_destroy(struct input_box *box)
{
    if(!box) return;
    if (box->command)
        free(box->command);
    if (box->output_err)
        free(box->output_err);
    if (box->output_res)
        free(box->output_res);
}

void widget_ibox_set_error(struct input_box *box, char *err)
{
    box->output_err = err;
}

void widget_ibox_set_msg(struct input_box *box, char *msg)
{
    box->output_res = msg;
}

int widget_ibox_handle_key(struct input_box *box, unsigned int c)
{
    if (c == CACA_KEY_ESCAPE)
    {
        if (box->command)
        {
            free(box->command);
            box->command = NULL;
        }
        return INPUT_BOX_ESC;
    }
    else if (c == CACA_KEY_LEFT)
    {
        if (box->x)
            box->x--;
    }
    else if (c == CACA_KEY_RIGHT)
    {
        if (box->x < box->size - 1)
            box->x++;
    }
    else if (c == CACA_KEY_RETURN)
    {
        return INPUT_BOX_RET;
    }
    else
    {
        if (c >= ' ' && c < 127)
            widget_ibox_add_char(box, c);
        else if (c == 8)
        {
            widget_ibox_del_char(box);
        }
    }
    return INPUT_BOX_NOTHING;
}



static void widget_ibox_add_char(struct input_box *box, unsigned int c)
{
    /* FIXME handle return values */
    if (!box->command)
    {
        box->size = 1;
        box->x = 0;
        box->command = (char *)malloc(2);
        box->command[0] = 0;
    }
    else
    {
        box->command = (char *)realloc(box->command, box->size + 1);
    }
    memmove(&box->command[box->x + 1],
            &box->command[box->x], (box->size - box->x));

    box->command[box->x] = c;
    box->x++;
    box->size++;
}

static void widget_ibox_del_char(struct input_box *box)
{
    if (box->x < 1)
        return;
    if (box->size > 1)
        box->size--;
    else
        return;

    memcpy(&box->command[box->x - 1], &box->command[box->x], box->size - box->x);

    box->command = (char *)realloc(box->command, box->size);

    if (box->x)
        box->x--;
    box->command[box->size - 1] = 0;
}
