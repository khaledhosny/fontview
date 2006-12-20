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

#include "font-model.h"

#include <ft2build.h>
#include <pango/pango.h>
#include <pango/pangofc-fontmap.h>

static GObjectClass *parent_class = NULL;

static void font_model_class_init (FontModelClass *klass);
static void font_model_init (GTypeInstance *instance, gpointer g_class);


static void font_model_init (GTypeInstance *instance, gpointer g_class) {
	//FontModel *self = (FontModel *)instance;
	
	/* initialise private here */
	
	/* initialise other members */
	
	g_print ("Font model init'd.\n");
	
}

static void font_model_class_init (FontModelClass *klass) {
	//GObjectClass *g_class = G_OBJECT_CLASS(klass);
	//GParamSpec *font_model_param_spec;
	
	parent_class = g_type_class_peek_parent (klass);
	
		

}

GType font_model_get_type (void) {
	static GType type = 0;
	
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (FontModelClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) font_model_class_init,
			NULL,	/* class_finalize */
			NULL, 	/* class_data */
			sizeof (FontModel),
			0,		/* n_preallocs */
			(GInstanceInitFunc) font_model_init
		};
		type = g_type_register_static (G_TYPE_OBJECT, "FontModelType", &info, 0);
	}
	return type;
}

GObject *font_model_new (gchar *fontfile) {
	FontModel *model;
	FT_Library library;
	
	g_return_val_if_fail (fontfile, NULL);
	
	FT_Init_FreeType(&library);

	model = g_object_new (FONT_MODEL_TYPE, NULL);
	FT_New_Face (library, fontfile, 0, &model->ft_face);	
	
/*	pattern = FcPatternCreate();
	pattern = FcPatternBuild(0, FC_FT_FACE, FcTypeFTFace, model->ft_face, NULL);
	FcPatternAddString (pattern, FC_FAMILY, model->ft_face->family_name);
	FcPatternPrint (pattern);
	model->desc = pango_fc_font_description_from_pattern (pattern, FALSE);
	*/

    // TODO: Need to check if font exists before opening it

	model->file = fontfile;
	model->family = model->ft_face->family_name;
	model->style = model->ft_face->style_name;
	
	g_print ("FontModel instantiated.\nFont File: %s\nFont Family: %s\nFont Style: %s\n\n", 
				model->file, model->family, model->style);
	
	
	
	return G_OBJECT (model);
}

void font_model_face_create (FontModel *model) {
	model->cr_face = cairo_ft_font_face_create_for_ft_face (model->ft_face, FT_LOAD_NO_AUTOHINT);

}


void font_model_face_destroy (FontModel *model) {
	cairo_font_face_destroy (model->cr_face);
}
