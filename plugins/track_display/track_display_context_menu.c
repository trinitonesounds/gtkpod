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
/**
 * pm_context_menu_init - initialize the right click menu for playlists
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/misc.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/context_menus.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/gp_private.h"
#include "libgtkpod/misc_track.h"
#include "display_tracks.h"
#include "track_display_context_menu.h"

/* Select All */
static void select_all(GtkMenuItem *mi, gpointer data) {
    tm_select_all_tracks();
}

static GtkWidget *add_select_all(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Select All"), GTK_STOCK_SELECT_ALL, G_CALLBACK (select_all), NULL);
}

static void context_menu_delete_tracks_head(GtkMenuItem *mi, gpointer data) {
    DeleteAction deleteaction = GPOINTER_TO_INT (data);
    delete_track_head(deleteaction);
}

static GtkWidget *add_delete_tracks_from_ipod(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Delete From iPod"), GTK_STOCK_DELETE, G_CALLBACK (context_menu_delete_tracks_head), GINT_TO_POINTER (DELETE_ACTION_IPOD));
}

static GtkWidget *add_delete_tracks_from_playlist(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Delete From Playlist"), GTK_STOCK_DELETE, G_CALLBACK (context_menu_delete_tracks_head), GINT_TO_POINTER (DELETE_ACTION_PLAYLIST));
}

static GtkWidget *add_delete_tracks_from_harddisk(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Delete From Harddisk"), GTK_STOCK_DELETE, G_CALLBACK (context_menu_delete_tracks_head), GINT_TO_POINTER (DELETE_ACTION_LOCAL));
}

static GtkWidget *add_delete_tracks_from_database(GtkWidget *menu) {
    return hookup_menu_item(menu, _("Delete From Database"), GTK_STOCK_DELETE, G_CALLBACK (context_menu_delete_tracks_head), GINT_TO_POINTER (DELETE_ACTION_DATABASE));
}

static void copy_selected_tracks_to_target_itdb(GtkMenuItem *mi, gpointer *userdata) {
    iTunesDB *t_itdb = *userdata;
    g_return_if_fail (t_itdb);
    if (tm_get_selected_tracks())
        copy_tracks_to_target_itdb(tm_get_selected_tracks(), t_itdb);
}

static void copy_selected_tracks_to_target_playlist(GtkMenuItem *mi, gpointer *userdata) {
    Playlist *t_pl = *userdata;
    g_return_if_fail (t_pl);
    if (tm_get_selected_tracks())
        copy_tracks_to_target_playlist(tm_get_selected_tracks(), t_pl);
}

static GtkWidget *add_copy_selected_tracks_to_target_itdb(GtkWidget *menu, const gchar *title) {
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

/**
 * tm_context_menu_init - initialize the right click menu for tracks
 */
void tm_context_menu_init(void) {
    if (widgets_blocked)
        return;

    GtkWidget *menu = NULL;
    Playlist *pl;

    pl = gtkpod_get_current_playlist();
    if (!pl)
        return;

    // Ensure that all the tracks in the view are the current selected tracks
    gtkpod_set_selected_tracks(tm_get_selected_tracks());

    ExtraiTunesDBData *eitdb;
    iTunesDB *itdb = pl->itdb;
    g_return_if_fail (itdb);
    eitdb = itdb->userdata;
    g_return_if_fail (eitdb);

    menu = gtk_menu_new();

    add_exec_commands(menu);
    add_separator(menu);

    GtkWidget *create_menu = add_sub_menu(menu, "Create Playlist", GTK_STOCK_NEW);
    add_create_playlist_file(create_menu);
    add_create_new_playlist(create_menu);
    add_separator(menu);

    GtkWidget *copy_menu = add_sub_menu(menu, "Copy", GTK_STOCK_COPY);
    add_copy_track_to_filesystem(copy_menu);
    add_copy_selected_tracks_to_target_itdb(copy_menu, _("Copy selected track(s) to..."));
    add_separator(menu);

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        if (!itdb_playlist_is_mpl(pl)) {
            GtkWidget *delete_menu = add_sub_menu(menu, "Delete", GTK_STOCK_DELETE);
            add_delete_tracks_from_ipod(delete_menu);
            add_delete_tracks_from_playlist(delete_menu);
        } else {
            add_delete_tracks_from_ipod(menu);
        }
    }
    if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        GtkWidget *delete_menu = add_sub_menu(menu, "Delete", GTK_STOCK_DELETE);
        add_delete_tracks_from_harddisk(delete_menu);
        add_delete_tracks_from_database(delete_menu);
        if (!itdb_playlist_is_mpl(pl)) {
            add_delete_tracks_from_playlist(delete_menu);
        }
    }

    add_separator(menu);
    add_update_tracks_from_file(menu);
    add_edit_track_details(menu);
    add_separator(menu);
    add_select_all(menu);

    /*
     * button should be button 0 as per the docs because we're calling
     * from a button release event
     */
    if (menu) {
        gtk_menu_popup(GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
    }
}
