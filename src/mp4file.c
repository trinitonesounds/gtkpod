/* Time-stamp: <2009-08-01 17:46:51 jcs>
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

#include <glib/gi18n-lib.h>
#ifdef HAVE_ENDIAN_H
#  include <endian.h> /* for be32toh () */
#endif
#include "charset.h"
#include "itdb.h"
#include "misc.h"
#include "prefs.h"
#include "mp4file.h"



/* ------------------------------------------------------------

   Info on how to implement new file formats.

   You need to supply a function

   Track *xxx_get_file_info (gchar *filename)

   that returns a new Track structure with as many of the following
   fields filled in (in UTF8):

   gchar   *album;            /+ album (utf8)          +/
   gchar   *artist;           /+ artist (utf8)         +/
   gchar   *title;            /+ title (utf8)          +/
   gchar   *genre;            /+ genre (utf8)          +/
   gchar   *comment;          /+ comment (utf8)        +/
   gchar   *composer;         /+ Composer (utf8)       +/
   gchar   *filetype;         /+ Format description (utf8)   +/
   gchar   *charset;          /+ charset used for tags       +/
   gchar   *description;      /+ Description text (podcasts) +/
   gchar   *podcasturl;       /+ URL/Title (podcasts)        +/
   gchar   *podcastrss;       /+ Podcast RSS                 +/
   gchar   *subtitle;         /+ Subtitle (podcasts)         +/
   guint32 time_released;     /+ For podcasts: release date as
				 displayed next to the title in the
				 Podcast playlist  +/
   gint32  cd_nr;             /+ CD number             +/
   gint32  cds;               /+ number of CDs         +/
   gint32  track_nr;          /+ track number          +/
   gint32  tracks;            /+ number of tracks      +/
   gint32  year;              /+ year                  +/
   gint32  tracklen;          /+ Length of track in ms +/
   gint32  bitrate;           /+ bitrate in kbps       +/
   guint16 samplerate;        /+ e.g.: CD is 44100     +/
   guint32 peak_signal;	      /+ LAME Peak Signal * 0x800000         +/
   gboolean compilation;      /+ Track is part of a compilation CD   +/
   gboolean lyrics_flag;
   gint16 bpm;

   If prefs_get_int("readtags") returns FALSE you only should fill in
   tracklen, bitrate, samplerate, soundcheck and filetype

   If prefs_get_int("coverart_apic") returns TRUE you should try to
   read APIC coverart data from the tags and set it with
   gp_set_thumbnails_from_data().

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

   gboolean xxx_write_file_info (gchar *filename, Track *track)

   and return TRUE on success or FALSE on error. In that case it
   should log an error message using gtkpod_warning().

   You need to add your handler to file_write_info() in file.c


   Finally, you may want to provide a function that can
   read and set the soundcheck field:

   gboolean xxx_read_soundcheck (gchar *filename, Track *track)

   and return TRUE when the soundcheck value could be determined.

   You need to add your handler to read_soundcheck() in file.c.

   ------------------------------------------------------------ */

#define HAVE_LIBMP4V2 1
#define HAVE_LIBMP4V2_2 0

#ifdef HAVE_LIBMP4V2
/* Use mp4v2 from the mpeg4ip project to handle mp4 (m4a, m4p, m4b) files
   (http://mpeg4ip.sourceforge.net/) */
/* Copyright note: code for mp4_get_file_info() is based on
 * mp4info.cpp of the mpeg4ip project */

/* define metadata bug is present (see note at mp4_write_file_info()) */
#define MP4V2_HAS_METADATA_BUG TRUE

#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

/* mp4v2 dynamic load declarations - library handle and required definitions from mp4.h */

static void *mp4v2_handle = NULL;

#define DEFAULT(x)

typedef int         bool;
typedef void*		MP4FileHandle;
typedef u_int32_t	MP4TrackId;
typedef u_int32_t	MP4SampleId;
typedef u_int64_t	MP4Timestamp;
typedef u_int64_t	MP4Duration;

/* Don't use any symbols from itmf_tags.h since we're not linking against them, only getting the
 * format for the structs.  Included at this spot because MP4FileHandle must be defined.
 */
#define MP4V2_EXPORT
#if HAVE_MP4V2_ITMF_TAGS_H
# include <mp4v2/itmf_tags.h> /* For MP4Tags */
#endif

#define MP4_OD_TRACK_TYPE		"odsm"
#define MP4_AUDIO_TRACK_TYPE	"soun"
#define MP4_VIDEO_TRACK_TYPE	"vide"
#define MP4_INVALID_FILE_HANDLE	((MP4FileHandle)NULL)
#define MP4_MILLISECONDS_TIME_SCALE 1000
#define MP4_MSECS_TIME_SCALE	MP4_MILLISECONDS_TIME_SCALE

typedef MP4FileHandle (*MP4Read_t)(
	const char* fileName,
	u_int32_t verbosity DEFAULT(0));

typedef u_int32_t (*MP4GetNumberOfTracks_t)(
	MP4FileHandle hFile,
	const char* type DEFAULT(NULL),
	u_int8_t subType DEFAULT(0));

typedef MP4TrackId (*MP4FindTrackId_t)(
	MP4FileHandle hFile,
	u_int16_t index,
	const char* type DEFAULT(NULL),
	u_int8_t subType DEFAULT(0));

typedef const char* (*MP4GetTrackType_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId);

typedef void (*MP4Close_t)(
	MP4FileHandle hFile);

typedef bool (*MP4ReadSample_t)(
	/* input parameters */
	MP4FileHandle hFile,
	MP4TrackId trackId,
	MP4SampleId sampleId,
	/* input/output parameters */
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	/* output parameters */
	MP4Timestamp* pStartTime DEFAULT(NULL),
	MP4Duration* pDuration DEFAULT(NULL),
	MP4Duration* pRenderingOffset DEFAULT(NULL),
	bool* pIsSyncSample DEFAULT(NULL));

typedef u_int64_t (*MP4ConvertFromTrackTimestamp_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId,
	MP4Timestamp timeStamp,
	u_int32_t timeScale);

typedef u_int64_t (*MP4ConvertFromTrackDuration_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId,
	MP4Duration duration,
	u_int32_t timeScale);

typedef u_int32_t (*MP4GetTrackBitRate_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId);

typedef u_int32_t (*MP4GetTrackTimeScale_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId);

typedef u_int32_t (*MP4GetTrackMaxSampleSize_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId);

typedef MP4SampleId (*MP4GetTrackNumberOfSamples_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId);

typedef MP4Timestamp (*MP4GetSampleTime_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId,
	MP4SampleId sampleId);

typedef MP4Duration (*MP4GetTrackDuration_t)(
	MP4FileHandle hFile,
	MP4TrackId trackId);

typedef bool (*MP4GetMetadataName_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataArtist_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataAlbumArtist_t)(MP4FileHandle hFile,    char** value);
typedef bool (*MP4GetMetadataWriter_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataComment_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataYear_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataAlbum_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataTrack_t)(MP4FileHandle hFile,
			 u_int16_t* track, u_int16_t* totalTracks);
typedef bool (*MP4GetMetadataDisk_t)(MP4FileHandle hFile,
			u_int16_t* disk, u_int16_t* totalDisks);
typedef bool (*MP4GetMetadataGrouping_t)(MP4FileHandle hFile, char **grouping);
typedef bool (*MP4GetMetadataGenre_t)(MP4FileHandle hFile, char **genre);
typedef bool (*MP4GetMetadataTempo_t)(MP4FileHandle hFile, u_int16_t* tempo);
typedef bool (*MP4GetMetadataCoverArt_t)(MP4FileHandle hFile,
			    u_int8_t **coverArt, u_int32_t* size,
			    uint32_t index DEFAULT(0));
typedef bool (*MP4GetMetadataCompilation_t)(MP4FileHandle hFile, u_int8_t* cpl);
typedef bool (*MP4GetMetadataTool_t)(MP4FileHandle hFile, char** value);
typedef bool (*MP4GetMetadataFreeForm_t)(MP4FileHandle hFile, const char *name,
			    u_int8_t** pValue, u_int32_t* valueSize, const char *owner DEFAULT(NULL));

typedef bool (*MP4HaveAtom_t)(MP4FileHandle hFile,
		 const char *atomName);

typedef bool (*MP4GetIntegerProperty_t)(MP4FileHandle hFile,
		 const char* propName, u_int64_t *retval);
typedef bool (*MP4GetStringProperty_t)(MP4FileHandle hFile,
		 const char* propName, const char **retvalue);
typedef bool (*MP4GetBytesProperty_t)(MP4FileHandle hFile,
		 const char* propName, u_int8_t** ppValue, u_int32_t* pValueSize);
typedef bool (*MP4SetVerbosity_t)(MP4FileHandle hFile,
		 u_int32_t verbosity);

typedef bool (*MP4SetMetadataName_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataArtist_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataAlbumArtist_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataWriter_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataComment_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataYear_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataAlbum_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataTrack_t)(MP4FileHandle hFile,
			 u_int16_t track, u_int16_t totalTracks);
typedef bool (*MP4SetMetadataDisk_t)(MP4FileHandle hFile,
			u_int16_t disk, u_int16_t totalDisks);
typedef bool (*MP4SetMetadataTempo_t)(MP4FileHandle hFile, u_int16_t tempo);
typedef bool (*MP4SetMetadataGrouping_t)(MP4FileHandle hFile, const char *grouping);
typedef bool (*MP4SetMetadataGenre_t)(MP4FileHandle hFile, const char *genre);
typedef bool (*MP4SetMetadataCompilation_t)(MP4FileHandle hFile, u_int8_t cpl);
typedef bool (*MP4SetMetadataTool_t)(MP4FileHandle hFile, const char* value);
typedef bool (*MP4SetMetadataCoverArt_t)(MP4FileHandle hFile,
			    u_int8_t *coverArt, u_int32_t size);

typedef MP4FileHandle (*MP4Modify_t)(
	const char* fileName,
	u_int32_t verbosity DEFAULT(0),
	u_int32_t flags DEFAULT(0));

typedef const MP4Tags* (*MP4TagsAlloc_t)();
typedef void (*MP4TagsFetch_t)( const MP4Tags* tags, MP4FileHandle hFile );
typedef void (*MP4TagsFree_t)( const MP4Tags* tags );

typedef bool (*MP4MetadataDelete_t)(MP4FileHandle hFile);

static MP4Read_t MP4Read = NULL;
static MP4GetNumberOfTracks_t MP4GetNumberOfTracks = NULL;
static MP4FindTrackId_t MP4FindTrackId = NULL;
static MP4GetTrackType_t MP4GetTrackType = NULL;
static MP4Close_t MP4Close = NULL;
static MP4ReadSample_t MP4ReadSample = NULL;
static MP4ConvertFromTrackTimestamp_t MP4ConvertFromTrackTimestamp = NULL;
static MP4ConvertFromTrackDuration_t MP4ConvertFromTrackDuration = NULL;
static MP4GetTrackBitRate_t MP4GetTrackBitRate = NULL;
static MP4GetTrackTimeScale_t MP4GetTrackTimeScale = NULL;
static MP4GetTrackMaxSampleSize_t MP4GetTrackMaxSampleSize = NULL;
static MP4GetTrackNumberOfSamples_t MP4GetTrackNumberOfSamples = NULL;
static MP4GetSampleTime_t MP4GetSampleTime = NULL;
static MP4GetTrackDuration_t MP4GetTrackDuration = NULL;
static MP4GetMetadataName_t MP4GetMetadataName = NULL;
static MP4GetMetadataArtist_t MP4GetMetadataArtist = NULL;
static MP4GetMetadataAlbumArtist_t MP4GetMetadataAlbumArtist = NULL;
static MP4GetMetadataWriter_t MP4GetMetadataWriter = NULL;
static MP4GetMetadataComment_t MP4GetMetadataComment = NULL;
static MP4GetMetadataYear_t MP4GetMetadataYear = NULL;
static MP4GetMetadataAlbum_t MP4GetMetadataAlbum = NULL;
static MP4GetMetadataTrack_t MP4GetMetadataTrack = NULL;
static MP4GetMetadataDisk_t MP4GetMetadataDisk = NULL;
static MP4GetMetadataGrouping_t MP4GetMetadataGrouping = NULL;
static MP4GetMetadataGenre_t MP4GetMetadataGenre = NULL;
static MP4GetMetadataTempo_t MP4GetMetadataTempo = NULL;
static MP4GetMetadataCoverArt_t MP4GetMetadataCoverArt = NULL;
static MP4GetMetadataCompilation_t MP4GetMetadataCompilation = NULL;
static MP4GetMetadataTool_t MP4GetMetadataTool = NULL;
static MP4GetMetadataFreeForm_t MP4GetMetadataFreeForm = NULL;
static MP4HaveAtom_t MP4HaveAtom = NULL;
static MP4GetIntegerProperty_t MP4GetIntegerProperty = NULL;
static MP4GetStringProperty_t MP4GetStringProperty = NULL;
static MP4GetBytesProperty_t MP4GetBytesProperty = NULL;
static MP4SetVerbosity_t MP4SetVerbosity = NULL;
static MP4SetMetadataName_t MP4SetMetadataName = NULL;
static MP4SetMetadataArtist_t MP4SetMetadataArtist = NULL;
static MP4SetMetadataAlbumArtist_t MP4SetMetadataAlbumArtist = NULL;
static MP4SetMetadataWriter_t MP4SetMetadataWriter = NULL;
static MP4SetMetadataComment_t MP4SetMetadataComment = NULL;
static MP4SetMetadataYear_t MP4SetMetadataYear = NULL;
static MP4SetMetadataAlbum_t MP4SetMetadataAlbum = NULL;
static MP4SetMetadataTrack_t MP4SetMetadataTrack = NULL;
static MP4SetMetadataDisk_t MP4SetMetadataDisk = NULL;
static MP4SetMetadataTempo_t MP4SetMetadataTempo = NULL;
static MP4SetMetadataGrouping_t MP4SetMetadataGrouping = NULL;
static MP4SetMetadataGenre_t MP4SetMetadataGenre = NULL;
static MP4SetMetadataCompilation_t MP4SetMetadataCompilation = NULL;
static MP4SetMetadataTool_t MP4SetMetadataTool = NULL;
static MP4SetMetadataCoverArt_t MP4SetMetadataCoverArt = NULL;
static MP4Modify_t MP4Modify = NULL;
static MP4MetadataDelete_t MP4MetadataDelete = NULL;
static MP4TagsAlloc_t MP4TagsAllocFunc = NULL;
static MP4TagsFetch_t MP4TagsFetchFunc = NULL;
static MP4TagsFree_t MP4TagsFreeFunc = NULL;

/* end mp4v2 dynamic load declarations */

/* mp4v2 initialization code */

void mp4_init()
{
    mp4v2_handle = dlopen("libmp4v2.so.1", RTLD_LAZY);

    if (!mp4v2_handle)
    {
        mp4v2_handle = dlopen("libmp4v2.so.0", RTLD_LAZY);
        
        if (!mp4v2_handle)
        {
            return;
        }
    }

    MP4Read = (MP4Read_t) dlsym(mp4v2_handle, "MP4Read");
    MP4GetNumberOfTracks = (MP4GetNumberOfTracks_t) dlsym(mp4v2_handle, "MP4GetNumberOfTracks");
    MP4FindTrackId = (MP4FindTrackId_t) dlsym(mp4v2_handle, "MP4FindTrackId");
    MP4GetTrackType = (MP4GetTrackType_t) dlsym(mp4v2_handle, "MP4GetTrackType");
    MP4Close = (MP4Close_t) dlsym(mp4v2_handle, "MP4Close");
    MP4ReadSample = (MP4ReadSample_t) dlsym(mp4v2_handle, "MP4ReadSample");
    MP4ConvertFromTrackTimestamp = (MP4ConvertFromTrackTimestamp_t) dlsym(mp4v2_handle, "MP4ConvertFromTrackTimestamp");
    MP4ConvertFromTrackDuration = (MP4ConvertFromTrackDuration_t) dlsym(mp4v2_handle, "MP4ConvertFromTrackDuration");
    MP4GetTrackBitRate = (MP4GetTrackBitRate_t) dlsym(mp4v2_handle, "MP4GetTrackBitRate");
    MP4GetTrackTimeScale = (MP4GetTrackTimeScale_t) dlsym(mp4v2_handle, "MP4GetTrackTimeScale");
    MP4GetTrackMaxSampleSize = (MP4GetTrackMaxSampleSize_t) dlsym(mp4v2_handle, "MP4GetTrackMaxSampleSize");
    MP4GetTrackNumberOfSamples = (MP4GetTrackNumberOfSamples_t) dlsym(mp4v2_handle, "MP4GetTrackNumberOfSamples");
    MP4GetSampleTime = (MP4GetSampleTime_t) dlsym(mp4v2_handle, "MP4GetSampleTime");
    MP4GetTrackDuration = (MP4GetTrackDuration_t) dlsym(mp4v2_handle, "MP4GetTrackDuration");
    MP4GetMetadataName = (MP4GetMetadataName_t) dlsym(mp4v2_handle, "MP4GetMetadataName");
    MP4GetMetadataArtist = (MP4GetMetadataArtist_t) dlsym(mp4v2_handle, "MP4GetMetadataArtist");
    MP4GetMetadataAlbumArtist = (MP4GetMetadataAlbumArtist_t) dlsym(mp4v2_handle, "MP4GetMetadataAlbumArtist");
    MP4GetMetadataWriter = (MP4GetMetadataWriter_t) dlsym(mp4v2_handle, "MP4GetMetadataWriter");
    MP4GetMetadataComment = (MP4GetMetadataComment_t) dlsym(mp4v2_handle, "MP4GetMetadataComment");
    MP4GetMetadataYear = (MP4GetMetadataYear_t) dlsym(mp4v2_handle, "MP4GetMetadataYear");
    MP4GetMetadataAlbum = (MP4GetMetadataAlbum_t) dlsym(mp4v2_handle, "MP4GetMetadataAlbum");
    MP4GetMetadataTrack = (MP4GetMetadataTrack_t) dlsym(mp4v2_handle, "MP4GetMetadataTrack");
    MP4GetMetadataDisk = (MP4GetMetadataDisk_t) dlsym(mp4v2_handle, "MP4GetMetadataDisk");
    MP4GetMetadataGrouping = (MP4GetMetadataGrouping_t) dlsym(mp4v2_handle, "MP4GetMetadataGrouping");
    MP4GetMetadataGenre = (MP4GetMetadataGenre_t) dlsym(mp4v2_handle, "MP4GetMetadataGenre");
    MP4GetMetadataTempo = (MP4GetMetadataTempo_t) dlsym(mp4v2_handle, "MP4GetMetadataTempo");
    MP4GetMetadataCoverArt = (MP4GetMetadataCoverArt_t) dlsym(mp4v2_handle, "MP4GetMetadataCoverArt");
    MP4GetMetadataCompilation = (MP4GetMetadataCompilation_t) dlsym(mp4v2_handle, "MP4GetMetadataCompilation");
    MP4GetMetadataTool = (MP4GetMetadataTool_t) dlsym(mp4v2_handle, "MP4GetMetadataTool");
    MP4GetMetadataFreeForm = (MP4GetMetadataFreeForm_t) dlsym(mp4v2_handle, "MP4GetMetadataFreeForm");
    MP4HaveAtom = (MP4HaveAtom_t) dlsym(mp4v2_handle, "MP4HaveAtom");
    MP4GetIntegerProperty = (MP4GetIntegerProperty_t) dlsym(mp4v2_handle, "MP4GetIntegerProperty");
    MP4GetStringProperty = (MP4GetStringProperty_t) dlsym(mp4v2_handle, "MP4GetStringProperty");
    MP4GetBytesProperty = (MP4GetBytesProperty_t) dlsym(mp4v2_handle, "MP4GetBytesProperty");
    MP4SetVerbosity = (MP4SetVerbosity_t) dlsym(mp4v2_handle, "MP4SetVerbosity");
    MP4SetMetadataName = (MP4SetMetadataName_t) dlsym(mp4v2_handle, "MP4SetMetadataName");
    MP4SetMetadataArtist = (MP4SetMetadataArtist_t) dlsym(mp4v2_handle, "MP4SetMetadataArtist");
    MP4SetMetadataAlbumArtist = (MP4SetMetadataAlbumArtist_t) dlsym(mp4v2_handle, "MP4SetMetadataAlbumArtist");
    MP4SetMetadataWriter = (MP4SetMetadataWriter_t) dlsym(mp4v2_handle, "MP4SetMetadataWriter");
    MP4SetMetadataComment = (MP4SetMetadataComment_t) dlsym(mp4v2_handle, "MP4SetMetadataComment");
    MP4SetMetadataYear = (MP4SetMetadataYear_t) dlsym(mp4v2_handle, "MP4SetMetadataYear");
    MP4SetMetadataAlbum = (MP4SetMetadataAlbum_t) dlsym(mp4v2_handle, "MP4SetMetadataAlbum");
    MP4SetMetadataTrack = (MP4SetMetadataTrack_t) dlsym(mp4v2_handle, "MP4SetMetadataTrack");
    MP4SetMetadataDisk = (MP4SetMetadataDisk_t) dlsym(mp4v2_handle, "MP4SetMetadataDisk");
    MP4SetMetadataTempo = (MP4SetMetadataTempo_t) dlsym(mp4v2_handle, "MP4SetMetadataTempo");
    MP4SetMetadataGrouping = (MP4SetMetadataGrouping_t) dlsym(mp4v2_handle, "MP4SetMetadataGrouping");
    MP4SetMetadataGenre = (MP4SetMetadataGenre_t) dlsym(mp4v2_handle, "MP4SetMetadataGenre");
    MP4SetMetadataCompilation = (MP4SetMetadataCompilation_t) dlsym(mp4v2_handle, "MP4SetMetadataCompilation");
    MP4SetMetadataTool = (MP4SetMetadataTool_t) dlsym(mp4v2_handle, "MP4SetMetadataTool");
    MP4SetMetadataCoverArt = (MP4SetMetadataCoverArt_t) dlsym(mp4v2_handle, "MP4SetMetadataCoverArt");
    MP4Modify = (MP4Modify_t) dlsym(mp4v2_handle, "MP4Modify");
    MP4MetadataDelete = (MP4MetadataDelete_t) dlsym(mp4v2_handle, "MP4MetadataDelete");
    MP4TagsAllocFunc = (MP4TagsAlloc_t) dlsym(mp4v2_handle, "MP4TagsAlloc");
    MP4TagsFetchFunc = (MP4TagsFetch_t) dlsym(mp4v2_handle, "MP4TagsFetch");
    MP4TagsFreeFunc = (MP4TagsFree_t) dlsym(mp4v2_handle, "MP4TagsFree");

    /* alternate names for HAVE_LIBMP4V2_2 */

    if(!MP4GetMetadataWriter)
    {
        MP4GetMetadataWriter = (MP4GetMetadataWriter_t) dlsym(mp4v2_handle, "MP4GetMetadataComposer");
    }

    if(!MP4GetMetadataYear)
    {
        MP4GetMetadataYear = (MP4GetMetadataYear_t) dlsym(mp4v2_handle, "MP4GetMetadataReleaseDate");
    }

    if(!MP4GetMetadataTempo)
    {
        MP4GetMetadataTempo = (MP4GetMetadataTempo_t) dlsym(mp4v2_handle, "MP4GetMetadataBPM");
    }

    if(!MP4SetMetadataWriter)
    {
        MP4SetMetadataWriter = (MP4SetMetadataWriter_t) dlsym(mp4v2_handle, "MP4SetMetadataComposer");
    }

    if(!MP4SetMetadataYear)
    {
        MP4SetMetadataYear = (MP4SetMetadataYear_t) dlsym(mp4v2_handle, "MP4SetMetadataReleaseDate");
    }

    if(!MP4SetMetadataTempo)
    {
        MP4SetMetadataTempo = (MP4SetMetadataTempo_t) dlsym(mp4v2_handle, "MP4SetMetadataYear");
    }
}

void mp4_close()
{
    if (mp4v2_handle)
    {
        dlclose(mp4v2_handle);
    }
}

/* end mp4v2 initialization code */

static guint32 mediaTypeTagToMediaType(guint8 media_type)
{
   switch (media_type)
   {
      case 0: /* Movie */
         return ITDB_MEDIATYPE_MOVIE;
      case 1: /* Normal */
         break;
      case 2: /* Audiobook */
         return ITDB_MEDIATYPE_AUDIOBOOK;
      case 5: /* Whacked Bookmark */
         break;
      case 6: /* Music Video */
         return ITDB_MEDIATYPE_MUSICVIDEO;
      case 9: /* Short Film */
         break;
      case 10: /* TV Show */
         return ITDB_MEDIATYPE_TVSHOW;
      case 11: /* Booklet */
         break;
   }
   return 0;
}

/* According to http://atomicparsley.sourceforge.net/mpeg-4files.html, the format is different
 * for different atoms.  Sheesh, Apple!  Fortunately, the ones we care about (tvsn, stik, tves)
 * are all the same either 17 or 20 byte format.
 */
static gboolean mp4_get_apple_uint8_property (MP4FileHandle hFile, const char* propName, u_int8_t* ppValue)
{
   u_int8_t *pos;
   u_int8_t *value;
   guint32 valuelen;
   guint32 class_flag;
   guint8 atom_version;
   gboolean success = FALSE;

   success = MP4GetBytesProperty (hFile, propName, &value, &valuelen);
   if (success == TRUE && valuelen > 16)
   {
      success = FALSE;
      pos = value;
      pos += 8; /* Skip over the length and the atom name */

      /* pos now points to a 1-byte atom version followed by a 3-byte class/flag field */
      atom_version = *pos;
      class_flag = be32toh(*(guint32*)pos) & 0x00ffffff;
      if (class_flag == 21 || class_flag == 0)
      {
         pos += 4; /* Skip over the atom version and class/flag */
         pos += 4; /* Skip over the null space */
         if (valuelen == 17)
            success = TRUE, *ppValue = pos[0];
         else if (valuelen == 20)
            success = TRUE, *ppValue = pos[3];
      }
   }
   g_free (value);
   return success;
}

static gboolean mp4_get_apple_text_property (MP4FileHandle hFile, const char* propName, gchar** ppValue)
{
   u_int8_t *pos;
   u_int8_t *value;
   guint32 valuelen;
   guint32 class_flag;
   guint8 atom_version;
   gboolean success = FALSE;

   success = MP4GetBytesProperty (hFile, propName, &value, &valuelen);
   if (success == TRUE && valuelen >= 16)
   {
      success = FALSE;
      pos = value;
      pos += 8; /* Skip over the length and the atom name */
      /* pos now points to a 1-byte atom version followed by a 3-byte class/flag field */
      atom_version = *pos;
      class_flag = be32toh(*(guint32*)pos) & 0x00ffffff;
      if (class_flag == 1)
      {
         pos += 4; /* Skip over the atom version and class/flag */
         pos += 4; /* Skip over the null space */
         /* The string is already in UTF-8 format */
         *ppValue = g_strndup (pos, valuelen - (pos - value));
         success = TRUE;
      }
   }
   g_free (value);
   return success;
}

static gboolean mp4_scan_soundcheck (MP4FileHandle mp4File, Track *track)
{
    gboolean success = FALSE;
    u_int8_t *ppValue;
    u_int32_t pValueSize;


    g_return_val_if_fail (mp4File != MP4_INVALID_FILE_HANDLE, FALSE);

    if (MP4GetMetadataFreeForm(mp4File, "iTunNORM",
			       &ppValue, &pValueSize, NULL))
    {
	gchar *str;
	guint sc1=0, sc2=0;
	str = g_malloc0((pValueSize+1)*sizeof(gchar));
	memcpy(str, ppValue, pValueSize*sizeof(gchar));
	/* This field consists of a number of hex numbers
	   represented in ASCII, e.g. " 00000FA7 00000B3F
	   000072CF 00006AB6 0001CF53 00016310 0000743A
	   00007C1F 00023DD5 000162E2". iTunes seems to
	   choose the larger one of the first two numbers
	   as the value for track->soundcheck */
	sscanf (str, "%x %x", &sc1, &sc2);
	g_free (str);
	if (sc1 > sc2)
	    track->soundcheck = sc1;
	else
	    track->soundcheck = sc2;
	success = TRUE;
    }

    if (MP4GetMetadataFreeForm(mp4File, "replaygain_track_gain",
			       &ppValue, &pValueSize, NULL))
    {
	gchar *str;
	gdouble rg;
	str = g_malloc0((pValueSize+1)*sizeof(gchar));
	memcpy(str, ppValue, pValueSize*sizeof(gchar));
	rg = g_strtod (str, NULL);
	track->soundcheck = replaygain_to_soundcheck (rg);
	g_free (str);

	success = TRUE;
    }

    return success;
}


gboolean mp4_read_soundcheck (gchar *mp4FileName, Track *track)
{
    if (!mp4v2_handle)
    {
        gtkpod_warning (_("m4a/m4p/m4b soundcheck update for '%s' failed: m4a/m4p/m4b not supported without the mp4v2 library. You must install the mp4v2 library.\n"), mp4FileName);
        return FALSE;
    }

    gboolean success = FALSE;
    MP4FileHandle mp4File;

    g_return_val_if_fail (mp4FileName, FALSE);
    g_return_val_if_fail (track, FALSE);

    mp4File = MP4Read(mp4FileName, 0);

    if (mp4File != MP4_INVALID_FILE_HANDLE)
    {
	MP4TrackId trackId;
	const char *trackType;
	u_int32_t track_cur, tracks_num;
	gboolean audio_or_video_found = FALSE;

	tracks_num = MP4GetNumberOfTracks (mp4File, NULL,  0);

	for (track_cur=0; track_cur < tracks_num; ++track_cur)
	{
	    trackId = MP4FindTrackId(mp4File, track_cur, NULL, 0);
	    trackType = MP4GetTrackType(mp4File, trackId);

	    if (trackType &&
		((strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0) ||
		 (strcmp(trackType, MP4_VIDEO_TRACK_TYPE) == 0) ||
		 (strcmp(trackType, MP4_OD_TRACK_TYPE) == 0)))
	    {
		audio_or_video_found = TRUE;
		success = mp4_scan_soundcheck (mp4File, track);
	    }
	    if (audio_or_video_found) break;
	}
	if (!audio_or_video_found)
	{
	    gchar *filename = charset_to_utf8 (mp4FileName);
	    gtkpod_warning (
		_("'%s' does not appear to be a mp4 audio or video file.\n"),
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

    return success;
}



Track *mp4_get_file_info (gchar *mp4FileName)
{
    if (!mp4v2_handle)
    {
        gtkpod_warning (_("Import of '%s' failed: m4a/m4p/m4b not supported without the mp4v2 library. You must install the mp4v2 library.\n"), mp4FileName);
        return NULL;
    }

    Track *track = NULL;
    MP4FileHandle mp4File;

    g_return_val_if_fail (mp4FileName, NULL);

    mp4File = MP4Read(mp4FileName, 0);

    if (mp4File != MP4_INVALID_FILE_HANDLE)
    {
	MP4TrackId trackId;
	const char *trackType;
	u_int32_t track_cur, tracks_num;
	gboolean audio_or_video_found = FALSE;
/*	gboolean artwork_found = FALSE; not used yet */

	tracks_num = MP4GetNumberOfTracks (mp4File, NULL,  0);

	for (track_cur=0; track_cur < tracks_num; ++track_cur)
	{
	    trackId = MP4FindTrackId(mp4File, track_cur, NULL, 0);
	    trackType = MP4GetTrackType(mp4File, trackId);
	    if (trackType && (strcmp(trackType, "text") == 0))
	    {
		u_int32_t m_max_frame_size;
		m_max_frame_size = MP4GetTrackMaxSampleSize(mp4File, trackId) + 4;
		MP4SampleId samples = MP4GetTrackNumberOfSamples(mp4File, trackId);
		MP4SampleId i;
		Itdb_Chapterdata *chapterdata = itdb_chapterdata_new();
		for (i=1; i<=samples; i++)
		{
		    u_int8_t *m_buffer;
		    m_buffer = (u_int8_t *) malloc(m_max_frame_size * sizeof(u_int8_t));
		    u_int32_t m_this_frame_size = m_max_frame_size;
		    u_int8_t *buffer;
		    buffer = m_buffer;
		    gchar *title;
		    if (!MP4ReadSample(mp4File, trackId, i, &buffer, &m_this_frame_size, NULL, NULL, NULL, NULL))
		    {
			/* chapter title couldn't be read; probably using
			 * an older version of libmp4v2.  We'll just make
			 * our own titles, since the ipod doesn't display
			 * them anyway
			 */
			free (m_buffer);
			m_buffer = (u_int8_t *) malloc(12 * sizeof(u_int8_t));
			sprintf(m_buffer, "Chapter %03i", i);
			m_buffer[11] = '\0';
			title = g_strdup(m_buffer);
		    }
		    else
		    {
			int titlelength = (buffer[0] << 8) + buffer[1];
			/* If a title begins with 0xFFFE, it's a UTF-16 title */
			if (titlelength>2 && buffer[2]==0xff && buffer[3]==0xfe)
			{
			        titlelength -= 2;
			        gchar *newtitle = g_utf16_to_utf8((const gunichar2 *)&buffer[4], titlelength, NULL, NULL, NULL);
			        title = g_strdup (newtitle);
			        g_free(newtitle);
			}
			else
			{
			        gchar *newtitle = (gchar *) malloc((titlelength+1) * sizeof(gchar));
			        newtitle = g_strndup (&buffer[2], titlelength);
			        newtitle[titlelength] = '\0';
			        title = g_strdup (newtitle);
			        free (newtitle);
			}
		    }

		    MP4Timestamp sampletime = MP4GetSampleTime(mp4File, trackId, i);
		    u_int64_t convertedsampletime = MP4ConvertFromTrackTimestamp(mp4File,
			    trackId, sampletime, MP4_MILLISECONDS_TIME_SCALE);
		    itdb_chapterdata_add_chapter(chapterdata, convertedsampletime, title);
		}
		track->chapterdata = itdb_chapterdata_duplicate (chapterdata);

		itdb_chapterdata_free(chapterdata);
	    }
	    if (trackType &&
		(audio_or_video_found == FALSE) &&
		((strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0) ||
		 (strcmp(trackType, MP4_VIDEO_TRACK_TYPE) == 0) ||
		 (strcmp(trackType, MP4_OD_TRACK_TYPE) == 0)))
	    {
		gchar *value;
		guint16 numvalue, numvalue2;
		u_int8_t numvalue3;
		gboolean possibly_tv_show = FALSE;
#if HAVE_MP4V2_ITMF_TAGS_H
		const MP4Tags* mp4tags = NULL;
#endif
		MP4Duration trackDuration = MP4GetTrackDuration(mp4File, trackId);
		double msDuration =
		    (double)MP4ConvertFromTrackDuration(mp4File, trackId,
							trackDuration,
							MP4_MSECS_TIME_SCALE);
		guint32 avgBitRate = MP4GetTrackBitRate(mp4File, trackId);
		guint32 samplerate = MP4GetTrackTimeScale(mp4File, trackId);

		track = gp_track_new ();

		track->tracklen = msDuration;
		track->bitrate = avgBitRate/1000;
		track->samplerate = samplerate;
		value = strrchr (mp4FileName, '.');
		if (value)
		{
		    if (g_strcasecmp (value, ".m4a") == 0)
			track->filetype = g_strdup ("AAC audio file");
		    if (g_strcasecmp (value, ".m4p") == 0)
			track->filetype = g_strdup ("Protected AAC audio file");
		    if (g_strcasecmp (value, ".m4b") == 0)
			track->filetype = g_strdup ("AAC audio book file");
		    if (g_strcasecmp (value, ".mp4") == 0)
			track->filetype = g_strdup ("MP4 video file");
		}
		if (prefs_get_int("readtags"))
		{
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

            if(MP4SetMetadataAlbumArtist)
            {
                if (!track->artist || !*track->artist)
		        {
			    g_free (track->artist);
			    track->artist = NULL;
			    if (MP4GetMetadataAlbumArtist(mp4File, &value) && value != NULL)
			    {
			        track->artist = charset_to_utf8 (value);
			    }
		        }
		        else
		        {
			    if (MP4GetMetadataAlbumArtist(mp4File, &value) && value != NULL)
			    {
			        track->albumartist = charset_to_utf8 (value);
			    }
		        }
            }

#if HAVE_LIBMP4V2_2
		    if (MP4GetMetadataComposer(mp4File, &value) && value != NULL)
#else
		    if (MP4GetMetadataWriter(mp4File, &value) && value != NULL)
#endif
		    {
			track->composer = charset_to_utf8 (value);
			g_free(value);
		    }
		    if (MP4GetMetadataComment(mp4File, &value) && value != NULL)
		    {
			track->comment = charset_to_utf8 (value);
			g_free(value);
		    }
#if HAVE_LIBMP4V2_2
		    if (MP4GetMetadataReleaseDate(mp4File, &value) && value != NULL)
#else
		    if (MP4GetMetadataYear(mp4File, &value) && value != NULL)
#endif
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
		    if (MP4GetMetadataGrouping(mp4File, &value) && value != NULL)
		    {
			track->grouping = charset_to_utf8 (value);
			g_free (value);
		    }
		    if (MP4GetMetadataGenre(mp4File, &value) && value != NULL)
		    {
			track->genre = charset_to_utf8 (value);
			g_free(value);
		    }
#if HAVE_LIBMP4V2_2
		    if (MP4GetMetadataBPM (mp4File, &numvalue))
#else
		    if (MP4GetMetadataTempo (mp4File, &numvalue))
#endif
		    {
			track->BPM = numvalue;
		    }
		    /* Apple-specific atoms */
		    if (MP4HaveAtom (mp4File, "moov.udta.meta.ilst.\251lyr"))
		    {
			track->lyrics_flag = 0x01;
		    }
#if HAVE_MP4V2_ITMF_TAGS_H
		    if (MP4TagsAllocFunc != NULL && MP4TagsFetchFunc != NULL && MP4TagsFreeFunc != NULL)
		    {
			mp4tags = MP4TagsAllocFunc ();
			MP4TagsFetchFunc (mp4tags, mp4File);
			if (mp4tags->tvShow)
			    track->tvshow = g_strdup (mp4tags->tvShow);
			if (mp4tags->tvEpisodeID)
			    track->tvepisode = g_strdup (mp4tags->tvEpisodeID);
			if (mp4tags->tvNetwork)
			    track->tvnetwork = g_strdup (mp4tags->tvNetwork);
			if (mp4tags->tvSeason)
			    track->season_nr = *mp4tags->tvSeason;
			if (mp4tags->tvEpisode)
			    track->episode_nr = *mp4tags->tvEpisode;
			if (mp4tags->tvEpisode)
			    track->mediatype = mediaTypeTagToMediaType (*mp4tags->mediaType);
			MP4TagsFreeFunc (mp4tags);
		    }
		    else
#endif
		    {
			/* Since we either weren't compiled with mp4v2/itmf_tags.h, or the MP4Tags* functions
			 * weren't available when libmp4v2 was dlopen()ed, we'll dig for the atom props manually. */
			if (mp4_get_apple_text_property (mp4File, "moov.udta.meta.ilst.tvsh.data", &track->tvshow))
			    possibly_tv_show = TRUE;
			if (mp4_get_apple_uint8_property (mp4File, "moov.udta.meta.ilst.tvsn.data", &numvalue3))
			    track->season_nr = numvalue3, possibly_tv_show = TRUE;
			if (mp4_get_apple_uint8_property (mp4File, "moov.udta.meta.ilst.tves.data", &numvalue3))
			    track->episode_nr = numvalue3, possibly_tv_show = TRUE;
			/* For some reason, the stik's data atom doesn't get found, so we make a guess at the
			 * media type with possibly_tv_show */
			if (mp4_get_apple_uint8_property (mp4File, "moov.udta.meta.ilst.stik.data", &numvalue3))
			{
			    track->mediatype = mediaTypeTagToMediaType (numvalue3);
			    fprintf (stderr, "Got a stik atom: %d, %d\n", numvalue3, track->mediatype);
			}
			else if (possibly_tv_show)
			{
			    track->mediatype = ITDB_MEDIATYPE_TVSHOW;
			}
		    }
		}
		mp4_scan_soundcheck (mp4File, track);
		audio_or_video_found = TRUE;

		if (prefs_get_int("coverart_apic"))
		{
		    u_int8_t *image_data;
		    u_int32_t image_data_len;
		    if (MP4GetMetadataCoverArt (mp4File,
						&image_data, &image_data_len, 0))
		    {
			if (image_data)
			{
/*			    FILE *file = fopen ("/tmp/tttt", "w");
			    fwrite (image_data, 1, image_data_len, file);
			    fclose (file);*/
			    gp_track_set_thumbnails_from_data (track,
							       image_data,
							       image_data_len);
			    g_free (image_data);
			}
		    }
		}
	    }
	}
	if (!audio_or_video_found)
	{
	    gchar *filename = charset_to_utf8 (mp4FileName);
	    gtkpod_warning (
		_("'%s' does not appear to be a mp4 audio or video file.\n"),
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


gboolean mp4_write_file_info (gchar *mp4FileName, Track *track)
{
    if (!mp4v2_handle)
    {
        gtkpod_warning (_("m4a/m4p/m4b metadata update for '%s' failed: m4a/m4p/m4b not supported without the mp4v2 library. You must install the mp4v2 library.\n"), mp4FileName);
        return FALSE;
    }

    gboolean result = TRUE;
    MP4FileHandle mp4File = MP4Modify(mp4FileName, 0, FALSE);

    if (mp4File != MP4_INVALID_FILE_HANDLE)
    {
	MP4TrackId trackId;
	const char *trackType;

	trackId = MP4FindTrackId(mp4File, 0, NULL, 0);
	trackType = MP4GetTrackType(mp4File, trackId);
	if (trackType && ((strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0)||(strcmp(trackType, MP4_VIDEO_TRACK_TYPE) == 0)))
	{
	    gchar *value;

#if MP4V2_HAS_METADATA_BUG
	    /* It could have been so easy. But: due to a bug in mp4v2
	     * you have to delete all meta data before modifying
	     * it. Therefore we have to read it first to avoid data
	     * loss. (Bug present in mpeg4ip-1.0RC1.) */
/*	    gchar *m_name = NULL, *m_artist = NULL, *m_albumartist = NULL;
	    gchar *m_writer = NULL, *m_comment = NULL;
	    gchar *m_year = NULL;
	    gchar *m_album = NULL, *m_genre = NULL;*/
	    gchar *m_tool = NULL;
/*	    guint16 m_track, m_tracks, m_disk, m_disks; */
	    guint16 m_tempo;
	    guint8 *m_covert = NULL, m_cpl;
	    guint32 m_size;
        /*	    gboolean has_track = MP4GetMetadataTrack (mp4File,
						      &m_track, &m_tracks);
	    gboolean has_disk = MP4GetMetadataDisk (mp4File,
	    &m_disk, &m_disks);*/
#if HAVE_LIBMP4V2_2
	    gboolean has_tempo = MP4GetMetadataBPM (mp4File,
						      &m_tempo);
#else
	    gboolean has_tempo = MP4GetMetadataTempo (mp4File,
						      &m_tempo);
#endif
	    gboolean has_compilation = MP4GetMetadataCompilation (mp4File,
								  &m_cpl);
	    MP4GetMetadataTool (mp4File, &m_tool);
	    MP4GetMetadataCoverArt (mp4File, &m_covert, &m_size, 0);
	    MP4MetadataDelete (mp4File);
#endif
	    value = charset_from_utf8 (track->title);
	    MP4SetMetadataName (mp4File, value);
	    g_free (value);

	    value = charset_from_utf8 (track->artist);
	    MP4SetMetadataArtist (mp4File, value);
	    g_free (value);

        if(MP4SetMetadataAlbumArtist)
        {
	        value = charset_from_utf8 (track->albumartist);
	        MP4SetMetadataAlbumArtist (mp4File, value);
	        g_free (value);
        }

	    value = charset_from_utf8 (track->composer);
#if HAVE_LIBMP4V2_2
	    MP4SetMetadataComposer (mp4File, value);
#else
	    MP4SetMetadataWriter (mp4File, value);
#endif
	    g_free (value);

	    value = charset_from_utf8 (track->comment);
	    MP4SetMetadataComment (mp4File, value);
	    g_free (value);

	    value = g_strdup_printf ("%d", track->year);
#if HAVE_LIBMP4V2_2
	    MP4SetMetadataReleaseDate (mp4File, value);
#else
	    MP4SetMetadataYear (mp4File, value);
#endif
	    g_free (value);

	    value = charset_from_utf8 (track->album);
	    MP4SetMetadataAlbum (mp4File, value);
	    g_free (value);

	    MP4SetMetadataTrack (mp4File, track->track_nr, track->tracks);

	    MP4SetMetadataDisk (mp4File, track->cd_nr, track->cds);

#if HAVE_LIBMP4V2_2
	    MP4SetMetadataBPM (mp4File, track->BPM);
#else
	    MP4SetMetadataTempo (mp4File, track->BPM);
#endif

	    value = charset_from_utf8 (track->grouping);
	    MP4SetMetadataGrouping (mp4File, value);
	    g_free (value);

	    value = charset_from_utf8 (track->genre);
	    MP4SetMetadataGenre (mp4File, value);
	    g_free (value);

#if MP4V2_HAS_METADATA_BUG
#if HAVE_LIBMP4V2_2
	    if (has_tempo) MP4SetMetadataBPM (mp4File, m_tempo);
#else
	    if (has_tempo) MP4SetMetadataTempo (mp4File, m_tempo);
#endif
	    if (has_compilation) MP4SetMetadataCompilation (mp4File, m_cpl);
	    if (m_tool)     MP4SetMetadataTool (mp4File, m_tool);
        if (m_covert)   MP4SetMetadataCoverArt (mp4File, m_covert, m_size);

	    g_free (m_tool);
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
/* We don't support mp4 without the mp4v2 library */
Track *mp4_get_file_info (gchar *name)
{
    gtkpod_warning (_("Import of '%s' failed: m4a/m4p/m4b not supported without the mp4v2 library. You must compile the gtkpod source together with the mp4v2 library.\n"), name);
    return NULL;
}

gboolean mp4_write_file_info (gchar *filename, Track *track)
{
    gtkpod_warning (_("m4a/m4p/m4b metadata update for '%s' failed: m4a/m4p/m4b not supported without the mp4v2 library. You must compile the gtkpod source together with the mp4v2 library.\n"), filename);
    return FALSE;
}

gboolean mp4_read_soundcheck (gchar *filename, Track *track)
{
    gtkpod_warning (_("m4a/m4p/m4b soundcheck update for '%s' failed: m4a/m4p/m4b not supported without the mp4v2 library. You must compile the gtkpod source together with the mp4v2 library.\n"), filename);
    return FALSE;
}
#endif
