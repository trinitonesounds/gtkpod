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
#include "song.h"
#include "interface.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"
#include "itunesdb.h"
#include "display.h"

static GtkWidget *about_window = NULL;


/* Callback after one directory has been added */
void add_dir_selected (gchar *dir)
{
  add_directory_recursively (dir);
  prefs_set_last_dir_dir_browse_for_filename(dir);
}

static void add_files_ok_button (GtkWidget *button, GtkFileSelection *selector)
{
  gchar **names;
  gint i;

  names = gtk_file_selection_get_selections (GTK_FILE_SELECTION (selector));
  for (i=0; names[i] != NULL; ++i)
    {
      add_song_by_filename (names[i]);
      if(!i)
	  prefs_set_last_dir_file_browse_for_filename(names[i]);
    }
  g_strfreev (names);
}

void create_add_files_fileselector (gchar *startdir)
{
  GtkWidget *file_selector;
    
    /* Create the selector */
   file_selector = gtk_file_selection_new (_("Select files or directories to add."));
   gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (file_selector),
					   TRUE);
   gtk_file_selection_set_filename(GTK_FILE_SELECTION (file_selector),
				   startdir);

   g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                     "clicked",
                     G_CALLBACK (add_files_ok_button),
                     file_selector);
   			   
   /* Ensure that the dialog box is destroyed when the user clicks a button. */
   g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy), 
                             (gpointer) file_selector); 

   g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
                             "clicked",
                             G_CALLBACK (gtk_widget_destroy),
                             (gpointer) file_selector); 
   
   /* Display that dialog */
   gtk_widget_show (file_selector);
}



/* Concats "dir" and "file" into full filename, taking
   into account that "dir" may or may not end with "/".
   You must free the return string after use */
gchar *concat_dir (G_CONST_RETURN gchar *dir, G_CONST_RETURN gchar *file)
{
  if (file[strlen(file)-1] == '/')
    {
      return g_strdup_printf ("%s%s", dir, file);
    }
  else
    {
      return g_strdup_printf ("%s/%s", dir, file);
    }
}



/* used to handle export of database */
void handle_export ()
{
  if (flush_songs () == FALSE) return; /* some error occured
					  while writing files to iPod */
  itunesdb_write (cfg->ipod_mount);    /* write iTunesDB to iPod */
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

/**
 * gtkpod_main_quit
 */
void
gtkpod_main_quit(void)
{
  remove_all_playlists ();
  remove_all_songs ();
  cleanup_listviews ();
  write_prefs ();
  gtk_main_quit ();
}
