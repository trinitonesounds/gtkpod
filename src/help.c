/*
|  Copyright (C) 2007 Maia Kozheva <sikon at users sourceforge net>
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include "help.h"
#include "misc.h"

/*------------------------------------------------------------------*\
 *                                                                  *
 *             About Window                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function, make sure you define a
   separate callback for gtkpod.glade */
G_MODULE_EXPORT void open_about_window ()
{
	const gchar *authors[] = {
		_("© 2002 - 2010\n"
		    "Jorg Schuler (jcsjcs at users dot sourceforge dot net)\n"
		    "Corey Donohoe (atmos at atmos dot org)\n"
		    ""),
		_("Codebase includes contribution from the following people and many other helpful individuals\n"),
		_("Ramesh Dharan: Multi-Edit (edit tags of several tracks in one run)"),
		_("Hiroshi Kawashima: Japanese charset autodetection feature"),
		_("Adrian Ulrich: porting of playlist code from mktunes.pl to itunesdb.c"),
		_("Walter Bell: correct handling of DND URIs with escaped characters and/or cr/newlines at the end"),
		_("Sam Clegg: user defined filenames when exporting tracks from the iPod"),
		_("Chris Cutler: automatic creation of various playlist types"),
		_("Graeme Wilford: reading and writing of the 'Composer' ID3 tags, progress dialogue during sync"),
		_("Edward Matteucci: debugging, special playlist creation, most of the volume normalizing code"),
		_("Jens Lautenbach: some optical improvements"),
		_("Alex Tribble: iPod eject patch"),
		_("Yaroslav Halchenko: Orphaned and dangling tracks handling"),
		_("Andrew Huntwork: Filename case sensitivity fix and various other bugfixes"),
		_("Ero Carrera: Filename validation and quick sync when copying tracks from the iPod"),
		_("Jens Taprogge: Support for LAME's replay gain tag to normalize volume"),
		_("Armando Atienza: Support with external playcounts"),
		_("D.L. Sharp: Support for m4b files (bookmarkable AAC files)"),
		_("Jim Hall: Decent INSTALL file"),
		_("Juergen Helmers, Markus Gaugusch: Conversion scripts to sync calendar/contacts to the iPod"),    /* J"urgen! */
		_("Flavio Stanchina: bugfixes"),
		_("Chris Micacchi: when sorting ignore 'the' and similar at the beginning of the title"),
		_("Steve Jay: use statvfs() instead of df (better portability, faster)"),
		"",
		_("Christoph Kunz: address compatibility issues when writing id3v2.4 type mp3 tags"),
		"",
		_("James Liggett:\n"
		    "replacement of old GTK file selection dialogs with new GTK filechooser dialogs\n"
		    "refactored user preferences system."),
		"",
		_("Daniel Kercher: sync scripts for abook and webcalendar"),
		"",
		_("Clinton Gormley: sync scripts for thunderbird"),
		"",
		_("Sebastien Beridot: sync script for ldif addressbook format"),
		"",
		_("Sebastian Scherer: sync script for kNotes"),
		"",
		_("Nick Piper: sync script for Palm, type-ahead search"),
		"",
		_("Uwe Hermann: help with support for iPod Video"),
		"",
		_("Iain Benson: support for compilation tag in mp3 files and separate display of compilations in the sort tab."),
		_("Nicolas Chariot: icons of buttons\n"),
		_("Others past and present for their excellent contributions\n\n"),
		_("This program borrows code from the following projects:"),
		_("gnutools: (mktunes.pl, ported to C) reading and writing of iTunesDB  (http://www.gnu.org/software/gnupod/)"),
		_("iPod.cpp, iPod.h by Samuel Wood (sam dot wood at gmail dot com): some code for smart playlists is based on his C++-classes."),
		_("mp3info: mp3 playlength detection (http://ibiblio.org/mp3info/)"),
		_("xmms: dirbrowser, mp3 playlength detection (http://www.xmms.org)"),
		"",
		_("The GUI was created with the help of glade (http://glade.gnome.org/)."),
		NULL };

	gchar  *translators[] = {
		_("French: David Le Brun (david at dyn-ns dot net)"),
		_("French: Éric Lassauge (rpmfarm at free dot fr)"),
		_("German: Jorg Schuler (jcsjcs at users dot sourceforge dot net)"),
		_("German: Kai-Ove"),
		_("Hebrew: Assaf Gillat (gillata at gmail dot com)"),
		_("Italian: Edward Matteucci (edward_matteucc at users dot sourceforge dot net)"),
		_("Italian: Daniele Forsi (dforsi at gmail dot com)"),
		_("Japanese: Ayako Sano"),
		_("Japanese: Kentaro Fukuchi (fukuchi at users dot sourceforge dot net)"),
		_("Romanian: Alex Eftimie (alexeftimie at gmail dot com)"),
		_("Spanish: Alejandro Lamas Daviña (alejandro.lamas at ific dot uv dot es)"),
		_("Swedish: Stefan Asserhall (stefan.asserhall at comhem dot se)"),
		NULL
	};

	gchar *license = _(
		"This program is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU General Public License as\n"
		"published by the Free Software Foundation; either version 2 of the\n"
		"License, or (at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful, but\n"
		"WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See\n"
		"the GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public\n"
		"License along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA\n02111-1307, USA."
	);

	gchar *copyright = _("© 2002-2007\nJorg Schuler <jcsjcs at users.sourceforge.net>\nCorey Donohoe <atmos at atmos.org>");
	gchar *translator_credits = g_strjoinv("\n", translators);
	GdkPixbuf *icon = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "gtkpod-logo.png", NULL);

#ifdef LIBGPOD_VERSION
	gchar *version = g_strdup_printf (_("(using libgpod %s)"), LIBGPOD_VERSION);
	gchar *description = g_strdup_printf ("%s\n%s", _("Cross-platform multilingual interface to Apple's iPod™"),
											version);

	g_free (version);
#else
	gchar *description = _("Cross-platform multilingual interface to Apple's iPod™");
#endif

	gtk_show_about_dialog(GTK_WINDOW(gtkpod_window),
		"name", "gtkpod",
		"version", VERSION,
		"logo", icon,
		"comments", description,
		"copyright", copyright,
		"website", "http://gtkpod.org",
		"license", license,
		"authors", authors,
		"translator_credits", translator_credits,
		NULL);

	g_free (translator_credits);

#ifdef LIBGPOD_VERSION
	g_free (description);
#endif

	if(icon)
		g_object_unref(icon);
}


/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function make sure you define a
   separate callback for gtkpod.glade */
G_MODULE_EXPORT void open_help_contents ()
{
	gtkpod_open_help_context("gtkpod");
}

/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function, make sure you define a
   separate callback for gtkpod.glade */
G_MODULE_EXPORT void open_help_online ()
{
	gtkpod_open_in_browser("http://www.gtkpod.org/contact.html");
}

/* ATTENTION: directly used as callback in gtkpod.glade -- if you
   change the arguments of this function, make sure you define a
   separate callback for gtkpod.glade */
G_MODULE_EXPORT void open_help_reportbug ()
{
	gtkpod_open_in_browser("http://sourceforge.net/tracker/?group_id=67873&atid=519273");
}

void gtkpod_open_in_browser (const gchar *url)
{
	const gchar *xdg_open_argv[]   = { "xdg-open", url, NULL };
	const gchar *gnome_open_argv[] = { "gnome-open", url, NULL };
	const gchar *kfmclient_argv[]  = { "kfmclient", "exec", url, NULL };

	if(g_spawn_async(NULL, (gchar **)xdg_open_argv, NULL, G_SPAWN_SEARCH_PATH,
					 NULL, NULL, NULL, NULL))
	{
		return;
	}

	if(g_spawn_async(NULL, (gchar **)gnome_open_argv, NULL, G_SPAWN_SEARCH_PATH,
					 NULL, NULL, NULL, NULL))
	{
		return;
	}

	g_spawn_async(NULL, (gchar **)kfmclient_argv, NULL, G_SPAWN_SEARCH_PATH,
					 NULL, NULL, NULL, NULL);
}

void gtkpod_open_help_context (const gchar *context)
{
	const gchar *docdir =
		PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "doc" G_DIR_SEPARATOR_S;

	gchar *filename = g_strdup_printf("%sgtkpod.xml#%s", docdir, context);
	const gchar *yelp_open_argv[] = { "yelp", filename, NULL };

	if(g_spawn_async(NULL, (gchar **)yelp_open_argv, NULL, G_SPAWN_SEARCH_PATH,
					 NULL, NULL, NULL, NULL))
	{
		g_free(filename);
		return;
	}

	g_free(filename);
	filename = g_strdup_printf("%s%s.html", docdir, context);
	gtkpod_open_in_browser(filename);
	g_free(filename);
}
