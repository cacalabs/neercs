/*
 *  neercs — console-based window manager
 *
 *  Copyright © 2006—2015 Sam Hocevar <sam@hocevar.net>
 *            © 2008—2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://www.wtfpl.net/ for more details.
 */

#pragma once

#include <stdint.h>
#include <caca.h>

#include "widgets.h"

enum wm_types
{
    WM_FULL,
    WM_CARD,
    WM_HSPLIT,
    WM_VSPLIT,

    WM_MAX,
};

enum mouse_report
{
    MOUSE_NONE,
    MOUSE_X10,
    MOUSE_VT200,
    MOUSE_VT200_HIGHLIGHT,
    MOUSE_BTN_EVENT,
    MOUSE_ANY_EVENT,
};


/* ISO-2022 Conversion State */
struct iso2022_conv_state
{
    /* cs = coding system/coding method: */
    /* (with standard return) */
    /* '@' = ISO-2022, */
    /* 'G' = UTF-8 without implementation level, */
    /* '8' = UTF-8 (Linux console and imitators), */
    /* and many others that are rarely used; */
    /* (without standard return) */
    /* '/G' = UTF-8 Level 1, */
    /* '/H' = UTF-8 Level 2, */
    /* '/I' = UTF-8 Level 3, */
    /* and many others that are rarely used */
    uint32_t cs;
    /* ctrl8bit = allow 8-bit controls */
    uint8_t ctrl8bit;
    /* cn[0] = C0 control charset (0x00 ... 0x1f):
     * '@' = ISO 646,
     * '~' = empty,
     * and many others that are rarely used */
    /* cn[1] = C1 control charset (0x80 ... 0x9f):
     * 'C' = ISO 6429-1983,
     * '~' = empty,
     * and many others that are rarely used */
    uint32_t cn[2];
    /* glr[0] = GL graphic charset (94-char. 0x21 ... 0x7e,
     *                              94x94-char. 0x21/0x21 ... 0x7e/0x7e),
     * and
     * glr[1] = GR graphic charset (94-char. 0xa1 ... 0xfe,
     *                              96-char. 0xa0 ... 0xff,
     *                              94x94-char. 0xa1/0xa1 ... 0xfe/0xfe,
     *                              96x96-char. 0xa0/0xa0 ... 0xff/0xff):
     * 0 = G0, 1 = G1, 2 = G2, 3 = G3 */
    uint8_t glr[2];
    /* gn[i] = G0/G1/G2/G3 graphic charset state:
     * (94-char. sets)
     * '0' = DEC ACS (VT100 and imitators),
     * 'B' = US-ASCII,
     * and many others that are rarely used for e.g. various national ASCII variations;
     * (96-char. sets)
     * '.A' = ISO 8859-1 "Latin 1" GR,
     * '.~' = empty 96-char. set,
     * and many others that are rarely used for e.g. ISO 8859-n GR;
     * (double-byte 94x94-charsets)
     * '$@' = Japanese Character Set ("old JIS") (JIS C 6226:1978),
     * '$A' = Chinese Character Set (GB 2312),
     * '$B' = Japanese Character Set (JIS X0208/JIS C 6226:1983),
     * '$C' = Korean Graphic Character Set (KSC 5601:1987),
     * '$D' = Supplementary Japanese Graphic Character Set (JIS X0212),
     * '$E' = CCITT Chinese Set (GB 2312 + GB 8565),
     * '$G' = CNS 11643 plane 1,
     * '$H' = CNS 11643 plane 2,
     * '$I' = CNS 11643 plane 3,
     * '$J' = CNS 11643 plane 4,
     * '$K' = CNS 11643 plane 5,
     * '$L' = CNS 11643 plane 6,
     * '$M' = CNS 11643 plane 7,
     * '$O' = JIS X 0213 plane 1,
     * '$P' = JIS X 0213 plane 2,
     * '$Q' = JIS X 0213-2004 Plane 1,
     * and many others that are rarely used for e.g. traditional
     * ideographic Vietnamese and BlissSymbolics;
     * (double-byte 96x96-charsets)
     * none standardized or in use on terminals AFAIK (Mule does use
     * some internally)
     */
    uint32_t gn[4];
    /* ss = single-shift state: 0 = GL, 2 = G2, 3 = G3 */
    uint8_t ss;
};

struct screen
{
    /* Graphics stuff */
    int init;
    caca_canvas_t *cv;
    uint32_t clearattr;
    uint8_t fg, bg;   /* ANSI-context fg/bg */
    uint8_t dfg, dbg; /* Default fg/bg */
    uint8_t bold, blink, italics, negative, concealed, underline;
    uint8_t faint, strike, proportional; /* unsupported */
    struct iso2022_conv_state conv_state; /* charset mess */

    /* Other stuff */
    int visible;                 /* Draw canvas and border flag */
    int fd;                      /* pty fd */
    unsigned char *buf;          /* text buffer */
    long int total;              /* buffer length */
    char *title;                 /* tty title */
    int bell;                    /* bell occuring */
    unsigned int scroll, s1, s2; /* FIXME, ANSI scroll properties */
    int pid;                     /* running program pid */
    int changed;                 /* content was updated */

    int x, y;                    /* Canvas position */
    int w, h;                    /* Canvas size */

    int orig_x, orig_y;          /* Used by recurrents */
    int orig_w, orig_h;          /* Used by recurrents */

    int report_mouse;            /* ANSI */
};

enum socket_type
{
    SOCK_SERVER=0,
    SOCK_CLIENT=1
};

struct cube_props
{
    int in_switch;
    int side;
    long long unsigned int duration;
};

struct interpreter_props
{
    /* Input box */
    struct input_box *box;
};

struct screensaver
{
    /* ScreenSaver stuff */
    long long unsigned int timeout;     /* Screensaver timeout in us */
    int in_screensaver;
    void *data;
};

struct comm
{
    /* Detaching */
    int attached;                /* Are we attached to a terminal or a server */
    int socket[2];               /* Sockets to write to the server / to the client */
    char *socket_path[2];        /* Sockets to write to the server / to the client */
    char *socket_dir;            /* Where to create the socket */
    char *session_name;          /* Name of the session */
};

struct lock
{
    int locked;
    int lock_offset;
    int lock_on_detach;
    long long unsigned int  autolock_timeout;
    char lockpass[1024];
    char lockmsg[1024];
};

struct modals
{
    /* Add-ons*/
    int mini;                    /* Thumbnails */
    int status;                  /* Status bar */
    int help;                    /* Help */
    int python_command;          /* Python command */
    int window_list;             /* Window list */
    int cur_in_list;             /* Window list */
};

struct sys
{
    char *default_shell;
    char *user_path;
    int  *to_grab;
    char **to_start;
    int  nb_to_grab;
    int  attach, forceattach;
};

struct screen_list
{
    int outfd;                   /* Debug */
    int in_bell;                 /* Bell occuring in a window  */
    int was_in_bell;
    int dont_update_coords;      /* Used by recurrents */
    int changed;                 /* Global redraw (e.g. adding a screen) */
    int delay;                   /* Minimal time between two refresh (ms) */
    int requested_delay;
    int force_refresh;
    int need_refresh;            /* If we skipped a refresh, do it next time */
    int command;
    int eyecandy;                /* Eye Candy */

    long long unsigned int last_key_time;
    long long unsigned int last_refresh_time;

    struct comm comm;            /* Client/Server communications */
    struct lock lock;            /* Lock */
    struct modals modals;        /* Modal windows */
    struct interpreter_props interpreter_props; /* Python interpreter */
    struct screensaver screensaver;/* Screensaver stuff */

    int pty, prevpty;            /* Current and previous window */
    int count;                   /* Window count */
    int wm_type;                 /* Window manager type */
    int border_size;             /* Borders */
    struct cube_props cube;      /* Cube */
    long long unsigned int last_switch; /* Cube */

    struct screen **screen;      /* Windows */
    struct option *config;       /* Option parsing and configuration */
    struct sys sys;              /* System stuff */

    struct recurrent_list *recurrent_list;  /* Recurrents functions */

    char *title;                 /* Window title */
    int width, height;           /* caca window size */
    int old_x, old_y;            /* Mouse */
    int mouse_button;
    caca_canvas_t *cv;
    caca_display_t *dp;
};

/* Configuration */
struct option
{
    char *key;
    char *value;

    struct option *next;
};
struct config_line
{
    const char name[32];
    int  (*set) (const char *argv, struct screen_list * screen_list);
    char* (*get) (struct screen_list * screen_list);
};

/* Recurrents */
struct recurrent
{
    int (*function)(struct screen_list*, struct recurrent* rec, void *user, long long unsigned int t);
    void *user;
    long long unsigned int  start_time;
    int kill_me;
};

struct recurrent_list
{
    int count;
    struct recurrent **recurrent;
};



void version(void);
void usage(int argc, char **argv);
struct screen_list *init_neercs(int argc, char **argv);

int handle_command_line(int argc, char *argv[], struct screen_list *screen_list);

struct screen_list *create_screen_list(void);
void free_screen_list(struct screen_list *screen_list);

int start_client(struct screen_list * screen_list);
/** \defgroup client neercs client
 * @{ */
void mainloop(struct screen_list *screen_list);
int mainloop_tick(char **pbuf, struct screen_list *screen_list);
/** }@ */


int create_pty(char *cmd, unsigned int w, unsigned int h, int *cpid);
int create_pty_grab(long pid, unsigned int w, unsigned int h, int *cpid);
int grab_process(long pid, char *ptyname, int ptyfd, int *newpid);
long select_process(struct screen_list* screen_list);

long int import_term(struct screen_list *screen_list, struct screen *sc, void const *data, unsigned int size);
int set_tty_size(int fd, unsigned int w, unsigned int h);
int update_terms(struct screen_list* screen_list);
void refresh_screens(struct screen_list *screen_list);
int update_screens_contents(struct screen_list* screen_list);
int install_fds(struct screen_list *screen_list);
long long get_us(void);

void attach(struct screen_list* screen_list);
int detach(struct screen_list* screen_list);
int request_attach(struct screen_list* screen_list);
char * build_socket_path(char *socket_dir, char *session_name, enum socket_type socktype);
int create_socket(struct screen_list* screen_list, enum socket_type socktype);
char * connect_socket(struct screen_list* screen_list, enum socket_type socktype);
char ** list_sockets(char *socket_dir, char *session_name);
int start_server(struct screen_list *screen_list);
int send_event(caca_event_t ev, struct screen_list* screen_list);
int send_delay(struct screen_list* screen_list);
int send_ansi_sequence(struct screen_list *screen_list, char *str);

/* Screens management */
struct screen* create_screen(int w, int h, char *command);
struct screen* create_screen_grab(int w, int h, int pid);
int destroy_screen(struct screen *s);
int add_screen(struct screen_list *list, struct screen *s);
int remove_screen(struct screen_list *list, int n, int please_kill);
void resize_screen(struct screen *s, int z, int h);

/* Window managers */
void update_windows_props(struct screen_list *screen_list);
void update_windows_props_cards(struct screen_list *screen_list);
void update_windows_props_hsplit(struct screen_list *screen_list);
void update_windows_props_full(struct screen_list *screen_list);
void update_windows_props_vsplit(struct screen_list *screen_list);
void update_windows_props_cube(struct screen_list *screen_list);

void wm_refresh(struct screen_list *screen_list);
void wm_refresh_card(struct screen_list *screen_list);
void wm_refresh_cube(struct screen_list *screen_list);
void wm_refresh_full(struct screen_list *screen_list);
void wm_refresh_hsplit(struct screen_list *screen_list);
void wm_refresh_vsplit(struct screen_list *screen_list);
int switch_screen_recurrent(struct screen_list* screen_list, struct recurrent* rec, void *user, long long unsigned int t);


/* Effects and addons */
void draw_thumbnails(struct screen_list *screen_list);
void draw_status(struct screen_list *screen_list);
void draw_help(struct screen_list *screen_list);
int help_handle_key(struct screen_list *screen_list, unsigned int c);
int update_window_list(int c, struct screen_list *screen_list);
void draw_list(struct screen_list *screen_list);
void draw_lock(struct screen_list *screen_list);
int update_lock(int c, struct screen_list *screen_list);
int validate_lock(struct screen_list *screen_list, char *user, char *pass);

int close_screen_recurrent(struct screen_list*, struct recurrent* rec, void *user, long long unsigned int t);

/* Input to ANSI */
void *convert_input_ansi(unsigned int *c, int *size);
int  handle_command_input(struct screen_list*screen_list, unsigned int c);


/* Screensavers */
void screensaver_init(struct screen_list *screen_list);
void screensaver_kill(struct screen_list *screen_list);

void draw_screensaver(struct screen_list *screen_list);
void screensaver_flying_toasters(struct screen_list *screen_list);

void screensaver_flying_toasters_init(struct screen_list *screen_list);

void screensaver_flying_toasters_kill(struct screen_list *screen_list);

/* Actions */
void dump_to_file(struct screen_list *screen_list);

/* Recurrents */
int handle_recurrents(struct screen_list* screen_list);
int add_recurrent(struct recurrent_list *list,
                  int (*function)(struct screen_list*, struct recurrent* rec, void *user, long long unsigned int t),
                  void *user);
int remove_recurrent(struct recurrent_list *list, int n);



/* Configuration file */
int read_configuration_file(char *filename, struct screen_list *screen_list);
int parse_conf_line(char *buf, int size, struct screen_list *screen_list);
int get_key_value(char *line, struct option *option);
int fill_config(struct screen_list *screen_list);
struct config_line *get_config(const char *name);
struct config_line *get_config_option(void);

/* Python interpreter */
#ifdef USE_PYTHON
int python_init(struct screen_list *sl);
int python_close(struct screen_list *sl);
int  python_command_handle_key(struct screen_list *screen_list, unsigned int c);
void draw_python_command(struct screen_list *screen_list);
#endif

#if __cplusplus
    /* do nothing */
#elif defined DEBUG
#   include <stdio.h>
#   include <stdarg.h>
static inline void debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "** neercs debug ** ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}
#else
#   define debug(format, ...) do {} while(0)
#endif

