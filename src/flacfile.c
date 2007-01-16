#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "charset.h"
#include "itdb.h"
#include "misc.h"
#include "flacfile.h"

/* Info on how to implement new file formats: see mp3file.c for more info */

#ifdef HAVE_FLAC

#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <FLAC/metadata.h>
#include "prefs.h"

Track *flac_get_file_info (gchar *flacFileName)
{
    Track *track = NULL;
    FLAC__StreamMetadata stream_data;
    FLAC__StreamMetadata *tags;

    if(!FLAC__metadata_get_streaminfo (flacFileName, &stream_data))
    {
        gchar *filename = NULL;
        filename = charset_to_utf8 (flacFileName);

	gtkpod_warning (_("'%s' does not appear to be an FLAC audio file.\n"),
	                filename);
        g_free (filename);
    }
    else
    {
        ExtraTrackData *etr = NULL;

        track = gp_track_new ();
	track->description = g_strdup ("FLAC audio file");

        track->bitrate = stream_data.data.stream_info.bits_per_sample/1000;
        track->samplerate = stream_data.data.stream_info.sample_rate;
        track->tracklen = stream_data.data.stream_info.total_samples / (stream_data.data.stream_info.sample_rate / 1000);

        if (prefs_get_int("readtags")) 
        {
            if (!FLAC__metadata_get_tags (flacFileName, &tags)) 
            {
                gchar *filename = NULL;
                filename = charset_to_utf8 (flacFileName);
                gtkpod_warning (_("Error retrieving tags for '%s'.\n"),
	                        filename);
                g_free (filename);
                /* FIXME: should NULL be returned if no tags? */
            }
            else {
                gint i;

                for (i = 0 ; i < tags->data.vorbis_comment.num_comments ; i++) 
                {
                    gchar *tag = (gchar*)tags->data.vorbis_comment.comments[i].entry;

                    if (g_ascii_strncasecmp("ARTIST=", tag, 7) == 0) {
                        track->artist = charset_to_utf8 (tag + 7);
                    }
                    if (g_ascii_strncasecmp("ALBUM=", tag, 6) == 0) {
                        track->album = charset_to_utf8 (tag + 6);
                    }
                    if (g_ascii_strncasecmp("TITLE=", tag, 6) == 0) {
                        track->title = charset_to_utf8 (tag + 6);
                    }
                    if (g_ascii_strncasecmp("GENRE=", tag, 6) == 0) {
                        track->genre = charset_to_utf8 (tag + 6);
                    }
                    if (g_ascii_strncasecmp("YEAR=", tag, 5) == 0) {
                        track->year = atoi (tag + 5);
                    }
                    if (g_ascii_strncasecmp("TRACKNUMBER=", tag, 12) == 0) {
                        track->track_nr = atoi (tag + 12);
                    }
                    if (g_ascii_strncasecmp("COMPOSER=", tag, 9) == 0) {
                        track->composer = charset_to_utf8 (tag + 9);
                    }
                    if (g_ascii_strncasecmp("COMMENT=", tag, 8) == 0) {
                        track->comment = charset_to_utf8 (tag + 8);
                    }
                    if (g_ascii_strncasecmp("TRACKS=", tag, 7) == 0) {
                        track->tracks = atoi (tag  + 7);
                    }
                    if (g_ascii_strncasecmp("CNDR=", tag, 5) == 0) {
                        track->cd_nr = atoi (tag + 5);
                    }
                    if (g_ascii_strncasecmp("CDS=", tag, 4) == 0) {
                        track->cds = atoi (tag  + 4);
                    }
                }
            }

            FLAC__metadata_object_delete (tags);
	}
        etr = track->userdata;
        etr->conv = g_new0(TrackConv, 1);
        etr->conv->type = FILE_TYPE_FLAC; /* FIXME: is it useful? */
        /* TODO: immediate conversion if thread ? */
    }

    return track;
}

gboolean flac_write_file_info (gchar *flacFileName, Track *track)
{
    gboolean result = FALSE;
    gtkpod_warning ("Not supported yet\n"); /* FIXME: */
    return result;
}

#else
/* We don't support FLAC without the FLAC library */
Track *flac_get_file_info (gchar *name)
{
    gtkpod_warning (_("Import of '%s' failed: FLAC not supported without the FLAC library. You must compile the gtkpod source together with the FLAC library.\n"), name);
    return NULL;
}

gboolean flac_write_file_info (gchar *filename, Track *track)
{
    gtkpod_warning (_("FLAC metadata update for '%s' failed: FLAC not supported without the FLAC library. You must compile the gtkpod source together with the FLAC library.\n"), filename);
    return FALSE;
}
#endif
