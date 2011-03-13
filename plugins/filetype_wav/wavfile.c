/* Time-stamp: <2005-09-17 21:55:20 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include "libgtkpod/charset.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "wavfile.h"

/* Code for determining the playlength is strongly based on xmms code
 * (wav.c) which is distributed under the terms of the GPL V2 or any
 * later version. */
/* Original copyright notice (they are not involved with gtkpod,
 * however):
 * Copyright (C) 1998-2000 Peter Alm, Mikael Alm, Olle Hallnas,
 * Thomas Nilsson and 4Front Technologies */

#define	WAVE_FORMAT_UNKNOWN		(0x0000)
#define	WAVE_FORMAT_PCM			(0x0001)
#define	WAVE_FORMAT_ADPCM		(0x0002)
#define	WAVE_FORMAT_ALAW		(0x0006)
#define	WAVE_FORMAT_MULAW		(0x0007)
#define	WAVE_FORMAT_OKI_ADPCM		(0x0010)
#define	WAVE_FORMAT_DIGISTD		(0x0015)
#define	WAVE_FORMAT_DIGIFIX		(0x0016)
#define	IBM_FORMAT_MULAW	(0x0101)
#define	IBM_FORMAT_ALAW			(0x0102)
#define	IBM_FORMAT_ADPCM	(0x0103)

typedef struct {
    FILE *file;
    short format_tag, channels, block_align, bits_per_sample, eof, going;
    long samples_per_sec, avg_bytes_per_sec;
} WaveFile;

static gint read_le_long(FILE * file, glong *ret) {
    guchar buf[4];

    if (fread(buf, 1, 4, file) != 4)
        return 0;

    *ret = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    return TRUE;
}

static gint read_le_short(FILE * file, gshort *ret) {
    guchar buf[2];

    if (fread(buf, 1, 2, file) != 2)
        return 0;

    *ret = (buf[1] << 8) | buf[0];
    return TRUE;
}

Track *wav_get_file_info(const gchar *filename, GError **error) {
    Track *track = NULL;
    gchar *fn;
    gchar magic[4];
    gulong len;
    WaveFile *wav_file;

    wav_file = g_malloc(sizeof(WaveFile));
    memset(wav_file, 0, sizeof(WaveFile));
    if (!(wav_file->file = fopen(filename, "rb"))) {
        gchar *fn = charset_to_utf8(filename);
        filetype_log_error(error, g_strdup_printf(_("Could not open '%s' for reading.\n"), fn));
        g_free(fn);
        g_free(wav_file);
        wav_file = NULL;
        return NULL;
    }

    fread(magic, 1, 4, wav_file->file);
    if (strncmp(magic, "RIFF", 4) != 0)
        goto file_error;
    read_le_long(wav_file->file, &len);
    fread(magic, 1, 4, wav_file->file);
    if (strncmp(magic, "WAVE", 4) != 0)
        goto file_error;
    for (;;) {
        fread(magic, 1, 4, wav_file->file);
        if (!read_le_long(wav_file->file, &len))
            goto file_error;
        if (!strncmp("fmt ", magic, 4))
            break;
        fseek(wav_file->file, len, SEEK_CUR);
    }
    if (len < 16)
        goto file_error;
    read_le_short(wav_file->file, &wav_file->format_tag);
    switch (wav_file->format_tag) {
    case WAVE_FORMAT_UNKNOWN:
    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
    case WAVE_FORMAT_ADPCM:
    case WAVE_FORMAT_OKI_ADPCM:
    case WAVE_FORMAT_DIGISTD:
    case WAVE_FORMAT_DIGIFIX:
    case IBM_FORMAT_MULAW:
    case IBM_FORMAT_ALAW:
    case IBM_FORMAT_ADPCM:
        goto file_error;
    }
    read_le_short(wav_file->file, &wav_file->channels);
    read_le_long(wav_file->file, &wav_file->samples_per_sec);
    read_le_long(wav_file->file, &wav_file->avg_bytes_per_sec);
    read_le_short(wav_file->file, &wav_file->block_align);
    read_le_short(wav_file->file, &wav_file->bits_per_sample);
    /*    if (wav_file->bits_per_sample != 8 && wav_file->bits_per_sample != 16)
     goto file_error;*/
    len -= 16;
    if (len)
        fseek(wav_file->file, len, SEEK_CUR);

    for (;;) {
        fread(magic, 4, 1, wav_file->file);

        if (!read_le_long(wav_file->file, &len))
            goto file_error;
        if (!strncmp("data", magic, 4))
            break;
        fseek(wav_file->file, len, SEEK_CUR);
    }

    track = gp_track_new();
    track->mediatype = ITDB_MEDIATYPE_AUDIO;

    track->bitrate = wav_file->samples_per_sec * wav_file->channels * wav_file->bits_per_sample;
    track->samplerate = wav_file->samples_per_sec;
    track->tracklen = 1000 * ((double) 8 * len / track->bitrate);
    track->bitrate /= 1000; /* change to kbps */
    track->filetype = g_strdup("WAV audio file");

    fclose(wav_file->file);
    g_free(wav_file);
    wav_file = NULL;
    return track;

    file_error: fclose(wav_file->file);
    g_free(wav_file);
    wav_file = NULL;
    fn = charset_to_utf8(filename);
    filetype_log_error(error, g_strdup_printf(_("%s does not appear to be a supported wav file.\n"), fn));
    g_free(fn);
    return NULL;
}

gboolean wav_can_convert() {
    gchar *cmd = wav_get_conversion_cmd();
    return cmd && cmd[0] && prefs_get_int("convert_wav");
}

gchar *wav_get_conversion_cmd() {
    return prefs_get_string("path_conv_wav");
}
