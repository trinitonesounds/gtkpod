/* Time-stamp: <2005-02-12 02:10:54 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL:  http://gtkpod.sourceforge.net/
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

/* The code in the first section of this file is taken from the
 * mp3info (http://www.ibiblio.org/mp3info/) project. Only the code
 * needed for the playlength calculation has been extracted. */

/* The code in the second section of this file is taken from the
 * mpg123 code used in xmms-1.2.7 (Input/mpg123). Only the code needed
 * for the playlength calculation has been extracted. */

/* The code in the last section of this file is original gtkpod
 * code. */

/****************
 * Declarations *
 ****************/

#include <glib.h>
#include <math.h>
/*
 * Description of each item of the TagList list
 */
typedef struct _File_Tag File_Tag;
struct _File_Tag
{
    gchar *title;          /* Title of track */
    gchar *artist;         /* Artist name */
    gchar *album;          /* Album name */
    gchar *year;           /* Year of track */
    gchar *trackstring;    /* Position of track in the album */
    gchar *track_total;    /* The number of tracks for the album (ex: 12/20) */
    gchar *genre;          /* Genre of song */
    gchar *comment;        /* Comment */
    gchar *composer;	   /* Composer */
    guint32 songlen;       /* Length of file in ms */
};

/* This code is taken from the mp3info code. Only the code needed for
 * the playlength calculation has been extracted */

/*
    mp3tech.c - Functions for handling MP3 files and most MP3 data
		structure manipulation.

    Copyright (C) 2000-2001  Cedric Tefft <cedric@earthling.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  ***************************************************************************

  This file is based in part on:

	* MP3Info 0.5 by Ricardo Cerqueira <rmc@rccn.net>
	* MP3Stat 0.9 by Ed Sweetman <safemode@voicenet.com> and
			 Johannes Overmann <overmann@iname.com>

*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mp3file.h"
#include "charset.h"
#include "itdb.h"
#include "file.h"
#include "misc.h"
#include "support.h"


/* MIN_CONSEC_GOOD_FRAMES defines how many consecutive valid MP3 frames
   we need to see before we decide we are looking at a real MP3 file */
#define MIN_CONSEC_GOOD_FRAMES 4
#define FRAME_HEADER_SIZE 4
#define MIN_FRAME_SIZE 21

enum VBR_REPORT { VBR_VARIABLE, VBR_AVERAGE, VBR_MEDIAN };

typedef struct {
    gulong sync;
    guint  version;
    guint  layer;
    guint  crc;
    guint  bitrate;
    guint  freq;
    guint  padding;
    guint  extension;
    guint  mode;
    guint  mode_extension;
    guint  copyright;
    guint  original;
    guint  emphasis;
} mp3header;

typedef struct {
    gchar *filename;
    FILE *file;
    off_t datasize;
    gint header_isvalid;
    mp3header header;
    gint id3_isvalid;
    gint vbr;
    float vbr_average;
    gint milliseconds;
    gint frames;
    gint badframes;
} mp3info;

/* This is for xmms code */
static guint get_track_time(gchar *path);



/* ------------------------------------------------------------

   start of first section

   ------------------------------------------------------------ */
void get_mp3_info(mp3info *mp3);

gint frequencies[3][4] = {
   {22050,24000,16000,50000},  /* MPEG 2.0 */
   {44100,48000,32000,50000},  /* MPEG 1.0 */
   {11025,12000,8000,50000}    /* MPEG 2.5 */
};

/* "0" added by JCS */
gint bitrate[2][3][16] = {
  { /* MPEG 2.0 */
    {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},/* layer 1 */
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},     /* layer 2 */
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0}      /* layer 3 */
  },

  { /* MPEG 1.0 */
    {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},/* layer 1 */
    {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0},   /* layer 2 */
    {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0}     /* layer 3 */
  }
};

gint frame_size_index[] = {24000, 72000, 72000};


gchar *mode_text[] = {
   "stereo", "joint stereo", "dual channel", "mono"
};

gchar *emphasis_text[] = {
  "none", "50/15 microsecs", "reserved", "CCITT J 17"
};


static gint mp3file_header_bitrate(mp3header *h) {
    return bitrate[h->version & 1][3-h->layer][h->bitrate];
}


static gint mp3file_header_frequency(mp3header *h) {
    return frequencies[h->version][h->freq];
}


gint frame_length(mp3header *header) {
	return header->sync == 0xFFE ?
		    (frame_size_index[3-header->layer]*((header->version&1)+1)*
		    mp3file_header_bitrate(header)/(float)mp3file_header_frequency(header))+
		    header->padding : 1;
}

/* Get next MP3 frame header.
   Return codes:
   positive value = Frame Length of this header
   0 = No, we did not retrieve a valid frame header
*/
gint get_header(FILE *file,mp3header *header)
{
    guchar buffer[FRAME_HEADER_SIZE];
    gint fl;

    if(fread(&buffer,FRAME_HEADER_SIZE,1,file)<1) {
	header->sync=0;
	return 0;
    }
    header->sync=(((gint)buffer[0]<<4) | ((gint)(buffer[1]&0xE0)>>4));
    if(buffer[1] & 0x10) header->version=(buffer[1] >> 3) & 1;
		    else header->version=2;
    header->layer=(buffer[1] >> 1) & 3;
    if (header->layer == 0)
    {
	header->layer = 1; /* sanity added by JCS */
    }
    if((header->sync != 0xFFE) || (header->layer != 1)) {
	header->sync=0;
	return 0;
    }
    header->crc=buffer[1] & 1;
    header->bitrate=(buffer[2] >> 4) & 0x0F;
    header->freq=(buffer[2] >> 2) & 0x3;
    header->padding=(buffer[2] >>1) & 0x1;
    header->extension=(buffer[2]) & 0x1;
    header->mode=(buffer[3] >> 6) & 0x3;
    header->mode_extension=(buffer[3] >> 4) & 0x3;
    header->copyright=(buffer[3] >> 3) & 0x1;
    header->original=(buffer[3] >> 2) & 0x1;
    header->emphasis=(buffer[3]) & 0x3;

    return ((fl=frame_length(header)) >= MIN_FRAME_SIZE ? fl : 0);
}

gint sameConstant(mp3header *h1, mp3header *h2) {
    if((*(guint*)h1) == (*(guint*)h2)) return 1;

    if((h1->version       == h2->version         ) &&
       (h1->layer         == h2->layer           ) &&
       (h1->crc           == h2->crc             ) &&
       (h1->freq          == h2->freq            ) &&
       (h1->mode          == h2->mode            ) &&
       (h1->copyright     == h2->copyright       ) &&
       (h1->original      == h2->original        ) &&
       (h1->emphasis      == h2->emphasis        ))
		return 1;
    else return 0;
}


gint get_first_header(mp3info *mp3, long startpos)
{
  gint k, l=0,c;
  mp3header h, h2;
  long valid_start=0;

  fseek(mp3->file,startpos,SEEK_SET);
  while (1) {
     while((c=fgetc(mp3->file)) != 255 && (c != EOF));
     if(c == 255) {
	ungetc(c,mp3->file);
	valid_start=ftell(mp3->file);
	if((l=get_header(mp3->file,&h))) {
	  fseek(mp3->file,l-FRAME_HEADER_SIZE,SEEK_CUR);
	  for(k=1; (k < MIN_CONSEC_GOOD_FRAMES) && (mp3->datasize-ftell(mp3->file) >= FRAME_HEADER_SIZE); k++) {
	    if(!(l=get_header(mp3->file,&h2))) break;
	    if(!sameConstant(&h,&h2)) break;
	    fseek(mp3->file,l-FRAME_HEADER_SIZE,SEEK_CUR);
	  }
	  if(k == MIN_CONSEC_GOOD_FRAMES) {
		fseek(mp3->file,valid_start,SEEK_SET);
		memcpy(&(mp3->header),&h2,sizeof(mp3header));
		mp3->header_isvalid=1;
		return 1;
	  }
	}
     } else {
	return 0;
     }
   }

  return 0;
}


/* get_next_header() - read header at current position or look for
   the next valid header if there isn't one at the current position
*/
gint get_next_header(mp3info *mp3)
{
  gint l=0,c,skip_bytes=0;
  mp3header h;

   while(1) {
     while((c=fgetc(mp3->file)) != 255 && (ftell(mp3->file) < mp3->datasize)) skip_bytes++;
     if(c == 255) {
	ungetc(c,mp3->file);
	if((l=get_header(mp3->file,&h))) {
	  if(skip_bytes) mp3->badframes++;
	  fseek(mp3->file,l-FRAME_HEADER_SIZE,SEEK_CUR);
	  return 15-h.bitrate;
	} else {
		skip_bytes += FRAME_HEADER_SIZE;
	}
     } else {
	  if(skip_bytes) mp3->badframes++;
	  return 0;
     }
  }
}


void get_mp3_info(mp3info *mp3)
{
  gint frame_type[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  float milliseconds=0,total_rate=0;
  gint frames=0,frame_types=0,frames_so_far=0;
  gint vbr_median=-1;
  guint bitrate;
  gint counter=0;
  mp3header header;
  struct stat filestat;
  off_t data_start=0;


  stat(mp3->filename,&filestat);
  mp3->datasize=filestat.st_size;

  if(get_first_header(mp3,0L)) {
      data_start=ftell(mp3->file);
      while((bitrate=get_next_header(mp3))) {
	  if (bitrate < 15)  /* sanity added by JCS */
	      frame_type[15-bitrate]++;
	  frames++;
      }
      memcpy(&header,&(mp3->header),sizeof(mp3header));
      for(counter=0;counter<15;counter++) {
	  if(frame_type[counter]) {
	      float header_bitrate; /* introduced by JCS to speed up */
	      frame_types++;
	      header.bitrate=counter;
	      frames_so_far += frame_type[counter];
	      header_bitrate = mp3file_header_bitrate(&header);
	      if (header_bitrate != 0)
		  milliseconds += (float)(8*frame_length(&header)*frame_type[counter])/header_bitrate;
	      total_rate += header_bitrate*frame_type[counter];
	      if((vbr_median == -1) && (frames_so_far >= frames/2))
		  vbr_median=counter;
	  }
      }
      mp3->milliseconds=(gint)(milliseconds+0.5);
      mp3->header.bitrate=vbr_median;
      mp3->vbr_average=total_rate/(float)frames;
      mp3->frames=frames;
      if(frame_types > 1) {
	  mp3->vbr=1;
      }
  }
}



/* Returns a filled-in mp3info struct that must be g_free'd after use */
static mp3info *mp3file_get_info (gchar *filename)
{
    mp3info *mp3 = NULL;
    FILE *fp;

    if ( !( fp=fopen(filename,"r") ) ) {
	gtkpod_warning (_("Error opening MP3 file '%s'"),filename);
    } else {
	mp3 = g_malloc0 (sizeof (mp3info));
	mp3->filename=filename;
	mp3->file=fp;
	get_mp3_info (mp3);
	fclose (fp);
    }
    return mp3;
}


/* ------------------------------------------------------------

	 xmms code


   ------------------------------------------------------------ */

/*
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to
|  compile with the gtkpod project. 2003/04/01
*/

/* This code is taken from the mpg123 code used in xmms-1.2.7
 * (Input/mpg123). Only the code needed for the playlength calculation
 * has been extracted */

#include "mp3file.h"
#include <stdio.h>
#include <string.h>

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3
#define MAXFRAMESIZE 1792
#define real float

struct bitstream_info
{
	int bitindex;
	unsigned char *wordpointer;
};

struct bitstream_info bsi;

real mpg123_muls[27][64];	/* also used by layer 1 */

int tabsel_123[2][3][16] =
{
	{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,},
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
       {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}},

	{
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}}
};

long mpg123_freqs[9] =
{44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000};

/*
 * structure to receive extracted header
 */
typedef struct
{
	int frames;		/* total bit stream frames from Xing header data */
	int bytes;		/* total bit stream bytes from Xing header data */
	unsigned char toc[100];	/* "table of contents" */
} xing_header_t;

struct al_table
{
	short bits;
	short d;
};

struct frame
{
	struct al_table *alloc;
	int (*synth) (real *, int, unsigned char *, int *);
	int (*synth_mono) (real *, unsigned char *, int *);
#ifdef USE_3DNOW
	void (*dct36)(real *,real *,real *,real *,real *);
#endif
	int stereo;
	int jsbound;
	int single;
	int II_sblimit;
	int down_sample_sblimit;
	int lsf;
	int mpeg25;
	int down_sample;
	int header_change;
	int lay;
	int (*do_layer) (struct frame * fr);
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int framesize;		/* computed framesize */
};

static guint32 convert_to_header(guint8 * buf)
{

	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static int mpg123_head_check(unsigned long head)
{
	if ((head & 0xffe00000) != 0xffe00000)
		return FALSE;
	if (!((head >> 17) & 3))
		return FALSE;
	if (((head >> 12) & 0xf) == 0xf)
		return FALSE;
	if (!((head >> 12) & 0xf))
		return FALSE;
	if (((head >> 10) & 0x3) == 0x3)
		return FALSE;
	if (((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 && ((head >> 16) & 1) == 1)
		return FALSE;
	if ((head & 0xffff0000) == 0xfffe0000)
		return FALSE;

	return TRUE;
}


/*
 * the code a header and write the information
 * into the frame structure
 */
static int mpg123_decode_header(struct frame *fr, unsigned long newhead)
{
    int ssize;

	if (newhead & (1 << 20))
	{
		fr->lsf = (newhead & (1 << 19)) ? 0x0 : 0x1;
		fr->mpeg25 = 0;
	}
	else
	{
		fr->lsf = 1;
		fr->mpeg25 = 1;
	}
	fr->lay = 4 - ((newhead >> 17) & 3);
	if (fr->mpeg25)
	{
		fr->sampling_frequency = 6 + ((newhead >> 10) & 0x3);
	}
	else
		fr->sampling_frequency = ((newhead >> 10) & 0x3) + (fr->lsf * 3);
	fr->error_protection = ((newhead >> 16) & 0x1) ^ 0x1;

	fr->bitrate_index = ((newhead >> 12) & 0xf);
	fr->padding = ((newhead >> 9) & 0x1);
	fr->extension = ((newhead >> 8) & 0x1);
	fr->mode = ((newhead >> 6) & 0x3);
	fr->mode_ext = ((newhead >> 4) & 0x3);
	fr->copyright = ((newhead >> 3) & 0x1);
	fr->original = ((newhead >> 2) & 0x1);
	fr->emphasis = newhead & 0x3;

	fr->stereo = (fr->mode == MPG_MD_MONO) ? 1 : 2;

	ssize = 0;

	if (!fr->bitrate_index)
		return (0);

	switch (fr->lay)
	{
		case 1:
/*			fr->do_layer = mpg123_do_layer1; */
/*			mpg123_init_layer2();	/\* inits also shared tables with layer1 *\/ */
			fr->framesize = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
			break;
		case 2:
/*			fr->do_layer = mpg123_do_layer2; */
/*			mpg123_init_layer2();	/\* inits also shared tables with layer1 *\/ */
			fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize += fr->padding - 4;
			break;
		case 3:
/*			fr->do_layer = mpg123_do_layer3; */
			if (fr->lsf)
				ssize = (fr->stereo == 1) ? 9 : 17;
			else
				ssize = (fr->stereo == 1) ? 17 : 32;
			if (fr->error_protection)
				ssize += 2;
			fr->framesize = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
			fr->framesize = fr->framesize + fr->padding - 4;
			break;
		default:
			return (0);
	}
	if(fr->framesize > MAXFRAMESIZE)
		return 0;
	return 1;
}

#define GET_INT32BE(b) \
(i = (b[0] << 24) | (b[1] << 16) | b[2] << 8 | b[3], b += 4, i)

static int mpg123_get_xing_header(xing_header_t * xing, unsigned char *buf)
{
	int i, head_flags;
	int id, mode;

	memset(xing, 0, sizeof(xing_header_t));

	/* get selected MPEG header data */
	id = (buf[1] >> 3) & 1;
	mode = (buf[3] >> 6) & 3;
	buf += 4;

	/* Skip the sub band data */
	if (id)
	{
		/* mpeg1 */
		if (mode != 3)
			buf += 32;
		else
			buf += 17;
	}
	else
	{
		/* mpeg2 */
		if (mode != 3)
			buf += 17;
		else
			buf += 9;
	}

	if (strncmp(buf, "Xing", 4))
		return 0;
	buf += 4;

	head_flags = GET_INT32BE(buf);

	if (head_flags & FRAMES_FLAG)
		xing->frames = GET_INT32BE(buf);
	if (xing->frames < 1)
		xing->frames = 1;
	if (head_flags & BYTES_FLAG)
		xing->bytes = GET_INT32BE(buf);

	if (head_flags & TOC_FLAG)
	{
		for (i = 0; i < 100; i++)
			xing->toc[i] = buf[i];
		buf += 100;
	}

#ifdef XING_DEBUG
	for (i = 0; i < 100; i++)
	{
		if ((i % 10) == 0)
			fprintf(stderr, "\n");
		fprintf(stderr, " %3d", xing->toc[i]);
	}
#endif

	return 1;
}

static double mpg123_compute_tpf(struct frame *fr)
{
	const int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = bs[fr->lay];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

static double mpg123_compute_bpf(struct frame *fr)
{
	double bpf;

	switch (fr->lay)
	{
		case 1:
			bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
			bpf *= 12000.0 * 4.0;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
			break;
		case 2:
		case 3:
			bpf = tabsel_123[fr->lsf][fr->lay - 1][fr->bitrate_index];
			bpf *= 144000;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
			break;
		default:
			bpf = 1.0;
	}

	return bpf;
}


unsigned int mpg123_getbits(int number_of_bits)
{
	unsigned long rval;

#ifdef DEBUG_GETBITS
	fprintf(stderr, "g%d", number_of_bits);
#endif

	if(!number_of_bits)
		return 0;

#if 0
	check_buffer_range(number_of_bits + bsi.bitindex);
#endif

	{
		rval = bsi.wordpointer[0];
		rval <<= 8;
		rval |= bsi.wordpointer[1];
		rval <<= 8;
		rval |= bsi.wordpointer[2];

		rval <<= bsi.bitindex;
		rval &= 0xffffff;

		bsi.bitindex += number_of_bits;

		rval >>= (24-number_of_bits);

		bsi.wordpointer += (bsi.bitindex >> 3);
		bsi.bitindex &= 7;
	}

#ifdef DEBUG_GETBITS
	fprintf(stderr,":%x ",rval);
#endif

	return rval;
}


void I_step_one(unsigned int balloc[], unsigned int scale_index[2][SBLIMIT], struct frame *fr)
{
	unsigned int *ba = balloc;
	unsigned int *sca = (unsigned int *) scale_index;

	if (fr->stereo)
	{
		int i;
		int jsbound = fr->jsbound;

		for (i = 0; i < jsbound; i++)
		{
			*ba++ = mpg123_getbits(4);
			*ba++ = mpg123_getbits(4);
		}
		for (i = jsbound; i < SBLIMIT; i++)
			*ba++ = mpg123_getbits(4);

		ba = balloc;

		for (i = 0; i < jsbound; i++)
		{
			if ((*ba++))
				*sca++ = mpg123_getbits(6);
			if ((*ba++))
				*sca++ = mpg123_getbits(6);
		}
		for (i = jsbound; i < SBLIMIT; i++)
			if ((*ba++))
			{
				*sca++ = mpg123_getbits(6);
				*sca++ = mpg123_getbits(6);
			}
	}
	else
	{
		int i;

		for (i = 0; i < SBLIMIT; i++)
			*ba++ = mpg123_getbits(4);
		ba = balloc;
		for (i = 0; i < SBLIMIT; i++)
			if ((*ba++))
				*sca++ = mpg123_getbits(6);
	}
}

void I_step_two(real fraction[2][SBLIMIT], unsigned int balloc[2 * SBLIMIT],
		unsigned int scale_index[2][SBLIMIT], struct frame *fr)
{
	int i, n;
	int smpb[2 * SBLIMIT];	/* values: 0-65535 */
	int *sample;
	register unsigned int *ba;
	register unsigned int *sca = (unsigned int *) scale_index;

	if (fr->stereo)
	{
		int jsbound = fr->jsbound;
		register real *f0 = fraction[0];
		register real *f1 = fraction[1];

		ba = balloc;
		for (sample = smpb, i = 0; i < jsbound; i++)
		{
			if ((n = *ba++))
				*sample++ = mpg123_getbits(n + 1);
			if ((n = *ba++))
				*sample++ = mpg123_getbits(n + 1);
		}
		for (i = jsbound; i < SBLIMIT; i++)
			if ((n = *ba++))
				*sample++ = mpg123_getbits(n + 1);

		ba = balloc;
		for (sample = smpb, i = 0; i < jsbound; i++)
		{
			if ((n = *ba++))
				*f0++ = (real) (((-1) << n) + (*sample++) + 1) * mpg123_muls[n + 1][*sca++];
			else
				*f0++ = 0.0;
			if ((n = *ba++))
				*f1++ = (real) (((-1) << n) + (*sample++) + 1) * mpg123_muls[n + 1][*sca++];
			else
				*f1++ = 0.0;
		}
		for (i = jsbound; i < SBLIMIT; i++)
		{
			if ((n = *ba++))
			{
				real samp = (((-1) << n) + (*sample++) + 1);

				*f0++ = samp * mpg123_muls[n + 1][*sca++];
				*f1++ = samp * mpg123_muls[n + 1][*sca++];
			}
			else
				*f0++ = *f1++ = 0.0;
		}
		for (i = fr->down_sample_sblimit; i < 32; i++)
			fraction[0][i] = fraction[1][i] = 0.0;
	}
	else
	{
		register real *f0 = fraction[0];

		ba = balloc;
		for (sample = smpb, i = 0; i < SBLIMIT; i++)
			if ((n = *ba++))
				*sample++ = mpg123_getbits(n + 1);
		ba = balloc;
		for (sample = smpb, i = 0; i < SBLIMIT; i++)
		{
			if ((n = *ba++))
				*f0++ = (real) (((-1) << n) + (*sample++) + 1) * mpg123_muls[n + 1][*sca++];
			else
				*f0++ = 0.0;
		}
		for (i = fr->down_sample_sblimit; i < 32; i++)
			fraction[0][i] = 0.0;
	}
}

static guint get_track_time_file(FILE * file)
{
	guint32 head;
	guchar tmp[4], *buf;
	struct frame frm;
	xing_header_t xing_header;
	double tpf, bpf;
	guint32 len;

	if (!file)
		return -1;

	fseek(file, 0, SEEK_SET);
	if (fread(tmp, 1, 4, file) != 4)
		return 0;
	head = convert_to_header(tmp);
	while (!mpg123_head_check(head))
	{
		head <<= 8;
		if (fread(tmp, 1, 1, file) != 1)
			return 0;
		head |= tmp[0];
	}
	if (mpg123_decode_header(&frm, head))
	{
		buf = g_malloc(frm.framesize + 4);
		fseek(file, -4, SEEK_CUR);
		fread(buf, 1, frm.framesize + 4, file);
		tpf = mpg123_compute_tpf(&frm);
		if (mpg123_get_xing_header(&xing_header, buf))
		{
			g_free(buf);
			return ((guint) (tpf * xing_header.frames * 1000));
		}
		g_free(buf);
		bpf = mpg123_compute_bpf(&frm);
		fseek(file, 0, SEEK_END);
		len = ftell(file);
		fseek(file, -128, SEEK_END);
		fread(tmp, 1, 3, file);
		if (!strncmp(tmp, "TAG", 3))
			len -= 128;
		return ((guint) ((guint)(len / bpf) * tpf * 1000));
	}
	return 0;
}

static guint get_track_time (gchar *path)
{
    guint result = 0;

    if (path)
    {
	FILE *file = fopen (path, "r");
	result = get_track_time_file (file);
	if (file) fclose (file);
    }
    return result;
}


/* libid3tag stuff */

#include <id3tag.h>
#include "prefs.h"


static gchar* id3_get_string (struct id3_tag *tag, char *frame_name)
{
    const id3_ucs4_t *string;
    struct id3_frame *frame;
    union id3_field *field;
    gchar *utf8 = NULL;
    enum id3_field_textencoding encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

    frame = id3_tag_findframe (tag, frame_name, 0);
    if (!frame) return NULL;

    /* Find the encoding used for the field */
    field = id3_frame_field (frame, 0);
/*     printf ("field: %p\n", field); */
    if (field && (id3_field_type (field) == ID3_FIELD_TYPE_TEXTENCODING))
    {
	encoding = field->number.value;
/*	printf ("encoding: %d\n", encoding); */
    }

    if (frame_name == ID3_FRAME_COMMENT)
	field = id3_frame_field (frame, 3);
    else
	field = id3_frame_field (frame, 1);

/*     printf ("field: %p\n", field); */

    if (!field) return NULL;

    if (frame_name == ID3_FRAME_COMMENT)
	string = id3_field_getfullstring (field);
    else
	string = id3_field_getstrings (field, 0);

/*     printf ("string: %p\n", string); */

    if (!string) return NULL;

    if (frame_name == ID3_FRAME_GENRE)
       string = id3_genre_name (string);

    if (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1)
    {
	/* ISO_8859_1 is just a "marker" -- most people just drop
	   whatever coding system they are using into it, so we use
	   charset_to_utf8() to convert to utf8 */
	id3_latin1_t *raw = id3_ucs4_latin1duplicate (string);
	utf8 = charset_to_utf8 (raw);
	g_free (raw);
    }
    else
    {
	/* Standard unicode is being used -- we won't have to worry
	   about charsets then. */
	utf8 = id3_ucs4_utf8duplicate (string);
    }
    return utf8;
}

static void id3_set_string (struct id3_tag *tag, const char *frame_name, const char *data, enum id3_field_textencoding encoding)
{
    int res;
    struct id3_frame *frame;
    union id3_field *field;
    id3_ucs4_t *ucs4;

    if (data == NULL)
	return;

/*    printf ("updating id3 (enc: %d): %s: %s\n", encoding, frame_name, data);*/

    /*
     * An empty string removes the frame altogether.
     */
    if (strlen(data) == 0)
    {
/*	printf("removing ID3 frame: %s\n", frame_name);*/
	while ((frame = id3_tag_findframe (tag, frame_name, 0)))
	    id3_tag_detachframe (tag, frame);
	return;
    }

    frame = id3_tag_findframe (tag, frame_name, 0);
    if (!frame)
    {
/*	puts("new frame!");*/
	frame = id3_frame_new (frame_name);
	id3_tag_attachframe (tag, frame);
    }

    if (frame_name == ID3_FRAME_COMMENT)
    {
	field = id3_frame_field (frame, 3);
	field->type = ID3_FIELD_TYPE_STRINGFULL;
    }
    else
    {
	field = id3_frame_field (frame, 1);
	field->type = ID3_FIELD_TYPE_STRINGLIST;
    }

    /* Use the specified text encoding */
    id3_field_settextencoding(field, encoding);

    if (frame_name == ID3_FRAME_GENRE)
    {
	id3_ucs4_t *tmp_ucs4 = id3_utf8_ucs4duplicate ((id3_utf8_t *)data);
	int index = id3_genre_number (tmp_ucs4);
	if (index != -1)
	{
	    /* valid genre -- simply store the genre number */
	    gchar *tmp = g_strdup_printf("%d", index);
	    ucs4 = id3_latin1_ucs4duplicate (tmp);
	    g_free (tmp);
	}
	else
	{
	    /* oups -- not a valid genre -- save the entire genre string */
	    if (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1)
	    {
		/* we read 'ISO_8859_1' to stand for 'any locale
		   charset' -- most programs seem to work that way */
		id3_latin1_t *raw = charset_from_utf8 (data);
		ucs4 = id3_latin1_ucs4duplicate (raw);
		g_free (raw);
	    }
	    else
	    {
		/* Yeah! We use unicode encoding and won't have to
		   worry about charsets */
		ucs4 = tmp_ucs4;
		tmp_ucs4 = NULL;
	    }
	}
	g_free (tmp_ucs4);
    }
    else
    {
	if (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1)
	{
	    /* we read 'ISO_8859_1' to stand for 'any locale charset'
	       -- most programs seem to work that way */
	    id3_latin1_t *raw = charset_from_utf8 (data);
	    ucs4 = id3_latin1_ucs4duplicate (raw);
	    g_free (raw);
	}
	else
	{
	    /* Yeah! We use unicode encoding and won't have to
	       worry about charsets */
	    ucs4 = id3_utf8_ucs4duplicate ((id3_utf8_t *)data);
	}
    }

    if (frame_name == ID3_FRAME_COMMENT)
	res = id3_field_setfullstring (field, ucs4);
    else
	res = id3_field_setstrings (field, 1, &ucs4);

    g_free (ucs4);

    if (res != 0)
	g_print(_("Error setting ID3 field: %s\n"), frame_name);
}


/***
 * Reads id3v1.x / id3v2 tag and load data into the Id3tag structure.
 * If a tag entry exists (ex: title), we allocate memory, else value
 * stays to NULL
 * @returns: TRUE on success, else FALSE.
 */
gboolean id3_tag_read (gchar *filename, File_Tag *tag)
{
    struct id3_file *id3file;
    struct id3_tag *id3tag;
    gchar* string;
    gchar* string2;

    if (!filename || !tag)
	return FALSE;

    memset (tag, 0, sizeof (File_Tag));

    if (!(id3file = id3_file_open (filename, ID3_FILE_MODE_READONLY)))
    {
	gchar *fbuf = charset_to_utf8 (filename);
	g_print(_("ERROR while opening file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
	return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file)))
    {
	tag->title = id3_get_string (id3tag, ID3_FRAME_TITLE);
	tag->artist = id3_get_string (id3tag, ID3_FRAME_ARTIST);
	tag->album = id3_get_string (id3tag, ID3_FRAME_ALBUM);
	tag->year = id3_get_string (id3tag, ID3_FRAME_YEAR);
	tag->composer = id3_get_string (id3tag, "TCOM");
	tag->comment = id3_get_string (id3tag, ID3_FRAME_COMMENT);
	tag->genre = id3_get_string (id3tag, ID3_FRAME_GENRE);

	string = id3_get_string (id3tag, "TLEN");
	if (string)
	{
	    tag->songlen = (guint32) strtoul (string, 0, 10);
	    g_free (string);
	}

	string = id3_get_string (id3tag, ID3_FRAME_TRACK);
	if (string)
	{
	    string2 = strchr(string,'/');
	    if (string2)
	    {
		tag->track_total = g_strdup_printf ("%.2d", atoi (string2+1));
		*string2 = '\0';
	    }
	    tag->trackstring = g_strdup_printf ("%.2d", atoi (string));
	    g_free(string);
	}
    }

    id3_file_close (id3file);
    return TRUE;
}

static enum id3_field_textencoding get_encoding_of (struct id3_tag *tag, const char *frame_name)
{
    struct id3_frame *frame;
    enum id3_field_textencoding encoding = -1;

    frame = id3_tag_findframe (tag, frame_name, 0);
    if (frame)
    {
	union id3_field *field = id3_frame_field (frame, 0);
	if (field && (id3_field_type (field) == ID3_FIELD_TYPE_TEXTENCODING))
	    encoding = field->number.value;
    }
    return encoding;
}

/* Find out which encoding is being used. If in doubt, return
 * latin1. This code assumes that the same encoding is used in all
 * fields.  */
static enum id3_field_textencoding get_encoding (struct id3_tag *tag)
{
    enum id3_field_textencoding enc;

    enc = get_encoding_of (tag, ID3_FRAME_TITLE);
    if (enc != -1) return enc;
    enc = get_encoding_of (tag, ID3_FRAME_ARTIST);
    if (enc != -1) return enc;
    enc = get_encoding_of (tag, ID3_FRAME_ALBUM);
    if (enc != -1) return enc;
    enc = get_encoding_of (tag, "TCOM");
    if (enc != -1) return enc;
    enc = get_encoding_of (tag, ID3_FRAME_COMMENT);
    if (enc != -1) return enc;
    enc = get_encoding_of (tag, ID3_FRAME_YEAR);
    if (enc != -1) return enc;
    return ID3_FIELD_TEXTENCODING_ISO_8859_1;
}


/**
 * Write the ID3 tags to the file.
 * @returns: TRUE on success, else FALSE.
 */
gboolean mp3_write_file_info (gchar *filename, Track *track)
{
    struct id3_tag* id3tag;
    struct id3_file* id3file;
    gint error = 0;

    id3file = id3_file_open (filename, ID3_FILE_MODE_READWRITE);
    if (!id3file)
    {
	gchar *fbuf = charset_to_utf8 (filename);
	g_print(_("ERROR while opening file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
	return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file)))
    {
	char *string1;

	enum id3_field_textencoding encoding;

	/* use the same coding as before... */
	encoding = get_encoding (id3tag);
	/* ...unless it's ISO_8859_1 and prefs say we should use
	   unicode (i.e. ID3v2.4) */
	if (prefs_get_id3_write_id3v24 () &&
	    (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1))
	    encoding = ID3_FIELD_TEXTENCODING_UTF_8;

	id3_set_string (id3tag, ID3_FRAME_TITLE, track->title, encoding);
	id3_set_string (id3tag, ID3_FRAME_ARTIST, track->artist, encoding);
	id3_set_string (id3tag, ID3_FRAME_ALBUM, track->album, encoding);
	id3_set_string (id3tag, ID3_FRAME_GENRE, track->genre, encoding);
	id3_set_string (id3tag, ID3_FRAME_COMMENT, track->comment, encoding);
	id3_set_string (id3tag, "TCOM", track->composer, encoding);
	if (track->tracks)
	    string1 = g_strdup_printf ("%d/%d",
				       track->track_nr, track->tracks);
	else
	    string1 = g_strdup_printf ("%d", track->track_nr);
	id3_set_string (id3tag, ID3_FRAME_TRACK, string1, encoding);
	g_free(string1);
    }

    if (id3_file_update(id3file) != 0)
    {
	gchar *fbuf = charset_to_utf8 (filename);
	g_print(_("ERROR while writing tag to file: '%s' (%s).\n"),
		fbuf, g_strerror(errno));
	g_free (fbuf);
	return FALSE;
    }

    id3_file_close (id3file);

    if (error) return FALSE;
    else       return TRUE;
}


/*
 * Code to read the ReplayGain Values stored by LAME in its own tag.
 *
 * Most of the relevant information has been extracted from them LAME sources
 * (http://lame.sourceforge.net/).
 * The "Mp3 info Tag rev 1 specifications - draft 0"
 * (http://gabriel.mp3-tech.org/mp3infotag.html) by Gabriel Bouvigne describes
 * the actual Tag (except for small changes).
 * Details on the actual ReplayGain fields have been obtained from
 * http://www.replaygain.org .
 *
 * Apart from that, some information has been derived from phwip's LameTag
 * (http://www.silisoftware.com/applets/?scriptname=LameTag)
 *
 * 
 * Code to read the ReplayGain Values stored in an Ape tag.
 *
 * Info on Lyrics3 V2.00 can be found at:
 * http://www.id3.org/lyrics3200.html
 * On the actual Ape Tag V2.0:
 * http://www.personal.uni-jena.de/~pfk/mpp/sv8/apetag.html
 *
 * Copyright (C) 2004 Jens Taprogge <jens.taprogge at post.rwth-aachen.de>
 *
 * Provided under GPL according to Jens Taprogge. (JCS -- 12 March 2004)
 */

#define TAG_FOOTER		0x10
#define LAME_OFFSET		0x74
#define SIDEINFO_MPEG1_MONO	17
#define SIDEINFO_MPEG1_MULTI	32
#define SIDEINFO_MPEG2_MONO	9
#define SIDEINFO_MPEG2_MULTI	17
#define ID3V1_SIZE		0x80
#define APE_FOOTER_SIZE 	0x20
#define LYRICS_FOOTER_SIZE 	0x0f


static gint lame_vcmp(gchar a[5], gchar b[5]) {
	int r;

	r = strncmp(a, b, 4);
	if (r) return r;

	if (a[4] == b[4]) return 0;

	/* check for '.': indicates subminor version */
	if (a[4] == '.') return 1;
	if (b[4] == '.') return -1;
	
	/* check for alpha or beta versions */
	if (a[4] == ' ') return 1;
	if (b[4] == ' ') return -1;

	/* check for alpha, beta etc. indicated by a, b... */
	return strncmp(&a[4], &b[4], 1);
}


static void read_lame_replaygain(char buf[], Track *track, int gain_adjust) {
	char oc, nc;
	gint gain;
	ExtraTrackData *etr;

	g_return_if_fail (track);
	etr = track->userdata;
	g_return_if_fail (etr);

	/* check originator */
	oc = (buf[0] & 0x1c) >> 2;
	if ((oc <= 0) || (oc > 3)) return;

	/* check name code */
	nc = buf[0] & 0xe0;
	if (!((nc == 0x20) || (nc == 0x40))) return;
	
	gain = ((buf[0] & 0x1) << 8) + buf[1];
	
	/* This would be a value of -0.
	 * That value however is illegal by current standards and reserved for
	 * future use. */
	if ((!gain) && (buf[0] & 0x02)) return;
	
	if (buf[0] & 2) gain = -gain;
	
	gain += gain_adjust;

	switch (nc) {
		case 0x20:
			if (etr->radio_gain_set) return;
			etr->radio_gain = (gdouble)gain / 10;
			etr->radio_gain_set = TRUE;
/*			printf("radio_gain (lame): %i\n", etr->radio_gain); */
			break;
		case 0x40:
			if (etr->audiophile_gain_set) return;
			etr->audiophile_gain = (gdouble)gain / 10;
			etr->audiophile_gain_set = TRUE;
/*			printf("audiophile_gain (lame): %i\n", 
					etr->audiophile_gain);*/
			break;
	}
}


static inline guint32 parse_ape_uint32(char *buf) {
	return (buf[0] & 0xff) | (buf[1] & 0xff) << 8 
		| (buf[2] & 0xff) << 16 | (buf[3] & 0xff) << 24;
}

static inline guint32 parse_lame_uint32(char *buf) {
	return (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 
		| (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
}


/* 
 * mp3_get_track_lame_replaygain - read the specified file and scan for LAME Tag
 * ReplayGain information.
 *
 * @path: localtion of the file
 * @track: structure holding track information
 *
 * FIXME: Are there other encoders writing a LAME Tag using a different magic
 * string?
 * TODO: Check CRC.
 */

gboolean mp3_get_track_lame_replaygain(gchar *path, Track *track)
{
	struct {
		/* All members are defined in terms of chars so padding does not
		 * occur. Is there a cleaner way to keep the compiler from
		 * padding? */
		
		char     id[3];
		char     version[2];
		char     flags;
		char     size[4];
	} id3head;

	FILE *file = NULL;
	char buf[4], version[5];
	int gain_adjust = 0;
	int sideinfo;
	guint32 ps;
	ExtraTrackData *etr;

	g_return_val_if_fail (track, FALSE);
	etr = track->userdata;
	g_return_val_if_fail (etr, FALSE);

	etr->radio_gain = 0;
	etr->audiophile_gain = 0;
	etr->peak_signal = 0;
	etr->radio_gain_set = FALSE;
	etr->audiophile_gain_set = FALSE;
	etr->peak_signal_set = FALSE;
	
	if (!path)
		goto rg_fail;
	
	file = fopen (path, "r");
	
	/* Skip ID3 header if appropriate */
	if (fread(&id3head, 1, sizeof(id3head), file) != 
			sizeof(id3head))
		goto rg_fail;
	
	if (!strncmp(id3head.id, "ID3", 3)) {
		int realsize = 0;
		
		realsize = (id3head.size[0] & 0x7f) << 21 
			| (id3head.size[1] & 0x7f) << 14 
			| (id3head.size[2] & 0x7f) << 7 
			| (id3head.size[3] & 0x7f);

		if (id3head.flags & TAG_FOOTER) {
			/* footer is copy of header */
			realsize += sizeof(id3head);
		}

		if (fseek(file, realsize-1, SEEK_CUR) ||
				(!fread(&buf[0], 1, 1, file)))
			goto rg_fail;
	} else {
		/* no ID3 Tag - go back */
		fseek(file, -sizeof(id3head), SEEK_CUR);
	}

	/* Search Xing header. The location is dependant on the MPEG Layer and
	 * whether the stream is mono or not. */
	if (fread(buf, 1, 4, file) != 4) goto rg_fail;
	
	/* should start with 0xff 0xf? (synch) */
	if (((buf[0] & 0xff) != 0xff) 
			|| ((buf[1] & 0xf0) != 0xf0)) goto rg_fail;
	
	/* determine the length of the sideinfo */
	if (buf[1] & 0x08) {
		sideinfo = ((buf[3] & 0xc0) == 0xc0) ? 
			SIDEINFO_MPEG1_MONO : SIDEINFO_MPEG1_MULTI;
	} else {
		sideinfo = ((buf[3] & 0xc0) == 0xc0) ? 
			SIDEINFO_MPEG2_MONO : SIDEINFO_MPEG2_MULTI;
	}
	
	if (fseek(file, sideinfo, SEEK_CUR) ||
			(fread(&buf[0], 1, 4, file) != 4))
		goto rg_fail;
	
	/* Is this really a Xing or Info Header? 
	 * FIXME: Apparently (according to madplay sources) there is a different
	 * possible location for the Xing header ("due to an unfortunate
	 * historical event"). I do not thing we need to care though since
	 * RaplayGain information is only conatined in recent files.  */
	if (strncmp(buf, "Xing", 4) && strncmp(buf, "Info", 4))
		goto rg_fail;

	/* Check for LAME Tag */
	if (fseek(file, LAME_OFFSET, SEEK_CUR) ||
				(fread(&buf[0], 1, 4, file) != 4))
		goto rg_fail;
	if (strncmp(buf, "LAME", 4))
		goto rg_fail;
	
	/* Check LAME Version */
	if (fread(version, 1, 5, file) != 5)
		goto rg_fail;
	
	/* Skip really old versions altogether. I am not sure when radio_gain
	 * information was introduced. 3.89 does not seem to supprt it though.
	 * */
	if (lame_vcmp(version, "3.90") < 0) {
/*		fprintf(stderr, "Old lame version (%c%c%c%c%c). Not used.\n",
				version[0], version[1], version[2], version[3], version[4]); */
		goto rg_fail;
	}
		
	if (fseek(file, 0x2, SEEK_CUR) || (fread(buf, 1, 4, file) != 4))
		goto rg_fail;

	/* get the peak signal. */
	ps = parse_lame_uint32(buf);
	
	/* Don't know when fixed-point PeakSingleAmplitude
	 * was introduced exactly. 3.94b will be used for now.) */
	if ((lame_vcmp(version, "3.94b") >= 0)) {
		if ((!etr->peak_signal_set) && ps) {
			etr->peak_signal = ps;
			etr->peak_signal_set = TRUE;
/*			printf("peak_signal (lame): %f\n", (double)
					etr->peak_signal / 0x800000);*/
		}
	} else {
		float f = *((float *) (void *) (&ps)) * 0x800000;
		etr->peak_signal = (guint32) f;
		/* I would like to see an example of that. */
/*		printf("peak_signal (lame floating point): %f. PLEASE report.\n", 
				(double) etr->peak_signal / 0x800000);*/
	}

	/*
	 * Versions prior to 3.95.1 used a reference volume of 83dB.
	 * (As compared to the currently used 89dB.)
	 */
	if ((lame_vcmp(version, "3.95.") < 0)) {
		gain_adjust = 60;
/*		fprintf(stderr, "Old lame version (%c%c%c%c%c). Adjusting gain.\n",
				version[0], version[1], version[2], version[3], version[4]); */
	}

	if (fread(&buf[0], 1, 2, file) != 2)
		goto rg_fail;

	/* radio gain */
	read_lame_replaygain (buf, track, gain_adjust);

	if (fread(&buf[0], 1, 2, file) != 2)
		goto rg_fail;

	/* audiophile gain */
	read_lame_replaygain (buf, track, gain_adjust);

	fclose(file);
	return TRUE;

rg_fail:
	if (file)
		fclose(file);
	return FALSE;
}


/* 
 * mp3_get_track_ape_replaygain - read the specified file and scan for Ape Tag
 * ReplayGain information.
 *
 * @path: localtion of the file
 * @track: structure holding track information
 *
 * The function only modifies the gains if they have not previously been set.
 */

gboolean mp3_get_track_ape_replaygain(gchar *path, Track *track)
{
	/* The Ape Tag is located a t the end of the file. Or at least that
	 * seems where it can most likely be found. Either it is at the very end
	 * or before a trailing ID3v1 Tag. Sometimes a Lyrics3 Tag is placed
	 * between the ID3v1 and the Ape Tag.
	 * If you find files that have the Tags located in different
	 * positions please let me know. */

	FILE *file = NULL;
	char buf[16];
	char *dbuf = NULL, *ep;

	int offset = 0;
	int i;
	int pos = 0, pos2 = 0;
	guint32 version;
	guint32 data_length;
	guint32 entry_length = 0;
	guint32 entries;
	double d;
	ExtraTrackData *etr;

	g_return_val_if_fail (track, FALSE);
	etr = track->userdata;
	g_return_val_if_fail (etr, FALSE);
	g_return_val_if_fail (path, FALSE);

	file = fopen (path, "r");

	/* check for ID3v1 Tag */
	if (fseek(file, -ID3V1_SIZE, SEEK_END) ||
			fread(&buf, 1, 3, file) != 3)
		goto rg_fail;
	if (!strncmp(buf, "TAG", 3)) offset -= ID3V1_SIZE;

	/* check for Lyrics3 Tag */
	if (fseek(file, -9 + offset, SEEK_END) ||
			fread(&buf, 1, 9, file) != 9)
		goto rg_fail;
	if (!strncmp(buf, "LYRICS200", 9)) {
		if (fseek(file, -LYRICS_FOOTER_SIZE + offset, SEEK_END) ||
				fread(&buf, 1, 9, file) != 9)
			goto rg_fail;
		data_length = buf[0] - '0';
		for (i = 1; i < 6; i++) {
			data_length *= 10;
			data_length += buf[i] - '0';
		}
		if (fseek(file, -LYRICS_FOOTER_SIZE - data_length + offset,
					SEEK_END) ||
				fread(&buf, 1, 11, file) != 11)
			goto rg_fail;
		if (!strncmp(buf, "LYRICSBEGIN", 11)) 
			offset -= LYRICS_FOOTER_SIZE + data_length;
	}

	/* check for APE Tag */
	if (fseek(file, -APE_FOOTER_SIZE + offset, SEEK_END) ||
			fread(&buf, 1, 8, file) != 8)
		goto rg_fail;
	if (strncmp(buf, "APETAGEX", 8)) goto rg_fail;

	/* Check the version of the tag. 1000 and 2000 (v1.0 and 2.0) are the
	 * only ones I know about. Make suer things do not break in the future.
	 * */
	if (fread(&buf, 1, 4, file) != 4)
		goto rg_fail;
	version = parse_ape_uint32(buf);
	if (version != 1000 && version != 2000)
		goto rg_fail;

	/* determine data length */
	if (fread(&buf, 1, 4, file) != 4)
		goto rg_fail;
	data_length = parse_ape_uint32(buf) - APE_FOOTER_SIZE;

	/* determine number of entries */
	if (fread(&buf, 1, 4, file) != 4)
		goto rg_fail;
	entries = parse_ape_uint32(buf);

	/* seek to first entry and read the whole buffer*/
	if (fseek(file, -APE_FOOTER_SIZE + offset - data_length, SEEK_END))
		goto rg_fail;
	if (!(dbuf = malloc(data_length)))
		goto rg_fail;
	if (fread(dbuf, 1, data_length, file) != data_length)
		goto rg_fail;
	
	for (i = 0; i < entries; i++) {
		if (etr->radio_gain_set && etr->peak_signal_set) break;
		pos = pos2 + entry_length;
		if (pos > data_length - 10) break;
		
		entry_length = parse_ape_uint32(&dbuf[pos]); pos += 4;
		pos += 4;
		
		pos2 = pos;
		while (dbuf[pos2] && pos2 < data_length) pos2++;
		if (pos2 == data_length) break;
		pos2++;
		
		if (entry_length + 1 > sizeof(buf))
			continue;
/* 		printf ("%s:%d:%d\n",&dbuf[pos], pos2, pos); */
		if (!etr->radio_gain_set && !strcasecmp(&dbuf[pos],
					"REPLAYGAIN_TRACK_GAIN")) {
			memcpy(buf, &dbuf[pos2], entry_length);
			buf[entry_length] = '\0';
			
			d = g_ascii_strtod(buf, &ep);
/* 			printf("%f\n", d); */
			if ((ep == buf + entry_length - 3) 
					&& (!strncasecmp(ep, " dB", 3))) {
			    etr->radio_gain = d;
				etr->radio_gain_set = TRUE;
/*				printf("radio_gain (ape): %i\n", etr->radio_gain);*/
			}
			
			continue;
		}
		if (!etr->peak_signal_set && !strcasecmp(&dbuf[pos],
					"REPLAYGAIN_TRACK_PEAK")) {
			memcpy(buf, &dbuf[pos2], entry_length);
			buf[entry_length] = '\0';
			
			d = g_ascii_strtod(buf, &ep);
			if (ep == buf + entry_length) {
				d *= 0x800000;
				etr->peak_signal = (guint32) floor(d + 0.5);
				etr->peak_signal_set = TRUE;
/*				printf("peak_signal (ape): %f\n", (double) etr->peak_signal / 0x800000);*/
			}

			continue;
		}
	}

	free(dbuf);
	fclose(file);
	return TRUE;

rg_fail:
	if (dbuf)
		free(dbuf);
	if (file)
		fclose(file);
	return FALSE;
}


/* ----------------------------------------------------------------------

	      mp3gain code

---------------------------------------------------------------------- */

#include <sys/wait.h>
#include <fcntl.h>


/* this function initiates mp3gain on a given track and causes it to write the
 * Radio Replay Gain and Peak to an Ape Tag */

/* mp3gain version 1.4.2 */
static gboolean mp3_calc_gain (gchar *path, Track *track)
{
    gint k,n;  /*for's counter*/
    gchar *mp3gain_path;
    gchar *mp3gain_exec;
    const gchar *mp3gain_set;
    pid_t pid,tpid;
    int ret = 2;
    int status;
    int errsv;
    int fdnull;

    k=0;
    n=0;

    /* see if full path to mp3gain was set using the prefs dialogue */
    mp3gain_set = prefs_get_path (PATH_MP3GAIN);
    /* use default if not */
    if (!mp3gain_set || !(*mp3gain_set)) mp3gain_set = "mp3gain";
    /* find full path */
    mp3gain_path = g_find_program_in_path (mp3gain_set);
    /* show error message if mp3gain cannot be found */
    if (!mp3gain_path)
    {
	gtkpod_warning (_("Could not find mp3gain. I tried to use the following "
				"executable: '%s'.\n\nIf the mp3gain executable "
				"is not in your path or named differently, you "
				"can set the full path in the 'Tools' section of "
				"the preferences dialog.\n\nIf you do not have "
				"mp3gain installed, you can download it from "
				"http://www.sourceforge.net/projects/mp3gain."),
			mp3gain_set);
	return FALSE;
    }

    mp3gain_exec = g_path_get_basename (mp3gain_path);

    /*fork*/
    pid=fork();

    /*and cast mp3gain*/
    switch (pid)
    {
    case -1: /* parent and error, now what?*/
	break;
    case 0: /*child*/
	/* redirect output to /dev/null */
	if ((fdnull = open("/dev/null", O_WRONLY | O_NDELAY)) != -1) {
		dup2(fdnull, fileno(stdout));
	}
	
	/* this call may add a tag to the mp3file!! */
        execl(mp3gain_path, mp3gain_exec, 
			"-q", /* quiet */
			"-k", /* set ReplayGain so that clipping is prevented */
			path, NULL);
	errsv = errno;
        fprintf(stderr, "execl() failed: %s\n", strerror(errsv));
	/* mp3gain (can) return 1 on success. So only values greater 1 can
	 * designate failure. */
	exit(2);
    default: /*parent*/
	tpid = waitpid (pid, &status, 0); /*wait mp3gain termination */
	if WIFEXITED(status) ret = WEXITSTATUS(status);
	if (ret > 1) gtkpod_warning (_("Execution of mp3gain ('%s') failed."),
			mp3gain_set);
	break;
    }/*end switch*/

    /*free everything left*/
    g_free (mp3gain_path);
    g_free (mp3gain_exec);

    /*and happily return the right value*/
    return (ret > 1) ? FALSE : TRUE;
}




/* 
 * mp3_get_gain - try to read the ReplayGain values from the LAME or Ape
 * Tags. If that does not work run mp3gain. And try to read again.
 *
 * @path: localtion of the file
 * @track: structure holding track information
 *
 * The function always rereads the gains. Even if they have been
 * previuosly known.
 *
 * Returns TRUE if at least the radio_gain could be read.
 */

gboolean mp3_get_gain (gchar *path, Track *track) 
{
    ExtraTrackData *etr;

    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);

    etr->radio_gain_set = FALSE;
    etr->audiophile_gain_set = FALSE;
    etr->peak_signal_set = FALSE;

    mp3_get_track_lame_replaygain (path, track);
/*    printf ("%d:%d\n", track->radio_gain_set, track->peak_signal_set); */
    if (etr->radio_gain_set && etr->peak_signal_set) return TRUE;
    mp3_get_track_ape_replaygain (path, track);
    if (etr->radio_gain_set) return TRUE;
	    
    if (mp3_calc_gain (path, track))
	mp3_get_track_ape_replaygain (path, track);
    if (etr->radio_gain_set) return TRUE;
    return FALSE;
}



/* 
 * mp3_read_gain - as mp3_get_gain() above, but will never run
 * mp3gain.
 *
 * Returns TRUE if at least the radio_gain could be read.
 */

gboolean mp3_read_gain (gchar *path, Track *track) 
{
    ExtraTrackData *etr;

    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);

    etr->radio_gain_set = FALSE;
    etr->audiophile_gain_set = FALSE;
    etr->peak_signal_set = FALSE;

    mp3_get_track_lame_replaygain(path, track);
    if (etr->radio_gain_set && etr->peak_signal_set) return TRUE;

    mp3_get_track_ape_replaygain(path, track);
    if (etr->radio_gain_set) return TRUE;
    
    return FALSE;
}



/* ----------------------------------------------------------------------

	      From here starts original gtkpod code

---------------------------------------------------------------------- */

/* Return a Track structure with all information read from the mp3
   file filled in */
Track *mp3_get_file_info (gchar *name)
{
    Track *track = NULL;
    File_Tag filetag;
    mp3info *mp3info;

    track = gp_track_new ();
    if (prefs_get_readtags() && (id3_tag_read (name, &filetag) == TRUE))
    {
	track->fdesc = g_strdup ("MPEG audio file");

	if (filetag.album)
	{
	    track->album = filetag.album;
	}

	if (filetag.artist)
	{
	    track->artist = filetag.artist;
	}

	if (filetag.title)
	{
	    track->title = filetag.title;
	}

	if (filetag.genre)
	{
	    track->genre = filetag.genre;
	}

	if (filetag.composer)
	{
	    track->composer = filetag.composer;
	}

	if (filetag.comment)
	{
	    track->comment = filetag.comment;
	}
	if (filetag.year == NULL)
	{
	    track->year = 0;
	}
	else
	{
	    track->year = atoi(filetag.year);
	    g_free (filetag.year);
	}

	if (filetag.trackstring == NULL)
	{
	    track->track_nr = 0;
	}
	else
	{
	    track->track_nr = atoi(filetag.trackstring);
	    g_free (filetag.trackstring);
	}

	if (filetag.track_total == NULL)
	{
	    track->tracks = 0;
	}
	else
	{
	    track->tracks = atoi(filetag.track_total);
	    g_free (filetag.track_total);
	}
    }

    mp3_read_gain (name, track);

    /* Get additional info (play time and bitrate */
    mp3info = mp3file_get_info (name);
    if (mp3info)
    {
	track->tracklen = mp3info->milliseconds;
	track->bitrate = (gint)(mp3info->vbr_average);
 	track->samplerate = mp3file_header_frequency (&mp3info->header);
	g_free (mp3info);
    }
    /* Fall back to xmms code if tracklen is 0 */
    if (track->tracklen == 0)
    {
	track->tracklen = get_track_time (name);
	if (track->tracklen)
	    track->bitrate = (float)track->size*8/track->tracklen;
    }

    if (track->tracklen == 0)
    {
	/* Tracks with zero play length are ignored by iPod... */
	gtkpod_warning (_("File \"%s\" has zero play length. Ignoring.\n"),
			name);
	gp_track_free (track);
	track = NULL;
    }

    return track;
}
