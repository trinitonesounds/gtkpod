/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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

#ifndef MISC_PLAYLIST_H_
#define MISC_PLAYLIST_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "itdb.h"
#include "gp_itdb.h"
#include "misc_conversion.h"

Playlist *add_new_pl_user_name(iTunesDB *itdb, gchar *dflt, gint32 position);
void add_new_pl_or_spl_user_name(iTunesDB *itdb, gchar *dflt, gint32 position);
Playlist *generate_random_playlist(iTunesDB *itdb);
Playlist *generate_selected_playlist(void);
Playlist *generate_displayed_playlist(void);
void generate_category_playlists(iTunesDB *itdb, T_item cat);
void each_rating_pl(iTunesDB *itdb);
void most_rated_pl(iTunesDB *itdb);
void most_listened_pl(iTunesDB *itdb);
void last_listened_pl(iTunesDB *itdb);
void since_last_pl(iTunesDB *itdb);
void never_listened_pl(iTunesDB *itdb);
Playlist *generate_not_listed_playlist(iTunesDB *itdb);
void delete_playlist_head (DeleteAction deleteaction);


#endif /* MISC_PLAYLIST_H_ */
