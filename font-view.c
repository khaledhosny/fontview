/*
 * FontView - font viewing app
 * Part of the Fontable Project
 * Copyright (C) 2006 Alex Roberts
 * Copyright (C) 2010-2018 Khaled Hosny, <khaledhosny@eglug.org>
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
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <pango/pangofc-fontmap.h>
#include <hb-ft.h>
#include <hb-glib.h>
#include <fribidi.h>
#include "font-view.h"

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

G_DEFINE_TYPE_WITH_PRIVATE (FontView, font_view, GTK_TYPE_DRAWING_AREA);

static void font_view_redraw (FontView *view);

static gboolean font_view_draw (GtkWidget *view, cairo_t *cr);
static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e);

static void font_view_class_init (FontViewClass *klass) {
    GtkWidgetClass *widget_class;

    widget_class = GTK_WIDGET_CLASS (klass);
    widget_class->draw = font_view_draw;
    widget_class->button_release_event = font_view_clicked;
}

static void font_view_init (FontView *view) {
    FontViewPrivate *priv;
    gint i;

    priv = font_view_get_instance_private(view);

    for (i = 0; i < G_N_ELEMENTS(priv->extents); i++) {
        priv->extents[i] = FALSE;
    }
    priv->extents[TEXT] = TRUE;
    priv->size = 50;

    gtk_widget_add_events (GTK_WIDGET (view),
            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
}

GtkWidget *font_view_new () {
    return g_object_new (FONT_VIEW_TYPE, NULL);
}

GtkWidget *font_view_new_with_model (gchar *font) {
    FontView *view;
    FontViewPrivate *priv;
    FontModel *model;

    g_return_val_if_fail (font, NULL);

    view = g_object_new (FONT_VIEW_TYPE, NULL);
    priv = font_view_get_instance_private (view);

    model = FONT_MODEL(font_model_new (font));
    if (model == NULL)
        return NULL;

    priv->model = model;

    if (priv->model->sample) {
        priv->text = g_strdup (priv->model->sample);
        priv->extents[TEXT] = TRUE;
    }

    return GTK_WIDGET(view);
}

void font_view_set_model (FontView *view, FontModel *model) {
    FontViewPrivate *priv;
    priv = font_view_get_instance_private (view);

    if (IS_FONT_MODEL(model)) {
        priv->model = model;
    }
}

FontModel *font_view_get_model (FontView *view) {
    FontViewPrivate *priv;
    priv = font_view_get_instance_private (view);
    return priv->model;
}

static void
show_layout_with_color (cairo_t *cr,
                        PangoLayout *layout,
                        FontViewPrivate *priv,
                        double x,
                        double y)
{
    int x_position = 0;
    PangoLayoutIter *iter;
    FontModel *model;

    model = priv->model;

    iter = pango_layout_get_iter (layout);

    do {
        PangoLayoutRun *run = pango_layout_iter_get_run (iter);
        if (run) {
            cairo_font_face_t *cr_face;
            PangoGlyphString* glyphs;
            PangoGlyphInfo *gi;
            double cx, cy;

            glyphs = run->glyphs;

            cr_face = cairo_ft_font_face_create_for_ft_face (model->ft_face, 0);
            cairo_set_font_face (cr, cr_face);
            /* our size is in points, so we convert to cairo user units */
            cairo_set_font_size (cr, priv->size * 96 / 72.0);

            for (int i = 0; i < glyphs->num_glyphs; i++) {
                cairo_glyph_t glyph;

                gi = &glyphs->glyphs[i];
                if (gi->glyph != PANGO_GLYPH_EMPTY) {
                    gconstpointer key = GINT_TO_POINTER (gi->glyph);

                    cx = x + (double)(x_position + gi->geometry.x_offset) / PANGO_SCALE;
                    cy = y + (double)(gi->geometry.y_offset) / PANGO_SCALE;

                    if (g_hash_table_contains (model->color.glyphs, key)) {
                        ColorGlyph *color_glyph = g_hash_table_lookup (model->color.glyphs, key);
                        for (int j = 0; j < color_glyph->num_layers; j++) {
                            ColorLayer layer = color_glyph->layers[j];
                            Color color = layer.colors[model->color.palette];
                            glyph.index = layer.gid;
                            glyph.x = cx;
                            glyph.y = cy;

                            cairo_set_source_rgba (cr, color.r, color.g, color.b, color.a);
                            cairo_show_glyphs (cr, &glyph, 1);
                        }
                    } else {
                        glyph.index = gi->glyph & PANGO_GLYPH_UNKNOWN_FLAG ? 0 : gi->glyph;
                        glyph.x = cx;
                        glyph.y = cy;

                        cairo_set_source_rgba (cr, 0, 0, 0, 1);
                        cairo_show_glyphs (cr, &glyph, 1);
                    }
                }

                x_position += gi->geometry.width;
            }
        }
    } while (pango_layout_iter_next_run (iter));

    cairo_set_source_rgba (cr, 0, 0, 0, 1);

    pango_layout_iter_free (iter);
}

static gboolean
is_rtl_paragraph (gchar *text)
{
    FriBidiChar* unicode = g_new (FriBidiChar, strlen (text));
    FriBidiStrIndex len = fribidi_charset_to_unicode (FRIBIDI_CHAR_SET_UTF8,
                                                      text, strlen (text),
                                                      unicode);
    FriBidiCharType *types = g_new (FriBidiCharType, len);
    fribidi_get_bidi_types (unicode, len, types);
    FriBidiParType par = fribidi_get_par_direction (types, len);

    g_free (unicode);
    g_free (types);

    return !!(par == FRIBIDI_PAR_RTL || par == FRIBIDI_PAR_WLTR);
}

static void render (GtkWidget *w, cairo_t *cr) {
    GtkAllocation allocation;

    FontViewPrivate *priv = font_view_get_instance_private (FONT_VIEW(w));

    gtk_widget_get_allocation (w, &allocation);
    gint width = allocation.width;
    gint height = allocation.height;

    cairo_rectangle (cr, 0, 0, width, height);
    cairo_set_source_rgba (cr, 1, 1, 1, 1);
    cairo_paint (cr);
    cairo_stroke (cr);

    cairo_set_source_rgba (cr, 1, 0.3, 0.3, 1);
    cairo_set_line_width (cr, 1.0);

    cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 1);

    /* position text in the center */
    gdouble indent = width / 2 / 10;
    gdouble y = height / 2 + 20;
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
        FontModel *model;
        PangoContext *context;
        PangoFontDescription *desc;
        PangoFontMap *fontmap;
        PangoLayout *layout;

        model = priv->model;

        fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
        pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (fontmap), model->config);
        context = pango_font_map_create_context (fontmap);

        desc = pango_font_description_new ();
#define UNTAG(tag) ((char)((tag)>>24)), ((char)((tag)>>16)), ((char)((tag)>>8)), ((char)(tag))
        if (model->mmcoords) {
            GString* variations = g_string_new ("");
            FT_UInt i;
            char *sep = "";

            for (i = 0; i < model->mmvar->num_axis; i++) {
                g_string_append_printf (variations, "%s%c%c%c%c=%g", sep,
                                        UNTAG(model->mmvar->axis[i].tag),
                                        model->mmcoords[i] / 65536.);
                sep = ",";
            }

            pango_font_description_set_variations (desc, variations->str);
            g_string_free (variations, TRUE);
        }
#undef UNTAG

        cairo_set_source_rgba (cr, 0, 0, 0, 1);

        pango_font_description_set_size (desc, priv->size * PANGO_SCALE);

        layout = pango_layout_new (context);
        pango_layout_set_text (layout, priv->text, -1);
        pango_layout_set_font_description (layout, desc);

#if 0
        gint baseline = pango_layout_get_baseline (layout) / PANGO_SCALE;
        /* Causes line breaks, but we don’t handle those. */
        pango_layout_set_width (layout, (width - indent * 2) * PANGO_SCALE);
        cairo_translate (cr, x, y - baseline);
#else
        if (is_rtl_paragraph (priv->text)) {
            gint layout_width;
            pango_layout_get_pixel_size (layout, &layout_width, NULL);
            x = width - x - layout_width;
        }
#endif

        if (!model->color.glyphs) {
            gint baseline = pango_layout_get_baseline (layout) / PANGO_SCALE;
            cairo_translate (cr, x, y - baseline);
            pango_cairo_update_context (cr, context);
            pango_cairo_show_layout (cr, layout);
        } else {
            show_layout_with_color (cr, layout, priv, x, y);
        }

        g_object_unref (layout);
        pango_font_description_free (desc);
        g_object_unref (context);
        g_object_unref (fontmap);
    }
}


static gboolean font_view_draw (GtkWidget *w, cairo_t *cr) {
    render (w, cr);

    return FALSE;
}

static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e) {
    FontViewPrivate *priv;

    priv = font_view_get_instance_private (FONT_VIEW (w));

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

    cairo_region_destroy (region);
}

gdouble font_view_get_pt_size (FontView *view) {
    FontViewPrivate *priv;

    priv = font_view_get_instance_private (view);
    return priv->size;
}

void font_view_set_pt_size (FontView *view, gdouble size) {
    FontViewPrivate *priv;

    priv = font_view_get_instance_private (view);

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

    priv = font_view_get_instance_private (view);
    return g_strdup(priv->text);
}

void font_view_set_text (FontView *view, gchar *text) {
    FontViewPrivate *priv;

    priv = font_view_get_instance_private (view);

    if (g_strcmp0 (priv->text, text) == 0)
        return;

    priv->text = NULL;

    priv->text = g_strdup(text);
    priv->extents[TEXT] = TRUE;

    font_view_redraw (view);
}

void font_view_select_named_instance (FontView *view, gint index)
{
    FontModel* model = font_view_get_model (FONT_VIEW (view));

    if (model->mmvar) {
        model->mmcoords = model->mmvar->namedstyle[index].coords;
    }

    font_view_redraw (view);
}

void font_view_set_palette (FontView *view, gint index)
{
    FontModel* model = font_view_get_model (FONT_VIEW (view));
    model->color.palette = index;
    font_view_redraw (view);
}

void font_view_rerender (FontView *view) {
    FontViewPrivate *priv;
    FontModel *model;

    priv = font_view_get_instance_private (view);
    model = FONT_MODEL(font_model_new (priv->model->file));
    if (model != NULL) {
        priv->model = model;
        priv->extents[TEXT] = TRUE;
        font_view_redraw (view);
    }
}

