/* Time-stamp: <2006-05-20 23:23:44 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include "charset.h"
#include "fileselection.h"
#include "info.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "prefs_window.h"


#define DEBUG_MISC 0

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


/* Retrieves a string (and option) from the user using a dialog.
   @title: title of the dialogue (may be NULL)
   @message: text (question) to be displayed (may be NULL)
   @dflt: default string to be returned (may be NULL)
   @opt_msg: message for the option checkbox (or NULL)
   @opt_state: original state of the checkbox. Will be updated
   return value: the string entered by the user or NULL if the dialog
   was cancelled. */
gchar *get_user_string (gchar *title, gchar *message, gchar *dflt,
			gchar *opt_msg, gboolean *opt_state)
{

    GtkWidget *dialog, *image, *label=NULL;
    GtkWidget *entry, *checkb=NULL, *hbox;
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

    /* create option checkbox */
    if (opt_msg && opt_state)
    {
	checkb = gtk_check_button_new_with_mnemonic (opt_msg);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkb),
				      *opt_state);
    }

    /* add to vbox */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			hbox, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			entry, FALSE, FALSE, 2);
    if (checkb)
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    checkb, FALSE, FALSE, 2);

    /* Start the dialogue */
    gtk_widget_show_all (dialog);
    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_OK)
    {
	result = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	/* get state of checkbox */
	if (checkb)
	{
	    *opt_state = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (checkb));
	}
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

    stn = prefs_get_int("sort_tab_num");
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

    if((w = gtkpod_xml_get_widget (main_window_xml,  name)))
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
	bws = add_blocked_widget (bws, "load_ipods_button");
	bws = add_blocked_widget (bws, "save_changes_button");
	bws = add_blocked_widget (bws, "add_files_button");
	bws = add_blocked_widget (bws, "add_dirs_button");
	bws = add_blocked_widget (bws, "add_PL_button");
	bws = add_blocked_widget (bws, "new_PL_button");
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
