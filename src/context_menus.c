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
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "support.h"
#include "prefs.h"
#include "display.h"
#include "song.h"
#include "playlist.h"
#include "interface.h"
#include "callbacks.h"
#include "misc.h"
#include "file.h"
#include "file_export.h"
#include <sys/types.h>
#include <unistd.h>

/* Macro to attach menu items to your context menu */
/* @_m - the GtkMenu we're attaching to
 * @_mi - a GtkWidget we're gonna hook into the menu
 * @_str - a gchar* with the menu label
 * @_func - the callback for when the item is selected
 */
#define HOOKUP(_m, _mi, _str, _func) { \
    _mi = gtk_menu_item_new_with_label(_str); \
    gtk_widget_show(_mi); \
    gtk_widget_set_sensitive(_mi, TRUE); \
    g_signal_connect(G_OBJECT(_mi), "activate", G_CALLBACK(_func), NULL); \
    gtk_menu_append(_m, _mi); \
}

static GtkWidget *menu = NULL;
static GList *selected_songs = NULL;
static Playlist *selected_playlist = NULL;

/**
 * export_entries - export the currently selected files to disk
 * @mi - the menu item selected
 * @data - ignored, shoould be NULL
 */
static void 
export_entries(GtkWidget *w, gpointer data)
{
    if(selected_playlist)
	file_export_init(selected_playlist->members);
    else if(selected_songs)
	file_export_init(selected_songs);
}

/**
 * edit_entries - open a dialog to edit the song(s) or playlist
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 * FIXME: How this should work needs to be decided
 */
static void 
edit_entries(GtkButton *b, gpointer data)
{

    fprintf(stderr, "edit entries Selected\n");
}

/*
 * play_entries - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
play_entries(GtkMenuItem *mi, gpointer data)
{
    GList *l;
    gchar *str, *xmms;
    
    if(selected_playlist) selected_songs = selected_playlist->members;
    switch(fork())
    {
	case 0:
	    xmms = prefs_get_xmms_path();
	    for(l = selected_songs; l; l = l->next)
	    {
		if((str = get_song_name_on_disk_verified((Song*)l->data)))
		{
		    switch(fork())
		    {
			case 0:
			    execl(xmms, "xmms", "-e", str, NULL);
			    exit(0);
			    break;
			default:
			    break;
		    }
		}
	    }
	    exit(0);
	    break;
	default:
	    break;
    }
}

/*
 * update_entries - update the entries currently selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
update_entries(GtkMenuItem *mi, gpointer data)
{
    if(selected_playlist)
	update_selected_playlist();
    else if(selected_songs)
	update_selected_songs();
}

/**
 * delete_entries - delete the currently selected entry, be it a playlist,
 * items in a sort tab, or songs
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
delete_entries(GtkMenuItem *mi, gpointer data)
{
    if(selected_playlist)
	delete_playlist_head();
    else if(selected_songs)
	delete_song_head();
}

void
create_sm_menu(void)
{
    if(!menu)
    {
	GtkWidget *w = NULL;
	menu =  gtk_menu_new();

	HOOKUP(menu, w, _("Edit"), edit_entries);
	HOOKUP(menu, w, _("Play"), play_entries);
	HOOKUP(menu, w, _("Export"), export_entries);
	HOOKUP(menu, w, _("Update"), update_entries);
	HOOKUP(menu, w, _("Delete"), delete_entries);
    }
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
	    NULL, NULL, 3, gtk_get_current_event_time()); 
}

/**
 * sm_context_menu_init - initialize the right click menu for songs
 */
void
sm_context_menu_init(void)
{
    selected_playlist = NULL;
    if((selected_songs = get_currently_selected_songs()))
	create_sm_menu();
}
/**
 * pm_context_menu_init - initialize the right click menu for playlists 
 */
void
pm_context_menu_init(void)
{
    selected_songs = NULL;
    if((selected_playlist = get_currently_selected_playlist()) && 
	    (selected_playlist->type != PL_TYPE_MPL))
	create_sm_menu();
}
/**
 * st_context_menu_init - initialize the right click menu for sort tabs 
 * FIXME: This needs to be hooked in.
 */
void
st_context_menu_init(void)
{
    selected_songs = NULL;
    selected_playlist = NULL;
    /*
    create_sm_menu();
    */
}
