/* Time-stamp: <2007-01-16 14:08:05 jcs>
|
|  Copyright (C) 2007 Marc d[r]eadlock <m.dreadlock at gmail com>
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

#include "charset.h"
#include "itdb.h"
#include "misc.h"
#include "oggfile.h"

/* Info on how to implement new file formats: see mp3file.c for more info */

#ifdef HAVE_LIBVORBISFILE
/* Ogg/Vorbis library (www.xiph.org) */
#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "vorbis/vorbisfile.h"
#include "prefs.h"

Track *ogg_get_file_info (gchar *oggFileName)
{
    Track *track = NULL;
    FILE *file= NULL;

    file=fopen(oggFileName, "rb");
    
    if (file == NULL)
    {
	gchar *filename = charset_to_utf8 (oggFileName);
	gtkpod_warning (
	    _("Could not open '%s' for reading.\n"),
	    filename);
	g_free (filename);
    }
    else
    {
        OggVorbis_File oggFile;
        if (ov_open(file, &oggFile, NULL , 0)!=0)
        {
	    gchar *filename=NULL;
            filename= charset_to_utf8 (oggFileName);
	    gtkpod_warning (_("'%s' does not appear to be an ogg audio file.\n"),
			    filename);
	    g_free (filename);
	    fclose(file);
	}
        else
        {
            TrackConv *aConv=NULL;
            ExtraTrackData *etr=NULL;
	    track = gp_track_new ();
	    track->description = g_strdup ("OGG audio file");
            vorbis_info *vi=ov_info(&oggFile,-1);
            /*FIXME: if (!vi) */
            track->bitrate=vi->bitrate_nominal/1000;
            track->samplerate=vi->rate;
            track->tracklen=(ov_time_total(&oggFile,-1))*1000; /* in seconds */
	    if (prefs_get_int("readtags"))
	    {
                vorbis_comment *vc=ov_comment(&oggFile,-1);
                if (vc) {
                    char *str;
                    if ((str=vorbis_comment_query(vc,"artist",0))!=NULL){
                        track->artist=charset_to_utf8(str);
                    }
                    if ((str=vorbis_comment_query(vc,"album",0))!=NULL){
                        track->album=charset_to_utf8(str);
                    }
                    if ((str=vorbis_comment_query(vc,"title",0))!=NULL){
                        track->title=charset_to_utf8(str);
                    }
                    if ((str=vorbis_comment_query(vc,"genre",0))!=NULL){
                        track->genre=charset_to_utf8(str);
                    }
                    if ((str=vorbis_comment_query(vc,"year",0))!=NULL){
                        track->year=atoi(str);
                    }
                    if ((str=vorbis_comment_query(vc,"tracknumber",0))!=NULL){
		        track->track_nr = atoi(str);
                    }
                    if ((str=vorbis_comment_query(vc,"composer",0))!=NULL){
		        track->composer = charset_to_utf8(str);
                    }
                    if ((str=vorbis_comment_query(vc,"comment",0))!=NULL){
		        track->comment = charset_to_utf8(str);
                    }
                    if ((str=vorbis_comment_query(vc,"tracks",0))!=NULL){
		        track->tracks = atoi(str);
                    }
                    if ((str=vorbis_comment_query(vc,"cdnr",0))!=NULL){
		        track->cd_nr = atoi(str);
                    }
                    if ((str=vorbis_comment_query(vc,"cds",0))!=NULL){
		        track->cds = atoi(str);
                    }
                }
                    
            }
            ov_clear(&oggFile); /* performs the fclose(file); */
            etr=track->userdata;
            aConv=g_new0(TrackConv, 1);
            etr->conv=aConv;
            aConv->type=FILE_TYPE_OGG;/* track->type; FIXME: is it usefull ?*/
            /* TODO: immediate conversion if thread ? */
	}
    }
    
    return track;
}

gboolean ogg_write_file_info (gchar *oggFileName, Track *track)
{
    gboolean result=FALSE;
    /*FIXME: seems to be not easy with common API. all other projects 
     * are using vcedit.ch from vorbis-tools (vorbiscomment). Maybe 
     * using a library could help. LibTag looks good... */
    printf("Not supported yet\n");
    return result;
}

#else
/* We don't support ogg without the vorbisfile library */
Track *ogg_get_file_info (gchar *name)
{
    gtkpod_warning (_("Import of '%s' failed: ogg not supported without the ogg library. You must compile the gtkpod source together with the ogg library.\n"), name);
    return NULL;
}

gboolean ogg_write_file_info (gchar *filename, Track *track)
{
    gtkpod_warning (_("ogg metadata update for '%s' failed: ogg not supported without the ogg library. You must compile the gtkpod source together with the ogg library.\n"), filename);
    return FALSE;
}
#endif
