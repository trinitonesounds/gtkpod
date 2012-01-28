/*
 |  Copyright (C) 2007 Jorg Schuler <jcsjcs at users sourceforge net>
 |  Part of the gtkpod project.
 |
 |  URL: http://www.gtkpod.org/
 |  URL: http://gtkpod.sourceforge.net/
 |
 |  Gtkpod is free software; you can redistribute it and/or modify
 |  it under the terms of the GNU General Public License as published by
 |  the Free Software Foundation; either version 2 of the License, or
 |  (at your option) any later version.
 |
 |  Gtkpod is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 |
 |  You should have received a copy of the GNU General Public License
 |  along with gtkpod; if not, write to the Free Software
 |  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
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

#include "libgtkpod/charset.h"
#include "libgtkpod/gp_itdb.h"
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "m4afile.h"
#include "mp4file.h"

/* Info on how to implement new file formats: see mp3file.c for more info */

Track *m4a_get_file_info(const gchar *m4aFileName, GError **error) {
    return mp4_get_file_info(m4aFileName, error);
}

gboolean m4a_write_file_info(const gchar *filename, Track *track, GError **error) {
    return mp4_write_file_info(filename, track, error);
}

gboolean m4a_read_soundcheck(const gchar *filename, Track *track, GError **error) {
    return mp4_read_soundcheck(filename, track, error);
}

gboolean m4a_can_convert() {
    gchar *cmd = m4a_get_conversion_cmd();
    /*
     * Return TRUE if
     * Command exists and fully formed
     * Target format is NOT set to AAC
     * convert_m4a preference is set to TRUE
     */
    return cmd && cmd[0] && (prefs_get_int("conversion_target_format") != TARGET_FORMAT_AAC)
            && prefs_get_int("convert_m4a");
}

gchar *m4a_get_conversion_cmd() {
    /* Convert an m4a to an mp3 */
    return prefs_get_string("path_conv_mp3");
}

gchar *m4a_get_gain_cmd() {
    return prefs_get_string("path_aacgain");
}
