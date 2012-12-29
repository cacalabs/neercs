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
#include "py_module.h"

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

/* FIXME : Find a way to pass a user pointer to PyModuleDef or something */
static struct screen_list *screen_list;

#if defined HAVE_PYTHON3
static PyObject *PyInit_neercs(void);
#else
static void PyInit_neercs(void);
#endif
static void removeTrailingStuff(char *b);


static void addVariableFromConfig(PyObject * dictionary,
                                  const char *varname, const char *configname)
{
    char *v = get_config(configname)->get(screen_list);
    if (v != NULL)
    {
        PyObject *value = Py_BuildValue("s", v);
        PyDict_SetItemString(dictionary, varname, value);
    }

    debug("py get '%s' to '%s'\n", varname,
          get_config(configname)->get(screen_list));
}

static void removeTrailingStuff(char *b)
{
    if(!b)
        return;
    if(b[0]=='\'')
    {
        memmove(b, &b[1], strlen(b)-1);
        b[strlen(b)-2] = 0;
    }
}

void setExportedValues(PyObject * dictionary)
{
    struct config_line *config_option = get_config_option();
    int i = 0;

    while (strncmp(config_option[i].name, "last", strlen("last")))
    {
        /* Get variable */
        PyObject *res =
            PyDict_GetItemString(dictionary, config_option[i].name);

        /* Got it */
        if (res)
        {
            /* Get object representation
             * FIXME : find a way to check object's type */
            PyObject *str = PyObject_Repr(res);

            /* Make sure it's a string */
            char *err =
#if defined HAVE_PYTHON3
                PyBytes_AS_STRING(PyUnicode_AsEncodedString
                                  (str, "utf-8", "Error ~"));
#elif defined HAVE_PYTHON2
                PyString_AsString(str);
#endif
            /* FIXME leak leak leak */
            char *s = strdup(err);

            if (s != NULL)
            {
                /* Representation can include '' around strings */
                removeTrailingStuff(s);
                get_config(config_option[i].name)->set(s, screen_list);
            }
        }
        i++;
    }
}

void getExportedValues(PyObject * dictionary)
{
    struct config_line *config_option = get_config_option();
    int i = 0;
    while (strncmp(config_option[i].name, "last", strlen("last")))
    {
        addVariableFromConfig(dictionary, config_option[i].name,
                              config_option[i].name);
        i++;
    }
}

static PyObject *neercs_get(PyObject * self, PyObject * args)
{
    char *s = NULL;

    debug("Get using list at %p", screen_list);

    if (!PyArg_ParseTuple(args, "s", &s))
    {
        PyErr_SetString(PyExc_ValueError, "Can't parse argument");
        debug("py Can't parse");
        return NULL;
    }
    debug("py Argument : '%s'", s);
    struct config_line *c = get_config(s);

    if (c)
        return Py_BuildValue("s", c->get(screen_list));


    PyErr_SetString(PyExc_ValueError,
                    "Can't get value for specified variable");
    return NULL;
}

static PyObject *neercs_version(PyObject * self, PyObject * args)
{
    return Py_BuildValue("s", PACKAGE_VERSION);
}

static PyMethodDef NeercsMethods[] =
{
    { "version", neercs_version, METH_NOARGS, "Return the neercs version." },
    { "get", neercs_get, METH_VARARGS,
      "Return the specified variable's value." },
    { NULL, NULL, 0, NULL }
};

#if defined HAVE_PYTHON3
static PyObject *PyInit_neercs(void)
{
    static PyModuleDef NeercsModule =
    {
        PyModuleDef_HEAD_INIT, "neercs", NULL, -1, NeercsMethods,
        NULL, NULL, NULL, NULL
    };

    return PyModule_Create(&NeercsModule);
}

#elif defined HAVE_PYTHON2
static void PyInit_neercs(void)
{
    PyMethodDef *m = NeercsMethods;
    PyObject *mod = PyModule_New("neercs");
    PyModule_AddStringConstant(mod, "__file__", "<synthetic>");

    for (m = NeercsMethods; m->ml_name; m++)
        PyModule_AddObject(mod, m->ml_name, PyCFunction_New(m, NULL));
}
#endif

void initNeercsModule(struct screen_list *sl)
{
    screen_list = sl;
    PyImport_AppendInittab("neercs", &PyInit_neercs);
    Py_Initialize();
}

#endif
