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
 * Copyright (C) 2010 Khaled Hosny, <khaledhosny@eglug.org>
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
#include <glib/gi18n.h>
#include "font-view.h"

#define GET_GBOPJECT(A,B) GTK_WIDGET(gtk_builder_get_object(A,B));

static void
font_view_about (GtkWidget *w,
                 gpointer parent)
{
    static const gchar * const authors[] = {
        "Alex Roberts <alex@redprocess.com>",
        "Jon Phillips <jon@rejon.org>",
        "Khaled Hosny <khaledhosnt@eglug.org>",
        NULL
    };

    gtk_show_about_dialog (GTK_WINDOW (parent),
        "name", _("Font View"),
        "authors", authors,
        "version", VERSION,
        "copyright", _("Copyright \u00A9 2007, Alex Roberts\nCopyright \u00A9 2010, Khaled Hosny"),
        "comments", _("A font viewing utility.\nPart of the Serif font management project."),
        "license", _("GNU General Public License 2.0\n\nSee COPYING for more information."),
        "translator-credits", _("translator-credits"),
        NULL);
}

static void
font_view_info_window (GtkWidget *w,
                       gpointer data)
{
    GtkWidget *window, *about;
    GtkWidget *name, *style, *version, *copyright, *desc, *file;
    GtkBuilder *infowindow;
    FontModel *model;
    GtkWindow *parent;

    infowindow = gtk_builder_new ();
    gtk_builder_add_from_resource (infowindow, "/org/serif/fontview/infowindow.ui", NULL);
    gtk_builder_connect_signals (infowindow, NULL);

    parent = GTK_WINDOW (gtk_widget_get_toplevel (w));
    window = GET_GBOPJECT (infowindow, "infowindow");

    about = GET_GBOPJECT (infowindow, "about_button");
    g_signal_connect (about, "clicked", G_CALLBACK(font_view_about), parent);

    model = font_view_get_model (FONT_VIEW (data));

    name = GET_GBOPJECT (infowindow, "name_label");
    style = GET_GBOPJECT (infowindow, "style_label");
    version = GET_GBOPJECT (infowindow, "version_label");
    copyright = GET_GBOPJECT (infowindow, "copyright_label");
    desc = GET_GBOPJECT (infowindow, "descr_label");
    file = GET_GBOPJECT (infowindow, "file_label");

    gtk_label_set_text (GTK_LABEL(name), model->family);
    gtk_label_set_text (GTK_LABEL(style), model->style);
    gtk_label_set_text (GTK_LABEL(version), model->version);
    gtk_label_set_text (GTK_LABEL(copyright), model->copyright);
    gtk_label_set_text (GTK_LABEL(desc), model->description);
    gtk_label_set_text (GTK_LABEL(file), model->file);

    gtk_window_set_transient_for (GTK_WINDOW (window), parent);
    gtk_dialog_run (GTK_DIALOG (window));

    gtk_widget_destroy (window);
}


static void
render_text_changed (GtkEntry *w,
                     gpointer data)
{
    gchar *text = g_strdup ((gchar *)gtk_entry_get_text (w));

    font_view_set_text (FONT_VIEW(data), text);

    g_free (text);
}

static void
render_size_changed (GtkSpinButton *w,
                     gpointer data)
{
    GtkWidget* window;
    FontView* view;
    FontModel *model;
    gint size;
    gchar *title;

    view = FONT_VIEW(data);
    size = gtk_spin_button_get_value_as_int (w);
    font_view_set_pt_size (view, size);

    model = font_view_get_model (view);
    title = g_strdup_printf ("%s %s – %.0fpt",
                             model->family,
                             model->style,
                             font_view_get_pt_size (view));

    window = gtk_widget_get_toplevel (GTK_WIDGET (data));
    if (gtk_widget_is_toplevel (window))
        gtk_window_set_title (GTK_WINDOW (window), title);

    g_free (title);
}

static void
namedinstance_changed (GtkComboBox *w,
                       gpointer data)
{
    gint index = gtk_combo_box_get_active (w);
    font_view_select_named_instance (FONT_VIEW (data), index);
}

static void
render_file_changed (GFileMonitor *monitor,
                     GFile *file,
                     GFile *other_file,
                     GFileMonitorEvent event,
                     gpointer data)
{
    if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
        font_view_rerender (FONT_VIEW(data));
}

static void
setup_mmvar (GtkBuilder* window, GtkWidget* fontview) {
    GtkWidget* namedinstance;
    FontModel* model;

    model = font_view_get_model (FONT_VIEW (fontview));
    if (model->mmvar) {
        FT_MM_Var* mmvar = model->mmvar;
        namedinstance = GET_GBOPJECT (window, "named-instance");
        gtk_widget_set_visible (namedinstance, TRUE);
        for (FT_UInt i = 0; i < mmvar->num_namedstyles; i++) {
            FT_Var_Named_Style style = mmvar->namedstyle[i];
            gchar* name = get_font_name (model->ft_face, style.strid);
            gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (namedinstance), name);
//          if (g_strcmp0 (name, "Regular") == 0) {
//              gtk_combo_box_set_active (GTK_COMBO_BOX (namedinstance), i);
//          }
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (namedinstance), 3);
    }
}

void
print_usage (void)
{
    g_print ("\nUsage:\n\tfontview <path_to_font>\n\n");
    exit(1);
}

int
main (int argc, char *argv[]) {
    GtkBuilder *mainwindow;
    GtkWidget *w, *entry, *sizew, *container, *font, *namedinstance;
    GFile *file;
    GFileMonitor *monitor;
    gchar *text;

    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

    gtk_init (&argc, &argv);

    if (!argv[1]) {
        print_usage();
        return 1;
    }

    mainwindow = gtk_builder_new ();
    gtk_builder_add_from_resource (mainwindow, "/org/serif/fontview/mainwindow.ui", NULL);
    gtk_builder_connect_signals (mainwindow, NULL);

    font = font_view_new_with_model (argv[1]);
    if (font == NULL)
        exit (EXIT_FAILURE);
    container = GET_GBOPJECT (mainwindow, "font-view");
    gtk_container_add (GTK_CONTAINER (container), font);

    gtk_widget_show (font);
    gtk_widget_show (container);

    entry = GET_GBOPJECT (mainwindow, "render_str");
    text = font_view_get_text (FONT_VIEW (font));
    if (text)
        gtk_entry_set_text (GTK_ENTRY (entry), text);
    g_signal_connect (entry, "changed", G_CALLBACK(render_text_changed), font);
    g_signal_emit_by_name (entry, "changed");

    w = GET_GBOPJECT (mainwindow, "info_button");
    g_signal_connect (w, "clicked", G_CALLBACK(font_view_info_window), font);

    sizew = GET_GBOPJECT (mainwindow, "size_spin");
    g_signal_connect (sizew, "value-changed", G_CALLBACK(render_size_changed), font);
    g_signal_emit_by_name (sizew, "value-changed");

    file = g_file_new_for_path(argv[1]);
    monitor = g_file_monitor_file (file, 0, NULL, NULL);
    g_signal_connect (monitor, "changed", G_CALLBACK(render_file_changed), font);

    namedinstance = GET_GBOPJECT (mainwindow, "named-instance");
    g_signal_connect (namedinstance, "changed", G_CALLBACK(namedinstance_changed), font);

    setup_mmvar (mainwindow, font);

    gtk_main();

    return 0;
}

