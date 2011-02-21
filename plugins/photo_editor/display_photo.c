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

#include <glib/gprintf.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include "libgtkpod/gp_private.h"
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/directories.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "display_photo.h"
#include "photo_editor_context_menu.h"
#include "plugin.h"

#define DEBUG 0

#define PHOTO_YES_DONT_DISPLAY_RESPONSE 1

static GPhoto *photo_editor = NULL;

/* Drag n Drop Definitions */
static GtkTargetEntry photo_drag_types[] =
    {
        { DND_GTKPOD_PHOTOIMAGELIST_TYPE, 0, DND_GTKPOD_PHOTOIMAGELIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };

static GtkTargetEntry photo_drop_types[] =
    {
        { DND_GTKPOD_PHOTOIMAGELIST_TYPE, 0, DND_GTKPOD_PHOTOIMAGELIST },
        { "text/plain", 0, DND_TEXT_PLAIN },
        { "STRING", 0, DND_TEXT_PLAIN } };

/* Declarations */
static void gphoto_create_albumview();
static void gphoto_create_thumbnailview();
static void gphoto_build_thumbnail_model(gchar *album_name);
static void gphoto_album_selection_changed(GtkTreeSelection *selection, gpointer user_data);
static void gphoto_thumb_selection_changed(GtkIconView *iconview, gpointer user_data);
static void gphoto_display_photo_preview(Artwork *artwork);
static void on_photodb_add_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void on_photodb_add_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void on_photodb_add_image_dir_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void on_photodb_remove_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void on_photodb_remove_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void on_photodb_view_full_size_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static void on_photodb_rename_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data);
static gboolean on_click_preview_image(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void signal_data_changed();
static gchar *gphoto_get_selected_album_name();
static void gphoto_add_image_to_database(gchar *photo_filename);
static void gphoto_add_image_to_iconview(Artwork *photo, gint index);
static gboolean gphoto_button_press(GtkWidget *w, GdkEventButton *e, gpointer data);
/* DnD */
static gboolean
dnd_album_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data);
static void
        dnd_album_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);
static void
        dnd_images_drag_data_get(GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *data, guint info, guint time, gpointer user_data);

enum {
    COL_ALBUM_NAME = 0, NUM_ALBUM_COLUMNS
};

enum {
    COL_THUMB_NAIL = 0, COL_THUMB_FILENAME, COL_THUMB_ARTWORK, NUM_THUMB_COLUMNS
};

static void create_photo_editor() {
    GtkWidget *photo_window;

    photo_editor = g_malloc0(sizeof(GPhoto));

    gchar *glade_path = g_build_filename(get_glade_dir(), "photo_editor.xml", NULL);
    photo_editor->builder = gtkpod_builder_xml_new(glade_path);
    g_free(glade_path);

    photo_window = gtkpod_builder_xml_get_widget(photo_editor->builder, "photo_window");
    photo_editor->photo_album_window = gtkpod_builder_xml_get_widget(photo_editor->builder, "photo_album_window");
    photo_editor->photo_thumb_window = gtkpod_builder_xml_get_widget(photo_editor->builder, "photo_thumbnail_window");
    photo_editor->photo_preview_image_event_box = gtkpod_builder_xml_get_widget(photo_editor->builder, "photo_preview_image_event_box");
    photo_editor->photo_preview_image = GTK_IMAGE (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_preview_image"));
    photo_editor->add_album_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_add_album_menuItem"));
    photo_editor->add_image_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_add_image_menuItem"));
    photo_editor->add_image_dir_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_add_image_dir_menuItem"));
    photo_editor->remove_album_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_remove_album_menuItem"));
    photo_editor->remove_image_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_remove_image_menuItem"));
    photo_editor->view_full_size_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_view_full_size_menuItem"));
    photo_editor->rename_album_menuItem
            = GTK_MENU_ITEM (gtkpod_builder_xml_get_widget (photo_editor->builder, "photo_rename_album_menuItem"));

    photo_editor->photo_viewport = gtkpod_builder_xml_get_widget(photo_editor->builder, "photo_viewport");
    g_object_ref(photo_editor->photo_album_window);
    g_object_ref(photo_editor->photo_thumb_window);
    g_object_ref(photo_editor->photo_preview_image);
    g_object_ref(photo_editor->photo_viewport);
    gtk_container_remove(GTK_CONTAINER (photo_window), photo_editor->photo_viewport);
    /* we don't need this any more */
    gtk_widget_destroy(photo_window);

    /* Bring the menus to life */
    g_signal_connect (G_OBJECT(photo_editor->add_album_menuItem), "activate", G_CALLBACK(on_photodb_add_album_menuItem_activate),
            NULL);
    g_signal_connect (G_OBJECT(photo_editor->add_image_menuItem), "activate", G_CALLBACK(on_photodb_add_image_menuItem_activate),
            NULL);
    g_signal_connect (G_OBJECT(photo_editor->add_image_dir_menuItem), "activate", G_CALLBACK(on_photodb_add_image_dir_menuItem_activate),
            NULL);
    g_signal_connect (G_OBJECT(photo_editor->remove_album_menuItem), "activate", G_CALLBACK(on_photodb_remove_album_menuItem_activate),
            NULL);
    g_signal_connect (G_OBJECT(photo_editor->remove_image_menuItem), "activate", G_CALLBACK(on_photodb_remove_image_menuItem_activate),
            NULL);
    g_signal_connect (G_OBJECT(photo_editor->view_full_size_menuItem), "activate", G_CALLBACK(on_photodb_view_full_size_menuItem_activate),
            NULL);
    g_signal_connect (G_OBJECT(photo_editor->rename_album_menuItem), "activate", G_CALLBACK(on_photodb_rename_album_menuItem_activate),
            NULL);

    /* Add mouse click to preview image */
    g_signal_connect (G_OBJECT(photo_editor->photo_preview_image_event_box), "button-press-event", G_CALLBACK(on_click_preview_image), NULL);


    gphoto_create_albumview();
    gphoto_create_thumbnailview();

    /* Add widget in Shell. Any number of widgets can be added */
    photo_editor_plugin->photo_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_ref(photo_editor_plugin->photo_window);
    photo_editor->window = photo_editor_plugin->photo_window;
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (photo_editor->window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (photo_editor->window), GTK_SHADOW_IN);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(photo_editor->window), GTK_WIDGET (photo_editor->photo_viewport));
    anjuta_shell_add_widget(ANJUTA_PLUGIN(photo_editor_plugin)->shell, photo_editor->window, "PhotoEditorPlugin", _("  iPod Photo Editor"), NULL, ANJUTA_SHELL_PLACEMENT_CENTER, NULL);

    gtk_widget_show_all(photo_editor->window);
}

static void gphoto_set_itdb(iTunesDB *itdb) {
    ExtraiTunesDBData *eitdb;
    GtkListStore *album_model;
    GtkListStore *thumbnail_model;
    GtkTreeIter iter;
    GList *gl_album;

    if (! photo_editor)
        return;


    if (photo_editor->itdb == itdb)
            return;

#if DEBUG
    printf ("Displaying photo window\n");
    debug_list_photos (itdb);
#endif

    album_model = GTK_LIST_STORE(gtk_tree_view_get_model(photo_editor->album_view));
    if (album_model)
        gtk_list_store_clear(album_model);

    thumbnail_model = GTK_LIST_STORE (gtk_icon_view_get_model (photo_editor->thumbnail_view));
    if (thumbnail_model)
        gtk_list_store_clear(thumbnail_model);

    gtk_image_clear(photo_editor->photo_preview_image);

    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->add_album_menuItem), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->add_image_menuItem), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->add_image_dir_menuItem), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->remove_album_menuItem), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->remove_image_menuItem), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->view_full_size_menuItem), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->rename_album_menuItem), FALSE);

    photo_editor->itdb = NULL;
    photo_editor->photodb = NULL;
    photo_editor->device = NULL;

    eitdb = itdb->userdata;
    if (!eitdb->photodb || ! itdb_device_supports_photo(itdb->device)) {
        return;
    }

    photo_editor->itdb = itdb;
    photo_editor->photodb = eitdb->photodb;
    photo_editor->device = itdb->device;

    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->add_album_menuItem), TRUE);

    album_model = GTK_LIST_STORE(gtk_tree_view_get_model(photo_editor->album_view));
    for (gl_album = photo_editor->photodb->photoalbums; gl_album; gl_album = gl_album->next) {
        PhotoAlbum *album = gl_album->data;
        g_return_if_fail (album);

        gchar *name = album->name ? album->name : _("<Unnamed>");
        /*printf ("name of album: %s\n", name);*/
        /* Add a new row to the model */
        gtk_list_store_append(album_model, &iter);
        gtk_list_store_set(album_model, &iter, COL_ALBUM_NAME, name, -1);
    }

    gphoto_build_thumbnail_model(NULL);

    if (eitdb->photo_data_changed != TRUE)
        eitdb->photo_data_changed = FALSE;
}

/**
 * gphoto_display_photo_window
 *
 * Creates and opens the photo editor
 *
 * @itdb: itunes db
 */
void gphoto_display_photo_window(iTunesDB *itdb) {
    if (!photo_editor || !photo_editor->window) {
        create_photo_editor();
    }
    else {
        gtkpod_display_widget(photo_editor->window);
    }
    gphoto_set_itdb(itdb);
}

/**
 * gphoto_get_selected_photo_count
 *
 * Function to return the number of photos
 * currently selected in the iconview.
 *
 */
gint gphoto_get_selected_photo_count ()
{
    GList *selected_items= NULL;

    selected_items = gtk_icon_view_get_selected_items (photo_editor->thumbnail_view);

    if (selected_items == NULL)
        return 0;

    return g_list_length (selected_items);
}

/**
 * gphoto_create_albumview:
 *
 * Construct the album tree based upon the albums
 * stored on the iPod. If necessary destory and old
 * existing tree object.
 *
 */
static void gphoto_create_albumview() {
    GtkListStore *model;
    GtkTreeSelection *selection;
    GtkCellRenderer *renderer;

    /* destroy old listview */
    if (photo_editor->album_view) {
        model = GTK_LIST_STORE (gtk_tree_view_get_model (photo_editor->album_view));
        g_return_if_fail (model);
        g_object_unref(model);
        gtk_widget_destroy(GTK_WIDGET (photo_editor->album_view));
        photo_editor->album_view = NULL;
    }

    /* === Create New One === */

    /* create tree view */
    photo_editor->album_view = GTK_TREE_VIEW (gtk_tree_view_new ());
    if (!gtk_widget_get_realized(GTK_WIDGET(photo_editor->album_view)))
        gtk_widget_set_events(GTK_WIDGET(photo_editor->album_view), GDK_KEY_PRESS_MASK);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(photo_editor->album_view, -1, _("Photo Albums"), renderer, "text", COL_ALBUM_NAME, NULL);

    /* create model */
    model = gtk_list_store_new(NUM_ALBUM_COLUMNS, G_TYPE_STRING);

    /* set tree model */
    gtk_tree_view_set_model(photo_editor->album_view, GTK_TREE_MODEL (model));
    gtk_tree_view_set_rules_hint(photo_editor->album_view, TRUE);

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(photo_editor->album_view), GTK_SELECTION_SINGLE);

    gtk_container_add(GTK_CONTAINER (photo_editor->photo_album_window), GTK_WIDGET(photo_editor->album_view));
    gtk_widget_show_all(photo_editor->photo_album_window);

    /* function to be enacted when the album is changed */
    selection = gtk_tree_view_get_selection(photo_editor->album_view);
    g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (gphoto_album_selection_changed),
            NULL);

    g_signal_connect (G_OBJECT (photo_editor->album_view), "button-press-event", G_CALLBACK (gphoto_button_press), (gpointer) GPHOTO_ALBUM_VIEW);

    /* Disable the remove album menu item until an album is selected */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->remove_album_menuItem), FALSE);

    /* Dnd destinaton for album view */
    gtk_drag_dest_set(GTK_WIDGET (photo_editor->album_view), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT, photo_drop_types, TGNR (photo_drop_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);

    g_signal_connect ((gpointer) photo_editor->album_view, "drag-drop",
            G_CALLBACK (dnd_album_drag_drop),
            NULL);

    g_signal_connect ((gpointer) photo_editor->album_view, "drag-data-received",
            G_CALLBACK (dnd_album_drag_data_received),
            NULL);
}

/**
 * gphoto_create_thumbnailview:
 *
 * Construct the thumbnail tree based upon the
 * photos stored on the iPod associated with the
 * selected album.
 *
 */
static void gphoto_create_thumbnailview() {
    /* destroy old listview */
    if (photo_editor->thumbnail_view) {
        gtk_widget_destroy(GTK_WIDGET (photo_editor->thumbnail_view));
        photo_editor->thumbnail_view = NULL;
    }

    /* === Create New One === */
    if (photo_editor->thumbnail_view == NULL)
        photo_editor->thumbnail_view = GTK_ICON_VIEW (gtk_icon_view_new ());

    if (!gtk_widget_get_realized(GTK_WIDGET(photo_editor->thumbnail_view)))
        gtk_widget_set_events(GTK_WIDGET(photo_editor->thumbnail_view), GDK_KEY_PRESS_MASK);

    gtk_container_add(GTK_CONTAINER (photo_editor->photo_thumb_window), GTK_WIDGET(photo_editor->thumbnail_view));
    gtk_widget_show_all(photo_editor->photo_thumb_window);

    g_signal_connect (G_OBJECT (photo_editor->thumbnail_view), "button-press-event", G_CALLBACK (gphoto_button_press), (gpointer) GPHOTO_ICON_VIEW);

    /* DnD */
    gtk_drag_source_set(GTK_WIDGET (photo_editor->thumbnail_view), GDK_BUTTON1_MASK, photo_drag_types, TGNR (photo_drag_types), GDK_ACTION_COPY
            | GDK_ACTION_MOVE);

    g_signal_connect ((gpointer) photo_editor->thumbnail_view, "drag-data-get",
            G_CALLBACK (dnd_images_drag_data_get),
            NULL);
}

/**
 * gphoto_build_thumbnail_model:
 *
 * Create the model for the thumbnail view
 * based upon the selected album.
 *
 * @ album_name: name of the selected album or null if none selected
 *
 */
static void gphoto_build_thumbnail_model(gchar *album_name) {
    GtkListStore *model;
    PhotoAlbum *album;
    GList *photos;
    gint i;

    model = GTK_LIST_STORE (gtk_icon_view_get_model (photo_editor->thumbnail_view));
    if (model)
        gtk_list_store_clear(model);
    else {
        /* create model */
        model = gtk_list_store_new(NUM_THUMB_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
        gtk_icon_view_set_model(photo_editor->thumbnail_view, GTK_TREE_MODEL(model));
    }

    /* Get currently selected album from photo database */
    album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    g_return_if_fail (album);

    for (i = 0, photos = album->members; i < g_list_length(photos); ++i) {
        Artwork *photo = g_list_nth_data(photos, i);
        g_return_if_fail (photo);

        gphoto_add_image_to_iconview(photo, (i + 1));
    }

    gtk_icon_view_set_pixbuf_column(photo_editor->thumbnail_view, 0);
    gtk_icon_view_set_text_column(photo_editor->thumbnail_view, 1);
    gtk_icon_view_set_selection_mode(photo_editor->thumbnail_view, GTK_SELECTION_MULTIPLE);
    gtk_icon_view_set_columns(photo_editor->thumbnail_view, 0);
    gtk_icon_view_set_item_width(photo_editor->thumbnail_view, -1); // let the model decide how wide

    /* function to be enacted when the thumbnail is changed */
    g_signal_connect (photo_editor->thumbnail_view, "selection-changed", G_CALLBACK (gphoto_thumb_selection_changed),
            NULL);

    /* Disable the remove image menu item until an image is selected */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->remove_image_menuItem), FALSE);
    /* Disable the view full size menu item until an image is selected */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->view_full_size_menuItem), FALSE);
    /* Disable the rename menu item untill an album is selected */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->rename_album_menuItem), FALSE);
}

/**
 * gphoto_album_selection_changed:
 *
 * When the album selection is changed, rebuild the thumbnail model
 * to display those thumbnails only associated with the album.
 *
 * @ selection: album name selection
 * @ user_data: not used.
 *
 */
static void gphoto_album_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    gchar *album_name = NULL;
    PhotoAlbum *selected_album = NULL;

    album_name = gphoto_get_selected_album_name(selection);

    /* if album_name returns NULL then it merely selects the default Photo Library */
    gphoto_build_thumbnail_model(album_name);
    selected_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    if (! selected_album)
        return;

    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->add_image_menuItem), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->add_image_dir_menuItem), TRUE);

    if (album_name) {
        g_free(album_name);
        album_name = NULL;
    }

    /* Enable the remove album menu if not the Photo Library */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->remove_album_menuItem), selected_album->album_type != 0x01);
    /* Only allow renaming of album if not the Photo Library */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->rename_album_menuItem), selected_album->album_type != 0x01);

    /* Select the first icon in the album */
    GtkTreePath * path = gtk_tree_path_new_first();
    gtk_icon_view_select_path(GTK_ICON_VIEW(photo_editor->thumbnail_view), path);
    gtk_tree_path_free (path);
}

/**
 * gphoto_thumb_selection_changed:
 *
 * When the thumb view selection is changed, update the
 * preview image to display that which is selected.
 *
 * @ iconview: thumbnail view
 * @ user_data: not used
 *
 */
static void gphoto_thumb_selection_changed(GtkIconView *iconview, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    Artwork *artwork;
    GList *selected_items = NULL;

    selected_items = gtk_icon_view_get_selected_items(iconview);

    if (selected_items == NULL)
        return;

    model = gtk_icon_view_get_model(iconview);
    gtk_tree_model_get_iter(model, &iter, selected_items->data);
    gtk_tree_model_get(model, &iter, COL_THUMB_ARTWORK, &artwork, -1);
    g_return_if_fail (artwork);
    gphoto_display_photo_preview(artwork);

    /* Enable the remove image menu item until an album is selected */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->remove_image_menuItem), TRUE);
    /* Enable the view full size menu item */
    gtk_widget_set_sensitive(GTK_WIDGET(photo_editor->view_full_size_menuItem), TRUE);
}

/**
 * gphoto_display_photo_preview:
 *
 * Display the supplied photo is the preview window.
 *
 * @ artwork: photo to be displayed
 *
 */
static void gphoto_display_photo_preview(Artwork *artwork) {
    GdkPixbuf *pixbuf;

    g_return_if_fail (artwork);

    pixbuf = itdb_artwork_get_pixbuf(photo_editor->device, artwork, PHOTO_FULL_SCREEN_WIDTH, PHOTO_FULL_SCREEN_HEIGHT);
    g_return_if_fail (pixbuf);

    gtk_image_set_from_pixbuf(photo_editor->photo_preview_image, pixbuf);
    gtk_misc_set_padding(GTK_MISC(photo_editor->photo_preview_image), 20, 20);
    g_object_unref(pixbuf);
}

/**
 *
 * signal_data_changed:
 *
 * Convenience function that sets the flags on the Extra iTunes Database
 * that the photo database has changed and will need saving
 *
 */
static void signal_data_changed() {
    ExtraiTunesDBData *eitdb;

    eitdb = photo_editor->itdb->userdata;
    eitdb->photo_data_changed = TRUE;
    eitdb->data_changed = TRUE;

    gtk_image_clear(photo_editor->photo_preview_image);
}

/**
 * gadd_image_to_database
 *
 * Add a photo from file name to the photo database and
 * hence to the gui. If an album is selected other than the
 * Photo Library then the photo is added to it.
 *
 * @ photo_filename: gchar *
 *
 */
static void gphoto_add_image_to_database(gchar *photo_filename) {
    gchar *album_name = NULL;
    PhotoAlbum *selected_album;
    GError *error = NULL;
    Artwork *image = NULL;

    g_return_if_fail (photo_filename);

    /* Add the photo to the photo database and the
     * default photo library album
     */
    image = itdb_photodb_add_photo(photo_editor->photodb, photo_filename, -1, GDK_PIXBUF_ROTATE_NONE, &error);

    if (image == NULL) {
        if (error && error->message)
            gtkpod_warning("%s\n\n", error->message);
            else
            g_warning ("error->message == NULL!\n");

        g_error_free(error);
        error = NULL;
        return;
    }

    /* Add the image to the selected album if there is one selected */
    album_name = gphoto_get_selected_album_name(gtk_tree_view_get_selection(photo_editor->album_view));

    /* Find the selected album. If no selection then returns the Main Album */
    selected_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    g_return_if_fail (selected_album);

    if (selected_album->album_type != 0x01) {
        /* Add the photo to the selected album only if it is not
         * the Photo Library, as already done that.
         */
        itdb_photodb_photoalbum_add_photo(photo_editor->photodb, selected_album, image, -1);
    }

    gphoto_add_image_to_iconview(image, g_list_length(selected_album->members));

    signal_data_changed();
}

/**
 * gphoto_add_image_to_iconview
 *
 * Add an Artwork image to the icon_view
 *
 * @ photo: Artwork
 *
 */
static void gphoto_add_image_to_iconview(Artwork *photo, gint index) {
    GdkPixbuf *pixbuf = NULL;
    GtkListStore *model = NULL;
    GtkTreeIter iter;
    /* default sizes taken from smallest photo image type in itdb_device.c */
    gint icon_width = 42;
    gint icon_height = 30;

    g_return_if_fail (photo);

    model = GTK_LIST_STORE (gtk_icon_view_get_model (photo_editor->thumbnail_view));
    pixbuf = itdb_artwork_get_pixbuf(photo_editor->device, photo, icon_width, icon_height);
    g_return_if_fail (pixbuf);

    gchar *index_str = NULL;
    index_str = g_strdup_printf("%d", index);

    /* Add a new row to the model */
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, COL_THUMB_NAIL, pixbuf, COL_THUMB_FILENAME, index_str, COL_THUMB_ARTWORK, photo, -1);
    g_object_unref(pixbuf);
    g_free(index_str);
}

/**
 * gphoto_remove_album_from_database
 *
 * Remove the selected album from the photo database and the view
 *
 */
void gphoto_remove_album_from_database() {
    GtkTreeModel *album_model;
    GtkTreeIter iter;
    GtkTreeSelection *curr_selection;
    gchar *album_name;
    PhotoAlbum *selected_album;

    curr_selection = gtk_tree_view_get_selection(photo_editor->album_view);
    if (curr_selection == NULL)
        return;

    if (gtk_tree_selection_get_selected(curr_selection, &album_model, &iter) == TRUE)
        gtk_tree_model_get(album_model, &iter, COL_ALBUM_NAME, &album_name, -1);
    else
        return;

    g_return_if_fail (album_name);

    /* Find the selected album. If no selection then returns the Main Album */
    selected_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    g_return_if_fail (selected_album);
    g_free(album_name);

    if (selected_album->album_type == 0x01) {
        gtkpod_warning(_("The Photo Library album cannot be removed"));
        return;
    }

    gboolean remove_pics = FALSE;
    if (prefs_get_int("photo_library_confirm_delete") == FALSE || g_list_length(selected_album->members) <= 0) {
        /* User has chosen to assume yes and not display a confirm dialog
         * or the album is empty
         */
        remove_pics = TRUE;
    }
    else {
        /* Display a dialog asking if the user wants the photos removed as well */
        gint result;
        GtkWidget
                *dialog =
                        gtk_message_dialog_new(GTK_WINDOW(gtkpod_app), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Do you want to remove the album's photos too?"));
        gtk_dialog_add_buttons(GTK_DIALOG (dialog), GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_NO, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, _("Yes. Do Not Display Again"), PHOTO_YES_DONT_DISPLAY_RESPONSE, NULL);

        result = gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);


        switch (result) {
        case PHOTO_YES_DONT_DISPLAY_RESPONSE:
            prefs_set_int("photo_library_confirm_delete", FALSE);
        case GTK_RESPONSE_YES:
            remove_pics = TRUE;
            break;
        case GTK_RESPONSE_NO:
            remove_pics = FALSE;
            break;
        case GTK_RESPONSE_REJECT:
            return;
        default:
            break;
        }
    }

    album_model = gtk_tree_view_get_model(photo_editor->album_view);
    gtk_list_store_remove(GTK_LIST_STORE(album_model), &iter);

    itdb_photodb_photoalbum_remove(photo_editor->photodb, selected_album, remove_pics);

    /* Display the default Photo Library */
    gphoto_build_thumbnail_model(NULL);

    signal_data_changed();
}

/**
 * gphoto_remove_image_from_album
 *
 * Remove the selected image from the album
 *
 */
void gphoto_remove_selected_photos_from_album(gboolean show_dialogs) {
    GList *selected_images;
    GtkTreeModel *thumbnail_model;
    gint i;
    GtkTreeIter image_iter;
    Artwork *image;
    gchar *album_name;
    PhotoAlbum *selected_album;

    /* Find which images are selected */
    selected_images = gtk_icon_view_get_selected_items(photo_editor->thumbnail_view);
    if (g_list_length(selected_images) == 0)
        return;

    /* Find which album is selected if any */
    album_name = gphoto_get_selected_album_name(gtk_tree_view_get_selection(photo_editor->album_view));

    selected_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    GtkWidget *dialog;
    gboolean delete_pics = FALSE;

    if (show_dialogs) {
        if (selected_album != NULL && selected_album->album_type != 0x01) {
            dialog
                    = gtk_message_dialog_new(GTK_WINDOW(gtkpod_app), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("This will remove the photo selection from the selected album.\n Do you want to delete them from the database as well?"));

            gtk_dialog_add_buttons(GTK_DIALOG (dialog), GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_NO, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
        }
        else {
            dialog
                    = gtk_message_dialog_new(GTK_WINDOW(gtkpod_app), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("This will delete the photo selection from the Photo Library and all albums. Are you sure?"));

            gtk_dialog_add_buttons(GTK_DIALOG (dialog), GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_REJECT, NULL);
        }
        /* Display the dialog  */
        gint result = gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);

        switch (result) {
        case GTK_RESPONSE_YES:
            delete_pics = TRUE;
            break;
        case GTK_RESPONSE_NO:
            delete_pics = FALSE;
            break;
        case GTK_RESPONSE_REJECT:
            return;
        default:
            return;
        }
    }
    else {
        delete_pics = FALSE;
    }

    thumbnail_model = gtk_icon_view_get_model(photo_editor->thumbnail_view);
    for (i = 0; i < g_list_length(selected_images); ++i) {
        /* Find the selected image and remove it */
        gtk_tree_model_get_iter(thumbnail_model, &image_iter, g_list_nth_data(selected_images, i));
        gtk_tree_model_get(thumbnail_model, &image_iter, COL_THUMB_ARTWORK, &image, -1);

        gtk_list_store_remove(GTK_LIST_STORE(thumbnail_model), &image_iter);
        if (delete_pics)
            itdb_photodb_remove_photo(photo_editor->photodb, NULL, image); /* pass in NULL to delete pics as well */
        else
            itdb_photodb_remove_photo(photo_editor->photodb, selected_album, image);
    }

    g_free(album_name);

    signal_data_changed();
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
 * string value representing the album name selected. Must be
 * g_free()ed after use.
 */
static gchar *gphoto_get_selected_album_name(GtkTreeSelection *selection) {
    gchar *album_name = NULL;
    GtkTreeModel *album_model;
    GtkTreeIter iter;

    if (selection == NULL)
        return NULL;

    if (gtk_tree_selection_get_selected(selection, &album_model, &iter) == TRUE) { /* handle new selection */
        gtk_tree_model_get(album_model, &iter, COL_ALBUM_NAME, &album_name, -1);
    }

    return album_name;
}

PhotoAlbum *gphoto_get_selected_album() {
    gchar *album_name = NULL;
    PhotoAlbum *selected_album = NULL;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection(photo_editor->album_view);
    album_name = gphoto_get_selected_album_name(selection);
    selected_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);

    return selected_album;
}

/**
 * gphoto_rename_selected_album
 *
 * Remove the selected image from the album
 *
 */
void gphoto_rename_selected_album() {
    gchar *album_name = NULL;
    PhotoAlbum *selected_album;
    GtkTreeSelection *selection;
    /* Get the currently selected album */
    selection = gtk_tree_view_get_selection(photo_editor->album_view);
    album_name = gphoto_get_selected_album_name(selection);
    /* Find the selected album. If no selection then returns the Main Album */
    selected_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    g_return_if_fail (selected_album);

    if (selected_album->album_type == 0x01) {
        /* Dont rename the Photo Library */
        return;
    }

    gchar
            *new_album_name =
                    get_user_string(_("New Photo Album Name"), _("Please enter a new name for the photo album"), NULL, NULL, NULL, GTK_STOCK_ADD);
    if (new_album_name == NULL || strlen(new_album_name) == 0)
        return;

    /* Check an album with this name doesnt already exist */
    PhotoAlbum *curr_album;
    curr_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, new_album_name);
    if (curr_album != NULL) {
        gtkpod_warning(_("An album with that name already exists."));
        g_free(new_album_name);
        return;
    }

    /* Rename the album in the database */
    selected_album->name = g_strdup(new_album_name);

    /* Update the row in the album view */
    GtkTreeModel *album_model;
    GtkTreeIter iter;

    album_model = gtk_tree_view_get_model(photo_editor->album_view);
    if (gtk_tree_selection_get_selected(selection, &album_model, &iter) == TRUE) {
        gtk_list_store_set(GTK_LIST_STORE(album_model), &iter, COL_ALBUM_NAME, new_album_name, -1);
    }

    g_free(new_album_name);

    signal_data_changed();

    /* Using the existing selection, reselect the album so it reloads the preview of the first image */
    gphoto_album_selection_changed(selection, NULL);
}

/**
 * on_drawing_area_exposed:
 *
 * Callback for the drawing area. When the drawing area is covered,
 * resized, changed etc.. This will be called the draw() function is then
 * called from this and the cairo redrawing takes place.
 *
 * @draw_area: drawing area where al the cairo drawing takes place
 * @event: gdk expose event
 *
 * Returns:
 * boolean indicating whether other handlers should be run.
 */
static gboolean on_gphoto_preview_dialog_exposed(GtkWidget *drawarea, GdkEventExpose *event, gpointer data) {
    GdkPixbuf *image = data;

    /* Draw the image using cairo */
    cairo_t *cairo_context;

    /* get a cairo_t */
    cairo_context = gdk_cairo_create(gtk_widget_get_window(drawarea));
    /* set a clip region for the expose event */
    cairo_rectangle(cairo_context, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_clip(cairo_context);

    gdk_cairo_set_source_pixbuf(cairo_context, image, 0, 0);
    cairo_paint(cairo_context);

    cairo_destroy(cairo_context);
    return FALSE;
}

/**
 * gphoto_display_image_dialog
 *
 * @GdkPixbuf: image
 *
 * function to load a transient dialog displaying the provided image at either
 * it maximum size or the size of the screen (whichever is smallest).
 *
 */
static void gphoto_display_image_dialog (GdkPixbuf *image)
{
    g_return_if_fail (image);

    GtkWidget *dialog;
    GtkWidget *drawarea;
    GtkWidget *res_label;
    GtkBuilder *builder;
    GdkPixbuf *scaled = NULL;
    gchar *text;

    gchar *glade_path = g_build_filename(get_glade_dir(), "photo_editor.xml", NULL);
    builder = gtkpod_builder_xml_new(glade_path);
    g_free(glade_path);

    dialog = gtkpod_builder_xml_get_widget (builder, "gphoto_preview_dialog");
    drawarea = gtkpod_builder_xml_get_widget (builder, "gphoto_preview_dialog_drawarea");
    res_label = gtkpod_builder_xml_get_widget (builder, "gphoto_preview_dialog_res_lbl");
    g_return_if_fail (dialog);
    g_return_if_fail (drawarea);
    g_return_if_fail (res_label);

    /* Set the dialog parent */
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtkpod_app));

    gint pixheight = gdk_pixbuf_get_height (image);
    gint pixwidth = gdk_pixbuf_get_width (image);

    /* Set the resolution in the label */
    text = g_markup_printf_escaped (_("<b>Image Dimensions: %d x %d</b>"), pixwidth, pixheight);
    gtk_label_set_markup (GTK_LABEL (res_label), text);
    g_free (text);

    gint scrheight = gdk_screen_height() - 100;
    gint scrwidth = gdk_screen_width() - 100;

    gdouble ratio = (gdouble) pixwidth / (gdouble) pixheight;
    if (pixwidth > scrwidth)
    {
        pixwidth = scrwidth;
        pixheight = pixwidth / ratio;
    }

    if (pixheight > scrheight)
    {
        pixheight = scrheight;
        pixwidth = pixheight * ratio;
    }

    scaled = gdk_pixbuf_scale_simple (
                    image,
                    pixwidth,
                    pixheight,
                    GDK_INTERP_BILINEAR);

    /* Set the draw area's minimum size */
    gtk_widget_set_size_request  (drawarea, pixwidth, pixheight);
    g_signal_connect (G_OBJECT (drawarea), "expose_event",  G_CALLBACK (on_gphoto_preview_dialog_exposed), scaled);

    /* Display the dialog and block everything else until the
     * dialog is closed.
     */
    gtk_widget_show_all (dialog);
    gtk_dialog_run (GTK_DIALOG(dialog));

    /* Destroy the dialog as no longer required */

    g_object_unref (scaled);
    gtk_widget_destroy (GTK_WIDGET (dialog));
    g_object_unref(builder);
}

/**
 *
 * gphoto_button_press:
 *
 *  When right mouse button is pressed on one of the widgets,
 * a popup menu is displayed.
 *
 * @ w: widget upon which button press has occurred
 * @ e: button event
 * @ data: not used
 */
static gboolean gphoto_button_press(GtkWidget *w, GdkEventButton *e, gpointer data) {
    g_return_val_if_fail (w && e, FALSE);
    switch (e->button) {
    case 3:
        gphoto_context_menu_init(GPOINTER_TO_INT(data));
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/* Callbacks for the menu items in the photo window */

/**
 * on_photodb_add_album_menuItem_activate:
 *
 * Callback for add album menu item
 *
 * @ menuitem: add album menu item
 * @ user_data: not used
 *
 */
static void on_photodb_add_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    PhotoAlbum *new_album;
    GtkTreeIter iter;
    GtkListStore *model;
    gchar *album_name;

    album_name
            = get_user_string(_("New Photo Album"), _("Please enter a name for the new photo album"), FALSE, NULL, NULL, GTK_STOCK_ADD);

    if (album_name == NULL || strlen(album_name) == 0)
        return;

    /* Check an album with this name doesnt already exist */
    new_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, album_name);
    if (new_album != NULL) {
        gtkpod_warning(_("An album with that name already exists."));
        g_free(album_name);
        return;
    }

    /* Album doesn't exist so create it */
    new_album = itdb_photodb_photoalbum_create(photo_editor->photodb, album_name, -1);
    if (new_album == NULL) {
        gtkpod_warning(_("The new album failed to be created."));
        g_free(album_name);
        return;
    }

    model = GTK_LIST_STORE (gtk_tree_view_get_model (photo_editor->album_view));
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, COL_ALBUM_NAME, album_name, -1);

    g_free(album_name);
    signal_data_changed();
}

/**
 * on_photodb_add_image_menuItem_activate:
 *
 * Callback for add image menu item
 *
 * @ menuitem: add image menu item
 * @ user_data: not used
 *
 */
static void on_photodb_add_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    gchar *image_name = fileselection_get_file_or_dir(_("Add Image to iPod"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN);

    if (image_name == NULL)
        return;

    gphoto_add_image_to_database(image_name);

    g_free(image_name);
}

/**
 * _strptrcmp:
 *
 * Comparision function for comparing the filenames
 * of newly added images.
 *
 * @ a: filename 1
 * @ b: filename 2
 *
 */
static int _strptrcmp(const void* _a, const void* _b) {
    const char* const * a = (const char* const *) _a;
    const char* const * b = (const char* const *) _b;

    /* paranoia */
    if (a == b)
        return 0;
    else if (!a)
        return -1;
    else if (!b)
        return 1;
    /* real work */
    else if (*a == *b)
        return 0;
    else
        return strcmp(*a, *b);
}

/**
 * on_photodb_add_image_dir_menuItem_activate:
 *
 * Callback for add image directory menu item
 *
 * @ menuitem: add album menu item
 * @ user_data: not used
 *
 */
static void on_photodb_add_image_dir_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    GDir *directory;
    GError *error = NULL;

    /* Open a dialog directory chooser window */
    gchar
            *dir_name =
                    fileselection_get_file_or_dir(_("Add a Directory of Images to the iPod. Select the Directory."), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    if (dir_name == NULL)
        return;

    if (!g_file_test(dir_name, G_FILE_TEST_IS_DIR)) {
        g_free(dir_name);
        return;
    }

    /* Get the directory from the name chosen */
    directory = g_dir_open(dir_name, 0, &error);

    if (directory == NULL) {
        if (error && error->message)
            gtkpod_warning("%s\n\n", error->message);
            else
            g_warning ("error->message == NULL!\n");

        g_error_free(error);
        error = NULL;
        g_free(dir_name);
        return;
    }
    /* Leaf through all the files inside the directory and check if they are image
     * files. If they are then add them to the database.
     */
    G_CONST_RETURN gchar *filename;
    GPtrArray* filename_arr = g_ptr_array_new();
    unsigned u;

    do {
        filename = g_dir_read_name(directory);
        if (filename != NULL) {
            g_ptr_array_add(filename_arr, (void*) filename);
        }
    }
    while (filename != NULL);

    /* Conduct an alphabetical sort on the filenames so
     * they are added in order.
     */
    g_ptr_array_sort(filename_arr, _strptrcmp);

    for (u = 0; u < filename_arr->len; ++u) {
        gchar *full_filename;
        GdkPixbufFormat *fileformat = NULL;

        filename = g_ptr_array_index(filename_arr, u);
        full_filename = g_build_filename(dir_name, filename, NULL);

        /* Only allow valid image files to be added to the photo db. If it file is not a
         * valid image according to pixbuf then that is good enough for me!
         */
        fileformat = gdk_pixbuf_get_file_info(full_filename, NULL, NULL);
        if (fileformat != NULL) {
            gphoto_add_image_to_database(full_filename);
        }
        g_free(full_filename);
    }

    g_ptr_array_free(filename_arr, TRUE);
    g_dir_close(directory);
    g_free(dir_name);
}

/**
 * on_photodb_remove_album_menuItem_activate:
 *
 * Callback for remove album menu item
 *
 * @ menuitem: remove album menu item
 * @ user_data: not used
 *
 */
static void on_photodb_remove_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    gphoto_remove_album_from_database();
}

/**
 * on_photodb_remove_image_menuItem_activate:
 *
 * Callback for remove image menu item
 *
 * @ menuitem: remove image menu item
 * @ user_data: not used
 *
 */
static void on_photodb_remove_image_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    gphoto_remove_selected_photos_from_album(TRUE);
}

/**
 * on_photodb_view_full_size_menuItem_activate
 *
 * Callback used to display a dialog contain a full
 * size / screen size version of the selected image.
 * Same as that used in coverart display.
 *
 * @ menuitem: remove image menu item
 * @ user_data: not used
 *
 */
static void on_photodb_view_full_size_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    GList * selected_images;
    GtkTreeModel *model;
    GtkTreePath *treePath = NULL;
    GtkTreeIter iter;
    Artwork *artwork = NULL;
    GdkPixbuf * pixbuf;

    /* Using the model find the first Artwork object from the selected images list
     * Should only be one in the list if the toolbar button is being enabled/disabled
     * correctly.
     */
    model = gtk_icon_view_get_model(GTK_ICON_VIEW(photo_editor->thumbnail_view));

    /* Find which images are selected */
    selected_images = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(photo_editor->thumbnail_view));
    if (selected_images == NULL) {
        gtk_tree_model_get_iter_first(model, &iter);
    }
    else {
        treePath = g_list_nth_data(selected_images, 0);
        gtk_tree_model_get_iter(model, &iter, treePath);
    }

    gtk_tree_model_get(model, &iter, COL_THUMB_ARTWORK, &artwork, -1);
    pixbuf = itdb_artwork_get_pixbuf(photo_editor->device, artwork, -1, -1);
    g_return_if_fail (pixbuf);

    gphoto_display_image_dialog(pixbuf);
    g_object_unref(pixbuf);

}

/**
 * on_photodb_rename_album_menuItem_activate
 *
 * Callback used to rename an album.
 *
 * @ menuitem: remove image menu item
 * @ user_data: not used
 *
 */
static void on_photodb_rename_album_menuItem_activate(GtkMenuItem *menuItem, gpointer user_data) {
    gphoto_rename_selected_album();
}

static gboolean on_click_preview_image(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    on_photodb_view_full_size_menuItem_activate(NULL, NULL);
    return TRUE;
}

/* -----------------------------------------------------------*/
/* Section for album display                                  */
/* drag and drop                                                   */
/* -----------------------------------------------------------*/

/**
 * dnd_album_drag_drop:
 *
 * Allow dnd of an image onto an album row in the
 * album tree. Gives ability to add an image to a
 * different album.
 *
 */
static gboolean dnd_album_drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data) {
    GdkAtom target;
    target = gtk_drag_dest_find_target(widget, drag_context, NULL);

    if (target != GDK_NONE) {
        gboolean rowfound;

        /* determine whether a row has been dropped over in album view */
        rowfound = gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, NULL, NULL);
        if (rowfound == TRUE) {
            gtk_drag_get_data(widget, drag_context, target, time);
            return TRUE;
        }
        else
            return FALSE;
    }
    return FALSE;
}

/**
 * dnd_images_drag_data_get:
 *
 * Provide the images which are to be dnded
 * onto the new album in the album tree.
 *
 */
static void dnd_images_drag_data_get(GtkWidget *widget, GdkDragContext *dc, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    GtkTreeModel *model;
    GList *selected_images;
    gint i;

    if (!data)
        return;

    /* Find which images are selected */
    selected_images = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(widget));
    if (selected_images == NULL)
        return;

    model = gtk_icon_view_get_model(GTK_ICON_VIEW(widget));

    GtkTreePath *treePath = NULL;
    GtkTreeIter iter;
    Artwork *artwork = NULL;
    GString *reply = g_string_sized_new(2000);

    for (i = 0; i < g_list_length(selected_images); ++i) {
        treePath = g_list_nth_data(selected_images, i);
        gtk_tree_model_get_iter(model, &iter, treePath);
        gtk_tree_model_get(model, &iter, COL_THUMB_ARTWORK, &artwork, -1);
        g_string_append_printf(reply, "%p\n", artwork);
    }

    switch (info) {
    case DND_GTKPOD_PHOTOIMAGELIST:
        gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8, reply->str, reply->len);
        g_string_free(reply, TRUE);
        break;
    default:
        g_warning ("Programming error: pm_drag_data_get received unknown info type (%d)\n", info);
        break;
    }
}

/**
 * dnd_album_data_received:
 *
 * The final part of the dnd images onto album dnd
 * operation. This uses the data received and adds
 * the images to the new album.
 *
 */
static void dnd_album_drag_data_received(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    g_return_if_fail (widget);
    g_return_if_fail (dc);
    g_return_if_fail (data);
    g_return_if_fail (gtk_selection_data_get_length(data) > 0);
    g_return_if_fail (gtk_selection_data_get_data(data));
    g_return_if_fail (gtk_selection_data_get_format(data) == 8);

    gboolean rowfound;
    GtkTreePath *treepath;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *tgt_name;
    gchar *src_name;
    PhotoAlbum *tgt_album;
    PhotoAlbum *src_album;

    /* determine whether a row has been dropped over in album view */
    rowfound = gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &treepath, NULL);
    if (!rowfound) {
        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }

    g_return_if_fail (treepath);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

    /* Find the target album to drop the artwork into */
    if (gtk_tree_model_get_iter(model, &iter, treepath))
        gtk_tree_model_get(model, &iter, COL_ALBUM_NAME, &tgt_name, -1);

    gtk_tree_path_free(treepath);
    treepath = NULL;
    g_return_if_fail (tgt_name);

    tgt_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, tgt_name);
    g_return_if_fail (tgt_album);
    if (tgt_name != NULL)
        g_free(tgt_name);

    /* Find the selected album, ie. the source, or else the Photo Library if no selection */
    GtkTreeSelection *selection = NULL;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    if (selection != NULL)
        src_name = gphoto_get_selected_album_name(selection);
    else
        src_name = NULL;

    /* Find the selected album. If no selection then returns the Photo Library */
    src_album = itdb_photodb_photoalbum_by_name(photo_editor->photodb, src_name);
    g_return_if_fail (src_album);
    if (src_name != NULL)
        g_free(src_name);

    if (src_album == tgt_album) {
        gtk_drag_finish(dc, FALSE, FALSE, time);
        return;
    }

    Artwork *artwork;
    GList *artwork_list = NULL;
    gchar *datap = (gchar *) gtk_selection_data_get_data(data);
    gint i = 0;

    /* parse artwork and add each one to a GList */
    while (parse_artwork_from_string(&datap, &artwork))
        artwork_list = g_list_append(artwork_list, artwork);

    if (tgt_album->album_type != 0x01) {
        /* Only if the target is not the Photo Library (which should have the photo
         * already) should the artwork be added to the album
         */
        for (i = 0; i < g_list_length(artwork_list); ++i) {
            artwork = g_list_nth_data(artwork_list, i);
            itdb_photodb_photoalbum_add_photo(photo_editor->photodb, tgt_album, artwork, -1);
        }
    }

    /* Remove the artwork from the selected album if it is not the Photo Library */
    if (src_album->album_type != 0x01)
        gphoto_remove_selected_photos_from_album(FALSE);

    signal_data_changed();
}

void photo_editor_select_playlist_cb(GtkPodApp *app, gpointer pl, gpointer data) {
    iTunesDB *itdb = gtkpod_get_current_itdb();
    gphoto_set_itdb(itdb);
}

#if DEBUG
static void debug_list_photos(iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;
    PhotoDB *db;
    GList *gl_album;

    g_return_if_fail (itdb);

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
        }
        else
        {
            printf (_("<No members>\n"));
        }
    }
}
#endif

