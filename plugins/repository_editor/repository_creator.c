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

#include "plugin.h"
#include "repository.h"
#include <gtk/gtk.h>
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/misc.h"

/* widget names for the "Create New Repository" window */
#define CRW_BACKUP_CHOOSER "crw_backup_chooser"
#define CRW_BACKUP_LABEL "crw_backup_label"
#define CRW_CANCEL_BUTTON "crw_cancel_button"
#define CRW_INSERT_BEFORE_AFTER_COMBO "crw_insert_before_after_combo"
#define CRW_IPOD_MODEL_LABEL "crw_ipod_model_label"
#define CRW_LOCAL_PATH_CHOOSER "crw_local_path_chooser"
#define CRW_LOCAL_PATH_LABEL "crw_local_path_label"
#define CRW_MOUNTPOINT_CHOOSER "crw_mountpoint_chooser"
#define CRW_MOUNTPOINT_LABEL "crw_mountpoint_label"
#define CRW_OK_BUTTON "crw_ok_button"
#define CRW_REPOSITORY_COMBO "crw_repository_combo"
#define CRW_REPOSITORY_NAME_ENTRY "crw_repository_name_entry"
#define CRW_REPOSITORY_TYPE_COMBO "crw_repository_type_combo"

static CreateRepWindow *createrep = NULL;

/* repository types */
enum {
    REPOSITORY_TYPE_IPOD = 0, REPOSITORY_TYPE_LOCAL = 1, REPOSITORY_TYPE_PODCAST = 2,
};
/* before/after */
enum {
    INSERT_BEFORE = 0, INSERT_AFTER = 1,
};

/* ------------------------------------------------------------
 *
 *        Callback functions (windows control)
 *
 * ------------------------------------------------------------ */

/* Free memory taken by @createrep */
static void createrep_free(CreateRepWindow *cr) {
    g_return_if_fail (cr);

    g_object_unref(cr->builder);

    if (cr->window) {
        gtk_widget_destroy(cr->window);
    }

    g_free(cr);
}

static void create_cancel_clicked(GtkButton *button, CreateRepWindow *cr) {
    g_return_if_fail (cr);

    createrep_free(cr);

    createrep = NULL;
}

static void create_ok_clicked(GtkButton *button, CreateRepWindow *cr) {
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
    type = gtk_combo_box_get_active(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_REPOSITORY_TYPE_COMBO)));

    bef_after = gtk_combo_box_get_active(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_INSERT_BEFORE_AFTER_COMBO)));

    itdb_index = gtk_combo_box_get_active(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_REPOSITORY_COMBO)));

    name = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (cr->builder, CRW_REPOSITORY_NAME_ENTRY)));

    mountpoint = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (GET_WIDGET (cr->builder, CRW_MOUNTPOINT_CHOOSER)));

    backup = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (GET_WIDGET (cr->builder, CRW_BACKUP_CHOOSER)));

    ipod_model = gtk_entry_get_text(GTK_ENTRY (GET_WIDGET (cr->builder, CRW_IPOD_MODEL_ENTRY)));
    if (strcmp(ipod_model, gettext(SELECT_OR_ENTER_YOUR_MODEL)) == 0) { /* User didn't choose a model */
        ipod_model = "";
    }

    local_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (GET_WIDGET (cr->builder, CRW_LOCAL_PATH_CHOOSER)));

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

    /* Finish */
    create_cancel_clicked(NULL, cr);
}

static void create_delete_event(GtkWidget *widget, GdkEvent *event, CreateRepWindow *cr) {
    create_cancel_clicked(NULL, cr);
}

/* ------------------------------------------------------------
 *
 *        Callback (repository type)
 *
 * ------------------------------------------------------------ */
static void show_hide_widgets(CreateRepWindow *cr, int index) {
    gint i;
    const gchar **show = NULL;
    /* widgets to show for iPod repositories */
    const gchar *show_ipod[] =
        {
            CRW_MOUNTPOINT_LABEL,
            CRW_MOUNTPOINT_CHOOSER,
            CRW_BACKUP_LABEL,
            CRW_BACKUP_CHOOSER,
            CRW_IPOD_MODEL_LABEL,
            CRW_IPOD_MODEL_COMBO,
            NULL
        };
    /* widgets to show for local repositories */
    const gchar *show_local[] =
        {
            CRW_LOCAL_PATH_LABEL,
            CRW_LOCAL_PATH_CHOOSER,
            NULL
        };
    /* list of all widgets that get hidden */
    const gchar *hide_all[] =
        {
            CRW_MOUNTPOINT_LABEL,
            CRW_MOUNTPOINT_CHOOSER,
            CRW_BACKUP_LABEL,
            CRW_BACKUP_CHOOSER,
            CRW_IPOD_MODEL_LABEL,
            CRW_IPOD_MODEL_COMBO,
            CRW_LOCAL_PATH_LABEL,
            CRW_LOCAL_PATH_CHOOSER,
            NULL
        };

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
        gtk_widget_hide(GET_WIDGET (cr->builder, hide_all[i]));
    }
    /* Show appropriate widgets */
    for (i = 0; show[i]; ++i) {
        gtk_widget_show(GET_WIDGET (cr->builder, show[i]));
    }
}

static void cr_repository_type_changed(GtkComboBox *cb, CreateRepWindow *cr) {
    gint index = 0;

    index = gtk_combo_box_get_active(cb);
    show_hide_widgets(cr, index);
}

/**
 * create_repository: Create a new repository.
 *
 * @repwin: if given, @repwin will be updated.
 *
 * Note: this is a modal dialog.
 */

void display_create_repository_dialog() {
    CreateRepWindow *cr;
    GtkComboBox *model_number_combo;
    gchar *str, *buf1, *buf2;
    struct itdbs_head *itdbs_head = gp_get_itdbs_head();

    createrep = g_malloc0(sizeof(CreateRepWindow));
    cr = createrep;

    cr->builder =init_repository_builder();
    cr->window = gtkpod_builder_xml_get_widget(cr->builder, "create_repository_window");
    g_return_if_fail (cr->window);

    gtk_window_set_transient_for(GTK_WINDOW(cr->window), GTK_WINDOW (gtkpod_app));

    /* Window control */
    g_signal_connect (GET_WIDGET (cr->builder, CRW_CANCEL_BUTTON), "clicked",
            G_CALLBACK (create_cancel_clicked), cr);

    g_signal_connect (GET_WIDGET (cr->builder, CRW_OK_BUTTON), "clicked",
            G_CALLBACK (create_ok_clicked), cr);

    g_signal_connect (createrep->window, "delete_event",
            G_CALLBACK (create_delete_event), cr);

    /* Combo callback */
    g_signal_connect (GET_WIDGET (cr->builder, CRW_REPOSITORY_TYPE_COMBO), "changed",
            G_CALLBACK (cr_repository_type_changed), cr);

    /* Setup model number combo */
    model_number_combo = GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_IPOD_MODEL_COMBO));
    repository_init_model_number_combo(model_number_combo);
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (cr->builder, CRW_IPOD_MODEL_ENTRY)), gettext (SELECT_OR_ENTER_YOUR_MODEL));

    /* Set initial repository type */
    gtk_combo_box_set_active(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_REPOSITORY_TYPE_COMBO)), REPOSITORY_TYPE_IPOD);

    /* Set before/after combo */
    gtk_combo_box_set_active(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_INSERT_BEFORE_AFTER_COMBO)), INSERT_AFTER);

    /* Set up repository combo */
    repository_combo_populate(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_REPOSITORY_COMBO)));
    gtk_combo_box_set_active(GTK_COMBO_BOX (GET_WIDGET (cr->builder, CRW_REPOSITORY_COMBO)), 0);

    /* Set default repository name */
    gtk_entry_set_text(GTK_ENTRY (GET_WIDGET (cr->builder, CRW_REPOSITORY_NAME_ENTRY)), _("New Repository"));

    /* Set initial mountpoint */
    str = prefs_get_string("initial_mountpoint");
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (GET_WIDGET (cr->builder, CRW_MOUNTPOINT_CHOOSER)), str);
    g_free(str);

    buf1 = prefs_get_cfgdir();
    g_return_if_fail (buf1);
    /* Set initial backup path */
    buf2 = g_strdup_printf("backupDB_%d", g_list_length(itdbs_head->itdbs));
    str = g_build_filename(buf1, buf2, NULL);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (GET_WIDGET (cr->builder, CRW_BACKUP_CHOOSER)), str);
    g_free(str);
    g_free(buf2);

    /* Set local repository file */
    buf2 = g_strdup_printf("local_%d.itdb", g_list_length(itdbs_head->itdbs));
    str = g_build_filename(buf1, buf2, NULL);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (GET_WIDGET (cr->builder, CRW_LOCAL_PATH_CHOOSER)), str);
    g_free(str);
    g_free(buf2);
    g_free(buf1);

    gtk_widget_show_all(cr->window);

    show_hide_widgets(cr, 0);
}
