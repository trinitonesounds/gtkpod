/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  SHA1 routine: David Puckett <niekze at yahoo.com>
|  SHA1 implemented from FIPS-160 standard
|  <http://www.itl.nist.gov/fipspubs/fip180-1.htm>
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include "charset.h"
#include "sha1.h"
#include "file.h"
#include "prefs.h"
#include "misc.h"
#include "misc_track.h"
#include "display.h"
#include "display_itdb.h"


typedef guint32 chunk;
union _block
{
   guint8 charblock[64];
   chunk chunkblock[16];
};
typedef union _block block;

union _hblock
{
   guint8 charblock[20];
   chunk chunkblock[5];
};
typedef union _hblock hblock;

struct _sha1
{
   block *blockdata;
   hblock *H;
};
typedef struct _sha1 sha1;

static guint8 *sha1_hash(const guint8 * text, guint32 len);
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
 * This value is multiplied by PATH_MAX_MD5 to determine how many bytes are read
 */
#define NR_PATH_MAX_BLOCKS 4
#define PATH_MAX_SHA1 4096

/* Set up or destory the sha1 hash table */
void setup_sha1()
{
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");

    /* gets called before itdbs are set up -> fail silently */
    if (itdbs_head)
    {
	if (prefs_get_int("sha1"))  /* SHA1 hashing turned on */
	{
	    gp_sha1_hash_tracks();
	
	    /* Display duplicates */
	    gp_duplicate_remove(NULL, NULL);
	}
	else
	    gp_sha1_free_hash();
    }
}

/**
 * get_filesize_for_file_descriptor - get the filesize on disk for the given
 * file descriptor
 * @fp - the filepointer we want the filesize for
 * Returns - the filesize in bytes
 */
static guint32
get_filesize_for_file_descriptor(FILE *fp)
{
    off_t result = 0;
    struct stat stat_info;
    int file_no = fileno(fp);

    if((fstat(file_no, &stat_info) == 0))	/* returns 0 on success */
	result = (int)stat_info.st_size;
    return (guint32)result;
}

/**
 * sha1_hash_on_file - read PATH_MAX_SHA1 * NR_PATH_MAX_BLOCKS bytes
 * from the file and ask sha1 for a hash of it, convert this hash to a
 * string of hex output @fp - an open file descriptor to read from
 * Returns - A Hash String - you handle memory returned
 */
static gchar *
sha1_hash_on_file(FILE * fp)
{
   gchar *result = NULL;

   if (fp)
   {
       int fsize = 0;
       int chunk_size = PATH_MAX_SHA1 * NR_PATH_MAX_BLOCKS;

       fsize = get_filesize_for_file_descriptor(fp);
       if(fsize < chunk_size)
	   chunk_size = fsize;

       if(fsize > 0)
       {
	   guint32 fsize_normal;
	   guint8 *hash = NULL;
	   int bread = 0, x = 0, last = 0;
	   guchar file_chunk[chunk_size + sizeof(int)];

	   /* allocate the digest we're returning */
	   result = g_malloc0(sizeof(gchar) * 41);

	   /* put filesize in the first 32 bits */
	   fsize_normal = GINT32_TO_LE (fsize);
	   memcpy(file_chunk, &fsize_normal, sizeof(guint32));

	   /* read chunk_size from fp */
	   bread = fread(&file_chunk[sizeof(int)], sizeof(gchar),
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
	  gtkpod_warning(_("Hashed file is 0 bytes long\n"));
       }
   }
   return (result);
}

/**
 * Generate a unique hash for the Track passed in
 * @s - The Track data structure, we want to hash based on the file on disk
 * Returns - an SHA1 hash in string format, is the hex output from the hash
 */
static gchar *
sha1_hash_track(Track * s)
{
   ExtraTrackData *etr;
   gchar *result = NULL;
   gchar *filename;

   g_return_val_if_fail (s, NULL);
   etr = s->userdata;
   g_return_val_if_fail (etr, NULL);

   if (etr->sha1_hash != NULL)
   {
       result = g_strdup(etr->sha1_hash);
   }
   else
   {
       filename = get_file_name_from_source (s, SOURCE_PREFER_LOCAL);
       if (filename)
       {
	   result = sha1_hash_on_filename (filename, FALSE);
	   g_free(filename);
       }
   }
   return (result);
}


/* @silent: don't print any warning */
gchar *sha1_hash_on_filename (gchar *name, gboolean silent)
{
    gchar *result = NULL;

    if (name)
    {
	FILE *fpit = fopen (name, "r");
	if (!fpit)
	{
	    if (!silent)
	    {
		gchar *name_utf8=charset_to_utf8 (name);
		gtkpod_warning (
		    _("Could not open '%s' to calculate SHA1 checksum: %s\n"),
		    name_utf8, strerror(errno));
		g_free (name_utf8);
	    }
	}
	else
	{
	    result = sha1_hash_on_file (fpit);
	    fclose (fpit);
	}
    }
    return result;
}


/**
 * Free up the dynamically allocated memory in @itdb's hash table
 */
void sha1_free_eitdb (ExtraiTunesDBData *eitdb)
{
    g_return_if_fail (eitdb);

    if (eitdb->sha1hash)
    {
	g_hash_table_destroy (eitdb->sha1hash);
	eitdb->sha1hash = NULL;
    }
}

/**
 * Free up the dynamically allocated memory in @itdb's hash table
 */
void sha1_free (iTunesDB *itdb)
{
    g_return_if_fail (itdb);
    g_return_if_fail (itdb->userdata);

    sha1_free_eitdb (itdb->userdata);
}

/**
 * Check to see if a track has already been added to the ipod
 * @s - the Track we want to know about. If the track does not exist, it
 * is inserted into the hash.
 * Returns a pointer to the duplicate track.
 */
Track *sha1_track_exists_insert (iTunesDB *itdb, Track * s)
{
    ExtraiTunesDBData *eitdb;
    ExtraTrackData *etr;
    gchar *val = NULL;
    Track *track = NULL;

    g_return_val_if_fail (itdb, NULL);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);

    g_return_val_if_fail (s, NULL);
    etr = s->userdata;
    g_return_val_if_fail (etr, NULL);

    if (prefs_get_int("sha1"))
    {
	if (eitdb->sha1hash == NULL)
	{
	    eitdb->sha1hash = g_hash_table_new_full(g_str_hash,
						   g_str_equal,
						   g_free, NULL);
	}
	val = sha1_hash_track (s);
	if (val != NULL)
	{
	    track = g_hash_table_lookup (eitdb->sha1hash, val);
	    if (track)
	    {
		g_free(val);
	    }
	    else
	    {   /* if it doesn't exist we register it in the hash */
		g_free (etr->sha1_hash);
		etr->sha1_hash = g_strdup (val);
		/* val is used in the next line -- we don't have to
		 * g_free() it */
		g_hash_table_insert (eitdb->sha1hash, val, s);
	    }
	}
    }
    return track;
}

/**
 * Check to see if a track has already been added to the ipod
 * @s - the Track we want to know about.
 * Returns a pointer to the duplicate track.
 */
Track *sha1_track_exists (iTunesDB *itdb, Track *s)
{
    ExtraiTunesDBData *eitdb;
    Track *track = NULL;

    g_return_val_if_fail (itdb, NULL);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);

    if (prefs_get_int("sha1") && eitdb->sha1hash)
    {
	gchar *val = sha1_hash_track (s);
	if (val)
	{
	    track = g_hash_table_lookup (eitdb->sha1hash, val);
	    g_free (val);
	}
    }
    return track;
}

/**
 * Check to see if a track has already been added to the ipod
 * @file - the Track we want to know about.
 * Returns a pointer to the duplicate track using sha1 for
 * identification. If sha1 checksums are off, NULL is returned.
 */
Track *sha1_file_exists (iTunesDB *itdb, gchar *file, gboolean silent)
{
    ExtraiTunesDBData *eitdb;
    Track *track = NULL;

    g_return_val_if_fail (file, NULL);
    g_return_val_if_fail (itdb, NULL);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);

    if (prefs_get_int("sha1") && eitdb->sha1hash)
    {
	gchar *val = sha1_hash_on_filename (file, silent);
	if (val)
	{
	    track = g_hash_table_lookup (eitdb->sha1hash, val);
	    g_free (val);
	}
    }
    return track;
}


/**
 * Check to see if a track has already been added to the ipod
 * @sha1 - the sha1 we want to know about.
 * Returns a pointer to the duplicate track using sha1 for
 * identification. If sha1 checksums are off, NULL is returned.
 */
Track *sha1_sha1_exists (iTunesDB *itdb, gchar *sha1)
{
    ExtraiTunesDBData *eitdb;
    Track *track = NULL;

    g_return_val_if_fail (sha1, NULL);
    g_return_val_if_fail (itdb, NULL);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, NULL);

    if (prefs_get_int("sha1") && eitdb->sha1hash)
    {
	track = g_hash_table_lookup (eitdb->sha1hash, sha1);
    }
    return track;
}

/**
 * Free the specified track from the ipod's unique file hash
 * @s - The Track that's being freed from the ipod
 */
void sha1_track_remove (Track *s)
{
    ExtraiTunesDBData *eitdb;

    g_return_if_fail (s);
    g_return_if_fail (s->itdb);
    eitdb = s->itdb->userdata;
    g_return_if_fail (eitdb);

    if (prefs_get_int("sha1") && eitdb->sha1hash)
    {
	gchar *val = sha1_hash_track (s);
	if (val)
	{
	    Track *track = g_hash_table_lookup (eitdb->sha1hash, val);
	    if (track)
	    {
		if (track == s) /* only remove if it's the same track */
		    g_hash_table_remove (eitdb->sha1hash, val);
	    }
	    g_free(val);
	}
    }
}

/* sha1_hash - hash value the input data with a given size.
 * @text - the data we're reading to seed sha1
 * @len - the length of the data for our seed
 * Returns a unique 20 char array.
 */
static guint8 *
sha1_hash(const guint8 * text, guint32 len)
{
   chunk x;
   chunk temp_len = len;
   const guint8 *temp_text = text;
   guint8 *digest;
   sha1 *message;

   digest = g_malloc0(sizeof(guint8) * 21);
   message = g_malloc0(sizeof(sha1));
   message->blockdata = g_malloc0(sizeof(block));
   message->H = g_malloc(sizeof(hblock));

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
   chunk T;                     /* temp guint32 */
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


