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

#include <caca.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "neercs.h"

void dump_to_file(struct screen_list *screen_list)
{
    char filename[14] = "hardcopy.0";
    int n = 0;
    struct stat bufstat;
    void * export;
    size_t len, wrote;
    FILE * out;

    /* FIXME maybe use glob and get next one directly */
    while (n<9999 && !stat(filename, &bufstat))
    {
        n++;
        sprintf(&filename[9], "%d", n);
    }
    if (n>=9999)
    {
        debug("Too many hardcopy files in current directory\n");
        return;
    }

    export = caca_export_canvas_to_memory(screen_list->cv, "ansi", &len);
    if (!export)
    {
        debug("Failed to export to ansi\n");
        return;
    }

    out = fopen(filename, "w");
    if (!out)
    {
        debug("Failed to open output file %s: %s\n", filename, strerror(errno));
        return;
    }
    wrote = fwrite(export, len, 1, out);
    if (!wrote)
    {
        debug("Failed to write to output file: %s\n", strerror(errno));
        return;
    }
    free(export);
}
