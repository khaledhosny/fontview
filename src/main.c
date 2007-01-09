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

void view_sized (GtkWidget *w, gdouble size) {
	g_print ("signal! FontView changed font size to %.2fpt.\n", size);
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

int main (int argc, char *argv[]) {
	GtkWidget *window, *vbox, *entry, *size;
	
	gtk_init (&argc, &argv);
	
	xml = glade_xml_new ("mainwindow.glade", NULL, NULL);
	if (!xml) {
		xml = glade_xml_new (PACKAGE_DATA_DIR"/mainwindow.glade", NULL, NULL);
	}
	g_return_if_fail (xml);
	
	glade_xml_signal_autoconnect (xml);
	/*
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_default_size (GTK_WINDOW(window), 490, 200);
	gtk_window_set_title (GTK_WINDOW(window), "FontView");
	*/
	if (!argv[1]) {
		print_usage();
	}	
	
	font = font_view_new_with_model (argv[1]);
	g_signal_connect (font, "sized", G_CALLBACK(view_sized), NULL);
	
	vbox = glade_xml_get_widget (xml, "vbox1");
	gtk_box_pack_end_defaults (GTK_BOX(vbox), GTK_WIDGET(font));
	gtk_widget_show (GTK_WIDGET(font));
	
	entry = glade_xml_get_widget (xml, "render_str");
	gtk_entry_set_text (GTK_ENTRY(entry), font_view_get_text(FONT_VIEW(font)));
	
	size = glade_xml_get_widget (xml, "render_size");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(size), font_view_get_pt_size (FONT_VIEW(font)));
	g_signal_connect (size, "value-changed", G_CALLBACK(render_size_changed), NULL);
	/*
	gtk_container_add (GTK_CONTAINER (window), font);

	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	
	gtk_widget_show_all (window);
	*/
	
	gtk_main();

	return 0;
}
