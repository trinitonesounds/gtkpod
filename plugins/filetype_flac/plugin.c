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
 |  $Id$
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include "libgtkpod/gtkpod_app_iface.h"
#include "libgtkpod/filetype_iface.h"
#include "plugin.h"
#include "flacfile.h"

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static gboolean activate_plugin(AnjutaPlugin *plugin) {
    FlacFileTypePlugin *flac_filetype_plugin;

    flac_filetype_plugin = (FlacFileTypePlugin*) plugin;
    g_return_val_if_fail(FILE_IS_TYPE(flac_filetype_plugin), TRUE);

    gtkpod_register_filetype(FILE_TYPE(flac_filetype_plugin));

    return TRUE; /* FALSE if activation failed */
}

static gboolean deactivate_plugin(AnjutaPlugin *plugin) {
    FlacFileTypePlugin *flac_filetype_plugin;

    flac_filetype_plugin = (FlacFileTypePlugin*) plugin;
    gtkpod_unregister_filetype(FILE_TYPE(flac_filetype_plugin));

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void flac_filetype_plugin_instance_init(GObject *obj) {
//    FlacFileTypePlugin *plugin = (FlacFileTypePlugin*) obj;
}

static void flac_filetype_plugin_class_init(GObjectClass *klass) {
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent(klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

static void flac_filetype_iface_init(FileTypeInterface *iface) {
    iface->category = AUDIO;
    iface->description = _("Flac audio file type");
    iface->name = "flac";
    iface->suffixes = g_list_append(iface->suffixes, "flac");

    iface->get_file_info = flac_get_file_info;
    iface->write_file_info = filetype_no_write_file_info; /* FIXME */
    iface->read_soundcheck = filetype_no_soundcheck; /* FIXME */
    iface->read_lyrics = filetype_no_read_lyrics; /* FIXME */
    iface->write_lyrics = filetype_no_write_lyrics; /* FIXME */
    iface->read_gapless = filetype_no_read_gapless; /* FIXME ?? */
    iface->can_convert = flac_can_convert;
    iface->get_conversion_cmd = flac_get_conversion_cmd;
    iface->get_gain_cmd = filetype_no_gain_cmd;
}

ANJUTA_PLUGIN_BEGIN (FlacFileTypePlugin, flac_filetype_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(flac_filetype, FILE_TYPE_TYPE);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (FlacFileTypePlugin, flac_filetype_plugin)
;
