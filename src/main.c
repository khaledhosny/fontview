/*
 * main.c
 *
 * GTK+ widget to display and handle fonts
 *
 * - Alex Roberts, 2006
 *
 */

/*
 * FontView Test - font viewing widget test app
 * Part of the Fontable Project
 * Copyright (C) 2006 Alex Roberts
 * Copyright (C) 2006 Jon Phillips, <jon@rejon.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA 02111-1307 USA
 * 
 */
 
 
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glade/glade-xml.h>
#include "font-model.h"
#include "font-view.h"

GladeXML *xml;
GtkWidget *font;

void render_size_changed (GtkSpinButton *w, gpointer data);

void view_size_changed (GtkWidget *w, gdouble size) {
	GtkWidget *sizew;
	g_print ("signal! FontView changed font size to %.2fpt.\n", size);
	
	sizew = glade_xml_get_widget (xml, "render_size");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(sizew), font_view_get_pt_size (FONT_VIEW(w)));
}

void render_text_changed (GtkEntry *w, gpointer data) {
	gchar *text = (gchar *)gtk_entry_get_text (w);
	
	g_message ("text_changed: %s", text);
	
	font_view_set_text (FONT_VIEW(font), text);
}

void render_size_changed (GtkSpinButton *w, gpointer data) {
	gdouble size = gtk_spin_button_get_value (w);
	
	font_view_set_pt_size (FONT_VIEW(font), size);
}

void print_usage ()
{
    g_print ("\nUsage:\n\tfv <path_to_font>\n\n");
    exit(1);
}

GtkWidget *font_custom_handler (GladeXML *xml, gchar *func, gchar *name, gchar *s1, gchar *s2, gint i1, gint i2, gpointer data) {
	GtkWidget *w = NULL;
	
	g_message ("font custom handler.");
	
	if (g_strcasecmp ("font_view_new_with_model", func) == 0) {
		g_message ("fch: yo! %s", (gchar *)data);
		w = font_view_new_with_model ((gchar *)data);
		return w;
	}
	
	return NULL;
}

int main (int argc, char *argv[]) {
	GtkWidget *entry, *size;
	
	gtk_init (&argc, &argv);

	if (!argv[1]) {
		print_usage();
		return 1;
	}	
	
	glade_set_custom_handler (font_custom_handler, argv[1]);
	xml = glade_xml_new ("mainwindow.glade", NULL, NULL);
	if (!xml) {
		xml = glade_xml_new (PACKAGE_DATA_DIR"/mainwindow.glade", NULL, NULL);
	}
	g_return_if_fail (xml);
	
	glade_xml_signal_autoconnect (xml);

	font = glade_xml_get_widget (xml, "font-view");
	g_signal_connect (font, "size-changed", G_CALLBACK(view_size_changed), NULL);
	gtk_widget_show (font);

	entry = glade_xml_get_widget (xml, "render_str");
	gtk_entry_set_text (GTK_ENTRY(entry), font_view_get_text(FONT_VIEW(font)));
	g_signal_connect (entry, "changed", G_CALLBACK(render_text_changed), NULL);
	
	size = glade_xml_get_widget (xml, "render_size");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(size), font_view_get_pt_size (FONT_VIEW(font)));
	g_signal_connect (size, "value-changed", G_CALLBACK(render_size_changed), font);
		
	gtk_main();

	return 0;
}
