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

#include "font-model.h"

#include <ft2build.h>
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TABLES_H


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
            NULL,    /* base_init */
            NULL,    /* base_finalize */
            (GClassInitFunc) font_model_class_init,
            NULL,    /* class_finalize */
            NULL,     /* class_data */
            sizeof (FontModel),
            0,        /* n_preallocs */
            (GInstanceInitFunc) font_model_init
        };
        type = g_type_register_static (G_TYPE_OBJECT,
                "FontModelType", &info, 0);
    }
    return type;
}

GObject *font_model_new (gchar *fontfile) {
    FontModel *model;
    FT_Library library;
    FT_SfntName sfname;
    gint len, i;
    TT_OS2* os2;
    TT_PCLT* pclt;

    g_return_val_if_fail (fontfile, NULL);

    if (FT_Init_FreeType(&library)) {
        g_warning ("FT_Init_FreeType failed");
        return NULL;
    }

    model = g_object_new (FONT_MODEL_TYPE, NULL);

    if (FT_New_Face (library, fontfile, 0, &model->ft_face)) {
        g_warning ("FT_New_Face failed");
        return NULL;
    }

    model->file = g_strdup (fontfile);
    model->family = model->ft_face->family_name;
    model->style = model->ft_face->style_name;
    model->units_per_em = model->ft_face->units_per_EM;

    model->xheight = 0;
    model->ascender = 0;
    model->descender = 0;

    model->sample = NULL;

    /* Get font metadata if available/applicable */
    if (FT_IS_SFNT(model->ft_face)) {
        os2 = FT_Get_Sfnt_Table(model->ft_face, ft_sfnt_os2);
        if (os2) {
            model->xheight = os2->sxHeight;
            model->ascender = os2->sTypoAscender;
            model->descender = os2->sTypoDescender;
        }

        if (model->xheight<0){
            pclt = FT_Get_Sfnt_Table(model->ft_face, ft_sfnt_pclt);
            if (pclt)
                model->xheight = pclt->xHeight;
        }

        len = FT_Get_Sfnt_Name_Count (model->ft_face);
        for (i = 0; i < len; i++) {
            if (FT_Get_Sfnt_Name(model->ft_face, i, &sfname) != 0)
                continue;

            /* only handle the unicode names for US langid */
            if (!(sfname.platform_id == TT_PLATFORM_MICROSOFT &&
                  sfname.encoding_id == TT_MS_ID_UNICODE_CS &&
                  sfname.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES))
                continue;

            int preferred_family = 0;
            int preferred_subfamily = 0;
            switch (sfname.name_id) {
                case TT_NAME_ID_PREFERRED_FAMILY:
                    g_free(model->family);
                    model->family = g_convert((gchar *)sfname.string,
                                    sfname.string_len,
                                    "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    preferred_family = 1;
                    break;
                case TT_NAME_ID_FONT_FAMILY:
                    if (!preferred_family) {
                        g_free(model->family);
                        model->family = g_convert((gchar *)sfname.string,
                                        sfname.string_len,
                                        "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    }
                    break;
                case TT_NAME_ID_PREFERRED_SUBFAMILY:
                    g_free(model->style);
                    model->style = g_convert((gchar *)sfname.string,
                                   sfname.string_len,
                                   "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    preferred_subfamily = 1;
                    break;
                case TT_NAME_ID_FONT_SUBFAMILY:
                    if (!preferred_subfamily) {
                        g_free(model->style);
                        model->style = g_convert((gchar *)sfname.string,
                                       sfname.string_len,
                                       "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    }
                    break;
                case TT_NAME_ID_COPYRIGHT:
                    g_free(model->copyright);
                    model->copyright = g_convert((gchar *)sfname.string,
                                       sfname.string_len,
                                       "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    break;
                case TT_NAME_ID_VERSION_STRING:
                    g_free(model->version);
                    model->version = g_convert((gchar *)sfname.string,
                                     sfname.string_len,
                                     "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    break;
                case TT_NAME_ID_DESCRIPTION:
                    g_free(model->description);
                    model->description = g_convert((gchar *)sfname.string,
                                         sfname.string_len,
                                         "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    break;
                case TT_NAME_ID_SAMPLE_TEXT:
                    g_free(model->sample);
                    model->sample = g_convert((gchar *)sfname.string,
                                    sfname.string_len,
                                    "UTF-8", "UTF-16BE", NULL, NULL, NULL);
                    break;
                default:
                    break;
            }
        }
    }

    return G_OBJECT (model);
}
