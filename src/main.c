/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>
#include "gtkpod.h"
#include <glib/gi18n-lib.h>
#ifdef HAVE_CLUTTER_GTK
    #include <clutter-gtk/clutter-gtk.h>
#endif
#ifdef HAVE_GSTREAMER
    #include <gst/gst.h>
#endif
#ifdef HAVE_BRASERO
    #include <brasero-media.h>
#endif

int
main (int argc, char *argv[])
{
    GOptionContext *ctx;
    GError *error = NULL;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

#ifdef G_THREADS_ENABLED

#if GLIB_CHECK_VERSION(2,31,0)
    /* No longer need to init threads manually anymore */
#else
    /* this must be called before gtk_init () */
    g_thread_init (NULL);
#endif

    /* FIXME: this call causes gtkpod to freeze as soon as tracks should be
       displayed */
    gdk_threads_init ();
#endif

#ifdef HAVE_CLUTTER_GTK
    gtk_clutter_init(&argc, &argv);
    clutter_threads_init();
#else
    gtk_init (&argc, &argv);
#endif

    ctx = g_option_context_new (_("- Interface with your iPod"));
    g_option_context_add_group (ctx, gtk_get_option_group (TRUE));

#ifdef HAVE_GSTREAMER
    g_option_context_add_group (ctx, gst_init_get_option_group ());
#endif

#ifdef HAVE_BRASERO
    g_option_context_add_group (ctx, brasero_media_get_option_group ());
#endif

    g_option_context_set_ignore_unknown_options (ctx, TRUE);
    g_option_context_parse (ctx, &argc, &argv, &error);
    if (error != NULL) {
        g_printerr ("Error parsing options: %s\n", error->message);
        g_error_free(error);
        exit(1);
    }
    g_option_context_free (ctx);

    g_set_application_name(_("gtkpod"));
    gtk_window_set_auto_startup_notification(FALSE);

    srand(time(NULL));

    gtkpod_init (argc, argv);

#ifdef G_THREADS_ENABLED
    gdk_threads_enter();
#endif
    gtk_main ();
#ifdef G_THREADS_ENABLED
    gdk_threads_leave();
#endif

    return 0;
}
