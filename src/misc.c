/*
|  Copyright (C) 2002 Jorg Schuler <jcsjcs at sourceforge.net>
|  Part of the gtkpod project.
| 
|  URL: 
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
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "song.h"
#include "interface.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"
#include "itunesdb.h"
#include "display.h"

static GtkWidget *main_window = NULL;
static GtkWidget *about_window = NULL;
static GtkWidget *file_selector = NULL;
  

static void add_files_ok_button (GtkWidget *button, GtkFileSelection *selector)
{
  gchar **names;
  gint i;

  names = gtk_file_selection_get_selections (GTK_FILE_SELECTION (selector));
  for (i=0; names[i] != NULL; ++i)
    {
      add_song_by_filename (names[i]);
      if(!i)
	  prefs_set_last_dir_browse(names[i]);
    }
  g_strfreev (names);
}

/* called when the file selector is closed */
static void add_files_close (GtkWidget *w1, GtkWidget *w2)
{
    if (file_selector)    gtk_widget_destroy(file_selector),
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



/* Concats "dir" and "file" into full filename, taking
   into account that "dir" may or may not end with "/".
   You must free the return string after use
   This code tries to take into account some stupid constellations
   when either "dir" or "file" is not set, or file starts with a "/"
   (taken as absolute path) etc.  */
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file)
{
    if (file && (*file == '/'))
    { /* we consider filenames starting with "/" to be absolute ->
	 discard the dir part. */
	return g_strdup (file);
    }
    if (dir)
    {
	if (strlen (dir) != 0)
	{	    
	    if(dir[strlen(dir)-1] == '/')
	    { /* "dir" ends with "/" */
		if (file)
		    return g_strdup_printf ("%s%s", dir, file);
		else
		    return g_strdup (dir);
	    }
	    else
	    { /* "dir" does not end with "/" */
		if (file)
		    return g_strdup_printf ("%s/%s", dir, file);
		else
		    return g_strdup_printf ("%s/", dir);
	    }
	}
	else
	{ /* strlen (dir) == 0 */
	    return g_strdup (file);
	}
    }
    else
    { /* dir == NULL */
	if (file)
	    return g_strdup (file);
	else
	    return g_strdup (""); /* how stupid can the caller be... */
    }
}


void open_about_window ()
{
  GtkLabel *about_label;
  gchar *buffer_text, *label_text;
  GtkTextView *about_credit_textview;

  if (about_window != NULL) return;
  about_window = create_gtkpod_about_window ();
  about_label = GTK_LABEL (lookup_widget (about_window, "about_label"));
  label_text = g_strdup_printf (_("gtkpod Version %s: Cross-Platform Multi-Lingual Interface to Apple's iPod(tm)."), VERSION);
  gtk_label_set_text (about_label, label_text);
  g_free (label_text);
  buffer_text = g_strdup_printf ("%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s",
				 _("(C) 2002 Jörg Schuler (jcsjcs at sourceforge.net)"),
				 _("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details."),
				 _("The code handling the reading and writing of the iTunesDB was ported from mktunes.pl of the gnuPod package written by Adrian Ulrich (http://www.blinkenlights.ch/cgi-bin/fm.pl?get=ipode). Adrian Ulrich ported the playlist part."),
				 _("This program borrows code from the following projects:"),
				 _("  xmms: dirbrowser\n  easytag: reading of ID3 tags\n  GNOMAD: playlength detection"),
				 _("The GUI was created with the help of glade-2."));
  about_credit_textview = GTK_TEXT_VIEW (lookup_widget (about_window, "credits_textview"));
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (about_credit_textview),
			    buffer_text, -1);
  g_free (buffer_text);
  gtk_widget_show (about_window);
}


void close_about_window (void)
{
  g_return_if_fail (about_window != NULL);
  gtk_widget_destroy (about_window);
  about_window = NULL;
}

/* parse a bunch of ipod ids delimited by \n
 * @s - address of the character string we're parsing
 * @id - pointer the ipod id parsed from the string
 * Returns FALSE when the string is empty, TRUE when the string can still be
 * 	parsed
 */
gboolean
parse_ipod_id_from_string(gchar **s, guint32 *id)
{
    if((s) && (*s))
    {
	int i = 0;
	gchar buf[4096];
	gchar *new = NULL;
	gchar *str = *s;
	guint max = strlen(str);

	for(i = 0; i < max; i++)
	{
	    if(str[i] == '\n')
	    {
		snprintf(buf, 4096, "%s", str);
		buf[i] = '\0';
		*id = (guint32)atoi(buf);
		if((i+1) < max)
		    new = g_strdup(&str[i+1]);
		break;
	    }
	}
	g_free(str);
	*s = new;
	return(TRUE);
    }
    return(FALSE);
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
 */
void
gtkpod_main_quit(void)
{
  remove_all_playlists ();  /* first remove playlists, then songs!
		    (otherwise non-existing songs may be accessed) */
  remove_all_songs ();
  cleanup_listviews ();
  write_prefs (); /* FIXME: how can we avoid saving options set by
		   * command line? */
  gtk_main_quit ();
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
    
    if(main_window)
    {
	if((w = lookup_widget(main_window, "import_button")))
	{
	    gtk_widget_set_sensitive(w, FALSE);
	}
	if((w = lookup_widget(main_window, "import_itunes_mi")))
	{
	    gtk_widget_set_sensitive(w, FALSE);
	}
    }
}

void
register_gtkpod_main_window(GtkWidget *win)
{
    main_window = win;
}

#define GTKPOD_MKDIR(buf) { \
    if((mkdir(buf, 0755) != 0)) \
    { \
	if(errno != EEXIST) \
	    return(FALSE); \
    } \
}

gboolean
create_ipod_directories(const gchar *ipod_dir)
{
    int i = 0;
    gchar buf[PATH_MAX];
    
    snprintf(buf, PATH_MAX, "%s/iPod_Control", ipod_dir);
    GTKPOD_MKDIR(buf);
    snprintf(buf, PATH_MAX, "%s/iPod_Control/Music", ipod_dir);
    GTKPOD_MKDIR(buf);
    snprintf(buf, PATH_MAX, "%s/iPod_Control/iTunes", ipod_dir);
    GTKPOD_MKDIR(buf);
    
    for(i = 0; i < 20; i++)
    {
	snprintf(buf, PATH_MAX, "%s/iPod_Control/Music/F%02d", ipod_dir, i);
	fprintf(stderr, "Making %s\n", buf);
	GTKPOD_MKDIR(buf);
    }
    return(TRUE);
}
