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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <caca.h>

#include "core.h"

using namespace lol;

extern "C" {
#include "neercs.h"
}

void resize_screen(struct screen *s, int w, int h)
{
    caca_canvas_t *oldc, *newc;

    if (w == s->w && h == s->h)
        return;
    if (w <= 0 || h <= 0)
        return;

    s->changed = 1;

    s->w = w;
    s->h = h;

    /*
     * caca_set_canvas_boundaries() is bugged as hell, so let's resize it by
     * hands
     */
    oldc = s->cv;
    newc = caca_create_canvas(w, h);
    caca_blit(newc, 0, 0, oldc, NULL);
    s->cv = newc;
    caca_gotoxy(newc, caca_get_cursor_x(oldc), caca_get_cursor_y(oldc));
    caca_free_canvas(oldc);
    /* FIXME: disabled */
    //set_tty_size(s->fd, w, h);

    s->orig_w = s->w;
    s->orig_h = s->h;
    s->orig_x = s->x;
    s->orig_y = s->y;
}

void update_windows_props(struct screen_list *screen_list)
{
    debug("%s, %d screens, type %d\n", __FUNCTION__, screen_list->count,
          screen_list->wm_type);

    if (!screen_list->count)
        return;

    switch (screen_list->wm_type)
    {
    case WM_CARD:
        update_windows_props_cards(screen_list);
        break;
    case WM_HSPLIT:
        update_windows_props_hsplit(screen_list);
        break;
    case WM_VSPLIT:
        update_windows_props_vsplit(screen_list);
        break;
    case WM_FULL:
    default:
        update_windows_props_full(screen_list);
        break;
    }
}

void update_windows_props_hsplit(struct screen_list *screen_list)
{
    int i;
    int w =
        (screen_list->width / screen_list->count) -
        (screen_list->border_size * 2);
    int h = screen_list->height - (screen_list->border_size * 2);

    for (i = 0; i < screen_list->count; i++)
    {
        screen_list->screen[i]->x = (i * w) + screen_list->border_size;
        screen_list->screen[i]->y = screen_list->border_size;
        screen_list->screen[i]->visible = 1;
        if (i != screen_list->count - 1)
        {
            resize_screen(screen_list->screen[i], w - 1, h);
        }
        else
        {
            resize_screen(screen_list->screen[i],
                          screen_list->width - i * w - 2, h);
        }
    }
}

void update_windows_props_vsplit(struct screen_list *screen_list)
{
    int i;
    int w = screen_list->width - (screen_list->border_size * 2);
    int h = (screen_list->height) / screen_list->count;

    for (i = 0; i < screen_list->count; i++)
    {
        screen_list->screen[i]->x = screen_list->border_size;
        screen_list->screen[i]->y = (i * h) + (screen_list->border_size);
        screen_list->screen[i]->visible = 1;
        if (i != screen_list->count - 1)
        {
            resize_screen(screen_list->screen[i], w,
                          h - (screen_list->border_size * 2));
        }
        else
        {
            resize_screen(screen_list->screen[i],
                          w,
                          screen_list->height - i * h -
                          (screen_list->border_size * 2));
        }
    }
}


void update_windows_props_full(struct screen_list *screen_list)
{
    int i;
    int w = screen_list->width - (screen_list->border_size * 2);
    int h = screen_list->height - (screen_list->border_size * 2);

    for (i = 0; i < screen_list->count; i++)
    {
        screen_list->screen[i]->visible = 0;
        screen_list->screen[i]->x = screen_list->border_size;
        screen_list->screen[i]->y = screen_list->border_size;

        resize_screen(screen_list->screen[i], w, h);
    }
    screen_list->screen[screen_list->pty]->visible = 1;
}


void update_windows_props_cards(struct screen_list *screen_list)
{
    int i;
    int w = (screen_list->width - screen_list->count * 3) + 1;
    int h = (screen_list->height - screen_list->count) - 1;
    int x = 1;
    int y = screen_list->count;

    for (i = 0; i < screen_list->count; i++)
    {
        screen_list->screen[i]->visible = 1;
        screen_list->screen[i]->x = x;
        screen_list->screen[i]->y = y;

        resize_screen(screen_list->screen[i], w, h);
        x += 3;
        y--;
    }
}

/* Window managers refresh */

void wm_refresh(struct screen_list *screen_list)
{
    /* FIXME : move set_color to a relevant place */
    caca_set_color_ansi(screen_list->cv, CACA_LIGHTRED, CACA_BLACK);

    switch (screen_list->wm_type)
    {
    case WM_CARD:
        wm_refresh_card(screen_list);
        break;
    case WM_HSPLIT:
        wm_refresh_hsplit(screen_list);
        break;
    case WM_VSPLIT:
        wm_refresh_hsplit(screen_list);
        break;
    case WM_FULL:
    default:
        wm_refresh_cube(screen_list);
        break;
    }
}

static void wm_bell(struct screen_list *screen_list)
{
    if (screen_list->screen[screen_list->pty]->bell)
    {
        caca_set_color_ansi(screen_list->cv, CACA_RED, CACA_BLACK);
        screen_list->in_bell--;
        screen_list->force_refresh = 1;
        if (!screen_list->in_bell)
        {
            screen_list->was_in_bell = 1;
            screen_list->screen[screen_list->pty]->bell = 0;
        }
    }
    else
    {
        if (screen_list->was_in_bell)
        {
            screen_list->screen[screen_list->pty]->bell = 0;
            screen_list->force_refresh = 1;
            screen_list->was_in_bell = 0;
            screen_list->changed = 1;
        }
        caca_set_color_ansi(screen_list->cv, CACA_LIGHTGREEN, CACA_BLACK);
    }
}

static void wm_box(struct screen_list *screen_list, int pty)
{
    if (!screen_list->screen[pty]->changed && !screen_list->changed)
        return;

    if (!screen_list->border_size)
        return;

    /* Color determined by wm_bell() */
    caca_draw_cp437_box(screen_list->cv,
                        screen_list->screen[pty]->x - 1,
                        screen_list->screen[pty]->y - 1,
                        screen_list->screen[pty]->w + 2,
                        screen_list->screen[pty]->h + 2);

    if (screen_list->screen[pty]->title)
    {
        caca_printf(screen_list->cv,
                    screen_list->screen[pty]->x,
                    screen_list->screen[pty]->y - 1,
                    " %.*s ",
                    screen_list->screen[pty]->w - 3,
                    screen_list->screen[pty]->title);
    }
}

static void wm_blit_current_screen(struct screen_list *screen_list)
{
    if (screen_list->screen[screen_list->pty]->changed || screen_list->changed)
        caca_blit(screen_list->cv,
                  screen_list->screen[screen_list->pty]->x,
                  screen_list->screen[screen_list->pty]->y,
                  screen_list->screen[screen_list->pty]->cv, NULL);
}

void wm_refresh_card(struct screen_list *screen_list)
{
    int i;

    for (i = screen_list->count - 1; i >= 0; i--)
    {
        if (i != screen_list->pty && screen_list->screen[i]->visible &&
            (screen_list->screen[i]->changed || screen_list->changed))
        {
            caca_blit(screen_list->cv,
                      screen_list->screen[i]->x,
                      screen_list->screen[i]->y,
                      screen_list->screen[i]->cv, NULL);

            wm_box(screen_list, i);
        }
    }

    /* Force 'changed' to force redraw */
    screen_list->screen[screen_list->pty]->changed = 1;
    wm_blit_current_screen(screen_list);
    wm_bell(screen_list);
    wm_box(screen_list, screen_list->pty);
}

void wm_refresh_full(struct screen_list *screen_list)
{
    wm_blit_current_screen(screen_list);
    wm_bell(screen_list);
    wm_box(screen_list, screen_list->pty);
}

void wm_refresh_vsplit(struct screen_list *screen_list)
{
    int i;

    for (i = screen_list->count - 1; i >= 0; i--)
    {
        if (i != screen_list->pty && screen_list->screen[i]->visible &&
            (screen_list->screen[i]->changed || screen_list->changed))
        {
            caca_blit(screen_list->cv,
                      screen_list->screen[i]->x,
                      screen_list->screen[i]->y,
                      screen_list->screen[i]->cv, NULL);

            wm_box(screen_list, i);
        }
    }

    wm_blit_current_screen(screen_list);
    wm_bell(screen_list);
    wm_box(screen_list, screen_list->pty);
}

void wm_refresh_hsplit(struct screen_list *screen_list)
{
    int i;

    for (i = screen_list->count - 1; i >= 0; i--)
    {
        if (i != screen_list->pty && screen_list->screen[i]->visible &&
            (screen_list->screen[i]->changed || screen_list->changed))
        {
            caca_blit(screen_list->cv,
                      screen_list->screen[i]->x,
                      screen_list->screen[i]->y,
                      screen_list->screen[i]->cv, NULL);

            wm_box(screen_list, i);
        }
    }

    wm_blit_current_screen(screen_list);
    wm_bell(screen_list);
    wm_box(screen_list, screen_list->pty);
}


static float
get_direction(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y)
{
    float d1x, d1y, d2x, d2y;

    d1x = p3x - p1x;
    d1y = p3y - p1y;
    d2x = p3x - p2x;
    d2y = p3y - p2y;
    return (d1x * d2y) - (d1y * d2x);
}


/* 3D Cube. Yeah I know, it's a mess. Just look anywhere else. */
static void draw_face(caca_canvas_t * cv,
                      int p1x, int p1y,
                      int p2x, int p2y,
                      int p3x, int p3y,
                      int p4x, int p4y, caca_canvas_t * tex,
                      int color, int borders)
{
    if (get_direction(p1x, p1y, p2x, p2y, p3x, p3y) >= 0)
    {
        int coords[6];
        float uv[6];
        coords[0] = p1x;
        coords[1] = p1y;
        coords[2] = p2x;
        coords[3] = p2y;
        coords[4] = p3x;
        coords[5] = p3y;
        uv[0] = 1;
        uv[1] = 1;
        uv[2] = 0;
        uv[3] = 1;
        uv[4] = 0;
        uv[5] = 0;
        caca_fill_triangle_textured(cv, coords, tex, uv);
        coords[0] = p1x;
        coords[1] = p1y;
        coords[2] = p3x;
        coords[3] = p3y;
        coords[4] = p4x;
        coords[5] = p4y;
        uv[0] = 1;
        uv[1] = 1;
        uv[2] = 0;
        uv[3] = 0;
        uv[4] = 1;
        uv[5] = 0;
        caca_fill_triangle_textured(cv, coords, tex, uv);
        caca_set_color_ansi(cv, color, CACA_BLACK);
        if (borders)
        {
            caca_draw_thin_line(cv, p1x, p1y, p2x, p2y);
            caca_draw_thin_line(cv, p2x, p2y, p3x, p3y);
            caca_draw_thin_line(cv, p3x, p3y, p4x, p4y);
            caca_draw_thin_line(cv, p4x, p4y, p1x, p1y);
        }
    }
}


void wm_refresh_cube(struct screen_list *screen_list)
{
    int i;

    if (!screen_list->cube.in_switch || !screen_list->eyecandy)
    {
        wm_refresh_full(screen_list);
        // screen_list->force_refresh = 0;
    }
    else
    {
        long long unsigned int cur_time = get_us() - screen_list->last_switch;

        if (cur_time >= screen_list->cube.duration || screen_list->count == 1)
        {
            screen_list->changed = 1;
            screen_list->force_refresh = 1;
            screen_list->cube.in_switch = 0;
        }
        else
        {
            float cube[12][3] = {
                {-1, -1, 1},
                {1, -1, 1},
                {1, 1, 1},
                {-1, 1, 1},

                {1, -1, 1},
                {1, -1, -1},
                {1, 1, -1},
                {1, 1, 1},

                {-1, -1, -1},
                {-1, -1, 1},
                {-1, 1, 1},
                {-1, 1, -1},
            };

            float cube_transformed[12][3];
            float cube_projected[12][2];
            float fov = 0.5f;
            float angle =
                90.0f * ((float)cur_time / (float)screen_list->cube.duration);

            angle *= (F_PI / 180.0f);

            if (screen_list->cube.side == 1)
                angle = -angle;

            float sina = lol::sin(angle);
            float cosa = lol::cos(angle);

            for (i = 0; i < 12; i++)
            {
                cube_transformed[i][2] = cube[i][2] * cosa - cube[i][0] * sina;
                cube_transformed[i][0] = cube[i][2] * sina + cube[i][0] * cosa;
                cube_transformed[i][1] = cube[i][1];

                cube_transformed[i][2] -= 3;

                cube_projected[i][0] =
                    cube_transformed[i][0] / (cube_transformed[i][2] * fov);
                cube_projected[i][1] =
                    cube_transformed[i][1] / (cube_transformed[i][2] * fov);

                cube_projected[i][0] /= 2.0f;
                cube_projected[i][1] /= 2.0f;
                cube_projected[i][0] += 0.5f;
                cube_projected[i][1] += 0.5f;

                cube_projected[i][0] *= screen_list->width;
                cube_projected[i][1] *= screen_list->height;
            }

            caca_set_color_ansi(screen_list->cv, CACA_WHITE, CACA_BLACK);
            caca_clear_canvas(screen_list->cv);

            caca_canvas_t *first =
                screen_list->screen[screen_list->prevpty]->cv;
            caca_canvas_t *second = screen_list->screen[screen_list->pty]->cv;

            draw_face(screen_list->cv,
                      cube_projected[0][0], cube_projected[0][1],
                      cube_projected[1][0], cube_projected[1][1],
                      cube_projected[2][0], cube_projected[2][1],
                      cube_projected[3][0], cube_projected[3][1],
                      first, CACA_LIGHTGREEN, screen_list->border_size);


            if (screen_list->cube.side)
            {
                draw_face(screen_list->cv,
                          cube_projected[4][0], cube_projected[4][1],
                          cube_projected[5][0], cube_projected[5][1],
                          cube_projected[6][0], cube_projected[6][1],
                          cube_projected[7][0], cube_projected[7][1],
                          second, CACA_LIGHTGREEN, screen_list->border_size);
            }
            else
            {
                draw_face(screen_list->cv,
                          cube_projected[8][0], cube_projected[8][1],
                          cube_projected[9][0], cube_projected[9][1],
                          cube_projected[10][0], cube_projected[10][1],
                          cube_projected[11][0], cube_projected[11][1],
                          second, CACA_LIGHTGREEN, screen_list->border_size);
            }

            screen_list->changed = 1;
            screen_list->force_refresh = 1;
            screen_list->cube.in_switch = 1;
        }
    }
}
