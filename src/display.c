/*
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
|
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  This program is free software; you can redistribute it and/or modify
|  it under the terms of the GNU General Public License as published by
|  the Free Software Foundation; either version 2 of the License, or
|  (at your option) any later version.
|
|  This program is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|  GNU General Public License for more details.
|
|  You should have received a copy of the GNU General Public License
|  along with this program; if not, write to the Free Software
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "details.h"
#include "display_private.h"
#include "info.h"
#include "infodlg.h"
#include "ipod_init.h"
#include "file_convert.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "sort_window.h"
#include "repository.h"
#include "syncdir.h"
#include "tools.h"
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <glib/gprintf.h>


GtkWidget *gtkpod_window = NULL;

/* Create the listviews etc */
void display_create (void)
{
    gint defx, defy;
    GtkTooltips *main_tooltips;

    g_return_if_fail (gtkpod_window);

    /* x,y-size */
    defx = prefs_get_int("size_gtkpod.x");
		defy = prefs_get_int("size_gtkpod.y");
    gtk_window_set_default_size (GTK_WINDOW (gtkpod_window), defx, defy);
/* we need to use the following line if the main window is already
   displayed */
/*    gtk_window_resize (GTK_WINDOW (gtkpod_window), defx, defy);*/

    /* Create tooltips */
    main_tooltips = gtk_tooltips_new ();
    g_object_set_data (G_OBJECT (gtkpod_window),
		       "main_tooltips", main_tooltips);
    /* indicate that main_tooltips was set up */
    g_object_set_data (G_OBJECT (gtkpod_window),
		       "main_tooltips_initialised", "set");

    tm_create_treeview ();
    st_create_tabs ();
    pm_create_treeview ();

    /* initialize sorting */
    tm_sort (prefs_get_int("tm_sortcol"), prefs_get_int("tm_sort"));

    st_set_default_sizes ();

    /* Hide/Show the toolbar */
    display_show_hide_toolbar ();
    /* Hide/Show tooltips */
    display_show_hide_tooltips ();
    /* Hide/Show searchbar */
    display_show_hide_searchbar ();
    /* change standard g_print () handler */
    g_set_print_handler ((GPrintFunc)gtkpod_warning);

    /* activate the delete menus correctly */
    display_adjust_menus ();
    /* activate status bars */
    gtkpod_statusbar_init ();
    gtkpod_tracks_statusbar_init ();
    gtkpod_space_statusbar_init ();

    /* Initialize the coverart display */
    coverart_init_display ();

    /* set the menu item for the info window correctly */
    /* CAREFUL: must be done after calling ..._space_statusbar_init() */
    display_set_info_window_menu ();
    /* check if info window should be opened */
    if (prefs_get_int("info_window"))
		open_info_dialog ();
}

/* redisplay the entire display (playlists, sort tabs, track view) and
 * reset the sorted treeviews to normal (according to @inst) */
/* @inst: which treeviews should be reset to normal?
   -2: all treeviews
   -1: only playlist
    0...SORT_TAB_MAX-1: sort tab of instance @inst
    SORT_TAB_MAX: track treeview
    SORT_TAB_MAX+1: all sort tabs */
void display_reset (gint inst)
{
    gint i;
    Playlist *cur_pl;

    /* remember */
    cur_pl = pm_get_selected_playlist ();

    /* remove all playlists from model (and reset "sortable") */
    if ((inst == -2) || (inst == -1))	pm_remove_all_playlists (TRUE);
    else                                pm_remove_all_playlists (FALSE);

    /* reset the sort tabs and track view */
    st_init (-1, 0);

    /* reset "sortable" */
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	if ((inst == -2) || (inst == i) || (inst == SORT_TAB_MAX+1))
	    st_remove_all_entries_from_model (i);
    }

    pm_set_selected_playlist (cur_pl);
    /* add playlists back to model (without selecting) */
    pm_add_all_itdbs ();
}


/* Clean up used memory (when quitting the program) */
void display_cleanup (void)
{
    st_cleanup ();
}


/* make sure only suitable menu items are available */
void display_adjust_menus (void)
{
    GtkWidget *delete;
    GtkWidget *edit1, *edit2, *edit3, *edit4, *edit5;
    GtkWidget *dtfpl, *dtfip, *dtfdb, *dtfhd;
    GtkWidget *defpl, *defip, *defdb, *defhd;
    GtkWidget *dpl, *dpltfip, *dpltfdb, *dpltfhd;
    GtkWidget *dsep1, *dsep2;
    GtkWidget *espl;
    Playlist *pl;

    delete = gtkpod_xml_get_widget (main_window_xml, "delete_menu");
    edit1 = gtkpod_xml_get_widget (main_window_xml, "edit_details_menu");
    edit2 = gtkpod_xml_get_widget (main_window_xml, "delete_menu");
    edit3 = gtkpod_xml_get_widget (main_window_xml, "create_playlists_menu");
    edit4 = gtkpod_xml_get_widget (main_window_xml, "randomize_current_playlist_menu");
    edit5 = gtkpod_xml_get_widget (main_window_xml, "save_track_order_menu");

#if 0
    edit6 = gtkpod_xml_get_widget (main_window_xml, "add_files1");
    edit7 = gtkpod_xml_get_widget (main_window_xml, "add_directory1");
    edit8 = gtkpod_xml_get_widget (main_window_xml, "add_playlist1");
#endif

    dtfpl = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_tracks_from_playlist");
    dtfip = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_tracks_from_ipod");
    dtfdb = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_tracks_from_database");
    dtfhd = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_tracks_from_harddisk");
    defpl = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_entry_from_playlist");
    defip = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_entry_from_ipod");
    defdb = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_entry_from_database");
    defhd = gtkpod_xml_get_widget (main_window_xml,
				   "delete_selected_entry_from_harddisk");
    dpl = gtkpod_xml_get_widget (main_window_xml,
				 "delete_selected_playlist");
    dpltfip = gtkpod_xml_get_widget (main_window_xml,
				     "delete_selected_playlist_including_tracks_from_ipod");
    dpltfdb = gtkpod_xml_get_widget (main_window_xml,
				     "delete_selected_playlist_including_tracks_from_database");
    dpltfhd = gtkpod_xml_get_widget (main_window_xml,
				     "delete_selected_playlist_including_tracks_from_harddisk");
    dsep1 = gtkpod_xml_get_widget (main_window_xml, "delete_separator1");
    dsep2 = gtkpod_xml_get_widget (main_window_xml, "delete_separator2");
    espl = gtkpod_xml_get_widget (main_window_xml,
				  "edit_smart_playlist");

#if 0
    tb_edit1 = gtkpod_xml_get_widget (main_window_xml, "add_files_button");
    tb_edit2 = gtkpod_xml_get_widget (main_window_xml, "add_dirs_button");
    tb_edit3 = gtkpod_xml_get_widget (main_window_xml, "add_PL_button");
    tb_edit4 = gtkpod_xml_get_widget (main_window_xml, "new_PL_button");
#endif

    pl = pm_get_selected_playlist ();

    if (pl == NULL)
    {
	gtk_widget_set_sensitive (delete, FALSE);
	gtk_widget_set_sensitive (edit1, FALSE);
	gtk_widget_set_sensitive (edit2, FALSE);
	gtk_widget_set_sensitive (edit3, FALSE);
	gtk_widget_set_sensitive (edit4, FALSE);
	gtk_widget_set_sensitive (edit5, FALSE);
	gtk_widget_set_sensitive (espl, FALSE);
    }
    else
    {
	iTunesDB *itdb = pl->itdb;
	g_return_if_fail (itdb);

	gtk_widget_set_sensitive (delete, TRUE);
	gtk_widget_set_sensitive (edit1, TRUE);
	gtk_widget_set_sensitive (edit2, TRUE);
	gtk_widget_set_sensitive (edit3, TRUE);
	gtk_widget_set_sensitive (edit4, TRUE);
	gtk_widget_set_sensitive (edit5, TRUE);

	gtk_widget_hide (dtfpl);
	gtk_widget_hide (dtfip);
	gtk_widget_hide (dtfdb);
	gtk_widget_hide (dtfhd);

	gtk_widget_hide (defpl);
	gtk_widget_hide (defip);
	gtk_widget_hide (defdb);
	gtk_widget_hide (defhd);

	gtk_widget_hide (dpl);
	gtk_widget_hide (dpltfip);
	gtk_widget_hide (dpltfdb);
	gtk_widget_hide (dpltfhd);

	gtk_widget_hide (dsep1);
	gtk_widget_hide (dsep2);

	gtk_widget_set_sensitive (espl, FALSE);

	if (pl->is_spl)
	{
	    gtk_widget_set_sensitive (espl, TRUE);
	}

	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    if (itdb_playlist_is_mpl (pl))
	    {
		gtk_widget_show (dtfip);
		gtk_widget_show (defip);
	    }
	    else
	    {
		if (itdb_playlist_is_podcasts (pl))
		{
		    gtk_widget_show (dtfip);
		    gtk_widget_show (defip);
		}
		else
		{
		    gtk_widget_show (dpl);
		    gtk_widget_show (dpltfip);

		    gtk_widget_show (dsep1);

		    gtk_widget_show (defpl);
		    gtk_widget_show (defip);

		    gtk_widget_show (dsep2);

		    gtk_widget_show (dtfpl);
		    gtk_widget_show (dtfip);
		}
	    }
	}
	if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	{
	    if (itdb_playlist_is_mpl (pl))
	    {
		gtk_widget_show (dtfdb);
		gtk_widget_show (dtfhd);

		gtk_widget_show (dsep2);

		gtk_widget_show (defdb);
		gtk_widget_show (defhd);
	    }
	    else
	    {
		gtk_widget_show (dtfpl);
		gtk_widget_show (dtfdb);
		gtk_widget_show (dtfhd);

		gtk_widget_show (dsep1);

		gtk_widget_show (defpl);
		gtk_widget_show (defdb);
		gtk_widget_show (defhd);

		gtk_widget_show (dsep2);

		gtk_widget_show (dpl);
		gtk_widget_show (dpltfdb);
		gtk_widget_show (dpltfhd);
	    }
	}
	if (itdb->usertype & GP_ITDB_TYPE_PODCASTS)
	{
	    if (itdb_playlist_is_mpl (pl))
	    {
		gtk_widget_show (dtfdb);
		gtk_widget_show (dtfhd);

		gtk_widget_show (dsep2);

		gtk_widget_show (defdb);
		gtk_widget_show (defhd);
	    }
	    else
	    {
		gtk_widget_show (dtfpl);
		gtk_widget_show (dtfdb);
		gtk_widget_show (dtfhd);

		gtk_widget_show (dsep1);

		gtk_widget_show (defpl);
		gtk_widget_show (defdb);
		gtk_widget_show (defhd);

		gtk_widget_show (dsep2);

		gtk_widget_show (dpl);
		gtk_widget_show (dpltfdb);
		gtk_widget_show (dpltfhd);
	    }
	}
    }
}

/* make the toolbar visible or hide it depending on the value set in
   the prefs */
void display_show_hide_toolbar (void)
{
    GtkWidget *tb = gtkpod_xml_get_widget (main_window_xml, "toolbar");
    GtkWidget *mi = gtkpod_xml_get_widget (main_window_xml, "toolbar_menu");

    if (prefs_get_int("display_toolbar"))
    {
	gtk_toolbar_set_style (GTK_TOOLBAR (tb), prefs_get_int("toolbar_style"));
	gtk_widget_show (tb);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    else
    {
	gtk_widget_hide (tb);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), FALSE);
    }
}


/* call gtkpod_tracks_statusbar_update() in order to redraw the
 * statusbar, making GTK aware of the changed position for the resize
 * grip. */
gboolean display_redraw_statusbar (gpointer data)
{
    gtkpod_tracks_statusbar_update ();
    return FALSE;
}


/* This is called (among others) when the window size changes.  This
   hack needed to make GTK aware of the changed position for the
   resize grip of the statusbar  */
G_MODULE_EXPORT gboolean
on_gtkpod_configure_event (GtkContainer *container,
			   gpointer      user_data)
{
    g_idle_add (display_redraw_statusbar, NULL);
    return FALSE;
}



/* make the search bar visible or hide it depending on the value set in
 * the prefs
 */
void display_show_hide_searchbar (void)
{
	GtkWidget *upbutton = gtkpod_xml_get_widget (main_window_xml, "searchbar_up_button");
	GtkWidget *searchbar = gtkpod_xml_get_widget (main_window_xml, "searchbar_hpanel");
	GtkCheckMenuItem *mi = GTK_CHECK_MENU_ITEM (gtkpod_xml_get_widget (main_window_xml, "filterbar_menu"));
	GtkStatusbar *sb = GTK_STATUSBAR (gtkpod_xml_get_widget (main_window_xml, "tracks_statusbar"));

	g_return_if_fail (upbutton);
	g_return_if_fail (searchbar);
	g_return_if_fail (mi);
	g_return_if_fail (sb);
		
	if (prefs_get_int ("display_search_entry"))
	{
		gtk_widget_show_all (searchbar);
		gtk_widget_hide (upbutton);
		gtk_check_menu_item_set_active (mi, TRUE);
		gtk_statusbar_set_has_resize_grip (sb, TRUE);
		/* hack needed to make GTK aware of the changed
		   position for the resize grip */
		g_idle_add (display_redraw_statusbar, NULL);
	}
	else
	{		
		gtk_widget_hide_all (searchbar);
		gtk_widget_show (upbutton);
		gtk_widget_set_sensitive (upbutton, TRUE);
		gtk_check_menu_item_set_active (mi, FALSE);
		gtk_statusbar_set_has_resize_grip (sb, FALSE);
	}
}

/* Adjust the menu item status according on the value set in the
   prefs. */
void display_set_info_window_menu (void)
{
    GtkWidget *mi = gtkpod_xml_get_widget (main_window_xml, "info_window_menu");

    if (prefs_get_int("info_window"))
    {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    else
    {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), FALSE);
    }
}

/* make the tooltips visible or hide it depending on the value set in
 * the prefs (tooltips_main) */
void display_show_hide_tooltips (void)
{
    GtkCheckMenuItem *mi;
    GtkToolbar *tb;
    GtkTooltips *mt;

    g_return_if_fail (main_window_xml);
    g_return_if_fail (gtkpod_window);

    /* ignore calls before window was set up properly (called by
       prefs) */
    if (!g_object_get_data (G_OBJECT (gtkpod_window),
			    "main_tooltips_initialised")) return;

    mi = GTK_CHECK_MENU_ITEM (
	gtkpod_xml_get_widget (main_window_xml, "tooltips_menu"));
    tb = GTK_TOOLBAR (gtkpod_xml_get_widget (main_window_xml, "toolbar"));
    mt = g_object_get_data (G_OBJECT (gtkpod_window), "main_tooltips");

    g_return_if_fail (mi);
    g_return_if_fail (tb);
    g_return_if_fail (mt);


    if (prefs_get_int("display_tooltips_main"))
    {
	gtk_tooltips_enable (mt);
	gtk_check_menu_item_set_active (mi, TRUE);
	gtk_toolbar_set_tooltips (tb, TRUE);
    }
    else
    {
	gtk_tooltips_disable (mt);
	gtk_check_menu_item_set_active (mi, FALSE);
	gtk_toolbar_set_tooltips (tb, FALSE);
    }
    /* Show/Hide tooltips of the special sorttabs */
    st_show_hide_tooltips ();
    /* Show/Hide tooltips of the prefs window */
    sort_window_show_hide_tooltips ();
}


/* update the cfg structure (preferences) with the current sizes /
   positions:
   x,y size of main window
   column widths of track model
   position of GtkPaned elements */
void display_update_default_sizes (void)
{
    gint x,y;

    /* x,y size of main window */
    if (gtkpod_window)
    {
	gtk_window_get_size (GTK_WINDOW (gtkpod_window), &x, &y);
	prefs_set_int("size_gtkpod.x", x);
	prefs_set_int("size_gtkpod.y", y);
    }
    tm_update_default_sizes ();
    st_update_default_sizes ();
    info_dialog_update_default_sizes ();
    details_update_default_sizes ();
    file_convert_update_default_sizes ();
}

/**
 * on_drawing_area_exposed:
 *
 * Callback for the drawing area. When the drwaing area is covered,
 * resized, changed etc.. This will be called the draw() function is then
 * called from this and the cairo redrawing takes place.
 * 
 * @draw_area: drawing area where al the cairo drawing takes place 
 * @event: gdk expose event
 * 
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
static gboolean on_coverart_preview_dialog_exposed (GtkWidget *drawarea, GdkEventExpose *event, gpointer data)
{
	GdkPixbuf *image = data;
	
	/* Draw the image using cairo */
		cairo_t *cairo_context;
		
		/* get a cairo_t */
		cairo_context = gdk_cairo_create (drawarea->window);
		/* set a clip region for the expose event */
		cairo_rectangle (cairo_context,
				event->area.x, event->area.y,
				event->area.width, event->area.height);
		cairo_clip (cairo_context);
	
		gdk_cairo_set_source_pixbuf (
				cairo_context,
				image,
				0,
				0);
		cairo_paint (cairo_context);
		
		cairo_destroy (cairo_context);
		return FALSE;
}

/**
 * display_image_dialog
 * 
 * @GdkPixbuf: image
 * 
 * function to load a transient dialog displaying the provided image at either
 * it maximum size or the size of the screen (whichever is smallest).
 * 
 */
void display_image_dialog (GdkPixbuf *image)
{
	g_return_if_fail (image);
	
	GladeXML *preview_xml;
	GtkWidget *dialog;
	GtkWidget *drawarea;
	GtkWidget *res_label;
	GdkPixbuf *scaled = NULL;
	gchar *text;
		
	preview_xml = gtkpod_xml_new (xml_file, "coverart_preview_dialog");
	dialog = gtkpod_xml_get_widget (preview_xml, "coverart_preview_dialog");
	drawarea = gtkpod_xml_get_widget (preview_xml, "coverart_preview_dialog_drawarea");
	res_label = gtkpod_xml_get_widget (preview_xml, "coverart_preview_dialog_res_lbl");
	g_return_if_fail (dialog);
	g_return_if_fail (drawarea);
	g_return_if_fail (res_label);
		
	/* Set the dialog parent */
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_window));
			
	gint pixheight = gdk_pixbuf_get_height (image);
	gint pixwidth = gdk_pixbuf_get_width (image);
		
	/* Set the resolution in the label */
	text = g_markup_printf_escaped (_("<b>Image Dimensions: %d x %d</b>"), pixwidth, pixheight);
	gtk_label_set_markup (GTK_LABEL (res_label), text);
	g_free (text);
	
	gint scrheight = gdk_screen_height() - 100;
	gint scrwidth = gdk_screen_width() - 100;
		
	gdouble ratio = (gdouble) pixwidth / (gdouble) pixheight;
	if (pixwidth > scrwidth)
	{
		pixwidth = scrwidth;
		pixheight = pixwidth / ratio;
	}
		
	if (pixheight > scrheight)
	{
		pixheight = scrheight;
		pixwidth = pixheight * ratio;
	}
	
	scaled = gdk_pixbuf_scale_simple (
					image, 
					pixwidth, 
					pixheight, 
					GDK_INTERP_BILINEAR);

	/* Set the draw area's minimum size */
	gtk_widget_set_size_request  (drawarea, pixwidth, pixheight);
	g_signal_connect (G_OBJECT (drawarea), "expose_event",  G_CALLBACK (on_coverart_preview_dialog_exposed), scaled);
	
	/* Display the dialog and block everything else until the
	 * dialog is closed.
	 */
	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG(dialog));
			
	/* Destroy the dialog as no longer required */
	
	g_object_unref (scaled);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* Utility function: returns a copy of the tracks currently
   selected. This means:

   @inst == -1:
      return list of tracks in selected playlist

   @inst == 0 ... prefs_get_sort_tab_num () - 1:
      return list of tracks in passed on to the next instance: selected
      tab entry (normal sort tab) or tracks matching specified
      conditions in a special sort tab

   @inst >= prefs_get_sort_tab_num ():
      return list of tracks selected in the track view

   You must g_list_free() the list after use.
*/
GList *display_get_selection (guint32 inst)
{
    if (inst == -1)
    {
	Playlist *pl = pm_get_selected_playlist ();
	if (pl)  return g_list_copy (pl->members);
	else     return NULL;
    }
    if ((inst >= 0) && (inst < prefs_get_int("sort_tab_num")))
	return g_list_copy (st_get_selected_members (inst));
    if (inst >= prefs_get_int("sort_tab_num"))
	return tm_get_selected_tracks ();
    return NULL;
}


/* Get the members that are passed on to the next instance.
   @inst: -1: playlist view
          0...prefs_get_sort_tab_num()-1: sort tabs
          >= prefs_get_sort_tab_num(): return NULL
   You must not g_list_free the list or otherwise modify it */
GList *display_get_selected_members (gint inst)
{
    GList *tracks=NULL;
    if (inst == -1)
    {
	Playlist *pl = pm_get_selected_playlist ();
	if (pl)  tracks = pl->members;
    }
    else
    {
	tracks = st_get_selected_members (inst);
    }
    return tracks;
}


/* ------------------------------------------------------------

           Functions for treeview autoscroll (during DND)

   ------------------------------------------------------------ */

static void _remove_scroll_row_timeout (GtkWidget *widget)
{
    g_return_if_fail (widget);

    g_object_set_data (G_OBJECT (widget),
		       "scroll_row_timeout", NULL);
    g_object_set_data (G_OBJECT (widget),
		       "scroll_row_times", NULL);
}

void display_remove_autoscroll_row_timeout (GtkWidget *widget)
{
    guint timeout;

    g_return_if_fail (widget);

    timeout = GPOINTER_TO_UINT(g_object_get_data (G_OBJECT (widget),
					"scroll_row_timeout"));

    if (timeout != 0)
    {
	g_source_remove (timeout);
	_remove_scroll_row_timeout (widget);
    }
}

static gint display_autoscroll_row_timeout (gpointer data)
{
    GtkTreeView *treeview = data;
    gint px, py;
    GdkModifierType mask;
    GdkRectangle vis_rect;
    guint times;
    gboolean resp = TRUE;
    const gint SCROLL_EDGE_SIZE = 12;

    g_return_val_if_fail (data, FALSE);

    gdk_threads_enter ();

    times = GPOINTER_TO_UINT(g_object_get_data (G_OBJECT (data), "scroll_row_times"));
    
    gdk_window_get_pointer (gtk_tree_view_get_bin_window (treeview),
			    &px, &py, &mask);
    gtk_tree_view_get_visible_rect (treeview, &vis_rect);
/*     printf ("px/py, w/h, mask: %d/%d, %d/%d, %d\n", px, py, */
/* 	    vis_rect.width, vis_rect.height, mask); */
    if ((vis_rect.height > 2.2 * SCROLL_EDGE_SIZE) &&
	((py < SCROLL_EDGE_SIZE) ||
	 (py > vis_rect.height-SCROLL_EDGE_SIZE)))
    {
	GtkTreePath *path = NULL;

	if (py < SCROLL_EDGE_SIZE/3)
	    ++times;
	if (py > vis_rect.height-SCROLL_EDGE_SIZE/3)
	    ++times;

	if (times > 0)
	{
	    if (gtk_tree_view_get_path_at_pos (treeview, px, py, 
					       &path, NULL, NULL, NULL))
	    {
		if (py < SCROLL_EDGE_SIZE)
		    gtk_tree_path_prev (path);
		if (py > vis_rect.height-SCROLL_EDGE_SIZE)
		    gtk_tree_path_next (path);
		gtk_tree_view_scroll_to_cell (treeview, path, NULL,
					      FALSE, 0, 0);
	    }
	    times = 0;
	}
	else
	{
	    ++times;
	}
    }
    else
    {
	times = 0;
    }
    g_object_set_data (G_OBJECT (data), "scroll_row_times",
		       GUINT_TO_POINTER(times));
    if (mask == 0)
    {
	_remove_scroll_row_timeout (data);
	resp = FALSE;
    }
    gdk_threads_leave ();
    return resp;
}

void display_install_autoscroll_row_timeout (GtkWidget *widget)
{
    if (!g_object_get_data (G_OBJECT (widget), "scroll_row_timeout"))
    {   /* install timeout function for autoscroll */
	guint timeout = g_timeout_add (75, display_autoscroll_row_timeout,
				       widget);
	g_object_set_data (G_OBJECT (widget), "scroll_row_timeout",
			   GUINT_TO_POINTER(timeout));
    }
}



/* ------------------------------------------------------------

      Callbacks for toolbar buttons and menu.

   ------------------------------------------------------------ */

/* The following functions are called directly without using callbacks
   in this section: 

   void handle_export (void)
   void create_add_files_dialog (void)
   void create_add_playlists_dialog(void)
   void dirbrowser_create (void)
   void open_about_window (void)
*/


G_MODULE_EXPORT void
on_edit_details_selected_playlist (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (pl)
    {
	details_edit (pl->members);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}

G_MODULE_EXPORT void
on_edit_details_selected_tab_entry (GtkMenuItem     *menuitem,
					 gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Edit selected entry of which sort tab?"));

    if (inst != -1)
    {
	TabEntry *entry = st_get_selected_entry (inst);
	if (entry == NULL)
	{  /* no entry selected */
	    gtkpod_statusbar_message (_("No entry selected."));
	    return;
	}
	details_edit (entry->members);
    }
}

G_MODULE_EXPORT void
on_edit_details_selected_tracks (GtkMenuItem     *menuitem,
				      gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	details_edit (tracks);
	g_list_free (tracks);
    }
    else
    {
	message_sb_no_tracks_selected ();
    }
}


/* callback for "add new playlist" button */
G_MODULE_EXPORT void
on_new_playlist_button                 (GtkButton       *button,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	add_new_pl_or_spl_user_name (itdb, NULL, -1);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


/* callback for "add new playlist" menu */
G_MODULE_EXPORT void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	add_new_pl_or_spl_user_name (itdb, NULL, -1);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_edit_repository_options_activate          (GtkMenuItem     *menuitem,
					      gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (pl)
    {
	repository_edit (pl->itdb, pl);
    }
    else
    {
	repository_edit (NULL, NULL);
    }
}

G_MODULE_EXPORT void
on_load_ipods_mi           (GtkMenuItem     *menuitem,
			    gpointer         user_data)
{
    gp_load_ipods ();
}


G_MODULE_EXPORT void
on_load_ipods_clicked               (GtkButton       *button,
				     gpointer         user_data)
{
    gp_load_ipods ();
}


G_MODULE_EXPORT void
on_save_changes_mi           (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    handle_export ();
}



G_MODULE_EXPORT void
on_save_changes_clicked               (GtkButton       *button,
				     gpointer         user_data)
{
    handle_export ();
}


G_MODULE_EXPORT 
void on_edit_smart_playlist (GtkMenuItem *mi,
			     gpointer data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (pl)
    {
	spl_edit (pl);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}


static void delete_selected_tracks (DeleteAction deleteaction)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	delete_track_head (deleteaction);
	g_list_free (tracks);
    }
    else
    {
	message_sb_no_tracks_selected ();
    }
}    

static void delete_selected_entry (DeleteAction deleteaction,
				   gchar *text)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (text);
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gtkpod_statusbar_message (_("No entry selected in Sort Tab %d"),
				  inst+1);
	return;
    }
    delete_entry_head (inst, deleteaction);
}

static void delete_selected_playlist (DeleteAction deleteaction)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (pl)
    {
	delete_playlist_head (deleteaction);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}

G_MODULE_EXPORT void
on_delete_selected_tracks_from_database (GtkMenuItem *mi,
					      gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_DATABASE);
}

G_MODULE_EXPORT void
on_delete_selected_playlist_including_tracks_from_harddisk (GtkMenuItem *mi,
							    gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_LOCAL);
}


G_MODULE_EXPORT void
on_delete_selected_entry_from_database (GtkMenuItem *mi,
					     gpointer data)
{
    delete_selected_entry (DELETE_ACTION_DATABASE, 
			   _("Remove entry of which sort tab from database?"));
}


G_MODULE_EXPORT void
on_delete_selected_entry_from_ipod (GtkMenuItem *mi,
					 gpointer data)
{
    delete_selected_entry (DELETE_ACTION_IPOD,
			   _("Remove tracks in selected entry of which filter tab from the iPod?"));
}


G_MODULE_EXPORT void
on_delete_selected_tracks_from_playlist (GtkMenuItem *mi,
					      gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_PLAYLIST);
}


G_MODULE_EXPORT void
on_delete_selected_tracks_from_harddisk (GtkMenuItem *mi,
					      gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_LOCAL);
}


G_MODULE_EXPORT void
on_delete_selected_entry_from_harddisk (GtkMenuItem *mi,
					     gpointer data)
{
    delete_selected_entry (DELETE_ACTION_LOCAL,
			   _("Remove tracks in selected entry of which filter tab from the harddisk?"));
}


G_MODULE_EXPORT void
on_delete_selected_tracks_from_ipod (GtkMenuItem *mi,
					  gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_IPOD);
}


G_MODULE_EXPORT void
on_delete_selected_playlist (GtkMenuItem *mi,
				  gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_PLAYLIST);
}


G_MODULE_EXPORT void
on_delete_selected_playlist_including_tracks_from_database (GtkMenuItem *mi,
							    gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_DATABASE);
}


G_MODULE_EXPORT void
on_delete_selected_entry_from_playlist (GtkMenuItem *mi,
					     gpointer data)
{
    delete_selected_entry (DELETE_ACTION_PLAYLIST,
			   _("Remove tracks in selected entry of which filter tab from playlist?"));
}


G_MODULE_EXPORT void
on_delete_selected_playlist_including_tracks_from_ipod (GtkMenuItem *mi,
							gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_IPOD);
}


G_MODULE_EXPORT void
on_ipod_directories_menu               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_ipod_itdb ();
    if (itdb)
    {
	ExtraiTunesDBData *eitdb = itdb->userdata;

	g_return_if_fail (eitdb);


	if (!eitdb->itdb_imported)
	{
	    gchar *mountpoint = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);
	    gchar *str = g_strdup_printf (_("iPod at '%s' is not loaded.\nPlease load it first."), mountpoint);
	    GtkWidget *dialog = gtk_message_dialog_new (
		GTK_WINDOW (gtkpod_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_OK,
		"%s", str);
	    gtk_dialog_run (GTK_DIALOG (dialog));
	    gtk_widget_destroy (dialog);
	    g_free (str);
	    g_free (mountpoint);
	}
	else
	{
	     gp_ipod_init (itdb);
	}
    }
    else
    {
	message_sb_no_ipod_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_check_ipod_files_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_ipod_itdb ();
    if (itdb)
    {
	ExtraiTunesDBData *eitdb = itdb->userdata;

	g_return_if_fail (eitdb);


	if (!eitdb->itdb_imported)
	{
	    gchar *mountpoint = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);
	    gchar *str = g_strdup_printf (_("iPod at '%s' is not loaded.\nPlease load it first."), mountpoint);
	    GtkWidget *dialog = gtk_message_dialog_new (
		GTK_WINDOW (gtkpod_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_OK,
		"%s", str);
	    gtk_dialog_run (GTK_DIALOG (dialog));
	    gtk_widget_destroy (dialog);
	    g_free (str);
	    g_free (mountpoint);
	}
	else
	{
	    check_db (itdb);
	}
    }
    else
    {
	message_sb_no_ipod_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_update_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    gp_do_selected_playlist (update_tracks);
}

/* update tracks in tab entry */
G_MODULE_EXPORT void
on_update_tab_entry_activate        (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) gp_do_selected_entry (update_tracks, inst);
}

G_MODULE_EXPORT void
on_update_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gp_do_selected_tracks (update_tracks);
}

G_MODULE_EXPORT void
on_mserv_from_file_playlist_menu_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gp_do_selected_playlist (mserv_from_file_tracks);
}

G_MODULE_EXPORT void
on_mserv_from_file_tracks_menu_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gp_do_selected_tracks (mserv_from_file_tracks);
}


G_MODULE_EXPORT void
on_mserv_from_file_entry_menu_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) gp_do_selected_entry (mserv_from_file_tracks, inst);
}


G_MODULE_EXPORT void
on_sync_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    Playlist *pl;

    pl = pm_get_selected_playlist();
    if (pl)
    {
	sync_playlist (pl, NULL,
		       KEY_SYNC_CONFIRM_DIRS, 0,
		       KEY_SYNC_DELETE_TRACKS, 0,
		       KEY_SYNC_CONFIRM_DELETE, 0,
		       KEY_SYNC_SHOW_SUMMARY, 0);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}

G_MODULE_EXPORT void
on_save_track_order1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tm_rows_reordered ();
}


G_MODULE_EXPORT void
on_toolbar_menu_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("display_toolbar",
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    display_show_hide_toolbar();
}


G_MODULE_EXPORT void
on_filterbar_menu_activate               (GtkMenuItem     *menuitem,
					  gpointer         user_data)
{
    prefs_set_int("display_search_entry",
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    display_show_hide_searchbar ();
}


G_MODULE_EXPORT void
on_more_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("sort_tab_num", prefs_get_int("sort_tab_num")+1);
    st_show_visible();
}


G_MODULE_EXPORT void
on_less_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("sort_tab_num", prefs_get_int("sort_tab_num")-1);
    st_show_visible();
}

G_MODULE_EXPORT void
on_export_playlist_activate  (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (!pl)
    {
	export_files_init (pl->members, NULL, FALSE, NULL);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}


G_MODULE_EXPORT void
on_export_tab_entry_activate (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Export selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gtkpod_statusbar_message (_("No entry selected in Sort Tab %d"),
				  inst+1);
	return;
    }
    export_files_init (entry->members, NULL, FALSE, NULL);
}


G_MODULE_EXPORT void
on_export_tracks_activate     (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	export_files_init (tracks, NULL, FALSE, NULL);
	g_list_free (tracks);
    }
    else
    {
	message_sb_no_tracks_selected ();
    }
}


G_MODULE_EXPORT void
on_playlist_file_playlist_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (pl)
    {
	export_playlist_file_init (pl->members);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}


G_MODULE_EXPORT void
on_playlist_file_tab_entry_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Create playlist file from selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gtkpod_statusbar_message (_("No entry selected in Sort Tab %d"),
				  inst+1);
	return;
    }
    export_playlist_file_init (entry->members);
}


G_MODULE_EXPORT void
on_playlist_file_tracks_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	export_playlist_file_init(tracks);
	g_list_free (tracks);
    }
    else
    {
	message_sb_no_tracks_selected ();
    }
}

G_MODULE_EXPORT void
on_play_playlist_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
    {
	tools_play_tracks (pl->members);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}


G_MODULE_EXPORT void
on_play_tab_entry_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Play tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gtkpod_statusbar_message (_("No entry selected in Sort Tab %d"),
				  inst+1);
	return;
    }
    tools_play_tracks (entry->members);
}


G_MODULE_EXPORT void
on_play_tracks_activate                 (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();
    if (tracks)
    {
	tools_play_tracks (tracks);
	g_list_free (tracks);
	tracks = NULL;
    }
    else
    {
	message_sb_no_tracks_selected ();
    }
}


G_MODULE_EXPORT void
on_enqueue_playlist_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
    {
	tools_enqueue_tracks (pl->members);
    }
    else
    {
	message_sb_no_playlist_selected ();
    }
}


G_MODULE_EXPORT void
on_enqueue_tab_entry_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Enqueue tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gtkpod_statusbar_message (_("No entry selected in Sort Tab %d"),
				  inst+1);
	return;
    }
    tools_enqueue_tracks (entry->members);
}


G_MODULE_EXPORT void
on_enqueue_tracks_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();
    if (tracks)
    {
	tools_enqueue_tracks (tracks);
	g_list_free (tracks);
	tracks = NULL;
    }
    else
    {
	message_sb_no_tracks_selected ();
    }    
}


G_MODULE_EXPORT void
on_arrange_sort_tabs_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    st_arrange_visible_sort_tabs ();
}

G_MODULE_EXPORT void
on_tooltips_menu_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("display_tooltips_main",
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    display_show_hide_tooltips();
    
}

G_MODULE_EXPORT void
on_pl_containing_displayed_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_displayed_playlist ();
}

G_MODULE_EXPORT void
on_pl_containing_selected_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_selected_playlist ();
}

G_MODULE_EXPORT void
on_pl_for_each_artist_activate         (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_category_playlists (itdb, T_ARTIST);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_pl_for_each_album_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_category_playlists (itdb, T_ALBUM);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_pl_for_each_genre_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_category_playlists (itdb, T_GENRE);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_pl_for_each_composer_activate         (GtkMenuItem     *menuitem,
					  gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_category_playlists (itdb, T_COMPOSER);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_pl_for_each_year_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_category_playlists (itdb, T_YEAR);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_most_listened_tracks1_activate       (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	most_listened_pl (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_all_tracks_never_listened_to1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	never_listened_pl (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_most_rated_tracks_playlist_s1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	most_rated_pl (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_most_recent_played_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	last_listened_pl (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_played_since_last_time1_activate    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	since_last_pl (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_sorting_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    sort_window_create ();
}

G_MODULE_EXPORT void
on_normalize_selected_playlist_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	nm_tracks_list (pl->members);
    else
	message_sb_no_playlist_selected ();
}


G_MODULE_EXPORT void
on_normalize_selected_tab_entry_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Normalize tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (entry)
    {
	nm_tracks_list (entry->members);
    }
    else
    {
	gtkpod_statusbar_message (_("No entry selected in Sort Tab %d"),
				  inst+1);
	return;
    }
}


G_MODULE_EXPORT void
on_normalize_selected_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
   GList *tracks = tm_get_selected_tracks ();
   nm_tracks_list (tracks);
   g_list_free (tracks);
}


G_MODULE_EXPORT void
on_normalize_displayed_tracks_activate  (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_all_tracks ();
    nm_tracks_list (tracks);
    g_list_free (tracks);
}


G_MODULE_EXPORT void
on_normalize_all_tracks                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb ();

    if (itdb)
    {
	Playlist *mpl = itdb_playlist_mpl (itdb);
	g_return_if_fail (mpl);
	nm_tracks_list (mpl->members);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_normalize_newly_added_tracks        (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	nm_new_tracks (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_info_window1_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
		open_info_dialog ();
    else
		close_info_dialog ();
}

G_MODULE_EXPORT void
on_conversion_log1_activate               (GtkMenuItem     *menuitem,
					   gpointer         user_data)
{
    gboolean state;

    state = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));

    if (state != prefs_get_int (FILE_CONVERT_DISPLAY_LOG))
    {
	prefs_set_int (FILE_CONVERT_DISPLAY_LOG, state);
	file_convert_prefs_changed ();
    }
}


G_MODULE_EXPORT void
on_sync_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct itdbs_head *itdbs_head = gp_get_itdbs_head (gtkpod_window);
    GList *gl;

    g_return_if_fail (itdbs_head);

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	g_return_if_fail (gl);
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    tools_sync_all (itdb);
	}
    }
}


G_MODULE_EXPORT void
on_sync_calendar_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_ipod_itdb ();
    if (itdb)
    {
	tools_sync_calendar (itdb);
    }
    else
    {
	message_sb_no_ipod_itdb_selected ();
    }
}


G_MODULE_EXPORT void
on_sync_contacts_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_ipod_itdb ();
    if (itdb)
    {
	tools_sync_contacts (itdb);
    }
    else
    {
	message_sb_no_ipod_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_sync_notes_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    iTunesDB *itdb = gp_get_ipod_itdb ();
    if (itdb)
    {
	tools_sync_notes (itdb);
    }
    else
    {
	message_sb_no_ipod_itdb_selected ();
    }
}
G_MODULE_EXPORT void
on_all_tracks_not_listed_in_any_playlist1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_not_listed_playlist (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_random_playlist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	generate_random_playlist(itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}

G_MODULE_EXPORT void
on_randomize_current_playlist_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    randomize_current_playlist();
}
G_MODULE_EXPORT void
on_pl_for_each_rating_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    iTunesDB *itdb = gp_get_selected_itdb();

    if (itdb)
    {
	each_rating_pl (itdb);
    }
    else
    {
	message_sb_no_itdb_selected ();
    }
}
