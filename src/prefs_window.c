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
#include "prefs.h"
#include "prefs_window.h"
#include "song.h"
#include "interface.h"
#include "support.h"
#include <stdio.h>

static GtkWidget *prefs_window = NULL;
static struct cfg *tmpcfg = NULL;

static void prefs_window_song_list_init(void);
/**
 * create_gtk_prefs_window
 * Create, Initialize, and Show the preferences window
 * allocate a static cfg struct for temporary variables
 */
static void
copy_cfg_songs_list_to_tmpcfg_songs_list(void)
{
    if(tmpcfg)
    {
	tmpcfg->song_list_show.artist = prefs_get_song_list_show_artist();
	tmpcfg->song_list_show.album = prefs_get_song_list_show_album();
	tmpcfg->song_list_show.track= prefs_get_song_list_show_track();
	tmpcfg->song_list_show.genre = prefs_get_song_list_show_genre();
    }
}
void
prefs_window_create(void)
{
    if(!prefs_window)
    {
	GtkWidget *w = NULL;
	
	if(!tmpcfg)
	{
	    tmpcfg = g_malloc0 (sizeof (struct cfg));
	    memset(tmpcfg, 0, sizeof(struct cfg));
	    tmpcfg->md5songs = cfg->md5songs;
	    tmpcfg->writeid3 = cfg->writeid3;
	    tmpcfg->ipod_mount = g_strdup(cfg->ipod_mount);
	    
	    copy_cfg_songs_list_to_tmpcfg_songs_list();
	}
	else
	{
	    fprintf(stderr, "tmpcfg is not NULL wtf !!\n");
	}
	
	prefs_window = create_prefs_window();
	if((w = lookup_widget(prefs_window, "cfg_mount_point")))
	{
	    gtk_entry_set_text(GTK_ENTRY(w), g_strdup(tmpcfg->ipod_mount));
	}
	if((w = lookup_widget(prefs_window, "cfg_md5songs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), tmpcfg->md5songs);
	}
	if((w = lookup_widget(prefs_window, "cfg_writeid3")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), tmpcfg->writeid3);
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
    prefs_set_md5songs_active(tmpcfg->md5songs);
    prefs_set_writeid3_active(tmpcfg->writeid3);
    prefs_set_mount_point(tmpcfg->ipod_mount);
    prefs_set_song_list_show_track(tmpcfg->song_list_show.track);
    prefs_set_song_list_show_genre(tmpcfg->song_list_show.genre);
    prefs_set_song_list_show_album(tmpcfg->song_list_show.album);
    prefs_set_song_list_show_artist(tmpcfg->song_list_show.artist);
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
    tmpcfg->ipod_mount = g_strdup_printf("%s",mp);
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
	"cfg_song_list_track",
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
