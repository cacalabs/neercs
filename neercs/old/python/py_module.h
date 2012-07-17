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

#include "config.h"

#ifdef USE_PYTHON

#include <Python.h>
#include "neercs.h"

void initNeercsModule(struct screen_list *sl);
void getExportedValues(PyObject * dictionary);
void setExportedValues(PyObject * dictionary);


#endif
