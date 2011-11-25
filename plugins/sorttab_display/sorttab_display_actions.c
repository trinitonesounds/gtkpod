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
 |  $Id$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "libgtkpod/prefs.h"
#include "sorttab_display_actions.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/file.h"
#include "display_sorttabs.h"
#include "sorttab_widget.h"

void on_more_sort_tabs_activate(GtkAction *action, SorttabDisplayPlugin* plugin) {
    int sort_tab_num = prefs_get_int("sort_tab_num") + 1;
    prefs_set_int("sort_tab_num", sort_tab_num);
    sorttab_display_append_widget();

    gtk_action_set_sensitive(action, sort_tab_num < SORT_TAB_MAX);
    gtk_action_set_sensitive(plugin->fewer_filtertabs_action, sort_tab_num > 0);
}

void on_fewer_sort_tabs_activate(GtkAction *action, SorttabDisplayPlugin* plugin) {
    int sort_tab_num = prefs_get_int("sort_tab_num") - 1;
    prefs_set_int("sort_tab_num", sort_tab_num);
    sorttab_display_remove_widget();

    gtk_action_set_sensitive(plugin->more_filtertabs_action, sort_tab_num < SORT_TAB_MAX);
    gtk_action_set_sensitive(action, sort_tab_num > 1);
}

static void delete_selected_entry(DeleteAction deleteaction, gchar *text) {
    SortTabWidget *st_widget;
    GList *tracks = NULL;
    gint inst;

    st_widget = sorttab_display_get_sort_tab_widget(text);
    if (!SORT_TAB_IS_WIDGET(st_widget))
        return;

    tracks = sort_tab_widget_get_selected_tracks(st_widget);
    inst = sort_tab_widget_get_instance(st_widget);
    if (!tracks) {
        gtkpod_statusbar_message(_("No tracks selected in Filter Tab %d"), inst + 1);
        return;
    }

    sort_tab_widget_delete_entry_head(st_widget, deleteaction);
}

void on_delete_selected_entry_from_database(GtkAction *action, SorttabDisplayPlugin* plugin) {
    delete_selected_entry(DELETE_ACTION_DATABASE, _("Remove entry of which filter tab from database?"));
}

void on_delete_selected_entry_from_ipod(GtkAction *action, SorttabDisplayPlugin* plugin) {
    delete_selected_entry(DELETE_ACTION_IPOD, _("Remove tracks in selected entry of which filter tab from the iPod?"));
}

void on_delete_selected_entry_from_harddisk(GtkAction *action, SorttabDisplayPlugin* plugin) {
    delete_selected_entry(DELETE_ACTION_LOCAL, _("Remove tracks in selected entry of which filter tab from the harddisk?"));
}

void on_delete_selected_entry_from_playlist(GtkAction *action, SorttabDisplayPlugin* plugin) {
    delete_selected_entry(DELETE_ACTION_PLAYLIST, _("Remove tracks in selected entry of which filter tab from playlist?"));
}

void on_delete_selected_entry_from_device(GtkAction *action, SorttabDisplayPlugin* plugin) {
    iTunesDB *itdb = gtkpod_get_current_itdb();
    if (!itdb)
        return;

    if (itdb->usertype & GP_ITDB_TYPE_IPOD) {
        on_delete_selected_entry_from_ipod(action, plugin);
    }
    else if (itdb->usertype & GP_ITDB_TYPE_LOCAL) {
        on_delete_selected_entry_from_harddisk(action, plugin);
    }
}

void on_update_selected_tab_entry (GtkAction *action, SorttabDisplayPlugin* plugin) {
    SortTabWidget *st_widget;
    GList *tracks = NULL;
    gint inst;

    st_widget = sorttab_display_get_sort_tab_widget(_("Update selected entry of which filter tab?"));
    if (!SORT_TAB_IS_WIDGET(st_widget))
        return;

    tracks = sort_tab_widget_get_selected_tracks(st_widget);
    inst = sort_tab_widget_get_instance(st_widget);
    if (!tracks) {
        gtkpod_statusbar_message(_("No entry selected in Filter Tab %d"), inst + 1);
        return;
    }

    update_tracks(tracks);
}
