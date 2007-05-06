/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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
#include "confirmation.h"
#include "misc.h"
#include "prefs.h"
#include "info.h"
#include "display_coverart.h"

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
			 _("The following has occurred:"),
			 text,                /* text to be displayed */
			 NULL, 0, NULL,       /* option 1 */
			 NULL, 0, NULL,       /* option 2 */
			 TRUE,                /* gboolean confirm_again, */
			 NULL, /* ConfHandlerOpt confirm_again_key, */
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
			       gchar **confirm_again_key,
			       GString **str)
{
    Track *s;
    GList *l;
    guint n;

    g_return_if_fail (dd);
    g_return_if_fail (dd->itdb);

    /* write title and label */
    n = g_list_length (dd->tracks);

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
			      "Delete Tracks Completely from iPod?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_int("delete_ipod");
	    if (confirm_again_key)
		*confirm_again_key = g_strdup("delete_ipod");
	    break;
	case DELETE_ACTION_PLAYLIST:
	    g_return_if_fail (dd->pl);
	    if (label)
		*label = g_strdup_printf(
		    ngettext ("Are you sure you want to remove the following track from the playlist \"%s\"?",
			      "Are you sure you want to remove the following tracks from the playlist \"%s\"?", n), dd->pl->name);
	    if (title)
		*title = g_strdup (
		    ngettext ("Remove Track From Playlist?",
			      "Remove Tracks From Playlist?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_int("delete_track");
	    if (confirm_again_key)
		*confirm_again_key = g_strdup("delete_track");
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
		*confirm_again = prefs_get_int("delete_local_file");
	    if (confirm_again_key)
		*confirm_again_key = g_strdup("delete_local_file");
	    break;
	case DELETE_ACTION_PLAYLIST:
	    g_return_if_fail (dd->pl);
	    if (label)
		*label = g_strdup_printf(
		    ngettext ("Are you sure you want to remove the following track from the playlist \"%s\"?",
			      "Are you sure you want to remove the following tracks from the playlist \"%s\"?", n), dd->pl->name);
	    if (title)
		*title = g_strdup (
		    ngettext ("Remove Track From Playlist?",
			      "Remove Tracks From Playlist?", n));
	    if (confirm_again)
		*confirm_again = prefs_get_int("delete_file");
	    if (confirm_again_key)
		*confirm_again_key = g_strdup("delete_file");
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
		*confirm_again = prefs_get_int("delete_database");
	    if (confirm_again_key)
		*confirm_again_key = g_strdup("delete_database");
	    break;
	default:
	    g_return_if_reached ();
	}
    }

    /* Write names of tracks */
    if (str)
    {
	*str = g_string_sized_new (2000);
	for(l = dd->tracks; l; l = l->next)
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

    g_list_free (dd->tracks);
    g_free (dd);
}



/* ok handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
void delete_track_ok (struct DeleteData *dd)
{
    gint n;
    GList *l;

    g_return_if_fail (dd);
    g_return_if_fail (dd->pl);
    g_return_if_fail (dd->itdb);

    /* should never happen */
    if (!dd->tracks)	delete_track_cancel (dd);

		/* Deafen the coverart display while deletion is occurring */
		coverart_block_change (TRUE);
		
    /* nr of tracks to be deleted */
    n = g_list_length (dd->tracks);
    if (dd->itdb->usertype & GP_ITDB_TYPE_IPOD)
    {
	switch (dd->deleteaction)
	{
	case DELETE_ACTION_IPOD:
	    gtkpod_statusbar_message (
		ngettext ("Deleted one track completely from iPod",
			  "Deleted %d tracks completely from iPod",
			  n), n);
	    break;
	case DELETE_ACTION_PLAYLIST:
	    gtkpod_statusbar_message (
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
	    gtkpod_statusbar_message (
		ngettext ("Deleted one track from harddisk",
			  "Deleted %d tracks from harddisk",
			  n), n);
	    break;
	case DELETE_ACTION_PLAYLIST:
	    gtkpod_statusbar_message (
		ngettext ("Deleted %d track from playlist '%s'",
			  "Deleted %d tracks from playlist '%s'",
			  n), n, dd->pl->name);
	    break;
	case DELETE_ACTION_DATABASE:
	    gtkpod_statusbar_message (
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
    for (l = dd->tracks; l; l = l->next)
    {
	gp_playlist_remove_track (dd->pl, l->data, dd->deleteaction);
    }

		/* Awaken coverart selection */
		coverart_block_change (FALSE);
    g_list_free (dd->tracks);
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
    gchar *confirm_again_key;

    pl = pm_get_selected_playlist ();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	message_sb_no_playlist_selected ();
	return;
    }
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    selected_tracks = tm_get_selected_tracks();
    if (selected_tracks == NULL)
    {  /* no tracks selected */
	message_sb_no_tracks_selected ();
	return;
    }

    dd = g_malloc0 (sizeof (struct DeleteData));
    dd->deleteaction = deleteaction;
    dd->tracks = selected_tracks;
    dd->pl = pl;
    dd->itdb = itdb;

    delete_populate_settings (dd,
			      &label, &title,
			      &confirm_again, &confirm_again_key,
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
	 confirm_again_key,/* ConfHandlerOpt confirm_again_key,*/
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
    g_free(confirm_again_key);
    g_string_free (str, TRUE);
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
    gchar *confirm_again_key;
    TabEntry *entry;
    GtkResponseType response;
    iTunesDB *itdb;

    g_return_if_fail (inst >= 0);
    g_return_if_fail (inst <= prefs_get_int("sort_tab_num"));

    pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	message_sb_no_playlist_selected ();
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
    dd->tracks = selected_tracks;
    dd->pl = pl;
    dd->itdb = itdb;

    delete_populate_settings (dd,
			      &label, &title,
			      &confirm_again, &confirm_again_key,
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
	 confirm_again_key,/* ConfHandlerOpt confirm_again_key,*/
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
    g_free(confirm_again_key);
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

    g_list_free (dd->tracks);
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
/*		display_reset (0);*/
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
    gchar *label = NULL, *title = NULL;
    gboolean confirm_again;
    gchar *confirm_again_key;
    guint32 n = 0;
    GString *str;

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	message_sb_no_playlist_selected ();
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
		dd->tracks = g_list_copy (pl->members);
		label = g_strdup_printf (_("Are you sure you want to remove all podcasts from your iPod?"));
	    }
	    else 
	    {   /* normal playlist */
		/* we set selected_tracks to get a list printed by
		 * delete_populate_settings() further down */
		dd->tracks = g_list_copy (pl->members);
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
		dd->tracks = g_list_copy (pl->members);
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
		dd->tracks = g_list_copy (pl->members);
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
			      &confirm_again_key,
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
	 confirm_again_key,  /* ConfHandlerOpt confirm_again_key,*/
	 CONF_NULL_HANDLER,      /* ConfHandler ok_handler,*/
	 NULL,                   /* don't show "Apply" button */
	 CONF_NULL_HANDLER,      /* cancel_handler,*/
	 NULL,                   /* gpointer user_data1,*/
	 NULL);                  /* gpointer user_data2,*/

    g_free (label);
    g_free (title);
    g_free(confirm_again_key);
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



/**
 * gtkpod_shutdown
 *
 * return value: TRUE if it's OK to quit.
 */
static gboolean
ok_to_close_gtkpod (void)
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
	return TRUE;
    }
    return FALSE;
}


/* callback for gtkpod window's close button */
gboolean
on_gtkpod_delete_event                 (GtkWidget       *widget,
					GdkEvent        *event,
					gpointer         user_data)
{
    if (!widgets_blocked)
    {
	if (ok_to_close_gtkpod ())
	{
	    gtkpod_shutdown ();
	    return FALSE;
	}
    }
    return TRUE; /* don't quit -- would cause numerous error messages */
}


/* callback for quit menu entry */
void
on_quit1_activate                      (GtkMenuItem     *menuitem,
					gpointer         user_data)
{
    if (!widgets_blocked)
    {
	if (ok_to_close_gtkpod ())
	{
	    gtkpod_shutdown ();
	}
    }
}
