/*
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

#include <string.h>
#include "misc.h"
#include "confirmation.h"
#include "prefs.h"


static GHashTable *id_hash = NULL;

typedef struct {
    GtkWidget *window;
    GladeXML *window_xml;
    gboolean  scrolled;
    gchar *option1_key;
    gboolean option1_invert;
    gchar *option2_key;
    gboolean option2_invert;
    gchar *confirm_again_key;
    ConfHandler ok_handler;
    ConfHandler apply_handler;
    ConfHandler cancel_handler;
    gpointer user_data1;
    gpointer user_data2;
} ConfData;



/* cleanup hash, store window size */
static void cleanup (gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	gint defx, defy;
	gtk_window_get_size (GTK_WINDOW (cd->window), &defx, &defy);
	if (cd->scrolled)
	{
		prefs_set_int("size_conf_sw.x", defx);
		prefs_set_int("size_conf_sw.y", defy);
	}
	else
	{
		prefs_set_int("size_conf.x", defx);
		prefs_set_int("size_conf.y", defy);
	}

	gtk_widget_destroy (cd->window);
	g_free (cd->option1_key);
	g_free (cd->option2_key);
	g_free (cd->confirm_again_key);

	g_hash_table_remove (id_hash, id);
    }
}


static void on_ok_clicked (GtkWidget *w, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	gtk_widget_set_sensitive (cd->window, FALSE);
	if (cd->ok_handler)
	    cd->ok_handler (cd->user_data1, cd->user_data2);
	cleanup (id);
    }
}

static void on_apply_clicked (GtkWidget *w, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	gtk_widget_set_sensitive (cd->window, FALSE);
	if (cd->apply_handler)
	    cd->apply_handler (cd->user_data1, cd->user_data2);
	gtk_widget_set_sensitive (cd->window, TRUE);
    }
}

static void on_cancel_clicked (GtkWidget *w, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	gtk_widget_set_sensitive (cd->window, FALSE);
	if (cd->cancel_handler)
	    cd->cancel_handler (cd->user_data1, cd->user_data2);
	cleanup (id);
    }
}


/* Handler to be used when the button should be displayed, but no
   action is required */
void CONF_NULL_HANDLER (gpointer d1, gpointer d2)
{
    return;
}


static void on_never_again_toggled (GtkToggleButton *t, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	if (cd->confirm_again_key)
	    prefs_set_int(cd->confirm_again_key, !gtk_toggle_button_get_active(t));
    }
}

static void on_option1_toggled (GtkToggleButton *t, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	if (cd->option1_key)
	{
	    gboolean state = gtk_toggle_button_get_active(t);
	    if (cd->option1_invert)  prefs_set_int (cd->option1_key, !state);
	    else                     prefs_set_int (cd->option1_key, state);
	}
    }
}

static void on_option2_toggled (GtkToggleButton *t, gpointer id)
{
    ConfData *cd;

    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	if (cd->option2_key)
	{
	    gboolean state = gtk_toggle_button_get_active(t);
	    if (cd->option2_invert)  prefs_set_int (cd->option2_key, !state);
	    else                     prefs_set_int (cd->option2_key, state);
	}
    }
}

static void on_response (GtkWidget *w, gint response, gpointer id)
{
    ConfData *cd;
/*     printf ("r: %d, i: %d\n", response, id); */
    cd = g_hash_table_lookup (id_hash, id);
    if (cd)
    {
	switch (response)
	{
	case GTK_RESPONSE_OK:
	    on_ok_clicked (w, id);
	    break;
	case GTK_RESPONSE_NONE:
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
	    on_cancel_clicked (w, id);
	    break;
	case GTK_RESPONSE_APPLY:
	    on_apply_clicked (w, id);
	    break;
	default:
	    g_warning ("Programming error: resonse '%d' received in on_response()\n", response);
	    on_cancel_clicked (w, id);
	    break;
	}
    }
}

static void confirm_append_text (GladeXML *xml, const gchar *text)
{
	int i;
	gchar **strings = g_strsplit (text, "\n", 0);
	GtkTreeIter iter;
	GtkAdjustment *adjustment;
	GtkWidget *w = gtkpod_xml_get_widget (xml, "tree");
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (w)));

	for (i = 0; strings[i]; i++)
	{
		if (strings[i][0])
		{
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, strings[i], -1);
		}
	}

	w = gtkpod_xml_get_widget (xml, "scroller");
	adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (w));
	gtk_adjustment_set_value (adjustment, adjustment->upper - adjustment->page_size);

	g_strfreev (strings);
}

/* gtkpod_confirmation(): open a confirmation window with the
   information given. If "OK" is clicked, ok_handler() is called,
   otherwise cancel_handler() is called, each with the parameters
   "user_data1" and "user_data2". Use "NULL" if you want to
   omit a parameter. If "confirm_again" is FALSE, ok_handler() is called
   directly.

   @id:    an ID: only one window with a given positive id can be
           open. Use negative ID if you don't care how many windows
	   are open at the same time (e.g. because they are modal).
	   With positive IDs @text is added to an already open window
           with the same ID.
   @modal: should the window be modal (i.e. block the program)?
   @title: title of the window
   @label: the text at the top of the window
   @text:  the text displayed in a scrolled window
   @option_text: text for the option checkbox (or NULL)
   @option_state: initial state of the option + a flag indicating
           whether the handler should be called with the inverse state
           of the toggle button: CONF_STATE_TRUE, CONF_STATE_FALSE,
	   CONF_STATE_INVERT_TRUE, CONF_STATE_INVERT_FALSE
   @option_key: prefs key for the option (is set to the state of the
           toggle box)
   @confirm_again:    state of the "confirm again" flag
   @confirm_again_key: prefs key for the 'never show this dialog
           again' toggle box (is set to the inverted current state
	   of the toggle box)
   @ok_handler:     function to be called when the OK button is pressed
   @apply_handler:  function to be called when the Apply button is pressed
   @cancel_handler: function to be called when the cancel button is pressed
	   Note: in modal windows, the ok_, apply_, and cancel_handlers
	   must be set to CONF_NULL_HANDLER if the button is to be shown.
   @user_data1:     first argument to be passed to the ConfHandler
   @user_data1:     second argument to be passed to the ConfHandler

   Pass NULL as "handler" if you want the corresponding button to be
   hidden.

   Pass CONF_NULL_HANDLER if you want the corresponding button to be
   shown, but don't want to specify a handler.

   Return value:

   non-modal dialogs:

     GTK_RESPONSE_REJECT: no window was opened because another window
     with the same ID is already open. Text was appended.

     GTK_RESPONSE_ACCEPT: either a window was opened, or ok_handler()
     was called directly.

   modal dialogs:

     GTK_RESPONSE_REJECT: no window was opened because another window
     with the same ID is already open. Text was appended.

     GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, GTK_RESPONSE_APPLY:
     OK/CANCEL/APPLY pressed. If the window is closed by the user,
     GTK_RESPONSE_CANCEL will be returned.
 */

GtkResponseType gtkpod_confirmation (gint id,
				     gboolean modal,
				     const gchar *title,
				     const gchar *label,
				     const gchar *text,
				     const gchar *option1_text,
				     CONF_STATE option1_state,
				     const gchar *option1_key,
				     const gchar *option2_text,
				     CONF_STATE option2_state,
				     const gchar *option2_key,
				     gboolean confirm_again,
				     const gchar *confirm_again_key,
				     ConfHandler ok_handler,
				     ConfHandler apply_handler,
				     ConfHandler cancel_handler,
				     gpointer user_data1,
				     gpointer user_data2)
{
    GtkWidget *window, *w;
    ConfData *cd;
    gint defx, defy;
    GladeXML *confirm_xml;
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    gchar *full_label;

    if (id_hash == NULL)
    {  /* initialize hash table to store IDs */
	id_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal,
							  NULL, g_free);
    }

    if (id >= 0)
    {
	if ((cd = g_hash_table_lookup (id_hash, GINT_TO_POINTER(id))))
	{ /* window with same ID already open -- add @text and return */
	    if (text && *text && cd->window)
	    {
		confirm_append_text (cd->window_xml, text);
	    }
	    return GTK_RESPONSE_REJECT;
	}
    }
    else /* find free ID */
    {
	id = 0;

	do
	{
	    --id;
	    cd = g_hash_table_lookup (id_hash, GINT_TO_POINTER(id));
	} while (cd != NULL);
    }

    if (!confirm_again)
    {
	/* This question was supposed to be asked "never again" ("don't
	   confirm again" -- so we just call the ok_handler */
	if (ok_handler && !modal)
	    ok_handler (user_data1, user_data2);

	if (!modal)
	    return GTK_RESPONSE_ACCEPT;

	return GTK_RESPONSE_OK;
    }

    /* window = create_confirm_dialog (); */
    confirm_xml = gtkpod_xml_new (xml_file, "confirm_dialog");
    window = gtkpod_xml_get_widget (confirm_xml, "confirm_dialog");
    glade_xml_signal_autoconnect (confirm_xml);

    /* insert ID into hash table */
    cd = g_new0 (ConfData, 1);
    cd->window = window;
    cd->window_xml = confirm_xml;
    cd->option1_key = g_strdup(option1_key);
    cd->option2_key = g_strdup(option2_key);
    cd->confirm_again_key = g_strdup(confirm_again_key);
    cd->ok_handler = ok_handler;
    cd->apply_handler = apply_handler;
    cd->cancel_handler = cancel_handler;
    cd->user_data1 = user_data1;
    cd->user_data2 = user_data2;
    g_hash_table_insert (id_hash, GINT_TO_POINTER(id), cd);

    full_label = g_markup_printf_escaped ("<span weight='bold' size='larger'>%s</span>\n\n%s",
					  title ? title : _("Confirmation"), label ? label : "");

    /* Set label */
    w = gtkpod_xml_get_widget (confirm_xml, "label");
    gtk_label_set_markup (GTK_LABEL(w), full_label);
    g_free (full_label);

    /* Set text */
    w = gtkpod_xml_get_widget (confirm_xml, "tree");
    store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w), GTK_TREE_MODEL (store));
    g_object_unref (store);

    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_text_new ();

/*
    gint mode, width;

    g_object_get (G_OBJECT (renderer),
		  "wrap-mode", &mode,
		  "wrap-width", &width,
		  NULL);

    printf ("wrap-mode: %d wrap-width:%d\n", mode, width);
*/

    /* I have no idea why 400, but neither 1000 nor 0 result in the
       right behavior... */
    g_object_set (G_OBJECT (renderer),
		  "wrap-mode", PANGO_WRAP_WORD_CHAR,
		  "wrap-width", 400,
		  NULL);

    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 0, NULL);
    g_object_set_data (G_OBJECT (w), "renderer", renderer);

    gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);

    if (text)
    {
		confirm_append_text (cd->window_xml, text);
		cd->scrolled = TRUE;
		defx = prefs_get_int("size_conf_sw.x");
		defy = prefs_get_int("size_conf_sw.y");
    }
    else
    {
		/* no text -> hide widget */
		if ((w = gtkpod_xml_get_widget (confirm_xml, "scroller")))
			gtk_widget_hide (w);

		cd->scrolled = FALSE;
		defx = prefs_get_int("size_conf.x");
		defy = prefs_get_int("size_conf.y");
    }

    gtk_widget_set_size_request (GTK_WIDGET (window), defx, defy);

    /* Set "Option 1" checkbox */
    w = gtkpod_xml_get_widget (confirm_xml, "option_vbox");

    if (w && option1_key && option1_text)
    {
	gboolean state, invert;
	GtkWidget *option1_button =
	    gtk_check_button_new_with_mnemonic (option1_text);

	state = ((option1_state==CONF_STATE_INVERT_TRUE) || (option1_state==CONF_STATE_TRUE));
	invert = ((option1_state==CONF_STATE_INVERT_FALSE) || (option1_state==CONF_STATE_INVERT_TRUE));
	cd->option1_invert = invert;

	gtk_widget_show (option1_button);
	gtk_box_pack_start (GTK_BOX (w), option1_button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(option1_button),
				     state);
	g_signal_connect ((gpointer)option1_button,
			  "toggled",
			  G_CALLBACK (on_option1_toggled),
			  GINT_TO_POINTER(id));
    }

    /* Set "Option 2" checkbox */
    w = gtkpod_xml_get_widget (confirm_xml, "option_vbox");
    if (w && option2_key && option2_text)
    {
	gboolean state, invert;
	GtkWidget *option2_button =
	    gtk_check_button_new_with_mnemonic (option2_text);

	state = ((option2_state==CONF_STATE_INVERT_TRUE) || (option2_state==CONF_STATE_TRUE));
	invert = ((option2_state==CONF_STATE_INVERT_FALSE) || (option2_state==CONF_STATE_INVERT_TRUE));
	cd->option2_invert = invert;

	gtk_widget_show (option2_button);
	gtk_box_pack_start (GTK_BOX (w), option2_button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(option2_button),
				     state);
	g_signal_connect ((gpointer)option2_button,
			  "toggled",
			  G_CALLBACK (on_option2_toggled),
			  GINT_TO_POINTER(id));
    }

    /* Set "Never Again" checkbox */
    w = gtkpod_xml_get_widget (confirm_xml, "never_again");

    if (w && confirm_again_key)
    {
	/* connect signal */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				     !confirm_again);
	g_signal_connect ((gpointer)w,
			  "toggled",
			  G_CALLBACK (on_never_again_toggled),
			  GINT_TO_POINTER(id));
    }
    else if (w)
    {
	/* hide "never again" button */
	gtk_widget_hide (w);
    }

    /* Hide and set "default" button that can be activated by pressing
       ENTER in the window (usually OK)*/
    /* Hide or default CANCEL button */
    if ((w = gtkpod_xml_get_widget (confirm_xml, "cancel")))
    {
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (w);

	if (!cancel_handler)
	    gtk_widget_hide (w);
    }

    /* Hide or default APPLY button */
    if ((w = gtkpod_xml_get_widget (confirm_xml, "apply")))
    {
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (w);

	if (!apply_handler)
	    gtk_widget_hide (w);
    }

    /* Hide or default OK button */
    if ((w = gtkpod_xml_get_widget (confirm_xml, "ok")))
    {
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (w);

	if (!ok_handler)
	    gtk_widget_hide (w);
    }

    /* Connect Close window */
    g_signal_connect (GTK_OBJECT (window),
		      "delete_event",
		      G_CALLBACK (on_cancel_clicked),
		      GINT_TO_POINTER(id));

    if (modal)
    {
	/* use gtk_dialog_run() to block the application */
	gint response = gtk_dialog_run (GTK_DIALOG (window));
	/* cleanup hash, store window size */
	cleanup (GINT_TO_POINTER(id));

	switch (response)
	{
	case GTK_RESPONSE_OK:
	case GTK_RESPONSE_APPLY:
	    return response;
	default:
	    return GTK_RESPONSE_CANCEL;
	}
    }
    else
    {
	/* Make sure we catch the response */
	g_signal_connect (GTK_OBJECT (window),
			  "response",
			  G_CALLBACK (on_response),
			  GINT_TO_POINTER(id));
	gtk_widget_show (window);

	return GTK_RESPONSE_ACCEPT;
    }
}

G_MODULE_EXPORT void on_confirm_tree_size_allocate (GtkWidget *sender, GtkAllocation *allocation, gpointer e)
{
    GtkCellRenderer *renderer = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (sender), "renderer"));
    g_object_set (renderer, "wrap-width", allocation->width, NULL);
}

gint gtkpod_confirmation_simple (GtkWindow *parent,
				 GtkMessageType icon,
				 const gchar *primary_text,
				 const gchar *secondary_text,
				 const gchar *accept_button_text)
{
    return gtkpod_confirmation_hig(parent, icon, primary_text, secondary_text,
				   accept_button_text, NULL, NULL, NULL);
}

gint gtkpod_confirmation_hig (GtkWindow *parent,
			      GtkMessageType icon,
			      const gchar *primary_text,
			      const gchar *secondary_text,
			      const gchar *accept_button_text,
			      const gchar *cancel_button_text,
			      const gchar *third_button_text,
			      const gchar *help_context)
{
    gint result;

    GtkWidget *dialog =
	gtk_message_dialog_new(parent,
			       GTK_DIALOG_MODAL,
			       icon,
			       GTK_BUTTONS_NONE,
			       "%s",
			       primary_text);

    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG(dialog),
						"%s",
						secondary_text);

    if(third_button_text)
	gtk_dialog_add_button(GTK_DIALOG(dialog), third_button_text, GTK_RESPONSE_APPLY);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			   cancel_button_text ? cancel_button_text : GTK_STOCK_CANCEL,
			   GTK_RESPONSE_CANCEL,
			   accept_button_text ? accept_button_text : GTK_STOCK_OK,
			   GTK_RESPONSE_OK,
			   NULL);

    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch (result)
    {
    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_APPLY:
	return result;
    default:
	return GTK_RESPONSE_CANCEL;
    };
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *                       gtkpod_warning                             *
 *                                                                  *
\*------------------------------------------------------------------*/

/* gtkpod_warning(): will pop up a window and display text as a
 * warning. If a warning window is already open, the text will be
 * added to the existing window. */
/* parameters: same as printf */
void gtkpod_warning (const gchar *format, ...)
{
    va_list arg;
    gchar *text;

    g_warning("TODO looking to replace warning dialogs with anjuta_util_*_dialog functions\n");
    va_start (arg, format);
    text = g_strdup_vprintf (format, arg);
    va_end (arg);

    gtkpod_confirmation (CONF_ID_GTKPOD_WARNING,    /* gint id, */
             FALSE,                     /* gboolean modal, */
             _("Warning"),              /* title */
             _("The following has occurred:"),
             text,                /* text to be displayed */
             NULL, 0, NULL,       /* option 1 */
             NULL, 0, NULL,       /* option 2 */
             TRUE,                /* gboolean confirm_again, */
             NULL, /* ConfHandlerOpt confirm_again_key, */
             CONF_NULL_HANDLER,   /* ConfHandler ok_handler,*/
             NULL,                /* don't show "Apply" */
             NULL,                /* cancel_handler,*/
             NULL,                /* gpointer user_data1,*/
             NULL);               /* gpointer user_data2,*/
    g_free (text);
}

void gtkpod_warning_simple (GtkWindow *parent, const gchar *format, ...)
{
    va_list arg;
    gchar *text;

    va_start (arg, format);
    text = g_strdup_vprintf (format, arg);
    va_end (arg);

    gtkpod_warning_hig (parent, GTK_MESSAGE_WARNING, _("Warning"), text);
    g_free (text);
}

void gtkpod_warning_hig (GtkWindow *parent,
                         GtkMessageType icon,
                         const gchar *primary_text,
                         const gchar *secondary_text)
{
    GtkWidget *dialog = gtk_message_dialog_new(parent,
                               GTK_DIALOG_MODAL,
                               icon,
                               GTK_BUTTONS_OK,
                               "%s",
                               primary_text);

    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG(dialog),
                            "%s",
                            secondary_text);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

}
