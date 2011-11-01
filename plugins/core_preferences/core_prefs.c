/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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
 */

#include <libanjuta/anjuta-preferences.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/charset.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/file_convert.h"
#include "libgtkpod/directories.h"
#include "core_prefs.h"
#include "plugin.h"

static gchar *builder_path = NULL;
static GtkWidget *notebook = NULL;
/*
 Begin types
 */
typedef struct _ind_string {
    gint index;
    const gchar *string;
} ind_string;

#define IND_STRING_END(x, i) ((x)[i].string == NULL)
#define COUNTOF(x) (sizeof(x) / sizeof((x)[0]))

/*
 Begin data

 0: checkbox glade ID
 1: preference
 2: dependency glade IDs, comma-separated
 */
const gchar *checkbox_map[][3] =
    {
    /* Music tab */
        { "background_transfer", "file_convert_background_transfer", NULL },
        { "add_subfolders", "add_recursively", NULL },
        { "allow_duplicates", "!sha1", NULL },
        { "delete_missing", "sync_delete_tracks", NULL },
        { "update_existing_track", "update_existing", NULL },
        { "include_neverplayed", "not_played_track", NULL },
    /* Metadata tab */
        { "read_tags", "readtags", NULL },
        { "parse_filename_tags", "parsetags", "customize_tags" },
        { "last_resort_tags", NULL, "tag_title,tag_artist,tag_album,tag_composer,tag_genre" },
        { "write_tags", "id3_write", "tag_encoding,write_tags_legacy" },
        { "write_tags_legacy", "!id3_write_id3v24", NULL },
        { "mass_modify_tags", "multi_edit", NULL },
        { "read_coverart", "coverart_apic", NULL },
        { "template_coverart", "coverart_file", "customize_coverart" },
        { "generate_video_thumbnails", "video_thumbnailer", "customize_video_thumbnailer" },
    /* Feedback tab */
        { "confirm_del_tracks", NULL, "confirm_from_ipod,confirm_from_hdd,confirm_from_db" },
        { "confirm_from_ipod", "delete_ipod", NULL },
        { "confirm_from_hdd", "delete_local_file", NULL },
        { "confirm_from_db", "delete_database", NULL },
        { "confirm_del_pl", "delete_file", NULL },
        { "confirm_del_sync", "sync_confirm_delete", NULL },
        { "msg_startup", "startup_messages", NULL },
        { "msg_duplicates", "show_duplicates", NULL },
        { "msg_results", "sync_show_summary", NULL },
        { "msg_updated", "show_updated", NULL },
        { "msg_unupdated", "show_non_updated", NULL }, };

const gchar *conv_checkbox_map[][3] =
    {
        { "convert_mp3", "convert_mp3", NULL },
        { "convert_aac", "convert_m4a", NULL },
        { "convert_wav", "convert_wav", NULL },
        { "display_conversion_log", "file_convert_display_log", NULL }
    };

ind_string tag_checkbox_map[] =
    {
        { 0, "tag_title" },
        { 1, "tag_artist" },
        { 2, "tag_album" },
        { 3, "tag_genre" },
        { 4, "tag_composer" }, };

const gchar *conv_audio_scripts[] =
    { CONVERT_TO_MP3_SCRIPT, CONVERT_TO_M4A_SCRIPT };

gchar *modifiable_conv_paths[] =
    {
        "path_conv_ogg",
        "path_conv_flac",
        "path_conv_wav"
    };

static TempPrefs *temp_prefs = NULL;
static GtkBuilder* builder = NULL;

static void update_checkbox_deps(GtkToggleButton *checkbox, const gchar *deps);
static void init_checkbox(GtkToggleButton *checkbox, const gchar *pref, const gchar *deps);
static gboolean tree_get_current_iter(GtkTreeView *view, GtkTreeIter *iter);

static GtkWindow *notebook_get_parent_window() {
    if (!notebook) {
        return NULL;
    }

    return GTK_WINDOW(gtk_widget_get_toplevel(notebook));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_agp_track_count_value_changed(GtkSpinButton *sender, gpointer e) {
    gint num = gtk_spin_button_get_value_as_int(sender);
    prefs_set_int("misc_track_nr", num);
}

/*
 generic glade callback, used by many checkboxes
 */
G_MODULE_EXPORT void on_simple_checkbox_toggled(GtkToggleButton *sender, gpointer e) {
    gboolean active = gtk_toggle_button_get_active(sender);
    gchar *pref = (gchar *) g_object_get_data(G_OBJECT(sender), "pref");
    gchar *deps = (gchar *) g_object_get_data(G_OBJECT(sender), "deps");

    if (pref) {
        if (pref[0] == '!') /* Checkbox is !preference */
            prefs_set_int(pref + 1, !active);
        else
            prefs_set_int(pref, active);
    }

    update_checkbox_deps(sender, deps);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_tag_checkbox_toggled(GtkToggleButton *sender, gpointer e) {
    gint index = *(gint *) g_object_get_data(G_OBJECT(sender), "index");
    prefs_set_int_index("tag_autoset", index, gtk_toggle_button_get_active(sender));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_browse_button_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dialog;
    gchar *base, *args, *path;
    const gchar *space, *current;
    GtkEntry *entry = GTK_ENTRY (g_object_get_data (G_OBJECT (sender), "entry"));

    g_return_if_fail (entry);

    dialog
            = gtk_file_chooser_dialog_new(_("Browse"), GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (sender))), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    current = gtk_entry_get_text(entry);
    /* separate filename from command line arguments */
    space = strchr(current, ' ');
    if (space) {
        base = g_strndup(current, space - current);
        args = g_strdup(space);
    }
    else {
        base = g_strdup(current);
        args = NULL;
    }

    path = g_find_program_in_path(base);

    if (path) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (dialog), path);
    }
    else {
        gchar *dir = g_path_get_dirname(base);
        if (dir) {
            if (g_file_test(dir, G_FILE_TEST_IS_DIR) && g_path_is_absolute(dir)) {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), dir);
            }
        }
        g_free(dir);
    }

    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
        if (args) { /* add args to filename */
            gchar *new = g_strdup_printf("%s%s", filename, args);
            gtk_entry_set_text(entry, new);
            g_free(new);
        }
        else {
            gtk_entry_set_text(entry, filename);
        }
        g_free(filename);
    }

    gtk_widget_destroy(GTK_WIDGET (dialog));

    g_free(base);
    g_free(path);
    g_free(args);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_unsetdeps_checkbox_toggled(GtkToggleButton *sender, gpointer e) {
    if (builder && !gtk_toggle_button_get_active(sender)) {
        int i;
        const gchar *deps = (gchar *) g_object_get_data(G_OBJECT(sender), "deps");
        gchar **deparray = g_strsplit(deps, ",", 0);

        for (i = 0; deparray[i]; i++) {
            GtkWidget *dep = GTK_WIDGET(gtk_builder_get_object(builder, deparray[i]));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dep), FALSE);
        }
        g_strfreev(deparray);
    }

    /* and then call the default handler */
    on_simple_checkbox_toggled(sender, e);
}

/*
 glade callback
 */
G_MODULE_EXPORT void open_encoding_dialog(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object (builder, "prefs_encoding_dialog"));
    GtkWidget *combo = GTK_WIDGET(gtk_builder_get_object (builder, "encoding_combo"));

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    init_checkbox(GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "use_encoding_for_update")), "update_charset", NULL);

    init_checkbox(GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "use_encoding_for_writing")), "write_charset", NULL);

    charset_init_combo_box(GTK_COMBO_BOX (combo));
    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_encoding_combo_changed(GtkComboBox *sender, gpointer e) {
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (!gtk_combo_box_get_active_iter(sender, &iter))
        return;

    model = gtk_combo_box_get_model(sender);

    gchar *description;
    gtk_tree_model_get(model, &iter, 0, &description, -1);

    gchar *charset = charset_from_description(description);

    prefs_set_string("charset", charset);
    g_free(charset);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_customize_tags_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object (builder, "prefs_tag_parse_dialog"));
    gchar *temp = prefs_get_string("parsetags_template");

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    if (temp) {
        gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "filename_pattern")), temp);

        g_free(temp);
    }

    init_checkbox(GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "overwrite_tags")), "parsetags_overwrite", NULL);

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_filename_pattern_changed(GtkEditable *sender, gpointer e) {
    prefs_set_string("parsetags_template", gtk_entry_get_text(GTK_ENTRY (sender)));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_customize_coverart_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object (builder, "prefs_coverart_dialog"));
    gchar *temp = prefs_get_string("coverart_template");

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    if (temp) {
        gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "coverart_pattern")), temp);

        g_free(temp);
    }

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_customize_video_thumbnailer_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object (builder, "prefs_video_thumbnailer_dialog"));
    gchar *temp = prefs_get_string("video_thumbnailer_prog");

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    if (temp) {
        gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "video_thumbnailer")), temp);

        g_free(temp);
    }

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_coverart_pattern_changed(GtkEditable *sender, gpointer e) {
    prefs_set_string("coverart_template", gtk_entry_get_text(GTK_ENTRY (sender)));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_video_thumbnailer_changed(GtkEditable *sender, gpointer e) {
    prefs_set_string("video_thumbnailer_prog", gtk_entry_get_text(GTK_ENTRY (sender)));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_save_threshold_spin_button_value_changed(GtkSpinButton *spinbutton, gpointer user_data) {
    prefs_set_int("file_saving_threshold", gtk_spin_button_get_value_as_int (spinbutton));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_exclusions_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object (builder, "prefs_exclusions_dialog"));
    GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object (builder, "exclusion_list"));
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    GtkTreeViewColumn *column = gtk_tree_view_column_new();
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gchar *temp = prefs_get_string("exclude_file_mask");

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    if (temp) {
        gint i;
        gchar **masks = g_strsplit(temp, ";", 0);
        GtkTreeIter iter;

        g_free(temp);

        for (i = 0; masks[i]; i++) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, masks[i], -1);
        }

        g_strfreev(masks);
    }

    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW (tree), column);
    gtk_tree_view_set_model(GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
    g_object_unref(store);

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

static gboolean tree_get_current_iter(GtkTreeView *view, GtkTreeIter *iter) {
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreePath *path;

    gtk_tree_view_get_cursor(view, &path, NULL);

    if (!path)
        return FALSE;

    gtk_tree_model_get_iter(model, iter, path);
    gtk_tree_path_free(path);

    return TRUE;
}

static void update_exclusions(GtkListStore *store) {
    GtkTreeModel *model = GTK_TREE_MODEL (store);
    gint rows = gtk_tree_model_iter_n_children(model, NULL);
    gchar **array = g_new (gchar *, rows + 1);
    gchar *temp;
    gint i;
    GtkTreeIter iter;

    array[rows] = NULL;

    for (i = 0; i < rows; i++) {
        gtk_tree_model_iter_nth_child(model, &iter, NULL, i);
        gtk_tree_model_get(model, &iter, 0, array + i, -1);
    }

    temp = g_strjoinv(";", array);
    prefs_set_string("exclude_file_mask", temp);
    g_free(temp);
    g_strfreev(array);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_add_exclusion_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object(builder, "exclusion_list"));
    GtkWidget *entry = GTK_WIDGET(gtk_builder_get_object(builder, "new_exclusion"));
    const gchar *text = gtk_entry_get_text(GTK_ENTRY (entry));

    if (text && text[0]) {
        GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
        GtkTreeIter iter;

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, text, -1);
        gtk_entry_set_text(GTK_ENTRY (entry), "");

        update_exclusions(store);
    }
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_remove_exclusion_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object(builder, "exclusion_list"));
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
    GtkTreeIter iter;

    if (!tree_get_current_iter(GTK_TREE_VIEW (tree), &iter) || gtk_list_store_iter_is_valid(store, &iter)) {
        gtk_list_store_remove(store, &iter);
        update_exclusions(store);
    }
}

static void cmd_setup_widget(const gchar *entry_name, const gchar *envname, const gchar *browse_name) {
    GtkWidget *entry = GTK_WIDGET(gtk_builder_get_object(builder, entry_name));
    gchar *temp = prefs_get_string(envname);
    if (!temp) {
        temp = g_strdup("");
    }

    g_object_set_data(G_OBJECT (entry), "envname", (gpointer) envname);
    g_object_set_data(G_OBJECT (gtk_builder_get_object(builder, browse_name)), "entry", entry);

    gtk_entry_set_text(GTK_ENTRY (entry), temp);
    g_free(temp);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_normalization_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_normalization_dialog"));

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    cmd_setup_widget("cmd_mp3gain", "path_mp3gain", "browse_mp3gain");
    cmd_setup_widget("cmd_aacgain", "path_aacgain", "browse_aacgain");

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_cmd_entry_changed(GtkEditable *sender, gpointer e) {
    const gchar *envname = g_object_get_data(G_OBJECT (sender), "envname");
    prefs_set_string(envname, gtk_entry_get_text(GTK_ENTRY (sender)));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_conversion_settings_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_conversion_dialog"));
    gchar *temp = prefs_get_string("file_convert_cachedir");
    gint i;

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    if (temp) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (gtk_builder_get_object(builder, "cache_folder")), temp);
        g_free(temp);
    }

    gtk_spin_button_set_value(GTK_SPIN_BUTTON (gtk_builder_get_object(builder, "bg_threads")), prefs_get_int("file_convert_max_threads_num"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON (gtk_builder_get_object(builder, "cache_size")), prefs_get_int("file_convert_maxdirsize"));

    for (i = 0; i < COUNTOF(conv_checkbox_map); i++) {
        init_checkbox(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, conv_checkbox_map[i][0])), conv_checkbox_map[i][1], conv_checkbox_map[i][2]);
    }

    GtkWidget *mp3toggle = GTK_WIDGET (gtk_builder_get_object(builder, conv_checkbox_map[0][0]));
    GtkWidget *m4atoggle = GTK_WIDGET (gtk_builder_get_object(builder, conv_checkbox_map[1][0]));

    if (prefs_get_int("conversion_target_format") == TARGET_FORMAT_MP3) {
        /* No point converting an mp3 to an mp3! */
        gtk_widget_set_sensitive(mp3toggle, FALSE);
        gtk_widget_set_sensitive(m4atoggle, TRUE);
    } else if (prefs_get_int("conversion_target_format") == TARGET_FORMAT_AAC) {
        /* No point converting an m4a to an m4a! */
        gtk_widget_set_sensitive(mp3toggle, TRUE);
        gtk_widget_set_sensitive(m4atoggle, FALSE);
    }

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
    file_convert_prefs_changed();
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_cache_folder_current_folder_changed(GtkFileChooser *sender, gpointer e) {
    prefs_set_string("file_convert_cachedir", gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (sender)));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_bg_threads_value_changed(GtkSpinButton *sender, gpointer e) {
    prefs_set_int("file_convert_max_threads_num", gtk_spin_button_get_value_as_int(sender));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_cache_size_value_changed(GtkSpinButton *sender, gpointer e) {
    prefs_set_int("file_convert_maxdirsize", gtk_spin_button_get_value_as_int(sender));
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_target_format_changed(GtkComboBox *sender, gpointer e) {
    gint index = gtk_combo_box_get_active(sender);
    gchar *script = g_build_filename(get_script_dir(), conv_audio_scripts[index], NULL);
    gint i;

    for (i = 0; i < COUNTOF (modifiable_conv_paths); i++) {
        prefs_set_string(modifiable_conv_paths[i], script);
    }

    prefs_set_int("conversion_target_format", index);
    g_free(script);
    file_convert_prefs_changed();
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_replaygain_clicked(GtkButton *sender, gpointer e) {
    GtkWidget *dlg = GTK_WIDGET(gtk_builder_get_object(builder, "prefs_replaygain_dialog"));
    GtkWidget *mode_album_radio = GTK_WIDGET(gtk_builder_get_object(builder, "mode_album"));
    GtkWidget *mode_track_radio = GTK_WIDGET(gtk_builder_get_object(builder, "mode_track"));

    gtk_window_set_transient_for(GTK_WINDOW (dlg), notebook_get_parent_window());

    gtk_spin_button_set_value(GTK_SPIN_BUTTON (gtk_builder_get_object(builder, "replaygain_offset")), prefs_get_int("replaygain_offset"));

    if (prefs_get_int("replaygain_mode_album_priority"))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (mode_album_radio), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (mode_track_radio), TRUE);

    gtk_builder_connect_signals(builder, NULL);
    gtk_dialog_run(GTK_DIALOG (dlg));
    gtk_widget_hide(dlg);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_replaygain_mode_album_toggled(GtkToggleButton *sender, gpointer e) {
    gboolean active = gtk_toggle_button_get_active(sender);

    prefs_set_int("replaygain_mode_album_priority", active);
}

/*
 glade callback
 */
G_MODULE_EXPORT void on_replaygain_offset_value_changed(GtkSpinButton *sender, gpointer e) {
    prefs_set_int("replaygain_offset", gtk_spin_button_get_value_as_int(sender));
}

static void update_checkbox_deps(GtkToggleButton *checkbox, const gchar *deps) {
    /* Enable or disable dependent checkboxes */
    gboolean active = gtk_toggle_button_get_active(checkbox);
    gchar **deparray;
    int i;

    if (!builder || !deps)
        return;

    deparray = g_strsplit(deps, ",", 0);

    for (i = 0; deparray[i]; i++) {
        GtkWidget *dep = GTK_WIDGET(gtk_builder_get_object(builder, deparray[i]));
        gtk_widget_set_sensitive(dep, active);
    }

    g_strfreev(deparray);
}

static void init_checkbox(GtkToggleButton *checkbox, const gchar *pref, const gchar *deps) {
    g_object_set_data(G_OBJECT(checkbox), "pref", (gchar *) pref);
    g_object_set_data(G_OBJECT(checkbox), "deps", (gchar *) deps);

    if (pref) {
        if (pref[0] == '!') /* Checkbox is !preference */
            gtk_toggle_button_set_active(checkbox, !prefs_get_int(pref + 1));
        else
            gtk_toggle_button_set_active(checkbox, prefs_get_int(pref));
    }

    update_checkbox_deps(checkbox, deps);
}

static void setup_values() {
    gint i;

    GtkWidget *skip_track_update_radio = GTK_WIDGET(gtk_builder_get_object (builder, "skip_track_update"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "agp_track_count")), prefs_get_int("misc_track_nr"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "save_threshold_spin_button")), prefs_get_int("file_saving_threshold"));

    /* Check boxes */
    for (i = 0; i < COUNTOF(checkbox_map); i++) {
        init_checkbox(GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, checkbox_map[i][0])), checkbox_map[i][1], checkbox_map[i][2]);
    }

    for (i = 0; i < COUNTOF(tag_checkbox_map); i++) {
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, tag_checkbox_map[i].string));
        g_object_set_data(G_OBJECT (widget), "index", &tag_checkbox_map[i].index);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), prefs_get_int_index("tag_autoset", tag_checkbox_map[i].index));
    }

    if (!prefs_get_int("update_existing"))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (skip_track_update_radio), TRUE);

    gtk_combo_box_set_active(GTK_COMBO_BOX (gtk_builder_get_object (builder, "target_format")), prefs_get_int("conversion_target_format"));
}

static GtkWidget *create_preference_notebook() {
    GtkWidget *notebook;
    GError* error = NULL;

    g_return_val_if_fail(builder_path, NULL);

    builder = gtk_builder_new();

    gtk_builder_add_from_file(builder, builder_path, &error);
    if (error) {
        g_warning("Failed to load core preferences component because '%s'", error->message);
        g_error_free(error);
        return NULL;
    }

    notebook = GTK_WIDGET (gtk_builder_get_object (builder, "settings_notebook"));
    GtkWidget *parent = gtk_widget_get_parent(notebook);
    g_object_ref(notebook);
    gtk_container_remove(GTK_CONTAINER(parent), notebook);
    gtk_widget_destroy(parent);

    setup_values();

    gtk_builder_connect_signals(builder, NULL);

    return notebook;
}

GtkWidget *init_settings_preferences(gchar *builder_file_path) {

    builder_path = builder_file_path;
    temp_prefs = temp_prefs_create();
    temp_prefs_copy_prefs(temp_prefs);

    notebook = create_preference_notebook();
    return notebook;
}

void destroy_settings_preferences() {
    if (notebook)
        gtk_widget_destroy(notebook);

    if (builder)
        g_object_unref(builder);

    builder_path = NULL;
}
