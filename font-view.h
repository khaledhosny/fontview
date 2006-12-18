/*
 * font-view.h
 *
 * GTK+ widget to display and handle fonts
 *
 * - Alex Roberts, 2006
 *
 */
 
#ifndef __FONT_VIEW_H__
#define __FONT_VIEW_H__

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "font-model.h"

G_BEGIN_DECLS

#define FONT_VIEW_TYPE				(font_view_get_type())
#define	FONT_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), FONT_VIEW_TYPE, FontView))
#define FONT_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), FONT_VIEW, FontViewClass))
#define IS_FONT_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FONT_VIEW_TYPE))
#define IS_FONT_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), FONT_VIEW_TYPE))

typedef struct _FontView		FontView;
typedef struct _FontViewClass	FontViewClass;

struct _FontView {
	GtkDrawingArea parent;
	
	gchar *face;
	gint size;
	

};

struct _FontViewClass {
	GtkDrawingAreaClass parent_class;
		
	void (* FontView)(FontView *view);
	
	/* signals */
	void (* sized)(FontView *self, gdouble size);
};

GtkWidget *font_view_new ();
GtkWidget *font_view_new_with_model (gchar *font);

void font_view_set_model (FontView *view, FontModel *model);
FontModel *font_view_get_model (FontView *view);

gdouble font_view_get_pt_size (FontView *view);
void font_view_set_pt_size (FontView *view, gdouble size);

G_END_DECLS

#endif
