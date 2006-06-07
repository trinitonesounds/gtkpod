/* Time-stamp: <2006-06-08 00:16:13 jcs>
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

/* Strings used several times */
const gchar *SELECT_OR_ENTER_YOUR_MODEL=N_("Select or enter your model");

/* string constants for window widgets used more than once */
static const gchar *MOUNTPOINT_ENTRY="mountpoint_entry";
static const gchar *MOUNTPOINT_BUTTON="mountpoint_button";
static const gchar *MODEL_COMBO="model_combo";

/* Columns for the model_combo tree model */
enum
{
    COL_POINTER,
    COL_STRING
};


/* shortcut to reference widgets when repwin->xml is already set */
#define GET_WIDGET(a) gtkpod_xml_get_widget (ii->xml,a)

/* mountpoint browse button was clicked -> open a directory browser
 * and copy the result into the mountpoint entry. */
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


static void
set_cell (GtkCellLayout   *cell_layout,
	  GtkCellRenderer *cell,
	  GtkTreeModel    *tree_model,
	  GtkTreeIter     *iter,
	  gpointer         data)
{
  gboolean header;
  gchar *text;
  IpodInfo *info;

  gtk_tree_model_get (tree_model, iter, COL_POINTER, &info, -1);
  g_return_if_fail (info);

  header = gtk_tree_model_iter_has_child (tree_model, iter);

  if (header)
  {
      text = g_strdup (
	  itdb_info_get_ipod_generation_string (info->ipod_generation));
  }
  else
  {
      if (info->capacity > 0)
      {
	  text = g_strdup_printf ("%2.0fGB %s (x%s)",
				  info->capacity,
				  itdb_info_get_ipod_model_name_string (
				      info->ipod_model),
				  info->model_number);
      }
      else
      {   /* no capacity information available */
	  text = g_strdup_printf ("%s (x%s)",
				  itdb_info_get_ipod_model_name_string (
				      info->ipod_model),
				  info->model_number);
      }
  }

  g_object_set (cell,
		"sensitive", !header,
		"text", text,
		NULL);

  g_free (text);
}


void init_model_combo (IpodInit *ii)
{
    const IpodInfo *table;
    Itdb_IpodGeneration generation;
    GtkCellRenderer *renderer;
    GtkTreeStore *store;
    gboolean info_found;
    GtkComboBox *cb;
    const IpodInfo *info;
    const gint BUFLEN = 50;
    gchar buf[BUFLEN];
    GtkEntry *entry;

    g_return_if_fail (ii && ii->itdb);

    table = itdb_info_get_ipod_info_table ();
    g_return_if_fail (table);

    /* We need the G_TYPE_STRING column because GtkComboBoxEntry
       requires it */
    store = gtk_tree_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);


    /* Create a tree model with the model numbers listed as a branch
       under each generation */
    generation = ITDB_IPOD_GENERATION_FIRST;
    do
    {
	GtkTreeIter iter;
	info = table;
	info_found = FALSE;
	while (info->model_number)
	{
	    if (info->ipod_generation == generation)
	    {
		GtkTreeIter iter_child;
		if (!info_found)
		{
		    gtk_tree_store_append (store, &iter, NULL);
		    gtk_tree_store_set (store, &iter,
					COL_POINTER, info,
					COL_STRING, "",
					-1);
		    info_found = TRUE;
		}
		gtk_tree_store_append (store, &iter_child, &iter);
		/* gtk_tree_store_set() is intelligent enough to copy
		   strings we pass to it */
		g_snprintf (buf, BUFLEN, "x%s", info->model_number);
		gtk_tree_store_set (store, &iter_child,
				    COL_POINTER, info,
				    COL_STRING, buf,
				    -1);
	    }
	    ++info;
	}
	++generation;
    } while (info_found);

    /* set the model, specify the text column, and clear the cell
       layout (glade seems to automatically add a text column which
       messes up the entire layout) */
    cb = GTK_COMBO_BOX (GET_WIDGET (MODEL_COMBO));
    gtk_combo_box_set_model (cb, GTK_TREE_MODEL (store));
    g_object_unref (store);
    gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (cb),
					 COL_STRING);
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (cb));

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), renderer, FALSE);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (cb),
					renderer,
					set_cell,
					NULL, NULL);

    /* If available set current model, otherwise indicate that */
    info = itdb_device_get_ipod_info (ii->itdb->device);
    if (info && (info->ipod_generation != ITDB_IPOD_GENERATION_UNKNOWN))
    {
	g_snprintf (buf, BUFLEN, "x%s", info->model_number);
    }
    else
    {
	g_snprintf (buf, BUFLEN, "%s", gettext (SELECT_OR_ENTER_YOUR_MODEL));
    }
    entry = GTK_ENTRY (gtk_bin_get_child(GTK_BIN (cb)));
    gtk_entry_set_text (entry, buf);
}



gboolean ipod_init (iTunesDB *itdb)
{
    IpodInit *ii;
    gint response;
    gboolean result = FALSE;
    gchar *mountpoint, *new_mount, *name, *model;
    GError *error = NULL;

    g_return_val_if_fail (itdb, FALSE);

    /* Create window */
    ii = g_new0 (IpodInit, 1);
    ii->itdb = itdb;
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

    /* Setup model combo */
    init_model_combo (ii);

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
	model = gtk_combo_box_get_active_text (
	    GTK_COMBO_BOX (GET_WIDGET (MODEL_COMBO)));
	if (strcmp (model, gettext(SELECT_OR_ENTER_YOUR_MODEL)) == 0)
	{   /* User didn't choose a model */
	    model[0] = 0;
	}

	/* Set model in the prefs system */
	set_itdb_prefs_string (itdb, KEY_IPOD_MODEL, model);

	name = get_itdb_prefs_string (itdb, "name");
	result = itdb_init_ipod (mountpoint, model, name, &error);
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
	g_free (model);
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
