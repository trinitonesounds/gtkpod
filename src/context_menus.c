/*
|  Copyright (C) 2003 Corey Donohoe <atmos at atmos dot org>
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
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "support.h"
#include "prefs.h"
#include "display.h"
#include "song.h"
#include "playlist.h"
#include "interface.h"
#include "callbacks.h"
#include "misc.h"
#include "file.h"
#include "file_export.h"
#include <sys/types.h>
#include <unistd.h>

static guint entry_inst = -1;
static GtkWidget *menu = NULL;
static GList *selected_songs = NULL;
static Playlist *selected_playlist = NULL;
static TabEntry *selected_entry = NULL; 

/**
 * which - run the shell command which, useful for querying default values
 * for executable, 
 * @name - the executable we're trying to find the path for
 * Returns the path to the executable, NULL on not found
 */
static gchar* 
which(const gchar *exe)
{
    FILE *fp = NULL;
    gchar *result = NULL; 
    gchar buf[PATH_MAX];
    gchar *which_exec = NULL;
   
    memset(&buf[0], 0, PATH_MAX);
    which_exec = g_strdup_printf("which %s", exe);
    if((fp = popen(which_exec, "r")))
    {
	int read_bytes = 0;
	if((read_bytes = fread(buf, sizeof(gchar), PATH_MAX, fp)) > 0)
	    result = g_strndup(buf, read_bytes-1);
	pclose(fp);
    }
    C_FREE(which_exec);
    return(result);
}

/**
 * export_entries - export the currently selected files to disk
 * @mi - the menu item selected
 * @data - ignored, shoould be NULL
 */
static void 
export_entries(GtkWidget *w, gpointer data)
{
    if(selected_songs)
	file_export_init(selected_songs);
}

/**
 * edit_entries - open a dialog to edit the song(s) or playlist
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 * FIXME: How this should work needs to be decided
 */
static void 
edit_entries(GtkButton *b, gpointer data)
{

    fprintf(stderr, "edit entries Selected\n");
}

/*
 * play_entries - play the entries currently selected in xmms
 * @play: the command to execute (e.g. "xmms -e %s")
 * @what: e.g. "Enqueue" or "Play Now" (used for error messages)
 */
static void 
do_command_on_entries (gchar *command, gchar *what)
{
    GList *l;
    gchar *str, *commandc, *next;
    gboolean percs = FALSE; /* did "%s" already appear? */
    GPtrArray *args;

    if ((!command) || (strlen (command) == 0))
    {
	gchar *buf = g_strdup_printf (_("No command set for '%s'"), what);
	gtkpod_statusbar_message (buf);
	C_FREE (buf);
	return;
    }

    /* find the command itself -- separated by ' ' */
    next = strchr (command, ' ');
    if (!next)
    {
	str = g_strdup (command);
    }
    else
    {
        str = g_strndup (command, next-command);
    }
    while (g_ascii_isspace (*command))  ++command;
    /* get the full path */
    commandc = which (str);
    if (!commandc)
    {
	gchar *buf = g_strdup_printf (_("Could not find '%s' set for '%s'"),
				      str, what);
	gtkpod_statusbar_message (buf);
	C_FREE (buf);
	C_FREE (str);
	return;
    }
    C_FREE (str);

    /* Create the command line */
    args = g_ptr_array_sized_new (g_list_length (selected_songs) + 10);
    /* first the full path */
    g_ptr_array_add (args, commandc);
    do
    {
	gchar *next;
	gboolean end;

	next = strchr (command, ' ');
	if (next == NULL) next = command + strlen (command);

	if (next == command)  end = TRUE;
	else                  end = FALSE;

	if (!end && (strncmp (command, "%s", 2) != 0))
	{   /* current token is not "%s" */
	    gchar *buf;
	    buf = g_strndup (command, next-command);
	    g_ptr_array_add (args, buf);
	}
	else if (!percs)
	{
	    for(l = selected_songs; l; l = l->next)
	    {
		if((str = get_song_name_on_disk_verified((Song*)l->data)))
		    g_ptr_array_add (args, str);
	    }
	    percs = TRUE; /* encountered a '%s' */
	}
	command = next;
	/* skip whitespace */
	while (g_ascii_isspace (*command))  ++command;
    } while (*command);
    /* need NULL pointer */
    g_ptr_array_add (args, NULL);

    switch(fork())
    {
    case 0: /* we are the child */
    {
	gchar **argv = (gchar **)args->pdata;
	execv(argv[0], &argv[1]);
	g_ptr_array_free (args, TRUE);
	exit(0);
	break;
    }
    case -1: /* we are the parent, fork() failed  */
	g_ptr_array_free (args, TRUE);
	break;
    default: /* we are the parent, everything's fine */
	break;
    }
}


/*
 * play_entries_now - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
play_entries_now (GtkMenuItem *mi, gpointer data)
{
    do_command_on_entries (prefs_get_play_now_path (), _("Play Now"));
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
play_entries_enqueue (GtkMenuItem *mi, gpointer data)
{
    do_command_on_entries (prefs_get_play_enqueue_path (), _("Enqueue"));
}


/*
 * update_entries - update the entries currently selected
 * @mi - the menu item selected
 * @data - Ignored, should be NULL
 */
static void 
update_entries(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	update_selected_playlist();
    else if(selected_entry)
	update_selected_entry(entry_inst);
    else if(selected_songs)
	update_selected_songs();
}

/**
 * delete_entries - delete the currently selected entry, be it a playlist,
 * items in a sort tab, or songs
 * @mi - the menu item selected
 * @data - ignored, should be NULL
 */
static void 
delete_entries(GtkMenuItem *mi, gpointer data)
{
    if (selected_playlist)
	delete_playlist_head();
    else if(selected_entry)
	delete_entry_head(entry_inst);
    else if(selected_songs)
	delete_song_head();
}


/* Attach a menu item to your context menu */
/* @m - the GtkMenu we're attaching to
 * @str - a gchar* with the menu label
 * @func - the callback for when the item is selected
 * @mi - the GtkWidget we're gonna hook into the menu
 */
GtkWidget *hookup_mi (GtkWidget *m, gchar *str, GCallback func)
{
    GtkWidget *mi = gtk_menu_item_new_with_label(str);
    gtk_widget_show(mi);
    gtk_widget_set_sensitive(mi, TRUE);
    g_signal_connect(G_OBJECT(mi), "activate", func, NULL);
    gtk_menu_append(m, mi);
    return mi;
}


void
create_sm_menu(void)
{
    if(!menu)
    {
	menu =  gtk_menu_new();
#if 0
	hookup_mi (menu, _("Edit"), G_CALLBACK (edit_entries));
#endif
	hookup_mi (menu, _("Play Now"), G_CALLBACK (play_entries_now));
	hookup_mi (menu, _("Enqueue"), G_CALLBACK (play_entries_enqueue));
	hookup_mi (menu, _("Export"), G_CALLBACK (export_entries));
	hookup_mi (menu, _("Update"), G_CALLBACK (update_entries));
	hookup_mi (menu, _("Delete"), G_CALLBACK (delete_entries));
    }
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
	    NULL, NULL, 3, gtk_get_current_event_time()); 
}

/**
 * sm_context_menu_init - initialize the right click menu for songs
 */
void
sm_context_menu_init(void)
{
    selected_entry = NULL; 
    selected_playlist = NULL;
    entry_inst = -1;
    if (selected_songs)  g_list_free (selected_songs);
    selected_songs = sm_get_selected_songs();
    if(selected_songs)
    {
	create_sm_menu();
    }
}
/**
 * pm_context_menu_init - initialize the right click menu for playlists 
 */
void
pm_context_menu_init(void)
{
    if (selected_songs)  g_list_free (selected_songs);
    selected_songs = NULL;
    selected_entry = NULL;
    entry_inst = -1;
    selected_playlist = pm_get_selected_playlist();
    if(selected_playlist)
    {
	selected_songs = g_list_copy (selected_playlist->members);
	create_sm_menu();
    }
}
/**
 * st_context_menu_init - initialize the right click menu for sort tabs 
 * FIXME: This needs to be hooked in.
 */
void
st_context_menu_init(gint inst)
{
    if (selected_songs)  g_list_free (selected_songs);
    selected_songs = NULL;
    selected_playlist = NULL;
    selected_entry = st_get_selected_entry(inst);
    if(selected_entry)
    {
	entry_inst = inst;
	selected_songs = g_list_copy (selected_entry->members);
	create_sm_menu();
    }
}
