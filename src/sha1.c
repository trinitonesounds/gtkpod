/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
|
|  SHA1 routine: David Puckett <niekze at yahoo.com>
|  SHA1 implemented from FIPS-160 standard
|  <http://www.itl.nist.gov/fipspubs/fip180-1.htm>
|
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include "md5.h"
#include "prefs.h"
#include "misc.h"
#include "support.h"


typedef u_int32_t chunk;
union _block
{
   u_char charblock[64];
   chunk chunkblock[16];
};
typedef union _block block;

union _hblock
{
   u_char charblock[20];
   chunk chunkblock[5];
};
typedef union _hblock hblock;

struct _sha1
{
   block *blockdata;
   hblock *H;
};
typedef struct _sha1 sha1;

static u_char *sha1_hash(const u_char * text, u_int32_t len);
static void process_block_sha1(sha1 * message);

#if BYTE_ORDER == LITTLE_ENDIAN
static void little_endian(hblock * stupidblock, int blocks);
#endif

/**
 * Create and manage a string hash for files on disk
 */

/** 
 * NR_PATH_MAX_BLOCKS
 * A seed of sorts for SHA1, if collisions occur increasing this value
 * should give more unique data to SHA1 as more of the file is read
 * This value is multiplied by PATH_MAX to determine how many bytes are read
 */
#define NR_PATH_MAX_BLOCKS 8

/** 
 * filehash
 * a string hash of the files on the ipod already, or are already registered
 * to be copied on next export
 * */
static GHashTable *filehash = NULL;

/**
 * get_filesize_for_file_descriptor - get the filesize on disk for the given
 * file descriptor
 * @fp - the filepointer we want the filesize for 
 * Returns - the filesize in bytes
 */
static int
get_filesize_for_file_descriptor(FILE *fp)
{
    int result = 0;
    struct stat stat_info;
    int file_no = fileno(fp);

    if((fstat(file_no, &stat_info) == 0))	/* returns 0 on success */
	result = (int)stat_info.st_size;
    return(result);
}

/**
 * md5_hash_on_file - read PATH_MAX * NR_PATH_MAX_BLOCKS bytes from the file
 * and ask sha1 for a hash of it, convert this hash to a string of hex output
 * @fp - an open file descriptor to read from
 * Returns - A Hash String - you handle memory returned
 */
gchar *
md5_hash_on_file(FILE * fp)
{
   gchar *result = NULL;

   if (fp)
   {
       int fsize = 0;
       int chunk_size = PATH_MAX * NR_PATH_MAX_BLOCKS;
       
       fsize = get_filesize_for_file_descriptor(fp);
       if(fsize < chunk_size)
	   chunk_size = fsize;

       if(fsize > 0)
       {
	   u_char *hash = NULL;
	   int bread = 0, x = 0, last = 0;
	   char file_chunk[chunk_size + sizeof(int)];
	  
	   /* allocate the digest we're returning */
	   if((result = (gchar*)g_malloc0(sizeof(gchar) * 41)) == NULL)
	       gtkpod_main_quit();	/* errno == ENOMEM */ 
	   
	   /* put filesize in the first 32 bits */
	   memcpy(file_chunk, &fsize, sizeof(int));	
	   
	   /* read chunk_size from fp */
	   bread = fread(&file_chunk[sizeof(int)], sizeof(char), 
			    chunk_size, fp);
	    
	   /* create hash from our data */
	   hash = sha1_hash(file_chunk, (bread + sizeof(int)));

	   /* put it in a format we like */
	   for (x = 0; x < 20; x++)
	       last += snprintf(&result[last], 4, "%02x", hash[x]);

	   /* free the hash value sha1_hash gave us */
	   g_free(hash);
       }
       else
       { 
	  gtkpod_warning(_("Hashed File is 0 bytes\n"));
       }
   }
   return (result);
}

/**
 * Generate a unique hash for the Song passed in
 * @s - The Song data structure, we want to hash based on the file on disk
 * Returns - an SHA1 hash in string format, is the hex output from the hash
 */
static gchar *
hash_song(Song * s)
{
   FILE *fp;
   gchar *result = NULL;
   gchar *filename = NULL;

   if (s != NULL)
   {
      if (s->md5_hash != NULL)
      {
         result = g_strdup(s->md5_hash);
      }
      else if ((filename = get_song_name_on_disk(s)) != NULL)
      {
         if ((fp = fopen(filename, "r")))
         {
            result = md5_hash_on_file(fp);
            fclose(fp);
         }
         else
         {
            gtkpod_warning(_("Unable to open file \"%s\"\n"), filename);
         }
         g_free(filename);
      }
   }
   return (result);
}

/**
 * Initialize the filelist hash table and insert all the Songs passed in
 * @songlist - A GList with data attribute of type Song *
 */
void
md5_unique_file_init(GList * songlist)
{
   Song *s = NULL;
   GList *l = NULL;
   gchar *val = NULL;
    
   if (cfg->md5songs)
   {
      if (filehash)
         md5_unique_file_free();
      filehash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

      /* populate the hash table */
      for (l = songlist; l; l = l->next)
      {
	  s = (Song *) l->data;
	  if((val = md5_song_exists_on_ipod(s)))
	      g_free(val);	
	  /* 
	   * TODO could eventually be used to detect duplicates if the user
	   * doesn't always enable them.  identify and fix.
	   */
      }
   }
}


/**
 * Free up the dynamically allocated memory in this table
 */
void
md5_unique_file_free(void)
{
   if ((cfg->md5songs) && (filehash))
   {
      g_hash_table_destroy(filehash);
      filehash = NULL;
   }
}

/**
 * Check to see if a song has already been added to the ipod
 * @s - the Song we want to know about
 * Returns the filename (or other tag) if the song is already 
 * on the ipod, NULL otherwise. Don't free the result. Use it
 * _before_ you free the song "s".
 */
gchar *
md5_song_exists_on_ipod(Song * s)
{
   gchar *val = NULL;
   gchar *result = NULL;
   Song *song = NULL;

   if ((cfg->md5songs) && (filehash))
   {
      val = hash_song(s);
      if (val != NULL)
      {
         if ((song = g_hash_table_lookup(filehash, val)))
         {
            g_free(val);
            if (song->pc_path_utf8 && strlen(song->pc_path_utf8))
               result = song->pc_path_utf8;
            else if ((song->title && strlen(song->title)))
               result = song->title;
            else if ((song->album && strlen(song->album)))
               result = song->album;
            else if ((song->artist && strlen(song->artist)))
               result = song->artist;
            else
               result = "";
         }
         else                   /* if it doesn't exist we register it in the
                                   hash */
         {
            if (s->md5_hash == NULL)
               s->md5_hash = g_strdup(val);
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
md5_song_removed_from_ipod(Song * s)
{
   gchar *val = NULL;

   if ((cfg->md5songs) && (filehash) && (s) && (val = hash_song(s)))
   {
      if (g_hash_table_lookup(filehash, val))
      {
         g_hash_table_remove(filehash, val);
      }
      g_free(val);
   }
}

/* sha1_hash - hash value the input data with a given size.
 * @text - the data we're reading to seed sha1
 * @len - the length of the data for our seed
 * Returns a unique 20 char array.
 */
static u_char *
sha1_hash(const u_char * text, u_int32_t len)
{
   chunk x;
   chunk temp_len = len;
   const u_char *temp_text = text;
   u_char *digest;
   sha1 *message;

   if((digest = (u_char *) g_malloc0(sizeof(u_char) * 21)) == NULL)
       gtkpod_main_quit();	/* errno == ENOMEM */
   if((message = (sha1 *) g_malloc0(sizeof(sha1))) == NULL)
       gtkpod_main_quit();	/* errno == ENOMEM */
   if((message->blockdata = (block *) g_malloc0(sizeof(block))) == NULL)
       gtkpod_main_quit();	/* errno == ENOMEM */
   if((message->H = (hblock *) g_malloc(sizeof(hblock))) == NULL)
       gtkpod_main_quit();	/* errno == ENOMEM */
   
   message->H->chunkblock[0] = 0x67452301;
   message->H->chunkblock[1] = 0xefcdab89;
   message->H->chunkblock[2] = 0x98badcfe;
   message->H->chunkblock[3] = 0x10325476;
   message->H->chunkblock[4] = 0xc3d2e1f0;
   while (temp_len >= 64)
   {
      for (x = 0; x < 64; x++)
         message->blockdata->charblock[x] = temp_text[x];
#if BYTE_ORDER == LITTLE_ENDIAN
      little_endian((hblock *) message->blockdata, 16);
#endif
      process_block_sha1(message);
      temp_len -= 64;
      temp_text += 64;
   }
   for (x = 0; x < temp_len; x++)
      message->blockdata->charblock[x] = temp_text[x];
   message->blockdata->charblock[temp_len] = 0x80;
   for (x = temp_len + 1; x < 64; x++)
      message->blockdata->charblock[x] = 0x00;
#if BYTE_ORDER == LITTLE_ENDIAN
   little_endian((hblock *) message->blockdata, 16);
#endif
   if (temp_len > 54)
   {
      process_block_sha1(message);
      for (x = 0; x < 60; x++)
         message->blockdata->charblock[x] = 0x00;
   }
   message->blockdata->chunkblock[15] = len * 8;
   process_block_sha1(message);
#if BYTE_ORDER == LITTLE_ENDIAN
   little_endian(message->H, 5);
#endif
   for (x = 0; x < 20; x++)
      digest[x] = message->H->charblock[x];
   digest[20] = 0x00;
   g_free(message->blockdata);
   g_free(message->H);
   g_free(message);
   return (digest);
}

/* 
 * process_block_sha1 - process one 512-bit block of data 
 * @message - the sha1 struct we're doing working on
 */
static void
process_block_sha1(sha1 * message)
{
   chunk x;
   chunk w[80];                 /* test block */
   chunk A;                     /* A - E: used for hash calc */
   chunk B;
   chunk C;
   chunk D;
   chunk E;
   chunk T;                     /* temp u_int32_t */
   chunk K[] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

   for (x = 0; x < 16; x++)
      w[x] = message->blockdata->chunkblock[x];
   for (x = 16; x < 80; x++)
   {
      w[x] = w[x - 3] ^ w[x - 8] ^ w[x - 14] ^ w[x - 16];
      w[x] = (w[x] << 1) | (w[x] >> 31);
   }
   A = message->H->chunkblock[0];
   B = message->H->chunkblock[1];
   C = message->H->chunkblock[2];
   D = message->H->chunkblock[3];
   E = message->H->chunkblock[4];
   for (x = 0; x < 80; x++)
   {
      T = ((A << 5) | (A >> 27)) + E + w[x] + K[x / 20];
      if (x < 20)
         T += (B & C) | ((~B) & D);
      else if (x < 40 || x >= 60)
         T += B ^ C ^ D;
      else
         T += ((C | D) & B) | (C & D);
      E = D;
      D = C;
      C = (B << 30) | (B >> 2);
      B = A;
      A = T;
   }
   message->H->chunkblock[0] += A;
   message->H->chunkblock[1] += B;
   message->H->chunkblock[2] += C;
   message->H->chunkblock[3] += D;
   message->H->chunkblock[4] += E;
}

#if BYTE_ORDER == LITTLE_ENDIAN
/* 
 * little_endian - swap the significants bits to cater to bigendian
 * @stupidblock - the block of data we're swapping 
 * @blocks - the number of blocks we're swapping
 */
static void
little_endian(hblock * stupidblock, int blocks)
{
   int x;
   for (x = 0; x < blocks; x++)
   {
  	stupidblock->chunkblock[x] = (stupidblock->charblock[x * 4] << 24 | stupidblock->charblock[x * 4 + 1] << 16 | stupidblock->charblock[x * 4 +2] << 8 | stupidblock->charblock[x * 4 + 3]); 
   }
}
#endif
