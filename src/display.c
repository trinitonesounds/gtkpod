/* Time-stamp: <2004-07-07 23:42:01 jcs>
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
|
|  $Id$
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "display_private.h"
#include "info.h"
#include "prefs.h"
#include "prefs_window.h"
#include "misc.h"
#include "support.h"


GtkWidget *gtkpod_window = NULL;

/* used for stopping of display refresh */
gint stop_add = SORT_TAB_MAX;

/* Move the paths listed in @data before or after (according to @pos)
   @path. Used for DND */
gboolean pmtm_move_pathlist (GtkTreeView *treeview,
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
	for (link = g_list_last (iterlist); link; link = link->prev)
	{
	    from_iter = (GtkTreeIter *)link->data;
	    if (tvt == TRACK_TREEVIEW)
		tm_list_store_move_after (GTK_LIST_STORE (model),
					  from_iter, &to_iter);
	    if (tvt == PLAYLIST_TREEVIEW)
		pm_list_store_move_after (GTK_LIST_STORE (model),
					  from_iter, &to_iter);
	}
	break;
    case GTK_TREE_VIEW_DROP_BEFORE:
	for (link = g_list_first (iterlist); link; link = link->next)
	{
	    from_iter = (GtkTreeIter *)link->data;

	    if (tvt == TRACK_TREEVIEW)
		tm_list_store_move_before (GTK_LIST_STORE (model),
					   from_iter, &to_iter);
	    if (tvt == PLAYLIST_TREEVIEW)
		pm_list_store_move_before (GTK_LIST_STORE (model),
					   from_iter, &to_iter);
	}
	break;
    }

    /* free iterlist */
    for (link = iterlist; link; link = link->next)
	g_free (link->data);
    g_list_free (iterlist);

    if (tvt == TRACK_TREEVIEW)     tm_rows_reordered ();
    if (tvt == PLAYLIST_TREEVIEW)  pm_rows_reordered ();
    return TRUE;
}


/* Create the listviews etc */
void display_create (GtkWidget *gtkpod)
{
    GtkWidget *stop_button;

    tm_create_treeview ();
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
    /* initialize sorting */
    tm_sort (prefs_get_tm_sortcol (), prefs_get_tm_sort ());
    /* activate the delete menus correctly */
    display_adjust_delete_menus ();
    /* activate/deactive the menu item 'check iPod' */
    display_set_check_ipod_menu ();
    /* activate status bars */
    gtkpod_statusbar_init ();
    gtkpod_tracks_statusbar_init ();
    gtkpod_space_statusbar_init ();
    /* set the menu item for the info window correctly */
    /* CAREFUL: must be done after calling ..._space_statusbar_init() */
    display_set_info_window_menu ();
    /* check if info window should be opened */
    if (prefs_get_info_window ())  info_open_window ();
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
    gint i,n;
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


/**
 * disable_import_buttons
 * Upon successfull itunes db importing we want to disable the import
 * buttons.  This retrieves the import buttons from the main gtkpod widget
 * and disables them from taking input.
 */
void
display_disable_gtkpod_import_buttons(void)
{
    GtkWidget *w = NULL;

    if(gtkpod_window)
    {
	if((w = lookup_widget(gtkpod_window, "import_button")))
	{
	    gtk_widget_set_sensitive(w, FALSE);
	    /* in case this widget has been blocked, we need to tell
	       update the desired state upon release */
	    update_blocked_widget (w, FALSE);
	}
	if((w = lookup_widget(gtkpod_window, "import_itunes_mi")))
	{
	    gtk_widget_set_sensitive(w, FALSE);
	    /* in case this widget has been blocked, we need to tell
	       update the desired state upon release */
	    update_blocked_widget (w, FALSE);
	}
    }
}


/* make sure only suitable delete menu items are available */
void display_adjust_delete_menus (void)
{
    GtkWidget *d  = lookup_widget (gtkpod_window, "delete_menu");
    GtkWidget *dp = lookup_widget (gtkpod_window, "delete_playlist_menu");
    GtkWidget *df  = lookup_widget (gtkpod_window, "delete_full_menu");
    GtkWidget *dfp = lookup_widget (gtkpod_window, "delete_full_playlist_menu");
#if 0
/* not used */
    GtkWidget *de = lookup_widget (gtkpod_window, "delete_tab_entry_menu");
    GtkWidget *dt = lookup_widget (gtkpod_window, "delete_tracks_menu");
    GtkWidget *dfe = lookup_widget (gtkpod_window,
				    "delete_full_tab_entry_menu");
    GtkWidget *dft = lookup_widget (gtkpod_window, "delete_full_tracks_menu");
#endif

    Playlist *pl = pm_get_selected_playlist ();
    if (pl == NULL)
    {
	gtk_widget_set_sensitive (d, FALSE);
	gtk_widget_set_sensitive (df, FALSE);
    }
    else
    {
	switch (pl->type)
	{
	case PL_TYPE_NORM:
	    gtk_widget_set_sensitive (d, TRUE);
	    gtk_widget_set_sensitive (df, TRUE);
	    gtk_widget_set_sensitive (dp, TRUE);
	    gtk_widget_set_sensitive (dfp, TRUE);
	    break;
	case PL_TYPE_MPL:
	    gtk_widget_set_sensitive (d, FALSE);
	    gtk_widget_set_sensitive (df, TRUE);
	    gtk_widget_set_sensitive (dfp, FALSE);
	}
    }
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

/* Adjust the menu item status according on the value set in the
   prefs. */
void display_set_info_window_menu (void)
{
    GtkWidget *mi = lookup_widget (gtkpod_window, "info_window_menu");

    if (prefs_get_info_window ())
    {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
    }
    else
    {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), FALSE);
    }
}

/** Takes care about 'Check IPOD Files' Menu Item and makes it unavailable
 * if gtkpod is offline or DB wasn't imported.
 */
void display_set_check_ipod_menu (void)
{
    GtkWidget *w = NULL
        , *wi = lookup_widget(gtkpod_window, "import_itunes_mi") ;

    if((w = lookup_widget(gtkpod_window, "check_ipod_files_mi")))
        gtk_widget_set_sensitive(w,
                                 !(widgets_blocked || prefs_get_offline() ||
                                   !wi || GTK_WIDGET_IS_SENSITIVE(wi) ));
    else
        g_warning ("check_ipod_files_mi(): Programming error: widget check_ipod_files_mi must be found\n");
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
    /* Show/Hide tooltips of the prefs window */
    sort_window_show_hide_tooltips ();
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
   column widths of track model
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
    tm_update_default_sizes ();
    st_update_default_sizes ();
    prefs_window_update_default_sizes ();
    info_update_default_sizes ();
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
    if ((inst >= 0) && (inst < prefs_get_sort_tab_num ()))
	return g_list_copy (st_get_selected_members (inst));
    if (inst >= prefs_get_sort_tab_num ())
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
