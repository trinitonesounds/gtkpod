/* Time-stamp: <2004-03-24 23:16:26 JST jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include "charset.h"
#include "dirbrowser.h"
#include "misc.h"
#include "prefs.h"
#include "prefs_window.h"
#include "support.h"


#define DEBUG_MISC 0

static GtkWidget *file_selector = NULL;
static GtkWidget *pl_file_selector = NULL;


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Add Files File Selector                              *
 *                                                                  *
\*------------------------------------------------------------------*/

static void add_files_ok_button (GtkWidget *button, GtkFileSelection *selector)
{
  gchar **names;
  gint i;
  Playlist *plitem;

  block_widgets ();
  names = gtk_file_selection_get_selections (GTK_FILE_SELECTION (selector));
  plitem = pm_get_selected_playlist ();
  for (i=0; names[i] != NULL; ++i)
  {
      add_track_by_filename (names[i], plitem,
			    prefs_get_add_recursively (),
			    NULL, NULL);
      if(i == 0)
	  prefs_set_last_dir_browse(names[i]);
  }
  /* clear log of non-updated tracks */
  display_non_updated ((void *)-1, NULL);
  /* display log of updated tracks */
  display_updated (NULL, NULL);
  /* display log of detected duplicates */
  remove_duplicate (NULL, NULL);
  gtkpod_statusbar_message(_("Successly Added Files"));
  gtkpod_tracks_statusbar_update();
  release_widgets ();
  g_strfreev (names);
}

/* called when the file selector is closed */
static void add_files_close (GtkWidget *w1, GtkWidget *w2)
{
    if (file_selector)    gtk_widget_destroy(file_selector),
    gtkpod_tracks_statusbar_update();
    file_selector = NULL;
}


void create_add_files_fileselector (void)
{
    if (file_selector) return; /* file selector already open -- abort */
    /* Create the selector */
    file_selector = gtk_file_selection_new (_("Select files or directories to add."));
    gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (file_selector),
					    TRUE);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION (file_selector),
				    prefs_get_last_dir_browse ());

    g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
		      "clicked",
		      G_CALLBACK (add_files_ok_button),
		      file_selector);

    /* Ensure that file_selector is set to NULL when window is deleted */
    g_signal_connect_swapped (GTK_OBJECT (file_selector),
			      "delete_event",
			      G_CALLBACK (add_files_close),
			      (gpointer) file_selector);

    /* Ensure that the dialog box is deleted when the user clicks a button. */
    g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
			      "clicked",
			      G_CALLBACK (add_files_close),
			      (gpointer) file_selector);

    g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
			      "clicked",
			      G_CALLBACK (add_files_close),
			      (gpointer) file_selector);

    /* Display that dialog */
    gtk_widget_show (file_selector);
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Add Playlists File Selector                          *
 *                                                                  *
\*------------------------------------------------------------------*/


static void add_playlists_ok_button (GtkWidget *button, GtkFileSelection *selector)
{
  gchar **names;
  gint i;

  block_widgets ();
  names = gtk_file_selection_get_selections (GTK_FILE_SELECTION (selector));
  for (i=0; names[i] != NULL; ++i)
    {
      add_playlist_by_filename (names[i], NULL, NULL, NULL);
      if(i == 0)
	  prefs_set_last_dir_browse(names[i]);
    }
  gtkpod_tracks_statusbar_update();
  release_widgets ();
  g_strfreev (names);
}

/* called when the file selector is closed */
static void add_playlists_close (GtkWidget *w1, GtkWidget *w2)
{
    if (pl_file_selector)    gtk_widget_destroy(pl_file_selector),
    gtkpod_tracks_statusbar_update();
    pl_file_selector = NULL;
}


void create_add_playlists_fileselector (void)
{
    if (pl_file_selector) return; /* file selector already open -- abort */
    /* Create the selector */
    pl_file_selector = gtk_file_selection_new (_("Select Playlists to add."));
    gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (pl_file_selector),
					    TRUE);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION (pl_file_selector),
				    prefs_get_last_dir_browse ());

    g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (pl_file_selector)->ok_button),
		      "clicked",
		      G_CALLBACK (add_playlists_ok_button),
		      pl_file_selector);

    /* Ensure that pl_file_selector is set to NULL when window is deleted */
    g_signal_connect_swapped (GTK_OBJECT (pl_file_selector),
			      "delete_event",
			      G_CALLBACK (add_playlists_close),
			      (gpointer) pl_file_selector);

    /* Ensure that the dialog box is deleted when the user clicks a button. */
    g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (pl_file_selector)->ok_button),
			      "clicked",
			      G_CALLBACK (add_playlists_close),
			      (gpointer) pl_file_selector);

    g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (pl_file_selector)->cancel_button),
			      "clicked",
			      G_CALLBACK (add_playlists_close),
			      (gpointer) pl_file_selector);

    /* Display that dialog */
    gtk_widget_show (pl_file_selector);
}



/* turn the file selector insensitive (if it's open) */
static void file_selector_block (void)
{
    if (file_selector)
	gtk_widget_set_sensitive (file_selector, FALSE);
    if (pl_file_selector)
	gtk_widget_set_sensitive (pl_file_selector, FALSE);
}

/* turn the file selector sensitive (if it's open) */
static void file_selector_release (void)
{
    if (file_selector)
	gtk_widget_set_sensitive (file_selector, TRUE);
    if (pl_file_selector)
	gtk_widget_set_sensitive (pl_file_selector, TRUE);
}



/* Concats @base_dir and @rel_dir if and only if @rel_dir is not
 * absolute (does not start with '~' or '/'). Otherwise simply return
 * a copy of @rel_dir. Must free return value after use */
gchar *concat_dir_if_relative (G_CONST_RETURN gchar *base_dir,
			       G_CONST_RETURN gchar *rel_dir)
{
    /* sanity */
    if (!rel_dir || !*rel_dir)
	return g_build_filename (base_dir, rel_dir, NULL);
				 /* this constellation is nonsense... */
    if ((*rel_dir == '/') || (*rel_dir == '~'))
	return g_strdup (rel_dir);             /* rel_dir is absolute */
					       /* make absolute path */
    return g_build_filename (base_dir, rel_dir, NULL);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Ask for User Input (String, SortTab Nr.)             *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Retrieves a string from the user using a dialog.
   @title: title of the dialogue (may be NULL)
   @message: text (question) to be displayed (may be NULL)
   @dflt: default string to be returned (may be NULL)
   return value: the string entered by the user or NULL if the dialog
   was cancelled. */
gchar *get_user_string (gchar *title, gchar *message, gchar *dflt)
{

    GtkWidget *dialog, *image, *label=NULL, *entry, *hbox;
    gint response;
    gchar *result = NULL;

    /* create the dialog window */
    dialog = gtk_dialog_new_with_buttons (
	title,
	GTK_WINDOW (gtkpod_window),
	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_STOCK_OK, GTK_RESPONSE_OK,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

    /* emulate gtk_message_dialog_new */
    image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
				    GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

    if (message)
    {
	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    }
    /* hbox to put the image+label in */
    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    if (label) gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    /* Create entry */
    entry = gtk_entry_new ();
    if (dflt)
    {
	gtk_entry_set_text (GTK_ENTRY (entry), dflt);
	gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
    }
    /* Pressing enter should activate the default response (default
       response set above */
    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    /* add to vbox */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			hbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			entry, FALSE, FALSE, 0);

    /* Start the dialogue */
    gtk_widget_show_all (dialog);
    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_OK)
    {
	result = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
    }
    gtk_widget_destroy (dialog);
    return result;
}



/* Let the user select a sort tab number */
/* @text: text to be displayed */
/* return value: -1: user selected cancel
   0...prefs_get_sort_tab_number()-1: selected tab */
gint get_sort_tab_number (gchar *text)
{
    static gint last_nr = 1;
    GtkWidget *mdialog;
    GtkDialog *dialog;
    GtkWidget *combo;
    gint result;
    gint i, nr, stn;
    GList *list=NULL, *lnk;
    gchar buf[20], *bufp;

    mdialog = gtk_message_dialog_new (
	GTK_WINDOW (gtkpod_window),
	GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_MESSAGE_QUESTION,
	GTK_BUTTONS_OK_CANCEL,
	text);

    dialog = GTK_DIALOG (mdialog);

    combo = gtk_combo_new ();
    gtk_widget_show (combo);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), combo);

    stn = prefs_get_sort_tab_num ();
    /* Create list */
    for (i=1; i<=stn; ++i)
    {
	bufp = g_strdup_printf ("%d", i);
	list = g_list_append (list, bufp);
    }

    /* set pull down items */
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);
    /* set standard entry */
    if (last_nr > stn) last_nr = 1;  /* maybe the stn has become
					smaller... */
    snprintf (buf, 20, "%d", last_nr);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), buf);

    result = gtk_dialog_run (GTK_DIALOG (mdialog));

    /* free the list */
    for (lnk = list; lnk; lnk = lnk->next)
    {
	C_FREE (lnk->data);
    }
    g_list_free (list);
    list = NULL;

    if (result == GTK_RESPONSE_CANCEL)
    {
	nr = -1;  /* no selection */
    }
    else
    {
	bufp = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (combo)->entry),
				      0, -1);
	nr = atoi (bufp)-1;
	last_nr = nr+1;
	C_FREE (bufp);
    }

    gtk_widget_destroy (mdialog);

    return nr;
}




/*------------------------------------------------------------------*\
 *                                                                  *
 *             Functions for blocking widgets (block input)         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* --------------------------------------------------------------*/
/* are widgets blocked at the moment? */
gboolean widgets_blocked = FALSE;
struct blocked_widget { /* struct to be kept in blocked_widgets */
    GtkWidget *widget;   /* widget that has been turned insensitive */
    gboolean  sensitive; /* state of the widget before */
};
/* --------------------------------------------------------------*/


enum {
    BR_BLOCK,
    BR_RELEASE,
    BR_UPDATE
};

/* function to add one widget to the blocked_widgets list */
static GList *add_blocked_widget (GList *blocked_widgets, gchar *name)
{
    GtkWidget *w;
    struct blocked_widget *bw;

    if((w = lookup_widget(gtkpod_window,  name)))
    {
	bw = g_malloc0 (sizeof (struct blocked_widget));
	bw->widget = w;
	/* we don't have to set the sensitive flag right now. It's
	 * done in "block_widgets ()" */
	blocked_widgets = g_list_append (blocked_widgets, bw);
    }
    return blocked_widgets;
}

/* called by block_widgets() and release_widgets() */
/* "block": TRUE = block, FALSE = release */
static void block_release_widgets (gint action, GtkWidget *w, gboolean sens)
{
    /* list with the widgets that are turned insensitive during
       import/export...*/
    static GList *bws = NULL;
    static gint count = 0; /* how many times are the widgets blocked? */
    GList *l;
    struct blocked_widget *bw;

    /* Create a list of widgets that are to be turned insensitive when
     * importing/exporting, adding tracks or directories etc. */
    if (bws == NULL)
    {
	bws = add_blocked_widget (bws, "menubar");
	bws = add_blocked_widget (bws, "import_button");
	bws = add_blocked_widget (bws, "add_files_button");
	bws = add_blocked_widget (bws, "add_dirs_button");
	bws = add_blocked_widget (bws, "add_PL_button");
	bws = add_blocked_widget (bws, "new_PL_button");
	bws = add_blocked_widget (bws, "export_button");
	widgets_blocked = FALSE;
    }

    switch (action)
    {
    case BR_BLOCK:
	/* we must block the widgets */
	++count;  /* increase number of locks */
	if (!widgets_blocked)
	{ /* only block widgets, if they are not already blocked */
	    for (l = bws; l; l = l->next)
	    {
		bw = (struct blocked_widget *)l->data;
		/* remember the state the widget was in before */
		bw->sensitive = GTK_WIDGET_SENSITIVE (bw->widget);
		gtk_widget_set_sensitive (bw->widget, FALSE);
	    }
	    sort_window_block ();
	    prefs_window_block ();
	    file_selector_block ();
	    dirbrowser_block ();
	    widgets_blocked = TRUE;
	}
	break;
    case BR_RELEASE:
	/* release the widgets if --count == 0 */
	if (widgets_blocked)
	{ /* only release widgets, if they are blocked */
	    --count;
	    if (count == 0)
	    {
		for (l = bws; l; l = l->next)
		{
		    bw = (struct blocked_widget *)l->data;
		    gtk_widget_set_sensitive (bw->widget, bw->sensitive);
		}
		sort_window_release ();
		prefs_window_release ();
		file_selector_release ();
		dirbrowser_release ();
		widgets_blocked = FALSE;
	    }
	}
	break;
    case BR_UPDATE:
	if (widgets_blocked)
	{ /* only update widgets, if they are blocked */
	    for (l = bws; l; l = l->next)
	    { /* find the required widget */
		bw = (struct blocked_widget *)l->data;
		if (bw->widget == w)
		{ /* found -> set to new desired state */
		    bw->sensitive = sens;
		    break;
		}
	    }
	}
	break;
    }
}


/* Block widgets (turn insensitive) listed in "bws" */
void block_widgets (void)
{
    block_release_widgets (BR_BLOCK, NULL, FALSE);
}

/* Release widgets (i.e. return them to their state before
   "block_widgets() was called */
void release_widgets (void)
{
    block_release_widgets (BR_RELEASE, NULL, FALSE);
}

void update_blocked_widget (GtkWidget *w, gboolean sens)
{
    block_release_widgets (BR_UPDATE, w, sens);
}

