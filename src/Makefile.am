
ui_DATA = mainwindow.ui infowindow.ui
uidir = $(datadir)/fontview/ui

fv_LDADD =	@FONTVIEW_LIBS@ @GTK_LIBS@
fv_CFLAGS =	@FONTVIEW_CFLAGS@ @GTK_CFLAGS@ -DPACKAGE_DATA_DIR=\""$(uidir)"\"
bin_PROGRAMS = fv
fv_SOURCES = font-model.c font-view.c main.c

noinst_HEADERS = font-model.h font-view.h
