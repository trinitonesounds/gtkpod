/* Time-stamp: <2003-06-26 23:08:20 jcs>
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <math.h>
#include "song.h"
#include "interface.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"
#include "itunesdb.h"
#include "display.h"
#include "confirmation.h"
#include "prefs_window.h"
#include "dirbrowser.h"
#include "file.h"
#include "charset.h"


#define DEBUG_MISC 0

GtkWidget *gtkpod_window = NULL;
static GtkWidget *about_window = NULL;
static GtkWidget *file_selector = NULL;
static GtkWidget *pl_file_selector = NULL;
static GtkWidget *gtkpod_statusbar = NULL;
static GtkWidget *gtkpod_songs_statusbar = NULL;
static GtkWidget *gtkpod_space_statusbar = NULL;
static guint statusbar_timeout_id = 0;

static glong get_ipod_free_space(void);
#if 0
static glong get_ipod_used_space(void);
#endif
/* --------------------------------------------------------------*/
/* are widgets blocked at the moment? */
gboolean widgets_blocked = FALSE;
struct blocked_widget { /* struct to be kept in blocked_widgets */
    GtkWidget *widget;   /* widget that has been turned insensitive */
    gboolean  sensitive; /* state of the widget before */
};
/* --------------------------------------------------------------*/

/* turn the file selector insensitive (if it's open) */
static void block_file_selector (void)
{
    if (file_selector)
	gtk_widget_set_sensitive (file_selector, FALSE);
    if (pl_file_selector)
	gtk_widget_set_sensitive (pl_file_selector, FALSE);
}

/* turn the file selector sensitive (if it's open) */
static void release_file_selector (void)
{
    if (file_selector)
	gtk_widget_set_sensitive (file_selector, TRUE);
    if (pl_file_selector)
	gtk_widget_set_sensitive (pl_file_selector, TRUE);
}

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
      add_song_by_filename (names[i], plitem,
			    prefs_get_add_recursively (),
			    NULL, NULL);
      if(i == 0)
	  prefs_set_last_dir_browse(names[i]);
  }
  /* clear log of non-updated songs */
  display_non_updated ((void *)-1, NULL);
  /* display log of updated songs */
  display_updated (NULL, NULL);
  /* display log of detected duplicates */
  remove_duplicate (NULL, NULL);
  gtkpod_statusbar_message(_("Successly Added Files"));
  gtkpod_songs_statusbar_update();
  release_widgets ();
  g_strfreev (names);
}

/* called when the file selector is closed */
static void add_files_close (GtkWidget *w1, GtkWidget *w2)
{
    if (file_selector)    gtk_widget_destroy(file_selector),
    gtkpod_songs_statusbar_update();
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
				    cfg->last_dir.browse);

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
  gtkpod_songs_statusbar_update();
  release_widgets ();
  g_strfreev (names);
}

/* called when the file selector is closed */
static void add_playlists_close (GtkWidget *w1, GtkWidget *w2)
{
    if (pl_file_selector)    gtk_widget_destroy(pl_file_selector),
    gtkpod_songs_statusbar_update();
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
				    cfg->last_dir.browse);

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



/* Concats "dir" and "file" into full filename, taking
   into account that "dir" may or may not end with "/".
   You must free the return string after use
   This code tries to take into account some stupid constellations
   when either "dir" or "file" is not set, or file starts with a "/"
   (ignore) etc.  */
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file)
{
    return itunesdb_concat_dir (dir, file);
}

/* Calculate the time in ms passed since @old_time. @old_time is
   updated with the current time if @update is TRUE*/
float get_ms_since (GTimeVal *old_time, gboolean update)
{
    GTimeVal new_time;
    float result;

    g_get_current_time (&new_time);
    result = (new_time.tv_sec - old_time->tv_sec) * 1000 +
	(float)(new_time.tv_usec - old_time->tv_usec) / 1000;
    if (update)
    {
	old_time->tv_sec = new_time.tv_sec;
	old_time->tv_usec = new_time.tv_usec;
    }
    return result;
}



void open_about_window ()
{
  GtkLabel *about_label;
  gchar *label_text;
  GtkTextView *textview;
  GtkTextIter ti;
  GtkTextBuffer *tb;

  if (about_window != NULL) return;
  about_window = create_gtkpod_about_window ();
  about_label = GTK_LABEL (lookup_widget (about_window, "about_label"));
  label_text = g_strdup_printf (_("gtkpod Version %s: Cross-Platform Multi-Lingual Interface to Apple's iPod(tm)."), VERSION);
  gtk_label_set_text (about_label, label_text);
  g_free (label_text);
  {
      gchar *text[] = {_("\
(C) 2002 - 2003\n\
Jorg Schuler (jcsjcs at users dot sourceforge dot net)\n\
Corey Donohoe (atmos at atmos dot org)\n\
\n\
\n"),
		       _("\
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.\n\
\n\
\n"),
		       _("\
Patches were supplied by the following people (list may be incomplete -- please contact me)\n\
\n\
Ramesh Dharan: Multi-Edit (edit tags of several songs in one run)\n\
Hiroshi Kawashima: Japanese charset autodetecion feature\n\
Adrian Ulrich: porting of playlist code from mktunes.pl to itunesdb.c\n\
Walter Bell: correct handling of DND URIs with escaped characters\n\
\n\
\n"),
		       _("\
This program borrows code from the following projects:\n\
    gnutools: (mktunes.pl, ported to C) reading and writing of iTunesDB (http://www.gnu.org/software/gnupod/)\n\
    mp3info:  mp3 playlength detection (http://ibiblio.org/mp3info/)\n\
    xmms:     dirbrowser, mp3 playlength detection (http://www.xmms.org)\n\
    easytag:  reading and writing of ID3 tags (http://easytag.sourceforge.net)\n\
\n\
The GUI was created with the help of glade-2 (http://glade.gnome.org/)."),
		       NULL };
      gchar **strp = text;
      textview = GTK_TEXT_VIEW (lookup_widget (about_window, "credits_textview"));
      tb = gtk_text_view_get_buffer (textview);
      while (*strp)
      {
	  gtk_text_buffer_get_end_iter (tb, &ti);
	  gtk_text_buffer_insert (tb, &ti, *strp, -1);
	  ++strp;
      }
  }

 {
     gchar  *text[] = { _("\
German:   Jorg Schuler (jcsjcs at users dot sourceforge dot net)\n"),
				     _("\
Japanese: Ayako Sano\n"),
				     _("\
French:   David Le Brun (david at dyn-ns dot net)\n"),
				     NULL };
      gchar **strp = text;
      textview = GTK_TEXT_VIEW (lookup_widget (about_window, "translators_textview"));
      tb = gtk_text_view_get_buffer (textview);
      while (*strp)
      {
	  gtk_text_buffer_get_end_iter (tb, &ti);
	  gtk_text_buffer_insert (tb, &ti, *strp, -1);
	  ++strp;
      }
  }

  gtk_widget_show (about_window);
}

void close_about_window (void)
{
  g_return_if_fail (about_window != NULL);
  gtk_widget_destroy (about_window);
  about_window = NULL;
}

/* parse a bunch of ipod ids delimited by \n
 * @s - address of the character string we're parsing (gets updated)
 * @id - pointer the ipod id parsed from the string
 * Returns FALSE when the string is empty, TRUE when the string can still be
 * 	parsed
 */
gboolean
parse_ipod_id_from_string(gchar **s, guint32 *id)
{
    if(s && (*s))
    {
	gchar *str = *s;
	gchar *strp = strchr (str, '\n');

	if (strp == NULL)
	{
	    *id = 0;
	    *s = NULL;
	    return FALSE;
	}
	*id = (guint32)atoi(str);
	++strp;
	if (*strp) *s = strp;
	else       *s = NULL;
	return TRUE;
    }
    return FALSE;
}



/* DND: add a list of iPod IDs to Playlist @pl */
void add_idlist_to_playlist (Playlist *pl, gchar *str)
{
    guint32 id = 0;
	    
    if (!pl) return;
    while(parse_ipod_id_from_string(&str,&id))
    {
	add_songid_to_playlist(pl, id, TRUE);
    }
    data_changed();
}

/* DND: add a list of files to Playlist @pl.  @pl: playlist to add to
   or NULL. If NULL, a "New Playlist" will be created for adding
   songs and when adding a playlist file, a playlist with the name of the
   playlist file will be added.
   @songaddfunc: passed on to add_song_by_filename() etc. */
void add_text_plain_to_playlist (Playlist *pl, gchar *str, gint pl_pos,
				 AddSongFunc songaddfunc, gpointer data)
{
    gchar **files = NULL, **filesp = NULL;
    Playlist *pl_playlist = pl; /* playlist for playlist file */

    if (!str)  return;

    /*   printf("pl: %x, pl_pos: %d\n%s\n", pl, pl_pos, str);*/

    block_widgets ();

    files = g_strsplit (str, "\n", -1);
    if (files)
    {
        filesp = files;
	while (*filesp)
	{
	    gboolean added = FALSE;
            gint file_len = -1;

            gchar *file = NULL;
            gchar *decoded_file = NULL;

            file = *filesp;
            /* file is in uri form (the ones we're looking for are
               file:///), file can include the \n, which isn't a valid
               character of the filename and will cause the uri decode
               / file test to fail, so we'll cut it off if its there.
            */
            file_len = strlen (file);
            if (file_len && (strcmp(&file[file_len-1],"\n") == 0))
            {
                file[file_len-1] = (gchar)NULL;
            }

            decoded_file = filename_from_uri (file, NULL, NULL);
            if (decoded_file != NULL)
            {
		if (g_file_test (decoded_file, G_FILE_TEST_IS_DIR))
		{   /* directory */
		    if (!pl)
		    {  /* no playlist yet -- create new one */
			pl = add_new_playlist (_("New Playlist"), pl_pos);
		    }
		    add_directory_by_name (decoded_file, pl,
					   prefs_get_add_recursively (),
					   songaddfunc, data);
		    added = TRUE;
		}
		if (g_file_test (file, G_FILE_TEST_IS_REGULAR))
		{   /* regular file */
		    gint decoded_len = strlen (decoded_file);
		    if (decoded_len >= 4)
		    {
			if (strcasecmp (&decoded_file[decoded_len-4],
                                        ".mp3") == 0)
			{   /* mp3 file */
			    if (!pl)
			    {  /* no playlist yet -- create new one */
				pl = add_new_playlist (
                                                       _("New Playlist"), 
                                                       pl_pos);
			    }
			    add_song_by_filename (decoded_file, pl,
						  prefs_get_add_recursively (),
						  songaddfunc, data);
			    added = TRUE;
			}
			else if ((strcasecmp (&decoded_file[decoded_len-4],
                                              ".plu") == 0) ||
				 (strcasecmp (&decoded_file[decoded_len-4],
                                              ".m3u") == 0))
			{
			    add_playlist_by_filename (decoded_file,
                                                      pl_playlist,
						      songaddfunc, data);
			    added = TRUE;
			}
		    }
                }
                g_free (decoded_file);
	    }
	    if (!added)
	    {
		if (strlen (*filesp) != 0)
		    gtkpod_warning (_("drag and drop: ignored '%s'"), *filesp);
	    }
	    ++filesp;
	}
	g_strfreev (files);
    }
    /* display log of non-updated songs */
    display_non_updated (NULL, NULL);
    /* display log updated songs */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    remove_duplicate (NULL, NULL);

    release_widgets ();
}


void cleanup_backup_and_extended_files (void)
{
  gchar *cfgdir, *cft, *cfe;

  cfgdir = prefs_get_cfgdir ();
  /* in offline mode, there are no backup files! */
  if (cfgdir && !cfg->offline)
    {
      cft = concat_dir (cfgdir, "iTunesDB");
      cfe = concat_dir (cfgdir, "iTunesDB.ext");
      if (!cfg->write_extended_info)
	/* delete extended info file from computer */
	if (g_file_test (cfe, G_FILE_TEST_EXISTS))
	  if (remove (cfe) != 0)
	    gtkpod_warning (_("Could not delete backup file: \"%s\"\n"), cfe);
      if (!cfg->keep_backups)
	if(g_file_test (cft, G_FILE_TEST_EXISTS))
	  if (remove (cft) != 0)
	    gtkpod_warning (_("Could not delete backup file: \"%s\"\n"), cft);
      g_free (cft);
      g_free (cfe);
    }
  C_FREE (cfgdir);
}


/**
 * gtkpod_main_quit
 * 
 * return value: FALSE if it's OK to quit.
 */
gboolean
gtkpod_main_quit(void)
{
    GtkWidget *dialog;
    gint result = GTK_RESPONSE_YES;

    if (!files_are_saved ())
    {
	dialog = gtk_message_dialog_new (
	    GTK_WINDOW (gtkpod_window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_YES_NO,
	    _("Data has been changed and not been saved.\nOK to exit gtkpod?"));
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
    }

    if (result == GTK_RESPONSE_YES)
    {
	remove_all_playlists ();  /* first remove playlists, then
				   * songs! (otherwise non-existing
				   *songs may be accessed) */
	remove_all_songs ();
	display_cleanup ();
	write_prefs (); /* FIXME: how can we avoid saving options set by
			 * command line? */
			/* Tag them as dirty?  seems nasty */
	if(prefs_get_automount())
	    unmount_ipod();
	call_script ("gtkpod.out");
	gtk_main_quit ();
	return FALSE;
    }
    return TRUE;
}

/**
 * disable_import_buttons
 * Upon successfull itunes db importing we want to disable the import
 * buttons.  This retrieves the import buttons from the main gtkpod widget
 * and disables them from taking input.
 */
void
disable_gtkpod_import_buttons(void)
{
    GtkWidget *w = NULL;
    
    if(gtkpod_window)
    {
	if((w = lookup_widget(gtkpod_window, "import_button")))
	{
	    gtk_widget_set_sensitive(w, FALSE);
	    /* in case this widget has been blocked, we need to tell
	       update the desired state upon release */
	    update_blocked_widget (w, FALSE);
	}
	if((w = lookup_widget(gtkpod_window, "import_itunes_mi")))
	{
	    gtk_widget_set_sensitive(w, FALSE);
	    /* in case this widget has been blocked, we need to tell
	       update the desired state upon release */
	    update_blocked_widget (w, FALSE);
	}
    }
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

    va_start (arg, format);
    text = g_strdup_vprintf (format, arg);
    va_end (arg);
    
    gtkpod_confirmation (CONF_ID_GTKPOD_WARNING,    /* gint id, */
			 FALSE,                     /* gboolean modal, */
			 _("Warning"),              /* title */
			 _("The following has occured:"),
			 text,                /* text to be displayed */
			 NULL, 0, NULL,       /* option 1 */
			 NULL, 0, NULL,       /* option 2 */
			 TRUE,                /* gboolean confirm_again, */
			 NULL, /* ConfHandlerOpt confirm_again_handler, */
			 NULL, /* ConfHandler ok_handler,*/
			 CONF_NO_BUTTON,      /* don't show "Apply" */
			 CONF_NO_BUTTON,      /* cancel_handler,*/
			 NULL,                /* gpointer user_data1,*/
			 NULL);               /* gpointer user_data2,*/
    g_free (text);
}





/*------------------------------------------------------------------*\
 *                                                                  *
 *                   Functions for Statusbar                        *
 *                                                                  *
\*------------------------------------------------------------------*/

void
gtkpod_statusbar_init(GtkWidget *sb)
{
    gtkpod_statusbar = sb;
}

static gint
gtkpod_statusbar_clear(gpointer data)
{
    if(gtkpod_statusbar)
    {
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_statusbar), 1);
    }
    statusbar_timeout_id = 0; /* indicate that timeout handler is
				 clear (0 cannot be a handler id) */
    return FALSE;
}

void
gtkpod_statusbar_message(const gchar *message)
{
    if(gtkpod_statusbar)
    {
	gchar buf[PATH_MAX];
	guint context = 1;
	     
	snprintf(buf, PATH_MAX, "  %s", message);
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_statusbar), context);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_statusbar), context,  buf);
	if (statusbar_timeout_id != 0) /* remove last timeout, if still present */
	    gtk_timeout_remove (statusbar_timeout_id);
	statusbar_timeout_id = gtk_timeout_add(prefs_get_statusbar_timeout (),
					       (GtkFunction) gtkpod_statusbar_clear,
					       NULL);
    }
}

void 
gtkpod_songs_statusbar_init(GtkWidget *w)
{
    gtkpod_songs_statusbar = w;
    gtkpod_songs_statusbar_update();
}

void 
gtkpod_songs_statusbar_update(void)
{
    if(gtkpod_songs_statusbar)
    {
	gchar *buf;
	
	buf = g_strdup_printf (_(" P:%d S:%d/%d"), get_nr_of_playlists
		() - 1, sm_get_nr_of_songs (), get_nr_of_songs ());
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_songs_statusbar), 1);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_songs_statusbar), 1,  buf);
	g_free (buf);
    }

}


/* translates a SM_COLUMN_... (defined in display.h) into a
 * S_... (defined in song.h). Returns -1 in case a translation is not
 * possible */
S_item SM_to_S (SM_item sm)
{
    switch (sm)
    {
    case SM_COLUMN_TITLE:         return S_TITLE;
    case SM_COLUMN_ARTIST:        return S_ARTIST;
    case SM_COLUMN_ALBUM:         return S_ALBUM;
    case SM_COLUMN_GENRE:         return S_GENRE;
    case SM_COLUMN_COMPOSER:      return S_COMPOSER;
    case SM_COLUMN_TRACK_NR:      return S_TRACK_NR;
    case SM_COLUMN_IPOD_ID:       return S_IPOD_ID;
    case SM_COLUMN_PC_PATH:       return S_PC_PATH;
    case SM_COLUMN_TRANSFERRED:   return S_TRANSFERRED;
    case SM_COLUMN_SIZE:          return S_SIZE;
    case SM_COLUMN_SONGLEN:       return S_SONGLEN;
    case SM_COLUMN_BITRATE:       return S_BITRATE;
    case SM_COLUMN_PLAYCOUNT:     return S_PLAYCOUNT;
    case SM_COLUMN_RATING:        return S_RATING;
    case SM_COLUMN_TIME_PLAYED:   return S_TIME_PLAYED;
    case SM_COLUMN_TIME_MODIFIED: return S_TIME_MODIFIED;
    case SM_NUM_COLUMNS:          return -1;
    }
    return -1;
}


/* translates a ST_CAT_... (defined in display.h) into a
 * S_... (defined in song.h). Returns -1 in case a translation is not
 * possible */
S_item ST_to_S (ST_CAT_item st)
{
    switch (st)
    {
    case ST_CAT_ARTIST:      return S_ARTIST;
    case ST_CAT_ALBUM:       return S_ALBUM;
    case ST_CAT_GENRE:       return S_GENRE;
    case ST_CAT_COMPOSER:    return S_COMPOSER;
    case ST_CAT_TITLE:       return S_TITLE;
    case ST_CAT_SPECIAL:
    case ST_CAT_NUM:         return -1;
    }
    return -1;
}

/* return some sensible input about the "song". Yo must free the
 * return string after use. */
gchar *get_song_info (Song *song)
{
    if (!song) return NULL;
    if (song->pc_path_utf8 && strlen(song->pc_path_utf8))
	return g_path_get_basename (song->pc_path_utf8);
    else if ((song->title && strlen(song->title)))
	return g_strdup (song->title);
    else if ((song->album && strlen(song->album)))
	return g_strdup (song->album);
    else if ((song->artist && strlen(song->artist)))
	return g_strdup (song->artist);
    return g_strdup_printf ("iPod ID: %d", song->ipod_id);
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *             Functions for blocking widgets                       *
 *                                                                  *
\*------------------------------------------------------------------*/

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
     * importing/exporting, adding songs or directories etc. */
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
	    block_prefs_window ();
	    block_file_selector ();
	    block_dirbrowser ();
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
		release_prefs_window ();
		release_file_selector ();
		release_dirbrowser ();
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

/*------------------------------------------------------------------*\
 *                                                                  *
 *             Create iPod directory hierarchy                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* ok handler for ipod directory creation */
/* @user_data1 is the mount point of the iPod */
static void ipod_directories_ok (gpointer user_data1, gpointer user_data2)
{
    gchar *mp = user_data1;
    gboolean success = TRUE;
    gchar pbuf[PATH_MAX+1];
    gchar *buf;
    gint i;

    if (mp)
    {
	snprintf(pbuf, PATH_MAX, "%s/Calendars", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/Contacts", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/iPod_Control", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/iPod_Control/Music", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	snprintf(pbuf, PATH_MAX, "%s/iPod_Control/iTunes", mp);
	if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	for(i = 0; i < 20; i++)
	{
	    snprintf(pbuf, PATH_MAX, "%s/iPod_Control/Music/F%02d", mp, i);
	    if((mkdir(pbuf, 0755) != 0)) success = FALSE;
	}

	if (success)
	    buf = g_strdup_printf (_("Successfully created iPod directories in '%s'."), mp);
	else
	    buf = g_strdup_printf (_("Problem creating iPod directories in '%s'."), mp);
	gtkpod_statusbar_message(buf);
	g_free (buf);
	g_free (mp);
    }
}


/* cancel handler for ipod directory creation */
static void ipod_directories_cancel (gpointer user_data1, gpointer user_data2)
{
    C_FREE (user_data1);
}


/* Pop up the confirmation window for creation of ipod directory
   hierarchy */
void ipod_directories_head (void)
{
    gchar *mp;
    GString *str;

    mp = prefs_get_ipod_mount ();
    if (mp)
    {
	if (strlen (mp) > 0)
	{ /* make sure the mount point does not end in "/" */
	    if (mp[strlen (mp) - 1] == '/')
		mp[strlen (mp) - 1] = 0;
	}
    }
    else
    {
	mp = g_strdup (".");
    }
    str = g_string_sized_new (2000);
    g_string_append_printf (str, "%s/Calendars\n", mp);
    g_string_append_printf (str, "%s/Contacts\n", mp);
    g_string_append_printf (str, "%s/iPod_Control\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/iTunes\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music/F00\n...\n", mp);
    g_string_append_printf (str, "%s/iPod_Control/Music/F19\n", mp);

    if (!gtkpod_confirmation (CONF_ID_IPOD_DIR,    /* gint id, */
			 FALSE,               /* gboolean modal, */
			 _("Create iPod directories"), /* title */
			 _("OK to create the following directories?"),
		         str->str,
			 NULL, 0, NULL,      /* option 1 */
			 NULL, 0, NULL,      /* option 2 */
			 TRUE,               /* gboolean confirm_again, */
			 NULL, /* ConfHandlerOpt confirm_again_handler, */
			 ipod_directories_ok, /* ConfHandler ok_handler,*/
			 CONF_NO_BUTTON,      /* don't show "Apply" */
			 ipod_directories_cancel, /* cancel_handler,*/
			 mp,                  /* gpointer user_data1,*/
			 NULL))               /* gpointer user_data2,*/
    { /* creation failed */
	g_free (mp);
    }
    g_string_free (str, TRUE);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete Playlist                                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* ok handler for delete playlist */
/* @user_data1 is the selected playlist */
static void delete_playlist_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *selected_playlist = (Playlist *)user_data1;
    gchar *buf;

    buf = g_strdup_printf (_("Deleted playlist '%s'"),
			   selected_playlist->name);
    remove_playlist (selected_playlist);
    gtkpod_statusbar_message (buf);
    g_free (buf);
}

void delete_playlist_head (void)
{
    gchar *buf;
    Playlist *pl = NULL;

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    if (pl->type == PL_TYPE_MPL)
    { /* master playlist */
	gtkpod_statusbar_message (_("Cannot delete master playlist."));
	return;
    }
    
    buf = g_strdup_printf(_("Are you sure you want to delete the playlist '%s'?"), pl->name);

    gtkpod_confirmation
	(-1,                   /* gint id, */
	 TRUE,                 /* gboolean modal, */
	 _("Delete Playlist?"), /* title */
	 buf,                   /* label */
	 NULL,                  /* scrolled text */
	 NULL, 0, NULL,         /* option 1 */
	 NULL, 0, NULL,         /* option 2 */
	 prefs_get_playlist_deletion (),   /* gboolean confirm_again, */
	 prefs_set_playlist_deletion, /* ConfHandlerOpt confirm_again_handler,*/
	 delete_playlist_ok, /* ConfHandler ok_handler,*/
	 CONF_NO_BUTTON,     /* don't show "Apply" button */
	 NULL,               /* cancel_handler,*/
	 pl,                 /* gpointer user_data1,*/
	 NULL);              /* gpointer user_data2,*/

    g_free (buf);
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete Songs                                         *
 *                                                                  *
\*------------------------------------------------------------------*/


/* This is the same for delete_song_head() and delete_st_head(), so I
 * moved it here to make changes easier */
void delete_populate_settings (Playlist *pl, GList *selected_songids,
			       gchar **label, gchar **title,
			       gboolean *confirm_again,
			       ConfHandlerOpt *confirm_again_handler,
			       GString **str)
{
    Song *s;
    GList *l;
    guint n;

    /* write title and label */
    n = g_list_length (selected_songids);
    if (!pl) pl = get_playlist_by_nr (0); /* NULL,0: MPL */
    if(pl->type == PL_TYPE_MPL)
    {
	if (label)
	    *label = g_strdup (ngettext ("Are you sure you want to delete the following song\ncompletely from your ipod?", "Are you sure you want to delete the following songs\ncompletely from your ipod?", n));
	if (title)
	    *title = ngettext (_("Delete Song Completely?"), _("Delete Songs Completey?"), n);
	if (confirm_again)
	    *confirm_again = prefs_get_song_ipod_file_deletion ();
	if (confirm_again_handler)
	    *confirm_again_handler = prefs_set_song_ipod_file_deletion;
    }
    else /* normal playlist */
    {
	if (label)
	    *label = g_strdup_printf(ngettext ("Are you sure you want to delete the following song\nfrom the playlist \"%s\"?", "Are you sure you want to delete the following songs\nfrom the playlist \"%s\"?", n), pl->name);
	if (title)
	    *title = ngettext (_("Delete Song From Playlist?"), _("Delete Songs From Playlist?"), n);
	if (confirm_again)
	    *confirm_again = prefs_get_song_playlist_deletion ();
	if (confirm_again_handler)
	    *confirm_again_handler = prefs_set_song_playlist_deletion;
    }

    /* Write names of songs */
    if (str)
    {
	*str = g_string_sized_new (2000);
	for(l = selected_songids; l; l = l->next)
	{
	    s = get_song_by_id ((guint32)l->data);
	    if (s)
		g_string_append_printf (*str, "%s-%s\n", s->artist, s->title);
	}
    }
}


/* ok handler for delete song */
/* @user_data1 the selected playlist, @user_data2 are the selected songs */
void delete_song_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *pl = user_data1;
    GList *selected_songids = user_data2;
    gint n;
    gchar *buf;
    GList *l;

    /* sanity checks */
    if (!pl)
    {
	pl = get_playlist_by_nr (0);  /* NULL,0 = MPL */
    }
    if (!selected_songids)
	return;

    n = g_list_length (selected_songids); /* nr of songs to be deleted */
    if (pl->type == PL_TYPE_MPL)
    {
	buf = g_strdup_printf (ngettext (_("Deleted one song completely from iPod"), _("Deleted %d songs completely from iPod"), n), n);
    }
    else /* normal playlist */
    {
	buf = g_strdup_printf (ngettext (_("Deleted song from playlist '%s'"), _("Deleted songs from playlist '%s'"), n), pl->name);
    }

    for (l = selected_songids; l; l = l->next)
    {
	Song *s = get_song_by_id ((guint32)l->data);
	if (s)
	    remove_song_from_playlist(pl, s);
    }

    gtkpod_statusbar_message (buf);
    g_list_free (selected_songids);
    g_free (buf);
}

/* cancel handler for delete song */
/* @user_data1 the selected playlist, @user_data2 are the selected songs */
static void delete_song_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *selected_songids = user_data2;

    g_list_free (selected_songids);
}

void delete_song_head (void)
{
    GList *selected_songids;
    Playlist *pl;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;

    pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    selected_songids = sm_get_selected_songids();
    if (selected_songids == NULL)
    {  /* no songs selected */
	gtkpod_statusbar_message (_("No songs selected."));
	return;
    }
    delete_populate_settings (pl, selected_songids,
			      &label, &title,
			      &confirm_again, &confirm_again_handler,
			      &str);
    /* open window */
    gtkpod_confirmation
	(-1,                   /* gint id, */
	 FALSE,                /* gboolean modal, */
	 title,                /* title */
	 label,                /* label */
	 str->str,             /* scrolled text */
	 NULL, 0, NULL,        /* option 1 */
	 NULL, 0, NULL,        /* option 2 */
	 confirm_again,        /* gboolean confirm_again, */
	 confirm_again_handler,/* ConfHandlerOpt confirm_again_handler,*/
	 delete_song_ok,       /* ConfHandler ok_handler,*/
	 CONF_NO_BUTTON,       /* don't show "Apply" button */
	 delete_song_cancel,   /* cancel_handler,*/
	 pl,                   /* gpointer user_data1,*/
	 selected_songids);    /* gpointer user_data2,*/

    g_free (label);
    g_string_free (str, TRUE);
}

void
gtkpod_main_window_set_active(gboolean active)
{
    if(gtkpod_window)
    {
	gtk_widget_set_sensitive(gtkpod_window, active);
    }
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Delete songs in st entry                             *
 *                                                                  *
\*------------------------------------------------------------------*/

/* ok handler for delete tab entry */
/* @user_data1 the selected playlist, @user_data2 are the selected songs */
static void delete_entry_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *pl = user_data1;
    GList *selected_songids = user_data2;
    TabEntry *entry;
    guint32 inst;

    /* We put the instance at the first position in
     * selected_songs. Retrieve it and delete it from the list */
    inst = (guint32)g_list_nth_data (selected_songids, 0);
    selected_songids = g_list_remove (selected_songids, (gpointer)inst);
    /* Same with the selected entry */
    entry = g_list_nth_data (selected_songids, 0);
    selected_songids = g_list_remove (selected_songids, entry);

    /* Delete the songs */
    delete_song_ok (pl, selected_songids);
    /* Delete the entry */
    st_remove_entry (entry, inst);
}


/* cancel handler for delete tab entry */
/* @user_data1 the selected playlist, @user_data2 are the selected songs */
static void delete_entry_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *selected_songids = user_data2;

    g_list_free (selected_songids);
}


void delete_entry_head (gint inst)
{
    GList *selected_songids=NULL;
    Playlist *pl;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;
    TabEntry *entry;
    GList *gl;

    if ((inst < 0) || (inst > prefs_get_sort_tab_num ()))   return;
    pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    if (inst == -1)
    { /* this should not happen... */
	g_warning ("delete_entry_head(): Programming error: inst == -1\n");
	return;
    }
    entry = st_get_selected_entry (inst);
    if (entry == NULL)
    {  /* no entry selected */
	gtkpod_statusbar_message (_("No entry selected."));
	return;
    }
    if (entry->members == NULL)
    {  /* no songs in entry -> just remove entry */
	if (!entry->master)  st_remove_entry (entry, inst);
	else   gtkpod_statusbar_message (_("Cannot remove entry 'All'"));
	return;
    }
    for (gl=entry->members; gl; gl=gl->next)
    {
	Song *s=(Song *)gl->data;
	selected_songids = g_list_append (selected_songids,
					  (gpointer)s->ipod_id);
    }

    delete_populate_settings (pl, selected_songids,
			      &label, &title,
			      &confirm_again, &confirm_again_handler,
			      &str);
    /* add "entry" to beginning of the "selected_songids" list -- we can
     * only pass two args, so this is the easiest way */
    selected_songids = g_list_prepend (selected_songids, entry);
    /* add "inst" the same way */
    selected_songids = g_list_prepend (selected_songids, (gpointer)inst);

    /* open window */
    gtkpod_confirmation
	(-1,                   /* gint id, */
	 TRUE,                 /* gboolean modal, */
	 title,                /* title */
	 label,                /* label */
	 str->str,             /* scrolled text */
	 NULL, 0, NULL,        /* option 1 */
	 NULL, 0, NULL,        /* option 2 */
	 confirm_again,        /* gboolean confirm_again, */
	 confirm_again_handler,/* ConfHandlerOpt confirm_again_handler,*/
	 delete_entry_ok,      /* ConfHandler ok_handler,*/
	 CONF_NO_BUTTON,       /* don't show "Apply" button */
	 delete_entry_cancel,  /* cancel_handler,*/
	 pl,                   /* gpointer user_data1,*/
	 selected_songids);      /* gpointer user_data2,*/

    g_free (label);
    g_string_free (str, TRUE);
}

/***************************************************************************
 * Mount Calls
 *
 **************************************************************************/
/**
 * mount_ipod - attempt to mount the ipod to prefs_get_ipod_mount() This
 * does not check prefs to see if the current prefs want gtkpod itself to
 * mount the ipod drive, that should be checked before making this call.
 */
void
mount_ipod(void)
{
    gchar *str = NULL;
    if((str = prefs_get_ipod_mount()))
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	    case 0: /* child */
		execl("/bin/mount", "mount", str, NULL);
		exit(0);
		break;
	    case -1: /* parent and error */
		break;
	    default: /* parent -- let's wait for the child to terminate */
		tpid = waitpid (pid, &status, 0);
		/* we could evaluate tpid and status now */
		break;
	}
	g_free(str);
    }
}

/**
 * mount_ipod - attempt to mount the ipod to prefs_get_ipod_mount()
 */
/**
 * umount_ipod - attempt to unmount the ipod from prefs_get_ipod_mount()
 * This does not check prefs to see if the current prefs want gtkpod itself
 * to unmount the ipod drive, that should be checked before making this
 * call.
 */
void
unmount_ipod(void)
{
    gchar *str = NULL;
    if((str = prefs_get_ipod_mount()))
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	    case 0: /* child */
		execl("/bin/umount", "umount", str, NULL);
		exit(0);
		break;
	    case -1: /* parent and error */
		break;
	    default: /* parent -- let's wait for the child to terminate */
		tpid = waitpid (pid, &status, 0);
		/* we could evaluate tpid and status now */
		break;
	}
	g_free(str);
    }
}


/***************************************************************************
 * gtkpod.in,out calls
 *
 **************************************************************************/

/* tries to call "/bin/sh @script" */
static void do_script (gchar *script)
{
    if (script)
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	case 0: /* child */
	    execl("/bin/sh", "sh", script, NULL);
	    exit(0);
	break;
	case -1: /* parent and error */
	break;
	default: /* parent -- let's wait for the child to terminate */
	    tpid = waitpid (pid, &status, 0);
	    /* we could evaluate tpid and status now */
	    break;
	}
    }
}


/* tries to execute "/bin/sh ~/.gtkpod/@script" or
 * "/bin/sh /etc/gtkpod/@script" if the former does not exist */
void call_script (gchar *script)
{
    gchar *cfgdir;
    gchar *file;

    if (!script) return;

    cfgdir =  prefs_get_cfgdir ();
    file = concat_dir (cfgdir, script);
    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
	do_script (file);
    }
    else
    {
	C_FREE (file);
	file = concat_dir ("/etc/gtkpod/", script);
	if (g_file_test (file, G_FILE_TEST_EXISTS))
	{
	    do_script (file);
	}
    }
    C_FREE (file);
    C_FREE (cfgdir);
}



/***************************************************************************
 * play / enqueue / general "do command on selected songs" stuff
 *
 **************************************************************************/

/**
 * which - run the shell command which, useful for querying default values
 * for executable, 
 * @name - the executable we're trying to find the path for
 * Returns the path to the executable, NULL on not found
 */
static gchar* 
which(const gchar *exe)
{
    FILE *fp = NULL;
    gchar *result = NULL; 
    gchar buf[PATH_MAX];
    gchar *which_exec = NULL;
   
    memset(&buf[0], 0, PATH_MAX);
    which_exec = g_strdup_printf("which %s", exe);
    if((fp = popen(which_exec, "r")))
    {
	int read_bytes = 0;
	if((read_bytes = fread(buf, sizeof(gchar), PATH_MAX, fp)) > 0)
	    result = g_strndup(buf, read_bytes-1);
	pclose(fp);
    }
    C_FREE(which_exec);
    return(result);
}

/*
 * do_command_on_entries - execute @play on songs in @selected_songs
 * @play: the command to execute (e.g. "xmms -e %s")
 * @what: e.g. "Enqueue" or "Play Now" (used for error messages)
 * @selected songs: list of songs to to be placed in the command line
 * at the position of "%s"
 *
 */
void 
do_command_on_entries (gchar *command, gchar *what, GList *selected_songs)
{
    GList *l;
    gchar *str, *commandc, *next;
    gboolean percs = FALSE; /* did "%s" already appear? */
    GPtrArray *args;

    if ((!command) || (strlen (command) == 0))
    {
	gchar *buf = g_strdup_printf (_("No command set for '%s'"), what);
	gtkpod_statusbar_message (buf);
	C_FREE (buf);
	return;
    }

    /* find the command itself -- separated by ' ' */
    next = strchr (command, ' ');
    if (!next)
    {
	str = g_strdup (command);
    }
    else
    {
        str = g_strndup (command, next-command);
    }
    while (g_ascii_isspace (*command))  ++command;
    /* get the full path */
    commandc = which (str);
    if (!commandc)
    {
	gchar *buf = g_strdup_printf (_("Could not find command '%s' specified for '%s'"),
				      str, what);
	gtkpod_statusbar_message (buf);
	C_FREE (buf);
	C_FREE (str);
	return;
    }
    C_FREE (str);

    /* Create the command line */
    args = g_ptr_array_sized_new (g_list_length (selected_songs) + 10);
    /* first the full path */
    g_ptr_array_add (args, commandc);
    do
    {
	gchar *next;
	gboolean end;

	next = strchr (command, ' ');
	if (next == NULL) next = command + strlen (command);

	if (next == command)  end = TRUE;
	else                  end = FALSE;

	if (!end && (strncmp (command, "%s", 2) != 0))
	{   /* current token is not "%s" */
	    gchar *buf;
	    buf = g_strndup (command, next-command);
	    g_ptr_array_add (args, buf);
	}
	else if (!percs)
	{
	    for(l = selected_songs; l; l = l->next)
	    {
		if((str = get_song_name_on_disk_verified((Song*)l->data)))
		    g_ptr_array_add (args, str);
	    }
	    percs = TRUE; /* encountered a '%s' */
	}
	command = next;
	/* skip whitespace */
	while (g_ascii_isspace (*command))  ++command;
    } while (*command);
    /* need NULL pointer */
    g_ptr_array_add (args, NULL);

    switch(fork())
    {
    case 0: /* we are the child */
    {
	gchar **argv = (gchar **)args->pdata;
#if DEBUG_MISC
	gchar **bufp = argv;
	while (*bufp)	{ puts (*bufp); ++bufp;	}
#endif
	execv(argv[0], &argv[1]);
	g_ptr_array_free (args, TRUE);
	exit(0);
	break;
    }
    case -1: /* we are the parent, fork() failed  */
	g_ptr_array_free (args, TRUE);
	break;
    default: /* we are the parent, everything's fine */
	break;
    }
}


/*
 * play_entries_now - play the entries currently selected in xmms
 * @selected_songs: list of songs to be played
 */
void play_songs (GList *selected_songs)
{
    do_command_on_entries (prefs_get_play_now_path (),
			   _("Play Now"),
			   selected_songs);
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @selected_songs: list of songs to be played
 */
void enqueue_songs (GList *selected_songs)
{
    do_command_on_entries (prefs_get_play_enqueue_path (),
			   _("Enqueue"),
			   selected_songs);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                       free space stuff                           *
 *                                                                  *
\*------------------------------------------------------------------*/


static gchar*
get_drive_stats_from_df(const gchar *mp)
{
    FILE *fp;
    gchar buf[PATH_MAX];
    gchar *result = NULL;
    guint bytes_read = 0;

    snprintf(buf, PATH_MAX, "df -k -P | grep %s", mp);
    if((fp = popen(buf, "r")))
    {
	if((bytes_read = fread(buf, 1, PATH_MAX, fp)) > 0)
	{
	    if(g_strstr_len(buf, PATH_MAX, mp))
	    {
		int i = 0;
		int j = 0;
		gchar buf2[PATH_MAX];
		
		while(i < bytes_read)
		{
		    while(!g_ascii_isspace(buf[i]))
			buf2[j++] = buf[i++];
		    buf2[j++] = ' ';
		    while(g_ascii_isspace(buf[i]))
			i++;
		}
		buf2[j] = '\0';
		result = g_strdup_printf("%s", buf2);
	    }
	}
	pclose(fp);	
    }
    return(result);
}

/* @size: size in kB (block of 1024 Bytes) */
static gchar* 
get_filesize_in_bytes(glong size)
{
    guint i = 0;
    gchar *result = NULL;
    double newsize = (double)size * 1024;
    gchar *sizes[] = { _("Bytes"), _("kB"), _("MB"), _("GB"), _("TB"), NULL };

    while((fabs(newsize) > 1000) && (i<4))
    {
	newsize /= 1000;
	++i;
    }
    if (fabs(newsize) < 10)
	result = g_strdup_printf("%0.2f %s", newsize, sizes[i]);
    else if (fabs(newsize) < 100)
	result = g_strdup_printf("%0.1f %s", newsize, sizes[i]);
    else
	result = g_strdup_printf("%0.0f %s", newsize, sizes[i]);
    return(result);
}
#if 0
static glong
get_ipod_used_space(void)
{
    glong result = 0;
    gchar *line = NULL;
    gchar **tokens = NULL;
    gchar *mp = prefs_get_ipod_mount ();
    
    if((line = get_drive_stats_from_df(mp)))
    {
	if((tokens = g_strsplit(line, " ", 5)))
	{
	    if(tokens[0] && tokens[1] && tokens[2]) 
		result = atol(tokens[2]);
	    g_strfreev(tokens);
	}
    }
    g_free(line);
    g_free (mp);
    return(result);
}
#endif

/* get free space in kB (block of 1024 Bytes) */
static glong
get_ipod_free_space(void)
{
    glong result = 0;
    gchar *line = NULL;
    gchar **tokens = NULL;
    gchar *mp = prefs_get_ipod_mount ();

    if((line = get_drive_stats_from_df(mp)))
    {
	if((tokens = g_strsplit(line, " ", 5)))
	{
	    if(tokens[0] && tokens[1] && tokens[2] && tokens[3]) 
		result = atol(tokens[3]);
	    g_strfreev(tokens);
	}
    }
    g_free(line);
    g_free(mp);
    return(result);
}

static guint
gtkpod_space_statusbar_update(void)
{
    if(gtkpod_space_statusbar)
    {
	gchar *buf = NULL;
	gchar *str = NULL;
	glong left, pending;

	left = get_ipod_free_space() + get_filesize_of_deleted_songs ();
	pending = get_filesize_of_nontransferred_songs();
	if((left-pending) > 0)
	{
	    str = get_filesize_in_bytes(left - pending);
	    buf = g_strdup_printf (_(" %s Free"), str);
	}
	else
	{
	    str = get_filesize_in_bytes(pending - left);
	    buf = g_strdup_printf (_(" %s Pending"), str);
	}
	gtk_statusbar_pop(GTK_STATUSBAR(gtkpod_space_statusbar), 1);
	gtk_statusbar_push(GTK_STATUSBAR(gtkpod_space_statusbar), 1,  buf);
	g_free (buf);
	g_free (str);
    }
    return(TRUE);
}

void
gtkpod_space_statusbar_init(GtkWidget *w)
{
    gtkpod_space_statusbar = w;

    gtkpod_space_statusbar_update();
    gtk_timeout_add(5000, (GtkFunction) gtkpod_space_statusbar_update, NULL);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                       Timestamp stuff                            *
 *                                                                  *
\*------------------------------------------------------------------*/

/* converts the time stamp @time to a string (max. length:
 * PATH_MAX). You must g_free the return value */
gchar *time_time_to_string (time_t time)
{
    gchar *format = prefs_get_time_format ();

    if (time && format)
    {
	gchar buf[PATH_MAX+1];
	struct tm *tm = localtime (&time);
	size_t size = strftime (buf, PATH_MAX, format, tm);
	buf[size] = 0;
	return g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
    }
    return g_strdup ("--");
}



/* get the timestamp SM_COLUMN_TIME_CREATE/PLAYED/MODIFIED */
time_t time_get_time (Song *song, SM_item sm_item)
{
    guint32 mactime = 0;

    if (song) switch (sm_item)
    {
    case SM_COLUMN_TIME_PLAYED:
	mactime = song->time_played;
	break;
    case SM_COLUMN_TIME_MODIFIED:
	mactime = song->time_modified;
	break;
    default:
	mactime = 0;
	break;
    }
    return (itunesdb_time_mac_to_host (mactime));
}


/* hopefully obvious */
gchar *time_field_to_string (Song *song, SM_item sm_item)
{
    return (time_time_to_string (time_get_time (song, sm_item)));
}


/* get the timestamp SM_COLUMN_TIME_CREATE/PLAYED/MODIFIED */
void time_set_time (Song *song, time_t time, SM_item sm_item)
{
    guint32 mactime = itunesdb_time_host_to_mac (time);

    if (song) switch (sm_item)
    {
    case SM_COLUMN_TIME_PLAYED:
	song->time_played = mactime;
	break;
    case SM_COLUMN_TIME_MODIFIED:
	song->time_modified = mactime;
	break;
    default:
	break;
    }
}




/* compare @str1 and @str2 case-sensitively or case-insensitively
 * depending on prefs settings */
gint compare_string (gchar *str1, gchar *str2)
{
    if (prefs_get_case_sensitive ())
	return strcmp (str1, str2);
    else
	return compare_string_case_insensitive (str1, str2);
}


/* compare @str1 and @str2 case-sensitively or case-insensitively
 * depending on prefs settings */
gint compare_string_case_insensitive (gchar *str1, gchar *str2)
{
    gchar *string1 = g_utf8_casefold (str1, -1);
    gchar *string2 = g_utf8_casefold (str2, -1);
    gint result = g_utf8_collate (string1, string2);
    g_free (string1);
    g_free (string2);
    return result;
}


/* -------------------------------------------------------------------
 * The following is taken straight out of glib2.0.6 (gconvert.c):
 * g_filename_from_uri uses g_filename_from_utf8() to convert from
 * utf8. However, the user might have selected a different charset
 * inside gtkpod -- we must use gtkpod's charset_from_utf8()
 * instead. That's the only line changed...
 * -------------------------------------------------------------------*/

/* Test of haystack has the needle prefix, comparing case
 * insensitive. haystack may be UTF-8, but needle must
 * contain only ascii. */
static gboolean
has_case_prefix (const gchar *haystack, const gchar *needle)
{
  const gchar *h, *n;
  
  /* Eat one character at a time. */
  h = haystack;
  n = needle;

  while (*n && *h &&
	 g_ascii_tolower (*n) == g_ascii_tolower (*h))
    {
      n++;
      h++;
    }
  
  return *n == '\0';
}

static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0) 
    return -1;
  
  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0) 
    return -1;
  
  return (first_digit << 4) | second_digit;
}

static gchar *
g_unescape_uri_string (const char *escaped,
		       int         len,
		       const char *illegal_escaped_characters,
		       gboolean    ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c;
  
  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);
  
  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
	{
	  /* catch partial escape sequences past the end of the substring */
	  if (in + 3 > in_end)
	    break;

	  c = unescape_character (in + 1);

	  /* catch bad escape sequences and NUL characters */
	  if (c <= 0)
	    break;

	  /* catch escaped ASCII */
	  if (ascii_must_not_be_escaped && c <= 0x7F)
	    break;

	  /* catch other illegal escaped characters */
	  if (strchr (illegal_escaped_characters, c) != NULL)
	    break;

	  in += 2;
	}

      *out++ = c;
    }
  
  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end || !g_utf8_validate (result, -1, NULL))
    {
      g_free (result);
      return NULL;
    }

  return result;
}

static gboolean
is_escalphanum (gunichar c)
{
  return c > 0x7F || g_ascii_isalnum (c);
}

static gboolean
is_escalpha (gunichar c)
{
  return c > 0x7F || g_ascii_isalpha (c);
}

/* allows an empty string */
static gboolean
hostname_validate (const char *hostname)
{
  const char *p;
  gunichar c, first_char, last_char;

  p = hostname;
  if (*p == '\0')
    return TRUE;
  do
    {
      /* read in a label */
      c = g_utf8_get_char (p);
      p = g_utf8_next_char (p);
      if (!is_escalphanum (c))
	return FALSE;
      first_char = c;
      do
	{
	  last_char = c;
	  c = g_utf8_get_char (p);
	  p = g_utf8_next_char (p);
	}
      while (is_escalphanum (c) || c == '-');
      if (last_char == '-')
	return FALSE;
      
      /* if that was the last label, check that it was a toplabel */
      if (c == '\0' || (c == '.' && *p == '\0'))
	return is_escalpha (first_char);
    }
  while (c == '.');
  return FALSE;
}

/**
 * g_filename_from_uri:
 * @uri: a uri describing a filename (escaped, encoded in UTF-8).
 * @hostname: Location to store hostname for the URI, or %NULL.
 *            If there is no hostname in the URI, %NULL will be
 *            stored in this location.
 * @error: location to store the error occuring, or %NULL to ignore
 *         errors. Any of the errors in #GConvertError may occur.
 * 
 * Converts an escaped UTF-8 encoded URI to a local filename in the
 * encoding used for filenames. 
 * 
 * Return value: a newly-allocated string holding the resulting
 *               filename, or %NULL on an error.
 **/
gchar *
filename_from_uri (const char *uri,
		   char      **hostname,
		   GError    **error)
{
  const char *path_part;
  const char *host_part;
  char *unescaped_hostname;
  char *result;
  char *filename;
  int offs;
#ifdef G_OS_WIN32
  char *p, *slash;
#endif

  if (hostname)
    *hostname = NULL;

  if (!has_case_prefix (uri, "file:/"))
    {
      g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		   _("The URI '%s' is not an absolute URI using the file scheme"),
		   uri);
      return NULL;
    }
  
  path_part = uri + strlen ("file:");
  
  if (strchr (path_part, '#') != NULL)
    {
      g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		   _("The local file URI '%s' may not include a '#'"),
		   uri);
      return NULL;
    }
	
  if (has_case_prefix (path_part, "///")) 
    path_part += 2;
  else if (has_case_prefix (path_part, "//"))
    {
      path_part += 2;
      host_part = path_part;

      path_part = strchr (path_part, '/');

      if (path_part == NULL)
	{
	  g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		       _("The URI '%s' is invalid"),
		       uri);
	  return NULL;
	}

      unescaped_hostname = g_unescape_uri_string (host_part, path_part - host_part, "", TRUE);

      if (unescaped_hostname == NULL ||
	  !hostname_validate (unescaped_hostname))
	{
	  g_free (unescaped_hostname);
	  g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		       _("The hostname of the URI '%s' is invalid"),
		       uri);
	  return NULL;
	}
      
      if (hostname)
	*hostname = unescaped_hostname;
      else
	g_free (unescaped_hostname);
    }

  filename = g_unescape_uri_string (path_part, -1, "/", FALSE);

  if (filename == NULL)
    {
      g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		   _("The URI '%s' contains invalidly escaped characters"),
		   uri);
      return NULL;
    }

  offs = 0;
#ifdef G_OS_WIN32
  /* Drop localhost */
  if (hostname && *hostname != NULL &&
      g_ascii_strcasecmp (*hostname, "localhost") == 0)
    {
      g_free (*hostname);
      *hostname = NULL;
    }

  /* Turn slashes into backslashes, because that's the canonical spelling */
  p = filename;
  while ((slash = strchr (p, '/')) != NULL)
    {
      *slash = '\\';
      p = slash + 1;
    }

  /* Windows URIs with a drive letter can be like "file://host/c:/foo"
   * or "file://host/c|/foo" (some Netscape versions). In those cases, start
   * the filename from the drive letter.
   */
  if (g_ascii_isalpha (filename[1]))
    {
      if (filename[2] == ':')
	offs = 1;
      else if (filename[2] == '|')
	{
	  filename[2] = ':';
	  offs = 1;
	}
    }
#endif

  /* This is where we differ from glib2.0.6: we use
     gtkpod's charset_from_utf8() instead of glib's
     g_filename_from_utf8() */
  result = charset_from_utf8 (filename + offs);
  g_free (filename);
  
  return result;
}
