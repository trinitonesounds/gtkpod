/* Time-stamp: <2003-06-29 12:11:56 jcs>
|
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
static struct cfg *origcfg = NULL;

/* keeps the check buttons for "Select Entry 'All' in Sorttab %d" */
static GtkWidget *autoselect_widget[SORT_TAB_MAX];

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


/* make the tooltips visible or hide it depending on the value set in
 * the prefs (tooltips_prefs) */
void prefs_window_show_hide_tooltips (void)
{
    if (prefs_window)
    {
	GtkTooltips *tt = GTK_TOOLTIPS (lookup_widget (prefs_window,
						      "tooltips"));
	if (tt)
	{
	    if (prefs_get_display_tooltips_prefs ()) gtk_tooltips_enable (tt);
	    else                                     gtk_tooltips_disable (tt);
	}
    }
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
	
	if(!tmpcfg && !origcfg)
	{
	    tmpcfg = clone_prefs();
	    origcfg = clone_prefs();
	}
	else
	{
	    g_warning ("Programming error: tmpcfg is not NULL wtf !!\n");
	    return;
	}
	prefs_window = create_prefs_window();
	if((w = lookup_widget(prefs_window, "cfg_mount_point")))
	{
	    if (tmpcfg->ipod_mount)
	    {  /* we should copy the new path first because by setting
		  the text we might get a callback destroying the old
		  value... */
		gchar *buf = g_strdup (tmpcfg->ipod_mount);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free (buf);
	    }
	}
	if((w = lookup_widget(prefs_window, "play_now_path_entry")))
	{
	    if (tmpcfg->play_now_path)
	    {  /* we should copy the new path first because by setting
		  the text we might get a callback destroying the old
		  value... */
		gchar *buf = g_strdup (tmpcfg->play_now_path);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free (buf);
	    }
	}
	if((w = lookup_widget(prefs_window, "play_enqueue_path_entry")))
	{
	    if (tmpcfg->play_enqueue_path)
	    {  /* we should copy the new path first because by setting
		  the text we might get a callback destroying the old
		  value... */
		gchar *buf = g_strdup (tmpcfg->play_enqueue_path);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free (buf);
	    }
	}
	if((w = lookup_widget(prefs_window, "time_format_entry")))
	{
	    if (tmpcfg->time_format)
	    {  /* we should copy the new path first because by setting
		  the text we might get a callback destroying the old
		  value... */
		gchar *buf = g_strdup (tmpcfg->time_format);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free (buf);
	    }
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
	if((w = lookup_widget(prefs_window, "cfg_show_duplicates")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_duplicates);
	    if (!tmpcfg->md5songs) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_show_updated")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_updated);
	    if (!tmpcfg->update_existing) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_show_non_updated")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_non_updated);
	    if (!tmpcfg->update_existing) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_show_sync_dirs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_sync_dirs);
	}
	if((w = lookup_widget(prefs_window, "cfg_sync_remove")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->sync_remove);
	}
	if((w = lookup_widget(prefs_window, "cfg_display_toolbar")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->display_toolbar);
	}
	if((w = lookup_widget(prefs_window, "cfg_toolbar_style_icons")))
	{
	    if (tmpcfg->toolbar_style == GTK_TOOLBAR_ICONS)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	    if (!tmpcfg->display_toolbar) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_toolbar_style_text")))
	{
	    if (tmpcfg->toolbar_style == GTK_TOOLBAR_TEXT)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	    if (!tmpcfg->display_toolbar) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_toolbar_style_both")))
	{
	    if (tmpcfg->toolbar_style == GTK_TOOLBAR_BOTH)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	    if (!tmpcfg->display_toolbar) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_display_tooltips_main")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->display_tooltips_main);
	}
	if((w = lookup_widget(prefs_window, "cfg_display_tooltips_prefs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->display_tooltips_prefs);
	}
	if((w = lookup_widget(prefs_window, "cfg_multi_edit")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->multi_edit);
	}
	if((w = lookup_widget(prefs_window, "cfg_not_played_song")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->not_played_song);
	}
	if((w = lookup_widget(prefs_window, "cfg_misc_song_nr")))
	{
	    gtk_spin_button_set_range (GTK_SPIN_BUTTON (w),
				       0, 100);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				       prefs_get_misc_song_nr ());
	    prefs_window_set_misc_song_nr (tmpcfg->misc_song_nr);
	}
	if((w = lookup_widget(prefs_window, "cfg_multi_edit_title")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->multi_edit_title);
	    gtk_widget_set_sensitive (w, tmpcfg->multi_edit);
	}
	if((w = lookup_widget(prefs_window, "cfg_update_charset")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->update_charset);
	}
	if((w = lookup_widget(prefs_window, "cfg_save_sorted_order")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->save_sorted_order);
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
	if((w = lookup_widget(prefs_window, "cfg_write_charset")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->write_charset);
	    if (!tmpcfg->id3_write) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = lookup_widget(prefs_window, "cfg_add_recursively")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->add_recursively);
	}
	if((w = lookup_widget(prefs_window, "cfg_case_sensitive")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->case_sensitive);
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
	if((w = lookup_widget(prefs_window, "cfg_sync_remove_confirm")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.syncing);
	    gtk_widget_set_sensitive (w, tmpcfg->sync_remove);
	}
	if((w = lookup_widget(prefs_window, "cfg_sync_remove_confirm2")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.syncing);
	}
	if((w = lookup_widget(prefs_window, "cfg_autoimport")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->autoimport);
	}
	if((w = lookup_widget(prefs_window, "autoselect_hbox")))
	{
	    for (i=0; i<SORT_TAB_MAX; ++i)
	    {
		gchar *buf;
		GtkWidget *as;
		gint padding;

		buf = g_strdup_printf ("%d", i+1);
		as = gtk_check_button_new_with_mnemonic (buf);
		autoselect_widget[i] = as;
		gtk_widget_show (as);
		if (i==0) padding = 0;
		else      padding = 5;
		gtk_box_pack_start (GTK_BOX (w), as, FALSE, FALSE, padding);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(as),
					     tmpcfg->st[i].autoselect);
		g_signal_connect ((gpointer)as,
				  "toggled",
				  G_CALLBACK (on_cfg_st_autoselect_toggled),
				  (gpointer)i);
		g_free (buf);
	    }
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

	if ((w = lookup_widget (prefs_window, "notebook")))
	{
	    gtk_notebook_set_current_page (GTK_NOTEBOOK (w),
					   prefs_get_last_prefs_page ());
	}
	if ((w = lookup_widget (prefs_window, "cfg_automount_ipod")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    prefs_get_automount());
	}
	if ((w = lookup_widget (prefs_window, "cfg_sort_tab_num_sb")))
	{
	    gtk_spin_button_set_range (GTK_SPIN_BUTTON (w),
				       0, SORT_TAB_MAX);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				       prefs_get_sort_tab_num ());
	    prefs_window_set_sort_tab_num (tmpcfg->sort_tab_num);
	}
	prefs_window_show_hide_tooltips ();
	gtk_widget_show(prefs_window);
    }
}


/**
 * save_gtkpod_prefs_window
 * UI has requested preferences update(by clicking ok on the prefs window)
 * Frees the tmpcfg variable
 */
static void 
prefs_window_set(void)
{
    gint i;

    if (tmpcfg)
    {
	/* Need this in case user reordered column order (we don't
	 * catch the reorder signal) */
	sm_store_col_order ();
	prefs_set_id3_write(tmpcfg->id3_write);
	prefs_set_id3_writeall(tmpcfg->id3_writeall);
	prefs_set_mount_point(tmpcfg->ipod_mount);
	prefs_set_play_now_path(tmpcfg->play_now_path);
	prefs_set_play_enqueue_path(tmpcfg->play_enqueue_path);
	prefs_set_time_format(tmpcfg->time_format);
	prefs_set_charset(tmpcfg->charset);
	prefs_set_auto_import(tmpcfg->autoimport);
	for (i=0; i<SORT_TAB_MAX; ++i) {
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
	prefs_set_sync_remove_confirm(tmpcfg->deletion.syncing);
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
	prefs_set_sort_tab_num(tmpcfg->sort_tab_num, TRUE);
	prefs_set_show_duplicates(tmpcfg->show_duplicates);
	prefs_set_show_updated(tmpcfg->show_updated);
	prefs_set_show_non_updated(tmpcfg->show_non_updated);
	prefs_set_show_sync_dirs(tmpcfg->show_sync_dirs);
	prefs_set_sync_remove(tmpcfg->sync_remove);
	prefs_set_save_sorted_order(tmpcfg->save_sorted_order);
	prefs_set_toolbar_style(tmpcfg->toolbar_style);
	prefs_set_display_toolbar(tmpcfg->display_toolbar);
	prefs_set_display_tooltips_main (tmpcfg->display_tooltips_main);
	prefs_set_display_tooltips_prefs (tmpcfg->display_tooltips_prefs);
	prefs_set_multi_edit (tmpcfg->multi_edit);
	prefs_set_multi_edit_title (tmpcfg->multi_edit_title);
	prefs_set_misc_song_nr (tmpcfg->misc_song_nr);
	prefs_set_not_played_song (tmpcfg->not_played_song);
	prefs_set_update_charset(tmpcfg->update_charset);
	prefs_set_write_charset(tmpcfg->write_charset);
	prefs_set_add_recursively(tmpcfg->add_recursively);
	prefs_set_case_sensitive(tmpcfg->case_sensitive);
	prefs_set_automount(tmpcfg->automount);

	sm_show_preferred_columns();
    }
}


/**
 * cancel_gtk_prefs_window
 * UI has requested preference changes be ignored -- write back the
 * original values
 * Frees the tmpcfg and origcfg variable
 */
void
prefs_window_cancel(void)
{
    cfg_free (tmpcfg);
    /* exchange tmpcfg for origcfg */
    tmpcfg = origcfg;
    origcfg = NULL;

    /* "save" (i.e. reset) original configs */
    prefs_window_set ();

    /* delete cfg struct */
    cfg_free (tmpcfg);
    tmpcfg = NULL;

    /* close the window */
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;
}

/* when window is deleted, we keep the currently applied prefs and
   save the notebook page */
void prefs_window_delete(void)
{
    GtkWidget *nb;

    /* delete cfg structs */
    cfg_free (tmpcfg);
    tmpcfg = NULL;
    cfg_free (origcfg);
    origcfg = NULL;

    /* save current notebook page */
    nb = lookup_widget (prefs_window, "notebook");
    prefs_set_last_prefs_page (gtk_notebook_get_current_page (
				   GTK_NOTEBOOK (nb)));
    /* close the window */
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;
}

/* apply the current settings and close the window */
/* Frees the tmpcfg and origcfg variable */
void
prefs_window_ok (void)
{
    GtkWidget *nb;

    /* save current settings */
    prefs_window_set ();

    /* delete cfg structs */
    cfg_free (tmpcfg);
    tmpcfg = NULL;
    cfg_free (origcfg);
    origcfg = NULL;

    /* save current notebook page */
    nb = lookup_widget (prefs_window, "notebook");
    prefs_set_last_prefs_page (gtk_notebook_get_current_page (
				   GTK_NOTEBOOK (nb)));
    /* close the window */
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;
}


/* apply the current settings, don't close the window */
void
prefs_window_apply (void)
{
    GtkWidget *w;
    /* save current settings */
    prefs_window_set ();
    /* reset the validated path entries */
    if((w = lookup_widget(prefs_window, "play_now_path_entry")))
    {
	gtk_entry_set_text(GTK_ENTRY(w), prefs_get_play_now_path ());
	/* tmpcfg gets set by the "changed" callback */
    }
    if((w = lookup_widget(prefs_window, "play_enqueue_path_entry")))
    {
	gtk_entry_set_text(GTK_ENTRY(w), prefs_get_play_enqueue_path ());
	/* tmpcfg gets set by the "changed" callback */
    }
    if((w = lookup_widget(prefs_window, "time_format_entry")))
    {
	gtk_entry_set_text(GTK_ENTRY(w), prefs_get_time_format ());
	/* tmpcfg gets set by the "changed" callback */
    }
}


/**
 * prefs_window_set_md5songs
 * @val - truth value of whether or not we should use the md5 hash to
 * prevent file duplication. changes temp variable 
 */
void
prefs_window_set_md5songs(gboolean val)
{
    GtkWidget *w;

    tmpcfg->md5songs = val;
    if((w = lookup_widget(prefs_window, "cfg_show_duplicates")))
	gtk_widget_set_sensitive (w, val);
}

void
prefs_window_set_block_display(gboolean val)
{
    tmpcfg->block_display = val;
}

void
prefs_window_set_update_existing(gboolean val)
{
    GtkWidget *w;

    tmpcfg->update_existing = val;
    if((w = lookup_widget(prefs_window, "cfg_show_updated")))
	gtk_widget_set_sensitive (w, val);
    if((w = lookup_widget(prefs_window, "cfg_show_non_updated")))
	gtk_widget_set_sensitive (w, val);
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
    if((w = lookup_widget(prefs_window, "cfg_write_charset")))
	gtk_widget_set_sensitive (w, val);
}

/**
 * prefs_window_set_mount_point
 * @mp - set the temporary config variable to the mount point specified
 */
void
prefs_window_set_mount_point(const gchar *mp)
{
    g_free (tmpcfg->ipod_mount);
    tmpcfg->ipod_mount = g_strdup(mp);
}

void prefs_window_set_play_now_path(const gchar *path)
{
    if (!path) return;
    g_free (tmpcfg->play_now_path);
    tmpcfg->play_now_path = g_strdup (path);
}

void prefs_window_set_play_enqueue_path(const gchar *path)
{
    if (!path) return;
    g_free (tmpcfg->play_enqueue_path);
    tmpcfg->play_enqueue_path = g_strdup (path);
}

void prefs_window_set_time_format(const gchar *format)
{
    if (!format) return;
    g_free (tmpcfg->time_format);
    tmpcfg->time_format = g_strdup (format);
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
prefs_window_set_sync_remove_confirm(gboolean val)
{
    tmpcfg->deletion.syncing = val;
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
    if ((inst >= 0) && (inst < SORT_TAB_MAX))
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

void prefs_window_set_show_duplicates (gboolean val)
{
    tmpcfg->show_duplicates = val;
}

void prefs_window_set_show_updated (gboolean val)
{
    tmpcfg->show_updated = val;
}

void prefs_window_set_show_non_updated (gboolean val)
{
    tmpcfg->show_non_updated = val;
}

void prefs_window_set_show_sync_dirs (gboolean val)
{
    tmpcfg->show_sync_dirs = val;
}

void prefs_window_set_sync_remove (gboolean val)
{
    GtkWidget *w;

    tmpcfg->sync_remove = val;
    if((w = lookup_widget(prefs_window, "cfg_sync_remove_confirm")))
	gtk_widget_set_sensitive (w, val);
}

void prefs_window_set_display_toolbar (gboolean val)
{
    GtkWidget *w1 = lookup_widget (prefs_window, "cfg_toolbar_style_icons");
    GtkWidget *w2 = lookup_widget (prefs_window, "cfg_toolbar_style_text");
    GtkWidget *w3 = lookup_widget (prefs_window, "cfg_toolbar_style_both");

    tmpcfg->display_toolbar = val;

    gtk_widget_set_sensitive (w1, val);
    gtk_widget_set_sensitive (w2, val);
    gtk_widget_set_sensitive (w3, val);
}

void prefs_window_set_display_tooltips_main (gboolean val)
{
    tmpcfg->display_tooltips_main = val;
}

void prefs_window_set_display_tooltips_prefs (gboolean val)
{
    tmpcfg->display_tooltips_prefs = val;
}

void prefs_window_set_multi_edit (gboolean val)
{
    GtkWidget *w = lookup_widget (prefs_window, "cfg_multi_edit_title");

    tmpcfg->multi_edit = val;
    gtk_widget_set_sensitive (w, val);
}

void prefs_window_set_multi_edit_title (gboolean val)
{
    tmpcfg->multi_edit_title = val;
}

void prefs_window_set_update_charset (gboolean val)
{
    tmpcfg->update_charset = val;
}

void prefs_window_set_not_played_song (gboolean val)
{
    tmpcfg->not_played_song = val;
}

void prefs_window_set_misc_song_nr (gint val)
{
    tmpcfg->misc_song_nr = val;
}

void prefs_window_set_write_charset (gboolean val)
{
    tmpcfg->write_charset = val;
}

void prefs_window_set_add_recursively (gboolean val)
{
    tmpcfg->add_recursively = val;
}

void prefs_window_set_case_sensitive (gboolean val)
{
    tmpcfg->case_sensitive = val;
}

void prefs_window_set_save_sorted_order (gboolean val)
{
    tmpcfg->save_sorted_order = val;
}

void prefs_window_set_sort_tab_num (gint num)
{
    gint i;

    tmpcfg->sort_tab_num = num;
    for (i=0; i<SORT_TAB_MAX; ++i)
    {   /* make all checkboxes with i<num sensitive, the others
	   insensitive */
	gtk_widget_set_sensitive (autoselect_widget[i], i<num);
    }
}

void prefs_window_set_toolbar_style (GtkToolbarStyle style)
{
    tmpcfg->toolbar_style = style;
}

void prefs_window_set_automount(gboolean val)
{
    tmpcfg->automount = val;
}
