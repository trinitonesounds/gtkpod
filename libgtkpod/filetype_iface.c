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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include "gp_itdb.h"
#include "gp_private.h"
#include "file.h"
#include "filetype_iface.h"

static void filetype_iface_base_init(FileTypeInterface *klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        klass->name = NULL;
        klass->description = NULL;
        klass->suffixes = NULL;
        klass->get_file_info = NULL;
        klass->write_file_info = NULL;
        klass->read_soundcheck = NULL;
        klass->read_lyrics = NULL;
        klass->write_lyrics = NULL;
        initialized = TRUE;
    }
}

GType filetype_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo info =
            { sizeof(FileTypeInterface), (GBaseInitFunc) filetype_iface_base_init, NULL, NULL, NULL, NULL, 0, 0, NULL };
        type = g_type_register_static(G_TYPE_INTERFACE, "FileType", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
    }
    return type;
}

// Built-in file types
GTKPOD_CORE_FILE_TYPE(PLS, pls, "PLS File Type")
;
GTKPOD_CORE_FILE_TYPE(M3U, m3u, "M3U File Type")
;

void filetype_init_core_types(GHashTable *typetable) {

    PLSFileType *pls = g_object_new(pls_filetype_get_type(), NULL);
    FILE_TYPE_GET_INTERFACE(pls)->category = PLAYLIST;
    g_hash_table_insert(typetable, FILE_TYPE_GET_INTERFACE(pls)->name, pls);

    M3UFileType *m3u = g_object_new(m3u_filetype_get_type(), NULL);
    FILE_TYPE_GET_INTERFACE(m3u)->category = PLAYLIST;
    g_hash_table_insert(typetable, FILE_TYPE_GET_INTERFACE(m3u)->name, m3u);
}

gchar *filetype_get_name(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return NULL;
    return FILE_TYPE_GET_INTERFACE(filetype)->name;
}

gchar *filetype_get_description(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return NULL;
    return FILE_TYPE_GET_INTERFACE(filetype)->description;
}

GList *filetype_get_suffixes(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return NULL;
    return FILE_TYPE_GET_INTERFACE(filetype)->suffixes;
}

Track *filetype_get_file_info(FileType *filetype, const gchar *filename, GError **error) {
    if (!FILE_IS_TYPE(filetype))
        return NULL;
    return FILE_TYPE_GET_INTERFACE(filetype)->get_file_info(filename, error);
}

gboolean filetype_write_file_info(FileType *filetype, const gchar *filename, Track *track, GError **error) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->write_file_info(filename, track, error);
}

gboolean filetype_read_soundcheck(FileType *filetype, const gchar *filename, Track *track, GError **error) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->read_soundcheck(filename, track, error);
}

gchar *filetype_get_gain_cmd(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return NULL;
    return FILE_TYPE_GET_INTERFACE(filetype)->get_gain_cmd();
}

gboolean filetype_read_lyrics(FileType *filetype, const gchar *filename, gchar **lyrics, GError **error) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->read_lyrics(filename, lyrics, error);
}

gboolean filetype_write_lyrics(FileType *filetype, const gchar *filename, const gchar *lyrics, GError **error) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->write_lyrics(filename, lyrics, error);
}

gboolean filetype_read_gapless(FileType *filetype, const gchar *filename, Track *track, GError **error) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->read_gapless(filename, track, error);
}

gboolean filetype_can_convert(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->can_convert();
}

gchar *filetype_get_conversion_cmd(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return NULL;
    return FILE_TYPE_GET_INTERFACE(filetype)->get_conversion_cmd();
}

gboolean filetype_is_playlist_filetype(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->category == PLAYLIST;
}

gboolean filetype_is_video_filetype(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->category == VIDEO;
}

gboolean filetype_is_audio_filetype(FileType *filetype) {
    if (!FILE_IS_TYPE(filetype))
        return FALSE;
    return FILE_TYPE_GET_INTERFACE(filetype)->category == AUDIO;
}

Track *filetype_no_track_info(const gchar *name, GError **error) {
    return NULL;
}

gboolean filetype_no_write_file_info(const gchar *filename, Track *track, GError **error) {
    return FALSE;
}

gboolean filetype_no_soundcheck(const gchar *filename, Track *track, GError **error) {
    return FALSE;
}

gchar *filetype_no_gain_cmd() {
    return NULL;
}

gboolean filetype_no_read_lyrics(const gchar *filename, gchar **lyrics, GError **error) {
    filetype_log_error (error,
            _("Error: Lyrics not supported for this file format."));
    return FALSE;
}

gboolean filetype_no_write_lyrics(const gchar *filename, const gchar *lyrics, GError **error) {
    return FALSE;
}

gboolean filetype_no_read_gapless(const gchar *filename, Track *track, GError **error) {
    return FALSE;
}

gboolean filetype_no_convert() {
    return FALSE;
}

gchar * filetype_no_conversion_cmd() {
    return NULL;
}

void filetype_log_error(GError **error, gchar *msg) {
    g_set_error (error,
                GTKPOD_GENERAL_ERROR,                       /* error domain */
                GTKPOD_GENERAL_ERROR_FAILED,               /* error code */
                msg);
}
