/*
 |Copyright (C) 2007 P.G. Richardson <phantom_sf at users.sourceforge.net>
 |Part of the gtkpod project.
 | 
 |URL: http://www.gtkpod.org/
 |URL: http://gtkpod.sourceforge.net/
 |
 |This program is free software; you can redistribute it and/or modify
 |it under the terms of the GNU General Public License as published by
 |the Free Software Foundation; either version 2 of the License, or
 |(at your option) any later version.
 |
 |This program is distributed in the hope that it will be useful,
 |but WITHOUT ANY WARRANTY; without even the implied warranty of
 |MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 |GNU General Public License for more details.
 |
 |You should have received a copy of the GNU General Public License
 |along with this program; if not, write to the Free Software
 |Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 |
 |iTunes and iPod are trademarks of Apple
 |
 |This product is not supported/written/published by Apple!
 |
 */

#include "stock_icons.h"

static void register_stock_icon (const gchar *name, const gchar *stockid)
{
	GtkIconSet *pl_iconset;
	GtkIconSource *source;

	g_return_if_fail (name);
	g_return_if_fail (stockid);
	
	pl_iconset = gtk_icon_set_new ();
	source = gtk_icon_source_new ();
			
	gtk_icon_source_set_icon_name (source, name);
	gtk_icon_set_add_source (pl_iconset, source);
	
	GtkIconFactory *factory = gtk_icon_factory_new ();
	gtk_icon_factory_add (factory, stockid, pl_iconset);
	
	gtk_icon_factory_add_default (factory);
}

/**
 * stockid_init
 *
 * Initialises paths used for gtkpod specific icons.
 * This needs to be loaded early as it uses the path
 * of the binary to determine where to load the file from, in the
 * same way as main() determines where to load the glade file
 * from.
 *
 * @progpath: path of the gtkpod binary being loaded.
 *  
 */
void stockid_init (gchar *progpath)
{
	gchar *progname = g_find_program_in_path (progpath);
	static const gchar *SEPsrcSEPgtkpod = G_DIR_SEPARATOR_S "src" G_DIR_SEPARATOR_S "gtkpod";
	gchar *path;

	if (!progname)
		return;
		
	if (!g_path_is_absolute (progname))
	{
		gchar *cur_dir = g_get_current_dir ();
		gchar *prog_absolute;

		if (g_str_has_prefix (progname, "." G_DIR_SEPARATOR_S))
			prog_absolute = g_build_filename (cur_dir, progname+2, NULL);
		else
			prog_absolute = g_build_filename (cur_dir, progname, NULL);
		
		g_free (progname);
		g_free (cur_dir);
		progname = prog_absolute;
	}
	
	if (g_str_has_suffix (progname, SEPsrcSEPgtkpod))
	{
		gchar *suffix = g_strrstr (progname, SEPsrcSEPgtkpod);
		
		if (suffix)
		{
			*suffix = 0;
		}
	}
	
	path = g_build_filename (progname, "data", "icons", NULL);
	g_free (progname);
		
	if (path && !g_file_test (path, G_FILE_TEST_EXISTS))
	{
		g_free (path);
		path = NULL;
	}
	
	if (!path)
		path = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "icons", NULL);

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), path);
	g_free (path);
	
	register_stock_icon ("playlist-photo", GPHOTO_PLAYLIST_ICON_STOCK_ID);
	register_stock_icon ("playlist", TUNES_PLAYLIST_ICON_STOCK_ID);
	
}
