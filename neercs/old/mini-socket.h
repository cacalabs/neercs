/*
 *  neercs — console-based window manager
 *
 *  Copyright © 2006—2015 Sam Hocevar <sam@hocevar.net>
 *            © 2008—2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *            © 2008—2010 Pascal Terjan <pterjan@linuxfr.org>
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What the Fuck You Want
 *  to Public License, Version 2, as published by the WTFPL Task Force.
 *  See http://www.wtfpl.net/ for more details.
 */

#include <sys/types.h>

typedef struct nrx_socket nrx_socket_t;

nrx_socket_t * socket_open(char const *path, int server);
int socket_select(nrx_socket_t *sock, int usecs);
int socket_puts(nrx_socket_t *sock, char const *str);
ptrdiff_t socket_read(nrx_socket_t *sock, void *buf, size_t count);
void socket_close(nrx_socket_t *socket);

