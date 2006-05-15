/* Time-stamp: <2006-05-16 00:20:45 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
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

/* This file provides functions for the edit repositories/playlist
 * option window */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "display_itdb.h"
#include "fileselection.h"
#include "info.h"
#include "misc.h"
#include "prefs.h"
#include "repository.h"
#include "syncdir.h"

/* List with all repository edit windows (we only allow one, however) */
static GList *repwins = NULL;


struct _RepWin
{
    GladeXML *xml;           /* XML info                             */
    GtkWidget *window;       /* pointer to repository window         */
    iTunesDB *itdb;          /* currently displayed repository       */
    gint itdb_index;         /* index number of itdb                 */
    Playlist *playlist;      /* currently displayed playlist         */
    Playlist *next_playlist; /* playlist to display next (or NULL)   */
    TempPrefs *temp_prefs;   /* changes made so far                  */
    TempPrefs *extra_prefs;  /* changes to non-prefs items (e.g.
				live-update                          */
};

typedef struct _RepWin RepWin;

/* where to find the scripts */
static const gchar *scriptdir = PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "scripts" G_DIR_SEPARATOR_S;

typedef enum
{
    IPOD_SYNC_CONTACTS,
    IPOD_SYNC_CALENDAR,
    IPOD_SYNC_NOTES
} iPodSyncType;


/* string constants for window widgets used more than once */
static const gchar *PLAYLIST_COMBO="playlist_combo";
static const gchar *REPOSITORY_COMBO="repository_combo";
static const gchar *MOUNTPOINT_LABEL="mountpoint_label";
static const gchar *MOUNTPOINT_ENTRY="mountpoint_entry";
static const gchar *MOUNTPOINT_BUTTON="mountpoint_button";
static const gchar *BACKUP_LABEL="backup_label";
static const gchar *BACKUP_ENTRY="backup_entry";
static const gchar *IPOD_MODEL_LABEL="ipod_model_label";
static const gchar *IPOD_MODEL_ENTRY="ipod_model_entry";
static const gchar *LOCAL_PATH_LABEL="local_path_label";
static const gchar *LOCAL_PATH_ENTRY="local_path_entry";
static const gchar *STANDARD_PLAYLIST_VBOX="standard_playlist_vbox";
static const gchar *SPL_VBOX="spl_vbox";
static const gchar *SPL_LIVE_UPDATE_TOGGLE="spl_live_update_toggle";
static const gchar *PLAYLIST_AUTOSYNC_MODE_NONE_RADIO="playlist_autosync_mode_none_radio";
static const gchar *PLAYLIST_AUTOSYNC_MODE_AUTOMATIC_RADIO="playlist_autosync_mode_automatic_radio";
static const gchar *PLAYLIST_AUTOSYNC_MODE_MANUAL_RADIO="playlist_autosync_mode_manual_radio";
static const gchar *MANUAL_SYNCDIR_ENTRY="manual_syncdir_entry";
static const gchar *MANUAL_SYNCDIR_BUTTON="manual_syncdir_button";
static const gchar *DELETE_REPOSITORY_CHECKBUTTON="delete_repository_checkbutton";
static const gchar *DELETE_REPOSITORY_BUTTON="delete_repository_button";
static const gchar *REPOSITORY_VBOX="repository_vbox";
static const gchar *IPOD_SYNC_LABEL="ipod_sync_label";
static const gchar *IPOD_SYNC_CONTACTS_LABEL="ipod_sync_contacts_label";
static const gchar *IPOD_SYNC_CONTACTS_ENTRY="ipod_sync_contacts_entry";
static const gchar *IPOD_SYNC_CONTACTS_BUTTON="ipod_sync_contacts_button";
static const gchar *IPOD_SYNC_CALENDAR_LABEL="ipod_sync_calendar_label";
static const gchar *IPOD_SYNC_CALENDAR_ENTRY="ipod_sync_calendar_entry";
static const gchar *IPOD_SYNC_CALENDAR_BUTTON="ipod_sync_calendar_button";
static const gchar *IPOD_SYNC_NOTES_LABEL="ipod_sync_notes_label";
static const gchar *IPOD_SYNC_NOTES_ENTRY="ipod_sync_notes_entry";
static const gchar *IPOD_SYNC_NOTES_BUTTON="ipod_sync_notes_button";
static const gchar *IPOD_CONCAL_AUTOSYNC_TOGGLE="ipod_concal_autosync_toggle";
static const gchar *SYNC_OPTIONS_HBOX="sync_options_hbox";
static const gchar *PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE="playlist_sync_delete_tracks_toggle";
static const gchar *PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE="playlist_sync_confirm_delete_toggle";
static const gchar *PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE="playlist_sync_show_summary_toggle";
static const gchar *UPDATE_PLAYLIST_BUTTON="update_playlist_button";
static const gchar *UPDATE_ALL_PLAYLISTS_BUTTON="update_all_playlists_button";

/* string constants for preferences */
static const gchar *KEY_REPOSITORY_WINDOW_DEFX="repository_window_defx";
static const gchar *KEY_REPOSITORY_WINDOW_DEFY="repository_window_defy";

/* some key names used several times */
const gchar *KEY_CONCAL_AUTOSYNC="concal_autosync";
const gchar *KEY_SYNC_DELETE_TRACKS="sync_delete_tracks";
const gchar *KEY_SYNC_CONFIRM_DELETE="sync_confirm_delete";
const gchar *KEY_SYNC_CONFIRM_DIRS="sync_confirm_dirs";
const gchar *KEY_SYNC_SHOW_SUMMARY="sync_show_summary";
const gchar *KEY_MOUNTPOINT="mountpoint";
const gchar *KEY_IPOD_MODEL="ipod_model";
const gchar *KEY_FILENAME="filename";
const gchar *KEY_PATH_SYNC_CONTACTS="path_sync_contacts";
const gchar *KEY_PATH_SYNC_CALENDAR="path_sync_calendar";
const gchar *KEY_PATH_SYNC_NOTES="path_sync_notes";
const gchar *KEY_SYNCMODE="syncmode";
const gchar *KEY_MANUAL_SYNCDIR="manual_syncdir";



/* shortcut to reference widgets when repwin->xml is already set */
#define GET_WIDGET(a) gtkpod_xml_get_widget (repwin->xml,a)


/* Declarations */
static void update_buttons (RepWin *repwin);
static void repwin_free (RepWin *repwin);
static void init_repository_combo (RepWin *repwin);
static void init_playlist_combo (RepWin *repwin);
static void select_repository (RepWin *repwin,
			       iTunesDB *itdb, Playlist *playlist);



/* Store the window size */
static void store_window_state (RepWin *repwin)
{
    gint defx, defy;

    g_return_if_fail (repwin);

    gtk_window_get_size (GTK_WINDOW (repwin->window), &defx, &defy);
    prefs_set_int (KEY_REPOSITORY_WINDOW_DEFX, defx);
    prefs_set_int (KEY_REPOSITORY_WINDOW_DEFY, defy);
}



/* ------------------------------------------------------------
 *
 *        Helper functions for prefs interfaceing.
 *
 * ------------------------------------------------------------ */


/* Get prefs string -- either from repwin->temp_prefs or from the main
   prefs system. Return an empty string if no value was set. */
/* Free string after use */
static gchar *get_current_prefs_string (RepWin *repwin, const gchar *key)
{
    gchar *value;

    g_return_val_if_fail (repwin && key, NULL);

    value = temp_prefs_get_string (repwin->temp_prefs, key);
    if (value == NULL)
    {
	value = prefs_get_string (key);
    }
    if (value == NULL)
    {
	value = g_strdup ("");
    }
    return value;
}


/* Get integer prefs value -- either from repwin->temp_prefs or from
   the main prefs system. Return 0 if no value was set. */
/* Free string after use */
static gint get_current_prefs_int (RepWin *repwin, const gchar *key)
{
    gint value;

    g_return_val_if_fail (repwin && key, 0);

    if (!temp_prefs_get_int_value (repwin->temp_prefs, key, &value))
    {
	value = prefs_get_int (key);
    }
    return value;
}




/* ------------------------------------------------------------
 *
 *        Callback functions (buttons, entries...)
 *
 * ------------------------------------------------------------ */


/* Compare the value of @str with the value stored for @key in the
   prefs system. If values differ, store @str for @key in

   @repwin->temp_prefs, otherwise remove a possibly existing entry
   @key in @repwin->temp_prefs.

   Return value: TRUE if a new string was set, FALSE if no new string
   was set, or the new string was identical to the one stored in the
   prefs system. */

/* Attention: g_frees() @key and @str for you */
static gboolean finish_string_storage (RepWin *repwin,
				       gchar *key, gchar *str)
{
    gchar *prefs_str;
    gboolean result;

    g_return_val_if_fail (repwin && key && str, FALSE);

    prefs_str = prefs_get_string (key);

    if ((!prefs_str && (strlen (str) > 0)) ||
	(prefs_str && (strcmp (str, prefs_str) != 0)))
    {   /* value has changed with respect to the value stored in the
	   prefs */
printf ("setting '%s' to '%s'\n", key, str);
	temp_prefs_set_string (repwin->temp_prefs, key, str);
	result = TRUE;
    }
    else
    {   /* value has not changed -- remove key from temp prefs (in
	   case it exists */
printf ("removing '%s'.\n", key);
	temp_prefs_set_string (repwin->temp_prefs, key, NULL);
	result = FALSE;
    }
    update_buttons (repwin);
    g_free (key);
    g_free (str);
    g_free (prefs_str);
    return result;
}


/* Retrieve the current text in @editable and call
   finish_string_storage()

   Return value: see finish_string_storage() */
static gboolean finish_editable_storage (RepWin *repwin,
					 gchar *key,
					 GtkEditable *editable)
{
    gchar *str;

    g_return_val_if_fail (repwin && key && editable, FALSE);

    str = gtk_editable_get_chars (editable, 0, -1);
    return finish_string_storage (repwin, key, str);
}

/* Compare the value of @val with the value stored for @key in the
   prefs system. If values differ, store @val for @key in
   @repwin->temp_prefs, otherwise remove a possibly existing entry
   @key in @repwin->temp_prefs. */
/* Attention: g_frees() @key for you */
static void finish_int_storage (RepWin *repwin,
				gchar *key, gint val)
{
    gint prefs_val;

    g_return_if_fail (repwin && key);

    /* defaults to '0' if not set */
    prefs_val = prefs_get_int (key);
    if (prefs_val != val)
    {   /* value has changed with respect to the value stored in the
	   prefs */
printf ("setting '%s' to '%d'\n", key, val);
	temp_prefs_set_int (repwin->temp_prefs, key, val);
    }
    else
    {   /* value has not changed -- remove key from temp prefs (in
	   case it exists */
printf ("removing '%s'.\n", key);
	temp_prefs_set_string (repwin->temp_prefs, key, NULL);
    }
    update_buttons (repwin);
    g_free (key);
}


/* text in standard text entry has changed */
static void standard_itdb_entry_changed (GtkEditable *editable,
					 RepWin *repwin)
{
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repwin);

    keybase = g_object_get_data (G_OBJECT (editable), "key");
    g_return_if_fail (keybase);

    key = get_itdb_prefs_key (repwin->itdb_index, keybase);

    finish_editable_storage (repwin, key, editable);
}


/* text for manual_syncdir has changed */
static void manual_syncdir_changed (GtkEditable *editable,
				    RepWin *repwin)
{
    gchar *key;
    gchar changed;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
				  KEY_MANUAL_SYNCDIR);

    changed = finish_editable_storage (repwin, key, editable);

    if (changed)
    {
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_MANUAL_RADIO)),
	    TRUE);
    }
}


/* playlist_autosync_mode_none was toggled */
static void playlist_autosync_mode_none_toggled (GtkToggleButton *togglebutton,
						 RepWin *repwin)
{
    gchar *key;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
				  KEY_SYNCMODE);

    if (gtk_toggle_button_get_active (togglebutton))
    {
	finish_int_storage (repwin, key,
			    PLAYLIST_AUTOSYNC_MODE_NONE);
	update_buttons (repwin);
    }
}


/* playlist_autosync_mode_none was toggled */
static void playlist_autosync_mode_manual_toggled (GtkToggleButton *togglebutton,
						   RepWin *repwin)
{
    gchar *key;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
				  KEY_SYNCMODE);

    if (gtk_toggle_button_get_active (togglebutton))
    {
	finish_int_storage (repwin, key,
			    PLAYLIST_AUTOSYNC_MODE_MANUAL);
	update_buttons (repwin);
    }
}


/* playlist_autosync_mode_none was toggled */
static void playlist_autosync_mode_automatic_toggled (GtkToggleButton *togglebutton,
						      RepWin *repwin)
{
    gchar *key;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
				  KEY_SYNCMODE);

    if (gtk_toggle_button_get_active (togglebutton))
    {
	finish_int_storage (repwin, key,
			    PLAYLIST_AUTOSYNC_MODE_AUTOMATIC);
	update_buttons (repwin);
    }
}


static void standard_itdb_checkbutton_toggled (GtkToggleButton *togglebutton,
					       RepWin *repwin)
{
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repwin);

    keybase = g_object_get_data (G_OBJECT (togglebutton), "key");
    g_return_if_fail (keybase);
    key = get_itdb_prefs_key (repwin->itdb_index, keybase);
    finish_int_storage (repwin, key,
			gtk_toggle_button_get_active (togglebutton));
}


static void standard_playlist_checkbutton_toggled (GtkToggleButton *togglebutton,
						   RepWin *repwin)
{
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repwin);

    keybase = g_object_get_data (G_OBJECT (togglebutton), "key");
    g_return_if_fail (keybase);
    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
				  keybase);
    finish_int_storage (repwin, key,
			gtk_toggle_button_get_active (togglebutton));
}


/* spl_live_update was toggled */
static void spl_live_update_toggled (GtkToggleButton *togglebutton,
				     RepWin *repwin)
{
    gchar *key;
    gboolean val;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->playlist);

    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
			    "liveupdate");

    val = gtk_toggle_button_get_active (togglebutton);

    if (val == repwin->playlist->splpref.liveupdate)
	temp_prefs_set_string (repwin->extra_prefs, key, NULL);
    else
	temp_prefs_set_int (repwin->extra_prefs, key, val);

    update_buttons (repwin);
    g_free (key);
}


/* delete_repository_checkbutton was toggled */
static void delete_repository_checkbutton_toggled (GtkToggleButton *togglebutton,
						   RepWin *repwin)
{
    g_return_if_fail (repwin);
    g_return_if_fail (repwin->temp_prefs);

    if (!gtk_toggle_button_get_active (togglebutton))
    {   /* Un-delete if necessary */
	gchar *key = get_itdb_prefs_key (repwin->itdb_index, "deleted");

	temp_prefs_set_string (repwin->extra_prefs, key, NULL);
	g_free (key);
    }
    update_buttons (repwin);
}



/* delete_repository_button was clicked */
static void delete_repository_button_clicked (GtkButton *button,
					      RepWin *repwin)
{
    gchar *key;

    g_return_if_fail (repwin);

    key = get_itdb_prefs_key (repwin->itdb_index, "deleted");

    temp_prefs_set_int (repwin->extra_prefs, key, TRUE);
    g_free (key);
    update_buttons (repwin);
}


/* mountpoint browse button was clicked */
static void mountpoint_button_clicked (GtkButton *button, RepWin *repwin)
{
    gchar *key, *old_dir, *new_dir;

    g_return_if_fail (repwin);

    key = get_itdb_prefs_key (repwin->itdb_index, "mountpoint");
    old_dir = get_current_prefs_string (repwin, key);
    g_free (key);

    new_dir = fileselection_get_dir (_("Select mountpoint"), old_dir);

    if (new_dir)
    {
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (MOUNTPOINT_ENTRY)),
			    new_dir);
	g_free (new_dir);
    }
}


/* mountpoint browse button was clicked */
static void manual_syncdir_button_clicked (GtkButton *button,
					   RepWin *repwin)
{
    gchar *key, *old_dir, *new_dir;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key (repwin->itdb_index, repwin->playlist,
				  KEY_MANUAL_SYNCDIR);

    old_dir = get_current_prefs_string (repwin, key);

    new_dir = fileselection_get_dir (_("Select directory for synchronization"), old_dir);

    if (new_dir)
    {
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (MANUAL_SYNCDIR_ENTRY)),
			    new_dir);
	g_free (new_dir);
    }
    g_free (key);
}



static void ipod_sync_button_clicked (RepWin *repwin, iPodSyncType type)
{
    const gchar *title;
    const gchar *entry;
    gchar *text, *key, *oldpath, *newpath;

    g_return_if_fail (repwin);

    switch (type)
    {
    case IPOD_SYNC_CONTACTS:
	title = _("Please select command to sync contacts");
	entry = IPOD_SYNC_CONTACTS_ENTRY;
	key = get_itdb_prefs_key (repwin->itdb_index,
				  "path_sync_contacts");
	break;
    case IPOD_SYNC_CALENDAR:
	title = _("Please select command to sync calendar");
	entry = IPOD_SYNC_CALENDAR_ENTRY;
	key = get_itdb_prefs_key (repwin->itdb_index,
				  "path_sync_calendar");
	break;
    case IPOD_SYNC_NOTES:
	title = _("Please select command to sync notes");
	entry = IPOD_SYNC_NOTES_ENTRY;
	key = get_itdb_prefs_key (repwin->itdb_index,
				  "path_sync_notes");
	break;
    default:
	g_return_if_reached ();
    }

    oldpath = prefs_get_string (key);
    g_free (key);

    text = g_markup_printf_escaped (_("<i>Have a look at the scripts provided in '%s'. If you write a new script, please send it to jcsjcs at users.sourceforge.net for inclusion into the next release.</i>"), scriptdir);

    newpath = fileselection_select_script (oldpath,
					   scriptdir,
					   title,
					   text);
    g_free (oldpath);
    g_free (text);

    if (newpath)
    {
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (entry)), newpath);
	g_free (newpath);
    }
}



/* Callback */
static void ipod_sync_contacts_button_clicked (GtkButton *button,
					       RepWin *repwin)
{
    ipod_sync_button_clicked (repwin, IPOD_SYNC_CONTACTS);
}


/* Callback */
static void ipod_sync_calendar_button_clicked (GtkButton *button,
					       RepWin *repwin)
{
    ipod_sync_button_clicked (repwin, IPOD_SYNC_CALENDAR);

}


/* Callback */
static void ipod_sync_notes_button_clicked (GtkButton *button,
					    RepWin *repwin)
{
    ipod_sync_button_clicked (repwin, IPOD_SYNC_NOTES);

}


/**
 * sync_or_update_playlist:
 *
 * Sync (normal playlist) or update (spl) @playlist (in repository
 * @itdb_index) using the currently displayed options.
 */
static void sync_or_update_playlist (RepWin *repwin,
				     gint itdb_index,
				     Playlist *playlist)
{
    g_return_if_fail (repwin);
    g_return_if_fail (playlist);

    if (playlist->is_spl)
    {
	/* live update */
    }
    else
    {
	gchar *key_sync_delete_tracks, *key_sync_confirm_delete;
	gchar *key_sync_show_summary, *key_manual_sync_dir, *key_syncmode;
	gchar *sync_delete_tracks_orig, *sync_confirm_delete_orig;
	gchar *sync_show_summary_orig;
	gint sync_delete_tracks_current, sync_confirm_delete_current;
	gint sync_show_summary_current;
	gchar *manual_sync_dir = NULL;
	gint value_new;

	/* create needed prefs keys */
	key_sync_delete_tracks = get_playlist_prefs_key (
	    itdb_index, playlist, KEY_SYNC_DELETE_TRACKS);
	key_sync_confirm_delete = get_playlist_prefs_key (
	    itdb_index, playlist, KEY_SYNC_CONFIRM_DELETE);
	key_sync_show_summary = get_playlist_prefs_key (
	    itdb_index, playlist, KEY_SYNC_SHOW_SUMMARY);
	key_manual_sync_dir = get_playlist_prefs_key (
	    itdb_index, playlist, KEY_MANUAL_SYNCDIR);
	key_syncmode = get_playlist_prefs_key (
	    itdb_index, playlist, KEY_SYNCMODE);

	/* retrieve original settings for prefs strings */
	sync_delete_tracks_orig =
	    prefs_get_string (key_sync_delete_tracks);
	sync_confirm_delete_orig =
	    prefs_get_string (key_sync_confirm_delete);
	sync_show_summary_orig =
	    prefs_get_string (key_sync_show_summary);

	/* retrieve current settings for prefs_strings */
	sync_delete_tracks_current =
	    get_current_prefs_int (repwin, key_sync_delete_tracks);
	sync_confirm_delete_current =
	    get_current_prefs_int (repwin, key_sync_confirm_delete);
	sync_show_summary_current =
	    get_current_prefs_int (repwin, key_sync_show_summary);

	/* temporarily apply current settings */
	prefs_set_int (
	    key_sync_delete_tracks, sync_delete_tracks_current);
	prefs_set_int (
	    key_sync_confirm_delete, sync_confirm_delete_current);
	prefs_set_int (
	    key_sync_show_summary, sync_show_summary_current);

	/* sync directory or directories */
	switch (get_current_prefs_int (repwin, key_syncmode))
	{
	case PLAYLIST_AUTOSYNC_MODE_NONE:
	    break; /* should never happen */
	case PLAYLIST_AUTOSYNC_MODE_MANUAL:
	    manual_sync_dir = 
		get_current_prefs_string (repwin, key_manual_sync_dir);
	    /* no break;! we continue calling sync_playlist() */
	case PLAYLIST_AUTOSYNC_MODE_AUTOMATIC:
	    sync_playlist (playlist,
			   manual_sync_dir,
			   NULL, FALSE,
			   key_sync_delete_tracks, 0,
			   key_sync_confirm_delete, 0,
			   NULL, sync_show_summary_current);
	    break;
	}

	/* Update temporary prefs in case some settings were changed
	 * (currently only 'key_sync_confirm_delete' can be changed
	 * by sync_playlist()) */
	value_new = prefs_get_int (key_sync_confirm_delete);
	if (value_new != sync_confirm_delete_current)
	{
	    if (playlist == repwin->playlist)
	    {   /* currently displayed --> adjust toggle button */
		gtk_toggle_button_set_active (
		    GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE)),
		    value_new);
	    }
	    else
	    {   /* not currently displayed --> copy to temp_prefs */
		temp_prefs_set_int (repwin->temp_prefs,
				    key_sync_confirm_delete,
				    value_new);
	    }
	}

	/* Copy original values back to prefs */
	prefs_set_string (key_sync_delete_tracks,
			  sync_delete_tracks_orig);
	prefs_set_string (key_sync_confirm_delete,
			  sync_confirm_delete_orig);
	prefs_set_string (key_sync_show_summary,
			  sync_show_summary_orig);

	/* Free memory used by all strings */
	g_free (key_sync_delete_tracks);
	g_free (key_sync_confirm_delete);
	g_free (key_sync_show_summary);
	g_free (key_manual_sync_dir);
	g_free (sync_delete_tracks_orig);
	g_free (sync_confirm_delete_orig);
	g_free (sync_show_summary_orig);
	g_free (manual_sync_dir);
    }
}




/* Callback */
static void update_all_playlists_button_clicked (GtkButton *button,
						 RepWin *repwin)
{
    GList *gl;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    for (gl=repwin->itdb->playlists; gl; gl=gl->next)
    {
	Playlist *pl = gl->data;
	g_return_if_fail (pl);
	sync_or_update_playlist (repwin, repwin->itdb_index, pl);
    }
}


/* Callback */
static void update_playlist_button_clicked (GtkButton *button,
					    RepWin *repwin)
{
    g_return_if_fail (repwin);
    sync_or_update_playlist (repwin, repwin->itdb_index, repwin->playlist);
}


/* ------------------------------------------------------------
 *
 *        Callback functions (windows control)
 *
 * ------------------------------------------------------------ */

static void apply_clicked (GtkButton *button, RepWin *repwin)
{
    gint i, itdb_num, del_num;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (repwin);

    itdbs_head = gp_get_itdbs_head (gtkpod_window);
    g_return_if_fail (itdbs_head);
    itdb_num = g_list_length (itdbs_head->itdbs);

    temp_prefs_apply (repwin->temp_prefs);

    del_num = 0;
    for (i=0; i<itdb_num; ++i)
    {
	gchar *key, *subkey;
	gboolean deleted=FALSE;

	iTunesDB *itdb = g_list_nth_data (itdbs_head->itdbs, i-del_num);
	g_return_if_fail (itdb);

	subkey = get_itdb_prefs_key (i, "");
	if (temp_prefs_subkey_exists (repwin->extra_prefs, subkey))
	{
	    gboolean deleted;
	    GList *gl;

	    key = get_itdb_prefs_key (i, "deleted");
	    deleted = temp_prefs_get_int (repwin->extra_prefs, key);
	    g_free (key);
	    if (deleted)
	    {
                /* FIXME: ask if serious, then delete */ 
		if (TRUE)
		{
		    iTunesDB *itdb;
		    gint j;

		    /* flush all keys relating to the deleted itdb */
		    key = get_itdb_prefs_key (i-del_num, "");
		    prefs_flush_subkey (key);
		    g_free (key);

		    for (j=i-del_num; j<itdb_num-del_num-1; ++j)
		    {
			gchar *from_key = get_itdb_prefs_key (j+1, "");
			gchar *to_key = get_itdb_prefs_key (j, "");
			prefs_rename_subkey (from_key, to_key);
			g_free (from_key);
			g_free (to_key);
		    }

		    itdb = g_list_nth_data (itdbs_head->itdbs, i-del_num);
		    gp_itdb_remove (itdb);
		    gp_itdb_free (itdb);

		    /* keep itdb_index of currently displayed repository
		       updated in case we need to select a new one */
		    if (repwin->itdb_index > i-del_num)
		    {
			--repwin->itdb_index;
		    }
		    ++del_num;
		}
		else
		{
		    deleted = FALSE;
		}
	    }

	    if (!deleted)
	    {
		for (gl=itdb->playlists; gl; gl=gl->next)
		{
		    Playlist *pl = gl->data;
		    gint val;
		    g_return_if_fail (pl);
		    key = get_playlist_prefs_key (i, pl, "liveupdate");
		    if (temp_prefs_get_int_value (repwin->extra_prefs, key, &val))
		    {
			pl->splpref.liveupdate = val;
			data_changed (itdb);
		    }
		    g_free (key);
		}

	    }
	}

	if (!deleted &&
	    temp_prefs_subkey_exists (repwin->temp_prefs, subkey))
	{
	    gchar *mp;

	    key = get_itdb_prefs_key (i, "mountpoint");
	    mp = temp_prefs_get_string (repwin->temp_prefs, key);
	    g_free (key);

	    if (mp)
	    {   /* have to set mountpoint */
		itdb_set_mountpoint (itdb, mp);
		space_set_ipod_mount (mp);
		g_free (mp);
	    }

	    data_changed (itdb);
	}
	g_free (subkey);
    }

    temp_prefs_destroy (repwin->temp_prefs);
    temp_prefs_destroy (repwin->extra_prefs);

    repwin->temp_prefs = temp_prefs_create ();
    repwin->extra_prefs = temp_prefs_create ();

    if (g_list_length (itdbs_head->itdbs) < itdb_num)
    {   /* at least one repository has been removed */
	iTunesDB *new_itdb = g_list_nth_data (itdbs_head->itdbs,
					      repwin->itdb_index);
	Playlist *old_playlist = repwin->playlist;

	init_repository_combo (repwin);
	if (new_itdb == repwin->itdb)
	{
	    select_repository (repwin, new_itdb, old_playlist);
	}
	else
	{
	    select_repository (repwin, new_itdb, NULL);
	}
    }
printf ("index: %d\n", repwin->itdb_index);

    update_buttons (repwin);
}

static void cancel_clicked (GtkButton *button, RepWin *repwin)
{
    g_return_if_fail (repwin);

    store_window_state (repwin);

    repwins = g_list_remove (repwins, repwin);

    repwin_free (repwin);
}


static void ok_clicked (GtkButton *button, RepWin *repwin)
{
    g_return_if_fail (repwin);

    apply_clicked (NULL, repwin);
    cancel_clicked (NULL, repwin);
}


static void delete_event (GtkWidget *widget,
			  GdkEvent *event,
			  RepWin *repwin)
{
    cancel_clicked (NULL, repwin);
}


/* Used by select_playlist() to find the new playlist. If found,
   select it */
static gboolean select_playlist_find (GtkTreeModel *model,
				      GtkTreePath *path,
				      GtkTreeIter *iter,
				      gpointer data)
{
    Playlist *playlist;
    RepWin *repwin = data;
    g_return_val_if_fail (repwin, TRUE);

    gtk_tree_model_get (model, iter, 0, &playlist, -1);
    if(playlist == repwin->next_playlist)
    {
	gtk_combo_box_set_active_iter (
	    GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
						  PLAYLIST_COMBO)),
	    iter);
	return TRUE;
    }
    return FALSE;
}


/* Select @playlist

   If @playlist == NULL, select first playlist (MPL);
*/
static void select_playlist (RepWin *repwin, Playlist *playlist)
{
    GtkTreeModel *model;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    if (!playlist)
	playlist = itdb_playlist_mpl (repwin->itdb);
    g_return_if_fail (playlist);

    g_return_if_fail (playlist->itdb == repwin->itdb);

    model= gtk_combo_box_get_model (
	GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
					      PLAYLIST_COMBO)));
    g_return_if_fail (model);

    repwin->next_playlist = playlist;
    gtk_tree_model_foreach (model, select_playlist_find, repwin);
    repwin->next_playlist = NULL;
}



/* Select @itdb and playlist @playlist

   If @itdb == NULL, only change to @playlist.

   If @playlist == NULL, select first playlist.
*/
static void select_repository (RepWin *repwin,
			       iTunesDB *itdb, Playlist *playlist)
{
    g_return_if_fail (repwin);

    if (itdb)
    {
	gint index;

	repwin->next_playlist = playlist;

	index = get_itdb_index (itdb);

	gtk_combo_box_set_active (
	    GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
						  REPOSITORY_COMBO)),
	    index);
	/* The combo callback will set the playlist */
    }
    else
    {
	if (repwin->itdb)
	    select_playlist (repwin, playlist);
    }
}


/* set @entry with value of @key */
static void set_entry (RepWin *repwin,
		       const gchar *key, const gchar *entry)
{
    gchar *buf;

    g_return_if_fail (repwin && key && entry);

    buf = get_current_prefs_string (repwin, key);
    gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (entry)),
			buf);
    g_free (buf);
}


/****** Fill in info about selected repository *****/
static void display_repository_info (RepWin *repwin)
{
    iTunesDB *itdb;
    gint index;
    gchar *buf, *key;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    itdb = repwin->itdb;
    index = repwin->itdb_index;

    /* Repository type */
    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("iPod"));
    }
    else if (itdb->usertype & GP_ITDB_TYPE_PODCASTS)
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("Podcasts Repository"));
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("Local Repository"));
    }
    else
    {
	buf = g_markup_printf_escaped ("<b>Unknown -- please report bug</b>");
    }
    gtk_label_set_markup (GTK_LABEL(GET_WIDGET("repository_type_label")),
			  buf);
    g_free (buf);

    /* Hide/show corresponding widgets in table */
    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	const gchar *widgets_show[] = {
	    MOUNTPOINT_LABEL,
	    MOUNTPOINT_ENTRY,
	    MOUNTPOINT_BUTTON,
	    BACKUP_LABEL,
	    BACKUP_ENTRY,
	    IPOD_MODEL_LABEL,
	    IPOD_MODEL_ENTRY,
	    LOCAL_PATH_ENTRY,
	    IPOD_SYNC_LABEL,
	    IPOD_SYNC_CONTACTS_LABEL,
	    IPOD_SYNC_CONTACTS_ENTRY,
	    IPOD_SYNC_CONTACTS_BUTTON,
	    IPOD_SYNC_CALENDAR_LABEL,
	    IPOD_SYNC_CALENDAR_ENTRY,
	    IPOD_SYNC_CALENDAR_BUTTON,
	    IPOD_SYNC_NOTES_LABEL,
	    IPOD_SYNC_NOTES_ENTRY,
	    IPOD_SYNC_NOTES_BUTTON,
	    IPOD_CONCAL_AUTOSYNC_TOGGLE,
	    NULL};
	const gchar *widgets_hide[] = {
	    LOCAL_PATH_LABEL,
	    LOCAL_PATH_ENTRY,
	    NULL};
	const gchar **widget;

	for (widget=widgets_show; *widget; ++widget)
	{
	    gtk_widget_show (GET_WIDGET (*widget));
	}
	for (widget=widgets_hide; *widget; ++widget)
	{
	    gtk_widget_hide (GET_WIDGET (*widget));
	}

	key = get_itdb_prefs_key (index, "mountpoint");
	set_entry (repwin, key, MOUNTPOINT_ENTRY);
	g_free (key);

	key = get_itdb_prefs_key (index, "path_sync_contacts");
	set_entry (repwin, key, IPOD_SYNC_CONTACTS_ENTRY);
	g_free (key);

	key = get_itdb_prefs_key (index, "path_sync_calendar");
	set_entry (repwin, key, IPOD_SYNC_CALENDAR_ENTRY);
	g_free (key);

	key = get_itdb_prefs_key (index, "path_sync_notes");
	set_entry (repwin, key, IPOD_SYNC_NOTES_ENTRY);
	g_free (key);

	key = get_itdb_prefs_key (index, KEY_CONCAL_AUTOSYNC);
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (GET_WIDGET (IPOD_CONCAL_AUTOSYNC_TOGGLE)),
	    get_current_prefs_int (repwin, key));
	g_free (key);
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	const gchar *widgets_show[] = {
	    LOCAL_PATH_LABEL,
	    LOCAL_PATH_ENTRY,
	    NULL};
	const gchar *widgets_hide[] = {
	    MOUNTPOINT_LABEL,
	    MOUNTPOINT_ENTRY,
	    MOUNTPOINT_BUTTON,
	    BACKUP_LABEL,
	    BACKUP_ENTRY,
	    IPOD_MODEL_LABEL,
	    IPOD_MODEL_ENTRY,
	    IPOD_SYNC_LABEL,
	    IPOD_SYNC_CONTACTS_LABEL,
	    IPOD_SYNC_CONTACTS_ENTRY,
	    IPOD_SYNC_CONTACTS_BUTTON,
	    IPOD_SYNC_CALENDAR_LABEL,
	    IPOD_SYNC_CALENDAR_ENTRY,
	    IPOD_SYNC_CALENDAR_BUTTON,
	    IPOD_SYNC_NOTES_LABEL,
	    IPOD_SYNC_NOTES_ENTRY,
	    IPOD_SYNC_NOTES_BUTTON,
	    IPOD_CONCAL_AUTOSYNC_TOGGLE,
	    NULL};
	const gchar **widget;

	for (widget=widgets_show; *widget; ++widget)
	{
	    gtk_widget_show (GET_WIDGET (*widget));
	}
	for (widget=widgets_hide; *widget; ++widget)
	{
	    gtk_widget_hide (GET_WIDGET (*widget));
	}

	key = get_itdb_prefs_key (index, "filename");
	set_entry (repwin, key, LOCAL_PATH_ENTRY);
	g_free (key);
    }
    else
    {
	g_return_if_reached ();
    }

    key = get_itdb_prefs_key (index, "deleted");
    gtk_toggle_button_set_active (
	GTK_TOGGLE_BUTTON (GET_WIDGET (DELETE_REPOSITORY_CHECKBUTTON)),
	temp_prefs_get_int (repwin->extra_prefs, key));
    g_free (key);
}


/****** Fill in info about selected playlist *****/
static void display_playlist_info (RepWin *repwin)
{
    gchar *buf, *key;
    Playlist *playlist;
    iTunesDB *itdb;
    gint i, index;
    const gchar *widget_names[] = {
	PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE,
	PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE,
	PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE,
	NULL };
    const gchar *key_names[] = {
	KEY_SYNC_DELETE_TRACKS,
	KEY_SYNC_CONFIRM_DELETE,
	KEY_SYNC_SHOW_SUMMARY,
	NULL };

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);
    g_return_if_fail (repwin->playlist);

    /* for convenience */
    itdb = repwin->itdb;
    index = repwin->itdb_index;
    playlist = repwin->playlist;

    /* Playlist type */
    if (itdb_playlist_is_mpl (playlist))
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("Master Playlist"));
    }
    else if (itdb_playlist_is_podcasts (playlist))
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("Podcasts Playlist"));
    }
    else if (playlist->is_spl)
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("Smart Playlist"));
    }
    else
    {
	buf = g_markup_printf_escaped ("<i>%s</i>", _("Regular Playlist"));
    }
    gtk_label_set_markup (GTK_LABEL(GET_WIDGET("playlist_type_label")),
			  buf);
    g_free (buf);

    /* Hide/show corresponding widgets in table */
    if (playlist->is_spl)
    {
	gint liveupdate;

	gtk_widget_show (GET_WIDGET (SPL_VBOX));
	gtk_widget_show (GET_WIDGET (PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE));
	gtk_widget_hide (GET_WIDGET (STANDARD_PLAYLIST_VBOX));

	key = get_playlist_prefs_key (index, playlist, "liveupdate");
	if (!temp_prefs_get_int_value (repwin->extra_prefs,
				       key, &liveupdate))
	    liveupdate = playlist->splpref.liveupdate;
	g_free (key);

	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (GET_WIDGET (SPL_LIVE_UPDATE_TOGGLE)),
	    liveupdate);
    }
    else
    {
	gint syncmode;
	gchar *dir;
	gtk_widget_show (GET_WIDGET (STANDARD_PLAYLIST_VBOX));
	gtk_widget_hide (GET_WIDGET (SPL_VBOX));

	key = get_playlist_prefs_key (index, playlist, KEY_SYNCMODE);
	syncmode = get_current_prefs_int (repwin, key);
	g_free (key);

	/* Need to set manual_syncdir_entry here as it may set the
	   syncmode to 'MANUAL' -- this will be corrected by setting
	   the radio button with the original syncmode setting further
	   down. */
	key = get_playlist_prefs_key (index, playlist,
				      KEY_MANUAL_SYNCDIR);
	dir = get_current_prefs_string (repwin, key);
	g_free (key);
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (MANUAL_SYNCDIR_ENTRY)),
			    dir);
	g_free (dir);

	switch (syncmode)
	{
	case PLAYLIST_AUTOSYNC_MODE_NONE:
	    gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_NONE_RADIO)),
		TRUE);
	    break;
	case PLAYLIST_AUTOSYNC_MODE_MANUAL:
	    gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_MANUAL_RADIO)),
		TRUE);
	    break;
	case PLAYLIST_AUTOSYNC_MODE_AUTOMATIC:
	    gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_AUTOMATIC_RADIO)),
		TRUE);
	    break;
	default:
	    /* repair broken prefs */
	    prefs_set_int (key, PLAYLIST_AUTOSYNC_MODE_NONE);
	    gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_NONE_RADIO)),
		TRUE);
	    break;
	}
	/* make options available where appropriate */
	gtk_widget_set_sensitive (GET_WIDGET (SYNC_OPTIONS_HBOX),
				  syncmode != PLAYLIST_AUTOSYNC_MODE_NONE);
	/* set standard toggle buttons */
	for (i=0; widget_names[i]; ++i)
	{
	    key = get_playlist_prefs_key (index, playlist, key_names[i]);
	    gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (GET_WIDGET (widget_names[i])),
		get_current_prefs_int (repwin, key));
	    g_free (key);
	}
    }
}



/****** New repository was selected */
static void repository_changed (GtkComboBox *combobox,
				RepWin *repwin)
{
    struct itdbs_head *itdbs_head;
    iTunesDB *itdb;
    gint index;

printf ("Repository changed (%p)\n", repwin);

    g_return_if_fail (repwin);

    index = gtk_combo_box_get_active (
	GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
					      REPOSITORY_COMBO)));

    itdbs_head = gp_get_itdbs_head (gtkpod_window);
    g_return_if_fail (itdbs_head);

    itdb = g_list_nth_data (itdbs_head->itdbs, index);

    if (repwin->itdb != itdb)
    {
	repwin->itdb = itdb;
	repwin->itdb_index = index;
	display_repository_info (repwin);
	init_playlist_combo (repwin);
	update_buttons (repwin);
    }
}



/****** New playlist was selected */
static void playlist_changed (GtkComboBox *combobox,
				RepWin *repwin)
{
    GtkTreeModel *model;
    GtkComboBox *cb;
    Playlist *playlist;
    GtkTreeIter iter;
    gint index;

printf ("Playlist changed (%p)\n", repwin);

    g_return_if_fail (repwin);

    cb = GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
					       PLAYLIST_COMBO));

    index = gtk_combo_box_get_active (cb);
    /* We can't just use the index to find the right playlist in
       itdb->playlists because they might have been reordered. Instead
       use the playlist pointer stored in the model. */
    model= gtk_combo_box_get_model (cb);
    g_return_if_fail (model);
    g_return_if_fail (gtk_tree_model_iter_nth_child (model, &iter,
						     NULL, index));
    gtk_tree_model_get (model, &iter, 0, &playlist, -1);

    if (repwin->playlist != playlist)
    {
	g_return_if_fail (playlist->itdb == repwin->itdb);
	repwin->playlist = playlist;
	display_playlist_info (repwin);
    }
}



/****** Initialize the repository combo *****/
static void init_repository_combo (RepWin *repwin)
{
    struct itdbs_head *itdbs_head;
    GtkCellRenderer *cell;
    GtkListStore *store;
    GtkComboBox *cb;
    GList *gl;

    g_return_if_fail (repwin);

    itdbs_head = gp_get_itdbs_head (gtkpod_window);
    g_return_if_fail (itdbs_head);

    cb = GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
					       REPOSITORY_COMBO));

    if (g_object_get_data (G_OBJECT (cb), "combo_set") == NULL)
    {   /* the combo has not yet been initialized */
	cell = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (cell),
		      "weight", PANGO_WEIGHT_BOLD,
		      "style", PANGO_STYLE_NORMAL,
		      NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), cell,
					"text", 0,
					NULL);

	g_signal_connect (cb, "changed",
			  G_CALLBACK (repository_changed), repwin);

	g_object_set_data (G_OBJECT (cb), "combo_set", "set");
    }

    store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_combo_box_set_model (cb, GTK_TREE_MODEL (store));
    g_object_unref (store);

    for (gl=itdbs_head->itdbs; gl; gl=gl->next)
    {
	iTunesDB *itdb = gl->data;
	Playlist *mpl;
	g_return_if_fail (itdb);

	mpl = itdb_playlist_mpl (itdb);
	g_return_if_fail (mpl);

	gtk_combo_box_append_text (cb, mpl->name);
    }

    repwin->itdb = NULL;
    repwin->playlist = NULL;
}



/* Provide graphic indicator in playlist combobox */
static void playlist_cb_cell_data_func_pix (GtkCellLayout *cell_layout,
					    GtkCellRenderer *cell,
					    GtkTreeModel *model,
					    GtkTreeIter *iter,
					    gpointer data)
{
    Playlist *playlist;

    g_return_if_fail (cell);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get (model, iter, 0, &playlist, -1);

    /* playlist == NULL is allowed (not item set) */
    if (playlist)
    {
	if (playlist->is_spl)
	{
	    g_object_set (G_OBJECT (cell),
			  "stock-id", GTK_STOCK_PROPERTIES, NULL);
	}
	else if (!itdb_playlist_is_mpl (playlist))
	{
	    g_object_set (G_OBJECT (cell),
			  "stock-id", GTK_STOCK_JUSTIFY_LEFT, NULL);
	}
	else
	{
	    g_object_set (G_OBJECT (cell),
			  "stock-id", NULL, NULL);
	}
    }
    else
    {
	g_object_set (G_OBJECT (cell),
		      "stock-id", NULL, NULL);
    }
}


/* Provide playlist name in combobox */
static void playlist_cb_cell_data_func_text (GtkCellLayout *cell_layout,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel *model,
                                             GtkTreeIter *iter,
                                             gpointer data)
{
    Playlist *playlist;

    g_return_if_fail (cell);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get (model, iter, 0, &playlist, -1);

    /* playlist == NULL is allowed (not item set) */
    if (playlist)
    {
	if (itdb_playlist_is_mpl (playlist))
	{   /* mark MPL */
	    g_object_set (G_OBJECT (cell),
			  "text", playlist->name, 
			  "weight", PANGO_WEIGHT_BOLD,
			  "style", PANGO_STYLE_NORMAL,
			  NULL);
	}
	else
	{
	    if (itdb_playlist_is_podcasts (playlist))
	    {
		g_object_set (G_OBJECT (cell),
			      "text", playlist->name, 
			      "weight", PANGO_WEIGHT_SEMIBOLD,
			      "style", PANGO_STYLE_ITALIC,
			      NULL);
	    }
	    else
	    {
		g_object_set (G_OBJECT (cell),
			      "text", playlist->name, 
			      "weight", PANGO_WEIGHT_NORMAL,
			      "style", PANGO_STYLE_NORMAL,
			      NULL);
	    }
	}
    }
    else
    {
	g_object_set (G_OBJECT (cell),
		      "text", "",
		      NULL);
    }
}



/****** Initialize the playlist combo *****/
static void init_playlist_combo (RepWin *repwin)
{
    GtkCellRenderer *cell;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkComboBox *cb;
    GList *gl;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    cb = GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
					       PLAYLIST_COMBO));

    if (g_object_get_data (G_OBJECT (cb), "combo_set") == NULL)
    {
	/* Cell for graphic indicator */
	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), cell, FALSE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb), cell,
					    playlist_cb_cell_data_func_pix,
					    NULL, NULL);
	/* Cell for playlist name */
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), cell, FALSE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb), cell,
					    playlist_cb_cell_data_func_text,
					    NULL, NULL);

	g_signal_connect (cb, "changed",
			  G_CALLBACK (playlist_changed), repwin);

	g_object_set_data (G_OBJECT (cb), "combo_set", "set");
    }

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    gtk_combo_box_set_model (cb, GTK_TREE_MODEL (store));
    g_object_unref (store);

    if (repwin->itdb)
    {
	for (gl=repwin->itdb->playlists; gl; gl=gl->next)
	{
	    Playlist *pl = gl->data;
	    g_return_if_fail (pl);

	    gtk_list_store_append (store, &iter);
	    gtk_list_store_set (store, &iter, 0, pl, -1);
	}
    }

    if (repwin->itdb)
    {
	select_playlist (repwin, repwin->next_playlist);
	repwin->next_playlist = NULL;
    }
}



/* Render apply insensitive when no changes were made.
   When an itdb is marked for deletion, make entries insensitive */
static void update_buttons (RepWin *repwin)
{
    gboolean apply, ok, deleted;
    gchar *key;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->temp_prefs);
    g_return_if_fail (repwin->extra_prefs);

    if ((temp_prefs_size (repwin->temp_prefs) > 0) ||
	(temp_prefs_size (repwin->extra_prefs) > 0))
    {
	apply = TRUE;
	ok = TRUE;
    }
    else
    {
	apply = FALSE;
	ok = TRUE;
    }

    gtk_widget_set_sensitive (GET_WIDGET ("apply_button"), apply);
    gtk_widget_set_sensitive (GET_WIDGET ("ok_button"), ok);

    if (repwin->itdb)
    {
	gtk_widget_set_sensitive (GET_WIDGET (REPOSITORY_VBOX), TRUE);

	/* Check if this itdb is marked for deletion */
	key = get_itdb_prefs_key (repwin->itdb_index, "deleted");
	deleted = temp_prefs_get_int (repwin->extra_prefs, key);
	g_free (key);

	gtk_widget_set_sensitive (GET_WIDGET ("path_table_ipod"), !deleted);
	gtk_widget_set_sensitive (GET_WIDGET ("update_all_playlists_button"), !deleted);
	gtk_widget_set_sensitive (GET_WIDGET ("playlist_info_view"), !deleted);
	gtk_widget_set_sensitive (GET_WIDGET ("delete_repository_button"),
				  !deleted && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET (DELETE_REPOSITORY_CHECKBUTTON))));

	if (repwin->playlist)
	{
	    gboolean sens = FALSE;
	    if (repwin->playlist->is_spl)
	    {
		sens = TRUE;
	    }
	    else
	    {
		gint mode;
		key = get_playlist_prefs_key (repwin->itdb_index,
					      repwin->playlist,
					      KEY_SYNCMODE);
		mode = get_current_prefs_int (repwin, key);
		g_free (key);
		if (mode != PLAYLIST_AUTOSYNC_MODE_NONE)
		{
		    sens = TRUE;
		}
		gtk_widget_set_sensitive (GET_WIDGET (SYNC_OPTIONS_HBOX),
					  sens);
	    }
	    gtk_widget_set_sensitive (GET_WIDGET (UPDATE_PLAYLIST_BUTTON),
				      sens);
	}
    }
    else
    {   /* no itdb loaded */
	gtk_widget_set_sensitive (GET_WIDGET (REPOSITORY_VBOX), FALSE);
    }
}



/* Free memory taken by @repwin */
static void repwin_free (RepWin *repwin)
{
    g_return_if_fail (repwin);

    /* FIXME: how do we free the repwin->xml? */

    if (repwin->window)
    {
	gtk_widget_destroy (repwin->window);
    }

    g_free (repwin);
}


/* Open the repository options window and display repository @itdb and
   playlist @playlist. If @itdb and @playlist is NULL, display first
   repository and playlist. If @itdb is NULL and @playlist is not
   NULL, display @playlist and assume repository is playlist->itdb. */
void repository_edit (iTunesDB *itdb, Playlist *playlist)
{
    RepWin *repwin;
    gint defx, defy;
    gint i;
    /* widget names and corresponding keys for 'standard' toggle
       options */
    const gchar *playlist_widget_names_toggle[] = {
	PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE,
	PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE,
	PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE,
	NULL };
    const gchar *playlist_key_names_toggle[] = {
	KEY_SYNC_DELETE_TRACKS,
	KEY_SYNC_CONFIRM_DELETE,
	KEY_SYNC_SHOW_SUMMARY,
	NULL };
    const gchar *itdb_widget_names_toggle[] = {
	IPOD_CONCAL_AUTOSYNC_TOGGLE,
	NULL };
    const gchar *itdb_key_names_toggle[] = {
	KEY_CONCAL_AUTOSYNC,
	NULL };
    /* widget names and corresponding keys for 'standard' strings */
    const gchar *itdb_widget_names_entry[] = {
	MOUNTPOINT_ENTRY,
	IPOD_MODEL_ENTRY,
	LOCAL_PATH_ENTRY,
	IPOD_SYNC_CONTACTS_ENTRY,
	IPOD_SYNC_CALENDAR_ENTRY,
	IPOD_SYNC_NOTES_ENTRY,
	NULL };
    const gchar *itdb_key_names_entry[] = {
	KEY_MOUNTPOINT,
	KEY_IPOD_MODEL,
	KEY_FILENAME,
	KEY_PATH_SYNC_CONTACTS,
	KEY_PATH_SYNC_CALENDAR,
	KEY_PATH_SYNC_NOTES,
	NULL };

    /* If window is already open, raise existing window to the top */
    if (repwins)
    {
	repwin = repwins->data;
	g_return_if_fail (repwin);
	g_return_if_fail (repwin->window);
	gdk_window_raise (repwin->window->window);
	return;
    }

    repwin = g_malloc0 (sizeof (RepWin));

    repwin->xml = glade_xml_new (xml_file, "repository_window", NULL);
/*  no signals to connect -> comment out */
/*     glade_xml_signal_autoconnect (detail->xml); */
    repwin->window = gtkpod_xml_get_widget (repwin->xml,
						"repository_window");
    g_return_if_fail (repwin->window);

    repwins = g_list_append (repwins, repwin);

    /* Window control */
    g_signal_connect (GET_WIDGET ("apply_button"), "clicked",
		      G_CALLBACK (apply_clicked), repwin);

    g_signal_connect (GET_WIDGET ("cancel_button"), "clicked",
		      G_CALLBACK (cancel_clicked), repwin);

    g_signal_connect (GET_WIDGET ("ok_button"), "clicked",
		      G_CALLBACK (ok_clicked), repwin);

    g_signal_connect (repwin->window, "delete_event",
		      G_CALLBACK (delete_event), repwin);

    /* Entry callbacks */
    g_signal_connect (GET_WIDGET (MANUAL_SYNCDIR_ENTRY), "changed",
		      G_CALLBACK (manual_syncdir_changed), repwin);
    /* connect standard text entries */
    for (i=0; itdb_widget_names_entry[i]; ++i)
    {
	GtkWidget *w = GET_WIDGET (itdb_widget_names_entry[i]);

	g_signal_connect (w, "changed",
			  G_CALLBACK (standard_itdb_entry_changed),
			  repwin);
	g_object_set_data (G_OBJECT (w), "key",
			   (gpointer)itdb_key_names_entry[i]);
    }

    /* Togglebutton callbacks */
    g_signal_connect (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_NONE_RADIO),
		      "toggled",
		      G_CALLBACK (playlist_autosync_mode_none_toggled),
		      repwin);

    g_signal_connect (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_MANUAL_RADIO),
		      "toggled",
		      G_CALLBACK (playlist_autosync_mode_manual_toggled),
		      repwin);

    g_signal_connect (GET_WIDGET (PLAYLIST_AUTOSYNC_MODE_AUTOMATIC_RADIO),
		      "toggled",
		      G_CALLBACK (playlist_autosync_mode_automatic_toggled),
		      repwin);

    g_signal_connect (GET_WIDGET (SPL_LIVE_UPDATE_TOGGLE),
		      "toggled",
		      G_CALLBACK (spl_live_update_toggled), repwin);

    g_signal_connect (GET_WIDGET (DELETE_REPOSITORY_CHECKBUTTON),
		      "toggled",
		      G_CALLBACK (delete_repository_checkbutton_toggled),
		      repwin);
    /* connect standard toggle buttons */
    for (i=0; playlist_widget_names_toggle[i]; ++i)
    {
	GtkWidget *w = GET_WIDGET (playlist_widget_names_toggle[i]);

	g_signal_connect (w, "toggled",
			  G_CALLBACK (standard_playlist_checkbutton_toggled),
			  repwin);
	g_object_set_data (G_OBJECT (w), "key",
			   (gpointer)playlist_key_names_toggle[i]);
    }
    for (i=0; itdb_widget_names_toggle[i]; ++i)
    {
	GtkWidget *w = GET_WIDGET (itdb_widget_names_toggle[i]);

	g_signal_connect (w, "toggled",
			  G_CALLBACK (standard_itdb_checkbutton_toggled),
			  repwin);
	g_object_set_data (G_OBJECT (w), "key",
			   (gpointer)itdb_key_names_toggle[i]);
    }


    /* Button callbacks */
    g_signal_connect (GET_WIDGET (MOUNTPOINT_BUTTON), "clicked",
		      G_CALLBACK (mountpoint_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (MANUAL_SYNCDIR_BUTTON), "clicked",
 		      G_CALLBACK (manual_syncdir_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (DELETE_REPOSITORY_BUTTON), "clicked",
 		      G_CALLBACK (delete_repository_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (IPOD_SYNC_CONTACTS_BUTTON), "clicked",
 		      G_CALLBACK (ipod_sync_contacts_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (IPOD_SYNC_CALENDAR_BUTTON), "clicked",
 		      G_CALLBACK (ipod_sync_calendar_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (IPOD_SYNC_NOTES_BUTTON), "clicked",
 		      G_CALLBACK (ipod_sync_notes_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (UPDATE_PLAYLIST_BUTTON), "clicked",
 		      G_CALLBACK (update_playlist_button_clicked), repwin);

    g_signal_connect (GET_WIDGET (UPDATE_ALL_PLAYLISTS_BUTTON), "clicked",
 		      G_CALLBACK (update_all_playlists_button_clicked), repwin);

    init_repository_combo (repwin);

    /* set default size */
    defx = prefs_get_int (KEY_REPOSITORY_WINDOW_DEFX);
    defy = prefs_get_int (KEY_REPOSITORY_WINDOW_DEFY);

    if ((defx == 0) || (defy == 0))
    {
	defx = 540;
	defy = 480;
    }
/*     gtk_window_set_default_size (GTK_WINDOW (repwin->window), */
/* 				 defx, defy); */
    gtk_window_resize (GTK_WINDOW (repwin->window),
		       defx, defy);

    /* Set up temp_prefs struct */
    repwin->temp_prefs = temp_prefs_create ();
    repwin->extra_prefs = temp_prefs_create ();

    if (!itdb && playlist)
	itdb = playlist->itdb;
    if (!itdb)
    {
	struct itdbs_head *itdbs_head = gp_get_itdbs_head (gtkpod_window);
	itdb = g_list_nth_data (itdbs_head->itdbs, 0);
    }
    g_return_if_fail (itdb);

    select_repository (repwin, itdb, playlist);

    update_buttons (repwin);

    gtk_widget_show (repwin->window);
}


/* Use the dimension of the first open window */
void update_default_sizes (void)
{
    if (repwins)
	store_window_state (repwins->data);
}
