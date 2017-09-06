/*
 * font-view.h
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
 
#ifndef __FONT_VIEW_H__
#define __FONT_VIEW_H__

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "font-model.h"

G_BEGIN_DECLS

#define FONT_VIEW_TYPE            (font_view_get_type())
#define FONT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FONT_VIEW_TYPE, FontView))
#define FONT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  FONT_VIEW, FontViewClass))
#define IS_FONT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FONT_VIEW_TYPE))
#define IS_FONT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  FONT_VIEW_TYPE))

typedef struct _FontView       FontView;
typedef struct _FontViewClass  FontViewClass;

struct _FontView {
    GtkDrawingArea parent;

    gchar *face;
    gint size;
};

struct _FontViewClass {
    GtkDrawingAreaClass parent_class;

    void (* FontView)(FontView *view);

    /* signals */
    void (* size_changed)(FontView *self, gdouble size);
};

GType font_view_get_type (void) G_GNUC_CONST;

GtkWidget *font_view_new ();
GtkWidget *font_view_new_with_model (gchar *font);

void font_view_set_model (FontView *view, FontModel *model);
FontModel *font_view_get_model (FontView *view);

void font_view_set_pt_size (FontView *view, gdouble size);
gdouble font_view_get_pt_size (FontView *view);

gchar *font_view_get_text (FontView *view);
void font_view_set_text (FontView *view, gchar *text);
void font_view_select_named_instance (FontView *view, gint index);

void font_view_rerender (FontView *view);

G_END_DECLS

#endif
