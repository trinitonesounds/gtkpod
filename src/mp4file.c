/* Time-stamp: <2003-11-08 15:30:33 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
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
|
|  $Id$
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "mp4file.h"
#include "song.h"
#include "charset.h"
#include "misc.h"
#include "support.h"

/* ------------------------------------------------------------

   Info on how to implement new file formats.

   You need to supply a function

   Track *file_get_xxx_info (gchar *filename)

   that returns a new Track structure with as many of the following
   fields filled in (in UTF8):

   gchar   *album;            /+ album (utf8)          +/
   gchar   *artist;           /+ artist (utf8)         +/
   gchar   *title;            /+ title (utf8)          +/
   gchar   *genre;            /+ genre (utf8)          +/
   gchar   *comment;          /+ comment (utf8)        +/
   gchar   *composer;         /+ Composer (utf8)       +/
   gchar   *fdesc;            /+ Format description (utf8) +/
   gint32  tracklen;          /+ Length of track in ms +/
   gint32  cd_nr;             /+ CD number             +/
   gint32  cds;               /+ number of CDs         +/
   gint32  track_nr;          /+ track number          +/
   gint32  tracks;            /+ number of tracks      +/
   gint32  year;              /+ year                  +/
   gint32  bitrate;           /+ bitrate               +/
   gchar   *charset;          /+ charset used for tags +/

   Please note that the iPod will only play as much of the track as
   specified in "tracklen".

   You don't have to fill in the value for charset if you use the
   default charset (i.e. you use charset_to_utf8() to convert to
   UTF8). Otherwise please specify the charset used.

   When an error occurs, the function returns NULL and logs an error
   message using gtkpod_warning().

   You need to add your handler to get_track_info_from_file() in
   file.c


   You also have to write a function to write TAGs back to the
   file. That function should be named 

   gboolean file_write_xxx_info (gchar *filename, Track *track, T_item tag_id)

   and return TRUE on success or FALSE on error. In that case it
   should log an error message using gtkpod_warning().

   @tag_id determines which TAGs of @track out of the list above are
   to be updated. If @tag_id==T_ALL, upudate all (supported) TAGs.

   You need to add your handler to file_write_info() in file.c

   ------------------------------------------------------------ */



#ifdef HAVE_LIBMP4V2
/* Use mp4v2 from the mpeg4ip project to handle mp4 (m4a, m4p) files
   (http://mpeg4ip.sourceforge.net/) */
/* Copyright note: code for file_get_mp4_info() is based on
 * mp4info.cpp of the mpeg4ip project */

/* define metadata bug is present (see note at file_write_mp4_info()) */
#define MP4V2_HAS_METADATA_BUG TRUE

#include "mp4.h"

Track *file_get_mp4_info (gchar *mp4FileName)
{
    Track *track = NULL;
    MP4FileHandle mp4File = MP4Read(mp4FileName, 0);

    if (mp4File != MP4_INVALID_FILE_HANDLE)
    {
	MP4TrackId trackId;
	const char *trackType;

	trackId = MP4FindTrackId(mp4File, 0, NULL, 0);
	trackType = MP4GetTrackType(mp4File, trackId);
	if (trackType && (strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0))
	{
	    gchar *value;
	    uint16_t numvalue, numvalue2;
	    MP4Duration trackDuration = MP4GetTrackDuration(mp4File, trackId);
	    double msDuration = UINT64_TO_DOUBLE(
		MP4ConvertFromTrackDuration(mp4File, trackId, 
					    trackDuration, MP4_MSECS_TIME_SCALE));
	    u_int32_t avgBitRate = MP4GetTrackBitRate(mp4File, trackId);

	    track = g_malloc0 (sizeof (Track));

	    track->tracklen = msDuration;
	    track->bitrate = avgBitRate/1000;
	    value = strrchr (mp4FileName, '.');
	    if (value)
	    {
		if (strcmp (value, ".m4a") == 0)
		    track->fdesc = g_strdup ("AAC audio file");
		if (strcmp (value, ".m4p") == 0)
		    track->fdesc = g_strdup ("Protected AAC audio file");
	    }
	    if (MP4GetMetadataName(mp4File, &value) && value != NULL)
	    {
		track->title = charset_to_utf8 (value);
		g_free(value);
	    }
	    if (MP4GetMetadataArtist(mp4File, &value) && value != NULL)
	    {
		track->artist = charset_to_utf8 (value);
		g_free(value);
	    }
	    if (MP4GetMetadataWriter(mp4File, &value) && value != NULL)
	    {
		track->composer = charset_to_utf8 (value);
		g_free(value);
	    }
	    if (MP4GetMetadataComment(mp4File, &value) && value != NULL)
	    {
		track->comment = charset_to_utf8 (value);
		g_free(value);
	    }
	    if (MP4GetMetadataYear(mp4File, &value) && value != NULL)
	    {
		track->year = atoi (value);
		g_free(value);
	    }
	    if (MP4GetMetadataAlbum(mp4File, &value) && value != NULL)
	    {
		track->album = charset_to_utf8 (value);
		g_free(value);
	    }
	    if (MP4GetMetadataTrack(mp4File, &numvalue, &numvalue2))
	    {
		track->track_nr = numvalue;
		track->tracks = numvalue2;
	    }
	    if (MP4GetMetadataDisk(mp4File, &numvalue, &numvalue2))
	    {
		track->cd_nr = numvalue;
		track->cds = numvalue2;
	    }
	    if (MP4GetMetadataGenre(mp4File, &value) && value != NULL)
	    {
		track->genre = charset_to_utf8 (value);
		g_free(value);
	    }
	}
	else
	{
	    gchar *filename = charset_to_utf8 (mp4FileName);
	    gtkpod_warning (_("'%s' does not appear to be a mp4 audio file.\n"),
			    filename);
	    g_free (filename);
	}
	MP4Close(mp4File);
    }
    else
    {
	gchar *filename = charset_to_utf8 (mp4FileName);
	gtkpod_warning (
	    _("Could not open '%s' for reading, or file is not an mp4 file.\n"),
	    filename);
	g_free (filename);
    }

    return track;
}


gboolean file_write_mp4_info (gchar *mp4FileName, Track *track, T_item tag_id)
{
    gboolean result = TRUE;
    MP4FileHandle mp4File = MP4Modify(mp4FileName, 0, FALSE);

    if (mp4File != MP4_INVALID_FILE_HANDLE)
    {
	MP4TrackId trackId;
	const char *trackType;

	trackId = MP4FindTrackId(mp4File, 0, NULL, 0);
	trackType = MP4GetTrackType(mp4File, trackId);
	if (trackType && (strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0))
	{
	    gchar *value;

#if MP4V2_HAS_METADATA_BUG
	    /* It could have been so easy. But: due to a bug in mp4v2
	     * you have to delete all meta data before modifying
	     * it. Therefore we have to read it first to avoid data
	     * loss. (Bug present in mpeg4ip-1.0RC1.) */
	    gchar *m_name = NULL, *m_artist = NULL;
	    gchar *m_writer = NULL, *m_comment = NULL;
	    gchar *m_tool = NULL, *m_year = NULL;
	    gchar *m_album = NULL, *m_genre = NULL;
	    u_int16_t m_track, m_tracks, m_disk, m_disks, m_tempo;
	    u_int8_t *m_covert = NULL, m_cpl;
	    u_int32_t m_size;
	    gboolean has_track = MP4GetMetadataTrack (mp4File,
						      &m_track, &m_tracks);
	    gboolean has_disk = MP4GetMetadataDisk (mp4File,
						    &m_disk, &m_disks);
	    gboolean has_tempo = MP4GetMetadataTempo (mp4File,
						      &m_tempo);
	    gboolean has_compilation = MP4GetMetadataCompilation (mp4File,
								  &m_cpl);
	    MP4GetMetadataName (mp4File, &m_name);
	    MP4GetMetadataArtist (mp4File, &m_artist);
	    MP4GetMetadataWriter (mp4File, &m_writer);
	    MP4GetMetadataComment (mp4File, &m_comment);
	    MP4GetMetadataTool (mp4File, &m_tool);
	    MP4GetMetadataYear (mp4File, &m_year);
	    MP4GetMetadataAlbum (mp4File, &m_album);
	    MP4GetMetadataGenre (mp4File, &m_genre);
	    MP4GetMetadataCoverArt (mp4File, &m_covert, &m_size);
	    MP4MetadataDelete (mp4File);
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_TITLE))
	    {
		value = charset_from_utf8 (track->title);
		MP4SetMetadataName (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_name) MP4SetMetadataName (mp4File, m_name);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_ARTIST))
	    {
		value = charset_from_utf8 (track->artist);
		MP4SetMetadataArtist (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_artist) MP4SetMetadataArtist (mp4File, m_artist);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_COMPOSER))
	    {
		value = charset_from_utf8 (track->composer);
		MP4SetMetadataWriter (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_writer) MP4SetMetadataWriter (mp4File, m_writer);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_COMMENT))
	    {
		value = charset_from_utf8 (track->comment);
		MP4SetMetadataComment (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_comment) MP4SetMetadataComment (mp4File, m_comment);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_YEAR))
	    {
		value = g_strdup_printf ("%d", track->year);
		MP4SetMetadataYear (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_year) MP4SetMetadataYear (mp4File, m_year);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_ALBUM))
	    {
		value = charset_from_utf8 (track->album);
		MP4SetMetadataAlbum (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_album) MP4SetMetadataAlbum (mp4File, m_album);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_TRACK_NR))
	    {
		MP4SetMetadataTrack (mp4File, track->track_nr, track->tracks);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (has_track) MP4SetMetadataTrack (mp4File, m_track, m_tracks);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_CD_NR))
	    {
		MP4SetMetadataDisk (mp4File, track->cd_nr, track->cds);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (has_disk) MP4SetMetadataDisk (mp4File, m_disk, m_disks);
	    }
#endif
	    if ((tag_id == T_ALL) || (tag_id == T_GENRE))
	    {
		value = charset_from_utf8 (track->genre);
		MP4SetMetadataGenre (mp4File, value);
		g_free (value);
	    }
#if MP4V2_HAS_METADATA_BUG
	    else
	    {
		if (m_genre) MP4SetMetadataGenre (mp4File, m_genre);
	    }

	    if (has_tempo) MP4SetMetadataTempo (mp4File, m_tempo);
	    if (has_compilation) MP4SetMetadataCompilation (mp4File, m_cpl);
	    if (m_tool)     MP4SetMetadataTool (mp4File, m_tool);
	    if (m_covert)   MP4SetMetadataCoverArt (mp4File, m_covert, m_size);
	    g_free (m_name);
	    g_free (m_artist);
	    g_free (m_writer);
	    g_free (m_comment);
	    g_free (m_tool);
	    g_free (m_year);
	    g_free (m_album);
	    g_free (m_genre);
	    g_free (m_covert);
#endif
	}
	else
	{
	    gchar *filename = charset_to_utf8 (mp4FileName);
	    gtkpod_warning (_("'%s' does not appear to be a mp4 audio file.\n"),
			    filename);
	    g_free (filename);
	    result = FALSE;
	}
	MP4Close (mp4File);
    }
    else
    {
	gchar *filename = charset_to_utf8 (mp4FileName);
	gtkpod_warning (
	    _("Could not open '%s' for writing, or file is not an mp4 file.\n"),
	    filename);
	g_free (filename);
	result = FALSE;
    }

    return result;
}

#else
/* Use our own code to read some information from mp4 files */
Track *file_get_mp4_info (gchar *name)
{
    gtkpod_warning (_("m4a/m4p not yet supported without the mp4v2 library.\n"));
    return NULL;
}

gboolean file_write_mp4_info (gchar *filename, Track *track, T_item tag_id)
{
    gtkpod_warning (_("m4a/m4p metadata writing not supported without the mp4v2 library.\n"));
    return FALSE;
}
#endif
