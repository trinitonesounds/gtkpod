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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 *          Mike Hearn  <mike@theoretic.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc.h"
#include "plugin.h"

#include "sound-juicer.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <brasero-medium-selection.h>
#include <brasero-volume.h>
#include <gst/gst.h>
#include <gst/pbutils/encoding-profile.h>

#include "rb-gst-media-types.h"
#include "sj-metadata.h"
#include "sj-metadata-getter.h"
#include "sj-extractor.h"
#include "sj-structures.h"
#include "sj-error.h"
#include "sj-util.h"
#include "sj-main.h"
#include "sj-prefs.h"
#include "sj-genres.h"

static void reread_cd (gboolean ignore_no_media);
static void update_ui_for_album (AlbumDetails *album);

/* Prototypes for the signal blocking/unblocking in update_ui_for_album */
G_MODULE_EXPORT void on_title_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_person_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_year_edit_changed(GtkEditable *widget, gpointer user_data);
G_MODULE_EXPORT void on_disc_number_edit_changed(GtkEditable *widget, gpointer user_data);

GtkBuilder *builder;

SjMetadataGetter *metadata;
SjExtractor *extractor;

GSettings *sj_settings;

/* 
 * Added since gtkpod lacks a built-in GActionGroup/GActionMap
 * due to it not being a GtkApplication
 */
static GSimpleActionGroup *action_group;

static GtkWidget *vbox1;
static GtkWidget *message_area_eventbox;
static GtkWidget *title_entry, *artist_entry, *composer_label, *composer_entry, *duration_label, *genre_entry, *year_entry, *disc_number_entry;
static GtkWidget *entry_table; /* GtkTable containing composer_entry */
static GtkTreeViewColumn *composer_column; /* Treeview column containing composers */
static GtkWidget *track_listview, *extract_button, *select_button;
static GtkWidget *status_bar;
GtkListStore *track_store;
GtkCellRenderer *toggle_renderer, *title_renderer, *artist_renderer, *composer_renderer;

GtkWidget *current_message_area;

char *path_pattern, *file_pattern;
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

static char *device = NULL, **uris = NULL;

static guint debug_flags = 0;

#define DEFAULT_PARANOIA 15
#define RAISE_WINDOW "raise-window"
#define SJCD_SCHEMA "org.gtkpod.sjcd"
#define COMPOSER_ROW 2 /* Row of entry_table containing composer_entry */

void sj_debug (SjDebugDomain domain, const gchar* format, ...)
{
  va_list args;
  gchar *string;

  if (debug_flags & domain) {
    va_start (args, format);
    string = g_strdup_vprintf (format, args);
    va_end (args);
    g_printerr ("%s", string);
    g_free (string);
  }
}

static void sj_debug_init (void)
{
  const char *str;
  const GDebugKey debug_keys[] = {
    { "cd", DEBUG_CD },
    { "metadata", DEBUG_METADATA },
    { "playing", DEBUG_PLAYING },
    { "extracting", DEBUG_EXTRACTING }
  };

  str = g_getenv ("SJ_DEBUG");
  if (str) {
    debug_flags = g_parse_debug_string (str, debug_keys, G_N_ELEMENTS (debug_keys));
  }
}

static void error_on_start (GError *error)
{
  gtkpod_statusbar_message("Could not start sound juicer because %s", error->message);
  g_error_free(error);
}

/**
 * Clicked Eject
 */
static void on_eject_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  brasero_drive_eject (drive, FALSE, NULL);
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

static void on_select_all_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (track_store), select_all_foreach_cb, GINT_TO_POINTER (TRUE));
  gtk_widget_set_sensitive (extract_button, TRUE);
  set_action_enabled ("select-all", FALSE);
  set_action_enabled ("deselect-all", TRUE);

  gtk_actionable_set_action_name(GTK_ACTIONABLE(select_button), "win.deselect-all");
  gtk_button_set_label(GTK_BUTTON(select_button), _("Select None"));
  no_of_tracks_selected = total_no_of_tracks;
}

static void on_deselect_all_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (track_store), select_all_foreach_cb, GINT_TO_POINTER (FALSE));
  gtk_widget_set_sensitive (extract_button, FALSE);
  set_action_enabled ("deselect-all", FALSE);
  set_action_enabled ("select-all", TRUE);

  gtk_actionable_set_action_name(GTK_ACTIONABLE(select_button), "win.select-all");
  gtk_button_set_label(GTK_BUTTON(select_button), _("Select All"));
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
    g_object_set (G_OBJECT (cell), "icon-name", NULL, NULL);
    break;
  case STATE_PLAYING:
    {
      gboolean rtl = gtk_widget_get_direction (track_listview) == GTK_TEXT_DIR_RTL;
      gchar *name = rtl ? "media-playback-start-rtl" : "media-playback-start";
      g_object_set (G_OBJECT (cell), "icon-name", name, NULL);
    }
    break;
  case STATE_PAUSED:
    g_object_set (G_OBJECT (cell), "icon-name", "media-playback-pause", NULL);
    break;
  case STATE_EXTRACTING:
    g_object_set (G_OBJECT (cell), "icon-name", "media-record", NULL);
    break;
  default:
    g_warning("Unhandled track state %d\n", state);
  }
}

/* Taken from gedit */
static void
set_info_bar_text_and_icon (GtkInfoBar  *infobar,
                            const gchar *icon_name,
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

  hbox_content = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_show (hbox_content);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
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
                           _("Ca_ncel"), GTK_RESPONSE_CANCEL);

  /* Translators: title, artist */
  primary_text = g_strdup_printf (_("Could not find %s by %s on MusicBrainz."), title, artist);

  set_info_bar_text_and_icon (GTK_INFO_BAR (infobar),
                              "dialog-information",
                              primary_text,
                              _("You can improve the MusicBrainz database by adding this album."),
                              button);

  g_free (primary_text);

  return infobar;
}

/**
 * Clicked the Submit menu item in the UI
 */
static void on_submit_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
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

static void on_preferences_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  show_preferences_dialog ();
}

/**
 * Clicked on duplicate in the UI (button/menu)
 */
static void on_duplicate_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
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

static void
musicbrainz_submit_info_bar_response (GtkInfoBar *infobar,
                                      int         response_id,
                                      gpointer    user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    on_submit_activate (NULL, NULL, NULL);
  }

  set_message_area (message_area_eventbox, NULL);
}
#define TABLE_ROW_SPACING 6 /* spacing of rows in entry_table */

/*
 * Show composer entry and composer column in track listview
 */
static void
show_composer_fields (void)
{
  if (!gtk_widget_get_visible (GTK_WIDGET (composer_label))) {
    gtk_table_set_row_spacing (GTK_TABLE (entry_table), COMPOSER_ROW,
                               TABLE_ROW_SPACING);
    gtk_widget_show (GTK_WIDGET (composer_entry));
    gtk_widget_show (GTK_WIDGET (composer_label));
    gtk_tree_view_column_set_visible (composer_column, TRUE);
  }
}

#undef TABLE_ROW_SPACING

/*
 * Hide composer entry and composer column in track listview
 */
static void
hide_composer_fields (void)
{
  if (gtk_widget_get_visible (GTK_WIDGET (composer_label))) {
    gtk_table_set_row_spacing (GTK_TABLE (entry_table), COMPOSER_ROW, 0);
    gtk_widget_hide (GTK_WIDGET (composer_entry));
    gtk_widget_hide (GTK_WIDGET (composer_label));
    gtk_tree_view_column_set_visible (composer_column, FALSE);
  }
}

/*
 * Determine if the composer fields should be shown based on genre,
 * always show composer fields if they are non-empty.
 */
static void
composer_show_hide (const char* genre)
{
  const static char *composer_genres[] = {
    N_("Classical"), N_("Lieder"), N_("Opera"), N_("Chamber"), N_("Musical")
  };  /* Genres for which the composer fields should be shown. */
#define COUNT (G_N_ELEMENTS (composer_genres))
  static char *genres[COUNT]; /* store localized genre names */
  static gboolean init = FALSE; /* TRUE if localized genre names initalized*/
  gboolean composer_show = FALSE;
  int i;
  GList* l;
  char *folded_genre;

  if (composer_column == NULL)
    return;

  if (!init) {
    for (i = 0; i < COUNT; i++)
      genres[i] = g_utf8_casefold (gettext (composer_genres[i]), -1);

    init = TRUE;
  }

  composer_show = !sj_str_is_empty (current_album->composer);
  for (l = current_album->tracks; l; l = g_list_next (l)) {
    if (!sj_str_is_empty (((TrackDetails*) (l->data))->composer) == TRUE) {
      composer_show = TRUE;
      break;
    }
  }

  folded_genre = g_utf8_casefold (genre, -1);
  for (i = 0; i < COUNT; i++) {
    if (g_str_equal (folded_genre, genres[i])) {
      composer_show = TRUE;
      break;
    }
  }
  g_free (folded_genre);

  if (composer_show)
    show_composer_fields ();
  else
    hide_composer_fields ();
  return;
}
#undef COUNT

/**
 * Utility function to update the UI for a given Album
 */
static void update_ui_for_album (AlbumDetails *album)
{
  GList *l;
  int album_duration = 0;
  char* duration_text;
  total_no_of_tracks=0;

  hide_composer_fields ();

  if (album == NULL) {
    gtk_list_store_clear (track_store);
    gtk_entry_set_text (GTK_ENTRY (title_entry), "");
    gtk_entry_set_text (GTK_ENTRY (artist_entry), "");
    gtk_entry_set_text (GTK_ENTRY (composer_entry), "");
    gtk_entry_set_text (GTK_ENTRY (genre_entry), "");
    gtk_entry_set_text (GTK_ENTRY (year_entry), "");
    gtk_entry_set_text (GTK_ENTRY (disc_number_entry), "");
    gtk_label_set_text (GTK_LABEL (duration_label), "");
    gtk_widget_set_sensitive (title_entry, FALSE);
    gtk_widget_set_sensitive (artist_entry, FALSE);
    gtk_widget_set_sensitive (composer_entry, FALSE);
    gtk_widget_set_sensitive (genre_entry, FALSE);
    gtk_widget_set_sensitive (year_entry, FALSE);
    gtk_widget_set_sensitive (disc_number_entry, FALSE);
    gtk_widget_set_sensitive (extract_button, FALSE);
    set_action_enabled ("select-all", FALSE);
    set_action_enabled ("deselect-all", FALSE);
    set_action_enabled ("duplicate", FALSE);

    set_message_area (message_area_eventbox, NULL);
  } else {
    gtk_list_store_clear (track_store);

    g_signal_handlers_block_by_func (title_entry, on_title_edit_changed, NULL);
    g_signal_handlers_block_by_func (artist_entry, on_person_edit_changed, NULL);
    g_signal_handlers_block_by_func (composer_entry, on_person_edit_changed, NULL);
    g_signal_handlers_block_by_func (year_entry, on_year_edit_changed, NULL);
    g_signal_handlers_block_by_func (disc_number_entry, on_disc_number_edit_changed, NULL);
    gtk_entry_set_text (GTK_ENTRY (title_entry), album->title);
    gtk_entry_set_text (GTK_ENTRY (artist_entry), album->artist);
    if (!sj_str_is_empty (album->composer)) {
      gtk_entry_set_text (GTK_ENTRY (composer_entry), album->composer);
      show_composer_fields ();
    } else {
      gtk_entry_set_text (GTK_ENTRY (composer_entry), "");
    }
    if (album->disc_number) {
      gchar *disc_number = g_strdup_printf ("%d", album->disc_number);
      gtk_entry_set_text (GTK_ENTRY (disc_number_entry), disc_number);
      g_free (disc_number);
    }
    if (album->release_date && gst_date_time_has_year (album->release_date)) {
      gchar *release_date =  g_strdup_printf ("%d", gst_date_time_get_year (album->release_date));
      gtk_entry_set_text (GTK_ENTRY (year_entry), release_date);
      g_free (release_date);
    }
    g_signal_handlers_unblock_by_func (title_entry, on_title_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (artist_entry, on_person_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (composer_entry, on_person_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (year_entry, on_year_edit_changed, NULL);
    g_signal_handlers_unblock_by_func (disc_number_entry, on_disc_number_edit_changed, NULL);
    /* Clear the genre field, it's from the user */
    gtk_entry_set_text (GTK_ENTRY (genre_entry), "");

    gtk_widget_set_sensitive (title_entry, TRUE);
    gtk_widget_set_sensitive (artist_entry, TRUE);
    gtk_widget_set_sensitive (composer_entry, TRUE);
    gtk_widget_set_sensitive (genre_entry, TRUE);
    gtk_widget_set_sensitive (year_entry, TRUE);
    gtk_widget_set_sensitive (disc_number_entry, TRUE);
    gtk_widget_set_sensitive (extract_button, TRUE);
    set_action_enabled ("select-all", FALSE);
    set_action_enabled ("deselect-all", TRUE);
    set_action_enabled ("duplicate", TRUE);

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
                          COLUMN_COMPOSER, track->composer,
                          COLUMN_DURATION, track->duration,
                          COLUMN_DETAILS, track,
                          -1);
      if (!sj_str_is_empty (track->composer))
        show_composer_fields ();
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
 * NULL safe utility to collate utf8 strings
 */
static gint collate (const char *a, const char *b)
{
  gint ret_val = 0;

  if (a) {
    if (b) {
      ret_val = g_utf8_collate (a, b);
    } else {
      ret_val = 1;
    }
  } else if (b) {
    ret_val = -1;
  }
  return ret_val;
}

static gint sj_gst_date_time_compare_field (GstDateTime *lhs, GstDateTime *rhs,
                                            gboolean (*has_field) (const GstDateTime *datetime),
                                            gint (*get_field) (const GstDateTime *datetime))
{
  gint field_lhs = -1;
  gint field_rhs = -1;

  if (has_field (lhs)) {
    field_lhs = get_field (lhs);
  }
  if (has_field (rhs)) {
    field_rhs = get_field (rhs);
  }

  return (field_lhs - field_rhs);
}

static gint sj_gst_date_time_compare (gpointer lhs, gpointer rhs)
{
  GstDateTime *date_lhs = (GstDateTime *)lhs;
  GstDateTime *date_rhs = (GstDateTime *)rhs;

  int comparison;

  comparison = sj_gst_date_time_compare_field (date_lhs, date_rhs,
                                               gst_date_time_has_year,
                                               gst_date_time_get_year);
  if (comparison != 0) {
      return comparison;
  }

  comparison = sj_gst_date_time_compare_field (date_lhs, date_rhs,
                                               gst_date_time_has_month,
                                               gst_date_time_get_month);
  if (comparison != 0) {
      return comparison;
  }

  comparison = sj_gst_date_time_compare_field (date_lhs, date_rhs,
                                               gst_date_time_has_day,
                                               gst_date_time_get_day);

  return comparison;
}

/**
 * Utility function to sort albums in multiple_album_dialog
 */
static gint sort_release_info (GtkTreeModel *model, GtkTreeIter *a,
                               GtkTreeIter *b, gpointer user_data)
{
  AlbumDetails *album_a, *album_b;
  GList *label_a, *label_b;
  gint ret_val = 0;
  const gint column = GPOINTER_TO_INT (user_data);

  gtk_tree_model_get (model, a, column, &album_a, -1);
  gtk_tree_model_get (model, b, column, &album_b, -1);

  ret_val = collate (album_a->title, album_b->title);
  if (ret_val)
    return ret_val;

  ret_val = collate (album_a->artist_sortname, album_b->artist_sortname);
  if (ret_val)
    return ret_val;

  ret_val = collate (album_a->country, album_b->country);
  if (ret_val)
    return ret_val;

  if (album_a->release_date) {
    if (album_b->release_date) {
      ret_val = sj_gst_date_time_compare (album_a->release_date, album_b->release_date);
      if (ret_val)
        return ret_val;
    } else {
      return -1;
    }
  } else if (album_b->release_date) {
    return 1;
  }

  label_a = album_a->labels;
  label_b = album_b->labels;
  while (label_a && label_b) {
    LabelDetails *a = label_a->data;
    LabelDetails *b = label_b->data;
    ret_val = collate (a->sortname,b->sortname);
    if (ret_val)
      return ret_val;

    label_a = label_a->next;
    label_b = label_b->next;
  }
  if (label_a && !label_b)
    return -1;
  if (!label_a && label_b)
    return 1;

  ret_val = (album_a->disc_number < album_b->disc_number) ? -1 :
    ((album_a->disc_number > album_b->disc_number) ? 1 : 0);
  if (ret_val)
    return ret_val;

  return (album_a->disc_count < album_b->disc_count) ? -1 :
    ((album_a->disc_count > album_b->disc_count) ? 1 : 0);
}


/**
 * Utility function to format label string for multiple_album_dialog
 */
static GString* format_label_text (GList* labels)
{
  int count;
  GString *label_text;

  if (labels == NULL)
    return NULL;

  label_text = g_string_new (NULL);
  count = g_list_length (labels);
  while (count > 2) {
    g_string_append (label_text, ((LabelDetails*)labels->data)->name);
    g_string_append (label_text, ", ");
    labels = labels->next;
    count--;
  }

  if (count > 1) {
    g_string_append (label_text, ((LabelDetails*)labels->data)->name);
    g_string_append (label_text, " & ");
  }

  g_string_append (label_text, ((LabelDetails*)labels->data)->name);

  return label_text;
}

/**
 * Utility function for multiple_album_dialog to format the
 * release label, date and country.
 */
static char *format_release_details (AlbumDetails *album)
{
  gchar *details;
  GString *label_text = NULL;

  if (album->labels)
    label_text = format_label_text (album->labels);

  if (!sj_str_is_empty (album->country)) {
    if (album->labels) {
      if (album->release_date && gst_date_time_has_year (album->release_date)) {
        /* Translators: this string appears when multiple CDs were
         * found in musicbrainz online database, it corresponds to
         * "Released: <country> in <year> on <label>" */
        details = g_strdup_printf (_("Released: %s in %d on %s"),
                                   album->country,
                                   gst_date_time_get_year (album->release_date),
                                   label_text->str);
      } else {
        /* Translators: this string appears when multiple CDs were
         * found in musicbrainz online database, it corresponds to
         * "Released: <country> on <label>" */
        details = g_strdup_printf (_("Released: %s on %s"), album->country, label_text->str);
      }
    } else if (album->release_date && gst_date_time_has_year (album->release_date)) {
      /* Translators: this string appears when multiple CDs were
       * found in musicbrainz online database, it corresponds to
       * "Released: <country> in <year>" */
      details = g_strdup_printf (_("Released: %s in %d"), album->country,
                                 gst_date_time_get_year (album->release_date));
    } else {
      /* Translators: this string appears when multiple CDs were
       * found in musicbrainz online database, it corresponds to
       * "Released: <country>" */
      details = g_strdup_printf (_("Released: %s"), album->country);
    }
  } else if (album->release_date && gst_date_time_has_year (album->release_date)) {
    if (album->labels) {
        /* Translators: this string appears when multiple CDs were
         * found in musicbrainz online database, it corresponds to
         * "Released in <year> on <label>" */
        details = g_strdup_printf (_("Released in %d on %s"),
                                   gst_date_time_get_year (album->release_date),
                                   label_text->str);
    } else {
        /* Translators: this string appears when multiple CDs were
         * found in musicbrainz online database, it corresponds to
         * "Released in <year>" */
        details = g_strdup_printf(_("Released in %d"),
                                  gst_date_time_get_year (album->release_date));
    }
  } else if (album->labels) {
    /* Translators: this string appears when multiple CDs were
     * found in musicbrainz online database, it corresponds to
     * "Released on <label>" */
    details = g_strdup_printf (_("Released on %s"), label_text->str);
  } else {
    details = _("Release label, year & country unknown");
  }

  if (label_text)
    g_string_free (label_text, TRUE);

  return details;
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
  enum COLUMNS
  {
    COLUMN_TITLE,
    COLUMN_ARTIST,
    COLUMN_RELEASE_DETAILS,
    COLUMN_DETAILS,
    COLUMN_COUNT
  };

  if (dialog == NULL) {
    GtkTreeViewColumn *column = gtk_tree_view_column_new ();
    GtkCellArea *cell_area = gtk_cell_area_box_new ();
    GtkCellRenderer *title_renderer  = gtk_cell_renderer_text_new ();
    GtkCellRenderer *artist_renderer = gtk_cell_renderer_text_new ();
    GtkCellRenderer *release_details_renderer   = gtk_cell_renderer_text_new ();

    dialog = GET_WIDGET ("multiple_dialog");
    g_assert (dialog != NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_app));
    albums_listview = GET_WIDGET ("albums_listview");
    ok_button       = GET_WIDGET ("ok_button");

    g_object_get (G_OBJECT (column), "cell-area", &cell_area, NULL);
    g_assert (cell_area != NULL);
    gtk_orientable_set_orientation (GTK_ORIENTABLE (cell_area),
                                    GTK_ORIENTATION_VERTICAL);
    gtk_tree_view_column_set_title (column, _("Albums"));
    gtk_tree_view_column_pack_start (column, title_renderer,  TRUE);
    gtk_tree_view_column_pack_start (column, artist_renderer, TRUE);
    gtk_tree_view_column_pack_start (column, release_details_renderer,   TRUE);
    g_object_set(title_renderer, "weight", PANGO_WEIGHT_BOLD, "weight-set",
                 TRUE, NULL);
    g_object_set(artist_renderer, "style", PANGO_STYLE_ITALIC, "style-set",
                 TRUE, NULL);
    gtk_tree_view_column_add_attribute (column, title_renderer,  "text",
                                        COLUMN_TITLE);
    gtk_tree_view_column_add_attribute (column, artist_renderer, "text",
                                        COLUMN_ARTIST);
    gtk_tree_view_column_add_attribute (column, release_details_renderer,   "text",
                                        COLUMN_RELEASE_DETAILS);

    g_signal_connect (albums_listview, "row-activated",
                      G_CALLBACK (album_row_activated), dialog);

    albums_store = gtk_list_store_new (COLUMN_COUNT, G_TYPE_STRING,
                                       G_TYPE_STRING, G_TYPE_STRING,
                                       G_TYPE_POINTER);
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (albums_store),
                                     COLUMN_DETAILS, sort_release_info,
                                     GINT_TO_POINTER (COLUMN_DETAILS), NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (albums_listview), column);

    gtk_tree_view_set_model (GTK_TREE_VIEW (albums_listview),
                             GTK_TREE_MODEL (albums_store));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (albums_listview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    gtk_widget_set_sensitive (ok_button, FALSE);
    g_signal_connect (selection, "changed", (GCallback)selected_album_changed,
                      ok_button);
  }

  gtk_list_store_clear (albums_store);
  for (; albums ; albums = g_list_next (albums)) {
    GtkTreeIter iter;
    AlbumDetails *album = (AlbumDetails*)(albums->data);
    GString *album_title = g_string_new (album->title);
    gchar *release_details = format_release_details (album);

    if (album->disc_number > 0 && album->disc_count > 1)
      g_string_append_printf (album_title,_(" (Disc %d/%d)"),
                              album->disc_number, album->disc_count);
    gtk_list_store_append (albums_store, &iter);
    gtk_list_store_set (albums_store, &iter,
                        COLUMN_TITLE, album_title->str,
                        COLUMN_ARTIST, album->artist,
                        COLUMN_RELEASE_DETAILS, release_details,
                        COLUMN_DETAILS, album,
                        -1);

    g_string_free (album_title, TRUE);
    g_free (release_details);
  }

  /* Sort the model */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (albums_store),
                                        COLUMN_DETAILS, GTK_SORT_ASCENDING);

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
    gtk_tree_model_get (GTK_TREE_MODEL (albums_store), &iter, COLUMN_DETAILS,
                        &album, -1);
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
  if (sj_str_is_empty (value)) {
     base_uri = sj_get_default_music_directory ();
  } else {
    GFileType file_type;
     base_uri = g_file_new_for_uri (value);
    file_type = g_file_query_file_type (base_uri, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);
    if (file_type != G_FILE_TYPE_DIRECTORY) {
      g_object_unref (base_uri);
      base_uri = sj_get_default_music_directory ();
    }
  }
  g_free (value);
}

 /**
  * The GSettings key for the directory pattern changed
  */
static void path_pattern_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  g_assert (strcmp (key, SJ_SETTINGS_PATH_PATTERN) == 0);
  g_free (path_pattern);
  path_pattern = g_settings_get_string (settings, key);
  if (sj_str_is_empty (path_pattern)) {
    g_free (path_pattern);
    path_pattern = g_strdup (sj_get_default_path_pattern ());
  }
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
  if (sj_str_is_empty (file_pattern)) {
    g_free (file_pattern);
    file_pattern = g_strdup (sj_get_default_file_pattern ());
  }
  /* TODO: sanity check the pattern */
}

 /**
 * The GSettings key for the paranoia mode has changed
  */
static void paranoia_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  int value;
  g_assert (strcmp (key, SJ_SETTINGS_PARANOIA) == 0);
  value = g_settings_get_flags (settings, key);

  if (value >= 0) {
    if (value < 32) {
      sj_extractor_set_paranoia (extractor, value);
    } else {
      sj_extractor_set_paranoia (extractor, DEFAULT_PARANOIA);
    }
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
    set_action_enabled ("submit-tracks", TRUE);
  }
  set_action_enabled ("re-read", TRUE);

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
  GdkCursor *cursor;
  GdkWindow *window;
  gboolean realized = gtk_widget_get_realized (GTK_WIDGET(gtkpod_app));

  window = gtk_widget_get_window (GTK_WIDGET(gtkpod_app));

  set_action_enabled ("re-read", FALSE);

  /* Set watch cursor */
  if (realized) {
    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (gtkpod_app)), GDK_WATCH);
    gdk_window_set_cursor (window, cursor);
    g_object_unref (cursor);
    gdk_display_sync (gtk_widget_get_display (GTK_WIDGET (gtkpod_app)));
  }

  /* Set statusbar message */
  gtk_statusbar_push(GTK_STATUSBAR(status_bar), 0, _("Retrieving track listing...please wait."));

  if (!drive)
    sj_debug (DEBUG_CD, "Attempting to re-read NULL drive\n");

  g_free (current_submit_url);
  current_submit_url = NULL;
  set_action_enabled ("submit-tracks", FALSE);

  if (!is_audio_cd (drive)) {
    sj_debug (DEBUG_CD, "Media is not an audio CD\n");
    update_ui_for_album (NULL);
    // TODO Use gtkpod statusbar instead
    gtk_statusbar_pop(GTK_STATUSBAR(status_bar), 0);
    if (realized)
      gdk_window_set_cursor (window, NULL);
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

  sj_debug (DEBUG_CD, "Media added to device %s\n", brasero_drive_get_device (brasero_medium_get_drive (medium)));
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

  sj_debug (DEBUG_CD, "Media removed from device %s\n", brasero_drive_get_device (brasero_medium_get_drive (medium)));
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
    set_action_enabled ("eject", brasero_drive_can_eject (drive));
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
      return;
#endif
    }
  } else {
    device = value;
  }
  set_device (device, ignore_no_media);
  g_free (value);
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
    gtkpod_warning(_("sjcd plugin: the currently selected audio profile is not available on your installation."));
  }

  if (profile != NULL)
    gst_encoding_profile_unref (profile);
}

/**
 * Clicked on Reread in the UI (button/menu)
 */
static void on_reread_activate (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  reread_cd (FALSE);
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
    no_of_tracks_selected++;
  } else {
    /* Reuse the boolean extract */
    extract = FALSE;
    gtk_tree_model_foreach (GTK_TREE_MODEL (track_store), (GtkTreeModelForeachFunc)extract_available_foreach, &extract);
    gtk_widget_set_sensitive (extract_button, extract);
    no_of_tracks_selected--;
  }
  /* Enable and disable the Select/Deselect All buttons */
  if (no_of_tracks_selected == total_no_of_tracks) {
    set_action_enabled ("deselect-all", TRUE);
    set_action_enabled ("select-all", FALSE);
  } else if (no_of_tracks_selected == 0) {
    set_action_enabled ("deselect-all", FALSE);
    set_action_enabled ("select-all", TRUE);
  } else {
    set_action_enabled ("select-all", TRUE);
    set_action_enabled ("deselect-all", TRUE);
  }
}

/**
 * Callback when the title, artist or composer cells are edited in the list.
 * column_data contains the column number in the model which was modified.
 */
static void on_cell_edited (GtkCellRendererText *renderer,
                 gchar *path, gchar *string,
                 gpointer column_data)
{
  ViewColumn column = GPOINTER_TO_INT (column_data);
  GtkTreeIter iter;
  TrackDetails *track;

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (track_store), &iter, path))
    return;
  gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter,
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
    if (track->artist_sortname) {
      g_free (track->artist_sortname);
      track->artist_sortname = NULL;
    }
    if (track->artist_id) {
      g_free (track->artist_id);
      track->artist_id = NULL;
    }
    break;
  case COLUMN_COMPOSER:
    g_free (track->composer);
    track->composer = g_strdup (string);
    gtk_list_store_set (track_store, &iter, COLUMN_COMPOSER, track->composer, -1);
    if (track->composer_sortname) {
      g_free (track->composer_sortname);
      track->composer_sortname = NULL;
    }
    break;
  default:
    g_warning (_("Unknown column %d was edited"), column);
  }

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

/**
 * Return TRUE if s1 and s2 are equal according to g_utf8_casefold or
 * if they are NULL, NUL or just ascii space. NULL, NUL and space are
 * considered equal
 */
static gboolean str_case_equal (const char*s1, const char *s2)
{
  gboolean retval;
  char *t1, *t2;

  if (sj_str_is_empty (s1) && sj_str_is_empty (s2))
    return TRUE;

  /* is_empty can handle NULL pointers but g_utf8_casefold cannot */
  if (!s1 || !s2)
    return FALSE;

  t1 = g_utf8_casefold (s1, -1);
  t2 = g_utf8_casefold (s2, -1);
  retval = g_str_equal (t1, t2);
  g_free (t1);
  g_free (t2);
  return retval;
}

G_MODULE_EXPORT void on_person_edit_changed(GtkEditable *widget,
                                            gpointer user_data) {
  GtkTreeIter iter;
  gboolean ok; /* TRUE if iter is valid */
  TrackDetails *track;
  gchar *former_album_person = NULL;
  /* Album person name and sortname */
  gchar **album_person_name, **album_person_sortname;
  /* Offsets for track person name and sortname */
  int off_person_name, off_person_sortname;
  int column; /* column for person in listview */

  g_return_if_fail (current_album != NULL);
  if (widget == GTK_EDITABLE (artist_entry)) {
    column = COLUMN_ARTIST;
    album_person_name = &current_album->artist;
    album_person_sortname = &current_album->artist_sortname;
    off_person_name = G_STRUCT_OFFSET (TrackDetails, artist);
    off_person_sortname = G_STRUCT_OFFSET (TrackDetails,
                                           artist_sortname);
  } else if (widget == GTK_EDITABLE (composer_entry)) {
    column = COLUMN_COMPOSER;
    album_person_name = &current_album->composer;
    album_person_sortname = &current_album->composer_sortname;
    off_person_name = G_STRUCT_OFFSET (TrackDetails, composer);
    off_person_sortname = G_STRUCT_OFFSET (TrackDetails,
                                           composer_sortname);
  } else {
    g_warning (_("Unknown widget calling on_person_edit_changed."));
    return;
  }

  remove_musicbrainz_ids (current_album);

  /* Unset the sortname field, as we can't change it automatically */
  if (*album_person_sortname) {
    g_free (*album_person_sortname);
    *album_person_sortname = NULL;
  }

  if (*album_person_name) {
    former_album_person = *album_person_name;
  }

  /* get all the characters */
  *album_person_name = gtk_editable_get_chars (widget, 0, -1);

  /* Set the person field in each tree row */
  for (ok = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (track_store), &iter);
       ok;
       ok = gtk_tree_model_iter_next (GTK_TREE_MODEL (track_store), &iter)) {
    gchar *current_track_person;
    gchar **track_person_name, **track_person_sortname;

    gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter, column,
                        &current_track_person, -1);

    /* Change track person if it matched album person before the change */
    if (!str_case_equal (current_track_person, former_album_person) &&
        !str_case_equal (current_track_person, *album_person_name))
      continue;

    gtk_tree_model_get (GTK_TREE_MODEL (track_store), &iter, COLUMN_DETAILS,
                        &track, -1);
    track_person_name     = G_STRUCT_MEMBER_P (track, off_person_name);
    track_person_sortname = G_STRUCT_MEMBER_P (track, off_person_sortname);

    g_free (*track_person_name);
    *track_person_name = g_strdup (*album_person_name);

    /* Unset the sortname field, as we can't change it automatically */
    if (*track_person_sortname) {
      g_free (*track_person_sortname);
      *track_person_sortname = NULL;
    }

    gtk_list_store_set (track_store, &iter, column, *track_person_name, -1);
  }
  g_free (former_album_person);
}

G_MODULE_EXPORT void on_genre_edit_changed(GtkEditable *widget, gpointer user_data) {
  g_return_if_fail (current_album != NULL);
  if (current_album->genre) {
    g_free (current_album->genre);
  }
  current_album->genre = gtk_editable_get_chars (widget, 0, -1); /* get all the characters */
  /* Toggle visibility of composer fields based on genre */
  composer_show_hide (current_album->genre);
}

G_MODULE_EXPORT void on_year_edit_changed(GtkEditable *widget, gpointer user_data) {
  const gchar* yearstr;
  int year;

  g_return_if_fail (current_album != NULL);

  yearstr = gtk_entry_get_text (GTK_ENTRY (widget));
  year = atoi (yearstr);
  if (year > 0) {
    if (current_album->release_date) {
      gst_date_time_unref (current_album->release_date);
    }
    current_album->release_date = gst_date_time_new_y (year);
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

static void on_contents_activate(GSimpleAction *action, GVariant *parameter, gpointer data) {
  GError *error = NULL;

  gtk_show_uri (NULL, "help:sound-juicer", GDK_CURRENT_TIME, &error);
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

/**
 * Performs various checks to ensure CD duplication is available.
 * If this is found TRUE is returned, otherwise FALSE is returned.
 */
static gboolean
is_cd_duplication_available(void)
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

GActionEntry app_entries[] = {
  { "re-read", on_reread_activate, NULL, NULL, NULL },
  { "duplicate", on_duplicate_activate, NULL, NULL, NULL },
  { "eject", on_eject_activate, NULL, NULL, NULL },
  { "submit-tracks", on_submit_activate, NULL, NULL, NULL },
  { "preferences", on_preferences_activate, NULL, NULL, NULL },
  { "help", on_contents_activate, NULL, NULL, NULL }
};

GActionEntry win_entries[] = {
  { "select-all", on_select_all_activate, NULL, NULL, NULL },
  { "deselect-all", on_deselect_all_activate, NULL, NULL, NULL }
};

GtkWidget *sj_create_sound_juicer()
{
  gchar *builderXML;
  GtkWidget *w;
  GtkTreeSelection *selection;
  GError *error = NULL;

  g_setenv ("PULSE_PROP_media.role", "music", TRUE);

  sj_debug_init ();

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
  
  builderXML = sjcd_plugin_get_builder_file();
  builder = gtkpod_builder_xml_new(builderXML);
  g_free(builderXML);

  gtk_builder_connect_signals (builder, NULL);

  w                             = GET_WIDGET ("main_window");
  vbox1                      = GET_WIDGET ("vbox1");
  g_object_ref(vbox1);
  gtk_container_remove(GTK_CONTAINER(w), vbox1);
  gtk_widget_destroy(w);

  message_area_eventbox = GET_WIDGET ("message_area_eventbox");
  title_entry           = GET_WIDGET ("title_entry");
  artist_entry          = GET_WIDGET ("artist_entry");
  composer_label        = GET_WIDGET ("composer_label");
  composer_entry        = GET_WIDGET ("composer_entry");
  duration_label        = GET_WIDGET ("duration_label");
  genre_entry           = GET_WIDGET ("genre_entry");
  year_entry            = GET_WIDGET ("year_entry");
  disc_number_entry     = GET_WIDGET ("disc_number_entry");
  track_listview        = GET_WIDGET ("track_listview");
  extract_button        = GET_WIDGET ("extract_button");
  select_button         = GET_WIDGET ("select_button");
  status_bar            = GET_WIDGET ("status_bar");
  entry_table           = GET_WIDGET ("entry_table");

  /*
   * Adding entries taken from sj but inserting action group
   * into vbox1 to couple the actions to the buttons
   */
  action_group = g_simple_action_group_new ();

  g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   NULL);

  gtk_widget_insert_action_group(GTK_WIDGET(vbox1), "app", G_ACTION_GROUP(action_group));

  g_action_map_add_action_entries (G_ACTION_MAP (action_group),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   NULL);

  gtk_widget_insert_action_group(GTK_WIDGET(vbox1), "win", G_ACTION_GROUP(action_group));

  gtk_button_set_label(GTK_BUTTON(select_button), _("Select None"));
  gtk_actionable_set_action_name(GTK_ACTIONABLE(select_button), "win.deselect-all");

  

  /* window actions are only available via shortcuts */
//   gtk_application_add_accelerator (GTK_APPLICATION (app),
//                                    "<Primary>a", "win.select-all", NULL);
//   gtk_application_add_accelerator (GTK_APPLICATION (app),
//                                    "<Primary><Shift>a", "win.deselect-all", NULL);

  { /* ensure that the extract/play button's size is constant */
    GtkWidget *fake_button1, *fake_button2;
    GtkSizeGroup *size_group;

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    fake_button1 = gtk_button_new_with_label (_("E_xtract"));
    gtk_button_set_use_underline (GTK_BUTTON (fake_button1), TRUE);
    gtk_size_group_add_widget (size_group, fake_button1);
    g_signal_connect_swapped (extract_button, "destroy",
                              G_CALLBACK (gtk_widget_destroy),
                              fake_button1);

    fake_button2 = gtk_button_new_with_label (_("_Stop"));
    gtk_button_set_use_underline (GTK_BUTTON (fake_button2), TRUE);
    gtk_size_group_add_widget (size_group, fake_button2);
    g_signal_connect_swapped (extract_button, "destroy",
                              G_CALLBACK (gtk_widget_destroy),
                              fake_button2);

    gtk_size_group_add_widget (size_group, extract_button);
    g_object_unref (G_OBJECT (size_group));
  }

  { /* ensure that the select/unselect button's size is constant */
    GtkWidget *fake_button1, *fake_button2;
    GtkSizeGroup *size_group;

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    fake_button1 = gtk_button_new_with_label (_("Select All"));
    gtk_size_group_add_widget (size_group, fake_button1);
    g_signal_connect_swapped (select_button, "destroy",
                              G_CALLBACK (gtk_widget_destroy),
                              fake_button1);

    fake_button2 = gtk_button_new_with_label (_("Select None"));
    gtk_size_group_add_widget (size_group, fake_button2);
    g_signal_connect_swapped (select_button, "destroy",
                              G_CALLBACK (gtk_widget_destroy),
                              fake_button2);

    gtk_size_group_add_widget (size_group, select_button);
    g_object_unref (G_OBJECT (size_group));
  }

  setup_genre_entry (genre_entry);

  /* Remove row spacing from hidden row containing composer_entry */
  gtk_table_set_row_spacing (GTK_TABLE (entry_table), COMPOSER_ROW, 0);

  track_store = gtk_list_store_new (COLUMN_TOTAL, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
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

    composer_renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Composer"),
                                                       composer_renderer,
                                                       "text", COLUMN_COMPOSER,
                                                       NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_expand (column, TRUE);
    g_signal_connect (composer_renderer, "edited", G_CALLBACK (on_cell_edited), GUINT_TO_POINTER (COLUMN_COMPOSER));
    g_object_set (G_OBJECT (composer_renderer), "editable", TRUE, NULL);
    gtk_tree_view_column_set_visible (column, FALSE);
    composer_column = column;
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

  baseuri_changed_cb (sj_settings, SJ_SETTINGS_BASEURI, NULL);
  path_pattern_changed_cb (sj_settings, SJ_SETTINGS_PATH_PATTERN, NULL);
  file_pattern_changed_cb (sj_settings, SJ_SETTINGS_FILE_PATTERN, NULL);
  profile_changed_cb (sj_settings, SJ_SETTINGS_AUDIO_PROFILE, NULL);
  paranoia_changed_cb (sj_settings, SJ_SETTINGS_PARANOIA, NULL);
  strip_changed_cb (sj_settings, SJ_SETTINGS_STRIP, NULL);
  eject_changed_cb (sj_settings, SJ_SETTINGS_EJECT, NULL);
  open_changed_cb (sj_settings, SJ_SETTINGS_OPEN, NULL);
  if (device == NULL && uris == NULL) {
    /* FIXME: this should set the device gsettings key to a meaningful
     * value if it's empty (which is the case until the user changes it in
     * the prefs)
     */
    device_changed_cb (sj_settings, SJ_SETTINGS_DEVICE, NULL);
  } else {
    if (device)
      set_device (device, TRUE);
    else {
      char *d;

      /* Mash up the CDDA URIs into a device path */
      if (g_str_has_prefix (uris[0], "cdda://")) {
        gint len;
        d = g_strdup_printf ("/dev/%s%c", uris[0] + strlen ("cdda://"), '\0');
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
  set_action_enabled ("duplicate", FALSE);
  duplication_enabled = is_cd_duplication_available();

  brasero_media_library_stop ();

  return vbox1;
}

void set_action_enabled (const char *name, gboolean enabled)
{
  GActionMap *map = G_ACTION_MAP (action_group);
  GAction *action = g_action_map_lookup_action (map, name);

  if (action == NULL)
	g_warning("action %s is null", name);

  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}