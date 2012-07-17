
if USE_CACA
noinst_PROGRAMS = neercs
endif

neercs_SOURCES = \
    neercs.cpp neercs.h \
    \
    video/render.cpp video/render.h \
    video/text-render.cpp video/text-render.h \
    video/simple.lolfx \
    video/blurh.lolfx video/blurv.lolfx \
    video/remanency.lolfx video/glow.lolfx video/postfx.lolfx video/radial.lolfx \
    video/text.lolfx
neercs_CPPFLAGS = @LOL_CFLAGS@ @PIPI_CFLAGS@ @CACA_CFLAGS@
neercs_LDADD =
neercs_LDFLAGS = $(top_builddir)/src/liblol.a \
                 @LOL_LIBS@ @PIPI_LIBS@ @CACA_LIBS@ @UTIL_LIBS@ @PAM_LIBS@
neercs_DEPENDENCIES = $(top_builddir)/src/liblol.a

if USE_CACA
all-local: neercs$(EXEEXT)
	test $(MAKE_FSELF) = no || make_fself neercs$(EXEEXT) neercs.self
endif

CLEANFILES = $(noinst_PROGRAMS:%$(EXEEXT)=%.self) \
             $(noinst_PROGRAMS:%$(EXEEXT)=%.elf) \
             $(noinst_PROGRAMS:%$(EXEEXT)=%.exe)

SUFFIXES = .lolfx
.lolfx.o:
	$(LOLFX_BUILD)

