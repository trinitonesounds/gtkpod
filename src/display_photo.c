/*
 |Copyright (C) 2007 P.G. Richardson <phantom_sf at users.sourceforge.net>
 |Part of the gtkpod project.
 | 
 |URL: http://www.gtkpod.org/
 |URL: http://gtkpod.sourceforge.net/
 |
 |This program is free software; you can redistribute it and/or modify
 |it under the terms of the GNU General Public License as published by
 |the Free Software Foundation; either version 2 of the License, or
 |(at your option) any later version.
 |
 |This program is distributed in the hope that it will be useful,
 |but WITHOUT ANY WARRANTY; without even the implied warranty of
 |MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 |GNU General Public License for more details.
 |
 |You should have received a copy of the GNU General Public License
 |along with this program; if not, write to the Free Software
 |Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 |
 |iTunes and iPod are trademarks of Apple
 |
 |This product is not supported/written/published by Apple!
 |
 */

#include "display_photo.h"
#include "display_private.h"
#include "misc.h"
#include "prefs.h"
#include "fileselection.h"
#include "context_menus.h"
#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>


#define DEBUG 0

/* Array recording the current pages of the sorttabs so they can be redisplayed
 * if another playlist is selected.
 */
static gint sorttab_pages [SORT_TAB_MAX];

/* ipod database */
static iTunesDB *ipod_itdb;
/* Photo Database of selected ipod */
static PhotoDB *photodb;
/* Itdb Device for use with getting pixbufs from the database */
static Itdb_Device *device;
/* Main viewport containing all of the photo related components */
static GtkWidget *photo_viewport= NULL;
/* Window displaying the list of albums */
static GtkWidget *photo_album_window= NULL;
/* Window displaying the thumbnails of the photos */
static GtkWidget *photo_thumb_window= NULL;
/* pointer to the treeview for the photo album display */
static GtkTreeView *album_view= NULL;
/* pointer to the treeview for the photo thumbnail display */
static GtkIconView *thumbnail_view= NULL;
/* pointer to the gtkimage that holds the preview image */
static GtkImage *photo_preview_image= NULL;
/* Menu Items */
static GtkMenuItem *photo_add_album_menuItem= NULL;
static GtkMenuItem *photo_add_image_menuItem= NULL;
static GtkMenuItem *photo_add_image_dir_menuItem= NULL;
static GtkMenuItem *photo_remove_album_menuItem= NULL;
static GtkMenuItem *photo_remove_image_menuItem= NULL;

/* Drag n Drop Definitions */
static GtkTargetEntry photo_drag_types [] = {
		{ DND_GTKPOD_PHOTOIMAGELIST_TYPE, 0, DND_GTKPOD_PHOTOIMAGELIST },
		{ "text/plain", 0, DND_TEXT_PLAIN },
		{ "STRING", 0, DND_TEXT_PLAIN }
};

static GtkTargetEntry photo_drop_types [] = {
		{ DND_GTKPOD_PHOTOIMAGELIST_TYPE, 0, DND_GTKPOD_PHOTOIMAGELIST },
		{ "text/plain", 0, DND_TEXT_PLAIN },
		{ "STRING", 0, DND_TEXT_PLAIN }
};

/* Photo types to try and use in displaying thumbnails */
static gint photo_types[] = {
				ITDB_THUMB_PHOTO_SMALL,
		    ITDB_THUMB_PHOTO_LARGE,
		    ITDB_THUMB_PHOTO_FULL_SCREEN,
		    ITDB_THUMB_PHOTO_TV_SCREEN
		    };
static gint PHOTO_TYPES_SIZE = 4;

/* Declarations */
static void gphoto_create_albumview();
static void gphoto_create_thumbnailview();
static void gphoto_build_thumbnail_model(gchar *album_name);
static void gphoto_album_selection_changed(GtkTreeSelection *selection, gpointer user_data);
static void gphoto_thumb_selection_changed(GtkIconView *iconview, gpointer user_data);
static void gphoto_display_photo_preview(Artwork *artwork);
void on_photodb_add_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
void on_photodb_add_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
void on_photodb_add_image_dir_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
void on_photodb_remove_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
void on_photodb_remove_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void signal_data_changed();
static gchar *gphoto_get_selected_album_name();
static void gphoto_add_image_to_database(gchar *photo_filename);
static void gphoto_add_image_to_iconview(Artwork *photo, gint index);
static gboolean gphoto_button_press(GtkWidget *w, GdkEventButton *e, gpointer data);
/* DnD */
static gboolean dnd_album_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data);
static void dnd_album_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);
static void dnd_images_drag_data_get(GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *data, guint info, guint time, gpointer user_data);

enum
{
	COL_ALBUM_NAME = 0,
	NUM_ALBUM_COLUMNS
};

enum
{
	COL_THUMB_NAIL = 0,
	COL_THUMB_FILENAME,
	COL_THUMB_ARTWORK,
	NUM_THUMB_COLUMNS
};

/**
 * gphoto_load_photodb:
 *
 * Using the info in the provided itunes db, load the photo db
 * from the ipod if there is one present. Reference it in the
 * extra itunes db data structure for later use.
 * 
 * @ itdb: itunes database
 * 
 */
void gphoto_load_photodb(iTunesDB *itdb)
{
	ExtraiTunesDBData *eitdb;
	PhotoDB *db;
	const gchar *mp;
	GError *error= NULL;

	if (itdb == NULL)
		return;

	eitdb = itdb->userdata;
	mp = itdb_get_mountpoint (itdb);
	db = itdb_photodb_parse (mp, &error);
	if (db == NULL)
	{
		if (error)
		{
			gtkpod_warning (_("Error reading iPod photo database (%s).\n"), error->message);
			g_error_free (error);
			error = NULL;
		} else
		{
			gtkpod_warning (_("Error reading iPod photo database.\n"));
		}
		eitdb->photodb = NULL;
		return;
	}

	/* Set the reference to the photo database */
	eitdb->photodb = db;
	/*printf ("Reference to photo db successfully set\n");*/
}

/**
 * gphoto_ipod_supports_photos
 *
 * Convenience function that passes onto libgpod and requests
 * whether the ipod has photo support
 * 
 * Returns:
 * true/false whether the ipod has photo support
 */
gboolean gphoto_ipod_supports_photos(iTunesDB *itdb)
{
	gboolean status =itdb_device_supports_photo (itdb->device);
/*
	if (status)
		printf ("device supports photos\n");
	else
		printf ("photos not supported\n");
*/
	return status;
}

/**
 * gphoto_is_photo_playlist
 *
 * Convenience function that determines whether the
 * playlist is the photo playlist.
 * 
 * @ pl: playlist
 * 
 * Returns:
 * true/false whether the playlist is the photo playlist
 */
gboolean gphoto_is_photo_playlist(Playlist *pl)
{
	return pl->type == GP_PL_TYPE_PHOTO;
}

/**
 * gphoto_display_photo_window
 *
 * When the photo playlist is clicked on, it hands off to this
 * function which changes the entire display to the photo
 * window
 * 
 * @itdb: itunes db associated with the photo playlist clicked on
 */
void gphoto_display_photo_window(iTunesDB *itdb)
{
	ExtraiTunesDBData *eitdb;

	ipod_itdb = itdb;
#if DEBUG
	printf ("Displaying photo window\n");
	debug_list_photos (itdb);
#endif

	eitdb = itdb->userdata;
	photodb = eitdb->photodb;
	device = itdb->device;

	if (!photodb)
	{
		gtkpod_warning (_("Could not access the ipod's photo database."));
		return;
	}

	gphoto_change_to_photo_window (TRUE);

	gphoto_create_albumview ();
	gphoto_create_thumbnailview ();

	if (eitdb->photo_data_changed != TRUE)
		eitdb->photo_data_changed = FALSE;
}

/**
 * gphoto_change_to_photo_window
 *
 * When the photo playlist is NOT clicked on;
 * this changes the entire display back to original
 * rather than the photo window.
 * 
 */
void gphoto_change_to_photo_window(gboolean showflag)
{
	GtkWidget *paned1 = gtkpod_xml_get_widget (main_window_xml, "paned1");
	GtkWidget *downbutton = gtkpod_xml_get_widget (main_window_xml, "cover_down_button");
	GtkWidget *coverpanel = gtkpod_xml_get_widget (main_window_xml, "cover_display_window");
	GtkWidget *controlbox = gtkpod_xml_get_widget (main_window_xml, "cover_display_controlbox");
	GtkWidget *upbutton = gtkpod_xml_get_widget (main_window_xml, "cover_up_button");

	GtkWidget *addfilesbutton = gtkpod_xml_get_widget (main_window_xml, "add_files_button");
	GtkWidget *adddirsbutton = gtkpod_xml_get_widget (main_window_xml, "add_dirs_button");
	GtkWidget *menu_add_files = gtkpod_xml_get_widget (main_window_xml, "add_files1");
	GtkWidget *menu_add_dirs = gtkpod_xml_get_widget (main_window_xml, "add_directory1");
	GtkWidget *menu_update_file = gtkpod_xml_get_widget (main_window_xml, "update_menu");
	GtkWidget *menu_mserv_file = gtkpod_xml_get_widget (main_window_xml, "mserv_from_file_menu");
	GtkWidget *sync_menu = gtkpod_xml_get_widget (main_window_xml, "sync_menu");
	GtkWidget *export_menu = gtkpod_xml_get_widget (main_window_xml, "export_menu");
	GtkWidget *create_pl_menu = gtkpod_xml_get_widget (main_window_xml, "create_playlist_file_menu");
	GtkWidget *edit_menu = gtkpod_xml_get_widget (main_window_xml, "edit_menu");
	GtkWidget *more_sort_tabs = gtkpod_xml_get_widget (main_window_xml, "more_sort_tabs");
	GtkWidget *less_sort_tabs = gtkpod_xml_get_widget (main_window_xml, "less_sort_tabs");
	GtkWidget *arr_sort_tabs = gtkpod_xml_get_widget (main_window_xml, "arrange_sort_tabs");
	GtkWidget *tools_menu = gtkpod_xml_get_widget (main_window_xml, "tools1");
	GtkWidget *main_vbox = gtkpod_xml_get_widget (main_window_xml, "main_vbox");
	GladeXML *photo_xml;
	GtkWidget *photowin;
	gint i;

	if (showflag)
	{
		/* Record which pages are selected in the visible sorttabs */
		for (i = 0; i < SORT_TAB_MAX; ++i)
		{
			sorttab_pages[i] = st_get_sorttab_page_number (i);
		}

		/* Hide track related panes */
		gtk_widget_hide_all (paned1);
		/* Hide the coverart display */
		gtk_widget_hide (downbutton);
		gtk_widget_hide_all (coverpanel);
		gtk_widget_hide_all (controlbox);
		gtk_widget_show (upbutton);
		/* Hide menu entries and buttons */
		gtk_widget_set_sensitive (upbutton, FALSE);
		gtk_widget_set_sensitive (addfilesbutton, FALSE);
		gtk_widget_set_sensitive (adddirsbutton, FALSE);
		gtk_widget_set_sensitive (menu_add_files, FALSE);
		gtk_widget_set_sensitive (menu_add_dirs, FALSE);
		gtk_widget_set_sensitive (menu_update_file, FALSE);
		gtk_widget_set_sensitive (menu_mserv_file, FALSE);
		gtk_widget_set_sensitive (sync_menu, FALSE);
		gtk_widget_set_sensitive (export_menu, FALSE);
		gtk_widget_set_sensitive (create_pl_menu, FALSE);
		gtk_widget_set_sensitive (edit_menu, FALSE);
		gtk_widget_set_sensitive (more_sort_tabs, FALSE);
		gtk_widget_set_sensitive (less_sort_tabs, FALSE);
		gtk_widget_set_sensitive (arr_sort_tabs, FALSE);
		gtk_widget_set_sensitive (tools_menu, FALSE);

		if (photo_viewport == NULL)
		{
			photo_xml = glade_xml_new (xml_file, "photo_panel", NULL);
			photowin = gtkpod_xml_get_widget (photo_xml, "photo_panel");
			photo_album_window = gtkpod_xml_get_widget (photo_xml, "photo_album_window");
			photo_thumb_window = gtkpod_xml_get_widget (photo_xml, "photo_thumbnail_window");
			photo_preview_image = GTK_IMAGE (gtkpod_xml_get_widget (photo_xml, "photo_preview_image"));
			photo_add_album_menuItem = GTK_MENU_ITEM (gtkpod_xml_get_widget (photo_xml, "photo_add_album_menuItem"));
			photo_add_image_menuItem = GTK_MENU_ITEM (gtkpod_xml_get_widget (photo_xml, "photo_add_image_menuItem"));
			photo_add_image_dir_menuItem = GTK_MENU_ITEM (gtkpod_xml_get_widget (photo_xml, "photo_add_image_dir_menuItem"));
			photo_remove_album_menuItem = GTK_MENU_ITEM (gtkpod_xml_get_widget (photo_xml, "photo_remove_album_menuItem"));
			photo_remove_image_menuItem = GTK_MENU_ITEM (gtkpod_xml_get_widget (photo_xml, "photo_remove_image_menuItem"));

			photo_viewport = gtkpod_xml_get_widget (photo_xml, "photo_viewport");
			g_object_ref (photo_album_window);
			g_object_ref (photo_thumb_window);
			g_object_ref (photo_preview_image);
			g_object_ref (photo_viewport);
			gtk_container_remove (GTK_CONTAINER (photowin), photo_viewport);
			/* we don't need this any more */
			gtk_widget_destroy (photowin);
		}

		if (gtk_widget_get_parent (photo_viewport) == NULL)
			gtk_container_add (GTK_CONTAINER (main_vbox), photo_viewport);

		/* Bring the menus to life */
		g_signal_connect (G_OBJECT(photo_add_album_menuItem), "activate", G_CALLBACK(on_photodb_add_album_menuItem_activate), 
		NULL);
		g_signal_connect (G_OBJECT(photo_add_image_menuItem), "activate", G_CALLBACK(on_photodb_add_image_menuItem_activate), 
		NULL);
		g_signal_connect (G_OBJECT(photo_add_image_dir_menuItem), "activate", G_CALLBACK(on_photodb_add_image_dir_menuItem_activate), 
		NULL);
		g_signal_connect (G_OBJECT(photo_remove_album_menuItem), "activate", G_CALLBACK(on_photodb_remove_album_menuItem_activate), 
		NULL);
		g_signal_connect (G_OBJECT(photo_remove_image_menuItem), "activate", G_CALLBACK(on_photodb_remove_image_menuItem_activate), 
		NULL);
	} else
	{
		if (!GTK_WIDGET_VISIBLE (paned1))
		{
			if (gtk_widget_get_parent (photo_viewport) != NULL)
				gtk_container_remove (GTK_CONTAINER (main_vbox), photo_viewport);

			/* Show track related panes */
			gtk_widget_show_all (paned1);
			st_show_visible();

			/* Reselect the originally selected sorttab pages */
			for (i = 0; i < SORT_TAB_MAX; ++i)
			{
				/* Need to change current category in sorttab */
				st_set_sorttab_page ((gint) i, sorttab_pages [i]);
			}
		}

		if (prefs_get_int (KEY_DISPLAY_COVERART))
		{
			/* Show the coverart display */
			gtk_widget_show (downbutton);
			gtk_widget_show_all (coverpanel);
			gtk_widget_show_all (controlbox);
			gtk_widget_hide (upbutton);
		}
		gtk_widget_set_sensitive (upbutton, TRUE);
		gtk_widget_set_sensitive (addfilesbutton, TRUE);
		gtk_widget_set_sensitive (adddirsbutton, TRUE);
		gtk_widget_set_sensitive (menu_add_files, TRUE);
		gtk_widget_set_sensitive (menu_add_dirs, TRUE);
		gtk_widget_set_sensitive (menu_update_file, TRUE);
		gtk_widget_set_sensitive (menu_mserv_file, TRUE);
		gtk_widget_set_sensitive (sync_menu, TRUE);
		gtk_widget_set_sensitive (export_menu, TRUE);
		gtk_widget_set_sensitive (create_pl_menu, TRUE);
		gtk_widget_set_sensitive (edit_menu, TRUE);
		gtk_widget_set_sensitive (more_sort_tabs, TRUE);
		gtk_widget_set_sensitive (less_sort_tabs, TRUE);
		gtk_widget_set_sensitive (arr_sort_tabs, TRUE);
		gtk_widget_set_sensitive (tools_menu, TRUE);
	}
}

/* Create album listview */
static void gphoto_create_albumview()
{
	GtkListStore *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GList *gl_album;

	/* destroy old listview */
	if (album_view)
	{
		model = GTK_LIST_STORE (gtk_tree_view_get_model (album_view));
		g_return_if_fail (model);
		g_object_unref (model);
		gtk_widget_destroy (GTK_WIDGET (album_view));
		album_view = NULL;
	}

	/* === Create New One === */

	/* create tree view */
	album_view = GTK_TREE_VIEW (gtk_tree_view_new ());
	if (!GTK_WIDGET_REALIZED(album_view))
		gtk_widget_set_events (GTK_WIDGET(album_view), GDK_KEY_PRESS_MASK);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (album_view, -1, _("Photo Albums"), renderer, "text", COL_ALBUM_NAME, 
	NULL);

	/* create model */
	model = gtk_list_store_new (NUM_ALBUM_COLUMNS, G_TYPE_STRING);
	for (gl_album=photodb->photoalbums; gl_album; gl_album=gl_album->next)
	{
		PhotoAlbum *album = gl_album->data;
		g_return_if_fail (album);

		gchar *name = album->name ? album->name : _("<Unnamed>");
		printf ("name of album: %s\n", name);
		/* Add a new row to the model */
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, COL_ALBUM_NAME, name, -1);
	}

	/* set tree model */
	gtk_tree_view_set_model (album_view, GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (album_view, TRUE);

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (album_view), GTK_SELECTION_SINGLE);

	gtk_container_add (GTK_CONTAINER (photo_album_window), GTK_WIDGET(album_view));
	gtk_widget_show_all (photo_album_window);

	/* function to be enacted when the album is changed */
	selection = gtk_tree_view_get_selection (album_view);
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (gphoto_album_selection_changed), 
	NULL);

	g_signal_connect (G_OBJECT (album_view), "button-press-event", G_CALLBACK (gphoto_button_press), (gpointer) GPHOTO_ALBUM_VIEW);

	/* Disable the remove album menu item until an album is selected */
	gtk_widget_set_sensitive (GTK_WIDGET(photo_remove_album_menuItem), FALSE);

	/* Dnd destinaton for album view */
	gtk_drag_dest_set (
			GTK_WIDGET (album_view), 
			GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT, 
			photo_drop_types, 
			TGNR (photo_drop_types), 
			GDK_ACTION_COPY|GDK_ACTION_MOVE);

	g_signal_connect ((gpointer) album_view, "drag-drop",
			G_CALLBACK (dnd_album_drag_drop), 
			NULL);
	
	g_signal_connect ((gpointer) album_view, "drag-data-received",
			G_CALLBACK (dnd_album_drag_data_received), 
			NULL);


}

/* Create thumbnail view */
static void gphoto_create_thumbnailview()
{
	/* destroy old listview */
	if (thumbnail_view)
	{
		gtk_widget_destroy (GTK_WIDGET (thumbnail_view));
		thumbnail_view = NULL;
	}

	/* === Create New One === */
	if (thumbnail_view == NULL)
		thumbnail_view = GTK_ICON_VIEW (gtk_icon_view_new ());

	if (!GTK_WIDGET_REALIZED(thumbnail_view))
		gtk_widget_set_events (GTK_WIDGET(thumbnail_view), GDK_KEY_PRESS_MASK);

	gphoto_build_thumbnail_model (NULL);

	gtk_container_add (GTK_CONTAINER (photo_thumb_window), GTK_WIDGET(thumbnail_view));
	gtk_widget_show_all (photo_thumb_window);

	g_signal_connect (G_OBJECT (thumbnail_view), "button-press-event", G_CALLBACK (gphoto_button_press), (gpointer) GPHOTO_ICON_VIEW);

	/* DnD */
	gtk_drag_source_set (
			GTK_WIDGET (thumbnail_view),
			GDK_BUTTON1_MASK,
			photo_drag_types, 
			TGNR (photo_drag_types),
			GDK_ACTION_COPY|GDK_ACTION_MOVE);

	g_signal_connect ((gpointer) thumbnail_view, "drag-data-get", 
			G_CALLBACK (dnd_images_drag_data_get), 
			NULL);
}

/* Build the thumbnail model */
static void gphoto_build_thumbnail_model(gchar *album_name)
{
	GtkListStore *model;
	PhotoAlbum *album;
	GList *photos;
	gint i;

	model = GTK_LIST_STORE (gtk_icon_view_get_model (thumbnail_view));
	if (model)
		gtk_list_store_clear (model);
	else
	{
		/* create model */
		model = gtk_list_store_new (NUM_THUMB_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_icon_view_set_model (thumbnail_view, GTK_TREE_MODEL(model));
	}

	/* Get currently selected album from photo database */
	album = itdb_photodb_photoalbum_by_name (photodb, album_name);
	g_return_if_fail (album);

	for (i = 0, photos = album->members; i < g_list_length (photos); ++i)
	{
		Artwork *photo = g_list_nth_data (photos, i);
		g_return_if_fail (photo);

		gphoto_add_image_to_iconview (photo, (i + 1));

		/* On creation of the model, place the first image into the preview pane */
		if (i == 0)
			gphoto_display_photo_preview (photo);
	}

	gtk_icon_view_set_pixbuf_column (thumbnail_view, 0);
	gtk_icon_view_set_text_column (thumbnail_view, 1);
	gtk_icon_view_set_selection_mode (thumbnail_view, GTK_SELECTION_MULTIPLE);
	gtk_icon_view_set_columns (thumbnail_view, 0);
	gtk_icon_view_set_item_width(thumbnail_view, -1); // let the model decide how wide

	/* function to be enacted when the thumbnail is changed */
	g_signal_connect (thumbnail_view, "selection-changed", G_CALLBACK (gphoto_thumb_selection_changed), 
	NULL);

	/* Disable the remove image menu item until an image is selected */
	gtk_widget_set_sensitive (GTK_WIDGET(photo_remove_image_menuItem), FALSE);
}

/* Callback when the selection of the album is changed */
static void gphoto_album_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
	gchar *album_name= NULL;

	album_name = gphoto_get_selected_album_name (selection);

	/* if album_name returns NULL then it merely selects the default Photo Library */
	gphoto_build_thumbnail_model (album_name);

	if (album_name != NULL)
	{
		/* Enable the remove album menu item now that one is selected */
		gtk_widget_set_sensitive (GTK_WIDGET(photo_remove_album_menuItem), TRUE);
	}
}

/* Callback when the selection of a thumbnail image is changed */
static void gphoto_thumb_selection_changed(GtkIconView *iconview, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	Artwork *artwork;
	GList *selected_items= NULL;

	selected_items = gtk_icon_view_get_selected_items (iconview);

	if (selected_items == NULL|| g_list_length (selected_items) == 0)
		return;

	model = gtk_icon_view_get_model (iconview);
	gtk_tree_model_get_iter (model, &iter, selected_items->data);
	gtk_tree_model_get (model, &iter, COL_THUMB_ARTWORK, &artwork, -1);
	g_return_if_fail (artwork);
	gphoto_display_photo_preview (artwork);

	/* Enable the remove image menu item until an album is selected */
	gtk_widget_set_sensitive (GTK_WIDGET(photo_remove_image_menuItem), TRUE);
}

/* Display the selected thumbnail image in the preview window */
static void gphoto_display_photo_preview(Artwork *artwork)
{
	Thumb *thumb = NULL;
	GdkPixbuf *pixbuf, *scaled;
	gint width, height;
	gdouble ratio;
	gint i;
	
	g_return_if_fail (artwork);

	for (i = (PHOTO_TYPES_SIZE - 1); i >= 0 && thumb == NULL; --i)
	{
		/* Start from biggest photo type and go smaller */
		thumb = itdb_artwork_get_thumb_by_type (artwork, photo_types[i]);
	}
			
	/* should have a thumb now but check anyway and fire off a warning if it is still null */
	g_return_if_fail (thumb);
	
	pixbuf = itdb_thumb_get_gdk_pixbuf (device, thumb);
	g_return_if_fail (pixbuf);

	/* Size of the preview GtkImage is set to 220x176, technically 
	 * the same as PHOTO_FULL_SCREEN for a normal ipod.
	 */
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	ratio = (gdouble) width / (gdouble) height;

	if (height > PHOTO_FULL_SCREEN_HEIGHT)
	{
		height = PHOTO_FULL_SCREEN_HEIGHT;
		width = ratio * height;
	}

	if (width > PHOTO_FULL_SCREEN_WIDTH)
	{
		width = PHOTO_FULL_SCREEN_WIDTH;
		height = width / ratio;
	}

	scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_NEAREST);
	gdk_pixbuf_unref (pixbuf);

	gtk_image_set_from_pixbuf (photo_preview_image, scaled);
}

/* Convenience function that sets the flags on the Extra iTunes Database
 * that the photo database has changed and will need saving
 */
static void signal_data_changed()
{
	ExtraiTunesDBData *eitdb;

	eitdb = ipod_itdb->userdata;
	eitdb->photo_data_changed = TRUE;
	eitdb->data_changed = TRUE;
	
	gtk_image_clear (photo_preview_image);
}

/**
 * get_selected_album_name
 *
 * Given the selection of the album_treeview,
 * return the album name selected..
 * 
 * @ selection: GtkTreeSelection
 * 
 * Returns:
 * string value representing the album name selected
 */
static gchar *gphoto_get_selected_album_name(GtkTreeSelection *selection)
{
	gchar *album_name= NULL;
	GtkTreeModel *album_model;
	GtkTreeIter iter;

	if (selection == NULL)
		return NULL;

	if (gtk_tree_selection_get_selected (selection, &album_model, &iter) == TRUE)
	{ /* handle new selection */
		gtk_tree_model_get (album_model, &iter, COL_ALBUM_NAME, &album_name, -1);
	}

	return album_name;
}

/**
 * gphoto_add_image_to_database
 *
 * Add a photo from file name to the photo database and
 * hence to the gui. If an album is selected other than the
 * Photo Library then the photo is added to it.
 * 
 * @ photo_filename: gchar *
 * 
 */
static void gphoto_add_image_to_database(gchar *photo_filename)
{
	gchar *album_name= NULL;
	PhotoAlbum *selected_album;
	GError *error= NULL;
	Artwork *image= NULL;

	/* Add the photo to the photo database and the 
	 * default photo library album
	 */
	image = itdb_photodb_add_photo (photodb, photo_filename, -1, GDK_PIXBUF_ROTATE_NONE, &error);

	if (image == NULL)
	{
		if (error && error->message)
			gtkpod_warning ("%s\n\n", error->message);
		else
			g_warning ("error->message == NULL!\n");

		g_error_free (error);
		error = NULL;
		return;
	}

	/* Add the image to the selected album if there is one selected */
	album_name= gphoto_get_selected_album_name (gtk_tree_view_get_selection (album_view));

	/* Find the selected album. If no selection then returns the Main Album */
	selected_album = itdb_photodb_photoalbum_by_name (photodb, album_name);
	g_return_if_fail (selected_album);

	if (selected_album->album_type != 0x01)
	{
		/* Add the photo to the selected album only if it is not 
		 * the Photo Library, as already done that.
		 */
		itdb_photodb_photoalbum_add_photo (photodb, selected_album, image, -1);
	}

	gphoto_add_image_to_iconview (image, g_list_length (selected_album->members));

	signal_data_changed ();
}

/**
 * gphoto_add_image_to_iconview
 *
 * Add an Artwork image to the icon_view
 * 
 * @ photo: Artwork
 * 
 */
static void gphoto_add_image_to_iconview(Artwork *photo, gint index)
{
	GdkPixbuf *pixbuf= NULL;
		GdkPixbuf *scaled = NULL;
		Thumb *thumb= NULL;
		GtkListStore *model= NULL;
		GtkTreeIter iter;
		gint i;
		/* default sizes taken from smallest photo image type in itdb_device.c */
		gint icon_width = 42, icon_height = 30;
		gfloat pixbuf_width, pixbuf_height;
		gfloat ratio;

		model = GTK_LIST_STORE (gtk_icon_view_get_model (thumbnail_view));
		for (i = 0; i < PHOTO_TYPES_SIZE && thumb == NULL; ++i)
		{
			thumb = itdb_artwork_get_thumb_by_type (photo, photo_types[i]);
		}
		
		/* should have a thumb now but check anyway and fire off a warning if it is still null */
		g_return_if_fail (thumb);
		
		pixbuf = itdb_thumb_get_gdk_pixbuf (device, thumb);
		g_return_if_fail (pixbuf);

		pixbuf_width = gdk_pixbuf_get_width (pixbuf);
		pixbuf_height = gdk_pixbuf_get_height (pixbuf);
		ratio = pixbuf_width / pixbuf_height;
		
		if (pixbuf_width > icon_width)
		{
			pixbuf_width = icon_width;
			pixbuf_height = pixbuf_width / ratio;
		}
		
		if (pixbuf_height > icon_height)
		{
			pixbuf_height = icon_height;
			pixbuf_width = pixbuf_height * ratio;
		}
		
		scaled = gdk_pixbuf_scale_simple(pixbuf, pixbuf_width, pixbuf_height, GDK_INTERP_NEAREST);
		gdk_pixbuf_unref (pixbuf);
		
		gchar *index_str= NULL;
		index_str = (gchar *) g_malloc (sizeof(gint));
		g_sprintf (index_str, "%d", index);
		
		/* Add a new row to the model */
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, COL_THUMB_NAIL, scaled, COL_THUMB_FILENAME, index_str, COL_THUMB_ARTWORK, photo, -1);
		g_free (index_str);
}

/**
 * gphoto_remove_album_from_database
 *
 * Remove the selected album from the photo database and the view
 * 
 */
void gphoto_remove_album_from_database()
{
	GtkTreeModel *album_model;
	GtkTreeIter iter;
	GtkTreeSelection *curr_selection;
	gchar *album_name;
	PhotoAlbum *selected_album;

	curr_selection = gtk_tree_view_get_selection (album_view);
	if (curr_selection == NULL)
		return;

	if (gtk_tree_selection_get_selected (curr_selection, &album_model, &iter) == TRUE)
		gtk_tree_model_get (album_model, &iter, COL_ALBUM_NAME, &album_name, -1);
	else
		return;
	
	g_return_if_fail (album_name);

	/* Find the selected album. If no selection then returns the Main Album */
	selected_album = itdb_photodb_photoalbum_by_name (photodb, album_name);
	g_return_if_fail (selected_album);

	if (selected_album->album_type == 0x01)
	{
		gtkpod_warning (_("The Photo Library album cannot be removed"));
		return;
	}

	gboolean remove_pics = FALSE;
	if (g_list_length (selected_album->members) > 0)
	{
		/* Display a dialog asking if the user wants the photos removed as well */
		gint result;
		GtkWindow *parent = GTK_WINDOW (gtkpod_xml_get_widget (main_window_xml, "gtkpod"));
		GtkWidget *dialog = gtk_message_dialog_new (parent,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				_("Do you want to remove the album's photos too?"));

		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		switch (result)
		{
			case GTK_RESPONSE_YES:
				remove_pics = TRUE;
				break;
			default:
				break;
		}
	}
	album_model = gtk_tree_view_get_model (album_view);
	gtk_list_store_remove (GTK_LIST_STORE(album_model), &iter);

	itdb_photodb_photoalbum_remove (photodb, selected_album, remove_pics);

	g_free (album_name);

	/* Display the default Photo Library */
	gphoto_build_thumbnail_model (NULL);

	signal_data_changed ();
}

/**
 * gphoto_remove_image_from_album
 *
 * Remove the selected image from the album
 * 
 */
void gphoto_remove_selected_photos_from_album (gboolean show_dialogs)
{
	GList *selected_images;
	GtkTreeModel *thumbnail_model;
	gint i;
	GtkTreeIter image_iter;
	Artwork *image;
	gchar *album_name;
	PhotoAlbum *selected_album;

	/* Find which images are selected */
	selected_images = gtk_icon_view_get_selected_items (thumbnail_view);
	if (g_list_length (selected_images) == 0)
		return;

	/* Find which album is selected if any */
	album_name = gphoto_get_selected_album_name (gtk_tree_view_get_selection (album_view));

	selected_album = itdb_photodb_photoalbum_by_name (photodb, album_name);
	GtkWindow *parent = GTK_WINDOW (gtkpod_xml_get_widget (main_window_xml, "gtkpod"));

	gchar *message;
	if (selected_album != NULL&& selected_album->album_type != 0x01)
		message
				= _("The photo selection will be removed from the selected album.\nTo remove the photo from the database, remove it from the Photo Library album.");
	else
		message = _("The photo selection will be deleted from the Photo Library album");

	if (show_dialogs)
	{
		GtkWidget *dialog = gtk_message_dialog_new (parent, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	
	thumbnail_model = gtk_icon_view_get_model (thumbnail_view);
	for (i = 0; i < g_list_length (selected_images); ++i)
	{
		/* Find the selected image and remove it */
		gtk_tree_model_get_iter (thumbnail_model, &image_iter, g_list_nth_data (selected_images, i));
		gtk_tree_model_get (thumbnail_model, &image_iter, COL_THUMB_ARTWORK, &image, -1);

		gtk_list_store_remove (GTK_LIST_STORE(thumbnail_model), &image_iter);
		itdb_photodb_remove_photo (photodb, selected_album, image);
	}

	g_free (album_name);

	signal_data_changed ();
}

/* When right mouse button is pressed on one of the widgets,
 * a popup menu is displayed.
 */
static gboolean gphoto_button_press(GtkWidget *w, GdkEventButton *e, gpointer data)
{
	g_return_val_if_fail (w && e, FALSE);
	switch (e->button)
	{
		case 3:
			gphoto_context_menu_init ((gint) data);
			return TRUE;
		default:
			break;
	}
	return FALSE;
}

/* Callbacks for the menu items in the photo window */
void on_photodb_add_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data)
{
	PhotoAlbum *new_album;
	GtkTreeIter iter;
	GtkListStore *model;

	gchar *album_name = get_user_string (_("New Photo Album"), _("Please enter a name for the new photo album"), 
	NULL, 
	NULL, 
	NULL);

	if (album_name == NULL|| strlen (album_name) == 0)
		return;

	/* Check an album with this name doesnt already exist */
	new_album = itdb_photodb_photoalbum_by_name (photodb, album_name);
	if (new_album != NULL)
	{
		gtkpod_warning (_("An album with that name already exists."));
		g_free (album_name);
		return;
	}

	/* Album doesn't exist so create it */
	new_album = itdb_photodb_photoalbum_create (photodb, album_name, -1);
	if (new_album == NULL)
	{
		gtkpod_warning (_("The new album failed to be created."));
		g_free (album_name);
		return;
	}

	model = GTK_LIST_STORE (gtk_tree_view_get_model (album_view));
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, COL_ALBUM_NAME, album_name, -1);

	g_free (album_name);
	signal_data_changed ();
}

void on_photodb_add_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data)
{
	gchar *image_name = fileselection_get_file_or_dir (_("Add Image to iPod"), 
	NULL, GTK_FILE_CHOOSER_ACTION_OPEN);

	if (image_name == NULL)
		return;

	gphoto_add_image_to_database (image_name);

	g_free (image_name);
}

void on_photodb_add_image_dir_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data)
{
	GDir *directory;
	GError *error= NULL;

	/* Open a dialog directory chooser window */
	gchar*dir_name = fileselection_get_file_or_dir (_("Add a Directory of Images to the iPod. Select the Directory."), 
	NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	if (dir_name == NULL)
		return;

	if (!g_file_test (dir_name, G_FILE_TEST_IS_DIR))
	{
		g_free (dir_name);
		return;
	}

	/* Get the directory from the name chosen */
	directory = g_dir_open (dir_name, 0, &error);

	if (directory == NULL)
	{
		if (error && error->message)
			gtkpod_warning ("%s\n\n", error->message);
		else
			g_warning ("error->message == NULL!\n");

		g_error_free (error);
		error = NULL;
		g_free (dir_name);
		return;
	}

	/* Leaf through all the files inside the directory and check if they are image
	 * files. If they are then add them to the database.
	 */
	G_CONST_RETURN gchar *filename;
	do
	{
		filename = g_dir_read_name(directory);
		if (filename != NULL)
		{
			gchar *full_filename = g_build_filename (dir_name, filename, NULL);

			/* Only allow valid image files to be added to the photo db. If it file is not a
			 * valid image according to pixbuf then that is good enough for me!
			 */
			GdkPixbufFormat *fileformat= NULL;
			fileformat = gdk_pixbuf_get_file_info (full_filename, NULL, NULL);
			if (fileformat != NULL)
			{
				gphoto_add_image_to_database (full_filename);
			}
			g_free (full_filename);
		}
	} while (filename != NULL);

	g_dir_close(directory);
	g_free(dir_name);
}

void on_photodb_remove_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data)
{
	gphoto_remove_album_from_database ();
}

void on_photodb_remove_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data)
{
	gphoto_remove_selected_photos_from_album (TRUE);
}

/* -----------------------------------------------------------*/
/* Section for album display                                  */
/* drag and drop                                                   */
/* -----------------------------------------------------------*/

/* remove dragged playlist after successful MOVE */

static gboolean dnd_album_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data)
{
	GdkAtom target;
	target = gtk_drag_dest_find_target (widget, drag_context, NULL);

	if (target != GDK_NONE)
	{
		gboolean rowfound;
		
		/* determine whether a row has been dropped over in album view */
		rowfound = gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW(widget), x, y, NULL, NULL);
		if (rowfound == TRUE)
		{
			gtk_drag_get_data (widget, drag_context, target, time);
			return TRUE;
		}
		else
			return FALSE;
	}
	return FALSE;
}

static void dnd_images_drag_data_get(GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *data, guint info, guint time,
		gpointer user_data)
{
	GtkTreeModel *model;
	GList *selected_images;
	gint i;
	
	if (!data)
		return;

	/* Find which images are selected */
	selected_images = gtk_icon_view_get_selected_items (GTK_ICON_VIEW(widget));
	if (selected_images == NULL|| g_list_length (selected_images) == 0)
		return;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW(widget));
	
	GtkTreePath *treePath = NULL;
	GtkTreeIter iter;
	Artwork *artwork = NULL;
	GString *reply = g_string_sized_new (2000);
				
	for (i = 0; i < g_list_length(selected_images); ++i)
	{
		treePath = g_list_nth_data (selected_images, i);
		gtk_tree_model_get_iter (model, &iter, treePath);
		gtk_tree_model_get (model, &iter, COL_THUMB_ARTWORK, &artwork, -1);
		g_string_append_printf (reply, "%p\n", artwork);
	}
	
	switch (info)
	{
		case DND_GTKPOD_PHOTOIMAGELIST:
			gtk_selection_data_set(data, data->target, 8, reply->str, reply->len);
			g_string_free (reply, TRUE);
			break;
		default:
			g_warning ("Programming error: pm_drag_data_get received unknown info type (%d)\n", info);
			break;
	}
}

static void dnd_album_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info,
		guint time, gpointer user_data)
{
	g_return_if_fail (widget);
	g_return_if_fail (dc);
	g_return_if_fail (data);
	g_return_if_fail (data->length > 0);
	g_return_if_fail (data->data);
	g_return_if_fail (data->format == 8);

	gboolean rowfound;
	GtkTreePath *treepath;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *tgt_name;
	gchar *src_name;
	PhotoAlbum *tgt_album;
	PhotoAlbum *src_album;
	
	/* determine whether a row has been dropped over in album view */
	rowfound = gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW(widget), x, y, &treepath, NULL);
	if (! rowfound)
	{
		gtk_drag_finish (dc, FALSE, FALSE, time);
		return;
	}
	
	g_return_if_fail (treepath);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget));
	
	/* Find the target album to drop the artwork into */
	if(gtk_tree_model_get_iter (model, &iter, treepath))
		gtk_tree_model_get (model, &iter, COL_ALBUM_NAME, &tgt_name, -1);
	
	gtk_tree_path_free (treepath);
	treepath = NULL;
	g_return_if_fail (tgt_name);
	
	tgt_album = itdb_photodb_photoalbum_by_name (photodb, tgt_name);
	g_return_if_fail (tgt_album);
	if (tgt_name != NULL)
		g_free (tgt_name);
	
	/* Find the selected album, ie. the source, or else the Photo Library if no selection */
	GtkTreeSelection *selection = NULL;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(widget));
	if (selection != NULL)
		src_name = gphoto_get_selected_album_name (selection);
	else
		src_name = NULL;
	
	/* Find the selected album. If no selection then returns the Photo Library */
	src_album = itdb_photodb_photoalbum_by_name (photodb, src_name);
	g_return_if_fail (src_album);
	if (src_name != NULL)
		g_free (src_name);
	
	if (src_album == tgt_album)
	{
		gtk_drag_finish (dc, FALSE, FALSE, time);
		return;
	}
	
	Artwork *artwork;
	GList *artwork_list = NULL;
	gchar *datap = data->data;
	gint i = 0;
	
	/* parse artwork and add each one to a GList */
	while (parse_artwork_from_string (&datap, &artwork))
		artwork_list = g_list_append (artwork_list, artwork);
			    
	if (tgt_album->album_type != 0x01)
	{
		/* Only if the target is not the Photo Library (which should have the photo
		 * already) should the artwork be added to the album
		 */
		for (i = 0; i < g_list_length (artwork_list); ++i)
		{
			artwork = g_list_nth_data (artwork_list, i);
			itdb_photodb_photoalbum_add_photo (photodb, tgt_album, artwork, -1);
		}
	}
	
	/* Remove the artwork from the selected album if it is not the Photo Library */	
	if (src_album->album_type != 0x01)
		gphoto_remove_selected_photos_from_album (FALSE);
	
	signal_data_changed ();
}

#if DEBUG
static void debug_list_photos(iTunesDB *itdb)
{
	ExtraiTunesDBData *eitdb;
	PhotoDB *db;
	GList *gl_album;

	eitdb = itdb->userdata;
	db = eitdb->photodb;
	if (db == NULL)
	{
		printf ("Reference to photo database is null\n");
		return;
	}

	printf ("List of Photos stored on the ipod\n");
	for (gl_album=db->photoalbums; gl_album; gl_album=gl_album->next)
	{
		GList *gl_photo;
		PhotoAlbum *album = gl_album->data;
		g_return_if_fail (album);

		printf ("%s: ", album->name ? album->name : _("<Unnamed>"));

		for (gl_photo=album->members; gl_photo; gl_photo=gl_photo->next)
		{
			Artwork *photo = gl_photo->data;
			g_return_if_fail (photo);

			printf ("%d ", photo->id);
		}
		if (g_list_length (album->members) > 0)
		{
			printf ("\n");
		} else
		{
			printf (_("<No members>\n"));
		}
	}
}
#endif
	
