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
#include <hb-ft.h>
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

static gboolean font_view_expose (GtkWidget *view, GdkEventExpose *event);
static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e);

static void font_view_class_init (FontViewClass *klass) {
    GObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = G_OBJECT_CLASS (klass);

    widget_class = GTK_WIDGET_CLASS (klass);
    widget_class->expose_event = font_view_expose;
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

    /* default string to render */
    priv->text = _("How quickly daft jumping zebras vex.");

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
    GtkStyle *style;
    GtkAllocation *allocation;
    gint width, height;
    gdouble y, x, xx;
    FontViewPrivate *priv;
    gchar *title;
    gint p_height, i, rtl;

    priv = FONT_VIEW_GET_PRIVATE (FONT_VIEW(w));

    gtk_widget_get_allocation (w, allocation);
    width = allocation->width;
    height = allocation->height;

    style = gtk_rc_get_style (GTK_WIDGET (w));

    cairo_rectangle (cr, 0, 0, width, height);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_paint (cr);
    cairo_stroke (cr);

    cairo_set_source_rgb (cr, 1, 0.3, 0.3);
    cairo_set_line_width (cr, 1.0);

    cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);

    /* position text in the center */
    y = height/2+20;
    x = floor (width /2 * 0.1);

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
        rtl = ISRTL(pango_find_base_dir (priv->text, -1));

        cairo_font_face_t *cr_face = cairo_ft_font_face_create_for_ft_face (priv->model->ft_face, 0);
        cairo_set_font_face (cr, cr_face);
        cairo_set_font_size (cr, priv->size);

        cairo_scaled_font_t *cr_scaled_font = cairo_get_scaled_font (cr);
        FT_Face ft_face = cairo_ft_scaled_font_lock_face (cr_scaled_font);

        hb_font_t *hb_font = hb_ft_font_create (ft_face, NULL);
        hb_buffer_t *hb_buffer = hb_buffer_create ();

        //hb_buffer_set_direction (hb_buffer, rtl ? HB_DIRECTION_RTL: HB_DIRECTION_LTR);
        //hb_buffer_set_script (hb_buffer, hb_script_from_string ("arab"));
        //hb_buffer_set_language (hb_buffer, hb_language_from_string ("ar"));
        int length = strlen(priv->text);
        hb_buffer_add_utf8 (hb_buffer, priv->text, length, 0, length);
        hb_shape (hb_font, hb_buffer, NULL, 0);

        int num_glyphs = hb_buffer_get_length (hb_buffer);
        hb_glyph_info_t *hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
        hb_glyph_position_t *hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);

        cairo_glyph_t glyphs[num_glyphs];
        xx = x;
        for (i = 0; i < num_glyphs; i++) {
            glyphs[i].index = hb_glyph->codepoint;
            glyphs[i].x = xx + (hb_position->x_offset/64);
            glyphs[i].y = y -  (hb_position->y_offset/64);
            xx += (hb_position->x_advance/64);
            hb_glyph++;
            hb_position++;
        }

        if (rtl) {
            for (i = 0; i < num_glyphs; i++) {
                glyphs[i].x += width-x-xx;
            }
        }

        hb_buffer_destroy (hb_buffer);
        hb_font_destroy (hb_font);
        cairo_ft_scaled_font_unlock_face (cr_scaled_font);

        gdk_cairo_set_source_color (cr, style->fg);
        cairo_show_glyphs (cr, glyphs, num_glyphs);
    }


    /* draw header bar */
    title = g_strdup_printf ("%s %s - %.0fpt",
            priv->model->family,
            priv->model->style,
            priv->size);
    PangoLayout *layout = gtk_widget_create_pango_layout (w, title);
    g_free (title);

    pango_layout_get_pixel_size (layout, NULL, &p_height);
    cairo_rectangle (cr, 0, 0, width, p_height + 1);
    gdk_cairo_set_source_color (cr, style->bg);
    cairo_fill (cr);

    gdk_cairo_set_source_color (cr, style->fg);
    cairo_move_to (cr, 5, 1);
    pango_cairo_show_layout (cr, layout);

}


static gboolean font_view_expose (GtkWidget *w, GdkEventExpose *event) {
    cairo_t *cr;
    GdkWindow *window;

    window = gtk_widget_get_window (w);

    cr = gdk_cairo_create (window);
    cairo_rectangle (cr,
            event->area.x,
            event->area.y,
            event->area.width,
            event->area.height);
    cairo_clip (cr);

    render (w, cr);

    cairo_destroy (cr);
    cr = NULL;

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
    GdkRegion *region;
    GdkWindow *window;

    widget = GTK_WIDGET (view);
    window = gtk_widget_get_window (widget);

    if (!window)
        return;

    region = gdk_drawable_get_clip_region (window);
    gdk_window_invalidate_region (window, region, TRUE);
    gdk_window_process_updates (window, TRUE);

    gdk_region_destroy (region);
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

gchar *font_view_get_text (FontView *view) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (view);
    return g_strdup(priv->text);
}

void font_view_set_text (FontView *view, gchar *text) {
    FontViewPrivate *priv;

    priv = FONT_VIEW_GET_PRIVATE (view);

    if (g_strcasecmp (priv->text, text) == 0)
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

