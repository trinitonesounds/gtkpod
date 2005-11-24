/* Time-stamp: <2005-11-24 20:52:04 jcs>
|
|  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
|  Copyright (C) 2003-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

#include "itdb.h"
#include "details.h"
#include "display.h"
#include "file.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "tools.h"
#include "podcast.h"

#define LOCALDEBUG 0

static guint entry_inst = -1;
static GList *selected_tracks = NULL;
static Playlist *selected_playlist = NULL;
static TabEntry *selected_entry = NULL; 
static iTunesDB *active_itdb = NULL;
/* types of context menus (PM/ST/TM) */
typedef enum {
    CM_PM = 0,
    CM_ST,
    CM_TM,
    CM_NUM
} CM_type;


#if LOCALDEBUG
/**
 * do_special - for debugging: change as needed to obtain information
 * on selected tracks
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
do_special(GtkWidget *w, gpointer data)
{
	GList *gl;
	for (gl=selected_tracks; gl; gl=gl->next)
	{
	    Track *tr = gl->data;
	    g_return_if_fail (tr);
 	    printf ("track: %p: thumbnails: %p id: %d num: %d\n",
 		    tr, tr->thumbnails, tr->image_id, g_list_length (tr->thumbnails));
	    if (tr->thumbnails)
	    {
		GList *gl2;
		for (gl2=tr->thumbnails; gl2; gl2=gl2->next)
		{
		    guchar *chardata;
		    Image *img = gl2->data;
		    g_return_if_fail (img);
		    printf ("  %s offset: %d\n", img->filename, img->offset);
		chardata = itdb_image_get_rgb_data (tr->itdb, img);
		if (chardata) printf ("%s\n", chardata);
		g_free (chardata);
		}
	    }
	}
}
#endif



/**
 * export_entries - export the currently selected files to disk
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
export_entries(GtkWidget *w, gpointer data)
{
    if(selected_tracks)
	export_files_init (selected_tracks, NULL, FALSE, NULL);
}

/**
 * create_playlist_file - write a playlist file containing the
 * currently selected tracks.
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
create_playlist_file(GtkWidget *w, gpointer data)
{
    if(selected_tracks)
	export_playlist_file_init(selected_tracks);
}


/**
 * edit_entries - open a dialog to edit the track(s) or playlist
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 * FIXME: How this should work needs to be decided
 */
#if 0
static void 
edit_entries(GtkButton *b, gpointer data)
{

    fprintf(stderr, "edit entries Selected\n");
}
#endif

/*
 * play_entries_now - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
play_entries_now (GtkMenuItem *mi, gpointer data)
{
    tools_play_tracks (selected_tracks);
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
play_entries_enqueue (GtkMenuItem *mi, gpointer data)
{
    tools_enqueue_tracks (selected_tracks);
}


/*
 * show_details_entries - show details of tracks currently selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
show_details_entries(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	details_show (selected_playlist->members);
    else if(selected_entry)
	details_show (selected_entry->members);
    else if(selected_tracks)
	details_show (selected_tracks);
}

/*
 * update_entries - update the entries currently selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
update_entries(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	gp_do_selected_playlist (update_tracks);
    else if(selected_entry)
	gp_do_selected_entry (update_tracks, entry_inst);
    else if(selected_tracks)
	gp_do_selected_tracks (update_tracks);
}

/*
 * sync_dirs_ entries - sync the directories of the entries
 *                      selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
sync_dirs_entries(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	gp_do_selected_playlist (sync_tracks);
    else if(selected_entry)
	gp_do_selected_entry (sync_tracks, entry_inst);
    else if(selected_tracks)
	gp_do_selected_tracks (sync_tracks);
}

/**
 * delete_entries - delete the currently selected entry, be it a playlist,
 * items in a sort tab, or tracks
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
delete_entries(GtkMenuItem *mi, gpointer data)
{
    DeleteAction deleteaction = GPOINTER_TO_INT (data);

    if (selected_playlist)
	delete_playlist_head (deleteaction);
    else if(selected_entry)
	delete_entry_head (entry_inst, deleteaction);
    else if(selected_tracks)
	delete_track_head (deleteaction);
}

/* Edit selected smart playlist */
static void edit_spl (GtkMenuItem *mi, gpointer data)
{
    Playlist *pl = NULL;

    if (selected_tracks)   pl = pm_get_selected_playlist ();
    if (selected_entry)    pl = pm_get_selected_playlist ();
    if (selected_playlist) pl = selected_playlist;

    spl_edit (pl);
}


static void
create_playlist_from_entries (GtkMenuItem *mi, gpointer data)
{
    generate_new_playlist (active_itdb, selected_tracks);
}

/**
 * alphabetize - alphabetize the currently selected entry
 * sort tab). The sort order is being cycled through.
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
alphabetize(GtkMenuItem *mi, gpointer data)
{
    if (selected_entry)
    {
	switch (prefs_get_st_sort ())
	{
	case SORT_ASCENDING:
	    prefs_set_st_sort (SORT_DESCENDING);
	    break;
	case SORT_DESCENDING:
	    prefs_set_st_sort (SORT_NONE);
	    break;
	case SORT_NONE:
	    prefs_set_st_sort (SORT_ASCENDING);
	    break;
	}
	st_sort (prefs_get_st_sort ());
    }
}


static void normalize_entries (GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	nm_tracks_list (selected_playlist->members);
    else if(selected_entry)
	nm_tracks_list (selected_entry->members);
    else if(selected_tracks)
	nm_tracks_list (selected_tracks);
}


/* Attach a menu item to your context menu */
/* @m - the GtkMenu we're attaching to
 * @str - a gchar* with the menu label
 * @stock - name of the stock icon (or NULL if none is to be used)
 * @func - the callback for when the item is selected (or NULL)
 * @mi - the GtkWidget we're gonna hook into the menu
 */
GtkWidget *hookup_mi (GtkWidget *m, gchar *str, gchar *stock, GCallback func, gpointer userdata)
{
    GtkWidget *mi;

    if (stock)
    {
	GtkWidget *image;
	mi = gtk_image_menu_item_new_with_mnemonic (str);
	image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
	gtk_widget_show (image);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
    }
    else
    {
	mi = gtk_menu_item_new_with_label(str);
    }
    gtk_widget_show(mi);
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_add_events(mi, GDK_BUTTON_RELEASE_MASK);
    if (func)
	g_signal_connect(G_OBJECT(mi), "activate", func, userdata);
    gtk_container_add (GTK_CONTAINER (m), mi);
    return mi;
}


/* Add separator to Menu @m and return pointer to separator widget */
GtkWidget *add_separator (GtkWidget *m)
{
    GtkWidget *sep = NULL;
    if (m)
    {
	sep = gtk_separator_menu_item_new ();
	gtk_widget_show(sep);
	gtk_widget_set_sensitive(sep, TRUE);
	gtk_container_add (GTK_CONTAINER (m), sep);
    }
    return sep;
}


void
create_context_menu(CM_type type)
{
    static GtkWidget *menu[CM_NUM];
    static GtkWidget *mi_exp[CM_NUM];  /* Export Tracks */
    static GtkWidget *mi_delpl[CM_NUM];   /* DELETE_ACTION_PLAYLIST */
    static GtkWidget *mi_delipod[CM_NUM]; /* DELETE_ACTION_IPOD     */
    static GtkWidget *mi_dellocal[CM_NUM];/* DELETE_ACTION_LOCAL    */
    static GtkWidget *mi_deldb[CM_NUM];   /* DELETE_ACTION_DATABASE */
    static GtkWidget *mi_delpcipod[CM_NUM]; /* DELETE_ACTION_IPOD   */
    static GtkWidget *mi_delsep[CM_NUM];  /* separator              */
    static GtkWidget *mi_spl[CM_NUM];  /* edit smart playlist    */
    static GtkWidget *mi_delipod_all[CM_NUM];/* DELETE_ACTION_IPOD (all
					   * tracks)      */
    static GtkWidget *mi_deldb_all[CM_NUM];  /* DELETE_ACTION_DATABASE
					   * (all tracks  */
    static GtkWidget *mi_podcasts_sep[CM_NUM]; /* Podcasts Separator */
    static GtkWidget *mi_podcasts_update[CM_NUM]; /* Update Podcasts */
    static GtkWidget *mi_podcasts_prefs[CM_NUM];  /* Podcasts Prefs */

    Playlist *pl;

    if(!menu[type])
    {
	menu[type] =  gtk_menu_new();
#if 0
	hookup_mi (menu[type], _("Edit"), NULL, G_CALLBACK (edit_entries));
#endif
	hookup_mi (menu[type], _("Play Now"), GTK_STOCK_CDROM,
		   G_CALLBACK (play_entries_now), NULL);
	hookup_mi (menu[type], _("Enqueue"), GTK_STOCK_CDROM,
		   G_CALLBACK (play_entries_enqueue), NULL);
	mi_exp[type] = hookup_mi (menu[type], 
				  _("Export Tracks"), GTK_STOCK_FLOPPY,
				  G_CALLBACK (export_entries), NULL);
	hookup_mi (menu[type], _("Create Playlist File"), GTK_STOCK_FLOPPY,
		   G_CALLBACK (create_playlist_file), NULL);
	hookup_mi (menu[type], _("Show Details"), NULL,
		   G_CALLBACK (show_details_entries), NULL);
	hookup_mi (menu[type], _("Update"), GTK_STOCK_REFRESH,
		   G_CALLBACK (update_entries), NULL);
	hookup_mi (menu[type], _("Sync Dirs"), GTK_STOCK_REFRESH,
		   G_CALLBACK (sync_dirs_entries), NULL);
	hookup_mi (menu[type], _("Normalize"), NULL,
		   G_CALLBACK (normalize_entries), NULL);
	hookup_mi (menu[type], _("Create new Playlist"),
		   GTK_STOCK_JUSTIFY_LEFT,
		   G_CALLBACK (create_playlist_from_entries), NULL);
	mi_spl[type] = hookup_mi (menu[type], _("Edit Smart Playlist"),
				  GTK_STOCK_PROPERTIES,
				  G_CALLBACK (edit_spl), NULL);

	if (type == CM_ST)
	{
	    hookup_mi (menu[type], _("Alphabetize"),
		       GTK_STOCK_SORT_ASCENDING,
		       G_CALLBACK (alphabetize), NULL);
/* example for sub menus!
	    GtkWidget *mi;
	    GtkWidget *sub;

	    mi = hookup_mi (menu[type], _("Alphabetize"), NULL, NULL);
	    sub = gtk_menu_new ();
	    gtk_widget_show (sub);
	    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub);
	    hookup_mi (sub, _("Ascending"), GTK_STOCK_SORT_ASCENDING,
		       G_CALLBACK (alphabetize_ascending), NULL);
	    hookup_mi (sub, _("Descending"), GTK_STOCK_SORT_DESCENDING,
		       G_CALLBACK (alphabetize_descending), NULL);
	    hookup_mi (sub, _("Reset"), GTK_STOCK_UNDO,
	               G_CALLBACK (reset_alphabetize), NULL);
*/
	}
	if ((type == CM_ST) || (type == CM_TM))
	{
	    mi_delsep[type] = add_separator (menu[type]);
	    mi_delipod[type] =
		hookup_mi (menu[type],
			   _("Delete From iPod"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_IPOD));
	    mi_dellocal[type] = 
		hookup_mi (menu[type],
			   _("Delete From Harddisk"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_LOCAL));
	    mi_deldb[type] =
		hookup_mi (menu[type],
			   _("Delete From Database"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_DATABASE));
	    mi_delpl[type] =
		hookup_mi (menu[type],
			   _("Delete From Playlist"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_PLAYLIST));
	}
#if LOCALDEBUG
	/* This is for debugging purposes -- this allows to inspect
	 * any track with a custom function */
	if (type == CM_TM)
	{
	    hookup_mi (menu[type], "Special", GTK_STOCK_STOP,
		       G_CALLBACK (do_special), NULL);
	}
#endif
	if (type == CM_PM)
	{
	    mi_delsep[type] = add_separator (menu[type]);
	    mi_delipod[type] =
		hookup_mi (menu[type],
			   _("Delete Including Tracks"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_IPOD));
	    mi_dellocal[type] =
		hookup_mi (menu[type],
			   _("Delete Including Tracks (Harddisk)"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_LOCAL));
	    mi_deldb[type] =
		hookup_mi (menu[type],
			   _("Delete Including Tracks (Database)"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_DATABASE));
	    mi_delpl[type] =
		hookup_mi (menu[type],
			   _("Delete But Keep Tracks"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_PLAYLIST));
	    mi_delipod_all[type] =
		hookup_mi (menu[type],
			   _("Remove All Tracks from iPod"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_IPOD));
	    mi_deldb_all[type] =
		hookup_mi (menu[type],
			   _("Remove All Tracks from Database"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_DATABASE));

	    mi_delpcipod[type] =
		hookup_mi (menu[type],
			   _("Remove All Podcasts from iPod"),
			   GTK_STOCK_DELETE,
			   G_CALLBACK (delete_entries),
			   GINT_TO_POINTER (DELETE_ACTION_IPOD));

	    mi_podcasts_sep[type] = add_separator (menu[type]);

	    mi_podcasts_update[type] =
		hookup_mi (menu[type],
			   _("Update Podcasts"),
			   GTK_STOCK_REFRESH,
			   G_CALLBACK (podcast_fetch),
			   GINT_TO_POINTER (DELETE_ACTION_DATABASE));

	    mi_podcasts_prefs[type] =
		hookup_mi (menu[type],
			   _("Podcasts Preferences"),
			   GTK_STOCK_PREFERENCES,
			   G_CALLBACK (prefs_window_podcasts),
			   GINT_TO_POINTER (DELETE_ACTION_DATABASE));
	}
    }
    /* Make sure, only available options are displayed */
    pl = pm_get_selected_playlist();
    if (pl)
    {
	iTunesDB *itdb = pl->itdb;
	g_return_if_fail (itdb);

	/* Make sure, only available delete options are displayed */
	switch (type)
	{
	case CM_PM:
	    gtk_widget_hide (mi_spl[type]);
	    gtk_widget_hide (mi_dellocal[type]);
	    gtk_widget_hide (mi_delpl[type]);
	    gtk_widget_hide (mi_deldb[type]);
	    gtk_widget_hide (mi_deldb_all[type]);
	    gtk_widget_hide (mi_delsep[type]);
	    gtk_widget_hide (mi_delipod[type]);
	    gtk_widget_hide (mi_delipod_all[type]);
	    gtk_widget_hide (mi_delpcipod[type]);
	    gtk_widget_hide (mi_dellocal[type]);
	    gtk_widget_hide (mi_podcasts_sep[type]);
	    gtk_widget_hide (mi_podcasts_update[type]);
	    gtk_widget_hide (mi_podcasts_prefs[type]);

	    if (pl->is_spl)
	    {
		gtk_widget_show (mi_spl[type]);
	    }
	    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	    {
		if (itdb_playlist_is_mpl (pl))
		{
		    gtk_widget_show (mi_delipod_all[type]);
		}
		else
		{
		    if (itdb_playlist_is_podcasts (pl))
		    {
			gtk_widget_show (mi_delpcipod[type]);
		    }
		    else
		    {
			gtk_widget_show (mi_delsep[type]);
			gtk_widget_show (mi_delipod[type]);
			gtk_widget_show (mi_delpl[type]);
		    }
		}
	    }
	    if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	    {
		if (itdb_playlist_is_mpl (pl))
		{
		    gtk_widget_show (mi_deldb_all[type]);
		}
		else
		{
		    gtk_widget_show (mi_delsep[type]);
		    gtk_widget_show (mi_dellocal[type]);
		    gtk_widget_show (mi_deldb[type]);
		    gtk_widget_show (mi_delpl[type]);
		}
	    }

	    if (itdb->usertype & GP_ITDB_TYPE_PODCASTS)
	    {
		gtk_widget_show (mi_delsep[type]);
		gtk_widget_show (mi_podcasts_sep[type]);
		gtk_widget_show (mi_podcasts_update[type]);
		gtk_widget_show (mi_podcasts_prefs[type]);
	    }
	    break;
	case CM_ST:
	case CM_TM:
	    gtk_widget_hide (mi_spl[type]);
	    gtk_widget_hide (mi_dellocal[type]);
	    gtk_widget_hide (mi_deldb[type]);
	    gtk_widget_hide (mi_delpl[type]);
	    gtk_widget_hide (mi_delipod[type]);

	    if (pl->is_spl)
	    {
		gtk_widget_show (mi_spl[type]);
	    }
	    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	    {
		gtk_widget_show (mi_delipod[type]);
		if (!itdb_playlist_is_mpl (pl) &&
		    !itdb_playlist_is_podcasts (pl))
		{
		    gtk_widget_show (mi_delpl[type]);
		}
	    }
	    if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	    {
		gtk_widget_show (mi_dellocal[type]);
		gtk_widget_show (mi_deldb[type]);
		/* actually, local repositories are not supposed to
		   have podcasts playlists, but for completeness' sake
		   I'll test anyway*/
		if(!itdb_playlist_is_mpl (pl) &&
		   !itdb_playlist_is_podcasts (pl))
		{
		    gtk_widget_show (mi_delpl[type]);
		}
	    }
	    break;
	case CM_NUM:  /* to avoid compiler warning */
	    break;
	}
	/* turn 'export tracks' insensitive when necessary */
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	{
	    gtk_widget_set_sensitive (mi_exp[type],
				      !prefs_get_offline ());
	}
	else
	{
	    gtk_widget_set_sensitive (mi_exp[type], TRUE);
	}
    }


    /* 
     * button should be button 0 as per the docs because we're calling
     * from a button release event
     */
    gtk_menu_popup(GTK_MENU(menu[type]), NULL, NULL,
	    NULL, NULL, 0, gtk_get_current_event_time()); 
}

/**
 * tm_context_menu_init - initialize the right click menu for tracks
 */
void
tm_context_menu_init(void)
{
    if (widgets_blocked) return;

    tm_stop_editing (TRUE);

    selected_entry = NULL; 
    selected_playlist = NULL;
    active_itdb = gp_get_active_itdb ();
    entry_inst = -1;
    if (selected_tracks)  g_list_free (selected_tracks);
    selected_tracks = tm_get_selected_tracks();
    if(selected_tracks)
    {
	create_context_menu (CM_TM);
    }
}
/**
 * pm_context_menu_init - initialize the right click menu for playlists 
 */
void
pm_context_menu_init(void)
{
    if (widgets_blocked) return;

    pm_stop_editing (TRUE);

    if (selected_tracks)  g_list_free (selected_tracks);
    selected_tracks = NULL;
    selected_entry = NULL;
    entry_inst = -1;
    selected_playlist = pm_get_selected_playlist();
    active_itdb = gp_get_active_itdb ();
    if(selected_playlist)
    {
	selected_tracks = g_list_copy (selected_playlist->members);
	create_context_menu (CM_PM);
    }
}
/**
 * st_context_menu_init - initialize the right click menu for sort tabs 
 * FIXME: This needs to be hooked in.
 */
void
st_context_menu_init(gint inst)
{
    if (widgets_blocked) return;

    st_stop_editing (inst, TRUE);

    if (selected_tracks)  g_list_free (selected_tracks);
    selected_tracks = NULL;
    selected_playlist = NULL;
    selected_entry = st_get_selected_entry (inst);
    active_itdb = gp_get_active_itdb ();
    if(selected_entry)
    {
	entry_inst = inst;
	selected_tracks = g_list_copy (selected_entry->members);
	create_context_menu (CM_ST);
    }
}
