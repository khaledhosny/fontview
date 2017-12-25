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

FT_ULong
load_table (FT_Face face, FT_ULong tag, FT_Byte** buffer) {
    FT_ULong len = 0;

    if (FT_Load_Sfnt_Table (face, tag, 0, NULL, &len))
        return 0;

    *buffer = g_new (FT_Byte, len);
    if (FT_Load_Sfnt_Table (face, tag, 0, *buffer, &len)) {
        g_free (*buffer);
        *buffer = NULL;
        return 0;
    }

    return len;
}

static FT_Byte GetByte(FT_Byte **ptr)
{
    FT_Byte v = (*ptr)[0];
    *ptr += 1;
    return v;
}

static FT_UShort GetUShort(FT_Byte **ptr)
{
    FT_UShort v = (*ptr)[0] << 8 | (*ptr)[1];
    *ptr += 2;
    return v;
}

static FT_ULong GetULong(FT_Byte **ptr)
{
    FT_ULong v = (*ptr)[0] << 24 | (*ptr)[1] << 16 |
                 (*ptr)[2] << 8  | (*ptr)[3];
    *ptr += 4;
    return v;
}

static const size_t BaseGlyphSize = 6;
static const size_t LayerSize = 4;
static const size_t ColrHeaderSize = 14;
static const size_t CpalV0HeaderBaseSize = 12;
static const size_t ColorSize = 4;

void
load_color_table (FontModel *model) {
    FT_Face face;
    FT_Byte *colr_table = NULL;
    FT_Byte *cpal_table = NULL;
    FT_Byte *p = NULL;
    FT_ULong len = 0;
    FT_ULong colr_base_glyph_begin, colr_base_glyph_end, colr_layer_begin, colr_layer_end;
    FT_ULong cpal_colors_begin, cpal_colors_end;
    FT_UShort colr_version, colr_num_base_glyphs, colr_num_layers;
    FT_Byte *colr_base_glyphs, *colr_layers;
    FT_UShort cpal_version, cpal_num_palettes_entries, cpal_num_palettes, cpal_num_colors;
    FT_Byte *cpal_colors, *cpal_color_indices;

    face = model->ft_face;

    len = load_table (face, FT_MAKE_TAG ('C','O','L','R'), &colr_table);
    if (!len)
        goto done;

    if (len < ColrHeaderSize)
        goto bad;

    p = colr_table;

    colr_version = GetUShort(&p);
    colr_num_base_glyphs = GetUShort(&p);
    colr_base_glyph_begin = GetULong(&p);
    colr_layer_begin = GetULong(&p);
    colr_num_layers = GetUShort(&p);
    colr_base_glyphs = colr_table + colr_base_glyph_begin;
    colr_layers = colr_table + colr_layer_begin;

    if (colr_version != 0)
        goto done;

    colr_base_glyph_end = colr_base_glyph_begin + colr_num_base_glyphs * BaseGlyphSize;
    colr_layer_end = colr_layer_begin + colr_num_layers * LayerSize;
    if (colr_base_glyph_end < colr_base_glyph_begin ||
        colr_base_glyph_end > len ||
        colr_layer_end < colr_layer_begin ||
        colr_layer_end > len)
        goto bad;

    if (colr_base_glyphs < colr_table || colr_layers < colr_table)
        goto bad;

    len = load_table (face, FT_MAKE_TAG ('C','P','A','L'), &cpal_table);
    if (!len)
        goto done;

    if (len < CpalV0HeaderBaseSize)
        goto bad;

    p = cpal_table;
    cpal_version = GetUShort(&p);
    cpal_num_palettes_entries = GetUShort(&p);
    cpal_num_palettes = GetUShort(&p);
    cpal_num_colors = GetUShort(&p);
    cpal_colors_begin = GetULong(&p);
    cpal_color_indices = p;
    cpal_colors = cpal_table + cpal_colors_begin;

    if (cpal_version != 0 && cpal_version != 1)
        goto done;

    cpal_colors_end = cpal_colors_begin + cpal_num_colors * ColorSize;
    if (cpal_colors_end < cpal_colors_begin || cpal_colors_end > len)
        goto bad;

    if (cpal_colors < cpal_table)
        goto bad;

    model->color_layers = g_hash_table_new (g_direct_hash, g_direct_equal);

    p = colr_base_glyphs;
    while (p <= colr_table + colr_base_glyph_end) {
        ColorGlyph *glyph;
        FT_Byte *pp;
        FT_UShort gid, first_layer, num_layers;

        gid = GetUShort(&p);
        first_layer = GetUShort(&p);
        num_layers = GetUShort(&p);

        glyph = g_new (ColorGlyph, 1);
        glyph->num_layers = num_layers;
        glyph->layers = g_new (ColorLayer, num_layers);

        pp = colr_layers + first_layer * LayerSize;
        for (FT_UShort idx = 0; idx < num_layers; idx++) {
            FT_UShort color_index;
            FT_Int color_offset;

            glyph->layers[idx].gid = GetUShort(&pp);
            color_index = GetUShort(&pp);

            glyph->layers[idx].r = glyph->layers[idx].g = glyph->layers[idx].b = 0;
            glyph->layers[idx].a = 1;

            if (color_index != 0xFFFF) {
                FT_Byte *ppp;
                ppp = cpal_color_indices + 0 /*palette_index*/ * sizeof(FT_UShort);

                color_offset = GetUShort(&ppp);
                ppp = cpal_colors + color_offset + ColorSize * color_index;
                glyph->layers[idx].b = GetByte(&ppp) / 255.0;
                glyph->layers[idx].g = GetByte(&ppp) / 255.0;
                glyph->layers[idx].r = GetByte(&ppp) / 255.0;
                glyph->layers[idx].a = GetByte(&ppp) / 255.0;
            }
        }

        g_hash_table_insert (model->color_layers, GINT_TO_POINTER (gid), glyph);
    }

    goto done;

bad:
    g_warning ("Ignoring bad or corrupt COLR or CPAL table");

done:
    g_free (colr_table);
    g_free (cpal_table);
}

GObject *font_model_new (gchar *fontfile) {
    FontModel *model;
    FT_Library library;
    FT_Face face;
    FcConfig *config;
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

    model->color_layers = NULL;

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

    load_color_table (model);

    return G_OBJECT (model);
}
