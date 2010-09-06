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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include "font-view.h"

G_DEFINE_TYPE (FontView, font_view, GTK_TYPE_DRAWING_AREA);

#define FONT_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), FONT_VIEW_TYPE, FontViewPrivate))

#define ZOOM_LEVELS 30

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
	
	gdouble height;
	gdouble max_ascend;
	gdouble size;

	gdouble dpi;
	
	gchar *render_str;
	PangoLayout *layout;
	
	FontModel *model;
};

static void font_view_redraw (FontView *view);

static gboolean font_view_expose (GtkWidget *view, GdkEventExpose *event);
static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e);
static gboolean font_view_key (GtkWidget *w, GdkEventKey *e);

static void _font_view_pre_render (FontView *view);

enum {
	FONT_VIEW_SIZE_CHANGED_SIGNAL,
	LAST_SIGNAL
};
static guint font_view_signals[LAST_SIGNAL] = { 0 };

static void font_view_class_init (FontViewClass *klass) {
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = G_OBJECT_CLASS (klass);
	
	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event = font_view_expose;
	widget_class->button_release_event = font_view_clicked;
	widget_class->key_press_event = font_view_key;
	
	g_type_class_add_private (object_class, sizeof (FontViewPrivate));
	
	/* signals */
	font_view_signals[FONT_VIEW_SIZE_CHANGED_SIGNAL] =
			g_signal_new ("size-changed", G_TYPE_FROM_CLASS (klass),
							G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
							G_STRUCT_OFFSET (FontViewClass, size_changed),
							NULL, NULL,
							g_cclosure_marshal_VOID__DOUBLE, G_TYPE_NONE, 1,
							G_TYPE_DOUBLE);
}

static void font_view_init (FontView *view) {
	FontViewPrivate *priv;
	cairo_t *cr;
	cairo_surface_t *buffer;
	gint i;
	priv = FONT_VIEW_GET_PRIVATE(view);
	
	for (i = 0; i < G_N_ELEMENTS(priv->extents); i++) {
		priv->extents[i] = FALSE;
	}
	priv->extents[TEXT] = TRUE;
	priv->size = 50;

	/* until a better way to get the X11 dpi appears, 
	   we can guess for now. */
	priv->dpi = 72;
	
	/* default string to render */
	priv->render_str = "How quickly daft jumping zebras vex.";
	
	buffer = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (buffer);
	cairo_surface_destroy (buffer);
	buffer = NULL;
	
	priv->layout = pango_cairo_create_layout (cr);
	
	cairo_destroy (cr);
	cr = NULL;
	
	gtk_widget_add_events (GTK_WIDGET (view),
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
			
	GTK_WIDGET_SET_FLAGS(GTK_WIDGET(view), GTK_CAN_FOCUS);
}

GtkWidget *font_view_new () {
	return g_object_new (FONT_VIEW_TYPE, NULL);
}

GtkWidget *font_view_new_with_model (gchar *font) {
	FontView *view;
	FontViewPrivate *priv;

	g_return_val_if_fail (font, NULL);

	view = g_object_new (FONT_VIEW_TYPE, NULL);
	priv = FONT_VIEW_GET_PRIVATE(view);
		
	priv->model = FONT_MODEL(font_model_new (font));
	
	_font_view_pre_render (view);
	
	return GTK_WIDGET(view);
}

void font_view_set_model (FontView *view, FontModel *model) {
	FontViewPrivate *priv;
	priv = FONT_VIEW_GET_PRIVATE(view);
	
	if (IS_FONT_MODEL(model)) {
		priv->model = model;
	}
}

FontModel *font_view_get_model (FontView *view) {
	FontViewPrivate *priv;
	priv = FONT_VIEW_GET_PRIVATE(view);
	return priv->model;
}

void _font_view_get_extents (FontView *view) {
	cairo_t *cr;
	cairo_surface_t *buffer;
	cairo_font_face_t *cr_face;
	cairo_font_extents_t extents;
	cairo_text_extents_t t_extents;
	gint px;
	FontViewPrivate *priv = FONT_VIEW_GET_PRIVATE(view);
	
	px = priv->dpi * (priv->size/72);

	buffer = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (buffer);
	cairo_surface_destroy (buffer);
	buffer = NULL;
	
	cr_face = font_model_face_create (priv->model);
	cairo_set_font_face (cr, cr_face);
	cairo_set_font_size (cr, px);
	
	/* get font extents - for calculating descender...*/
	cairo_font_extents (cr, &extents);
	
	/* get x-height */
	cairo_text_extents (cr, "x", &t_extents);
	priv->xheight = t_extents.y_bearing;
	
	/* get ascender */
	cairo_text_extents (cr, "HJKLMTYXi", &t_extents);
	priv->ascender = t_extents.y_bearing;
	
	/* get descender */
	priv->descender = extents.descent;
	
	priv->height = t_extents.height;
	
	cairo_font_face_destroy (cr_face);
	cairo_destroy (cr);
	cr = NULL;
}

/* pre render the text */
static void _font_view_pre_render (FontView *view) {
	PangoFontDescription *desc;
	
	FontViewPrivate *priv = FONT_VIEW_GET_PRIVATE(view);

	if ((strlen (priv->render_str) < 1)) {
		priv->extents[TEXT] = FALSE;
		return;
	} else {
		priv->extents[TEXT] = TRUE;
	}

	pango_layout_set_text (priv->layout, priv->render_str, strlen (priv->render_str));

	desc = pango_font_description_from_string (font_model_desc_for_size (priv->model, priv->size));
	pango_layout_set_font_description (priv->layout, desc);
	pango_font_description_free (desc);
		
	/* fire off signal that we changed size */
	g_signal_emit_by_name (G_OBJECT (view), "size-changed", priv->size);

}


static void render (GtkWidget *w, cairo_t *cr) {
	GtkStyle *style;
	gint width, height;
	cairo_font_extents_t extents;
	cairo_text_extents_t t_extents;
	cairo_font_face_t *cr_face;
	gdouble y, x;
	FontViewPrivate *priv;
	gdouble px;
	gchar *title;
	gint p_height;
	PangoLayout *layout;
	
	priv = FONT_VIEW_GET_PRIVATE (FONT_VIEW(w));
	
	px = priv->dpi * (priv->size/72);
	
	width = w->allocation.width;
	height = w->allocation.height;
	
	style = gtk_rc_get_style (GTK_WIDGET (w));
	
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_paint (cr);
	cairo_stroke (cr);
	
	cairo_set_source_rgb (cr, 1, 0.3, 0.3);
	cairo_set_line_width (cr, 1.0);

	cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
	
	/* position text in the center */
	y = floor ((height / 2) + (priv->height / 2) - (priv->descender / 2)) + 0.5;
	x = floor (width /2 * 0.1);
	
	/* baseline */
	if (priv->extents[BASELINE]) {
		cairo_move_to (cr, x, y);
		cairo_line_to (cr, width - x, y);
		cairo_stroke (cr);
	}
	
	/* descender height */
	if (priv->extents[DESCENDER]) {
		cairo_move_to (cr, x, y + priv->descender);
		cairo_line_to (cr, width - x, y + priv->descender);
		cairo_stroke (cr);
	}

	/* ascender height */
	if (priv->extents[ASCENDER]) {
		cairo_move_to (cr, x, y + priv->ascender);
		cairo_line_to (cr, width - x, y + priv->ascender);
		cairo_stroke (cr);
	}

	/* x-height */
	if (priv->extents[XHEIGHT]) {
		cairo_move_to (cr, x, y + priv->xheight);
		cairo_line_to (cr, width - x, y + priv->xheight);
		cairo_stroke (cr);
	}

	/* display sample text */
	if (priv->extents[TEXT]) {
		pango_cairo_update_layout (cr, priv->layout);
		pango_layout_get_pixel_size (priv->layout, NULL, &p_height);

		cairo_move_to (cr, x, floor (y + priv->descender) - p_height);
		gdk_cairo_set_source_color (cr, style->fg);
		pango_cairo_show_layout (cr, priv->layout);
	}
	
	
	/* draw header bar */
	title = g_strdup_printf ("%s %s - %.0fpt", priv->model->family, priv->model->style, priv->size);
	layout = gtk_widget_create_pango_layout (w, title);
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

	cr = gdk_cairo_create (w->window);
	cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
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

static gboolean font_view_key (GtkWidget *w, GdkEventKey *e) {
	FontViewPrivate *priv;
	
	priv = FONT_VIEW_GET_PRIVATE (w);
	
	if (e->keyval == GDK_minus || e->keyval == GDK_KP_Subtract || 
	    e->keyval == GDK_plus || e->keyval == GDK_KP_Add || e->keyval == GDK_equal) {
	
		if (e->keyval == GDK_minus || e->keyval == GDK_KP_Subtract) { 
			priv->size -= 10; 
		}
		if (e->keyval == GDK_plus || e->keyval == GDK_KP_Add || e->keyval == GDK_equal) {
			priv->size += 10;
		}
	
		if (priv->size <= 0) {
			priv->size = 10;
		} else if (priv->size > ZOOM_LEVELS * 10) {
			priv->size = ZOOM_LEVELS * 10;
		} else {	
			_font_view_pre_render (FONT_VIEW(w));
		}
	
		font_view_redraw (FONT_VIEW (w));
	}
	
	if (e->keyval == GDK_v) {
		priv->extents[TEXT] = !priv->extents[TEXT];
		font_view_redraw (FONT_VIEW (w));
	}
	
	if (e->keyval == GDK_space) {
		font_view_clicked (w, NULL);
	}
	
	return FALSE;
}

static void font_view_redraw (FontView *view) {
	GtkWidget *widget;
	GdkRegion *region;

	widget = GTK_WIDGET (view);
	
	if (!widget->window) return;
	
	region = gdk_drawable_get_clip_region (widget->window);
	gdk_window_invalidate_region (widget->window, region, TRUE);
	gdk_window_process_updates (widget->window, TRUE);
	
	gdk_region_destroy (region);
}

gdouble font_view_get_pt_size (FontView *view) {
	FontViewPrivate *priv;
	
	priv = FONT_VIEW_GET_PRIVATE (view);
	return priv->size;
}

void font_view_set_pt_size (FontView *view, gdouble size) {
	FontViewPrivate *priv;
	
	priv = FONT_VIEW_GET_PRIVATE (view);
	
	if (priv->size == size) return;
	
	priv->size = size;
	_font_view_get_extents (view);
	_font_view_pre_render (view);
	
	font_view_redraw (view);
	
}

gchar *font_view_get_text (FontView *view) {
	FontViewPrivate *priv;
	
	priv = FONT_VIEW_GET_PRIVATE (view);
	return g_strdup(priv->render_str);
}

void font_view_set_text (FontView *view, gchar *text) {
	FontViewPrivate *priv;
	
	priv = FONT_VIEW_GET_PRIVATE (view);
	
	if (g_strcasecmp (priv->render_str, text) == 0) return;

	priv->render_str = NULL;

	priv->render_str = g_strdup(text);
	_font_view_pre_render (view);
	
	font_view_redraw (view);
}

