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
#include "charset.h"
#include "misc.h"
#include "help.h"
#include "prefs.h"
#include "display_coverart.h"
#include "sort_window.h"

/*
	Begin types
*/
typedef struct _ind_string
{
	gint index;
	const gchar *string;
} ind_string;

ind_string toolbar_styles[] = {
	{ -1, N_("Hide") },
	{ GTK_TOOLBAR_ICONS, N_("Icons only") },
	{ GTK_TOOLBAR_TEXT, N_("Text only") },
	{ GTK_TOOLBAR_BOTH, N_("Text under icons") },
	{ GTK_TOOLBAR_BOTH_HORIZ, N_("Text beside icons") },
	{ -1, NULL }
};

#define IND_STRING_END(x, i) ((x)[i].string == NULL)
#define COUNTOF(x) (sizeof(x) / sizeof((x)[0]))

/*
	Begin data

	0: checkbox glade ID
	1: preference
	2: dependency glade IDs, comma-separated
*/
const gchar *checkbox_map[][3] = {
	/* Display tab */
	{ "filter_tabs_top", "filter_tabs_top", NULL },
	{ "horizontal_scrollbar", "horizontal_scrollbar", NULL },
	{ "group_compilations", "group_compilations", NULL },
	{ "display_tooltips", "display_tooltips_main", NULL },
	/* Music tab */
	{ "background_transfer", "file_convert_background_transfer", NULL },
	{ "add_subfolders", "add_recursively", NULL },
	{ "allow_duplicates", "!sha1", NULL },
	{ "delete_missing", "sync_delete_tracks", NULL },
	{ "update_existing_track", "update_existing", NULL },
	{ "include_neverplayed", "not_played_track", NULL },
	{ "enable_conversion", "conversion_enable", "target_format,conversion_settings" },
	/* Metadata tab */
	{ "read_tags", "readtags", NULL },
	{ "parse_filename_tags", "parsetags", "customize_tags" },
	{ "last_resort_tags", NULL, "tag_title,tag_artist,tag_album,tag_composer,tag_genre" },
	{ "write_tags", "id3_write", "tag_encoding,write_tags_legacy,mass_modify_tags" },
	{ "write_tags_legacy", "!id3_write_id3v24", NULL },
	{ "mass_modify_tags", "multi_edit", NULL },
	{ "read_coverart", "coverart_apic", NULL },
	{ "template_coverart", "coverart_file", "customize_coverart" },
	{ "generate_video_thumbnails", "video_thumbnailer", "customize_video_thumbnailer" },
	/* Feedback tab */
	{ "confirm_del_tracks", NULL, "confirm_from_ipod,confirm_from_hdd,confirm_from_db" },
	{ "confirm_from_ipod", "delete_ipod", NULL },
	{ "confirm_from_hdd", "delete_local_file", NULL },
	{ "confirm_from_db", "delete_database", NULL },
	{ "confirm_del_pl", "delete_file", NULL },
	{ "confirm_del_sync", "sync_confirm_delete", NULL },
	{ "msg_startup", "startup_messages", NULL },
	{ "msg_duplicates", "show_duplicates", NULL },
	{ "msg_results", "sync_show_summary", NULL },
	{ "msg_updated", "show_updated", NULL },
	{ "msg_unupdated", "show_non_updated", NULL },
};

const gchar *conv_checkbox_map[][3] = {
	{ "convert_ogg", "convert_ogg", NULL },
	{ "convert_flac", "convert_flac", NULL },
	{ "convert_compatible", NULL, "convert_mp3,convert_aac,convert_wav" },
	{ "convert_mp3", "convert_mp3", NULL },
	{ "convert_aac", "convert_m4a", NULL },
	{ "convert_wav", "convert_wav", NULL },
	{ "display_conversion_log", "", NULL },
};

ind_string tag_checkbox_map[] = {
	{ 0, "tag_title" },
	{ 1, "tag_artist" },
	{ 2, "tag_album" },
	{ 3, "tag_genre" },
	{ 4, "tag_composer" },
};

const gchar *conv_scripts[] = {
	"convert-2mp3.sh",
	"convert-2m4a.sh",
};

ind_string conv_paths[] = {
	{ -1, "path_conv_ogg" },
    { -1, "path_conv_flac" },
    { TARGET_FORMAT_AAC, "path_conv_m4a" },
    { TARGET_FORMAT_MP3, "path_conv_mp3" },
    { -1, "path_conv_wav" },
};

static GladeXML *prefs_xml = NULL;
static GtkWidget *prefs_dialog = NULL;
static TempPrefs *temp_prefs = NULL;

/*
	Convenience functions
*/
void combo_box_clear (GtkComboBox *combo)
{
	GtkListStore *store;
	GtkTreeModel *model = gtk_combo_box_get_model (combo);
	g_return_if_fail (GTK_IS_LIST_STORE (model));
	store = GTK_LIST_STORE (model);
	gtk_list_store_clear (store);
}

gint ind_string_find(ind_string *array, gint index)
{
	gint i;
	
	for(i = 0; !IND_STRING_END(array, i); i++)
	{
		if(array[i].index == index)
			return i;
	}
	
	return -1;
}

void ind_string_fill_combo (ind_string *array, GtkComboBox *combo)
{
	gint i;
	combo_box_clear(combo);

	for (i = 0; !IND_STRING_END(array, i); i++)
	{
		gtk_combo_box_append_text(combo, gettext(array[i].string));
	}
}

void update_checkbox_deps (GtkToggleButton *checkbox, const gchar *deps)
{
	/* Enable or disable dependent checkboxes */
	GladeXML *xml = GLADE_XML (g_object_get_data (G_OBJECT (checkbox), "xml"));
	gboolean active = gtk_toggle_button_get_active (checkbox);
	gchar **deparray;
	int i;

	if(!xml || !deps)
		return;

	deparray = g_strsplit (deps, ",", 0);
	
	for(i = 0; deparray[i]; i++)
	{
		GtkWidget *dep = gtkpod_xml_get_widget (xml, deparray[i]);
		gtk_widget_set_sensitive (dep, active);
	}
	
	g_strfreev (deparray);
}

static void init_checkbox (GtkToggleButton *checkbox, GladeXML *xml, const gchar *pref, const gchar *deps)
{
	g_object_set_data(G_OBJECT(checkbox), "pref", (gchar *) pref);
	g_object_set_data(G_OBJECT(checkbox), "deps", (gchar *) deps);
	g_object_set_data(G_OBJECT(checkbox), "xml", xml);
	
	if(pref)
	{	
		if(pref[0] == '!')		/* Checkbox is !preference */
			gtk_toggle_button_set_active(checkbox, !prefs_get_int(pref + 1));
		else
			gtk_toggle_button_set_active(checkbox, prefs_get_int(pref));
	}
	
	update_checkbox_deps (checkbox, deps);
}

static gint column_tree_sort (GtkTreeModel *model,
							  GtkTreeIter *a,
							  GtkTreeIter *b,
							  gpointer user_data)
{
    gchar *str1, *str2;
    gint result;

    gtk_tree_model_get (model, a, 0, &str1, -1);
    gtk_tree_model_get (model, b, 0, &str2, -1);
    result = g_utf8_collate (str1, str2);

    g_free (str1);
    g_free (str2);
    return result;
}

static void setup_column_tree (GtkTreeView *treeview, gboolean list_visible)
{
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gint i;
	
	/* Delete any existing columns first */
	while (TRUE)
	{
		column = gtk_tree_view_get_column (treeview, 0);
		
		if (!column)
			break;
		
		gtk_tree_view_remove_column (treeview, column);
	}

    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 0, NULL);

    gtk_tree_view_append_column (treeview, column);
    gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

    g_object_unref (G_OBJECT (store));

    for (i = 0; i < TM_NUM_COLUMNS; i++)
    {
		gint visible = prefs_get_int_index("col_visible", i);
		
		if ((!list_visible && visible) || (list_visible && !visible))
			continue;
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set(store, &iter,
				   0, gettext (get_tm_string (i)),
				   1, i, -1);
    }
	
	if(!list_visible)
	{
		/* Sort invisible columns */
		gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),
							 column_tree_sort,
							 NULL,
							 NULL);
		
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
						  GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
						  GTK_SORT_ASCENDING);
	}
}

static gboolean tree_get_current_iter (GtkTreeView *view, GtkTreeIter *iter)
{
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreePath *path;

	gtk_tree_view_get_cursor (view, &path, NULL);
	
	if (!path)
		return FALSE;
	
	gtk_tree_model_get_iter (model, iter, path);
	gtk_tree_path_free (path);
	
	return TRUE;
}

/*
	Disconnects all signals connected to a GtkWidget.
	The "data" paremeter is ignored.
*/
static void disconnect_signals_internal (GtkWidget *widget, gpointer data)
{
	GType type;
	
	for (type = G_TYPE_FROM_INSTANCE (widget);
		 type && type != GTK_TYPE_OBJECT && type != GTK_TYPE_WIDGET;
		 type = g_type_parent (type))
	{	
		guint i;
		guint n_ids;
		guint *ids;
		
		ids = g_signal_list_ids (type, &n_ids);
		
		for (i = 0; i < n_ids; i++)
		{
			gint handler;
			
			while (TRUE)
			{
				handler = g_signal_handler_find (widget, G_SIGNAL_MATCH_ID, ids[i], 0, NULL, NULL, NULL);
				
				if (!handler)
					break;
				
				g_signal_handler_disconnect (widget, handler);
			}
		}
	}
}

static void disconnect_all_internal (GtkWidget *widget, gpointer data)
{
	disconnect_signals_internal (widget, NULL);
	
	if (GTK_IS_CONTAINER (widget))
	{
		gtk_container_foreach (GTK_CONTAINER (widget), disconnect_all_internal, NULL);
	}
}

/*
	Recursively disconnects all signals connected to a GtkWidget and its children.
*/
static void disconnect_all (GtkWidget *widget)
{
	disconnect_all_internal (widget, NULL);
}

/*
	Real functions
*/
void setup_prefs_dlg (GladeXML *xml, GtkWidget *dlg)
{
	gint i;
	GtkWidget *toolbar_style_combo = gtkpod_xml_get_widget (xml, "toolbar_style");
	GtkWidget *skip_track_update_radio = gtkpod_xml_get_widget (xml, "skip_track_update");
	GtkWidget *filter_tabs_bottom_radio = gtkpod_xml_get_widget (xml, "filter_tabs_bottom");
	GtkWidget *coverart_bgcolorselect_button = gtkpod_xml_get_widget (xml, "coverart_display_bg_button");
	GtkWidget *coverart_fgcolorselect_button = gtkpod_xml_get_widget (xml, "coverart_display_fg_button");
	GdkColor *color;
	
	/* Display */
	
	/* Toolbar */
	ind_string_fill_combo(toolbar_styles, GTK_COMBO_BOX (toolbar_style_combo));
	
	if (!prefs_get_int("display_toolbar"))
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX (toolbar_style_combo),
								 ind_string_find(toolbar_styles, -1));
	}
	else
	{
		gint style = prefs_get_int("toolbar_style");
		gint index = ind_string_find(toolbar_styles, style);
		
		gtk_combo_box_set_active(GTK_COMBO_BOX (toolbar_style_combo), index);
	}
	/* End toolbar */
	
	/* Columns */
	setup_column_tree (GTK_TREE_VIEW (gtkpod_xml_get_widget (xml, "displayed_columns")), TRUE);
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtkpod_xml_get_widget (xml, "filter_tabs_count")),
							   prefs_get_int("sort_tab_num"));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtkpod_xml_get_widget (xml, "agp_track_count")),
							   prefs_get_int("misc_track_nr"));
	
	/* Check boxes */
	for (i = 0; i < COUNTOF(checkbox_map); i++)
	{
		init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, checkbox_map[i][0])),
					   xml, checkbox_map[i][1], checkbox_map[i][2]);
	}
	
	for (i = 0; i < COUNTOF(tag_checkbox_map); i++)
	{
		GtkWidget *widget = gtkpod_xml_get_widget (xml, tag_checkbox_map[i].string);
		g_object_set_data (G_OBJECT (widget), "index", &tag_checkbox_map[i].index);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
									  prefs_get_int_index ("tag_autoset", tag_checkbox_map[i].index));
	}
	
	if(!prefs_get_int("update_existing"))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (skip_track_update_radio), TRUE);
	
	if(!prefs_get_int("filter_tabs_top"))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_tabs_bottom_radio), TRUE);
	
	color = coverart_get_background_display_color();
	gtk_color_button_set_color (GTK_COLOR_BUTTON(coverart_bgcolorselect_button), color);
	g_free (color);
	
	color = coverart_get_foreground_display_color();
	gtk_color_button_set_color (GTK_COLOR_BUTTON(coverart_fgcolorselect_button), color);
	g_free (color);
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (gtkpod_xml_get_widget (xml, "target_format")),
							  prefs_get_int ("conversion_target_format"));
}

/*
	glade callback
*/
G_MODULE_EXPORT void open_prefs_dlg ()
{
	if(prefs_dialog)
	{
		gtk_window_present(GTK_WINDOW(prefs_dialog));
		return;
	}
	
    temp_prefs = temp_prefs_create();
	temp_prefs_copy_prefs (temp_prefs);

	prefs_xml = gtkpod_xml_new (xml_file, "prefs_dialog");
	prefs_dialog = gtkpod_xml_get_widget (prefs_xml, "prefs_dialog");
	
	setup_prefs_dlg(prefs_xml, prefs_dialog);
	/*
		This is after setup because we don't want on_changed signals to fire
		during setup
	*/
	glade_xml_signal_autoconnect (prefs_xml);
	gtk_widget_show(prefs_dialog);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_prefs_dialog_help ()
{
	gtkpod_open_help_context ("gtkpod");
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_prefs_dialog_close ()
{
	if (!prefs_dialog)
		return;
	
	gtk_widget_destroy(prefs_dialog);
	g_object_unref(prefs_xml);
    temp_prefs_destroy(temp_prefs);
	
	prefs_dialog = NULL;
	prefs_xml = NULL;
	temp_prefs = NULL;
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_prefs_dialog_revert ()
{
	disconnect_all (prefs_dialog);
    temp_prefs_apply (temp_prefs);
	setup_prefs_dlg (prefs_xml, prefs_dialog);
	glade_xml_signal_autoconnect (prefs_xml);

	/* Apply all */
	tm_store_col_order ();

	tm_show_preferred_columns();
	st_show_visible();
	display_show_hide_tooltips();
	display_show_hide_toolbar();

	file_convert_prefs_changed ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_filter_tabs_count_value_changed (GtkSpinButton *sender, gpointer e)
{
    gint num = gtk_spin_button_get_value_as_int (sender);

	/* Update the number of filter tabs */
    prefs_set_int ("sort_tab_num", num);
	st_show_visible();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_agp_track_count_value_changed (GtkSpinButton *sender, gpointer e)
{
    gint num = gtk_spin_button_get_value_as_int (sender);
    prefs_set_int ("misc_track_nr", num);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_toolbar_style_changed (GtkComboBox *sender, gpointer e)
{
	gint index = toolbar_styles[gtk_combo_box_get_active(sender)].index;
	
	if (index == -1)
	{
		prefs_set_int ("display_toolbar", 0);
	}
	else
	{
		prefs_set_int ("display_toolbar", 1);
		prefs_set_int ("toolbar_style", index);
	}

	display_show_hide_toolbar ();
}

/*
	generic glade callback, used by many checkboxes
*/
G_MODULE_EXPORT void on_simple_checkbox_toggled (GtkToggleButton *sender, gpointer e)
{
	gboolean active = gtk_toggle_button_get_active (sender);
	gchar *pref = (gchar *) g_object_get_data (G_OBJECT(sender), "pref");
	gchar *deps = (gchar *) g_object_get_data (G_OBJECT(sender), "deps");
	
	if(pref)
	{
		if(pref[0] == '!')		/* Checkbox is !preference */
			prefs_set_int(pref + 1, !active);
		else
			prefs_set_int(pref, active);
	}
	
	update_checkbox_deps (sender, deps);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_tag_checkbox_toggled (GtkToggleButton *sender, gpointer e)
{
	gint index = *(gint *) g_object_get_data (G_OBJECT(sender), "index");
	prefs_set_int_index ("tag_autoset", index, gtk_toggle_button_get_active (sender));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_browse_button_clicked (GtkButton *sender, gpointer e)
{
	GtkEntry *entry = GTK_ENTRY (g_object_get_data (G_OBJECT (sender), "entry"));
	GtkWidget *dialog;

	g_return_if_fail (entry);
	
	dialog = gtk_file_chooser_dialog_new (_("Browse"),
										  GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (sender))),
										  GTK_FILE_CHOOSER_ACTION_OPEN,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
										  NULL);

	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
								   gtk_entry_get_text (entry));
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text (entry, filename);
		g_free (filename);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_group_compilations_toggled (GtkToggleButton *sender, gpointer e)
{
	gboolean active = gtk_toggle_button_get_active (sender);
	
	prefs_set_int ("group_compilations", active);
	st_show_visible();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_display_tooltips_toggled (GtkToggleButton *sender, gpointer e)
{
	gboolean active = gtk_toggle_button_get_active (sender);
	
	prefs_set_int ("display_tooltips", active);
	display_show_hide_tooltips ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_filter_tabs_top_toggled (GtkToggleButton *sender, gpointer e)
{
	gboolean active = gtk_toggle_button_get_active (sender);
	
	prefs_set_int ("filter_tabs_top", active);
	st_update_paned_position ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_horizontal_scrollbar_toggled (GtkToggleButton *sender, gpointer e)
{
	gboolean active = gtk_toggle_button_get_active (sender);
	
	prefs_set_int ("horizontal_scrollbar", active);
	tm_show_preferred_columns ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_column_add_clicked (GtkButton *sender, gpointer e)
{
	gint i;
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_columns_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_columns_dialog");
	GtkTreeView *view = GTK_TREE_VIEW (gtkpod_xml_get_widget (xml, "available_columns"));
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));
	setup_column_tree (view, FALSE);
	
	/* User pressed Cancel */
	if(!gtk_dialog_run (GTK_DIALOG (dlg)))
	{
		gtk_widget_destroy (dlg);
		g_object_unref (xml);
		return;
	}
	
	/* User pressed Add */
	model = gtk_tree_view_get_model (view);
	tree_get_current_iter (view, &iter);
	gtk_tree_model_get (model, &iter, 1, &i, -1);
	
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
	
	view = GTK_TREE_VIEW (gtkpod_xml_get_widget (prefs_xml, "displayed_columns"));
	model = gtk_tree_view_get_model (view);
	
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set(GTK_LIST_STORE (model), &iter,
			   0, gettext (get_tm_string (i)),
			   1, i, -1);
	
	prefs_set_int_index ("col_visible", i, TRUE);
	tm_store_col_order ();
	tm_show_preferred_columns ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_column_remove_clicked (GtkButton *sender, gpointer e)
{
	gint i;
	GtkTreeView *view = GTK_TREE_VIEW (gtkpod_xml_get_widget (prefs_xml, "displayed_columns"));
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreeIter iter;
	
	if(!tree_get_current_iter (view, &iter) || !gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
		return;

	gtk_tree_model_get (model, &iter, 1, &i, -1);
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	
	prefs_set_int_index ("col_visible", i, FALSE);
	tm_store_col_order ();
	tm_show_preferred_columns ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_unsetdeps_checkbox_toggled (GtkToggleButton *sender, gpointer e)
{
	GladeXML *xml = GLADE_XML (g_object_get_data (G_OBJECT(sender), "xml"));
	
	if(xml && !gtk_toggle_button_get_active (sender))
	{
		int i;
		const gchar *deps = (gchar *) g_object_get_data (G_OBJECT(sender), "deps");
		gchar **deparray = g_strsplit (deps, ",", 0);
		
		for(i = 0; deparray[i]; i++)
		{
			GtkWidget *dep = gtkpod_xml_get_widget (xml, deparray[i]);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dep), FALSE);
		}
		g_strfreev (deparray);
	}

	/* and then call the default handler */
	on_simple_checkbox_toggled (sender, e);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_coverart_dialog_bg_color_set (GtkColorButton *widget, gpointer user_data)
{
	GdkColor color;
	gtk_color_button_get_color (widget, &color);
	gchar *hexstring = g_strdup_printf("#%02X%02X%02X",
									   color.red >> 8,
									   color.green >> 8,
									   color.blue >> 8);
	
	prefs_set_string ("coverart_display_bg_color", hexstring);
	g_free (hexstring);
	coverart_display_update (FALSE);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_coverart_dialog_fg_color_set (GtkColorButton *widget, gpointer user_data)
{
	GdkColor color;
	gtk_color_button_get_color (widget, &color);
	gchar *hexstring = g_strdup_printf("#%02X%02X%02X",
									   color.red >> 8,
									   color.green >> 8,
									   color.blue >> 8);
	
	prefs_set_string ("coverart_display_fg_color", hexstring);
	g_free (hexstring);
	coverart_display_update (FALSE);
}

/*
	glade callback
*/
G_MODULE_EXPORT void open_encoding_dialog (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_encoding_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_encoding_dialog");
	GtkWidget *combo = gtkpod_xml_get_widget (xml, "encoding_combo");

	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));
	
	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "use_encoding_for_update")),
				   xml, "update_charset", NULL);

	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "use_encoding_for_writing")),
				   xml, "write_charset", NULL);

	charset_init_combo_box (GTK_COMBO_BOX (combo));
	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_encoding_combo_changed (GtkComboBox *sender, gpointer e)
{
	gchar *description = gtk_combo_box_get_active_text (sender);
	gchar *charset = charset_from_description (description);
	
	prefs_set_string ("charset", charset);
	g_free (charset);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_customize_tags_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_tag_parse_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_tag_parse_dialog");
	gchar *temp = prefs_get_string("parsetags_template");
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if(temp)
	{
		gtk_entry_set_text (GTK_ENTRY (gtkpod_xml_get_widget (xml, "filename_pattern")),
							temp);
		
		g_free (temp);
	}
	
	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "overwrite_tags")),
				   xml, "parsetags_overwrite", NULL);
	
	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_filename_pattern_changed (GtkEditable *sender, gpointer e)
{
	prefs_set_string ("parsetags_template", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_customize_coverart_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_coverart_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_coverart_dialog");
	gchar *temp = prefs_get_string("coverart_template");
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if(temp)
	{
		gtk_entry_set_text (GTK_ENTRY (gtkpod_xml_get_widget (xml, "coverart_pattern")),
							temp);
		
		g_free (temp);
	}
	
	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_customize_video_thumbnailer_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_video_thumbnailer_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_video_thumbnailer_dialog");
	gchar *temp = prefs_get_string("video_thumbnailer_prog");
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if(temp)
	{
		gtk_entry_set_text (GTK_ENTRY (gtkpod_xml_get_widget (xml, "video_thumbnailer")),
							temp);
		
		g_free (temp);
	}
	
	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}


/*
	glade callback
*/
G_MODULE_EXPORT void on_coverart_pattern_changed (GtkEditable *sender, gpointer e)
{
	prefs_set_string ("coverart_template", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_video_thumbnailer_changed (GtkEditable *sender, gpointer e)
{
	prefs_set_string ("video_thumbnailer_prog", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_exclusions_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_exclusions_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_exclusions_dialog");
	GtkWidget *tree = gtkpod_xml_get_widget (xml, "exclusion_list");
	GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    GtkTreeViewColumn *column = gtk_tree_view_column_new ();
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gchar *temp = prefs_get_string("exclude_file_mask");
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if (temp)
	{
		gint i;
		gchar **masks = g_strsplit (temp, ";", 0);
		GtkTreeIter iter;
		
		g_free (temp);
		
		for (i = 0; masks[i]; i++)
		{
			gtk_list_store_append (store, &iter);
			gtk_list_store_set(store, &iter, 0, masks[i], -1);
		}
		
		g_strfreev (masks);
	}
	
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
	g_object_unref (store);
	
	g_object_set_data (G_OBJECT (gtkpod_xml_get_widget (xml, "add_exclusion")),
					   "xml", xml);
	
	g_object_set_data (G_OBJECT (gtkpod_xml_get_widget (xml, "remove_exclusion")),
					   "xml", xml);
	
	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}

static void update_exclusions (GtkListStore *store)
{
	GtkTreeModel *model = GTK_TREE_MODEL (store);
	gint rows = gtk_tree_model_iter_n_children (model, NULL);
	gchar **array = g_new (gchar *, rows + 1);
	gchar *temp;
	gint i;
	GtkTreeIter iter;
	
	array[rows] = NULL;
	
	for (i = 0; i < rows; i++)
	{
		gtk_tree_model_iter_nth_child (model, &iter, NULL, i);
		gtk_tree_model_get (model, &iter, 0, array + i, -1);
	}
	
	temp = g_strjoinv (";", array);
	prefs_set_string ("exclude_file_mask", temp);
	g_free (temp);
	g_strfreev (array);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_add_exclusion_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = GLADE_XML (g_object_get_data (G_OBJECT (sender), "xml"));
	GtkWidget *tree = gtkpod_xml_get_widget (xml, "exclusion_list");
	GtkWidget *entry = gtkpod_xml_get_widget (xml, "new_exclusion");
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
	
	if (text && text[0])
	{
		GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
		GtkTreeIter iter;
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set(store, &iter, 0, text, -1);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		
		update_exclusions (store);
	}
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_remove_exclusion_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = GLADE_XML (g_object_get_data (G_OBJECT (sender), "xml"));
	GtkWidget *tree = gtkpod_xml_get_widget (xml, "exclusion_list");
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
		GtkTreeIter iter;
	
	if(!tree_get_current_iter (GTK_TREE_VIEW (tree), &iter) || gtk_list_store_iter_is_valid (store, &iter))
	{
		gtk_list_store_remove (store, &iter);
		update_exclusions (store);
	}
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_mserv_settings_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_mserv_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_mserv_dialog");
	gchar *temp = prefs_get_string ("mserv_username");
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if(temp)
	{
		gtk_entry_set_text (GTK_ENTRY (gtkpod_xml_get_widget (xml, "mserv_username")),
							temp);
		
		g_free (temp);
	}
	
	temp = prefs_get_string ("path_mserv_music_root");
	
	if(temp)
	{
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gtkpod_xml_get_widget (xml, "music_root")),
											 temp);
		
		g_free (temp);
	}
	
	temp = prefs_get_string ("path_mserv_trackinfo_root");
	
	if(temp)
	{
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gtkpod_xml_get_widget (xml, "mserv_root")),
											 temp);
		
		g_free (temp);
	}
	
	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "use_mserv")),
				   xml, "mserv_use", "mserv_settings_frame");
	
	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "report_mserv_problems")),
				   xml, "mserv_report_probs", NULL);
	
	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_mserv_username_changed (GtkEditable *sender, gpointer e)
{
	prefs_set_string ("mserv_username", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_music_root_current_folder_changed (GtkFileChooser *sender, gpointer e)
{
	prefs_set_string ("path_mserv_music_root",
					  gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_mserv_root_current_folder_changed (GtkFileChooser *sender, gpointer e)
{
	prefs_set_string ("path_mserv_trackinfo_root",
					  gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_commands_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_commands_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_commands_dialog");
	gchar *temp = prefs_get_string ("path_play_now");
	gchar *path;
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if(temp)
	{
		gtk_entry_set_text (GTK_ENTRY (gtkpod_xml_get_widget (xml, "cmd_playnow")),
							temp);
		
		g_free (temp);
	}

	temp = prefs_get_string ("path_play_enqueue");
	
	if(temp)
	{
		gtk_entry_set_text (GTK_ENTRY (gtkpod_xml_get_widget (xml, "cmd_enqueue")),
							temp);
		
		g_free (temp);
	}

	temp = prefs_get_string ("path_mp3gain");
	
	if(temp)
	{
		path = g_find_program_in_path (temp);
		
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (gtkpod_xml_get_widget (xml, "cmd_mp3gain")),
									   path);
		
		g_free (temp);
		g_free (path);
	}

	temp = prefs_get_string ("aacgain_path");
	
	if(temp)
	{
		path = g_find_program_in_path (temp);

		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (gtkpod_xml_get_widget (xml, "cmd_aacgain")),
									   temp);
		
		g_free (temp);
		g_free (path);
	}
	
	g_object_set_data (G_OBJECT (gtkpod_xml_get_widget (xml, "browse_playnow")),
					   "entry", gtkpod_xml_get_widget (xml, "cmd_playnow"));

	g_object_set_data (G_OBJECT (gtkpod_xml_get_widget (xml, "browse_enqueue")),
					   "entry", gtkpod_xml_get_widget (xml, "cmd_enqueue"));

	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_cmd_playnow_changed (GtkEditable *sender, gpointer e)
{
	prefs_set_string ("path_play_now", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_cmd_enqueue_changed (GtkEditable *sender, gpointer e)
{
	prefs_set_string ("path_play_enqueue", gtk_entry_get_text (GTK_ENTRY (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_cmd_mp3gain_file_set (GtkFileChooserButton *sender, gpointer e)
{
	prefs_set_string ("path_mp3gain",
					  gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_cmd_aacgain_file_set (GtkFileChooserButton *sender, gpointer e)
{
	prefs_set_string ("aacgain_path",
					  gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_conversion_settings_clicked (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_conversion_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_conversion_dialog");
	gchar *temp = prefs_get_string ("file_convert_cachedir");
	gint i;
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefs_dialog));

	if(temp)
	{
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gtkpod_xml_get_widget (xml, "cache_folder")),
											 temp);
		
		g_free (temp);
	}
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtkpod_xml_get_widget (xml, "bg_threads")),
							   prefs_get_int("file_convert_max_threads_num"));
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtkpod_xml_get_widget (xml, "cache_size")),
							   prefs_get_int("file_convert_maxdirsize"));

	for (i = 0; i < COUNTOF(conv_checkbox_map); i++)
	{
		init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, conv_checkbox_map[i][0])),
					   xml, conv_checkbox_map[i][1], conv_checkbox_map[i][2]);
	}

	glade_xml_signal_autoconnect (xml);
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	g_object_unref (xml);
	file_convert_prefs_changed ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_cache_folder_current_folder_changed (GtkFileChooser *sender, gpointer e)
{
	prefs_set_string ("file_convert_cachedir",
					  gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (sender)));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_bg_threads_value_changed (GtkSpinButton *sender, gpointer e)
{
    prefs_set_int ("file_convert_max_threads_num", gtk_spin_button_get_value_as_int (sender));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_cache_size_value_changed (GtkSpinButton *sender, gpointer e)
{
    prefs_set_int ("file_convert_maxdirsize", gtk_spin_button_get_value_as_int (sender));
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_target_format_changed (GtkComboBox *sender, gpointer e)
{
	gint index = gtk_combo_box_get_active (sender);
	gchar *script = g_build_filename (SCRIPTDIR, conv_scripts[index], NULL);
	gint i;
	
	for (i = 0; i < COUNTOF (conv_paths); i++)
	{
		if (conv_paths[i].index == index)
		{
			/*
				The source format is the same as the target format -
				we set "null conversion" without touching the boolean preference
			*/
			prefs_set_string (conv_paths[i].string, "");
		}
		else
			prefs_set_string (conv_paths[i].string, script);
	}
	
	prefs_set_int ("conversion_target_format", index);
	g_free (script);
	file_convert_prefs_changed ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void on_sorting_button_clicked (GtkButton *sender, gpointer e)
{
	sort_window_create ();
}
