/* Time-stamp: <2003-11-08 01:43:34 jcs>
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

   @tag_id determines which TAGs of @track out of the list above is to
   be updated. If @tag_id==T_ALL, upudate all (supported) TAGs.

   ------------------------------------------------------------ */



#ifdef HAVE_LIBMP4V2
/* Use mp4v2 from the mpeg4ip project to handle mp4 (m4a, m4p) files
   (http://mpeg4ip.sourceforge.net/) */
/* Copyright note: code for file_get_mp4_info() is based on
 * mp4info.cpp of the mpeg4ip project */


#include "mp4.h"

Track *file_get_mp4_info (gchar *mp4FileName)
{
    MP4FileHandle mp4File;
    Track *track = NULL;

    mp4File = MP4Read(mp4FileName, 0);
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
	    _("Could not open '%s' for reading, or '%s' is not an mp4 file.\n"),
	    filename);
	g_free (filename);
    }

    return track;
}


gboolean file_write_mp4_info (gchar *filename, Track *track, T_item tag_id)
{
    gtkpod_warning (_("m4a/m4p metadata writing not yet supported.\n"));
    return FALSE;
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
    gtkpod_warning (_("m4a/m4p metadata writing not yet supported.\n"));
    return FALSE;
}
#endif
