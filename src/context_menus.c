/*
|  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
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

#include "display.h"
#include "file.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "tools.h"
#include "itdb.h"

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


/**
 * export_entries - export the currently selected files to disk
 * @mi - the menu item selected
 * @data - ignored, shoould be NULL
 */
static void 
export_entries(GtkWidget *w, gpointer data)
{
    if(selected_tracks)
	export_files_init(selected_tracks);
}

/**
 * create_playlist_file - write a playlist file containing the
 * currently selected tracks.
 * @mi - the menu item selected
 * @data - ignored, shoould be NULL
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
    if (selected_playlist)
	delete_playlist_head (FALSE);
    else if(selected_entry)
	delete_entry_head (entry_inst, FALSE);
    else if(selected_tracks)
	delete_track_head (FALSE);
}

/**
 * delete_entries_full - delete the currently selected entry, be it a playlist,
 * items in a sort tab, or tracks. All member tracks are removed from
 * the iPod completely.
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
delete_entries_full(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
    {
	delete_playlist_head (TRUE);
    }
    else if (selected_entry)
    {
	delete_entry_head (entry_inst, TRUE);
    }
    else if (selected_tracks)
	delete_track_head (TRUE);
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
GtkWidget *hookup_mi (GtkWidget *m, gchar *str, gchar *stock, GCallback func)
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
	g_signal_connect(G_OBJECT(mi), "activate", func, NULL);
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
    static GtkWidget *mi_d[CM_NUM];   /* delete but keep track menu */
    static GtkWidget *mi_df[CM_NUM];  /* delete with tracks menu */
    static GtkWidget *mi_sep[CM_NUM]; /* separator */
    static GtkWidget *mi_spl[CM_NUM]; /* edit smart playlist */
    Playlist *pl;

    if(!menu[type])
    {
	menu[type] =  gtk_menu_new();
#if 0
	hookup_mi (menu[type], _("Edit"), NULL, G_CALLBACK (edit_entries));
#endif
	hookup_mi (menu[type], _("Play Now"), "gtk-cdrom",
		   G_CALLBACK (play_entries_now));
	hookup_mi (menu[type], _("Enqueue"), "gtk-cdrom",
		   G_CALLBACK (play_entries_enqueue));
	hookup_mi (menu[type], _("Copy from iPod"), "gtk-floppy",
		   G_CALLBACK (export_entries));
	hookup_mi (menu[type], _("Create Playlist File"), "gtk-floppy",
		   G_CALLBACK (create_playlist_file));
	hookup_mi (menu[type], _("Update"), "gtk-refresh",
		   G_CALLBACK (update_entries));
	hookup_mi (menu[type], _("Sync Dirs"), "gtk-refresh",
		   G_CALLBACK (sync_dirs_entries));
	hookup_mi (menu[type], _("Normalize"), NULL,
		   G_CALLBACK (normalize_entries));

	if (type == CM_ST)
	{
	    hookup_mi (menu[type], _("Alphabetize"), "gtk-sort-ascending",
		       G_CALLBACK (alphabetize));
/* example for sub menus!
	    GtkWidget *mi;
	    GtkWidget *sub;

	    mi = hookup_mi (menu[type], _("Alphabetize"), NULL, NULL);
	    sub = gtk_menu_new ();
	    gtk_widget_show (sub);
	    gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub);
	    hookup_mi (sub, _("Ascending"), "gtk-sort-ascending",
		       G_CALLBACK (alphabetize_ascending));
	    hookup_mi (sub, _("Descending"), "gtk-sort-descending",
		       G_CALLBACK (alphabetize_descending));
	    hookup_mi (sub, _("Reset"), "gtk-undo",
	               G_CALLBACK (reset_alphabetize));
*/
	}
	if ((type == CM_ST) || (type == CM_TM))
	{
	    hookup_mi (menu[type], _("Create new Playlist"), "gtk-justify-left",
		       G_CALLBACK (create_playlist_from_entries));
	    mi_sep[type] = add_separator (menu[type]);
	    mi_d[type] = hookup_mi (menu[type], _("Delete From Playlist"),
				    "gtk-delete",
				    G_CALLBACK (delete_entries));
	    mi_df[type] = hookup_mi (menu[type], _("Delete From iPod"),
				     "gtk-delete",
				     G_CALLBACK (delete_entries_full));
	    mi_spl[type] = hookup_mi (menu[type], _("Edit Smart Playlist"),
				      "gtk-properties",
				      G_CALLBACK (edit_spl));
	}
	if (type == CM_PM)
	{
	    mi_sep[type] = add_separator (menu[type]);
	    mi_d[type] = hookup_mi (menu[type], _("Delete But Keep Tracks"),
				    "gtk-delete",
				    G_CALLBACK (delete_entries));
	    mi_df[type] = hookup_mi (menu[type], _("Delete Including Tracks"),
				     "gtk-delete",
				     G_CALLBACK (delete_entries_full));
	    mi_spl[type] = hookup_mi (menu[type], _("Edit Smart Playlist"),
				      "gtk-properties",
				      G_CALLBACK (edit_spl));
	}
    }
    /* Make sure, only available delete options are displayed */
    pl = pm_get_selected_playlist();
    if (pl)
    {
	switch (type)
	{
	case CM_PM:
	    if (pl->is_spl)
	    {
		gtk_widget_show (mi_sep[type]);
		gtk_widget_show (mi_d[type]);
		gtk_widget_hide (mi_df[type]);
		gtk_widget_show (mi_spl[type]);
	    }
	    else
	    {
		gtk_widget_hide (mi_spl[type]);
		if (pl->type == ITDB_PL_TYPE_NORM)
		{
		    gtk_widget_show (mi_sep[type]);
		    gtk_widget_show (mi_d[type]);
		    gtk_widget_show (mi_df[type]);
		}
		else
		{
		    gtk_widget_hide (mi_sep[type]);
		    gtk_widget_hide (mi_d[type]);
		    gtk_widget_hide (mi_df[type]);
		}
	    }
	    break;
	case CM_ST:
	case CM_TM:
	    if (pl->is_spl)
	    {
		gtk_widget_hide (mi_d[type]);
		gtk_widget_hide (mi_df[type]);
		gtk_widget_show (mi_spl[type]);
	    }
	    else
	    {
		gtk_widget_hide (mi_spl[type]);
		if (pl->type == ITDB_PL_TYPE_NORM)
		{
		    gtk_widget_show (mi_d[type]);
		    gtk_widget_show (mi_df[type]);
		}
		else
		{
		    gtk_widget_hide (mi_d[type]);
		    gtk_widget_show (mi_df[type]);
		}
	    }
	    break;
	case CM_NUM:  /* to avoid compiler warning */
	    break;
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
