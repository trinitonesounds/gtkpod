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
#include "misc.h"
#include "delete_window.h"
#include "display.h"
#include "interface.h"
#include "support.h"
#include "prefs.h"
#include "playlist.h"
#include "song.h"
#include <stdio.h>


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
    
    if(!selected_songs)   selected_songs = get_currently_selected_songs();
    for(l = selected_songs; l; l = l->next)
    {
	s = (Song*)l->data;
	if(result)
	{
	    snprintf(buf, PATH_MAX, "%s\n%s-%s", result, s->artist, s->title);
	    g_free(result);
	}
	else
	{
	    snprintf(buf, PATH_MAX, "%s-%s", s->artist, s->title);
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
    if (confirmation_window) gtk_widget_destroy(confirmation_window);
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
 *
 */
static void
set_message_label_to_string(GtkWidget *w, gchar *str)
{
    GtkTextBuffer *tv = NULL;

    tv = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(tv, str, strlen(str));
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(w), tv);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(w), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(w), FALSE);
}

/**
 * create_ipod_song_deleteion_interface - create a dialog window to delete
 * all traces of an mp3 on your system.
 */
static void
create_ipod_song_deletion_interface(void)
{
    GtkWidget *w = NULL;
    gchar *buf;
    gchar *song_name = NULL;
    gint n;

    song_name = get_current_selected_song_name();
    n = g_list_length (selected_songs);
    buf = ngettext ("Are you sure you want to delete the following song\ncompletely from your ipod?", "Are you sure you want to delete the following songs\ncompletely from your ipod?", n);
    if(buf && (w = lookup_widget(confirmation_window, "msg_label_title")))
	gtk_label_set_text(GTK_LABEL(w), buf);
    if(song_name && (w = lookup_widget(confirmation_window, "msg_label")))
	set_message_label_to_string(w, song_name);
    gtk_window_set_title(GTK_WINDOW(confirmation_window), _("Delete Song Completely?"));
    gtk_widget_show(confirmation_window);
    if(song_name) g_free(song_name);
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
    gchar *buf;
    gchar *song_name = NULL;
    gint n;

    song_name = get_current_selected_song_name();
    n = g_list_length (selected_songs);
    buf = g_strdup_printf(ngettext ("Are you sure you want to delete the following song\nfrom the playlist \"%s\"?", "Are you sure you want to delete the following songs\nfrom the playlist \"%s\"?", n), pl_name);
    if(buf && (w = lookup_widget(confirmation_window, "msg_label_title")))
	gtk_label_set_text(GTK_LABEL(w), buf);
    if(song_name && (w = lookup_widget(confirmation_window, "msg_label")))
	set_message_label_to_string(w, song_name);
    gtk_window_set_title(GTK_WINDOW(confirmation_window), _("Delete Song From Playlist?"));
    gtk_widget_show(confirmation_window);
    if(song_name) g_free(song_name);
    if(buf) g_free(buf);
}

void
create_playlist_deletion_interface(const gchar *pl_name)
{
    GtkWidget *w = NULL;
    gchar *buf;

    buf = g_strdup_printf(_("Are you sure you want to delete the playlist \"%s\"?"), pl_name);
    if(buf && (w = lookup_widget(confirmation_window, "msg_label_title")))
	gtk_label_set_text(GTK_LABEL(w), buf);
    gtk_window_set_title(GTK_WINDOW(confirmation_window), _("Delete Playlist?"));
    gtk_widget_show(confirmation_window);
    if(buf) g_free(buf);
}

static void create_ipod_dir_interface()
{
    GtkWidget *w = NULL;
    gchar *mp;
    GString *str;
    gint i;

    confirmation_window = create_create_confirmation();
    mp = prefs_get_ipod_mount ();
    if (mp)
    {
	if (strlen (mp) > 0)
	{ /* make sure the mount point does not end in "/" */
	    if (mp[strlen (mp) - 1] == '/')
		mp[strlen (mp) - 1] = 0;
	}
    }
    else
    {
	mp = g_strdup ("");
    }
    str = g_string_sized_new (2000);
    g_string_append_printf (str, "%s/iPod_Control\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/iTunes\n", mp);
    for(i = 0; i < 20; i++)
    {
	g_string_append_printf (str, "%s/iPod_Control/Music/F%02d\n", mp, i);
    }
    if((w = lookup_widget(confirmation_window, "msg_label")))
	set_message_label_to_string(w, str->str);
    g_string_free (str, TRUE);
    if((w = lookup_widget(confirmation_window, "msg_label_title")))
	gtk_label_set_text(GTK_LABEL(w),
			   _("OK to create the following directories?"));
    gtk_window_set_title(GTK_WINDOW(confirmation_window),
			 _("Create iPod directories"));
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

	switch(confirmation_type)
	{
	    case CONFIRMATION_WINDOW_PLAYLIST:
		pl_name = get_current_selected_playlist_name();
		if(pl_name && selected_playlist->type == PL_TYPE_NORM)
		{
		    confirmation_window = create_delete_confirmation_pl();
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
		selected_songs = get_currently_selected_songs();
		pl_name = get_current_selected_playlist_name();
		if (!selected_songs || !pl_name)
		{  /* no songs selected */
		    confirmation_window_cleanup();
		    break;
		}
		confirmation_window = create_delete_confirmation();
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
	    case CONFIRMATION_WINDOW_CREATE_IPOD_DIRS:
		create_ipod_dir_interface();
		break;
	    default:
		fprintf(stderr, "Programming error: Unknown confirmation Ok clicked\n");
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
	gchar buf[PATH_MAX];
	guint songs_length = 0;

	memset(&buf, 0, PATH_MAX);
	songs_length = g_list_length(selected_songs);
	switch(confirmation_type)
	{
	    case CONFIRMATION_WINDOW_PLAYLIST:
		if(selected_playlist->type != PL_TYPE_MPL)
		{
		    remove_playlist(selected_playlist);
		    snprintf(buf, PATH_MAX, "%s %s", (_("Deleted Playlist")),
			    selected_playlist->name);
		}
		break;
	    case CONFIRMATION_WINDOW_SONG_FROM_IPOD:
		if(songs_length > 1)	    
		    snprintf(buf, PATH_MAX, "%s", 
			    (_("Deleted Songs Completely from iPod")));
		else if(songs_length >= 0)	    
		    snprintf(buf, PATH_MAX, "%s", 
			    (_("Deleted Song Completely from iPod")));
		for (l = selected_songs; l; l = l->next)
		{
		    s = (Song *) l->data;
		    remove_song_from_playlist(NULL, s);
		}
		break;
	    case CONFIRMATION_WINDOW_SONG_FROM_PLAYLIST:
		if(songs_length > 1)	    
		    snprintf(buf, PATH_MAX, "%s %s", 
			    (_("Deleted Songs from Playlist")), 
			    selected_playlist->name);
		else if(songs_length >= 0)	    
		    snprintf(buf, PATH_MAX, "%s %s", 
			    (_("Deleted Song from Playlist")), 
			    selected_playlist->name);
		for (l = selected_songs; l; l = l->next)
		{
		    s = (Song *) l->data;
		    remove_song_from_playlist(selected_playlist, s);
		}
		break;
	    case CONFIRMATION_WINDOW_CREATE_IPOD_DIRS:
		break;
	    default:
		fprintf(stderr, "Programming error: Unknown confirmation Ok clicked\n");
		break;
	}
	if(strlen(buf) > 0)
	    gtkpod_statusbar_message(buf);
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
	    fprintf(stderr, "Programming error: Unknown confirmation toggled\n");
	    break;
    }
}
