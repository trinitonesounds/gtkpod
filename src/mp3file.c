/*
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to
|  compile "standalone" with the gtkpod project, to return time in
|  ms as integer, and to accept the filesize as integer.. 2002/11/28
*/


/* mp3file.c
   Accessing the metadata of a raw MPEG stream
   Copyright (C) 2001 Linus Walleij

This file is part of the GNOMAD package.

GNOMAD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

You should have received a copy of the GNU General Public License
along with GNOMAD; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA. 

*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "mp3file.h"

/* This code is an apapted version of the file playlength
 * detection algorithm written by Michael Mutschler for the
 * Windows program "mp3ext". Thanks a lot for your great
 * code Michael! */


/* definition of the first 4 bytes in an MP3-Frame
 * 33222222222211111111110000000000
 * 10987654321098765432109876543210
 * |  b0  ||  b1  ||  b2  ||  b3  |
 *
 * 31-21 = sync (0xfff) (or search the stream until sync is found)
 * 20-19 = version	( 2=MPEG2, 3=MPEG1 0=MPEG2.5)
 * 18-17 = layer   (Layernumber = 4 - layer)
 * 16    = error protect
 * 15-12 = bitrate index
 * 11-10 = sample rate
 * 9     = pad
 * 8     = extension
 * 7-6   = mode
 * 5-4   = mode extension
 * 3     = Copyright
 * 2     = original
 * 1-0   = emphasis
 */

// VBR:
// 4   Xing
// 4   flags
// 4   frames
// 4   bytes
// 100 toc

static int s_freq[4] = {44100, 48000, 32000, 0};

static int bitrate[3][16] = {
	{0,32000,64000,96000,128000,160000,192000,224000,256000,288000,320000,352000,384000,416000,448000, 0},
	{0,32000,48000,56000,64000,80000,96000,112000,128000,160000,192000,224000,256000,320000,384000, 0},
	{0,32000,40000,48000,56000,64000,80000,96000,112000,128000,160000,192000,224000,256000,320000, 0}
};

static int bitrate3_2[16] = {		/* bitraten sind ausprobiert */
//	0,16000,20000,24000,32000,40000,48000,56000,64000,80000,56000,128000,0,0,0,0
	0,16000,20000,24000,32000,64000,80000,56000,64000,128000,160000,112000,128000,256000,320000,0
};

static int bitrate3_25[16] = {		/* bitraten sind ausprobiert */
	0,8000,16000,24000,32000,0,0,0,0,0,0,0,0,0,0
};

/* unused ??
static char *ModeNameA[4] = {  "Stereo",  "Joint-Stereo",  "Dual-Channel",  "Single-Channel" };
static char *ModeNameShortA[4] = {  "S",  "JS",  "DC",  "SC" };
static char *LayerNameA[5] = { "-",  "I",  "II",  "III",  "-"};
static char *VersionNameA[5] = { "-",  "1",  "2",  "-",  "2.5"};
*/

/* VBR definitions */
#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define FRAMES_AND_BYTES (FRAMES_FLAG | BYTES_FLAG)

/* Get the big endian representation from a
 * character buffer */
static int ExtractI4(unsigned char *buf)
{
  int x;

  x = buf[0];
  x <<= 8;
  x |= buf[1];
  x <<= 8;
  x |= buf[2];
  x <<= 8;
  x |= buf[3];
  
  return x;
}

static void get_header_info(unsigned char* head, mp3metadata_t *mp3meta)
{
  unsigned char *vbr;

  // since the first byte if 0xff, we just increase head
  if (head[0] != 0xff)
    return; // Not so good
  head++;

  mp3meta->VbrScale = -1;	// no scale
  mp3meta->VariableBitrate = 0;	// no VBR
  
  mp3meta->Version = 4-((head[0]>>3)&0x3);
  mp3meta->Layer = 4-((head[0]>>1)&0x3);
  mp3meta->Bitrate = bitrate[mp3meta->Layer-1][head[1]>>4];
  mp3meta->SampleFrequency = s_freq[(head[1]>>2)&0x3];
  mp3meta->Mode = (head[2]>>6)&0x3;

  // check for VBR
  if (mp3meta->Version&1)
    {        // mpeg1
      if (mp3meta->Mode != 3)	vbr = &head[32+4-1];		// version == 1
      else						vbr = &head[17+4-1];
    }
  else {      // mpeg2
    if (mp3meta->Mode != 3 )	vbr = &head[17+4-1];	// version == 2
    else						vbr = &head[9+4-1];
  }
  if (vbr[0] == 'X' &&
      vbr[1] == 'i' &&
      vbr[2] == 'n' &&
      vbr[3] == 'g')
    {
      // found a vbr header
      int bytes, head_flags;
      vbr += 4;
      if (mp3meta->Version == 2 && mp3meta->Layer == 3)
	mp3meta->SampleFrequency /= 2;
      
      mp3meta->VariableBitrate = 1;
      
      head_flags = ExtractI4(vbr); vbr+=4;      // get flags
      
      mp3meta->Frames = 0;
      if (head_flags & FRAMES_FLAG)
	{
	  mp3meta->Frames = ExtractI4(vbr);
	  vbr+=4;
	}
      
      bytes = 0;
      if (head_flags & BYTES_FLAG) {
	bytes = ExtractI4(vbr);
	vbr+=4;
      }
      
      if (head_flags & TOC_FLAG ) {
	//			 if (X->toc != NULL ) {
	//				  for(i=0;i<100;i++) X->toc[i] = vbr[i];
	//			 }
	vbr+=100;
      }
      
      if (head_flags & VBR_SCALE_FLAG )
	{
	  mp3meta->VbrScale = ExtractI4(vbr);
	  vbr+=4;
	}
      mp3meta->Bitrate = 0;
      if (mp3meta->Frames > 0) {
	mp3meta->Bitrate = (((bytes!=0)?bytes:mp3meta->FileSize)/mp3meta->Frames)*mp3meta->SampleFrequency/144;
	// round the bitrate:
	mp3meta->Bitrate -= mp3meta->Bitrate%1000;
      }
    }
  else
    {	// no vbr
      // in case of MPEG2 & Layer 3:
      if (mp3meta->Version == 2 && mp3meta->Layer == 3) {
	mp3meta->Bitrate = bitrate3_2[head[1]>>4];
	mp3meta->SampleFrequency /= 2;
      }
      // in case of MPEG2.5 & Layer 3:
      if (mp3meta->Version == 4 && mp3meta->Layer == 3) {
	mp3meta->Bitrate = bitrate3_25[head[1]>>4];
	mp3meta->SampleFrequency /= 4;
      }
      
      // check if we have a valid bitrate...
      mp3meta->Frames = 0;
      if (mp3meta->Bitrate >0)
	mp3meta->Frames = (int)(((float)mp3meta->FileSize*mp3meta->SampleFrequency)/144/mp3meta->Bitrate);
      /*		mp3meta->Frames = mp3meta->SampleFrequency/144;
			mp3meta->Frames *= mp3meta->FileSize;
			mp3meta->Frames /= mp3meta->Bitrate;
      */	}
  // if Bitrate == 0, then the header is wrong
  mp3meta->PlayLength = 0;
  if (mp3meta->Bitrate > 0)
    mp3meta->PlayLength = (mp3meta->FileSize*8)/mp3meta->Bitrate;

  /* g_print("Header with bitrate %u filesize %u and length %u...\n", mp3meta->Bitrate, mp3meta->FileSize, mp3meta->PlayLength); */
}

static mp3metadata_t *get_metadata(FILE *mp3file, size_t mp3filesize)
{
  unsigned char head[4096];
  size_t readbytes, i, FrameCount;
#define NUMFRAMES 8 // number of headers to look for
  static mp3metadata_t mp3meta[NUMFRAMES];
  mp3metadata_t *returnmeta;

  /* Clear all metadata posts */
  memset(&mp3meta, 0, sizeof(mp3meta));
  returnmeta = g_malloc0 (sizeof (mp3metadata_t));

  /* Set all header frames file sizes to mp3filesize */
  for (i = 0; i < NUMFRAMES; i++) {
    mp3meta[i].FileSize = mp3filesize;
  }
  
  /* Read 2048 bytes from the mp3 file */
  readbytes = fread(head, 1, sizeof(head), mp3file);
  /* g_print("Read %u bytes from file...\n", readbytes); */

  /* Scan for NUMFRAMES MPEG frames */
  FrameCount = 0;
  /* scan for the sync */
  for(i=0; (FrameCount<NUMFRAMES) && (i+2)<readbytes; i++) {
    /* only check on byte-boundary */
    if (head[i] == 0xff && (head[i+1]&0xe0) == 0xe0)
      {
	get_header_info(&head[i], &mp3meta[FrameCount]);
	// check if everything is OK
	if (mp3meta[FrameCount].Bitrate > 0 &&
	    mp3meta[FrameCount].Layer >= 2 &&
	    ((FrameCount==0) || 
	     (mp3meta[FrameCount].SampleFrequency == mp3meta[0].SampleFrequency))
	    // Freq is always the same
	    )
	  {
	    FrameCount++;
	  }
      }
  }

  /* If some frames were found, copy data and calculate playlength */
  if (FrameCount>0)
    {
      /* Copy the 1. frame, for the data */
      memcpy(returnmeta, &mp3meta[0], sizeof(mp3metadata_t));
      /* now adjust the bitrate if MPEG Version 2.5: */
      if (returnmeta->Version == 4)
	{
	  int br = 0;
	  for (i=0; i<FrameCount; i++)
	    br += mp3meta[i].Bitrate;
	  returnmeta->Bitrate = br/FrameCount;
	}
    }
  
  /* Calculate the playlength */
  returnmeta->PlayLength = (returnmeta->Bitrate==0)?0:((returnmeta->FileSize)*8)/returnmeta->Bitrate;
  return returnmeta;
}


/* you need to g_free the returned metadata */
mp3metadata_t *get_mp3metadata_from_file(gchar *path, guint32 filesize)
{
  size_t size = 0;
  size_t length = 0;
  FILE *InFile;
  unsigned char buf[10];
  gint i;
  mp3metadata_t *mp3meta;

  if (!filesize)
    return NULL;
  else
    size = filesize;

  InFile = fopen(path, "r");
  if (InFile == NULL)
    return 0;
  i = fread(buf, 1, 10, InFile);
  if (ferror(InFile))
    return 0;

  // Skip past ID3 header
  if(buf[0] != 0xff) {
    if(!((buf[6]|buf[7]|buf[8]|buf[9]) & 0x80) &&
       buf[0] == 'I' &&
       buf[1] == 'D' &&
       buf[2] == '3') {

      size_t offset = 0;

      length  = (unsigned long) buf[6] & 0x7f;
      length <<= 7;
      length += (unsigned long) buf[7] & 0x7f;
      length <<= 7;
      length += (unsigned long) buf[8] & 0x7f;
      length <<= 7;
      length += (unsigned long) buf[9] & 0x7f;

      offset = (length + 10);
      
      /* g_print("Skipping an ID3 tag of %lu bytes...\n", offset); */
      /* FIXME: add a hook to subtract ID3v1 tag length from file totals too */
    }
  }

  mp3meta = get_metadata(InFile, size);
  fclose(InFile);
  /* g_print("%s is %u seconds long.\n", path, mp3meta.PlayLength); */
  return mp3meta;
}

/* ------------------------------------------------------------ */
/* from here it's xmms' mp123 input code taken from xmms V1.2.7 */

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
/* 			fr->do_layer = mpg123_do_layer1; */
/* 			mpg123_init_layer2();	/\* inits also shared tables with layer1 *\/ */
			fr->framesize = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
			break;
		case 2:
/* 			fr->do_layer = mpg123_do_layer2; */
/* 			mpg123_init_layer2();	/\* inits also shared tables with layer1 *\/ */
			fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize += fr->padding - 4;
			break;
		case 3:
/* 			fr->do_layer = mpg123_do_layer3; */
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

static guint get_song_time_file(FILE * file)
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

guint get_song_time (gchar *path)
{
    FILE *file = fopen (path, "r");
    guint result = get_song_time_file (file);
    fclose (file);
    return result;
}
