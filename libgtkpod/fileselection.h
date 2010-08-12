/* Time-stamp: <2007-06-25 00:53:20 jcs>
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

/***************************************************************************
 *            fileselection.h
 *	Contains callback functions for file chooser dialogs
 *
 *  Fri May 27 13:36:16 2005
 *  Copyright  2005  James Liggett
 *  Email jrliggett@cox.net
 ****************************************************************************/

#ifndef _FILESELECTION_H
#define _FILESELECTION_H

#include "itdb.h"

void create_add_files_dialog (Playlist *pl);
void create_add_playlists_dialog (iTunesDB *itdb);
gchar *fileselection_get_cover_filename(void);
gchar *fileselection_get_file_or_dir (const gchar *title,
				      const gchar *cur_file,
				      GtkFileChooserAction action);
gchar *fileselection_select_script (const gchar *opath,
				    const gchar *fallback,
				    const gchar *title,
				    const gchar *additional_text);
GSList* fileselection_get_files(const gchar *title);

/* dirbrowser */
void dirbrowser_create (Playlist *pl);
#endif
