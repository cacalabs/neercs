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

#include <sys/types.h>

typedef struct nrx_socket nrx_socket_t;

nrx_socket_t * socket_open(char const *path, int server);
int socket_select(nrx_socket_t *sock, int usecs);
int socket_puts(nrx_socket_t *sock, char const *str);
ssize_t socket_read(nrx_socket_t *sock, void *buf, size_t count);
void socket_close(nrx_socket_t *socket);
