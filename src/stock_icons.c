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

static void register_stock_icon (gchar *path, const gchar *stockid);

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
	gchar *progname;
	
	progname = g_find_program_in_path (progpath);

	if (progname)
	{
		static const gchar *SEPsrcSEPgtkpod = G_DIR_SEPARATOR_S "src" G_DIR_SEPARATOR_S "gtkpod";
		
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
				GPHOTO_PLAYLIST_ICON_PATH = g_build_filename (progname, "data", "gphoto_playlist_icon-48.png", NULL);
				TUNE_PLAYLIST_ICON_PATH = g_build_filename (progname, "data", "tunes_playlist_icon-48.png", NULL);
			}
		}
		
		g_free (progname);
		
		/* Photo playlist icon */
		if (GPHOTO_PLAYLIST_ICON_PATH && !g_file_test (GPHOTO_PLAYLIST_ICON_PATH, G_FILE_TEST_EXISTS))
		{
			g_free (GPHOTO_PLAYLIST_ICON_PATH);
			GPHOTO_PLAYLIST_ICON_PATH = NULL;
		}
				
		if (!GPHOTO_PLAYLIST_ICON_PATH)
		{
			GPHOTO_PLAYLIST_ICON_PATH = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "gphoto_playlist_icon-48.png", NULL);
		}
		
		/* Normal Playlist icon */
		if (TUNE_PLAYLIST_ICON_PATH && !g_file_test (TUNE_PLAYLIST_ICON_PATH, G_FILE_TEST_EXISTS))
		{
			g_free (TUNE_PLAYLIST_ICON_PATH);
			TUNE_PLAYLIST_ICON_PATH = NULL;
		}
						
		if (!TUNE_PLAYLIST_ICON_PATH)
		{
			TUNE_PLAYLIST_ICON_PATH = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "tunes_playlist_icon-48.png", NULL);
		}
		
		register_stock_icon (GPHOTO_PLAYLIST_ICON_PATH, GPHOTO_PLAYLIST_ICON_STOCK_ID);
		register_stock_icon (TUNE_PLAYLIST_ICON_PATH, TUNES_PLAYLIST_ICON_STOCK_ID);
	}
}

/**
 * register_stock_icons
 *
 * Add pixbuf images to the default icon factory for use
 * as stock items should they be required.
 *  
 */
static void register_stock_icon (gchar *path, const gchar *stockid)
{
	 GError *error = NULL;
	 GdkPixbuf *image;
		
	 g_return_if_fail (path);
	 
	 image = gdk_pixbuf_new_from_file (path, &error);
	  
	 if(error != NULL)
	 {	
		 printf("Error occurred loading photo icon - \nCode: %d\nMessage: %s\n", error->code, error->message); 
		 g_error_free (error);
		 g_return_if_fail (image);
	 }
	 
	 GtkIconSet *pl_iconset = gtk_icon_set_new_from_pixbuf (image);
	 GtkIconFactory *factory = gtk_icon_factory_new ();
	 gtk_icon_factory_add (
			 factory, 
			 stockid,
			 pl_iconset);
	 gtk_icon_factory_add_default (factory);
	 
	 gdk_pixbuf_unref (image);
}
