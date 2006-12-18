/*
 * main.c
 *
 * GTK+ widget to display and handle fonts
 *
 * - Alex Roberts, 2006
 *
 */
 
 
#include <gtk/gtk.h>
#include "font-model.h"
#include "font-view.h"

void view_sized (GtkWidget *w, gdouble size) {
	g_print ("signal! FontView changed font size to %.2fpt.\n", size);
}

int main (int argc, char *argv[]) {
	GtkWidget *window;
	GtkWidget *font;
	
	gtk_init (&argc, &argv);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_default_size (GTK_WINDOW(window), 490, 200);
	gtk_window_set_title (GTK_WINDOW(window), "FontView");
	
	//model = FONT_MODEL(font_model_new ("/home/bse/src/georgia.ttf"));
	font = font_view_new_with_model (argv[1]);
	g_signal_connect (font, "sized", G_CALLBACK (view_sized), NULL);
	
	gtk_container_add (GTK_CONTAINER (window), font);

	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	
	gtk_widget_show_all (window);
	
	gtk_main();

	return 0;
}
