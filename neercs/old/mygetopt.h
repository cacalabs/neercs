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

/*
 *  mygetopt.h: getopt_long reimplementation
 */

struct myoption
{
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

extern int myoptind;
extern char *myoptarg;

int mygetopt(int, char * const[], const char *, const struct myoption *, int *);

