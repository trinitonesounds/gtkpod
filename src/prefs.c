/* Time-stamp: <2006-06-10 20:19:32 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

/* -------------------------------------------------------------------
 *
 * HOWTO add a new_option to the prefs dialog
 *
 * - add the desired option to the prefs window using glade-2
 *
 * - modify the cfg structure in prefs.h accordingly
 *
 * - set the default value of new_option in cfg_new() in prefs.c
 *
 * - add function prefs_get_new_option() and
 *   prefs_set_new_option() to prefs.[ch]. These functions are
 *   called from within gtkpod to query/set the state of the option.
 *   prefs_set_new_option() should verify that the value passed is
 *   valid.
 *
 * - add a callback on_new_option_*() to prefs_windows.c to set the
 *   new value in the temporary prefs struct.
 *   The value is applied to the actual prefs when pressing the "OK"
 *   or "Apply" button in the prefs window.
 *
 * - if your option is a pointer to data, make sure the data is copied
 *   in clone_prefs() in prefs.c
 *
 * - add code to prefs_window_create() in prefs_window.c to set the
 *   correct state of the option in the prefs window.
 *
 * - add code to prefs_window_set() in prefs_window.c to actually take
 *   over the new state when closing the prefs window
 *
 * - add code to write_prefs_to_file_desc() to write the state of
 *   new_option to the prefs file
 *
 * - add code to read_prefs_from_file_desc() to read the
 *   new_option from the prefs file
 *
 * - if you want new_option to be a command line option as well, add
 *   code to usage() and read_prefs_old()
 *
 * ---------------------------------------------------------------- */

/* FIXME: simplify code to make adding of new options easier:
		  prefs_window_create()
		  write_prefs_to_file_desc()
		  read_prefs_from_file_desc()
*/

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
   Ditto for AIX 3.2 and <stdlib.h>.  */
#ifndef _NO_PROTO
# define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_GETOPT_LONG_ONLY
#  include <getopt.h>
#else
#  include "getopt.h"
#endif

#include "clientserver.h"
#include "display.h"
#include "info.h"
#include "misc.h"
#include "misc_track.h"
#include "prefs.h"

/* New prefs backend. Will replace stuff above */

/*
 * Data global to this module only
 */

struct sub_data
{
    TempPrefs *temp_prefs;
    TempPrefs *temp_prefs_orig;
    const gchar *subkey;
    const gchar *subkey2;
    gboolean exists;
};

/* Pointer to prefrences hash table */
static GHashTable *prefs_table = NULL;

/*
 * Functions used by this module only
 */

/* Set default prefrences */
static void set_default_preferences()
{
    int i; 
  
    prefs_set_int("update_existing", FALSE);
    prefs_set_int("id3_write", FALSE);
    prefs_set_int("id3_write_id3v24", FALSE);
    prefs_set_int(KEY_SYNC_DELETE_TRACKS, TRUE);
    prefs_set_int(KEY_SYNC_CONFIRM_DELETE, TRUE);
    prefs_set_int(KEY_SYNC_SHOW_SUMMARY, TRUE);
    prefs_set_int("show_duplicates", TRUE);
    prefs_set_int("show_non_updated", TRUE);
    prefs_set_int("show_updated", TRUE);
    prefs_set_int("mserv_report_probs", TRUE);
    prefs_set_int("delete_ipod", TRUE);
    prefs_set_int("delete_file", TRUE);
    prefs_set_int("delete_local_file", TRUE);
    prefs_set_int("delete_database", TRUE);
    prefs_set_string("initial_mountpoint", "/mnt/ipod");
    prefs_set_string ("path_play_now", "xmms %s");
    prefs_set_string ("path_play_enqueue", "xmms -e %s");
    prefs_set_string ("path_mserv_trackinfo_root", "/var/lib/mserv/trackinfo/");
  
    /* Set sorting tab defaults */
    for (i = 0; i < SORT_TAB_MAX; i++)
    {
	prefs_set_int_index("st_autoselect", i, TRUE);
	prefs_set_int_index("st_category", i, (i < ST_CAT_NUM ? i : 0));
	prefs_set_int_index("sp_or", i, FALSE);
	prefs_set_int_index("sp_rating_cond", i, FALSE);
	prefs_set_int_index("sp_playcount_cond", i, FALSE);
	prefs_set_int_index("sp_played_cond", i, FALSE);
	prefs_set_int_index("sp_modified_cond", i, FALSE);
	prefs_set_int_index("sp_added_cond", i, FALSE);
	prefs_set_int_index("sp_rating_state", i, 0);
	prefs_set_string_index("sp_played_state", i, ">4w");
	prefs_set_string_index("sp_modified_state", i, "<1d");
	prefs_set_string_index("sp_added_state", i, "<1d");
	prefs_set_int_index("sp_playcount_low", i, 0);
	prefs_set_int_index("sp_playcount_high", i, -1);
	prefs_set_int_index("sp_autodisplay", i, FALSE);
    }
    
    prefs_set_int("sort_tab_num", 2);
    
    /* Set colum prefrences */
    for (i = 0; i < TM_NUM_COLUMNS; i++)
    {
	prefs_set_int_index("tm_col_width", i, 80);
	prefs_set_int_index("col_visible", i, FALSE);
	prefs_set_int_index("col_order", i, i);
    }
		
    for (i = 0; i < TM_NUM_TAGS_PREFS; i++)
	prefs_set_int_index("tag_autoset", i, FALSE);
		
    prefs_set_int_index("tag_autoset", TM_COLUMN_TITLE, TRUE);
    
    prefs_set_int_index("col_visible", TM_COLUMN_ARTIST, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_ALBUM, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_TITLE, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_GENRE, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_PLAYCOUNT, TRUE);
    prefs_set_int_index("col_visible", TM_COLUMN_RATING, TRUE);

    /* Set pane positions--Let gtk worry about position */
    for (i = 0; i < PANED_NUM; i++)
	prefs_set_int_index("paned_pos_", i, -1);
	
    prefs_set_int("mpl_autoselect", TRUE);
	
    /* Set window sizes */
    prefs_set_int("size_gtkpod.x", 600);
    prefs_set_int("size_gtkpod.y", 500);
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

    prefs_set_int("autoimport", FALSE);
    prefs_set_int("readtags", TRUE);
    prefs_set_int("write_extended_info", TRUE);
    prefs_set_int("parsetags", FALSE);
    prefs_set_int("parsetags_overwrite", FALSE);
    prefs_set_string("parsetags_template", "%a - %A/%T %t.mp3;%t.wav");
    prefs_set_int("coverart", TRUE);
    prefs_set_string("coverart_template", "%A;folder.jpg");
    prefs_set_int("mserv_use", FALSE);
    prefs_set_string("mserv_username", "");
}

/* Initialize default variable-length list entries */
static void set_default_list_entries()
{
    if (!prefs_get_string_value_index("sort_ign_string_", 0, NULL))
    {
	prefs_set_string_index("sort_ign_string_", 0, "a ");
	prefs_set_string_index("sort_ign_string_", 1, "an ");
	prefs_set_string_index("sort_ign_string_", 2, LIST_END_MARKER);
    }
}

/* A printf-like function that outputs in the system locale */
static void locale_fprintf(FILE *fp, const gchar *format, ...)
{
    gchar *utf8_string; /* Raw UTF-8 string */
    gchar *locale_string;  /* String in system locale format */
    va_list format_list;  /* Printf-like formatting arguments */
	
    /* Create the locale format string based on the given format */
    va_start(format_list, format);
    utf8_string = g_strdup_vprintf(format, format_list);
    va_end(format_list);
	
    locale_string = g_locale_from_utf8 (utf8_string, -1, NULL, NULL, NULL);
	
    if (fp)
	fprintf(fp, "%s", locale_string);
	
    g_free(utf8_string);
    g_free(locale_string);
}

/* Print commandline usage information */
static void usage(FILE *fp)
{
  locale_fprintf(fp, _("gtkpod version %s usage:\n"), VERSION);
  locale_fprintf(fp, _("  -h, --help:   display this message\n"));
  locale_fprintf(fp, _("  -p <filename>:increment playcount for file by one\n"));
  locale_fprintf(fp, _("  -m path:      define the mountpoint of your iPod\n"));
  locale_fprintf(fp, _("  --mountpoint: same as '-m'.\n"));
  locale_fprintf(fp, _("  -a:           import database automatically after start.\n"));
  locale_fprintf(fp, _("  --auto:       same as '-a'.\n"));
  locale_fprintf(fp, _("  -o:           use offline mode. No changes are exported to the iPod,\n                but to ~/.gtkpod/ instead. iPod is updated if 'Sync' is\n                used with 'Offline' deactivated.\n"));
  locale_fprintf(fp, _("  --offline:    same as '-o'.\n"));
}

/* Not quite ready to use this--disable for now */
#if 0
/* Parse commandline based options */
static gboolean read_commandline(int argc, char *argv[])
{
    int option; /* Code returned by getopt */
	
    /* The options data structure. The format is standard getopt. */
    struct option const options[] =
	{
	    {"h",           no_argument,				NULL, GP_HELP},
	    {"help",        no_argument,				NULL, GP_HELP},
	    {"m",           required_argument,	NULL, GP_MOUNT},
	    {"mountpoint",  required_argument,	NULL, GP_MOUNT},
	    {0, 0, 0, 0}
	};
	
    /* Handle commandline options */
    while ((option = getopt_long_only(argc, argv, "", options, NULL)) != -1)
    {
	switch (option)
	{
	case GP_HELP:
	    usage(stdout);
	    exit(0);
	    return FALSE;
	    break;
	case GP_MOUNT:
	    prefs_set_string("initial_mountpoint", optarg);
	    return TRUE;
	    break;
	default:
	    locale_fprintf(stderr, "Unknown option: %s\n", argv[optind]);
	    usage(stderr);
	    exit(1);
	    return FALSE;
	    break;
	};
    }
    return TRUE;	
}
#endif

/* Read options from environment variables */
static void read_environment()
{
    gchar *buf; 
  
    buf = convert_filename(getenv("IPOD_MOUNTPOINT"));
    if (buf)
	prefs_set_string("initial_mountpoint", buf);
    g_free(buf);
}
 
/* Create a full numbered key from a base key string and a number.
 * Free returned string. */
static gchar *create_full_key(const gchar *base_key, gint index)
{
    if (base_key)
	return g_strdup_printf("%s%i", base_key, index);
    else 
	return NULL;
}

/* Copy key data from the temp prefs tree to the hash table */
static gboolean copy_key(gpointer key, gpointer value, gpointer user_data)
{
    if (prefs_table)
    {
	g_hash_table_replace(prefs_table, (gpointer)g_strdup(key), 
			     (gpointer)g_strdup(value));
    }
	
    return FALSE;
}

/* Remove key present in the temp prefs tree from the hash table */
static gboolean flush_key (gpointer key, gpointer value, gpointer user_data)
{
    g_return_val_if_fail (prefs_table, FALSE);

    g_hash_table_remove (prefs_table, key);

    return FALSE;
}


/* Copy key data from the temp prefs tree to the hash table (or to
 * sub_data->temp_prefs_orig if non-NULL). The old key is removed. */
static gboolean subst_key (gpointer key, gpointer value, gpointer user_data)
{
    struct sub_data *sub_data = user_data;
    gint len;

    g_return_val_if_fail (key && value && user_data, FALSE);
    g_return_val_if_fail (sub_data->subkey && sub_data->subkey2, FALSE);
    if (!sub_data->temp_prefs_orig)
	g_return_val_if_fail (prefs_table, FALSE);
    if (sub_data->temp_prefs_orig)
	g_return_val_if_fail (sub_data->temp_prefs_orig->tree, FALSE);

    len = strlen (sub_data->subkey);

    if (strncmp (key, sub_data->subkey, len) == 0)
    {
	gchar *new_key = g_strdup_printf ("%s%s",
					  sub_data->subkey2,
					  ((gchar *)key)+len);
	if (sub_data->temp_prefs_orig)
	{
	    g_tree_remove (sub_data->temp_prefs_orig->tree, key);
	    g_tree_insert (sub_data->temp_prefs_orig->tree,
			   new_key, g_strdup(value));
	}
	else
	{
	    g_hash_table_remove (prefs_table, key);
	    g_hash_table_insert (prefs_table, new_key, g_strdup(value));
	}
    }
    return FALSE;
}

/* return TRUE if @key starts with @subkey */
static gboolean match_subkey (gpointer key, gpointer value, gpointer subkey)
{
    g_return_val_if_fail (key && subkey, FALSE);

    if (strncmp (key, subkey, strlen (subkey)) == 0)  return TRUE;
    return FALSE;
}


/* return TRUE and set sub_data->exists to TRUE if @key starts with
 * @subkey */
static gboolean check_subkey (gpointer key, gpointer value, gpointer user_data)
{
    struct sub_data *sub_data = user_data;

    g_return_val_if_fail (key && user_data, TRUE);
    g_return_val_if_fail (sub_data->subkey, TRUE);

    if (strncmp (key, sub_data->subkey, strlen (sub_data->subkey)) == 0)
    {
	sub_data->exists = TRUE;
	return TRUE;
    }
    return FALSE;
}



/* Add key/value to temp_prefs if it matches subkey -- called by
 * prefs_create_subset() and temp_prefs_create_subset() */
gboolean get_subset (gpointer key, gpointer value, gpointer user_data)
{
    struct sub_data *sub_data = user_data;

    g_return_val_if_fail (key && value && user_data, TRUE);
    g_return_val_if_fail (sub_data->subkey && sub_data->temp_prefs, TRUE);

    if (strncmp (key, sub_data->subkey,
		 strlen (sub_data->subkey)) == 0)
    {  /* match */
	temp_prefs_set_string (sub_data->temp_prefs, key, value);
    }
    return FALSE; /* continue traversal (g_tree), ignored for g_hash */
}


/* Copy a variable-length list to the prefs table */
static gboolean copy_list(gpointer key, gpointer value, gpointer user_data)
{
	prefs_apply_list((gchar*)key, (GList*)value);
	return FALSE;
}
 
/* Callback that writes pref table data to a file */
static void write_key(gpointer key, gpointer value, gpointer user_data)
{
    FILE *fp;  /* file pointer passed in through user_data */
	
    /* Write out each key and value to the given file */
    fp = (FILE*)user_data;
	
    if (fp)
	fprintf(fp, "%s=%s\n", (gchar*)key, (gchar*)value);
}

/* Gets a string that contains ~/.gtkpod/ If the folder doesn't exist,
 * create it. Free the string when you are done with it.
 * If the folder wasn't found, and couldn't be created, return NULL */
gchar *get_config_dir()
{
    gchar *folder;  /* Folder path */
	
    /* Create the folder path. If the folder doesn't exist, create it. */
    folder = g_build_filename(g_get_home_dir(), ".gtkpod", NULL);
	
    if (!g_file_test(folder, G_FILE_TEST_IS_DIR))
    {
	if ((mkdir(folder, 0755)) == -1)
	{
	    printf("Couldn't create ~/.gtkpod");
	    return NULL;
	}
    }
	
    return folder;
}

/* Disable this until the transition is done */
#if 0
/* Read preferences from a file */
static void read_prefs_from_file(FILE *fp)
{
    gchar buf[PATH_MAX];  /* Buffer that contains one line */
    gchar *buf_start; /* Pointer to where actual useful data starts in line */
    gchar *key;  /* Pref value key */
    gchar *value; /* Pref value */
    size_t len;  /* string length */
	
    if (fp)
    {
	while (fgets(buf, PATH_MAX, fp))
	{
	    /* Strip out any comments (lines that begin with ; or #) */
	    if ((buf[0] == ';') || (buf[0] == '#')) 
		continue;
			
	    /* Find the key and value, and look for malformed lines */
	    value = strchr(buf, '=');
			
	    if ((!value) || (value == buf))
		printf("Parse error reading prefs: %s", buf);
			
	    /* Strip off whitespace */
	    buf_start = buf;
			
	    while (g_ascii_isspace(*buf_start))
		buf_start++;
			
	    /* Find the key name */
	    key = g_strndup(buf, (value - buf_start));
	    value++;
			
	    /* remove newline */
	    len = strlen(value);
			
	    if ((len > 0) && (value[len - 1] == 0x0a))
		value[len - 1] = 0;
			
	    /* Strip whitespace off the key value */
	    while (g_ascii_isspace(*value))
		value++;
			
	    /* Finally, load each key/value pair into the pref hash table */
	    if (prefs_table)
	    {
		g_hash_table_insert(prefs_table, (gpointer)key, 
				    (gpointer)g_strdup(value));
	    }
	}
    }
}
#endif

/* Write prefs to file */
static void write_prefs_to_file(FILE *fp)
{
    if (prefs_table)
	g_hash_table_foreach(prefs_table, write_key, (gpointer)fp);
}

/* Load preferences, first loading the defaults, and then overwrite that with
 * preferences in the user home folder. */
static void load_prefs()
{
#if 0
    gchar *filename; /* Config path to open */
    gchar *config_dir;  /* Directory where config is (usually ~/.gtkpod) */
    FILE *fp;
#endif
	
    /* Start by initializing the prefs to their default values */
    set_default_preferences();
	
#if 0
    /* and then override those values with those found in the home folder. */
    config_dir = get_config_dir();
	
    if (config_dir)
    {
	filename = g_build_filename(config_dir, "prefs", NULL);
		
	if (filename)
	{
	    fp = fopen(filename, "r");
			
	    if (fp)
	    {
		read_prefs_from_file(fp);
		fclose(fp);
	    }
			
	    g_free(filename);
	}
		
	g_free(config_dir);
    }
#endif
	
    /* Finally, initialize variable-length lists. Do this after everything else
     * so that list defaults don't hang out in the table after prefs have been
     * read from the file. */
    set_default_list_entries();
}

/* Save preferences to user home folder (~/gtkpod/prefs) */
static void save_prefs()
{
    gchar *filename;  /* Path of file to write to */
    gchar *config_dir;   /* Folder where prefs file is */
    FILE *fp;  /* File pointer */
	
    /* Open $HOME/.gtkpod/prefs, and write prefs */
    config_dir = get_config_dir();
	
    if (config_dir)
    {
	filename = g_build_filename(config_dir, "prefs", NULL);
		
	if (filename)
	{
	    fp = fopen(filename, "w");

	    if (fp)
	    {
		/* remove fprintf() once the old prefs
		   system is completely gone. */
		fprintf(fp, "version=%s\n", VERSION);

		write_prefs_to_file(fp);
		fclose(fp);
	    }
		
	    g_free(filename);
	}
		
	g_free(config_dir);
    }
}

/* Removes already existing list keys from the prefs table */
static void wipe_list(const gchar *key)
{
    gchar *full_key; /* Complete key, with its number suffix */
    guint i;  /* Loop counter */
	
    /* Go through the prefs table, starting at key<number>, delete it and go 
     * through key<number+1>... until there are no keys left */
	
    for (i = 0;;i++)
    {
	full_key = create_full_key(key, i);
		
	if (g_hash_table_remove(prefs_table, full_key))
	{
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
static void cleanup_keys()
{
    gchar *buf;
    gint int_buf;
    gint i;
    gint x, y, p;  /* Window position */
    float version=0;

    /* get version */
    if (prefs_get_string_value ("version", &buf))
    {
	version = g_ascii_strtod (buf, NULL);
	g_free (buf);
    }
    else
    {
	/* version must only be set to VERSION if no prefs file was
	   found --> elsewhere */
	g_return_if_reached ();
    }

    /* rename mountpoint to initial_mountpoint */
    if (prefs_get_string_value("mountpoint", &buf))
    {
	prefs_set_string("initial_mountpoint", buf);
	g_free(buf);
	prefs_set_string(KEY_MOUNTPOINT, NULL);
    }
  
    /* Convert old path numbered keys to named ones */
  
  
    /* Play Now */
    if (prefs_get_string_value_index("path", PATH_PLAY_NOW, &buf))
    {
	prefs_set_string("path_play_now", buf);
	prefs_set_string_index("path", PATH_PLAY_NOW, NULL);
	if (version < 0.87)
	{  /* default changed from "xmms -p %s" to "xmms
	      %s" which avoids xmms from hanging --
	      thanks to Chris Vine */
	    if (strcmp (buf, "xmms -p %s") == 0)
	    {
		prefs_set_string ("path_play_now", "xmms %s");
	    }
	}
	g_free(buf);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_PLAY_NOW, &buf))
    {
	prefs_set_string("path_play_now", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_PLAY_NOW, NULL);
    }
  
    /* Enqueue */
    if (prefs_get_string_value_index("path", PATH_PLAY_ENQUEUE, &buf))
    {
	prefs_set_string("path_play_enqueue", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_PLAY_ENQUEUE, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_PLAY_ENQUEUE, &buf))
    {
	prefs_set_string("path_play_enqueue", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_PLAY_ENQUEUE, NULL);
    }
  
    /* MP3 Gain */
    if (prefs_get_string_value_index("path", PATH_MP3GAIN, &buf))
    {
	prefs_set_string("path_mp3gain", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_MP3GAIN, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_MP3GAIN, &buf))
    {
	prefs_set_string("path_mp3gain", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_MP3GAIN, NULL);
    }
  
    /* Sync contacts */
    if (prefs_get_string_value_index("path", PATH_SYNC_CONTACTS, &buf))
    {
	prefs_set_string("itdb_0_path_sync_contacts", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_SYNC_CONTACTS, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_SYNC_CONTACTS, &buf))
    {
	prefs_set_string("itdb_0_path_sync_contacts", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_SYNC_CONTACTS, NULL);
    }
  
    /* Sync calendar */
    if (prefs_get_string_value_index("path", PATH_SYNC_CALENDAR, &buf))
    {
	prefs_set_string("itdb_0_path_sync_calendar", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_SYNC_CALENDAR, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_SYNC_CALENDAR, &buf))
    {
	prefs_set_string("itdb_0_path_sync_calendar", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_SYNC_CALENDAR, NULL);
    }
  
    /* Sync notes */
    if (prefs_get_string_value_index("path", PATH_SYNC_NOTES, &buf))
    {
	prefs_set_string("itdb_0_path_sync_notes", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_SYNC_NOTES, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_SYNC_NOTES, &buf))
    {
	prefs_set_string("itdb_0_path_sync_notes", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_SYNC_NOTES, NULL);
    }
  
    /* MSERV music root */
    if (prefs_get_string_value_index("path", PATH_MSERV_MUSIC_ROOT, &buf))
    {
	prefs_set_string("path_mserv_music_root", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_MSERV_MUSIC_ROOT, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_MSERV_MUSIC_ROOT, &buf))
    {
	prefs_set_string("path_mserv_music_root", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_MSERV_MUSIC_ROOT, NULL);
    }
  
    /* MSERV track info root */
    if (prefs_get_string_value_index("path", PATH_MSERV_TRACKINFO_ROOT, &buf))
    {
	prefs_set_string("path_mserv_trackinfo_root", buf);
	g_free(buf);
	prefs_set_string_index("path", PATH_MSERV_TRACKINFO_ROOT, NULL);
    }
  
    if (prefs_get_string_value_index("toolpath", PATH_MSERV_TRACKINFO_ROOT, &buf))
    {
	prefs_set_string("path_mserv_trackinfo_root", buf);
	g_free(buf);
	prefs_set_string_index("toolpath", PATH_MSERV_TRACKINFO_ROOT, NULL);
    }

    /* If there's an extra (PATH_NUM) key, delete it */
    if (prefs_get_string_value_index("path", PATH_NUM, NULL))
	prefs_set_string_index("path", PATH_NUM, NULL);
  
    if (prefs_get_string_value_index("toolpath", PATH_NUM, NULL))
	prefs_set_string_index("toolpath", PATH_NUM, NULL);
  
    /* Ignore play_now_path */
    if (prefs_get_string_value("play_now_path", NULL))
	prefs_set_string("play_now_path", NULL);
  
    /* Ignore sync_remove */
    if (prefs_get_string_value("sync_remove", NULL))
	prefs_set_string("sync_remove", NULL);
  
    /* Ignore sync_remove_confirm */
    if (prefs_get_string_value("sync_remove_confirm", NULL))
	prefs_set_string("sync_remove_confirm", NULL);
  
    /* Ignore show_sync_dirs */
    if (prefs_get_string_value("show_sync_dirs", NULL))
	prefs_set_string("show_sync_dirs", NULL);
  
    /* Ignore play_enqueue_path */
    if (prefs_get_string_value("play_enqueue_path", NULL))
	prefs_set_string("play_enqueue_path", NULL);
  
    /* Ignore mp3gain_path */
    if (prefs_get_string_value("mp3gain_path", NULL))
	prefs_set_string("mp3gain_path", NULL);
  
    /* sp_created_cond renamed to sp_added_cond */
    for (i = 0; i < SORT_TAB_MAX; i++)
    {
	if (prefs_get_int_value_index("sp_created_cond", i, &int_buf))
	    prefs_set_int_index("sp_added_cond", i, int_buf);
    }
  
    /* sp_created_state renamed to sp_added_state */
    for (i = 0; i < SORT_TAB_MAX; i++)
    {
	if (prefs_get_int_value_index("sp_created_state", i, &int_buf))
	    prefs_set_int_index("sp_added_state", i, int_buf);
    }
  
    /* sm_col_width renamed to tm_col_width */
    for (i = 0; i < TM_NUM_COLUMNS; i++)
    {
	if (prefs_get_int_value_index("sm_col_width", i, &int_buf))
	    prefs_set_int_index("tm_col_width", i, int_buf);
    }
  
    /* handle version changes in prefs */
    if (version == 0.0)
    {
	/* most likely prefs file written by V0.50 */
	/* I added two new PANED elements since V0.50 --> shift */
	for (i=PANED_NUM_ST-1; i>=0; --i)
	{
	    prefs_set_int_index("paned_pos_", PANED_NUM_GLADE + i,
				prefs_get_int_index("paned_pos_", PANED_NUM_GLADE + i - 2));
	}
	prefs_set_int_index("paned_pos_", PANED_STATUS1, -1);
	prefs_set_int_index("paned_pos_", PANED_STATUS2, -1);
    }

    /* set statusbar paned to a decent value if unset */
    if (prefs_get_int_index("paned_pos_", PANED_STATUS1) == -1)
    {
	x = prefs_get_int("size_gtkpod.x");
	/* set to about 2/3 of the window width */
	if (x>0)   
	    prefs_set_int_index("paned_pos_", PANED_STATUS1, 20*x/30);
    }
  
    if (prefs_get_int_index("paned_pos_", PANED_STATUS2) == -1)
    {
	x = prefs_get_int("size_gtkpod.x");
	y = prefs_get_int("size_gtkpod.y");
	p = prefs_get_int_index("paned_pos_", PANED_STATUS1);
	/* set to about half of the remaining window */
	if (x>0)   
	    prefs_set_int_index("paned_pos_", PANED_STATUS2, (x-p)/2 );
    }

    /* Changed layout of info window between 0.72 and 0.73 */
    if (version < 0.73)
    {
	prefs_set_string("size_info.x", NULL);
	prefs_set_string("size_info.y", NULL);
    }
    
    /* auto_import reanmed to autoimport */
    if (prefs_get_int_value("auto_import", &int_buf))
    {
	prefs_set_int("autoimport", int_buf);
	prefs_set_string("auto_import", NULL);
    }

    prefs_set_string ("version", VERSION);
}

/* Do things that need to be done before saving prefs */
static void finalize_prefs()
{
  /* Sort column order needs to be stored */
  tm_store_col_order();
  
  /* Update default sizes */
  display_update_default_sizes();
}

/* Initialize the prefs table and read configuration */
void init_prefs(int argc, char *argv[])
{
    /* Create the prefs hash table */
    prefs_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
					g_free);
	
    /* Load preferences */
    load_prefs();
	
    /* Read environment variables */
    read_environment(); 
	
#if 0
    /* Read commandline arguments */
    read_commandline(argc, argv);
#endif
  
    /* Leave this here--will work when transition is complete */
#if 0
    /* Clean up old prefs keys */
    cleanup_keys();
#endif
}

/* Save prefs data to a file, and then delete the hash table */
void cleanup_prefs()
{
    if (prefs_table)
    {
	/* Let prefs do some things before saving */
	finalize_prefs();
    
	/* Save prefs */
	save_prefs();
		
	/* Delete the prefs hash table */
	g_hash_table_destroy(prefs_table);
	prefs_table = NULL;
    }
}

/* Create the temp prefs tree */
/* Free the returned structure with delete_temp_prefs() */
TempPrefs *temp_prefs_create()
{
    TempPrefs *temp_prefs;  /* Retunred temp prefs structure */

    temp_prefs = (TempPrefs*)g_malloc(sizeof(TempPrefs));

    temp_prefs->tree = g_tree_new_full((GCompareDataFunc)strcmp, NULL,
				       g_free, g_free);

    return temp_prefs;	
}

/* Delete temp prefs */
void temp_prefs_destroy(TempPrefs *temp_prefs)
{
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    g_tree_destroy(temp_prefs->tree);
    g_free(temp_prefs);
}

/* Copy the data from the temp prefs tree to the permanent prefs table */
void temp_prefs_apply(TempPrefs *temp_prefs)
{
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    g_tree_foreach (temp_prefs->tree, copy_key, NULL);
}


/* Create a temp_prefs tree containing a subset of keys in the
   permanent prefs table (those starting with @subkey */
TempPrefs *prefs_create_subset (const gchar *subkey)
{
    struct sub_data sub_data;

    g_return_val_if_fail (prefs_table, NULL);

    sub_data.temp_prefs = temp_prefs_create ();
    sub_data.subkey = subkey;

    g_hash_table_foreach (prefs_table, (GHFunc)get_subset, &sub_data);

    return sub_data.temp_prefs;
}


/* Create a temp_prefs tree containing a subset of keys in the
   permanent prefs table (those starting with @subkey */
TempPrefs *temp_prefs_create_subset (TempPrefs *temp_prefs,
				     const gchar *subkey)
{
    struct sub_data sub_data;

    g_return_val_if_fail (temp_prefs, NULL);
    g_return_val_if_fail (temp_prefs->tree, NULL);

    sub_data.temp_prefs = temp_prefs_create ();
    sub_data.subkey = subkey;

    g_tree_foreach (temp_prefs->tree, get_subset, &sub_data);

    return sub_data.temp_prefs;
}


/* Remove all keys in the temp prefs tree from the permanent prefs
   table */
void temp_prefs_flush(TempPrefs *temp_prefs)
{
    g_return_if_fail (temp_prefs);
    g_return_if_fail (temp_prefs->tree);

    g_tree_foreach (temp_prefs->tree, flush_key, NULL);
}

/* Return the number of keys stored in @temp_prefs */
gint temp_prefs_size (TempPrefs *temp_prefs)
{
    g_return_val_if_fail (temp_prefs, 0);
    g_return_val_if_fail (temp_prefs->tree, 0);

    return g_tree_nnodes (temp_prefs->tree);
}


/* Returns TRUE if at least one key starting with @subkey exists */
gboolean temp_prefs_subkey_exists (TempPrefs *temp_prefs,
				   const gchar *subkey)
{
    struct sub_data sub_data;

    g_return_val_if_fail (temp_prefs && subkey, FALSE);

    sub_data.temp_prefs = NULL;
    sub_data.subkey = subkey;
    sub_data.exists = FALSE;

    g_tree_foreach (temp_prefs->tree, check_subkey, &sub_data);

    return sub_data.exists;
}


/* Special functions */

/* Remove all keys that start with @subkey */
void prefs_flush_subkey (const gchar *subkey)
{
    g_return_if_fail (prefs_table);

    g_hash_table_foreach_remove (prefs_table, match_subkey, (gchar *)subkey);
}


/* Rename all keys that start with @subkey_old in such a way that they
   start with @subkey_new */
void prefs_rename_subkey (const gchar *subkey_old, const gchar *subkey_new){
    struct sub_data sub_data;

    g_return_if_fail (prefs_table);
    g_return_if_fail (subkey_old);
    g_return_if_fail (subkey_new);

    sub_data.temp_prefs = prefs_create_subset (subkey_old);
    sub_data.temp_prefs_orig = NULL;

    if (temp_prefs_size (sub_data.temp_prefs) > 0)
    {
	sub_data.subkey = subkey_old;
	sub_data.subkey2 = subkey_new;
	g_tree_foreach (sub_data.temp_prefs->tree, subst_key, &sub_data);
    }

    temp_prefs_destroy (sub_data.temp_prefs);
}


/* Rename all keys that start with @subkey_old in such a way that they
   start with @subkey_new */
void temp_prefs_rename_subkey (TempPrefs *temp_prefs,
			       const gchar *subkey_old,
			       const gchar *subkey_new)
{
    struct sub_data sub_data;

    g_return_if_fail (temp_prefs);
    g_return_if_fail (subkey_old);
    g_return_if_fail (subkey_new);

    sub_data.temp_prefs_orig = temp_prefs;
    sub_data.temp_prefs = temp_prefs_create_subset (temp_prefs,
						    subkey_old);

    if (temp_prefs_size (sub_data.temp_prefs) > 0)
    {
	sub_data.subkey = subkey_old;
	sub_data.subkey2 = subkey_new;
	g_tree_foreach (sub_data.temp_prefs->tree, subst_key, &sub_data);
    }

    temp_prefs_destroy (sub_data.temp_prefs);
}




/* Functions for non-numbered pref keys */

/* Set a string value with the given key, or remove key if @value is
   NULL */
void prefs_set_string(const gchar *key, const gchar *value)
{
    g_return_if_fail (key);

    if (prefs_table)
    {
	if (value)
	    g_hash_table_insert (prefs_table,
				 g_strdup(key), g_strdup(value));
	else
	    g_hash_table_remove (prefs_table, key);
    }
}

/* Set a key value to a given integer */
void prefs_set_int(const gchar *key, const gint value)
{
    gchar *strvalue; /* String value converted from integer */

    if (prefs_table)
    {
	strvalue = g_strdup_printf("%i", value);
	g_hash_table_insert(prefs_table, g_strdup(key), strvalue);
    }
}

/* Set a key to an int64 value */
void prefs_set_int64(const gchar *key, const gint64 value)
{
    gchar *strvalue; /* String value converted from int64 */
	
    if (prefs_table)
    {
	strvalue = g_strdup_printf("%llu", value);
	g_hash_table_insert(prefs_table, g_strdup(key), strvalue);	
    }
}

/* Get a string value associated with a key. Free returned string. */
gchar *prefs_get_string(const gchar *key)
{	
    if (prefs_table)
	return g_strdup(g_hash_table_lookup(prefs_table, key));
    else
	return NULL;
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean prefs_get_string_value(const gchar *key, gchar **value)
{
    gchar *string;  /* String value from prefs table */
	
    if (prefs_table)
    {
	string = g_hash_table_lookup(prefs_table, key);
		
	if (value)
	    *value = g_strdup (string);
	if (string)
	    return TRUE;
    }
    return FALSE;
}

/* Get an integer value from a key */
gint prefs_get_int(const gchar *key)
{
    gchar *string; /* Hash value string */
    gint value;  /* Retunred value */
	
    value = 0;
	
    if (prefs_table)
    {
	string = g_hash_table_lookup(prefs_table, key);
		
	if (string)
	    value = atoi(string);
    }

    return value;
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean prefs_get_int_value(const gchar *key, gint *value)
{
    gchar *string;  /* String value from prefs table */
	
    if (prefs_table)
    {
	string = g_hash_table_lookup(prefs_table, key);

	if (value)
	{
	    if (string)
		*value = atoi(string);
	    else
		*value = 0;
	}

	if (string)
	    return TRUE;
    }
    return FALSE;
}

/* Get a 64 bit integer value from a key */
gint64 prefs_get_int64(const gchar *key)
{
    gchar *string;  /* Key value string */
    gint64 value;  /* Retunred value */
	
    value = 0;

    if (prefs_table)
    {
	string = g_hash_table_lookup(prefs_table, key);

	if (string)
	    value = g_ascii_strtoull(string, NULL, 10);
    }
	
    return value;
}

/* Get a 64 bit integer value from a key */
/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean prefs_get_int64_value(const gchar *key, gint64 *value)
{
    gchar *string;  /* String value from prefs table */
	
    if (prefs_table)
    {
	string = g_hash_table_lookup(prefs_table, key);
		
	if (string)
	{
	    if (value)
		*value = g_ascii_strtoull(string, NULL, 10);
			
	    return TRUE;
	}
    }
    return FALSE;
}

/* Functions for numbered pref keys */

/* Set a string value with the given key */
void prefs_set_string_index(const gchar *key, const guint index, 
			    const gchar *value)
{
    gchar *full_key; /* Complete numbered key */
	
    full_key = create_full_key(key, index);
    prefs_set_string(full_key, value);
	
    g_free(full_key);
}

/* Set a key value to a given integer */
void prefs_set_int_index(const gchar *key, const guint index, 
			 const gint value)
{
    gchar *full_key; /* Complete numbered key */
	
    full_key = create_full_key(key, index);
    prefs_set_int(full_key, value);
	
    g_free(full_key);
}

/* Set a key to an int64 value */
void prefs_set_int64_index(const gchar *key, const guint index, 
			   const gint64 value)
{
    gchar *full_key; /* Complete numbered key */
	
    full_key = create_full_key(key, index);
    prefs_set_int64(full_key, value);
	
    g_free(full_key);
}

/* Get a string value associated with a key. Free returned string. */
gchar *prefs_get_string_index(const gchar *key, const guint index)
{	
    gchar *full_key; /* Complete numbered key */
    gchar *string;  /* Return string */
	
    full_key = create_full_key(key, index);
    string = prefs_get_string(full_key);
	
    g_free(full_key);
    return string;
}

/* Get a string value associated with a key. Free returned string. */
/* Use this if you need to know if the given key actually exists */
gboolean prefs_get_string_value_index(const gchar *key, const guint index, 
				      gchar **value)
{
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */
	
    full_key = create_full_key(key, index);
    ret = prefs_get_string_value(full_key, value);
	
    g_free(full_key);
    return ret;
}

/* Get an integer value from a key */
gint prefs_get_int_index(const gchar *key, const guint index)
{
    gchar *full_key; /* Complete numbered key */
    gint value;  /* Returned integer value */

    full_key = create_full_key(key, index);
    value = prefs_get_int(full_key);
	
    g_free(full_key);
    return value;
}

/* Get an integer value from a key */
/* Use this if you need to know if the given key actually exists */
gboolean prefs_get_int_value_index(const gchar *key, const guint index,  
				   gint *value)
{
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */
	
    full_key = create_full_key(key, index);
    ret = prefs_get_int_value(full_key, value);
	
    g_free(full_key);
    return ret;
}

/* Get a 64 bit integer value from a key */
gint64 prefs_get_int64_index(const gchar *key, const guint index)
{
    gchar *full_key; /* Complete numbered key */
    gint64 value; /* Return value */
	
    full_key = create_full_key(key, index);
    value = prefs_get_int64(full_key);
	
    g_free(full_key);
    return value;
}

/* Get a 64 bit integer value from a key */
/* Use this if you need to know if the given key actually exists */
gboolean prefs_get_int64_value_index(const gchar *key, const guint index, 
				     gint64 *value)
{
    gchar *full_key; /* Complete numbered key */
    gboolean ret; /* Return value */
	
    full_key = create_full_key(key, index);
    ret = prefs_get_int64_value(key, value);
	
    g_free(full_key);
    return ret;
}

/* Add string value with the given key to temp prefs. Remove the key
 * if @value is NULL. */
void temp_prefs_set_string(TempPrefs *temp_prefs, const gchar *key, 
			   const gchar *value)
{
    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    if (value)
	g_tree_insert (temp_prefs->tree, g_strdup(key), g_strdup(value));
    else
	g_tree_remove (temp_prefs->tree, key);
}

/* Add an integer value to temp prefs */
void temp_prefs_set_int(TempPrefs *temp_prefs, const gchar *key, 
			const gint value)
{
    gchar *strvalue; /* String value converted from integer */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    strvalue = g_strdup_printf("%i", value);
    g_tree_insert(temp_prefs->tree, g_strdup(key), strvalue);
}

/* Add an int64 to temp prefs */
void temp_prefs_set_int64(TempPrefs *temp_prefs, const gchar *key, 
			  const gint64 value)
{
    gchar *strvalue; /* String value converted from int64 */
	
    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    strvalue = g_strdup_printf("%llu", value);
    g_tree_insert(temp_prefs->tree, g_strdup(key), strvalue);
}

/* Get a string value associated with a key. Free returned string. */
gchar *temp_prefs_get_string(TempPrefs *temp_prefs, const gchar *key)
{	
    g_return_val_if_fail (temp_prefs && temp_prefs->tree, NULL);
    g_return_val_if_fail (key, NULL);

    return g_strdup (g_tree_lookup (temp_prefs->tree, key));
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean temp_prefs_get_string_value(TempPrefs *temp_prefs,
				     const gchar *key, gchar **value)
{
    gchar *string;  /* String value from prefs table */
	
    g_return_val_if_fail (temp_prefs && temp_prefs->tree, FALSE);
    g_return_val_if_fail (key, FALSE);

    string = g_tree_lookup (temp_prefs->tree, key);

    if (value)
	*value = g_strdup (string);

    if (string)
	return TRUE;
    else
	return FALSE;
}

/* Get an integer value from a key */
gint temp_prefs_get_int(TempPrefs *temp_prefs, const gchar *key)
{
    gchar *string; /* Hash value string */
    gint value;  /* Retunred value */
	
    g_return_val_if_fail (temp_prefs && temp_prefs->tree, 0);
    g_return_val_if_fail (key, 0);

    value = 0;
	
    string = g_tree_lookup (temp_prefs->tree, key);
		
    if (string)
	value = atoi(string);

    return value;
}

/* Use this if you need to know if the given key actually exists */
/* The value parameter can be NULL if you don't need the value itself. */
gboolean temp_prefs_get_int_value(TempPrefs *temp_prefs,
				  const gchar *key, gint *value)
{
    gchar *string;  /* String value from prefs table */
	
    g_return_val_if_fail (temp_prefs && temp_prefs->tree, FALSE);
    g_return_val_if_fail (key, FALSE);

    string = g_tree_lookup (temp_prefs->tree, key);

    if (value)
    {
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


/* Functions for numbered pref keys */

/* Set a string value with the given key */
void temp_prefs_set_string_index(TempPrefs *temp_prefs, const gchar *key,
				 const guint index, const gchar *value)
{
    gchar *full_key; /* Complete numbered key */
	
    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    full_key = create_full_key(key, index);
    temp_prefs_set_string(temp_prefs, full_key, value);
	
    g_free(full_key);
}

/* Set a key value to a given integer */
void temp_prefs_set_int_index(TempPrefs *temp_prefs, const gchar *key,
			      const guint index, const gint value)
{
    gchar *full_key; /* Complete numbered key */

    g_return_if_fail (temp_prefs && temp_prefs->tree);
    g_return_if_fail (key);

    full_key = create_full_key(key, index);
    temp_prefs_set_int(temp_prefs, full_key, value);

    g_free(full_key);
}

/* Set a key to an int64 value */
void temp_prefs_set_int64_index(TempPrefs *temp_prefs, const gchar *key,  
				const guint index, const gint64 value)
{
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
TempLists *temp_lists_create()
{
    TempLists *temp_lists;  /* Allocated temp list structure */
	
    temp_lists = (TempLists*)g_malloc(sizeof(TempLists));
	

    temp_lists->tree = g_tree_new_full((GCompareDataFunc)strcmp, NULL,
				       g_free,
				       (GDestroyNotify)prefs_free_list);
    return temp_lists;
}

/* Destroys the list tree */
void temp_lists_destroy(TempLists *temp_lists)
{
    if (temp_lists)
    {
	if (temp_lists->tree)
	    g_tree_destroy(temp_lists->tree);
		
	g_free(temp_lists);
    }
}

/* Add a list with the given key prefix to a temp list tree */
void temp_list_add(TempLists *temp_lists, const gchar *key, GList *list)
{
    if (temp_lists)
    {
	if (temp_lists->tree)
	    g_tree_insert(temp_lists->tree, g_strdup(key), list);
    }
}
		
/* Copy the items of the lists in the given tree to the prefs table */
void temp_lists_apply(TempLists *temp_lists)
{
    if (temp_lists)
    {
	if (temp_lists->tree)
	    g_tree_foreach(temp_lists->tree, copy_list, NULL);
    }
}

/* Copy one list to the prefs table. Useful for lists not changed by a window */
void prefs_apply_list(gchar *key, GList *list)
{
    GList *node;  /* Current list node */
    guint i;  /* Counter */
	
    i = 0;
	
    if (prefs_table)
    {
	/* Clean the existing list */
	wipe_list(key);
		
	node = list;
		
	/* Add the new list items to the table */
	while (node)
	{
	    g_hash_table_insert(prefs_table, create_full_key(key, i), 
				g_strdup(node->data));
				
	    node = g_list_next(node);
	    i++;
	}
		
	/* Add the end marker */
	g_hash_table_insert(prefs_table, create_full_key(key, i),
			    g_strdup(LIST_END_MARKER));
    }
}

/* Get the items in a variable-length list from the prefs table */
GList *prefs_get_list(const gchar *key)
{
    guint end_marker_hash;  /* Hash value of the list end marker */
    guint item_hash;  /* Hash value of current list string */
    gchar *item_string;  /* List iterm string */
    guint i;  /* Counter */
    GList *list;  /* List that contains items */
	
    /* Go through each key in the table until we find the end marker */
    end_marker_hash = g_str_hash(LIST_END_MARKER);
    list = NULL;
	
    for (i = 0;;i++)
    {
	item_string = prefs_get_string_index(key, i);
		
	if (item_string)
	{
	    item_hash = g_str_hash(item_string);
			
	    if (item_hash != end_marker_hash)
	    {
		list = g_list_append(list, item_string);
		continue;
	    }
	    else
	    {
		g_free(item_string);
		break;
	    }
	}
    }
	
    return list;
}

/* Free a list and its strings */
void prefs_free_list(GList *list)
{
    GList *node;  /* Current list node */
	
    node = list;
	
    /* Go through the list, freeing the strings */
	
    while (node)
    {
	if (node->data)
	    g_free(node->data);
		
	node = g_list_next(node);
    }
	
    g_list_free(list);
}

/* Creates a list from lines in a GtkTextBuffer. Free the list when done. */
GList *get_list_from_buffer(GtkTextBuffer *buffer)
{
    GtkTextIter start_iter; /* Start of buffer text */
    GtkTextIter end_iter; /* End of buffer text */
    gchar *text_buffer; /* Raw text buffer */
    gchar **string_array; /* Contains each line of the buffer */
    gchar **string_iter;  /* Pointer for iterating through the string vector */
    GList *list; /* List that contains each string */
	
    list = NULL;
	
    /* Grab the text from the buffer, and then split it up by lines */
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
	
    text_buffer = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
    string_array = g_strsplit(text_buffer, "\n", -1);
    string_iter = string_array;
	
    /* Go through each string and put it in the list */
    while (*string_iter)
    {
	if (strlen(*string_iter) != 0)
	    list = g_list_append(list, g_strdup(*string_iter));
		
	string_iter++;
    }
	
    return list;
}

/* config struct */
static struct cfg *cfg = NULL;

/* enum for reading of options */
enum {
  GP_HELP,
  GP_PLAYCOUNT,
  GP_MOUNT,
  GP_AUTO,
  GP_OFFLINE
};


struct cfg *cfg_new(void)
{
    struct cfg *mycfg = NULL;
    gchar curdir[PATH_MAX];
    gchar *cfgdir;

    cfgdir = prefs_get_cfgdir ();

    mycfg = g_malloc0 (sizeof (struct cfg));
    if(getcwd(curdir, PATH_MAX))
    {
	prefs_set_string ("last_dir_browsed", curdir);
    }
    else
    {
	gchar *dir = convert_filename ("~/");
	prefs_set_string ("last_dir_browsed", dir);
	g_free (dir);
    }

    mycfg->charset = NULL;    
    mycfg->md5tracks = TRUE;
    mycfg->block_display = FALSE;
    mycfg->offline = FALSE;
    mycfg->display_toolbar = TRUE;
    mycfg->toolbar_style = GTK_TOOLBAR_BOTH;
    mycfg->display_tooltips_main = TRUE;
    mycfg->display_tooltips_prefs = TRUE;
    mycfg->update_charset = FALSE;
    mycfg->write_charset = FALSE;
    mycfg->add_recursively = TRUE;
    mycfg->last_prefs_page = 0;
    mycfg->statusbar_timeout = STATUSBAR_TIMEOUT;

    mycfg->tmp_disable_sort = TRUE;
    mycfg->startup_messages = TRUE;
    mycfg->automount = FALSE;
    mycfg->info_window = FALSE;
    mycfg->multi_edit = FALSE;
    mycfg->multi_edit_title = TRUE;
    mycfg->not_played_track = TRUE;
    mycfg->misc_track_nr = 25;
    mycfg->sortcfg.pm_sort = SORT_NONE;
    mycfg->sortcfg.st_sort = SORT_NONE;
    mycfg->sortcfg.tm_sort = SORT_NONE;
    mycfg->sortcfg.tm_sortcol = TM_COLUMN_TITLE;
    mycfg->sortcfg.tm_autostore = FALSE;
    mycfg->sortcfg.case_sensitive = FALSE;

    g_free (cfgdir);

    return(mycfg);
}


/* Compare strlen(@arg) chars of @arg with @line. Return strlen (@arg)
   in @off if @off != NULL */
static gint arg_comp (const gchar *line, const gchar *arg, gint *off)
{
    if (arg && line)
    {
	gint len = strlen (arg);
	if (off) *off = len;
	return g_ascii_strncasecmp (line, arg, len);
    }
    else
    {
	if (*off)   *off = 0;
	return 0;
    }
}


/* sort order was reversed between V0.82_CVS and V0.83.CVS */
static gint correct_sort (gint sort)
{
    if (cfg && (cfg->version < 0.83))
    {
	switch (sort)
	{
	case SORT_ASCENDING:
	    sort = SORT_DESCENDING;
	    break;
	case SORT_DESCENDING:
	    sort = SORT_ASCENDING;
	    break;
	}
    }
    return sort;
}

/* default ignore strings -- must end with a space */
static char* sort_ign_strings[] =
{
    "the ",
    "a ",
/*    "le ", 
    "la ", 
    "les ", 
    "lo ", 
    "los ",
    "der ",
    "die ",
    "das ",*/ /* will make sorting very slow -- only add the words you
                 really want to skip */
   LIST_END_MARKER,  /* end marker */
    NULL,
};


static void
read_prefs_from_file_desc(FILE *fp)
{
    gchar buf[PATH_MAX];
    gchar *line, *arg, *bufp;
    gint len, i;

    /* set ignore strings */
    for (i=0; sort_ign_strings[i]; ++i)
    {
	bufp = g_strdup_printf ("sort_ign_string_%d", i);
	prefs_set_string (bufp, sort_ign_strings[i]);
	g_free (bufp);
    }
    /* set ignore fields (ignore above words for artist) */
    bufp = g_strdup_printf ("sort_ign_field_%d", T_ARTIST);
    prefs_set_int (bufp, 1);
    g_free (bufp);

    if(fp)
    {
      while (fgets (buf, PATH_MAX, fp))
	{
	  /* allow comments */
	  if ((buf[0] == ';') || (buf[0] == '#')) continue;
	  arg = strchr (buf, '=');
	  if (!arg || (arg == buf))
	    {
	      gtkpod_warning (_("Error while reading prefs: %s\n"), buf);
	      continue;
	    }
	  /* skip whitespace */
	  bufp = buf;
	  while (g_ascii_isspace(*bufp)) ++bufp;
	  line = g_strndup (buf, arg-bufp);
	  ++arg;
	  len = strlen (arg); /* remove newline */
	  if((len>0) && (arg[len-1] == 0x0a))  arg[len-1] = 0;
	  /* skip whitespace */
	  while (g_ascii_isspace(*arg)) ++arg;
	  if(g_ascii_strcasecmp (line, "version") == 0)
	  {
	      cfg->version = g_ascii_strtod (arg, NULL);
	      prefs_set_string ("version", arg);
	  }
	  else if(g_ascii_strcasecmp (line, "time_format") == 0)
	  {
	      /* removed in 0.87 */
	  }
	  else if((g_ascii_strcasecmp (line, "filename_format") == 0) ||
		  (g_ascii_strcasecmp (line, "export_template") == 0))
	  {  /* changed to "export_template" in 0.73CVS */
	      /* this "funky" string was the result of a wrong
		 autoconvert -- just ignore it */
	      if (strcmp (arg, "%a - %a/%T - %T.mp3") != 0)
	      {
		  if (cfg->version < 0.72)
		  {
		      /* changed the meaning of the %x in export_template */
		      gchar *sp = arg;
		      if (sp) while (*sp)
		      {
			  if (sp[0] == '%')
			  {
			      switch (sp[1]) {
			      case 'A':
				  sp[1] = 'a';
				  break;
			      case 'd':
				  sp[1] = 'A';
				  break;
			      case 'n':
				  sp[1] = 't';
				  break;
			      case 't':
				  sp[1] = 'T';
				  break;
			      default:
				  break;
			      }
			  }
			  ++sp;
		      }
		  }
		  prefs_set_string (EXPORT_FILES_TPL, arg);
	      }
	  }
	  else if(g_ascii_strcasecmp (line, "charset") == 0)
	  {
		if(strlen (arg))      prefs_set_charset(arg);
	  }
	  else if(g_ascii_strcasecmp (line, "id3_all") == 0)
	  {
	      /* obsoleted since 0.71 */
	  }
	  else if(g_ascii_strcasecmp (line, "md5") == 0)
	  {
	      prefs_set_md5tracks((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "block_display") == 0)
	  {
	      prefs_set_block_display((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "offline") == 0)
	  {
	      prefs_set_offline((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "group_compilations") == 0)
	  {
	      prefs_set_group_compilations((gboolean)atoi(arg), FALSE);
	  }
	  else if(g_ascii_strcasecmp (line, "toolbar_style") == 0)
	  {
	      prefs_set_toolbar_style(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "pm_autostore") == 0)
	  {
	      /* ignore */
	  }
	  else if((g_ascii_strcasecmp (line, "tm_autostore") == 0) ||
		  (g_ascii_strcasecmp (line, "sm_autostore") == 0))
	  {
	      prefs_set_tm_autostore((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "pm_sort") == 0)
	  {
	      gint sort = correct_sort (atoi(arg));
	      prefs_set_pm_sort(sort);
	  }
	  else if(g_ascii_strcasecmp (line, "st_sort") == 0)
	  {
	      gint sort = correct_sort (atoi(arg));
	      prefs_set_st_sort(sort);
	  }
	  else if((g_ascii_strcasecmp (line, "tm_sort_") == 0) ||
		  (g_ascii_strcasecmp (line, "sm_sort_") == 0))
	  {
	      gint sort = correct_sort (atoi(arg));
	      prefs_set_tm_sort(sort);
	  }
	  else if((g_ascii_strcasecmp (line, "tm_sortcol") == 0) ||
		  (g_ascii_strcasecmp (line, "sm_sortcol") == 0))
	  {
	      prefs_set_tm_sortcol(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "last_prefs_page") == 0)
	  {
	      prefs_set_last_prefs_page(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "backups") == 0)
	  {
	      /* removed with version after 0.82-CVS */
	  }
	  else if(g_ascii_strcasecmp (line, "dir_browse") == 0)
	  {
	      prefs_set_string ("last_dir_browsed", arg);
	  }
	  else if(g_ascii_strcasecmp (line, "dir_export") == 0)
	  {
	      prefs_set_string (EXPORT_FILES_PATH, arg);
	  }
	  else if(g_ascii_strcasecmp (line, "display_toolbar") == 0)
	  {
	      prefs_set_display_toolbar((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "display_tooltips_main") == 0)
	  {
	      prefs_set_display_tooltips_main((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "display_tooltips_prefs") == 0)
	  {
	      prefs_set_display_tooltips_prefs((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "multi_edit") == 0)
	  {
	      prefs_set_multi_edit((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "multi_edit_title") == 0)
	  {
	      prefs_set_multi_edit_title((gboolean)atoi(arg));
	  }
	  else if((g_ascii_strcasecmp (line, "not_played_track") == 0) ||
		  (g_ascii_strcasecmp (line, "not_played_song") == 0))
	  {
	      prefs_set_not_played_track((gboolean)atoi(arg));
	  }
	  else if((g_ascii_strcasecmp (line, "misc_track_nr") == 0) ||
		  (g_ascii_strcasecmp (line, "misc_song_nr") == 0))
	  {
	      prefs_set_misc_track_nr(atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "update_charset") == 0)
	  {
	      prefs_set_update_charset((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "write_charset") == 0)
	  {
	      prefs_set_write_charset((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "add_recursively") == 0)
	  {
	      prefs_set_add_recursively((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "case_sensitive") == 0)
	  {
	      prefs_set_case_sensitive((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "save_sorted_order") == 0)
	  {
	      /* ignore option -- has been deleted with 0.53 */
	  }
	  else if(g_ascii_strcasecmp (line, "export_check_existing") == 0)
	  {
	      prefs_set_int (EXPORT_FILES_CHECK_EXISTING,
				   atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "fix_path") == 0)
	  {
	      /* ignore -- wie always fix the export path (replace
	       * non-compatible chars) */
	  }
	  else if(g_ascii_strcasecmp (line, "automount") == 0)
	  {
	      prefs_set_automount (atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "info_window") == 0)
	  {
	      prefs_set_info_window (atoi (arg));
	  }
	  else if(g_ascii_strcasecmp (line, "write_gaintag") == 0)
	  {
	      /* ignore -- not used any more */
	  }
	  else if(g_ascii_strcasecmp (line, "concal_autosync") == 0)
	  {
	      prefs_set_int ("itdb_0_concal_autosync", atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "tmp_disable_sort") == 0)
	  {
	      prefs_set_tmp_disable_sort ((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "startup_messages") == 0)
	  {
	      /* only set if no upgrade/downgrade was performed */
	      if (cfg->version == g_ascii_strtod (VERSION, NULL))
		  prefs_set_startup_messages ((gboolean)atoi(arg));
	  }
	  else if(g_ascii_strcasecmp (line, "special_export_charset") == 0)
	  {
	      prefs_set_int (EXPORT_FILES_SPECIAL_CHARSET,
				   atoi (arg));
	  }
	  else
	  {   /* All leftover options will be stored into the prefs
		 setting hash (generic options -- should have had this
		 idea much sooner... */
	      gboolean skip = FALSE;
	      if (cfg->version < 0.91)
	      {
		  if(arg_comp (line, "itdb_", NULL) == 0)
		  {   /* set incorrectly in 0.90 -- delete */
		      skip = TRUE;
		  }
	      }
	      if (!skip)
		  prefs_set_string (line, arg);
	  }
	  g_free(line);
	}
    }
}


/* we first read from /etc/gtkpod/prefs and then overwrite the
   settings with ~/.gtkpod/prefs */
void
read_prefs_defaults(void)
{
  gchar *cfgdir = NULL;
  gchar *filename;
  FILE *fp = NULL;
  gboolean have_prefs = FALSE;

  cfgdir = prefs_get_cfgdir ();

  filename = g_build_filename (cfgdir, "prefs", NULL);
  if(g_file_test(filename, G_FILE_TEST_EXISTS))
  {
      if((fp = fopen(filename, "r")))
      {
	  read_prefs_from_file_desc(fp);
	  fclose(fp);
    cleanup_keys();
	  have_prefs = TRUE; /* read prefs */
      }
      else
      {
	  gtkpod_warning(_("Unable to open config file '%s' for reading\n"), filename);
	  }
  }
  g_free (filename);

  if (!have_prefs)
  {
      filename = g_build_filename ("/etc", "gtkpod", "prefs", NULL);

      if (g_file_test (filename, G_FILE_TEST_EXISTS))
      {
	  if((fp = fopen(filename, "r")))
	  {
	      read_prefs_from_file_desc(fp);
	      fclose(fp);
	      have_prefs = TRUE; /* read prefs */
	  }
      }
      g_free (filename);
  }
  g_free (cfgdir);
  /* set version of the prefs file to "current" if none was read */
  if (!have_prefs)
  {
      cfg->version = g_ascii_strtod (VERSION, NULL);
      prefs_set_string ("version", VERSION);
  }
}

/* Read Preferences and initialise the cfg-struct */
/* Return value: FALSE if "-p" argument was given -> stop program */
gboolean read_prefs_old (GtkWidget *gtkpod, int argc, char *argv[])
{
  GtkCheckMenuItem *menu;
  int opt;
  int option_index;
  gboolean result = TRUE;
  struct option const options[] =
    {
      { "h",           no_argument,	NULL, GP_HELP },
      { "help",        no_argument,	NULL, GP_HELP },
      { "p",           required_argument,       NULL, GP_PLAYCOUNT },
      { "m",           required_argument,	NULL, GP_MOUNT },
      { "mountpoint",  required_argument,	NULL, GP_MOUNT },
      { "offline",     no_argument,	NULL, GP_OFFLINE },
      { "a",           no_argument,	NULL, GP_AUTO },
      { "auto",        no_argument,	NULL, GP_AUTO },
      { 0, 0, 0, 0 }
    };
		
  if (cfg != NULL) discard_prefs ();

  cfg = cfg_new();
  read_prefs_defaults();

  while((opt=getopt_long_only(argc, argv, "", options, &option_index)) != -1) {
    switch(opt)
      {
      case GP_HELP:
	  usage(stdout);
	  exit(0);
	  break;
      case GP_PLAYCOUNT:
	  client_playcount (optarg);
	  result = FALSE;
	  break;
      case GP_MOUNT:
	  prefs_set_string ("initial_mountpoint", optarg);
	  break;
      case GP_AUTO:
	  prefs_set_int("autoimport_commandline", TRUE);
	  break;
      case GP_OFFLINE:
	  prefs_set_offline (TRUE);
	  break;
      default:
	  locale_fprintf(stderr, _("Unknown option: %s\n"), argv[optind]);
	  usage(stderr);
	  exit(1);
      }
  }

  menu = GTK_CHECK_MENU_ITEM (gtkpod_xml_get_widget (main_window_xml, "offline_menu"));
  gtk_check_menu_item_set_active (menu, prefs_get_offline ());
  return result;
}

static void
write_prefs_to_file_desc(FILE *fp)
{

    if(!fp)
	fp = stderr;


    if (cfg->charset)
    {
	fprintf(fp, "charset=%s\n", cfg->charset);
    } else {
	fprintf(fp, "charset=\n");
    }
    fprintf(fp, "md5=%d\n",prefs_get_md5tracks ());
    fprintf(fp, "block_display=%d\n",prefs_get_block_display());
    fprintf(fp, "group_compilations=%d\n",prefs_get_group_compilations());
    fprintf(fp, "last_prefs_page=%d\n",prefs_get_last_prefs_page());
    fprintf(fp, "offline=%d\n",prefs_get_offline());
    fprintf(fp, "display_toolbar=%d\n",prefs_get_display_toolbar());
    fprintf(fp, "toolbar_style=%d\n",prefs_get_toolbar_style());
    fprintf(fp, "tm_autostore=%d\n",prefs_get_tm_autostore());
    fprintf(fp, "pm_sort=%d\n",prefs_get_pm_sort());
    fprintf(fp, "st_sort=%d\n",prefs_get_st_sort());
    fprintf(fp, "tm_sort_=%d\n",prefs_get_tm_sort());
    fprintf(fp, "tm_sortcol=%d\n",prefs_get_tm_sortcol());
    fprintf(fp, "display_tooltips_main=%d\n",
	    prefs_get_display_tooltips_main());
    fprintf(fp, "display_tooltips_prefs=%d\n",
	    prefs_get_display_tooltips_prefs());
    fprintf(fp, "multi_edit=%d\n", prefs_get_multi_edit());
    fprintf(fp, "multi_edit_title=%d\n", prefs_get_multi_edit_title());
    fprintf(fp, "misc_track_nr=%d\n", prefs_get_misc_track_nr());
    fprintf(fp, "not_played_track=%d\n", prefs_get_not_played_track());
    fprintf(fp, "update_charset=%d\n",prefs_get_update_charset());
    fprintf(fp, "write_charset=%d\n",prefs_get_write_charset());
    fprintf(fp, "add_recursively=%d\n",prefs_get_add_recursively());
    fprintf(fp, "case_sensitive=%d\n",prefs_get_case_sensitive());
    fprintf (fp, "automount=%d\n", cfg->automount);
    fprintf (fp, "info_window=%d\n", cfg->info_window);
    fprintf (fp, "tmp_disable_sort=%d\n", cfg->tmp_disable_sort);
    fprintf (fp, "startup_messages=%d\n", cfg->startup_messages);
}


void
write_prefs (void)
{
    gchar *filename;
    gchar *cfgdir;
    FILE *fp = NULL;

    cfgdir = prefs_get_cfgdir ();

    filename = g_build_filename (cfgdir, "prefs", NULL);
    if((fp = fopen(filename, "a")))
    {
	write_prefs_to_file_desc(fp);
	fclose(fp);
    }
    else
    {
	gtkpod_warning (_("Unable to open '%s' for writing\n"),
			filename);
    }

    g_free (filename);
    g_free (cfgdir);
}



/* Free all memory including the cfg-struct itself. */
void discard_prefs ()
{
    cfg_free(cfg);
    cfg = NULL;
}

void cfg_free(struct cfg *c)
{
    if(c)
    {
      g_free (c->charset);
      g_free (c);
    }
}

void sortcfg_free(struct sortcfg *c)
{
    g_return_if_fail (c);
    g_list_free (c->tmp_sort_ign_fields);
    g_free (c->tmp_sort_ign_strings);
    g_free (c);
}


void prefs_set_offline(gboolean active)
{
  if (cfg->offline != active)   space_data_update ();
  cfg->offline = active;
  info_update_totals_view_space ();
}

/* If the status of md5 hash flag changes, free or re-init the md5
   hash table */
void prefs_set_md5tracks (gboolean active)
{
    struct itdbs_head *itdbs_head;

    g_return_if_fail (gtkpod_window);
    itdbs_head = g_object_get_data (G_OBJECT (gtkpod_window),
				    "itdbs_head");
/*    g_return_if_fail (itdbs_head);*/
    /* gets called before itdbs are set up -> fail silently */
    if (!itdbs_head)
    {
	cfg->md5tracks = active;
	return;
    }

    if (cfg->md5tracks && !active)
    { /* md5 checksums have been turned off */
	cfg->md5tracks = FALSE;
	gp_md5_free_hash ();
    }
    if (!cfg->md5tracks && active)
    { /* md5 checksums have been turned on */
	cfg->md5tracks = TRUE; /* must be set before calling
				  gp_md5_hash_tracks() */
	gp_md5_hash_tracks ();
	/* display duplicates */
	gp_duplicate_remove (NULL, NULL);
    }
}

gboolean prefs_get_md5tracks(void)
{
    return cfg->md5tracks;
}

/* Should the display be blocked (be insenstive) while it is updated
   after a selection (playlist, tab entry) change? */
void prefs_set_block_display(gboolean active)
{
    cfg->block_display = active;
}

gboolean prefs_get_block_display(void)
{
    return cfg->block_display;
}

gboolean prefs_get_offline(void)
{
  return cfg->offline;
}

void prefs_set_charset (gchar *charset)
{
    prefs_cfg_set_charset (cfg, charset);
}

void prefs_cfg_set_charset (struct cfg *cfgd, gchar *charset)
{
    C_FREE (cfgd->charset);
    if (charset && strlen (charset))
	cfgd->charset = g_strdup (charset);
/*     printf ("set_charset: '%s'\n", charset);	 */
}

gchar *prefs_get_charset (void)
{
    return cfg->charset;
/*     printf ("get_charset: '%s'\n", cfg->charset); */
}

struct cfg *clone_prefs(void)
{
    struct cfg *result = NULL;

    if(cfg)
    {
	result = g_memdup (cfg, sizeof (struct cfg));
	result->charset = g_strdup(cfg->charset);
    }
    return(result);
}

struct sortcfg *clone_sortprefs(void)
{
    struct sortcfg *result = NULL;

    if(cfg)
    {
	result = g_memdup (&cfg->sortcfg, sizeof (struct sortcfg));
    }
    /* GLists are not copied */
    result->tmp_sort_ign_fields = NULL;
    result->tmp_sort_ign_strings = NULL;
    return(result);
}

/* Returns "$HOME/.gtkpod" and tries to create it if it does not
   exist. */
gchar *prefs_get_cfgdir (void)
{
  G_CONST_RETURN gchar *str;
  gchar *cfgdir=NULL;

  if((str = g_get_home_dir ()))
  {
      cfgdir = g_build_filename (str, ".gtkpod", NULL);
      if(!g_file_test(cfgdir, G_FILE_TEST_IS_DIR))
      {
	  if(mkdir(cfgdir, 0777) == -1)
	  {
	      /* if this error occurs before we have initialized config
		 and display we crash --> resort to fprintf() */
	      if (cfg)
		  gtkpod_warning(_("Unable to 'mkdir %s'\n"), cfgdir);
	      else
		  fprintf(stderr, _("Unable to 'mkdir %s'\n"), cfgdir);
	  }
      }
  }
  return cfgdir;
}

/* A value of "0" will set the default defined in misc.c */
void prefs_set_statusbar_timeout (guint32 val)
{
    if (val == 0)  val = STATUSBAR_TIMEOUT;
    cfg->statusbar_timeout = val;
    gtkpod_statusbar_reset_timeout ();
}

guint32 prefs_get_statusbar_timeout (void)
{
    return cfg->statusbar_timeout;
}

gboolean prefs_get_display_toolbar (void)
{
    return cfg->display_toolbar;
}

void prefs_set_display_toolbar (gboolean val)
{
    cfg->display_toolbar = val;
    display_show_hide_toolbar ();
}

gboolean prefs_get_update_charset (void)
{
    return cfg->update_charset;
}

void prefs_set_update_charset (gboolean val)
{
    cfg->update_charset = val;
}

gboolean prefs_get_write_charset (void)
{
    return cfg->write_charset;
}

void prefs_set_write_charset (gboolean val)
{
    cfg->write_charset = val;
}

gboolean prefs_get_add_recursively (void)
{
    return cfg->add_recursively;
}

void prefs_set_add_recursively (gboolean val)
{
    cfg->add_recursively = val;
}

gboolean prefs_get_case_sensitive (void)
{
    return cfg->sortcfg.case_sensitive;
}

void prefs_set_case_sensitive (gboolean val)
{
    cfg->sortcfg.case_sensitive = val;
}

gboolean prefs_get_group_compilations (void)
{
    return cfg->group_compilations;
}

void prefs_set_group_compilations (gboolean val, gboolean update_display)
{
    cfg->group_compilations = val;
    if (update_display) st_show_visible ();
}

GtkToolbarStyle prefs_get_toolbar_style (void)
{
    return cfg->toolbar_style;
}

void prefs_set_toolbar_style (GtkToolbarStyle i)
{
    switch (i)
    {
    case GTK_TOOLBAR_ICONS:
    case GTK_TOOLBAR_TEXT:
    case GTK_TOOLBAR_BOTH:
	break;
    case GTK_TOOLBAR_BOTH_HORIZ:
	i = GTK_TOOLBAR_BOTH;
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning (_("prefs_set_toolbar_style: illegal style '%d' ignored\n"), i);
	return;
    }

    cfg->toolbar_style = i;
    display_show_hide_toolbar ();
}

gboolean prefs_get_tm_autostore (void)
{
    return cfg->sortcfg.tm_autostore;
}

void prefs_set_tm_autostore (gboolean val)
{
    cfg->sortcfg.tm_autostore = val;
}

gint prefs_get_pm_sort (void)
{
    return cfg->sortcfg.pm_sort;
}

void prefs_set_pm_sort (gint i)
{
    switch (i)
    {
    case SORT_ASCENDING:
    case SORT_DESCENDING:
    case SORT_NONE:
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning (_("prefs_set_pm_sort: illegal type '%d' ignored\n"), i);
	return;
    }

    cfg->sortcfg.pm_sort = i;
}

gint prefs_get_st_sort (void)
{
    return cfg->sortcfg.st_sort;
}

void prefs_set_st_sort (gint i)
{
    switch (i)
    {
    case SORT_ASCENDING:
    case SORT_DESCENDING:
    case SORT_NONE:
	cfg->sortcfg.st_sort = i;
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning ("Programming error: prefs_set_st_sort: illegal type '%d' ignored\n", i);
	break;
    }
}

gint prefs_get_tm_sort (void)
{
    return cfg->sortcfg.tm_sort;
}

void prefs_set_tm_sort (gint i)
{
    switch (i)
    {
    case SORT_ASCENDING:
    case SORT_DESCENDING:
    case SORT_NONE:
	cfg->sortcfg.tm_sort = i;
	break;
    default:  /* illegal -- ignore */
	gtkpod_warning ("Programming error: prefs_set_tm_sort: illegal type '%d' ignored\n", i);
	break;
    }

}

TM_item prefs_get_tm_sortcol (void)
{
    return cfg->sortcfg.tm_sortcol;
}

void prefs_set_tm_sortcol (TM_item i)
{
    if (i < TM_NUM_COLUMNS)
	cfg->sortcfg.tm_sortcol = i;
}

void prefs_set_display_tooltips_main (gboolean state)
{
    cfg->display_tooltips_main = state;
    display_show_hide_tooltips ();
}

gboolean prefs_get_display_tooltips_main (void)
{
    return cfg->display_tooltips_main;
}

void prefs_set_display_tooltips_prefs (gboolean state)
{
    cfg->display_tooltips_prefs = state;
    display_show_hide_tooltips ();
}

gboolean prefs_get_display_tooltips_prefs (void)
{
    return cfg->display_tooltips_prefs;
}

void prefs_set_multi_edit (gboolean state)
{
    cfg->multi_edit = state;
}

gboolean prefs_get_multi_edit (void)
{
    return cfg->multi_edit;
}

void prefs_set_not_played_track (gboolean state)
{
    cfg->not_played_track = state;
}

gboolean prefs_get_not_played_track (void)
{
    return cfg->not_played_track;
}

void prefs_set_misc_track_nr (gint state)
{
    cfg->misc_track_nr = state;
}

gint prefs_get_misc_track_nr (void)
{
    return cfg->misc_track_nr;
}

void prefs_set_multi_edit_title (gboolean state)
{
    cfg->multi_edit_title = state;
}

gboolean prefs_get_multi_edit_title (void)
{
    return cfg->multi_edit_title;
}

gint prefs_get_last_prefs_page (void)
{
    return cfg->last_prefs_page;
}

void prefs_set_last_prefs_page (gint i)
{
    cfg->last_prefs_page = i;
}

gboolean 
prefs_get_automount (void)
{
    return cfg->automount;
}

void
prefs_set_automount(gboolean val)
{
    cfg->automount = val;
}

gboolean
prefs_get_info_window(void)
{
    return cfg->info_window;
}
void

prefs_set_info_window(gboolean val)
{
    cfg->info_window = val;
}

gboolean prefs_get_tmp_disable_sort(void)
{
    return(cfg->tmp_disable_sort);
}

void prefs_set_tmp_disable_sort(gboolean val)
{
    cfg->tmp_disable_sort = val;
}

gboolean prefs_get_startup_messages(void)
{
    return(cfg->startup_messages);
}

void prefs_set_startup_messages(gboolean val)
{
    cfg->startup_messages = val;
}

/* sorting gets disabled temporarily if either of the options
   'tmp_disable_sort' or 'block_display' is checked */
gboolean prefs_get_disable_sorting(void)
{
    return (prefs_get_block_display() || prefs_get_tmp_disable_sort());
}

