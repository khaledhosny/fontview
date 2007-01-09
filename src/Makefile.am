
glade_DATA = mainwindow.glade
gladedir = $(datadir)/fontview/glade

fv_LDADD =	@FONTVIEW_LIBS@ @GTK_LIBS@
fv_CFLAGS =	@FONTVIEW_CFLAGS@ @GTK_CFLAGS@ -DPACKAGE_DATA_DIR=\""$(gladedir)"\"
bin_PROGRAMS = fv
fv_SOURCES = font-model.c font-view.c main.c

EXTRA_DIST = mainwindow.gladep

noinst_HEADERS = font-model.h font-view.h
