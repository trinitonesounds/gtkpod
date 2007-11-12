/*
|  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
|  Copyright (C) 2003-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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
#include "info.h"
#include "details.h"
#include "display.h"
#include "display_itdb.h"
#include "file.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "tools.h"
#include "repository.h"
#include "syncdir.h"
#include "display_coverart.h"
#include "display_photo.h"

#define LOCALDEBUG 0

static guint entry_inst = -1;
static GList *selected_tracks = NULL;
static Playlist *selected_playlist = NULL;
static TabEntry *selected_entry = NULL; 
static iTunesDB *active_itdb = NULL;
/* types of context menus (PM/ST/TM) */
typedef enum {
    CM_PL = 0,
    CM_ST,
    CM_TM,
    CM_NUM,
    CM_CAD,
    CM_PH_AV,
    CM_PH_IV
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
	gchar *mp;
	Track *tr = gl->data;
	g_return_if_fail (tr);
	g_object_get (tr->itdb->device, "mount-point", &mp, NULL);
	printf ("mountpoint: %s\n", mp);
	g_free (mp);

	printf ("track: %p: thumbnails: %p id: %d num: %d\n",
		tr, tr->artwork->thumbnails, tr->artwork->id, g_list_length (tr->artwork->thumbnails));
	if (tr->artwork->thumbnails)
	{
	    GList *gl2;
	    for (gl2=tr->artwork->thumbnails; gl2; gl2=gl2->next)
	    {
		Thumb *img = gl2->data;
		g_return_if_fail (img);
		printf ("  %s offset: %d size: %d width: %d height: %d\n", img->filename, img->offset, img->size, img->width, img->height);
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
edit_details_entries(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	details_edit (selected_playlist->members);
    else if(selected_entry)
	details_edit (selected_entry->members);
    else if(selected_tracks)
	details_edit (selected_tracks);
}

/*
 * display the dialog with the full size cd artwork cover
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void
display_track_artwork(GtkMenuItem *mi, gpointer data)
{
	if (selected_tracks)
		coverart_display_big_artwork (selected_tracks);
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
 * sync_dirs_ entries - sync the directories of the selected playlist
 *
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
sync_dirs (GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
    {
	sync_playlist (selected_playlist, NULL,
		       KEY_SYNC_CONFIRM_DIRS, 0,
		       KEY_SYNC_DELETE_TRACKS, 0,
		       KEY_SYNC_CONFIRM_DELETE, 0,
		       KEY_SYNC_SHOW_SUMMARY, 0);
    }
    else
    {
	g_return_if_reached ();
    }
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

/* Display repository options */
static void edit_properties (GtkMenuItem *mi, gpointer data)
{
    g_return_if_fail (selected_playlist);

    repository_edit (selected_playlist->itdb, selected_playlist);
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
	switch (prefs_get_int("st_sort"))
	{
	case SORT_ASCENDING:
	    prefs_set_int("st_sort", SORT_DESCENDING);
	    break;
	case SORT_DESCENDING:
	    prefs_set_int("st_sort", SORT_NONE);
	    break;
	case SORT_NONE:
	    prefs_set_int("st_sort", SORT_ASCENDING);
	    break;
	}
	st_sort (prefs_get_int("st_sort"));
    }
}


static void load_ipod (GtkMenuItem *mi, gpointer data)
{
    g_return_if_fail (selected_playlist);

    gp_load_ipod (selected_playlist->itdb);
}


static void eject_ipod (GtkMenuItem *mi, gpointer data)
{
    iTunesDB *itdb;
    ExtraiTunesDBData *eitdb;

    /* all of the checks below indicate a programming error -> give a
       warning through the g_..._fail macros */
    g_return_if_fail (selected_playlist);
    itdb = selected_playlist->itdb;
    g_return_if_fail (itdb);
    g_return_if_fail (itdb->usertype & GP_ITDB_TYPE_IPOD);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);
    g_return_if_fail (eitdb->itdb_imported == TRUE);

    gp_eject_ipod (itdb);
}


static void save_changes (GtkMenuItem *mi, gpointer data)
{
    g_return_if_fail (selected_playlist);

    gp_save_itdb (selected_playlist->itdb);
}




/* ------------------------------------------------------------

      Copying selected item(s) to target destination

   ------------------------------------------------------------ */


/*
 * Copy the selected playlist to a specified itdb.
 */
void
copy_playlist_to_target_itdb (Playlist     *pl,
			      iTunesDB     *t_itdb)
{
    Playlist	*pl_n;
    GList	*addtracks = NULL;
    g_return_if_fail (pl);
    g_return_if_fail (t_itdb);
    if (pl->itdb != t_itdb)
    {
	addtracks = export_trackglist_when_necessary (pl->itdb,
						      t_itdb,
						      pl->members);
	if (addtracks || !pl->members)
	{
	    pl_n = gp_playlist_add_new(t_itdb, pl->name, FALSE, -1);
	    add_trackglist_to_playlist (pl_n, addtracks);
	    gtkpod_statusbar_message (_("Copied \"%s\" playlist to %s"),
				      pl->name, (itdb_playlist_mpl (t_itdb))->name);
	}
	g_list_free (addtracks);
	addtracks = NULL;
    }
    else
    {
	pl_n = itdb_playlist_duplicate (pl);
	g_return_if_fail (pl_n);
	gp_playlist_add (t_itdb, pl_n, -1);
    }
}


/*
 * Copy selected tracks to a specified itdb.
 */
void
copy_tracks_to_target_itdb (GList        *tracks,
			    iTunesDB     *t_itdb)
{
    GList       *addtracks = NULL;
    Track	*first = tracks->data;
    Playlist    *mpl;
    gint        n;

    g_return_if_fail(tracks);
    g_return_if_fail(t_itdb);

    mpl = itdb_playlist_mpl (t_itdb);
    g_return_if_fail(mpl);

    addtracks = export_trackglist_when_necessary (first->itdb, t_itdb, tracks);

    if (addtracks)
    {
	add_trackglist_to_playlist (mpl, addtracks);
	n = g_list_length (addtracks);
	gtkpod_statusbar_message (ngettext ("Copied %d track to '%s'",
					    "Copied %d tracks to '%s'", n),
				  n, mpl->name);
	g_list_free (addtracks);
	addtracks = NULL;
    }
}


void
copy_playlist_to_target_playlist (Playlist     *pl,
				  Playlist     *t_pl)
{
    GList	*addtracks = NULL;
    Playlist    *t_mpl;

    g_return_if_fail (pl);
    g_return_if_fail (t_pl);

    t_mpl = itdb_playlist_mpl (t_pl->itdb);
    g_return_if_fail(t_mpl);

    addtracks = export_trackglist_when_necessary (pl->itdb,
						  t_pl->itdb,
						  pl->members);
    if (addtracks || !pl->members)
    {
	add_trackglist_to_playlist (t_pl, addtracks);
	gtkpod_statusbar_message (_("Copied '%s' playlist to '%s' in '%s'"),
				  pl->name, t_pl->name, t_mpl->name);
	g_list_free(addtracks);
	addtracks = NULL;
    }
}



void
copy_tracks_to_target_playlist (GList        *tracks,
				Playlist     *t_pl)
{
    GList	*addtracks = NULL;
    Track	*first;
    Playlist    *mpl;
    gint        n;

    g_return_if_fail (tracks);
    g_return_if_fail (t_pl);
    g_return_if_fail (t_pl->itdb);

    mpl = itdb_playlist_mpl (t_pl->itdb);
    g_return_if_fail(mpl);

    if (tracks)
    {
	first = tracks->data;
	g_return_if_fail (first);
	addtracks = export_trackglist_when_necessary (first->itdb, t_pl->itdb, tracks);
	add_trackglist_to_playlist (t_pl, addtracks);
    }
    n = g_list_length (addtracks);
    gtkpod_statusbar_message (ngettext ("Copied %d track to '%s' in '%s'",
					"Copied %d tracks to %s in '%s'", n),
			      n, t_pl->name, mpl->name);
    g_list_free (addtracks);
    addtracks = NULL;
}



static void copy_selected_to_target_itdb (GtkMenuItem *mi, gpointer *userdata)
{
    iTunesDB *t_itdb = *userdata;
    g_return_if_fail (t_itdb);
    if (selected_playlist)
	copy_playlist_to_target_itdb (selected_playlist, t_itdb);
    else if (selected_entry)
	copy_tracks_to_target_itdb (selected_entry->members, t_itdb);
    else if (selected_tracks)
	copy_tracks_to_target_itdb (selected_tracks, t_itdb);
}


static void copy_selected_to_target_playlist (GtkMenuItem *mi, gpointer *userdata)
{
    Playlist *t_pl = *userdata;
    g_return_if_fail (t_pl);
    if (selected_playlist)
	copy_playlist_to_target_playlist (selected_playlist, t_pl);
    else if (selected_entry)
	copy_tracks_to_target_playlist (selected_entry->members, t_pl);
    else if (selected_tracks)
	copy_tracks_to_target_playlist (selected_tracks, t_pl);
}


/* Attach a menu item to your context menu */
/* @m - the GtkMenu we're attaching to
 * @str - a gchar* with the menu label
 * @stock - name of the stock icon (or NULL if none is to be used)
 * @func - the callback for when the item is selected (or NULL)
 * @mi - the GtkWidget we're gonna hook into the menu
 */
GtkWidget *hookup_mi (GtkWidget *m,
		      const gchar *str,
		      const gchar *stock,
		      GCallback func,
		      gpointer userdata)
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


static GtkWidget *add_copy_selected_to_target_itdb (GtkWidget *menu,
						    const gchar *title)
{
    GtkWidget *mi;
    GtkWidget *sub;
    GtkWidget *pl_mi;
    GtkWidget *pl_sub;
    GList *itdbs;
    GList *db;
    struct itdbs_head *itdbs_head;
    iTunesDB *itdb;
    gchar *stock_id = NULL;
    Playlist *pl;

  
    g_return_val_if_fail (gtkpod_window, NULL);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
                                    "itdbs_head");

    mi = hookup_mi (menu, title,
		    GTK_STOCK_COPY,
		    NULL, NULL);
    sub = gtk_menu_new ();
    gtk_widget_show (sub);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub);

    for (itdbs=itdbs_head->itdbs; itdbs; itdbs=itdbs->next)
	    {
		itdb = itdbs->data;
		ExtraiTunesDBData *eitdb=itdb->userdata;
		if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
		{
			stock_id = GTK_STOCK_HARDDISK;
		}
		else
		{
		    if (eitdb->itdb_imported)
		    {
		        stock_id = GTK_STOCK_CONNECT;
		    }
		    else
		    {
		        stock_id = GTK_STOCK_DISCONNECT;
		    }
		}
		pl_mi = hookup_mi (sub, _(itdb_playlist_mpl(itdb)->name),
				   stock_id, NULL, NULL);
		pl_sub = gtk_menu_new ();
		gtk_widget_show (pl_sub);
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (pl_mi), pl_sub);
		hookup_mi (pl_sub, _(itdb_playlist_mpl(itdb)->name),
			   stock_id, G_CALLBACK(copy_selected_to_target_itdb), &itdbs->data);
		add_separator(pl_sub);
		for (db=itdb->playlists; db; db=db->next)
		{
			pl=db->data;
			if (!itdb_playlist_is_mpl (pl))
			{
				if (pl->is_spl)
					stock_id = GTK_STOCK_PROPERTIES;
				else
					stock_id = GTK_STOCK_JUSTIFY_LEFT;
				hookup_mi(pl_sub, _(pl->name), stock_id, 
				G_CALLBACK(copy_selected_to_target_playlist) , &db->data);
			}
		}
	    }
    return mi;
}


static GtkWidget *add_play_now (GtkWidget *menu)
{
    return hookup_mi (menu, _("Play Now"), GTK_STOCK_CDROM,
		      G_CALLBACK (play_entries_now), NULL);
}

static GtkWidget *add_enqueue (GtkWidget *menu)
{
    return hookup_mi (menu, _("Enqueue"), GTK_STOCK_CDROM,
		      G_CALLBACK (play_entries_enqueue), NULL);
}

static GtkWidget *add_copy_track_to_filesystem (GtkWidget *menu)
{
    return hookup_mi (menu, _("Copy Tracks to Filesystem"),
		      GTK_STOCK_SAVE_AS,
		      G_CALLBACK (export_entries), NULL);
}

static GtkWidget *add_create_playlist_file (GtkWidget *menu)
{
    return hookup_mi (menu, _("Create Playlist File"),
		      GTK_STOCK_SAVE_AS,
		      G_CALLBACK (create_playlist_file), NULL);
}

static GtkWidget *add_create_new_playlist (GtkWidget *menu)
{
    return hookup_mi (menu, _("Create new Playlist"),
		      GTK_STOCK_JUSTIFY_LEFT,
		      G_CALLBACK (create_playlist_from_entries), NULL);
}

static GtkWidget *add_update_tracks_from_file (GtkWidget *menu)
{
    return hookup_mi (menu, _("Update Tracks from File"),
		      GTK_STOCK_REFRESH,
		      G_CALLBACK (update_entries), NULL);
}

static GtkWidget *add_edit_smart_playlist (GtkWidget *menu)
{
    return hookup_mi (menu, _("Edit Smart Playlist"),
		      GTK_STOCK_PROPERTIES,
		      G_CALLBACK (edit_spl), NULL);
}

static GtkWidget *add_sync_playlist_with_dirs (GtkWidget *menu)
{
    return hookup_mi (menu, _("Sync Playlist with Dir(s)"),
		      GTK_STOCK_REFRESH,
		      G_CALLBACK (sync_dirs), NULL);
}

static GtkWidget *add_remove_all_tracks_from_ipod (GtkWidget *menu)
{
    GtkWidget *mi;
    GtkWidget *sub;

    mi = hookup_mi (menu, _("Remove All Tracks from iPod"),
		    GTK_STOCK_DELETE,
		    NULL, NULL);
    sub = gtk_menu_new ();
    gtk_widget_show (sub);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub);
    hookup_mi (sub, _("I'm sure"),
	       NULL,
	       G_CALLBACK (delete_entries),
	       GINT_TO_POINTER (DELETE_ACTION_IPOD));
    return mi;
}

static GtkWidget *add_remove_all_podcasts_from_ipod (GtkWidget *menu)
{
    GtkWidget *mi;
    GtkWidget *sub;

    mi = hookup_mi (menu, _("Remove All Podcasts from iPod"),
		    GTK_STOCK_DELETE,
		    NULL, NULL);
    sub = gtk_menu_new ();
    gtk_widget_show (sub);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub);
    hookup_mi (sub, _("I'm sure"),
	       NULL,
	       G_CALLBACK (delete_entries),
	       GINT_TO_POINTER (DELETE_ACTION_IPOD));
    return mi;
}

static GtkWidget *add_delete_including_tracks (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete Including Tracks"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_IPOD));
}

static GtkWidget *add_delete_but_keep_tracks (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete But Keep Tracks"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_PLAYLIST));
}

static GtkWidget *add_edit_ipod_properties (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Edit iPod Properties"),
		      GTK_STOCK_PREFERENCES,
		      G_CALLBACK (edit_properties), NULL);
}

static GtkWidget *add_edit_repository_properties (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Edit Repository Properties"),
		      GTK_STOCK_PREFERENCES,
		      G_CALLBACK (edit_properties), NULL);
}

static GtkWidget *add_edit_playlist_properties (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Edit Playlist Properties"),
		      GTK_STOCK_PREFERENCES,
		      G_CALLBACK (edit_properties), NULL);
}

static GtkWidget *add_edit_track_details (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Edit Track Details"),
		      GTK_STOCK_EDIT,
		      G_CALLBACK (edit_details_entries), NULL);
}

static GtkWidget *add_display_big_coverart (GtkWidget *menu)
{
    const gchar *icon;
/* gets defined in gtk+ V2.8, but we only require V2.6 */
#ifndef GTK_STOCK_FULLSCREEN
#define GTK_STOCK_FULLSCREEN "gtk-fullscreen"
#endif
    if (gtk_check_version (2,8,0) == NULL)
    {
	icon = GTK_STOCK_FULLSCREEN;
    }
    else
    {
	icon = NULL;
    }

    return hookup_mi (menu,  _("View Full Size Artwork"),
		      icon,
		      G_CALLBACK (display_track_artwork), NULL);	
}

static GtkWidget *add_get_cover_from_file (GtkWidget *menu)
{
		return hookup_mi (menu, _("Select Cover From File"),
					GTK_STOCK_FLOPPY,
					G_CALLBACK (coverart_set_cover_from_file), NULL);
}

static GtkWidget *add_check_ipod_files (GtkWidget *menu)
{
    /* FIXME */
    return NULL;
}

static GtkWidget *add_load_ipod (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Load iPod"),
		      GTK_STOCK_CONNECT,
		      G_CALLBACK (load_ipod), NULL);
}

static GtkWidget *add_eject_ipod (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Eject iPod"),
		      GTK_STOCK_DISCONNECT,
		      G_CALLBACK (eject_ipod), NULL);
}

static GtkWidget *add_save_changes (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Save Changes"),
		      GTK_STOCK_SAVE,
		      G_CALLBACK (save_changes), NULL);
}

static GtkWidget *add_remove_all_tracks_from_database (GtkWidget *menu)
{
    GtkWidget *mi;
    GtkWidget *sub;

    mi = hookup_mi (menu, _("Remove All Tracks from Database"),
		    GTK_STOCK_DELETE,
		    NULL, NULL);
    sub = gtk_menu_new ();
    gtk_widget_show (sub);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub);
    hookup_mi (sub, _("I'm sure"),
	       NULL,
	       G_CALLBACK (delete_entries),
	       GINT_TO_POINTER (DELETE_ACTION_DATABASE));
    return mi;
}

static GtkWidget *add_delete_including_tracks_harddisk (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete Including Tracks (Harddisk)"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_LOCAL));
}

static GtkWidget *add_delete_including_tracks_database (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete Including Tracks (Database)"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_DATABASE));
}

static GtkWidget *add_delete_from_ipod (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete From iPod"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_IPOD));
}

static GtkWidget *add_delete_from_playlist (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete From Playlist"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_PLAYLIST));
}

static GtkWidget *add_delete_from_harddisk (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete From Harddisk"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_LOCAL));
}

static GtkWidget *add_delete_from_database (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Delete From Database"),
		      GTK_STOCK_DELETE,
		      G_CALLBACK (delete_entries),
		      GINT_TO_POINTER (DELETE_ACTION_DATABASE));
}

static GtkWidget *add_alphabetize (GtkWidget *menu)
{
    return hookup_mi (menu,  _("Alphabetize"),
		      GTK_STOCK_SORT_ASCENDING,
		      G_CALLBACK (alphabetize), NULL);
}

#if LOCALDEBUG
static GtkWidget *add_special (GtkWidget *menu)
{
    return hookup_mi (menu, "Special",
		      GTK_STOCK_STOP,
		      G_CALLBACK (do_special), NULL);
}
#endif

GtkWidget *gphoto_menuitem_remove_album_from_db_item(GtkWidget *menu)
{
	return hookup_mi (
			menu, 
			_("Remove Album"), 
			GTK_STOCK_DELETE, 
			G_CALLBACK (gphoto_remove_album_from_database), 
			NULL);
}

GtkWidget *gphoto_menuitem_remove_photo_from_album_item(GtkWidget *menu)
{
	return hookup_mi (
			menu, 
			_("Remove Photo"),
			GTK_STOCK_DELETE, 
			G_CALLBACK (gphoto_remove_selected_photos_from_album), 
			NULL);
}

GtkWidget *gphoto_menuitem_rename_photoalbum_item(GtkWidget *menu)
{
	return hookup_mi (
			menu, 
			_("Rename Album"),
			GTK_STOCK_DELETE, 
			G_CALLBACK (gphoto_rename_selected_album), 
			NULL);
}

void create_context_menu (CM_type type)
{
    static GtkWidget *menu = NULL;
    Playlist *pl;

    if (menu)
    {   /* free memory for last menu */
	gtk_widget_destroy (menu);
	menu = NULL;
    }

    pl = pm_get_selected_playlist();
    if (pl)
    {
	ExtraiTunesDBData *eitdb;
	iTunesDB *itdb = pl->itdb;
	g_return_if_fail (itdb);
	eitdb = itdb->userdata;
	g_return_if_fail (eitdb);

	menu = gtk_menu_new ();

	switch (type)
	{
	case CM_PL:
	    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	    {
		if (eitdb->itdb_imported)
		{
		    add_play_now (menu);
		    add_enqueue (menu);
		    add_copy_track_to_filesystem (menu);
		    add_create_playlist_file (menu);
		    add_update_tracks_from_file (menu);
		    if (!pl->is_spl)
		    {
			add_sync_playlist_with_dirs (menu);
		    }
		    add_separator (menu);
		    if (itdb_playlist_is_mpl (pl))
		    {
			add_remove_all_tracks_from_ipod (menu);
		    }
		    else if (itdb_playlist_is_podcasts (pl))
		    {
			add_remove_all_podcasts_from_ipod (menu);
		    }
		    else
		    {
			add_delete_including_tracks (menu);
			add_delete_but_keep_tracks (menu);
		    }
		    add_copy_selected_to_target_itdb (menu,
						      _("Copy selected playlist to..."));
		    add_separator (menu);
		    add_edit_track_details (menu);
		    if (pl->is_spl)
		    {
			add_edit_smart_playlist (menu);
		    }
		    if (itdb_playlist_is_mpl (pl))
		    {
			add_edit_ipod_properties (menu);
		    }
		    else
		    {
			add_edit_playlist_properties (menu);
		    }
		    add_check_ipod_files (menu);
		    add_eject_ipod (menu);
		}
		else
		{   /* not imported */
		    add_edit_ipod_properties (menu);
		    add_check_ipod_files (menu);
		    add_separator (menu);
		    add_load_ipod (menu);
		}
	    }
	    if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	    {
		add_play_now (menu);
		add_enqueue (menu);
		add_copy_track_to_filesystem (menu);
		add_create_playlist_file (menu);
		add_update_tracks_from_file (menu);
		if (!pl->is_spl)
		{
		    add_sync_playlist_with_dirs (menu);
		}
		add_separator (menu);
		if (itdb_playlist_is_mpl (pl))
		{
		    add_remove_all_tracks_from_database (menu);
		}
		else
		{
		    add_delete_including_tracks_database (menu);
		    add_delete_including_tracks_harddisk (menu);
		    add_delete_but_keep_tracks (menu);
		}
		add_copy_selected_to_target_itdb (menu,
						  _("Copy selected playlist to..."));
		add_separator (menu);
		add_edit_track_details (menu);
		if (pl->is_spl)
		{
		    add_edit_smart_playlist (menu);
		}
		if (itdb_playlist_is_mpl (pl))
		{
		    add_edit_repository_properties (menu);
		}
		else
		{
		    add_edit_playlist_properties (menu);
		}
		add_save_changes (menu);
	    }
	    break;
	case CM_ST:
	case CM_TM:
	    add_play_now (menu);
	    add_enqueue (menu);
	    add_copy_track_to_filesystem (menu);
	    add_create_playlist_file (menu);
	    add_create_new_playlist (menu);
	    add_update_tracks_from_file (menu);
	    if (!pl->is_spl)
	    {
		add_sync_playlist_with_dirs (menu);
	    }
	    if (type == CM_ST)
	    {
		add_alphabetize (menu);
	    }
	    add_separator (menu);
	    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	    {
		add_delete_from_ipod (menu);
		if (!itdb_playlist_is_mpl (pl))
		{
		    add_delete_from_playlist (menu);
		}
	    }
	    if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	    {
		add_delete_from_harddisk (menu);
		add_delete_from_database (menu);
		if (!itdb_playlist_is_mpl (pl))
		{
		    add_delete_from_playlist (menu);
		}
	    }
	    add_copy_selected_to_target_itdb (menu,
					      _("Copy selected track(s) to..."));
	    add_separator (menu);
	    add_edit_track_details (menu);
#if LOCALDEBUG
	    /* This is for debugging purposes -- this allows to inspect
	     * any track with a custom function */
	    if (type == CM_TM)
	    {
		add_special (menu);
	    }
#endif
	    break;
	case CM_CAD:
			add_get_cover_from_file (menu);
			add_display_big_coverart (menu);
			add_edit_track_details (menu);
			break;
	case CM_PH_AV:
			gphoto_menuitem_remove_album_from_db_item (menu);
			gphoto_menuitem_rename_photoalbum_item (menu);
			break;
	case CM_PH_IV:
			gphoto_menuitem_remove_photo_from_album_item (menu);
			break;
	case CM_NUM:
	    g_return_if_reached ();
	}
    }
    /* 
     * button should be button 0 as per the docs because we're calling
     * from a button release event
     */
    if (menu)
    {
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			NULL, NULL, 0, gtk_get_current_event_time()); 
    }
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
    active_itdb = gp_get_selected_itdb ();
    entry_inst = -1;
    if (selected_tracks)  g_list_free (selected_tracks);
    selected_tracks = tm_get_selected_tracks();
    if(selected_tracks)
    {
	create_context_menu (CM_TM);
    }
}

/**
 * cad_context_menu_init - initialize the right click menu for coverart display 
 */
void
cad_context_menu_init(void)
{
    if (widgets_blocked) return;
    
    selected_entry = NULL; 
    selected_playlist = NULL;
    selected_tracks = NULL;
    active_itdb = gp_get_selected_itdb ();
    entry_inst = -1;
    
     if (selected_tracks) g_list_free (selected_tracks);
    	selected_tracks = g_list_copy (coverart_get_displayed_tracks());
    /*
    int i;
    for (i = 0; i < g_list_length(selected_tracks); ++i)
    {
    	Track *track;
    	track = g_list_nth_data (selected_tracks, i);
    	printf ("context_menu_init - Artist:%s  Album:%s  Title:%s\n", track->artist, track->album, track->title);
    }
    */
    if(selected_tracks)
			create_context_menu (CM_CAD);
}

/**
 * photo_context_menu_init - initialize the right click menu for photo management display 
 */
void
gphoto_context_menu_init (gint component)
{
    if (widgets_blocked) return;
    
    selected_entry = NULL; 
    selected_playlist = NULL;
    selected_tracks = NULL;
    active_itdb = gp_get_selected_itdb ();
    entry_inst = -1;
    
     if (selected_tracks) g_list_free (selected_tracks);

     switch (component)
     {
    	 case GPHOTO_ALBUM_VIEW:
    		 create_context_menu (CM_PH_AV);
    		 break;
    	 case GPHOTO_ICON_VIEW:
    		 create_context_menu (CM_PH_IV);
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
    
    /* Dont allow context menu to display if the playlist is the photo one */
    if (gphoto_is_photo_playlist (selected_playlist)) return;
        
    active_itdb = gp_get_selected_itdb ();
    if(selected_playlist)
    {
	selected_tracks = g_list_copy (selected_playlist->members);
	create_context_menu (CM_PL);
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
    active_itdb = gp_get_selected_itdb ();
    if(selected_entry)
    {
	entry_inst = inst;
	selected_tracks = g_list_copy (selected_entry->members);
	create_context_menu (CM_ST);
    }
}
