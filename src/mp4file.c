/* Time-stamp: <2003-11-05 01:04:12 jcs>
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

#include "mp4file.h"
#include "song.h"
#include "charset.h"
#include "misc.h"
#include "support.h"

#ifdef HAVE_LIBMP4V2
/* Use mp4v2 from the mpeg4ip project to handle mp4 (m4a, m4p) files
   (http://mpeg4ip.sourceforge.net/) */
/* Copyright note: code for file_get_mp4_info() was essentially copied
 * from mp4info.cpp of the mpeg4ip project */


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
	    track->fdesc = g_strdup ("AAC audio file");

	    if (MP4GetMetadataName(mp4File, &value) && value != NULL)
	    {
		track->title = charset_to_utf8 (value);
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
		g_free(value);
	    }
	    if (MP4GetMetadataArtist(mp4File, &value) && value != NULL)
	    {
		track->artist = charset_to_utf8 (value);
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
		g_free(value);
	    }
	    if (MP4GetMetadataWriter(mp4File, &value) && value != NULL)
	    {
		track->composer = charset_to_utf8 (value);
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
		g_free(value);
	    }
	    if (MP4GetMetadataComment(mp4File, &value) && value != NULL)
	    {
		track->comment = charset_to_utf8 (value);
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
		g_free(value);
	    }
	    if (MP4GetMetadataYear(mp4File, &value) && value != NULL)
	    {
		track->year = atoi (value);
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
		g_free(value);
	    }
	    if (MP4GetMetadataAlbum(mp4File, &value) && value != NULL)
	    {
		track->album = charset_to_utf8 (value);
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
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
		if (!track->auto_charset)
		    track->auto_charset = charset_check_auto (value);
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
#else
/* Use our own code to read some information from mp4 files */
Track *file_get_mp4_info (gchar *name)
{
    return NULL;
}
#endif
