/* Time-stamp: <2003-06-15 02:31:51 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
| 
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "display_private.h"
#include "prefs.h"
#include "prefs_window.h"
#include "misc.h"
#include "support.h"


/* used for stopping of display refresh */
gint stop_add = SORT_TAB_MAX;


/* Move the paths listed in @data before or after (according to @pos)
   @path. Used for DND */
gboolean pmsm_move_pathlist (GtkTreeView *treeview,
			     gchar *data,
			     GtkTreePath *path,
			     GtkTreeViewDropPosition pos,
			     TreeViewType tvt)
{
    GtkTreeIter to_iter;
    GtkTreeIter *from_iter;
    GtkTreeModel *model;
    GList *iterlist = NULL;
    GList *link;
    gchar **paths, **pathp;

    if (!data)          return FALSE;
    if (!strlen (data)) return FALSE;
    model = gtk_tree_view_get_model (treeview);
    if (!gtk_tree_model_get_iter (model, &to_iter, path))  return FALSE;

    /* split the path list into individual strings */
    paths = g_strsplit (data, "\n", -1);
    pathp = paths;
    /* Convert the list of paths into a list of iters */
    while (*pathp)
    {
	/* check that we won't move the master playlist (path = "0") */
	if (!( (tvt == PLAYLIST_TREEVIEW) && (atoi (*pathp) == 0) ))
	{
	    from_iter = g_malloc (sizeof (GtkTreeIter));
	    if ((strlen (*pathp) > 0) &&
		gtk_tree_model_get_iter_from_string (model, from_iter, *pathp))
	    {
		iterlist = g_list_append (iterlist, from_iter);
	    }
	}
	++pathp;
    }
    g_strfreev (paths);
    /* Move the iters in iterlist before or after @to_iter */
    switch (pos)
    {
    case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
    case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
    case GTK_TREE_VIEW_DROP_AFTER:
	while (iterlist)
	{
	    link = g_list_last (iterlist);
	    from_iter = (GtkTreeIter *)link->data;
	    if (tvt == SONG_TREEVIEW)
		sm_list_store_move_after (GTK_LIST_STORE (model),
					  from_iter, &to_iter);
	    if (tvt == PLAYLIST_TREEVIEW)
		pm_list_store_move_after (GTK_LIST_STORE (model),
					  from_iter, &to_iter);
	    iterlist = g_list_delete_link (iterlist, link);
	    g_free (from_iter);
	}
	break;
    case GTK_TREE_VIEW_DROP_BEFORE:
	while (iterlist)
	{
	    link = g_list_first (iterlist);
	    from_iter = (GtkTreeIter *)link->data;
	    if (tvt == SONG_TREEVIEW)
		sm_list_store_move_before (GTK_LIST_STORE (model),
					   from_iter, &to_iter);
	    if (tvt == PLAYLIST_TREEVIEW)
		pm_list_store_move_before (GTK_LIST_STORE (model),
					   from_iter, &to_iter);
	    iterlist = g_list_delete_link (iterlist, link);
	    g_free (from_iter);
	}
	break;
    }
    if (tvt == SONG_TREEVIEW)      sm_rows_reordered ();
    if (tvt == PLAYLIST_TREEVIEW)  pm_rows_reordered ();
    return TRUE;
}


/* Create the different listviews to display the various information */
void display_create (GtkWidget *gtkpod)
{
    GtkWidget *stop_button;

    sm_create_treeview ();
    st_create_tabs ();
    pm_create_treeview ();
    /* set certain sizes, positions, widths... to default values */
    display_set_default_sizes ();
    /* Hide the "stop_button" */
    stop_button = lookup_widget (gtkpod_window, "stop_button");
    if (stop_button) gtk_widget_hide (stop_button);
    /* Hide/Show the toolbar */
    display_show_hide_toolbar ();
    /* Hide/Show tooltips */
    display_show_hide_tooltips ();
    /* change standard g_print () handler */
    g_set_print_handler ((GPrintFunc)gtkpod_warning);
}

/* redisplay the entire display (playlists, sort tabs, song view) and
 * reset the sorted treeviews to normal (according to @inst) */
/* @inst: which treeviews should be reset to normal?
   -2: all treeviews
   -1: only playlist
    0...SORT_TAB_MAX-1: sort tab of instance @inst
    SORT_TAB_MAX: song treeview */
void display_reset (gint inst)
{
    gint i,n;
    Playlist *cur_pl;

    /* remember */
    cur_pl = pm_get_selected_playlist ();

    /* remove all playlists from model (and reset "sortable") */
    if ((inst == -2) || (inst == -1))	pm_remove_all_playlists (TRUE);
    else                                pm_remove_all_playlists (FALSE);

    /* reset the sort tabs and song view */
    st_init (-1, 0);

    /* reset "sortable" */
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	if ((inst == -2) || (inst == i))
	    st_remove_all_entries_from_model (TRUE, i);
    }

    /* add playlists back to model (without selecting) */
    pm_set_selected_playlist (cur_pl);
    n = get_nr_of_playlists ();
    for (i=0; i<n; ++i)
    {
	pm_add_playlist (get_playlist_by_nr (i), -1);
    }
}


/* Clean up used memory (when quitting the program) */
void display_cleanup (void)
{
    st_cleanup ();
}

/* make the toolbar visible or hide it depending on the value set in
   the prefs */
void display_show_hide_toolbar (void)
{
    GtkWidget *tb = lookup_widget (gtkpod_window, "toolbar");
    GtkWidget *mi = lookup_widget (gtkpod_window, "toolbar_menu");

    if (prefs_get_display_toolbar ())
    {
	gtk_toolbar_set_style (GTK_TOOLBAR (tb), prefs_get_toolbar_style ());
	gtk_widget_show (tb);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    else
    {
	gtk_widget_hide (tb);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), FALSE);
    }
}

/* make the tooltips visible or hide it depending on the value set in
 * the prefs (tooltips_main) */
void display_show_hide_tooltips (void)
{
    /* so far only tooltips in the toolbar are used... */
    GtkTooltips *tt = NULL; /* = GTK_TOOLTIPS (lookup_widget (gtkpod_window,
      "tooltips")); */
    GtkCheckMenuItem *mi = GTK_CHECK_MENU_ITEM (
	lookup_widget (gtkpod_window, "tooltips_menu"));
    GtkToolbar *tb = GTK_TOOLBAR (lookup_widget (gtkpod_window, "toolbar"));


    if (prefs_get_display_tooltips_main ())
    {
	if (tt)  gtk_tooltips_enable (tt);
	if (mi)  gtk_check_menu_item_set_active (mi, TRUE);
	if (tb)  gtk_toolbar_set_tooltips (tb, TRUE);
    }
    else
    {
	if (tt)  gtk_tooltips_disable (tt);
	if (mi)  gtk_check_menu_item_set_active (mi, FALSE);
	if (tb)  gtk_toolbar_set_tooltips (tb, FALSE);
    }
    /* Show/Hide tooltips of the special sorttabs */
    st_show_hide_tooltips ();
    /* Show/Hide tooltips of the prefs window */
    prefs_window_show_hide_tooltips ();
}


/* set the default sizes for the gtkpod main window according to prefs:
   x, y, position of the PANED_NUM GtkPaned elements (the widht of the
   colums is set when setting up the colums in the listview */
void display_set_default_sizes (void)
{
    gint defx, defy;

    /* x,y-size */
    prefs_get_size_gtkpod (&defx, &defy);
    gtk_window_set_default_size (GTK_WINDOW (gtkpod_window), defx, defy);
    st_set_default_sizes ();
}


/* update the cfg structure (preferences) with the current sizes /
   positions:
   x,y size of main window
   column widths of song model
   position of GtkPaned elements */
void display_update_default_sizes (void)
{
    gint x,y;

    /* x,y size of main window */
    if (gtkpod_window)
    {
	gtk_window_get_size (GTK_WINDOW (gtkpod_window), &x, &y);
	prefs_set_size_gtkpod (x, y);
    }
    sm_update_default_sizes ();
    st_update_default_sizes ();
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

    /* lookup stop_button */
    if (stop_button == NULL)
    {
	stop_button = lookup_widget (gtkpod_window, "stop_button");
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
	    if (level >= inst)
	    {
		level = inst;
		r_brc = brc;
		r_user_data1 = user_data1;
		r_user_data2 = user_data2;
		if (timeout_id == 0)
		{
		    timeout_id = 
			gtk_idle_add_priority (G_PRIORITY_HIGH,
					       selection_callback_timeout,
					       NULL);
		}	       
	    }
	}
	else
	{
	    if (inst < level)
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
void stop_display_update (gint inst)
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
