/* Time-stamp: <2005-04-29 12:15:37 jcs>
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
|
|  $Id$
*/

#include <stdio.h>
#include <string.h>
#include "callbacks.h"
#include "charset.h"
#include "misc.h"
#include "prefs.h"
#include "prefs_window.h"
#include "display_itdb.h"

GladeXML *prefs_window_xml;
GladeXML *sort_window_xml;

static GtkWidget *prefs_window = NULL;
static GtkWidget *sort_window = NULL;
static struct cfg *tmpcfg = NULL;
static struct cfg *origcfg = NULL;
static struct sortcfg *tmpsortcfg = NULL;
static struct sortcfg *origsortcfg = NULL;

/* keeps the check buttons for "Select Entry 'All' in Sorttab %d" */
static GtkWidget *autoselect_widget[SORT_TAB_MAX];

static void prefs_window_set_st_autoselect (guint32 inst, gboolean autoselect);
static void prefs_window_set_autosettags (gint category, gboolean autoset);
static void prefs_window_set_col_visible (gint column, gboolean visible);
static void prefs_window_set_path (PathType i, const gchar *path);

/* Definition of path button names.
   E.g. path_button_names[PATH_PLAY_ENQUEUE] is
   "play_enqueue_path_button".
   path_fileselector_titles[] specifies the title for the file
   chooser. path_type[] specifies whether to browse for dirs or for
   files.
*/
static const gchar *path_button_names[] =
{
    "play_now_path_button",
    "play_enqueue_path_button",
    "mp3gain_path_button",
    "sync_contacts_path_button",
    "sync_calendar_path_button",
    "mserv_music_root_button",
    "mserv_trackinfo_root_button",
    "sync_notes_path_button",
    NULL
};
const gchar *path_entry_names[] =
{
    "play_now_path_entry",
    "play_enqueue_path_entry",
    "mp3gain_path_entry",
    "sync_contacts_path_entry",
    "sync_calendar_path_entry",
    "mserv_music_root_entry",
    "mserv_trackinfo_root_entry",
    "sync_notes_path_entry",
    NULL
};
static const gchar *path_fileselector_titles[] =
{
    N_("Please select command for 'Play Now'"),
    N_("Please select command for 'Enqueue'"),
    N_("Please select the mp3gain executable"),
    N_("Please select command to sync contacts"),
    N_("Please select command to sync calendar"),
    N_("Select the mserv music root directory"),
    N_("Select the mserv trackinfo root directory"),
    NULL
};
static const GtkFileChooserAction path_type[] =
{
    GTK_FILE_CHOOSER_ACTION_OPEN,  /* select file */
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, /* select folder */
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    -1
};

/* pointers to fileselectors */
static GtkWidget *path_filechooser[PATH_NUM];


static void on_cfg_st_autoselect_toggled (GtkToggleButton *togglebutton,
					  gpointer         user_data)
{
    prefs_window_set_st_autoselect (
	(guint32)user_data,
	gtk_toggle_button_get_active(togglebutton));
}

static void on_cfg_autosettags_toggled (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    prefs_window_set_autosettags (
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


/* Close all open path filechoosers */
static void path_close_windows (void)
{
    gint i;
    for (i=0; i<PATH_NUM; ++i)
    {
	if (path_filechooser[i])
	{
	    gtk_widget_destroy (path_filechooser[i]);
	    path_filechooser[i] = NULL;
	}
    }
}


/* ok button on path fileselector window was pressed */
static void on_path_ok (PathType i)
{
    g_return_if_fail (i>=0 && i<PATH_NUM);

    if (path_filechooser[i])
    {
	const gchar *npath = gtk_file_chooser_get_filename (
	    GTK_FILE_CHOOSER (path_filechooser[i]));
	const gchar *opath = tmpcfg->path[i];
	/* find command line arguments */
	const gchar *opathp = strchr (opath, ' ');
	gchar *newpath;
	GtkWidget *w;

	/* attach command line arguments if present */
	if (opathp)
	    newpath = g_strdup_printf ("%s%s", npath, opathp);
	else
	    newpath = g_strdup (npath);

	if ((w = glade_xml_get_widget (prefs_window_xml, path_entry_names[i])))
	{
	    gchar *newpath_utf8 = g_filename_to_utf8 (newpath, -1, NULL, NULL, NULL);
	    gtk_entry_set_text(GTK_ENTRY(w), newpath_utf8);
	    g_free (newpath_utf8);
	}
	g_free (newpath);

	gtk_widget_destroy (path_filechooser[i]);
	path_filechooser[i] = NULL;
    }
}


static void on_fc_response (GtkFileChooser *fc,
			    gint response,
			    PathType i)
{
    g_return_if_fail (i>=0 && i<PATH_NUM && path_filechooser[i]);

    switch (response)
    {
    case GTK_RESPONSE_CANCEL:
	gtk_widget_destroy (path_filechooser[i]);
	path_filechooser[i] = NULL;
	break;
    case GTK_RESPONSE_ACCEPT:
	on_path_ok (i);
	break;
    case GTK_RESPONSE_NONE:
    case GTK_RESPONSE_DELETE_EVENT:
	path_filechooser[i] = NULL;
	break;
    default:
	printf ("programming error: received unknown response %d in on_fc_response\n", response);
	break;
    }
}


/* one of the "..." buttons has been pressed -> open a file chooser
   dialog and let the user select a file or directory */
static void on_path_button_pressed (GtkButton *button, gpointer user_data)
{
    PathType i = (PathType)user_data;
    GtkFileChooser *fc;
    const gchar *path, *pathp;
    gchar *buf, *fbuf;

    g_return_if_fail (tmpcfg);
    g_return_if_fail (i>=0 && i<PATH_NUM);

    if (path_filechooser[i])
    {  /* filechooser already open --> simply raise to the top */
	gdk_window_raise(path_filechooser[i]->window);
	return;
    }
    path_filechooser[i] = gtk_file_chooser_dialog_new (
	_(path_fileselector_titles[i]),
	NULL,
	path_type[i],
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	NULL);
    /* for convenience */
    fc = GTK_FILE_CHOOSER (path_filechooser[i]);
    g_signal_connect (fc, "response",
		      G_CALLBACK (on_fc_response),
		      (gpointer)i);
    /* find first whitespace separating path from command line
     * arguments */
    path = tmpcfg->path[i];
    pathp = strchr (path, ' ');
    if (pathp)
	buf = g_strndup (path, pathp-path);
    else
	buf = g_strdup (path);

    /* get full path */
    fbuf = g_find_program_in_path (buf);
    if (!fbuf) fbuf = g_strdup (buf);

    if (fbuf)
    {
	gchar *fbuf_utf8 = g_filename_from_utf8 (fbuf, -1, NULL, NULL, NULL);
	if (path_type[i] == GTK_FILE_CHOOSER_ACTION_OPEN)
	{
	    gtk_file_chooser_set_filename (fc, fbuf_utf8);
	}
	if (path_type[i] == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
	{
	    gtk_file_chooser_set_current_folder (fc, fbuf_utf8);
	}
	g_free (fbuf_utf8);
	g_free (fbuf);
    }
    g_free (buf);

    gtk_widget_show (path_filechooser[i]);
}


static void on_path_entry_changed (GtkEditable     *editable,
				   gpointer         user_data)
{
    PathType i = (PathType)user_data;
    gchar *buf = gtk_editable_get_chars(editable, 0, -1);
    prefs_window_set_path (i, buf);
    g_free (buf);
}


/* turn the prefs window insensitive (if it's open) */
void prefs_window_block (void)
{
    if (prefs_window)
	gtk_widget_set_sensitive (prefs_window, FALSE);
}

/* turn the prefs window sensitive (if it's open) */
void prefs_window_release (void)
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
	GtkTooltips *tt;
	GtkTooltipsData *tooltipsdata;
		
	tooltipsdata = gtk_tooltips_data_get (glade_xml_get_widget (prefs_window_xml, "cfg_mount_point"));
	if (tooltipsdata) {
		tt = tooltipsdata->tooltips;
		if (tt)
		{
	    	if (prefs_get_display_tooltips_prefs ()) gtk_tooltips_enable (tt);
	    	else                                     gtk_tooltips_disable (tt);
		}
    	} else {
		g_message("tooltipsdata is NULL");
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
    gint defx, defy;
    char *xml_file;

    if(!prefs_window)
    {
	GtkWidget *w = NULL;
	GtkTooltips *tt = NULL;

	if(!tmpcfg && !origcfg)
	{
	    tmpcfg = clone_prefs();
	    origcfg = clone_prefs();
	}
	else
	{
	    g_warning ("Programming error: tmpcfg is not NULL!!\n");
	    return;
	}

	xml_file = g_build_filename (PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S, PACKAGE, "gtkpod.glade", NULL);
	prefs_window_xml = glade_xml_new (xml_file, "prefs_window", NULL);
	glade_xml_signal_autoconnect (prefs_window_xml);

	prefs_window = glade_xml_get_widget(prefs_window_xml,"prefs_window");

	prefs_get_size_prefs (&defx, &defy);
	gtk_window_set_default_size (GTK_WINDOW (prefs_window), defx, defy);
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_mount_point")))
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
	if((w = glade_xml_get_widget (prefs_window_xml, "charset_combo")))
	{
	    charset_init_combo (GTK_COMBO (w));
	}

	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_md5tracks")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->md5tracks);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_update_existing")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->update_existing);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_duplicates")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_duplicates);
	    if (!tmpcfg->md5tracks) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_updated")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_updated);
	    if (!tmpcfg->update_existing) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_non_updated")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_non_updated);
	    if (!tmpcfg->update_existing) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_sync_dirs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->show_sync_dirs);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_sync_remove")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->sync_remove);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_display_toolbar")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->display_toolbar);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_toolbar_style_icons")))
	{
	    if (tmpcfg->toolbar_style == GTK_TOOLBAR_ICONS)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	    if (!tmpcfg->display_toolbar) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_toolbar_style_text")))
	{
	    if (tmpcfg->toolbar_style == GTK_TOOLBAR_TEXT)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	    if (!tmpcfg->display_toolbar) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_toolbar_style_both")))
	{
	    if (tmpcfg->toolbar_style == GTK_TOOLBAR_BOTH)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	    if (!tmpcfg->display_toolbar) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_display_tooltips_main")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->display_tooltips_main);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_display_tooltips_prefs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->display_tooltips_prefs);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_multi_edit")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->multi_edit);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_not_played_track")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->not_played_track);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_misc_track_nr")))
	{
	    gtk_spin_button_set_range (GTK_SPIN_BUTTON (w),
				       0, 0xffffffff);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				       prefs_get_misc_track_nr ());
	    prefs_window_set_misc_track_nr (tmpcfg->misc_track_nr);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_multi_edit_title")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->multi_edit_title);
	    gtk_widget_set_sensitive (w, tmpcfg->multi_edit);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_update_charset")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->update_charset);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_block_display")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->block_display);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_id3_write")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->id3_write);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_id3_write_id3v24")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->id3_write_id3v24);
	    if (!tmpcfg->id3_write) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_write_charset")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->write_charset);
	    if (!tmpcfg->id3_write) gtk_widget_set_sensitive (w, FALSE);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_add_recursively")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->add_recursively);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_delete_track_from_playlist")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.track);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_delete_track_from_ipod")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.ipod_file);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_sync_remove_confirm")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.syncing);
	    gtk_widget_set_sensitive (w, tmpcfg->sync_remove);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_sync_remove_confirm2")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->deletion.syncing);
	}
	if((w = glade_xml_get_widget (prefs_window_xml,  "cfg_autoimport")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->autoimport);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "autoselect_hbox")))
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
	/* connect signals for path entrys and selectors */
	for (i=0; i<PATH_NUM; ++i)
	{
	    if ((w = glade_xml_get_widget (prefs_window_xml, path_button_names[i])))
	    {
		g_signal_connect ((gpointer)w,
				  "clicked",
				  G_CALLBACK (on_path_button_pressed),
				  (gpointer)i);
	    }
	    if ((w = glade_xml_get_widget (prefs_window_xml, path_entry_names[i])))
	    {
		if (tmpcfg->path[i])
		{  /* we should copy the new path first because by setting
		      the text we might get a callback destroying the old
		      value... */
		    gchar *buf = g_strdup (tmpcfg->path[i]);
		    gtk_entry_set_text(GTK_ENTRY(w), buf);
		    g_free (buf);
		}
		g_signal_connect ((gpointer)w,
				  "changed",
				  G_CALLBACK (on_path_entry_changed),
				  (gpointer)i);
	    }
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_mpl_autoselect")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->mpl_autoselect);
	}
	for (i=0; i<TM_NUM_TAGS_PREFS; ++i)
	{
	    gchar *buf;
	    buf = g_strdup_printf ("tag_autoset%d", i);
	    if((w = glade_xml_get_widget (prefs_window_xml, buf)))
	    {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					     tmpcfg->autosettags[i]);
		/* glade makes a "GTK_OBJECT (i)" which segfaults
		   because "i" is not a GTK object. So we have to set up
		   the signal handlers ourselves */
		g_signal_connect ((gpointer)w,
				  "toggled",
				  G_CALLBACK (on_cfg_autosettags_toggled),
				  (gpointer)i);
	    }
	    g_free (buf);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "readtags")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->readtags);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "parsetags")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->parsetags);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "parsetags_overwrite")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->parsetags_overwrite);
	    gtk_widget_set_sensitive (w, tmpcfg->parsetags);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "parsetags_template")))
	{
	    if (tmpcfg->parsetags_template)
	    {  /* we should copy the new path first because by setting
		  the text we might get a callback destroying the old
		  value... */
		gchar *buf = g_strdup (tmpcfg->parsetags_template);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free (buf);
	    }
	    gtk_widget_set_sensitive (w, tmpcfg->parsetags);
	}
	/* get tooltips */
	tt = GTK_TOOLTIPS (glade_xml_get_widget (prefs_window_xml, "tooltips"));
	for (i=0; i<TM_NUM_COLUMNS; ++i)
	{
	    gchar *buf = g_strdup_printf ("col_visible%d", i);
	    if((w = glade_xml_get_widget (prefs_window_xml, buf)))
	    {
		/* set label */
		gtk_button_set_label (GTK_BUTTON (w),
				      gettext (tm_col_strings[i]));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					     tmpcfg->col_visible[i]);
		/* set tooltip if available */
		if (tt && tm_col_tooltips[i])
		{
		    gtk_tooltips_set_tip (tt, w, 
					  gettext (tm_col_tooltips[i]),
					  NULL);
		}
		/* glade makes a "GTK_OBJECT (i)" which segfaults
		   because "i" is not a GTK object. So we have to set
		   up the signal handlers ourselves */
		g_signal_connect ((gpointer)w,
				  "toggled",
				  G_CALLBACK (on_cfg_col_visible_toggled),
				  (gpointer)i);
	    }
	    g_free (buf);
	}

	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_write_extended")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    tmpcfg->write_extended_info);
	}

	if ((w = glade_xml_get_widget (prefs_window_xml, "notebook")))
	{
	    gtk_notebook_set_current_page (GTK_NOTEBOOK (w),
					   prefs_get_last_prefs_page ());
	}
	if ((w = glade_xml_get_widget (prefs_window_xml, "cfg_automount_ipod")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					    prefs_get_automount());
	}
	if ((w = glade_xml_get_widget (prefs_window_xml, "cfg_sort_tab_num_sb")))
	{
	    gtk_spin_button_set_range (GTK_SPIN_BUTTON (w),
				       0, SORT_TAB_MAX);
	    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				       prefs_get_sort_tab_num ());
	    prefs_window_set_sort_tab_num (tmpcfg->sort_tab_num);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "concal_autosync")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->concal_autosync);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_tmp_disable_sort")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->tmp_disable_sort);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_startup_messages")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->startup_messages);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_mserv_use")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->mserv_use);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "cfg_mserv_report_probs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpcfg->mserv_report_probs);
	}
	if((w = glade_xml_get_widget (prefs_window_xml, "mserv_username_entry")))
	{
	    if (tmpcfg->mserv_username)
	    {  /* we should copy the new path first because by setting
		  the text we might get a callback destroying the old
		  value... */
		gchar *buf = g_strdup (tmpcfg->mserv_username);
		gtk_entry_set_text(GTK_ENTRY(w), buf);
		g_free (buf);
	    }
	}
/* 	if((w = lookup_widget(prefs_window, "cfg_unused_gboolean3"))) */
/* 	{ */
/* 	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), */
/* 					 tmpcfg->unused_gboolean3); */
/* 	} */
	if((w = glade_xml_get_widget (prefs_window_xml, "concal_label")))
	{
	    gchar *str = g_strdup_printf (_("Have a look at the scripts provided in '%s'. If you write a new script, please send it to jcsjcs at users.sourceforge.net for inclusion into the next release."), PACKAGE_DATA_DIR G_DIR_SEPARATOR_S "scripts" G_DIR_SEPARATOR_S);
	    gtk_label_set_text (GTK_LABEL (w), str);
	    g_free (str);
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
    if (tmpcfg)
    {
	gint i;
	/* Need this in case user reordered column order (we don't
	 * catch the reorder signal) */
	tm_store_col_order ();
	prefs_set_id3_write(tmpcfg->id3_write);
	prefs_set_id3_write_id3v24(tmpcfg->id3_write_id3v24);
	prefs_set_ipod_mount(tmpcfg->ipod_mount);
	for (i=0; i<PATH_NUM; ++i)
	{
	    prefs_set_path (i, tmpcfg->path[i]);
	}
	prefs_set_charset(tmpcfg->charset);
	prefs_set_autoimport(tmpcfg->autoimport);
	for (i=0; i<SORT_TAB_MAX; ++i) {
	    prefs_set_st_autoselect (i, tmpcfg->st[i].autoselect);
	    prefs_set_st_category (i, tmpcfg->st[i].category);
	}
	for (i=0; i<TM_NUM_TAGS_PREFS; ++i) {
	    prefs_set_autosettags (i, tmpcfg->autosettags[i]);
	}
	prefs_set_readtags(tmpcfg->readtags);
	prefs_set_parsetags(tmpcfg->parsetags);
	prefs_set_parsetags_overwrite(tmpcfg->parsetags_overwrite);
	prefs_set_parsetags_template(tmpcfg->parsetags_template);
	for (i=0; i<TM_NUM_COLUMNS; ++i)
	{
	    prefs_set_col_visible (i, tmpcfg->col_visible[i]);
	}
	prefs_set_mpl_autoselect (tmpcfg->mpl_autoselect);
	prefs_set_track_playlist_deletion(tmpcfg->deletion.track);
	prefs_set_track_ipod_file_deletion(tmpcfg->deletion.ipod_file);
	prefs_set_sync_remove_confirm(tmpcfg->deletion.syncing);
	prefs_set_write_extended_info(tmpcfg->write_extended_info);
	prefs_set_show_duplicates(tmpcfg->show_duplicates);
	prefs_set_show_updated(tmpcfg->show_updated);
	prefs_set_show_non_updated(tmpcfg->show_non_updated);
	prefs_set_show_sync_dirs(tmpcfg->show_sync_dirs);
	/* this call well automatically destroy/setup the md5 hash table */
	prefs_set_md5tracks(tmpcfg->md5tracks);
	prefs_set_update_existing(tmpcfg->update_existing);
	prefs_set_block_display(tmpcfg->block_display);
	prefs_set_sort_tab_num(tmpcfg->sort_tab_num, TRUE);
	prefs_set_sync_remove(tmpcfg->sync_remove);
	prefs_set_toolbar_style(tmpcfg->toolbar_style);
	prefs_set_display_toolbar(tmpcfg->display_toolbar);
	prefs_set_display_tooltips_main (tmpcfg->display_tooltips_main);
	prefs_set_display_tooltips_prefs (tmpcfg->display_tooltips_prefs);
	prefs_set_multi_edit (tmpcfg->multi_edit);
	prefs_set_multi_edit_title (tmpcfg->multi_edit_title);
	prefs_set_misc_track_nr (tmpcfg->misc_track_nr);
	prefs_set_not_played_track (tmpcfg->not_played_track);
	prefs_set_update_charset(tmpcfg->update_charset);
	prefs_set_write_charset(tmpcfg->write_charset);
	prefs_set_add_recursively(tmpcfg->add_recursively);
	prefs_set_automount(tmpcfg->automount);
	prefs_set_concal_autosync(tmpcfg->concal_autosync);
	prefs_set_tmp_disable_sort(tmpcfg->tmp_disable_sort);
	prefs_set_startup_messages(tmpcfg->startup_messages);
	prefs_set_mserv_use(tmpcfg->mserv_use);
	prefs_set_mserv_report_probs(tmpcfg->mserv_report_probs);
	prefs_set_mserv_username(tmpcfg->mserv_username);
	prefs_set_unused_gboolean3(tmpcfg->unused_gboolean3);

	tm_show_preferred_columns();
    }
}


/* save current window size */
void prefs_window_update_default_sizes (void)
{
    if (prefs_window)
    {
	gint defx, defy;
	gtk_window_get_size (GTK_WINDOW (prefs_window), &defx, &defy);
	prefs_set_size_prefs (defx, defy);
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

    /* save current window size */
    prefs_window_update_default_sizes ();

    /* close the window */
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;

    /* close path_fileselector windows */
    path_close_windows ();
}

/* when window is deleted, we keep the currently applied prefs and
   save the notebook page */
void prefs_window_delete(void)
{
    gint defx, defy;
    GtkWidget *nb;

    /* delete cfg structs */
    cfg_free (tmpcfg);
    tmpcfg = NULL;
    cfg_free (origcfg);
    origcfg = NULL;

    /* save current notebook page */
    nb = glade_xml_get_widget (prefs_window_xml, "notebook");
    prefs_set_last_prefs_page (gtk_notebook_get_current_page (
				   GTK_NOTEBOOK (nb)));

    /* save current window size */
    gtk_window_get_size (GTK_WINDOW (prefs_window), &defx, &defy);
    prefs_set_size_prefs (defx, defy);

    /* close the window */
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;

    /* close path_fileselector windows */
    path_close_windows ();
}

/* apply the current settings and close the window */
/* Frees the tmpcfg and origcfg variable */
void
prefs_window_ok (void)
{
    gint defx, defy;
    GtkWidget *nb;

    /* save current settings */
    prefs_window_set ();

    /* delete cfg structs */
    cfg_free (tmpcfg);
    tmpcfg = NULL;
    cfg_free (origcfg);
    origcfg = NULL;

    /* save current notebook page */
    nb = glade_xml_get_widget (prefs_window_xml, "notebook");
    prefs_set_last_prefs_page (gtk_notebook_get_current_page (
				   GTK_NOTEBOOK (nb)));

    /* save current window size */
    gtk_window_get_size (GTK_WINDOW (prefs_window), &defx, &defy);
    prefs_set_size_prefs (defx, defy);

    /* close the window */
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;

    /* close path_fileselector windows */
    path_close_windows ();
}


/* apply the current settings, don't close the window */
void
prefs_window_apply (void)
{
    gint defx, defy, i;
    GtkWidget *nb, *w;

    /* save current settings */
    prefs_window_set ();

    /* reset the validated path entries */
    for (i=0; i<PATH_NUM; ++i)
    {
	if((w = glade_xml_get_widget (prefs_window_xml, path_entry_names[i])))
	{
	    gtk_entry_set_text(GTK_ENTRY(w), tmpcfg->path[i]);
	    /* tmpcfg gets set by the "changed" callback */
	}
    }

    if((w = glade_xml_get_widget (prefs_window_xml, "mserv_username_entry")))
    {
	gtk_entry_set_text(GTK_ENTRY(w), prefs_get_mserv_username ());
	/* tmpcfg gets set by the "changed" callback */
    }

    /* save current notebook page */
    nb = glade_xml_get_widget (prefs_window_xml, "notebook");
    prefs_set_last_prefs_page (gtk_notebook_get_current_page (
				   GTK_NOTEBOOK (nb)));

    /* save current window size */
    gtk_window_get_size (GTK_WINDOW (prefs_window), &defx, &defy);
    prefs_set_size_prefs (defx, defy);
}


/**
 * prefs_window_set_md5tracks
 * @val - truth value of whether or not we should use the md5 hash to
 * prevent file duplication. changes temp variable
 */
void
prefs_window_set_md5tracks(gboolean val)
{
    GtkWidget *w;

    tmpcfg->md5tracks = val;
    if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_duplicates")))
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
    if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_updated")))
	gtk_widget_set_sensitive (w, val);
    if((w = glade_xml_get_widget (prefs_window_xml, "cfg_show_non_updated")))
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
    if((w = glade_xml_get_widget (prefs_window_xml, "cfg_id3_write_id3v24")))
	gtk_widget_set_sensitive (w, val);
    if((w = glade_xml_get_widget (prefs_window_xml, "cfg_write_charset")))
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

static void prefs_window_set_path (PathType i, const gchar *path)
{
    if (!path) return;
    g_return_if_fail (i>=0 && i<PATH_NUM);
    g_free (tmpcfg->path[i]);
    tmpcfg->path[i] = g_strdup (path);
}

void prefs_window_set_write_extended_info(gboolean active)
{
  tmpcfg->write_extended_info = active;
}

void
prefs_window_set_delete_track_ipod(gboolean val)
{
    tmpcfg->deletion.ipod_file = val;
}

void
prefs_window_set_delete_track_playlist(gboolean val)
{
    tmpcfg->deletion.track = val;
}

void
prefs_window_set_sync_remove_confirm(gboolean val)
{
    tmpcfg->deletion.syncing = val;
}

void
prefs_window_set_autoimport(gboolean val)
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

void prefs_window_set_autosettags (gint category, gboolean autoset)
{
    if (category < TM_NUM_TAGS_PREFS)
	tmpcfg->autosettags[category] = autoset;
}

void
prefs_window_set_readtags(gboolean val)
{
    tmpcfg->readtags = val;
}

void
prefs_window_set_parsetags(gboolean val)
{
    GtkWidget *w;

    tmpcfg->parsetags = val;
    if((w = glade_xml_get_widget (prefs_window_xml, "parsetags_overwrite")))
	gtk_widget_set_sensitive (w, val);
    if((w = glade_xml_get_widget (prefs_window_xml, "parsetags_template")))
	gtk_widget_set_sensitive (w, val);
}

void
prefs_window_set_parsetags_overwrite(gboolean val)
{
    tmpcfg->parsetags_overwrite = val;
}

void
prefs_window_set_parsetags_template(const gchar *val)
{
    g_free (tmpcfg->parsetags_template);
    tmpcfg->parsetags_template = g_strdup(val);
}

void prefs_window_set_col_visible (gint column, gboolean visible)
{
    if (column < TM_NUM_COLUMNS)
	tmpcfg->col_visible[column] = visible;
}

void prefs_window_set_id3_write_id3v24 (gboolean val)
{
    tmpcfg->id3_write_id3v24 = val;
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
    if((w = glade_xml_get_widget (prefs_window_xml, "cfg_sync_remove_confirm")))
	gtk_widget_set_sensitive (w, val);
}

void prefs_window_set_display_toolbar (gboolean val)
{
    GtkWidget *w1 = glade_xml_get_widget (prefs_window_xml, "cfg_toolbar_style_icons");
    GtkWidget *w2 = glade_xml_get_widget (prefs_window_xml, "cfg_toolbar_style_text");
    GtkWidget *w3 = glade_xml_get_widget (prefs_window_xml, "cfg_toolbar_style_both");

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
    GtkWidget *w = glade_xml_get_widget  (prefs_window_xml, "cfg_multi_edit_title");

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

void prefs_window_set_not_played_track (gboolean val)
{
    tmpcfg->not_played_track = val;
}

void prefs_window_set_misc_track_nr (gint val)
{
    tmpcfg->misc_track_nr = val;
}

void prefs_window_set_write_charset (gboolean val)
{
    tmpcfg->write_charset = val;
}

void prefs_window_set_add_recursively (gboolean val)
{
    tmpcfg->add_recursively = val;
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

void
prefs_window_set_concal_autosync(gboolean val)
{
    tmpcfg->concal_autosync = val;
}

void
prefs_window_set_tmp_disable_sort(gboolean val)
{
    tmpcfg->tmp_disable_sort = val;
}

void
prefs_window_set_startup_messages(gboolean val)
{
    tmpcfg->startup_messages = val;
}

void
prefs_window_set_mserv_use(gboolean val)
{
    tmpcfg->mserv_use = val;
}

void
prefs_window_set_mserv_report_probs(gboolean val)
{
    tmpcfg->mserv_report_probs = val;
}

void prefs_window_set_mserv_username(const gchar *val)
{
    if (!val) return;
    g_free (tmpcfg->mserv_username);
    tmpcfg->mserv_username = g_strdup (val);
}

void
prefs_window_set_unused_gboolean3(gboolean val)
{
    tmpcfg->unused_gboolean3 = val;
}


/* ------------------------------------------------------------ *\
 *                                                              *
 * Sort-Prefs Window                                            *
 *                                                              *
\* ------------------------------------------------------------ */

/**
 * sort_window_create
 * Create, Initialize, and Show the sorting preferences window
 * allocate a static sort struct for temporary variables
 */
void sort_window_create (void)
{
    gchar *xml_file; 
   
    if (sort_window)
    {  /* sort options already open --> simply raise to the top */
	gdk_window_raise(sort_window->window);
    }
    else
    {
	GList *collist = NULL;
	GtkWidget *w;
	gint i;

	if(!tmpsortcfg && !origsortcfg)
	{
	    tmpsortcfg = clone_sortprefs();
	    origsortcfg = clone_sortprefs();
	}
	else
	{
	    g_warning ("Programming error: tmpsortcfg is not NULL!!\n");
	    return;
	}

	xml_file = g_build_filename (PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S, PACKAGE, "gtkpod.glade", NULL);
	sort_window_xml = glade_xml_new (xml_file, "sort_window", NULL);
	glade_xml_signal_autoconnect (sort_window_xml);

	sort_window = glade_xml_get_widget (sort_window_xml, "sort_window");

	/* Set Sort-Column-Combo */
	/* create the list in the order of the columns displayed */
	tm_store_col_order ();
	for (i=0; i<TM_NUM_COLUMNS; ++i)
	{   /* first the visible columns */
	    TM_item col = prefs_get_col_order (i);
	    if (col != -1)
	    {
		if (prefs_get_col_visible (col))
		    collist = g_list_append (collist,
					     gettext (tm_col_strings[col]));
	    }
	}
	for (i=0; i<TM_NUM_COLUMNS; ++i)
	{   /* first the visible columns */
	    TM_item col = prefs_get_col_order (i);
	    if (col != -1)
	    {
		if (!prefs_get_col_visible (col))
		    collist = g_list_append (collist,
					     gettext (tm_col_strings[col]));
	    }
	}
	w = glade_xml_get_widget (sort_window_xml, "sort_combo");
	gtk_combo_set_popdown_strings (GTK_COMBO (w), collist);
	g_list_free (collist);
	collist = NULL;

	sort_window_update ();

	sort_window_show_hide_tooltips ();
	gtk_widget_show (sort_window);
    }
}



/* Update sort_window's settings */
void sort_window_update (void)
{
    if (sort_window && tmpsortcfg)
    {
	gchar *str;
	GtkWidget *w = NULL;
	switch (tmpsortcfg->pm_sort)
	{
	case SORT_ASCENDING:
	    w = glade_xml_get_widget (sort_window_xml, "pm_ascend");
	    break;
	case SORT_DESCENDING:
	    w = glade_xml_get_widget (sort_window_xml, "pm_descend");
	    break;
	case SORT_NONE:
	    w = glade_xml_get_widget (sort_window_xml, "pm_none");
	    break;
	}
	if (w)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

	w = NULL;
	switch (tmpsortcfg->st_sort)
	{
	case SORT_ASCENDING:
	    w = glade_xml_get_widget (sort_window_xml, "st_ascend");
	    break;
	case SORT_DESCENDING:
	    w = glade_xml_get_widget (sort_window_xml, "st_descend");
	    break;
	case SORT_NONE:
	    w = glade_xml_get_widget (sort_window_xml, "st_none");
	    break;
	}
	if (w)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

	switch (tmpsortcfg->tm_sort)
	{
	case SORT_ASCENDING:
	    w = glade_xml_get_widget (sort_window_xml, "tm_ascend");
	    break;
	case SORT_DESCENDING:
	    w = glade_xml_get_widget (sort_window_xml, "tm_descend");
	    break;
	case SORT_NONE:
	    w = glade_xml_get_widget (sort_window_xml, "tm_none");
	    break;
	}
	if (w)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

	w = glade_xml_get_widget (sort_window_xml, "pm_autostore");
	if (w)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpsortcfg->pm_autostore);

	w = glade_xml_get_widget (sort_window_xml, "tm_autostore");
	if (w)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpsortcfg->tm_autostore);

	if((w = glade_xml_get_widget (sort_window_xml, "cfg_case_sensitive")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
					 tmpsortcfg->case_sensitive);
	}
	/* set standard entry in combo */
	str = gettext (tm_col_strings[prefs_get_tm_sortcol ()]);
	w = glade_xml_get_widget (sort_window_xml, "sort_combo");
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w)->entry), str);
    }
}


/* turn the sort window insensitive (if it's open) */
void sort_window_block (void)
{
    if (sort_window)
	gtk_widget_set_sensitive (sort_window, FALSE);
}

/* turn the sort window sensitive (if it's open) */
void sort_window_release (void)
{
    if (sort_window)
	gtk_widget_set_sensitive (sort_window, TRUE);
}


/* make the tooltips visible or hide it depending on the value set in
 * the prefs (tooltips_prefs) */
void sort_window_show_hide_tooltips (void)
{
    if (sort_window)
    {
	GtkTooltips *tt;
	GtkTooltipsData *tooltips_data;
	tooltips_data = gtk_tooltips_data_get (glade_xml_get_widget (sort_window_xml, "sort_combo-entry"));
	tt = tooltips_data->tooltips;
	if (tt)
	{
	    if (prefs_get_display_tooltips_prefs ()) gtk_tooltips_enable (tt);
	    else                                     gtk_tooltips_disable (tt);
	}
    }
}


/* get the sort_column selected in the combo */
static TM_item sort_window_get_sort_col (void)
{
    const gchar *str;
    GtkWidget *w;
    gint i = -1;

    w = glade_xml_get_widget (sort_window_xml, "sort_combo");
    str = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (w)->entry));
    /* Check which string is selected in the combo */
    if (str)
	for (i=0; tm_col_strings[i]; ++i)
	    if (strcmp (gettext (tm_col_strings[i]), str) == 0)  break;
    if ((i<0) || (i>= TM_NUM_COLUMNS))
    {
	fprintf (stderr,
		 "Programming error: cal_get_category () -- item not found.\n");
	/* set to something reasonable at least */
	i = TM_COLUMN_TITLE;
    }

    return i;
}


/* copy newly set values from tmpsortcfg to the original cfg struct */
void sort_window_set (void)
{
    if (tmpsortcfg && origsortcfg)
    {
	struct sortcfg *tsc = clone_sortprefs ();
	prefs_set_pm_sort (tmpsortcfg->pm_sort);
	prefs_set_pm_autostore (tmpsortcfg->pm_autostore);
	prefs_set_st_sort (tmpsortcfg->st_sort);
	prefs_set_tm_sort (tmpsortcfg->tm_sort);
	prefs_set_tm_autostore (tmpsortcfg->tm_autostore);
	prefs_set_case_sensitive(tmpsortcfg->case_sensitive);
	tmpsortcfg->tm_sortcol = sort_window_get_sort_col ();
	prefs_set_tm_sortcol (tmpsortcfg->tm_sortcol);
	/* if sort type has changed, initialize display */
	if (tsc->pm_sort != tmpsortcfg->pm_sort)
	    pm_sort (tmpsortcfg->pm_sort);
	if (tsc->st_sort != tmpsortcfg->st_sort)
	    st_sort (tmpsortcfg->st_sort);
	if ((tsc->tm_sort != tmpsortcfg->tm_sort) ||
	    (tsc->tm_sortcol != tmpsortcfg->tm_sortcol))
	{
	    tm_sort (prefs_get_tm_sortcol (), tmpsortcfg->tm_sort);
	}
	/* if auto sort was changed to TRUE, store order */
	if (!tsc->pm_autostore && tmpsortcfg->pm_autostore)
	    pm_rows_reordered ();
	if (!tsc->tm_autostore && tmpsortcfg->tm_autostore)
	    tm_rows_reordered ();
	sortcfg_free (tsc);
    }
}


/**
 * sort_window_cancel
 * UI has requested sort prefs changes be ignored -- write back the
 * original values
 * Frees the tmpsortcfg and origsortcfg variable
 */
void sort_window_cancel(void)
{
    struct sortcfg *tt;

    /* exchange tmpsortcfg with origsortcfg */
    tt = tmpsortcfg;
    tmpsortcfg = origsortcfg;
    origsortcfg = tt;
    tt = NULL;

    /* "save" (i.e. reset) original configs */
    sort_window_set ();

    /* delete cfg struct */
    sortcfg_free (tmpsortcfg);
    sortcfg_free (origsortcfg);
    tmpsortcfg = NULL;
    origsortcfg = NULL;

    /* close the window */
    if(sort_window)
	gtk_widget_destroy(sort_window);
    sort_window = NULL;
}

/* when window is deleted, we keep the currently applied prefs */
void sort_window_delete(void)
{
    /* delete sortcfg structs */
    sortcfg_free (tmpsortcfg);
    tmpsortcfg = NULL;
    sortcfg_free (origsortcfg);
    origsortcfg = NULL;

    /* close the window */
    if(sort_window)
	gtk_widget_destroy(sort_window);
    sort_window = NULL;
}

/* apply the current settings and close the window */
/* Frees the tmpsortcfg and origsortcfg variable */
void sort_window_ok (void)
{
    /* save current settings */
    sort_window_set ();

    /* delete sortcfg structs */
    sortcfg_free (tmpsortcfg);
    tmpsortcfg = NULL;
    sortcfg_free (origsortcfg);
    origsortcfg = NULL;

    /* close the window */
    if(sort_window)
	gtk_widget_destroy(sort_window);
    sort_window = NULL;
}


/* apply the current settings, don't close the window */
void sort_window_apply (void)
{
    /* save current settings */
    sort_window_set ();
}

void sort_window_set_pm_autostore (gboolean val)
{
    tmpsortcfg->pm_autostore = val;
}

void sort_window_set_tm_autostore (gboolean val)
{
    tmpsortcfg->tm_autostore = val;
}

void sort_window_set_pm_sort (gint val)
{
    tmpsortcfg->pm_sort = val;
}

void sort_window_set_st_sort (gint val)
{
    tmpsortcfg->st_sort = val;
}

void sort_window_set_tm_sort (gint val)
{
    tmpsortcfg->tm_sort = val;
}

void sort_window_set_case_sensitive (gboolean val)
{
    tmpsortcfg->case_sensitive = val;
}
