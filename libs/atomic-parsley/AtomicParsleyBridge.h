/*
 |  Copyright (C) 2002-2011 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                             Paul Richardson <phantom_sf at users.sourceforge.net>
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
 */

#ifndef ATOMIC_PARSLEY_BRIDGE_H_
#define ATOMIC_PARSLEY_BRIDGE_H_

#include "libgtkpod/itdb.h"

#ifdef __cplusplus
extern "C"
#endif
void AP_extract_metadata(const char *path, Track *track);



/*
 * Want to use:
 * APar_ExtractTrackDetails
 * APar_ExtractMovieDetails
 * APar_Extract_iods_Info
 */

//char *MP4GetMetadataName(const ca, &value);
//MP4GetMetadataArtist(mp4File, &value) && value
//MP4GetMetadataAlbumArtist(mp4File, &value) && value
//MP4GetMetadataComposer(mp4File, &value) && value != NULL)
//MP4GetMetadataComment(mp4File, &value) && value
//MP4GetMetadataReleaseDate(mp4File, &value) && value != NULL)
//MP4GetMetadataAlbum(mp4File, &value) && value
//MP4GetMetadataTrack(mp4File, &numvalue, &numvalue2))
//MP4GetMetadataDisk(mp4File, &numvalue, &numvalue2))
//MP4GetMetadataGrouping(mp4File, &value) && value
//MP4GetMetadataGenre(mp4File, &value) && value
//MP4GetMetadataBPM(mp4File, &numvalue))

//MP4GetMetadataLyrics(mp4File);
//
//MP4GetMetadataTVShow
//
//MP4TagsFetchFunc( mp4tags, mp4File);
//mp4tags->tvShow)
//track->tvshow = g_strdup(mp4tags->tvShow);
//mp4tags->tvEpisodeID)
//track->tvepisode = g_strdup(mp4tags->tvEpisodeID);
//mp4tags->tvNetwork)
//track->tvnetwork = g_strdup(mp4tags->tvNetwork);
//mp4tags->tvSeason)
//track->season_nr = *mp4tags->tvSeason;
//mp4tags->tvEpisode)
//track->episode_nr = *mp4tags->tvEpisode;
//mp4tags->tvEpisode)
//track->mediatype = mediaTypeTagToMediaType(*mp4tags->mediaType);
//MP4TagsFreeFunc( mp4tags);
//
//MP4GetMetadataCoverArt(mp4File, &image_data, &image_data_len, 0))

#endif /* ATOMIC_PARSLEY_BRIDGE_H_ */
