/* Time-stamp: <2003-11-04 00:30:00 jcs>
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

#ifdef HAVE_LIBMP4V2
/* Use mp4v2 from the mpeg4ip project to handle mp4 (m4a, m4p) files
   (http://mpeg4ip.sourceforge.net/) */
#include "mp4.h"

Track *file_get_mp4_info (gchar *mp4FileName)
{
    char* info = MP4FileInfo(mp4FileName, MP4_INVALID_TRACK_ID);
    MP4FileHandle mp4file;

    printf("%s:\n", mp4FileName);
    if (!info) {
	fprintf(stderr, "can't open %s\n", mp4FileName);
	return NULL;
    }

    fputs(info, stdout);
    mp4file = MP4Read(mp4FileName, 0);
    if (mp4file != MP4_INVALID_FILE_HANDLE) {
	char *value;
	uint16_t numvalue, numvalue2;
	if (MP4GetMetadataName(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Name: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataArtist(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Artist: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataWriter(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Writer: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataComment(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Comment: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataTool(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Tool: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataYear(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Year: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataAlbum(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Album: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataTrack(mp4file, &numvalue, &numvalue2)) {
	    fprintf(stdout, "Metadata track: %u of %u\n", numvalue,
		    numvalue2);
	}
	if (MP4GetMetadataDisk(mp4file, &numvalue, &numvalue2)) {
	    fprintf(stdout, "Metadata Disk: %u of %u\n", numvalue,
		    numvalue2);
	}
	if (MP4GetMetadataGenre(mp4file, &value) && value != NULL) {
	    fprintf(stdout, "Metadata Genre: %s\n", value);
	    free(value);
	}
	if (MP4GetMetadataTempo(mp4file, &numvalue)) {
	    fprintf(stdout, "Metadata Tempo: %u\n", numvalue);
	}
	MP4Close(mp4file);
    }
    free(info);

    return NULL;
}
#else
/* Use our own code to read some information from mp4 files */
Track *file_get_mp4_info (gchar *name)
{
    return NULL;
}
#endif
