/*
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-main.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 *          Mike Hearn  <mike@theoretic.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/misc.h"

#include "sound-juicer.h"

#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <brasero-medium-selection.h>
#include <brasero-volume.h>
#include <gst/gst.h>

#include "bacon-message-connection.h"
#include "rb-gst-media-types.h"
#include "sj-metadata-getter.h"
#include "sj-extractor.h"
#include "sj-structures.h"
#include "sj-error.h"
#include "sj-util.h"
#include "sj-main.h"
#include "sj-prefs.h"
#include "sj-genres.h"

gboolean on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data);

static void reread_cd (gboolean ignore_no_media);
static void update_ui_for_album (AlbumDetails *album);
static void set_duplication (gboolean enable);

/* Prototypes for the signal blocking/unblocking in update_ui_for_album */
G_MODULE_EXPORT void on_title_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_artist_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_year_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_disc_number_edit_changed(GtkEditable *widget, gpointer user_data);

GtkBuilder *builder;

SjMetadataGetter *metadata;
SjExtractor *extractor;

GSettings *sj_settings;

static GtkWidget *vbox1;
static GtkWidget *message_area_eventbox;
static GtkWidget *title_entry, *artist_entry, *duration_label, *genre_entry, *year_entry, *disc_number_entry;
static GtkWidget *track_listview, *extract_button;
static GtkWidget *status_bar;
static GtkWidget *extract_menuitem, *select_all_menuitem, *deselect_all_menuitem;
static GtkWidget *submit_menuitem;
static GtkWidget *duplicate, *eject;
GtkListStore *track_store;
static BaconMessageConnection *connection;
GtkCellRenderer *toggle_renderer, *title_renderer, *artist_renderer;

GtkWidget *current_message_area;

char *path_pattern = NULL;
char *file_pattern = NULL;
GFile *base_uri;
BraseroDrive *drive = NULL;
gboolean strip_chars;
gboolean eject_finished;
gboolean open_finished;
gboolean extracting = FALSE;
static gboolean duplication_enabled;

static gint total_no_of_tracks;
static gint no_of_tracks_selected;
static AlbumDetails *current_album;
static char *current_submit_url = NULL;

#define DEFAULT_PARANOIA 4
#define RAISE_WINDOW "raise-window"
#define SJCD_SCHEMA "org.gtkpod.sjcd"

static void error_on_start (GError *error)
{
  gtkpod_statusbar_message("Could not start sound juicer because %s", error->message);
  g_error_free(error);
}

/**
 * Clicked Eject
 */
G_MODULE_EXPORT void on_eject_activate (GtkMenuItem *item, gpointer user_data)
{
  brasero_drive_eject (drive, FALSE, NULL);
}

G_MODULE_EXPORT gboolean on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  if (extracting) {
    GtkWidget *dialog;
    int response;

    dialog = gtk_message_dialog_new (GTK_WINDOW (gtkpod_app), GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_NONE,
                                     _("You are currently extracting a CD. Do you want to quit now or continue?"));
    gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-quit", GTK_RESPONSE_ACCEPT);
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Continue"), GTK_RESPONSE_REJECT);
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    if (response == GTK_RESPONSE_ACCEPT) {
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

static gboolean select_all_foreach_cb (GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
  gboolean b = (gboolean)GPOINTER_TO_INT (data);
  gtk_list_store_set (track_store, iter, COLUMN_EXTRACT, b, -1);
  return FALSE;
}

G_MODULE_EXPORT void on_select_all_activate (GtkMenuItem *item, gpointer user_data)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (track_store), select_all_foreach_cb, GINT_TO_POINTER (TRUE));
  gtk_widget_set_sensitive (extract_button, TRUE);
  gtk_widget_set_sensitive (extract_menuitem, TRUE);
  gtk_widget_set_sensitive (select_all_menuitem, FALSE);
  gtk_widget_set_sensitive (deselect_all_menuitem, TRUE);
  no_of_tracks_selected = total_no_of_tracks;
}

G_MODULE_EXPORT void on_deselect_all_activate (GtkMenuItem *item, gpointer user_data)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (track_store), select_all_foreach_cb, GINT_TO_POINTER (FALSE));
  gtk_widget_set_sensitive (extract_button, FALSE);
  gtk_widget_set_sensitive (extract_menuitem, FALSE);
  gtk_widget_set_sensitive (deselect_all_menuitem, FALSE);
  gtk_widget_set_sensitive (select_all_menuitem,TRUE);
  no_of_tracks_selected = 0;
}

/**
 * GtkTreeView cell renderer callback to render durations
 */
static void duration_cell_data_cb (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer *cell,
                                GtkTreeModel *tree_model,
                                GtkTreeIter *iter,
                                gpointer data)
{
  int duration;
  char *string;

  gtk_tree_model_get (tree_model, iter, COLUMN_DURATION, &duration, -1);
  if (duration != 0) {
    string = g_strdup_printf("%d:%02d", duration / 60, duration % 60);
  } else {
    string = g_strdup(_("(unknown)"));
  }
  g_object_set (G_OBJECT (cell), "text", string, NULL);
  g_free (string);
}

static void number_cell_icon_data_cb (GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer *cell,
                                      GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
  TrackState state;
  gtk_tree_model_get (tree_model, iter, COLUMN_STATE, &state, -1);
  switch (state) {
  case STATE_IDLE:
    g_object_set (G_OBJECT (cell), "stock-id", "", NULL);
    break;
  case STATE_PLAYING:
    g_object_set (G_OBJECT (cell), "stock-id", GTK_STOCK_MEDIA_PLAY, NULL);
    break;
  case STATE_PAUSED:
    g_object_set (G_OBJECT (cell), "stock-id", GTK_STOCK_MEDIA_PAUSE, NULL);
    break;
  case STATE_EXTRACTING:
    g_object_set (G_OBJECT (cell), "stock-id", GTK_STOCK_MEDIA_RECORD, NULL);
    break;
  default:
    g_warning("Unhandled track state %d\n", state);
  }
}

/* Taken from gedit */
static void
set_info_bar_text_and_icon (GtkInfoBar  *infobar,
                            const gchar *icon_stock_id,
                            const gchar *primary_text,
                            const gchar *secondary_text,
                            GtkWidget   *button)
{
  GtkWidget *content_area;
  GtkWidget *hbox_content;
  GtkWidget *image;
  GtkWidget *vbox;
  gchar *primary_markup;
  gchar *secondary_markup;
  GtkWidget *primary_label;
  GtkWidget *secondary_label;
  AtkObject *ally_target;

  ally_target = gtk_widget_get_accessible (button);

  hbox_content = gtk_hbox_new (FALSE, 8);
  gtk_widget_show (hbox_content);

  image = gtk_image_new_from_stock (icon_stock_id, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

  primary_markup = g_markup_printf_escaped ("<b>%s</b>", primary_text);
  primary_label = gtk_label_new (primary_markup);
  g_free (primary_markup);
  gtk_widget_show (primary_label);
  gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
  gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
  atk_object_add_relationship (ally_target,
                               ATK_RELATION_LABELLED_BY,
                               gtk_widget_get_accessible (primary_label));

  if (secondary_text != NULL) {
    secondary_markup = g_markup_printf_escaped ("<small>%s</small>",
                                                secondary_text);
    secondary_label = gtk_label_new (secondary_markup);
    g_free (secondary_markup);
    gtk_widget_show (secondary_label);
    gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
    gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
    gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
    atk_object_add_relationship (ally_target,
                                 ATK_RELATION_LABELLED_BY,
                                 gtk_widget_get_accessible (secondary_label));
  }

  content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (infobar));
  gtk_container_add (GTK_CONTAINER (content_area), hbox_content);
}

/* Taken from gedit */
static void
set_message_area (GtkWidget *container,
                  GtkWidget *message_area)
{
  if (current_message_area == message_area)
    return;

  if (current_message_area != NULL)
    gtk_widget_destroy (current_message_area);

  current_message_area = message_area;

  if (message_area == NULL)
    return;

  gtk_container_add (GTK_CONTAINER (container), message_area);

  g_object_add_weak_pointer (G_OBJECT (current_message_area),
                             (gpointer)&current_message_area);
}

static GtkWidget*
musicbrainz_submit_info_bar_new (char *title, char *artist)
{
  GtkWidget *infobar, *button;
  char *primary_text;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (artist != NULL, NULL);

  infobar = gtk_info_bar_new ();
  button = gtk_info_bar_add_button (GTK_INFO_BAR (infobar),
                                    _("S_ubmit Album"), GTK_RESPONSE_OK);
  gtk_info_bar_add_button (GTK_INFO_BAR (infobar),
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  /* Translators: title, artist */
  primary_text = g_strdup_printf (_("Could not find %s by %s on MusicBrainz."), title, artist);

  set_info_bar_text_and_icon (GTK_INFO_BAR (infobar),
                              "gtk-dialog-info",
                              primary_text,
                              _("You can improve the MusicBrainz database by adding this album."),
                              button);

  g_free (primary_text);

  return infobar;
}

static void
musicbrainz_submit_info_bar_response (GtkInfoBar *infobar,
                                      int         response_id,
                                      gpointer    user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    on_submit_activate (NULL, NULL);
  }

  set_message_area (message_area_eventbox, NULL);
}

/**
 * Utility function to update the UI for a given Album
 */
static void update_ui_for_album (AlbumDetails *album)
{
  GList *l;
  int album_duration = 0;
  char* duration_text;
  total_no_of_tracks=0;

  if (album == NULL) {
    gtk_list_store_clear (track_store);
    gtk_entry_set_text (GTK_ENTRY (title_entry), "");
    gtk_entry_set_text (GTK_ENTRY (artist_entry), "");
    gtk_entry_set_text (GTK_ENTRY (genre_entry), "");
    gtk_entry_set_text (GTK_ENTRY (year_entry), "");
    gtk_entry_set_text (GTK_ENTRY (disc_number_entry), "");
    gtk_label_set_text (GTK_LABEL (duration_label), "");
    gtk_widget_set_sensitive (title_entry, FALSE);
    gtk_widget_set_sensitive (artist_entry, FALSE);
    gtk_widget_set_sensitive (genre_entry, FALSE);
    gtk_widget_set_sensitive (year_entry, FALSE);
    gtk_widget_set_sensitive (disc_number_entry, FALSE);
    gtk_widget_set_sensitive (extract_button, FALSE);
    gtk_widget_set_sensitive (extract_menuitem, FALSE);
    gtk_widget_set_sensitive (select_all_menuitem, FALSE);
    gtk_widget_set_sensitive (deselect_all_menuitem, FALSE);
    set_duplication (FALSE);

    set_message_area (message_area_eventbox, NULL);
  } else {
    gtk_list_store_clear (track_store);

    g_signal_handlers_block_by_func (title_entry, on_title_edit_changed, NULL);
    g_signal_handlers_block_by_func (artist_entry, on_artist_edit_changed, NULL);
    g_signal_handlers_block_by_func (year_entry, on_year_edit_changed, NULL);
    g_signal_handlers_block_by_func (disc_number_entry, on_disc_number_edit_changed, NULL);
    gtk_entry_set_text (GTK_ENTRY (title_entry), album->title);
    gtk_entry_set_text (GTK_ENTRY (artist_entry), album->artist);
    if (album->disc_number) {
      gchar *disc_number = g_strdup_printf ("%d", album->disc_number);
      gtk_entry_set_text (GTK_ENTRY (disc_number_entry), disc_number);
      g_free (disc_number);
    }
    if (album->release_date && g_date_valid (album->release_date)) {
      gchar *release_date =  g_strdup_printf ("%d", g_date_get_year (album->release_date));
      gtk_entry_set_text (GTK_ENTRY (year_entry), release_date);
      g_free (release_date);
    }
    g_signal_handlers_unblock_by_func (title_entry, on_title_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (artist_entry, on_artist_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (year_entry, on_year_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (disc_number_entry, on_disc_number_edit_changed, NULL);
    /* Clear the genre field, it's from the user */
    gtk_entry_set_text (GTK_ENTRY (genre_entry), "");

    gtk_widget_set_sensitive (title_entry, TRUE);
    gtk_widget_set_sensitive (artist_entry, TRUE);
    gtk_widget_set_sensitive (genre_entry, TRUE);
    gtk_widget_set_sensitive (year_entry, TRUE);
    gtk_widget_set_sensitive (disc_number_entry, TRUE);
    gtk_widget_set_sensitive (extract_button, TRUE);
    gtk_widget_set_sensitive (extract_menuitem, TRUE);
    gtk_widget_set_sensitive (select_all_menuitem, FALSE);
    gtk_widget_set_sensitive (deselect_all_menuitem, TRUE);
    set_duplication (TRUE);

    for (l = album->tracks; l; l=g_list_next (l)) {
      GtkTreeIter iter;
      TrackDetails *track = (TrackDetails*)l->data;
      album_duration += track->duration;
      gtk_list_store_append (track_store, &iter);
      gtk_list_store_set (track_store, &iter,
                          COLUMN_STATE, STATE_IDLE,
                          COLUMN_EXTRACT, TRUE,
                          COLUMN_NUMBER, track->number,
                          COLUMN_TITLE, track->title,
                          COLUMN_ARTIST, track->artist,
                          COLUMN_DURATION, track->duration,
                          COLUMN_DETAILS, track,
                          -1);
     total_no_of_tracks++;
    }
    no_of_tracks_selected=total_no_of_tracks;

    /* Some albums don't have track durations :( */
    if (album_duration) {
      duration_text = g_strdup_printf("%d:%02d", album_duration / 60, album_duration % 60);
      gtk_label_set_text (GTK_LABEL (duration_label), duration_text);
      g_free (duration_text);
    } else {
      gtk_label_set_text (GTK_LABEL (duration_label), _("(unknown)"));
    }

    /* If album details don't come from MusicBrainz ask user to add them */
    if (album->metadata_source != SOURCE_MUSICBRAINZ) {
      GtkWidget *infobar;

      infobar = musicbrainz_submit_info_bar_new (album->title, album->artist);

      set_message_area (message_area_eventbox, infobar);

      g_signal_connect (infobar,
                        "response",
                        G_CALLBACK (musicbrainz_submit_info_bar_response),
                        NULL);

      gtk_info_bar_set_default_response (GTK_INFO_BAR (infobar),
                                         GTK_RESPONSE_CANCEL);

      gtk_widget_show (infobar);
    }
  }
}

/**
 * Callback that gets fired when a user double clicks on a row
 */
static void album_row_activated (GtkTreeView *treeview,
                                 GtkTreePath *arg1,
                                 GtkTreeViewColumn *arg2,
                                 gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG (user_data);
  gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

/**
 * Callback that gets fired when an the selection changes. We use this to
 * change the sensitivity of the continue button
 */
static void selected_album_changed (GtkTreeSelection *selection,
                                    gpointer *user_data)
{
  GtkWidget *ok_button = GTK_WIDGET (user_data);

  if (gtk_tree_selection_get_selected (selection, NULL, NULL)) {
    gtk_widget_set_sensitive (ok_button, TRUE);
  } else {
    gtk_widget_set_sensitive (ok_button, FALSE);
  }
}

/**
 * Utility function for when there are more than one albums available
 */
AlbumDetails* multiple_album_dialog(GList *albums)
{
  static GtkWidget *dialog = NULL, *albums_listview;
  static GtkListStore *albums_store;
  static GtkTreeSelection *selection;
  AlbumDetails *album;
  GtkTreeIter iter;
  int response;
  GtkWidget *ok_button = NULL;

  if (dialog == NULL) {
    GtkTreeViewColumn *column;
    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new ();

    dialog = GET_WIDGET ("multiple_dialog");
    g_assert (dialog != NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_app));
    albums_listview = GET_WIDGET ("albums_listview");
    ok_button       = GET_WIDGET ("ok_button");

    g_signal_connect (albums_listview, "row-activated", G_CALLBACK (album_row_activated), dialog);

    albums_store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                       text_renderer,
                                                       "text", 0,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (albums_listview), column);

    column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                       text_renderer,
                                                       "text", 1,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (albums_listview), column);
    gtk_tree_view_set_model (GTK_TREE_VIEW (albums_listview), GTK_TREE_MODEL (albums_store));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (albums_listview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    gtk_widget_set_sensitive (ok_button, FALSE);
    g_signal_connect (selection, "changed", (GCallback)selected_album_changed, ok_button);
  }

  gtk_list_store_clear (albums_store);
  for (; albums ; albums = g_list_next (albums)) {
    GtkTreeIter iter;
    AlbumDetails *album = (AlbumDetails*)(albums->data);
    gtk_list_store_append (albums_store, &iter);
    gtk_list_store_set (albums_store, &iter,
                        0, album->title,
                        1, album->artist,
                        2, album,
                        -1);
  }

  /* Select the first album */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (albums_store), &iter))
  {
    gtk_tree_selection_select_iter (selection, &iter);
  }

  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  if (response == GTK_RESPONSE_DELETE_EVENT) {
    return NULL;
  }

  if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (albums_store), &iter, 2, &album, -1);
    return album;
  } else {
    return NULL;
  }
}

 /**
 * The GSettings key for the base path changed
  */
static void baseuri_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  gchar *value;
  g_assert (strcmp (key, SJ_SETTINGS_BASEURI) == 0);
  if (base_uri) {
     g_object_unref (base_uri);
  }
  value = g_settings_get_string (settings, key);
  if ((value == NULL) || (*value == '\0')) {
     base_uri = sj_get_default_music_directory ();
  } else {
     base_uri = g_file_new_for_uri (value);
  }
  g_free (value);
  /* TODO: sanity check the URI somewhat */
}

 /**
  * The GSettings key for the directory pattern changed
  */
static void path_pattern_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_PATH_PATTERN) == 0);
  g_free (path_pattern);
  path_pattern = g_settings_get_string (settings, key);
  /* TODO: sanity check the pattern */
}

 /**
  * The GSettings key for the filename pattern changed
  */
static void file_pattern_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_FILE_PATTERN) == 0);
  g_free (file_pattern);
  file_pattern = g_settings_get_string (settings, key);
  /* TODO: sanity check the pattern */
}

 /**
 * The GSettings key for the paranoia mode has changed
  */
static void paranoia_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  int value;
  g_assert (strcmp (key, SJ_SETTINGS_PARANOIA) == 0);
  value = g_settings_get_int (settings, key);
  if (value == 0 || value == 2 || value == 4 || value == 8 || value == 16 || value == 255) {
    sj_extractor_set_paranoia (extractor, value);
  }
}

 /**
  * The GSettings key for the strip characters option changed
  */
static void strip_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_STRIP) == 0);
  strip_chars = g_settings_get_boolean (settings, key);
}

 /**
  * The GSettings key for the eject when finished option changed
  */
static void eject_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_EJECT) == 0);
  eject_finished = g_settings_get_boolean (settings, key);
}

 /**
  * The GSettings key for the open when finished option changed
  */
static void open_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_OPEN) == 0);
  open_finished = g_settings_get_boolean (settings, key);
}

 /**
  * The GSettings key for audio volume changes
  */
static void audio_volume_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_AUDIO_VOLUME) == 0);

  GtkWidget *volb = GET_WIDGET ("volume_button");
  gtk_scale_button_set_value (GTK_SCALE_BUTTON (volb), g_settings_get_double (settings, key));
}

static void
metadata_cb (SjMetadataGetter *m, GList *albums, GError *error)
{
  gboolean realized = gtk_widget_get_realized (GTK_WIDGET(gtkpod_app));

  if (realized)
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET(gtkpod_app)), NULL);
    /* Clear the statusbar message */
    gtk_statusbar_pop(GTK_STATUSBAR(status_bar), 0);

  if (error && !(error->code == SJ_ERROR_CD_NO_MEDIA)) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup (realized ? GTK_WINDOW (gtkpod_app) : NULL, 0,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 "<b>%s</b>\n\n%s\n%s: %s",
                                                 _("Could not read the CD"),
                                                 _("Sound Juicer could not read the track listing on this CD."),
                                                 _("Reason"),
                                                 error->message);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    update_ui_for_album (NULL);
    return;
  }

  g_free (current_submit_url);
  current_submit_url = sj_metadata_getter_get_submit_url (metadata);
  if (current_submit_url) {
    gtk_widget_set_sensitive (submit_menuitem, TRUE);
  }

  /* Free old album details */
  if (current_album != NULL) {
    album_details_free (current_album);
    current_album = NULL;
  }
  /* Set the new current album pointer */
  if (albums == NULL) {
    current_album = NULL;
  } else if (g_list_next (albums)) {
    current_album = multiple_album_dialog (albums);
    /* Concentrate here. We remove the album we want from the list, and then
       deep-free the list. */
    albums = g_list_remove (albums, current_album);
    g_list_deep_free (albums, (GFunc)album_details_free);
    albums = NULL;
  } else {
    current_album = albums->data;
    /* current_album now owns ->data, so just free the list */
    g_list_free (albums);
    albums = NULL;
  }
  update_ui_for_album (current_album);
}

static gboolean
is_audio_cd (BraseroDrive *drive)
{
  BraseroMedium *medium;
  BraseroMedia type;

  medium = brasero_drive_get_medium (drive);
  if (medium == NULL) return FALSE;
  type = brasero_medium_get_status (medium);
  if (type == BRASERO_MEDIUM_UNSUPPORTED) {
    g_warning ("Error getting media type\n");
  }
  if (type == BRASERO_MEDIUM_BUSY) {
    g_warning ("BUSY getting media type, should re-check\n");
  }
  return BRASERO_MEDIUM_IS (type, BRASERO_MEDIUM_HAS_AUDIO|BRASERO_MEDIUM_CD);
}

/**
 * Utility function to reread a CD
 */
static void reread_cd (gboolean ignore_no_media)
{
  /* TODO: remove ignore_no_media? */
  GError *error = NULL;
//  GdkCursor *cursor;
//  GdkWindow *window;
  gboolean realized = gtk_widget_get_realized (GTK_WIDGET(gtkpod_app));
//
//  window = gtk_widget_get_window (GTK_WIDGET(gtkpod_app));
//
//  /* Set watch cursor */
//  if (realized) {
//    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (gtkpod_app)), GDK_WATCH);
//    gdk_window_set_cursor (window, cursor);
//    gdk_cursor_unref (cursor);
//    gdk_display_sync (gtk_widget_get_display (GTK_WIDGET (gtkpod_app)));
//  }

  /* Set statusbar message */
  gtk_statusbar_push(GTK_STATUSBAR(status_bar), 0, _("Retrieving track listing...please wait."));

  g_free (current_submit_url);
  current_submit_url = NULL;
  gtk_widget_set_sensitive (submit_menuitem, FALSE);

  if (!is_audio_cd (drive)) {
    update_ui_for_album (NULL);
    // TODO Use gtkpod statusbar instead
    gtk_statusbar_pop(GTK_STATUSBAR(status_bar), 0);
//    if (realized)
//      gdk_window_set_cursor (window, NULL);
    return;
  }

  sj_metadata_getter_list_albums (metadata, &error);

  if (error && !(error->code == SJ_ERROR_CD_NO_MEDIA && ignore_no_media)) {
      //TODO
      // Change dialog to be gtkpod dialog

    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (realized ? GTK_WINDOW (gtkpod_app) : NULL, 0,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", _("Could not read the CD"));
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
                                                "%s\n%s: %s",
                                                _("Sound Juicer could not read the track listing on this CD."),
                                                _("Reason"),
                                                error->message);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free (error);
    update_ui_for_album (NULL);
    return;
  }
}

static void
media_added_cb (BraseroMediumMonitor	*drive,
		BraseroMedium		*medium,
                gpointer           	 data)
{
  if (extracting == TRUE) {
    /* FIXME: recover? */
  }

  reread_cd (TRUE);
}

static void
media_removed_cb (BraseroMediumMonitor	*drive,
		  BraseroMedium		*medium,
                  gpointer           data)
{
  if (extracting == TRUE) {
    /* FIXME: recover? */
  }

  update_ui_for_album (NULL);
}

static void
set_drive_from_device (const char *device)
{
  BraseroMediumMonitor *monitor;

  if (drive) {
    g_object_unref (drive);
    drive = NULL;
  }

  if (! device)
    return;

  monitor = brasero_medium_monitor_get_default ();
  drive = brasero_medium_monitor_get_drive (monitor, device);
  if (! drive) {
    GtkWidget *dialog;
    char *message;
    message = g_strdup_printf (_("Sound Juicer could not use the CD-ROM device '%s'"), device);
    dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtkpod_app),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 "<b>%s</b>\n\n%s",
                                                 message,
                                                 _("HAL daemon may not be running."));
    g_free (message);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return;
  }

  g_signal_connect (monitor, "medium-added", G_CALLBACK (media_added_cb), NULL);
  g_signal_connect (monitor, "medium-removed", G_CALLBACK (media_removed_cb), NULL);
}

static void
set_device (const char* device, gboolean ignore_no_media)
{
  gboolean tray_opened;

  if (device == NULL) {
    set_drive_from_device (device);
  } else if (access (device, R_OK) != 0) {
    GtkWidget *dialog;
    char *message;
    const char *error;

    error = g_strerror (errno);
    message = g_strdup_printf (_("Sound Juicer could not access the CD-ROM device '%s'"), device);

    dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtkpod_app),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 "<b>%s</b>\n\n%s\n%s: %s",
                                                 _("Could not read the CD"),
                                                 message,
                                                 _("Reason"),
                                                 error);
    g_free (message);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    /* Set a null device */
    set_drive_from_device (NULL);
  } else {
    set_drive_from_device (device);
  }

  sj_metadata_getter_set_cdrom (metadata, device);
  sj_extractor_set_device (extractor, device);

  if (drive != NULL) {
    tray_opened = brasero_drive_is_door_open (drive);
    if (tray_opened == FALSE) {
      reread_cd (ignore_no_media);
    }

    /* Enable/disable the eject options based on wether the drive supports ejection */
    gtk_widget_set_sensitive (eject, brasero_drive_can_eject (drive));
  }
}

gboolean cd_drive_exists (const char *device)
{
  BraseroMediumMonitor *monitor;
  BraseroDrive *drive;
  gboolean exists;

  if (device == NULL)
    return FALSE;

  monitor = brasero_medium_monitor_get_default ();
  drive = brasero_medium_monitor_get_drive (monitor, device);
  exists = (drive != NULL);
  if (exists)
    g_object_unref (drive);

  return exists;
}

const char *
prefs_get_default_device (void)
{
  static const char * default_device = NULL;

  if (default_device == NULL) {
    BraseroMediumMonitor *monitor;

    BraseroDrive *drive;
    GSList *drives;

    monitor = brasero_medium_monitor_get_default ();
    drives = brasero_medium_monitor_get_drives (monitor, BRASERO_DRIVE_TYPE_ALL);
    if (drives == NULL)
      return NULL;

    drive = drives->data;
    default_device = brasero_drive_get_device (drive);

    g_slist_foreach (drives, (GFunc) g_object_unref, NULL);
    g_slist_free (drives);
  }
  return default_device;
}

/**
 * The GSettings key for the device changed
 */
static void device_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  const char *device;
  char *value;
  gboolean ignore_no_media = GPOINTER_TO_INT (user_data);
  g_assert (strcmp (key, SJ_SETTINGS_DEVICE) == 0);

  value = g_settings_get_string (settings, key);
  if (!cd_drive_exists (value)) {
    device = prefs_get_default_device();
    if (device == NULL) {
#ifndef IGNORE_MISSING_CD
      GtkWidget *dialog;
      dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtkpod_app),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "<b>%s</b>\n\n%s",
                                                   _("No CD-ROM drives found"),
                                                   _("Sound Juicer could not find any CD-ROM drives to read."));
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      exit (1);
#endif
    }
  } else {
    device = value;
  }
  set_device (device, ignore_no_media);
}

static void profile_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  GstEncodingProfile *profile;
  gchar *media_type;

  g_assert (strcmp (key, SJ_SETTINGS_AUDIO_PROFILE) == 0);
  media_type = g_settings_get_string (settings, key);
  profile = rb_gst_get_encoding_profile (media_type);
  g_free (media_type);

  if (profile != NULL)
    g_object_set (extractor, "profile", profile, NULL);

  if (profile == NULL || !sj_extractor_supports_profile(profile)) {
    GtkWidget *dialog;
    int response;

    dialog = gtk_message_dialog_new (GTK_WINDOW (gtkpod_app),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_NONE,
                                     _("The currently selected audio profile is not available on your installation."));
    gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-quit", GTK_RESPONSE_REJECT);
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Change Profile"), GTK_RESPONSE_ACCEPT);
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
      gtk_widget_destroy (dialog);
      on_edit_preferences_cb (NULL, NULL);
    } else {
      /* Can't use gtk_main_quit here, we may be outside the main loop */
      exit(0);
    }
  }

  if (profile != NULL)
    gst_encoding_profile_unref (profile);
}

/**
 * Configure the http proxy
 */
static void
http_proxy_setup (GSettings *settings)
{
  if (!g_settings_get_boolean (settings, SJ_SETTINGS_HTTP_PROXY_ENABLE)) {
    sj_metadata_getter_set_proxy (metadata, NULL);
  } else {
    char *host;
    int port;

    host = g_settings_get_string (settings, SJ_SETTINGS_HTTP_PROXY);
    sj_metadata_getter_set_proxy (metadata, host);
    g_free (host);
    port = g_settings_get_int (settings, SJ_SETTINGS_HTTP_PROXY_PORT);
    sj_metadata_getter_set_proxy_port (metadata, port);
  }
}

/**
 * The GSettings key for the HTTP proxy being enabled changed.
 */
static void http_proxy_enable_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_HTTP_PROXY_ENABLE) == 0);
  http_proxy_setup (settings);
}

/**
 * The GSettings key for the HTTP proxy changed.
 */
static void http_proxy_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_HTTP_PROXY) == 0);
  http_proxy_setup (settings);
}

/**
 * The GSettings key for the HTTP proxy port changed.
 */
static void http_proxy_port_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_HTTP_PROXY_PORT) == 0);
  http_proxy_setup (settings);
}

/**
 * Clicked on Reread in the UI (button/menu)
 */
G_MODULE_EXPORT void on_reread_activate (GtkWidget *button, gpointer user_data)
{
  reread_cd (FALSE);
}

/**
 * Clicked the Submit menu item in the UI
 */
G_MODULE_EXPORT void on_submit_activate (GtkWidget *menuitem, gpointer user_data)
{
  GError *error = NULL;

  if (current_submit_url) {
      if (!gtk_show_uri (NULL, current_submit_url, GDK_CURRENT_TIME, &error)) {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtkpod_app),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "<b>%s</b>\n\n%s\n%s: %s",
                                                   _("Could not open URL"),
                                                   _("Sound Juicer could not open the submission URL"),
                                                   _("Reason"),
                                                   error->message);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      g_error_free (error);
    }
  }

}

/**
 * Called in on_extract_toggled to see if there are selected tracks or not.
 * extracting points to the boolean to set to if there are tracks to extract,
 * and starts as false.
 */
static gboolean extract_available_foreach (GtkTreeModel *model,
                                           GtkTreePath *path,
                                           GtkTreeIter *iter,
                                           gboolean *extracting)
{
  gboolean selected;
  gtk_tree_model_get (GTK_TREE_MODEL (track_store), iter, COLUMN_EXTRACT, &selected, -1);
  if (selected) {
    *extracting = TRUE;
    /* Don't bother walking the list more, we've found a track to be extracted. */
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
 * Called when the user clicked on the Extract column check boxes
 */
static void on_extract_toggled (GtkCellRendererToggle *cellrenderertoggle,
                                gchar *path,
                                gpointer user_data)
{
  gboolean extract;
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (track_store), &iter, path))
      return;
  gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter, COLUMN_EXTRACT, &extract, -1);
  /* extract is the old state here, so toggle */
  extract = !extract;
  gtk_list_store_set (track_store, &iter, COLUMN_EXTRACT, extract, -1);

  /* Update the Extract buttons */
  if (extract) {
    /* If true, then we can extract */
    gtk_widget_set_sensitive (extract_button, TRUE);
    gtk_widget_set_sensitive (extract_menuitem, TRUE);
    no_of_tracks_selected++;
  } else {
    /* Reuse the boolean extract */
    extract = FALSE;
    gtk_tree_model_foreach (GTK_TREE_MODEL (track_store), (GtkTreeModelForeachFunc)extract_available_foreach, &extract);
    gtk_widget_set_sensitive (extract_button, extract);
    gtk_widget_set_sensitive (extract_menuitem, extract);
    no_of_tracks_selected--;
  }
  /* Enable and disable the Select/Deselect All buttons */
  if (no_of_tracks_selected == total_no_of_tracks) {
    gtk_widget_set_sensitive(deselect_all_menuitem, TRUE);
    gtk_widget_set_sensitive(select_all_menuitem, FALSE);
  } else if (no_of_tracks_selected == 0) {
    gtk_widget_set_sensitive(deselect_all_menuitem, FALSE);
    gtk_widget_set_sensitive(select_all_menuitem, TRUE);
  } else {
    gtk_widget_set_sensitive(select_all_menuitem, TRUE);
    gtk_widget_set_sensitive(deselect_all_menuitem, TRUE);
  }
}

/**
 * Callback when the title or artist cells are edited in the list. column_data
 * contains the column number in the model which was modified.
 */
static void on_cell_edited (GtkCellRendererText *renderer,
                 gchar *path, gchar *string,
                 gpointer column_data)
{
  ViewColumn column = GPOINTER_TO_INT (column_data);
  GtkTreeIter iter;
  TrackDetails *track;
  char *artist, *title;

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (track_store), &iter, path))
    return;
  gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter,
                      COLUMN_ARTIST, &artist,
                      COLUMN_TITLE, &title,
                      COLUMN_DETAILS, &track,
                      -1);
  switch (column) {
  case COLUMN_TITLE:
    g_free (track->title);
    track->title = g_strdup (string);
    gtk_list_store_set (track_store, &iter, COLUMN_TITLE, track->title, -1);
    break;
  case COLUMN_ARTIST:
    g_free (track->artist);
    track->artist = g_strdup (string);
    gtk_list_store_set (track_store, &iter, COLUMN_ARTIST, track->artist, -1);
    break;
  default:
    g_warning (_("Unknown column %d was edited"), column);
  }
  g_free (artist);
  g_free (title);

  return;
}

/*
 * Remove all of the data which is intrinsic to the album being correctly
 * detected, such as MusicBrainz IDs, ASIN, and Wikipedia links.
 */
static void
remove_musicbrainz_ids (AlbumDetails *album)
{
  GList *l;
#define UNSET(id) g_free (album->id);           \
  album->id = NULL;

  UNSET (album_id);
  UNSET (artist_id);
  UNSET (asin);
  UNSET (discogs);
  UNSET (wikipedia);
#undef UNSET

#define UNSET(id) g_free (track->id);           \
  track->id = NULL;

  for (l = album->tracks; l; l = l->next) {
    TrackDetails *track = l->data;
    UNSET (track_id);
    UNSET (artist_id);
  }
#undef UNSET
}

G_MODULE_EXPORT void on_title_edit_changed(GtkEditable *widget, gpointer user_data) {
  g_return_if_fail (current_album != NULL);

  remove_musicbrainz_ids (current_album);

  if (current_album->title) {
    g_free (current_album->title);
  }
  current_album->title = gtk_editable_get_chars (widget, 0, -1); /* get all the characters */
}

G_MODULE_EXPORT void on_artist_edit_changed(GtkEditable *widget, gpointer user_data) {
  GtkTreeIter iter;
  TrackDetails *track;
  gchar *current_track_artist, *former_album_artist = NULL;

  g_return_if_fail (current_album != NULL);

  remove_musicbrainz_ids (current_album);

  /* Unset the sortable artist field, as we can't change it automatically */
  if (current_album->artist_sortname) {
    g_free (current_album->artist_sortname);
    current_album->artist_sortname = NULL;
  }

  if (current_album->artist) {
    former_album_artist = current_album->artist;
  }
  current_album->artist = gtk_editable_get_chars (widget, 0, -1); /* get all the characters */

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (track_store), &iter)) {
    g_free (former_album_artist);
    return;
  }

  /* Set the artist field in each tree row */
  do {
    gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter, COLUMN_ARTIST, &current_track_artist, -1);
    /* Change track artist if it matched album artist before the change */
    if ((strcasecmp (current_track_artist, former_album_artist) == 0) || (strcasecmp (current_track_artist, current_album->artist) == 0)) {
      gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter, COLUMN_DETAILS, &track, -1);

      g_free (track->artist);
      track->artist = g_strdup (current_album->artist);

      if (track->artist_sortname) {
        g_free (track->artist_sortname);
        track->artist_sortname = NULL;
      }

      gtk_list_store_set (track_store, &iter, COLUMN_ARTIST, track->artist, -1);
    }
  } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (track_store), &iter));

  g_free (former_album_artist);
}

G_MODULE_EXPORT void on_genre_edit_changed(GtkEditable *widget, gpointer user_data) {
  g_return_if_fail (current_album != NULL);
  if (current_album->genre) {
    g_free (current_album->genre);
  }
  current_album->genre = gtk_editable_get_chars (widget, 0, -1); /* get all the characters */
}

G_MODULE_EXPORT void on_year_edit_changed(GtkEditable *widget, gpointer user_data) {
  const gchar* yearstr;
  int year;

  g_return_if_fail (current_album != NULL);

  yearstr = gtk_entry_get_text (GTK_ENTRY (widget));
  year = atoi (yearstr);
  if (year > 0) {
    if (current_album->release_date) {
      g_date_set_dmy (current_album->release_date, 1, 1, year);
    } else {
      current_album->release_date = g_date_new_dmy (1, 1, year);
    }
  }
}

G_MODULE_EXPORT void on_disc_number_edit_changed(GtkEditable *widget, gpointer user_data) {
    const gchar* discstr;
    int disc_number;

    g_return_if_fail (current_album != NULL);
    discstr = gtk_entry_get_text (GTK_ENTRY (widget));
    disc_number = atoi(discstr);
    current_album->disc_number = disc_number;
}

G_MODULE_EXPORT void on_contents_activate(GtkWidget *button, gpointer user_data) {
  GError *error = NULL;

  gtk_show_uri (NULL, "ghelp:sound-juicer", GDK_CURRENT_TIME, &error);
  if (error) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (GTK_WINDOW (gtkpod_app),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Could not display help for Sound Juicer\n"
                                       "%s"),
                                     error->message);
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_widget_show (dialog);
    g_error_free (error);
  }
}

static void
on_message_received (const char *message, gpointer user_data)
{
  if (message == NULL)
    return;
  if (strcmp (RAISE_WINDOW, message) == 0) {
    gtk_window_present (GTK_WINDOW (gtkpod_app));
  }
}

/**
 * Performs various checks to ensure CD duplication is available.
 * If this is found TRUE is returned, otherwise FALSE is returned.
 */
static gboolean
is_cd_duplication_available()
{
  /* First check the brasero tool is available in the path */
  gchar* brasero_cd_burner = g_find_program_in_path ("brasero");
  if (brasero_cd_burner == NULL) {
    return FALSE;
  }
  g_free(brasero_cd_burner);

  /* Second check the cdrdao tool is available in the path */
  gchar* cdrdao = g_find_program_in_path ("cdrdao");
  if (cdrdao == NULL) {
    return FALSE;
  }
  g_free(cdrdao);

  /* Now check that there is at least one cd recorder available */
  BraseroMediumMonitor     *monitor;
  GSList		   *drives;
  GSList		   *iter;

  monitor = brasero_medium_monitor_get_default ();
  drives = brasero_medium_monitor_get_drives (monitor, BRASERO_DRIVE_TYPE_ALL);

  for (iter = drives; iter; iter = iter->next) {
    BraseroDrive *drive;

    drive = iter->data;
    if (brasero_drive_can_write (drive)) {
      g_slist_foreach (drives, (GFunc) g_object_unref, NULL);
      g_slist_free (drives);
      return TRUE;
    }
  }

  g_slist_foreach (drives, (GFunc) g_object_unref, NULL);
  g_slist_free (drives);
  return FALSE;
}

/**
 * Clicked on duplicate in the UI (button/menu)
 */
G_MODULE_EXPORT void on_duplicate_activate (GtkWidget *button, gpointer user_data)
{
  GError *error = NULL;
  const gchar* device;

  device = brasero_drive_get_device (drive);
  if (!g_spawn_command_line_async (g_strconcat ("brasero -c ", device, NULL), &error)) {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtkpod_app),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "<b>%s</b>\n\n%s\n%s: %s",
                                                   _("Could not duplicate disc"),
                                                   _("Sound Juicer could not duplicate the disc"),
                                                   _("Reason"),
                                                   error->message);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      g_error_free (error);
  }
}

/**
 * Sets the duplication buttons sensitive property if duplication is enabled.
 * This is setup in the main entry point.
 */
static void set_duplication(gboolean enabled)
{
  if (duplication_enabled) {
    gtk_widget_set_sensitive (duplicate, enabled);
  }
}

GtkWidget *sj_create_sound_juicer()
{
  gchar *glade_path;
  GtkWidget *w;
  GError *error = NULL;
  GtkTreeSelection *selection;
  char *device = NULL, **uris = NULL;
  GSettings *http_settings;

  g_setenv ("PULSE_PROP_media.role", "music", TRUE);

  connection = bacon_message_connection_new ("sound-juicer");
  if (bacon_message_connection_get_is_server (connection) == FALSE) {
    bacon_message_connection_send (connection, RAISE_WINDOW);
    bacon_message_connection_free (connection);
    return NULL;
  } else {
    bacon_message_connection_set_callback (connection, on_message_received, NULL);
  }

  brasero_media_library_start ();

  metadata = sj_metadata_getter_new ();
  g_signal_connect (metadata, "metadata", G_CALLBACK (metadata_cb), NULL);

  sj_settings = g_settings_new (SJCD_SCHEMA);
  if (sj_settings == NULL) {
    g_warning (_("Could not create GSettings object.\n"));
    return NULL;
  }

  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_DEVICE,
                    (GCallback)device_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_EJECT,
                    (GCallback)eject_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_OPEN,
                    (GCallback)open_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_BASEURI,
                    (GCallback)baseuri_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_STRIP,
                    (GCallback)strip_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_AUDIO_PROFILE,
                    (GCallback)profile_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_PARANOIA,
                    (GCallback)paranoia_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_PATH_PATTERN,
                    (GCallback)path_pattern_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_FILE_PATTERN,
                    (GCallback)file_pattern_changed_cb, NULL);
  g_signal_connect (sj_settings, "changed::"SJ_SETTINGS_AUDIO_VOLUME,
                    (GCallback)audio_volume_changed_cb, NULL);

  http_settings = g_settings_new ("org.gnome.system.proxy.http");
  if (http_settings == NULL) {
    g_warning (_("Could not create GSettings object.\n"));
    exit (1);
  }
  g_signal_connect (http_settings, "changed::"SJ_SETTINGS_HTTP_PROXY_ENABLE,
                    (GCallback)http_proxy_enable_changed_cb, NULL);
  g_signal_connect (http_settings, "changed::"SJ_SETTINGS_HTTP_PROXY,
                    (GCallback)http_proxy_changed_cb, NULL);
  g_signal_connect (http_settings, "changed::"SJ_SETTINGS_HTTP_PROXY_PORT,
                    (GCallback)http_proxy_port_changed_cb, NULL);

  glade_path = g_build_filename(get_glade_dir(), "sjcd.xml", NULL);
  builder = gtkpod_builder_xml_new(glade_path);
  g_free(glade_path);

  gtk_builder_connect_signals (builder, NULL);

  w                             = GET_WIDGET ("main_window");
  vbox1                      = GET_WIDGET ("vbox1");
  g_object_ref(vbox1);
  gtk_container_remove(GTK_CONTAINER(w), vbox1);
  gtk_widget_destroy(w);

  message_area_eventbox = GET_WIDGET ("message_area_eventbox");
  select_all_menuitem   = GET_WIDGET ("select_all");
  deselect_all_menuitem = GET_WIDGET ("deselect_all");
  submit_menuitem       = GET_WIDGET ("submit");
  title_entry           = GET_WIDGET ("title_entry");
  artist_entry          = GET_WIDGET ("artist_entry");
  duration_label        = GET_WIDGET ("duration_label");
  genre_entry           = GET_WIDGET ("genre_entry");
  year_entry            = GET_WIDGET ("year_entry");
  disc_number_entry     = GET_WIDGET ("disc_number_entry");
  track_listview        = GET_WIDGET ("track_listview");
  extract_button        = GET_WIDGET ("extract_button");
  extract_menuitem      = GET_WIDGET ("extract_menuitem");
  status_bar            = GET_WIDGET ("status_bar");
  duplicate             = GET_WIDGET ("duplicate_menuitem");
  eject                 = GET_WIDGET ("eject");


  setup_genre_entry (genre_entry);

  track_store = gtk_list_store_new (COLUMN_TOTAL, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
  gtk_tree_view_set_model (GTK_TREE_VIEW (track_listview), GTK_TREE_MODEL (track_store));
  {
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    toggle_renderer = gtk_cell_renderer_toggle_new ();
    g_signal_connect (toggle_renderer, "toggled", G_CALLBACK (on_extract_toggled), NULL);
    column = gtk_tree_view_column_new_with_attributes ("",
                                                       toggle_renderer,
                                                       "active", COLUMN_EXTRACT,
                                                       NULL);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (track_listview), column);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Track"));
    gtk_tree_view_column_set_expand (column, FALSE);
    gtk_tree_view_column_set_resizable (column, FALSE);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "text", COLUMN_NUMBER);
    renderer = gtk_cell_renderer_pixbuf_new ();
    g_object_set (renderer, "stock-size", GTK_ICON_SIZE_MENU, "xalign", 0.0, NULL);
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, number_cell_icon_data_cb, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (track_listview), column);

    title_renderer = gtk_cell_renderer_text_new ();
    g_signal_connect (title_renderer, "edited", G_CALLBACK (on_cell_edited), GUINT_TO_POINTER (COLUMN_TITLE));
    g_object_set (G_OBJECT (title_renderer), "editable", TRUE, NULL);
    column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                       title_renderer,
                                                       "text", COLUMN_TITLE,
                                                       NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (track_listview), column);

    artist_renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                       artist_renderer,
                                                       "text", COLUMN_ARTIST,
                                                       NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_expand (column, TRUE);
    g_signal_connect (artist_renderer, "edited", G_CALLBACK (on_cell_edited), GUINT_TO_POINTER (COLUMN_ARTIST));
    g_object_set (G_OBJECT (artist_renderer), "editable", TRUE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (track_listview), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Duration"),
                                                       renderer,
                                                       NULL);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, duration_cell_data_cb, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (track_listview), column);
  }

  extractor = SJ_EXTRACTOR (sj_extractor_new());
  error = sj_extractor_get_new_error (extractor);
  if (error) {
    error_on_start (error);
    return NULL;
  }

  update_ui_for_album (NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (track_listview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  http_proxy_setup (http_settings);
  baseuri_changed_cb (sj_settings, SJ_SETTINGS_BASEURI, NULL);
  path_pattern_changed_cb (sj_settings, SJ_SETTINGS_PATH_PATTERN, NULL);
  file_pattern_changed_cb (sj_settings, SJ_SETTINGS_FILE_PATTERN, NULL);
  profile_changed_cb (sj_settings, SJ_SETTINGS_AUDIO_PROFILE, NULL);
  paranoia_changed_cb (sj_settings, SJ_SETTINGS_PARANOIA, NULL);
  strip_changed_cb (sj_settings, SJ_SETTINGS_STRIP, NULL);
  eject_changed_cb (sj_settings, SJ_SETTINGS_EJECT, NULL);
  open_changed_cb (sj_settings, SJ_SETTINGS_OPEN, NULL);
  audio_volume_changed_cb (sj_settings, SJ_SETTINGS_AUDIO_VOLUME, NULL);
  if (device == NULL && uris == NULL) {
    /* FIXME: this should set the device gsettings key to a meaningful
     * value if it's empty (which is the case until the user changes it in
     * the prefs)
     */
    device_changed_cb (sj_settings, SJ_SETTINGS_DEVICE, NULL);
  } else {
    if (device) {
#ifdef __sun
      if (strstr(device, "/dev/dsk/") != NULL ) {
        device = g_strdup_printf("/dev/rdsk/%s", device + strlen("/dev/dsk/"));
      }
#endif
      set_device (device, TRUE);
    } else {
      char *d;

      /* Mash up the CDDA URIs into a device path */
      if (g_str_has_prefix (uris[0], "cdda://")) {
        gint len;
#ifdef __sun
        d = g_strdup_printf ("/dev/rdsk/%s", uris[0] + strlen ("cdda://"));
#else
        d = g_strdup_printf ("/dev/%s%c", uris[0] + strlen ("cdda://"), '\0');
#endif
        /* Take last '/' out of path, or set_device thinks it is part of the device name */
        len = strlen (d);
        if (d[len - 1] == '/')
            d [len - 1] = '\0';
        set_device (d, TRUE);
        g_free (d);
      } else {
        device_changed_cb (sj_settings, SJ_SETTINGS_DEVICE, NULL);
      }
    }
  }

  if (sj_extractor_supports_encoding (&error) == FALSE) {
    error_on_start (error);
    return NULL;
  }

  /* Set whether duplication of a cd is available using the brasero tool */
  gtk_widget_set_sensitive (duplicate, FALSE);
  duplication_enabled = is_cd_duplication_available();

  g_object_unref (base_uri);
  g_object_unref (metadata);
  g_object_unref (extractor);
  g_object_unref (sj_settings);
  g_object_unref (http_settings);
  brasero_media_library_stop ();

  return vbox1;
}
