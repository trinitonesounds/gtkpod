/* mp3file.h
|
|  Changed by Jorg Schuler <jcsjcs at users.sourceforge.net> to
|  compile "standalone" with the gtkpod project. 2003/05/11
|
|  $Id$
*/

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

#ifndef MP3FILEH_INCLUDED
#define MP3FILEH_INCLUDED 1

#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

/* These are for mp3info code */
gint mp3file_header_layer(mp3header *h);
gint mp3file_header_bitrate(mp3header *h);
gchar *mp3file_header_emphasis(mp3header *h);
gchar *mp3file_header_mode(mp3header *h);
int mp3file_header_frequency(mp3header *h);
mp3info *mp3file_get_info (gchar *name);

/* This is for xmms code */
guint get_song_time(gchar *path);
#endif
