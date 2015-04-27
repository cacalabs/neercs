/*
 *  neercs — console-based window manager
 *
 *  Copyright © 2009—2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
 *
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What the Fuck You Want
 *  to Public License, Version 2, as published by the WTFPL Task Force.
 *  See http://www.wtfpl.net/ for more details.
 */

#ifdef USE_PYTHON

#include <Python.h>
#include "neercs.h"

void initNeercsModule(struct screen_list *sl);
void getExportedValues(PyObject * dictionary);
void setExportedValues(PyObject * dictionary);


#endif
