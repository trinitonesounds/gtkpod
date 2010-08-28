/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libgtkpod/misc.h"
#include "libgtkpod/prefs.h"
#include "libgtkpod/directories.h"
#include "plugin.h"
#include "display_sorttabs.h"

G_MODULE_EXPORT void on_filter_tabs_count_value_changed (GtkSpinButton *sender, gpointer e)
{
    gint num = gtk_spin_button_get_value_as_int (sender);

    /* Update the number of filter tabs */
    prefs_set_int ("sort_tab_num", num);
    st_show_visible();
}

/*
    glade callback
*/
G_MODULE_EXPORT void on_group_compilations_toggled (GtkToggleButton *sender, gpointer e)
{
    gboolean active = gtk_toggle_button_get_active (sender);

    prefs_set_int ("group_compilations", active);
    st_show_visible();
}

GtkWidget *init_sorttab_preferences() {
    GtkWidget *notebook;
    GladeXML *pref_xml;

    gchar *glade_path = g_build_filename(get_glade_dir(), "sorttab_display.glade", NULL);
    pref_xml = gtkpod_xml_new(glade_path, "sorttab_settings_notebook");
    notebook = gtkpod_xml_get_widget(pref_xml, "sorttab_settings_notebook");
    g_object_ref(notebook);
    g_free(glade_path);

    glade_xml_signal_autoconnect(pref_xml);

    return notebook;
}
