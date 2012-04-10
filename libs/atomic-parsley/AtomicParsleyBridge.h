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

/**
 * Open and scan the metadata of the m4a/mp4/... file
 * and populate the given track.
 */
#ifdef __cplusplus
extern "C"
#endif
void AP_extract_metadata(const char *filePath, Track *track);

/**
 * Using the given track, set the metadata of the target
 * file
 */
#ifdef __cplusplus
extern "C"
#endif
void AP_write_metadata(Track *track, const char *filePath, GError **error);

/**
 * Read any lyrics from the given file
 */
#ifdef __cplusplus
extern "C"
#endif
char *AP_read_lyrics(const char *filePath, GError **error);

/**
 * Write lyrics to the file at filePath
 */
#ifdef __cplusplus
extern "C"
#endif
void AP_write_lyrics(const char *lyrics, const char *filePath, GError **error);

#endif /* ATOMIC_PARSLEY_BRIDGE_H_ */
