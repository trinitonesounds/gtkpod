/* Time-stamp: <2004-06-28 22:17:47 jcs>
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
#include "support.h"


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
void delete_populate_settings (Playlist *pl, GList *selected_trackids,
			       gchar **label, gchar **title,
			       gboolean *confirm_again,
			       ConfHandlerOpt *confirm_again_handler,
			       GString **str)
{
    Track *s;
    GList *l;
    guint n;

    /* write title and label */
    n = g_list_length (selected_trackids);
    if (!pl) pl = get_playlist_by_nr (0); /* NULL,0: MPL */
    if(pl->type == PL_TYPE_MPL)
    {
	if (label)
	    *label = g_strdup (ngettext ("Are you sure you want to delete the following track completely from your ipod? The number of playlists this track is a member of is indicated in parentheses.", "Are you sure you want to delete the following tracks completely from your ipod? The number of playlists the tracks are member of is indicated in parentheses.", n));
	if (title)
	    *title = ngettext ("Delete Track Completely?",
			       "Delete Tracks Completey?", n);
	if (confirm_again)
	    *confirm_again = prefs_get_track_ipod_file_deletion ();
	if (confirm_again_handler)
	    *confirm_again_handler = prefs_set_track_ipod_file_deletion;
    }
    else /* normal playlist */
    {
	if (label)
	    *label = g_strdup_printf(ngettext ("Are you sure you want to delete the following track from the playlist \"%s\"?", "Are you sure you want to delete the following tracks from the playlist \"%s\"?", n), pl->name);
	if (title)
	    *title = ngettext ("Delete Track From Playlist?",
			       "Delete Tracks From Playlist?", n);
	if (confirm_again)
	    *confirm_again = prefs_get_track_playlist_deletion ();
	if (confirm_again_handler)
	    *confirm_again_handler = prefs_set_track_playlist_deletion;
    }

    /* Write names of tracks */
    if (str)
    {
	*str = g_string_sized_new (2000);
	for(l = selected_trackids; l; l = l->next)
	{
	    s = get_track_by_id ((guint32)l->data);
	    if (s)
		g_string_append_printf (*str, "%s-%s (%d)\n",
					s->artist, s->title,
					track_is_in_playlists (s));
	}
    }
}


/* ok handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
void delete_track_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *pl = user_data1;
    GList *selected_trackids = user_data2;
    gint n;
    gchar *buf;
    GList *l;

    /* sanity checks */
    if (!pl)
    {
	pl = get_playlist_by_nr (0);  /* NULL,0 = MPL */
    }
    if (!selected_trackids)
	return;

    n = g_list_length (selected_trackids); /* nr of tracks to be deleted */
    if (pl->type == PL_TYPE_MPL)
    {
	buf = g_strdup_printf (
	    ngettext ("Deleted one track completely from iPod",
		      "Deleted %d tracks completely from iPod", n), n);
    }
    else /* normal playlist */
    {
	buf = g_strdup_printf (
	    ngettext ("Deleted track from playlist '%s'",
		      "Deleted tracks from playlist '%s'", n), pl->name);
    }

    for (l = selected_trackids; l; l = l->next)
	remove_trackid_from_playlist (pl, (guint32)l->data);

    gtkpod_statusbar_message (buf);
    gtkpod_tracks_statusbar_update ();
    g_list_free (selected_trackids);
    g_free (buf);
    /* mark data as changed */
    data_changed ();
}

/* cancel handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
static void delete_track_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *selected_trackids = user_data2;

    g_list_free (selected_trackids);
}


/* Deletes selected tracks from current playlist.
   @full_delete: if TRUE, tracks are removed from the iPod completely */
void delete_track_head (gboolean full_delete)
{
    Playlist *pl;
    GList *selected_trackids;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;

    if (full_delete) pl = get_playlist_by_nr (0);
    else             pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    selected_trackids = tm_get_selected_trackids();
    if (selected_trackids == NULL)
    {  /* no tracks selected */
	gtkpod_statusbar_message (_("No tracks selected."));
	return;
    }
    delete_populate_settings (pl, selected_trackids,
			      &label, &title,
			      &confirm_again, &confirm_again_handler,
			      &str);
    /* open window */
    gtkpod_confirmation
	(-1,                   /* gint id, */
	 FALSE,                /* gboolean modal, */
	 title,                /* title */
	 label,                /* label */
	 str->str,             /* scrolled text */
	 NULL, 0, NULL,        /* option 1 */
	 NULL, 0, NULL,        /* option 2 */
	 confirm_again,        /* gboolean confirm_again, */
	 confirm_again_handler,/* ConfHandlerOpt confirm_again_handler,*/
	 delete_track_ok,      /* ConfHandler ok_handler,*/
	 NULL,                 /* don't show "Apply" button */
	 delete_track_cancel,  /* cancel_handler,*/
	 pl,                   /* gpointer user_data1,*/
	 selected_trackids);   /* gpointer user_data2,*/

    g_free (label);
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
void delete_entry_head (gint inst, gboolean delete_full)
{
    Playlist *pl;
    GList *selected_trackids=NULL;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;
    TabEntry *entry;
    GList *gl;
    GtkResponseType response;

    if ((inst < 0) || (inst > prefs_get_sort_tab_num ()))   return;
    if (delete_full)  pl = get_playlist_by_nr (0);
    else              pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    if (inst == -1)
    { /* this should not happen... */
	g_warning ("delete_entry_head(): Programming error: inst == -1\n");
	return;
    }
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
    for (gl=entry->members; gl; gl=gl->next)
    {
	Track *s=(Track *)gl->data;
	selected_trackids = g_list_append (selected_trackids,
					  (gpointer)s->ipod_id);
    }

    delete_populate_settings (pl, selected_trackids,
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
	delete_track_ok (pl, selected_trackids);
	/* Delete the entry */
	st_remove_entry (entry, inst);
	/* mark data as changed */
	data_changed ();
	break;
    default:
	g_list_free (selected_trackids);
	break;
    }

    g_free (label);
    g_string_free (str, TRUE);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete Playlist                                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* ok handler for delete playlist */
/* @user_data1 is the selected playlist */
static void delete_playlist_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *selected_playlist = (Playlist *)user_data1;
    gchar *buf;

    if (!selected_playlist) return;
    buf = g_strdup_printf (_("Deleted playlist '%s'"),
			   selected_playlist->name);
    remove_playlist (selected_playlist);
    gtkpod_statusbar_message (buf);
    g_free (buf);
    /* mark data as changed */
    data_changed ();
}

/* ok handler for delete playlist including tracks */
/* @user_data1 is the selected playlist, @user_data2 are the selected tracks */
static void delete_playlist_full_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *selected_playlist = (Playlist *)user_data1;
    GList *l, *selected_trackids = user_data2;
    guint32 n;
    gchar *buf;

    if (!selected_playlist) return;
    n = g_list_length (selected_trackids);
    buf = g_strdup_printf (ngettext ("Deleted playlist '%s' including %d member track", "Deleted playlist '%s' including %d member tracks", n),
			   selected_playlist->name, n);
    /* remove tracks */
    for (l = selected_trackids; l; l = l->next)
	remove_trackid_from_playlist (NULL, (guint32)l->data);
    /* remove playlist */
    remove_playlist (selected_playlist);

    gtkpod_statusbar_message (buf);
    g_list_free (selected_trackids);
    g_free (buf);
    /* mark data as changed */
    data_changed ();
}

/* delete currently selected playlist
   @delete_full: if TRUE, member songs are removed from the iPod */
void delete_playlist_head (gboolean delete_full)
{
    Playlist *pl = pm_get_selected_playlist();
    GtkResponseType response = GTK_RESPONSE_NONE;
    GList *selected_trackids = NULL;

    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    if (pl->type == PL_TYPE_MPL)
    { /* master playlist */
	gtkpod_statusbar_message (_("Cannot delete master playlist."));
	return;
    }

    if (delete_full)
    { /* remove tracks and playlist from iPod */
	GString *str;
	gchar *label, *title;
	gboolean confirm_again;
	ConfHandlerOpt confirm_again_handler;
	GList *gl;
	guint32 n = 0;

	for (gl=pl->members; gl; gl=gl->next)
	{
	    Track *s=(Track *)gl->data;
	    selected_trackids = g_list_append (selected_trackids,
					       (gpointer)s->ipod_id);
	    ++n;
	}
	delete_populate_settings (NULL, selected_trackids,
				  NULL, &title,
				  &confirm_again, &confirm_again_handler,
				  &str);
	label = g_strdup_printf (ngettext ("Are you sure you want to delete playlist '%s' and the following track completely from your ipod? The number of playlists this track is a member of is indicated in parentheses.", "Are you sure you want to delete playlist '%s' and the following tracks completely from your ipod? The number of playlists the tracks are member of is indicated in parentheses.", n), pl->name);
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
	g_string_free (str, TRUE);
    }
    else
    { /* remove only playlist, keep tracks */
	gchar *buf = g_strdup_printf(_("Are you sure you want to delete the playlist '%s'?"), pl->name);

	response = gtkpod_confirmation
	    (-1,                    /* gint id, */
	     TRUE,                  /* gboolean modal, */
	     _("Delete Playlist?"), /* title */
	     buf,                   /* label */
	     NULL,                  /* scrolled text */
	     NULL, 0, NULL,         /* option 1 */
	     NULL, 0, NULL,         /* option 2 */
	     prefs_get_track_playlist_deletion (),/* confirm_again, */
	     prefs_set_track_playlist_deletion, /* confirm_again_handler,*/
	     CONF_NULL_HANDLER,     /* ConfHandler ok_handler,*/
	     NULL,                  /* don't show "Apply" button */
	     CONF_NULL_HANDLER,     /* cancel_handler,*/
	     NULL,                  /* gpointer user_data1,*/
	     NULL);                 /* gpointer user_data2,*/
	g_free (buf);
    }
    switch (response)
    {
    case GTK_RESPONSE_OK:
	if (delete_full)   delete_playlist_full_ok (pl, selected_trackids);
	else               delete_playlist_ok (pl, NULL);
	break;
    default:
	g_list_free (selected_trackids);
	break;
    }
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Create iPod directory hierarchy                      *
 *                                                                  *
\*------------------------------------------------------------------*/
/* ok handler for ipod directory creation */
/* @user_data1 is the mount point of the iPod */
static void ipod_directories_ok (gchar *mp)
{
    gboolean success = TRUE;
    gchar pbuf[PATH_MAX+1];
    gchar *buf;
    gint i;

    if (mp)
    {
	snprintf(pbuf, PATH_MAX, "%s/Calendars", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/Contacts", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/iPod_Control", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/iPod_Control/Music", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/iPod_Control/iTunes", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	for(i = 0; i < 20; i++)
	{
	    snprintf(pbuf, PATH_MAX, "%s/iPod_Control/Music/F%02d", mp, i);
	    if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	}

	if (success)
	    buf = g_strdup_printf (_("Successfully created iPod directories in '%s'."), mp);
	else
	    buf = g_strdup_printf (_("Problem creating iPod directories in '%s'."), mp);
	gtkpod_statusbar_message(buf);
	g_free (buf);
    }
}


/* Pop up the confirmation window for creation of ipod directory
   hierarchy. */
void ipod_directories_head (void)
{
    gchar *mp;
    GString *str;
    GtkResponseType response;

    if (prefs_get_ipod_mount ())
    {
	mp = g_strdup (prefs_get_ipod_mount ());
	if (strlen (mp) > 0)
	{ /* make sure the mount point does not end in "/" */
	    if (mp[strlen (mp) - 1] == '/')
		mp[strlen (mp) - 1] = 0;
	}
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
    g_string_append_printf (str, "%s/iPod_Control/Music/F19\n", mp);

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
			 mp,                  /* gpointer user_data1,*/
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

	remove_all_playlists ();  /* first remove playlists, then
				   * tracks! (otherwise non-existing
				   *tracks may be accessed) */
	remove_all_tracks ();
	display_cleanup ();
	write_prefs (); /* FIXME: how can we avoid saving options set by
			 * command line? */
			/* Tag them as dirty?  seems nasty */
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
