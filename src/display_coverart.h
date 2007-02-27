#ifndef __DISPLAY_COVERART_H__
#define __DISPLAY_COVERART_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

#define IMG_MAIN 4
#define IMG_NEXT 1
#define IMG_PREV 2
#define IMG_TOTAL 9
#define BORDER 10

typedef struct {
	Itdb_Track *track;
	gdouble img_x;
	gdouble img_y;
	gdouble img_width;
	gdouble img_height;
	GnomeCanvasItem *cdcvrgrp;
	GnomeCanvasItem *cdimage;
	GnomeCanvasItem *cdreflection;
	GnomeCanvasItem *highlight;
} Cover_Item;

typedef struct {
	GtkWidget *contentpanel;
	GtkWidget *canvasbox;
	GtkWidget *controlbox;
	GnomeCanvas *canvas;
	GnomeCanvasItem *bground;
	GnomeCanvasText *cvrtext;
	GtkButton *leftbutton;
	GtkHScale *cdslider;
	GtkButton *rightbutton;
	GPtrArray *cdcovers;
	GList *displaytracks;
	gint first_imgindex;
	gboolean block_display_change;
} CD_Widget;


extern const gchar *DISPLAY_COVER_SHOW;

void init_default_file (gchar *progpath);
void coverart_sort_images (GtkSortType order);
void coverart_select_cover (Itdb_Track *track);
void coverart_set_images ();
void coverart_clear_images ();
void coverart_block_change ();
void coverart_init_display ();
#endif
