/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
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
#include "delete_window.h"
#include "display.h"
#include "interface.h"
#include "support.h"
#include "prefs.h"
#include "playlist.h"
#include "song.h"
#include <stdio.h>

/**
 * FIXME: Internationalize?
 */
static gchar *messages[] = {
    "Are you sure you want to delete "
};

/**
 * FIXME: Internationalize?
 */
static gchar *titles[] = {
    "Delete Playlist?",
    "Delete Song Completely?",
    "Delete Song From Playlist?",
};

/**
 * confirmation_type - the type of the current delete window in context
 */
static int confirmation_type = 0;
/**
 * selected_songs - A list of (Song*) that are references to the currently
 * selected songs from the playlist
 */
static GList *selected_songs = NULL;
/**
 * selected_playlist - A references to a Playlist that is currently the
 * selected playlist in the display model
 */
static Playlist *selected_playlist = NULL;

/**
 * confirmation_window - handle for the window widget
 */
static GtkWidget *confirmation_window = NULL;

/**
 * get_current_selected_playlist - Communicates with the display model to
 * get the title of the playlist currently being displayed by the interface.
 * Returns - an allocated string with the playlist's name, you must free
 * this.
 */ 
static gchar *
get_current_selected_playlist_name(void)
{
    gchar *result = NULL;
    Playlist *pl = NULL;

    if((pl = get_currently_selected_playlist()))
    {
	    selected_playlist = pl;
	    result = g_strdup(pl->name);
    }
    return(result);
}

/**
 * get_current_selected_song_name - communicates with the display model and
 * queries the interface to return the song(s) name(s) that are selected
 */
static gchar *
get_current_selected_song_name(void)
{
    Song *s = NULL;
    GList *l = NULL;
    gchar buf[PATH_MAX];
    gchar *result = NULL;
    
    selected_songs = get_currently_selected_songs();
    for(l = selected_songs; l; l = l->next)
    {
	s = (Song*)l->data;
	if(result)
	{
	    snprintf(buf, PATH_MAX, "%s%s-%s\n", result, s->artist, s->title);
	    g_free(result);
	}
	else
	{
	    snprintf(buf, PATH_MAX, "\n%s-%s\n", s->artist, s->title);
	}
	result = g_strdup(buf);
    }
    return(result);
}

/**
 * confirmation_window_cleanup - cleanup any structures used by this
 * subsystem.
 */

static void
confirmation_window_cleanup(void)
{
    GList *l = NULL;
    confirmation_type = 0;
    gtk_widget_destroy(confirmation_window);
    confirmation_window = NULL;
    for(l = selected_songs; l; l = l->next)
    {
	l->data = NULL;	
    }
    g_list_free(l);
    selected_songs = NULL;
    selected_playlist = NULL;

}

/**
 * create_ipod_song_deleteion_interface - create a dialog window to delete
 * all traces of an mp3 on your system.
 */
static void
create_ipod_song_deletion_interface(void)
{
    GtkWidget *w = NULL;
    gchar buf[PATH_MAX];
    gchar *song_name = NULL;

    song_name = get_current_selected_song_name();
    snprintf(buf, PATH_MAX, "%s%scompletely from your ipod?", messages[0],
		song_name);
    if((w = lookup_widget(confirmation_window, "msg_label")))
	gtk_label_set_text(GTK_LABEL(w), buf);
    gtk_widget_show(confirmation_window);
    gtk_window_set_title(GTK_WINDOW(confirmation_window), titles[1]);
    if(song_name) g_free(song_name);
    gtk_widget_show(confirmation_window);
}

/**
 * create_playlist_song_deletion_interface - build the custom delete
 * interface for a "delete song from playlist X" request.  X being any
 * playlist on the ipod except for the master playlist.
 * @pl_name - the name of the playlist so it can be displayed in the delete
 * dialog
 */
static void
create_playlist_song_deletion_interface(const gchar *pl_name)
{
    GtkWidget *w = NULL;
    gchar buf[PATH_MAX];
    gchar *song_name = NULL;

    song_name = get_current_selected_song_name();
    snprintf(buf, PATH_MAX, "%s %sfrom the playlist\n%s?  ", messages[0],
	    song_name, pl_name);
    if((w = lookup_widget(confirmation_window, "msg_label")))
	gtk_label_set_text(GTK_LABEL(w), buf);
    gtk_widget_show(confirmation_window);
    gtk_window_set_title(GTK_WINDOW(confirmation_window), titles[2]);
    
    if(song_name) g_free(song_name);
    gtk_widget_show(confirmation_window);
}

void
create_playlist_deletion_interface(const gchar *pl_name)
{
    GtkWidget *w = NULL;
    gchar buf[PATH_MAX];

    snprintf(buf, PATH_MAX, "%s the playlist\n%s?", messages[0], pl_name);
    if((w = lookup_widget(confirmation_window, "msg_label")))
	gtk_label_set_text(GTK_LABEL(w), buf);
    gtk_widget_show(confirmation_window);
    gtk_window_set_title(GTK_WINDOW(confirmation_window), titles[0]);
    gtk_widget_show(confirmation_window);
}

/**
 * confirmation_window_create - Build a file deletion dialog
 * @window_type - The type of deletion it is, see the enum in delete_window
 * for the different window types.
 * Build the dialog with the name of the playlist or file(s) you could be
 * wanting to delete.   Whatever is requested to be deleted should be
 * displayed to the user.
 */
void 
confirmation_window_create(int window_type)
{
    gchar *pl_name = NULL;

    if(!confirmation_window)
    {
	confirmation_type = window_type;
	confirmation_window = create_delete_confirmation();
	    
	pl_name = get_current_selected_playlist_name();
	switch(confirmation_type)
	{
	    case CONFIRMATION_WINDOW_PLAYLIST:
		if(selected_playlist->type != PL_TYPE_MPL)
		{
		    if(prefs_get_playlist_deletion())
		    {
			create_playlist_deletion_interface(pl_name);
		    }
		    else
		    {
			confirmation_window_ok_clicked();
		    }
		}
		else
		{
		    confirmation_window_cleanup();
		}
		break;
	    case CONFIRMATION_WINDOW_SONG:
		if(selected_playlist->type == PL_TYPE_MPL)
		{
		    confirmation_type = CONFIRMATION_WINDOW_SONG_FROM_IPOD;
		    if(prefs_get_song_ipod_file_deletion())
		    {
			create_ipod_song_deletion_interface();
		    }
		    else
		    {
			gchar *tmp = NULL;

			if((tmp = get_current_selected_song_name()))
			{
			    confirmation_window_ok_clicked();
			    g_free(tmp);
			}
		    }
		}
		else
		{
		    confirmation_type = CONFIRMATION_WINDOW_SONG_FROM_PLAYLIST;
		    if(prefs_get_song_playlist_deletion())
		    {
			create_playlist_song_deletion_interface(pl_name);
		    }
		    else
		    {
			gchar *tmp = NULL;

			if((tmp = get_current_selected_song_name()))
			{
			    confirmation_window_ok_clicked();
			    g_free(tmp);
			}
		    }
		}
		break;
	    default:
		fprintf(stderr, "Unknown confirmation Ok clicked\n");
		break;
	}
	if(pl_name) g_free(pl_name);
    }
}

/**
 * confirmation_window_ok_clicked - Handle an "Ok" request for a
 * file/playlist delete dialog window.
 */
void confirmation_window_ok_clicked(void)
{
    Song *s = NULL;
    GList *l = NULL;

    if(selected_playlist)
    {
	switch(confirmation_type)
	{
	    case CONFIRMATION_WINDOW_PLAYLIST:
		if(selected_playlist->type != PL_TYPE_MPL)
		    remove_playlist(selected_playlist);
		break;
	    case CONFIRMATION_WINDOW_SONG_FROM_IPOD:
		for (l = selected_songs; l; l = l->next)
		{
		    s = (Song *) l->data;
		    remove_song_from_playlist(NULL, s);
		}
		break;
	    case CONFIRMATION_WINDOW_SONG_FROM_PLAYLIST:
		for (l = selected_songs; l; l = l->next)
		{
		    s = (Song *) l->data;
		    remove_song_from_playlist(selected_playlist, s);
		}
		break;
	    default:
		fprintf(stderr, "Unknown confirmation Ok clicked\n");
		break;
	}
    }
    confirmation_window_cleanup();
}
void confirmation_window_cancel_clicked(void)
{
    confirmation_window_cleanup();
}
void confirmation_window_prefs_toggled(gboolean val)
{
    gboolean myval = FALSE;

    if(val == TRUE)
	myval = FALSE;
    else
	myval = TRUE;

    switch(confirmation_type)
    {
	case CONFIRMATION_WINDOW_PLAYLIST:
	    prefs_set_playlist_deletion(myval);
	    break;
	case CONFIRMATION_WINDOW_SONG_FROM_PLAYLIST:
	    prefs_set_song_playlist_deletion(myval);
	    break;
	case CONFIRMATION_WINDOW_SONG_FROM_IPOD:
	    prefs_set_song_ipod_file_deletion(myval);
	    break;
	default:
	    fprintf(stderr, "Unknown confirmation toggled\n");
	    break;
    }
}