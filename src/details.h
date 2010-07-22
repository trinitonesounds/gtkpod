/* Time-stamp: <2008-11-08 17:35:16 jcs>
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

#ifndef __DETAILS_H__
#define __DETAILS_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libgtkpod/itdb.h"

#define DETAILS_LYRICS_NOTEBOOK_PAGE 3

struct _Detail
{
    GladeXML *xml;      /* XML info                           */
    GtkWidget *window;  /* pointer to details window          */
    iTunesDB *itdb;     /* pointer to the original itdb       */
    GList *orig_tracks; /* tracks displayed in details window */
    GList *tracks;      /* tracks displayed in details window */
    Track *track;       /* currently displayed track          */
    gboolean artwork_ok;/* artwork can be displayed or not    */
    gboolean changed;   /* at least one track was changed     */

};

typedef struct _Detail Detail;

/* details window */
void details_edit (GList *selected_tracks);
void details_remove_track (Track *track);
Detail *details_get_selected_detail ();
void destroy_details_editor();

void lyrics_edit (GList *selected_tracks);

void details_editor_track_removed_cb(GtkPodApp *app, gpointer tk, gpointer data);
void details_editor_set_tracks_cb(GtkPodApp *app, gpointer tks, gpointer data);

#endif
