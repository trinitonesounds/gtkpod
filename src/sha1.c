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
#include <stdlib.h>
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

#define FREE_SHA1_DIGEST(d) \
{ \
    int i = 0; \
    for(i = 0; i < 20; i++) \
	d[i] = 0x01; \
    d[20] = 0x00; \
    free(d); \
    d = NULL; \
}

#if BYTE_ORDER == LITTLE_ENDIAN
static void little_endian(hblock * stupidblock, int bytes);
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
 * do_hash_on_file - read PATH_MAX * NR_PATH_MAX_BLOCKS bytes from the file
 * and ask sha1 for a hash of it, convert this has to a string of hex output
 * @fp - an open file descriptor to read from
 * Returns - A Hash String
 */
gchar *
do_hash_on_file(FILE * fp)
{
   int x = 0;
   int last = 0;
   glong bread = 0;
   u_char *hash = NULL;
   int blocks = NR_PATH_MAX_BLOCKS;
   char file_chunk[PATH_MAX * blocks];

   /* 40 chars for hash */
   u_char *digest = NULL;

   if (fp)
   {
      digest = (u_char *) malloc(sizeof(u_char) * 41);
      memset(digest, 0, sizeof(u_char) * 41);
      bread = fread(file_chunk, 1, PATH_MAX * blocks, fp);
      hash = sha1_hash(file_chunk, bread);
      for (x = 0; x < 20; x++)
         last += snprintf(&digest[last], 4, "%02x", hash[x]);
      digest[last] = 0x00;
      FREE_SHA1_DIGEST(hash);

   }
   return (digest);
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
            result = do_hash_on_file(fp);
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
unique_file_repository_init(GList * songlist)
{
   Song *s = NULL;
   GList *l = NULL;
   gchar *val = NULL;

   if (cfg->md5songs)
   {
      if (filehash)
         unique_file_repository_free();
      filehash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

      /* populate the hash table */
      for (l = songlist; l; l = l->next)
      {
         s = (Song *) l->data;
         if ((val = hash_song(s)) != NULL)
         {
            if (s->md5_hash == NULL)
               s->md5_hash = g_strdup(val);
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
 * on the ipod, NULL otherwise
 */
gchar *
song_exists_on_ipod(Song * s)
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
song_removed_from_ipod(Song * s)
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
 * Returns a unique 20 char array, you have to free it with FREE_SHA1_DIGEST 
 */
static u_char *
sha1_hash(const u_char * text, u_int32_t len)
{
   chunk x;
   chunk temp_len = len;
   const u_char *temp_text = text;
   u_char *digest = (u_char *) malloc(sizeof(u_char) * 21);
   sha1 *message = (sha1 *) malloc(sizeof(sha1));

   message->blockdata = (block *) malloc(sizeof(block));
   message->H = (hblock *) malloc(sizeof(hblock));
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
      little_endian((hblock *) message->blockdata, 64);
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
   little_endian((hblock *) message->blockdata, 64);
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
   little_endian(message->H, 20);
#endif
   for (x = 0; x < 20; x++)
      digest[x] = message->H->charblock[x];
   digest[20] = 0x00;
   free(message->blockdata);
   free(message->H);
   free(message);
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
   int x = 0;
   u_char temp;

   for (x = 0; x < blocks; x += 4)
   {
      temp = stupidblock->charblock[x];
      stupidblock->charblock[x] = stupidblock->charblock[x + 3];
      stupidblock->charblock[x + 3] = temp;
      temp = stupidblock->charblock[x + 1];
      stupidblock->charblock[x + 1] = stupidblock->charblock[x + 2];
      stupidblock->charblock[x + 2] = temp;
   }
}
#endif
