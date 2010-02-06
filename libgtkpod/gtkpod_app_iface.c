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

#include "gtkpod_app_iface.h"
#include "gtkpod_app-marshallers.h"
#include <glib/gi18n-lib.h>

static void gtkpod_app_base_init(GtkPodAppInterface* klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        klass->current_itdb = NULL;
        klass->current_playlist = NULL;
        klass->sort_enablement = TRUE;

        gtkpod_app_signals[ITDB_UPDATED]
                = g_signal_new(SIGNAL_ITDB_UPDATED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, _gtkpod_app_marshal_VOID__POINTER_POINTER, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

        gtkpod_app_signals[PLAYLIST_SELECTED]
                = g_signal_new(SIGNAL_PLAYLIST_SELECTED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACKS_DISPLAYED]
                = g_signal_new(SIGNAL_TRACKS_DISPLAYED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[TRACKS_SELECTED]
                = g_signal_new(SIGNAL_TRACKS_SELECTED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

        gtkpod_app_signals[SORT_ENABLEMENT]
                = g_signal_new(SIGNAL_SORT_ENABLEMENT, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

        gtkpod_app_signals[PLAYLIST_ADDED]
                = g_signal_new(SIGNAL_PLAYLIST_ADDED, G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, _gtkpod_app_marshal_VOID__POINTER_INT, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);

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
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->xml_file = xml_file;
}

gchar* gtkpod_get_glade_xml() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->xml_file;
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

void gtkpod_set_current_itdb(iTunesDB* itdb) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_itdb = itdb;

    if (!itdb) // If setting itdb to null then set playlist to null too
        gtkpod_set_current_playlist(NULL);

    if (itdb && g_list_index(itdb->playlists, gtkpod_get_current_playlist()) == -1)
        // if playlist is not in itdb then set it to null
        gtkpod_set_current_playlist(NULL);
}

Playlist* gtkpod_get_current_playlist() {
    g_return_val_if_fail (GTKPOD_IS_APP(gtkpod_app), NULL);
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_playlist;
}

void gtkpod_set_current_playlist(Playlist* playlist) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));

    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_playlist = playlist;
    if (playlist) {// if playlist not null then set its itdb as current
        gtkpod_set_current_itdb(playlist->itdb);
        gtkpod_set_displayed_tracks(playlist->members);
    }

    g_signal_emit(gtkpod_app, gtkpod_app_signals[PLAYLIST_SELECTED], 0, playlist);
}

void gtkpod_playlist_updated(Playlist *playlist) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    g_return_if_fail (playlist);

    if (GTKPOD_APP_GET_INTERFACE (gtkpod_app)->current_playlist == playlist) {
        g_signal_emit(gtkpod_app, gtkpod_app_signals[PLAYLIST_SELECTED], 0, playlist);
        gtkpod_set_displayed_tracks(playlist->members);
    }
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
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->displayed_tracks = tracks;

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
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->selected_tracks = tracks;

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
