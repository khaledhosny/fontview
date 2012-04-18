INCLUDES = -I$(top_srcdir) -I$(includedir) \
	   -DLOCALEDIR=\""$(datadir)/locale"\"

fontview_LDADD = @FONTVIEW_LIBS@
fontview_CFLAGS = @FONTVIEW_CFLAGS@ \
		  -DPACKAGE_DATA_DIR=\""$(datadir)/fontview"\"

bin_PROGRAMS = fontview
fontview_SOURCES = font-model.c font-view.c main.c

noinst_HEADERS = font-model.h font-view.h
