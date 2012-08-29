
if BUILD_NEERCS
noinst_PROGRAMS = neercs
endif

neercs_SOURCES = \
    neercs.cpp neercs.h \
    \
    $(old_sources) \
    \
    video/render.cpp video/render.h \
    video/text-render.cpp video/text-render.h \
    video/simple.lolfx \
    video/blurh.lolfx video/blurv.lolfx video/glow.lolfx \
    video/remanency.lolfx video/copper.lolfx video/color.lolfx \
    video/noise.lolfx video/postfx.lolfx \
    video/text.lolfx
neercs_CPPFLAGS = @LOL_CFLAGS@ @PIPI_CFLAGS@ @CACA_CFLAGS@ -Iold
neercs_LDADD =
neercs_LDFLAGS = $(top_builddir)/src/liblol.a \
                 @LOL_LIBS@ @PIPI_LIBS@ @CACA_LIBS@ @UTIL_LIBS@ @PAM_LIBS@
neercs_DEPENDENCIES = $(top_builddir)/src/liblol.a

if BUILD_NEERCS
all-local: neercs$(EXEEXT)
	test $(MAKE_FSELF) = no || make_fself neercs$(EXEEXT) neercs.self
endif

CLEANFILES = $(noinst_PROGRAMS:%$(EXEEXT)=%.self) \
             $(noinst_PROGRAMS:%$(EXEEXT)=%.elf) \
             $(noinst_PROGRAMS:%$(EXEEXT)=%.exe)

SUFFIXES = .lolfx
.lolfx.o:
	$(LOLFX_BUILD)

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
