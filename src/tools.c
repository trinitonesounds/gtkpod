/* Time-stamp: <2003-09-07 20:52:57 jcs>
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

/* this function return the @song gain in dB */
/* mp3gain version 1.4.2 */
gint get_song_gain(Song *song)
{
   /*pipe's definition*/
    enum {
	READ = 0,
	WRITE = 1
    };
   #define BUFLEN 200 /*FIXME this should be a [safe/runtime detected] value*/
   gint i,j,k,l,n;  /*for counter*/
   gint len = 0;
   gint gain = SONGGAINERROR;
   gint tab = 0;
   gint fdpipe[2];  /*a pipe*/
   gboolean end_line=FALSE;
   gchar *filename=NULL;
   gchar gain_char[BUFLEN];
   gchar gain_temp[BUFLEN];
   gchar num[BUFLEN];
   gchar *dust=NULL;
   gchar *mp3gain_path=NULL;
   gchar *mp3gain=NULL;
   pid_t pid,tpid;

   k=0;
   n=0;

   memset(gain_char,0,BUFLEN);
   memset(gain_temp,0,BUFLEN);
   memset(num,0,BUFLEN);

   mp3gain_path = which ("mp3gain");
   if (!mp3gain_path)
   {
       gtkpod_warning (_("Could not find mp3gain executable."));
       return SONGGAINERROR;
   }
   mp3gain = g_strdup ("mp3gain");

   /*start sed s/:/\//*/
   filename=get_track_name_on_disk_verified (song);
   for(i=0; filename && (i<strlen(filename)); i++){
      if (filename[i] == ':')  filename[i] = '/';
   }
   /* end sed*/
   
   /*FIXME: libglib2.0 documention says this is not safe*/
   /*what if mp3gain changes attribute or is deleted before the execution?*/
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
               execl(mp3gain_path,mp3gain,"-s s","-o",filename,NULL);
            }
            break; 
         default: /*parent*/
            close(fdpipe[WRITE]);
            tpid = waitpid (pid, NULL, 0); /*wait mp3gain termination FIXME: it freeze gtkpod*/
            len=read(fdpipe[READ],gain_char,BUFLEN);
            close(fdpipe[READ]);
            /*now gain_char contain the mp3gain stdout*/
            /*let's parse it*/
            /* mp3gain output for a single file is something like this:
             * FIRST LINE (header)
             * FILENAME\tMP3GAIN\tOTHER NOT INTERESTING OUTPUT\n
             * ALBUM\tALBUMGAIN\tOTHER OUTPUT\n
             * */
            /*FIXME this parsing code is ugly*/
            /*delete the first line*/
            for(j=0;j<len;j++)
            {
               if(end_line)
               {
                  gain_temp[k]=gain_char[j];
                  k++;
               }
               if(gain_char[j]=='\n'&&!end_line) /*we have passed the first line*/
               {
                  end_line=TRUE;
                  i=j;
                  i++;
               }
            }
            /*extract the mp3gain (extract from the first \t to the next \t)*/
            l=0;
            /*FIXME this parsing code is VERY ugly*/
            while(l<k&&tab!=2)
            {
               if(gain_temp[l]=='\t')
               {
                  tab++;
               }
               if(tab==1&&gain_temp[l]!='\t')/*we have passed the first tab*/
               {
                  num[n]=gain_temp[l];
                  n++;
               }
               if(tab==2&&gain_temp[l]=='\t')
               {
                  num[n+1]='\0'; /*atoi need this to work*/
               }
               l++;
            }
            /*convert the mp3gain*/
            if(n>0)
            {
               gain=atoi(num);
            }
            break;
          }/*end switch*/
   } /*end if*/

   /*free everything left*/
   g_free(filename);
   g_free(mp3gain_path);
   g_free(mp3gain);
   g_free(dust);
   g_free(num);

   /*and happily return the right value*/
   return gain;
   
}

/* normalize all the songs in the @list, it does work if @list is NULL */
/*FIXME: it should be able to get the album gain*/
void normalize_songs_list(GList *list)
{
   Song *song;
   gint volume=0;
   gint songs_nr=0;
   gchar *str=NULL;
   gint n=0;
   gboolean changed=FALSE;
   
   block_widgets ();
   list=g_list_first(list);
   songs_nr=g_list_length(list);
   if(songs_nr>0){ /*else @list==NULL*/
      do{
         song=list->data;
         volume=get_song_gain (song);
	 if (volume == SONGGAINERROR)   break;
         n++;
         if((song->volume)!=volume)
         {
            song->volume=volume;
            changed=TRUE; /*something is changed*/
         }
	 str=g_strdup_printf (ngettext (_("Normalized %d/%d song."),
					_("Normalized %d/%d songs."), n),
			      n,songs_nr);
	 gtkpod_statusbar_message (str);
	 C_FREE (str);
	 while (widgets_blocked && gtk_events_pending ()) 
	     gtk_main_iteration ();
      }
      while((list=g_list_next(list)));
   }
   
   if (volume == SONGGAINERROR)
   {
       gtkpod_statusbar_message (_("Normalization unsuccessful."));
   }
   else
   {
       if(changed==TRUE)
       {
	   data_changed();/*let the world knows*/
	   gtkpod_statusbar_message (_("Normalization completed."));
       }
       else if(changed==FALSE)
       {
	   gtkpod_statusbar_message (_("No changes occurred."));
       }
   }
   release_widgets ();
}

/* normalize all the songs in the iPod*/
void normalize_iPod_songs(void)
{
   Playlist *plitem;
   plitem=get_playlist_by_nr(0);
   normalize_songs_list(plitem->members);
}

/*normalize the newly inserted songs, if @zero_gain is true
 * this function normalize even the songs that have 0 in the gain's field*/
void normalize_new_songs(gboolean zero_gain)
{
   GList *songs=NULL;
   gint i=0;
   Song *song;

   while ((song=get_next_song(i)))
   {
      i=1; /* for get_next_song() */
      /*FIXME a non transferred songs doesn't have a song->ipod_path*/
//      if(!(song->transferred)||(zero_gain&&song->volume==0)) /*if not transferred-> it's a new songs ;-)*/
      if((zero_gain&&song->volume==0))
      {
         songs = g_list_append(songs, song);
      }
   }
   normalize_songs_list(songs);
}
