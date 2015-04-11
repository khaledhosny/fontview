SUBDIRS = po

bin_PROGRAMS = fontview

noinst_HEADERS = src/font-model.h src/font-view.h

desktopdir = $(datadir)/applications
desktop_in_files = data/fontview.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

UI_FILES = data/mainwindow.ui \
	   data/infowindow.ui

fontview-resources.c: data/fontview.gresource.xml $(UI_FILES)
	$(AM_V_GEN)glib-compile-resources --target=$@ --sourcedir=$(top_srcdir)/data --generate-source --c-name fontview $<

fontview-resources.h: data/fontview.gresource.xml $(UI_FILES)
	$(AM_V_GEN)glib-compile-resources --target=$@ --sourcedir=$(top_srcdir)/data --generate-header --c-name fontview $<

FONTVIEW_RESOURCES = fontview-resources.c fontview-resources.h

BUILT_SOURCES = $(FONTVIEW_RESOURCES)

fontview_LDADD = $(FONTVIEW_LIBS)
fontview_CFLAGS = $(FONTVIEW_CFLAGS) \
		  -DLOCALEDIR=\""$(datadir)/locale"\"


fontview_SOURCES = src/font-model.c src/font-view.c src/main.c $(FONTVIEW_RESOURCES)

EXTRA_DIST = $(desktop_in_files) \
	     data/fontview.gresource.xml \
	     $(UI_FILES)

MAINTAINERCLEANFILES = \
  $(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
  $(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
  $(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN)

-include $(top_srcdir)/git.mk
