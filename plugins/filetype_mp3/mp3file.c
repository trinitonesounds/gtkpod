/* Time-stamp: <2009-01-31 18:21:09 jcs>
 |
 |  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
 |  Part of the gtkpod project.
 |
 |  URL: http://www.gtkpod.org/
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

#define LOCALDEBUG 0

#if LOCALDEBUG
#  define DEBUG(args...) printf(args)
#else
#  define DEBUG(args...)
#endif

/* The code in the first section of this file is taken from the
 * mp3info (http://www.ibiblio.org/mp3info/) project. Only the code
 * needed for the playlength calculation has been extracted. */

/* The code in the second section of this file is taken from the
 * mpg123 code used in xmms-1.2.7 (Input/mpg123). Only the code needed
 * for the playlength calculation has been extracted. */

/* The code in the third section of this file is taken from the
 * crc code used in libmad (crc.h). */

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
typedef struct _GainData GainData;
typedef struct _GaplessData GaplessData;
typedef struct _LameTag LameTag;

struct _File_Tag {
    gchar *title; /* Title of track */
    gchar *artist; /* Artist name */
    gchar *album; /* Album name */
    gchar *year; /* Year of track */
    gchar *trackstring; /* Position of track in the album */
    gchar *track_total; /* The number of tracks for the album (ex: 12/20) */
    gchar *genre; /* Genre of song */
    gchar *comment; /* Comment */
    gchar *composer; /* Composer */
    guint32 songlen; /* Length of file in ms */
    gchar *cdnostring; /* Position of disc in the album */
    gchar *cdno_total; /* The number of discs in the album (ex: 1/2) */
    gchar *compilation; /* The track is a member of a compilation */
    gchar *podcasturl; /* The following are mainly used for podcasts */
    gchar *sort_artist;
    gchar *sort_title;
    gchar *sort_album;
    gchar *sort_albumartist;
    gchar *sort_composer;
    gchar *description;
    gchar *podcastrss;
    gchar *time_released;
    gchar *subtitle;
    gchar *BPM; /* beats per minute */
    gchar *lyrics; /* does not appear to be the full lyrics --
     only used to set the flag 'lyrics_flag'
     of the Track structure */
    gchar *albumartist; /* Album artist  */
};

struct _GainData {
    guint32 peak_signal; /* LAME Peak Signal * 0x800000             */
    gdouble radio_gain; /* RadioGain in dB
     (as defined by www.replaygain.org)      */
    gdouble audiophile_gain;/* AudiophileGain in dB
     (as defined by www.replaygain.org)      */
    gboolean peak_signal_set; /* has the peak signal been set?      */
    gboolean radio_gain_set; /* has the radio gain been set?       */
    gboolean audiophile_gain_set;/* has the audiophile gain been set?  */
};

struct _GaplessData {
    guint32 pregap; /* number of pregap samples */
    guint64 samplecount; /* number of actual music samples */
    guint32 postgap; /* number of postgap samples */
    guint32 gapless_data; /* number of bytes from the first sync frame to the 8th to last frame */
};

#define LAME_TAG_SIZE 0x24
#define INFO_TAG_CRC_SIZE 0xBE	/* number of bytes to pass to crc_compute */

/* A structure to hold the various data found in a LAME info tag.
 * Please see http://gabriel.mp3-tech.org/mp3infotag.html for full
 * documentation.
 */
struct _LameTag {
    gchar encoder[4];
    gchar version_string[5];
    guint8 info_tag_revision;
    guint8 vbr_method;
    guint8 lowpass;
    float peak_signal_amplitude;
    guchar radio_replay_gain[2];
    guchar audiophile_replay_gain[2];
    guint8 encoding_flags;
    guint8 ath_type;
    guint8 bitrate;
    guint16 delay;
    guint16 padding;
    guint8 noise_shaping;
    guint8 stereo_mode;
    gboolean unwise_settings;
    guint8 source_sample_frequency;
    guint8 mp3_gain;
    guint8 surround_info;
    guint16 preset;
    guint32 music_length;
    guint16 music_crc;
    guint16 info_tag_crc;
    guint16 calculated_info_tag_crc;
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
#include "libgtkpod/charset.h"
#include "libgtkpod/itdb.h"
#include "libgtkpod/file.h"
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"

/* MIN_CONSEC_GOOD_FRAMES defines how many consecutive valid MP3 frames
 we need to see before we decide we are looking at a real MP3 file */
#define MIN_CONSEC_GOOD_FRAMES 4
#define FRAME_HEADER_SIZE 4
#define MIN_FRAME_SIZE 21

enum VBR_REPORT {
    VBR_VARIABLE, VBR_AVERAGE, VBR_MEDIAN
};

typedef struct {
    gulong sync;
    guint version;
    guint layer;
    guint crc;
    guint bitrate;
    guint freq;
    guint padding;
    guint extension;
    guint mode;
    guint mode_extension;
    guint copyright;
    guint original;
    guint emphasis;
} MP3Header;

typedef struct {
    const gchar *filename;
    FILE *file;
    off_t datasize;
    gint header_isvalid;
    MP3Header header;
    gint id3_isvalid;
    gint vbr;
    float vbr_average;
    gint milliseconds;
    gint frames;
    gint badframes;
} MP3Info;

/* This is for xmms code */
static guint get_track_time(const gchar *path);

/* This is for soundcheck code */
gboolean mp3_read_lame_tag(const gchar *path, LameTag *lt);

/* ------------------------------------------------------------

 start of first section

 ------------------------------------------------------------ */
void get_mp3_info(MP3Info *mp3);

gint frequencies[3][4] =
    {
        { 22050, 24000, 16000, 50000 }, /* MPEG 2.0 */
        { 44100, 48000, 32000, 50000 }, /* MPEG 1.0 */
        { 11025, 12000, 8000, 50000 } /* MPEG 2.5 */
    };

/* "0" added by JCS */
gint bitrate[2][3][16] =
    {
        { /* MPEG 2.0 */
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },/* layer 1 */
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, /* layer 2 */
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 } /* layer 3 */
        },

        { /* MPEG 1.0 */
            { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },/* layer 1 */
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 }, /* layer 2 */
            { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 } /* layer 3 */
        } };

gint frame_size_index[] =
    { 24000, 72000, 72000 };

gchar *mode_text[] =
    { "stereo", "joint stereo", "dual channel", "mono" };

gchar *emphasis_text[] =
    { "none", "50/15 microsecs", "reserved", "CCITT J 17" };

static gint mp3file_header_bitrate(MP3Header *h) {
    return bitrate[h->version & 1][3 - h->layer][h->bitrate];
}

static gint mp3file_header_frequency(MP3Header *h) {
    return frequencies[h->version][h->freq];
}

gint frame_length(MP3Header *header) {
    return header->sync == 0xFFE ? (frame_size_index[3 - header->layer] * ((header->version & 1) + 1)
            * mp3file_header_bitrate(header) / (float) mp3file_header_frequency(header)) + header->padding : 1;
}

/* Get next MP3 frame header.
 Return codes:
 positive value = Frame Length of this header
 0 = No, we did not retrieve a valid frame header
 */
gint get_header(FILE *file, MP3Header *header) {
    guchar buffer[FRAME_HEADER_SIZE];
    gint fl;

    if (fread(&buffer, FRAME_HEADER_SIZE, 1, file) < 1) {
        header->sync = 0;
        return 0;
    }
    header->sync = (((gint) buffer[0] << 4) | ((gint) (buffer[1] & 0xE0) >> 4));
    if (buffer[1] & 0x10)
        header->version = (buffer[1] >> 3) & 1;
    else
        header->version = 2;
    header->layer = (buffer[1] >> 1) & 3;
    if (header->layer == 0) {
        header->layer = 1; /* sanity added by JCS */
    }
    if ((header->sync != 0xFFE) || (header->layer != 1)) {
        header->sync = 0;
        return 0;
    }
    header->crc = buffer[1] & 1;
    header->bitrate = (buffer[2] >> 4) & 0x0F;
    header->freq = (buffer[2] >> 2) & 0x3;
    header->padding = (buffer[2] >> 1) & 0x1;
    header->extension = (buffer[2]) & 0x1;
    header->mode = (buffer[3] >> 6) & 0x3;
    header->mode_extension = (buffer[3] >> 4) & 0x3;
    header->copyright = (buffer[3] >> 3) & 0x1;
    header->original = (buffer[3] >> 2) & 0x1;
    header->emphasis = (buffer[3]) & 0x3;

    return ((fl = frame_length(header)) >= MIN_FRAME_SIZE ? fl : 0);
}

gint sameConstant(MP3Header *h1, MP3Header *h2) {
    if ((*(guint*) h1) == (*(guint*) h2))
        return 1;

    if ((h1->version == h2->version) && (h1->layer == h2->layer) && (h1->crc == h2->crc) && (h1->freq == h2->freq)
            && (h1->mode == h2->mode) && (h1->copyright == h2->copyright) && (h1->original == h2->original)
            && (h1->emphasis == h2->emphasis))
        return 1;
    else
        return 0;
}

gint get_first_header(MP3Info *mp3, long startpos) {
    gint k, l = 0, c;
    MP3Header h, h2;
    long valid_start = 0;

    fseek(mp3->file, startpos, SEEK_SET);
    while (1) {
        while ((c = fgetc(mp3->file)) != 255 && (c != EOF))
            ;
        if (c == 255) {
            ungetc(c, mp3->file);
            valid_start = ftell(mp3->file);
            if ((l = get_header(mp3->file, &h))) {
                fseek(mp3->file, l - FRAME_HEADER_SIZE, SEEK_CUR);
                for (k = 1; (k < MIN_CONSEC_GOOD_FRAMES) && (mp3->datasize - ftell(mp3->file) >= FRAME_HEADER_SIZE); k++) {
                    if (!(l = get_header(mp3->file, &h2)))
                        break;
                    if (!sameConstant(&h, &h2))
                        break;
                    fseek(mp3->file, l - FRAME_HEADER_SIZE, SEEK_CUR);
                }
                if (k == MIN_CONSEC_GOOD_FRAMES) {
                    fseek(mp3->file, valid_start, SEEK_SET);
                    memcpy(&(mp3->header), &h2, sizeof(MP3Header));
                    mp3->header_isvalid = 1;
                    return 1;
                }
            }
        }
        else {
            return 0;
        }
    }

    return 0;
}

/* get_next_header() - read header at current position or look for
 the next valid header if there isn't one at the current position
 */
gint get_next_header(MP3Info *mp3) {
    gint l = 0, c, skip_bytes = 0;
    MP3Header h;

    while (1) {
        while ((c = fgetc(mp3->file)) != 255 && (ftell(mp3->file) < mp3->datasize))
            skip_bytes++;
        if (c == 255) {
            ungetc(c, mp3->file);
            if ((l = get_header(mp3->file, &h))) {
                if (skip_bytes)
                    mp3->badframes++;
                fseek(mp3->file, l - FRAME_HEADER_SIZE, SEEK_CUR);
                return 15 - h.bitrate;
            }
            else {
                skip_bytes += FRAME_HEADER_SIZE;
            }
        }
        else {
            if (skip_bytes)
                mp3->badframes++;
            return 0;
        }
    }
}

void get_mp3_info(MP3Info *mp3) {
    gint frame_type[15] =
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    double milliseconds = 0, total_rate = 0;
    gint frames = 0, frame_types = 0, frames_so_far = 0;
    gint vbr_median = -1;
    guint bitrate;
    gint counter = 0;
    MP3Header header;
    struct stat filestat;
    off_t data_start = 0;

    stat(mp3->filename, &filestat);
    mp3->datasize = filestat.st_size;

    if (get_first_header(mp3, 0L)) {
        data_start = ftell(mp3->file);
        while ((bitrate = get_next_header(mp3))) {
            if (bitrate < 15) /* sanity added by JCS */
                frame_type[15 - bitrate]++;
            frames++;
        }
        memcpy(&header, &(mp3->header), sizeof(MP3Header));
        for (counter = 0; counter < 15; counter++) {
            if (frame_type[counter]) {
                float header_bitrate; /* introduced by JCS to speed up */
                frame_types++;
                header.bitrate = counter;
                frames_so_far += frame_type[counter];
                header_bitrate = mp3file_header_bitrate(&header);
                if (header_bitrate != 0)
                    milliseconds += 8 * (double) frame_length(&header) * (double) frame_type[counter] / header_bitrate;
                total_rate += header_bitrate * frame_type[counter];
                if ((vbr_median == -1) && (frames_so_far >= frames / 2))
                    vbr_median = counter;
            }
        }
        mp3->milliseconds = (gint) (milliseconds + 0.5);
        mp3->header.bitrate = vbr_median;
        mp3->vbr_average = total_rate / (float) frames;
        mp3->frames = frames;
        if (frame_types > 1) {
            mp3->vbr = 1;
        }
    }
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

struct bitstream_info {
    int bitindex;
    unsigned char *wordpointer;
};

struct bitstream_info bsi;

real mpg123_muls[27][64]; /* also used by layer 1 */

int tabsel_123[2][3][16] =
    {
        {
            { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, },
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, },
            { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, } },

        {
            { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, },
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, },
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, } } };

long mpg123_freqs[9] =
    { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

/*
 * structure to receive extracted header
 */
typedef struct {
    int frames; /* total bit stream frames from Xing header data */
    int bytes; /* total bit stream bytes from Xing header data */
    unsigned char toc[100]; /* "table of contents" */
} xing_header_t;

struct al_table {
    short bits;
    short d;
};

struct frame {
    struct al_table *alloc;
    int (*synth)(real *, int, unsigned char *, int *);
    int (*synth_mono)(real *, unsigned char *, int *);
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
    int (*do_layer)(struct frame * fr);
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
    int framesize; /* computed framesize */
};

static guint32 convert_to_header(guint8 * buf) {

    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static int mpg123_head_check(unsigned long head) {
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
static int mpg123_decode_header(struct frame *fr, unsigned long newhead) {
    int ssize;

    if (newhead & (1 << 20)) {
        fr->lsf = (newhead & (1 << 19)) ? 0x0 : 0x1;
        fr->mpeg25 = 0;
    }
    else {
        fr->lsf = 1;
        fr->mpeg25 = 1;
    }
    fr->lay = 4 - ((newhead >> 17) & 3);
    if (fr->mpeg25) {
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

    switch (fr->lay) {
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
    if (fr->framesize > MAXFRAMESIZE)
        return 0;
    return 1;
}

#define GET_INT32BE(b) \
(i = (b[0] << 24) | (b[1] << 16) | b[2] << 8 | b[3], b += 4, i)

static int mpg123_get_xing_header(xing_header_t * xing, unsigned char *buf) {
    int i, head_flags;
    int id, mode;

    memset(xing, 0, sizeof(xing_header_t));

    /* get selected MPEG header data */
    id = (buf[1] >> 3) & 1;
    mode = (buf[3] >> 6) & 3;
    buf += 4;

    /* Skip the sub band data */
    if (id) {
        /* mpeg1 */
        if (mode != 3)
            buf += 32;
        else
            buf += 17;
    }
    else {
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

    if (head_flags & TOC_FLAG) {
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

static double mpg123_compute_tpf(struct frame *fr) {
    const int bs[4] =
        { 0, 384, 1152, 1152 };
    double tpf;

    tpf = bs[fr->lay];
    tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
    return tpf;
}

static double mpg123_compute_bpf(struct frame *fr) {
    double bpf;

    switch (fr->lay) {
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

unsigned int mpg123_getbits(int number_of_bits) {
    unsigned long rval;

#ifdef DEBUG_GETBITS
    fprintf(stderr, "g%d", number_of_bits);
#endif

    if (!number_of_bits)
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

        rval >>= (24 - number_of_bits);

        bsi.wordpointer += (bsi.bitindex >> 3);
        bsi.bitindex &= 7;
    }

#ifdef DEBUG_GETBITS
    fprintf(stderr,":%x ",rval);
#endif

    return rval;
}

void I_step_one(unsigned int balloc[], unsigned int scale_index[2][SBLIMIT], struct frame *fr) {
    unsigned int *ba = balloc;
    unsigned int *sca = (unsigned int *) scale_index;

    if (fr->stereo) {
        int i;
        int jsbound = fr->jsbound;

        for (i = 0; i < jsbound; i++) {
            *ba++ = mpg123_getbits(4);
            *ba++ = mpg123_getbits(4);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            *ba++ = mpg123_getbits(4);

        ba = balloc;

        for (i = 0; i < jsbound; i++) {
            if ((*ba++))
                *sca++ = mpg123_getbits(6);
            if ((*ba++))
                *sca++ = mpg123_getbits(6);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            if ((*ba++)) {
                *sca++ = mpg123_getbits(6);
                *sca++ = mpg123_getbits(6);
            }
    }
    else {
        int i;

        for (i = 0; i < SBLIMIT; i++)
            *ba++ = mpg123_getbits(4);
        ba = balloc;
        for (i = 0; i < SBLIMIT; i++)
            if ((*ba++))
                *sca++ = mpg123_getbits(6);
    }
}

void I_step_two(real fraction[2][SBLIMIT], unsigned int balloc[2 * SBLIMIT], unsigned int scale_index[2][SBLIMIT], struct frame *fr) {
    int i, n;
    int smpb[2 * SBLIMIT]; /* values: 0-65535 */
    int *sample;
    register unsigned int *ba;
    register unsigned int *sca = (unsigned int *) scale_index;

    if (fr->stereo) {
        int jsbound = fr->jsbound;
        register real *f0 = fraction[0];
        register real *f1 = fraction[1];

        ba = balloc;
        for (sample = smpb, i = 0; i < jsbound; i++) {
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);

        ba = balloc;
        for (sample = smpb, i = 0; i < jsbound; i++) {
            if ((n = *ba++))
                *f0++ = (real) (((-1) << n) + (*sample++) + 1) * mpg123_muls[n + 1][*sca++];
            else
                *f0++ = 0.0;
            if ((n = *ba++))
                *f1++ = (real) (((-1) << n) + (*sample++) + 1) * mpg123_muls[n + 1][*sca++];
            else
                *f1++ = 0.0;
        }
        for (i = jsbound; i < SBLIMIT; i++) {
            if ((n = *ba++)) {
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
    else {
        register real *f0 = fraction[0];

        ba = balloc;
        for (sample = smpb, i = 0; i < SBLIMIT; i++)
            if ((n = *ba++))
                *sample++ = mpg123_getbits(n + 1);
        ba = balloc;
        for (sample = smpb, i = 0; i < SBLIMIT; i++) {
            if ((n = *ba++))
                *f0++ = (real) (((-1) << n) + (*sample++) + 1) * mpg123_muls[n + 1][*sca++];
            else
                *f0++ = 0.0;
        }
        for (i = fr->down_sample_sblimit; i < 32; i++)
            fraction[0][i] = 0.0;
    }
}

static guint get_track_time_file(FILE * file) {
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
    while (!mpg123_head_check(head)) {
        head <<= 8;
        if (fread(tmp, 1, 1, file) != 1)
            return 0;
        head |= tmp[0];
    }
    if (mpg123_decode_header(&frm, head)) {
        buf = g_malloc(frm.framesize + 4);
        fseek(file, -4, SEEK_CUR);
        fread(buf, 1, frm.framesize + 4, file);
        tpf = mpg123_compute_tpf(&frm);
        if (mpg123_get_xing_header(&xing_header, buf)) {
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
        return ((guint) ((guint) (len / bpf) * tpf * 1000));
    }
    return 0;
}

static guint get_track_time(const gchar *path) {
    guint result = 0;

    if (path) {
        FILE *file = fopen(path, "r");
        result = get_track_time_file(file);
        if (file)
            fclose(file);
    }
    return result;
}

/* libid3tag stuff */

#include <id3tag.h>

#ifndef ID3_FRAME_GROUP
#define ID3_FRAME_GROUP "TPE2"
#endif

static const gchar* id3_get_binary(struct id3_tag *tag, char *frame_name, id3_length_t *len, int index) {
    const id3_byte_t *binary = NULL;
    struct id3_frame *frame;
    union id3_field *field;

    g_return_val_if_fail (len, NULL);

    *len = 0;

    frame = id3_tag_findframe(tag, frame_name, index);
    DEBUG ("frame: %p\n", frame);

    if (!frame)
        return NULL;

#if LOCALDEBUG
    printf (" nfields: %d\n", frame->nfields);
    if (strncmp (frame_name, "APIC", 4) == 0)
    {
        field = id3_frame_field (frame, 2);
        printf (" picture type: %ld\n", field->number.value);
    }
#endif

#if 0
    /*-----------------*/
    /* just to show that this field (before last) contains the d8 ff e0 ff
     part of the start of a jpeg file when the coverart war embedded by iTunes */

    const id3_ucs4_t *string = NULL;
    gchar *raw = NULL;

    /* The last field contains the data */
    field = id3_frame_field (frame, frame->nfields-2);

#if LOCALDEBUG
    printf (" field: %p\n", field);
#endif

    if (!field) return NULL;

#if LOCALDEBUG
    printf (" type: %d\n", field->type);
#endif

    switch (field->type)
    {
        case ID3_FIELD_TYPE_STRING:
        string = id3_field_getstring (field);
        break;
        default:
        break;
    }

    /* ISO_8859_1 is just a "marker" -- most people just drop
     whatever coding system they are using into it, so we use
     charset_to_utf8() to convert to utf8 */

    if (string)
    {
        raw = id3_ucs4_latin1duplicate (string);
    }

#if LOCALDEBUG
    {
        FILE *file;
        printf (" string len: %d\n", raw?strlen(raw):0);
        file = fopen ("/tmp/folder1.jpg", "w");
        fwrite (raw, 1, raw?strlen(raw):0, file);
        fclose (file);
    }
#endif
    g_free (raw);

    /*-----------------*/
#endif

    /* The last field contains the data */
    field = id3_frame_field(frame, frame->nfields - 1);

    DEBUG (" field: %p\n", field);

    if (!field)
        return NULL;

    DEBUG (" type: %d\n", field->type);

    switch (field->type) {
    case ID3_FIELD_TYPE_BINARYDATA:
        binary = id3_field_getbinarydata(field, len);
        break;
    default:
        break;
    }

#if LOCALDEBUG
    {
        FILE *file;
        printf (" binary len: %ld\n", *len);
        file = fopen ("/tmp/folder2.jpg", "w");
        fwrite (binary, 1, *len, file);
        fclose (file);
    }
#endif

    return binary;
}

static gchar* id3_get_string(struct id3_tag *tag, char *frame_name) {
    const id3_ucs4_t *string = NULL;
    const id3_byte_t *binary = NULL;
    id3_length_t len = 0;
    struct id3_frame *frame;
    union id3_field *field;
    gchar *utf8 = NULL;
    enum id3_field_textencoding encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

    frame = id3_tag_findframe(tag, frame_name, 0);
    DEBUG ("frame: %p\n", frame);

    if (!frame)
        return NULL;

    /* Find the encoding used for the field */
    field = id3_frame_field(frame, 0);
    DEBUG ("field: %p\n", field); DEBUG ("type: %d\n", id3_field_type (field));

    if (field && (id3_field_type(field) == ID3_FIELD_TYPE_TEXTENCODING)) {
        encoding = field->number.value;
        DEBUG ("encoding: %d\n", encoding);
    }

    /* The last field contains the data */
    field = id3_frame_field(frame, frame->nfields - 1);

    DEBUG ("field: %p\n", field);

    if (!field)
        return NULL;

    DEBUG ("type: %d\n", field->type);

    switch (field->type) {
    case ID3_FIELD_TYPE_STRINGLIST:
        string = id3_field_getstrings(field, 0);
        break;
    case ID3_FIELD_TYPE_STRINGFULL:
        string = id3_field_getfullstring(field);
        break;
    case ID3_FIELD_TYPE_BINARYDATA:
        binary = id3_field_getbinarydata(field, &len);
        DEBUG ("len: %ld\nbinary: %s\n", len, binary+1);
        if (len > 0)
            return charset_to_utf8(binary + 1);
        break;
    default:
        break;
    }

    /*     printf ("string: %p\n", string); */

    if (!string)
        return NULL;

    if (strcmp(frame_name, ID3_FRAME_GENRE) == 0)
        string = id3_genre_name(string);

    if (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1) {
        /* ISO_8859_1 is just a "marker" -- most people just drop
         whatever coding system they are using into it, so we use
         charset_to_utf8() to convert to utf8 */
        id3_latin1_t *raw = id3_ucs4_latin1duplicate(string);
        utf8 = charset_to_utf8(raw);
        g_free(raw);
    }
    else {
        /* Standard unicode is being used -- we won't have to worry
         about charsets then. */
        utf8 = id3_ucs4_utf8duplicate(string);
    }
    return utf8;
}

static void id3_set_string(struct id3_tag *tag, const char *frame_name, const char *data, enum id3_field_textencoding encoding) {
    int res;
    struct id3_frame *frame;
    union id3_field *field;
    id3_ucs4_t *ucs4;

    /* clear the frame, because of bug in libid3tag see
     http://www.mars.org/mailman/public/mad-dev/2004-October/001113.html
     */
    while ((frame = id3_tag_findframe(tag, frame_name, 0))) {
        id3_tag_detachframe(tag, frame);
        id3_frame_delete(frame);
    }

    if ((data == NULL) || (strlen(data) == 0))
        return;

    frame = id3_frame_new(frame_name);
    id3_tag_attachframe(tag, frame);

    /* Use the specified text encoding */
    field = id3_frame_field(frame, 0);
    id3_field_settextencoding(field, encoding);

    if ((strcmp(frame_name, ID3_FRAME_COMMENT) == 0) || (strcmp(frame_name, "USLT") == 0)) {
        field = id3_frame_field(frame, 3);
        field->type = ID3_FIELD_TYPE_STRINGFULL;
    }
    else {
        field = id3_frame_field(frame, 1);
        field->type = ID3_FIELD_TYPE_STRINGLIST;
    }

    /* maybe could be optimized see
     http://www.mars.org/mailman/public/mad-dev/2002-October/000739.html
     */
    /* don't handle the genre frame differently any more */
#define USE_GENRE_IDS 0
#if USE_GENRE_IDS
    if (strcmp (frame_name, ID3_FRAME_GENRE) == 0)
    {
        id3_ucs4_t *tmp_ucs4 = id3_utf8_ucs4duplicate ((id3_utf8_t *)data);
        int index = id3_genre_number (tmp_ucs4);
        if (index != -1)
        {
            /* valid genre -- simply store the genre number */
            gchar *tmp = g_strdup_printf("(%d)", index);
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
#endif
    if (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1) {
        /* we read 'ISO_8859_1' to stand for 'any locale charset'
         -- most programs seem to work that way */
        id3_latin1_t *raw = charset_from_utf8(data);
        ucs4 = id3_latin1_ucs4duplicate(raw);
        g_free(raw);
    }
    else {
        /* Yeah! We use unicode encoding and won't have to
         worry about charsets */
        ucs4 = id3_utf8_ucs4duplicate((id3_utf8_t *) data);
    }
#if USE_GENRE_IDS
}
#endif

    if ((strcmp(frame_name, ID3_FRAME_COMMENT) == 0) || (strcmp(frame_name, "USLT") == 0))
        res = id3_field_setfullstring(field, ucs4);
    else
        res = id3_field_setstrings(field, 1, &ucs4);

    g_free(ucs4);

    if (res != 0)
        g_print(_("Error setting ID3 field: %s\n"), frame_name);
}

/***
 * Reads id3v1.x / id3v2 apic data
 * @returns: TRUE on success, else FALSE.
 */
static gboolean id3_apic_read(const gchar *filename, guchar **image_data, gsize *image_data_len) {
    struct id3_file *id3file;
    struct id3_tag *id3tag;

    g_return_val_if_fail (filename, FALSE);
    g_return_val_if_fail (image_data, FALSE);
    g_return_val_if_fail (image_data_len, FALSE);

    *image_data = NULL;
    *image_data_len = 0;

    if (!(id3file = id3_file_open(filename, ID3_FILE_MODE_READONLY))) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file))) {
        id3_length_t len;
        const guchar *coverart = NULL;
        int i;
        struct id3_frame *frame;

        /* Loop through APIC tags and set coverart.  The picture type should be
         * 3 -- Cover (front), but iTunes has been known to use 0 -- Other. */
        for (i = 0; (frame = id3_tag_findframe(id3tag, "APIC", i)) != NULL; i++) {
            union id3_field *field = id3_frame_field(frame, 2);
            int pictype = field->number.value;
            /*	    printf ("%s: found apic type %d\n", filename, pictype);*/

            /* We'll prefer type 3 (cover) over type 0 (other) */
            if (pictype == 3) {
                coverart = id3_get_binary(id3tag, "APIC", &len, i);
                break;
            }
            if ((pictype == 0) && !coverart) {
                coverart = id3_get_binary(id3tag, "APIC", &len, i);
            }
        }

        if (coverart) { /* I guess iTunes is doing something wrong -- the
         * beginning of the coverart data ends up in a different
         field... We'll just add the missing data manually. */
            const guchar itunes_broken_jfif_marker[] =
                { 0x10, 'J', 'F', 'I', 'F' };
            if (len >= 5) {
                if (strncmp(itunes_broken_jfif_marker, coverart, 5) == 0) {
                    const guchar itunes_missing_header[] =
                        { 0xff, 0xd8, 0xff, 0xe0, 0x00 };
                    *image_data = g_malloc(len + 5);
                    memcpy(*image_data, itunes_missing_header, 5);
                    memcpy((*image_data) + 5, coverart, len);
                    *image_data_len = len + 5;
                }
            }
            if (!*image_data) {
                *image_data = g_malloc(len);
                memcpy(*image_data, coverart, len);
                *image_data_len = len;
            }
#if LOCALDEBUG
            if (*image_data)
            {
                FILE *file;
                file = fopen ("/tmp/folder.jpg", "w");
                fwrite (*image_data, 1, *image_data_len, file);
                fclose (file);
            }
#endif
        }
    }
    id3_file_close(id3file);
    return TRUE;
}

/* Do some checks on the genre string -- ideally this should
 * be done within the id3tag library, I think */
static void handle_genre_variations(gchar **genrep) {
    /* http://www.id3.org/id3v2.3.0#head-42b02d20fb8bf48e38ec5415e34909945dd849dc */

    /* The 'Content type', which previously was stored as a one byte
     * numeric value only, is now a numeric string. You may use one or
     * several of the types as ID3v1.1 did or, since the category list
     * would be impossible to maintain with accurate and up to date
     * categories, define your own.

     * References to the ID3v1 genres can be made by, as first byte, enter
     * "(" followed by a number from the genres list (appendix A) and
     * ended with a ")" character. This is optionally followed by a
     * refinement, e.g. "(21)" or "(4)Eurodisco". Several references can
     * be made in the same frame, e.g. "(51)(39)". If the refinement
     * should begin with a "(" character it should be replaced with "((",
     * e.g. "((I can figure out any genre)" or "(55)((I think...)". The
     * following new content types is defined in ID3v2 and is implemented
     * in the same way as the numeric content types, e.g. "(RX)".
     */

    /* If a "refinement" exists, we will forget about the ID's. */
    /* If only an ID is given, we will translate that ID into a string */
    gchar *genre, *oldgenre, *utf8_genre = NULL;
    const gchar *newgenre = NULL;
    g_return_if_fail (genrep);

    genre = *genrep;
    oldgenre = *genrep;
    if (genre == NULL)
        return;

    while (*genre) {
        if (genre[0] == '(') {
            if (genre[1] == '(') {
                /* starting with "((" */
                newgenre = &genre[1];
                break;
            }
            if (isdigit (genre[1])) { /* possibly a genre ID */
                int num, genreid;
                num = sscanf(genre, "(%d)", &genreid);
                if (num != 1) { /* invalid ID -> give up */
                    newgenre = &genre[0];
                    break;
                }
                genre = strchr(&genre[1], ')');
                g_return_if_fail (genre);
                ++genre;
                if (!newgenre) { /* retrieve genre string from ID -- we only
                 * convert the first ID */
                    id3_ucs4_t const *ucs4_genre = id3_genre_index(genreid);
                    if (ucs4_genre == NULL) {
                        break;
                    }
                    utf8_genre = id3_ucs4_utf8duplicate(ucs4_genre);
                    newgenre = utf8_genre;
                }
            }
            else {
                newgenre = &genre[0];
                break;
            }
        }
        else {
            newgenre = &genre[0];
            break;
        }
    }
    if (newgenre && (newgenre != oldgenre)) {
        *genrep = g_strdup(newgenre);
        g_free(oldgenre);
    }
    g_free(utf8_genre);
}

/***
 * Reads id3v1.x / id3v2 tag and load data into the Id3tag structure.
 * If a tag entry exists (ex: title), we allocate memory, else value
 * stays to NULL
 * @returns: TRUE on success, else FALSE.
 */
gboolean id3_tag_read(const gchar *filename, File_Tag *tag) {
    struct id3_file *id3file;
    struct id3_tag *id3tag;
    gchar* string;
    gchar* string2;

    g_return_val_if_fail (filename, FALSE);
    g_return_val_if_fail (tag, FALSE);

    memset(tag, 0, sizeof(File_Tag));

    if (!(id3file = id3_file_open(filename, ID3_FILE_MODE_READONLY))) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file))) {
        tag->title = id3_get_string(id3tag, ID3_FRAME_TITLE);
        tag->artist = id3_get_string(id3tag, ID3_FRAME_ARTIST);
        if (!tag->artist || !*tag->artist) {
            g_free(tag->artist);
            tag->artist = id3_get_string(id3tag, ID3_FRAME_GROUP);
        }
        else {
            tag->albumartist = id3_get_string(id3tag, ID3_FRAME_GROUP);
        }
        tag->album = id3_get_string(id3tag, ID3_FRAME_ALBUM);
        tag->year = id3_get_string(id3tag, ID3_FRAME_YEAR);
        tag->composer = id3_get_string(id3tag, "TCOM");
        tag->comment = id3_get_string(id3tag, ID3_FRAME_COMMENT);
        tag->genre = id3_get_string(id3tag, ID3_FRAME_GENRE);
        tag->compilation = id3_get_string(id3tag, "TCMP");
        tag->subtitle = id3_get_string(id3tag, "TIT3");
        tag->lyrics = id3_get_string(id3tag, "USLT");
        tag->podcasturl = id3_get_string(id3tag, "YTID");
        tag->podcastrss = id3_get_string(id3tag, "YWFD");
        tag->description = id3_get_string(id3tag, "YTDS");
        tag->time_released = id3_get_string(id3tag, "YTDR");
        tag->BPM = id3_get_string(id3tag, "TBPM");
        tag->sort_artist = id3_get_string(id3tag, "TSOP");
        tag->sort_album = id3_get_string(id3tag, "TSOA");
        tag->sort_title = id3_get_string(id3tag, "TSOT");
        tag->sort_albumartist = id3_get_string(id3tag, "TSO2");
        tag->sort_composer = id3_get_string(id3tag, "TSOC");

        string = id3_get_string(id3tag, "TLEN");
        if (string) {
            tag->songlen = (guint32) strtoul(string, 0, 10);
            g_free(string);
        }

        string = id3_get_string(id3tag, ID3_FRAME_TRACK);
        if (string) {
            string2 = strchr(string, '/');
            if (string2) {
                tag->track_total = g_strdup_printf("%.2d", atoi(string2 + 1));
                *string2 = '\0';
            }
            tag->trackstring = g_strdup_printf("%.2d", atoi(string));
            g_free(string);
        }

        /* CD/disc number tag handling */
        string = id3_get_string(id3tag, "TPOS");
        if (string) {
            string2 = strchr(string, '/');
            if (string2) {
                tag->cdno_total = g_strdup_printf("%.2d", atoi(string2 + 1));
                *string2 = '\0';
            }
            tag->cdnostring = g_strdup_printf("%.2d", atoi(string));
            g_free(string);
        }

        /* Do some checks on the genre string -- ideally this should
         * be done within the id3tag library, I think */
        handle_genre_variations(&tag->genre);
    }

    id3_file_close(id3file);
    return TRUE;
}

static enum id3_field_textencoding get_encoding_of(struct id3_tag *tag, const char *frame_name) {
    struct id3_frame *frame;
    enum id3_field_textencoding encoding = -1;

    frame = id3_tag_findframe(tag, frame_name, 0);
    if (frame) {
        union id3_field *field = id3_frame_field(frame, 0);
        if (field && (id3_field_type(field) == ID3_FIELD_TYPE_TEXTENCODING))
            encoding = field->number.value;
    }
    return encoding;
}

/* Find out which encoding is being used. If in doubt, return
 * latin1. This code assumes that the same encoding is used in all
 * fields.  */
static enum id3_field_textencoding get_encoding(struct id3_tag *tag) {
    enum id3_field_textencoding enc;

    enc = get_encoding_of(tag, ID3_FRAME_TITLE);
    if (enc != -1)
        return enc;
    enc = get_encoding_of(tag, ID3_FRAME_ARTIST);
    if (enc != -1)
        return enc;
    enc = get_encoding_of(tag, ID3_FRAME_ALBUM);
    if (enc != -1)
        return enc;
    enc = get_encoding_of(tag, "TCOM");
    if (enc != -1)
        return enc;
    enc = get_encoding_of(tag, ID3_FRAME_COMMENT);
    if (enc != -1)
        return enc;
    enc = get_encoding_of(tag, ID3_FRAME_YEAR);
    if (enc != -1)
        return enc;
    return ID3_FIELD_TEXTENCODING_ISO_8859_1;
}

/* I'm not really sure about this: The original TAG identifier was
 "TID", but no matter what I do I end up writing "YTID" */
void set_uncommon_tag(struct id3_tag *id3tag, const gchar *id, const gchar *text, enum id3_field_textencoding encoding) {
#if 0
    struct id3_frame *frame;
    union id3_field *field;

    frame = id3_tag_findframe (id3tag, id, 0);
    frame->flags = 0;
    field = id3_frame_field (frame, 0);
    if (field)
    {
        string1 = g_strdup_printf ("%c%s", '\0',
                track->podcasturl);
        id3_field_setbinarydata (field, string1,
                strlen(track->podcasturl)+1);
        g_free (string1);
    }

#endif
}

/**
 * Write the ID3 tags to the file.
 * @returns: TRUE on success, else FALSE.
 */
gboolean mp3_write_file_info(const gchar *filename, Track *track) {
    struct id3_tag* id3tag;
    struct id3_file* id3file;
    gint error = 0;

    id3file = id3_file_open(filename, ID3_FILE_MODE_READWRITE);
    if (!id3file) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file))) {
        char *string1;
        enum id3_field_textencoding encoding;

        /* use the same coding as before... */
        encoding = get_encoding(id3tag);
        /* ...unless it's ISO_8859_1 and prefs say we should use
         unicode (i.e. ID3v2.4) */
        if (prefs_get_int("id3_write_id3v24") && (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1))
            encoding = ID3_FIELD_TEXTENCODING_UTF_8;

        /* always render id3v1 to prevent dj studio from crashing */
        id3_tag_options(id3tag, ID3_TAG_OPTION_ID3V1, ~0);

        /* turn off frame compression and crc information to let
         itunes read tags see
         http://www.mars.org/mailman/public/mad-dev/2002-October/000742.html
         */
        id3_tag_options(id3tag, ID3_TAG_OPTION_COMPRESSION, 0);
        id3_tag_options(id3tag, ID3_TAG_OPTION_CRC, 0);

        id3_set_string(id3tag, ID3_FRAME_TITLE, track->title, encoding);
        id3_set_string(id3tag, ID3_FRAME_ARTIST, track->artist, encoding);
        id3_set_string(id3tag, ID3_FRAME_GROUP, track->albumartist, encoding);
        id3_set_string(id3tag, ID3_FRAME_ALBUM, track->album, encoding);
        id3_set_string(id3tag, ID3_FRAME_GENRE, track->genre, encoding);
        id3_set_string(id3tag, ID3_FRAME_COMMENT, track->comment, encoding);
        id3_set_string(id3tag, "TIT3", track->subtitle, encoding);
        id3_set_string(id3tag, "TSOP", track->sort_artist, encoding);
        id3_set_string(id3tag, "TSOA", track->sort_album, encoding);
        id3_set_string(id3tag, "TSOT", track->sort_title, encoding);
        id3_set_string(id3tag, "TSO2", track->sort_albumartist, encoding);
        id3_set_string(id3tag, "TSOC", track->sort_composer, encoding);

        set_uncommon_tag(id3tag, "YTID", track->podcasturl, encoding);
        set_uncommon_tag(id3tag, "YTDS", track->description, encoding);
        set_uncommon_tag(id3tag, "YWFD", track->podcastrss, encoding);

        id3_set_string(id3tag, "TCOM", track->composer, encoding);

        string1 = g_strdup_printf("%d", track->year);
        id3_set_string(id3tag, ID3_FRAME_YEAR, string1, encoding);
        g_free(string1);

        string1 = g_strdup_printf("%d", track->BPM);
        id3_set_string(id3tag, "TBPM", string1, encoding);
        g_free(string1);

        if (track->tracks)
            string1 = g_strdup_printf("%d/%d", track->track_nr, track->tracks);
        else
            string1 = g_strdup_printf("%d", track->track_nr);
        id3_set_string(id3tag, ID3_FRAME_TRACK, string1, encoding);
        g_free(string1);

        if (track->cds)
            string1 = g_strdup_printf("%d/%d", track->cd_nr, track->cds);
        else
            string1 = g_strdup_printf("%d", track->cd_nr);
        id3_set_string(id3tag, "TPOS", string1, encoding);
        g_free(string1);

        string1 = g_strdup_printf("%d", track->compilation);
        id3_set_string(id3tag, "TCMP", string1, encoding);
        g_free(string1);
    }

    if (id3_file_update(id3file) != 0) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while writing tag to file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    id3_file_close(id3file);

    if (error)
        return FALSE;
    else
        return TRUE;
}

/*
 * Code used to calculate the CRC-16 of the info tag.  Used to check the
 * validity of the data read from the lame tag.
 * Code taken from the libmad project, licensed under GPL
 */

static
unsigned short const crc_table[256] =
    {
        0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241, 0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1,
        0xc481, 0x0440, 0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40, 0x0a00, 0xcac1, 0xcb81, 0x0b40,
        0xc901, 0x09c0, 0x0880, 0xc841, 0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40, 0x1e00, 0xdec1,
        0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41, 0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
        0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,

        0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240, 0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0,
        0x3480, 0xf441, 0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41, 0xfa01, 0x3ac0, 0x3b80, 0xfb41,
        0x3900, 0xf9c1, 0xf881, 0x3840, 0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41, 0xee01, 0x2ec0,
        0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40, 0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
        0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,

        0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240, 0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0,
        0x6480, 0xa441, 0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41, 0xaa01, 0x6ac0, 0x6b80, 0xab41,
        0x6900, 0xa9c1, 0xa881, 0x6840, 0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41, 0xbe01, 0x7ec0,
        0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40, 0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
        0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,

        0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241, 0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1,
        0x9481, 0x5440, 0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40, 0x5a00, 0x9ac1, 0x9b81, 0x5b40,
        0x9901, 0x59c0, 0x5880, 0x9841, 0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40, 0x4e00, 0x8ec1,
        0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41, 0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
        0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040 };

unsigned short crc_compute(char const *data, unsigned int length, unsigned short init) {
    register unsigned int crc;

    for (crc = init; length >= 8; length -= 8) {
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    }

    switch (length) {
    case 7:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 6:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 5:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 4:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 3:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 2:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 1:
        crc = crc_table[(crc ^ *data++) & 0xff] ^ (crc >> 8);
    case 0:
        break;
    }

    return crc;
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
    if (r)
        return r;

    if (a[4] == b[4])
        return 0;

    /* check for '.': indicates subminor version */
    if (a[4] == '.')
        return 1;
    if (b[4] == '.')
        return -1;

    /* check for alpha or beta versions */
    if (a[4] == ' ')
        return 1;
    if (b[4] == ' ')
        return -1;

    /* check for alpha, beta etc. indicated by a, b... */
    return strncmp(&a[4], &b[4], 1);
}

/* buf[] must be declared unsigned -- otherwise the casts, shifts and
 additions below produce funny results */
static void read_lame_replaygain(unsigned char buf[], GainData *gd, int gain_adjust) {
    char oc, nc;
    gint gain;

    g_return_if_fail (gd);

    /* buf[0] and buf[1] are a bit field:
     3 bits: name  (mask: 0xe0 = 11100000)
     3 bits: originator (mask: 0x1c = 00011100)
     1 bit: negative if set (mask: 0x02 = 00000010)
     9 bits: value
     */

    /* check originator */
    oc = (buf[0] & 0x1c) >> 2;
    if ((oc <= 0) || (oc > 3))
        return;

    /* check name code */
    nc = buf[0] & 0xe0;
    if (!((nc == 0x20) || (nc == 0x40)))
        return;

    gain = ((((guint) buf[0]) & 0x1) << 8) + buf[1];

    /* This would be a value of -0.
     * That value however is illegal by current standards and reserved for
     * future use. */
    if ((!gain) && (buf[0] & 0x02))
        return;

    if (buf[0] & 2)
        gain = -gain;

    gain += gain_adjust;

    switch (nc) {
    case 0x20:
        if (gd->radio_gain_set)
            return;
        gd->radio_gain = (gdouble) gain / 10;
        gd->radio_gain_set = TRUE;
        DEBUG ("radio gain (lame): %f\n", gd->radio_gain);
        break;
    case 0x40:
        if (gd->audiophile_gain_set)
            return;
        gd->audiophile_gain = (gdouble) gain / 10;
        gd->audiophile_gain_set = TRUE;
        DEBUG ("album gain (lame): %f\n", gd->audiophile_gain);
        break;
    }
}

static inline guint32 parse_ape_uint32(char *buf) {
    return (buf[0] & 0xff) | (buf[1] & 0xff) << 8 | (buf[2] & 0xff) << 16 | (buf[3] & 0xff) << 24;
}

static inline guint32 parse_lame_uint32(char *buf) {
    return (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
}

static inline guint16 parse_lame_uint16(char *buf) {
    return (buf[0] & 0xff) << 8 | (buf[1] & 0xff);
}

/*
 * mp3_get_track_lame_replaygain:
 *
 * @path: location of the file
 * @gd: #GainData structure
 *
 * Parse ReplayGain data for a given path from the LAME Tag.
 *
 * FIXME: Are there other encoders writing a LAME Tag using a different magic
 * string?
 */
gboolean mp3_get_track_lame_replaygain(const gchar *path, GainData *gd) {
    unsigned char ubuf[2];
    int gain_adjust = 0;
    LameTag lt;

    g_return_val_if_fail (path, FALSE);

    if (!mp3_read_lame_tag(path, &lt))
        goto rg_fail;

    g_return_val_if_fail (gd, FALSE);

    gd->radio_gain = 0;
    gd->audiophile_gain = 0;
    gd->peak_signal = 0;
    gd->radio_gain_set = FALSE;
    gd->audiophile_gain_set = FALSE;
    gd->peak_signal_set = FALSE;

    /* Replay Gain data is only available since Lame version 3.94b */
    if (lame_vcmp(lt.version_string, "3.94b") < 0) {
        DEBUG ("Old lame version (%s). Not used.\n", lt.version_string);
        goto rg_fail;
    }

    if ((!gd->peak_signal_set) && lt.peak_signal_amplitude) {
        gd->peak_signal = lt.peak_signal_amplitude;
        gd->peak_signal_set = TRUE;
        DEBUG ("peak signal (lame): %f\n",
                (double)gd->peak_signal / 0x800000);
    }

    /*
     * Versions prior to 3.95.1 used a reference volume of 83dB.
     * (As compared to the currently used 89dB.)
     */
    if ((lame_vcmp(lt.version_string, "3.95.") < 0)) {
        gain_adjust = 60;
        DEBUG ("Old lame version (%s). Adjusting gain.\n",
                lt.version_string);
    }

    /* radio gain */
    memcpy(ubuf, &lt.radio_replay_gain, 2);
    read_lame_replaygain(ubuf, gd, gain_adjust);

    /* audiophile gain */
    memcpy(ubuf, &lt.audiophile_replay_gain, 2);
    read_lame_replaygain(ubuf, gd, gain_adjust);

    return TRUE;

    rg_fail: return FALSE;
}

/*
 * mp3_get_track_id3_replaygain:
 *
 * @path: location of the file
 * @gd: #GainData structure
 *
 * Read the specified file and scan for ReplayGain information in
 * common ID3v2 tags.
 *
 */
gboolean mp3_get_track_id3_replaygain(const gchar *path, GainData *gd) {
    int i;
    double d;
    char *ep, *key, *val;
    struct id3_file *id3file;
    struct id3_tag *id3tag;
    struct id3_frame *frame;

    g_return_val_if_fail (path, FALSE);
    g_return_val_if_fail (gd, FALSE);

    gd->radio_gain = 0;
    gd->audiophile_gain = 0;
    gd->peak_signal = 0;
    gd->radio_gain_set = FALSE;
    gd->audiophile_gain_set = FALSE;
    gd->peak_signal_set = FALSE;

    if (!(id3file = id3_file_open(path, ID3_FILE_MODE_READONLY))) {
        gchar *fbuf = charset_to_utf8(path);
        g_print(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    if (!(id3tag = id3_file_tag(id3file))) {
        id3_file_close(id3file);
        return FALSE;
    }

    for (i = 0; (frame = id3_tag_findframe(id3tag, "TXXX", i)); i++) {
        if (gd->radio_gain_set && gd->audiophile_gain_set && gd->peak_signal_set)
            break;

        if (frame->nfields < 3)
            continue;

        key = (char *) id3_ucs4_utf8duplicate(id3_field_getstring(&frame->fields[1]));

        val = (char *) id3_ucs4_utf8duplicate(id3_field_getstring(&frame->fields[2]));

        if (g_ascii_strcasecmp(key, "replaygain_album_gain") == 0) {
            d = g_ascii_strtod(val, &ep);
            if (!g_ascii_strncasecmp(ep, " dB", 3)) {
                gd->audiophile_gain = d;
                gd->audiophile_gain_set = TRUE;
                DEBUG ("album gain (id3): %f\n", gd->audiophile_gain);
            }
        }
        else if (g_ascii_strcasecmp(key, "replaygain_album_peak") == 0) {
            d = g_ascii_strtod(val, NULL);
            d *= 0x800000;
            gd->peak_signal = (guint32) floor(d + 0.5);
            gd->peak_signal_set = TRUE;
            DEBUG ("album peak signal (id3): %f\n",
                    (double)gd->peak_signal / 0x800000);
        }
        else if (g_ascii_strcasecmp(key, "replaygain_track_gain") == 0) {
            d = g_ascii_strtod(val, &ep);
            if (!g_ascii_strncasecmp(ep, " dB", 3)) {
                gd->radio_gain = d;
                gd->radio_gain_set = TRUE;
                DEBUG ("radio gain (id3): %f\n", gd->radio_gain);
            }
        }
        else if (g_ascii_strcasecmp(key, "replaygain_track_peak") == 0) {
            d = g_ascii_strtod(val, NULL);
            d *= 0x800000;
            gd->peak_signal = (guint32) floor(d + 0.5);
            gd->peak_signal_set = TRUE;
            DEBUG ("radio peak signal (id3): %f\n",
                    (double)gd->peak_signal / 0x800000);
        }

        g_free(key);
        g_free(val);
    }

    id3_file_close(id3file);

    if (!gd->radio_gain_set && !gd->audiophile_gain_set && !gd->peak_signal_set)
        return FALSE;
    return TRUE;
}

/*
 * mp3_get_track_ape_replaygain:
 *
 * @path: location of the file
 * @gd: #GainData structure
 *
 * Read the specified file and scan for Ape Tag ReplayGain information.
 *
 */
gboolean mp3_get_track_ape_replaygain(const gchar *path, GainData *gd) {
    /* The Ape Tag is located at the end of the file. Or at least that seems
     * where it can most likely be found. Either it is at the very end or
     * before a trailing ID3v1 Tag. Sometimes a Lyrics3 Tag is placed
     * between the ID3v1 and the Ape Tag.  If you find files that have the
     * Tags located in different positions please report it to
     * gtkpod-devel@sourceforge.net.
     */

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

    g_return_val_if_fail (gd, FALSE);
    g_return_val_if_fail (path, FALSE);

    gd->radio_gain = 0;
    gd->audiophile_gain = 0;
    gd->peak_signal = 0;
    gd->radio_gain_set = FALSE;
    gd->audiophile_gain_set = FALSE;
    gd->peak_signal_set = FALSE;

    file = fopen(path, "r");

    if (!file)
        goto rg_fail;

    /* check for ID3v1 Tag */
    if (fseek(file, -ID3V1_SIZE, SEEK_END) || fread(&buf, 1, 3, file) != 3)
        goto rg_fail;
    if (!strncmp(buf, "TAG", 3))
        offset -= ID3V1_SIZE;

    /* check for Lyrics3 Tag */
    if (fseek(file, -9 + offset, SEEK_END) || fread(&buf, 1, 9, file) != 9)
        goto rg_fail;
    if (!strncmp(buf, "LYRICS200", 9)) {
        if (fseek(file, -LYRICS_FOOTER_SIZE + offset, SEEK_END) || fread(&buf, 1, 9, file) != 9)
            goto rg_fail;
        data_length = buf[0] - '0';
        for (i = 1; i < 6; i++) {
            data_length *= 10;
            data_length += buf[i] - '0';
        }
        if (fseek(file, -LYRICS_FOOTER_SIZE - data_length + offset, SEEK_END) || fread(&buf, 1, 11, file) != 11)
            goto rg_fail;
        if (!strncmp(buf, "LYRICSBEGIN", 11))
            offset -= LYRICS_FOOTER_SIZE + data_length;
    }

    /* check for APE Tag */
    if (fseek(file, -APE_FOOTER_SIZE + offset, SEEK_END) || fread(&buf, 1, 8, file) != 8)
        goto rg_fail;
    if (strncmp(buf, "APETAGEX", 8))
        goto rg_fail;

    /* Check the version of the tag. 1000 and 2000 (v1.0 and 2.0) are the
     * only ones I know about. Make suer things do not break in the future.
     */
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

    /* seek to first entry and read the whole buffer */
    if (fseek(file, -APE_FOOTER_SIZE + offset - data_length, SEEK_END))
        goto rg_fail;
    if (!(dbuf = malloc(data_length)))
        goto rg_fail;
    if (fread(dbuf, 1, data_length, file) != data_length)
        goto rg_fail;

    for (i = 0; i < entries; i++) {
        if (gd->radio_gain_set && gd->peak_signal_set && gd->audiophile_gain_set)
            break;
        pos = pos2 + entry_length;
        if (pos > data_length - 10)
            break;

        entry_length = parse_ape_uint32(&dbuf[pos]);
        pos += 4;
        pos += 4;

        pos2 = pos;
        while (dbuf[pos2] && pos2 < data_length)
            pos2++;
        if (pos2 == data_length)
            break;
        pos2++;

        if (entry_length + 1 > sizeof(buf))
            continue;

        /* album gain */
        if (!gd->audiophile_gain_set && !strcasecmp(&dbuf[pos], "REPLAYGAIN_ALBUM_GAIN")) {
            memcpy(buf, &dbuf[pos2], entry_length);
            buf[entry_length] = '\0';

            d = g_ascii_strtod(buf, &ep);
            if ((ep == buf + entry_length - 3) && (!strncasecmp(ep, " dB", 3))) {
                gd->audiophile_gain = d;
                gd->audiophile_gain_set = TRUE;
                DEBUG ("album gain (ape): %f\n", gd->audiophile_gain);
            }

            continue;
        }
        if (!gd->peak_signal_set && !strcasecmp(&dbuf[pos], "REPLAYGAIN_ALBUM_PEAK")) {
            memcpy(buf, &dbuf[pos2], entry_length);
            buf[entry_length] = '\0';

            d = g_ascii_strtod(buf, &ep);
            if (ep == buf + entry_length) {
                d *= 0x800000;
                gd->peak_signal = (guint32) floor(d + 0.5);
                gd->peak_signal_set = TRUE;
                DEBUG ("album peak signal (ape): %f\n",
                        (double)gd->peak_signal / 0x800000);
            }

            continue;
        }

        /* track gain */
        if (!gd->radio_gain_set && !strcasecmp(&dbuf[pos], "REPLAYGAIN_TRACK_GAIN")) {
            memcpy(buf, &dbuf[pos2], entry_length);
            buf[entry_length] = '\0';

            d = g_ascii_strtod(buf, &ep);
            if ((ep == buf + entry_length - 3) && (!strncasecmp(ep, " dB", 3))) {
                gd->radio_gain = d;
                gd->radio_gain_set = TRUE;
                DEBUG ("radio gain (ape): %f\n", gd->radio_gain);
            }

            continue;
        }
        if (!gd->peak_signal_set && !strcasecmp(&dbuf[pos], "REPLAYGAIN_TRACK_PEAK")) {
            memcpy(buf, &dbuf[pos2], entry_length);
            buf[entry_length] = '\0';

            d = g_ascii_strtod(buf, &ep);
            if (ep == buf + entry_length) {
                d *= 0x800000;
                gd->peak_signal = (guint32) floor(d + 0.5);
                gd->peak_signal_set = TRUE;
                DEBUG ("radio peak signal (ape): %f\n",
                        (double)gd->peak_signal / 0x800000);
            }

            continue;
        }
    }

    free(dbuf);
    fclose(file);
    return TRUE;

    rg_fail: if (dbuf)
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

/*
 * mp3_read_soundcheck:
 *
 * @path: localtion of the file
 * @track: structure holding track information
 *
 * Try to read ReplayGain values and set the track's soundcheck field
 * accordingly.  If an ID3 tag is present and contains replaygain
 * fields (in the format used by Foobar2k and others), the values are
 * read from that tag.  If no ID3 tag is present, an APE tag is
 * checked and used if possible.  Lastly, the LAME tag is checked.  In
 * all cases, audiophile (aka album) gain is preferred over radio (aka
 * track) gain if both gain types are set.
 *
 * The function always rereads the gain from the file.
 *
 * Returns TRUE if the soundcheck field could be set.
 */
gboolean mp3_read_soundcheck(const gchar *path, Track *track) {
    GainData gd;
    gint replaygain_offset;
    gint replaygain_mode_album_priority;

    replaygain_offset = prefs_get_int("replaygain_offset");
    replaygain_mode_album_priority = prefs_get_int("replaygain_mode_album_priority");

    g_return_val_if_fail (track, FALSE);

    memset(&gd, 0, sizeof(GainData));

    gd.radio_gain_set = FALSE;
    gd.audiophile_gain_set = FALSE;
    gd.peak_signal_set = FALSE;

    if (mp3_get_track_id3_replaygain(path, &gd))
        DEBUG ("Using ID3 ReplayGain data\n");
    else if (mp3_get_track_ape_replaygain(path, &gd))
        DEBUG ("Using APE ReplayGain data\n");
    else if (mp3_get_track_lame_replaygain(path, &gd))
        DEBUG ("Using LAME ReplayGain data\n");
    else
        return FALSE;

    if (gd.audiophile_gain_set && replaygain_mode_album_priority) {
        DEBUG ("Setting Soundcheck value from album ReplayGain\n");
        track->soundcheck = replaygain_to_soundcheck(gd.audiophile_gain + replaygain_offset);
        return TRUE;
    }
    if (gd.radio_gain_set) {
        DEBUG ("Setting Soundcheck value from radio ReplayGain\n");
        track->soundcheck = replaygain_to_soundcheck(gd.radio_gain + replaygain_offset);
        return TRUE;
    }

    return FALSE;
}

/* ----------------------------------------------------------------------

 From here starts original gtkpod code

 ---------------------------------------------------------------------- */

/* mpeg audio slot size in bytes */
int slotsize[3] =
    { 4, 1, 1 }; /* layer 1, layer 2, layer 3 */

int samplesperframe[2][3] =
    {
        { /* MPEG 2.0 */
        384, 1152, 576 /* layer 1, layer 2, layer 3 */
        },

        { /* MPEG 1.0 */
        384, 1152, 1152 /* layer 1, layer 2, layer 3 */
        } };

/*
 * mp3_read_lame_tag - read the data from the lame tag (if it exists)
 *
 * @path: location of the file
 * @lt: pointer to structure to be filled
 */
gboolean mp3_read_lame_tag(const gchar *path, LameTag *lt) {
    MP3Info *mp3i = NULL;
    MP3Header h;
    guint32 flags, peak_amplitude;
    gint toskip = 0;
    FILE *file;
    guchar ubuf[LAME_TAG_SIZE];
    gint sideinfo;
    guchar full_info_tag[INFO_TAG_CRC_SIZE];

    g_return_val_if_fail (path, FALSE);

    /* Attempt to open the file */
    file = fopen(path, "r");
    if (!file)
        goto lt_fail;

    mp3i = g_malloc0(sizeof(MP3Info));
    mp3i->filename = path;
    mp3i->file = file;
    get_mp3_info(mp3i);

    /* use get_first_header() to seek to the first mp3 header */
    get_first_header(mp3i, 0);

    if (fread(full_info_tag, 1, INFO_TAG_CRC_SIZE, mp3i->file) != INFO_TAG_CRC_SIZE)
        goto lt_fail;
    fseek(mp3i->file, -INFO_TAG_CRC_SIZE, SEEK_CUR);

    if (!get_header(mp3i->file, &h))
        goto lt_fail;

    /* Determine offset of Xing header based on sideinfo size */
    if (h.version & 0x1) {
        sideinfo = (h.mode & 0x2) ? SIDEINFO_MPEG1_MONO : SIDEINFO_MPEG1_MULTI;
    }
    else {
        sideinfo = (h.mode & 0x2) ? SIDEINFO_MPEG2_MONO : SIDEINFO_MPEG2_MULTI;
    }

    if (fseek(mp3i->file, sideinfo, SEEK_CUR) || (fread(&ubuf[0], 1, 4, mp3i->file) != 4))
        goto lt_fail;

    /* Is this really a Xing or Info Header?
     * FIXME: Apparently (according to madplay sources) there is a different
     * possible location for the Xing header ("due to an unfortunate
     * historical event"). I do not thing we need to care though since
     * ReplayGain information is only contained in recent files.  */
    if (strncmp(ubuf, "Xing", 4) && strncmp(ubuf, "Info", 4))
        goto lt_fail;

    /* Determine the offset of the LAME tag based on contents of the Xing header */
    fread(ubuf, 4, 1, mp3i->file);
    flags = (ubuf[0] << 24) | (ubuf[1] << 16) | (ubuf[2] << 8) | ubuf[3];

    if (flags & FRAMES_FLAG) { /* frames field is set */
        toskip += 4;
    }
    if (flags & BYTES_FLAG) { /* bytes field is set */
        toskip += 4;
    }
    if (flags & TOC_FLAG) { /* TOC field is set */
        toskip += 100;
    }
    if (flags & VBR_SCALE_FLAG) { /* quality field is set */
        toskip += 4;
    }

    /* Check for LAME Tag */
    if (fseek(mp3i->file, toskip, SEEK_CUR) || (fread(ubuf, 1, LAME_TAG_SIZE, mp3i->file) != LAME_TAG_SIZE))
        goto lt_fail;
    if (strncmp(ubuf, "LAME", 4)) {
        goto lt_fail;
    }

    strncpy(lt->encoder, &ubuf[0x0], 4);

    strncpy(lt->version_string, &ubuf[0x4], 5);

    lt->info_tag_revision = (ubuf[0x9] >> 4);
    lt->vbr_method = (ubuf[0x9] & 0xf);
    lt->lowpass = ubuf[0xa];

    /* convert BE float */
    peak_amplitude = (ubuf[0xb] << 24) | (ubuf[0xc] << 16) | (ubuf[0xd] << 8) | ubuf[0xe];
    memcpy(&lt->peak_signal_amplitude, &peak_amplitude, 4);
    memcpy(&lt->radio_replay_gain, &ubuf[0xf], 2);
    memcpy(&lt->audiophile_replay_gain, &ubuf[0x11], 2);

    lt->encoding_flags = ubuf[0x13] >> 4;
    lt->ath_type = ubuf[0x13] & 0xf;

    lt->bitrate = ubuf[0x14];

    lt->delay = (ubuf[0x15] << 4) + (ubuf[0x16] >> 4);
    lt->padding = ((ubuf[0x16] & 0xf) << 8) + ubuf[0x17];

    lt->noise_shaping = ubuf[0x18] & 0x3;
    lt->stereo_mode = (ubuf[0x18] >> 2) & 0x7;
    lt->unwise_settings = (ubuf[0x18] >> 5) & 0x1;
    lt->source_sample_frequency = (ubuf[0x18] >> 6) & 0x3;

    lt->mp3_gain = ubuf[0x19];

    lt->surround_info = (ubuf[0x1a] >> 3) & 0x7;
    lt->preset = ((ubuf[0x1a] & 0x7) << 8) + ubuf[0x1b];

    lt->music_length = parse_lame_uint32(&ubuf[0x1c]);

    lt->music_crc = parse_lame_uint16(&ubuf[0x20]);
    lt->info_tag_crc = parse_lame_uint16(&ubuf[0x22]);

    lt->calculated_info_tag_crc = crc_compute(full_info_tag, INFO_TAG_CRC_SIZE, 0x0000);

    fclose(file);
    g_free(mp3i);
    return (lt->calculated_info_tag_crc == lt->info_tag_crc);

    lt_fail: if (file)
        fclose(file);
    g_free(mp3i);
    return FALSE;

}

/*
 * mp3_get_track_gapless - read the specified file and calculate gapless
 * information: totalsamples and gapless_data
 *
 * @mp3i: MP3Info of the file; should already have run get_mp3_info() on it
 * @gd: structure holding gapless information; should have pregap and
 * 	postgap already filled
 */

gboolean mp3_get_track_gapless(MP3Info *mp3i, GaplessData *gd) {
    int i;
    int xing_header_offset;
    int mysamplesperframe;
    int totaldatasize;
    int lastframes[8];
    int totalframes;
    int finaleight;
    int l;

    g_return_val_if_fail (mp3i, FALSE);
    g_return_val_if_fail (gd, FALSE);

    /* use get_first_header() to seek to the first mp3 header */
    get_first_header(mp3i, 0);

    xing_header_offset = ftell(mp3i->file);

    get_header(mp3i->file, &(mp3i->header));

    mysamplesperframe = samplesperframe[mp3i->header.version & 1][3 - mp3i->header.layer];

    /* jump to the end of the frame with the xing header */
    if (fseek(mp3i->file, xing_header_offset + frame_length(&(mp3i->header)), SEEK_SET))
        goto gp_fail;

    /* counts bytes from the start of the 1st sync frame */
    totaldatasize = frame_length(&(mp3i->header));

    /* lastframes keeps track of the last 8 frame sizes */

    /* counts number of music frames */
    totalframes = 0;

    /* quickly parse the file, reading only frame headers */
    l = 0;
    while ((l = get_header(mp3i->file, &(mp3i->header))) != 0) {
        lastframes[totalframes % 8] = l;
        totaldatasize += l;
        totalframes++;

        if (fseek(mp3i->file, l - FRAME_HEADER_SIZE, SEEK_CUR))
            goto gp_fail;
    }

    finaleight = 0;
    for (i = 0; i < 8; i++) {
        finaleight += lastframes[i];
    }

    /* For some reason, iTunes appears to add an extra frames worth of
     * samples to the samplecount for CBR files.  CBR files don't currently
     * (2 Jul 07) play gaplessly whether uploaded from iTunes or gtkpod,
     * even with apparently correct values, but we will attempt to emulate
     * iTunes' behavior */
    if (mp3i->vbr == 0) // CBR
        totalframes++;

    /* all but last eight frames */
    gd->gapless_data = totaldatasize - finaleight;
    /* total samples minus pre/postgap */
    gd->samplecount = totalframes * mysamplesperframe - gd->pregap - gd->postgap;

    return TRUE;

    gp_fail: return FALSE;

}

/**
 * mp3_read_gapless:
 *
 * try to read the gapless values from the LAME Tag and set
 * the track's pregap, postgap, samplecount, and gapless_data fields
 * accordingly.
 *
 * @path: location of the file
 * @track: structure holding track information
 *
 * The function always rereads the data from the file.
 *
 * Returns TRUE if all four gapless fields could be
 * set. etrack->tchanged is set to TRUE if data has been changed,
 * FALSE otherwise.
 */

gboolean mp3_read_gapless(const gchar *path, Track *track) {
    MP3Info *mp3i = NULL;
    FILE *file;

    GaplessData gd;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, FALSE);

    etr = track->userdata;

    g_return_val_if_fail (etr, FALSE);

    memset(&gd, 0, sizeof(GaplessData));

    g_return_val_if_fail (path, FALSE);

    /* Attempt to open the file */
    file = fopen(path, "r");
    if (file) {
        mp3i = g_malloc0(sizeof(MP3Info));
        mp3i->filename = path;
        mp3i->file = file;
        get_mp3_info(mp3i);

        /* Try the LAME tag for pregap and postgap */
        LameTag lt;
        if (mp3_read_lame_tag(path, &lt)) {
            gd.pregap = lt.delay;
            gd.postgap = lt.padding;
        }
        else {
            /* insert non-LAME methods of finding pregap and postgap */
            fclose(file);
            g_free(mp3i);
            return FALSE;
        }

        mp3_get_track_gapless(mp3i, &gd);

        etr->tchanged = FALSE;

        if ((gd.pregap) && (gd.samplecount) && (gd.postgap) && (gd.gapless_data)) {
            if ((track->pregap != gd.pregap) || (track->samplecount != gd.samplecount)
                    || (track->postgap != gd.postgap) || (track->gapless_data != gd.gapless_data)
                    || (track->gapless_track_flag == FALSE)) {
                etr->tchanged = TRUE;
                track->pregap = gd.pregap;
                track->samplecount = gd.samplecount;
                track->postgap = gd.postgap;
                track->gapless_data = gd.gapless_data;
                track->gapless_track_flag = TRUE;
            }
        }
        else { /* remove gapless data which doesn't seem to be valid any
         * more */
            if (track->gapless_track_flag == TRUE) {
                etr->tchanged = TRUE;
            }
            track->pregap = 0;
            track->samplecount = 0;
            track->postgap = 0;
            track->gapless_data = 0;
            track->gapless_track_flag = FALSE;
        }

        fclose(file);
        g_free(mp3i);
        return TRUE;
    }
    return FALSE;
}

/* Read ID3 tags of filename @name into track structure @track */
/* Return value: TRUE if tags could be read, FALSE if an error
 occured */
gboolean id3_read_tags(const gchar *name, Track *track) {
    File_Tag filetag;

    g_return_val_if_fail (name && track, FALSE);

    if (id3_tag_read(name, &filetag)) {
        guchar *image_data = NULL;
        gsize image_data_len = 0;

        if (filetag.album) {
            track->album = filetag.album;
        }

        if (filetag.artist) {
            track->artist = filetag.artist;
        }

        if (filetag.albumartist) {
            track->albumartist = filetag.albumartist;
        }

        if (filetag.title) {
            track->title = filetag.title;
        }

        if (filetag.genre) {
            track->genre = filetag.genre;
        }

        if (filetag.composer) {
            track->composer = filetag.composer;
        }

        if (filetag.comment) {
            track->comment = filetag.comment;
        }

        if (filetag.podcasturl) {
            track->podcasturl = filetag.podcasturl;
        }

        if (filetag.podcastrss) {
            track->podcastrss = filetag.podcastrss;
        }

        if (filetag.subtitle) {
            track->subtitle = filetag.subtitle;
        }

        if (filetag.description) {
            track->description = filetag.description;
        }

        if (filetag.sort_artist) {
            track->sort_artist = filetag.sort_artist;
        }

        if (filetag.sort_title) {
            track->sort_title = filetag.sort_title;
        }

        if (filetag.sort_album) {
            track->sort_album = filetag.sort_album;
        }

        if (filetag.sort_albumartist) {
            track->sort_albumartist = filetag.sort_albumartist;
        }

        if (filetag.sort_composer) {
            track->sort_composer = filetag.sort_composer;
        }

        if (filetag.year == NULL) {
            track->year = 0;
        }
        else {
            track->year = atoi(filetag.year);
            g_free(filetag.year);
        }

        if (filetag.trackstring == NULL) {
            track->track_nr = 0;
        }
        else {
            track->track_nr = atoi(filetag.trackstring);
            g_free(filetag.trackstring);
        }

        if (filetag.track_total == NULL) {
            track->tracks = 0;
        }
        else {
            track->tracks = atoi(filetag.track_total);
            g_free(filetag.track_total);
        }
        /* CD/disc number handling */
        if (filetag.cdnostring == NULL) {
            track->cd_nr = 0;
        }
        else {
            track->cd_nr = atoi(filetag.cdnostring);
            g_free(filetag.cdnostring);
        }

        if (filetag.cdno_total == NULL) {
            track->cds = 0;
        }
        else {
            track->cds = atoi(filetag.cdno_total);
            g_free(filetag.cdno_total);
        }

        if (filetag.compilation == NULL) {
            track->compilation = 0;
        }
        else {
            track->compilation = atoi(filetag.compilation);
            g_free(filetag.compilation);
        }

        if (filetag.BPM == NULL) {
            track->BPM = 0;
        }
        else {
            track->BPM = atoi(filetag.BPM);
            g_free(filetag.BPM);
        }

        if (filetag.lyrics) {
            track->lyrics_flag = 0x01;
            g_free(filetag.lyrics);
        }
        else {
            track->lyrics_flag = 0x00;
        }

        if (prefs_get_int("coverart_apic") && (id3_apic_read(name, &image_data, &image_data_len) == TRUE)) {
            if (image_data) {
                gp_track_set_thumbnails_from_data(track, image_data, image_data_len);
                g_free(image_data);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* ----------------------------------------------------------------------

 From here starts original gtkpod code

 ---------------------------------------------------------------------- */

/* Return a Track structure with all information read from the mp3
 file filled in */
Track *mp3_get_file_info(const gchar *name) {
    Track *track = NULL;
    MP3Info *mp3i = NULL;
    FILE *file;

    g_return_val_if_fail (name, NULL);

    /* Attempt to open the file */
    file = fopen(name, "r");
    if (file) {
        mp3i = g_malloc0(sizeof(MP3Info));
        mp3i->filename = name;
        mp3i->file = file;
        get_mp3_info(mp3i);
        mp3i->file = NULL;
        fclose(file);
    }
    else {
        gchar *fbuf = charset_to_utf8(name);
        gtkpod_warning(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return NULL;
    }

    track = gp_track_new();
    track->filetype = g_strdup("MPEG audio file");

    if (prefs_get_int("readtags")) {
        id3_read_tags(name, track);
    }

    mp3_read_soundcheck(name, track);

    mp3_read_gapless(name, track);

    /* Get additional info (play time and bitrate */
    if (mp3i) {
        track->tracklen = mp3i->milliseconds;
        track->bitrate = (gint) (mp3i->vbr_average);
        track->samplerate = mp3file_header_frequency(&mp3i->header);
        g_free(mp3i);
    }
    /* Fall back to xmms code if tracklen is 0 */
    if (track->tracklen == 0) {
        track->tracklen = get_track_time(name);
        if (track->tracklen)
            track->bitrate = (float) track->size * 8 / track->tracklen;
    }

    if (track->tracklen == 0) {
        /* Tracks with zero play length are ignored by iPod... */
        gtkpod_warning(_("File \"%s\" has zero play length. Ignoring.\n"), name);
        gp_track_free(track);
        track = NULL;
    }

    /* Set mediatype to audio */
    if (track) {
        track->mediatype = ITDB_MEDIATYPE_AUDIO;
        if (track->genre) {
            if (g_ascii_strcasecmp (track->genre, "audiobook") == 0)
                track->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
            else if (g_ascii_strcasecmp (track->genre, "podcast") == 0)
                track->mediatype = ITDB_MEDIATYPE_PODCAST;
            }
        }

    return track;
}
/*
 *
 * @returns: TRUE on success, else FALSE.
 */
gboolean id3_lyrics_read(const gchar *filename, gchar **lyrics) {
    struct id3_file *id3file;
    struct id3_tag *id3tag;

    g_return_val_if_fail (filename, FALSE);
    g_return_val_if_fail (lyrics, FALSE);

    if (!(id3file = id3_file_open(filename, ID3_FILE_MODE_READONLY))) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file))) {
        *lyrics = id3_get_string(id3tag, "USLT");
    }

    id3_file_close(id3file);
    return TRUE;
}

gboolean id3_lyrics_save(const gchar *filename, const gchar *lyrics) {
    struct id3_file *id3file;
    struct id3_tag *id3tag;

    g_return_val_if_fail (filename, FALSE);

    id3file = id3_file_open(filename, ID3_FILE_MODE_READWRITE);
    if (!id3file) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while opening file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    if ((id3tag = id3_file_tag(id3file))) {
        enum id3_field_textencoding encoding;

        /* actually the iPod only understands UTF8 lyrics, but for
         consistency's sake we'll do the same as for the other tags
         */
        /* use the same coding as before... */
        encoding = get_encoding(id3tag);
        /* ...unless it's ISO_8859_1 and prefs say we should use
         unicode (i.e. ID3v2.4) */
        if (prefs_get_int("id3_write_id3v24") && (encoding == ID3_FIELD_TEXTENCODING_ISO_8859_1))
            encoding = ID3_FIELD_TEXTENCODING_UTF_8;

        /* always render id3v1 to prevent dj studio from crashing */
        id3_tag_options(id3tag, ID3_TAG_OPTION_ID3V1, ~0);

        /* turn off frame compression and crc information to let
         itunes read tags see
         http://www.mars.org/mailman/public/mad-dev/2002-October/000742.html
         */
        id3_tag_options(id3tag, ID3_TAG_OPTION_COMPRESSION, 0);
        id3_tag_options(id3tag, ID3_TAG_OPTION_CRC, 0);

        id3_set_string(id3tag, "USLT", lyrics, encoding);
    }

    if (id3_file_update(id3file) != 0) {
        gchar *fbuf = charset_to_utf8(filename);
        g_print(_("ERROR while writing tag to file: '%s' (%s).\n"), fbuf, g_strerror(errno));
        g_free(fbuf);
        return FALSE;
    }

    id3_file_close(id3file);

    return TRUE;
}

gboolean mp3_can_convert() {
    gchar *cmd = mp3_get_conversion_cmd();
    return cmd && cmd[0] && prefs_get_int("convert_mp3");
}

gchar *mp3_get_conversion_cmd() {
    return prefs_get_string("path_conv_mp3");
}

gchar *mp3_get_gain_cmd() {
    return prefs_get_string ("path_mp3gain");
}


