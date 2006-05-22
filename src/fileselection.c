/* Time-stamp: <2006-05-22 23:19:00 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

/***********************************************************************
 * Functions for file chooser dialogs provided by:
 *
 *  Fri May 27 22:13:20 2005
 *  Copyright  2005  James Liggett
 *  Email jrliggett@cox.net
 ***********************************************************************/
 
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include "charset.h"
#include "display.h"
#include "file.h"
#include "prefs.h"
#include "info.h"
#include "misc.h"
#include "misc_track.h"
#include "fileselection.h"

/* 
 * Data global to this module only
 */

static iTunesDB *fc_active_itdb = NULL;  /* the active iTunesDB, if any */

/* Cleans up the GSList of filenames returned by the file chooser */
static void delete_file_list(GSList* list)
{
    GSList* current;  /* Current node in list */
	
    /* Delete the string contained in each node */
    current = list;
	
    while (current != NULL)
    {
	g_free(current->data);
	current = current->next;
    }
	
    g_slist_free(list);
}

/* OK button */
static void add_files_ok(GtkFileChooser* filechooser)
{
    GSList* names;   /* List of selected names */
    GSList* current; /* Current node in list */
    Playlist* playlist; /* Playlist to add songs to */
    gboolean result = TRUE;  /* Result of file adding */
	
    /* If we don't have a playlist to add to, don't add anything */
    g_return_if_fail(fc_active_itdb);
	
    block_widgets ();

    playlist = pm_get_selected_playlist();

    names = gtk_file_chooser_get_filenames(filechooser);
    current = names;

    if (current)
    {
	gchar *dirname = gtk_file_chooser_get_current_folder (filechooser);
	prefs_set_string ("last_dir_browsed", dirname);
	g_free (dirname);
    }

    /* Get the filenames and add them */
    while (current != NULL)
    {
	result &= add_track_by_filename(fc_active_itdb,
					(gchar*)current->data,
					playlist,
					prefs_get_add_recursively(),
					NULL, NULL);
	current = current->next;
    }
	
    /* clear log of non-updated tracks */
    display_non_updated ((void *)-1, NULL);
  
    /* display log of updated tracks */
    display_updated (NULL, NULL);
  
    /* display log of detected duplicates */
    gp_duplicate_remove (NULL, NULL);
	
    /* Were all files successfully added? */
    if (result == TRUE)
	gtkpod_statusbar_message (_("Successfully added files"));
    else
	gtkpod_statusbar_message (_("Some files were not added successfully"));
	
    /* Clean up the names list */
    delete_file_list(names);
    release_widgets ();
}

/* 
 * Add Files Dialog
 */
/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function make sure you define a
   separate callback for gtkpod.glade */
void create_add_files_dialog (void)
{
    GtkWidget* fc;  /* The file chooser dialog */
    gint response;  /* The response of the filechooser */
    gchar *last_dir;
	
    /* Grab the current playlist to add songs to */
    fc_active_itdb = gp_get_active_itdb ();
    g_return_if_fail (fc_active_itdb);
	
    /* Create the file chooser, and handle the response */
    fc = gtk_file_chooser_dialog_new (_("Add Files"),
				      NULL,
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL,
				      GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN,
				      GTK_RESPONSE_ACCEPT,
				      NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (fc), TRUE);
    last_dir = prefs_get_string ("last_dir_browsed");
    if (last_dir)
    {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fc),
					     last_dir);
	g_free (last_dir);
    }

    response = gtk_dialog_run(GTK_DIALOG(fc));
	
    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
	add_files_ok(GTK_FILE_CHOOSER(fc));
	break;
    case GTK_RESPONSE_CANCEL:
	break;
    default:	/* Fall through */
	break;
    }		
    gtk_widget_destroy(fc);
}


/* OK Button */
static void add_playlists_ok(GtkFileChooser* filechooser)
{
    GSList* names;  /* List of selected names */
    GSList* current;  /* Current node in names list */
	
    /* Get the names of the playlist(s) and add them */
	
    /* If we don't have a playlist to add to, return */
    g_return_if_fail(fc_active_itdb);
	
    block_widgets ();

    names = gtk_file_chooser_get_filenames(filechooser);
    current = names;

    if (current)
    {
	gchar *dirname = gtk_file_chooser_get_current_folder (filechooser);
	prefs_set_string ("last_dir_browsed", dirname);
	g_free (dirname);
    }

    while (current != NULL)
    {
	add_playlist_by_filename (fc_active_itdb,
				  (gchar*)current->data, NULL, 
				  -1, NULL, NULL);
	current = current->next;
    }
		
    gtkpod_tracks_statusbar_update();
    delete_file_list(names);

    release_widgets ();
}


/*
 * Add Playlist Dialog
 */
/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function make sure you define a
   separate callback for gtkpod.glade */
void create_add_playlists_dialog(void)
{
    GtkWidget* fc ; /* The file chooser dialog */
    gint response;  /* The response of the filechooser */
    gchar *last_dir;
	
    /* Grab the current playlist to add songs to */
    fc_active_itdb = gp_get_active_itdb ();
    g_return_if_fail (fc_active_itdb);
	
    /* Create the file chooser, and handle the response */
    fc = gtk_file_chooser_dialog_new (_("Add Playlists"),
				      NULL, 
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL,
				      GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN,
				      GTK_RESPONSE_ACCEPT,
				      NULL);
	
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (fc), TRUE);
    last_dir = prefs_get_string ("last_dir_browsed");
    if (last_dir)
    {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fc),
					     last_dir);
	g_free (last_dir);
    }

    response = gtk_dialog_run(GTK_DIALOG(fc));
	
    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
	add_playlists_ok(GTK_FILE_CHOOSER(fc));
	break;
    case GTK_RESPONSE_CANCEL:
	break;
    default:	/* Fall through */
	break;
    }	
    gtk_widget_destroy(fc);
}




/* 
 * Add Cover Art
 */
gchar *fileselection_get_cover_filename (void)
{
    GtkWidget* fc;  /* The file chooser dialog */
    gint response;  /* The response of the filechooser */
    gchar *filename = NULL; /* The chosen file */
    gchar *last_dir, *new_dir;

    /* Create the file chooser, and handle the response */
    fc = gtk_file_chooser_dialog_new (_("Set Cover"),
				      NULL,
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL,
				      GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN,
				      GTK_RESPONSE_ACCEPT,
				      NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (fc), FALSE);

    last_dir = prefs_get_string ("last_dir_browsed");
    if (last_dir)
    {
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fc),
					     last_dir);
	g_free (last_dir);
    }

    response = gtk_dialog_run(GTK_DIALOG(fc));

    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
	new_dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (fc));
	prefs_set_string ("last_dir_browsed", new_dir);
	g_free (new_dir);
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fc));
	break;
    case GTK_RESPONSE_CANCEL:
	break;
    default:	/* Fall through */
	break;
    }		
    gtk_widget_destroy(fc);
    return filename;
}



/* Get a file or directory
 *
 * @title:    title for the file selection dialog
 * @cur_file: initial file to be selected. If NULL, then use
 *            last_dir_browse.
 * @action:
 *   GTK_FILE_CHOOSER_ACTION_OPEN   Indicates open mode. The file chooser
 *                                  will only let the user pick an
 *                                  existing file.
 *   GTK_FILE_CHOOSER_ACTION_SAVE   Indicates save mode. The file
 *                                  chooser will let the user pick an
 *                                  existing file, or type in a new
 *                                  filename.
 *   GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
 *                                  Indicates an Open mode for
 *                                  selecting folders. The file
 *                                  chooser will let the user pick an
 *                                  existing folder.
 *   GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
 *                                  Indicates a mode for creating a
 *                                  new folder. The file chooser will
 *                                  let the user name an existing or
 *                                  new folder.
 */
gchar *fileselection_get_file_or_dir (const gchar *title,
				      const gchar *cur_file,
				      GtkFileChooserAction action)
{
    GtkWidget* fc;  /* The file chooser dialog */
    gint response;  /* The response of the filechooser */
    gchar *new_file = NULL; /* The chosen file */

    g_return_val_if_fail (title, NULL);

    /* Create the file chooser, and handle the response */
    fc = gtk_file_chooser_dialog_new (
	title,
	NULL,
	action,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
	NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (fc), FALSE);

    if (cur_file)
    {
	/* Sanity checks: must exist and be absolute */
	if (g_path_is_absolute (cur_file))
	    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (fc),
					   cur_file);
	else
	    cur_file = NULL;
    }
    if (cur_file == NULL)
    {
	gchar *filename = prefs_get_string ("last_dir_browsed");
	if (filename)
	{
	    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fc),
						 filename);
	    g_free (filename);
	}
    }

    response = gtk_dialog_run(GTK_DIALOG(fc));

    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
	new_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fc));
	break;
    case GTK_RESPONSE_CANCEL:
	break;
    default:	/* Fall through */
	break;
    }		
    gtk_widget_destroy(fc);
    return new_file;
}



/* Used by the prefs system (prefs_windows.c, repository.c) when a
 * script should be selected. Takes into account that command line
 * arguments can be present
 *
 * @opath: the current path to the script including command line
 *         arguments. May be NULL.
 * @fallback: default dir in case @key is not set.
 * @title: title of the file selection window.
 * @additional_text: additional explanotary text to be displayed
 *
 * Return value: The new script including command line arguments. NULL
 * if the selection was aborted.
 */
gchar *fileselection_select_script (const gchar *opath,
				    const gchar *fallback,
				    const gchar *title,
				    const gchar *additional_text)
{
    gchar *npath=NULL;
    gchar *buf, *fbuf;
    const gchar *opathp;
    GtkFileChooser *fc;
    gint response;  /* The response of the filechooser */

    fc = GTK_FILE_CHOOSER (gtk_file_chooser_dialog_new (
			       title,
			       NULL,
			       GTK_FILE_CHOOSER_ACTION_OPEN,
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			       GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			       NULL));

    /* find first whitespace separating path from command line
     * arguments */

    if (opath)
	opathp = strchr (opath, ' ');
    else
	opathp = NULL;

    if (opathp)
	buf = g_strndup (opath, opathp-opath);
    else
	buf = g_strdup (opath);

    /* get full path -- if the file cannot be found it can't be
     * selected in the filechooser */
    fbuf = g_find_program_in_path (buf);
    g_free (buf);

    if (!fbuf)
    {   /* set default */
	fbuf = g_strdup (fallback);
    }

    if (fbuf && *fbuf)
    {
	gchar *fbuf_utf8 = g_filename_from_utf8 (fbuf, -1, NULL, NULL, NULL);
	if (g_file_test (fbuf, G_FILE_TEST_IS_DIR))
	{
	    gtk_file_chooser_set_current_folder (fc, fbuf_utf8);
	}
	else
	{
	    gtk_file_chooser_set_filename (fc, fbuf_utf8);
	}
	g_free (fbuf_utf8);
    }
    g_free (fbuf);

    response = gtk_dialog_run(GTK_DIALOG(fc));

    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
	buf = gtk_file_chooser_get_filename (fc);
	/* attach command line arguments if present */
	if (opathp)
	    npath = g_strdup_printf ("%s%s", buf, opathp);
	else
	    npath = g_strdup (buf);
	g_free (buf);
	break;
    case GTK_RESPONSE_CANCEL:
	break;
    default:	/* Fall through */
	break;
    }
    gtk_widget_destroy(GTK_WIDGET (fc));

    return npath;
}





/*
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to compile
|  "standalone" with the gtkpod project 2002/11/24
|  (http://gtkpod.sourceforge.net)
|  Modified for UTF8 string handling under GKT2 (Jan 2003).
|
|  last version before moving over to fileselection.c:
|  dirbrowser.c,v 1.30 2005/04/29 04:01:17 jcsjcs Exp
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


/* XPM */
static char *db_folder[] =
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
static char *db_ofolder[] =
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

static GdkPixmap *db_folder_pixmap = NULL, *db_ofolder_pixmap;
static GdkBitmap *db_folder_mask, *db_ofolder_mask;
static GtkWidget *dirbrowser = NULL;
static iTunesDB *db_active_itdb = NULL;


static GtkWidget *xmms_create_dir_browser(const gchar * title, const gchar * current_path, GtkSelectionMode mode, void (*handler) (gchar *));

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
void dirbrowser_block (void)
{
    if (dirbrowser)
	gtk_widget_set_sensitive (dirbrowser, FALSE);
}

/* turn the dirbrowser sensitive (if it's open) */
void dirbrowser_release (void)
{
    if (dirbrowser)
	gtk_widget_set_sensitive (dirbrowser, TRUE);
}

/* Callback after one directory has been added */
static void add_dir_selected (gchar *dir)
{
    g_return_if_fail (db_active_itdb);

    if (dir)
    {
	Playlist *plitem = pm_get_selected_playlist ();
	add_directory_by_name (db_active_itdb, dir, plitem,
			       prefs_get_add_recursively (),
			       NULL, NULL);
	prefs_set_string ("last_dir_browsed", dir);
	gtkpod_tracks_statusbar_update();
    }
    else
    {
	/* clear log of non-updated tracks */
	display_non_updated ((void *)-1, NULL);
	/* display log of updated tracks */
	display_updated (NULL, NULL);
	/* display log of detected duplicates */
	gp_duplicate_remove (NULL, NULL);
    }
}


/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function make sure you define a
   separate callback for gtkpod.glade */
void dirbrowser_create (void)
{
    gchar *cur_dir;

    if (dirbrowser)
    {   /* file selector already open -- raise to the top */
	gdk_window_raise (dirbrowser->window);
	return;
    }
    db_active_itdb = gp_get_active_itdb ();
    cur_dir = prefs_get_string ("last_dir_browsed");
    dirbrowser = xmms_create_dir_browser (
	_("Select directory to add recursively"),
	cur_dir,
	GTK_SELECTION_MULTIPLE,
	add_dir_selected);
    g_free (cur_dir);
    gtk_widget_show (dirbrowser);
}

/* called when dirbrowser gets destroyed with the window-close button */
static void dirbrowser_destroyed (GtkWidget *w, gpointer userdata)
{
    g_return_if_fail (dirbrowser);

    dirbrowser = NULL;
}


/* called when the file selector is closed */
static void add_dir_close (GtkWidget *w1, GtkWidget *w2)
{
    gint x,y;

    g_return_if_fail (dirbrowser);

    gtk_window_get_size (GTK_WINDOW (dirbrowser), &x, &y);
    /* store size for next time */
    prefs_set_size_dirbr (x, y);
    gtk_widget_destroy(dirbrowser);
    /* dirbrowser = NULL; -- will be done by the dirbrowser_destroy()
       as part of the callback */
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
					     NODE_SPACING, db_folder_pixmap,
					     db_folder_mask, db_ofolder_pixmap,
					     db_ofolder_mask, !has_subdir, FALSE);
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

static GtkWidget *xmms_create_dir_browser (const char *title,
					   const char *current_path,
					   GtkSelectionMode mode,
					   void (*handler) (char *))
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
    if (!db_folder_pixmap)
    {
	db_folder_pixmap = gdk_pixmap_create_from_xpm_d(window->window,
							&db_folder_mask,
							NULL, db_folder);
	db_ofolder_pixmap = gdk_pixmap_create_from_xpm_d(window->window,
							 &db_ofolder_mask,
							 NULL, db_ofolder);
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
				      db_folder_pixmap, db_folder_mask,
				      db_ofolder_pixmap, db_ofolder_mask,
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

    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       GTK_SIGNAL_FUNC(dirbrowser_destroyed),
		       NULL);

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
