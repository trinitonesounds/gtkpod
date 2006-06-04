/* Time-stamp: <2006-06-05 01:19:33 jcs>
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

#include "ipod_init.h"
#include "prefs.h"
#include "misc.h"
#include "fileselection.h"
#include <string.h>

struct _IpodInit
{
    GladeXML *xml;           /* XML info                             */
    GtkWidget *window;       /* pointer to repository window         */
    iTunesDB *itdb;
};

typedef struct _IpodInit IpodInit;

/* string constants for window widgets used more than once */
static const gchar *MOUNTPOINT_ENTRY="mountpoint_entry";
static const gchar *MOUNTPOINT_BUTTON="mountpoint_button";

/* shortcut to reference widgets when repwin->xml is already set */
#define GET_WIDGET(a) gtkpod_xml_get_widget (ii->xml,a)


/* mountpoint browse button was clicked */
static void mountpoint_button_clicked (GtkButton *button, IpodInit *ii)
{
    const gchar *old_dir;
    gchar *new_dir;

    g_return_if_fail (ii);

    old_dir = gtk_entry_get_text (
	GTK_ENTRY (GET_WIDGET (MOUNTPOINT_ENTRY)));

    new_dir = fileselection_get_file_or_dir (
	_("Select mountpoint"),
	old_dir,
	GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    if (new_dir)
    {
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (MOUNTPOINT_ENTRY)),
			    new_dir);
	g_free (new_dir);
    }
}



gboolean ipod_init (iTunesDB *itdb)
{
    IpodInit *ii;
    gint response;
    gboolean result = FALSE;
    gchar *mountpoint, *new_mount, *name;
    GError *error = NULL;

    g_return_val_if_fail (itdb, FALSE);

    /* Create window */
    ii = g_new0 (IpodInit, 1);
    ii->xml = glade_xml_new (xml_file, "ipod_init_dialog", NULL);
    ii->window = gtkpod_xml_get_widget (ii->xml,
					"ipod_init_dialog");
    g_return_val_if_fail (ii->window, FALSE);

    /* Set mountpoint */
    mountpoint = get_itdb_prefs_string (itdb, KEY_MOUNTPOINT);
    if (mountpoint)
    {
	gtk_entry_set_text (GTK_ENTRY (GET_WIDGET (MOUNTPOINT_ENTRY)),
			    mountpoint);
    }

    /* Signal handlers */
    g_signal_connect (GET_WIDGET (MOUNTPOINT_BUTTON), "clicked",
		      G_CALLBACK (mountpoint_button_clicked), ii);

    response = gtk_dialog_run (GTK_DIALOG (ii->window));

    switch (response)
    {
    case GTK_RESPONSE_OK:
	new_mount = g_strdup (
	    gtk_entry_get_text (
		GTK_ENTRY (GET_WIDGET (MOUNTPOINT_ENTRY))));
	/* remove trailing '/' in case it's present. */
	if (mountpoint && (strlen (mountpoint) > 0))
	{
	    if (G_IS_DIR_SEPARATOR(mountpoint[strlen(mountpoint) - 1]))
	    {
		mountpoint[strlen(mountpoint) - 1] = 0;
	    }
	}
	if (new_mount && (strlen (new_mount) > 0))
	{
	    if (G_IS_DIR_SEPARATOR(new_mount[strlen(new_mount) - 1]))
	    {
		new_mount[strlen(new_mount) - 1] = 0;
	    }
	}
	if (!(mountpoint && new_mount &&
	      (strcmp (mountpoint, new_mount) == 0)))
	{   /* mountpoint has changed */
	    g_free (mountpoint);
	    mountpoint = new_mount;
	    new_mount = NULL;
	    set_itdb_prefs_string (itdb, KEY_MOUNTPOINT, mountpoint);
	    call_script ("gtkpod.load", mountpoint, NULL);
	    itdb_set_mountpoint (itdb, mountpoint);
	}
	else
	{
	    g_free (new_mount);
	    new_mount = NULL;
	}
	name = get_itdb_prefs_string (itdb, "name");
	result = itdb_init_ipod (mountpoint, "PA005", name, &error);
	if (!result)
	{
	    if (error)
	    {
		gtkpod_warning (_("Error initialising iPod: %s\n"),
				error->message);
		g_error_free (error);
		error = NULL;
	    }
	    else
	    {
		gtkpod_warning (_("Error initialising iPod, unknown error\n"));
	    }
	}
	g_free (name);
	break;
    default:
	/* canceled -- do nothing */
	break;
    }

    gtk_widget_destroy (ii->window);

    g_free (mountpoint);

    g_free (ii);

    return result;
}
