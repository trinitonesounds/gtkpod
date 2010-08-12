/*
|  Copyright (C) 2007 P.G. Richardson <phantom_sf at users.sourceforge.net>
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

#ifndef __FETCHCOVER_H__
#define __FETCHCOVER_H__

#include <string.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "libgtkpod/gp_private.h"
#include "libgtkpod/itdb.h"
#include "libgtkpod/prefs.h"

typedef struct
{
	GdkPixbuf *image;
	GString *url;
	gchar *dir;
	gchar *filename;
	GList *tracks;
	gchar *err_msg;
} Fetch_Cover;

Fetch_Cover *fetchcover_new (gchar *url_path, GList *trks);
gboolean fetchcover_net_retrieve_image (Fetch_Cover *fetch_cover);
gboolean fetchcover_select_filename (Fetch_Cover *fetch_cover);
void free_fetchcover (Fetch_Cover *fcover);

#endif
