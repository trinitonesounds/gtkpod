/*
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to compile
|  "standalone" with the gtkpod project 2002/11/24
|  (http://gtkpod.sourceforge.net)
|  Modified for UTF8 string handling under GKT2 (Jan 2003).
*/

/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "misc.h"
#include "support.h"
#include "prefs.h"
#include "song.h"
#include "charset.h"
#include "file.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <unistd.h>

#include <string.h>

#include <gtk/gtk.h>
#include <stdio.h>

/* XPM */
static char *folder[] =
{
	"16 16 16 1",
	" 	c None",
	".	c #f4f7e4",
	"X	c #dee4b5",
	"o	c #e1e7b9",
	"O	c #c6cba4",
	"+	c #dce2b8",
	"@	c #e9e9ec",
	"#	c #d3d8ae",
	"$	c #d8daca",
	"%	c #b2b2b5",
	"&	c #767862",
	"*	c #e3e6c3",
	"=	c #1b1b1a",
	"-	c #939684",
	";	c #555555",
	":	c #000000",
	"                ",
	"                ",
	"  ::::          ",
	" :.@@O:         ",
	":-&&&&&:::::    ",
	":.@@@@@*$O#O=   ",
	":@*+XXXX+##O:   ",
	":.*#oooXXXXX:   ",
	":@+XoXXXXXX#:   ",
	":@*ooXXXXXX#:   ",
	":@**XXXXXXX#:   ",
	":@*XXXXXXXX%:   ",
	":$.*OOOOOO%-:   ",
	" ;:::::::::::   ",
	"                ",
	"                "};

/* Icon by Jakub Steiner <jimmac@ximian.com> */

/* XPM */
static char *ofolder[] =
{
	"16 16 16 1",
	" 	c None",
	".	c #a9ad93",
	"X	c #60634d",
	"o	c #dee4b5",
	"O	c #9ca085",
	"+	c #0c0d04",
	"@	c #2f2f31",
	"#	c #3b3d2c",
	"$	c #c8cda2",
	"%	c #e6e6e9",
	"&	c #b3b5a5",
	"*	c #80826d",
	"=	c #292a1c",
	"-	c #fefef6",
	";	c #8f937b",
	":	c #000000",
	"                ",
	"                ",
	"  ::::          ",
	" :-%%&:         ",
	":-;;;OX:::::    ",
	":-;;;;O;O;&.:   ",
	":-*X##@@@@@=#:  ",
	":%*+-%%ooooooO: ",
	":%X;%ooooooo.*: ",
	":.+-%oooooooO:  ",
	":*O-oooooooo*:  ",
	":O-oooooooo.:   ",
	":*-%$$$$$$OX:   ",
	" :::::::::::    ",
	"                ",
	"                "};

#define NODE_SPACING 4

static GdkPixmap *folder_pixmap = NULL, *ofolder_pixmap;
static GdkBitmap *folder_mask, *ofolder_mask;
static GtkWidget *dirbrowser = NULL;

static GtkWidget *xmms_create_dir_browser(gchar * title, gchar * current_path, GtkSelectionMode mode, void (*handler) (gchar *));

struct dirnode
{
    unsigned int scanned : 1;
    char *path;
    char *dir;
};


/* ------------------------------------------------------------ *
 * functions added for gtkpod                                   *
 * ------------------------------------------------------------ */

/* turn the dirbrowser insensitive (if it's open) */
void block_dirbrowser (void)
{
    if (dirbrowser)
	gtk_widget_set_sensitive (dirbrowser, FALSE);
}

/* turn the dirbrowser sensitive (if it's open) */
void release_dirbrowser (void)
{
    if (dirbrowser)
	gtk_widget_set_sensitive (dirbrowser, TRUE);
}

/* Callback after one directory has been added */
static void add_dir_selected (gchar *dir)
{
    if (dir)
    {
	Playlist *plitem = pm_get_selected_playlist ();
	add_directory_by_name (dir, plitem,
			       prefs_get_add_recursively (),
			       NULL, NULL);
	prefs_set_last_dir_browse(dir);
	gtkpod_songs_statusbar_update();
    }
    else
    {
	/* clear log of non-updated songs */
	display_non_updated ((void *)-1, NULL);
	/* display log of updated songs */
	display_updated (NULL, NULL);
	/* display log of detected duplicates */
	remove_duplicate (NULL, NULL);
    }
}

void create_dir_browser (void)
{
    if(dirbrowser)  return;
    dirbrowser = xmms_create_dir_browser (
	_("Select directory to add recursively"),
	cfg->last_dir.browse,
	GTK_SELECTION_MULTIPLE,
	add_dir_selected);
    gtk_widget_show (dirbrowser);
}

/* called when the file selector is closed */
static void add_dir_close (GtkWidget *w1, GtkWidget *w2)
{
    if (dirbrowser)
    {
	gint x,y;
	gtk_window_get_size (GTK_WINDOW (dirbrowser), &x, &y);
	/* stor size for next time */
	prefs_set_size_dirbr (x, y);
	gtk_widget_destroy(dirbrowser);
	dirbrowser = NULL;
    }
}


/* ------------------------------------------------------------ *
 * end of added functions                                       *
 * ------------------------------------------------------------ */

static gboolean check_for_subdir(char *path)
{
	DIR *dir;
	struct dirent *dirent;
	struct stat statbuf;
	char *npath;

	if ((dir = opendir(path)) != NULL)
	{
		while ((dirent = readdir(dir)) != NULL)
		{
			if (dirent->d_name[0] == '.')
				continue;

			npath = g_strconcat(path, dirent->d_name, NULL);
			if (stat(npath, &statbuf) != -1 &&
			    S_ISDIR(statbuf.st_mode))
			{
				g_free(npath);
				closedir(dir);
				return TRUE;
			}
			g_free(npath);
		}
		closedir(dir);
	}
	return FALSE;
}

static void destroy_cb(gpointer data)
{
	struct dirnode *node = data;

	g_free(node->path);
	g_free(node->dir);
	g_free(node);
}

static void add_dir(GtkCTree *tree, GtkCTreeNode *pnode, char* parent, char *dir)
{
	struct stat statbuf;
	char *path;
	gchar *dir_utf8;

	/* Don't show hidden dirs, nor . and .. */
	if (dir[0] == '.')
		return;

	path = g_strconcat(parent, dir, NULL);
	if (stat(path, &statbuf) != -1 && S_ISDIR(statbuf.st_mode))
	{
		gboolean has_subdir;
		char *text = "";
		GtkCTreeNode *node;
		struct dirnode *dirnode = g_malloc0(sizeof (struct dirnode));
		dirnode->path = g_strconcat(path, "/", NULL);
		dirnode->dir = g_strdup (dir);
 		has_subdir = check_for_subdir(dirnode->path);
		dir_utf8 = charset_to_utf8 (dir);
		node = gtk_ctree_insert_node(tree, pnode, NULL, &dir_utf8,
					     NODE_SPACING, folder_pixmap,
					     folder_mask, ofolder_pixmap,
					     ofolder_mask, !has_subdir, FALSE);
		g_free (dir_utf8);
		gtk_ctree_node_set_row_data_full(tree, node, dirnode,
						 destroy_cb);
		if (has_subdir)
			gtk_ctree_insert_node(tree, node, NULL, &text,
					      NODE_SPACING, NULL, NULL,
					      NULL, NULL, FALSE, FALSE);
	}
	g_free(path);
}

static void expand_cb(GtkWidget *widget, GtkCTreeNode *parent_node)
{
	struct dirent *dirent;
	GtkCTree *tree = GTK_CTREE(widget);
	struct dirnode *parent_dirnode;

	parent_dirnode = gtk_ctree_node_get_row_data(tree, parent_node);
	if (!parent_dirnode->scanned)
	{
		DIR *dir;

		gtk_clist_freeze(GTK_CLIST(widget));
		gtk_ctree_remove_node(tree,
				      GTK_CTREE_ROW(parent_node)->children);
		if ((dir = opendir(parent_dirnode->path)) != NULL)
		{
			while ((dirent = readdir(dir)) != NULL)
			{
				add_dir(tree, parent_node,
					parent_dirnode->path, dirent->d_name);
			}
			closedir(dir);
			gtk_ctree_sort_node(tree, parent_node);
		}
		gtk_clist_thaw(GTK_CLIST(widget));
		parent_dirnode->scanned = TRUE;
	}
}

static void select_row_cb(GtkWidget *widget, int row, int column, GdkEventButton *bevent, gpointer data)
{
	struct dirnode *dirnode;
	GtkCTreeNode *node;
	void (*handler) (char *);

	if (bevent && (bevent->type == GDK_2BUTTON_PRESS))
	{
		node = gtk_ctree_node_nth(GTK_CTREE(widget), row);
		dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(widget), node);
		handler = gtk_object_get_user_data(GTK_OBJECT(widget));
		if (handler)
			handler(dirnode->path);
		if (handler) handler(NULL); /* call once with "NULL" to
					       indicate "end" */
	}
}

static void show_cb(GtkWidget *widget, gpointer data)
{
	GtkCTree *tree = GTK_CTREE(data);
	GtkCTreeNode *node = gtk_object_get_data(GTK_OBJECT(tree),
						 "selected_node");

	if (node)
		gtk_ctree_node_moveto(tree, node, -1, 0.6, 0);
}

static void ok_clicked(GtkWidget *widget, GtkWidget *tree)
{
	GtkCTreeNode *node;
	struct dirnode *dirnode;
	GList *list_node;
	GtkWidget *window;
	void (*handler) (char *) = NULL;

	window = gtk_object_get_user_data(GTK_OBJECT(widget));
	list_node = GTK_CLIST(tree)->selection;
	while (list_node)
	{
		node = list_node->data;
		dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(tree), node);
		handler = gtk_object_get_user_data(GTK_OBJECT(tree));
		if (handler)
			handler(dirnode->path);
		list_node = g_list_next(list_node);
	}
	if (handler) handler(NULL); /* call once with "NULL" to
				       indicate "end" */
	gtk_widget_hide(window);
	add_dir_close (widget, tree);
}

static GtkWidget *xmms_create_dir_browser(char *title, char *current_path, GtkSelectionMode mode, void (*handler) (char *))
{
	GtkWidget *window, *scroll_win, *tree, *vbox, *bbox, *ok, *cancel, *sep;
	char *root_text = "/", *text = "";
	GtkCTreeNode *root_node, *node, *selected_node = NULL;
	GtkCTree *ctree;
	struct dirnode *dirnode;
	gint x,y;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	prefs_get_size_dirbr (&x, &y);
	gtk_window_set_default_size(GTK_WINDOW(window), x, y);
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_container_border_width(GTK_CONTAINER(window), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_widget_show(scroll_win);

	gtk_widget_realize(window);
	if (!folder_pixmap)
	{
		folder_pixmap = gdk_pixmap_create_from_xpm_d(window->window,
							     &folder_mask,
							     NULL, folder);
		ofolder_pixmap = gdk_pixmap_create_from_xpm_d(window->window,
							      &ofolder_mask,
							      NULL, ofolder);
	}

	tree = gtk_ctree_new(1, 0);
	ctree = GTK_CTREE(tree);
	gtk_clist_set_column_auto_resize(GTK_CLIST(tree), 0, TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(tree), mode);
	gtk_ctree_set_line_style(ctree, GTK_CTREE_LINES_DOTTED);
	gtk_signal_connect(GTK_OBJECT(tree), "tree_expand",
			   GTK_SIGNAL_FUNC(expand_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(tree), "select_row",
			   GTK_SIGNAL_FUNC(select_row_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "show",
			   GTK_SIGNAL_FUNC(show_cb), tree);
	gtk_container_add(GTK_CONTAINER(scroll_win), tree);
	gtk_object_set_user_data(GTK_OBJECT(tree), handler);

	root_node = gtk_ctree_insert_node(ctree, NULL, NULL,
					  &root_text, NODE_SPACING,
					  folder_pixmap, folder_mask,
					  ofolder_pixmap, ofolder_mask,
					  FALSE, FALSE);
	dirnode = g_malloc0(sizeof (struct dirnode));
	dirnode->path = g_strdup("/");
	gtk_ctree_node_set_row_data_full(ctree, root_node,
					 dirnode, destroy_cb);
	node = gtk_ctree_insert_node(ctree, root_node, NULL,
				     &text, 4, NULL, NULL, NULL,
				     NULL, TRUE, TRUE);
	gtk_ctree_expand(ctree, root_node);
	gtk_widget_show(tree);

	sep = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);
	gtk_widget_show(sep);

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);

	ok = gtk_button_new_with_label(_("Ok"));
	gtk_object_set_user_data(GTK_OBJECT(ok), window);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_window_set_default(GTK_WINDOW(window), ok);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(ok_clicked), tree);
	gtk_widget_show(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
				  GTK_SIGNAL_FUNC(add_dir_close),
				  GTK_OBJECT(window));
	gtk_widget_show(cancel);

	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
	gtk_widget_show(bbox);
	gtk_widget_show(vbox);

	if (current_path && *current_path)
	{
		char **dir;
		int i;

		dir = g_strsplit(current_path, "/", 0);
		node = root_node;
		for (i = 0; dir[i] != NULL; i++)
		{
			if (dir[i][0] == '\0')
				continue;

			for (node = GTK_CTREE_ROW(node)->children; node != NULL;
			     node = GTK_CTREE_ROW(node)->sibling)
			{
				struct dirnode *dn;
				dn = gtk_ctree_node_get_row_data(GTK_CTREE(tree), node);
					if (!strcmp(dir[i], dn->dir))
					    break;
			}
			if (!node)
				break;
			if (!GTK_CTREE_ROW(node)->is_leaf && dir[i+1] && *dir[i+1])
				gtk_ctree_expand(ctree, node);
			else
			{
				selected_node = node;
				break;
			}
		}
		g_strfreev(dir);
	}

	if (!selected_node)
		selected_node = root_node;

	gtk_ctree_select(ctree, selected_node);
	gtk_object_set_data(GTK_OBJECT(tree), "selected_node", selected_node);

	return window;
}
