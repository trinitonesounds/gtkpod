/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
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

#include <stdio.h>
#include <limits.h>
#include <openssl/sha.h>
#include "md5.h"
#include "prefs.h"
#include "misc.h"
#include "support.h"

/**
 * Create and manage a string hash for files on disk
 */

/** 
 * NR_PATH_MAX_BLOCKS
 * A seed of sorts for SHA1, if collisions occur increasing this value
 * should give more unique data to SHA1 as more of the file is read
 * This value is multiplied by PATH_MAX to determine how many bytes are read
 */
#define NR_PATH_MAX_BLOCKS 4

/** 
 * filehash
 * a string hash of the files on the ipod already, or are already registered
 * to be copied on next export
 * */
static GHashTable *filehash = NULL;

/**
 * do_hash_on_file - read PATH_MAX * NR_PATH_MAX_BLOCKS bytes from the file
 * and ask sha1 for a hash of it, convert this has to a string of hex output
 * @fp - an open file descriptor to read from
 * Returns - A Hash String
 */
gchar *
do_hash_on_file(FILE *fp)
{
    glong bread;
    gchar *result = NULL;
    unsigned char md[SHA_DIGEST_LENGTH]; /* 20 Bytes */
    int blocks = NR_PATH_MAX_BLOCKS*2;
    char file_chunk[PATH_MAX * blocks];
	    
    bread = fread(file_chunk, 1, PATH_MAX * blocks, fp);
    if(SHA1(file_chunk, bread, &md[0]))
    {
	char buf[PATH_MAX];		/* the copy buffer */
	int i = 0, last = 0; 
	/**
	 * md guarantees twenty bytes of data, and we want to treat it as a
	 * string.  since NULL might be in there, sprintf would fail,
	 * snprintf it in hex format for all twenty 
	 */
	for(i = 0; i < 20; i++)
	{
	  /* This should produce a string 40 long */
	    last += snprintf(&buf[last], 4,"%02x", md[i]);
	}
	buf[last] = '\0';
	result = g_strdup(buf);
    }
    return(result);
}

/**
 * Generate a unique hash for the Song passed in
 * @s - The Song data structure, we want to hash based on the file on disk
 * Returns - an SHA1 hash in string format, is the hex output from the hash
 */
static gchar *
hash_song(Song *s)
{
    FILE *fp;
    gchar *result = NULL;
    gchar *filename = NULL;
    
    if(s != NULL)
       {
	 if (s->md5_hash != NULL)
	   {
	     result = g_strdup (s->md5_hash);
	   }
	 else if ((filename = get_song_name_on_disk(s)) != NULL)
	   {
	     if ((fp = fopen(filename, "r")))
	       {
		 result = do_hash_on_file(fp);
		 fclose(fp);
	       }
	     else
	       {
		 gtkpod_warning (_("Unable to open file \"%s\"\n"), filename);
	       }
	     g_free(filename);
	   }
       }
    return(result);
}

/**
 * Initialize the filelist hash table and insert all the Songs passed in
 * @songlist - A GList with data attribute of type Song *
 */
void
unique_file_repository_init(GList *songlist)
{
    Song *s = NULL;
    GList *l = NULL;
    gchar *val = NULL;
    
    if(cfg->md5songs)
    {
	if(filehash) 
	    unique_file_repository_free();
	filehash = g_hash_table_new_full (g_str_hash, g_str_equal,
					  g_free, NULL);
	/* populate the hash table */
	for(l = songlist; l; l = l->next)
	{
	    s = (Song*)l->data;
	    if ((val = hash_song(s)) != NULL)
	      {
		if (s->md5_hash == NULL)
		  s->md5_hash = g_strdup (val);
		g_hash_table_insert(filehash, val, s);
	      }
	}
    }
}


/**
 * Free up the dynamically allocated memory in this table
 */
void
unique_file_repository_free(void)
{
    if((cfg->md5songs) && (filehash))
    {
	g_hash_table_destroy(filehash);
	filehash = NULL;
    }
}

/**
 * Check to see if a song has already been added to the ipod
 * @s - the Song we want to know about
 * Returns the filename (or other tag) if the song is already 
 * on the ipod, NULL otherwise
 */
gchar *song_exists_on_ipod(Song *s)
{
    gchar *val = NULL;
    gchar *result = NULL;
    Song *song;

    if((cfg->md5songs) && (filehash))
      {
	val = hash_song(s);
	if (val != NULL)
	  {
	    if((song = g_hash_table_lookup(filehash, val)))
	      {
		g_free (val);
		if (song->pc_path_utf8 && strlen(song->pc_path_utf8))
		    result = song->pc_path_utf8;
		else if ((song->title && strlen(song->title)))
		    result = song->title;
		else if ((song->album && strlen(song->album)))
		    result = song->album;
		else if ((song->artist && strlen(song->artist)))
		    result = song->artist;
		else result = "";
	      }
	    else	/* if it doesn't exist we register it in the hash */
	      {
		if (s->md5_hash == NULL)
		  s->md5_hash = g_strdup (val);
		g_hash_table_insert(filehash, val, s);
	      }
	  }
      }
    return result;
}

/**
 * Free the specified song from the ipod's unique file hash
 * @s - The Song that's being freed from the ipod
 */
void
song_removed_from_ipod(Song *s)
{
    gchar *val = NULL;

    if((cfg->md5songs) && (filehash) && (val = hash_song(s)))
    {
	if(g_hash_table_lookup(filehash, val))
	{
	    g_hash_table_remove(filehash, val);
	}
	g_free (val);
    }
}
