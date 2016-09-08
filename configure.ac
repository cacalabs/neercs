dnl
dnl  Configure script for a Lol Engine project
dnl

AC_INIT(neercs, 0.0)

dnl
dnl  Standard autoconf setup and tools requirements
dnl

AC_PREREQ(2.50)
AC_CONFIG_AUX_DIR(.auto)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([subdir-objects no-define tar-ustar silent-rules])
AM_DEFAULT_VERBOSITY=0

AC_PROG_CXX
AM_PROG_LIBTOOL
AC_LIBTOOL_CXX

dnl
dnl  Inherit all Lol Engine checks
dnl

LOL_AC_SUBPROJECT()

dnl
dnl  Additional checks
dnl

PKG_CHECK_MODULES(CACA, caca >= 0.99.beta17)

dnl  Unix-specific libutil
AC_CHECK_LIB(util, forkpty,
 [UTIL_LIBS="${UTIL_LIBS} -lutil"
  dnl  Override future forkpty detection
  ac_cv_func_forkpty="yes"])
AC_CHECK_FUNCS(forkpty)
AC_CHECK_FUNCS(getopt_long)

AC_CHECK_HEADERS(glob.h endian.h getopt.h)
AC_CHECK_HEADERS(libutil.h util.h pty.h)
AC_CHECK_HEADERS(linux/kdev_t.h linux/major.h)
AC_CHECK_HEADERS(security/pam_appl.h security/pam_misc.h)
AC_CHECK_HEADERS(pam/pam_appl.h pam/pam_misc.h)

AC_SUBST(UTIL_LIBS)
AC_SUBST(PAM_LIBS)

dnl
dnl  Perform the actual commands
dnl

AC_CONFIG_FILES(
 [Makefile
  neercs/Makefile
])

AC_OUTPUT

