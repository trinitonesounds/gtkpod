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
