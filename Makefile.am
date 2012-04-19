SUBDIRS = data src po

desktopdir = $(datadir)/applications
desktop_in_files = fontview.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = $(desktop_in_files)

@INTLTOOL_DESKTOP_RULE@
