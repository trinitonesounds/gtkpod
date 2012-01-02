/* 
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sound-juicer.h
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

#ifndef SOUND_JUICER_H
#define SOUND_JUICER_H

#include <glib/gi18n.h>
#include <brasero-medium-selection.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include "sj-extractor.h"

/**
 * A GSettings object
 */
extern GSettings *sj_settings;

/**
 * The main window
 */
extern GtkWidget *main_window;

/**
 * The GtkBuilder UI file
 */
extern GtkBuilder *builder;
#define GET_WIDGET(name) GTK_WIDGET(gtk_builder_get_object(builder, (name)))

/**
 * The extractor GObject being used
 */
extern SjExtractor *extractor;

/**
 * Set if currently extracting
 */
extern gboolean extracting;

/**
 * The columns in the list view
 */
typedef enum {
  COLUMN_STATE,
  COLUMN_EXTRACT,
  COLUMN_NUMBER,
  COLUMN_TITLE,
  COLUMN_ARTIST,
  COLUMN_DURATION,
  COLUMN_DETAILS,
  COLUMN_TOTAL
} ViewColumn;

typedef enum {
  STATE_IDLE,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_EXTRACTING
} TrackState;

typedef enum {
  DEBUG_CD = 1 << 0,
  DEBUG_METADATA = 1 << 1,
  DEBUG_EXTRACTING = 1 << 2,
  DEBUG_PLAYING = 1 << 3,
} SjDebugDomain;

/**
 * The GtkTreeModel which all of the tracks are stored in
 */
extern GtkListStore *track_store;

/**
 * The device we are extracting from.
 */
extern BraseroDrive *drive;

/**
 * The root path to write files too
 */
extern GFile *base_uri;

/**
 * The pattern to expand when naming folders
 */
extern char *path_pattern;

/**
 * The pattern to expand when naming files
 */
extern char *file_pattern;

/**
 * If file names should be shell-friendly (i.e no [ /&*?\] characters
 */
extern gboolean strip_chars;

/**
 * If the CD-ROM should be ejected when it has been extracted.
 */
extern gboolean eject_finished;

/**
 * If the destination folder should be opened when the rip has finished.
 */
extern gboolean open_finished;

/**
 * If we are in auto-start mode
 */
extern gboolean autostart;

/**
 * Toggle, Title and Artist Renderers
 */
extern GtkCellRenderer *toggle_renderer, *title_renderer, *artist_renderer;

/**
 * Debug
 */
void sj_debug (SjDebugDomain domain, const gchar* format, ...);

/**
 * GSettings key names
 */
#define SJ_SETTINGS_DEVICE "device"
#define SJ_SETTINGS_EJECT "eject"
#define SJ_SETTINGS_OPEN "open-completed"
#define SJ_SETTINGS_BASEPATH "base-path"
#define SJ_SETTINGS_BASEURI "base-uri"
#define SJ_SETTINGS_FILE_PATTERN "file-pattern"
#define SJ_SETTINGS_PATH_PATTERN "path-pattern"
#define SJ_SETTINGS_AUDIO_PROFILE "audio-profile"
#define SJ_SETTINGS_PARANOIA "paranoia"
#define SJ_SETTINGS_STRIP "strip-special"
#define SJ_SETTINGS_WINDOW "window"
#define SJ_SETTINGS_AUDIO_VOLUME "volume"

#define SJ_SETTINGS_HTTP_PROXY_ENABLE "enabled"
#define SJ_SETTINGS_HTTP_PROXY "host"
#define SJ_SETTINGS_HTTP_PROXY_PORT "port"

/**
 * Custom stock icons
 */
#define SJ_STOCK_EXTRACT "sj-stock-extract"

/* TODO: need to add a SjWindow object or something */
void sj_main_set_title (const char* detail);

#endif /* SOUND_JUICER_H */
