/*
 * font-model.h
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

#ifndef __FONT_MODEL_H__
#define __FONT_MODEL_H__

#include <glib-object.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <pango/pango.h>
#include <pango/pangoft2.h>

#define FONT_MODEL_TYPE				(font_model_get_type())
#define FONT_MODEL(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), FONT_MODEL_TYPE, FontModel))
#define FONT_MODEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), FONT_MODEL_TYPE, FontModelClass))
#define IS_FONT_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FONT_MODEL_TYPE))
#define IS_FONT_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), FONT_MODEL_TYPE))
#define FONT_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), FONT_MODEL_TYPE, FontModelClass))

typedef struct _FontModel FontModel;
typedef struct _FontModelClass FontModelClass;

struct _FontModel {
	GObject parent;
	
	/* use priv in future */
	FT_Face ft_face;
	cairo_font_face_t *cr_face;
	PangoFontDescription *desc;
		
	gchar *file;
	gchar *family;
	gchar *style;
	
	gchar *version;
	gchar *copyright;
	gchar *description;
};

struct _FontModelClass {
	GObjectClass parent;
};

GType font_model_get_type (void);

GObject *font_model_new (gchar *font);

void font_model_face_create (FontModel *model);
void font_model_face_destroy (FontModel *model);

#endif /* __FONT_MODEL_H__ */
