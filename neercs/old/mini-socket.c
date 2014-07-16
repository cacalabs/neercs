/*
 *  neercs        console-based window manager
 *  Copyright (c) 2006-2011 Sam Hocevar <sam@hocevar.net>
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

#include <stdio.h> /* perror() */
#include <stdlib.h> /* malloc(), free() */
#include <unistd.h> /* unlink() */
#include <fcntl.h> /* fcntl() */
#include <string.h> /* memcpy() */
#include <sys/select.h> /* select() */
#include <sys/types.h> /* bind(), connect() */
#include <sys/socket.h> /* bind(), connect() */
#include <sys/stat.h>  /* stat(), struct stat */
#include <sys/un.h> /* AF_UNIX */
#include <errno.h> /* AF_UNIX */
#include <time.h> /* time */

#include "mini-neercs.h"
#include "mini-socket.h"

#define SIZEOF_SUN_PATH (sizeof(((struct sockaddr_un *)NULL)->sun_path))

#define offsetof(s, f) ((int)(intptr_t)((s *)NULL)->f)

struct nrx_socket
{
#if 1
    /* Linux sockets */
    int fd;
    int server;
    int connected;
    char path[SIZEOF_SUN_PATH];
#else
#   error No socket implementation
#endif
};

#define QLEN 10

int
serv_listen(const char *name)
{
    int                 fd, len, err, rval;
    struct sockaddr_un  un;

    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
       return(-1);
    unlink(name);   /* in case it already exists */

    /* fill in socket address structure */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

    /* bind the name to the descriptor */
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -2;
        goto errout;
    }
    if (listen(fd, QLEN) < 0) { /* tell kernel we're a server */
        rval = -3;
        goto errout;
    }
    return(fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return(rval);
}

#define STALE   30  /* client's name can't be older than this (sec) */

/*
 * Wait for a client connection to arrive, and accept it.
 * We also obtain the client's user ID from the pathname
 * that it must bind before calling us.
 * Returns new fd if all OK, <0 on error
 */
int
serv_accept(int listenfd, uid_t *uidptr)
{
    int                 clifd, len, err, rval;
    time_t              staletime;
    struct sockaddr_un  un;
    struct stat         statbuf;

    len = sizeof(un);
    if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0)
        return(-1);     /* often errno=EINTR, if signal caught */

    /* obtain the client's uid from its calling address */
    len -= offsetof(struct sockaddr_un, sun_path); /* len of pathname */
    un.sun_path[len] = 0;           /* null terminate */

    if (stat(un.sun_path, &statbuf) < 0) {
        rval = -2;
        goto errout;
    }
#ifdef S_ISSOCK     /* not defined for SVR4 */
    if (S_ISSOCK(statbuf.st_mode) == 0) {
        rval = -3;      /* not a socket */
        goto errout;
    }
#endif
    if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
        (statbuf.st_mode & S_IRWXU) != S_IRWXU) {
          rval = -4;     /* is not rwx------ */
          goto errout;
    }

    staletime = time(NULL) - STALE;
    if (statbuf.st_atime < staletime ||
        statbuf.st_ctime < staletime ||
        statbuf.st_mtime < staletime) {
          rval = -5;    /* i-node is too old */
          goto errout;
    }
    if (uidptr != NULL)
        *uidptr = statbuf.st_uid;   /* return uid of caller */
    unlink(un.sun_path);        /* we're done with pathname now */
    return(clifd);

errout:
    err = errno;
    close(clifd);
    errno = err;
    return(rval);
}

#define CLI_PATH    "/var/tmp/"      /* +5 for pid = 14 chars */
#define CLI_PERM    S_IRWXU          /* rwx for user only */

/*
 * Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.
 */
int
cli_conn(const char *name)
{
    int                fd, len, err, rval;
    struct sockaddr_un un;

    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return(-1);

    /* fill socket address structure with our address */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s%05d", CLI_PATH, getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    unlink(un.sun_path);        /* in case it already exists */
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -2;
        goto errout;
    }
    if (chmod(un.sun_path, CLI_PERM) < 0) {
        rval = -3;
        goto errout;
    }
    /* fill socket address structure with server's address */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
    if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -4;
        goto errout;
    }
    return(fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return(rval);
}

nrx_socket_t * socket_open(char const *path, int server)
{
    nrx_socket_t * sock;
    struct sockaddr_un addr;
    int ret, fd;

#if 0
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    //fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("socket creation");
        return NULL;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, SIZEOF_SUN_PATH - 1);

    if (server)
    {
        unlink(path);
        ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    }
    else
    {
        ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    }

    if (ret < 0)
    {
        perror(server ? "socket binding" : "socket connection");
        close(fd);
        return NULL;
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
if (server)
    fd = serv_listen(path);
else
    fd = cli_conn(path);
if (fd < 0) return NULL;

    sock = malloc(sizeof(*sock));
    sock->fd = fd;
    sock->server = server;
    sock->connected = 0;
    strncpy(sock->path, path, SIZEOF_SUN_PATH - 1);

    return sock;
}

int socket_select(nrx_socket_t *sock, int usecs)
{
    fd_set rfds;
    struct timeval tv;
    int ret;

    FD_ZERO(&rfds);
    FD_SET(sock->fd, &rfds);

    tv.tv_sec = usecs / 1000000;
    tv.tv_usec = usecs % 1000000;

    ret = select(sock->fd + 1, &rfds, NULL, NULL, &tv);
    if (ret < 0)
        return -1;

    if (FD_ISSET(sock->fd, &rfds))
        return 1;

    return 0;
}

int socket_puts(nrx_socket_t *sock, char const *str)
{
    int ret;
fprintf(stderr, "pid %i sending %i bytes on %s: %s\n", getpid(), (int)strlen(str), sock->path, str);
    ret = write(sock->fd, str, strlen(str));
    return ret;
}

ptrdiff_t socket_read(nrx_socket_t *sock, void *buf, size_t count)
{
    int ret;
    ret = read(sock->fd, buf, count);
if (ret >= 0) ((char *)buf)[ret] = 0;
if (ret >= 0) fprintf(stderr, "pid %i recving %i bytes on %s: %s\n", getpid(), ret, sock->path, (char *)buf);
    return ret;
}

void socket_close(nrx_socket_t *sock)
{
    close(sock->fd);
    if (sock->server)
        unlink(sock->path);
    free(sock);
}

