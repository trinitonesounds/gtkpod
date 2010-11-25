/*
 |  Copyright (C) 2002-2009 Paul Richardson <phantom_sf at users sourceforge net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <glib/gi18n-lib.h>

#include "gtkpod_app_iface.h"
#include "gtkpod_app-marshallers.h"
#include "misc.h"
#include "context_menus.h"
#include "prefs.h"

static void gtkpod_app_base_init(GtkPodAppInterface* klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        klass->current_itdb = NULL;
        klass->current_playlist = NULL;
        klass->displayed_tracks = NULL;
        klass->selected_tracks = NULL;
        klass->sort_enablement = TRUE;
        klass->xml_file = NULL;
        klass->exporter = NULL;
        klass->repository_editor = NULL;
        klass->details_editor = NULL;
        klass->lyrics_editor = NULL;
        klass->photo_editor = NULL;
        klass->track_commands = NULL;

        klass->filetypes = g_hash_table_new (g_str_hash, g_str_equal);
        filetype_init_core_types(klass->filetypes);

        klass->itdb_updated = NULL;
        klass->statusbar_message = NULL;
        klass->gtkpod_warning = NULL;
        klass->gtkpod_warning_hig = NULL;
        klass->gtkpod_confirmation_hig = NULL;
        klass->gtkpod_confirmation = NULL;
        klass->statusbar_busy_push = NULL;
        klass->statusbar_busy_pop = NULL;
        klass->export_tracks_as_gchar = NULL;
        klass->export_tracks_as_glist = NULL;
        klass->display_widget = NULL;

        gtkpod_app_signals[ITDB_UPDATED]
                = g_signal_new(SIGNAL_ITDB_UPDATED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, _gtkpod_app_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

        gtkpod_app_signals[PLAYLIST_SELECTED]
                = g_signal_new(SIGNAL_PLAYLIST_SELECTED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACKS_DISPLAYED]
                = g_signal_new(SIGNAL_TRACKS_DISPLAYED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACKS_SELECTED]
                = g_signal_new(SIGNAL_TRACKS_SELECTED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACK_REMOVED]
                = g_signal_new(SIGNAL_TRACK_REMOVED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACK_UPDATED]
                = g_signal_new(SIGNAL_TRACK_UPDATED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACK_ADDED]
                = g_signal_new(SIGNAL_TRACK_ADDED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[SORT_ENABLEMENT]
                = g_signal_new(SIGNAL_SORT_ENABLEMENT, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

        gtkpod_app_signals[PLAYLIST_ADDED]
                = g_signal_new(SIGNAL_PLAYLIST_ADDED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, _gtkpod_app_marshal_VOID__POINTER_INT, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);

        gtkpod_app_signals[PLAYLIST_REMOVED]
                = g_signal_new(SIGNAL_PLAYLIST_REMOVED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[ITDB_ADDED]
                = g_signal_new(SIGNAL_ITDB_ADDED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, _gtkpod_app_marshal_VOID__POINTER_INT, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);

        gtkpod_app_signals[ITDB_REMOVED]
                = g_signal_new(SIGNAL_ITDB_REMOVED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[PREFERENCE_CHANGE]
                = g_signal_new(SIGNAL_PREFERENCE_CHANGE, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, _gtkpod_app_marshal_VOID__POINTER_INT, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);

        gtkpod_app_signals[ITDB_DATA_CHANGED]
                = g_signal_new(SIGNAL_ITDB_DATA_CHANGED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[ITDB_DATA_SAVED]
                = g_signal_new(SIGNAL_ITDB_DATA_SAVED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        initialized = TRUE;
    }
}

GType gtkpod_app_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo info =
            { sizeof(GtkPodAppInterface), (GBaseInitFunc) gtkpod_app_base_init, NULL, NULL, NULL, NULL, 0, 0, NULL };
        type = g_type_register_static(G_TYPE_INTERFACE, "GtkPodAppInterface", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
    }
    return type;
}

/* Handler to be used when the button should be displayed, but no
 action is required */
void CONF_NULL_HANDLER(gpointer d1, gpointer d2) {
    return;
}

void gtkpod_app_set_glade_xml(gchar *xml_file) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->xml_file = g_strdup(xml_file);
}

gchar* gtkpod_get_glade_xml() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->xml_file;
}

/**
 * gtkpod_shutdown
 *
 * return value: TRUE if it's OK to quit.
 */
gboolean ok_to_close_gtkpod(void) {
    gint result = GTK_RESPONSE_OK;

    if (!files_are_saved()) {
        const gchar
                *str =
                        _("Data has been changed and not been saved. If you quit gtkpod, all unsaved changes will be lost.\n\nDo you want to save your changes first?");

        result
                = gtkpod_confirmation_hig(GTK_MESSAGE_WARNING, _("Save changes before quiting?"), str, GTK_STOCK_SAVE, GTK_STOCK_CANCEL, _("Quit without saving"), NULL);
    }

    /* User pressed Cancel */
    if (result == GTK_RESPONSE_CANCEL)
        return FALSE;

    /* User pressed Save */
    if (result == GTK_RESPONSE_OK)
        handle_export();

    return TRUE;
}

/**
 * clean up bits n pieces
 */
gint gtkpod_cleanup_quit() {
    if (!widgets_blocked) {
        gtkpod_shutdown();
        return TRUE; // Already to carry on quitting
    }
    return FALSE; // dont quit!
}

void gtkpod_statusbar_message(gchar* message, ...) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    gchar* msg;
    va_list args;
    va_start (args, message);
    msg = g_strdup_vprintf(message, args);
    va_end (args);

    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->statusbar_message(gtkpod_app, msg);
    g_free(msg);
}

void gtkpod_statusbar_busy_push() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->statusbar_busy_push(gtkpod_app);
}

void gtkpod_statusbar_busy_pop() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->statusbar_busy_pop(gtkpod_app);
}

void gtkpod_tracks_statusbar_update(void) {
    gchar *buf;
    Playlist *pl;
    pl = gtkpod_get_current_playlist();
    /* select of which iTunesDB data should be displayed */
    if (pl) {
        iTunesDB *itdb = pl->itdb;
        gint trknr = g_list_length(pl->members);
        g_return_if_fail (itdb);

        buf = g_strdup_printf(_(" P:%d T:%d/%d"), itdb_playlists_number(itdb) - 1, trknr, itdb_tracks_number(itdb));
    }
    else {
        buf = g_strdup("");
    }
    gtkpod_statusbar_message(buf);
    g_free(buf);
}

void gtkpod_warning(gchar* message, ...) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    gchar* msg;
    va_list args;
    va_start (args, message);
    msg = g_strdup_vprintf(message, args);
    va_end (args);

    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_warning(gtkpod_app, msg);
    g_free(msg);
}

void gtkpod_warning_simple(const gchar *format, ...) {
    va_list arg;
    gchar *text;

    va_start (arg, format);
    text = g_strdup_vprintf(format, arg);
    va_end (arg);

    gtkpod_warning_hig(GTK_MESSAGE_WARNING, _("Warning"), text);
    g_free(text);
}

void gtkpod_warning_hig(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_warning_hig(gtkpod_app, icon, primary_text, secondary_text);
}

GtkResponseType gtkpod_confirmation(gint id, gboolean modal, const gchar *title, const gchar *label, const gchar *text, const gchar *option1_text, CONF_STATE option1_state, const gchar *option1_key, const gchar *option2_text, CONF_STATE option2_state, const gchar *option2_key, gboolean confirm_again, const gchar *confirm_again_key, ConfHandler ok_handler, ConfHandler apply_handler, ConfHandler cancel_handler, gpointer user_data1, gpointer user_data2) {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), -1);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_confirmation(gtkpod_app, id, modal, title, label, text, option1_text, option1_state, option1_key, option2_text, option2_state, option2_key, confirm_again, confirm_again_key, ok_handler, apply_handler, cancel_handler, user_data1, user_data2);
}

gint gtkpod_confirmation_simple(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text) {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), -1);
    return gtkpod_confirmation_hig(icon, primary_text, secondary_text, accept_button_text, NULL, NULL, NULL);
}

gint gtkpod_confirmation_hig(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text, const gchar *cancel_button_text, const gchar *third_button_text, const gchar *help_context) {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), -1);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_confirmation_hig(gtkpod_app, icon, primary_text, secondary_text, accept_button_text, cancel_button_text, third_button_text, help_context);
}

iTunesDB* gtkpod_get_current_itdb() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_itdb;
}

Playlist* gtkpod_get_current_playlist() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_playlist;
}

void gtkpod_set_current_playlist(Playlist* playlist) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));

    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_playlist = playlist;
    GList *tracks = NULL;
    if (playlist) {// if playlist not null then set its itdb as current
        GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_itdb = playlist->itdb;
        tracks = playlist->members;
    } else {
        GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_itdb = NULL;
    }

    g_signal_emit(gtkpod_app, gtkpod_app_signals[PLAYLIST_SELECTED], 0, playlist);
    gtkpod_set_displayed_tracks(tracks);
}

void gtkpod_track_added(Track *track) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (track);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[TRACK_UPDATED], 0, track);
}

void gtkpod_track_removed(Track *track) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (track);

    GList *selected_tracks = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks;
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks = g_list_remove(selected_tracks, track);

    GList *displayed_tracks = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks;
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks = g_list_remove(displayed_tracks, track);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[TRACK_REMOVED], 0, track);
}



void gtkpod_track_updated(Track *track) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (track);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[TRACK_UPDATED], 0, track);
}

GList *gtkpod_get_displayed_tracks() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    GList *current_tracks = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks;
    if (current_tracks && g_list_length(current_tracks) > 0) {
        return g_list_copy(current_tracks);
    }

    /* current_tracks is null or empty */
    Playlist *playlist = gtkpod_get_current_playlist();
    if (playlist) {
        return g_list_copy(playlist->members);
    }

    return NULL;
}

void gtkpod_set_displayed_tracks(GList *tracks) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    if (tracks == GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks)
        return; // Nothing to do

    if (tracks)
        GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks = g_list_copy(tracks);
    else {
        GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks = NULL;
        gtkpod_set_selected_tracks(NULL);
    }

    g_signal_emit(gtkpod_app, gtkpod_app_signals[TRACKS_DISPLAYED], 0, tracks);
}

GList *gtkpod_get_selected_tracks() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    GList *selected_tracks = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks;
    if (selected_tracks && g_list_length(selected_tracks) > 0) {
        return g_list_copy(selected_tracks);
    }

    return gtkpod_get_displayed_tracks();
}

void gtkpod_set_selected_tracks(GList *tracks) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    if (tracks == GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks)
        return; // Nothing to do

    if (tracks)
        GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks = g_list_copy(tracks);
    else
        GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks = NULL;

    g_signal_emit(gtkpod_app, gtkpod_app_signals[TRACKS_SELECTED], 0, tracks);
}

void gtkpod_set_sort_enablement(gboolean enable) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->sort_enablement = enable;

    g_signal_emit(gtkpod_app, gtkpod_app_signals[SORT_ENABLEMENT], 0, enable);
}

gboolean gtkpod_get_sort_enablement() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), TRUE);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->sort_enablement;
}

void gtkpod_playlist_added(iTunesDB *itdb, Playlist *playlist, gint32 pos) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (playlist);
    g_return_if_fail (playlist->itdb == itdb);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[PLAYLIST_ADDED], 0, playlist, pos);
}

void gtkpod_notify_data_changed(iTunesDB *itdb) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (itdb);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[ITDB_DATA_CHANGED], 0, itdb);

}

void gtkpod_notify_data_unchanged(iTunesDB *itdb) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (itdb);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[ITDB_DATA_SAVED], 0, itdb);

}

void gtkpod_broadcast_preference_change(gchar *preference_name, gint value) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (preference_name);

    g_signal_emit(gtkpod_app, gtkpod_app_signals[PREFERENCE_CHANGE], 0, preference_name, value);
}

/**
 * Register functions for the exporting of tracks, according to the
 * given contract. Used by the exported plugin to register its
 * functions as the official export utilties.
 */
void gtkpod_register_exporter(Exporter *exporter) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (EXPORTER_IS_EXPORTER(exporter));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->exporter = exporter;
}

/**
 * Unregisters the exporter if one has been assigned.
 */
void gtkpod_unregister_exporter() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->exporter = NULL;
}

gboolean gtkpod_has_exporter() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), FALSE);
    return (GTKPOD_APP_GET_INTERFACE (gtkpod_app)->exporter != NULL);
}

Exporter *gtkpod_get_exporter() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);

    if (!gtkpod_has_exporter())
        return NULL;

    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->exporter;
}

void gtkpod_display_widget(GtkWidget *widget) {
    g_return_if_fail(widget);
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE(gtkpod_app)->display_widget(widget);
}

void gtkpod_register_repository_editor(RepositoryEditor *editor) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (REPOSITORY_EDITOR_IS_EDITOR(editor));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->repository_editor = editor;
}

void gtkpod_unregister_repository_editor() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->repository_editor = NULL;
}

gboolean gtkpod_has_repository_editor() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), FALSE);
    RepositoryEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->repository_editor;
    return REPOSITORY_EDITOR_IS_EDITOR(editor);
}

void gtkpod_edit_repository(iTunesDB *itdb, Playlist *playlist) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));

    RepositoryEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->repository_editor;
    repository_editor_edit_repository(editor, itdb, playlist);
}

gboolean gtkpod_init_repository(iTunesDB *itdb) {
    g_return_val_if_fail(GTKPOD_IS_APP(gtkpod_app), FALSE);

    RepositoryEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->repository_editor;
    return repository_editor_init_repository(editor, itdb);
}

void gtkpod_populate_repository_model(iTunesDB *itdb, const gchar *old_model) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));

    RepositoryEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->repository_editor;
    repository_edtor_set_repository_model(editor, itdb, old_model);
}

void gtkpod_register_details_editor(DetailsEditor *editor) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (DETAILS_EDITOR_IS_EDITOR(editor));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->details_editor = editor;
}

void gtkpod_unregister_details_editor() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->details_editor = NULL;
}

gboolean gtkpod_has_details_editor() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), FALSE);
    DetailsEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->details_editor;
    return DETAILS_EDITOR_IS_EDITOR(editor);
}

void gtkpod_edit_details(GList *selected_tracks) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (GTKPOD_APP_GET_INTERFACE (gtkpod_app)->details_editor);

    DetailsEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->details_editor;
    details_editor_edit_details(editor, selected_tracks);
}

void gtkpod_register_lyrics_editor(LyricsEditor *editor) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (LYRICS_EDITOR_IS_EDITOR(editor));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->lyrics_editor = editor;
}

void gtkpod_unregister_lyrics_editor() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->lyrics_editor = NULL;
}

gboolean gtkpod_has_lyrics_editor() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), FALSE);
    return (GTKPOD_APP_GET_INTERFACE (gtkpod_app)->lyrics_editor != NULL);
}

void gtkpod_edit_lyrics(GList *tracks) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));

    LyricsEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->lyrics_editor;
    lyrics_editor_edit_lyrics(editor, tracks);
}

void gtkpod_register_photo_editor(PhotoEditor *editor) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (PHOTO_EDITOR_IS_EDITOR(editor));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->photo_editor = editor;
}

void gtkpod_unregister_photo_editor() {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->photo_editor = NULL;
}

gboolean gtkpod_has_photo_editor() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), FALSE);
    PhotoEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->photo_editor;
    return PHOTO_EDITOR_IS_EDITOR(editor);
}

void gtkpod_edit_photos(iTunesDB *itdb) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (GTKPOD_APP_GET_INTERFACE (gtkpod_app)->photo_editor);

    PhotoEditor *editor = GTKPOD_APP_GET_INTERFACE (gtkpod_app)->photo_editor;
    photo_editor_edit_photos(editor, itdb);
}

void gtkpod_register_track_command(TrackCommand *command) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);
    gp_iface->track_commands = g_list_append(gp_iface->track_commands, command);
}

void gtkpod_unregister_track_command(TrackCommand *command) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);
    gp_iface->track_commands = g_list_remove(gp_iface->track_commands, command);
}

static void gtkpod_display_command_ctx_menu(GList *track_cmds, GList *tracks) {
    GtkWidget *menu = NULL;
    GList *cmds;

    if (!track_cmds)
        return;

    if (!tracks)
        return;

    if (widgets_blocked)
        return;

    menu = gtk_menu_new();
    cmds = track_cmds;

    while(cmds != NULL) {
        TrackCommand *cmd = cmds->data;
        GPtrArray *pairarr = g_ptr_array_new ();
        g_ptr_array_add (pairarr, cmd);
        g_ptr_array_add (pairarr, tracks);
        hookup_menu_item(menu, track_command_get_text(cmd), GTK_STOCK_EXECUTE, G_CALLBACK (on_track_command_menuitem_activate), pairarr);
        cmds = cmds->next;
    }

    gtk_menu_popup(GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

void gtkpod_execute_track_command(GList *tracks) {
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);

    if (!tracks) {
        return;
    }

    switch(g_list_length(gp_iface->track_commands)) {
    case 0:
        // Do nothing
        break;
    case 1:
        ;
        TrackCommand *cmd = g_list_nth_data(gp_iface->track_commands, 0);
        track_command_execute(cmd, tracks);
        break;
    default:
        ;
        // More than one so see if there is a preference set
        gchar *cmdpref = NULL;
        if (prefs_get_string_value(DEFAULT_TRACK_COMMAND_PREF_KEY, &cmdpref)) {
            for (gint i = 0; i < g_list_length(gp_iface->track_commands); ++i) {
                TrackCommand *cmd = g_list_nth_data(gp_iface->track_commands, i);
                if (cmdpref && g_str_equal(cmdpref, track_command_get_id(cmd))) {
                    track_command_execute(cmd, tracks);
                    return;
                }
            }
        }

        // Otherwise show the menu
        gtkpod_display_command_ctx_menu(gp_iface->track_commands, tracks);
    }
}

GList *gtkpod_get_registered_track_commands() {
    g_return_val_if_fail(GTKPOD_IS_APP(gtkpod_app), NULL);
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);
    return g_list_copy(gp_iface->track_commands);
}

void gtkpod_register_filetype(FileType *filetype) {
    g_return_if_fail(FILE_IS_TYPE(filetype));
    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);

    GList *suffixes = filetype_get_suffixes(filetype);
    if (!suffixes)
        return;

    while(suffixes) {
        g_hash_table_insert(gp_iface->filetypes, suffixes->data, filetype);
        suffixes = g_list_next(suffixes);
    }
}

void gtkpod_unregister_filetype(FileType *filetype) {
    g_return_if_fail(FILE_IS_TYPE(filetype));

    g_return_if_fail(GTKPOD_IS_APP(gtkpod_app));
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);
    g_hash_table_remove(gp_iface->filetypes, filetype_get_name(filetype));
}

gboolean gtkpod_filetype_is_supported(gchar *name) {
    return gtkpod_get_filetype(name) != NULL ? TRUE : FALSE;
}

FileType* gtkpod_get_filetype(gchar *name) {
    g_return_val_if_fail(GTKPOD_IS_APP(gtkpod_app), FALSE);
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);
    return g_hash_table_lookup(gp_iface->filetypes, name);
}

GList *gtkpod_get_filetypes() {
    g_return_val_if_fail(GTKPOD_IS_APP(gtkpod_app), FALSE);
    GtkPodAppInterface *gp_iface = GTKPOD_APP_GET_INTERFACE (gtkpod_app);
    return g_hash_table_get_keys(gp_iface->filetypes);
}
