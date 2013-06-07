/*
 * font-view.c
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

#include <math.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <cairo/cairo-ft.h>
#include <hb-ft.h>
#include <hb-glib.h>
#include "font-view.h"

G_DEFINE_TYPE (FontView, font_view, GTK_TYPE_DRAWING_AREA);

#define FONT_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                    FONT_VIEW_TYPE, FontViewPrivate))
#define ISRTL(A)                   ((A==PANGO_DIRECTION_RTL)|| \
                                    (A==PANGO_DIRECTION_WEAK_RTL)|| \
                                    (A==PANGO_DIRECTION_TTB_LTR))

enum {
    BASELINE,
    ASCENDER,
    DESCENDER,
    XHEIGHT,
    TEXT,
    N_EXTENTS
};

typedef struct _FontViewPrivate FontViewPrivate;

struct _FontViewPrivate {
    gboolean extents[N_EXTENTS];

    gdouble ascender;
    gdouble descender;
    gdouble xheight;

    gdouble size;

    gchar *text;

    FontModel *model;
};

static void font_view_redraw (FontView *view);

static gboolean font_view_draw (GtkWidget *view, cairo_t *cr);
static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e);

static void font_view_class_init (FontViewClass *klass) {
    GObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = G_OBJECT_CLASS (klass);

    widget_class = GTK_WIDGET_CLASS (klass);
    widget_class->draw = font_view_draw;
    widget_class->button_release_event = font_view_clicked;

    g_type_class_add_private (object_class, sizeof (FontViewPrivate));
}

static void font_view_init (FontView *view) {
    FontViewPrivate *priv;
    gint i;

    priv = FONT_VIEW_GET_PRIVATE(view);

    for (i = 0; i < G_N_ELEMENTS(priv->extents); i++) {
        priv->extents[i] = FALSE;
    }
    priv->extents[TEXT] = TRUE;
    priv->size = 50;

    gtk_widget_add_events (GTK_WIDGET (view),
            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
}

GtkWidget *font_view_new_with_model (gchar *font) {
    FontView *view;
    FontViewPrivate *priv;

    g_return_val_if_fail (font, NULL);

    view = g_object_new (FONT_VIEW_TYPE, NULL);
    priv = FONT_VIEW_GET_PRIVATE(view);

    priv->model = FONT_MODEL(font_model_new (font));

    if (priv->model->sample) {
        priv->text = g_strdup (priv->model->sample);
        priv->extents[TEXT] = TRUE;
    }

    return GTK_WIDGET(view);
}

FontModel *font_view_get_model (FontView *view) {
    FontViewPrivate *priv;
    priv = FONT_VIEW_GET_PRIVATE(view);
    return priv->model;
}

static void render (GtkWidget *w, cairo_t *cr) {
    GtkAllocation allocation;
    GdkRGBA fg, bg;

    FontViewPrivate *priv = FONT_VIEW_GET_PRIVATE (FONT_VIEW(w));

    gtk_widget_get_allocation (w, &allocation);
    gint width = allocation.width;
    gint height = allocation.height;

    GtkStyleContext *style = gtk_widget_get_style_context (GTK_WIDGET (w));
    gtk_style_context_get_color (style, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_get_background_color (style, GTK_STATE_FLAG_NORMAL, &bg);

    cairo_rectangle (cr, 0, 0, width, height);
    cairo_set_source_rgba (cr, 1, 1, 1, 1);
    cairo_paint (cr);
    cairo_stroke (cr);

    cairo_set_source_rgba (cr, 1, 0.3, 0.3, 1);
    cairo_set_line_width (cr, 1.0);

    cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 1);

    /* position text in the center */
    gdouble indent = floor (width /2 * 0.1);
    gdouble y = height/2+20;
    gdouble x = indent;

    /* baseline */
    if (priv->extents[BASELINE]) {
        cairo_move_to (cr, x, y);
        cairo_line_to (cr, width - x, y);
        cairo_stroke (cr);
    }

    /* descender height */
    if (priv->extents[DESCENDER] && priv->descender != 0.0) {
        cairo_move_to (cr, x, y - priv->descender);
        cairo_line_to (cr, width - x, y - priv->descender);
        cairo_stroke (cr);
    }

    /* ascender height */
    if (priv->extents[ASCENDER] && priv->ascender != 0.0) {
        cairo_move_to (cr, x, y - priv->ascender);
        cairo_line_to (cr, width - x, y - priv->ascender);
        cairo_stroke (cr);
    }

    /* x-height */
    if (priv->extents[XHEIGHT] && priv->xheight != 0.0) {
        cairo_move_to (cr, x, y - priv->xheight);
        cairo_line_to (cr, width - x, y - priv->xheight);
        cairo_stroke (cr);
    }

    /* display sample text */
    if (priv->extents[TEXT]) {
        PangoDirection base_dir = pango_find_base_dir (priv->text, -1);

        cairo_font_face_t *cr_face = cairo_ft_font_face_create_for_ft_face (priv->model->ft_face, 0);
        cairo_set_font_face (cr, cr_face);
        /* our size is in points, so we convert to cairo user units */
        cairo_set_font_size (cr, priv->size * 96 / 72.0);

        cairo_scaled_font_t *cr_scaled_font = cairo_get_scaled_font (cr);
        FT_Face ft_face = cairo_ft_scaled_font_lock_face (cr_scaled_font);

        hb_font_t *hb_font = hb_ft_font_create (ft_face, NULL);

        int total_num_glyphs = 0;
        cairo_glyph_t *glyphs = NULL;

        /* We abuse pango itemazation to split text into script and direction
         * runs, since we use our fonts directly no through pango, we don't
         * bother changing the default font, but we disable font fallback as
         * pango will split runs at font change */
        PangoContext *context = gtk_widget_get_pango_context (w);
        PangoAttrList *attr_list = pango_attr_list_new ();
        PangoAttribute *fallback_attr = pango_attr_fallback_new (FALSE);
        pango_attr_list_insert (attr_list, fallback_attr);
        GList *items = pango_itemize_with_base_dir (context, base_dir,
                                                    priv->text, 0, strlen (priv->text),
                                                    attr_list, NULL);

        /* reorder the items in the visual order */
        items = pango_reorder_items (items);

        for (GList *l = items; l; l = l->next) {
            PangoItem *item = l->data;
            PangoAnalysis analysis = item->analysis;

            hb_script_t script = hb_glib_script_to_script (analysis.script);
            hb_language_t lang = hb_language_from_string (pango_language_to_string (analysis.language), -1);
            hb_direction_t dir = HB_DIRECTION_LTR;
            if (analysis.level % 2)
                dir = HB_DIRECTION_RTL;

            hb_buffer_t *hb_buffer = hb_buffer_create ();
            hb_buffer_add_utf8 (hb_buffer, priv->text, -1, item->offset, item->length);
            hb_buffer_set_script (hb_buffer, script);
            hb_buffer_set_language (hb_buffer, lang);
            hb_buffer_set_direction (hb_buffer, dir);

            hb_shape (hb_font, hb_buffer, NULL, 0);

            int num_glyphs = hb_buffer_get_length (hb_buffer);
            hb_glyph_info_t *hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
            hb_glyph_position_t *hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);

            glyphs = g_renew (cairo_glyph_t, glyphs, total_num_glyphs + num_glyphs);

            for (int i = 0; i < num_glyphs; i++) {
                glyphs[total_num_glyphs + i].index = hb_glyph->codepoint;
                glyphs[total_num_glyphs + i].x = x + (hb_position->x_offset/64);
                glyphs[total_num_glyphs + i].y = y - (hb_position->y_offset/64);
                x += (hb_position->x_advance/64);
                y -= (hb_position->y_advance/64);

                hb_glyph++;
                hb_position++;
            }

            total_num_glyphs += num_glyphs;

            hb_buffer_destroy (hb_buffer);
        }

        g_list_foreach (items, (GFunc) pango_item_free, NULL);
        g_list_free (items);

        hb_font_destroy (hb_font);
        cairo_ft_scaled_font_unlock_face (cr_scaled_font);

        gdk_cairo_set_source_rgba (cr, &fg);

        /* right align if base direction is right to left */
        if (ISRTL (base_dir)) {
            for (int i = 0; i < total_num_glyphs; i++) {
                glyphs[i].x += width - x - indent;
            }
        }

        cairo_show_glyphs (cr, glyphs, total_num_glyphs);
        g_free (glyphs);
    }
}


static gboolean font_view_draw (GtkWidget *w, cairo_t *cr) {
    render (w, cr);

    return FALSE;
}

static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (w);

    priv->extents[BASELINE] = !priv->extents[BASELINE];
    priv->extents[ASCENDER] = !priv->extents[ASCENDER];
    priv->extents[DESCENDER] = !priv->extents[DESCENDER];
    priv->extents[XHEIGHT] = !priv->extents[XHEIGHT];

    font_view_redraw (FONT_VIEW(w));

    return FALSE;
}

static void font_view_redraw (FontView *view) {
    GtkWidget *widget;
    GdkWindow *window;
    cairo_region_t *region;

    widget = GTK_WIDGET (view);
    window = gtk_widget_get_window (widget);

    if (!window)
        return;

    region = gdk_window_get_clip_region (window);
    gdk_window_invalidate_region (window, region, TRUE);
    gdk_window_process_updates (window, TRUE);

    cairo_region_destroy (region);
}

void font_view_set_pt_size (FontView *view, gdouble size) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (view);

    if (priv->size == size)
        return;

    priv->size = size;
    priv->xheight = priv->model->xheight / priv->model->units_per_em * size;
    priv->ascender = priv->model->ascender / priv->model->units_per_em * size;
    priv->descender = priv->model->descender / priv->model->units_per_em * size;
    priv->extents[TEXT] = TRUE;

    font_view_redraw (view);
}

gdouble font_view_get_pt_size (FontView *view) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (view);
    return priv->size;
}

gchar *font_view_get_text (FontView *view) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (view);
    return g_strdup(priv->text);
}

void font_view_set_text (FontView *view, gchar *text) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (view);

    if (g_strcmp0 (priv->text, text) == 0)
        return;

    priv->text = NULL;

    priv->text = g_strdup(text);
    priv->extents[TEXT] = TRUE;

    font_view_redraw (view);
}

void font_view_rerender (FontView *view) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE(view);
    priv->model = FONT_MODEL(font_model_new (priv->model->file));
    priv->extents[TEXT] = TRUE;
    font_view_redraw (view);
}

