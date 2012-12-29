/*
 *  neercs        console-based window manager
 *  Copyright (c) 2009-2010 Jean-Yves Lamoureux <jylam@lnxscene.org>
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

#ifdef USE_PYTHON

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <caca.h>

#include "neercs.h"
#include "py_module.h"

static int python_execute(struct screen_list *sl);
char *getStringFromPyObject(PyObject * p);
static char *getPythonError(void);

#if ! defined HAVE_PYTHON26
static PyObject *PyUnicode_FromString(char const *str)
{
    PyObject *tmp, *ret;
    tmp = PyString_FromString(str);
    if (!tmp)
        return NULL;
    ret = PyString_AsDecodedObject(tmp, "utf-8", "replace");
    Py_DECREF(tmp);
    return ret;
}
#endif

int python_command_handle_key(struct screen_list *screen_list, unsigned int c)
{
    int ret = widget_ibox_handle_key(screen_list->interpreter_props.box, c);

    if (ret == INPUT_BOX_ESC)
    {
        widget_ibox_destroy(screen_list->interpreter_props.box);
        screen_list->interpreter_props.box = NULL;
        screen_list->modals.python_command = 0;
    }
    else if (ret == INPUT_BOX_RET)
    {
        python_execute(screen_list);
    }
    screen_list->changed = 1;

    return 1;
}

void draw_python_command(struct screen_list *screen_list)
{
    if (!screen_list->interpreter_props.box)
    {
        screen_list->interpreter_props.box =
            widget_ibox_init(screen_list->cv, 65, 6);
        debug("py Init %p\n", screen_list->interpreter_props.box);
    }
    widget_ibox_draw(screen_list->interpreter_props.box);
}

/* Actual Python interpreter stuff */
int python_init(struct screen_list *sl)
{
    initNeercsModule(sl);

    return 0;
}

int python_close(struct screen_list *sl)
{
    widget_ibox_destroy(sl->interpreter_props.box);
    sl->interpreter_props.box = NULL;

    Py_Finalize();
    return 0;
}


static int python_execute(struct screen_list *sl)
{
    char *command = widget_ibox_get_text(sl->interpreter_props.box);
    if (!command || strlen(command) < 1)
        return -1;
    int err = 0;

    debug("py Executing '%s'\n", command);


    PyObject *pModule, *pName;

    /* Module from which to call the function */
    pName = PyUnicode_FromString("neercs");
    if (!pName)
    {
        widget_ibox_set_error(sl->interpreter_props.box, getPythonError());
        err = 1;
        debug("py Error 1\n");
        goto end;
    }

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL)
    {
        PyObject *dictionary = PyModule_GetDict(pModule);

        getExportedValues(dictionary);

        PyObject *o =
            PyRun_String(command, Py_single_input,
                         dictionary, NULL);
        if (!o)
        {
            widget_ibox_set_error(sl->interpreter_props.box, getPythonError());
            err = 1;
        }
        else
        {
            setExportedValues(dictionary);

            widget_ibox_set_msg(sl->interpreter_props.box, getStringFromPyObject(o));
            err = 1;
        }

        Py_DECREF(pModule);
    }
    else
    {
        widget_ibox_set_error(sl->interpreter_props.box, getPythonError());
        err = 1;
    }

  end:

    if (!err)
    {
        widget_ibox_destroy(sl->interpreter_props.box);
        sl->interpreter_props.box = NULL;
        sl->modals.python_command = 0;
    }
    sl->changed = 1;

    return 0;
}

static char *getPythonError(void)
{
    char *err;
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);

    char *evalue = getStringFromPyObject(value);

    int r = asprintf(&err, "%s", evalue);
    (void)r;
    return err;
}

char *getStringFromPyObject(PyObject * p)
{
    PyObject *str = PyObject_Repr(p);
    char *tmp;
#if defined HAVE_PYTHON26
    tmp = PyBytes_AS_STRING(PyUnicode_AsEncodedString(str, "utf-8", "Error ~"));
#else
    tmp = PyString_AsString(str);
#endif
    return strdup(tmp);
}


#endif
