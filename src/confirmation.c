/*
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "support.h"
#include "confirmation.h"
#include "prefs.h"
#include "interface.h"


static GHashTable *id_hash = NULL;

typedef struct {
    GtkWidget *window;
    gboolean  *scrolled;
    ConfHandlerNA never_again_handler;
    ConfHandler ok_handler;
    ConfHandler cancel_handler;
    gpointer user_data1;
    gpointer user_data2;
} ConfData;


static void on_ok_clicked (GtkWidget *w, gpointer id)
{
    ConfData *cd;
    gint defx, defy;

    cd = g_hash_table_lookup (id_hash, &id);
    if (cd)
    {
	if (cd->ok_handler)
	    cd->ok_handler (cd->user_data1, cd->user_data2);
	gtk_window_get_size (GTK_WINDOW (cd->window), &defx, &defy);
	if (cd->scrolled)
	    prefs_set_size_conf_sw (defx, defy);
	else
	    prefs_set_size_conf (defx, defy);
	gtk_widget_destroy (cd->window);
	g_hash_table_remove (id_hash, &id);
    }
}

static void on_cancel_clicked (GtkWidget *w, gpointer id)
{
    ConfData *cd;
    gint defx, defy;

    cd = g_hash_table_lookup (id_hash, &id);
    if (cd)
    {
	if (cd->cancel_handler)
	    cd->cancel_handler (cd->user_data1, cd->user_data2);
	gtk_window_get_size (GTK_WINDOW (cd->window), &defx, &defy);
	if (cd->scrolled)
	    prefs_set_size_conf_sw (defx, defy);
	else
	    prefs_set_size_conf (defx, defy);
	gtk_widget_destroy (cd->window);
	g_hash_table_remove (id_hash, &id);
    }
}

static void on_never_again_toggled (GtkToggleButton *t, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, &id);
    if (cd)
    {
	if (cd->never_again_handler)
	    cd->never_again_handler (gtk_toggle_button_get_active(t));
    }
}

/* gtkpod_confirmation(): open a confirmation window with the
   information given. If "OK" is clicked, ok_handler() is called,
   otherwise cancel_handler() is called, each with the parameters
   "user_data1" and "user_data2". Use "NULL" if you want to
   omit a parameter. If "never_again" is TRUE, ok_handler() is called
   directly.

   @id:    an ID: only one window with a given id can be open
   @modal: should the window be modal (i.e. block the program)?
   @title: title of the window
   @label: the text on the top of the window
   @text:  the text displayed in a scrolled window
   @never_again:    state of the "never ask again" flag
   @never_again_handler: callback for the checkbox (is called with the
                    current state of the toggle box)
   @ok_handler:     function to be called when the OK button is pressed
   @cancel_handler: function to be called when the cancel button is pressed
   @user_data1:     first argument to be passed to the ConfHandler
   @user_data1:     second argument to be passed to the ConfHandler

   return value:
   FALSE: no window was opened because another window with the same ID
          is already open
   TRUE:  either a window was opened, or ok_handler() was called
          directly. */
gboolean gtkpod_confirmation (gint id,
			      gboolean modal,
			      gchar *title,
			      gchar *label,
			      gchar *text,
			      gboolean never_again,
			      ConfHandlerNA never_again_handler,
			      ConfHandler ok_handler,
			      ConfHandler cancel_handler,
			      gpointer user_data1,
			      gpointer user_data2)
{
    GtkTextBuffer *tv = NULL;
    GtkWidget *window, *w;
    ConfData *cd;
    gint defx, defy;
    gint *idp;

    if (id_hash == NULL)
    {  /* initialize hash table to store IDs */
	id_hash = g_hash_table_new_full (g_int_hash, g_int_equal,
					      g_free, g_free);
    }
    if ((cd = g_hash_table_lookup (id_hash, &id)))
    { /* window with same ID already open -- return */
	return FALSE;
    }

    if (never_again)
    { /* This question was supposed to be asked "never again" -- so we
	 just call the ok_handler */
	ok_handler (user_data1, user_data2);
	return TRUE;
    }

    window = create_confirm_window ();

    gtk_window_set_modal (GTK_WINDOW (window), modal);
    /* insert ID into hash table */
    idp = g_malloc (sizeof (gint));
    *idp = id;
    cd = g_malloc (sizeof (ConfData));
    cd->window = window;
    cd->never_again_handler = never_again_handler;
    cd->ok_handler = ok_handler;
    cd->cancel_handler = cancel_handler;
    cd->user_data1 = user_data1;
    cd->user_data2 = user_data2;
    g_hash_table_insert (id_hash, idp, cd);

    /* Set title */
    if (title)
	gtk_window_set_title (GTK_WINDOW(window), title);
    else
	gtk_window_set_title (GTK_WINDOW(window), _("Confirmation Dialogue"));

    /* Set label */
    if (label && (w = lookup_widget (window, "label")))
	gtk_label_set_text(GTK_LABEL(w), label);

    /* Set text */
    w = lookup_widget (window, "text");
    if (w && text)
    {
	tv = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(tv, text, strlen(text));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(w), tv);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(w), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(w), FALSE);
	prefs_get_size_conf_sw (&defx, &defy);
    }
    else
    { /* no text -> hide widget */
	if ((w = lookup_widget (window, "scroller")))
	    gtk_widget_hide (w);
	prefs_get_size_conf_sw (&defx, &defy);
    }
    gtk_window_set_default_size (GTK_WINDOW (window), defx, defy);

    /* Set "Never Again" checkbox */
    w = lookup_widget(window, "never_again");
    if (w && never_again_handler)
    { /* connect signal */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				     never_again);
	g_signal_connect ((gpointer)w,
			  "toggled",
			  G_CALLBACK (on_never_again_toggled),
			  (gpointer)id);
    }
    else if (w)
    { /* hide "never again" button */
	gtk_widget_hide (w);
    }

    /* Connect OK handler */
    if ((w = lookup_widget (window, "ok")))
    {
	g_signal_connect ((gpointer)w, "clicked",
			  G_CALLBACK (on_ok_clicked),
			  (gpointer)id);
    }

    /* Hide "Apply" button (possible later extension) */
    if ((w = lookup_widget (window, "apply")))
    {
	gtk_widget_hide (w);
    }

    /* Connect Cancel handler */
    if ((w = lookup_widget (window, "cancel")))
    {
	g_signal_connect ((gpointer)w, "clicked",
			  G_CALLBACK (on_cancel_clicked),
			  (gpointer)id);
    }

    /* Connect Close window */
    g_signal_connect (GTK_OBJECT (window),
		      "delete_event",
		      G_CALLBACK (on_cancel_clicked), 
		      (gpointer) id);

    gtk_widget_show (window);

    return TRUE;
}
