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

gchar*
get_font_name (FT_Face face, FT_UInt nameid) {
    FT_UInt len;

    len = FT_Get_Sfnt_Name_Count (face);
    for (FT_UInt i = 0; i < len; i++) {
        FT_SfntName name;
        if (FT_Get_Sfnt_Name(face, i, &name) != 0)
            continue;

        if (name.name_id != nameid)
            continue;

        /* only handle the unicode names for US langid */
        if (!(name.platform_id == TT_PLATFORM_MICROSOFT &&
              name.encoding_id == TT_MS_ID_UNICODE_CS &&
              name.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES))
            continue;

        return g_convert((gchar *)name.string, name.string_len,
                        "UTF-8", "UTF-16BE", NULL, NULL, NULL);
    }

    return NULL;
}

GObject *font_model_new (gchar *fontfile) {
    FontModel *model;
    FT_Library library;
    FT_Face face;
    FT_SfntName sfname;
    FcConfig *config;
    gint len, i;
    TT_OS2* os2;

    g_return_val_if_fail (fontfile, NULL);

    if (FT_Init_FreeType(&library)) {
        g_error ("FT_Init_FreeType failed");
        return NULL;
    }

    if (FT_New_Face (library, fontfile, 0, &face)) {
        g_error ("FT_New_Face failed");
        return NULL;
    }

    if (!FT_IS_SFNT(face)) {
        g_error ("Not an SFNT font!");
        return NULL;
    }

    config = FcConfigCreate ();
    if (!FcConfigAppFontAddFile (config, fontfile)) {
        g_error ("FcConfigAppFontAddFile failed");
        return NULL;
    }

    model = g_object_new (FONT_MODEL_TYPE, NULL);
    model->file = g_strdup (fontfile);
    model->ft_face = face;
    model->config = config;
    model->family = face->family_name;
    model->style = face->style_name;
    model->units_per_em = face->units_per_EM;

    model->xheight = 0;
    model->ascender = 0;
    model->descender = 0;

    model->sample = NULL;

    /* Get font metadata if available/applicable */
    os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    if (os2) {
        model->xheight = os2->sxHeight;
        model->ascender = os2->sTypoAscender;
        model->descender = os2->sTypoDescender;
    }

    model->family = get_font_name (face, TT_NAME_ID_PREFERRED_FAMILY);
    if (!model->family)
        model->family = get_font_name (face, TT_NAME_ID_FONT_FAMILY);

    model->style = get_font_name (face, TT_NAME_ID_PREFERRED_SUBFAMILY);
    if (!model->style)
        model->style = get_font_name (face, TT_NAME_ID_FONT_SUBFAMILY);

    model->copyright = get_font_name (face, TT_NAME_ID_COPYRIGHT);
    model->version = get_font_name (face, TT_NAME_ID_VERSION_STRING);
    model->description = get_font_name (face, TT_NAME_ID_DESCRIPTION);
    model->sample = get_font_name (face, TT_NAME_ID_SAMPLE_TEXT);

    model->mmvar = NULL;
    model->mmcoords = NULL;
    if (FT_HAS_MULTIPLE_MASTERS (face)) {
        if (FT_Get_MM_Var (face, &model->mmvar) != 0) {
            model->mmvar = NULL;
        }
    }

    return G_OBJECT (model);
}
