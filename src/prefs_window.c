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

#include <stdio.h>
#include "prefs.h"
#include "prefs_window.h"
#include "song.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "charset.h"

static GtkWidget *prefs_window = NULL;
static struct cfg *tmpcfg = NULL;

static void prefs_window_song_list_init(void);
/**
 * create_gtk_prefs_window
 * Create, Initialize, and Show the preferences window
 * allocate a static cfg struct for temporary variables
 */
void
prefs_window_create(void)
{
    gint i;

    if(!prefs_window)
    {
	GtkWidget *w = NULL;
	
	if(!tmpcfg)
	{
	    tmpcfg = clone_prefs();
	}
	else
	{
	    fprintf(stderr, "Programming error: tmpcfg is not NULL wtf !!\n");
	}
	prefs_window = create_prefs_window();
	if((w = lookup_widget(prefs_window, "cfg_mount_point")))
	{
	    gtk_entry_set_text(GTK_ENTRY(w), g_strdup(tmpcfg->ipod_mount));
	}
	if((w = lookup_widget(prefs_window, "charset_combo")))
	{
	    charset_init_combo (GTK_COMBO (w));
	}
	if((w = lookup_widget(prefs_window, "cfg_md5songs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), tmpcfg->md5songs);
	}
	if((w = lookup_widget(prefs_window, "cfg_writeid3")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), tmpcfg->writeid3);
	}
	if((w = lookup_widget(prefs_window, "cfg_delete_playlist")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.playlist);
	}
	if((w = lookup_widget(prefs_window, "cfg_delete_track_from_playlist")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.song);
	}
	if((w = lookup_widget(prefs_window, "cfg_delete_track_from_ipod")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.ipod_file);
	}
	if((w = lookup_widget(prefs_window, "cfg_autoimport")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->autoimport);
	}
	for (i=0; i<SORT_TAB_NUM; ++i)
	{
	    gchar *buf;
	    buf = g_strdup_printf ("cfg_st_autoselect%d", i);
	    if((w = lookup_widget(prefs_window,  buf)))
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					     tmpcfg->st_autoselect[i]);
		/* glade makes a "GTK_OBJECT (i)" which segfaults
		   because it's not GTK object. So we have to set up
		   the signal handlers ourselves */
		g_signal_connect ((gpointer)w,
				  "toggled",
				  G_CALLBACK (on_cfg_st_autoselect_toggled),
				  (gpointer)i);
	    }
	    g_free (buf);
	}
	if((w = lookup_widget(prefs_window, "cfg_keep_backups")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->keep_backups);
	}
	if((w = lookup_widget(prefs_window, "cfg_write_extended")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->write_extended_info);
	}
	prefs_window_song_list_init();
	gtk_widget_show(prefs_window);
    }
}
/**
 * cancel_gtk_prefs_window
 * UI has requested preference changes be ignored
 * Frees the tmpcfg variable
 */
void
prefs_window_cancel(void)
{
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    cfg_free(tmpcfg);
    tmpcfg =NULL;
    prefs_window = NULL;
}

/**
 * save_gtkpod_prefs_window
 * UI has requested preferences update(by clicking ok on the prefs window)
 * Frees the tmpcfg variable
 */
void 
prefs_window_save(void)
{
    gint i;

    prefs_set_md5songs_active(tmpcfg->md5songs);
    prefs_set_writeid3_active(tmpcfg->writeid3);
    prefs_set_mount_point(tmpcfg->ipod_mount);
    prefs_set_charset(tmpcfg->charset);
    prefs_set_auto_import(tmpcfg->autoimport);
    for (i=0; i<SORT_TAB_NUM; ++i)
	prefs_set_st_autoselect (i, tmpcfg->st_autoselect[i]);
    prefs_set_song_list_show_track(tmpcfg->song_list_show.track);
    prefs_set_song_list_show_genre(tmpcfg->song_list_show.genre);
    prefs_set_song_list_show_album(tmpcfg->song_list_show.album);
    prefs_set_song_list_show_artist(tmpcfg->song_list_show.artist);
    prefs_set_song_playlist_deletion(tmpcfg->deletion.song);
    prefs_set_song_ipod_file_deletion(tmpcfg->deletion.ipod_file);
    prefs_set_playlist_deletion(tmpcfg->deletion.playlist);
    prefs_set_write_extended_info(tmpcfg->write_extended_info);
    prefs_set_keep_backups(tmpcfg->keep_backups);

    cfg_free(tmpcfg);
    tmpcfg =NULL;
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;
}

/**
 * prefs_window_set_md5songs_active
 * @val - truth value of whether or not we should use the md5 hash to
 * prevent file duplication. changes temp variable 
 */
void
prefs_window_set_md5songs_active(gboolean val)
{
    tmpcfg->md5songs = val;
}

/**
 * prefs_window_set_writeid3_active
 * @val - truth value of whether or not we should allow id3 tags to be
 * interactively changed, changes temp variable
 */
void
prefs_window_set_writeid3_active(gboolean val)
{
    tmpcfg->writeid3 = val;
}

/**
 * prefs_window_set_mount_point
 * @mp - set the temporary config variable to the mount point specified
 */
void
prefs_window_set_mount_point(const gchar *mp)
{
    if(tmpcfg->ipod_mount) g_free(tmpcfg->ipod_mount);
    tmpcfg->ipod_mount = g_strdup(mp);
}

/**
 */
static void
prefs_window_song_list_init(void)
{
    if(prefs_get_song_list_show_all())
    {
	prefs_window_set_song_list_all(TRUE);
    }
    else
    {
	prefs_window_set_song_list_all(FALSE);
    }
}

void prefs_window_set_song_list_all(gboolean val)
{
    gchar *extras[] = {
	"cfg_song_list_artist",
	"cfg_song_list_album",
	"cfg_song_list_genre",
	"cfg_song_list_track"
    };
    guint i = 0, extra_size = 4;
    GtkWidget *w = NULL;
    
    if(val)
    {
	if((w = lookup_widget(prefs_window, "cfg_song_list_all")))
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	
	for(i = 0; i < extra_size; i++)
	{
	    w = lookup_widget(prefs_window, extras[i]);
	    gtk_widget_set_sensitive(w, FALSE);
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
	}
    }
    else
    {
	gboolean button_active = FALSE;

	if((w = lookup_widget(prefs_window, "cfg_song_list_all")))
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
	
	for(i = 0; i < extra_size; i++)
	{
	    if((w = lookup_widget(prefs_window, extras[i])))
	    {
		gtk_widget_set_sensitive(w, TRUE);
		switch(i)
		{
		    case 0:
			button_active = tmpcfg->song_list_show.artist;
			break; 
		    case 1:
			button_active = tmpcfg->song_list_show.album;
			break;
		    case 2:
			button_active = tmpcfg->song_list_show.genre;
			break;
		    case 3:
			button_active = tmpcfg->song_list_show.track;
			break;
		    default:
			break;
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
			button_active);
	    }
	}
    }
}

void prefs_window_set_keep_backups(gboolean active)
{
  tmpcfg->keep_backups = active;
}
void prefs_window_set_write_extended_info(gboolean active)
{
  tmpcfg->write_extended_info = active;
}
void 
prefs_window_set_song_list_album(gboolean val)
{
    tmpcfg->song_list_show.album = val;
}
void 
prefs_window_set_song_list_track(gboolean val) 
{
    tmpcfg->song_list_show.track = val;
}
void 
prefs_window_set_song_list_genre(gboolean val)
{
    tmpcfg->song_list_show.genre = val;
}
void 
prefs_window_set_song_list_artist(gboolean val)
{
    tmpcfg->song_list_show.artist = val;
}

void 
prefs_window_set_delete_playlist(gboolean val)
{
    tmpcfg->deletion.playlist = val;
}

void 
prefs_window_set_delete_song_ipod(gboolean val)
{
    tmpcfg->deletion.ipod_file = val;
}

void 
prefs_window_set_delete_song_playlist(gboolean val)
{
    tmpcfg->deletion.song = val;
}

void
prefs_window_set_auto_import(gboolean val)
{
    tmpcfg->autoimport = val;
}

void prefs_window_set_charset (gchar *charset)
{
    prefs_cfg_set_charset (tmpcfg, charset);
}

void prefs_window_set_st_autoselect (guint32 inst, gboolean autoselect)
{
    if (inst < SORT_TAB_NUM)
    {
	tmpcfg->st_autoselect[inst] = autoselect;
    }
}
