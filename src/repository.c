/* Time-stamp: <2008-10-01 23:36:57 jcs>
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

#include "plugin.h"
#include "repository.h"
#include "fileselection.h"
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/ipod_init.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/misc_playlist.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/syncdir.h"
#include "libgtkpod/gtkpod_app_iface.h"

/* print local debug message */
#define LOCAL_DEBUG 0

struct _RepWin {
    GladeXML *xml;           /* XML info                             */
    GtkWidget *window; /* pointer to repository window         */
    iTunesDB *itdb; /* currently displayed repository       */
    gint itdb_index; /* index number of itdb                 */
    Playlist *playlist; /* currently displayed playlist         */
    Playlist *next_playlist; /* playlist to display next (or NULL)   */
    TempPrefs *temp_prefs; /* changes made so far                  */
    TempPrefs *extra_prefs; /* changes to non-prefs items (e.g.
     live-update                          */
};

typedef struct _RepWin RepWin;

typedef enum {
    IPOD_SYNC_CONTACTS, IPOD_SYNC_CALENDAR, IPOD_SYNC_NOTES
} iPodSyncType;

/* string constants for window widgets used more than once */
static const gchar *SYNC_FRAME = "sync_frame";
static const gchar *PLAYLIST_COMBO = "playlist_combo";
static const gchar *REPOSITORY_COMBO = "repository_combo";
static const gchar *MOUNTPOINT_LABEL = "mountpoint_label";
static const gchar *MOUNTPOINT_ENTRY = "mountpoint_entry";
static const gchar *MOUNTPOINT_BUTTON = "mountpoint_button";
static const gchar *BACKUP_LABEL = "backup_label";
static const gchar *BACKUP_ENTRY = "backup_entry";
static const gchar *BACKUP_BUTTON = "backup_button";
static const gchar *IPOD_MODEL_LABEL = "ipod_model_label";
static const gchar *IPOD_MODEL_COMBO = "ipod_model_combo";
static const gchar *IPOD_MODEL_ENTRY = "ipod_model_entry--not-a-glade-name";
static const gchar *LOCAL_PATH_LABEL = "local_path_label";
static const gchar *LOCAL_PATH_ENTRY = "local_path_entry";
static const gchar *STANDARD_PLAYLIST_VBOX = "standard_playlist_vbox";
static const gchar *SPL_VBOX = "spl_vbox";
static const gchar *SPL_LIVE_UPDATE_TOGGLE = "spl_live_update_toggle";
static const gchar *SYNC_PLAYLIST_MODE_NONE_RADIO = "sync_playlist_mode_none_radio";
static const gchar *SYNC_PLAYLIST_MODE_AUTOMATIC_RADIO = "sync_playlist_mode_automatic_radio";
static const gchar *SYNC_PLAYLIST_MODE_MANUAL_RADIO = "sync_playlist_mode_manual_radio";
static const gchar *MANUAL_SYNCDIR_ENTRY = "manual_syncdir_entry";
static const gchar *MANUAL_SYNCDIR_BUTTON = "manual_syncdir_button";
static const gchar *DELETE_REPOSITORY_BUTTON = "delete_repository_button";
static const gchar *REPOSITORY_VBOX = "repository_vbox";
static const gchar *IPOD_SYNC_CONTACTS_ENTRY = "ipod_sync_contacts_entry";
static const gchar *IPOD_SYNC_CONTACTS_BUTTON = "ipod_sync_contacts_button";
static const gchar *IPOD_SYNC_CALENDAR_ENTRY = "ipod_sync_calendar_entry";
static const gchar *IPOD_SYNC_CALENDAR_BUTTON = "ipod_sync_calendar_button";
static const gchar *IPOD_SYNC_NOTES_ENTRY = "ipod_sync_notes_entry";
static const gchar *IPOD_SYNC_NOTES_BUTTON = "ipod_sync_notes_button";
static const gchar *IPOD_CONCAL_AUTOSYNC_TOGGLE = "ipod_concal_autosync_toggle";
static const gchar *SYNC_OPTIONS_HBOX = "sync_options_hbox";
static const gchar *PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE = "playlist_sync_delete_tracks_toggle";
static const gchar *PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE = "playlist_sync_confirm_delete_toggle";
static const gchar *PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE = "playlist_sync_show_summary_toggle";
static const gchar *UPDATE_PLAYLIST_BUTTON = "update_playlist_button";
static const gchar *UPDATE_ALL_PLAYLISTS_BUTTON = "update_all_playlists_button";
static const gchar *NEW_REPOSITORY_BUTTON = "new_repository_button";

/* some private key names used several times */
static const gchar *KEY_LIVEUPDATE = "liveupdate";

/* some public key names used several times */
const gchar *KEY_CONCAL_AUTOSYNC = "concal_autosync";
const gchar *KEY_SYNC_DELETE_TRACKS = "sync_delete_tracks";
const gchar *KEY_SYNC_CONFIRM_DELETE = "sync_confirm_delete";
const gchar *KEY_SYNC_CONFIRM_DIRS = "sync_confirm_dirs";
const gchar *KEY_SYNC_SHOW_SUMMARY = "sync_show_summary";
const gchar *KEY_MOUNTPOINT = "mountpoint";
const gchar *KEY_BACKUP = "filename";
const gchar *KEY_IPOD_MODEL = "ipod_model";
const gchar *KEY_FILENAME = "filename";
const gchar *KEY_PATH_SYNC_CONTACTS = "path_sync_contacts";
const gchar *KEY_PATH_SYNC_CALENDAR = "path_sync_calendar";
const gchar *KEY_PATH_SYNC_NOTES = "path_sync_notes";
const gchar *KEY_SYNCMODE = "syncmode";
const gchar *KEY_MANUAL_SYNCDIR = "manual_syncdir";

/* widget names for the "Create New Repository" window */
static const gchar *CRW_BACKUP_BUTTON = "crw_backup_button";
static const gchar *CRW_BACKUP_ENTRY = "crw_backup_entry";
static const gchar *CRW_BACKUP_LABEL = "crw_backup_label";
static const gchar *CRW_CANCEL_BUTTON = "crw_cancel_button";
static const gchar *CRW_INSERT_BEFORE_AFTER_COMBO = "crw_insert_before_after_combo";
static const gchar *CRW_IPOD_MODEL_COMBO = "crw_ipod_model_combo";
static const gchar *CRW_IPOD_MODEL_ENTRY = "crw_ipod_model_entry";
static const gchar *CRW_IPOD_MODEL_LABEL = "crw_ipod_model_label";
static const gchar *CRW_LOCAL_PATH_BUTTON = "crw_local_path_button";
static const gchar *CRW_LOCAL_PATH_ENTRY = "crw_local_path_entry";
static const gchar *CRW_LOCAL_PATH_LABEL = "crw_local_path_label";
static const gchar *CRW_MOUNTPOINT_BUTTON = "crw_mountpoint_button";
static const gchar *CRW_MOUNTPOINT_ENTRY = "crw_mountpoint_entry";
static const gchar *CRW_MOUNTPOINT_LABEL = "crw_mountpoint_label";
static const gchar *CRW_OK_BUTTON = "crw_ok_button";
static const gchar *CRW_REPOSITORY_COMBO = "crw_repository_combo";
static const gchar *CRW_REPOSITORY_NAME_ENTRY = "crw_repository_name_entry";
static const gchar *CRW_REPOSITORY_TYPE_COMBO = "crw_repository_type_combo";

/* Declarations */
static void update_buttons(RepWin *repwin);
static void repwin_free(RepWin *repwin);
static void init_repository_combo(RepWin *repwin);
static void init_playlist_combo(RepWin *repwin);
static void select_repository(RepWin *repwin, iTunesDB *itdb, Playlist *playlist);
static void create_repository(RepWin *repwin);

RepWin *repository_window = NULL;

/* ------------------------------------------------------------
 *
 *        Helper functions to retrieve widgets.
 *
 * ------------------------------------------------------------ */
/* shortcut to reference widgets when repwin->xml is already set */
#define GET_WIDGET(a) repository_xml_get_widget (repository_window->xml,a)

/* This is quite dirty: MODEL_ENTRY is not a real widget
 name. Instead it's the entry of a ComboBoxEntry -- hide this from
 the application */
GtkWidget *repository_xml_get_widget(GladeXML *xml, const gchar *name) {
    if (name == IPOD_MODEL_ENTRY) {
        GtkWidget *cb = gtkpod_xml_get_widget(xml, IPOD_MODEL_COMBO);
        return gtk_bin_get_child(GTK_BIN (cb));
    }
    else {
        return gtkpod_xml_get_widget(xml, name);
    }
}

/* ------------------------------------------------------------
 *
 *        Helper functions for prefs interfaceing.
 *
 * ------------------------------------------------------------ */

/* Get prefs string -- either from repwin->temp_prefs or from the main
 prefs system. Return an empty string if no value was set. */
/* Free string after use */
static gchar *get_current_prefs_string(RepWin *repwin, const gchar *key) {
    gchar *value;

    g_return_val_if_fail (repwin && key, NULL);

    value = temp_prefs_get_string(repwin->temp_prefs, key);
    if (value == NULL) {
        value = prefs_get_string(key);
    }
    if (value == NULL) {
        value = g_strdup("");
    }
    return value;
}

/* Get integer prefs value -- either from repwin->temp_prefs or from
 the main prefs system. Return 0 if no value was set. */
/* Free string after use */
static gint get_current_prefs_int(RepWin *repwin, const gchar *key) {
    gint value;

    g_return_val_if_fail (repwin && key, 0);

    if (!temp_prefs_get_int_value(repwin->temp_prefs, key, &value)) {
        value = prefs_get_int(key);
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
static gboolean finish_string_storage(RepWin *repwin, gchar *key, gchar *str) {
    gchar *prefs_str;
    gboolean result;

    g_return_val_if_fail (repwin && key && str, FALSE);

    prefs_str = prefs_get_string(key);

    if ((!prefs_str && (strlen(str) > 0)) || (prefs_str && (strcmp(str, prefs_str) != 0))) { /* value has changed with respect to the value stored in the
     prefs */
#       if LOCAL_DEBUG
        printf ("setting '%s' to '%s'\n", key, str);
#       endif
        temp_prefs_set_string(repwin->temp_prefs, key, str);
        result = TRUE;
    }
    else { /* value has not changed -- remove key from temp prefs (in
     case it exists */
#       if LOCAL_DEBUG
        printf ("removing '%s'.\n", key);
#       endif
        temp_prefs_remove_key(repwin->temp_prefs, key);
        result = FALSE;
    }
    update_buttons(repwin);
    g_free(key);
    g_free(str);
    g_free(prefs_str);
    return result;
}

/* Retrieve the current text in @editable and call
 finish_string_storage()

 Return value: see finish_string_storage() */
static gboolean finish_editable_storage(RepWin *repwin, gchar *key, GtkEditable *editable) {
    gchar *str;

    g_return_val_if_fail (repwin && key && editable, FALSE);

    str = gtk_editable_get_chars(editable, 0, -1);
    return finish_string_storage(repwin, key, str);
}

/* Compare the value of @val with the value stored for @key in the
 prefs system. If values differ, store @val for @key in
 @repwin->temp_prefs, otherwise remove a possibly existing entry
 @key in @repwin->temp_prefs. */
static void finish_int_storage(RepWin *repwin, gchar *key, gint val) {
    gint prefs_val;

    g_return_if_fail (repwin && key);

    /* defaults to '0' if not set */
    prefs_val = prefs_get_int(key);
    if (prefs_val != val) { /* value has changed with respect to the value stored in the
     prefs */
#       if LOCAL_DEBUG
        printf ("setting '%s' to '%d'\n", key, val);
#       endif
        temp_prefs_set_int(repwin->temp_prefs, key, val);
    }
    else { /* value has not changed -- remove key from temp prefs (in
     case it exists */
#       if LOCAL_DEBUG
        printf ("removing '%s'.\n", key);
#       endif
        temp_prefs_remove_key(repwin->temp_prefs, key);
    }
    update_buttons(repwin);
}

/* text in standard text entry has changed */
static void standard_itdb_entry_changed(GtkEditable *editable, RepWin *repwin) {
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repwin);

    keybase = g_object_get_data(G_OBJECT (editable), "key");
    g_return_if_fail (keybase);

    key = get_itdb_prefs_key(repwin->itdb_index, keybase);

    finish_editable_storage(repwin, key, editable);
}

/* text for manual_syncdir has changed */
static void manual_syncdir_changed(GtkEditable *editable, RepWin *repwin) {
    gchar *key;
    gchar changed;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_MANUAL_SYNCDIR);

    changed = finish_editable_storage(repwin, key, editable);

    if (changed) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (SYNC_PLAYLIST_MODE_MANUAL_RADIO)), TRUE);
    }
}

/* sync_playlist_mode_none was toggled */
static void sync_playlist_mode_none_toggled(GtkToggleButton *togglebutton, RepWin *repwin) {
    gchar *key;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_SYNCMODE);

    if (gtk_toggle_button_get_active(togglebutton)) {
        finish_int_storage(repwin, key, SYNC_PLAYLIST_MODE_NONE);
        update_buttons(repwin);
    }

    g_free(key);
}

/* sync_playlist_mode_none was toggled */
static void sync_playlist_mode_manual_toggled(GtkToggleButton *togglebutton, RepWin *repwin) {
    gchar *key;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_SYNCMODE);

    if (gtk_toggle_button_get_active(togglebutton)) {
        finish_int_storage(repwin, key, SYNC_PLAYLIST_MODE_MANUAL);
        update_buttons(repwin);
    }

    g_free(key);
}

/* sync_playlist_mode_none was toggled */
static void sync_playlist_mode_automatic_toggled(GtkToggleButton *togglebutton, RepWin *repwin) {
    gchar *key;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_SYNCMODE);

    if (gtk_toggle_button_get_active(togglebutton)) {
        finish_int_storage(repwin, key, SYNC_PLAYLIST_MODE_AUTOMATIC);
        update_buttons(repwin);
    }

    g_free(key);
}

static void standard_itdb_checkbutton_toggled(GtkToggleButton *togglebutton, RepWin *repwin) {
    const gchar *keybase;
    gchar *key;

    g_return_if_fail (repwin);

    keybase = g_object_get_data(G_OBJECT (togglebutton), "key");
    g_return_if_fail (keybase);
    key = get_itdb_prefs_key(repwin->itdb_index, keybase);
    finish_int_storage(repwin, key, gtk_toggle_button_get_active(togglebutton));

    g_free(key);
}

static void standard_playlist_checkbutton_toggled(GtkToggleButton *togglebutton, RepWin *repwin) {
    const gchar *keybase;
    gboolean active;
    gchar *key;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->playlist);

    keybase = g_object_get_data(G_OBJECT (togglebutton), "key");
    g_return_if_fail (keybase);
    key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, keybase);
    active = gtk_toggle_button_get_active(togglebutton);

    /* Check if this is the liveupdate toggle which needs special
     * treatment. */
    if (keybase == KEY_LIVEUPDATE) {
        if (active == repwin->playlist->splpref.liveupdate)
            temp_prefs_remove_key(repwin->extra_prefs, key);
        else
            temp_prefs_set_int(repwin->extra_prefs, key, active);

        update_buttons(repwin);
        g_free(key);
        return;
    }

    finish_int_storage(repwin, key, active);
    g_free(key);
}

/* delete_repository_button was clicked */
static void delete_repository_button_clicked(GtkButton *button, RepWin *repwin) {
    Playlist *mpl;
    gchar *message;
    gchar *key;
    gint response;

    g_return_if_fail (repwin);
    mpl = itdb_playlist_mpl(repwin->itdb);
    message
            = g_strdup_printf(_("Are you sure you want to delete repository \"%s\"? This action cannot be undone!"), mpl->name);

    response = gtkpod_confirmation_simple(GTK_MESSAGE_WARNING, _("Delete repository?"), message, GTK_STOCK_DELETE);

    g_free(message);

    if (response == GTK_RESPONSE_CANCEL)
        return;

    key = get_itdb_prefs_key(repwin->itdb_index, "deleted");

    temp_prefs_set_int(repwin->extra_prefs, key, TRUE);
    g_free(key);
    update_buttons(repwin);
}

/* mountpoint browse button was clicked */
static void mountpoint_button_clicked(GtkButton *button, RepWin *repwin) {
    gchar *key, *old_dir, *new_dir;

    g_return_if_fail (repwin);

    key = get_itdb_prefs_key(repwin->itdb_index, KEY_MOUNTPOINT);
    old_dir = get_current_prefs_string(repwin, key);
    g_free(key);

    new_dir = fileselection_get_file_or_dir(_("Select mountpoint"), old_dir, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    g_free(old_dir);

    if (new_dir) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (MOUNTPOINT_ENTRY)), new_dir);
        g_free(new_dir);
    }
}

/* mountpoint browse button was clicked */
static void backup_button_clicked(GtkButton *button, RepWin *repwin) {
    gchar *key, *old_backup, *new_backup;

    g_return_if_fail (repwin);

    key = get_itdb_prefs_key(repwin->itdb_index, KEY_FILENAME);
    old_backup = get_current_prefs_string(repwin, key);
    g_free(key);

    new_backup = fileselection_get_file_or_dir(_("Set backup file"), old_backup, GTK_FILE_CHOOSER_ACTION_SAVE);

    g_free(old_backup);

    if (new_backup) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (BACKUP_ENTRY)), new_backup);
        g_free(new_backup);
    }
}

/* mountpoint browse button was clicked */
static void new_repository_button_clicked(GtkButton *button, RepWin *repwin) {
    g_return_if_fail (repwin);

    create_repository(repwin);
}

/* mountpoint browse button was clicked */
static void manual_syncdir_button_clicked(GtkButton *button, RepWin *repwin) {
    gchar *key, *old_dir, *new_dir;

    g_return_if_fail (repwin);

    key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_MANUAL_SYNCDIR);

    old_dir = get_current_prefs_string(repwin, key);

    new_dir
            = fileselection_get_file_or_dir(_("Select directory for synchronization"), old_dir, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    if (new_dir) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (MANUAL_SYNCDIR_ENTRY)), new_dir);
        g_free(new_dir);
    }
    g_free(key);
}

static void ipod_sync_button_clicked(RepWin *repwin, iPodSyncType type) {
    const gchar *title;
    const gchar *entry;
    gchar *text, *key, *oldpath, *newpath;

    g_return_if_fail (repwin);

    switch (type) {
    case IPOD_SYNC_CONTACTS:
        title = _("Please select command to sync contacts");
        entry = IPOD_SYNC_CONTACTS_ENTRY;
        key = get_itdb_prefs_key(repwin->itdb_index, KEY_PATH_SYNC_CONTACTS);
        break;
    case IPOD_SYNC_CALENDAR:
        title = _("Please select command to sync calendar");
        entry = IPOD_SYNC_CALENDAR_ENTRY;
        key = get_itdb_prefs_key(repwin->itdb_index, KEY_PATH_SYNC_CALENDAR);
        break;
    case IPOD_SYNC_NOTES:
        title = _("Please select command to sync notes");
        entry = IPOD_SYNC_NOTES_ENTRY;
        key = get_itdb_prefs_key(repwin->itdb_index, KEY_PATH_SYNC_NOTES);
        break;
    default:
        g_return_if_reached ();
    }

    oldpath = prefs_get_string(key);
    g_free(key);

    text
            = g_markup_printf_escaped(_("<i>Have a look at the scripts provided in '%s'. If you write a new script or improve an existing one, please send it to jcsjcs at users.sourceforge.net for inclusion into the next release.</i>"), SCRIPTDIR);

    newpath = fileselection_select_script(oldpath, SCRIPTDIR, title, text);
    g_free(oldpath);
    g_free(text);

    if (newpath) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (entry)), newpath);
        g_free(newpath);
    }
}

/* Callback */
static void ipod_sync_contacts_button_clicked(GtkButton *button, RepWin *repwin) {
    ipod_sync_button_clicked(repwin, IPOD_SYNC_CONTACTS);
}

/* Callback */
static void ipod_sync_calendar_button_clicked(GtkButton *button, RepWin *repwin) {
    ipod_sync_button_clicked(repwin, IPOD_SYNC_CALENDAR);

}

/* Callback */
static void ipod_sync_notes_button_clicked(GtkButton *button, RepWin *repwin) {
    ipod_sync_button_clicked(repwin, IPOD_SYNC_NOTES);

}

/**
 * sync_or_update_playlist:
 *
 * Sync (normal playlist) or update (spl) @playlist (in repository
 * @itdb_index) using the currently displayed options.
 */
static void sync_or_update_playlist(RepWin *repwin, gint itdb_index, Playlist *playlist) {
    g_return_if_fail (repwin);
    g_return_if_fail (playlist);

    if (playlist->is_spl) {
        itdb_spl_update(playlist);

        if (gtkpod_get_current_playlist() == playlist) { /* redisplay */
            gtkpod_set_current_playlist(playlist);
        }

        gtkpod_statusbar_message(_("Smart playlist updated."));
    }
    else {
        gchar *key_sync_delete_tracks, *key_sync_confirm_delete;
        gchar *key_sync_show_summary, *key_manual_sync_dir, *key_syncmode;
        gchar *sync_delete_tracks_orig, *sync_confirm_delete_orig;
        gchar *sync_show_summary_orig;
        gint sync_delete_tracks_current, sync_confirm_delete_current;
        gint sync_show_summary_current;
        gchar *manual_sync_dir = NULL;
        gint value_new;

        /* create needed prefs keys */
        key_sync_delete_tracks = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNC_DELETE_TRACKS);
        key_sync_confirm_delete = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNC_CONFIRM_DELETE);
        key_sync_show_summary = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNC_SHOW_SUMMARY);
        key_manual_sync_dir = get_playlist_prefs_key(itdb_index, playlist, KEY_MANUAL_SYNCDIR);
        key_syncmode = get_playlist_prefs_key(itdb_index, playlist, KEY_SYNCMODE);

        /* retrieve original settings for prefs strings */
        sync_delete_tracks_orig = prefs_get_string(key_sync_delete_tracks);
        sync_confirm_delete_orig = prefs_get_string(key_sync_confirm_delete);
        sync_show_summary_orig = prefs_get_string(key_sync_show_summary);

        /* retrieve current settings for prefs_strings */
        sync_delete_tracks_current = get_current_prefs_int(repwin, key_sync_delete_tracks);
        sync_confirm_delete_current = get_current_prefs_int(repwin, key_sync_confirm_delete);
        sync_show_summary_current = get_current_prefs_int(repwin, key_sync_show_summary);

        /* temporarily apply current settings */
        prefs_set_int(key_sync_delete_tracks, sync_delete_tracks_current);
        prefs_set_int(key_sync_confirm_delete, sync_confirm_delete_current);
        prefs_set_int(key_sync_show_summary, sync_show_summary_current);

        /* sync directory or directories */
        switch (get_current_prefs_int(repwin, key_syncmode)) {
        case SYNC_PLAYLIST_MODE_NONE:
            break; /* should never happen */
        case SYNC_PLAYLIST_MODE_MANUAL:
            manual_sync_dir = get_current_prefs_string(repwin, key_manual_sync_dir);
            /* no break;! we continue calling sync_playlist() */
        case SYNC_PLAYLIST_MODE_AUTOMATIC:
            sync_playlist(playlist, manual_sync_dir, NULL, FALSE, key_sync_delete_tracks, 0, key_sync_confirm_delete, 0, NULL, sync_show_summary_current);
            break;
        }

        /* Update temporary prefs in case some settings were changed
         * (currently only 'key_sync_confirm_delete' can be changed
         * by sync_playlist()) */
        value_new = prefs_get_int(key_sync_confirm_delete);
        if (value_new != sync_confirm_delete_current) {
            if (playlist == repwin->playlist) { /* currently displayed --> adjust toggle button */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE)), value_new);
            }
            else { /* not currently displayed --> copy to temp_prefs */
                temp_prefs_set_int(repwin->temp_prefs, key_sync_confirm_delete, value_new);
            }
        }

        /* Copy original values back to prefs */
        prefs_set_string(key_sync_delete_tracks, sync_delete_tracks_orig);
        prefs_set_string(key_sync_confirm_delete, sync_confirm_delete_orig);
        prefs_set_string(key_sync_show_summary, sync_show_summary_orig);

        /* Free memory used by all strings */
        g_free(key_sync_delete_tracks);
        g_free(key_sync_confirm_delete);
        g_free(key_sync_show_summary);
        g_free(key_manual_sync_dir);
        g_free(sync_delete_tracks_orig);
        g_free(sync_confirm_delete_orig);
        g_free(sync_show_summary_orig);
        g_free(manual_sync_dir);
    }
}

/* Callback */
static void update_all_playlists_button_clicked(GtkButton *button, RepWin *repwin) {
    GList *gl;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    for (gl = repwin->itdb->playlists; gl; gl = gl->next) {
        Playlist *pl = gl->data;
        g_return_if_fail (pl);
        sync_or_update_playlist(repwin, repwin->itdb_index, pl);
    }
}

/* Callback */
static void update_playlist_button_clicked(GtkButton *button, RepWin *repwin) {
    g_return_if_fail (repwin);
    sync_or_update_playlist(repwin, repwin->itdb_index, repwin->playlist);
}

/* ------------------------------------------------------------
 *
 *        Callback functions (windows control)
 *
 * ------------------------------------------------------------ */

static void edit_apply_clicked(GtkButton *button, RepWin *repwin) {
    gint i, itdb_num, del_num;
    struct itdbs_head *itdbs_head;

    g_return_if_fail (repwin);

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);
    itdb_num = g_list_length(itdbs_head->itdbs);

    temp_prefs_apply(repwin->temp_prefs);

    del_num = 0;
    for (i = 0; i < itdb_num; ++i) {
        gchar *key, *subkey;
        gboolean deleted = FALSE;

        iTunesDB *itdb = g_list_nth_data(itdbs_head->itdbs, i - del_num);
        g_return_if_fail (itdb);

        subkey = get_itdb_prefs_key(i, "");

        if (temp_prefs_subkey_exists(repwin->extra_prefs, subkey)) {
            gboolean deleted;
            GList *gl;

            key = get_itdb_prefs_key(i, "deleted");
            deleted = temp_prefs_get_int(repwin->extra_prefs, key);
            g_free(key);
            if (deleted) {
                /* FIXME: ask if serious, then delete */
                if (TRUE) {
                    iTunesDB *itdb;
                    gint j;

                    /* flush all keys relating to the deleted itdb */
                    key = get_itdb_prefs_key(i - del_num, "");
                    prefs_flush_subkey(key);
                    g_free(key);

                    for (j = i - del_num; j < itdb_num - del_num - 1; ++j) {
                        gchar *from_key = get_itdb_prefs_key(j + 1, "");
                        gchar *to_key = get_itdb_prefs_key(j, "");
                        prefs_rename_subkey(from_key, to_key);
                        g_free(from_key);
                        g_free(to_key);
                    }

                    itdb = g_list_nth_data(itdbs_head->itdbs, i - del_num);
                    gp_itdb_remove(itdb);
                    gp_itdb_free(itdb);

                    /* keep itdb_index of currently displayed repository
                     updated in case we need to select a new one */
                    if (repwin->itdb_index > i - del_num) {
                        --repwin->itdb_index;
                    }
                    ++del_num;
                }
                else {
                    deleted = FALSE;
                }
            }

            if (!deleted) {
                /* apply the "live update flag", which is kept inside
                 the playlist struct */
                for (gl = itdb->playlists; gl; gl = gl->next) {
                    Playlist *pl = gl->data;
                    gint val;
                    g_return_if_fail (pl);
                    key = get_playlist_prefs_key(i, pl, KEY_LIVEUPDATE);
                    if (temp_prefs_get_int_value(repwin->extra_prefs, key, &val)) {
                        pl->splpref.liveupdate = val;
                        data_changed(itdb);
                    }
                    g_free(key);
                }
            }
        }

        if (!deleted && temp_prefs_subkey_exists(repwin->temp_prefs, subkey)) {
            gchar *str;
            /* check if mountpoint has changed */
            key = get_itdb_prefs_key(i, KEY_MOUNTPOINT);
            str = temp_prefs_get_string(repwin->temp_prefs, key);
            g_free(key);
            if (str) { /* have to set mountpoint */
                itdb_set_mountpoint(itdb, str);
                g_warning("TODO: edit_apply_clicked - space_set_ipod_itdb");
                //                space_set_ipod_itdb(itdb);
                g_free(str);
            }

            /* check if model_number has changed */
            key = get_itdb_prefs_key(i, KEY_IPOD_MODEL);
            str = temp_prefs_get_string(repwin->temp_prefs, key);
            g_free(key);
            if (str) { /* set model */
                if (itdb->usertype && GP_ITDB_TYPE_IPOD) {
                    itdb_device_set_sysinfo(itdb->device, "ModelNumStr", str);
                }
                g_free(str);
            }

            /* this repository was changed */
            data_changed(itdb);
        }
        g_free(subkey);
    }

    temp_prefs_destroy(repwin->temp_prefs);
    temp_prefs_destroy(repwin->extra_prefs);

    repwin->temp_prefs = temp_prefs_create();
    repwin->extra_prefs = temp_prefs_create();

    if (g_list_length(itdbs_head->itdbs) < itdb_num) { /* at least one repository has been removed */
        iTunesDB *new_itdb = g_list_nth_data(itdbs_head->itdbs, repwin->itdb_index);
        iTunesDB *old_itdb = repwin->itdb;
        Playlist *old_playlist = repwin->playlist;

        init_repository_combo(repwin);
        if (new_itdb == old_itdb) {
            select_repository(repwin, new_itdb, old_playlist);
        }
        else {
            select_repository(repwin, new_itdb, NULL);
        }
    }
#   if LOCAL_DEBUG
    printf ("index: %d\n", repwin->itdb_index);
#   endif

    update_buttons(repwin);
}

static void edit_cancel_clicked(GtkButton *button, RepWin *repwin) {
    g_return_if_fail (repwin);

    repwin_free(repwin);
}

static void edit_ok_clicked(GtkButton *button, RepWin *repwin) {
    g_return_if_fail (repwin);

    edit_apply_clicked(NULL, repwin);
    edit_cancel_clicked(NULL, repwin);
}

/* Used by select_playlist() to find the new playlist. If found,
 select it */
static gboolean select_playlist_find(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist;
    RepWin *repwin = data;
    g_return_val_if_fail (repwin, TRUE);

    gtk_tree_model_get(model, iter, 0, &playlist, -1);
    if (playlist == repwin->next_playlist) {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
                        PLAYLIST_COMBO)), iter);
        return TRUE;
    }
    return FALSE;
}

/* Select @playlist

 If @playlist == NULL, select first playlist (MPL);
 */
static void select_playlist(RepWin *repwin, Playlist *playlist) {
    GtkTreeModel *model;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    if (!playlist)
        playlist = itdb_playlist_mpl(repwin->itdb);
    g_return_if_fail (playlist);

    g_return_if_fail (playlist->itdb == repwin->itdb);

    model = gtk_combo_box_get_model(GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
                    PLAYLIST_COMBO)));
    g_return_if_fail (model);

    repwin->next_playlist = playlist;
    gtk_tree_model_foreach(model, select_playlist_find, repwin);
    repwin->next_playlist = NULL;
}

/* Select @itdb and playlist @playlist

 If @itdb == NULL, only change to @playlist.

 If @playlist == NULL, select first playlist.
 */
static void select_repository(RepWin *repwin, iTunesDB *itdb, Playlist *playlist) {
    g_return_if_fail (repwin);

    if (itdb) {
        gint index;

        repwin->next_playlist = playlist;

        index = get_itdb_index(itdb);

        gtk_combo_box_set_active(GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
                        REPOSITORY_COMBO)), index);
        /* The combo callback will set the playlist */
    }
    else {
        if (repwin->itdb)
            select_playlist(repwin, playlist);
    }
}

/* set @entry with value of @key */
static void set_entry_index(RepWin *repwin, gint itdb_index, const gchar *subkey, const gchar *entry) {
    gchar *buf;
    gchar *key;

    g_return_if_fail (repwin && subkey && entry);

    key = get_itdb_prefs_key(itdb_index, subkey);

    buf = get_current_prefs_string(repwin, key);
    if (buf) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (entry)), buf);
    }
    else {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (entry)), "");
    }
    g_free(buf);
    g_free(key);
}

/****** Fill in info about selected repository *****/
static void display_repository_info(RepWin *repwin) {
    iTunesDB *itdb;
    gint index;
    gchar *buf, *key;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    itdb = repwin->itdb;
    index = repwin->itdb_index;

    /* Repository type */
    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("iPod"));
    }
    else if (itdb->usertype & GP_ITDB_TYPE_PODCASTS) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Podcasts Repository"));
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Local Repository"));
    }
    else {
        buf = g_markup_printf_escaped("<b>Unknown -- please report bug</b>");
    }
    gtk_label_set_markup(GTK_LABEL(GET_WIDGET("repository_type_label")), buf);
    g_free(buf);

    /* Hide/show corresponding widgets in table */
    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        const gchar *widgets_show[] =
            {
                MOUNTPOINT_LABEL, MOUNTPOINT_ENTRY, MOUNTPOINT_BUTTON, BACKUP_LABEL, BACKUP_ENTRY, BACKUP_BUTTON,
                IPOD_MODEL_LABEL, IPOD_MODEL_COMBO, LOCAL_PATH_ENTRY, SYNC_FRAME,
                /*	    IPOD_SYNC_LABEL,
                 IPOD_SYNC_CONTACTS_LABEL,
                 IPOD_SYNC_CONTACTS_ENTRY,
                 IPOD_SYNC_CONTACTS_BUTTON,
                 IPOD_SYNC_CALENDAR_LABEL,
                 IPOD_SYNC_CALENDAR_ENTRY,
                 IPOD_SYNC_CALENDAR_BUTTON,
                 IPOD_SYNC_NOTES_LABEL,
                 IPOD_SYNC_NOTES_ENTRY,
                 IPOD_SYNC_NOTES_BUTTON,
                 IPOD_CONCAL_AUTOSYNC_TOGGLE,*/
                NULL };
        const gchar *widgets_hide[] =
            { LOCAL_PATH_LABEL, LOCAL_PATH_ENTRY, NULL };
        const gchar **widget;

        for (widget = widgets_show; *widget; ++widget) {
            gtk_widget_show(GET_WIDGET (*widget));
        }
        for (widget = widgets_hide; *widget; ++widget) {
            gtk_widget_hide(GET_WIDGET (*widget));
        }

        set_entry_index(repwin, index, KEY_MOUNTPOINT, MOUNTPOINT_ENTRY);

        set_entry_index(repwin, index, KEY_FILENAME, BACKUP_ENTRY);

        set_entry_index(repwin, index, KEY_PATH_SYNC_CONTACTS, IPOD_SYNC_CONTACTS_ENTRY);

        set_entry_index(repwin, index, KEY_PATH_SYNC_CALENDAR, IPOD_SYNC_CALENDAR_ENTRY);

        set_entry_index(repwin, index, KEY_PATH_SYNC_NOTES, IPOD_SYNC_NOTES_ENTRY);

        set_entry_index(repwin, index, KEY_IPOD_MODEL, IPOD_MODEL_ENTRY);

        key = get_itdb_prefs_key(index, KEY_CONCAL_AUTOSYNC);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (IPOD_CONCAL_AUTOSYNC_TOGGLE)), get_current_prefs_int(repwin, key));
        g_free(key);
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        const gchar *widgets_show[] =
            { LOCAL_PATH_LABEL, LOCAL_PATH_ENTRY, NULL };
        const gchar *widgets_hide[] =
            {
                MOUNTPOINT_LABEL, MOUNTPOINT_ENTRY, MOUNTPOINT_BUTTON, BACKUP_LABEL, BACKUP_ENTRY, BACKUP_BUTTON,
                IPOD_MODEL_LABEL, IPOD_MODEL_COMBO, SYNC_FRAME,
                /*	    IPOD_SYNC_LABEL,
                 IPOD_SYNC_CONTACTS_LABEL,
                 IPOD_SYNC_CONTACTS_ENTRY,
                 IPOD_SYNC_CONTACTS_BUTTON,
                 IPOD_SYNC_CALENDAR_LABEL,
                 IPOD_SYNC_CALENDAR_ENTRY,
                 IPOD_SYNC_CALENDAR_BUTTON,
                 IPOD_SYNC_NOTES_LABEL,
                 IPOD_SYNC_NOTES_ENTRY,
                 IPOD_SYNC_NOTES_BUTTON,
                 IPOD_CONCAL_AUTOSYNC_TOGGLE,*/
                NULL };
        const gchar **widget;

        for (widget = widgets_show; *widget; ++widget) {
            gtk_widget_show(GET_WIDGET (*widget));
        }
        for (widget = widgets_hide; *widget; ++widget) {
            gtk_widget_hide(GET_WIDGET (*widget));
        }

        set_entry_index(repwin, index, KEY_FILENAME, LOCAL_PATH_ENTRY);
    }
    else {
        g_return_if_reached ();
    }
}

/****** Fill in info about selected playlist *****/
static void display_playlist_info(RepWin *repwin) {
    gchar *buf, *key;
    Playlist *playlist;
    iTunesDB *itdb;
    gint i, index;
    const gchar *widget_names[] =
        {
            PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE, PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE, PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE,
            NULL };
    const gchar *key_names[] =
        { KEY_SYNC_DELETE_TRACKS, KEY_SYNC_CONFIRM_DELETE, KEY_SYNC_SHOW_SUMMARY, NULL };

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);
    g_return_if_fail (repwin->playlist);

    /* for convenience */
    itdb = repwin->itdb;
    index = repwin->itdb_index;
    playlist = repwin->playlist;

    /* Playlist type */
    if (itdb_playlist_is_mpl(playlist)) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Master Playlist"));
    }
    else if (itdb_playlist_is_podcasts(playlist)) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Podcasts Playlist"));
    }
    else if (playlist->is_spl) {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Smart Playlist"));
    }
    else {
        buf = g_markup_printf_escaped("<i>%s</i>", _("Regular Playlist"));
    }
    gtk_label_set_markup(GTK_LABEL(GET_WIDGET("playlist_type_label")), buf);
    g_free(buf);

    /* Hide/show corresponding widgets in table */
    if (playlist->is_spl) {
        gint liveupdate;

        gtk_widget_show(GET_WIDGET (SPL_VBOX));
        gtk_widget_show(GET_WIDGET (PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE));
        gtk_widget_hide(GET_WIDGET (STANDARD_PLAYLIST_VBOX));

        key = get_playlist_prefs_key(index, playlist, KEY_LIVEUPDATE);
        if (!temp_prefs_get_int_value(repwin->extra_prefs, key, &liveupdate))
            liveupdate = playlist->splpref.liveupdate;
        g_free(key);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (SPL_LIVE_UPDATE_TOGGLE)), liveupdate);
    }
    else {
        gint syncmode;
        gchar *dir;
        gtk_widget_show(GET_WIDGET (STANDARD_PLAYLIST_VBOX));
        gtk_widget_hide(GET_WIDGET (SPL_VBOX));

        key = get_playlist_prefs_key(index, playlist, KEY_SYNCMODE);
        syncmode = get_current_prefs_int(repwin, key);
        g_free(key);

        /* Need to set manual_syncdir_entry here as it may set the
         syncmode to 'MANUAL' -- this will be corrected by setting
         the radio button with the original syncmode setting further
         down. */
        key = get_playlist_prefs_key(index, playlist, KEY_MANUAL_SYNCDIR);
        dir = get_current_prefs_string(repwin, key);
        g_free(key);
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (MANUAL_SYNCDIR_ENTRY)), dir);
        g_free(dir);

        switch (syncmode) {
        case SYNC_PLAYLIST_MODE_NONE:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (SYNC_PLAYLIST_MODE_NONE_RADIO)), TRUE);
            break;
        case SYNC_PLAYLIST_MODE_MANUAL:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (SYNC_PLAYLIST_MODE_MANUAL_RADIO)), TRUE);
            break;
        case SYNC_PLAYLIST_MODE_AUTOMATIC:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (SYNC_PLAYLIST_MODE_AUTOMATIC_RADIO)), TRUE);
            break;
        default:
            /* repair broken prefs */
            prefs_set_int(key, SYNC_PLAYLIST_MODE_NONE);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (SYNC_PLAYLIST_MODE_NONE_RADIO)), TRUE);
            break;
        }
        /* make options available where appropriate */
        gtk_widget_set_sensitive(GET_WIDGET (SYNC_OPTIONS_HBOX), syncmode != SYNC_PLAYLIST_MODE_NONE);
        /* set standard toggle buttons */
        for (i = 0; widget_names[i]; ++i) {
            key = get_playlist_prefs_key(index, playlist, key_names[i]);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (GET_WIDGET (widget_names[i])), get_current_prefs_int(repwin, key));
            if (key_names[i] == KEY_SYNC_DELETE_TRACKS) {
                gtk_widget_set_sensitive(GET_WIDGET (PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE), get_current_prefs_int(repwin, key));
            }
            g_free(key);
        }
    }
}

/****** New repository was selected */
static void repository_changed(GtkComboBox *cb, RepWin *repwin) {
    struct itdbs_head *itdbs_head;
    iTunesDB *itdb;
    gint index;

#   if LOCAL_DEBUG
    printf ("Repository changed (%p)\n", repwin);
#   endif

    g_return_if_fail (repwin);

    index = gtk_combo_box_get_active(cb);

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);

    itdb = g_list_nth_data(itdbs_head->itdbs, index);

    if (repwin->itdb != itdb) {
        repwin->itdb = itdb;
        repwin->itdb_index = index;
        display_repository_info(repwin);
        init_playlist_combo(repwin);
        update_buttons(repwin);
    }
}

/****** New playlist was selected */
static void playlist_changed(GtkComboBox *cb, RepWin *repwin) {
    GtkTreeModel *model;
    Playlist *playlist;
    GtkTreeIter iter;
    gint index;

#   if LOCAL_DEBUG
    printf ("Playlist changed (%p)\n", repwin);
#   endif

    g_return_if_fail (repwin);

    index = gtk_combo_box_get_active(cb);
    /* We can't just use the index to find the right playlist in
     itdb->playlists because they might have been reordered. Instead
     use the playlist pointer stored in the model. */
    model = gtk_combo_box_get_model(cb);
    g_return_if_fail (model);
    g_return_if_fail (gtk_tree_model_iter_nth_child (model, &iter,
                    NULL, index));
    gtk_tree_model_get(model, &iter, 0, &playlist, -1);

    if (repwin->playlist != playlist) {
        g_return_if_fail (playlist->itdb == repwin->itdb);
        repwin->playlist = playlist;
        display_playlist_info(repwin);
    }
}

/**
 * pm_set_playlist_renderer_pix
 *
 * Set the appropriate playlist icon.
 *
 * @renderer: renderer to be set
 * @playlist: playlist to consider.
 */
static void set_playlist_renderer_pix(GtkCellRenderer *renderer, Playlist *playlist) {
    const gchar *stock_id = NULL;

    g_return_if_fail (renderer);

    stock_id = return_playlist_stock_image(playlist);
    if (!stock_id)
        return;

    g_object_set(G_OBJECT (renderer), "stock-id", stock_id, NULL);
    g_object_set(G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
}

/* Provide graphic indicator in playlist combobox */
static void playlist_cb_cell_data_func_pix(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist;

    g_return_if_fail (cell);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get(model, iter, 0, &playlist, -1);

    set_playlist_renderer_pix(cell, playlist);
}

/**
 * pm_set_renderer_text
 *
 * Set the playlist name in appropriate style.
 *
 * @renderer: renderer to be set
 * @playlist: playlist to consider.
 */
static void set_playlist_renderer_text(GtkCellRenderer *renderer, Playlist *playlist) {
    ExtraiTunesDBData *eitdb;

    g_return_if_fail (playlist);
    g_return_if_fail (playlist->itdb);
    eitdb = playlist->itdb->userdata;
    g_return_if_fail (eitdb);

    if (itdb_playlist_is_mpl(playlist)) { /* mark MPL */
        g_object_set(G_OBJECT (renderer), "text", playlist->name, "weight", PANGO_WEIGHT_BOLD, NULL);
        if (eitdb->data_changed) {
            g_object_set(G_OBJECT (renderer), "style", PANGO_STYLE_ITALIC, NULL);
        }
        else {
            g_object_set(G_OBJECT (renderer), "style", PANGO_STYLE_NORMAL, NULL);
        }
    }
    else {
        if (itdb_playlist_is_podcasts(playlist)) {
            g_object_set(G_OBJECT (renderer), "text", playlist->name, "weight", PANGO_WEIGHT_SEMIBOLD, "style", PANGO_STYLE_ITALIC, NULL);
        }
        else {
            g_object_set(G_OBJECT (renderer), "text", playlist->name, "weight", PANGO_WEIGHT_NORMAL, "style", PANGO_STYLE_NORMAL, NULL);
        }
    }
}

/* Provide playlist name in combobox */
static void playlist_cb_cell_data_func_text(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    Playlist *playlist;

    g_return_if_fail (cell);
    g_return_if_fail (model);
    g_return_if_fail (iter);

    gtk_tree_model_get(model, iter, 0, &playlist, -1);

    set_playlist_renderer_text(cell, playlist);
}

/****** common between init_repository_combo() and create_repository() */
static void set_repository_combo(GtkComboBox *cb) {
    struct itdbs_head *itdbs_head;
    GtkCellRenderer *cell;
    GtkListStore *store;
    GList *gl;

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);

    if (g_object_get_data(G_OBJECT (cb), "combo_set") == NULL) { /* the combo has not yet been initialized */

        /* Cell for graphic indicator */
        cell = gtk_cell_renderer_pixbuf_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (cb), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (cb), cell, playlist_cb_cell_data_func_pix, NULL, NULL);
        /* Cell for playlist name */
        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (cb), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (cb), cell, playlist_cb_cell_data_func_text, NULL, NULL);

        g_object_set(G_OBJECT (cell), "editable", FALSE, NULL);
    }

    store = gtk_list_store_new(1, G_TYPE_POINTER);

    for (gl = itdbs_head->itdbs; gl; gl = gl->next) {
        GtkTreeIter iter;
        Playlist *mpl;
        iTunesDB *itdb = gl->data;
        g_return_if_fail (itdb);

        mpl = itdb_playlist_mpl(itdb);
        g_return_if_fail (mpl);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, mpl, -1);
    }

    gtk_combo_box_set_model(cb, GTK_TREE_MODEL (store));
    g_object_unref(store);
}

/****** Initialize the repository combo *****/
static void init_repository_combo(RepWin *repwin) {
    GtkComboBox *cb;

    g_return_if_fail (repwin);

    cb = GTK_COMBO_BOX (GET_WIDGET (REPOSITORY_COMBO));

    set_repository_combo(cb);

    if (g_object_get_data(G_OBJECT (cb), "combo_set") == NULL) { /* the combo has not yet been initialized */
        g_signal_connect (cb, "changed",
                G_CALLBACK (repository_changed), repwin);

        g_object_set_data(G_OBJECT (cb), "combo_set", "set");
    }

    repwin->itdb = NULL;
    repwin->playlist = NULL;
}

/****** Initialize the playlist combo *****/
static void init_playlist_combo(RepWin *repwin) {
    GtkCellRenderer *cell;
    GtkListStore *store;
    GtkComboBox *cb;
    GList *gl;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->itdb);

    cb = GTK_COMBO_BOX (gtkpod_xml_get_widget (repwin->xml,
                    PLAYLIST_COMBO));

    if (g_object_get_data(G_OBJECT (cb), "combo_set") == NULL) {
        /* Cell for graphic indicator */
        cell = gtk_cell_renderer_pixbuf_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (cb), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (cb), cell, playlist_cb_cell_data_func_pix, NULL, NULL);
        /* Cell for playlist name */
        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (cb), cell, FALSE);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT (cb), cell, playlist_cb_cell_data_func_text, NULL, NULL);

        g_object_set(G_OBJECT (cell), "editable", FALSE, NULL);

        g_signal_connect (cb, "changed",
                G_CALLBACK (playlist_changed), repwin);

        g_object_set_data(G_OBJECT (cb), "combo_set", "set");
    }

    store = gtk_list_store_new(1, G_TYPE_POINTER);

    if (repwin->itdb) {
        for (gl = repwin->itdb->playlists; gl; gl = gl->next) {
            GtkTreeIter iter;
            Playlist *pl = gl->data;
            g_return_if_fail (pl);

            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, pl, -1);
        }
    }

    gtk_combo_box_set_model(cb, GTK_TREE_MODEL (store));
    g_object_unref(store);

    if (repwin->itdb) {
        select_playlist(repwin, repwin->next_playlist);
        repwin->next_playlist = NULL;
    }

}

/* Render apply insensitive when no changes were made.
 When an itdb is marked for deletion, make entries insensitive */
static void update_buttons(RepWin *repwin) {
    gboolean apply, ok, deleted;
    gchar *key;

    g_return_if_fail (repwin);
    g_return_if_fail (repwin->temp_prefs);
    g_return_if_fail (repwin->extra_prefs);

    if ((temp_prefs_size(repwin->temp_prefs) > 0) || (temp_prefs_size(repwin->extra_prefs) > 0)) {
        apply = TRUE;
        ok = TRUE;
    }
    else {
        apply = FALSE;
        ok = TRUE;
    }

    gtk_widget_set_sensitive(GET_WIDGET ("apply_button"), apply);
    gtk_widget_set_sensitive(GET_WIDGET ("ok_button"), ok);

    if (repwin->itdb) {
        gtk_widget_set_sensitive(GET_WIDGET (REPOSITORY_VBOX), TRUE);

        /* Check if this itdb is marked for deletion */
        key = get_itdb_prefs_key(repwin->itdb_index, "deleted");
        deleted = temp_prefs_get_int(repwin->extra_prefs, key);
        g_free(key);

        gtk_widget_set_sensitive(GET_WIDGET ("general_frame"), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET (SYNC_FRAME), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET ("update_all_playlists_button"), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET ("playlist_tab_label"), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET ("playlist_tab_contents"), !deleted);
        gtk_widget_set_sensitive(GET_WIDGET ("delete_repository_button"), !deleted);

        if (repwin->playlist) {
            gboolean sens = FALSE;
            if (repwin->playlist->is_spl) {
                sens = TRUE;
            }
            else {
                gint val;
                key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_SYNCMODE);
                val = get_current_prefs_int(repwin, key);
                g_free(key);
                if (val != SYNC_PLAYLIST_MODE_NONE) {
                    sens = TRUE;
                }
                gtk_widget_set_sensitive(GET_WIDGET (SYNC_OPTIONS_HBOX), sens);

                key = get_playlist_prefs_key(repwin->itdb_index, repwin->playlist, KEY_SYNC_DELETE_TRACKS);
                val = get_current_prefs_int(repwin, key);
                g_free(key);
                gtk_widget_set_sensitive(GET_WIDGET (PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE), val);
            }
            gtk_widget_set_sensitive(GET_WIDGET (UPDATE_PLAYLIST_BUTTON), sens);
        }
    }
    else { /* no itdb loaded */
        gtk_widget_set_sensitive(GET_WIDGET (REPOSITORY_VBOX), FALSE);
    }
}

/* Free memory taken by @repwin */
static void repwin_free(RepWin *repwin) {
    g_return_if_fail (repwin);

    g_object_unref(repwin->xml);

    if (repwin->window) {
        gtk_widget_destroy(repwin->window);
    }

    temp_prefs_destroy(repwin->temp_prefs);
    temp_prefs_destroy(repwin->extra_prefs);
    g_free(repwin);
}

/**
 * repository_edit_itdb_added:
 *
 * Notify the dialog that a new itdb was added to the display.
 *
 * @itdb: newly added itdb
 * @select: select the new itdb if TRUE.
 */
static void repository_edit_itdb_added(iTunesDB *itdb, gboolean select) {
//    struct itdbs_head *itdbs_head;
//    gint index, num;
//    GList *gl;

    g_warning("TODO: repository_edit_itdb_added");
//    g_return_if_fail (itdb);
//    itdbs_head = gp_get_itdbs_head();
//    g_return_if_fail (itdbs_head);
//
//    index = g_list_index(itdbs_head->itdbs, itdb);
//    g_return_if_fail (index != -1);
//    num = g_list_length(itdbs_head->itdbs);
//
//    /* Cycle through all open windows */
//    for (gl = repwins; gl; gl = gl->next) {
//        gint i;
//        iTunesDB *old_itdb;
//        Playlist *old_playlist;
//        RepWin *repwin = gl->data;
//        g_return_if_fail (repwin);
//
//        /* rename keys */
//        for (i = num - 2; i >= index; --i) {
//            gchar *from_key = get_itdb_prefs_key(i, "");
//            gchar *to_key = get_itdb_prefs_key(i + 1, "");
//#           if LOCAL_DEBUG
//            printf ("TP renaming %d to %d\n", i, i+1);
//#           endif
//            temp_prefs_rename_subkey(repwin->temp_prefs, from_key, to_key);
//            g_free(from_key);
//            g_free(to_key);
//        }
//
//        old_itdb = repwin->itdb;
//        old_playlist = repwin->playlist;
//
//        init_repository_combo(repwin);
//
//        if (select) {
//            select_repository(repwin, itdb, NULL);
//        }
//        else {
//            select_repository(repwin, old_itdb, old_playlist);
//        }
//    }
}

void destroy_repository_editor() {
    if (! repository_window)
        return;

    repository_window->window = NULL;
    g_free(repository_window);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget(ANJUTA_PLUGIN(repository_editor_plugin)->shell, repository_editor_plugin->repo_window, NULL);
}

static void create_repository_edit_view() {
    GtkWidget *repo_window;
    GtkWidget *viewport;
    GtkComboBox *cb;
    gint i;

    repository_window = g_malloc0(sizeof(RepWin));

    repository_window->xml = gtkpod_xml_new(GLADE_FILE, "repository_window");
    repo_window = gtkpod_xml_get_widget(repository_window->xml, "repository_window");
    viewport = gtkpod_xml_get_widget(repository_window->xml, "repository_viewport");

    /* according to GTK FAQ: move a widget to a new parent */
    gtk_widget_ref(viewport);
    gtk_container_remove(GTK_CONTAINER (repo_window), viewport);

    /* Add widget in Shell. Any number of widgets can be added */
    repository_editor_plugin->repo_window = gtk_scrolled_window_new(NULL, NULL);
    repository_editor_plugin->repo_view = viewport;
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (repository_editor_plugin->repo_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (repository_editor_plugin->repo_window), GTK_SHADOW_IN);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(repository_editor_plugin->repo_window), GTK_WIDGET (repository_editor_plugin->repo_view));
    anjuta_shell_add_widget(ANJUTA_PLUGIN(repository_editor_plugin)->shell, repository_editor_plugin->repo_window, "RepositoryEditorPlugin", "Edit iPod Repositories", NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    repository_window->window = repository_editor_plugin->repo_window;

    /* widget names and corresponding keys for 'standard' toggle
     options */
    const gchar *playlist_widget_names_toggle[] =
        {
            PLAYLIST_SYNC_DELETE_TRACKS_TOGGLE, PLAYLIST_SYNC_CONFIRM_DELETE_TOGGLE, PLAYLIST_SYNC_SHOW_SUMMARY_TOGGLE,
            SPL_LIVE_UPDATE_TOGGLE, NULL };
    const gchar *playlist_key_names_toggle[] =
        { KEY_SYNC_DELETE_TRACKS, KEY_SYNC_CONFIRM_DELETE, KEY_SYNC_SHOW_SUMMARY, KEY_LIVEUPDATE, NULL };
    const gchar *itdb_widget_names_toggle[] =
        { IPOD_CONCAL_AUTOSYNC_TOGGLE, NULL };
    const gchar *itdb_key_names_toggle[] =
        { KEY_CONCAL_AUTOSYNC, NULL };
    /* widget names and corresponding keys for 'standard' strings */
    const gchar *itdb_widget_names_entry[] =
        {
            MOUNTPOINT_ENTRY, BACKUP_ENTRY, IPOD_MODEL_ENTRY, LOCAL_PATH_ENTRY, IPOD_SYNC_CONTACTS_ENTRY,
            IPOD_SYNC_CALENDAR_ENTRY, IPOD_SYNC_NOTES_ENTRY, NULL };
    const gchar *itdb_key_names_entry[] =
        {
            KEY_MOUNTPOINT, KEY_BACKUP, KEY_IPOD_MODEL, KEY_FILENAME, KEY_PATH_SYNC_CONTACTS, KEY_PATH_SYNC_CALENDAR,
            KEY_PATH_SYNC_NOTES, NULL };

    /* Setup model number combo */
    cb = GTK_COMBO_BOX (GET_WIDGET (IPOD_MODEL_COMBO));
    gp_init_model_number_combo(cb);

    /* Window control */
    g_signal_connect (GET_WIDGET ("apply_button"), "clicked",
            G_CALLBACK (edit_apply_clicked), repository_window);

    g_signal_connect (GET_WIDGET ("cancel_button"), "clicked",
            G_CALLBACK (edit_cancel_clicked), repository_window);

    g_signal_connect (GET_WIDGET ("ok_button"), "clicked",
            G_CALLBACK (edit_ok_clicked), repository_window);

    /* Entry callbacks */
    g_signal_connect (GET_WIDGET (MANUAL_SYNCDIR_ENTRY), "changed",
            G_CALLBACK (manual_syncdir_changed), repository_window);
    /* connect standard text entries */
    for (i = 0; itdb_widget_names_entry[i]; ++i) {
        GtkWidget *w = GET_WIDGET (itdb_widget_names_entry[i]);

        g_signal_connect (w, "changed",
                G_CALLBACK (standard_itdb_entry_changed),
                repository_window);
        g_object_set_data(G_OBJECT (w), "key", (gpointer) itdb_key_names_entry[i]);
    }

    /* Togglebutton callbacks */
    g_signal_connect (GET_WIDGET (SYNC_PLAYLIST_MODE_NONE_RADIO),
            "toggled",
            G_CALLBACK (sync_playlist_mode_none_toggled),
            repository_window);

    g_signal_connect (GET_WIDGET (SYNC_PLAYLIST_MODE_MANUAL_RADIO),
            "toggled",
            G_CALLBACK (sync_playlist_mode_manual_toggled),
            repository_window);

    g_signal_connect (GET_WIDGET (SYNC_PLAYLIST_MODE_AUTOMATIC_RADIO),
            "toggled",
            G_CALLBACK (sync_playlist_mode_automatic_toggled),
            repository_window);

    /* connect standard toggle buttons */
    for (i = 0; playlist_widget_names_toggle[i]; ++i) {
        GtkWidget *w = GET_WIDGET (playlist_widget_names_toggle[i]);

        g_signal_connect (w, "toggled",
                G_CALLBACK (standard_playlist_checkbutton_toggled),
                repository_window);
        g_object_set_data(G_OBJECT (w), "key", (gpointer) playlist_key_names_toggle[i]);
    }
    for (i = 0; itdb_widget_names_toggle[i]; ++i) {
        GtkWidget *w = GET_WIDGET (itdb_widget_names_toggle[i]);

        g_signal_connect (w, "toggled",
                G_CALLBACK (standard_itdb_checkbutton_toggled),
                repository_window);
        g_object_set_data(G_OBJECT (w), "key", (gpointer) itdb_key_names_toggle[i]);
    }

    /* Button callbacks */
    g_signal_connect (GET_WIDGET (MOUNTPOINT_BUTTON), "clicked",
            G_CALLBACK (mountpoint_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (BACKUP_BUTTON), "clicked",
            G_CALLBACK (backup_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (MANUAL_SYNCDIR_BUTTON), "clicked",
            G_CALLBACK (manual_syncdir_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (DELETE_REPOSITORY_BUTTON), "clicked",
            G_CALLBACK (delete_repository_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (IPOD_SYNC_CONTACTS_BUTTON), "clicked",
            G_CALLBACK (ipod_sync_contacts_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (IPOD_SYNC_CALENDAR_BUTTON), "clicked",
            G_CALLBACK (ipod_sync_calendar_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (IPOD_SYNC_NOTES_BUTTON), "clicked",
            G_CALLBACK (ipod_sync_notes_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (UPDATE_PLAYLIST_BUTTON), "clicked",
            G_CALLBACK (update_playlist_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (UPDATE_ALL_PLAYLISTS_BUTTON), "clicked",
            G_CALLBACK (update_all_playlists_button_clicked), repository_window);

    g_signal_connect (GET_WIDGET (NEW_REPOSITORY_BUTTON), "clicked",
            G_CALLBACK (new_repository_button_clicked), repository_window);

    init_repository_combo(repository_window);

    /* Set up temp_prefs struct */
    repository_window->temp_prefs = temp_prefs_create();
    repository_window->extra_prefs = temp_prefs_create();
}

/**
 * repository_edit:
 *
 * Open the repository options window and display repository @itdb and
 * playlist @playlist. If @itdb and @playlist is NULL, display first
 * repository and playlist. If @itdb is NULL and @playlist is not
 * NULL, display @playlist and assume repository is playlist->itdb.
 */
void repository_edit(iTunesDB *itdb, Playlist *playlist) {
    if (! repository_editor_plugin->repo_window)
        create_repository_edit_view();

    if (!itdb && playlist)
        itdb = playlist->itdb;
    if (!itdb) {
        struct itdbs_head *itdbs_head = gp_get_itdbs_head();
        itdb = g_list_nth_data(itdbs_head->itdbs, 0);
    }
    g_return_if_fail (itdb);

    select_repository(repository_window, itdb, playlist);

    update_buttons(repository_window);

    gtk_widget_show_all(repository_window->window);
}

/* ------------------------------------------------------------
 * ************************************************************
 *
 * Create Repository Dialog
 *
 * ************************************************************
 * ------------------------------------------------------------ */

struct _CreateRep {
    GladeXML *xml; /* XML info                             */
    GtkWidget *window; /* pointer to repository window         */
};

typedef struct _CreateRep CreateRep;

static CreateRep *createrep = NULL;

/* repository types */
enum {
    REPOSITORY_TYPE_IPOD = 0, REPOSITORY_TYPE_LOCAL = 1, REPOSITORY_TYPE_PODCAST = 2,
};
/* before/after */
enum {
    INSERT_BEFORE = 0, INSERT_AFTER = 1,
};

/* shortcut to reference widgets when repwin->xml is already set */
#undef GET_WIDGET
#define GET_WIDGET(a) repository_xml_get_widget (cr->xml,a)

/* ------------------------------------------------------------
 *
 *        Callback functions (windows control)
 *
 * ------------------------------------------------------------ */

/* Free memory taken by @createrep */
static void createrep_free(CreateRep *cr) {
    g_return_if_fail (cr);

    g_object_unref(cr->xml);

    if (cr->window) {
        gtk_widget_destroy(cr->window);
    }

    g_free(cr);
}

static void create_cancel_clicked(GtkButton *button, CreateRep *cr) {
    g_return_if_fail (cr);

    createrep_free(cr);

    createrep = NULL;
}

static void create_ok_clicked(GtkButton *button, CreateRep *cr) {
    struct itdbs_head *itdbs_head;
    gint type, bef_after, itdb_index;
    const gchar *name, *mountpoint, *backup, *ipod_model, *local_path;
    iTunesDB *itdb;
    gint n, i;

    g_return_if_fail (cr);

    itdbs_head = gp_get_itdbs_head();
    g_return_if_fail (itdbs_head);
    n = g_list_length(itdbs_head->itdbs);

    /* retrieve current settings */
    type = gtk_combo_box_get_active(GTK_COMBO_BOX (GET_WIDGET (CRW_REPOSITORY_TYPE_COMBO)));

    bef_after = gtk_combo_box_get_active(GTK_COMBO_BOX (GET_WIDGET (CRW_INSERT_BEFORE_AFTER_COMBO)));

    itdb_index = gtk_combo_box_get_active(GTK_COMBO_BOX (GET_WIDGET (CRW_REPOSITORY_COMBO)));

    name = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_REPOSITORY_NAME_ENTRY)));

    mountpoint = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_MOUNTPOINT_ENTRY)));

    backup = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_BACKUP_ENTRY)));

    ipod_model = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_IPOD_MODEL_ENTRY)));
    if (strcmp(ipod_model, gettext(SELECT_OR_ENTER_YOUR_MODEL)) == 0) { /* User didn't choose a model */
        ipod_model = "";
    }

    local_path = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_LOCAL_PATH_ENTRY)));

    /* adjust position where new itdb is to be inserted */
    if (bef_after == INSERT_AFTER)
        ++itdb_index;

    /* rename pref keys */
    for (i = n - 1; i >= itdb_index; --i) {
        gchar *from_key = get_itdb_prefs_key(i, "");
        gchar *to_key = get_itdb_prefs_key(i + 1, "");
#       if LOCAL_DEBUG
        printf ("renaming %d to %d\n", i, i+1);
#       endif
        prefs_rename_subkey(from_key, to_key);
        g_free(from_key);
        g_free(to_key);
    }

    /* Setup prefs for new itdb */
    set_itdb_index_prefs_string(itdb_index, "name", name);
    switch (type) {
    case REPOSITORY_TYPE_IPOD:
        set_itdb_index_prefs_string(itdb_index, KEY_MOUNTPOINT, mountpoint);
        set_itdb_index_prefs_string(itdb_index, KEY_BACKUP, backup);
        set_itdb_index_prefs_int(itdb_index, "type", GP_ITDB_TYPE_IPOD);
        if (strlen(ipod_model) != 0)
            set_itdb_index_prefs_string(itdb_index, KEY_IPOD_MODEL, ipod_model);
        break;
    case REPOSITORY_TYPE_LOCAL:
        set_itdb_index_prefs_string(itdb_index, KEY_FILENAME, local_path);
        set_itdb_index_prefs_int(itdb_index, "type", GP_ITDB_TYPE_LOCAL);
        break;
    case REPOSITORY_TYPE_PODCAST:
        set_itdb_index_prefs_string(itdb_index, KEY_FILENAME, local_path);
        set_itdb_index_prefs_int(itdb_index, "type", GP_ITDB_TYPE_PODCASTS | GP_ITDB_TYPE_LOCAL);
        break;
    default:
        g_return_if_reached ();
    }

    /* Create new itdb */
    itdb = setup_itdb_n(itdb_index);
    g_return_if_fail (itdb);

    /* add to the display */
    gp_itdb_add(itdb, itdb_index);

    /* notify repository_edit dialog */
    repository_edit_itdb_added(itdb, TRUE);

    /* Finish */
    create_cancel_clicked(NULL, cr);
}

static void create_delete_event(GtkWidget *widget, GdkEvent *event, CreateRep *cr) {
    create_cancel_clicked(NULL, cr);
}

/* ------------------------------------------------------------
 *
 *        Callback (repository type)
 *
 * ------------------------------------------------------------ */

static void cr_repository_type_changed(GtkComboBox *cb, CreateRep *cr) {
    gint index, i;
    const gchar **show = NULL;
    /* widgets to show for iPod repositories */
    const gchar *show_ipod[] =
        {
            CRW_MOUNTPOINT_LABEL, CRW_MOUNTPOINT_ENTRY, CRW_MOUNTPOINT_BUTTON, CRW_BACKUP_LABEL, CRW_BACKUP_ENTRY,
            CRW_BACKUP_BUTTON, CRW_IPOD_MODEL_LABEL, CRW_IPOD_MODEL_COMBO, NULL };
    /* widgets to show for local repositories */
    const gchar *show_local[] =
        { CRW_LOCAL_PATH_LABEL, CRW_LOCAL_PATH_ENTRY, CRW_LOCAL_PATH_BUTTON, NULL };
    /* list of all widgets that get hidden */
    const gchar *hide_all[] =
        {
            CRW_MOUNTPOINT_LABEL, CRW_MOUNTPOINT_ENTRY, CRW_MOUNTPOINT_BUTTON, CRW_BACKUP_LABEL, CRW_BACKUP_ENTRY,
            CRW_BACKUP_BUTTON, CRW_IPOD_MODEL_LABEL, CRW_IPOD_MODEL_COMBO, CRW_LOCAL_PATH_LABEL, CRW_LOCAL_PATH_ENTRY,
            CRW_LOCAL_PATH_BUTTON, NULL };

    index = gtk_combo_box_get_active(cb);

    switch (index) {
    case REPOSITORY_TYPE_IPOD:
        show = show_ipod;
        break;
    case REPOSITORY_TYPE_LOCAL:
    case REPOSITORY_TYPE_PODCAST:
        show = show_local;
        break;
    }

    g_return_if_fail (show);

    /* Hide all widgets */
    for (i = 0; hide_all[i]; ++i) {
        gtk_widget_hide(GET_WIDGET (hide_all[i]));
    }
    /* Show appropriate widgets */
    for (i = 0; show[i]; ++i) {
        gtk_widget_show(GET_WIDGET (show[i]));
    }
}

/* ------------------------------------------------------------
 *
 *        Callback (buttons)
 *
 * ------------------------------------------------------------ */

/* mountpoint browse button was clicked */
static void cr_mountpoint_button_clicked(GtkButton *button, CreateRep *cr) {
    const gchar *old_dir;
    gchar *new_dir;

    g_return_if_fail (cr);

    old_dir = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_MOUNTPOINT_ENTRY)));

    new_dir = fileselection_get_file_or_dir(_("Select mountpoint"), old_dir, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    if (new_dir) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_MOUNTPOINT_ENTRY)), new_dir);
        g_free(new_dir);
    }
}

/* backup browse button was clicked */
static void cr_backup_button_clicked(GtkButton *button, CreateRep *cr) {
    const gchar *old_backup;
    gchar *new_backup;

    g_return_if_fail (cr);

    old_backup = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_BACKUP_ENTRY)));

    new_backup = fileselection_get_file_or_dir(_("Set backup file"), old_backup, GTK_FILE_CHOOSER_ACTION_SAVE);

    if (new_backup) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_BACKUP_ENTRY)), new_backup);
        g_free(new_backup);
    }
}

/* local path browse button was clicked */
static void cr_local_path_button_clicked(GtkButton *button, CreateRep *cr) {
    const gchar *old_path;
    gchar *new_path;

    g_return_if_fail (cr);

    old_path = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (CRW_LOCAL_PATH_ENTRY)));

    new_path = fileselection_get_file_or_dir(_("Set local repository file"), old_path, GTK_FILE_CHOOSER_ACTION_SAVE);

    if (new_path) {
        gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_LOCAL_PATH_ENTRY)), new_path);
        g_free(new_path);
    }
}

/**
 * create_repository: Create a new repository.
 *
 * @repwin: if given, @repwin will be updated.
 *
 * Note: this is a modal dialog.
 */

static void create_repository(RepWin *repwin1) {
    CreateRep *cr;
    GtkComboBox *cb;
    gchar *str, *buf1, *buf2;
    struct itdbs_head *itdbs_head = gp_get_itdbs_head();

    createrep = g_malloc0(sizeof(CreateRep));
    cr = createrep;

    cr->xml = gtkpod_xml_new(GLADE_FILE, "create_repository_window");
    /*  no signals to connect -> comment out */
    /*     glade_xml_signal_autoconnect (detail->xml); */
    cr->window = gtkpod_xml_get_widget(cr->xml, "create_repository_window");

    g_return_if_fail (cr->window);

    /* Window control */
    g_signal_connect (GET_WIDGET (CRW_CANCEL_BUTTON), "clicked",
            G_CALLBACK (create_cancel_clicked), cr);

    g_signal_connect (GET_WIDGET (CRW_OK_BUTTON), "clicked",
            G_CALLBACK (create_ok_clicked), cr);

    g_signal_connect (createrep->window, "delete_event",
            G_CALLBACK (create_delete_event), cr);

    /* Combo callback */
    g_signal_connect (GET_WIDGET (CRW_REPOSITORY_TYPE_COMBO), "changed",
            G_CALLBACK (cr_repository_type_changed), cr);

    /* Button callbacks */
    g_signal_connect (GET_WIDGET (CRW_MOUNTPOINT_BUTTON), "clicked",
            G_CALLBACK (cr_mountpoint_button_clicked), cr);

    g_signal_connect (GET_WIDGET (CRW_BACKUP_BUTTON), "clicked",
            G_CALLBACK (cr_backup_button_clicked), cr);

    g_signal_connect (GET_WIDGET (CRW_LOCAL_PATH_BUTTON), "clicked",
            G_CALLBACK (cr_local_path_button_clicked), cr);

    /* Setup model number combo */
    cb = GTK_COMBO_BOX (GET_WIDGET (CRW_IPOD_MODEL_COMBO));
    gp_init_model_number_combo(cb);
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_IPOD_MODEL_ENTRY)), gettext (SELECT_OR_ENTER_YOUR_MODEL));

    /* Set initial repository type */
    gtk_combo_box_set_active(GTK_COMBO_BOX (GET_WIDGET (CRW_REPOSITORY_TYPE_COMBO)), REPOSITORY_TYPE_IPOD);

    /* Set before/after combo */
    gtk_combo_box_set_active(GTK_COMBO_BOX (GET_WIDGET (CRW_INSERT_BEFORE_AFTER_COMBO)), INSERT_AFTER);

    /* Set up repository combo */
    set_repository_combo(GTK_COMBO_BOX (GET_WIDGET (CRW_REPOSITORY_COMBO)));
    gtk_combo_box_set_active(GTK_COMBO_BOX (GET_WIDGET (CRW_REPOSITORY_COMBO)), 0);

    /* Set default repository name */
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_REPOSITORY_NAME_ENTRY)), _("New Repository"));

    /* Set initial mountpoint */
    str = prefs_get_string("initial_mountpoint");
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_MOUNTPOINT_ENTRY)), str);
    g_free(str);

    buf1 = prefs_get_cfgdir();
    g_return_if_fail (buf1);
    /* Set initial backup path */
    buf2 = g_strdup_printf("backupDB_%d", g_list_length(itdbs_head->itdbs));
    str = g_build_filename(buf1, buf2, NULL);
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_BACKUP_ENTRY)), str);
    g_free(str);
    g_free(buf2);

    /* Set local repository file */
    buf2 = g_strdup_printf("local_%d.itdb", g_list_length(itdbs_head->itdbs));
    str = g_build_filename(buf1, buf2, NULL);
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (CRW_LOCAL_PATH_ENTRY)), str);
    g_free(str);
    g_free(buf2);
    g_free(buf1);
}
