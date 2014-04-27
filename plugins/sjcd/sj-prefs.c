/*
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-about.c
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// TODO need to remove when replacing the prefs to gtkpod prefs
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/misc.h"
#include "plugin.h"
#include "sound-juicer.h"

#include <string.h>
#include <gst/pbutils/encoding-profile.h>
#include <gtk/gtk.h>
#include <brasero-drive-selection.h>

#include "rb-gst-media-types.h"
#include "sj-util.h"
#include "sj-extracting.h"
#include "sj-prefs.h"

#define GET_BUILDER_WIDGET(x, a) gtkpod_builder_xml_get_widget (x, a)

GtkWidget *dialog = NULL;
GtkWidget *profile_option = NULL;
GtkWidget *cd_option = NULL;
GtkWidget *path_option = NULL;
GtkWidget *file_option = NULL;
GtkWidget *basepath_fcb = NULL;
GtkWidget *check_strip = NULL;
GtkWidget *check_eject = NULL;
GtkWidget *check_open = NULL;
GtkWidget *path_example_label = NULL;

typedef struct {
  char* name;
  char* pattern;
} FilePattern;

static const FilePattern path_patterns[] = {
  {N_("Album Artist, Album Title"), "%aa/%at"},
  {N_("Album Artist (sortable), Album Title"), "%as/%at"},
  {N_("Track Artist, Album Title"), "%ta/%at"},
  {N_("Track Artist (sortable), Album Title"), "%ts/%at"},
  {N_("Album Title"), "%at"},
  {N_("Album Artist"), "%aa"},
  {N_("Album Artist (sortable)"), "%as"},
  {N_("Album Artist - Album Title"), "%aa - %at"},
  {N_("Album Artist (sortable) - Album Title"), "%as - %at"},
  {N_("Album Composer, Album Title"), "%ac/%at"},
  {N_("Album Composer (sortable), Album Title"), "%ap/%at"},
  {N_("Track Composer, Album Title"), "%tc/%at"},
  {N_("Track Composer (sortable), Album Title"), "%tp/%at"},
  {N_("[none]"), "./"},
  {NULL, NULL}
};

static const FilePattern file_patterns[] = {
  {N_("Number - Title"), "%dn - %tt"},
  {N_("Track Title"), "%tt"},
  {N_("Track Artist - Track Title"), "%ta - %tt"},
  {N_("Track Artist (sortable) - Track Title"), "%ts - %tt"},
  {N_("Number. Track Artist - Track Title"), "%dN. %ta - %tt"},
  /* {N_("Number. Track Artist (sortable) - Track Title"), "%tN. %ts - %tt"}, */
  {N_("Number-Track Artist-Track Title (lowercase)"), "%dN-%tA-%tT"},
  /* {N_("Number-Track Artist (sortable)-Track Title (lowercase)"), "%tN-%tS-%tT"}, */
  {N_("Track Composer - Track Artist - Track Title"), "%tc - %ta - %tt"},
  {N_("Track Composer (sortable) - Track Artist (sortable) - Track Title"), "%tp - %ts - %tt"},
  {N_("Number. Track Composer - Track Artist - Track Title"), "%dN. %tc - %ta - %tt"},
  {N_("Number-Track Composer-Track Artist-Track Title (lowercase)"), "%dN-%tC-%tA-%tT"},
  {NULL, NULL}
};

const gchar*
sj_get_default_file_pattern (void)
{
  return file_patterns[0].pattern;
}

const gchar*
sj_get_default_path_pattern (void)
{
  return path_patterns[0].pattern;
}
void prefs_profile_changed (GtkWidget *widget, gpointer user_data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  /* Handle the change being to unselect a profile */
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
    char *media_type;
    gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
	                        0, &media_type, -1);

	g_settings_set_string (sj_settings, SJ_SETTINGS_AUDIO_PROFILE, media_type);
	g_free (media_type);
  }
}

/**
 * Show the gnome help browser
 */
void show_help (GtkWindow *parent)
{
  GError *error = NULL;

  gtk_show_uri (NULL, "help:sound-juicer/preferences", GDK_CURRENT_TIME, &error);
  if (error) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (parent,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Could not display help for Sound Juicer\n"
                                       "%s"),
                                     error->message);

    gtk_widget_show_all (dialog);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy(dialog);
    g_error_free (error);
  }
}

/**
 * Changed folder in the Prefs dialog
 */
G_MODULE_EXPORT void prefs_base_folder_changed (GtkWidget *chooser, gpointer user_data)
{
  char *new_uri, *current_uri;

  current_uri = g_settings_get_string (sj_settings, SJ_SETTINGS_BASEURI);
  new_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser));

  if (current_uri == NULL || strcmp(current_uri, new_uri) != 0) {
      g_settings_set_string (sj_settings, SJ_SETTINGS_BASEURI, new_uri);
  }

  g_free (new_uri);
  g_free (current_uri);
}

void prefs_path_option_changed (GtkComboBox *combo, gpointer user_data)
{
  gint active;
  const char* pattern;
  active = gtk_combo_box_get_active (combo);
  if (active == -1)
    return;

  pattern = path_patterns[active].pattern;
  if (pattern) {
    g_settings_set_string (sj_settings, SJ_SETTINGS_PATH_PATTERN, pattern);
  }
}

G_MODULE_EXPORT void prefs_file_option_changed (GtkComboBox *combo, gpointer user_data)
{
  gint active;
  const char* pattern;
  active = gtk_combo_box_get_active (combo);
  if (active == -1)
    return;

  pattern = file_patterns[active].pattern;
  if (pattern) {
    g_settings_set_string (sj_settings, SJ_SETTINGS_FILE_PATTERN, pattern);
  }
}

static void
sj_profile_option_set_active (GtkWidget *chooser, const char *profile)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean done;

  done = FALSE;
  model = gtk_combo_box_get_model(GTK_COMBO_BOX(chooser));
  if (gtk_tree_model_get_iter_first (model, &iter)) {
    do {
      char *media_type;

      gtk_tree_model_get (model, &iter, 0, &media_type, -1);
      if (g_strcmp0 (media_type, profile) == 0) {
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), &iter);
        done = TRUE;
      }
      g_free (media_type);
    } while (done == FALSE && gtk_tree_model_iter_next (model, &iter));
  }

  if (done == FALSE) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser), NULL);
  }
}

static void audio_profile_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  char *value;
  g_return_if_fail (strcmp (key, SJ_SETTINGS_AUDIO_PROFILE) == 0);

  value = g_settings_get_string (settings, key);
  sj_profile_option_set_active (profile_option, value);
  g_free (value);
}

static void baseuri_changed_cb  (GSettings *settings, gchar *key, gpointer user_data)
{
  /*
   * The conflict between separation of the prefs and the main window,
   * and the problem of duplication, is very clear here. This code is
   * identical to the code in the main key listener, but modifying the
   * prefs window from the main is ugly. Maybe put the validation code
   * into sj-utils?
   */
  const char* base_uri, *current_uri;
  g_return_if_fail (strcmp (key, SJ_SETTINGS_BASEURI) == 0);

  base_uri = g_settings_get_string (settings, key);

  if (base_uri == NULL || base_uri[0] == '\0') {
    GFile *dir;
    char *dir_uri;

    dir = sj_get_default_music_directory ();
    dir_uri = g_file_get_uri (dir);
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (basepath_fcb), dir_uri);
    g_free (dir_uri);
    g_object_unref (dir);
  } else {
    current_uri = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (basepath_fcb));
    if (current_uri == NULL || strcmp (current_uri, base_uri) != 0)
      gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (basepath_fcb), base_uri);

  }
}

static void pattern_label_update (void)
{
  char *file_pattern, *path_pattern;
  char *file_value, *path_value, *example, *format;
  char *media_type;
  GstEncodingProfile *profile;

  static const AlbumDetails sample_album = {
    .title = "Help!", /* title */
    .artist = "The Beatles", /* artist */
    .artist_sortname = "Beatles, The", /* artist_sortname */
    .genre = NULL, /* genre */
    .number = 0, /* number of tracks*/
    .disc_number = 1, /* disc number */
    .tracks = NULL, /* track list */
    .release_date = NULL, /* release date */
    .album_id = NULL, /* album ID */
    .artist_id = NULL /* artist ID */
  };
  static const TrackDetails sample_track = {
    .album = (AlbumDetails*)&sample_album,  /*album */
    .number = 7, /* track number */
    .title = "Ticket To Ride", /* title */
    .artist = "The Beatles", /* artist */
    .artist_sortname = "Beatles, The", /* artist_sortname */
    .composer = "John Lennon and Paul McCartney", /* composer */
    .composer_sortname = "Lennon, John", /* composer_sortname */
    .duration = 0, /* duration */
    .track_id = NULL, /* track ID */
    .artist_id = NULL, /* artist ID */
  };

  g_object_get (extractor, "profile", &profile, NULL);
  /* It's possible the profile isn't set, in which case do nothing */
  if (!profile) {
    return;
  }
  media_type = rb_gst_encoding_profile_get_media_type (profile);
  gst_encoding_profile_unref (profile);

  /* TODO: sucky. Replace with get-gconf-key-with-default mojo */
	file_pattern = g_settings_get_string (sj_settings, SJ_SETTINGS_FILE_PATTERN);
  if (file_pattern == NULL) {
    file_pattern = g_strdup (sj_get_default_file_pattern ());
  }
	path_pattern = g_settings_get_string (sj_settings, SJ_SETTINGS_PATH_PATTERN);
  if (path_pattern == NULL) {
    path_pattern = g_strdup (sj_get_default_path_pattern ());
  }

  file_value = filepath_parse_pattern (file_pattern, &sample_track);
  path_value = filepath_parse_pattern (path_pattern, &sample_track);

  example = g_build_filename (G_DIR_SEPARATOR_S, path_value, file_value, NULL);
  g_free (file_value);
  g_free (file_pattern);
  g_free (path_value);
  g_free (path_pattern);

  format = g_strconcat ("<small><i><b>",
                        _("Example Path"),
                        ":</b> ",
                        example,
                        ".",
                        rb_gst_media_type_to_extension (media_type),
                        "</i></small>", NULL);
  g_free (example);
  g_free (media_type);

  gtk_label_set_markup (GTK_LABEL (path_example_label), format);
  g_free (format);
}

static void path_pattern_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  char *value;
  int i = 0;
  g_return_if_fail (strcmp (key, SJ_SETTINGS_PATH_PATTERN) == 0);

  value = g_settings_get_string (settings, key);
  while (path_patterns[i].pattern && strcmp(path_patterns[i].pattern, value) != 0) {
    i++;
  }
  g_free (value);

  gtk_combo_box_set_active (GTK_COMBO_BOX (path_option), i);
  pattern_label_update ();
}

static void file_pattern_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  char *value;
  int i = 0;

  g_return_if_fail (strcmp (key, SJ_SETTINGS_FILE_PATTERN) == 0);

  value = g_settings_get_string (settings, key);
  while (file_patterns[i].pattern && strcmp(file_patterns[i].pattern, value) != 0) {
    i++;
  }
  g_free (value);

  gtk_combo_box_set_active (GTK_COMBO_BOX (file_option), i);
  pattern_label_update ();
}

/**
 * Default device changed (either GSettings key or the widget)
 */
static void device_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  BraseroDrive *drive;
  BraseroMediumMonitor *monitor;
  char *value;

  g_return_if_fail (strcmp (key, SJ_SETTINGS_DEVICE) == 0);

  value = g_settings_get_string (settings, key);
  if ((value != NULL) && (*value != '\0')) {
    monitor = brasero_medium_monitor_get_default ();
    drive = brasero_medium_monitor_get_drive (monitor, value);
    brasero_drive_selection_set_active (BRASERO_DRIVE_SELECTION (cd_option), drive);
    g_object_unref (drive);
    g_object_unref (monitor);
  } else {
    /* FIXME: see the FIXME in sj-main.c around one of the
     * device_changed_cb calls for a way to fix this
     */
    g_warn_if_reached();
  }

  g_free (value);
}

static void prefs_drive_changed (BraseroDriveSelection *selection, BraseroDrive *drive, gpointer user_data)
{
  if (drive) {
    g_settings_set_string (sj_settings, SJ_SETTINGS_DEVICE, brasero_drive_get_device (drive));
  } else {
    g_settings_set_string (sj_settings, SJ_SETTINGS_DEVICE, NULL);
  }
}

/**
 * The GSettings key for the strip characters option changed
 */
static void strip_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
  /* NOTE that strip_changed_cb in sj-main.c will also be called, and will also update
     the global value strip_chars - but which function will get called first?
     Make sure strip_chars is up to date BEFORE calling pattern_label_update */
  g_return_if_fail (strcmp (key, SJ_SETTINGS_STRIP) == 0);

  strip_chars = g_settings_get_boolean (settings, key);
  pattern_label_update ();
}

/**
 * Given a FilePattern array, populate the combo box.
 */
static void populate_pattern_combo (GtkComboBox *combo, const FilePattern *patterns)
{
  GtkListStore *store;
  int i;

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  for (i = 0; patterns[i].pattern; ++i) {
    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, _(patterns[i].name), 1, patterns[i].pattern, -1);
  }
  gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));
}

static void
on_response (GtkDialog *dialog, gint response, gpointer user_data)
{
  if (response == GTK_RESPONSE_HELP) {
    show_help (GTK_WINDOW (dialog));
  } else {
    gtk_widget_hide (GTK_WIDGET (dialog));
  }
}

static void populate_profile_combo (GtkComboBox *combo)
{
  GstEncodingTarget *target;
  const GList *p;
  GtkTreeModel *model;

  model = GTK_TREE_MODEL (gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));

  target = rb_gst_get_default_encoding_target ();
  for (p = gst_encoding_target_get_profiles (target); p != NULL; p = p->next) {
    GstEncodingProfile *profile = GST_ENCODING_PROFILE (p->data);
    char *media_type;

    media_type = rb_gst_encoding_profile_get_media_type (profile);
    if (media_type == NULL) {
      continue;
    }
    gtk_tree_store_insert_with_values (GTK_TREE_STORE (model),
                                       NULL, NULL, -1,
                                       0, media_type,
                                       1, gst_encoding_profile_get_description (profile),
                                       2, profile, -1);
    g_free (media_type);
  }

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
}

G_MODULE_EXPORT void on_destroy_dialog_content_cb(GtkWidget *widget, gpointer user_data) {

    g_settings_unbind (sj_settings, SJ_SETTINGS_EJECT);
    g_settings_unbind (sj_settings, SJ_SETTINGS_OPEN);
    g_settings_unbind (sj_settings, SJ_SETTINGS_STRIP);

    g_signal_handlers_disconnect_by_func(G_OBJECT (sj_settings),
            (GCallback)device_changed_cb, cd_option);
    g_signal_handlers_disconnect_by_func (G_OBJECT (sj_settings),
            (GCallback)baseuri_changed_cb, basepath_fcb);
    g_signal_handlers_disconnect_by_func (G_OBJECT (sj_settings),
            (GCallback)audio_profile_changed_cb, profile_option);
    g_signal_handlers_disconnect_by_func (G_OBJECT (sj_settings),
            (GCallback)path_pattern_changed_cb, path_option);
    g_signal_handlers_disconnect_by_func (G_OBJECT (sj_settings),
            (GCallback)file_pattern_changed_cb, file_option);
    g_signal_handlers_disconnect_by_func (G_OBJECT (sj_settings),
            (GCallback)strip_changed_cb, NULL);

    g_signal_handlers_disconnect_by_func (extractor, pattern_label_update, NULL);
}

static GtkWidget *create_preferences_dialog(GtkBuilder *builder) {
  const char *labels[] = { "cd_label", "path_label", "folder_label", "file_label", "profile_label" };
  guint i;
  GtkSizeGroup *group;

  dialog = GET_BUILDER_WIDGET (builder, "prefs_dialog");
  g_return_val_if_fail(dialog, NULL);
  g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &(dialog));

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_app));

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  for (i = 0; i < G_N_ELEMENTS (labels); i++) {
    GtkWidget *widget;
    widget = GET_BUILDER_WIDGET (builder, labels[i]);
    if (widget) {
        gtk_size_group_add_widget (group, widget);
    } else {
        g_warning ("Widget %s not found", labels[i]);
    }
  }
  g_object_unref (group);

  cd_option = GET_BUILDER_WIDGET (builder, "cd_option");
  basepath_fcb = GET_BUILDER_WIDGET (builder, "path_chooser");
  path_option = GET_BUILDER_WIDGET (builder, "path_option");
  file_option = GET_BUILDER_WIDGET (builder, "file_option");
  profile_option = GET_BUILDER_WIDGET (builder, "profile_option");
  check_strip = GET_BUILDER_WIDGET (builder, "check_strip");
  check_eject = GET_BUILDER_WIDGET (builder, "check_eject");
  check_open = GET_BUILDER_WIDGET (builder, "check_open");
  path_example_label = GET_BUILDER_WIDGET (builder, "path_example_label");

  sj_add_default_dirs (GTK_FILE_CHOOSER (basepath_fcb));

  populate_pattern_combo (GTK_COMBO_BOX (path_option), path_patterns);
  g_signal_connect (path_option, "changed", G_CALLBACK (prefs_path_option_changed), NULL);

  populate_pattern_combo (GTK_COMBO_BOX (file_option), file_patterns);
  g_signal_connect (file_option, "changed", G_CALLBACK (prefs_file_option_changed), NULL);
  populate_profile_combo (GTK_COMBO_BOX (profile_option));
  g_signal_connect (profile_option, "changed", G_CALLBACK (prefs_profile_changed), NULL);
  g_signal_connect (cd_option, "drive-changed", G_CALLBACK (prefs_drive_changed), NULL);

  /* Connect to GSettings to update the UI */
  g_settings_bind (sj_settings, SJ_SETTINGS_EJECT, G_OBJECT (check_eject), "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (sj_settings, SJ_SETTINGS_OPEN, G_OBJECT (check_open), "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (sj_settings, SJ_SETTINGS_STRIP, G_OBJECT (check_strip), "active", G_SETTINGS_BIND_DEFAULT);
  g_signal_connect (G_OBJECT (sj_settings), "changed::"SJ_SETTINGS_DEVICE,
                      (GCallback)device_changed_cb, NULL);
  g_signal_connect (G_OBJECT (sj_settings), "changed::"SJ_SETTINGS_BASEURI,
                      (GCallback)baseuri_changed_cb, NULL);
  g_signal_connect (G_OBJECT (sj_settings), "changed::"SJ_SETTINGS_AUDIO_PROFILE,
                      (GCallback)audio_profile_changed_cb, NULL);
  g_signal_connect (G_OBJECT (sj_settings), "changed::"SJ_SETTINGS_PATH_PATTERN,
                      (GCallback)path_pattern_changed_cb, NULL);
  g_signal_connect (G_OBJECT (sj_settings), "changed::"SJ_SETTINGS_FILE_PATTERN,
                      (GCallback)file_pattern_changed_cb, NULL);
  g_signal_connect (G_OBJECT (sj_settings), "changed::"SJ_SETTINGS_STRIP,
        (GCallback)strip_changed_cb, NULL);

  g_signal_connect (extractor, "notify::profile", pattern_label_update, NULL);

  baseuri_changed_cb (sj_settings, SJ_SETTINGS_BASEURI, NULL);
  audio_profile_changed_cb (sj_settings, SJ_SETTINGS_AUDIO_PROFILE, NULL);
  file_pattern_changed_cb (sj_settings, SJ_SETTINGS_FILE_PATTERN, NULL);
  path_pattern_changed_cb (sj_settings, SJ_SETTINGS_PATH_PATTERN, NULL);
  device_changed_cb (sj_settings, SJ_SETTINGS_DEVICE, NULL);

  g_signal_connect (GTK_DIALOG (dialog), "response", G_CALLBACK (on_response), NULL);

  return dialog;
}

/**
 * Clicked on Preferences in the UI
 */
void show_preferences_dialog ()
{
  GtkBuilder *prefBuilder;
  gchar *builderXML;
  GtkWidget *prefs_dialog;

  builderXML = sjcd_plugin_get_builder_file();
  prefBuilder = gtkpod_builder_xml_new(builderXML);
  gtk_builder_connect_signals(prefBuilder, NULL);

  prefs_dialog = create_preferences_dialog(prefBuilder);
  g_signal_connect (GTK_DIALOG (prefs_dialog), "response", G_CALLBACK (on_response), NULL);
  gtk_widget_show_all (prefs_dialog);

  g_free(builderXML);
  g_object_unref(prefBuilder);
}

GtkWidget *init_sjcd_preferences()
{
    GtkBuilder *prefBuilder;
    gchar *builderXML;
    GtkWidget *prefs_dialog;
    GtkWidget *container;
    GtkWidget *vbox;
    GList *children = NULL;

    builderXML = sjcd_plugin_get_builder_file();
    prefBuilder = gtkpod_builder_xml_new(builderXML);
    gtk_builder_connect_signals(prefBuilder, NULL);

    prefs_dialog = create_preferences_dialog(prefBuilder);
    container = gtk_dialog_get_content_area(GTK_DIALOG(prefs_dialog));
    children = gtk_container_get_children(GTK_CONTAINER(container));
    g_return_val_if_fail(children, NULL);

    vbox = GTK_WIDGET(children->data);
    g_object_ref(vbox);
    gtk_container_remove(GTK_CONTAINER(container), vbox);

    g_free(builderXML);
    g_object_unref(prefBuilder);

    return vbox;
}
