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

static void prefs_window_set_st_autoselect (guint32 inst, gboolean autoselect);
static void prefs_window_set_tag_autoset (gint category, gboolean autoset);
static void prefs_window_set_col_visible (gint column, gboolean visible);

static void on_cfg_st_autoselect_toggled (GtkToggleButton *togglebutton,
					  gpointer         user_data)
{
    prefs_window_set_st_autoselect (
	(guint32)user_data,
	gtk_toggle_button_get_active(togglebutton));
}

static void on_cfg_tag_autoset_toggled (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_tag_autoset (
	(guint32)user_data,
	gtk_toggle_button_get_active(togglebutton));
}


static void on_cfg_col_visible_toggled (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_col_visible (
	(guint32)user_data,
	gtk_toggle_button_get_active(togglebutton));
}


/* turn the prefs window insensitive (if it's open) */
void block_prefs_window (void)
{
    if (prefs_window)
	gtk_widget_set_sensitive (prefs_window, FALSE);
}

/* turn the prefs window sensitive (if it's open) */
void release_prefs_window (void)
{
    if (prefs_window)
	gtk_widget_set_sensitive (prefs_window, TRUE);
}


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
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->md5songs);
	}
	if((w = lookup_widget(prefs_window, "cfg_update_existing")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->update_existing);
	}
	if((w = lookup_widget(prefs_window, "cfg_block_display")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->block_display);
	}
	if((w = lookup_widget(prefs_window, "cfg_id3_write")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->id3_write);
	}
	if((w = lookup_widget(prefs_window, "cfg_id3_writeall")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->id3_writeall);
	    if (!tmpcfg->id3_write) gtk_widget_set_sensitive (w, FALSE);
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
					     tmpcfg->st[i].autoselect);
		/* glade makes a "GTK_OBJECT (i)" which segfaults
		   because "i" is not a GTK object. So we have to set up
		   the signal handlers ourselves */
		g_signal_connect ((gpointer)w,
				  "toggled",
				  G_CALLBACK (on_cfg_st_autoselect_toggled),
				  (gpointer)i);
	    }
	    g_free (buf);
	}
	if((w = lookup_widget(prefs_window, "cfg_mpl_autoselect")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->mpl_autoselect);
	}
	for (i=0; i<SM_NUM_TAGS_PREFS; ++i)
	{
	    gchar *buf;
	    buf = g_strdup_printf ("tag_autoset%d", i);
	    if((w = lookup_widget(prefs_window,  buf)))
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					     tmpcfg->tag_autoset[i]);
		/* glade makes a "GTK_OBJECT (i)" which segfaults
		   because "i" is not a GTK object. So we have to set up
		   the signal handlers ourselves */
		g_signal_connect ((gpointer)w,
				  "toggled",
				  G_CALLBACK (on_cfg_tag_autoset_toggled),
				  (gpointer)i);
	    }
	    g_free (buf);
	}
	for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
	{
	    gchar *buf;
	    buf = g_strdup_printf ("col_visible%d", i);
	    if((w = lookup_widget(prefs_window,  buf)))
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					     tmpcfg->col_visible[i]);
		/* glade makes a "GTK_OBJECT (i)" which segfaults
		   because "i" is not a GTK object. So we have to set up
		   the signal handlers ourselves */
		g_signal_connect ((gpointer)w,
				  "toggled",
				  G_CALLBACK (on_cfg_col_visible_toggled),
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

    prefs_set_id3_write(tmpcfg->id3_write);
    prefs_set_id3_writeall(tmpcfg->id3_writeall);
    prefs_set_mount_point(tmpcfg->ipod_mount);
    prefs_set_charset(tmpcfg->charset);
    prefs_set_auto_import(tmpcfg->autoimport);
    for (i=0; i<SORT_TAB_NUM; ++i) {
	prefs_set_st_autoselect (i, tmpcfg->st[i].autoselect);
	prefs_set_st_category (i, tmpcfg->st[i].category);
    }
    for (i=0; i<SM_NUM_TAGS_PREFS; ++i) {
	prefs_set_tag_autoset (i, tmpcfg->tag_autoset[i]);
    }
    for (i=0; i<SM_NUM_COLUMNS_PREFS; ++i)
    {
	prefs_set_col_visible (i, tmpcfg->col_visible[i]);
    }
    prefs_set_mpl_autoselect (tmpcfg->mpl_autoselect);
    prefs_set_song_playlist_deletion(tmpcfg->deletion.song);
    prefs_set_song_ipod_file_deletion(tmpcfg->deletion.ipod_file);
    prefs_set_playlist_deletion(tmpcfg->deletion.playlist);
    prefs_set_write_extended_info(tmpcfg->write_extended_info);
    prefs_set_keep_backups(tmpcfg->keep_backups);
    /* we delete all stored md5 checksums if the md5 checksumming got
       disabled */
    if (prefs_get_md5songs() && !tmpcfg->md5songs)
	clear_md5_hash_from_songs ();
    /* this call well automatically destroy/setup the md5 hash table */
    prefs_set_md5songs(tmpcfg->md5songs);
    prefs_set_update_existing(tmpcfg->update_existing);
    prefs_set_block_display(tmpcfg->block_display);

    cfg_free(tmpcfg);
    tmpcfg =NULL;

    sm_show_preferred_columns();

    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;
}

/**
 * prefs_window_set_md5songs
 * @val - truth value of whether or not we should use the md5 hash to
 * prevent file duplication. changes temp variable 
 */
void
prefs_window_set_md5songs(gboolean val)
{
    tmpcfg->md5songs = val;
}

void
prefs_window_set_block_display(gboolean val)
{
    tmpcfg->block_display = val;
}

void
prefs_window_set_update_existing(gboolean val)
{
    tmpcfg->update_existing = val;
}

/**
 * prefs_window_set_id3_write
 * @val - truth value of whether or not we should allow id3 tags to be
 * interactively changed, changes temp variable
 */
void
prefs_window_set_id3_write(gboolean val)
{
    GtkWidget *w;

    tmpcfg->id3_write = val;
    if((w = lookup_widget(prefs_window, "cfg_id3_writeall")))
	gtk_widget_set_sensitive (w, val);
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

void prefs_window_set_keep_backups(gboolean active)
{
  tmpcfg->keep_backups = active;
}
void prefs_window_set_write_extended_info(gboolean active)
{
  tmpcfg->write_extended_info = active;
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
	tmpcfg->st[inst].autoselect = autoselect;
    }
}

/* Should the MPL be selected automatically? */
void prefs_window_set_mpl_autoselect (gboolean autoselect)
{
    tmpcfg->mpl_autoselect = autoselect;
}

void prefs_window_set_tag_autoset (gint category, gboolean autoset)
{
    if (category < SM_NUM_TAGS_PREFS)
	tmpcfg->tag_autoset[category] = autoset;
}

void prefs_window_set_col_visible (gint column, gboolean visible)
{
    if (column < SM_NUM_COLUMNS_PREFS)
	tmpcfg->col_visible[column] = visible;
}

void prefs_window_set_id3_writeall (gboolean val)
{
    tmpcfg->id3_writeall = val;
}
