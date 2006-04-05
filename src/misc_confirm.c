/* Time-stamp: <2006-03-30 23:23:33 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <sys/stat.h>
#include "clientserver.h"
#include "confirmation.h"
#include "misc.h"
#include "prefs.h"
#include "info.h"

#ifdef HAVE_STATVFS
#include <sys/types.h>
#include <sys/statvfs.h>
#endif

#define DEBUG_MISC 0

/*------------------------------------------------------------------*\
 *                                                                  *
 *                       gtkpod_warning                             *
 *                                                                  *
\*------------------------------------------------------------------*/

/* gtkpod_warning(): will pop up a window and display text as a
 * warning. If a warning window is already open, the text will be
 * added to the existing window. */
/* parameters: same as printf */
void gtkpod_warning (const gchar *format, ...)
{
    va_list arg;
    gchar *text;

    va_start (arg, format);
    text = g_strdup_vprintf (format, arg);
    va_end (arg);

    gtkpod_confirmation (CONF_ID_GTKPOD_WARNING,    /* gint id, */
			 FALSE,                     /* gboolean modal, */
			 _("Warning"),              /* title */
			 _("The following has occured:"),
			 text,                /* text to be displayed */
			 NULL, 0, NULL,       /* option 1 */
			 NULL, 0, NULL,       /* option 2 */
			 TRUE,                /* gboolean confirm_again, */
			 NULL, /* ConfHandlerOpt confirm_again_handler, */
			 CONF_NULL_HANDLER,   /* ConfHandler ok_handler,*/
			 NULL,                /* don't show "Apply" */
			 NULL,                /* cancel_handler,*/
			 NULL,                /* gpointer user_data1,*/
			 NULL);               /* gpointer user_data2,*/
    g_free (text);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete Tracks                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* This is the same for delete_track_head() and delete_st_head(), so I
 * moved it here to make changes easier */
void delete_populate_settings (struct DeleteData *dd,
			       gchar **label, gchar **title,
			       gboolean *confirm_again,
			       ConfHandlerOpt *confirm_again_handler,
			       GString **str)
{
    Track *s;
    GList *l;
    guint n;

    g_return_if_fail (dd);
    g_return_if_fail (dd->itdb);
    g_return_if_fail (dd->pl);

    /* write title and label */
    n = g_list_length (dd->selected_tracks);

    if (dd->itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_LOCAL:
	case DELETE_ACTION_DATABASE:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	case DELETE_ACTION_IPOD:
	    if (label)
		*label = g_strdup (
		    ngettext ("Are you sure you want to delete the following track completely from your iPod? The number of playlists this track is a member of is indicated in parentheses.",
			      "Are you sure you want to delete the following tracks completely from your iPod? The number of playlists the tracks are member of is indicated in parentheses.", n));
	    if (title)
		*title = g_strdup (
		    ngettext ("Delete Track Completely from iPod?",
			      "Delete Tracks Completey from iPod?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_track_ipod_file_deletion ();
	    if (confirm_again_handler)
		*confirm_again_handler = prefs_set_track_ipod_file_deletion;
	    break;
	case DELETE_ACTION_PLAYLIST:
	    if (label)
		*label = g_strdup_printf(
		    ngettext ("Are you sure you want to remove the following track from the playlist \"%s\"?",
			      "Are you sure you want to remove the following tracks from the playlist \"%s\"?", n), dd->pl->name);
	    if (title)
		*title = g_strdup (
		    ngettext ("Remove Track From Playlist?",
			      "Remove Tracks From Playlist?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_track_playlist_deletion ();
	    if (confirm_again_handler)
		*confirm_again_handler = prefs_set_track_playlist_deletion;
	    break;
	default:
	    g_return_if_reached ();
	}
    }
    if (dd->itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_IPOD:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	case DELETE_ACTION_LOCAL:
	    if (label)
		*label = g_strdup (
		    ngettext ("Are you sure you want to delete the following track completely from your harddisk? The number of playlists this track is a member of is indicated in parentheses.",
			      "Are you sure you want to delete the following tracks completely from your harddisk? The number of playlists the tracks are member of is indicated in parentheses.", n));
	    if (title)
		*title = g_strdup (
		    ngettext ("Delete Track from Harddisk?",
			      "Delete Tracks from Harddisk?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_track_local_file_deletion ();
	    if (confirm_again_handler)
		*confirm_again_handler = prefs_set_track_local_file_deletion;
	    break;
	case DELETE_ACTION_PLAYLIST:
	    if (label)
		*label = g_strdup_printf(
		    ngettext ("Are you sure you want to remove the following track from the playlist \"%s\"?",
			      "Are you sure you want to remove the following tracks from the playlist \"%s\"?", n), dd->pl->name);
	    if (title)
		*title = g_strdup (
		    ngettext ("Remove Track From Playlist?",
			      "Remove Tracks From Playlist?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_track_playlist_deletion ();
	    if (confirm_again_handler)
		*confirm_again_handler = prefs_set_track_playlist_deletion;
	    break;
	case DELETE_ACTION_DATABASE:
	    if (label)
		*label = g_strdup (
		    ngettext ("Are you sure you want to remove the following track completely from your local database? The number of playlists this track is a member of is indicated in parentheses.",
			      "Are you sure you want to remove the following tracks completely from your local database? The number of playlists the tracks are member of is indicated in parentheses.", n));
	    if (title)
		*title = g_strdup (
		    ngettext ("Remove Track from Local Database?",
			      "Remove Tracks from Local Database?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_track_database_deletion ();
	    if (confirm_again_handler)
		*confirm_again_handler = prefs_set_track_database_deletion;
	    break;
	default:
	    g_return_if_reached ();
	}
    }

    /* Write names of tracks */
    if (str)
    {
	*str = g_string_sized_new (2000);
	for(l = dd->selected_tracks; l; l = l->next)
	{
	    s = l->data;
	    g_return_if_fail (s);
	    g_string_append_printf (*str, "%s-%s (%d)\n",
				    s->artist, s->title,
				    itdb_playlist_contain_track_number (s));
	}
    }
}


/* cancel handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
static void delete_track_cancel (struct DeleteData *dd)
{
    g_return_if_fail (dd);

    g_list_free (dd->selected_tracks);
    g_free (dd);
}



/* ok handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
void delete_track_ok (struct DeleteData *dd)
{
    gint n;
    gchar *buf = NULL;
    GList *l;

    g_return_if_fail (dd);
    g_return_if_fail (dd->pl);
    g_return_if_fail (dd->itdb);

    /* should never happen */
    if (!dd->selected_tracks)	delete_track_cancel (dd);

    /* nr of tracks to be deleted */
    n = g_list_length (dd->selected_tracks);
    if (dd->itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_IPOD:
	    buf = g_strdup_printf (
		ngettext ("Deleted one track completely from iPod",
			  "Deleted %d tracks completely from iPod",
			  n), n);
	    break;
	case DELETE_ACTION_PLAYLIST:
	    buf = g_strdup_printf (
		ngettext ("Deleted %d track from playlist '%s'",
			  "Deleted %d tracks from playlist '%s'",
			  n), n, dd->pl->name);
	    break;
	case DELETE_ACTION_LOCAL:
	case DELETE_ACTION_DATABASE:
	default:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	}
    }
    if (dd->itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_LOCAL:
	    buf = g_strdup_printf (
		ngettext ("Deleted one track from harddisk",
			  "Deleted %d tracks from harddisk",
			  n), n);
	    break;
	case DELETE_ACTION_PLAYLIST:
	    buf = g_strdup_printf (
		ngettext ("Deleted %d track from playlist '%s'",
			  "Deleted %d tracks from playlist '%s'",
			  n), n, dd->pl->name);
	    break;
	case DELETE_ACTION_DATABASE:
	    buf = g_strdup_printf (
		ngettext ("Deleted track from local database",
			  "Deleted %d tracks from local database",
			  n), n);
	    break;
	case DELETE_ACTION_IPOD:
	default:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	}
    }
    for (l = dd->selected_tracks; l; l = l->next)
    {
	gp_playlist_remove_track (dd->pl, l->data, dd->deleteaction);
    }
    gtkpod_statusbar_message (buf);
    g_free (buf);

    g_list_free (dd->selected_tracks);
    g_free (dd);

    gtkpod_tracks_statusbar_update ();
}

/* Deletes selected tracks from current playlist.
   @deleteaction: on of the DeleteActions defined in misc.h */
void delete_track_head (DeleteAction deleteaction)
{
    Playlist *pl;
    GList *selected_tracks;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    struct DeleteData *dd;
    iTunesDB *itdb;
    GtkResponseType response;
    ConfHandlerOpt confirm_again_handler;

    pl = pm_get_selected_playlist ();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    selected_tracks = tm_get_selected_tracks();
    if (selected_tracks == NULL)
    {  /* no tracks selected */
	gtkpod_statusbar_message (_("No tracks selected."));
	return;
    }

    dd = g_malloc0 (sizeof (struct DeleteData));
    dd->deleteaction = deleteaction;
    dd->selected_tracks = selected_tracks;
    dd->pl = pl;
    dd->itdb = itdb;

    delete_populate_settings (dd,
			      &label, &title,
			      &confirm_again, &confirm_again_handler,
			      &str);
    /* open window */
    response = gtkpod_confirmation
	(-1,                   /* gint id, */
	 TRUE,                 /* gboolean modal, */
	 title,                /* title */
	 label,                /* label */
	 str->str,             /* scrolled text */
	 NULL, 0, NULL,        /* option 1 */
	 NULL, 0, NULL,        /* option 2 */
	 confirm_again,        /* gboolean confirm_again, */
	 confirm_again_handler,/* ConfHandlerOpt confirm_again_handler,*/
	 CONF_NULL_HANDLER,    /* ConfHandler ok_handler,*/
	 NULL,                 /* don't show "Apply" button */
	 CONF_NULL_HANDLER,    /* cancel_handler,*/
	 NULL,                 /* gpointer user_data1,*/
	 NULL);                /* gpointer user_data2,*/

    switch (response)
    {
    case GTK_RESPONSE_OK:
	/* Delete the tracks */
	delete_track_ok (dd);
	break;
    default:
	delete_track_cancel (dd);
	break;
    }

    g_free (label);
    g_free (title);
    g_string_free (str, TRUE);
}

void
gtkpod_main_window_set_active(gboolean active)
{
    if(gtkpod_window)
    {
	gtk_widget_set_sensitive(gtkpod_window, active);
    }
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete tracks in st entry                             *
 *                                                                  *
\*------------------------------------------------------------------*/

/* deletes the currently selected entry from the current playlist
   @inst: selected entry of which instance?
   @delete_full: if true, member songs are removed from the iPod
   completely */
void delete_entry_head (gint inst, DeleteAction deleteaction)
{
    struct DeleteData *dd;
    Playlist *pl;
    GList *selected_tracks=NULL;
    GString *str;
    gchar *label = NULL, *title = NULL;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;
    TabEntry *entry;
    GtkResponseType response;
    iTunesDB *itdb;

    g_return_if_fail (inst >= 0);
    g_return_if_fail (inst <= prefs_get_sort_tab_num ());

    pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    entry = st_get_selected_entry (inst);
    if (entry == NULL)
    {  /* no entry selected */
	gtkpod_statusbar_message (_("No entry selected."));
	return;
    }

    if (entry->members == NULL)
    {  /* no tracks in entry -> just remove entry */
	if (!entry->master)  st_remove_entry (entry, inst);
	else   gtkpod_statusbar_message (_("Cannot remove entry 'All'"));
	return;
    }

    selected_tracks = g_list_copy (entry->members);

    dd = g_malloc0 (sizeof (struct DeleteData));
    dd->deleteaction = deleteaction;
    dd->selected_tracks = selected_tracks;
    dd->pl = pl;
    dd->itdb = itdb;

    delete_populate_settings (dd,
			      &label, &title,
			      &confirm_again, &confirm_again_handler,
			      &str);

    /* open window */
    response = gtkpod_confirmation
	(-1,                   /* gint id, */
	 TRUE,                 /* gboolean modal, */
	 title,                /* title */
	 label,                /* label */
	 str->str,             /* scrolled text */
	 NULL, 0, NULL,        /* option 1 */
	 NULL, 0, NULL,        /* option 2 */
	 confirm_again,        /* gboolean confirm_again, */
	 confirm_again_handler,/* ConfHandlerOpt confirm_again_handler,*/
	 CONF_NULL_HANDLER,    /* ConfHandler ok_handler,*/
	 NULL,                 /* don't show "Apply" button */
	 CONF_NULL_HANDLER,    /* cancel_handler,*/
	 NULL,                 /* gpointer user_data1,*/
	 NULL);                /* gpointer user_data2,*/

    switch (response)
    {
    case GTK_RESPONSE_OK:
	/* Delete the tracks */
	delete_track_ok (dd);
	/* Delete the entry */
	st_remove_entry (entry, inst);
	break;
    default:
	delete_track_cancel (dd);
	break;
    }

    g_free (label);
    g_free (title);
    g_string_free (str, TRUE);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete Playlist                                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* clean up delete playlist */
static void delete_playlist_cleanup (struct DeleteData *dd)
{
    g_return_if_fail (dd);

    g_list_free (dd->selected_tracks);
    g_free (dd);
}

static void delete_playlist_ok (struct DeleteData *dd)
{
    gint n;
    gchar *msg = NULL;

    g_return_if_fail (dd);
    g_return_if_fail (dd->pl);
    g_return_if_fail (dd->itdb);

    n = g_list_length (dd->pl->members);

    if (dd->itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_IPOD:
	    while (dd->pl->members)
	    {
		/* remove tracks from iPod */
		gp_playlist_remove_track (dd->pl, dd->pl->members->data,
					  dd->deleteaction);
	    }
	    if (itdb_playlist_is_mpl (dd->pl))
	    {
		msg = g_strdup_printf (_("Removed all %d tracks from the iPod"), n);
		display_reset (0);
	    }
	    else if (itdb_playlist_is_podcasts (dd->pl))
	    {
		msg = g_strdup_printf (_("Removed all podcasts from the iPod"));
		if (pm_get_selected_playlist () == dd->pl)
		    st_redisplay (0);
//		display_reset (0);
	    }
	    else
	    {
		/* first use playlist name */
		msg = g_strdup_printf (
		    ngettext ("Deleted playlist '%s' including %d member track",
			      "Deleted playlist '%s' including %d member tracks",
			      n),
		    dd->pl->name, n);
		/* then remove playlist */
		gp_playlist_remove (dd->pl);
	    }
	    break;
	case DELETE_ACTION_PLAYLIST:
	    if (itdb_playlist_is_mpl (dd->pl))
	    {	/* not allowed -- programming error */
		g_return_if_reached ();
	    }
	    else
	    {
		/* first use playlist name */
		msg = g_strdup_printf (_("Deleted playlist '%s'"),
				       dd->pl->name);
		/* then remove playlist */
		gp_playlist_remove (dd->pl);
	    }
	    break;
	case DELETE_ACTION_LOCAL:
	case DELETE_ACTION_DATABASE:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	}
    }
    if (dd->itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_LOCAL:
	    if (itdb_playlist_is_mpl (dd->pl))
	    {   /* for safety reasons this is not implemented (would
		   remove all tracks from your local harddisk) */
		g_return_if_reached ();
	    }
	    else
	    {
		while (dd->pl->members)
		{
		    /* remove tracks from playlist */
		    gp_playlist_remove_track (dd->pl,
					      dd->pl->members->data,
					      dd->deleteaction);
		}
		/* first use playlist name */
		msg = g_strdup_printf (
		    ngettext ("Deleted playlist '%s' including %d member track on harddisk",
			      "Deleted playlist '%s' including %d member tracks on harddisk",
			      n),
		    dd->pl->name, n);
		/* then remove playlist */
		gp_playlist_remove (dd->pl);
	    }
	    break;
	case DELETE_ACTION_DATABASE:
	    while (dd->pl->members)
	    {
		/* remove tracks from database */
		gp_playlist_remove_track (dd->pl, dd->pl->members->data,
					  dd->deleteaction);
	    }
	    if (itdb_playlist_is_mpl (dd->pl))
	    {
		msg = g_strdup_printf (_("Removed all %d tracks from the database"), n);
		display_reset (0);
	    }
	    else
	    {
		/* first use playlist name */
		msg = g_strdup_printf (
		    ngettext ("Deleted playlist '%s' including %d member track",
			      "Deleted playlist '%s' including %d member tracks",
			      n),
		    dd->pl->name, n);
		/* then remove playlist */
		gp_playlist_remove (dd->pl);
	    }
	    break;
	case DELETE_ACTION_PLAYLIST:
	    if (itdb_playlist_is_mpl (dd->pl))
	    {	/* not allowed -- programming error */
		g_return_if_reached ();
	    }
	    else
	    {
		/* first use playlist name */
		msg = g_strdup_printf (_("Deleted playlist '%s'"),
				       dd->pl->name);
		/* then remove playlist */
		gp_playlist_remove (dd->pl);
	    }
	    break;
	case DELETE_ACTION_IPOD:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	}
    }
    delete_playlist_cleanup (dd);

    gtkpod_tracks_statusbar_update ();
}



/* delete currently selected playlist
   @delete_full: if TRUE, member songs are removed from the iPod */
void delete_playlist_head (DeleteAction deleteaction)
{
    struct DeleteData *dd;
    Playlist *pl;
    iTunesDB *itdb;
    GtkResponseType response = GTK_RESPONSE_NONE;
    GString *str;
    gchar *label = NULL, *title = NULL;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;
    guint32 n = 0;

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }

    itdb = pl->itdb;
    g_return_if_fail (itdb);

    dd = g_malloc0 (sizeof (struct DeleteData));
    dd->deleteaction = deleteaction;
    dd->pl = pl;
    dd->itdb = itdb;

    if (itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	switch (deleteaction)
	{
	case DELETE_ACTION_IPOD:
	    if (itdb_playlist_is_mpl (pl))
	    {
		label = g_strdup_printf (_("Are you sure you want to remove all tracks from your iPod?"));
	    }
	    else if (itdb_playlist_is_podcasts (pl))
	    {   /* podcasts playlist */
		dd->selected_tracks = g_list_copy (pl->members);
		label = g_strdup_printf (_("Are you sure you want to remove all podcasts from your iPod?"));
	    }
	    else 
	    {   /* normal playlist */
		/* we set selected_tracks to get a list printed by
		 * delete_populate_settings() further down */
		dd->selected_tracks = g_list_copy (pl->members);
		label = g_strdup_printf (
		    ngettext ("Are you sure you want to delete playlist '%s' and the following track completely from your ipod? The number of playlists this track is a member of is indicated in parentheses.",
			      "Are you sure you want to delete playlist '%s' and the following tracks completely from your ipod? The number of playlists the tracks are member of is indicated in parentheses.", n), pl->name);
	    }
	    break;
	case DELETE_ACTION_PLAYLIST:
	    if (itdb_playlist_is_mpl (pl))
	    {	/* not allowed -- programming error */
		g_return_if_reached ();
	    }
	    else
	    {
		label = g_strdup_printf(_("Are you sure you want to delete the playlist '%s'?"), pl->name);
	    }
	    break;
	case DELETE_ACTION_LOCAL:
	case DELETE_ACTION_DATABASE:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	}
    }
    if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
    {
	switch (deleteaction)
	{
	case DELETE_ACTION_LOCAL:
	    if (itdb_playlist_is_mpl (pl))
	    {   /* for safety reasons this is not implemented (would
		   remove all tracks from your local harddisk */
		g_return_if_reached ();
	    }
	    else
	    {
		/* we set selected_tracks to get a list printed by
		 * delete_populate_settings() further down */
		dd->selected_tracks = g_list_copy (pl->members);
		label = g_strdup_printf (
		    ngettext ("Are you sure you want to delete playlist '%s' and remove the following track from your harddisk? The number of playlists this track is a member of is indicated in parentheses.",
			      "Are you sure you want to delete playlist '%s' and remove the following tracks from your harddisk? The number of playlists the tracks are member of is indicated in parentheses.",
			      n), pl->name);
	    }
	    break;
	case DELETE_ACTION_DATABASE:
	    if (itdb_playlist_is_mpl (pl))
	    {
		label = g_strdup_printf (_("Are you sure you want to remove all tracks from the database?"));
		
	    }
	    else
	    {
		/* we set selected_tracks to get a list printed by
		 * delete_populate_settings() further down */
		dd->selected_tracks = g_list_copy (pl->members);
		label = g_strdup_printf (
		    ngettext ("Are you sure you want to delete playlist '%s' and remove the following track from the database? The number of playlists this track is a member of is indicated in parentheses.",
			      "Are you sure you want to delete playlist '%s' and remove the following tracks from the database? The number of playlists the tracks are member of is indicated in parentheses.",
			      n), pl->name);
	    }
	    break;
	case DELETE_ACTION_PLAYLIST:
	    if (itdb_playlist_is_mpl (pl))
	    {	/* not allowed -- programming error */
		g_return_if_reached ();
	    }
	    else
	    {
		label = g_strdup_printf(_("Are you sure you want to delete the playlist '%s'?"), pl->name);
	    }
	    break;
	case DELETE_ACTION_IPOD:
	    /* not allowed -- programming error */
	    g_return_if_reached ();
	    break;
	}
    }

    delete_populate_settings (dd,
			      NULL, &title,
			      &confirm_again,
			      &confirm_again_handler,
			      &str);

    response = gtkpod_confirmation
	(-1,                     /* gint id, */
	 TRUE,                   /* gboolean modal, */
	 title,                  /* title */
	 label,                  /* label */
	 str->str,               /* scrolled text */
	 NULL, 0, NULL,          /* option 1 */
	 NULL, 0, NULL,          /* option 2 */
	 confirm_again,          /* gboolean confirm_again, */
	 confirm_again_handler,  /* ConfHandlerOpt confirm_again_handler,*/
	 CONF_NULL_HANDLER,      /* ConfHandler ok_handler,*/
	 NULL,                   /* don't show "Apply" button */
	 CONF_NULL_HANDLER,      /* cancel_handler,*/
	 NULL,                   /* gpointer user_data1,*/
	 NULL);                  /* gpointer user_data2,*/

    g_free (label);
    g_free (title);
    g_string_free (str, TRUE);

    switch (response)
    {
    case GTK_RESPONSE_OK:
	delete_playlist_ok (dd);
	break;
    default:
	delete_playlist_cleanup (dd);
	break;
    }
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Create iPod directory hierarchy                      *
 *                                                                  *
\*------------------------------------------------------------------*/

/* deterimine the number of directories that should be created */
/*
 * <20 GB: 20
 * <30 GB: 30
 * <40 GB: 40
 * <50 GB: 50
 * else:   60
 *
 */
static gint ipod_directories_number (gchar *mp)
{
    const gint default_nr = 20;
#ifdef HAVE_STATVFS
    struct statvfs stat;
    int	status;
    gdouble size;

    g_return_val_if_fail (mp, default_nr);

    status = statvfs (mp, &stat);
    if (status != 0)
    {
	return default_nr;
    }
    /* Get size in GB */
    size = ((gdouble)stat.f_blocks * stat.f_frsize) / 1024 / 1024 / 1024;
    if (size < 20) return 20;
    if (size < 30) return 30;
    if (size < 40) return 40;
    if (size < 50) return 50;
    return 60;
#else
    return default_nr;
#endif
}


/* ok handler for ipod directory creation */
/* @user_data1 is the mount point of the iPod */
static void ipod_directories_ok (gchar *mp)
{
    gchar *buf, *errordir=NULL;
    gchar *pbuf;
    gint i, dirnum;

    g_return_if_fail (mp);

    if (!errordir)
    {
	pbuf = g_build_filename (mp, "Calendars", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
		errordir = pbuf;
	}
	if (!errordir)
	    g_free (pbuf);
    }
    if (!errordir)
    {
	pbuf = g_build_filename (mp, "Contacts", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
		errordir = pbuf;
	}
	if (!errordir)
	    g_free (pbuf);
    }
    if (!errordir)
    {
	pbuf = g_build_filename (mp, "iPod_Control", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
		errordir = pbuf;
	}
	if (!errordir)
	    g_free (pbuf);
    }
    if (!errordir)
    {
	pbuf = g_build_filename (mp, "iPod_Control", "Music", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
		errordir = pbuf;
	}
	if (!errordir)
	    g_free (pbuf);
    }
    if (!errordir)
    {
	pbuf = g_build_filename (mp, "iPod_Control", "iTunes", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
		errordir = pbuf;
	}
	if (!errordir)
	    g_free (pbuf);
    }
    if (!errordir)
    {
	pbuf = g_build_filename (mp, "iPod_Control", "Artwork", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
		errordir = pbuf;
	}
	if (!errordir)
	    g_free (pbuf);
    }
    dirnum = ipod_directories_number (mp);
    for(i = 0; i < dirnum; i++)
    {
	if (!errordir)
	{
	    gchar *num = g_strdup_printf ("F%02d", i);
	    pbuf = g_build_filename (mp,
				     "iPod_Control", "Music", num, NULL);
	    if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	    {
		if((mkdir(pbuf, 0777) != 0))
		    errordir = pbuf;
		g_free (num);
	    }
	    if (!errordir)
		g_free (pbuf);
	}
    }

    if (errordir)
	buf = g_strdup_printf (_("Problem creating iPod directory: '%s'."), errordir);
    else
	buf = g_strdup_printf (_("Successfully created iPod directories in '%s'."), mp);
    gtkpod_statusbar_message(buf);
    g_free (buf);
    g_free (errordir);
}


/* Pop up the confirmation window for creation of ipod directory
   hierarchy. */
void ipod_directories_head (const gchar *mountpoint)
{
    gchar *mp;
    GString *str;
    GtkResponseType response;

    g_return_if_fail (mountpoint);
    mp = g_strdup (mountpoint);

    if (strlen (mp) > 0)
    { /* make sure the mount point does not end in "/" */
	if (mp[strlen (mp) - 1] == '/')
	    mp[strlen (mp) - 1] = 0;
    }
    else
    {
	mp = g_strdup (".");
    }
    str = g_string_sized_new (2000);
    g_string_append_printf (str, "%s/Calendars\n", mp);
    g_string_append_printf (str, "%s/Contacts\n", mp);
    g_string_append_printf (str, "%s/iPod_Control\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/iTunes\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music/F00\n...\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music/F%d\n",
			    mp, ipod_directories_number (mp)-1);

    response = gtkpod_confirmation (CONF_ID_IPOD_DIR,    /* gint id, */
			 TRUE,               /* gboolean modal, */
			 _("Create iPod directories"), /* title */
			 _("OK to create the following directories?"),
			 str->str,
			 NULL, 0, NULL,       /* option 1 */
			 NULL, 0, NULL,       /* option 2 */
			 TRUE,                /* gboolean confirm_again, */
			 NULL, /* ConfHandlerOpt confirm_again_handler, */
			 CONF_NULL_HANDLER,   /* ConfHandler ok_handler,*/
			 NULL,                /* don't show "Apply" */
			 CONF_NULL_HANDLER,   /* cancel_handler,*/
			 NULL,                /* gpointer user_data1,*/
			 NULL);               /* gpointer
					       * user_data2,*/
    switch (response)
    {
    case GTK_RESPONSE_OK:
	ipod_directories_ok (mp);
	break;
    default:
	break;
    }
    g_free (mp);
    g_string_free (str, TRUE);
}


/* callback for quit menu entry */
void
on_quit1_activate                      (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
  if (!widgets_blocked) gtkpod_main_quit ();
}


/* callback for gtkpod window's close button */
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

/**
 * gtkpod_main_quit
 *
 * return value: FALSE if it's OK to quit.
 */
gboolean
gtkpod_main_quit(void)
{
    gint result = GTK_RESPONSE_YES;

    if (!files_are_saved ())
    {
	GtkWidget *dialog = gtk_message_dialog_new (
	    GTK_WINDOW (gtkpod_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_YES_NO,
	    _("Data has been changed and not been saved.\nOK to exit gtkpod?"));
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
    }

    if (result == GTK_RESPONSE_YES)
    {
	server_shutdown (); /* stop accepting requests for playcount updates */

	write_prefs ();

/* FIXME: release memory in a clean way */
#if 0
	remove_all_playlists ();  /* first remove playlists, then
				   * tracks! (otherwise non-existing
				   *tracks may be accessed) */
	remove_all_tracks ();
#endif
	display_cleanup ();

	if(prefs_get_automount())
	{
	    unmount_ipod ();
	}
	call_script ("gtkpod.out");
	gtk_main_quit ();
	return FALSE;
    }
    return TRUE;
}
