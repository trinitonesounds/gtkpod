/* Time-stamp: <2008-09-30 23:36:02 jcs>
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

/* This file provides functions to initialize a new iPod */

#include "plugin.h"
#include "repository.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/file.h"

struct _IpodInit {
    GtkBuilder *builder; /* XML info                             */
    GtkWidget *window; /* pointer to repository window         */
    iTunesDB *itdb;
};

typedef struct _IpodInit IpodInit;

/* Strings used several times */
const gchar *SELECT_OR_ENTER_YOUR_MODEL = N_("Select or enter your model");

/* string constants for window widgets used more than once */
static const gchar *IID_MOUNTPOINT_CHOOSER = "iid_mountpoint_chooser";
static const gchar *IID_MODEL_COMBO = "iid_model_combo";
static const gchar *SIMD_MODEL_COMBO = "simd_model_combo";
static const gchar *SIMD_LABEL = "simd_label";

void set_cell(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data) {
    gboolean header;
    gchar *text;
    IpodInfo *info;

    gtk_tree_model_get(tree_model, iter, COL_POINTER, &info, -1);
    g_return_if_fail (info);

    header = gtk_tree_model_iter_has_child(tree_model, iter);

    if (header) {
        text = g_strdup(itdb_info_get_ipod_generation_string(info->ipod_generation));
    }
    else {
        if (info->capacity >= 1) { /* size in GB */
            text
                    = g_strdup_printf("%2.0f GB %s (x%s)", info->capacity, itdb_info_get_ipod_model_name_string(info->ipod_model), info->model_number);
        }
        else if (info->capacity > 0) { /* size in MB */
            text
                    = g_strdup_printf("%3.0f MB %s (x%s)", info->capacity * 1024, itdb_info_get_ipod_model_name_string(info->ipod_model), info->model_number);
        }
        else { /* no capacity information available */
            text
                    = g_strdup_printf("%s (x%s)", itdb_info_get_ipod_model_name_string(info->ipod_model), info->model_number);
        }
    }

    g_object_set(cell, "sensitive", !header, "text", text, NULL);

    g_free(text);
}

/**
 * repository_ipod_init:
 *
 * Ask for the iPod model and mountpoint and then create the directory
 * structure on the iPod.
 *
 * @itdb: itdb from where to extract the mountpoint. After
 * initialisation the model number is set.
 */
gboolean repository_ipod_init(iTunesDB *itdb) {
    IpodInit *ii;
    gint response;
    gboolean result = FALSE;
    gchar *mountpoint, *new_mount, *name, *model;
    GError *error = NULL;
    GtkEntry *entry;
    gchar buf[PATH_MAX];
    GtkComboBox *cb;
    const IpodInfo *info;
    GtkTreeIter iter;

    g_return_val_if_fail (itdb, FALSE);

    /* Create window */
    ii = g_new0 (IpodInit, 1);
    ii->itdb = itdb;

    ii->builder = init_repository_builder();

    ii->window = gtkpod_builder_xml_get_widget(ii->builder, "ipod_init_dialog");
    g_return_val_if_fail (ii->window, FALSE);

    /* Set mountpoint */
    mountpoint = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);
    if (mountpoint) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(GET_WIDGET (ii->builder, IID_MOUNTPOINT_CHOOSER)), mountpoint);
    }

    /* Setup model number combo */
    cb = GTK_COMBO_BOX (GET_WIDGET (ii->builder, IID_MODEL_COMBO));
    repository_init_model_number_combo(cb);

    /* If available set current model number, otherwise indicate that
     none is available */
    info = itdb_device_get_ipod_info(itdb->device);
    if (info && (info->ipod_generation != ITDB_IPOD_GENERATION_UNKNOWN)) {
        g_snprintf(buf, PATH_MAX, "x%s", info->model_number);
    }
    else {
        model = get_itdb_prefs_string(itdb, KEY_IPOD_MODEL);
        if (model && (strlen(g_strstrip (model)) != 0)) {
            g_snprintf(buf, PATH_MAX, "%s", model);
            g_free(model);
        }
        else {
            g_snprintf(buf, PATH_MAX, "%s", gettext (SELECT_OR_ENTER_YOUR_MODEL));
        }
    }
    entry = GTK_ENTRY (gtk_bin_get_child(GTK_BIN (cb)));
    gtk_entry_set_text(entry, buf);

    gtk_window_set_transient_for(GTK_WINDOW (ii->window), GTK_WINDOW (gtkpod_app));
    response = gtk_dialog_run(GTK_DIALOG (ii->window));

    switch (response) {
    case GTK_RESPONSE_OK:
        new_mount = g_strdup(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(GET_WIDGET (ii->builder, IID_MOUNTPOINT_CHOOSER))));
        /* remove trailing '/' in case it's present. */
        if (mountpoint && (strlen(mountpoint) > 0)) {
            if (G_IS_DIR_SEPARATOR(mountpoint[strlen(mountpoint) - 1])) {
                mountpoint[strlen(mountpoint) - 1] = 0;
            }
        }
        if (new_mount && (strlen(new_mount) > 0)) {
            if (G_IS_DIR_SEPARATOR(new_mount[strlen(new_mount) - 1])) {
                new_mount[strlen(new_mount) - 1] = 0;
            }
        }
        if (!(mountpoint && new_mount && (strcmp(mountpoint, new_mount) == 0))) { /* mountpoint has changed */
            g_free(mountpoint);
            mountpoint = new_mount;
            new_mount = NULL;
            set_itdb_prefs_string(itdb, KEY_MOUNTPOINT, mountpoint);
            call_script("gtkpod.load", mountpoint, NULL);
            itdb_set_mountpoint(itdb, mountpoint);
        }
        else {
            g_free(new_mount);
            new_mount = NULL;
        }

        g_return_val_if_fail(gtk_combo_box_get_active_iter(cb, &iter), FALSE);
        gtk_tree_model_get(gtk_combo_box_get_model(cb), &iter, COL_STRING, &model, -1);
        g_return_val_if_fail(model, FALSE);

        if ((strcmp(model, gettext(SELECT_OR_ENTER_YOUR_MODEL)) == 0) || (strlen(model) == 0)) { /* User didn't choose a model */
            g_free(model);
            model = NULL;
        }

        /* Set model in the prefs system */
        set_itdb_prefs_string(itdb, KEY_IPOD_MODEL, model);

        name = get_itdb_prefs_string(itdb, "name");
        result = itdb_init_ipod(mountpoint, model, name, &error);
        if (!result) {
            if (error) {
                gtkpod_warning(_("Error initialising iPod: %s\n"), error->message);
                g_error_free(error);
                error = NULL;
            }
            else {
                gtkpod_warning(_("Error initialising iPod, unknown error\n"));
            }
        }
        g_free(name);
        g_free(model);
        break;
    default:
        /* canceled -- do nothing */
        break;
    }

    gtk_widget_destroy(ii->window);

    g_free(mountpoint);

    g_free(ii);

    return result;
}

/**
 * repository_ipod_init_set_model:
 *
 * Ask for the iPod model, pre-select @old_model, set the selected
 * model in the preferences.
 *
 * @itdb: the itdb to set
 * @old_model: the model number string to initially propose.
 */
void repository_ipod_init_set_model(iTunesDB *itdb, const gchar *old_model) {
    GtkBuilder *builder;
    GtkWidget *window;
    gint response;
    gchar *model, *mountpoint;
    GtkEntry *entry;
    gchar buf[PATH_MAX];
    GtkComboBox *cb;
    const IpodInfo *info;
    GtkTreeIter iter;

    g_return_if_fail (itdb);

    /* Create window */
    builder = init_repository_builder();
    window = GET_WIDGET (builder, "set_ipod_model_dialog");
    g_return_if_fail (window);

    /* Set up label */
    mountpoint = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);
    gchar *displaymp = g_uri_unescape_string(mountpoint, NULL);
    g_return_if_fail (mountpoint);
    g_snprintf(buf, PATH_MAX, _("<b>Please select your iPod model at </b><i>%s</i>"), displaymp);
    gtk_label_set_markup(GTK_LABEL (GET_WIDGET (builder, SIMD_LABEL)), buf);
    g_free(mountpoint);
    g_free(displaymp);

    /* Setup model number combo */
    cb = GTK_COMBO_BOX (GET_WIDGET (builder, SIMD_MODEL_COMBO));
    repository_init_model_number_combo(cb);

    /* If available set current model number, otherwise indicate that
     none is available */
    info = itdb_device_get_ipod_info(itdb->device);
    if (info && (info->ipod_generation != ITDB_IPOD_GENERATION_UNKNOWN)) {
        g_snprintf(buf, PATH_MAX, "x%s", info->model_number);
    }
    else {
        model = get_itdb_prefs_string(itdb, KEY_IPOD_MODEL);
        if (model && (strlen(g_strstrip (model)) != 0)) {
            g_snprintf(buf, PATH_MAX, "%s", model);
            g_free(model);
        }
        else {
            g_snprintf(buf, PATH_MAX, "%s", gettext (SELECT_OR_ENTER_YOUR_MODEL));
        }
    }

    entry = GTK_ENTRY (gtk_bin_get_child(GTK_BIN (cb)));
    gtk_entry_set_text(entry, buf);

    response = gtk_dialog_run(GTK_DIALOG (window));

    switch (response) {
    case GTK_RESPONSE_OK:
        g_return_if_fail(gtk_combo_box_get_active_iter(cb, &iter));
        gtk_tree_model_get(gtk_combo_box_get_model(cb), &iter, COL_STRING, &model, -1);
        if (!model) {
            gtkpod_warning(_("Could not determine the model you selected -- this could be a bug or incompatibilty in the GTK+ or glade library.\n\n"));
        }
        else if (strcmp(model, gettext(SELECT_OR_ENTER_YOUR_MODEL)) == 0) { /* User didn't choose a model */
            g_free(model);
            model = NULL;
        }
        if (model) {
            /* Set model in the prefs system */
            set_itdb_prefs_string(itdb, KEY_IPOD_MODEL, model);
            /* Set the model on the iPod */
            itdb_device_set_sysinfo(itdb->device, "ModelNumStr", model);
            g_free(model);
        }
        break;
    default:
        /* canceled -- do nothing */
        break;
    }

    gtk_widget_destroy(window);
}
