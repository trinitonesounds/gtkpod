/*
|  Copyright (C) 2007 Matvey Kozhev <sikon at users sourceforge net>
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
#include "misc.h"
#include "help.h"

void setup_prefs_dlg (GladeXML *xml, GtkWidget *dlg)
{
	GtkWidget *toolbar_style_combo = gtkpod_xml_get_widget (xml, "toolbar_style");
	GtkWidget *target_format_combo = gtkpod_xml_get_widget (xml, "target_format");

	gtk_combo_box_set_active (GTK_COMBO_BOX(toolbar_style_combo), 0);
	gtk_combo_box_set_active (GTK_COMBO_BOX(target_format_combo), 0);
}

void open_prefs_dlg ()
{
	GladeXML *xml = glade_xml_new (xml_file, "prefs_dialog", NULL);
	glade_xml_signal_autoconnect (xml);
	GtkWidget *dlg = gtkpod_xml_get_widget (xml, "prefs_dialog");
	
	setup_prefs_dlg(xml, dlg);
	
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

void on_prefs_dialog_help ()
{
	gtkpod_open_help_context ("gtkpod");
}
