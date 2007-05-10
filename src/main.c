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
 
#include "config.h"

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glade/glade-xml.h>
#include "font-model.h"
#include "font-view.h"

GladeXML *xml;
GtkWidget *font;

enum {
	COLUMN_INT,
	COLUMN_STRING,
	N_COLUMNS
};

void render_size_changed (GtkComboBox *w, gpointer data);

void font_view_about (GtkWidget *w, gpointer data) {
	gtk_show_about_dialog (NULL, 
		"name", "Font View", 
		"version", VERSION, 
		"copyright", "Copyright © 2007, Alex Roberts", 
		"comments", "A font viewing utility.\nPart of the Serif font management project.",
		"license", "GNU General Public License 2.0\n\nSee COPYING for more information.", 
		NULL);
}

void font_view_info_window (GtkWidget *w, gpointer data) {
	GtkWidget *window, *close, *about;
	GtkWidget *name, *style, *version, *copyright, *desc, *file;
	GladeXML *infowindow;
	FontModel *model;
	gint result;
	
	infowindow = glade_xml_new ("mainwindow.glade", "infowindow", NULL);
	if (!xml) {
		infowindow = glade_xml_new (PACKAGE_DATA_DIR"/mainwindow.glade", "infowindow", NULL);
	}
	g_return_if_fail (infowindow);
	
	window = glade_xml_get_widget (infowindow, "infowindow");
	
	about = glade_xml_get_widget (infowindow, "about_button");
	g_signal_connect (about, "clicked", G_CALLBACK(font_view_about), NULL);
	
	model = font_view_get_model (FONT_VIEW (font));
	
	name = glade_xml_get_widget (infowindow, "name_label");
	style = glade_xml_get_widget (infowindow, "style_label");
	version = glade_xml_get_widget (infowindow, "version_label");
	copyright = glade_xml_get_widget (infowindow, "copyright_label");
	desc = glade_xml_get_widget (infowindow, "descr_label");
	file = glade_xml_get_widget (infowindow, "file_label");
	
	gtk_label_set_text (GTK_LABEL(name), model->family);
	gtk_label_set_text (GTK_LABEL(style), model->style);
	gtk_label_set_text (GTK_LABEL(version), model->version);
	gtk_label_set_text (GTK_LABEL(copyright), model->copyright);
	gtk_label_set_text (GTK_LABEL(desc), model->description);
	gtk_label_set_text (GTK_LABEL(file), model->file);
	
	result = gtk_dialog_run (GTK_DIALOG (window));
		
	gtk_widget_destroy (window);
	
}


void view_size_changed (GtkWidget *w, gdouble size) {
	GtkWidget *sizew;
	
	sizew = glade_xml_get_widget (xml, "render_size");
	//gtk_spin_button_set_value (GTK_SPIN_BUTTON(sizew), font_view_get_pt_size (FONT_VIEW(w)));
}

void render_text_changed (GtkEntry *w, gpointer data) {
	gchar *text = g_strdup ((gchar *)gtk_entry_get_text (w));
		
	font_view_set_text (FONT_VIEW(font), text);
	
	g_free (text);
}

void render_size_changed (GtkComboBox *w, gpointer data) {
	GtkTreeModel *sizes;
	GtkTreeIter iter;
	gint size;
	
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (w), &iter)) {
		sizes = gtk_combo_box_get_model (GTK_COMBO_BOX (w));
		gtk_tree_model_get (sizes, &iter, COLUMN_INT, &size, -1);
		font_view_set_pt_size (FONT_VIEW(font), size);
	}
	
}

void print_usage ()
{
    g_print ("\nUsage:\n\tfv <path_to_font>\n\n");
    exit(1);
}

GtkWidget *font_custom_handler (GladeXML *xml, gchar *func, gchar *name, gchar *s1, gchar *s2, gint i1, gint i2, gpointer data) {
	GtkWidget *w = NULL;
	
	if (g_strcasecmp ("font_view_new_with_model", func) == 0) {
		w = font_view_new_with_model ((gchar *)data);
		return w;
	}
	
	return NULL;
}

GtkListStore *fv_init_sizes (GtkListStore *sizes) {
	GtkTreeIter iter;
	gint i;	
	gint size_array[] = {6, 8, 9, 10, 12, 18, 24, 36, 48, 60, 72, 96, 112, 200};
	
	for (i = 0; i < G_N_ELEMENTS(size_array); i++) {
		gtk_list_store_append (sizes, &iter);
		gtk_list_store_set (sizes, &iter, 
					COLUMN_INT, size_array[i], 
					COLUMN_STRING, g_strdup_printf ("  %dpt", size_array[i]), 
					-1);
	}
	
	return sizes;
}

int main (int argc, char *argv[]) {
	GtkWidget *w, *entry, *size;
	GtkListStore *sizes;
	GtkCellRenderer *renderer = NULL;
	gchar *str;
	
	gtk_init (&argc, &argv);

	if (!argv[1]) {
		print_usage();
		return 1;
	}	
	
	glade_set_custom_handler (font_custom_handler, argv[1]);
	xml = glade_xml_new ("mainwindow.glade", "mainwindow", NULL);
	if (!xml) {
		xml = glade_xml_new (PACKAGE_DATA_DIR"/mainwindow.glade", "mainwindow", NULL);
	}
	g_return_if_fail (xml);
	
	glade_xml_signal_autoconnect (xml);

	font = glade_xml_get_widget (xml, "font-view");
	g_signal_connect (font, "size-changed", G_CALLBACK(view_size_changed), NULL);
	gtk_widget_show (font);

	entry = glade_xml_get_widget (xml, "render_str");
	str = font_view_get_text(FONT_VIEW(font));
	gtk_entry_set_text (GTK_ENTRY(entry), str);
	g_free (str);
	g_signal_connect (entry, "changed", G_CALLBACK(render_text_changed), NULL);
	
	w = glade_xml_get_widget (xml, "info_button");
	g_signal_connect (w, "clicked", G_CALLBACK(font_view_info_window), NULL);

	/* Set up the size selection combo box */
	sizes = fv_init_sizes (gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING));

	size = glade_xml_get_widget (xml, "size_combo");
	g_signal_connect (size, "changed", G_CALLBACK(render_size_changed), NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (size), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (size), renderer, "text", 1);

	gtk_combo_box_set_model (GTK_COMBO_BOX (size), GTK_TREE_MODEL (sizes));
	gtk_combo_box_set_active (GTK_COMBO_BOX (size), 8);
	
	gtk_main();

	return 0;
}

