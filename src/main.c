/*
|   Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|   Part of the gtkpod project.
|
|   URL: http://gtkpod.sourceforge.net/
|
|   This program is free software; you can redistribute it and/or modify
|   it under the terms of the GNU General Public License as published by
|   the Free Software Foundation; either version 2 of the License, or
|   (at your option) any later version.
|
|   This program is distributed in the hope that it will be useful,
|   but WITHOUT ANY WARRANTY; without even the implied warranty of
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|   GNU General Public License for more details.
|
|   You should have received a copy of the GNU General Public License
|   along with this program; if not, write to the Free Software
|   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/
/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <time.h>

#include "clientserver.h"
#include "display.h"
#include "file.h"
#include "interface.h"
#include "misc.h"
#include "playlist.h"
#include "prefs.h"
#include "support.h"

int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

#ifdef G_THREADS_ENABLED
  /* this must be called before gtk_init () */
  g_thread_init (NULL);
  /* FIXME: this call causes gtkpod to freeze as soon as tracks should be
     displayed */
/*   gdk_threads_init (); */
#endif

  gtk_init (&argc, &argv);

  add_pixmap_directory (PKGDATADIR G_DIR_SEPARATOR_S "pixmaps");

  srand(time(NULL));

  gtkpod_window = create_gtkpod ();
  if (!read_prefs (gtkpod_window, argc, argv)) return 0;

  display_create (gtkpod_window);
  create_mpl ();     /* needs at least the master playlist */

  /* stuff to be done before starting gtkpod */
  call_script ("gtkpod.in");

  gtk_widget_show (gtkpod_window);

  if(prefs_get_automount())      mount_ipod();
  if(prefs_get_autoimport())     handle_import();

  server_setup ();   /* start server to accept playcount updates */

/*   gdk_threads_enter (); */
  gtk_main ();
/*   gdk_threads_leave (); */

  /* all the cleanup is already done in gtkpod_main_quit() in misc.c */
  return 0;
}
