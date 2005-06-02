;;/* Time-stamp: <2005-06-02 23:11:42 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
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

/* Most function prototypes in this file were written by glade2. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "display.h"
#include "display_itdb.h"
#include "file.h"
#include "fileselection.h"
#include "info.h"
#include "misc.h"
#include "misc_track.h"
#include "tools.h"
#include "prefs.h"
#include "prefs_window.h"

void
on_add_files1_activate                 (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    create_add_files_dialog ();
}


void
on_add_directory1_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  dirbrowser_create ();
}


void
on_export_itunes1_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  handle_export ();
}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  if (!widgets_blocked) gtkpod_main_quit ();
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  open_about_window (); /* in misc.c */
}


void
on_add_files1_button                   (GtkButton       *button,
					gpointer         user_data)
{
  create_add_files_dialog ();
}


void
on_add_directory1_button               (GtkButton       *button,
					gpointer         user_data)
{
  dirbrowser_create ();
}

void
on_export_itunes1_button               (GtkButton       *button,
					gpointer         user_data)
{
  handle_export ();
}

gboolean
on_gtkpod_delete_event                 (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
    if (!widgets_blocked)
    {
	return gtkpod_main_quit ();
    }
    return TRUE; /* don't quit -- would cause numerous error messages */
}

gboolean
on_about_window_close                  (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
  close_about_window (); /* in misc.c */
  return FALSE;
}


void
on_about_window_close_button           (GtkButton       *button,
					gpointer         user_data)
{
  close_about_window (); /* in misc.c */
}

void
on_new_playlist_button                 (GtkButton       *button,
					gpointer         user_data)
{
  add_new_pl_or_spl_user_name (gp_get_active_itdb (), NULL, -1);
}

void
on_sorttab_switch_page                 (GtkNotebook     *notebook,
					GtkNotebookPage *page,
					guint            page_num,
					gpointer         user_data)
{
    space_data_update ();
    st_page_selected (notebook, page_num);
}


gboolean
on_prefs_window_delete_event           (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
  prefs_window_delete ();
  gtkpod_statusbar_message(_("Preferences not updated"));
  return FALSE;
}


void
on_prefs_ok_clicked                    (GtkButton       *button,
					gpointer         user_data)
{
    prefs_window_ok();
}


void
on_prefs_cancel_clicked                (GtkButton       *button,
					gpointer         user_data)
{
    prefs_window_cancel();
    gtkpod_statusbar_message(_("Preferences not updated"));
}


void
on_prefs_apply_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    prefs_window_apply ();
    gtkpod_statusbar_message(_("Preferences applied"));
}

void
on_edit_preferences1_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if(!widgets_blocked)  prefs_window_create();
}

void
on_offline1_activate                   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  prefs_set_offline (
     gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
  display_set_check_ipod_menu ();
}

void
on_import_itunes_mi_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  gp_merge_ipod_itdbs ();
}


void
on_import_button_clicked               (GtkButton       *button,
					gpointer         user_data)
{
  gp_merge_ipod_itdbs ();
}


void
on_delete_tracks_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    g_print ("not supported yet");
//    delete_track_head (FALSE);
}


void
on_delete_playlist_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    g_print ("not supported yet");
//    delete_playlist_head (FALSE);
}

void
on_delete_tab_entry_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    g_print ("not supported yet");
/*     gint inst = get_sort_tab_number ( */
/* 	_("Delete selected entry of which sort tab?")); */

/*     if (inst != -1)   delete_entry_head (inst, FALSE); */
}

void
on_delete_full_tracks_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    g_print ("not supported yet");
//     delete_track_head (TRUE);
}


void
on_delete_full_playlist_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    g_print ("not supported yet");
//    delete_playlist_head (TRUE);
}

void
on_delete_full_tab_entry_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    g_print ("not supported yet");
/*     gint inst = get_sort_tab_number ( */
/* 	_("Delete selected entry of which sort tab?")); */

/*     if (inst != -1) */
/*     { */
/* 	delete_entry_head (inst, TRUE); */
/*     } */
}

void
on_ipod_directories_menu               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_active_itdb ();
    if (!itdb)
    {
	gtkpod_statusbar_message (_("Currently no iPod database selected"));
    }
    else
    {
	ipod_directories_head (itdb->mountpoint);
    }
}

void
on_stop_button_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    display_stop_update (-1);
}

void
on_add_PL_button_clicked               (GtkButton       *button,
					gpointer         user_data)
{
    create_add_playlists_dialog ();
}


void
on_add_playlist1_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    create_add_playlists_dialog ();
}

void
on_update_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    gp_do_selected_playlist (update_tracks);
}

/* update tracks in tab entry */
void
on_update_tab_entry_activate        (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) gp_do_selected_entry (update_tracks, inst);
}

void
on_update_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gp_do_selected_tracks (update_tracks);
}


void
on_mserv_from_file_tracks_menu_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gp_do_selected_tracks (mserv_from_file_tracks);
}


void
on_mserv_from_file_entry_menu_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Update selected entry of which sort tab?"));

    if (inst != -1) gp_do_selected_entry (mserv_from_file_tracks, inst);
}


void
on_sync_playlist_activate (GtkMenuItem     *menuitem,
			     gpointer         user_data)
{
    gp_do_selected_playlist (sync_tracks);
}

/* sync tracks in tab entry */
void
on_sync_tab_entry_activate        (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gint inst = get_sort_tab_number (
	_("Sync dirs of selected entry in which sort tab?"));

    if (inst != -1) gp_do_selected_entry (sync_tracks, inst);
}

void
on_sync_tracks_activate            (GtkMenuItem     *menuitem,
				     gpointer         user_data)
{
    gp_do_selected_tracks (sync_tracks);
}


void
on_save_track_order1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tm_rows_reordered ();
    pm_rows_reordered ();
}


void
on_toolbar_menu_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_display_toolbar (
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}


void
on_more_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_sort_tab_num (prefs_get_sort_tab_num()+1, TRUE);
}


void
on_less_sort_tabs_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_sort_tab_num (prefs_get_sort_tab_num()-1, TRUE);
}

void
on_export_playlist_activate  (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (!pl)
    {
	gtkpod_statusbar_message (_("No playlist selected"));
	return;
    }
    export_files_init (pl->members);
}


void
on_export_tab_entry_activate (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Export selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    export_files_init (entry->members);
}


void
on_export_tracks_activate     (GtkMenuItem     *menuitem,
			      gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	export_files_init(tracks);
	g_list_free (tracks);
    }
    else
    {
	gtkpod_statusbar_message (_("No tracks selected"));
    }
}


void
on_playlist_file_playlist_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();

    if (!pl)
    {
	gtkpod_statusbar_message (_("No playlist selected"));
	return;
    }
    export_playlist_file_init (pl->members);
}


void
on_playlist_file_tab_entry_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Create playlist file from selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    export_playlist_file_init (entry->members);
}


void
on_playlist_file_tracks_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();

    if (tracks)
    {
	export_playlist_file_init(tracks);
	g_list_free (tracks);
    }
    else
    {
	gtkpod_statusbar_message (_("No tracks selected"));
    }
}

void
on_play_playlist_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	tools_play_tracks (pl->members);
    else
	gtkpod_statusbar_message (_("No playlist selected"));
}


void
on_play_tab_entry_activate             (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Play tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    tools_play_tracks (entry->members);
}


void
on_play_tracks_activate                 (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();
    if (tracks)
    {
	tools_play_tracks (tracks);
	g_list_free (tracks);
	tracks = NULL;
    }
    else
	gtkpod_statusbar_message (_("No tracks selected"));
}


void
on_enqueue_playlist_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	tools_enqueue_tracks (pl->members);
    else
	gtkpod_statusbar_message (_("No playlist selected"));
}


void
on_enqueue_tab_entry_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Enqueue tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (!entry)
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
    tools_enqueue_tracks (entry->members);
}


void
on_enqueue_tracks_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_selected_tracks ();
    if (tracks)
    {
	tools_enqueue_tracks (tracks);
	g_list_free (tracks);
	tracks = NULL;
    }
    else
	gtkpod_statusbar_message (_("No tracks selected"));
}


void
on_arrange_sort_tabs_activate          (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    st_arrange_visible_sort_tabs ();
}


void
on_sp_or_button_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_or (inst, gtk_toggle_button_get_active (togglebutton));
    sp_conditions_changed (inst);
}


void
on_sp_cond_button_toggled            (GtkToggleButton *togglebutton,
				      gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item cond = (guint32)user_data >> SP_SHIFT;

/*     printf ("%d/%d/%d\n",inst,cond,gtk_toggle_button_get_active (togglebutton)); */
    prefs_set_sp_cond (inst, cond,
		       gtk_toggle_button_get_active (togglebutton));
    sp_conditions_changed (inst);
}

void
on_sp_rating_n_toggled                 (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    guint32 n = (guint32)user_data >> SP_SHIFT;

    prefs_set_sp_rating_n (inst, n,
			   gtk_toggle_button_get_active (togglebutton));
    if (prefs_get_sp_cond (inst, T_RATING))
	sp_conditions_changed (inst);
}


void
on_sp_entry_activate             (GtkEditable     *editable,
				  gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item item = (guint32)user_data >> SP_SHIFT;
    gchar *buf = gtk_editable_get_chars(editable,0, -1);

/*    printf ("sp_entry_activate inst: %d, item: %d\n", inst, item);*/

    prefs_set_sp_entry (inst, item, buf);
    g_free (buf);
    sp_update_date_interval_from_string (inst, item, TRUE);
/*     if (prefs_get_sp_autodisplay (inst))  sp_go (inst); */
    sp_go (inst);
}


void
on_sp_cal_button_clicked        (GtkButton       *button,
				 gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item item = (guint32)user_data >> SP_SHIFT;

    cal_open_calendar (inst, item);
}


void
on_sp_go_clicked                       (GtkButton       *button,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    sp_go (inst);
}


void
on_sp_go_always_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    gboolean state = gtk_toggle_button_get_active (togglebutton);

    /* display data if autodisplay is turned on */
    if (state)  on_sp_go_clicked (NULL, user_data);
    prefs_set_sp_autodisplay(inst, state);
}

void
on_sp_playcount_low_value_changed      (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_playcount_low (inst,
				gtk_spin_button_get_value (spinbutton));
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
	sp_conditions_changed (inst);
}


void
on_sp_playcount_high_value_changed     (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_playcount_high (inst,
				 gtk_spin_button_get_value (spinbutton));
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
	sp_conditions_changed (inst);
}

void
on_tooltips_menu_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    prefs_set_display_tooltips_main (
	gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}


void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  add_new_pl_or_spl_user_name (gp_get_active_itdb(), NULL, -1);
}

void
on_smart_playlist_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    spl_edit_new (gp_get_active_itdb(), NULL, -1);
}

void
on_pl_containing_displayed_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_displayed_playlist ();
}

void
on_pl_containing_selected_tracks_activate (GtkMenuItem     *menuitem,
					    gpointer         user_data)
{
    generate_selected_playlist ();
}

void
on_pl_for_each_artist_activate         (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    generate_category_playlists (gp_get_active_itdb(), T_ARTIST);
}

void
on_pl_for_each_album_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data)
{
    generate_category_playlists (gp_get_active_itdb(), T_ALBUM);
}

void
on_pl_for_each_genre_activate         (GtkMenuItem     *menuitem,
				       gpointer         user_data)
{
    generate_category_playlists (gp_get_active_itdb(), T_GENRE);
}

void
on_pl_for_each_composer_activate         (GtkMenuItem     *menuitem,
					  gpointer         user_data)
{
    generate_category_playlists (gp_get_active_itdb(), T_COMPOSER);
}


void
on_pl_for_each_year_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    generate_category_playlists (gp_get_active_itdb(), T_YEAR);
}


void
on_most_listened_tracks1_activate       (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    most_listened_pl(gp_get_active_itdb());
}


void
on_all_tracks_never_listened_to1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    never_listened_pl(gp_get_active_itdb());
}

void
on_most_rated_tracks_playlist_s1_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    most_rated_pl(gp_get_active_itdb());
}


void
on_most_recent_played_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    last_listened_pl(gp_get_active_itdb());
}

void
on_played_since_last_time1_activate    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    since_last_pl(gp_get_active_itdb());
}


void
on_rebuild_ipod_db1_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
/*    rebuild_iTunesDB();*/
}


void
on_sorting_clicked                     (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_create ();
}


void
on_sorting_activate                    (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    sort_window_create ();
}


void
on_st_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_st_sort (SORT_ASCENDING);
}


void
on_st_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_st_sort (SORT_DESCENDING);
}


void
on_st_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_st_sort (SORT_NONE);
}


void
on_pm_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_pm_sort (SORT_ASCENDING);
}


void
on_pm_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_pm_sort (SORT_DESCENDING);
}


void
on_pm_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_pm_sort (SORT_NONE);
}


void
on_pm_autostore_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    sort_window_set_pm_autostore (gtk_toggle_button_get_active(togglebutton));
}



void
on_tm_ascend_toggled                   (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_tm_sort (SORT_ASCENDING);
}


void
on_tm_descend_toggled                  (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_tm_sort (SORT_DESCENDING);
}


void
on_tm_none_toggled                     (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    if (gtk_toggle_button_get_active(togglebutton))
	sort_window_set_tm_sort (SORT_NONE);
}

void
on_tm_autostore_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    sort_window_set_tm_autostore (gtk_toggle_button_get_active(togglebutton));
}


void
on_sort_case_sensitive_toggled         (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    sort_window_set_case_sensitive(
	gtk_toggle_button_get_active(togglebutton));
}


void
on_sort_apply_clicked                  (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_apply ();
}


void
on_sort_cancel_clicked                 (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_cancel ();
}


void
on_sort_ok_clicked                     (GtkButton       *button,
					gpointer         user_data)
{
    sort_window_ok ();
}


gboolean
on_sort_window_delete_event            (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
    sort_window_delete ();
    return FALSE;
}

void
on_normalize_selected_playlist_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    Playlist *pl = pm_get_selected_playlist ();
    if (pl)
	nm_tracks_list (pl->members);
    else
	gtkpod_statusbar_message (_("No playlist selected"));
}


void
on_normalize_selected_tab_entry_activate
					(GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    TabEntry *entry;
    gint inst;

    inst = get_sort_tab_number (_("Normalize tracks in selected entry of which sort tab?"));
    if (inst == -1) return;

    entry = st_get_selected_entry (inst);
    if (entry)
    {
	nm_tracks_list (entry->members);
    }
    else
    {
	gchar *str = g_strdup_printf(_("No entry selected in Sort Tab %d"),
				     inst+1);
	gtkpod_statusbar_message (str);
	g_free (str);
	return;
    }
}


void
on_normalize_selected_tracks_activate   (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
   GList *tracks = tm_get_selected_tracks ();
   nm_tracks_list (tracks);
   g_list_free (tracks);
}


void
on_normalize_displayed_tracks_activate  (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    GList *tracks = tm_get_all_tracks ();
    nm_tracks_list (tracks);
    g_list_free (tracks);
}


void
on_normalize_all_tracks                (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    iTunesDB *itdb = gp_get_active_itdb ();
    Playlist *mpl;
    g_return_if_fail (itdb);
    mpl = itdb_playlist_mpl (itdb);
    g_return_if_fail (mpl);
    nm_tracks_list (mpl->members);
}


void
on_normalize_newly_added_tracks        (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    nm_new_tracks (gp_get_active_itdb ());
}


void
on_info_window1_activate               (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
	 info_open_window ();
    else info_close_window ();
}


gboolean
on_gtkpod_info_delete_event            (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
    info_close_window ();
    return TRUE; /* don't close again -- info_close_window() already does it */
}


void
on_info_close_clicked                  (GtkButton       *button,
					gpointer         user_data)
{
    info_close_window ();
}


void
on_check_ipod_files_activate           (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    check_db (gp_get_ipod_itdb());
}


void
on_sync_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	tools_sync_all ();
}


void
on_sync_calendar_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tools_sync_calendar ();
}


void
on_sync_contacts_activate              (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    tools_sync_contacts ();
}


void
on_sync_notes_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	tools_sync_notes ();

}

void
on_all_tracks_not_listed_in_any_playlist1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    generate_not_listed_playlist (gp_get_active_itdb ());
}


void
on_random_playlist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    generate_random_playlist(gp_get_active_itdb ());
}


void
on_randomize_current_playlist_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    randomize_current_playlist();
}

void
on_pl_for_each_rating_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    each_rating_pl (gp_get_active_itdb ());
}
