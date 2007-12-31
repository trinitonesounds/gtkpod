/*
|  Copyright (C) 2007 Matvey Kozhev <sikon at users sourceforge net>
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include "misc.h"

enum info_dialog_columns
{
	C_DESCRIPTION = 0,
	C_TOTAL_IPOD,
	C_TOTAL_LOCAL,
	C_SELECTED_PLAYLIST,
	C_DISPLAYED_TRACKS,
	C_SELECTED_TRACKS,
	C_COUNT
};

static const gchar *column_headers[] =
{
	"",
	N_("Total\n(iPod)"),
	N_("Total\n(local)"),
	N_("Selected\nPlaylist"),
	N_("Displayed\nTracks"),
	N_("Selected\nTracks"),
	NULL
};

static const gchar *row_headers[] =
{
	N_("Number of tracks"),
	N_("Play time"),
	N_("File size"),
	N_("Number of playlists"),
	N_("Deleted tracks"),
	N_("File size (deleted)"),
	N_("Non-transferred tracks"),
	N_("File size (non-transferred)"),
	N_("Effective free space"),
	NULL
};

static GladeXML *info_xml = NULL;
static GtkWidget *info_dialog = NULL;
static GtkTreeView *tree = NULL;
static GtkListStore *store = NULL;

static void setup_info_dialog ()
{
	gint i;
	gint j;
	
	tree = GTK_TREE_VIEW (gtkpod_xml_get_widget (info_xml, "info_tree"));
	store = gtk_list_store_new (C_COUNT,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING,
								G_TYPE_STRING);
	
	for (i = 0; column_headers[i]; i++)
	{
		const gchar *hdr = column_headers[i][0] ? _(column_headers[i]) : column_headers[i];
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		g_object_set (renderer,
					  "editable",
					  i > 0,
					  "foreground",
					  i % 2 ? "#000000" : "#000000",
					  NULL);
		
		gtk_tree_view_insert_column_with_attributes (tree,
													 -1,
													 hdr,
													 renderer,
													 "markup",
													 i,
													 NULL);
	}
	
	for (i = 0; row_headers[i]; i++)
	{
		GtkTreeIter iter;
		gchar *text = g_strdup_printf ("<b>%s</b>", _(row_headers[i]));
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, C_DESCRIPTION, text, -1);
		g_free (text);
		
		for (j = C_DESCRIPTION + 1; j < C_COUNT; j++)
		{
			gtk_list_store_set (store, &iter, j, "dummy", -1);
		}
	}
	
	gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
	g_object_unref (store);
}

/*
	glade callback
*/
G_MODULE_EXPORT void open_info_dialog ()
{
	if(info_dialog)
	{
		gtk_window_present(GTK_WINDOW(info_dialog));
		return;
	}

	info_xml = gtkpod_xml_new (xml_file, "info_dialog");
	info_dialog = gtkpod_xml_get_widget (info_xml, "info_dialog");
	
	setup_info_dialog();
	glade_xml_signal_autoconnect (info_xml);
	gtk_widget_show(info_dialog);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_info_dialog_close ()
{
	gtk_widget_destroy(info_dialog);
	g_object_unref(info_xml);
	
	info_dialog = NULL;
	info_xml = NULL;
}
