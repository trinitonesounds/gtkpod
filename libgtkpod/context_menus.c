/*
 |  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
 |  Copyright (C) 2003-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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
#include "itdb.h"
#include "file.h"
#include "misc.h"
#include "misc_track.h"
#include "misc_playlist.h"
#include "prefs.h"
#include "tools.h"
#include "syncdir.h"
#include "track_command_iface.h"

#define LOCALDEBUG 0

/**
 * Attach a menu item to your context menu
 *
 * @m - the GtkMenu we're attaching to
 * @str - a gchar* with the menu label
 * @stock - name of the stock icon (or NULL if none is to be used)
 * @func - the callback for when the item is selected (or NULL)
 * @user_data - parameter to pass into the function callback
 */
GtkWidget *hookup_menu_item(GtkWidget *m, const gchar *str, const gchar *stock, GCallback func, gpointer userdata) {
    GtkWidget *mi;

    if (stock) {
        GtkWidget *image;
        mi = gtk_image_menu_item_new_with_mnemonic(str);
        image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_MENU);
        gtk_widget_show(image);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (mi), image);
    }
    else {
        mi = gtk_menu_item_new_with_label(str);
    }
    gtk_widget_show(mi);
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_add_events(mi, GDK_BUTTON_RELEASE_MASK);
    if (func)
        g_signal_connect(G_OBJECT(mi), "activate", func, userdata);
    gtk_container_add(GTK_CONTAINER (m), mi);
    return mi;
}

GtkWidget *add_sub_menu(GtkWidget *menu, gchar* label, gchar* stockid) {
    GtkWidget *item;
    GtkWidget *submenu;
    GtkWidget *image;

    item = gtk_image_menu_item_new_with_mnemonic(label);
    gtk_widget_set_sensitive(item, TRUE);
    gtk_widget_add_events(item, GDK_BUTTON_RELEASE_MASK);

    image = gtk_image_new_from_stock(stockid, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (item), image);

    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show_all(item);

    return submenu;
}

/**
 *  Add separator to Menu @m and return pointer to separator widget
 */
GtkWidget *add_separator(GtkWidget *menu) {
    GtkWidget *sep = NULL;
    if (menu) {
        sep = gtk_separator_menu_item_new();
        gtk_widget_show(sep);
        gtk_widget_set_sensitive(sep, TRUE);
        gtk_container_add(GTK_CONTAINER (menu), sep);
    }
    return sep;
}

void context_menu_delete_track_head(GtkMenuItem *mi, gpointer data) {
    DeleteAction deleteaction = GPOINTER_TO_INT (data);
    delete_track_head(deleteaction);
}

GtkWidget *add_exec_commands(GtkWidget *menu) {
    GList *trkcmds = gtkpod_get_registered_track_commands();
    GList *cmds = trkcmds;
    gint trksize = g_list_length(trkcmds);
    GtkWidget *mm;

    if (trksize == 0)
        return NULL;
    else if (trksize == 1) {
        mm = menu;
    }
    else {
        GtkWidget *submenu = add_sub_menu(menu, "Execute", GTK_STOCK_EXECUTE);
        mm = submenu;
    }

    while(cmds != NULL) {
        TrackCommand *cmd = cmds->data;
        GPtrArray *pairarr = g_ptr_array_new ();
        g_ptr_array_add (pairarr, cmd);
        g_ptr_array_add (pairarr, gtkpod_get_selected_tracks());
        hookup_menu_item(mm, track_command_get_text(cmd), GTK_STOCK_EXECUTE, G_CALLBACK (on_track_command_menuitem_activate), pairarr);
        cmds = cmds->next;
    }

    return mm;
}

/*
 * update_entries - update the entries currently selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void update_tracks_from_file(GtkMenuItem *mi, gpointer data) {
    update_tracks(gtkpod_get_selected_tracks());
}

GtkWidget *add_update_tracks_from_file(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Update Tracks from File"), GTK_STOCK_REFRESH, G_CALLBACK (update_tracks_from_file), NULL);
}

/*
 * sync_dirs_ entries - sync the directories of the selected playlist
 *
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void sync_dirs(GtkMenuItem *mi, gpointer data) {
    if (gtkpod_get_current_playlist()) {
        sync_playlist(gtkpod_get_current_playlist(), NULL, KEY_SYNC_CONFIRM_DIRS, 0, KEY_SYNC_DELETE_TRACKS, 0, KEY_SYNC_CONFIRM_DELETE, 0, KEY_SYNC_SHOW_SUMMARY, 0);
    }
    else {
        g_return_if_reached ();
    }
}

GtkWidget *add_sync_playlist_with_dirs(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Sync Playlist with Dir(s)"), GTK_STOCK_REFRESH, G_CALLBACK (sync_dirs), NULL);
}

static void copy_selected_tracks_to_target_itdb(GtkMenuItem *mi, gpointer *userdata) {
    iTunesDB *t_itdb = *userdata;
    g_return_if_fail (t_itdb);

    if (gtkpod_get_selected_tracks())
        copy_tracks_to_target_itdb(gtkpod_get_selected_tracks(), t_itdb);
}

static void copy_selected_tracks_to_target_playlist(GtkMenuItem *mi, gpointer *userdata) {
    Playlist *t_pl = *userdata;
    g_return_if_fail (t_pl);

    if (gtkpod_get_selected_tracks())
        copy_tracks_to_target_playlist(gtkpod_get_selected_tracks(), t_pl);
}

GtkWidget *add_copy_selected_tracks_to_target_itdb(GtkWidget *menu, const gchar *title) {
    GtkWidget *mi;
    GtkWidget *sub;
    GtkWidget *pl_mi;
    GtkWidget *pl_sub;
    GList *itdbs;
    GList *db;
    struct itdbs_head *itdbs_head;
    iTunesDB *itdb;
    const gchar *stock_id = NULL;
    Playlist *pl;

    itdbs_head = gp_get_itdbs_head();

    mi = hookup_menu_item(menu, title, GTK_STOCK_COPY, NULL, NULL);
    sub = gtk_menu_new();
    gtk_widget_show(sub);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM (mi), sub);

    for (itdbs = itdbs_head->itdbs; itdbs; itdbs = itdbs->next) {
        itdb = itdbs->data;
        ExtraiTunesDBData *eitdb = itdb->userdata;
        if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
            stock_id = GTK_STOCK_HARDDISK;
        }
        else {
            if (eitdb->itdb_imported) {
                stock_id = GTK_STOCK_CONNECT;
            }
            else {
                stock_id = GTK_STOCK_DISCONNECT;
            }
        }
        pl_mi = hookup_menu_item(sub, _(itdb_playlist_mpl(itdb)->name), stock_id, NULL, NULL);
        pl_sub = gtk_menu_new();
        gtk_widget_show(pl_sub);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM (pl_mi), pl_sub);
        hookup_menu_item(pl_sub, _(itdb_playlist_mpl(itdb)->name), stock_id, G_CALLBACK(copy_selected_tracks_to_target_itdb), &itdbs->data);
        add_separator(pl_sub);
        for (db = itdb->playlists; db; db = db->next) {
            pl = db->data;
            if (!itdb_playlist_is_mpl(pl)) {
                if (pl->is_spl)
                    stock_id = GTK_STOCK_PROPERTIES;
                else
                    stock_id = GTK_STOCK_JUSTIFY_LEFT;
                hookup_menu_item(pl_sub, _(pl->name), stock_id, G_CALLBACK(copy_selected_tracks_to_target_playlist), &db->data);
            }
        }
    }
    return mi;
}

static void create_playlist_from_entries(GtkMenuItem *mi, gpointer data) {
    if (gtkpod_get_current_itdb() && gtkpod_get_selected_tracks())
        generate_new_playlist(gtkpod_get_current_itdb(), gtkpod_get_selected_tracks());
}

GtkWidget *add_create_new_playlist(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Create new Playlist"), GTK_STOCK_JUSTIFY_LEFT, G_CALLBACK (create_playlist_from_entries), NULL);
}

/**
 * create_playlist_file - write a playlist file containing the
 * currently selected tracks.
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void create_playlist_file(GtkWidget *w, gpointer data)
{
    if (!gtkpod_has_exporter())
        return;

    Exporter *exporter = gtkpod_get_exporter();

    if(gtkpod_get_selected_tracks())
        exporter_export_tracks_to_playlist_file(exporter, gtkpod_get_selected_tracks());
}

GtkWidget *add_create_playlist_file (GtkWidget *menu)
{
    if (!gtkpod_has_exporter())
        return NULL;

    return hookup_menu_item (menu, _("Create Playlist File"),
              GTK_STOCK_SAVE_AS,
              G_CALLBACK (create_playlist_file), NULL);
}

/* Display track details options */
static void edit_track_details(GtkMenuItem *mi, gpointer data) {
    g_return_if_fail (gtkpod_get_selected_tracks());

    gtkpod_edit_details(gtkpod_get_selected_tracks());
}

GtkWidget *add_edit_track_details(GtkWidget *menu) {
    if (!gtkpod_has_details_editor())
        return menu;

    return hookup_menu_item(menu, _("Edit Track Details"), GTK_STOCK_PREFERENCES, G_CALLBACK (edit_track_details), NULL);
}

/**
 * export_entries - export the currently selected files to disk
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void export_entries(GtkWidget *w, gpointer data)
{
    Exporter *exporter = gtkpod_get_exporter();
    g_return_if_fail(exporter);

    if(gtkpod_get_selected_tracks())
        exporter_export_tracks_as_files (exporter, gtkpod_get_selected_tracks(), NULL, FALSE, NULL);
}

GtkWidget *add_copy_track_to_filesystem (GtkWidget *menu)
{
    if (! gtkpod_has_exporter())
        return NULL;

    return hookup_menu_item (menu, _("Copy Tracks to Filesystem"),
              GTK_STOCK_SAVE_AS,
              G_CALLBACK (export_entries), NULL);
}
