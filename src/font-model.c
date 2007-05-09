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
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include <pango/pango.h>
#include <pango/pangofc-fontmap.h>
#include <fontconfig/fontconfig.h>


static GObjectClass *parent_class = NULL;

static void font_model_class_init (FontModelClass *klass);
static void font_model_init (GTypeInstance *instance, gpointer g_class);


static void font_model_init (GTypeInstance *instance, gpointer g_class) {
}

static void font_model_class_init (FontModelClass *klass) {
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
	FT_SfntName sfname;
	gint len, i;
	FcChar8 *s;
	FcResult result;

	FcFontSet *fonts;

	g_return_val_if_fail (fontfile, NULL);

	if (FcConfigAppFontAddFile (FcConfigGetCurrent(), fontfile)) {
		g_message ("Loaded application specific font.");
	} else {
		g_error ("Failed to load app font.");
		exit;
	}
	
	fonts = FcConfigGetFonts (FcConfigGetCurrent(), FcSetApplication);
	FcPatternPrint (fonts->fonts[0]);
	
	FT_Init_FreeType(&library);

	model = g_object_new (FONT_MODEL_TYPE, NULL);
	FT_New_Face (library, fontfile, 0, &model->ft_face);	
	
	model->file = g_strdup (fontfile);
	model->family = model->ft_face->family_name;
	model->style = model->ft_face->style_name;

	// Get font metadata if available/applicable
	if (FT_IS_SFNT(model->ft_face)) {
		len = FT_Get_Sfnt_Name_Count (model->ft_face);
		
		for (i = 0; i < len; i++) {
			FT_Get_Sfnt_Name (model->ft_face, i, &sfname);

			if (sfname.platform_id != TT_PLATFORM_MACINTOSH) {
				continue;
			}
			
			switch (sfname.name_id) {
	    		case TT_NAME_ID_COPYRIGHT:
					model->copyright = g_locale_to_utf8 (sfname.string,
						sfname.string_len,
						NULL, NULL, NULL);
					break;
			    case TT_NAME_ID_VERSION_STRING:
					model->version = g_locale_to_utf8 (sfname.string,
						sfname.string_len,
						NULL, NULL, NULL);
					break;
			    case TT_NAME_ID_DESCRIPTION:
					model->description = g_locale_to_utf8 (sfname.string,
						sfname.string_len,
						NULL, NULL, NULL);
					break;
			    default:
					break;
		    }
#ifdef DEBUG			
			g_message ("sfname: (%d) %d: %s", sfname.platform_id, sfname.name_id, sfname.string);
#endif			
		}
	}
	
	
	return G_OBJECT (model);
}

cairo_font_face_t *font_model_face_create (FontModel *model) {
	return cairo_ft_font_face_create_for_ft_face (model->ft_face, FT_LOAD_NO_AUTOHINT);

}

gchar *font_model_desc_for_size (FontModel *model, gint size) {
	gchar *desc;
	
	desc = g_strdup_printf ("%s, %s %dpx", model->family, model->style, size);
	
	g_message ("font desc: %s", desc);
	return desc;
}

