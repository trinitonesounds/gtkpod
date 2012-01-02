/* 
 * Copyright (C) 2003-2005 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-about.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sound-juicer.h"

#include <gtk/gtk.h>
#include <string.h>
#include "sj-about.h"

G_MODULE_EXPORT void on_about_activate (void)
{
  char *license_trans;
    
  const gchar *authors[] = {
    "Ross Burton <ross@burtonini.com>",
    "And many others who have contributed patches.",
    NULL
  };
  const gchar *documentors[] = {
    "Shaun McCance <shaunm@gnome.org>",
    "Mike Hearn <mike@theoretic.com>",
    NULL
  };
  const gchar *artists[] = {
    "Lapo Calamandrei <l.calamandrei@neri.it>",
    NULL
  };
  const char *license[] = {
    N_("Sound Juicer is free software; you can redistribute it and/or modify "
       "it under the terms of the GNU General Public License as published by "
       "the Free Software Foundation; either version 2 of the License, or "
       "(at your option) any later version."),
    N_("Sound Juicer is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
       "GNU General Public License for more details."),
    N_("You should have received a copy of the GNU General Public License "
       "along with Sound Juicer; if not, write to the Free Software Foundation, Inc., "
       "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA")
  };

  license_trans = g_strconcat (_(license[0]), "\n\n",
                               _(license[1]), "\n\n",
                               _(license[2]), "\n\n",
                               NULL);
  
  gtk_show_about_dialog (GTK_WINDOW (main_window),
                         "comments", _("An Audio CD Extractor"),
                         "version", VERSION,
                         "copyright", "Copyright \xc2\xa9 2003-2008 Ross Burton",
                         "authors", authors,
                         "documenters", documentors,
                         "artists", artists,
                         /*
                          * Note to translators: put here your name and email so it will show
                          * up in the "about" box
                          */
                         "translator-credits", _("translator-credits"),
                         "logo-icon-name", "sound-juicer",
                         "license", license_trans,
                         "wrap-license", TRUE,
                         "website", "http://burtonini.com/blog/computers/sound-juicer",
                         NULL);
  
  g_free (license_trans);
}
