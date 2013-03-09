
include $(top_srcdir)/build/autotools/common.am

if !USE_ANDROID
if BUILD_NEERCS
bin_PROGRAMS = neercs
endif
endif

neercs_SOURCES = \
    neercs.cpp neercs.h \
    \
    $(old_sources) \
    \
    term/term.h term/term.cpp term/ansi.cpp \
    term/pty.h term/pty.cpp \
    \
    video/render.cpp video/render.h \
    video/text-render.cpp video/text-render.h \
    video/simple.lolfx \
    video/blurh.lolfx video/blurv.lolfx video/glow.lolfx \
    video/remanence.lolfx video/copper.lolfx video/color.lolfx \
    video/noise.lolfx video/postfx.lolfx video/mirror.lolfx \
    video/radial.lolfx video/text.lolfx
neercs_CPPFLAGS = $(AM_CPPFLAGS) @CACA_CFLAGS@ -Iold
neercs_LDFLAGS = $(AM_LDFLAGS) @CACA_LIBS@ @UTIL_LIBS@ @PAM_LIBS@
neercs_DEPENDENCIES = @LOL_DEPS@

old_sources = \
    old/actions.c \
    old/ansi.c \
    old/attach.c \
    old/client.c \
    old/configuration.c \
    old/effects.c \
    old/grab.c \
    old/help.c \
    old/input.c \
    old/lock.c \
    old/main.c \
    old/mygetopt.c \
    old/mygetopt.h \
    old/mytrace.c \
    old/mytrace.h \
    old/neercs.h \
    old/python/interpreter.c \
    old/python/py_module.c \
    old/python/py_module.h \
    old/recurrent.c \
    old/screen_list.c \
    old/screensaver.c \
    old/screens.c \
    old/server.c \
    old/term.c \
    old/widgets.c \
    old/widgets.h \
    old/wm.cpp
