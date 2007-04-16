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

typedef struct _FontViewPrivate FontViewPrivate;

struct _FontViewPrivate {
	gboolean baseline;
	gboolean ascender;
	gboolean descender;
	gboolean xheight;
	gboolean text;
	
	cairo_surface_t *render;
	
	gdouble max_ascend;
	gdouble size;

	gdouble dpi;
	
	gchar *render_str;
	
	FontModel *model;
};

static void font_view_redraw (FontView *view);

static gboolean font_view_expose (GtkWidget *view, GdkEventExpose *event);
static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e);
static gboolean font_view_key (GtkWidget *w, GdkEventKey *e);

cairo_surface_t *_font_view_pre_render_at_size (FontView *view, gdouble size);

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
	
	priv = FONT_VIEW_GET_PRIVATE(view);
	
	priv->baseline = FALSE;
	priv->ascender = FALSE;
	priv->descender = FALSE;
	priv->xheight = FALSE;
	priv->text = TRUE;
	
	priv->size = 50;

	/* until a better way to get the X11 dpi appears, 
	   we can guess for now. */
	priv->dpi = 72;
	
	/* default string to render */
	priv->render_str = "How quickly daft jumping zebras vex.";
	
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
	
	priv->render = _font_view_pre_render_at_size (view, priv->size);
	
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

/* pre render the text */
cairo_surface_t *_font_view_pre_render_at_size (FontView *view, gdouble size) {
	GtkStyle *style;
	cairo_surface_t *buffer, *render;
	gint width, height;
	cairo_font_extents_t extents;
	cairo_text_extents_t t_extents;
	gdouble ascender;
    cairo_t *cr;
    gdouble px;
	
	FontViewPrivate *priv;
	priv = FONT_VIEW_GET_PRIVATE(view);
	
	px = priv->dpi * (size/72);

#ifdef DEBUG
	g_message ("pre rendering at size: %.2fpt - %.2fpx @ %.0fdpi", size, px, priv->dpi);
#endif

	style = gtk_rc_get_style (GTK_WIDGET (view));
	
	buffer = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);

	cr = cairo_create (buffer);
	font_model_face_create (priv->model);
	
	cairo_set_font_face (cr, priv->model->cr_face);
	cairo_set_font_size (cr, floor(px));

	/* get font extents */
	/* for the font rendering store we want to display everything,
	   some fonts have disparative heights between characters, 
	   so we need to make sure we max out that ascension, and
	   store it for final rendering. */
	cairo_font_extents (cr, &extents);
	cairo_text_extents (cr, "ABCDEFGHIJKLMNOPQRSTUVWXYZdfgijklpq", &t_extents);
	ascender = -t_extents.y_bearing;
	priv->max_ascend = ascender;
	
	/* get max height now in case string to render has no characters
	   with descenders. */
	height = t_extents.height;
	
	/* http://en.wikipedia.org/wiki/Pangram */
	cairo_text_extents (cr, priv->render_str, &t_extents);
	width = t_extents.width;

	cairo_destroy (cr);
	cairo_surface_finish (buffer);
	cairo_surface_destroy (buffer);
		
	/* copy buffer contents into correctly sized surface */
	render = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width + (width * 0.25), height);
	cr = cairo_create (render);

	cairo_move_to (cr, 0, ascender);
	cairo_set_font_face (cr, priv->model->cr_face);
	cairo_set_font_size (cr, floor(px));
	gdk_cairo_set_source_color (cr, style->dark);
	cairo_show_text (cr, priv->render_str);
	
	cairo_destroy (cr);
	font_model_face_destroy (priv->model);
		
	/* fire off signal that we changed size */
	g_signal_emit_by_name (G_OBJECT (view), "size-changed", priv->size);
	
	return render;
}


static void render (GtkWidget *w, cairo_t *cr) {
	GtkStyle *style;
	gint width, height;
	cairo_font_extents_t extents;
	cairo_text_extents_t t_extents;
	gdouble xheight, ascender, y, x;
	FontViewPrivate *priv;
	gdouble px;
	gchar *title;
	
	priv = FONT_VIEW_GET_PRIVATE (FONT_VIEW(w));
	
	px = priv->dpi * (priv->size/72);
	
	width = w->allocation.width;
	height = w->allocation.height + 8;
	
	style = gtk_rc_get_style (GTK_WIDGET (w));
	
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_paint (cr);
	cairo_stroke (cr);
	
	cairo_set_source_rgb (cr, 1, 0.3, 0.3);
	cairo_set_line_width (cr, 1.0);

	
	font_model_face_create (priv->model);
	
	//cairo_select_font_face (cr, "Verdana", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_face (cr, priv->model->cr_face);
	cairo_set_font_size (cr, px);
	
	/* get font extents */
	cairo_font_extents (cr, &extents);
	
	/* get x-height */
	cairo_text_extents (cr, "x", &t_extents);
	xheight = t_extents.y_bearing;
	
	/* get ascender */
	cairo_text_extents (cr, "HJKLMTYXi", &t_extents);
	ascender = t_extents.y_bearing;

#ifdef DEBUG
	g_message ("main, ascender: %0.2f", ascender);
#endif 

	cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
	
	/* position text in the center */
	y = floor ((height / 2) + (t_extents.height / 2) - (extents.descent / 2)) + 0.5;
	x = floor (width /2 * 0.1);
	
	/* baseline */
	if (priv->baseline) {
		cairo_move_to (cr, x, y);
		cairo_line_to (cr, width - x, y);
		cairo_stroke (cr);
	}
	
	/* descender height */
	if (priv->descender) {
		cairo_move_to (cr, x, y + extents.descent);
		cairo_line_to (cr, width - x, y + extents.descent);
		cairo_stroke (cr);
	}

	/* ascender height */
	if (priv->ascender) {
		cairo_move_to (cr, x, y + ascender);
		cairo_line_to (cr, width - x, y + ascender);
		cairo_stroke (cr);
	}

	/* x-height */
	if (priv->xheight) {
		cairo_move_to (cr, x, y + xheight);
		cairo_line_to (cr, width - x, y + xheight);
		cairo_stroke (cr);
	}

	/* display sample text */
	if (priv->text) {
		//gdk_cairo_set_source_pixbuf (cr, priv->image, (width / 2 * 0.25), y + ascender);
		//cairo_set_source_surface (cr, priv->ref[priv->curzoom], (width / 2 * 0.25), y + ascender);
		cairo_set_source_surface (cr, priv->render, x, floor (y + ascender - (ascender + priv->max_ascend)));
		cairo_paint (cr);
	
	}
	
	
	/* draw header bar */
	cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 10);
	
	title = g_strdup_printf ("%s %s - %.0fpt", priv->model->family, priv->model->style, priv->size);

	cairo_text_extents (cr, title, &t_extents);
	cairo_font_extents (cr, &extents);
	cairo_rectangle (cr, 0, 0, width, t_extents.height + extents.descent + 1);
	gdk_cairo_set_source_color (cr, style->bg);
	cairo_fill (cr);
	
	gdk_cairo_set_source_color (cr, style->fg);
	cairo_move_to (cr, 5, t_extents.height);
	cairo_show_text (cr, title);
	g_free (title);
	
	font_model_face_destroy (priv->model);
}


static gboolean font_view_expose (GtkWidget *w, GdkEventExpose *event) {
	cairo_t *cr;

	cr = gdk_cairo_create (w->window);
	cairo_rectangle (cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip (cr);
	
	render (w, cr);
	
	cairo_destroy (cr);
	
	return FALSE;
}

static gboolean font_view_clicked (GtkWidget *w, GdkEventButton *e) {
	FontViewPrivate *priv;
	
	priv = FONT_VIEW_GET_PRIVATE (w);
	
	priv->baseline = !priv->baseline;
	priv->ascender = !priv->ascender;
	priv->descender = !priv->descender;
	priv->xheight = !priv->xheight;
	
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
			cairo_surface_destroy (priv->render);
			priv->render = _font_view_pre_render_at_size (FONT_VIEW(w), priv->size);
		}
	
		font_view_redraw (FONT_VIEW (w));
	}
	
	if (e->keyval == GDK_v) {
		priv->text = !priv->text;
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
	cairo_surface_finish (priv->render);
	cairo_surface_destroy (priv->render);
	priv->render = _font_view_pre_render_at_size (view, priv->size);
	
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

	priv->render_str = g_strdup(text);
	cairo_surface_finish (priv->render);
	cairo_surface_destroy (priv->render);
	priv->render = _font_view_pre_render_at_size (view, priv->size);
	
	font_view_redraw (view);
}
