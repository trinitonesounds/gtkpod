/* Time-stamp: <2006-06-25 00:22:56 jcs>
|
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
#include "ipod_init.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "prefs_window.h"
#include "repository.h"
#include "syncdir.h"
#include "tools.h"
#include <stdlib.h>
#include <string.h>


GtkWidget *gtkpod_window = NULL;

/* used for stopping of display refresh */
gint stop_add = SORT_TAB_MAX;

/* Create the listviews etc */
void display_create (void)
{
    GtkWidget *stop_button;
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
    st_set_default_sizes ();
    pm_create_treeview ();
    /* Hide the "stop_button" */
    stop_button = gtkpod_xml_get_widget (main_window_xml, "stop_button");
    if (stop_button) gtk_widget_hide (stop_button);
    /* Hide/Show the toolbar */
    display_show_hide_toolbar ();
    /* Hide/Show tooltips */
    display_show_hide_tooltips ();
    /* change standard g_print () handler */
    g_set_print_handler ((GPrintFunc)gtkpod_warning);

    /* activate the delete menus correctly */
    display_adjust_menus ();
    /* activate status bars */
    gtkpod_statusbar_init ();
    gtkpod_tracks_statusbar_init ();
    gtkpod_space_statusbar_init ();
    /* set the menu item for the info window correctly */
    /* CAREFUL: must be done after calling ..._space_statusbar_init() */
    display_set_info_window_menu ();
    /* check if info window should be opened */
    if (prefs_get_int("info_window"))  info_open_window ();
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
    pm_add_all_playlists ();
}


/* Clean up used memory (when quitting the program) */
void display_cleanup (void)
{
    st_cleanup ();
}


/* make sure only suitable delete menu items are available */
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

    pl = pm_get_selected_playlist ();

    if (pl == NULL)
    {
	gtk_widget_set_sensitive (delete, FALSE);
	gtk_widget_set_sensitive (edit1, FALSE);
	gtk_widget_set_sensitive (edit2, FALSE);
	gtk_widget_set_sensitive (edit3, FALSE);
	gtk_widget_set_sensitive (edit4, FALSE);
	gtk_widget_set_sensitive (edit5, FALSE);
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
    prefs_window_show_hide_tooltips ();
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
    prefs_window_update_default_sizes ();
    info_update_default_sizes ();
    details_update_default_sizes ();
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

           Functions for stopping display update

   ------------------------------------------------------------ */

enum {
    BR_BLOCK,
    BR_RELEASE,
    BR_ADD,
    BR_CALL,
};


#if DEBUG_CB_INIT
static gchar *act_to_str (gint action)
{
    switch (action)
    {
    case BR_BLOCK: return ("BLOCK");
    case BR_RELEASE: return ("RELEASE");
    case BR_ADD: return ("ADD");
    case BR_CALL: return ("CALL");
    }
    return "\"\"";
}
#endif

/* called by block_selection() and release_selection */
static void block_release_selection (gint inst, gint action,
				     br_callback brc,
				     gpointer user_data1,
				     gpointer user_data2)
{
    static gint count_st[SORT_TAB_MAX];
    static gint count_pl = 0;
    static GtkWidget *stop_button = NULL;
    /* instance that has a pending callback */
    static gint level = SORT_TAB_MAX; /* no level -> no registered callback */
    static br_callback r_brc;
    static gpointer r_user_data1;
    static gpointer r_user_data2;
    static guint timeout_id = 0;
    gint i;

#if DEBUG_CB_INIT
    printf ("block_release_selection: inst: %d, action: %s, callback: %p\n  user1: %p, user2: %d\n", inst, act_to_str (action), brc, user_data1, (guint)user_data2);
    printf ("  enter: level: %d, count_pl: %d, cst0: %d, cst1: %d\n",
	    level, count_pl, count_st[0], count_st[1]);
#endif

    /* lookup stop_button */
    if (stop_button == NULL)
    {
	stop_button = gtkpod_xml_get_widget (main_window_xml, "stop_button");
	if (stop_button == NULL)
	    g_warning ("Programming error: stop_button not found\n");
    }

    switch (action)
    {
    case BR_BLOCK:
	if (count_pl == 0)
	{
	    block_widgets ();
	    if (stop_button) gtk_widget_show (stop_button);
	}
	++count_pl;
	for (i=0; (i<=inst) && (i<SORT_TAB_MAX); ++i)
	{
	    ++count_st[i];
	}
	break;
    case BR_RELEASE:
	for (i=0; (i<=inst) && (i<SORT_TAB_MAX); ++i)
	{
	    --count_st[i];
	    if ((count_st[i] == 0) && (stop_add == i))
		stop_add = SORT_TAB_MAX;
	}
	--count_pl;
	if (count_pl == 0)
	{
	    if (stop_button) gtk_widget_hide (stop_button);
	    stop_add = SORT_TAB_MAX;
	    release_widgets ();
	}
	/* check if we have to call a callback */
	if (level < SORT_TAB_MAX)
	{
	    /* don't call it directly -- let's first return to the
	       calling function */
/* 	    if (timeout_id == 0) */
/* 	    { */
/* 		timeout_id =  */
/* 		    gtk_idle_add_priority (G_PRIORITY_HIGH_IDLE, */
/* 					   selection_callback_timeout, */
/* 					   NULL); */
/* 	    } */
	    /* remove timeout function just to be sure */
	    if (timeout_id)
	    {
		gtk_timeout_remove (timeout_id);
		timeout_id = 0;
	    }
	    if (((level == -1) && (count_pl == 0)) ||
		((level >= 0) && (count_st[level] == 0)))
	    {
		level = SORT_TAB_MAX;
		r_brc (r_user_data1, r_user_data2);
	    }
	}
	break;
    case BR_ADD:
/*	printf("adding: inst: %d, level: %d, brc: %p, data1: %p, data2: %p\n",
	inst, level, brc, user_data1, user_data2);*/
	if (((inst == -1) && (count_pl == 0)) ||
	    ((inst >= 0) && (count_st[inst] == 0)))
	{ /* OK, we could just call the desired function because there
	    is nothing to be stopped. However, due to a bug or a
            inevitability of gtk+, if we interrupt the selection
	    process with a gtk_main_iteration() call the button
	    release event will be "discarded". Therefore we register
	    the call here and have it activated with a timeout
	    function. */
	    /* We overwrite an older callback of the same level or
	     * higher  */
	    if (inst <= level)
	    {
		level = inst;
		r_brc = brc;
		r_user_data1 = user_data1;
		r_user_data2 = user_data2;
		if (timeout_id == 0)
		{
		    timeout_id =
			gtk_idle_add_priority (G_PRIORITY_HIGH_IDLE,
					       selection_callback_timeout,
					       NULL);
		}
	    }
	}
	else
	{
	    if (inst <= level)
	    {   /* Once the functions have stopped, down to the
		specified level/instance, the desired function is
		called (about 15 lines up: r_brc (...)) */
		/* We need to emit a stop_add signal */
		stop_add = inst;
		/* and safe the callback data */
		level = inst;
		r_brc = brc;
		r_user_data1 = user_data1;
		r_user_data2 = user_data2;
		/* don't let a timeout snatch away this callback
		 * before all functions have stopped */
		if (timeout_id)
		{
		    gtk_timeout_remove (timeout_id);
		    timeout_id = 0;
		}
	    }
	}
	break;
    case BR_CALL:
	if (timeout_id)
	{
	    gtk_timeout_remove (timeout_id);
	    timeout_id = 0;
	}
	if (level == SORT_TAB_MAX) break;  /* hmm... what happened to
					      our callback? */
	if (((level == -1) && (count_pl == 0)) ||
	    ((level >= 0) && (count_st[level] == 0)))
	{ /* Let's call the callback function */
		level = SORT_TAB_MAX;
		r_brc (r_user_data1, r_user_data2);
	}
	else
	{ /* This is strange and should not happen -- let's forget
	   * about the callback */
	    level = SORT_TAB_MAX;
	}
	break;
    default:
	g_warning ("Programming error: unknown BR_...: %d\n", action);
	break;
    }
#if DEBUG_CB_INIT
    printf ("   exit: level: %d, count_pl: %d, cst0: %d, cst1: %d\n",
	    level, count_pl, count_st[0], count_st[1]);
#endif
}

/* Will block the possibility to select another playlist / sort tab
   page / tab entry. Will also block the widgets and activate the
   "stop button" that can be pressed to stop the update
   process. "inst" is the sort tab instance up to which the selections
   should be blocked. "-1" corresponds to the playlist view */
void block_selection (gint inst)
{
    block_release_selection (inst, BR_BLOCK, NULL, NULL, NULL);
}

/* Makes selection possible again */
void release_selection (gint inst)
{
    block_release_selection (inst, BR_RELEASE, NULL, NULL, NULL);
}


/* Stops the display updates down to instance "inst". "-1" is the
 * playlist view */
void display_stop_update (gint inst)
{
    stop_add = inst;
}

/* registers @brc to be called as soon as all functions down to
   instance @inst have been stopped */
void add_selection_callback (gint inst, br_callback brc,
				    gpointer user_data1, gpointer user_data2)
{
    block_release_selection (inst, BR_ADD, brc, user_data1, user_data2);
}

/* Called as a high priority timeout to initiate the callback of @brc
   in the last function */
gboolean selection_callback_timeout (gpointer data)
{
    block_release_selection (0, BR_CALL, NULL, NULL, NULL);
    return FALSE;
}



/* Put all treeviews into the unsorted state (@enable=FALSE) or back
   to the sorted state (@enable=TRUE) */
/* Indicates that we begin adding playlists from iTunesDB.
   This is a good time to stop sorting display. */
void display_enable_disable_view_sort (gboolean enable)
{
    st_enable_disable_view_sort (0, enable);
/*    tm_enable_disable_view_sort (enable);*/
}



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


void on_edit_details_selected_playlist (GtkMenuItem     *menuitem,
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

void on_edit_details_selected_tab_entry (GtkMenuItem     *menuitem,
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

void on_edit_details_selected_tracks (GtkMenuItem     *menuitem,
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
void
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
void
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

void
on_edit_preferences_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if(!widgets_blocked)  prefs_window_create (-1);
}

void
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

void
on_load_ipods_mi           (GtkMenuItem     *menuitem,
			    gpointer         user_data)
{
    gp_load_ipods ();
}


void
on_load_ipods_clicked               (GtkButton       *button,
				     gpointer         user_data)
{
    gp_load_ipods ();
}


void
on_save_changes_mi           (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    handle_export ();
}



void
on_save_changes_clicked               (GtkButton       *button,
				     gpointer         user_data)
{
    handle_export ();
}



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

void on_delete_selected_tracks_from_database (GtkMenuItem *mi,
					      gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_DATABASE);
}

void
on_delete_selected_playlist_including_tracks_from_harddisk (GtkMenuItem *mi,
							    gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_LOCAL);
}


void on_delete_selected_entry_from_database (GtkMenuItem *mi,
					     gpointer data)
{
    delete_selected_entry (DELETE_ACTION_DATABASE, 
			   _("Remove entry of which sort tab from database?"));
}


void on_delete_selected_entry_from_ipod (GtkMenuItem *mi,
					 gpointer data)
{
    delete_selected_entry (DELETE_ACTION_IPOD,
			   _("Remove tracks in selected entry of which filter tab from the iPod?"));
}


void on_delete_selected_tracks_from_playlist (GtkMenuItem *mi,
					      gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_PLAYLIST);
}


void on_delete_selected_tracks_from_harddisk (GtkMenuItem *mi,
					      gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_LOCAL);
}


void on_delete_selected_entry_from_harddisk (GtkMenuItem *mi,
					     gpointer data)
{
    delete_selected_entry (DELETE_ACTION_LOCAL,
			   _("Remove tracks in selected entry of which filter tab from the harddisk?"));
}


void on_delete_selected_tracks_from_ipod (GtkMenuItem *mi,
					  gpointer data)
{
    delete_selected_tracks (DELETE_ACTION_IPOD);
}


void on_delete_selected_playlist (GtkMenuItem *mi,
				  gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_PLAYLIST);
}


void
on_delete_selected_playlist_including_tracks_from_database (GtkMenuItem *mi,
							    gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_DATABASE);
}


void on_delete_selected_entry_from_playlist (GtkMenuItem *mi,
					     gpointer data)
{
    delete_selected_entry (DELETE_ACTION_PLAYLIST,
			   _("Remove tracks in selected entry of which filter tab from playlist?"));
}


void
on_delete_selected_playlist_including_tracks_from_ipod (GtkMenuItem *mi,
							gpointer data)
{
    delete_selected_playlist (DELETE_ACTION_IPOD);
}


void
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
		str);
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

void
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
		str);
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


void
on_stop_button_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    display_stop_update (-1);
}

void
on_update_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    gp_do_selected_playlist (update_tracks);
}

/* update tracks in tab entry */
void
on_update_tab_entry_activate        (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) gp_do_selected_entry (update_tracks, inst);
}

void
on_update_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gp_do_selected_tracks (update_tracks);
}


void
on_mserv_from_file_tracks_menu_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gp_do_selected_tracks (mserv_from_file_tracks);
}


void
on_mserv_from_file_entry_menu_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) gp_do_selected_entry (mserv_from_file_tracks, inst);
}


void
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

void
on_save_track_order1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tm_rows_reordered ();
}


void
on_toolbar_menu_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("display_toolbar",
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    display_show_hide_toolbar();
}


void
on_more_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("sort_tab_num", prefs_get_int("sort_tab_num")+1);
    st_show_visible();
}


void
on_less_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("sort_tab_num", prefs_get_int("sort_tab_num")-1);
    st_show_visible();
}

void
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


void
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


void
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


void
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


void
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


void
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

void
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


void
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


void
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


void
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


void
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


void
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


void
on_arrange_sort_tabs_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    st_arrange_visible_sort_tabs ();
}

void
on_tooltips_menu_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_int("display_tooltips_main",
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    display_show_hide_tooltips();
    
}

void
on_pl_containing_displayed_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_displayed_playlist ();
}

void
on_pl_containing_selected_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_selected_playlist ();
}

void
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

void
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

void
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

void
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


void
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


void
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


void
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

void
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


void
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

void
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

void
on_sorting_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    sort_window_create ();
}

void
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


void
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


void
on_normalize_selected_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
   GList *tracks = tm_get_selected_tracks ();
   nm_tracks_list (tracks);
   g_list_free (tracks);
}


void
on_normalize_displayed_tracks_activate  (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_all_tracks ();
    nm_tracks_list (tracks);
    g_list_free (tracks);
}


void
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


void
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


void
on_info_window1_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
	 info_open_window ();
    else info_close_window ();
}

void
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


void
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


void
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


void
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

void
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


void
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


void
on_randomize_current_playlist_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    randomize_current_playlist();
}

void
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
