/* Time-stamp: <2003-09-27 01:11:19 jcs>
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


#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <stdlib.h>
#include "normalize.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"

#ifdef G_THREADS_ENABLED
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif

/* parse the mp3gain stdout to search the mp3gain output:
 * 
 * mp3gain stdout for a single file is something like this:
 * FIRST LINE (header)
 * FILENAME\tMP3GAIN\tOTHER NOT INTERESTING OUTPUT\n
 * ALBUM\tALBUMGAIN\tOTHER OUTPUT\n
 * 
 * we want to extract only the right file's MP3GAIN
 *
 * BEWARE: mp3gain doesn't separate stdout/stderror */
gint parse_mp3gain_stdout(gchar *mp3gain_stdout, gchar *songsfile)
{
   gint found=FALSE;
   gint gain=SONGGAINERROR;
   /*they are just pointers, don't need to be freed*/
   gchar *filename=NULL;
   gchar *num=NULL;
  
   filename=strtok((gchar *)mp3gain_stdout,"\t\n\0");
   while(!found&&filename!=NULL&&((gchar *)mp3gain_stdout)!=NULL)
   {
#ifdef MP3GAIN_PARSE_DEBUG
      printf("mp3gain_stdout %s",mp3gain_stdout);
      printf("filename %s",filename);
#endif
      if(strcmp((gchar *)songsfile,filename)==0)
      {
         num=strtok(NULL,"\t");
         strcat(num,"\0");
         gain=atoi(num);
         found=TRUE;
      }
      else{
         strtok(NULL,"\n\t\0");
         filename=strtok(NULL,"\t\n\0");
      }
#ifdef MP3GAIN_PARSE_DEBUG
      printf("mp3gain_stdout %s",mp3gain_stdout);
      printf("filename %s",filename);
#endif
   }
   return gain;
}

/* this function return the @song gain in dB */
/* mp3gain version 1.4.2 */
gint normalize_get_gain(Song *song)
{
   /*pipe's definition*/
    enum {
	READ = 0,
	WRITE = 1
    };
   #define BUFLEN 200 /*FIXME this should be a [safe/runtime detected] value*/
   gint k,n;  /*for's counter*/
   gint len = 0;
   gint gain = SONGGAINERROR;
   gint fdpipe[2];  /*a pipe*/
   gchar *filename=NULL; /*song's filename*/
   gchar gain_char[BUFLEN+1];
   gchar *mp3gain_path=NULL;
   gchar *mp3gain=NULL;
   pid_t pid,tpid;

   k=0;
   n=0;

   memset(gain_char,0,BUFLEN);

   mp3gain = g_strdup ("mp3gain");
   mp3gain_path = which (mp3gain);
   if (!mp3gain_path)
   {
       gtkpod_warning (_("Could not find mp3gain executable."));
       return SONGGAINERROR;
   }

   filename=get_track_name_on_disk_verified (song);
   
   /*FIXME: libglib2.0 documention says this is not safe*/
   /*what if mp3gain changes attribute or is deleted before the execution?*/
   //if(g_file_test(mp3gain_path,G_FILE_TEST_IS_EXECUTABLE))
   if(g_file_test(mp3gain_path,G_FILE_TEST_IS_EXECUTABLE))
   {
      /*create the pipe*/
      pipe(fdpipe);
      /*than fork*/
      pid=fork();
   
      /*and cast mp3gain*/
      switch (pid)
      {
         case -1: /* parent and error, now what?*/
	    break;
         case 0: /*child*/
            close(1); /*close stdout*/
            dup2(fdpipe[WRITE],fileno(stdout));
            close(fdpipe[READ]);
            close(fdpipe[WRITE]);
            if(prefs_get_write_gaintag())
            {
               execl(mp3gain_path,mp3gain,"-o",filename,NULL); /*this write on mp3file!!*/
            }
            else
            {
               execl(mp3gain_path, mp3gain, "-s", "s", "-o", filename, NULL);
            }
            break; 
         default: /*parent*/
            close(fdpipe[WRITE]);
            tpid = waitpid (pid, NULL, 0); /*wait mp3gain termination FIXME: it freeze gtkpod*/
            len=read(fdpipe[READ],gain_char,BUFLEN);
            close(fdpipe[READ]);
            /*now gain_char contain the mp3gain stdout*/
            gain = parse_mp3gain_stdout(gain_char,filename);
            break;
          }/*end switch*/
   } /*end if*/

   /*free everything left*/
   g_free(filename);
   g_free(mp3gain_path);
   g_free(mp3gain);

   /*and happily return the right value*/
   return gain;
   
}

#ifdef G_THREADS_ENABLED
/* Threaded getSongGain*/
static gpointer th_normalize_get_gain (gpointer song)
{
   gint volume=normalize_get_gain((Song *)song);
   g_mutex_lock (mutex);
   mutex_data = TRUE; /* signal that thread will end */
   g_cond_signal (cond);
   g_mutex_unlock (mutex);
   return (gpointer)volume;
}
#endif 

/* normalize the newly inserted songs (i.e. non-transferred songs) */
void normalize_new_songs (void)
{
    GList *songs=NULL;
    gint i=0;
    Song *song;

    while ((song=get_next_song(i)))
    {
	i=1; /* for get_next_song() */
	if(!song->transferred)
	{
	    songs = g_list_append(songs, song);
	}
    }
    normalize_songs_list(songs);
    g_list_free (songs);
}

static void normalization_abort(gboolean *abort)
{
   *abort=TRUE;
}

void normalize_songs_list(GList *list)
{
  gint count, n, nrs;
  gchar *buf;
  Song  *song;
  gint new_gain=0;
  static gboolean abort;
  GtkWidget *dialog, *progress_bar, *label, *image, *hbox;
  time_t diff, start, mins, secs;
  char *progtext = NULL;

#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  block_widgets ();

  /* create the dialog window */
  dialog = gtk_dialog_new_with_buttons (_("Information"),
                                         GTK_WINDOW (gtkpod_window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_NONE,
                                         NULL);

  
  /* emulate gtk_message_dialog_new */
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
				    GTK_ICON_SIZE_DIALOG);
  label = gtk_label_new (
      _("Press button to abort."));

  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  /* hbox to put the image+label in */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* Create the progress bar */
  progress_bar = gtk_progress_bar_new ();
  progtext = g_strdup (_("Normalizing..."));
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
  g_free (progtext);

  /* Indicate that user wants to abort */
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (normalization_abort),
			    &abort);

  /* Add the image/label + progress bar to dialog */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      progress_bar, FALSE, FALSE, 0);
  gtk_widget_show_all (dialog);

  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

  /* count number of songs to be normalized */
  n = g_list_length(list);
  count = 0; /* songs normalized */
  nrs = 0;
  abort = FALSE;
  start = time(NULL);

  if(n==0)
  {
     /* FIXME we should tell something*/

  }
  while (!abort &&  (list!=NULL)) /*FIXME:change it in a do-while cycle*/
  {
     song=list->data;
     if (song->transferred)
     {
#ifdef G_THREADS_ENABLED
        mutex_data = FALSE;
	thread = g_thread_create (th_normalize_get_gain, song, TRUE, NULL);
	if (thread)
	{
           gboolean first_abort = TRUE;
           g_mutex_lock (mutex);
	   do
           {
              while (widgets_blocked && gtk_events_pending ())
                 gtk_main_iteration ();
              /* wait a maximum of 10 ms */

	      if (abort && first_abort)
	      {
		  first_abort = FALSE;
		  progtext = g_strdup (_("Aborting..."));
		  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
					    progtext);
		  g_free (progtext);
		  gtkpod_statusbar_message(_("Will abort after current mp3gain process ends."));
		  while (widgets_blocked && gtk_events_pending ())
		      gtk_main_iteration ();
	      }
              g_get_current_time (&gtime);
              g_time_val_add (&gtime, 20000);
	      g_cond_timed_wait (cond, mutex, &gtime);
           }
           while(!mutex_data);
           new_gain = (gint)g_thread_join (thread);
           g_mutex_unlock (mutex);
        }
        else
        {
           g_warning ("Thread creation failed, falling back to default.\n");
           new_gain=normalize_get_gain(song);
        }
#else
        new_gain=normalize_get_gain(song);
#endif 

/*normalization part*/
        if(new_gain==SONGGAINERROR)
        {
           abort=TRUE;
        }
        else
        {
           if(new_gain!=song->volume)
           {
              song->volume=new_gain;
	      pm_song_changed (song);
	      data_changed ();
           }
        }
/*end normalization*/
        
        ++count;
        if (count == 1)  /* we need ***much*** longer timeout */
           prefs_set_statusbar_timeout (30*STATUSBAR_TIMEOUT);

        buf = g_strdup_printf (ngettext ("Normalized %d of %d new song.",
					       "Normalized %d of %d new songs.", n),
                                         count, n);
        gtkpod_statusbar_message(buf);
        g_free (buf);

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress_bar),
                                        (gdouble) count/n);

        diff = time(NULL) - start;
        mins = ((diff*n/count)-diff)/60;
        secs = ((((diff*n/count)-diff) & 60) / 5) * 5;
      /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
        progtext = g_strdup_printf (_("%d%% (%d:%02d) left"),
	 		  count*100/n, (int)mins, (int)secs);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
					progtext);
	g_free (progtext);
     } /*end if transferred*/

     if (abort && (count != n))
        gtkpod_statusbar_message (_("Some songs were not normalized. Normalization aborted!"));
     while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
     list=g_list_next(list);
  } /*end while*/

  prefs_set_statusbar_timeout (0);

  gtk_widget_destroy (dialog);
  release_widgets();
}
