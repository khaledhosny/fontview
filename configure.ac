AC_PREREQ([2.68])
AC_INIT([fontview], [0.10], [https://github.com/khaledhosny/fontview/issues])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

CFLAGS=

AC_PROG_CC
AC_PROG_CC_C99
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AM_PROG_CC_C_O

AC_LANG([C])


AC_SUBST(CFLAGS)
PKG_CHECK_MODULES(FONTVIEW, [gtk+-3.0 >= 3.12.0 freetype2 >= 9.10.0 harfbuzz >= 0.6.0 pangoft2])
AC_C_CONST

_save_libs="$LIBS"
_save_cflags="$CFLAGS"
LIBS="$LIBS $FONTVIEW_LIBS"
CFLAGS="$CFLAGS $FONTVIEW_CFLAGS"
AC_CHECK_FUNCS(pango_fc_font_map_set_config)
LIBS="$_save_libs"
CFLAGS="$_save_cflags"

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.19)

AC_CONFIG_FILES([Makefile
	  po/Makefile.in
	  ])
AC_OUTPUT
