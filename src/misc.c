/* Time-stamp: <2003-11-26 23:19:51 jcs>
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "charset.h"
#include "confirmation.h"
#include "dirbrowser.h"
#include "display.h"
#include "file.h"
#include "interface.h"
#include "itunesdb.h"
#include "misc.h"
#include "prefs.h"
#include "prefs_window.h"
#include "song.h"
#include "support.h"


#define DEBUG_MISC 0

GtkWidget *gtkpod_window = NULL;
static GtkWidget *about_window = NULL;
static GtkWidget *file_selector = NULL;
static GtkWidget *pl_file_selector = NULL;

/* used for special playlist creation */
typedef gboolean (*PL_InsertFunc)(Track *track);

/* --------------------------------------------------------------*/
/* are widgets blocked at the moment? */
gboolean widgets_blocked = FALSE;
struct blocked_widget { /* struct to be kept in blocked_widgets */
    GtkWidget *widget;   /* widget that has been turned insensitive */
    gboolean  sensitive; /* state of the widget before */
};
/* --------------------------------------------------------------*/

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


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Ask for User Input String                            *
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


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Add new playlist asking user for name                *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Add a new playlist at position @position. The name for the new
 * playlist is queried from the user. A default (@dflt) name can be
 * provided.
 * Return value: the new playlist or NULL if the dialog was
 * cancelled. */
Playlist *add_new_playlist_user_name (gchar *dflt, gint position)
{
    Playlist *result = NULL;
    gchar *name = get_user_string (
	_("New Playlist"),
	_("Please enter a name for the new playlist"),
	dflt? dflt:_("New Playlist"));
    if (name)
    {
	result = add_new_playlist (name, position);
	gtkpod_tracks_statusbar_update ();
    }
    return NULL;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *             About Window                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

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
Patches were supplied by the following people (list may be incomplete -- please contact me)\n\n"),
		       _("\
Ramesh Dharan: Multi-Edit (edit tags of several tracks in one run)\n"),
		       _("\
Hiroshi Kawashima: Japanese charset autodetecion feature\n"),
		       _("\
Adrian Ulrich: porting of playlist code from mktunes.pl to itunesdb.c\n"),
		       _("\
Walter Bell: correct handling of DND URIs with escaped characters and/or cr/newlines at the end\n"),
		       _("\
Sam Clegg: user defined filenames when exporting tracks from the iPod\n"),
		       _("\
Chris Cutler: automatic creation of various playlist types\n"),
		       _("\
Graeme Wilford: reading and writing of the 'Composer' ID3 tags, progress dialogue during sync\n"),
		       _("\
Edward Matteucci: debugging, special playlist creation, most of the volume normalizing code\n"),
		       _("\
Jens Lautenbach: some optical improvements\n"),
		       "\n\n",
		       _("\
This program borrows code from the following projects:\n"),
		       _("\
    gnutools: (mktunes.pl, ported to C) reading and writing of iTunesDB (http://www.gnu.org/software/gnupod/)\n"),
		       _("\
    mp3info:  mp3 playlength detection (http://ibiblio.org/mp3info/)\n"),
		       _("\
    xmms:     dirbrowser, mp3 playlength detection (http://www.xmms.org)\n"),
		       _("\
    easytag:  reading and writing of ID3 tags (http://easytag.sourceforge.net)\n"),
		       "\n",
		       _("\
The GUI was created with the help of glade-2 (http://glade.gnome.org/).\n"),
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
French:   David Le Brun (david at dyn-ns dot net)\n"),
				     _("\
German:   Jorg Schuler (jcsjcs at users dot sourceforge dot net)\n"),
				     _("\
Italian:  Edward Matteucci (edward_matteucc at users dot sourceforge dot net)\n"),
				     _("\
Japanese: Ayako Sano\n"),
				     _("\
Japanese: Kentaro Fukuchi (fukuchi at users dot sourceforge dot net)\n"),
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
void add_idlist_to_playlist (Playlist *pl, gchar *string)
{
    guint32 id = 0;
    gchar *str = g_strdup (string);
    
    if (!pl) return;
    while(parse_ipod_id_from_string(&str,&id))
    {
	add_trackid_to_playlist(pl, id, TRUE);
    }
    data_changed();
    g_free (str);
}

/* DND: add a list of files to Playlist @pl.  @pl: playlist to add to
   or NULL. If NULL, a "New Playlist" will be created for adding
   tracks and when adding a playlist file, a playlist with the name of the
   playlist file will be added.
   @trackaddfunc: passed on to add_track_by_filename() etc. */
void add_text_plain_to_playlist (Playlist *pl, gchar *str, gint pl_pos,
				 AddTrackFunc trackaddfunc, gpointer data)
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
               file:///), file can include the \n or \r\n, which isn't
               a valid character of the filename and will cause the
               uri decode / file test to fail, so we'll cut it off if
               its there. */
            file_len = strlen (file);
            if (file_len && (file[file_len-1] == '\n'))
            {
                file[file_len-1] = 0;
		--file_len;
            }
            if (file_len && (file[file_len-1] == '\r'))
            {
                file[file_len-1] = 0;
		--file_len;
            }

            decoded_file = filename_from_uri (file, NULL, NULL);
            if (decoded_file != NULL)
            {
		if (g_file_test (decoded_file, G_FILE_TEST_IS_DIR))
		{   /* directory */
		    if (!pl)
		    {  /* no playlist yet -- create new one */
			pl = add_new_playlist_user_name (NULL, pl_pos);
			if (!pl)  break; /* while (*filesp) */
		    }
		    add_directory_by_name (decoded_file, pl,
					   prefs_get_add_recursively (),
					   trackaddfunc, data);
		    added = TRUE;
		}
		if (g_file_test (decoded_file, G_FILE_TEST_IS_REGULAR))
		{   /* regular file */
		    gint decoded_len = strlen (decoded_file);
		    if (decoded_len >= 4)
		    {
			if (strcasecmp (&decoded_file[decoded_len-4],
                                        ".mp3") == 0)
			{   /* mp3 file */
			    if (!pl)
			    {  /* no playlist yet -- create new one */
				pl = add_new_playlist_user_name (NULL,
								 pl_pos);
				if (!pl)  break; /* while (*filesp) */
			    }
			    add_track_by_filename (decoded_file, pl,
						  prefs_get_add_recursively (),
						  trackaddfunc, data);
			    added = TRUE;
			}
			else if ((strcasecmp (&decoded_file[decoded_len-4],
                                              ".plu") == 0) ||
				 (strcasecmp (&decoded_file[decoded_len-4],
                                              ".m3u") == 0))
			{
			    add_playlist_by_filename (decoded_file,
                                                      pl_playlist,
						      trackaddfunc, data);
			    added = TRUE;
			}
		    }
                }
                g_free (decoded_file);
	    }
	    if (!added)
	    {
		if (strlen (*filesp) != 0)
		    gtkpod_warning (_("drag and drop: ignored '%s'\n"), *filesp);
	    }
	    ++filesp;
	}
	g_strfreev (files);
    }
    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log updated tracks */
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
      cft = g_build_filename (cfgdir, "iTunesDB", NULL);
      cfe = g_build_filename (cfgdir, "iTunesDB.ext", NULL);
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
				   * tracks! (otherwise non-existing
				   *tracks may be accessed) */
	remove_all_tracks ();
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

/* translates a TM_COLUMN_... (defined in display.h) into a
 * T_... (defined in track.h). Returns -1 in case a translation is not
 * possible */
T_item TM_to_T (TM_item sm)
{
    switch (sm)
    {
    case TM_COLUMN_TITLE:         return T_TITLE;
    case TM_COLUMN_ARTIST:        return T_ARTIST;
    case TM_COLUMN_ALBUM:         return T_ALBUM;
    case TM_COLUMN_GENRE:         return T_GENRE;
    case TM_COLUMN_COMPOSER:      return T_COMPOSER;
    case TM_COLUMN_TRACK_NR:      return T_TRACK_NR;
    case TM_COLUMN_CD_NR:         return T_CD_NR;
    case TM_COLUMN_IPOD_ID:       return T_IPOD_ID;
    case TM_COLUMN_PC_PATH:       return T_PC_PATH;
    case TM_COLUMN_TRANSFERRED:   return T_TRANSFERRED;
    case TM_COLUMN_SIZE:          return T_SIZE;
    case TM_COLUMN_TRACKLEN:      return T_TRACKLEN;
    case TM_COLUMN_BITRATE:       return T_BITRATE;
    case TM_COLUMN_PLAYCOUNT:     return T_PLAYCOUNT;
    case TM_COLUMN_RATING:        return T_RATING;
    case TM_COLUMN_TIME_PLAYED:   return T_TIME_PLAYED;
    case TM_COLUMN_TIME_MODIFIED: return T_TIME_MODIFIED;
    case TM_COLUMN_VOLUME:        return T_VOLUME;
    case TM_COLUMN_YEAR:          return T_YEAR;
    case TM_NUM_COLUMNS:          return -1;
    }
    return -1;
}


/* translates a ST_CAT_... (defined in display.h) into a
 * T_... (defined in track.h). Returns -1 in case a translation is not
 * possible */
T_item ST_to_T (ST_CAT_item st)
{
    switch (st)
    {
    case ST_CAT_ARTIST:      return T_ARTIST;
    case ST_CAT_ALBUM:       return T_ALBUM;
    case ST_CAT_GENRE:       return T_GENRE;
    case ST_CAT_COMPOSER:    return T_COMPOSER;
    case ST_CAT_TITLE:       return T_TITLE;
    case ST_CAT_YEAR:        return T_YEAR;
    case ST_CAT_SPECIAL:
    case ST_CAT_NUM:         return -1;
    }
    return -1;
}

/* return some sensible input about the "track". Yo must free the
 * return string after use. */
gchar *get_track_info (Track *track)
{
    if (!track) return NULL;
    if (track->pc_path_utf8 && strlen(track->pc_path_utf8))
	return g_path_get_basename (track->pc_path_utf8);
    else if ((track->title && strlen(track->title)))
	return g_strdup (track->title);
    else if ((track->album && strlen(track->album)))
	return g_strdup (track->album);
    else if ((track->artist && strlen(track->artist)))
	return g_strdup (track->artist);
    return g_strdup_printf ("iPod ID: %d", track->ipod_id);
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
    /* mark data as changed */
    data_changed ();
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
 *             Delete Tracks                                         *
 *                                                                  *
\*------------------------------------------------------------------*/


/* This is the same for delete_track_head() and delete_st_head(), so I
 * moved it here to make changes easier */
void delete_populate_settings (Playlist *pl, GList *selected_trackids,
			       gchar **label, gchar **title,
			       gboolean *confirm_again,
			       ConfHandlerOpt *confirm_again_handler,
			       GString **str)
{
    Track *s;
    GList *l;
    guint n;

    /* write title and label */
    n = g_list_length (selected_trackids);
    if (!pl) pl = get_playlist_by_nr (0); /* NULL,0: MPL */
    if(pl->type == PL_TYPE_MPL)
    {
	if (label)
	    *label = g_strdup (ngettext ("Are you sure you want to delete the following track\ncompletely from your ipod?", "Are you sure you want to delete the following tracks\ncompletely from your ipod?", n));
	if (title)
	    *title = ngettext ("Delete Track Completely?",
			       "Delete Tracks Completey?", n);
	if (confirm_again)
	    *confirm_again = prefs_get_track_ipod_file_deletion ();
	if (confirm_again_handler)
	    *confirm_again_handler = prefs_set_track_ipod_file_deletion;
    }
    else /* normal playlist */
    {
	if (label)
	    *label = g_strdup_printf(ngettext ("Are you sure you want to delete the following track\nfrom the playlist \"%s\"?", "Are you sure you want to delete the following tracks\nfrom the playlist \"%s\"?", n), pl->name);
	if (title)
	    *title = ngettext ("Delete Track From Playlist?",
			       "Delete Tracks From Playlist?", n);
	if (confirm_again)
	    *confirm_again = prefs_get_track_playlist_deletion ();
	if (confirm_again_handler)
	    *confirm_again_handler = prefs_set_track_playlist_deletion;
    }

    /* Write names of tracks */
    if (str)
    {
	*str = g_string_sized_new (2000);
	for(l = selected_trackids; l; l = l->next)
	{
	    s = get_track_by_id ((guint32)l->data);
	    if (s)
		g_string_append_printf (*str, "%s-%s\n", s->artist, s->title);
	}
    }
}


/* ok handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
void delete_track_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *pl = user_data1;
    GList *selected_trackids = user_data2;
    gint n;
    gchar *buf;
    GList *l;

    /* sanity checks */
    if (!pl)
    {
	pl = get_playlist_by_nr (0);  /* NULL,0 = MPL */
    }
    if (!selected_trackids)
	return;

    n = g_list_length (selected_trackids); /* nr of tracks to be deleted */
    if (pl->type == PL_TYPE_MPL)
    {
	buf = g_strdup_printf (
	    ngettext ("Deleted one track completely from iPod",
		      "Deleted %d tracks completely from iPod", n), n);
    }
    else /* normal playlist */
    {
	buf = g_strdup_printf (
	    ngettext ("Deleted track from playlist '%s'",
		      "Deleted tracks from playlist '%s'", n), pl->name);
    }

    for (l = selected_trackids; l; l = l->next)
	remove_trackid_from_playlist (pl, (guint32)l->data);

    gtkpod_statusbar_message (buf);
    g_list_free (selected_trackids);
    g_free (buf);
    /* mark data as changed */
    data_changed ();
}

/* cancel handler for delete track */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
static void delete_track_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *selected_trackids = user_data2;

    g_list_free (selected_trackids);
}


/* call with @pl = NULL for currently selected playlist */
void delete_track_head (Playlist *pl)
{
    GList *selected_trackids;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;

    if (pl == NULL)  pl = pm_get_selected_playlist();
    if (pl == NULL)
    { /* no playlist??? Cannot happen, but... */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    selected_trackids = tm_get_selected_trackids();
    if (selected_trackids == NULL)
    {  /* no tracks selected */
	gtkpod_statusbar_message (_("No tracks selected."));
	return;
    }
    delete_populate_settings (pl, selected_trackids,
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
	 delete_track_ok,      /* ConfHandler ok_handler,*/
	 CONF_NO_BUTTON,       /* don't show "Apply" button */
	 delete_track_cancel,  /* cancel_handler,*/
	 pl,                   /* gpointer user_data1,*/
	 selected_trackids);   /* gpointer user_data2,*/

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
 *             Delete tracks in st entry                             *
 *                                                                  *
\*------------------------------------------------------------------*/

/* ok handler for delete tab entry */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
static void delete_entry_ok (gpointer user_data1, gpointer user_data2)
{
    Playlist *pl = user_data1;
    GList *selected_trackids = user_data2;
    TabEntry *entry;
    guint32 inst;

    /* We put the instance at the first position in
     * selected_tracks. Retrieve it and delete it from the list */
    inst = (guint32)g_list_nth_data (selected_trackids, 0);
    selected_trackids = g_list_remove (selected_trackids, (gpointer)inst);
    /* Same with the selected entry */
    entry = g_list_nth_data (selected_trackids, 0);
    selected_trackids = g_list_remove (selected_trackids, entry);

    /* Delete the tracks */
    delete_track_ok (pl, selected_trackids);
    /* Delete the entry */
    st_remove_entry (entry, inst);
    /* mark data as changed */
    data_changed ();
}


/* cancel handler for delete tab entry */
/* @user_data1 the selected playlist, @user_data2 are the selected tracks */
static void delete_entry_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *selected_trackids = user_data2;

    g_list_free (selected_trackids);
}


/* if @pl == NULL, the currently selected playlist will be selected */
void delete_entry_head (gint inst, Playlist *pl)
{
    GList *selected_trackids=NULL;
    GString *str;
    gchar *label, *title;
    gboolean confirm_again;
    ConfHandlerOpt confirm_again_handler;
    TabEntry *entry;
    GList *gl;

    if ((inst < 0) || (inst > prefs_get_sort_tab_num ()))   return;
    if (pl == NULL)    pl = pm_get_selected_playlist();
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
    {  /* no tracks in entry -> just remove entry */
	if (!entry->master)  st_remove_entry (entry, inst);
	else   gtkpod_statusbar_message (_("Cannot remove entry 'All'"));
	return;
    }
    for (gl=entry->members; gl; gl=gl->next)
    {
	Track *s=(Track *)gl->data;
	selected_trackids = g_list_append (selected_trackids,
					  (gpointer)s->ipod_id);
    }

    delete_populate_settings (pl, selected_trackids,
			      &label, &title,
			      &confirm_again, &confirm_again_handler,
			      &str);
    /* add "entry" to beginning of the "selected_trackids" list -- we can
     * only pass two args, so this is the easiest way */
    selected_trackids = g_list_prepend (selected_trackids, entry);
    /* add "inst" the same way */
    selected_trackids = g_list_prepend (selected_trackids, (gpointer)inst);

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
	 selected_trackids);      /* gpointer user_data2,*/

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
    file = g_build_filename (cfgdir, script, NULL);
    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
	do_script (file);
    }
    else
    {
	C_FREE (file);
	file = g_build_filename ("/etc/gtkpod/", script, NULL);
	if (g_file_test (file, G_FILE_TEST_EXISTS))
	{
	    do_script (file);
	}
    }
    C_FREE (file);
    C_FREE (cfgdir);
}



/***************************************************************************
 * play / enqueue / general "do command on selected tracks" stuff
 *
 **************************************************************************/

/**
 * which - run the shell command which, useful for querying default values
 * for executable,
 * @name - the executable we're trying to find the path for
 * Returns the path to the executable, NULL on not found
 */
gchar* 
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
 * do_command_on_entries - execute @play on tracks in @selected_tracks
 * @play: the command to execute (e.g. "xmms -e %s")
 * @what: e.g. "Enqueue" or "Play Now" (used for error messages)
 * @selected tracks: list of tracks to to be placed in the command line
 * at the position of "%s"
 *
 */
void 
do_command_on_entries (gchar *command, gchar *what, GList *selected_tracks)
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
    args = g_ptr_array_sized_new (g_list_length (selected_tracks) + 10);
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
	    for(l = selected_tracks; l; l = l->next)
	    {
		if((str = get_track_name_on_disk_verified((Track*)l->data)))
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
 * @selected_tracks: list of tracks to be played
 */
void play_tracks (GList *selected_tracks)
{
    do_command_on_entries (prefs_get_play_now_path (),
			   _("Play Now"),
			   selected_tracks);
}

/*
 * play_entries_now - play the entries currently selected in xmms
 * @selected_tracks: list of tracks to be played
 */
void enqueue_tracks (GList *selected_tracks)
{
    do_command_on_entries (prefs_get_play_enqueue_path (),
			   _("Enqueue"),
			   selected_tracks);
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



/* get the timestamp TM_COLUMN_TIME_CREATE/PLAYED/MODIFIED */
time_t time_get_time (Track *track, TM_item tm_item)
{
    guint32 mactime = 0;

    if (track) switch (tm_item)
    {
    case TM_COLUMN_TIME_PLAYED:
	mactime = track->time_played;
	break;
    case TM_COLUMN_TIME_MODIFIED:
	mactime = track->time_modified;
	break;
    default:
	mactime = 0;
	break;
    }
    return (itunesdb_time_mac_to_host (mactime));
}


/* hopefully obvious */
gchar *time_field_to_string (Track *track, TM_item tm_item)
{
    return (time_time_to_string (time_get_time (track, tm_item)));
}


/* get the timestamp TM_COLUMN_TIME_CREATE/PLAYED/MODIFIED */
void time_set_time (Track *track, time_t time, TM_item tm_item)
{
    guint32 mactime = itunesdb_time_host_to_mac (time);

    if (track) switch (tm_item)
    {
    case TM_COLUMN_TIME_PLAYED:
	track->time_played = mactime;
	break;
    case TM_COLUMN_TIME_MODIFIED:
	track->time_modified = mactime;
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



/*------------------------------------------------------------------*\
 *                                                                  *
 *                     Special Playlist Stuff                       *
 *                                                                  *
\*------------------------------------------------------------------*/

/* generate_category_playlists: Create a playlist for each category
   @cat (T_ARTIST, T_ALBUM, T_GENRE, T_COMPOSER) */
void generate_category_playlists (T_item cat)
{
    Playlist *master_pl;
    gint i;
    gchar *qualifier;

    /* sanity */
    if ((cat != T_ARTIST) && (cat != T_ALBUM) &&
	(cat != T_GENRE) && (cat != T_COMPOSER)) return;

    /* Initialize the "qualifier". It is used to indicate the category of
       automatically generated playlists */
    switch (cat)
    {
    case T_ARTIST:
	qualifier = _("AR:");
	break;
    case T_ALBUM:
	qualifier = _("AL:");
	break;
    case T_GENRE:
	qualifier = _("GE:");
	break;
    case T_COMPOSER:
	qualifier = _("CO:");
	break;
    default:
	qualifier = NULL;
	break;
    }

    /* sanity */
    if (qualifier == NULL) return;

    /* FIXME: delete all playlists named '[<qualifier> .*]' and
     * remember which playlist was selected if it gets deleted */

    master_pl = get_playlist_by_nr (0);

    for(i = 0; i < get_nr_of_tracks_in_playlist (master_pl) ; i++)
    {
        Track *track = g_list_nth_data (master_pl->members, i);
        Playlist *cat_pl = NULL;
	gint j;
        gchar *category = NULL;
	gchar *track_cat = NULL;
        int playlists_len = get_nr_of_playlists();

	track_cat = track_get_item_utf8 (track, cat);

	if (track_cat)
	{
	    /* some tracks have empty strings in the genre field */
	    if(track_cat[0] == '\0')
	    {
		category = g_strdup_printf ("[%s %s]",
					    qualifier, _("Unknown"));
	    }
	    else
	    {
		category = g_strdup_printf ("[%s %s]",
					    qualifier, track_cat);
	    }

	    /* look for category playlist */
	    for(j = 1; j < playlists_len; j++)
	    {
		Playlist *pl = get_playlist_by_nr (j);

		if(g_ascii_strcasecmp(pl->name, category) == 0)
		{
		    cat_pl = pl;
		    break;
		}
	    }
	    /* or, create category playlist */
	    if(!cat_pl)
	    {
		cat_pl = add_new_playlist(category, -1);
	    }

	    add_track_to_playlist(cat_pl, track, TRUE);
	    C_FREE (category);
	}
    }
    gtkpod_tracks_statusbar_update();
}

/* Generate a new playlist containing all the tracks currently
   displayed */
Playlist *generate_displayed_playlist (void)
{
    GList *tracks = tm_get_all_tracks ();
    Playlist *result = generate_new_playlist (tracks);
    g_list_free (tracks);
    return result;
}


/* Generate a new playlist containing all the tracks currently
   selected */
Playlist *generate_selected_playlist (void)
{
    GList *tracks = tm_get_selected_tracks ();
    Playlist *result = generate_new_playlist (tracks);
    g_list_free (tracks);
    return result;
}

/* Generate a playlist consisting of the tracks in @tracks
 * with @name name */
Playlist *generate_playlist_with_name (GList *tracks,gchar *pl_name){
    GList *l;
    Playlist *pl=NULL;
    gint n = g_list_length (tracks);
    gchar *str;

    if(n>0)
    {
        pl = add_new_playlist (pl_name, -1);
        for (l=tracks; l; l=l->next)
        {
            Track *track = (Track *)l->data;
            add_track_to_playlist (pl, track, TRUE);
        }
        str = g_strdup_printf (ngettext ("Created playlist '%s' with %d track.",
				     "Created playlist '%s' with %d tracks.",
				     n), pl_name, n);
    }
    else
    { /* n==0 */
        str = g_strdup_printf (_("No tracks available, playlist not created"));
    }
    gtkpod_statusbar_message (str);
    gtkpod_tracks_statusbar_update();
    g_free (str);
    return pl;
}

/* Generate a playlist named "New Playlist" consisting of the tracks in @tracks. */
Playlist *generate_new_playlist (GList *tracks)
{
    return generate_playlist_with_name (tracks, _("New Playlist"));
}

/* look at the add_ranked_playlist help:
 * BEWARE this function shouldn't be used*/
static GList *create_ranked_glist(gint tracks_nr,PL_InsertFunc insertfunc,
                                  GCompareFunc comparefunc)
{
   GList *tracks=NULL;
   gint f=0;
   gint i=0;
   Track *track=NULL;
  
   while ((track=get_next_track(i)))
   {
      i=1; /* for get_next_track() */
      if (track && (!insertfunc || insertfunc (track)))
      {
         tracks = g_list_insert_sorted (tracks, track, comparefunc);
         ++f;
         if (tracks_nr && (f>tracks_nr))
         {   /*cut the tail*/
            tracks = g_list_remove(tracks,
                   g_list_nth_data(tracks, tracks_nr));
            --f;
         }
      }
   }
   return tracks;
}
/* Generate a new playlist named @pl_name, containing @tracks_nr tracks.
 *
 * @insertfunc: determines which tracks to enter into the new playlist.
 *              If @insertfunc is NULL, all tracks are added.
 * @comparefunc: determines order of tracks
 * @tracks_nr: max. number of tracks in playlist or 0 for no limit.
 *
 * Return value: the newly created playlist
 */
static Playlist *add_ranked_playlist(gchar *pl_name, gint tracks_nr,
				     PL_InsertFunc insertfunc,
				     GCompareFunc comparefunc)
{
    Playlist *result = NULL;
    GList *tracks = create_ranked_glist(tracks_nr,insertfunc,comparefunc);
    gint f;
    f=g_list_length(tracks);

    if (f != 0)
    /* else generate_playlist_with_name prints something*/
    {
#if 0
	GList *sl;
	for (sl=tracks; sl; sl=sl->next)
	{
	    Track *s=sl->data;
	    printf ("%ud\n", s->time_played);
	}
#endif
	result = generate_playlist_with_name (tracks, pl_name);
    }
    g_list_free (tracks);
    return result;
}

 /*update a ranked playlist according the iPod content,
 * @str is the playlist's name (no [ or ])*/
static void update_ranked_playlist(gchar *str, gint tracks_nr,
				   PL_InsertFunc insertfunc,
				   GCompareFunc compfunc)
{
    gchar *str2 = g_strdup_printf ("[%s]", str);
    Playlist *sel_pl, *new_pl;
    gboolean select = FALSE;

    /* currently selected playlist */
    sel_pl= pm_get_selected_playlist ();
    /* remove all playlists with named @str2 */
    remove_playlist_by_name (str2);
    /* check if we deleted the selected playlist */
    if (sel_pl && !playlist_exists (sel_pl))   select = TRUE;
    new_pl = add_ranked_playlist (str2, tracks_nr, insertfunc, compfunc);
    if (select)
    {   /* need to select newly created playlist because the old
	 * selection was deleted */
	pm_select_playlist (new_pl);
    }
    g_free (str2);
}

/* ------------------------------------------------------------ */
/* Generate a new playlist containing the most listened (playcount
 * reverse order) tracks. to enter this playlist a track must have been
 * played */

/* Sort Function: determines the order of the generated playlist */

/* NOTE: THE USE OF 'COMP' ARE NECESSARY FOR THE TIME_PLAYED COMPARES
   WHERE A SIGN OVERFLOW MAY OCCUR BECAUSE OF THE 32 BIT UNSIGNED MAC
   TIMESTAMPS. */
static gint Most_Listened_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->rating, a->rating);
	if (result == 0) result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Most_Listened_IF (Track *track)
{
    if (track)   return (track->playcount != 0);
    return      FALSE;
}

void most_listened_pl(void)
{
    gint tracks_nr = prefs_get_misc_track_nr();
    gchar *str = g_strdup_printf (_("Most Listened (%d)"), tracks_nr);
    update_ranked_playlist (str, tracks_nr,
			    Most_Listened_IF, Most_Listened_CF);
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing all songs never listened to. */

/* Sort Function: determines the order of the generated playlist */

/* NOTE: THE USE OF 'COMP' ARE NECESSARY FOR THE TIME_PLAYED COMPARES
   WHERE A SIGN OVERFLOW MAY OCCUR BECAUSE OF THE 32 BIT UNSIGNED MAC
   TIMESTAMPS. */
static gint Never_Listened_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->rating, a->rating);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Never_Listened_IF (Track *track)
{
    if (track)   return (track->playcount == 0);
    return      FALSE;
}

void never_listened_pl(void)
{
    gint tracks_nr = 0; /* no limit */
    gchar *str = g_strdup_printf (_("Never Listened"));
    update_ranked_playlist (str, tracks_nr,
			    Never_Listened_IF, Never_Listened_CF);
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the most rated (rate 
 * reverse order) tracks. */

/* Sort Function: determines the order of the generated playlist */
static gint Most_Rated_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;
    
    if (a && b)
    {
	result = COMP (b->rating, a->rating);
	if (result == 0) result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Most_Rated_IF (Track *track)
{
    if (track) return ((track->playcount != 0) || prefs_get_not_played_track());
    return FALSE;
}

void most_rated_pl(void)
{
    gint tracks_nr = prefs_get_misc_track_nr();
    gchar *str =  g_strdup_printf (_("Best Rated (%d)"), tracks_nr);
    update_ranked_playlist (str, tracks_nr,
			    Most_Rated_IF, Most_Rated_CF);
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the last listened (last time play 
 * reverse order) tracks. */

/* Sort Function: determines the order of the generated playlist */
static gint Last_Listened_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->time_played, a->time_played);
	if (result == 0) result = COMP (b->rating, a->rating);
	if (result == 0) result = COMP (b->playcount, a->playcount);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Last_Listened_IF (Track *track)
{
    if (track)   return (track->playcount != 0);
    return      FALSE;
}

void last_listened_pl(void)
{
    gint tracks_nr = prefs_get_misc_track_nr();
    gchar *str = g_strdup_printf (_("Recent (%d)"), tracks_nr);
    update_ranked_playlist (str, tracks_nr,
			    Last_Listened_IF, Last_Listened_CF);
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the tracks listened to since the
 * last time the iPod was connected to a computer (and the playcount
 * file got wiped) */

/* Sort Function: determines the order of the generated playlist */
static gint since_last_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->recent_playcount, a->recent_playcount);
	if (result == 0) result = COMP (b->time_played, a->time_played);
	if (result == 0) result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->rating, a->rating);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean since_last_IF (Track *track)
{
    if (track && (track->recent_playcount != 0))  return TRUE;
    else                                        return FALSE;
}

void since_last_pl(void)
{
    update_ranked_playlist (_("Last Time"), 0,
			    since_last_IF, since_last_CF);
}


/* ------------------------------------------------------------
------------------------------------------------------------------
--------                                                 ---------
--------                 UTF16 section                   ---------
--------                                                 ---------
------------------------------------------------------------------
   ------------------------------------------------------------ */
 
/* Get length of utf16 string in number of characters (words) */
guint32 utf16_strlen (gunichar2 *utf16)
{
  guint32 i=0;
  if (utf16)
      while (utf16[i] != 0) ++i;
  return i;
}

/* duplicate a utf16 string */
gunichar2 *utf16_strdup (gunichar2 *utf16)
{
    guint32 len;
    gunichar2 *new = NULL;

    if (utf16)
    {
	len = utf16_strlen (utf16);
	new = g_malloc (sizeof (gunichar2) * (len+1));
	if (new) memcpy (new, utf16, sizeof (gunichar2) * (len+1));
    }
    return new;
}


/* ------------------------------------------------------------
------------------------------------------------------------------
--------                                                 ---------
--------  THE REST OF THE FILE IS COMMENTED-OUT (#IF 0)  ---------
--------                                                 ---------
------------------------------------------------------------------
   ------------------------------------------------------------ */

#if 0
static gint relative_path_CF (gconstpointer aa, gconstpointer bb)
{
   gint result = 0;
   const Track *a = aa;
   const Track *b = bb;

   if (a && b)
   {
      result=strcmp(b->ipod_path,a->ipod_path);
   }
   return result;
}

/* a function to recover the itunesdb from an 
 * inconsistent state (if some file get lost or
 * there are some not more existent files in the
 * iTunesDB)*/
void recover_db(void)
{
/*BEWARE can broke the iTunesDB if it's run BEFORE uploading the iTunesdb*/
/*this function can be divided in several section
 * every section is independent (can be placed in a
 * separate function). there is only one common value
 * */
   /* THE COMMON VALUE*/
   gboolean changed=FALSE; /*ths iTunesDB has been changed?*/
   
/********************************
 * DELETE DEAD FILES's SECTION
 ********************************/
   Track *track=NULL;
   gchar *pathtrack=NULL;
   GList *ghost_tracks=NULL;
   gint g=0;
   while((track=get_next_track(g)))
   {
      g=1;
      pathtrack=get_track_name_on_ipod(track);
      if(!g_file_test(pathtrack,G_FILE_TEST_IS_REGULAR))
      {
         ghost_tracks=g_list_append(ghost_tracks,track);
         changed=TRUE;
      }
   }

   /*take care of this "ghost" tracks*/
   if(g_list_length(ghost_tracks)!=0)
   {
      delete_track_ok(NULL,ghost_tracks); /*NULL== erase from MPL*/
   }
   /*free everything*/
   g_free(pathtrack);
   free_track(track);
   track=NULL;
   g_list_free(ghost_tracks);
   ghost_tracks=NULL;


}void iani(void){gboolean changed=FALSE;
   
/**************************
 * ORPHANED FILES's SECTION
 **************************/
   Track *ipod_track=NULL; /*an existing non lost track*/
   gchar *ipod_filename=NULL; /*the ipod's track filename 
      * this file exists but we don't know if it has an iTunesDB entry*/
   gchar *ipod_validfile=NULL; /*not lost file */
   
   Track *lost_track=NULL; /*an existing track that has no entry in the iTunesDB*/
   gchar *lostfile_fullpath=NULL; /*the lost track's fullpath*/
   gboolean found;
   gchar *ipod_dir = NULL;
   GDir *dir_des;

#if 0 /*read the TODO below*/
   gboolean ordered_exist=FALSE;
   GList *ranked_list=NULL;
   gint mean;
   gint min,max;
#endif

   gint h;
   gboolean lost_pl_exist=FALSE;
   gchar *mp=g_strdup(prefs_get_ipod_mount());
   Playlist *lost_pl=NULL; /*a pl containing all the losts tracks*/

   for(h=0;h<20;h++)
   {
      /*directory name*/
      ipod_dir=g_strdup_printf("%s/iPod_Control/Music/F%02d",mp, h);
      dir_des=g_dir_open(ipod_dir,0,NULL);
      /*this shouldn't happend, but you never know */
      if(dir_des!=NULL){
      while((ipod_filename=g_strdup(g_dir_read_name(dir_des))))/*we have a file in the directory*/
      {
         lostfile_fullpath=g_strdup_printf("%s/%s",ipod_dir,ipod_filename);
         /*<TODO>*/
         /*FIXME:
          * we can create a path ordered glist 
          * containing all the tracks in the iTunesDB
          * and search the possibly lost files
          * in this list using a much less
          * expensive algoritm
          *     TODO:
          *     check the strcmp
          *     investigate the SEGFAULT
          */
#if 0
         if(!ordered_exist)
         {
            ordered_exist=TRUE;
            ranked_list=create_ranked_glist(0,NULL,relative_path_CF); /*SEGFAULT!!!*/
         }
         found=FALSE;
         
         min=0;
         /*if "max=1" while doesn't start but we need lost_track*/
         lost_track=g_list_nth_data(ranked_list,min);
         max=g_list_length(ranked_list)-1;
         while (min<max)
         {
            mean = (min+max)/2;
            lost_track=g_list_nth_data(ranked_list,mean);
            /*work?*/
            if (strcmp(lostfile_fullpath,lost_track->ipod_path)>0)
            {
               min = mean+1;
            }
            else
            {
               max = mean;
            }
         }
         if (strcmp(lostfile_fullpath,lost_track->ipod_path)!=0)
         {
            free_track(lost_track);
            lost_track = NULL;
            found=FALSE;
         }
         else
         {
            found=TRUE;
         }
#else         
         found=FALSE;
         ipod_track=get_next_track(0);
         while(!found&&ipod_track!=NULL)
         {
            ipod_validfile=get_track_name_on_ipod(ipod_track);
            if(strcmp(ipod_validfile,lostfile_fullpath)==0)
            {
               found=TRUE;
            }
            else
            {
               ipod_track=get_next_track(1);
            }
         /*</TODO>*/
         } /*end of while(!found&&track!=NULL)*/
#endif
         if(!found)/*it's a lost track, add it in the iTunesDB*/
         {
            if(!lost_pl_exist) /*create a lost files' pl*/
            {
               lost_pl=add_new_playlist(g_strdup(_("[Lost tracks]")),-1);
               lost_pl_exist=TRUE;
            }
            /*add an iTunesDB entry to the iTunesDB and to the lost_pl*/
            lost_track=get_track_info_from_file (lostfile_fullpath,lost_track);
            add_track_to_playlist(NULL,lost_track,TRUE); /*MPL*/
            add_track_to_playlist(lost_pl,lost_track,TRUE); /*lost_pl*/
            changed=TRUE;
         }
      } /*end of while(filename!=NULL)*/
      g_dir_close(dir_des);
      }/*if*/
   }/*end for*/

   g_free(lostfile_fullpath);
   g_free(ipod_filename);
   g_free(ipod_dir);
   free_track(ipod_track);
   ipod_track=NULL;
   free_track(lost_track);
   lost_track=NULL;
   free_playlist(lost_pl); /*a pl containing all the losts tracks*/
   g_free(ipod_validfile);
   g_free(mp);
   
   //GDir *dir_des;
   //Track *ipod_track=NULL; /*an existing non lost track*/

/***************************
 * NEWS TO THE WORLD SECTION
 ***************************/
   if(changed)
      data_changed();

}
#endif
