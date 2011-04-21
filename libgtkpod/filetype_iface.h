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

#ifndef FILE_TYPE_IFACE_H_
#define FILE_TYPE_IFACE_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gtk/gtk.h>
#include "itdb.h"

typedef enum {
    AUDIO,
    VIDEO,
    PLAYLIST,
    DIRECTORY
} filetype_category;

#define FILE_TYPE_TYPE            (filetype_get_type ())
#define FILE_TYPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), FILE_TYPE_TYPE, FileType))
#define FILE_IS_TYPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILE_TYPE_TYPE))
#define FILE_TYPE_GET_INTERFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), FILE_TYPE_TYPE, FileTypeInterface))

typedef struct _FileType FileType;
typedef struct _FileTypeInterface FileTypeInterface;

struct _FileTypeInterface {
    GTypeInterface g_iface;

    gchar *name;
    gchar *description;
    GList *suffixes;
    filetype_category category;
    Track * (* get_file_info) (const gchar *filename, GError **error);
    gboolean (* write_file_info) (const gchar *filename, Track *track, GError **error);
    gboolean (* read_soundcheck) (const gchar *filename, Track *track, GError **error);
    gboolean (* read_lyrics) (const gchar *filename, gchar **lyrics, GError **error);
    gboolean (* write_lyrics) (const gchar *filename, const gchar *lyrics, GError **error);
    gboolean (* read_gapless) (const gchar *filename, Track *track, GError **error);
    gchar * (* get_gain_cmd) (void);
    gboolean (* can_convert) (void);
    gchar * (* get_conversion_cmd) (void);
};

GType filetype_get_type (void);

#define GTKPOD_CORE_FILE_TYPE(class_name, prefix, type_description)        \
        typedef struct _##class_name##FileType {                      \
            GObject parent_instance;                                          \
        } class_name##FileType;                                              \
                                                                                              \
        typedef struct _##class_name##FileTypeClass {                               \
            GObjectClass parent_class;                                                          \
        } class_name##FileTypeClass;                                                \
       \
       extern GType prefix##_filetype_get_type (void);                                      \
       static void prefix##_filetype_init (class_name##FileType *self) {}           \
       static void prefix##_filetype_class_init (class_name##FileTypeClass *klass) {}         \
       static void prefix##_filetype_interface_init (FileTypeInterface *filetype) {     \
           filetype->name = #prefix;                                                                       \
           filetype->description = #type_description;                                                       \
           filetype->suffixes = g_list_append(filetype->suffixes, #prefix);                 \
           filetype->get_file_info = filetype_no_track_info;                                               \
           filetype->write_file_info = filetype_no_write_file_info;                                       \
           filetype->read_soundcheck = filetype_no_soundcheck;                                      \
           filetype->read_lyrics = filetype_no_read_lyrics;                                                  \
           filetype->write_lyrics = filetype_no_write_lyrics;                                                \
           filetype->read_gapless = filetype_no_read_gapless;                                          \
           filetype->get_gain_cmd = filetype_no_gain_cmd;                                                \
           filetype->can_convert = filetype_no_convert;                                                    \
           filetype->get_conversion_cmd = filetype_no_conversion_cmd;                         \
       }                                                                                                                            \
       \
       G_DEFINE_TYPE_WITH_CODE(class_name##FileType, prefix##_filetype, G_TYPE_OBJECT, \
       G_IMPLEMENT_INTERFACE (FILE_TYPE_TYPE, prefix##_filetype_interface_init));        \
       \
       gboolean filetype_is_##prefix##_filetype(FileType *filetype) {                      \
           if (!FILE_IS_TYPE(filetype))                                                                    \
               return FALSE;                                                                                     \
           GList *suffixes = FILE_TYPE_GET_INTERFACE(filetype)->suffixes;          \
           while(suffixes) {                                                                                     \
               return g_str_equal(suffixes->data, #prefix);                                       \
               suffixes = g_list_next(suffixes);                                                          \
           }                                                                                                             \
           return FALSE;                                                                                        \
       }


void filetype_init_core_types(GHashTable *typetable);

gchar *filetype_get_name(FileType *filetype);
gchar *filetype_get_description(FileType *filetype);
GList *filetype_get_suffixes(FileType *filetype);
Track *filetype_get_file_info (FileType *filetype, const gchar *filename, GError **error);
gboolean filetype_write_file_info (FileType *filetype, const gchar *filename, Track *track, GError **error);
gboolean filetype_read_soundcheck (FileType *filetype, const gchar *filename, Track *track, GError **error);
gchar *filetype_get_gain_cmd(FileType *filetype);
gboolean filetype_read_lyrics (FileType *filetype, const gchar *filename, gchar **lyrics, GError **error);
gboolean filetype_write_lyrics (FileType *filetype, const gchar *filename, const gchar *lyrics, GError **error);
gboolean filetype_read_gapless(FileType *filetype, const gchar *filename, Track *track, GError **error);

gboolean filetype_can_convert(FileType *filetype);
gchar *filetype_get_conversion_cmd(FileType *filetype);

gboolean filetype_is_playlist_filetype(FileType *filetype);
gboolean filetype_is_video_filetype(FileType *filetype);
gboolean filetype_is_audio_filetype(FileType *filetype);

gboolean filetype_is_m3u_filetype(FileType *filetype);
gboolean filetype_is_pls_filetype(FileType *filetype);

Track *filetype_no_track_info(const gchar *name, GError **error);
gboolean filetype_no_write_file_info (const gchar *filename, Track *track, GError **error);
gboolean filetype_no_soundcheck (const gchar *filename, Track *track, GError **error);
gboolean filetype_no_read_lyrics (const gchar *filename, gchar **lyrics, GError **error);
gboolean filetype_no_write_lyrics (const gchar *filename, const gchar *lyrics, GError **error);
gboolean filetype_no_read_gapless (const gchar *filename, Track *track, GError **error);
gchar *filetype_no_gain_cmd();
gboolean filetype_no_convert();
gchar *filetype_no_conversion_cmd();

#endif /* FILE_TYPE_IFACE_H_ */
