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

#ifndef GTKPOD_APP_IFACE_H_
#define GTKPOD_APP_IFACE_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade-xml.h>
#include "itdb.h"
#include "exporter_iface.h"
#include "repository_editor_iface.h"
#include "details_editor_iface.h"
#include "lyrics_editor_iface.h"
#include "photo_editor_iface.h"
#include "track_command_iface.h"
#include "filetype_iface.h"

#define GTKPOD_APP_ICON_STOCK_ID "gtkpod"

#define GTKPOD_APP_TYPE                (gtkpod_app_get_type ())
#define GTKPOD_APP(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTKPOD_APP_TYPE, GtkPodApp))
#define GTKPOD_IS_APP(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTKPOD_APP_TYPE))
#define GTKPOD_APP_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GTKPOD_APP_TYPE, GtkPodAppInterface))

#define SIGNAL_TRACKS_DISPLAYED "signal_tracks_displayed"
#define SIGNAL_TRACKS_SELECTED "signal_tracks_selected"
#define SIGNAL_TRACK_REMOVED "signal_track_removed"
#define SIGNAL_TRACK_UPDATED "signal_track_updated"
#define SIGNAL_TRACK_ADDED "signal_track_added"
#define SIGNAL_PLAYLIST_SELECTED "signal_playlist_selected"
#define SIGNAL_PLAYLIST_ADDED "signal_playlist_added"
#define SIGNAL_PLAYLIST_REMOVED "signal_playlist_removed"
#define SIGNAL_ITDB_UPDATED "signal_itdb_updated"
#define SIGNAL_ITDB_ADDED "signal_itdb_added"
#define SIGNAL_ITDB_REMOVED "signal_itdb_removed"
#define SIGNAL_SORT_ENABLEMENT "signal_sort_enablement"
#define SIGNAL_PREFERENCE_CHANGE "signal_preference_change"
#define SIGNAL_ITDB_DATA_CHANGED "signal_data_changed"
#define SIGNAL_ITDB_DATA_SAVED "signal_data_saved"

typedef void (*ConfHandler)(gpointer user_data1, gpointer user_data2);

/* states for gtkpod_confirmation options */
typedef enum {
    CONF_STATE_TRUE,
    CONF_STATE_FALSE,
    CONF_STATE_INVERT_TRUE,
    CONF_STATE_INVERT_FALSE,
} CONF_STATE;

/* predefined IDs for use with gtkpod_confirmation() */
enum {
    CONF_ID_IPOD_DIR = 0,
    CONF_ID_GTKPOD_WARNING,
    CONF_ID_DANGLING0,
    CONF_ID_DANGLING1,
    CONF_ID_SYNC_SUMMARY,
    CONF_ID_TRANSFER
} CONF_ID;

void CONF_NULL_HANDLER (gpointer d1, gpointer d2);

enum
{
    TRACKS_DISPLAYED,
    TRACKS_SELECTED,
    TRACK_ADDED,
    TRACK_REMOVED,
    TRACK_UPDATED,
    PLAYLIST_SELECTED,
    PLAYLIST_ADDED,
    PLAYLIST_REMOVED,
    ITDB_UPDATED,
    ITDB_ADDED,
    ITDB_REMOVED,
    SORT_ENABLEMENT,
    PREFERENCE_CHANGE,
    ITDB_DATA_CHANGED,
    ITDB_DATA_SAVED,
    LAST_SIGNAL
};

typedef struct _GtkPodApp GtkPodApp;
typedef struct _GtkPodAppInterface GtkPodAppInterface;

struct _GtkPodAppInterface {
    GTypeInterface g_iface;
    /* The current itdb database */
    iTunesDB *current_itdb;
    /* pointer to the currently selected playlist */
    Playlist *current_playlist;
    /* pointer to the currently displayed set of tracks */
    GList *displayed_tracks;
    /* pointer to the currently selected set of tracks */
    GList *selected_tracks;
    /* flag indicating whether sorting is enabled/disabled */
    gboolean sort_enablement;
    /* xml filename */
    gchar *xml_file;
    /* Exporter */
    Exporter *exporter;
    /* Repository Editor */
    RepositoryEditor *repository_editor;
    /* Details Editor */
    DetailsEditor *details_editor;
    /* Lyrics Editor */
    LyricsEditor *lyrics_editor;
    /* Photo Editor */
    PhotoEditor *photo_editor;
    /* Track Commands */
    GList *track_commands;
    /* Supported File Types */
    GHashTable *filetypes;

    void (*itdb_updated)(GtkPodApp *obj, iTunesDB *oldItdb, iTunesDB *newItbd);
    void (*statusbar_message)(GtkPodApp *obj, gchar* message);
    void (*gtkpod_warning)(GtkPodApp *obj, gchar *message);
    void (*gtkpod_warning_hig)(GtkPodApp *obj, GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text);
    gint
            (*gtkpod_confirmation_hig)(GtkPodApp *obj, GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text, const gchar *cancel_button_text, const gchar *third_button_text, const gchar *help_context);
    GtkResponseType
            (*gtkpod_confirmation)(GtkPodApp *obj, gint id, gboolean modal, const gchar *title, const gchar *label, const gchar *text, const gchar *option1_text, CONF_STATE option1_state, const gchar *option1_key, const gchar *option2_text, CONF_STATE option2_state, const gchar *option2_key, gboolean confirm_again, const gchar *confirm_again_key, ConfHandler ok_handler, ConfHandler apply_handler, ConfHandler cancel_handler, gpointer user_data1, gpointer user_data2);
    void (*statusbar_busy_push)(GtkPodApp *obj);
    void (*statusbar_busy_pop)(GtkPodApp *obj);
    GList * (*export_tracks_as_gchar)(iTunesDB *source_db, iTunesDB *dest_db, gchar *tracks);
    GList * (*export_tracks_as_glist)(iTunesDB *source_db, iTunesDB *dest_db, GList *tracks);
    void (*display_widget)(GtkWidget *widget);
};

GType gtkpod_app_get_type(void);

void gp_init(int argc, char *argv[]);

void gtkpod_app_set_glade_xml(gchar *xml_file);
gchar* gtkpod_get_glade_xml();

gboolean ok_to_close_gtkpod();
gint gtkpod_cleanup_quit();

void gtkpod_statusbar_message(gchar* message, ...);
void gtkpod_statusbar_busy_push();
void gtkpod_statusbar_busy_pop();
void gtkpod_tracks_statusbar_update(void);
void gtkpod_warning(gchar* message, ...);
void gtkpod_warning_simple (const gchar *format, ...);
void gtkpod_warning_hig(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text);
GtkResponseType
        gtkpod_confirmation(gint id, gboolean modal, const gchar *title, const gchar *label, const gchar *text, const gchar *option1_text, CONF_STATE option1_state, const gchar *option1_key, const gchar *option2_text, CONF_STATE option2_state, const gchar *option2_key, gboolean confirm_again, const gchar *confirm_again_key, ConfHandler ok_handler, ConfHandler apply_handler, ConfHandler cancel_handler, gpointer user_data1, gpointer user_data2);
gint
        gtkpod_confirmation_simple(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text);
gint
        gtkpod_confirmation_hig(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text, const gchar *cancel_button_text, const gchar *third_button_text, const gchar *help_context);

void gtkpod_notify_data_changed(iTunesDB *itdb);
void gtkpod_notify_data_unchanged(iTunesDB *itdb);

iTunesDB* gtkpod_get_current_itdb();

Playlist* gtkpod_get_current_playlist();
void gtkpod_set_current_playlist(Playlist* playlist);
void gtkpod_playlist_added(iTunesDB *itdb, Playlist *playlist, gint32 pos);

GList *gtkpod_get_displayed_tracks();
void gtkpod_set_displayed_tracks(GList *tracks);
GList *gtkpod_get_selected_tracks();
void gtkpod_set_selected_tracks(GList *tracks);
void gtkpod_track_added(Track *track);
void gtkpod_track_removed(Track *track);
void gtkpod_track_updated(Track *track);

void gtkpod_set_sort_enablement(gboolean enable);
gboolean gtkpod_get_sort_enablement();

void gtkpod_broadcast_preference_change(gchar *preference_name, gint value);

void gtkpod_register_exporter(Exporter *exporter);
void gtkpod_unregister_exporter();
gboolean gtkpod_has_exporter();
Exporter *gtkpod_get_exporter();

void gtkpod_display_widget(GtkWidget *widget);

void gtkpod_register_repository_editor(RepositoryEditor *editor);
void gtkpod_unregister_repository_editor();
gboolean gtkpod_has_repository_editor();
void gtkpod_edit_repository(iTunesDB *itdb, Playlist *playlist);
gboolean gtkpod_init_repository(iTunesDB *itdb);
void gtkpod_populate_repository_model(iTunesDB *itdb, const gchar *old_model);

void gtkpod_register_details_editor(DetailsEditor *editor);
void gtkpod_unregister_details_editor();
gboolean gtkpod_has_details_editor();
void gtkpod_edit_details(GList* selected_tracks);

void gtkpod_register_lyrics_editor(LyricsEditor *editor);
void gtkpod_unregister_lyrics_editor();
gboolean gtkpod_has_lyrics_editor();
void gtkpod_edit_lyrics(GList* tracks);

void gtkpod_register_photo_editor(PhotoEditor *editor);
void gtkpod_unregister_photo_editor();
gboolean gtkpod_has_photo_editor();
void gtkpod_edit_photos(iTunesDB *itdb);

void gtkpod_register_track_command(TrackCommand *command);
void gtkpod_unregister_track_command(TrackCommand *command);
void gtkpod_execute_track_command(GList *tracks);
GList *gtkpod_get_registered_track_commands();

void gtkpod_register_filetype(FileType *filetype);
void gtkpod_unregister_filetype(FileType *filetype);
gboolean gtkpod_filetype_is_supported(gchar *name);
FileType* gtkpod_get_filetype(gchar *name);
GList *gtkpod_get_filetypes();

GtkPodApp *gtkpod_app;
guint gtkpod_app_signals[LAST_SIGNAL];

#endif /* GTKPOD_APP_IFACE_H_ */
