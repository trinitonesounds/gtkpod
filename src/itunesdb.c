/*
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
| 
|  URL: http://gtkpod.sourceforge.net/
| 
|  Most of the code in this file has been ported from the perl
|  script "mktunes.pl" (part of the gnupod-tools collection) written
|  by Adrian Ulrich <pab at blinkenlights.ch>.
|
|  gnupod-tools: http://www.blinkenlights.ch/cgi-bin/fm.pl?get=ipod
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

/* Some notes on how to use the functions in this file:


   *** Reading the iTunesDB ***

   gboolean itunesdb_parse (gchar *path); /+ path to mountpoint /+
   will read an iTunesDB and pass the data over to your program. Your
   programm is responsible to keep a representation of the data.

   For each song itunesdb_parse() will pass a filled out Song structure
   to "add_song()", which has to be provided. The minimal Song
   structure looks like this (feel free to have add_song() do with it
   as it pleases -- and yes, you are responsible to free the memory):

   typedef struct
   {
     gunichar2 *album_utf16;    /+ album (utf16)         /+
     gunichar2 *artist_utf16;   /+ artist (utf16)        /+
     gunichar2 *title_utf16;    /+ title (utf16)         /+
     gunichar2 *genre_utf16;    /+ genre (utf16)         /+
     gunichar2 *comment_utf16;  /+ comment (utf16)       /+
     gunichar2 *composer_utf16; /+ Composer (utf16)      /+
     gunichar2 *fdesc_utf16;    /+ ? (utf16)             /+
     gunichar2 *ipod_path_utf16;/+ name of file on iPod: uses ":" instead of "/" /+
     guint32 ipod_id;           /+ unique ID of song     /+
     gint32  size;              /+ size of file in bytes /+
     gint32  songlen;           /+ Length of song in ms  /+
     gint32  cd_nr;             /+ CD number             /+
     gint32  cds;               /+ number of CDs         /+
     gint32  track_nr;          /+ track number          /+
     gint32  tracks;            /+ number of tracks      /+
     gint32  year;              /+ year                  /+
     gint32  bitrate;           /+ bitrate               /+
     gboolean transferred;      /+ has file been transferred to iPod? /+
   } Song;

   "transferred" will be set to TRUE because all songs read from a
   iTunesDB are obviously (or hopefully) already transferred to the
   iPod.

   By #defining ITUNESDB_PROVIDE_UTF8, itunesdb_parse() will also
   provide utf8 versions of the above utf16 strings. You must then add
   members "gchar *album"... to the Song structure.

   For each new playlist in the iTunesDB, add_playlist() is
   called with a pointer to the following Playlist struct:

   typedef struct
   {
     gunichar2 *name_utf16;
     guint32 type;         /+ 1: master play list (PL_TYPE_MPL) /+
   } Playlist;

   Again, by #defining ITUNESDB_PROVIDE_UTF8, a member "gchar *name"
   will be initialized with a utf8 version of the playlist name.

   add_playlist() must return a pointer under which it wants the
   playlist to be referenced when add_song_to_playlist() is called.

   For each song in the playlist, add_songid_to_playlist() is called
   with the above mentioned pointer to the playlist and the songid to
   be added.

   gboolean add_song (Song *song);
   Playlist *add_playlist (Playlist *plitem);
   void add_songid_to_playlist (Playlist *plitem, guint32 id);


   *** Writing the iTunesDB ***

   gboolean itunesdb_write (gchar *path), /+ path to mountpoint /+
   will write an updated version of the iTunesDB.

   It uses the following functions to retrieve the data necessary data
   from memory:

   guint get_nr_of_songs (void);
   Song *get_song_by_nr (guint32 n);
   guint32 get_nr_of_playlists (void);
   Playlist *get_playlist_by_nr (guint32 n);
   guint32 get_nr_of_songs_in_playlist (Playlist *plitem);
   Song *get_song_in_playlist_by_nr (Playlist *plitem, guint32 n);

   The master playlist is expected to be "get_playlist_by_nr(0)". Only
   the utf16 strings in the Playlist and Song struct are being used.

   Please note that non-transferred songs are not automatically
   transferred to the iPod. A function

   gboolean copy_song_to_ipod (gchar *path, Song *song, gchar *pcfile)

   is provided to help you do that, however.

   Jorg Schuler, 19.12.2002 */



#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <support.h>
#include <stdlib.h>

#include "misc.h"
#include "itunesdb.h"

/* We instruct itunesdb_parse to provide utf8 versions of the strings */
#define ITUNESDB_PROVIDE_UTF8

#define ITUNESDB_DEBUG 0
/* call itunesdb_parse () to read the iTunesDB  */
/* call itunesdb_write () to write the iTunesDB */

/* Header definitions for parsing the iTunesDB */
gchar ipodmagic[] = {'m', 'h', 'b', 'd', 0x68, 0x00, 0x00, 0x00};
static guint32 utf16_strlen(gunichar2 *utf16_string);

/* concat_dir (): concats "dir" and "file" into full filename, taking
   into account that "dir" may or may not end with "/".
   You must free the return string after use. Defined in misc.c */



/* Compare the two data. TRUE if identical */
static gboolean cmp_n_bytes (gchar *data1, gchar *data2, gint n)
{
  gint i;

  for(i=0; i<n; ++i)
    {
      if (data1[i] != data2[i]) return FALSE;
    }
  return TRUE;
}


/* Seeks to position "seek", then reads "n" bytes. Returns -1 on error
   during seek, or the number of bytes actually read */
static gint seek_get_n_bytes (FILE *file, gchar *data, glong seek, gint n)
{
  gint i;
  gint read;

  if (fseek (file, seek, SEEK_SET) != 0) return -1;

  for(i=0; i<n; ++i)
    {
      read = fgetc (file);
      if (read == EOF) return i;
      *data++ = (gchar)read;
    }
  return i;
}


/* Get the 4-byte-number stored at position "seek" in "file"
   (or -1 when an error occured) */
static gint32 get4int(FILE *file, glong seek)
{
  guchar data[4];
  gint32 n;

  if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
  n =  ((guint32)data[3]) << 24;
  n += ((guint32)data[2]) << 16;
  n += ((guint32)data[1]) << 8;
  n += ((guint32)data[0]);
  return n;
}




/* Fix UTF16 String for BIGENDIAN machines (like PPC) */
static gunichar2 *fixup_utf16(gunichar2 *utf16_string) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
gint32 i;
 for(i=0; i<utf16_strlen(utf16_string); i++)
 {
    utf16_string[i] = ((utf16_string[i]<<8) & 0xff00) | 
	                ((utf16_string[i]>>8) & 0xff);
 
 }
#endif
return utf16_string;
}


/* return the length of the header *ml, the genre number *mty,
   and a string with the entry (in UTF16?). After use you must
   free the string with g_free (). Returns NULL in case of error. */
static gunichar2 *get_mhod (FILE *file, glong seek, gint32 *ml, gint32 *mty)
{
  gchar data[4];
  gunichar2 *entry_utf16;
  gint32 xl;

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhod seek: %x\n", (int)seek);
#endif

  if (seek_get_n_bytes (file, data, seek, 4) != 4) 
    {
      *ml = -1;
      return NULL;
    }
  if (cmp_n_bytes (data, "mhod", 4) == FALSE )
    {
      *ml = -1;
      return NULL;
    }
  *ml = get4int (file, seek+8);       /* length         */
  *mty = get4int (file, seek+12);     /* mhod_id number */
  xl = get4int (file, seek+28);       /* entry length   */

#if ITUNESDB_DEBUG
  fprintf(stderr, "ml: %x mty: %x, xl: %x\n", *ml, *mty, xl);
#endif

  entry_utf16 = g_malloc (xl+2);
  if (seek_get_n_bytes (file, (gchar *)entry_utf16, seek+40, xl) != xl) {
    g_free (entry_utf16);
    *ml = -1;
    return NULL;
  }
  entry_utf16[xl/2] = 0; /* add trailing 0 */

  return fixup_utf16(entry_utf16);
}



/* get a PL, return pos where next PL should be, name and content */
static glong get_pl(FILE *file, glong seek) 
{
  gunichar2 *plname_utf16;
#ifdef ITUNESDB_PROVIDE_UTF8
  gchar *plname_utf8;
#endif ITUNESDB_PROVIDE_UTF8
  guint32 type, pltype, songnum, n;
  guint32 nextseek;
  gint32 zip;
  Playlist *plitem;
  guint32 ref;

  gchar data[4];


#if ITUNESDB_DEBUG
  fprintf(stderr, "mhyp seek: %x\n", (int)seek);
#endif

  if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
  if (cmp_n_bytes (data, "mhyp", 4) == FALSE)      return -1; /* not pl */
  pltype = get4int (file, seek+20);  /* Type of playlist (1= MPL) */
  songnum = get4int (file, seek+16); /* number of songs in playlist */
  nextseek = seek + get4int (file, seek+8); /* possible begin of next PL */
  seek += (680+76); /* skip uninteresting header info */
  if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
  plname_utf16 = get_mhod(file, seek, &zip, &type); /* PL name */
  if (zip == -1)
    {   /* we did not read a valid mhod header -> name is invalid */
        /* we simply make up our own name */
      if (pltype == 1)
	plname_utf16 = g_utf8_to_utf16 (_("Master-PL"),
					-1, NULL, NULL, NULL);
      else plname_utf16 = g_utf8_to_utf16 (_("Playlist"),
					    -1, NULL, NULL, NULL);
#ifdef ITUNESDB_PROVIDE_UTF8
      plname_utf8 = g_utf16_to_utf8 (plname_utf16, -1, NULL, NULL, NULL);
#endif ITUNESDB_PROVIDE_UTF8
    }
  else
    {
#ifdef ITUNESDB_PROVIDE_UTF8
      plname_utf8 = g_utf16_to_utf8(plname_utf16, -1, NULL, NULL, NULL);
#endif ITUNESDB_PROVIDE_UTF8
      seek += zip;
    }

#if ITUNESDB_DEBUG
  fprintf(stderr, "pln: %s(%d Tracks) \n", plname_utf8, (int)songnum);
#endif

  plitem = g_malloc0 (sizeof (Playlist));

#ifdef ITUNESDB_PROVIDE_UTF8
  plitem->name = plname_utf8;
#endif ITUNESDB_PROVIDE_UTF8
  plitem->name_utf16 = plname_utf16;
  plitem->type = pltype;

  /* create new playlist */
  plitem = add_playlist(plitem);

  n = 0;  /* number of songs read */
  while (n < songnum)
    {
      /* We read the mhip headers and skip everything else. If we
	 find a mhyp header before n==songnum, something is wrong */
      if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
      if (cmp_n_bytes (data, "mhyp", 4) == TRUE) return -1; /* Wrong!!! */
      if (cmp_n_bytes (data, "mhip", 4) == TRUE)
	{
	  ref = get4int(file, seek+24);
	  add_songid_to_playlist(plitem, ref);
	  ++n;
	}
      seek += get4int (file, seek+8);
    }
  return nextseek;
}


static glong get_nod_a(FILE *file, glong seek)
{
  Song *song;
  gchar data[4];
#ifdef ITUNESDB_PROVIDE_UTF8
  gchar *entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
  gunichar2 *entry_utf16;
  gint type;
  gint zip = 0;

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_nod_a seek: %x\n", (int)seek);
#endif

  if (seek_get_n_bytes (file, data, seek, 4) != 4) return -1;
  if (cmp_n_bytes (data, "mhit", 4) == FALSE ) return -1; /* we are lost! */

  song = g_malloc0 (sizeof (Song));

  song->ipod_id = get4int(file, seek+16);     /* iPod ID          */
  song->size = get4int(file, seek+36);        /* file size        */
  song->songlen = get4int(file, seek+40);     /* time             */
  song->cd_nr = get4int(file, seek+92);       /* CD nr            */
  song->cds = get4int(file, seek+96);         /* CD nr of..       */
  song->track_nr = get4int(file, seek+44);    /* track number     */
  song->tracks = get4int(file, seek+48);      /* nr of tracks     */
  song->year = get4int(file, seek+52);        /* year             */
  song->bitrate = get4int(file, seek+56);     /* bitrate          */
  song->transferred = TRUE;                   /* song is on iPod! */

  seek += 156;                 /* 1st mhod starts here! */
  while(zip != -1)
    {
     seek += zip;
     entry_utf16 = get_mhod (file, seek, &zip, &type);
     if (entry_utf16 != NULL) {
#ifdef ITUNESDB_PROVIDE_UTF8
       entry_utf8 = g_utf16_to_utf8 (entry_utf16, -1, NULL, NULL, NULL);
#endif ITUNESDB_PROVIDE_UTF8
       switch (type)
	 {
	 case MHOD_ID_ALBUM:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->album = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->album_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_ARTIST:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->artist = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->artist_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_TITLE:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->title = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->title_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_GENRE:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->genre = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->genre_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_PATH:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->ipod_path = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->ipod_path_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_FDESC:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->fdesc = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->fdesc_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_COMMENT:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->comment = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->comment_utf16 = entry_utf16;
	   break;
	 case MHOD_ID_COMPOSER:
#ifdef ITUNESDB_PROVIDE_UTF8
	   song->composer = entry_utf8;
#endif ITUNESDB_PROVIDE_UTF8
	   song->composer_utf16 = entry_utf16;
	   break;
	 default: /* unknown entry -- discard */
#ifdef ITUNESDB_PROVIDE_UTF8
	   g_free (entry_utf8);
#endif ITUNESDB_PROVIDE_UTF8
	   g_free (entry_utf16);
	   break;
	 }
     }
    }
  add_song (song);
  return seek;   /* no more black magic */
}


/* Parse the iTunesDB and store the songs 
   using add_song () defined in song.c. 
   Returns TRUE on success, FALSE on error.
   "path" should point to the mount point of the
   iPod, e.e. "/mnt/ipod" */
/* Support for playlists should be added later */
gboolean itunesdb_parse (gchar *path)
{
  gchar *filename = NULL;
  gboolean result;

  filename = concat_dir (path, "iPod_Control/iTunes/iTunesDB");
  result = itunesdb_parse_file (filename);
  if (filename)  g_free (filename);
  return result;
}

/* Same as itunesdb_parse(), but let's specify the filename directly */
gboolean itunesdb_parse_file (gchar *filename)
{
  FILE *itunes = NULL;
  gboolean result = FALSE;
  gchar data[8];
  glong seek;

#if ITUNESDB_DEBUG
  fprintf(stderr, "Parsing %s\nenter: %4d\n", filename, get_nr_of_songs ());
#endif

  itunes = fopen (filename, "r");
  do { /* dummy loop for easier error handling */
    if (itunes == NULL)
      {
	gtkpod_warning (_("Could not open iTunesDB \"%s\" for reading.\n"),
			filename);
	break;
      }
    if (seek_get_n_bytes (itunes, data, 0, 8) != 8)
      {
	gtkpod_warning (_("Error reading \"%s\".\n"), filename);
	break;
      }
    /* for(i=0; i<8; ++i)  printf("%02x ", data[i]); printf("\n");*/
    if (cmp_n_bytes (data, ipodmagic, 8) == FALSE) 
      {  
	gtkpod_warning (_("\"%s\" is not a iTunesDB.\n"), filename);
	break;
      }
    seek = 292; /* the magic number!! (the HARDCODED start of the first mhit) */
    /* get every file entry */
    while(seek != -1) {
      /* get_nod_a returns where it's guessing the next MHIT,
	 if it fails, it returns '-1' */
      seek = get_nod_a(itunes, seek);
    }
    
    /* Parse Playlists */
    /* FIXME: maybe better to skip the headers until we find "mhyp" */
    seek = get4int(itunes, 112) + 292; /* start position of playlists */

#if ITUNESDB_DEBUG
    fprintf(stderr, "iTunesDB part2 starts at: %x\n", (int)seek);
#endif
    
    while(seek != -1) {
     seek = get_pl(itunes, seek);
    }
    
    result = TRUE;
  } while (FALSE);

  if (itunes != NULL)     fclose (itunes);
#if ITUNESDB_DEBUG
  fprintf(stderr, "exit:  %4d\n", get_nr_of_songs ());
#endif 
  return result;
}


/* up to here we had the routines for reading the iTunesDB                */
/* ---------------------------------------------------------------------- */
/* from here on we have the routines for writing the iTunesDB             */

/* Name of the device in utf16 */
gunichar2 ipod_name[] = { 'g', 't', 'k', 'p', 'o', 'd', 0 };


/* Get length of utf16 string in number of characters (words) */
static guint32 utf16_strlen (gunichar2 *utf16)
{
  guint32 i=0;
  while (utf16[i] != 0) ++i;
  return i;
}


/* return dummy mac time stamp -- maybe we can improve later */
/* iPod doesn't seem to care...? */
static gint32 mactime()
{
  return 0x8c3abf9b;
}


/* Write 4-byte-integer "n" in correct order to "data".
   "data" must be sufficiently long ... */
static void store4int (guint32 n, guchar *data)
{
  data[3] = (n >> 24) & 0xff;
  data[2] = (n >> 16) & 0xff;
  data[1] = (n >>  8) & 0xff;
  data[0] =  n & 0xff;
}


/* Write "data", "n" bytes long to current position in file.
   Returns TRUE on success, FALSE otherwise */
static gboolean put_data_cur (FILE *file, gchar *data, gint n)
{
  if (fwrite (data, 1, n, file) != n) return FALSE;
  return TRUE;
}

/* Write 4-byte integer "n" to "file".
   Returns TRUE on success, FALSE otherwise */
static gboolean put_4int_cur (FILE *file, guint32 n)
{
  gchar data[4];

  store4int (n, data);
  return put_data_cur (file, data, 4);
}


/* Write 4-byte integer "n" to "file" at position "seek".
   After writing, the file position indicator is set
   to the end of the file.
   Returns TRUE on success, FALSE otherwise */
static gboolean put_4int_seek (FILE *file, guint32 n, gint seek)
{
  gboolean result;

  if (fseek (file, seek, SEEK_SET) != 0) return FALSE;
  result = put_4int_cur (file, n);
  if (fseek (file, 0, SEEK_END) != 0) return FALSE;
  return result;
}


/* Write "n" times 4-byte-zero at current position
   Returns TRUE on success, FALSE otherwise */
static gboolean put_n0_cur (FILE*file, guint32 n)
{
  guint32 i;
  gboolean result = TRUE;

  for (i=0; i<n; ++i)  result &= put_4int_cur (file, 0);
  return result;
}



/* Write out the mhbd header. Size will be written later */
static void mk_mhbd (FILE *file)
{
  put_data_cur (file, "mhbd", 4);
  put_4int_cur (file, 104); /* header size */
  put_4int_cur (file, -1);  /* size of whole mhdb -- fill in later */
  put_4int_cur (file, 1);   /* ? */
  put_4int_cur (file, 1);   /*  - changed to 2 from itunes2 to 3 .. 
				    version? We are iTunes version 1 ;) */
  put_4int_cur (file, 2);   /* ? */
  put_n0_cur (file, 20);    /* dummy space */
}

/* Fill in the missing items of the mhsd header:
   total size and number of mhods */
static void fix_mhbd (FILE *file, glong mhbd_seek, glong cur)
{
  put_4int_seek (file, cur-mhbd_seek, mhbd_seek+8); /* size of whole mhit */
}


/* Write out the mhsd header. Size will be written later */
static void mk_mhsd (FILE *file, guint32 type)
{
  put_data_cur (file, "mhsd", 4);
  put_4int_cur (file, 96);   /* Headersize */
  put_4int_cur (file, -1);   /* size of whole mhsd -- fill in later */
  put_4int_cur (file, type); /* type: 1 = song, 2 = playlist */
  put_n0_cur (file, 20);    /* dummy space */
}  


/* Fill in the missing items of the mhsd header:
   total size and number of mhods */
static void fix_mhsd (FILE *file, glong mhsd_seek, glong cur)
{
  put_4int_seek (file, cur-mhsd_seek, mhsd_seek+8); /* size of whole mhit */
}


/* Write out the mhlt header. */
static void mk_mhlt (FILE *file, guint32 song_num)
{
  put_data_cur (file, "mhlt", 4);
  put_4int_cur (file, 92);         /* Headersize */
  put_4int_cur (file, song_num);   /* songs in this itunesdb */
  put_n0_cur (file, 20);           /* dummy space */
}  


/* Write out the mhit header. Size will be written later */
static void mk_mhit (FILE *file, Song *song)
{
  put_data_cur (file, "mhit", 4);
  put_4int_cur (file, 156);  /* header size */
  put_4int_cur (file, -1);   /* size of whole mhit -- fill in later */
  put_4int_cur (file, -1);   /* nr of mhods in this mhit -- later   */
  put_4int_cur (file, song->ipod_id); /* song index number          */
  put_4int_cur (file, 1);
  put_4int_cur (file, 0);
  put_4int_cur (file, 256);           /* type                       */
  put_4int_cur (file, mactime());     /* timestamp                  */
  put_4int_cur (file, song->size);    /* filesize                   */
  put_4int_cur (file, song->songlen); /* length of song in ms       */
  put_4int_cur (file, song->track_nr);/* track number               */
  put_4int_cur (file, song->tracks);  /* number of tracks           */
  put_4int_cur (file, song->year);    /* the year                   */
  put_4int_cur (file, song->bitrate); /* bitrate                    */
  put_4int_cur (file, 0xac440000);    /* ?                          */
  put_n0_cur (file, 7);               /* dummy space                */
  put_4int_cur (file, song->cd_nr);   /* CD number                  */
  put_4int_cur (file, song->cds);     /* number of CDs              */
  put_4int_cur (file, 0);             /* hardcoded space            */
  put_4int_cur (file, mactime());     /* timestamp                  */
  put_n0_cur (file, 12);              /* dummy space                */
}  


/* Fill in the missing items of the mhit header:
   total size and number of mhods */
static void fix_mhit (FILE *file, glong mhit_seek, glong cur, gint mhod_num)
{
  put_4int_seek (file, cur-mhit_seek, mhit_seek+8); /* size of whole mhit */
  put_4int_seek (file, mhod_num, mhit_seek+12);     /* nr of mhods        */
}


/* Write out one mhod header.
     type: see enum of MHMOD_IDs;
     string: utf16 string to pack
     fqid: will be used for playlists -- use 1 for songs */
static void mk_mhod (FILE *file, guint32 type,
		     gunichar2 *string, guint32 fqid)
{
  guint32 mod;
  guint32 len;

  if (fqid == 1) mod = 40;   /* normal mhod */
  else           mod = 44;   /* playlist entry */

  len = utf16_strlen (string);         /* length of string in _words_     */

  put_data_cur (file, "mhod", 4);      /* header                          */
  put_4int_cur (file, 24);             /* size of header                  */
  put_4int_cur (file, 2*len+mod);      /* size of header + body           */
  put_4int_cur (file, type);           /* type of the entry               */
  put_n0_cur (file, 2);                /* dummy space                     */
  put_4int_cur (file, fqid);           /* refers to this ID if a PL item,
					  otherwise always 1              */
  put_4int_cur (file, 2*len);          /* size of string                  */
  if (type < 100)
    {                                     /* no PL mhod */
      put_n0_cur (file, 2);               /* trash      */
      /* FIXME: this assumes "string" is writable. 
	 However, this might not be the case,
	 e.g. ipod_name might be in read-only mem. */
      string = fixup_utf16(string); 
      put_data_cur (file, (gchar *)string, 2*len); /* the string */
      string = fixup_utf16(string);
    }
  else
    {                                     
      put_n0_cur (file, 3);     /* PL mhods are different ... */
    }
}


/* Write out the mhlp header. Size will be written later */
static void mk_mhlp (FILE *file, guint32 lists)
{
  put_data_cur (file, "mhlp", 4);      /* header                   */
  put_4int_cur (file, 92);             /* size of header           */
  put_4int_cur (file, lists);          /* playlists on iPod (including main!) */
  put_n0_cur (file, 20);               /* dummy space              */
}


/* Fix the mhlp header */
static void fix_mhlp (FILE *file, glong mhlp_seek, gint playlist_num)
{
  put_4int_seek (file, playlist_num, mhlp_seek+8); /* nr of playlists    */
}



/* Write out the "weird" header.
   This seems to be an itunespref thing.. dunno know this
   but if we set everything to 0, itunes doesn't show any data
   even if you drag an mp3 to your ipod: nothing is shown, but itunes
   will copy the file! 
   .. so we create a hardcoded-pref.. this will change in future
   Seems to be a Preferences mhod, every PL has such a thing 
   FIXME !!! */
static void mk_weired (FILE *file)
{
  put_data_cur (file, "mhod", 4);      /* header                   */
  put_4int_cur (file, 0x18);           /* size of header  ?        */
  put_4int_cur (file, 0x0288);         /* size of header + body    */
  put_4int_cur (file, 0x64);           /* type of the entry        */
  put_n0_cur (file, 6);
  put_4int_cur (file, 0x010084);       /* ? */
  put_4int_cur (file, 0x05);           /* ? */
  put_4int_cur (file, 0x09);           /* ? */
  put_4int_cur (file, 0x03);           /* ? */
  put_4int_cur (file, 0x120001);       /* ? */
  put_n0_cur (file, 3);
  put_4int_cur (file, 0xc80002);       /* ? */
  put_n0_cur (file, 3);
  put_4int_cur (file, 0x3c000d);       /* ? */
  put_n0_cur (file, 3);
  put_4int_cur (file, 0x7d0004);       /* ? */
  put_n0_cur (file, 3);
  put_4int_cur (file, 0x7d0003);       /* ? */
  put_n0_cur (file, 3);
  put_4int_cur (file, 0x640008);       /* ? */
  put_n0_cur (file, 3);
  put_4int_cur (file, 0x640017);       /* ? */
  put_4int_cur (file, 0x01);           /* bool? (visible? / colums?) */
  put_n0_cur (file, 2);
  put_4int_cur (file, 0x500014);       /* ? */
  put_4int_cur (file, 0x01);           /* bool? (visible?) */
  put_n0_cur (file, 2);
  put_4int_cur (file, 0x7d0015);       /* ? */
  put_4int_cur (file, 0x01);           /* bool? (visible?) */
  put_n0_cur (file, 114);
}


/* Write out the mhyp header. Size will be written later */
static void mk_mhyp (FILE *file, gunichar2 *listname,
		     guint32 type, guint32 song_num)
{
  put_data_cur (file, "mhyp", 4);      /* header                   */
  put_4int_cur (file, 108);            /* type			   */
  put_4int_cur (file, -1);             /* size -> later            */
  put_4int_cur (file, 2);              /* ?                        */
  put_4int_cur (file, song_num);       /* number of songs in plist -> later */
  put_4int_cur (file, type);           /* 1 = main, 0 = visible    */
  put_4int_cur (file, 0);              /* ?                        */
  put_4int_cur (file, 0);              /* ?                        */
  put_4int_cur (file, 0);              /* ?                        */
  put_n0_cur (file, 18);               /* dummy space              */
  mk_weired (file);
  mk_mhod (file, MHOD_ID_TITLE, listname, 1);
}


/* Fix the mhyp header */
static void fix_mhyp (FILE *file, glong mhyp_seek, glong cur)
{
  put_4int_seek (file, cur-mhyp_seek, mhyp_seek+8);
    /* size */
}


/* Header for new PL item */
static void mk_mhip (FILE *file, guint32 id)
{
  put_data_cur (file, "mhip", 4);
  put_4int_cur (file, 76);
  put_4int_cur (file, 76);
  put_4int_cur (file, 1);
  put_4int_cur (file, 0);
  put_4int_cur (file, id);  /* song id in playlist */
  put_4int_cur (file, id);  /* ditto.. don't know the difference, but this
                               seems to work. Maybe a special ID used for
			       playlists? */
  put_n0_cur (file, 12); 
}

static void
write_mhsd_one(FILE *file)
{
    Song *song;
    guint32 i, song_num, mhod_num;
    glong mhsd_seek, mhit_seek, mhlt_seek; 

    song_num = get_nr_of_songs();

    mhsd_seek = ftell (file);  /* get position of mhsd header */
    mk_mhsd (file, 1);         /* write header: type 1: song  */
    mhlt_seek = ftell (file);  /* get position of mhlt header */
    mk_mhlt (file, song_num);  /* write header with nr. of songs */
    for (i=0; i<song_num; ++i)  /* Write each song */
    {
	if((song = get_song_by_nr (i)) == 0)
	{
	    g_warning ("Invalid song Index!\n");
	    break;
	}
	mhit_seek = ftell (file);
	mk_mhit (file, song);
	mhod_num = 0;
	if (utf16_strlen (song->title_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_TITLE, song->title_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->ipod_path_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_PATH, song->ipod_path_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->album_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_ALBUM, song->album_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->artist_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_ARTIST, song->artist_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->genre_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_GENRE, song->genre_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->fdesc_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_FDESC, song->fdesc_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->comment_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_COMMENT, song->comment_utf16, 1);
	    ++mhod_num;
	}
	if (utf16_strlen (song->composer_utf16) != 0)
	{
	    mk_mhod (file, MHOD_ID_COMPOSER, song->composer_utf16, 1);
	    ++mhod_num;
	}
        /* Fill in the missing items of the mhit header */
	fix_mhit (file, mhit_seek, ftell (file), mhod_num);
    }
    fix_mhsd (file, mhsd_seek, ftell (file));
}

static void
write_playlist(FILE *file, Playlist *pl)
{
    Song *s;
    guint32 i, n;
    glong mhyp_seek;
    gunichar2 empty = 0;
    
    mhyp_seek = ftell(file);
    n = get_nr_of_songs_in_playlist (pl);
#if ITUNESDB_DEBUG
  fprintf(stderr, "Playlist: %s (%d tracks)\n", pl->name, n);
#endif    
    mk_mhyp(file, pl->name_utf16, pl->type, n);  
    for (i=0; i<n; ++i)
    {
        if((s = get_song_in_playlist_by_nr (pl, i)))
	{
	    mk_mhip(file, s->ipod_id);
	    mk_mhod(file, MHOD_ID_PLAYLIST, &empty, s->ipod_id); 
	}
    }
   fix_mhyp (file, mhyp_seek, ftell(file));
}



/* Expects the master playlist to be (get_playlist_by_nr (0)) */
static void
write_mhsd_two(FILE *file)
{
    guint32 playlists, i;
    glong mhsd_seek, mhlp_seek;
  
    mhsd_seek = ftell (file);  /* get position of mhsd header */
    mk_mhsd (file, 2);         /* write header: type 2: playlists  */
    mhlp_seek = ftell (file);
    playlists = get_nr_of_playlists();
    mk_mhlp (file, playlists);
    for(i = 0; i < playlists; i++)
    { 
	write_playlist(file, get_playlist_by_nr(i));
    }
    fix_mhlp (file, mhlp_seek, playlists);
    fix_mhsd (file, mhsd_seek, ftell (file));
}


/* Do the actual writing to the iTunesDB */
gboolean 
write_it (FILE *file)
{
    glong mhbd_seek;

    mhbd_seek = 0;             
    mk_mhbd (file);            
    write_mhsd_one(file);		/* write songs mhsd */
    write_mhsd_two(file);		/* write playlists mhsd */
    fix_mhbd (file, mhbd_seek, ftell (file));
    return TRUE;
}


/* Write out an iTunesDB.
   Note: only the _utf16 entries in the Song-struct are used
   Returns TRUE on success, FALSE on error.
   "path" should point to the mount point of the
   iPod, e.e. "/mnt/ipod" */
gboolean itunesdb_write (gchar *path)
{
    gchar *filename = NULL;
    gboolean result = FALSE;

    filename = concat_dir (path, "iPod_Control/iTunes/iTunesDB");
    result = itunesdb_write_to_file (filename);
    if (filename != NULL) g_free (filename);
    return result;
}

/* Same as itnuesdb_write (), but you specify the filename directly */
gboolean itunesdb_write_to_file (gchar *filename)
{
  FILE *file = NULL;
  gboolean result = FALSE;

#if ITUNESDB_DEBUG
  fprintf(stderr, "Writing to %s\n", filename);
#endif

  if((file = fopen (filename, "w+")))
    {
      write_it (file);
      fclose(file);
      result = TRUE;
    }
  else
    {
      gtkpod_warning (_("Could not open iTunesDB \"%s\" for writing.\n"),
		      filename);
    }
  return result;
}

/* Does this really belong here? -- Maybe, because it
   requires knowledge of the iPod's filestructure */
/* Copy one song to the ipod. The PC-Filename is
   "pcfile" and is taken literally.
   "path" is assumed to be the mountpoint of the iPod.
   For storage, the directories "f00 ... f19" will be
   cycled through. The filename is constructed from
   "song->ipod_id": "gtkpod_id" and written to
   "song->ipod_path_utf8" and "song->ipod_path_utf16" */
gboolean copy_song_to_ipod (gchar *path, Song *song, gchar *pcfile)
{
  static gint dir_num = -1;
  gchar *ipod_file, *ipod_fullfile;
  gboolean success;

  if (dir_num == -1) dir_num = (gint) (19.0*rand()/(RAND_MAX));
  if(song->transferred == TRUE) return TRUE; /* nothing to do */
  if (song == NULL)
    {
      g_warning ("Programming error: copy_song_to_ipod () called NULL-song\n");
      return FALSE;
    }

  /* The iPod seems to need the .mp3 ending to play the song.
     Of course the following line should be changed once gtkpod
     also supports other formats. */
  ipod_file = g_strdup_printf ("/iPod_Control/Music/F%02d/gtkpod%05d.mp3",
			       dir_num, song->ipod_id);
#if ITUNESDB_DEBUG
  fprintf(stderr, "ipod_file: %s\n", ipod_file);
#endif

  ipod_fullfile = concat_dir (path, ipod_file+1);
  success = cp (pcfile, ipod_fullfile);
  if (success)
    { /* need to store ipod_filename */
      gint i, len;
      len = strlen (ipod_file);
      for (i=0; i<len; ++i)     /* replace '/' by ':' */
	if (ipod_file[i] == '/')  ipod_file[i] = ':';
#ifdef ITUNESDB_PROVIDE_UTF8
      song->ipod_path = g_strdup (ipod_file);
#endif ITUNESDB_PROVIDE_UTF8
      song->ipod_path_utf16 = g_utf8_to_utf16 (ipod_file,
					       -1, NULL, NULL, NULL);
      song->transferred = TRUE;
      ++dir_num;
      if (dir_num == 20) dir_num = 0;
    }
  g_free (ipod_file);
  g_free (ipod_fullfile);
  return success;
}


/* Copy file "from_file" to "to_file".
   Returns TRUE on success, FALSE otherwise */
gboolean cp (gchar *from_file, gchar *to_file)
{
  gchar data[ITUNESDB_COPYBLK];
  glong bread, bwrite;
  gboolean success = TRUE;
  FILE *file_in = NULL;
  FILE *file_out = NULL;

  do { /* dummy loop for easier error handling */
    file_in = fopen (from_file, "r");
    if (file_in == NULL)
      {
	gtkpod_warning (_("Could not open file \"%s\" for reading.\n"), from_file);
	success = FALSE;
	break;
      }
    file_out = fopen (to_file, "w");
    if (file_out == NULL)
      {
	gtkpod_warning (_("Could not open file \"%s\" for writing.\n"), to_file);
	success = FALSE;
	break;
      }
    do {
      bread = fread (data, 1, ITUNESDB_COPYBLK, file_in);
      if (bread == 0)
	{
	  if (feof (file_in) == 0)
	    { /* error -- not end of file! */
	      gtkpod_warning (_("Error reading file \"%s\"."), from_file);
	      success = FALSE;
	    }
	}
      else
	{
	  bwrite = fwrite (data, 1, bread, file_out);
	  if (bwrite != bread)
	    {
	      gtkpod_warning (_("Error writing PC file \"%s\"."),to_file);
	      success = FALSE;
	    }
	} 
    } while (success && (bread != 0));
  } while (FALSE);
  if (file_in)  fclose (file_in);
  if (file_out)
    {
      fclose (file_out);
      if (!success) { /* error occured -> delete to_file */
	remove (to_file);
      }
    }
  return success;
}
