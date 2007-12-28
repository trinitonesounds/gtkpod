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
	{ "group_compilations", "group_compilations", NULL },
	{ "include_neverplayed", "not_played_track", NULL },
	/* Music tab */
	{ "add_subfolders", "add_recursively", NULL },
	{ "allow_duplicates", "!sha1", NULL },
	{ "delete_missing", "sync_delete_tracks", NULL },
	{ "update_existing_track", "update_existing", NULL },
	/* Metadata tab */
	{ "read_tags", "readtags", NULL },
	{ "parse_filename_tags", "parsetags", "customize_tags" },
	{ "last_resort_tags", NULL, "tag_title,tag_artist,tag_album,tag_composer,tag_genre" },
	{ "write_tags", "id3_write", "tag_encoding,write_tags_legacy,mass_modify_tags" },
	{ "write_tags_legacy", "!id3_write_id3v24", NULL },
	{ "mass_modify_tags", "multi_edit", NULL },
	{ "read_coverart", "coverart_apic", NULL },
	{ "template_coverart", "coverart_file", "customize_coverart" },
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

ind_string tag_checkbox_map[] = {
	{ 0, "tag_title" },
	{ 1, "tag_artist" },
	{ 2, "tag_album" },
	{ 3, "tag_genre" },
	{ 4, "tag_composer" },
};

static GladeXML *prefs_xml = NULL;
static GtkWidget *prefs_dialog = NULL;

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
	gboolean active = gtk_toggle_button_get_active (checkbox);
	gchar **deparray;
	int i;

	if(!deps)
		return;

	deparray = g_strsplit (deps, ",", 0);
	
	for(i = 0; deparray[i]; i++)
	{
		GtkWidget *dep = gtkpod_xml_get_widget (prefs_xml, deparray[i]);
		gtk_widget_set_sensitive (dep, active);
	}
	
	g_strfreev (deparray);
}

void init_checkbox (GtkToggleButton *checkbox, const gchar *pref, const gchar *deps)
{
	g_object_set_data(G_OBJECT(checkbox), "pref", (gchar *) pref);
	g_object_set_data(G_OBJECT(checkbox), "deps", (gchar *) deps);
	
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

GtkTreeIter tree_get_current_iter (GtkTreeView *view)
{
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreePath *path;
	GtkTreeIter iter;

	gtk_tree_view_get_cursor (view, &path, NULL);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	
	return iter;
}

/*
	Real functions
*/
void setup_prefs_dlg (GladeXML *xml, GtkWidget *dlg)
{
	gint i;
	GtkWidget *toolbar_style_combo = gtkpod_xml_get_widget (xml, "toolbar_style");
	GtkWidget *skip_track_update_radio = gtkpod_xml_get_widget (xml, "skip_track_update");
	GtkWidget *coverart_colorselect_button = gtkpod_xml_get_widget (xml, "coverart_display_bg_button");
	
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
					   checkbox_map[i][1], checkbox_map[i][2]);
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
	
	GdkColor *color = coverart_get_background_display_colour();
	gtk_color_button_set_color (GTK_COLOR_BUTTON(coverart_colorselect_button), color);
	
}

void open_prefs_dlg ()
{
	if(prefs_dialog)
	{
		gtk_window_present(GTK_WINDOW(prefs_dialog));
		return;
	}
	
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
	gtk_widget_destroy(prefs_dialog);
	g_object_unref(prefs_xml);
	
	prefs_dialog = NULL;
	prefs_xml = NULL;
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
G_MODULE_EXPORT void on_group_compilations_toggled (GtkToggleButton *sender, gpointer e)
{
	gboolean active = gtk_toggle_button_get_active (sender);
	
	prefs_set_int ("group_compilations", active);
	st_show_visible();
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
	iter = tree_get_current_iter (view);
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
	GtkTreeIter iter = tree_get_current_iter (view);
	
	if(!gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter))
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
	if(!gtk_toggle_button_get_active (sender))
	{
		int i;
		gchar *deps = (gchar *) g_object_get_data (G_OBJECT(sender), "deps");
		gchar **deparray = g_strsplit (deps, ",", 0);
		
		for(i = 0; deparray[i]; i++)
		{
			GtkWidget *dep = gtkpod_xml_get_widget (prefs_xml, deparray[i]);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(dep), FALSE);
		}
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
	
	prefs_set_string ("coverart_display_bg_colour", hexstring);
	g_free (hexstring);
	force_update_covers ();
}

/*
	glade callback
*/
G_MODULE_EXPORT void open_encoding_dialog (GtkButton *sender, gpointer e)
{
	GladeXML *xml = gtkpod_xml_new (xml_file, "prefs_encoding_dialog");
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_encoding_dialog");
	GtkWidget *combo = gtkpod_xml_get_widget (xml, "encoding_combo");
	
	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "use_encoding_for_update")),
									  "update_charset", NULL);

	init_checkbox (GTK_TOGGLE_BUTTON (gtkpod_xml_get_widget (xml, "use_encoding_for_writing")),
									  "write_charset", NULL);

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
