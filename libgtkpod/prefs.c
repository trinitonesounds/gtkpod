/*
 |  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
 |  Copyright (C) 2006 James Liggett <jrliggett at cox.net>
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

/* ----------------------------------------------------------------
 *
 * The prefs module should be thread safe. The hash table is locked
 * before each read or write access.
 *
 * The temp_prefs module is not thread-safe. If necessary a locking
 * mechanism can be implemented.
 *
 * ---------------------------------------------------------------- */

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
 Ditto for AIX 3.2 and <stdlib.h>.  */
#ifndef _NO_PROTO
# define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#ifdef HAVE_GETOPT_LONG_ONLY
#  include <getopt.h>
#else
#  include "getopt.h"
#endif

#include "prefs.h"
#include "misc.h"
#include "misc_conversion.h"
#include "clientserver.h"
#include "directories.h"

/* some public key names used several times */
const gchar *KEY_CONCAL_AUTOSYNC = "concal_autosync";
const gchar *KEY_SYNC_DELETE_TRACKS = "sync_delete_tracks";
const gchar *KEY_SYNC_CONFIRM_DELETE = "sync_confirm_delete";
const gchar *KEY_SYNC_CONFIRM_DIRS = "sync_confirm_dirs";
const gchar *KEY_SYNC_SHOW_SUMMARY = "sync_show_summary";
const gchar *KEY_MOUNTPOINT = "mountpoint";
const gchar *KEY_BACKUP = "filename";
const gchar *KEY_IPOD_MODEL = "ipod_model";
const gchar *KEY_FILENAME = "filename";
const gchar *KEY_PATH_SYNC_CONTACTS = "path_sync_contacts";
const gchar *KEY_PATH_SYNC_CALENDAR = "path_sync_calendar";
const gchar *KEY_PATH_SYNC_NOTES = "path_sync_notes";
const gchar *KEY_SYNCMODE = "syncmode";
const gchar *KEY_MANUAL_SYNCDIR = "manual_syncdir";

/*
 * Data global to this module only
 */

struct temp_prefs_save {
    GIOChannel *gio;
    GError **error;
    gboolean success;
};

struct sub_data {
    TempPrefs *temp_prefs;
    TempPrefs *temp_prefs_orig;
    const gchar *subkey;
    const gchar *subkey2;
    gboolean exists;
};

/* Pointer to preferences hash table */
static GHashTable *prefs_table = NULL;

#if GLIB_CHECK_VERSION(2,31,0)
static GMutex prefs_table_mutex;
#else
static GMutex *prefs_table_mutex = NULL;
#endif

static void _create_mutex() {
#if GLIB_CHECK_VERSION(2,31,0)
    // As it is static the mutex needs no initialisation
#else
    if (!prefs_table_mutex)
        prefs_table_mutex = g_mutex_new ();
#endif
}

static void _lock_mutex() {
#if GLIB_CHECK_VERSION(2,31,0)
    g_mutex_lock (&prefs_table_mutex);
#else
    g_return_if_fail (prefs_table_mutex);
    g_mutex_lock (prefs_table_mutex);
#endif
}

static void _unlock_mutex() {
#if GLIB_CHECK_VERSION(2,31,0)
    g_mutex_unlock (&prefs_table_mutex);
#else
    g_return_if_fail (prefs_table_mutex);
    g_mutex_unlock (prefs_table_mutex);
#endif
}

/*
 * Functions used by this module only
 */
void discard_prefs(void);

/* Different paths that can be set in the prefs window */
typedef enum {
    PATH_MP3GAIN, PATH_SYNC_CONTACTS, PATH_SYNC_CALENDAR, PATH_SYNC_NOTES, PATH_AACGAIN, PATH_NUM
} PathType;

/* enum for reading of options */
enum {
    GP_HELP, GP_PLAYCOUNT, GP_MOUNT, GP_PRINT_HASH,
};

/* Lock the prefs table. If the table is already locked the calling
 * thread will remain blocked until the lock is released by the other thread. */
static void lock_prefs_table() {
    _lock_mutex();
}

/* Unlock the prefs table again. */
static void unlock_prefs_table() {
    _unlock_mutex();
}

/* Set default preferences */
static void set_default_preferences() {
    int i;
    gchar *str;

    prefs_set_int("update_existing", FALSE);
    prefs_set_int("id3_write", FALSE);
    prefs_set_int("id3_write_id3v24", FALSE);
    prefs_set_int(KEY_SYNC_DELETE_TRACKS, TRUE);
    prefs_set_int(KEY_SYNC_CONFIRM_DELETE, TRUE);
    prefs_set_int(KEY_SYNC_SHOW_SUMMARY, TRUE);
    prefs_set_int("show_duplicates", TRUE);
    prefs_set_int("show_non_updated", TRUE);
    prefs_set_int("show_updated", TRUE);
    prefs_set_int("photo_library_confirm_delete", TRUE);
    prefs_set_int("delete_ipod", TRUE);
    prefs_set_int("delete_file", TRUE);
    prefs_set_int("delete_local_file", TRUE);
    prefs_set_int("delete_database", TRUE);
    prefs_set_string("initial_mountpoint", "/media/ipod");

    /*
     * When adding files, determines after how many a
     * save should be performed.
     */
    prefs_set_int("file_saving_threshold", 40);

    str = g_build_filename(get_script_dir(), CONVERT_TO_MP3_SCRIPT, NULL);
    prefs_set_string("path_conv_mp3", str);
    g_free(str);

    str = g_build_filename(get_script_dir(), CONVERT_TO_M4A_SCRIPT, NULL);
    prefs_set_string("path_conv_m4a", str);
    g_free(str);

    str = g_build_filename(get_script_dir(), CONVERT_TO_MP4_SCRIPT, NULL);
    prefs_set_string("path_conv_mp4", str);
    g_free(str);

    /* Set colum preferences */
    for (i = 0; i < TM_NUM_COLUMNS; i++) {
        prefs_set_int_index("col_order", i, i);
    }

    prefs_set_int_index("col_visible", TM_COLUMN_ARTIST, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_ALBUM, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_TITLE, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_TRACKLEN, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_RATING, TRUE);

    for (i = 0; i < TM_NUM_TAGS_PREFS; i++) {
        prefs_set_int_index("tag_autoset", i, FALSE);
    }

    prefs_set_int_index("tag_autoset", TM_COLUMN_TITLE, TRUE);

    prefs_set_int("horizontal_scrollbar", TRUE);
    prefs_set_int("filter_tabs_top", FALSE);

    prefs_set_int("mpl_autoselect", TRUE);

    /* Set window sizes */
    prefs_set_int("size_gtkpod.x", 780);
    prefs_set_int("size_gtkpod.y", 580);
    prefs_set_int("size_cal.x", 500);
    prefs_set_int("size_cal.y", 300);
    prefs_set_int("size_conf_sw.x", 300);
    prefs_set_int("size_conf_sw.y", 300);
    prefs_set_int("size_conf.x", 300);
    prefs_set_int("size_conf.y", -1);
    prefs_set_int("size_dirbr.x", 300);
    prefs_set_int("size_dirbr.y", 400);
    prefs_set_int("size_prefs.x", -1);
    prefs_set_int("size_prefs.y", 480);
    prefs_set_int("size_info.x", 510);
    prefs_set_int("size_info.y", 300);

    /* size of file dialog if there is not a details textview */
    prefs_set_int("size_file_dialog.x", 320);
    prefs_set_int("size_file_dialog.y", 140);

    /* size of file dialog if there is a details textview */
    prefs_set_int("size_file_dialog_details.x", 320);
    prefs_set_int("size_file_dialog_details.y", 140);

    prefs_set_int("readtags", TRUE);
    prefs_set_int("parsetags", FALSE);
    prefs_set_int("parsetags_overwrite", FALSE);
    prefs_set_string("parsetags_template", "%a - %A/%T %t.mp3;%t.wav");
    prefs_set_int("coverart_apic", TRUE);
    prefs_set_int("coverart_file", TRUE);
    prefs_set_string("coverart_template", "%A;folder.jpg");
    prefs_set_string("video_thumbnailer_prog", "totem-video-thumbnailer %f %o");
    prefs_set_int("startup_messages", TRUE);
    prefs_set_int("add_recursively", TRUE);
    prefs_set_int("info_window", FALSE);
    prefs_set_int("last_prefs_page", 0);
    prefs_set_int("multi_edit_title", TRUE);
    prefs_set_int("multi_edit", FALSE);
    prefs_set_int("not_played_track", TRUE);
    prefs_set_int("misc_track_nr", 25);
    prefs_set_int("update_charset", FALSE);
    prefs_set_int("display_tooltips_main", TRUE);
    prefs_set_int("display_tooltips_prefs", TRUE);
    prefs_set_int("display_toolbar", TRUE);
    prefs_set_int("toolbar_style", GTK_TOOLBAR_BOTH);
    prefs_set_int("sha1", TRUE);
    prefs_set_int("file_dialog_details_expanded", FALSE);

    /* Set last browsed directory */
    str = g_get_current_dir();

    if (str) {
        prefs_set_string("last_dir_browsed", str);
        g_free(str);
    }
    else
        prefs_set_string("last_dir_browsed", g_get_home_dir());

    /* Set sorting prefs */
    prefs_set_int("case_sensitive", FALSE);

    /* New conversion preferences */
    prefs_set_int("conversion_target_format", TARGET_FORMAT_MP3);

    /* ReplayGain prefs */
    prefs_set_int("replaygain_offset", 0);
    prefs_set_int("replaygain_mode_album_priority", TRUE);
}

/* Initialize default variable-length list entries */
static void set_default_list_entries() {
    if (!prefs_get_string_value_index("sort_ign_string_", 0, NULL)) {
        prefs_set_string_index("sort_ign_string_", 0, "a ");
        prefs_set_string_index("sort_ign_string_", 1, "an ");
        prefs_set_string_index("sort_ign_string_", 2, LIST_END_MARKER);
    }
}

/* A printf-like function that outputs in the system locale */
static void locale_fprintf(FILE *fp, const gchar *format, ...) {
    gchar *utf8_string; /* Raw UTF-8 string */
    gchar *locale_string; /* String in system locale format */
    va_list format_list; /* Printf-like formatting arguments */

    /* Create the locale format string based on the given format */
    va_start(format_list, format);
    utf8_string = g_strdup_vprintf(format, format_list);
    va_end(format_list);

    locale_string = g_locale_from_utf8(utf8_string, -1, NULL, NULL, NULL);

    if (fp)
        fprintf(fp, "%s", locale_string);

    g_free(utf8_string);
    g_free(locale_string);
}

/* Print commandline usage information */
static void usage(FILE *fp) {
    locale_fprintf(fp, _("gtkpod version %s usage:\n"), VERSION);
    locale_fprintf(fp, _("  -h, --help:   display this message\n"));
    locale_fprintf(fp, _("  -p <file>:    increment playcount for file by one\n"));
    locale_fprintf(fp, _("  --hash <file>:print gtkpod hash for file\n"));
    locale_fprintf(fp, _("  -m path:      define the mountpoint of your iPod\n"));
    locale_fprintf(fp, _("  --mountpoint: same as '-m'.\n"));
}

/* Parse commandline based options */
static void read_commandline(int argc, char *argv[]) {
    int option; /* Code returned by getopt */

    /* The options data structure. The format is standard getopt. */
    struct option const options[] =
        {
            { "h", no_argument, NULL, GP_HELP },
            { "help", no_argument, NULL, GP_HELP },
            { "p", required_argument, NULL, GP_PLAYCOUNT },
            { "hash", required_argument, NULL, GP_PRINT_HASH },
            { "m", required_argument, NULL, GP_MOUNT },
            { "mountpoint", required_argument, NULL, GP_MOUNT },
            { 0, 0, 0, 0 } };

    /* Handle commandline options */
    while ((option = getopt_long_only(argc, argv, "", options, NULL)) != -1) {
        switch (option) {
        case GP_HELP:
            usage(stdout);
            exit(0);
            break;
        case GP_PLAYCOUNT:
            client_playcount(optarg);
            exit(0);
            break;
        case GP_PRINT_HASH:
            print_sha1_hash(optarg);
            exit(0);
            break;
        case GP_MOUNT:
            prefs_set_string("initial_mountpoint", optarg);
            break;
        default:
            locale_fprintf(stderr, "Unknown option: %s\n", argv[optind]);
            usage(stderr);
            exit(1);
            break;
        };
    }
}

/* Read options from environment variables */
static void read_environment() {
    gchar *buf;

    buf = convert_filename(getenv("IPOD_MOUNTPOINT"));
    if (buf)
        prefs_set_string("initial_mountpoint", buf);
    g_free(buf);
}

/* Create a full numbered key from a base key string and a number.
 * Free returned string. */
static gchar *create_full_key(const gchar *base_key, gint index) {
    if (base_key)
        return g_strdup_printf("%s%i", base_key, index);
    else
        return NULL;
}

/* Remove key present in the temp prefs tree from the hash table */
static gboolean flush_key(gpointer key, gpointer value, gpointer user_data) {
    g_return_val_if_fail (prefs_table, FALSE);

    g_hash_table_remove(prefs_table, key);

    return FALSE;
}

/* Copy key data from the temp prefs tree to the hash table (or to
 * sub_data->temp_prefs_orig if non-NULL). The old key is removed. */
static gboolean subst_key(gpointer key, gpointer value, gpointer user_data) {
    struct sub_data *sub_data = user_data;
    gint len;

    g_return_val_if_fail (key && value && user_data, FALSE);
    g_return_val_if_fail (sub_data->subkey && sub_data->subkey2, FALSE);
    if (!sub_data->temp_prefs_orig)
        g_return_val_if_fail (prefs_table, FALSE);
    if (sub_data->temp_prefs_orig)
        g_return_val_if_fail (sub_data->temp_prefs_orig->tree, FALSE);

    len = strlen(sub_data->subkey);

    if (strncmp(key, sub_data->subkey, len) == 0) {
        gchar *new_key = g_strdup_printf("%s%s", sub_data->subkey2, ((gchar *) key) + len);
        if (sub_data->temp_prefs_orig) {
            g_tree_remove(sub_data->temp_prefs_orig->tree, key);
            g_tree_insert(sub_data->temp_prefs_orig->tree, new_key, g_strdup(value));
        }
        else {
            g_hash_table_remove(prefs_table, key);
            g_hash_table_insert(prefs_table, new_key, g_strdup(value));
        }
    }
    return FALSE;
}

/* return TRUE if @key starts with @subkey */
static gboolean match_subkey(gpointer key, gpointer value, gpointer subkey) {
    g_return_val_if_fail (key && subkey, FALSE);

    if (strncmp(key, subkey, strlen(subkey)) == 0)
        return TRUE;
    return FALSE;
}

/* return TRUE and set sub_data->exists to TRUE if @key starts with
 * @subkey */
static gboolean check_subkey(gpointer key, gpointer value, gpointer user_data) {
    struct sub_data *sub_data = user_data;

    g_return_val_if_fail (key && user_data, TRUE);
    g_return_val_if_fail (sub_data->subkey, TRUE);

    if (strncmp(key, sub_data->subkey, strlen(sub_data->subkey)) == 0) {
        sub_data->exists = TRUE;
        return TRUE;
    }
    return FALSE;
}

/* Add key/value to temp_prefs if it matches subkey -- called by
 * prefs_create_subset() and temp_prefs_create_subset() */
static gboolean get_subset(gpointer key, gpointer value, gpointer user_data) {
    struct sub_data *sub_data = user_data;

    g_return_val_if_fail (key && value && user_data, TRUE);
    g_return_val_if_fail (sub_data->subkey && sub_data->temp_prefs, TRUE);

    if (strncmp(key, sub_data->subkey, strlen(sub_data->subkey)) == 0) { /* match */
        temp_prefs_set_string(sub_data->temp_prefs, key, value);
    }
    return FALSE; /* continue traversal (g_tree), ignored for g_hash */
}

/* Copy a variable-length list to the prefs table */
static gboolean copy_list(gpointer key, gpointer value, gpointer user_data) {
    prefs_apply_list((gchar*) key, (GList*) value);
    return FALSE;
}

/* Callback that writes pref table data to a file */
static void write_key(gpointer key, gpointer value, gpointer user_data) {
    FILE *fp; /* file pointer passed in through user_data */

    /* Write out each key and value to the given file */
    fp = (FILE*) user_data;

    if (fp)
        fprintf(fp, "%s=%s\n", (gchar*) key, (gchar*) value);
}

/* Gets a string that contains ~/.gtkpod/ If the folder doesn't exist,
 * create it. Free the string when you are done with it.
 * If the folder wasn't found, and couldn't be created, return NULL */
gchar *prefs_get_cfgdir() {
    gchar *folder; /* Folder path */

    /* Create the folder path. If the folder doesn't exist, create it. */
    folder = g_build_filename(g_get_home_dir(), ".gtkpod", NULL);

    if (!g_file_test(folder, G_FILE_TEST_IS_DIR)) {
        if ((g_mkdir(folder, 0777)) == -1) {
            printf(_("Couldn't create '%s'\n"), folder);
            g_free(folder);
            return NULL;
        }
    }
    return folder;
}

/* get @key and @value from a string like "key=value" */
/* you must g_free (*key) and (*value) after use */
static gboolean read_prefs_get_key_value(const gchar *buf, gchar **key, gchar **value) {
    size_t len; /* string length */
    const gchar *buf_start; /* Pointer to where actual useful data starts in line */

    g_return_val_if_fail (buf && key && value, FALSE);

    /* Strip out any comments (lines that begin with ; or #) */
    if ((buf[0] == ';') || (buf[0] == '#'))
        return FALSE;

    /* Find the key and value, and look for malformed lines */
    buf_start = strchr(buf, '=');

    if ((!buf_start) || (buf_start == buf)) {
        printf("Parse error reading prefs: %s", buf);
        return FALSE;
    }

    /* Find the key name */
    *key = g_strndup(buf, (buf_start - buf));

    /* Strip whitespace */
    g_strstrip (*key);

    /* Find the value string */
    *value = strdup(buf_start + 1);

    /* remove newline */
    len = strlen(*value);
    if ((len > 0) && ((*value)[len - 1] == 0x0a))
        (*value)[len - 1] = 0;

    /* Don't strip whitespace! If there is any, there's a reason for it. */
    /* g_strstrip (*value); */

    return TRUE;
}

/* Read preferences from a file */
static void read_prefs_from_file(FILE *fp) {
    gchar buf[PATH_MAX]; /* Buffer that contains one line */
    gchar *key; /* Pref value key */
    gchar *value; /* Pref value */

    g_return_if_fail (prefs_table && fp);

    while (fgets(buf, PATH_MAX, fp)) {
        if (read_prefs_get_key_value(buf, &key, &value)) {
            g_hash_table_insert(prefs_table, key, value);
        }
    }
}

/* Write prefs to file */
static void write_prefs_to_file(FILE *fp) {
    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    g_hash_table_foreach(prefs_table, write_key, (gpointer) fp);

    unlock_prefs_table();
}

/* Load preferences, first loading the defaults, and then overwrite that with
 * preferences in the user home folder. */
static void load_prefs() {
    gchar *filename; /* Config path to open */
    gchar *config_dir; /* Directory where config is (usually ~/.gtkpod) */
    FILE *fp;

    /* Start by initializing the prefs to their default values */
    set_default_preferences();

    /* and then override those values with those found in the home folder. */
    config_dir = prefs_get_cfgdir();

    if (config_dir) {
        filename = g_build_filename(config_dir, "prefs", NULL);

        if (filename) {
            fp = fopen(filename, "r");

            if (fp) {
                read_prefs_from_file(fp);
                fclose(fp);
            }

            g_free(filename);
        }

        g_free(config_dir);
    }

    /* Finally, initialize variable-length lists. Do this after everything else
     * so that list defaults don't hang out in the table after prefs have been
     * read from the file. */
    set_default_list_entries();
}

/* Save preferences to user home folder (~/.gtkpod/prefs) */
void prefs_save() {
    gchar *filename; /* Path of file to write to */
    gchar *config_dir; /* Folder where prefs file is */
    FILE *fp; /* File pointer */

    /* Open $HOME/.gtkpod/prefs, and write prefs */
    config_dir = prefs_get_cfgdir();

    if (config_dir) {
        filename = g_build_filename(config_dir, "prefs", NULL);

        if (filename) {
            fp = fopen(filename, "w");

            if (fp) {
                write_prefs_to_file(fp);
                fclose(fp);
            }

            g_free(filename);
        }

        g_free(config_dir);
    }
}

static gboolean temp_prefs_save_fe(gchar *key, gchar *value, struct temp_prefs_save *tps) {
    gchar *buf;
    GIOStatus status;

    buf = g_strdup_printf("%s=%s\n", key, value);
    status = g_io_channel_write_chars(tps->gio, buf, -1, NULL, tps->error);
    g_free(buf);
    if (status != G_IO_STATUS_NORMAL) {
        tps->success = FALSE;
        return TRUE; /* stop traversal */
    }
    return FALSE;
}

/* Save @temp_prefs to @filename in the same manner as prefs_save is
 * saving to ~/.gtkpod/prefs. @error: location where to store
 * information about errors or NULL.
 *
 * Return value: TRUE on success, FALSE if an error occured, in which
 * case @error will be set accordingly.
 */
gboolean temp_prefs_save(TempPrefs *temp_prefs, const gchar *filename, GError **error) {
    GIOChannel *gio;
    struct temp_prefs_save tps;

    g_return_val_if_fail (temp_prefs && filename, FALSE);

    gio = g_io_channel_new_file(filename, "w", error);
    tps.gio = gio;
    tps.error = error;
    tps.success = TRUE;
    if (gio) {
        g_tree_foreach(temp_prefs->tree, (GTraverseFunc) temp_prefs_save_fe, &tps);
        g_io_channel_unref(gio);
    }

    return tps.success;
}

TempPrefs *temp_prefs_load(const gchar *filename, GError **error) {
    GIOChannel *gio;
    TempPrefs *temp_prefs = NULL;

    g_return_val_if_fail (filename, NULL);

    gio = g_io_channel_new_file(filename, "r", error);
    if (gio) {
        GIOStatus status;

        temp_prefs = temp_prefs_create();

        do {
            gchar *line = NULL;

            status = g_io_channel_read_line(gio, &line, NULL, NULL, error);
            if (status == G_IO_STATUS_NORMAL) {
                gchar *key, *value;
                if (read_prefs_get_key_value(line, &key, &value)) {
                    temp_prefs_set_string(temp_prefs, key, value);
                    g_free(key);
                    g_free(value);
                }
            }
            g_free(line);
        }
        while (status == G_IO_STATUS_NORMAL);

        g_io_channel_unref(gio);

        if (status != G_IO_STATUS_EOF) {
            temp_prefs_destroy(temp_prefs);
            temp_prefs = NULL;
        }
    }

    return temp_prefs;
}

/* Removes already existing list keys from the prefs table */
static void wipe_list(const gchar *key) {
    gchar *full_key; /* Complete key, with its number suffix */
    guint i; /* Loop counter */

    /* Go through the prefs table, starting at key<number>, delete it and go
     * through key<number+1>... until there are no keys left */

    for (i = 0;; i++) {
        full_key = create_full_key(key, i);

        if (g_hash_table_remove(prefs_table, full_key)) {
            g_free(full_key);
            continue;
        }
        else /* We got all the unneeded keys, leave the loop... */
        {
            g_free(full_key);
            break;
        }
    }
}

/* Delete and rename keys */
static void cleanup_keys() {
    gchar *buf;
    gint int_buf;
    gint i;
    //    gint x, y, p;  /* Window position */
    float version = 0;

    /* Get version */
    version = prefs_get_double("version");

    /* rename mountpoint to initial_mountpoint */
    if (prefs_get_string_value(KEY_MOUNTPOINT, &buf)) {
        prefs_set_string("initial_mountpoint", buf);
        g_free(buf);
        prefs_set_string(KEY_MOUNTPOINT, NULL);
    }

    /* rename coverart to coverart_file */
    if (prefs_get_string_value("coverart", &buf)) {
        prefs_set_string("coverart_file", buf);
        g_free(buf);
        prefs_set_string("coverart", NULL);
    }

    /* rename tm_sort_ to tm_sort */
    if (prefs_get_string_value("tm_sort_", &buf)) {
        prefs_set_string("tm_sort", buf);
        g_free(buf);
        prefs_set_string("tm_sort_", NULL);
    }

    /* rename md5 to sha1 */
    if (prefs_get_string_value("md5", &buf)) {
        prefs_set_string("sha1", buf);
        g_free(buf);
        prefs_set_string("md5", NULL);
    }

    /* Conversion Paths */
    gchar *str;
    str = g_build_filename(get_script_dir(), CONVERT_TO_MP3_SCRIPT, NULL);
    prefs_set_string("path_conv_mp3", str);
    g_free(str);

    str = g_build_filename(get_script_dir(), CONVERT_TO_M4A_SCRIPT, NULL);
    prefs_set_string("path_conv_m4a", str);
    g_free(str);

    str = g_build_filename(get_script_dir(), CONVERT_TO_MP4_SCRIPT, NULL);
    prefs_set_string("path_conv_mp4", str);
    g_free(str);

    /* MP3 Gain */
    if (prefs_get_string_value_index("path", PATH_MP3GAIN, &buf)) {
        prefs_set_string("path_mp3gain", buf);
        g_free(buf);
        prefs_set_string_index("path", PATH_MP3GAIN, NULL);
    }

    if (prefs_get_string_value_index("toolpath", PATH_MP3GAIN, &buf)) {
        prefs_set_string("path_mp3gain", buf);
        g_free(buf);
        prefs_set_string_index("toolpath", PATH_MP3GAIN, NULL);
    }

    /* Sync contacts */
    if (prefs_get_string_value_index("path", PATH_SYNC_CONTACTS, &buf)) {
        prefs_set_string("itdb_0_path_sync_contacts", buf);
        g_free(buf);
        prefs_set_string_index("path", PATH_SYNC_CONTACTS, NULL);
    }

    if (prefs_get_string_value_index("toolpath", PATH_SYNC_CONTACTS, &buf)) {
        prefs_set_string("itdb_0_path_sync_contacts", buf);
        g_free(buf);
        prefs_set_string_index("toolpath", PATH_SYNC_CONTACTS, NULL);
    }

    /* Sync calendar */
    if (prefs_get_string_value_index("path", PATH_SYNC_CALENDAR, &buf)) {
        prefs_set_string("itdb_0_path_sync_calendar", buf);
        g_free(buf);
        prefs_set_string_index("path", PATH_SYNC_CALENDAR, NULL);
    }

    if (prefs_get_string_value_index("toolpath", PATH_SYNC_CALENDAR, &buf)) {
        prefs_set_string("itdb_0_path_sync_calendar", buf);
        g_free(buf);
        prefs_set_string_index("toolpath", PATH_SYNC_CALENDAR, NULL);
    }

    /* Sync notes */
    if (prefs_get_string_value_index("path", PATH_SYNC_NOTES, &buf)) {
        prefs_set_string("itdb_0_path_sync_notes", buf);
        g_free(buf);
        prefs_set_string_index("path", PATH_SYNC_NOTES, NULL);
    }

    if (prefs_get_string_value_index("toolpath", PATH_SYNC_NOTES, &buf)) {
        prefs_set_string("itdb_0_path_sync_notes", buf);
        g_free(buf);
        prefs_set_string_index("toolpath", PATH_SYNC_NOTES, NULL);
    }

    /* If there's an extra (PATH_NUM) key, delete it */
    prefs_set_string_index("path", PATH_NUM, NULL);
    prefs_set_string_index("toolpath", PATH_NUM, NULL);

    /* Ignore/remove some keys */
    prefs_set_string("play_now_path", NULL);
    prefs_set_string("sync_remove", NULL);
    prefs_set_string("sync_remove_confirm", NULL);
    prefs_set_string("show_sync_dirs", NULL);
    prefs_set_string("play_enqueue_path", NULL);
    prefs_set_string("mp3gain_path", NULL);
    prefs_set_string("statusbar_timeout", NULL);
    prefs_set_string("offline", NULL);
    prefs_set_string("time_format", NULL);
    prefs_set_string("id3_all", NULL);
    prefs_set_string("pm_autostore", NULL);
    prefs_set_string("backups", NULL);
    prefs_set_string("save_sorted_order", NULL);
    prefs_set_string("fix_path", NULL);
    prefs_set_string("write_gaintag", NULL);
    prefs_set_string("automount", NULL);
    prefs_set_string("display_artcovers", NULL);
    prefs_set_string("block_display", NULL);
    prefs_set_string("tmp_disable_sort", NULL);
    prefs_set_string("size_file_dialog.x", NULL);
    prefs_set_string("size_file_dialog.y", NULL);
    prefs_set_string("size_file_dialog_details.x", NULL);
    prefs_set_string("size_file_dialog_details.y", NULL);
    prefs_set_string("autoimport", NULL);
    prefs_set_string("auto_import", NULL);
    prefs_set_string("conversion_enable", NULL);
    prefs_set_string("export_template", NULL);
    prefs_set_string("filename_format", NULL);
    prefs_set_string("write_extended_info", NULL);

    /* sm_col_width renamed to tm_col_width */
    for (i = 0; i < TM_NUM_COLUMNS; i++) {
        if (prefs_get_int_value_index("sm_col_width", i, &int_buf)) {
            prefs_set_int_index("tm_col_width", i, int_buf);
            prefs_set_string_index("sm_col_width", i, NULL);
        }
    }

    /* not_played_song renamed to not_played_track */
    if (prefs_get_int_value("not_played_song", &int_buf)) {
        prefs_set_int("not_played_track", int_buf);
        prefs_set_string("not_played_song", NULL);
    }

    /* misc_song_nr renamed to misc_track_nr */
    if (prefs_get_int_value("misc_song_nr", &int_buf)) {
        prefs_set_int("misc_track_nr", int_buf);
        prefs_set_string("misc_song_nr", NULL);
    }

    /* For versions < 0.91, remove all itdb keys */
    if (version < 0.91)
        prefs_flush_subkey("itdb_");

    prefs_set_string("version", VERSION);
}

/* Initialize the prefs table and read configuration */
void prefs_init(int argc, char *argv[]) {
    _create_mutex();

    lock_prefs_table();

    /* Create the prefs hash table */
    prefs_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    unlock_prefs_table();

    /* Load preferences */
    load_prefs();

    /* Clean up old prefs keys */
    cleanup_keys();

    /* Read environment variables */
    read_environment();

    /* Read commandline arguments */
    read_commandline(argc, argv);
}

/* Delete the hash table */
void prefs_shutdown() {
    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    /* Delete the prefs hash table */
    g_hash_table_destroy(prefs_table);
    prefs_table = NULL;

    unlock_prefs_table();

    /* We can't free the prefs_table_mutex in a thread-safe way */
}

/* Create the temp prefs tree */
/* Free the returned structure with delete_temp_prefs() */
TempPrefs *temp_prefs_create() {
    TempPrefs *temp_prefs; /* Returned temp prefs structure */

    temp_prefs = (TempPrefs*) g_malloc(sizeof(TempPrefs));

    temp_prefs->tree = g_tree_new_full((GCompareDataFunc) strcmp, NULL, g_free, g_free);

    return temp_prefs;
}

static void copy_key_to_temp_prefs(gpointer key, gpointer value, gpointer user_data) {
    temp_prefs_set_string((TempPrefs *) user_data, key, value);
}

void temp_prefs_copy_prefs(TempPrefs *temp_prefs) {
    g_return_if_fail (prefs_table);
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    lock_prefs_table();
    g_hash_table_foreach(prefs_table, copy_key_to_temp_prefs, temp_prefs);
    unlock_prefs_table();
}

/* Delete temp prefs */
void temp_prefs_destroy(TempPrefs *temp_prefs) {
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    g_tree_destroy(temp_prefs->tree);
    g_free(temp_prefs);
}

/* Copy key data from the temp prefs tree to the hash table */
static gboolean copy_key(gpointer key, gpointer value, gpointer user_data) {
    prefs_set_string(key, value);

    return FALSE;
}

/* Copy the data from the temp prefs tree to the permanent prefs table */
void temp_prefs_apply(TempPrefs *temp_prefs) {
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    g_tree_foreach(temp_prefs->tree, copy_key, NULL);
}

/* Create a temp_prefs tree containing a subset of keys in the
 permanent prefs table (those starting with @subkey */
static TempPrefs *prefs_create_subset_unlocked(const gchar *subkey) {
    struct sub_data sub_data;

    g_return_val_if_fail (prefs_table, NULL);

    sub_data.temp_prefs = temp_prefs_create();
    sub_data.subkey = subkey;

    g_hash_table_foreach(prefs_table, (GHFunc) get_subset, &sub_data);

    return sub_data.temp_prefs;
}

/* Create a temp_prefs tree containing a subset of keys in the
 permanent prefs table (those starting with @subkey */
TempPrefs *prefs_create_subset(const gchar *subkey) {
    TempPrefs *temp_prefs;

    lock_prefs_table();

    temp_prefs = prefs_create_subset_unlocked(subkey);

    unlock_prefs_table();

    return temp_prefs;
}

/* Create a temp_prefs tree containing a subset of keys in the
 permanent prefs table (those starting with @subkey */
TempPrefs *temp_prefs_create_subset(TempPrefs *temp_prefs, const gchar *subkey) {
    struct sub_data sub_data;

    g_return_val_if_fail (temp_prefs, NULL);
    g_return_val_if_fail (temp_prefs->tree, NULL);

    sub_data.temp_prefs = temp_prefs_create();
    sub_data.subkey = subkey;

    g_tree_foreach(temp_prefs->tree, get_subset, &sub_data);

    return sub_data.temp_prefs;
}

/* Remove all keys in the temp prefs tree from the permanent prefs
 table */
void temp_prefs_flush(TempPrefs *temp_prefs) {
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    lock_prefs_table();

    g_tree_foreach(temp_prefs->tree, flush_key, NULL);

    unlock_prefs_table();
}

/* Return the number of keys stored in @temp_prefs */
gint temp_prefs_size(TempPrefs *temp_prefs) {
    g_return_val_if_fail (temp_prefs, 0);
    g_return_val_if_fail (temp_prefs->tree, 0);

    return g_tree_nnodes(temp_prefs->tree);
}

/* Returns TRUE if at least one key starting with @subkey exists */
gboolean temp_prefs_subkey_exists(TempPrefs *temp_prefs, const gchar *subkey) {
    struct sub_data sub_data;

    g_return_val_if_fail (temp_prefs && subkey, FALSE);

    sub_data.temp_prefs = NULL;
    sub_data.subkey = subkey;
    sub_data.exists = FALSE;

    g_tree_foreach(temp_prefs->tree, check_subkey, &sub_data);

    return sub_data.exists;
}

/* Special functions */

/* Remove all keys that start with @subkey */
void prefs_flush_subkey(const gchar *subkey) {
    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    g_hash_table_foreach_remove(prefs_table, match_subkey, (gchar *) subkey);

    unlock_prefs_table();
}

/* Rename all keys that start with @subkey_old in such a way that they
 start with @subkey_new */
void prefs_rename_subkey(const gchar *subkey_old, const gchar *subkey_new) {
    struct sub_data sub_data;

    g_return_if_fail (subkey_old);
    g_return_if_fail (subkey_new);

    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    sub_data.temp_prefs = prefs_create_subset_unlocked(subkey_old);
    sub_data.temp_prefs_orig = NULL;

    if (temp_prefs_size(sub_data.temp_prefs) > 0) {
        sub_data.subkey = subkey_old;
        sub_data.subkey2 = subkey_new;
        g_tree_foreach(sub_data.temp_prefs->tree, subst_key, &sub_data);
    }

    temp_prefs_destroy(sub_data.temp_prefs);

    unlock_prefs_table();
}

/* Rename all keys that start with @subkey_old in such a way that they
 start with @subkey_new */
void temp_prefs_rename_subkey(TempPrefs *temp_prefs, const gchar *subkey_old, const gchar *subkey_new) {
    struct sub_data sub_data;

    g_return_if_fail (temp_prefs);
    g_return_if_fail (subkey_old);
    g_return_if_fail (subkey_new);

    sub_data.temp_prefs_orig = temp_prefs;
    sub_data.temp_prefs = temp_prefs_create_subset(temp_prefs, subkey_old);

    if (temp_prefs_size(sub_data.temp_prefs) > 0) {
        sub_data.subkey = subkey_old;
        sub_data.subkey2 = subkey_new;
        g_tree_foreach(sub_data.temp_prefs->tree, subst_key, &sub_data);
    }

    temp_prefs_destroy(sub_data.temp_prefs);
}

/* Functions for non-numbered pref keys */

/* Set a string value with the given key, or remove key if @value is
 NULL */
void prefs_set_string(const gchar *key, const gchar *value) {
    g_return_if_fail (key);

    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    if (value)
        g_hash_table_insert(prefs_table, g_strdup(key), g_strdup(value));
    else
        g_hash_table_remove(prefs_table, key);

    unlock_prefs_table();
}

/* Set a key value to a given integer */
void prefs_set_int(const gchar *key, const gint value) {
    gchar *strvalue; /* String value converted from integer */

    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    strvalue = g_strdup_printf("%i", value);
    g_hash_table_insert(prefs_table, g_strdup(key), strvalue);

    unlock_prefs_table();
}

/* Set a key to an int64 value */
void prefs_set_int64(const gchar *key, const gint64 value) {
    gchar *strvalue; /* String value converted from int64 */

    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    strvalue = g_strdup_printf("%" G_GINT64_FORMAT, value);
    g_hash_table_insert(prefs_table, g_strdup(key), strvalue);

    unlock_prefs_table();
}

void prefs_set_double(const gchar *key, gdouble value) {
    gchar *strvalue; /* String value converted from integer */

    lock_prefs_table();

    if (!prefs_table) {
        unlock_prefs_table();
        g_return_if_reached ();
    }

    strvalue = g_strdup_printf("%f", value);
    g_hash_table_insert(prefs_table, g_strdup(key), strvalue);

    unlock_prefs_table();
}

/* Get a string value associated with a key. Free returned string. */
gchar *prefs_get_string(const gchar *key) {
    gchar *string = NULL;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), NULL));

    string = g_strdup(g_hash_table_lookup(prefs_table, key));

    unlock_prefs_table();

    return string;
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean prefs_get_string_value(const gchar *key, gchar **value) {
    const gchar *string; /* String value from prefs table */
    gboolean valid = FALSE;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), FALSE));

    string = g_hash_table_lookup(prefs_table, key);

    if (value)
        *value = g_strdup(string);
    if (string)
        valid = TRUE;

    unlock_prefs_table();

    return valid;
}

/* Get an integer value from a key */
gint prefs_get_int(const gchar *key) {
    gchar *string; /* Hash value string */
    gint value; /* Returned value */

    value = 0;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), value));

    string = g_hash_table_lookup(prefs_table, key);

    if (string)
        value = atoi(string);

    unlock_prefs_table();

    return value;
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean prefs_get_int_value(const gchar *key, gint *value) {
    gchar *string; /* String value from prefs table */
    gboolean valid = FALSE;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), FALSE));

    string = g_hash_table_lookup(prefs_table, key);

    if (value) {
        if (string)
            *value = atoi(string);
        else
            *value = 0;
    }

    if (string)
        valid = TRUE;

    unlock_prefs_table();

    return valid;
}

/* Get a 64 bit integer value from a key */
gint64 prefs_get_int64(const gchar *key) {
    gchar *string; /* Key value string */
    gint64 value; /* Returned value */

    value = 0;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), value));

    string = g_hash_table_lookup(prefs_table, key);

    if (string)
        value = g_ascii_strtoull(string, NULL, 10);

    unlock_prefs_table();

    return value;
}

/* Get a 64 bit integer value from a key */
/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean prefs_get_int64_value(const gchar *key, gint64 *value) {
    gchar *string; /* String value from prefs table */
    gboolean valid = FALSE;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), FALSE));

    string = g_hash_table_lookup(prefs_table, key);

    if (value) {
        if (string)
            *value = g_ascii_strtoull(string, NULL, 10);
        else
            *value = 0;
    }

    if (string)
        valid = TRUE;

    unlock_prefs_table();

    return valid;
}

gdouble prefs_get_double(const gchar *key) {
    gchar *string; /* Key value string */
    gdouble value; /* Returned value */

    value = 0;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), value));

    string = g_hash_table_lookup(prefs_table, key);

    if (string)
        value = g_ascii_strtod(string, NULL);

    unlock_prefs_table();

    return value;
}

gboolean prefs_get_double_value(const gchar *key, gdouble *value) {
    gchar *string; /* String value from prefs table */
    gboolean valid = FALSE;

    lock_prefs_table();

    g_return_val_if_fail (prefs_table, (unlock_prefs_table(), FALSE));

    string = g_hash_table_lookup(prefs_table, key);

    if (value) {
        if (string)
            *value = g_ascii_strtod(string, NULL);
        else
            *value = 0;
    }

    if (string)
        valid = TRUE;

    unlock_prefs_table();

    return valid;
}

/* Functions for numbered pref keys */

/* Set a string value with the given key */
void prefs_set_string_index(const gchar *key, const guint index, const gchar *value) {
    gchar *full_key; /* Complete numbered key */

    full_key = create_full_key(key, index);
    prefs_set_string(full_key, value);

    g_free(full_key);
}

/* Set a key value to a given integer */
void prefs_set_int_index(const gchar *key, const guint index, const gint value) {
    gchar *full_key; /* Complete numbered key */

    full_key = create_full_key(key, index);
    prefs_set_int(full_key, value);

    g_free(full_key);
}

/* Set a key to an int64 value */
void prefs_set_int64_index(const gchar *key, const guint index, const gint64 value) {
    gchar *full_key; /* Complete numbered key */

    full_key = create_full_key(key, index);
    prefs_set_int64(full_key, value);

    g_free(full_key);
}

/* Set a key to a gdouble value */
void prefs_set_double_index(const gchar *key, guint index, gdouble value) {
    gchar *full_key; /* Complete numbered key */

    full_key = create_full_key(key, index);
    prefs_set_double(full_key, value);

    g_free(full_key);
}

/* Get a string value associated with a key. Free returned string. */
gchar *prefs_get_string_index(const gchar *key, const guint index) {
    gchar *full_key; /* Complete numbered key */
    gchar *string; /* Return string */

    full_key = create_full_key(key, index);
    string = prefs_get_string(full_key);

    g_free(full_key);
    return string;
}

/* Get a string value associated with a key. Free returned string. */
/* Use this if you need to know if the given key actually exists */
gboolean prefs_get_string_value_index(const gchar *key, const guint index, gchar **value) {
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */

    full_key = create_full_key(key, index);
    ret = prefs_get_string_value(full_key, value);

    g_free(full_key);
    return ret;
}

/* Get an integer value from a key */
gint prefs_get_int_index(const gchar *key, const guint index) {
    gchar *full_key; /* Complete numbered key */
    gint value; /* Returned integer value */

    full_key = create_full_key(key, index);
    value = prefs_get_int(full_key);

    g_free(full_key);
    return value;
}

/* Get an integer value from a key */
/* Use this if you need to know if the given key actually exists */
gboolean prefs_get_int_value_index(const gchar *key, const guint index, gint *value) {
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */

    full_key = create_full_key(key, index);
    ret = prefs_get_int_value(full_key, value);

    g_free(full_key);
    return ret;
}

/* Get a 64 bit integer value from a key */
gint64 prefs_get_int64_index(const gchar *key, const guint index) {
    gchar *full_key; /* Complete numbered key */
    gint64 value; /* Return value */

    full_key = create_full_key(key, index);
    value = prefs_get_int64(full_key);

    g_free(full_key);
    return value;
}

/* Get a 64 bit integer value from a key */
/* Use this if you need to know if the given key actually exists */
gboolean prefs_get_int64_value_index(const gchar *key, const guint index, gint64 *value) {
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */

    full_key = create_full_key(key, index);
    ret = prefs_get_int64_value(full_key, value);

    g_free(full_key);
    return ret;
}

gdouble prefs_get_double_index(const gchar *key, guint index) {
    gchar *full_key; /* Complete numbered key */
    gdouble value; /* Return value */

    full_key = create_full_key(key, index);
    value = prefs_get_double(full_key);

    g_free(full_key);
    return value;
}

gboolean prefs_get_double_value_index(const gchar *key, guint index, gdouble *value) {
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */

    full_key = create_full_key(key, index);
    ret = prefs_get_double_value(full_key, value);

    g_free(full_key);
    return ret;
}

/* Add string value with the given key to temp prefs. Note: use
 * temp_prefs_remove_key() to remove key from the temp prefs. Setting
 * it to NULL will not remove the key. It will instead remove the key
 * in the main prefs table when you call temp_prefs_apply(). */
void temp_prefs_set_string(TempPrefs *temp_prefs, const gchar *key, const gchar *value) {
    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    g_tree_insert(temp_prefs->tree, g_strdup(key), g_strdup(value));
}

/* Add string value with the given key to temp prefs. Remove the key
 * if @value is NULL. */
void temp_prefs_remove_key(TempPrefs *temp_prefs, const gchar *key) {
    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    g_tree_remove(temp_prefs->tree, key);
}

/* Add an integer value to temp prefs */
void temp_prefs_set_int(TempPrefs *temp_prefs, const gchar *key, const gint value) {
    gchar *strvalue; /* String value converted from integer */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    strvalue = g_strdup_printf("%i", value);
    g_tree_insert(temp_prefs->tree, g_strdup(key), strvalue);
}

/* Add an int64 to temp prefs */
void temp_prefs_set_int64(TempPrefs *temp_prefs, const gchar *key, const gint64 value) {
    gchar *strvalue; /* String value converted from int64 */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    strvalue = g_strdup_printf("%" G_GINT64_FORMAT, value);
    g_tree_insert(temp_prefs->tree, g_strdup(key), strvalue);
}

void temp_prefs_set_double(TempPrefs *temp_prefs, const gchar *key, gdouble value) {
    gchar *strvalue; /* String value converted from int64 */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    strvalue = g_strdup_printf("%fu", value);
    g_tree_insert(temp_prefs->tree, g_strdup(key), strvalue);
}

/* Get a string value associated with a key. Free returned string. */
gchar *temp_prefs_get_string(TempPrefs *temp_prefs, const gchar *key) {
    g_return_val_if_fail (temp_prefs && temp_prefs->tree, NULL);
    g_return_val_if_fail (key, NULL);

    return g_strdup(g_tree_lookup(temp_prefs->tree, key));
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean temp_prefs_get_string_value(TempPrefs *temp_prefs, const gchar *key, gchar **value) {
    gchar *string; /* String value from prefs table */

    g_return_val_if_fail (temp_prefs && temp_prefs->tree, FALSE);
    g_return_val_if_fail (key, FALSE);

    string = g_tree_lookup(temp_prefs->tree, key);

    if (value)
        *value = g_strdup(string);

    if (string)
        return TRUE;
    else
        return FALSE;
}

/* Get an integer value from a key */
gint temp_prefs_get_int(TempPrefs *temp_prefs, const gchar *key) {
    gchar *string; /* Hash value string */
    gint value; /* Returned value */

    g_return_val_if_fail (temp_prefs && temp_prefs->tree, 0);
    g_return_val_if_fail (key, 0);

    value = 0;

    string = g_tree_lookup(temp_prefs->tree, key);

    if (string)
        value = atoi(string);

    return value;
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean temp_prefs_get_int_value(TempPrefs *temp_prefs, const gchar *key, gint *value) {
    gchar *string; /* String value from prefs table */

    g_return_val_if_fail (temp_prefs && temp_prefs->tree, FALSE);
    g_return_val_if_fail (key, FALSE);

    string = g_tree_lookup(temp_prefs->tree, key);

    if (value) {
        if (string)
            *value = atoi(string);
        else
            *value = 0;
    }

    if (string)
        return TRUE;
    else
        return FALSE;
}

gdouble temp_prefs_get_double(TempPrefs *temp_prefs, const gchar *key) {
    gchar *string; /* Hash value string */
    gdouble value; /* Returned value */

    g_return_val_if_fail (temp_prefs && temp_prefs->tree, 0);
    g_return_val_if_fail (key, 0);

    value = 0.0f;

    string = g_tree_lookup(temp_prefs->tree, key);

    if (string)
        value = g_ascii_strtod(string, NULL);

    return value;
}

gboolean temp_prefs_get_double_value(TempPrefs *temp_prefs, const gchar *key, gdouble *value) {
    gchar *string; /* String value from prefs table */

    g_return_val_if_fail (temp_prefs && temp_prefs->tree, FALSE);
    g_return_val_if_fail (key, FALSE);

    string = g_tree_lookup(temp_prefs->tree, key);

    if (value) {
        if (string)
            *value = g_ascii_strtod(string, NULL);
        else
            *value = 0;
    }

    if (string)
        return TRUE;
    else
        return FALSE;
}

/* Functions for numbered pref keys */

/* Set a string value with the given key */
void temp_prefs_set_string_index(TempPrefs *temp_prefs, const gchar *key, const guint index, const gchar *value) {
    gchar *full_key; /* Complete numbered key */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    full_key = create_full_key(key, index);
    temp_prefs_set_string(temp_prefs, full_key, value);

    g_free(full_key);
}

/* Set a key value to a given integer */
void temp_prefs_set_int_index(TempPrefs *temp_prefs, const gchar *key, const guint index, const gint value) {
    gchar *full_key; /* Complete numbered key */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    full_key = create_full_key(key, index);
    temp_prefs_set_int(temp_prefs, full_key, value);

    g_free(full_key);
}

/* Set a key to an int64 value */
void temp_prefs_set_int64_index(TempPrefs *temp_prefs, const gchar *key, const guint index, const gint64 value) {
    gchar *full_key; /* Complete numbered key */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    full_key = create_full_key(key, index);
    temp_prefs_set_int64(temp_prefs, full_key, value);

    g_free(full_key);
}

/* Functions for variable-length lists */

/* Create a tree that contains lists that need to be rebuilt */
/* Free the returned structure with destroy_temp_lists */
TempLists *temp_lists_create() {
    TempLists *temp_lists; /* Allocated temp list structure */

    temp_lists = (TempLists*) g_malloc(sizeof(TempLists));

    temp_lists->tree = g_tree_new_full((GCompareDataFunc) strcmp, NULL, g_free, (GDestroyNotify) prefs_free_list);
    return temp_lists;
}

/* Destroys the list tree */
void temp_lists_destroy(TempLists *temp_lists) {
    if (temp_lists) {
        if (temp_lists->tree)
            g_tree_destroy(temp_lists->tree);

        g_free(temp_lists);
    }
}

/* Add a list with the given key prefix to a temp list tree */
void temp_list_add(TempLists *temp_lists, const gchar *key, GList *list) {
    if (temp_lists) {
        if (temp_lists->tree)
            g_tree_insert(temp_lists->tree, g_strdup(key), list);
    }
}

/* Copy the items of the lists in the given tree to the prefs table */
void temp_lists_apply(TempLists *temp_lists) {
    if (temp_lists) {
        if (temp_lists->tree)
            g_tree_foreach(temp_lists->tree, copy_list, NULL);
    }
}

/* Copy one list to the prefs table. Useful for lists not changed by a window */
void prefs_apply_list(gchar *key, GList *list) {
    GList *node; /* Current list node */
    guint i; /* Counter */

    i = 0;

    if (prefs_table) {
        /* Clean the existing list */
        wipe_list(key);

        node = list;

        /* Add the new list items to the table */
        while (node) {
            g_hash_table_insert(prefs_table, create_full_key(key, i), g_strdup(node->data));

            node = g_list_next(node);
            i++;
        }

        /* Add the end marker */
        g_hash_table_insert(prefs_table, create_full_key(key, i), g_strdup(LIST_END_MARKER));
    }
}

/* Get the items in a variable-length list from the prefs table */
GList *prefs_get_list(const gchar *key) {
    guint end_marker_hash; /* Hash value of the list end marker */
    guint item_hash; /* Hash value of current list string */
    gchar *item_string = NULL; /* List item string */
    guint i = 0; /* Counter */
    GList *list = NULL; /* List that contains items */

    /* Go through each key in the table until we find the end marker */
    end_marker_hash = g_str_hash(LIST_END_MARKER);

    /*
     * The moment that a preference index returns NULL then
     * return as a NULL cannot appear mid-list.
     *
     * Also return if a preference value is the end marker. This
     * should avoid this function getting stuck in an infinite loop.
     */
    do {
        item_string = prefs_get_string_index(key, i);

        if (item_string) {
            item_hash = g_str_hash(item_string);
            if (item_hash != end_marker_hash) {
                list = g_list_append(list, item_string);
            }
        }
        i++;
    }
    while (item_string != NULL && item_hash != end_marker_hash);

    return list;
}

/* Free a list and its strings */
void prefs_free_list(GList *list) {
    GList *node; /* Current list node */

    node = list;

    /* Go through the list, freeing the strings */

    while (node) {
        if (node->data)
            g_free(node->data);

        node = g_list_next(node);
    }

    g_list_free(list);
}

/* Creates a list from lines in a GtkTextBuffer. Free the list when done. */
GList *get_list_from_buffer(GtkTextBuffer *buffer) {
    GtkTextIter start_iter; /* Start of buffer text */
    GtkTextIter end_iter; /* End of buffer text */
    gchar *text_buffer; /* Raw text buffer */
    gchar **string_array; /* Contains each line of the buffer */
    gchar **string_iter; /* Pointer for iterating through the string vector */
    GList *list; /* List that contains each string */

    list = NULL;

    /* Grab the text from the buffer, and then split it up by lines */
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    text_buffer = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
    string_array = g_strsplit(text_buffer, "\n", -1);
    string_iter = string_array;

    /* Go through each string and put it in the list */
    while (*string_iter) {
        if (strlen(*string_iter) != 0)
            list = g_list_append(list, g_strdup(*string_iter));

        string_iter++;
    }

    g_free(text_buffer);
    g_strfreev(string_array);

    return list;
}
