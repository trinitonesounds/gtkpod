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

static gchar *messages[] = {
    "Are you sure you want to delete "
};

static gchar *titles[] = {
    "Delete Playlist?",
    "Delete Song Completely?",
    "Delete Song From Playlist?",
};

static GtkWidget *confirmation_window = NULL;
static int confirmation_type = 0;
static GList *selected_songs = NULL;
static Playlist *selected_playlist = NULL;

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
			confirmation_window_ok_clicked();
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
			confirmation_window_ok_clicked();
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

void confirmation_window_ok_clicked(void)
{
    switch(confirmation_type)
    {
	case CONFIRMATION_WINDOW_PLAYLIST:
	    fprintf(stderr, "Ok to delete this playlist\n");
	    break;
	case CONFIRMATION_WINDOW_SONG_FROM_IPOD:
	    fprintf(stderr, "Ok to delete this song from ipod\n");
	    break;
	case CONFIRMATION_WINDOW_SONG_FROM_PLAYLIST:
	    fprintf(stderr, "Ok to delete this song from playlist\n");
	    break;
	default:
	    fprintf(stderr, "Unknown confirmation Ok clicked\n");
	    break;
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
