/*
|  Copyright (C) 2002-2009 Paul Richardson <phantom_sf at users sourceforge net>
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

#ifndef GP_H_
#define GP_H_

#include <gtk/gtk.h>

#define GTKPOD_APP_TYPE                (gtkpod_app_get_type ())
#define GTKPOD_APP(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTKPOD_APP_TYPE, GtkPodApp))
#define GTKPOD_IS_APP(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTKPOD_APP_TYPE))
#define GTKPOD_APP_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GTKPOD_APP_TYPE, GtkPodAppInterface))

typedef struct _GtkPodApp GtkPodApp; /* dummy object */
typedef struct _GtkPodAppInterface GtkPodAppInterface;

struct _GtkPodAppInterface {
  GTypeInterface g_iface;

  void (*statusbar_message) (GtkPodApp *obj, gchar* message, ...);
};

GType gtkpod_app_get_type (void);

void gtkpod_app_statusbar_message (GtkPodApp *obj, gchar* message, ...);


/* full path to 'gtkpod.glade' */
gchar *gtkpod_xml_file;
GtkPodApp *gtkpod_app;

/* Functions relating to the instance of gtkpod_app */
void gp_init (GtkPodApp *window, int argc, char *argv[]);

#endif /* GP_H_ */
