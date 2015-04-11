bin_PROGRAMS = fontview
EXTRA_DIST = fontview.gresource.xml \
	     $(UI_FILES)

AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) \
	   -DLOCALEDIR=\""$(datadir)/locale"\"

fontview_LDADD = @FONTVIEW_LIBS@
fontview_CFLAGS = @FONTVIEW_CFLAGS@

UI_FILES = resources/mainwindow.ui \
	   resources/infowindow.ui

fontview-resources.c: fontview.gresource.xml $(UI_FILES)
	$(AM_V_GEN)glib-compile-resources --target=$@ --sourcedir=$(srcdir)/resources --generate-source --c-name fontview $(srcdir)/fontview.gresource.xml

fontview-resources.h: fontview.gresource.xml $(UI_FILES)
	$(AM_V_GEN)glib-compile-resources --target=$@ --sourcedir=$(srcdir)/resources --generate-header --c-name fontview $(srcdir)/fontview.gresource.xml

FONTVIEW_RESOURCES = fontview-resources.c fontview-resources.h

BUILT_SOURCES = $(FONTVIEW_RESOURCES) $(TYPES_SOURCE)

fontview_SOURCES = font-model.c font-view.c main.c $(FONTVIEW_RESOURCES)

noinst_HEADERS = font-model.h font-view.h

-include $(top_srcdir)/git.mk
