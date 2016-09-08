﻿/*
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

void client_init(void);
int client_step(void);
void client_fini(void);

void server_init(void);
int server_step(void);
void server_fini(void);

