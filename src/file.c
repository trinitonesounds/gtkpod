/* Time-stamp: <2004-01-17 17:47:31 jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
| 
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include "charset.h"
#include "confirmation.h"
#include "file.h"
#include "info.h"
#include "itunesdb.h"
#include "md5.h"
#include "misc.h"
#include "mp3file.h"
#include "mp4file.h"
#include "prefs.h"
#include "support.h"

/* only used when reading extended info from file */
struct track_extended_info
{
    guint ipod_id;
    gchar *pc_path_locale;
    gchar *pc_path_utf8;
    gchar *md5_hash;
    gchar *charset;
    gchar *hostname;
    gchar *ipod_path;
    gint32 oldsize;
    guint32 playcount;
    guint32 rating;
    gboolean transferred;
};
/* List with tracks pending deletion */
static GList *pending_deletion = NULL;
/* Flag to indicate if it's safe to quit (i.e. all tracks exported or
   at least a offline database written). It's state is set to TRUE in
   handle_export().  It's state can be accessed by the public function
   file_are_saved(). It can be set to FALSE by calling
   data_changed() */
static gboolean files_saved = TRUE;
#ifdef G_THREADS_ENABLED
/* Thread specific */
static  GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gboolean mutex_data = FALSE;
#endif 
/* Used to keep the "extended information" until the iTunesDB is 
   loaded */
static GHashTable *extendedinfohash = NULL;
static GHashTable *extendedinfohash_md5 = NULL;
static float extendedinfoversion = 0.0;


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Playlists                                               *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Add all files specified in playlist @plfile. Will create a new
 * playlist with the name "basename (plfile)", even if one of the same
 * name already exists (if @plitem is != NULL, all tracks will be added
 * to @plitem and no new playlist will be created).
 * It will then add all tracks listed in @plfile. If set in the prefs,
 * duplicates will be detected (and the track already present in the
 * database will be added to the playlist instead). */
/* @addtrackfunc: if != NULL this will be called instead of
   "add_track_to_playlist () -- used for dropping tracks at a specific
   position in the track view */
gboolean add_playlist_by_filename (gchar *plfile, Playlist *plitem,
				   AddTrackFunc addtrackfunc, gpointer data)
{
    enum {
	PLT_M3U,   /* M3U playlist file */
	PLT_PLS,   /* PLS playlist file */
	PLT_MISC   /* something else -- assume it works the same as M3U */
    };
    gchar *bufp, *plfile_utf8;
    gchar *dirname = NULL, *plname = NULL;
    gchar buf[PATH_MAX];
    gint type = PLT_MISC; /* type of playlist file */
    gint line, tracks;
    FILE *fp;
    gboolean error;

    if (!plfile)  return TRUE;

    if (g_file_test (plfile, G_FILE_TEST_IS_DIR))
    {
	/* FIXME: Status */
	return FALSE;  /* definitely not! */
    }

    plfile_utf8 = charset_to_utf8 (plfile);

    plname = g_path_get_basename (plfile_utf8);
    bufp = g_strrstr (plname, "."); /* find last occurence of '.' */
    if (bufp)
    {
	*bufp = 0;          /* truncate playlist name */
	++bufp;
	if (compare_string_case_insensitive (bufp, "m3u") == 0)
	    type = PLT_M3U;
	else if (compare_string_case_insensitive (bufp, "pls") == 0)
	    type = PLT_PLS;
	else if (compare_string_case_insensitive (bufp, "mp3") == 0)
	{
	    /* FIXME: Status */
	    g_free (plname);
	    return FALSE;  /* definitely not! */
	}
	else if (compare_string_case_insensitive (bufp, "wav") == 0)
	{
	    /* FIXME: Status */
	    g_free (plname);
	    return FALSE;  /* definitely not! */
	}
    }

    /* attempt to open playlist file */
    if (!(fp = fopen (plfile, "r")))
    {
	/* FIXME: Status */
	g_free (plname);
	return FALSE;  /* definitely not! */
    }
    /* create playlist (if none is specified) */
    if (!plitem)   plitem = add_new_playlist (plname, -1);
    C_FREE (plname);

    /* need dirname if playlist file contains relative paths */
    dirname = g_path_get_dirname (plfile);

    /* for now assume that all playlist files will be line-based
       all of these are line based -- add different code for different
       playlist files */
    line = 0; /* nr of line being read */
    tracks = 0; /* nr of tracks added */
    error = FALSE;
    while (!error && fgets (buf, PATH_MAX, fp))
    {
	gchar *bufp = buf;
	gchar *filename = NULL;
	gint len = strlen (bufp); /* remove newline */
	if((len>0) && (bufp[len-1] == 0x0a))  bufp[len-1] = 0;
	switch (type)
	{
	case PLT_MISC:
	    /* skip whitespace */
	    while (isspace (*bufp)) ++bufp;
	    /* assume comments start with ';' or '#' */
	    if ((*bufp == ';') || (*bufp == '#')) break;
	    /* assume the rest of the line is a filename */
	    filename = concat_dir_if_relative (dirname, bufp);
	    if (add_track_by_filename (filename, plitem,
				      prefs_get_add_recursively (),
				      addtrackfunc, data))
		++tracks;
	    break;
	case PLT_M3U:
	    /* comments start with '#' */
	    if (*bufp == '#') break;
	    /* assume the rest of the line is a filename */
	    filename = concat_dir_if_relative (dirname, bufp);
	    if (add_track_by_filename (filename, plitem,
				      prefs_get_add_recursively (),
				      addtrackfunc, data))
		++tracks;
	    break;
	case PLT_PLS:
	    /* I don't know anything about pls playlist files and just
	       looked at one example produced by xmms -- please
	       correct the code if you know more */ 
	    if (line == 0)
	    { /* check for "[playlist]" */
		if (strncasecmp (bufp, "[playlist]", 10) != 0)
		    error = TRUE;
	    }
	    else if (strncasecmp (bufp, "File", 4) == 0)
	    { /* looks like a file entry */
		bufp = strchr (bufp, '=');
		if (bufp) ++bufp;
		filename = concat_dir_if_relative (dirname, bufp);
		if (add_track_by_filename (filename, plitem,
					  prefs_get_add_recursively (),
					  addtrackfunc, data))
		    ++tracks;
	    }
	    break;
	}
	++line;
	g_free (filename);
    }
    fclose (fp);
    C_FREE (dirname);

    /* I don't think it's too interesting to pop up the list of
       duplicates -- but we should reset the list. */
    remove_duplicate (NULL, (void *)-1);
    return !error;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Dir                                                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Add all files in directory and subdirectories.
   If @name is a regular file, just add that.
   If @name == NULL, just return
   If @plitem != NULL, add tracks also to Playlist @plitem
   @descend: TRUE: add recursively
             FALSE: don't enter subdirectories */
/* Not nice: the return value has not much meaning. TRUE: all files
 * were added successfully. FALSE: some files could not be
   added (e.g: duplicates)  */
/* @addtrackfunc: if != NULL this will be called instead of
   "add_track_to_playlist () -- used for dropping tracks at a specific
   position in the track view */
gboolean add_directory_by_name (gchar *name, Playlist *plitem,
				gboolean descend,
				AddTrackFunc addtrackfunc, gpointer data)
{
  gboolean result = TRUE;

  if (name == NULL) return TRUE;
  if (g_file_test (name, G_FILE_TEST_IS_DIR))
  {
      GDir *dir = g_dir_open (name, 0, NULL);
      block_widgets ();
      if (dir != NULL) {
	  G_CONST_RETURN gchar *next;
	  do {
	      next = g_dir_read_name (dir);
	      if (next != NULL)
	      {
		  gchar *nextfull = g_build_filename (name, next, NULL);
		  if (descend ||
		      !g_file_test (nextfull, G_FILE_TEST_IS_DIR))
		      result &= add_directory_by_name (nextfull, plitem,
						       descend,
						       addtrackfunc, data);
		  g_free (nextfull);
	      }
	  } while (next != NULL);
	  g_dir_close (dir);
      }
      release_widgets ();
  }
  else
  {
      result = add_track_by_filename (name, plitem, descend, addtrackfunc, data);
  }
  return result;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Fill in track struct with data from file                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Used by set_entry_from_filename() and parse_filename() */
static void set_entry (gchar **entry_utf8, gunichar2 **entry_utf16, gchar *str)
{
  C_FREE (*entry_utf8);
  C_FREE (*entry_utf16);
  *entry_utf8 = str;
  *entry_utf16 = g_utf8_to_utf16 (*entry_utf8, -1, NULL, NULL, NULL);
}

static void parse_filename (Track *track)
{
    GList *tokens=NULL, *gl;
    const gchar *template;
    gchar *tpl, *fn;
    gchar *sps, *sp, *str;

    template = prefs_get_parsetags_template ();
    if (!template) return;
    if ((template[0] == '%') && (template[1] != '%'))
	 tpl = g_strdup_printf ("%c%s", G_DIR_SEPARATOR, template);
    else tpl = g_strdup (template);

    fn = g_strdup (track->pc_path_utf8);

    sps = tpl;
    while ((sp = strchr (sps, '%')))
    {
	if (sps != sp)
	    tokens = g_list_prepend (tokens, g_strndup (sps, sp-sps));
	if (sp[1] != '%')
	{
	    tokens = g_list_prepend (tokens, g_strndup (sp, 2));
	}
	else
	{
	    tokens = g_list_prepend (tokens, g_strdup ("%"));
	}
	if (!sp[1]) break;
	sps = sp+2;
    }
    if (sps[0] != 0)
	tokens = g_list_prepend (tokens, g_strdup (sps));

    str = g_list_nth_data (tokens, 0);
    if (str && (strchr (str, '.') == NULL))
    {
	gchar *str = strrchr (fn, '.');
	if (str) str[0] = 0;
    }

#ifdef DEBUG
    puts (tpl);
    for (gl=tokens; gl; gl=gl->next)
	puts (gl->data);
    puts (fn);
#endif

    gl = tokens;
    while (gl)
    {
	gchar *token = gl->data;
	if ((token[0] == '%') && (strlen (token) == 2))
	{   /* handle tag item */
	    GList *gln = gl->next;
	    if (gln)
	    {
		gchar *itm;
		gchar *next_token = gln->data;
		gchar *fnp = g_strrstr (fn, next_token);
		gboolean parse_error = FALSE;
		
		if (!fnp)   break;
		fnp[0] = 0;
		fnp = fnp + strlen (next_token);
#ifdef DEBUG
		printf ("%s: '%s'\n", token, fnp);
#endif
		itm = g_strdup (fnp);
		switch (token[1])
		{
		case 'a': /* artist */
		    if (!track->artist || prefs_get_parsetags_overwrite ())
			set_entry (&track->artist, &track->artist_utf16, itm);
		    break;
		case 'A': /* album */
		    if (!track->album || prefs_get_parsetags_overwrite ())
			set_entry (&track->album, &track->album_utf16, itm);
		    break;
		case 'c': /* composer */
		    if (!track->composer || prefs_get_parsetags_overwrite ())
			set_entry (&track->composer, &track->composer_utf16,
				   itm);
		    break;
		case 't': /* title */
		    if (!track->title || prefs_get_parsetags_overwrite ())
			set_entry (&track->title, &track->title_utf16, itm);
		    break;
		case 'g': /* genre */
		case 'G': /* genre */
		    if (!track->genre || prefs_get_parsetags_overwrite ())
			set_entry (&track->genre, &track->genre_utf16, itm);
		    break;
		case 'T': /* track */
		    if (track->track_nr == 0 
			|| prefs_get_parsetags_overwrite ())
			track->track_nr = atoi (itm);
		    g_free (itm);
		    break;
		case 'C': /* CD */
		    if (track->cd_nr == 0 || prefs_get_parsetags_overwrite ())
			track->cd_nr = atoi (itm);
		    g_free (itm);
		    break;
		case '*': /* placeholder to skip a field */
		    g_free (itm);
		    break;
		default:
		    g_free (itm);
		    parse_error = TRUE;
		}
		if (parse_error) break;
		gl = gln->next;
	    }
	    else break;
	}
	else
	{   /* skip text */
	    gchar *fnp = g_strrstr (fn, token);
	    if (!fnp)  break;  /* could not match */
	    if (fnp - fn + strlen (fnp) != strlen (fn))
		break; /* not at the last position */
	    fnp[0] = 0;
	    gl = gl->next;
	}
    }

    g_free (fn);
    g_free (tpl);
    g_list_foreach (tokens, (GFunc)g_free, NULL);
    g_list_free (tokens);
}


/* Set entry "column" (TM_COLUMN_TITLE etc) according to filename */
/* TODO: make the TAG extraction more intelligent -- if possible, this
   should be user configurable. */
static void set_entry_from_filename (Track *track, gint column)
{
    gchar *str;

    if (prefs_get_autosettags (column) &&
	track->pc_path_utf8 && strlen (track->pc_path_utf8))
    {
	switch (column)
	{
	case TM_COLUMN_TITLE:
	    str = g_path_get_basename (track->pc_path_utf8);
	    set_entry (&track->title, &track->title_utf16, str);
	    break;
	case TM_COLUMN_ALBUM:
	    str = g_path_get_basename (track->pc_path_utf8);
	    set_entry (&track->album, &track->album_utf16, str);
	    break;
	case TM_COLUMN_ARTIST:
	    str = g_path_get_basename (track->pc_path_utf8);
	    set_entry (&track->artist, &track->artist_utf16, str);
	    break;
	case TM_COLUMN_GENRE:
	    str = g_path_get_basename (track->pc_path_utf8);
	    set_entry (&track->genre, &track->genre_utf16, str);
	    break;
	case TM_COLUMN_COMPOSER:
	    str = g_path_get_basename (track->pc_path_utf8);
	    set_entry (&track->composer, &track->composer_utf16, str);
	    break;
	}
    }
}


static void set_unset_entries_from_filename (Track *track)
{
    /* try to fill tags from filename */
    if (prefs_get_parsetags ())
	parse_filename (track);
    /* fill up what is left unset */
    if (!track->album && prefs_get_autosettags (TM_COLUMN_ALBUM))
	set_entry_from_filename (track, TM_COLUMN_ALBUM);
    if (!track->artist && prefs_get_autosettags (TM_COLUMN_ARTIST))
	set_entry_from_filename (track, TM_COLUMN_ARTIST);
    if (!track->title && prefs_get_autosettags (TM_COLUMN_TITLE))
	set_entry_from_filename (track, TM_COLUMN_TITLE);
    if (!track->genre && prefs_get_autosettags (TM_COLUMN_GENRE))
	set_entry_from_filename (track, TM_COLUMN_GENRE);
    if (!track->composer && prefs_get_autosettags (TM_COLUMN_COMPOSER))
	set_entry_from_filename (track, TM_COLUMN_COMPOSER);
}


/* update the track->charset info with the currently used charset */
void update_charset_info (Track *track)
{
    if (track)
    {
	const gchar *charset = prefs_get_charset ();
	C_FREE (track->charset);
	if (!charset || !strlen (charset))
	{    /* use standard locale charset */
	    g_get_charset (&charset);
	}
	/* only set charset if it's not GTKPOD_JAPAN_AUTOMATIC */
	if (charset && (strcmp (charset, GTKPOD_JAPAN_AUTOMATIC) != 0))
	{
	    track->charset = g_strdup (charset);
	}
    }
}


/* Copy "new" info read from file to an old Track structure. If @to is
   NULL, nothing will be done.
   Return value: a pointer to the track the data was copied to. */
Track *copy_new_info (Track *from, Track *to)
{
    if (!from || !to) return NULL;

    C_FREE (to->album);
    C_FREE (to->artist);
    C_FREE (to->title);
    C_FREE (to->genre);
    C_FREE (to->comment);
    C_FREE (to->composer);
    C_FREE (to->fdesc);
    C_FREE (to->album_utf16);
    C_FREE (to->artist_utf16);
    C_FREE (to->title_utf16);
    C_FREE (to->genre_utf16);
    C_FREE (to->comment_utf16);
    C_FREE (to->composer_utf16);
    C_FREE (to->fdesc_utf16);
    C_FREE (to->pc_path_utf8);
    C_FREE (to->pc_path_locale);
    C_FREE (to->charset);
    /* copy strings */
    to->album = g_strdup (from->album);
    to->album_utf16 = utf16_strdup (from->album_utf16);
    to->artist = g_strdup (from->artist);
    to->artist_utf16 = utf16_strdup (from->artist_utf16);
    to->title = g_strdup (from->title);
    to->title_utf16 = utf16_strdup (from->title_utf16);
    to->genre = g_strdup (from->genre);
    to->genre_utf16 = utf16_strdup (from->genre_utf16);
    to->comment = g_strdup (from->comment);
    to->comment_utf16 = utf16_strdup (from->comment_utf16);
    to->composer = g_strdup (from->composer);
    to->composer_utf16 = utf16_strdup (from->composer_utf16);
    to->fdesc = g_strdup (from->fdesc);
    to->fdesc_utf16 = utf16_strdup (from->fdesc_utf16);
    to->pc_path_utf8 = g_strdup (from->pc_path_utf8);
    to->pc_path_locale = g_strdup (from->pc_path_locale);
    to->charset = g_strdup (from->charset);
    to->size = from->size;
    to->tracklen = from->tracklen;
    to->cd_nr = from->cd_nr;
    to->cds = from->cds;
    to->track_nr = from->track_nr;
    to->tracks = from->tracks;
    to->bitrate = from->bitrate;
    to->year = from->year;
    g_free (to->year_str);
    to->year_str = g_strdup_printf ("%d", to->year);

    return to;
}

/* Fills the supplied @or_track with data from the file @name. If
 * @or_track is NULL, a new track struct is created The entries
 * pc_path_utf8 and pc_path_locale are not changed if an entry already
 * exists */ 
/* turns NULL on error, a pointer to the Track otherwise */
Track *get_track_info_from_file (gchar *name, Track *orig_track)
{
    Track *track = NULL;
    Track *nti = NULL;
    gint len;

    if (!name) return NULL;

    if (g_file_test (name, G_FILE_TEST_IS_DIR)) return NULL;
    if (!g_file_test (name, G_FILE_TEST_EXISTS)) return NULL;

    /* reset the auto detection charset (see explanation in charset.c */
    charset_reset_auto ();

    /* check for filetype */
    len = strlen (name);
    if (len < 4) return NULL;

    if (strcasecmp (&name[len-4], ".mp3") == 0)
	nti = file_get_mp3_info (name);
    if (strcasecmp (&name[len-4], ".m4p") == 0)
	nti = file_get_mp4_info (name);
    if (strcasecmp (&name[len-4], ".m4a") == 0)
	nti = file_get_mp4_info (name);

    if (nti)
    {
	FILE *file;
	if (nti->charset == NULL)
	{   /* Fill in currently used charset. Try if auto_charset is
	     * set first. If not, use the currently set charset. */
	    nti->charset = charset_get_auto ();
	    if (nti->charset == NULL)
		update_charset_info (nti);
	}
	/* set path file information */
	nti->pc_path_utf8 = charset_to_utf8 (name);
	nti->pc_path_locale = g_strdup (name);
	/* set length of file */
	file = fopen (name, "r");
	if (file)
	{
	    fseek (file, 0, SEEK_END);
	    nti->size = ftell (file); /* get the filesize in bytes */
	    fclose(file);
	}
	if (nti->bitrate == 0)
	{  /* estimate bitrate */
	    if (nti->tracklen)
		nti->bitrate = nti->size * 8 / nti->tracklen;
	}
	/* Set unset strings (album...) from filename */
	set_unset_entries_from_filename (nti);
	/* Make sure all strings are initialized -- that way we don't 
	   have to worry about it when we are handling the
	   strings. Also, validate_entries() will fill in the utf16
	   strings if that hasn't already been done. */
	/* exception: md5_hash, charset and hostname: these may be NULL. */
	validate_entries (nti);

	if (orig_track)
	{ /* we need to copy all information over to the original
	   * track */
	    track = copy_new_info (nti, orig_track);
	    free_track (nti);
	    nti = NULL;
	}
	else
	{ /* just use nti */
	    track = nti;
	    nti = NULL;
	}
    }

    while (widgets_blocked && gtk_events_pending ())
	gtk_main_iteration ();

    return track;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Update Track Data from File                                  *
 *                                                                  *
\*------------------------------------------------------------------*/


/* reads info from file and updates the ID3 tags of
   @selected_trackids. */
void update_trackids (GList *selected_trackids)
{
    GList *gl_id;

    if (g_list_length (selected_trackids) == 0)
    {
	gtkpod_statusbar_message(_("Nothing to update"));
	return;
    }

    block_widgets ();
    for (gl_id = selected_trackids; gl_id; gl_id = gl_id->next)
    {
	guint32 id = (guint32)gl_id->data;
	Track *track = get_track_by_id (id);
	gchar *buf = g_strdup_printf (_("Updating %s"), get_track_info (track));
	gtkpod_statusbar_message (buf);
	g_free (buf);
	update_track_from_file (track);
    }
    release_widgets ();
    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log of updated tracks */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    remove_duplicate (NULL, NULL);
    gtkpod_statusbar_message(_("Updated selected tracks with info from file."));
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Sync Track Dirs                                              *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Callback for adding tracks (makes sure track isn't added to playlist
 * again if it already exists */
static void sync_addtrackfunc (Playlist *plitem, Track *track, gpointer data)
{
    if (!track) return;
    if (!plitem) plitem = get_playlist_by_nr (0); /* NULL/0: MPL */

    /* only add if @track isn't already a member of the current
       playlist */
    if (!g_list_find (plitem->members, track))
	add_track_to_playlist (plitem, track, TRUE);
}


/* Synchronize the directory @key (gchar *dir). Called by
 * sync_trackids().
 * @user_data: selected playlist */
static void sync_dir (gpointer key,
		      gpointer value,
		      gpointer user_data)
{
    gchar *dir = (gchar *)key;
    gchar *charset = (gchar *)value;
    Playlist *pl = (Playlist *)user_data;
    gchar *buf = NULL;
    gchar *dir_utf8 = NULL;
    /* old charset */
    gchar *prefs_charset = NULL;

    /* Assertions */
    if (!dir) return;
    if (!g_file_test (dir, G_FILE_TEST_IS_DIR))  return;

    /* change default charset if prefs don't say otherwise */
    if (!prefs_get_update_charset () && charset)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used for the directory */
	prefs_set_charset (charset);
    }	

    /* report status */
    /* convert dir to UTF8. Use @charset if specified */
    if (charset)
	dir_utf8 = charset_to_charset (charset, "UTF8", dir);
    else
	dir_utf8 = charset_to_utf8 (dir);
    buf = g_strdup_printf (_("Syncing directory '%s'"), dir_utf8);
    gtkpod_statusbar_message (buf);
    g_free (buf);
    g_free (dir_utf8);

    /* sync dir */
    add_directory_by_name (dir, pl, FALSE, sync_addtrackfunc, NULL);

    /* reset charset */
    if (!prefs_get_update_charset () && charset)
	prefs_set_charset (prefs_charset);
    g_free (prefs_charset);
}


/* ok handler for sync_remove */
/* @user_data1 is NULL, @user_data2 are the selected trackids */
static void sync_remove_ok (gpointer user_data1, gpointer user_data2)
{
    if (prefs_get_sync_remove ())
	delete_track_ok (NULL, user_data2);
}


/* cancel handler for sync_remove */
/* @user_data1 is NULL, @user_data2 are the selected trackids */
static void sync_remove_cancel (gpointer user_data1, gpointer user_data2)
{
    GList *id_list = user_data2;

    g_list_free (id_list);

    gtkpod_statusbar_message (_("Syncing completed. No files deleted."));
}


static void sync_dir_ok (gpointer user_data1, gpointer user_data2)
{
    GHashTable *hash = (GHashTable *)user_data1;
    /* currently selected playlist */
    Playlist *playlist = (Playlist *)user_data2;
    /* state of "update existing" */
    gboolean update;

    /* assertion */
    if (!hash) return;

    /* set "update existing" to TRUE */
    update = prefs_get_update_existing();
    prefs_set_update_existing (TRUE);

    /* sync all dirs stored in the hash */
    g_hash_table_foreach (hash, sync_dir, playlist);

    /* reset "update existing" */
    prefs_set_update_existing (update);

    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log updated tracks */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    remove_duplicate (NULL, NULL);

    if (prefs_get_sync_remove ())
    {   /* remove tracks that are no longer present in the dirs */
	GList *id_list = NULL;
	GString *str;
	gchar *label, *title;
	Track *s;
	GList *l;

	/* add all tracks that are no longer present in the dirs */
	for (s=get_next_track (0); s; s=get_next_track (1))
	{
	    if (s->pc_path_locale && *s->pc_path_locale)
	    {
		gchar *dirname = g_path_get_dirname (s->pc_path_locale);
		if (g_hash_table_lookup (hash, dirname) &&
		    !g_file_test (s->pc_path_locale, G_FILE_TEST_EXISTS))
		{
		    id_list = g_list_append (id_list, (gpointer)s->ipod_id);
		}
		g_free (dirname);
	    }
	}
	if (g_list_length (id_list) > 0)
	{
	    /* populate "title" and "label" */
	    delete_populate_settings (NULL, id_list,
				      &label, &title,
				      NULL, NULL, NULL);
	    /* create a list of tracks */
	    str = g_string_sized_new (2000);
	    for(l = id_list; l; l=l->next)
	    {
		s = get_track_by_id ((guint32)l->data);
		if (s)
		    g_string_append_printf (str, "%s (%s-%s)\n",
					    s->pc_path_utf8,
					    s->artist, s->title);
	    }
	    /* open window */
	    gtkpod_confirmation
		(-1,                   /* gint id, */
		 FALSE,                /* gboolean modal, */
		 title,                /* title */
		 label,                /* label */
		 str->str,             /* scrolled text */
		 _("Never delete any files when syncing"),/* opt 1 text */
		 CONF_STATE_INVERT_FALSE,                 /* opt 1 state */
		 prefs_set_sync_remove,                   /* opt 1 handler */
		 NULL, 0, NULL,        /* option 2 */
		 prefs_get_sync_remove_confirm(), /* gboolean confirm_again, */
		 prefs_set_sync_remove_confirm,   /* confirm_again_handler,*/
		 sync_remove_ok,       /* ConfHandler ok_handler,*/
		 CONF_NO_BUTTON,       /* don't show "Apply" button */
		 sync_remove_cancel,   /* cancel_handler,*/
		 NULL,                 /* gpointer user_data1,*/
		 id_list);             /* gpointer user_data2,*/

	    g_free (label);
	    g_string_free (str, TRUE);
	}
	else sync_remove_cancel (hash, id_list);
    }
    else
    {
	gtkpod_statusbar_message (_("Syncing completed."));
    }
    /* destroy the hash table (and free the memory taken by the
       dirnames */
    g_hash_table_destroy (hash);
}


/* sync dirs cancelled */
static void sync_dir_cancel (gpointer user_data1, gpointer user_data2)
{
    if (user_data1)  g_hash_table_destroy ((GHashTable *)user_data1);
    gtkpod_statusbar_message (_("Syncing aborted"));
}


/* Append @key (gchar *dir) to GString *user_data. Called by
 * sync_trackids(). Charset to use is @value. If @value is not set, use
 * the charset specified in the preferences */
static void sync_add_dir_to_string  (gpointer key,
				     gpointer value,
				     gpointer user_data)
{
    gchar *dir = (gchar *)key;
    gchar *charset = (gchar *)value;
    GString *str = (GString *)user_data;
    gchar *buf;

    if (!dir || !str) return;

    /* convert to UTF8. Use @charset if specified */
    if (charset)
	buf = charset_to_charset (charset, "UTF8", dir);
    else
	buf = charset_to_utf8 (dir);
    g_string_append_printf (str, "%s\n", buf);
    g_free (buf);
}


/* Sync all directories referred to by the pc_path_locale entries in
   the selected tracks */
void sync_trackids (GList *selected_trackids)
{
    GHashTable *hash;
    GList *gl_id;
    guint32 dirnum = 0;

    if (g_list_length (selected_trackids) == 0)
    {
	gtkpod_statusbar_message (_("No tracks in selection"));
	return;
    }

    /* Create a hash to keep the directory names ("key", and "value"
       to be freed with g_free). key is dirname, value is charset used
       */
    hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    /* Get the dirs of all tracks selected and enter them into the hash
       table if the dir exists. Using a hash table automatically
       removes duplicates */
    for (gl_id = selected_trackids; gl_id; gl_id = gl_id->next)
    {
	guint32 id = (guint32)gl_id->data;
	Track *track = get_track_by_id (id);
	if (track && track->pc_path_locale && *track->pc_path_locale)
	{
	    gchar *dirname = g_path_get_dirname (track->pc_path_locale);

	    ++dirnum;
	    if (g_file_test (dirname, G_FILE_TEST_IS_DIR))
	    {
		if (track->charset && *track->charset)
		{   /* charset set -- just insert the entry */
		    gchar *charset = g_strdup (track->charset);
		    g_hash_table_insert (hash, dirname, charset);
		}
		else
		{   /* no charset -- make sure we don't replace a dir
		     * entry that had the charset set */
		    if (!g_hash_table_lookup (hash, dirname))
			g_hash_table_insert (hash, dirname, NULL);
		}
	    }
	    else
	    {
		if (g_file_test (dirname, G_FILE_TEST_EXISTS))
		    gtkpod_warning (_("'%s' is not a directory. Ignored.\n"),
				    dirname);
		else
		    gtkpod_warning (_("'%s' does not exist. Ignored.\n"),
				    dirname);
	    }
	    g_free (dirname);
	}
    }
    if (g_hash_table_size (hash) == 0)
    {   /* no directory names available */
	if (dirnum == 0) gtkpod_warning (_("\
No directory names were stored. Make sure that you enable 'Write extended information' in the Export section of the preferences at the time of importing files.\n\
\n\
To synchronize directories now, activate the duplicate detection ('Don't allow file duplication') in the Import section and add the directories you want to sync again.\n"));
	else	         gtkpod_warning (_("\
No valid directories have been found. Sync aborted.\n"));
    }
    else
    {
	GString *str = g_string_sized_new (2000);
	/* build a string with all directories in the hash */
	g_hash_table_foreach (hash, sync_add_dir_to_string, str);
	if (!gtkpod_confirmation (
		-1,                     /* gint id, */
		FALSE,                  /* gboolean modal, */
		_("Synchronize directories"), /* title */
		_("OK to synchronize the following directories?"),
		str->str,               /* text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		prefs_get_show_sync_dirs(),/* gboolean confirm_again, */
		prefs_set_show_sync_dirs,/* confirm_again_handler*/
		sync_dir_ok,            /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,         /* don't show "Apply" */
		sync_dir_cancel,        /* cancel_handler,*/
		hash,                   /* gpointer user_data1,*/
		pm_get_selected_playlist())) /* gpointer user_data2,*/
	{ /* creation failed */
	    g_hash_table_destroy (hash);
	}
	g_string_free (str, TRUE);
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *  Generic functions to "do" things on selected playlist / entry   *
 *  / tracks                                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Make a list of all selected tracks and call @do_func with that list
   as argument */
void do_selected_tracks (void (*do_func)(GList *trackids))
{
    GList *selected_trackids = NULL;

    if (!do_func) return;

    /* I'm using ids instead of "Track *" -pointer because it would be
     * possible that a track gets removed during the process */
    selected_trackids = tm_get_selected_trackids();
    do_func (selected_trackids);
    g_list_free (selected_trackids);
}


/* Make a list of all tracks in the currently selected entry of sort
   tab @inst and call @do_func with that list as argument */
void do_selected_entry (void (*do_func)(GList *trackids), gint inst)
{
    GList *selected_trackids = NULL;
    TabEntry *entry;
    GList *gl;

    if (!do_func) return;

    if ((inst < 0) || (inst > prefs_get_sort_tab_num ()))   return;

    entry = st_get_selected_entry (inst);
    if (entry == NULL)
    {  /* no entry selected */
	gtkpod_statusbar_message (_("No entry selected."));
	return;
    }
    for (gl=entry->members; gl; gl=gl->next)
    { /* make a list with all trackids in this entry */
	Track *track = (Track *)gl->data;
	if (track)
	    selected_trackids = g_list_append (selected_trackids,
					      (gpointer)track->ipod_id);
    }
    do_func (selected_trackids);
    g_list_free (selected_trackids);
}


/* Make a list of the tracks in the current playlist and call @do_func
   with that list as argument */
void do_selected_playlist (void (*do_func)(GList *trackids))
{
    GList *selected_trackids = NULL;
    Playlist *pl;
    GList *gl;

    if (!do_func) return;

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    for (gl=pl->members; gl; gl=gl->next)
    { /* make a list with all trackids in this entry */
	Track *track = (Track *)gl->data;
	if (track)
	    selected_trackids = g_list_append (selected_trackids,
					      (gpointer)track->ipod_id);
    }
    do_func (selected_trackids);
    g_list_free (selected_trackids);
}


/* Logs tracks (@track) that could not be updated from file for some
   reason. Pop up a window with the log by calling with @track = NULL,
   or remove the log by calling with @track = -1.
   @txt (if available)w is added as an explanation as to why it was
   impossible to update the track */
void display_non_updated (Track *track, gchar *txt)
{
   gchar *buf;
   static gint track_nr = 0;
   static GString *str = NULL;

   if ((track == NULL) && str)
   {
       if (prefs_get_show_non_updated() && str->len)
       { /* Some tracks have not been updated. Print a notice */
	   buf = g_strdup_printf (
	       ngettext ("The following track could not be updated",
			 "The following %d tracks could not be updated",
			 track_nr), track_nr);
	   gtkpod_confirmation
	       (-1,                 /* gint id, */
		FALSE,              /* gboolean modal, */
		_("Failed Track Update"),   /* title */
		buf,                /* label */
		str->str,           /* scrolled text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		TRUE,               /* gboolean confirm_again, */
		prefs_set_show_non_updated,/* confirm_again_handler,*/
		NULL,               /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,     /* don't show "Apply" button */
		CONF_NO_BUTTON,     /* cancel_handler,*/
		NULL,               /* gpointer user_data1,*/
		NULL);              /* gpointer user_data2,*/
	   g_free (buf);
       }
       display_non_updated ((void *)-1, NULL);
   }

   if (track == (void *)-1)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       track_nr = 0;
       gtkpod_tracks_statusbar_update();
   }
   else if (prefs_get_show_non_updated() && track)
   {
       /* add info about it to str */
       buf = get_track_info (track);
       if (!str)
       {
	   track_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record of
					     * non-updated tracks */
       }
       if (txt) g_string_append_printf (str, "%s (%s)\n", buf, txt);
       else     g_string_append_printf (str, "%s\n", buf);
       g_free (buf);
       ++track_nr; /* count tracks */
   }
}


/* Logs tracks (@track) that could be updated from file. Pop up a window
   with the log by calling with @track = NULL, or remove the log by
   calling with @track = -1.
   @txt (if available)w is added as an explanation as to why it was
   impossible to update the track */
void display_updated (Track *track, gchar *txt)
{
   gchar *buf;
   static gint track_nr = 0;
   static GString *str = NULL;

   if (prefs_get_show_updated() && (track == NULL) && str)
   {
       if (str->len)
       { /* Some tracks have been updated. Print a notice */
	   buf = g_strdup_printf (
	       ngettext ("The following track has been updated",
			 "The following %d tracks have been updated",
			 track_nr), track_nr);
	   gtkpod_confirmation
	       (-1,                 /* gint id, */
		FALSE,              /* gboolean modal, */
		_("Successful Track Update"),   /* title */
		buf,                /* label */
		str->str,           /* scrolled text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		TRUE,               /* gboolean confirm_again, */
		prefs_set_show_updated,/* confirm_again_handler,*/
		NULL,               /* ConfHandler ok_handler,*/
		CONF_NO_BUTTON,     /* don't show "Apply" button */
		CONF_NO_BUTTON,     /* cancel_handler,*/
		NULL,               /* gpointer user_data1,*/
		NULL);              /* gpointer user_data2,*/
	   g_free (buf);
       }
       display_updated ((void *)-1, NULL);
   }

   if (track == (void *)-1)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       track_nr = 0;
       gtkpod_tracks_statusbar_update();
   }
   else if (prefs_get_show_updated() && track)
   {
       /* add info about it to str */
       buf = get_track_info (track);
       if (!str)
       {
	   track_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record of
					     * non-updated tracks */
       }
       if (txt) g_string_append_printf (str, "%s (%s)\n", buf, txt);
       else     g_string_append_printf (str, "%s\n", buf);
       g_free (buf);
       ++track_nr; /* count tracks */
   }
}


/* Update information of @track from data in original file. This
   requires that the original filename is available, and that the file
   exists.

   A list of non-updated tracks can be displayed by calling
   display_non_updated (NULL, NULL). This list can be deleted by
   calling display_non_updated ((void *)-1, NULL);

   It is also possible that duplicates get detected in the process --
   a list of those can be displayed by calling "remove_duplicate
   (NULL, NULL)", that list can be deleted by calling
   "remove_duplicate (NULL, (void *)-1)"*/
void update_track_from_file (Track *track)
{
    Track *oldtrack;
    gchar *prefs_charset = NULL;
    gchar *trackpath = NULL;
    gint32 oldsize = 0;
    gboolean charset_set;

    if (!track) return;

    /* remember size of track on iPod */
    if (track->transferred) oldsize = track->size;
    else                    oldsize = 0;

    /* remember if charset was set */
    if (track->charset)  charset_set = TRUE;
    else                 charset_set = FALSE;

    if (!prefs_get_update_charset () && charset_set)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used when first importing the track */
	prefs_set_charset (track->charset);
    }

    if (track->pc_path_locale)
    {   /* need to copy because we cannot pass track->pc_path_locale to
	get_track_info_from_file () since track->pc_path gets g_freed
	there */
	trackpath = g_strdup (track->pc_path_locale);
    }

    if (!(track->pc_path_locale && *track->pc_path_locale))
    { /* no path available */
	display_non_updated (track, _("no filename available"));
    }
    else if (get_track_info_from_file (trackpath, track))
    { /* update successfull */
	/* remove track from md5 hash and reinsert it
	   (hash value may have changed!) */
	gchar *oldhash = track->md5_hash;
	md5_track_removed (track);
	track->md5_hash = NULL;  /* need to remove the old value manually! */
	oldtrack = md5_track_exists_insert (track);
	if (oldtrack) { /* track exists, remove old track
			  and register the new version */
	    md5_track_removed (oldtrack);
	    remove_duplicate (track, oldtrack);
	    md5_track_exists_insert (track);
	}
	/* track may have to be copied to iPod on next export */
	/* since it will copied under the same name as before, we
	   don't have to manually remove it */
	if (oldhash && track->md5_hash)
	{   /* do we really have to copy the track again? */
	    if (strcmp (oldhash, track->md5_hash) != 0)
	    {
		track->transferred = FALSE;
		data_changed ();
	    }
	}
	else
	{   /* no hash available -- copy! */
	    track->transferred = FALSE;
	    data_changed ();
	}
	/* set old size if track has to be transferred (for free space
	 * calculation) */
	if (!track->transferred) track->oldsize = oldsize;
	/* notify display model */
	pm_track_changed (track);
	display_updated (track, NULL);
        C_FREE (oldhash);
    }
    else
    { /* update not successful -- log this track for later display */
	if (g_file_test (trackpath,
			 G_FILE_TEST_EXISTS) == FALSE)
	{
	    display_non_updated (track, _("file not found"));
	}
	else
	{
	    display_non_updated (track, _("format not supported"));
	}
    }

    if (!prefs_get_update_charset () && charset_set)
    {   /* reset charset */
	prefs_set_charset (prefs_charset);
    }
    C_FREE (trackpath);

    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add File                                                    *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Append file @name to the list of tracks.
   @name is in the current locale
   @plitem: if != NULL, add track to plitem as well (unless it's the MPL)
   descend: TRUE:  add directories recursively
            FALSE: add contents of directories passed but don't descend
                   into its subdirectories */
/* Not nice: currently only accepts files ending on .mp3 */
/* @addtrackfunc: if != NULL this will be called instead of
   "add_track_to_playlist () -- used for dropping tracks at a specific
   position in the track view */
gboolean add_track_by_filename (gchar *name, Playlist *plitem, gboolean descend,
			       AddTrackFunc addtrackfunc, gpointer data)
{
  static gint count = 0; /* do a gtkpod_tracks_statusbar_update() every
			    10 tracks */
  Track *oldtrack;
  gchar str[PATH_MAX];
  gchar *basename, *suffix;

  if (name == NULL) return TRUE;

  if (g_file_test (name, G_FILE_TEST_IS_DIR))
  {
      return add_directory_by_name (name, plitem, descend, addtrackfunc, data);
  }

  /* check if file is a playlist */
  suffix = strrchr (name, '.');
  if (suffix)
  {
      if ((strcasecmp (suffix, ".pls") == 0) ||
	  (strcasecmp (suffix, ".m3u") == 0))
      {
	  return add_playlist_by_filename (name, plitem, addtrackfunc, data);
      }
  }

  /* print a message about which file is being processed */
  basename = g_path_get_basename (name);
  if (basename)
  {
      gchar *bn_utf8 = charset_to_utf8 (basename);
      snprintf (str, PATH_MAX, _("Processing '%s'..."), bn_utf8);
      gtkpod_statusbar_message (str);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
      g_free (bn_utf8);
  }
  C_FREE (basename);

  /* Check if there exists already a track with the same filename */
  oldtrack = get_track_by_filename (name);
  /* If a track already exists in the database, either update it or
     just add it to the current playlist (if it doesn't already exist) */
  if (oldtrack)
  {
      if (prefs_get_update_existing ())
      {   /* update the information */
	  update_track_from_file (oldtrack);
      }
      /* add to current playlist if it's not already in there */
      if (plitem && (plitem->type != PL_TYPE_MPL))
      {
	  if (!track_is_in_playlist (plitem, oldtrack))
	  {
	      if (addtrackfunc)
		  addtrackfunc (plitem, oldtrack, data);
	      else
		  add_track_to_playlist (plitem, oldtrack, TRUE);
	  }
      }
  }
  else  /* oldtrack == NULL */
  {  /* Only read the new track if there doesn't already exist an old
	track with the same filename in the database */
      Track *track = get_track_info_from_file (name, NULL);
      if (track)
      {
	  Track *added_track = NULL;

	  track->ipod_id = 0;
	  track->transferred = FALSE;
	  if (gethostname (str, PATH_MAX-2) == 0)
	  {
	      str[PATH_MAX-1] = 0;
	      track->hostname = g_strdup (str);
	  }
	  /* add_track may return pointer to a different track if an
	     identical one (MD5 checksum) was found */
	  added_track = add_track (track);
	  if(added_track)                   /* add track to memory */
	  {
	      if (addtrackfunc)
	      {
		  if (!plitem || (plitem->type == PL_TYPE_MPL))
		  {   /* add track to master playlist (if it hasn't been
		       * done before) */
		      if (added_track == track) 
			  addtrackfunc (plitem, added_track, data);
		  }
		  else
		  {   /* (plitem != NULL) && (type == NORM) */
		      /* add track to master playlist (if it hasn't been
		       * done before) */
		      if (added_track == track)
			  add_track_to_playlist (NULL, added_track, TRUE);
		      /* add track to specified playlist */
		      addtrackfunc (plitem, added_track, data);
		  }
	      }
	      else  /* no addtrackfunc */
	      {
		  /* add track to master playlist (if it hasn't been done before) */
		  if (added_track == track) add_track_to_playlist (NULL, added_track,
								   TRUE);
		  /* add track to specified playlist, but not to MPL */
		  if (plitem && (plitem->type != PL_TYPE_MPL))
		      add_track_to_playlist (plitem, added_track, TRUE);
	      }
	      /* indicate that non-transferred files exist */
	      data_changed ();
	      ++count;
	      if (count >= 10)  /* update every ten tracks added */
	      {
		  gtkpod_tracks_statusbar_update();
		  count = 0;
	      }
	  }
      }
  }
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return TRUE;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Write Tags                                                  *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Call the correct tag writing function for the filename @name */
static gboolean file_write_info (gchar *name, Track *track)
{
    if (name && track)
    {
	gchar *suff = strrchr (name, '.');
	if (suff)
	{
	    if (strcasecmp (suff, ".mp3") == 0)
		return file_write_mp3_info (name, track);
	    if (strcasecmp (suff, ".m4a") == 0)
		return file_write_mp4_info (name, track);
	    if (strcasecmp (suff, ".m4p") == 0)
		return file_write_mp4_info (name, track);
	}
    }
    return FALSE;
}


/* Write tags to file */
gboolean write_tags_to_file (Track *track)
{
    gchar *ipod_fullpath;
    gchar *prefs_charset = NULL;
    Track *oldtrack;
    gboolean track_charset_set;

    if (!track) return FALSE;

    /* if we are to use the charset used when first importing
       the track, change the prefs settings temporarily */
    if (track->charset)  track_charset_set = TRUE;
    else                track_charset_set = FALSE;
    if (!prefs_get_write_charset () && track_charset_set)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used when first importing the track */
	prefs_set_charset (track->charset);
    }
    else
    {   /* we should update the track->charset information */
	update_charset_info (track);
    }

    if (track->pc_path_locale && (strlen (track->pc_path_locale) > 0))
    {
	if (file_write_info (
		track->pc_path_locale, track) == FALSE)
	{
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    track->pc_path_locale);
	}
    }
    if (!prefs_get_offline () &&
	track->transferred &&
	track->ipod_path &&
	(g_utf8_strlen (track->ipod_path, -1) > 0))
    {
	/* need to get ipod filename */
	ipod_fullpath = get_track_name_on_ipod (track);
	if (file_write_info (
		track->pc_path_locale, track) == FALSE)
	{
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    ipod_fullpath);
	}
	g_free (ipod_fullpath);
    }
    /* remove track from md5 hash and reinsert it (hash value has changed!) */
    md5_track_removed (track);
    C_FREE (track->md5_hash);  /* need to remove the old value manually! */
    oldtrack = md5_track_exists_insert (track);
    if (oldtrack) { /* track exists, remove and register the new version */
	md5_track_removed (oldtrack);
	remove_duplicate (track, oldtrack);
	md5_track_exists_insert (track);
    }

    if (!prefs_get_write_charset () && track_charset_set)
    {   /* reset charset */
	prefs_set_charset (prefs_charset);
    }
    g_free (prefs_charset);
    return TRUE;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Import of iTunesDB                                   *
 *                                                                  *
\*------------------------------------------------------------------*/


/* fills in extended info if available (called from add_track()) */
void fill_in_extended_info (Track *track)
{
  gint ipod_id=0;
  struct track_extended_info *sei=NULL;

  if (extendedinfohash && track->ipod_id)
  {
      ipod_id = track->ipod_id;
      sei = g_hash_table_lookup (extendedinfohash, &ipod_id);
  }
  if (extendedinfohash_md5)
  {
      if (!track->md5_hash)
      {
	  gchar *filename = get_track_name_on_ipod (track);
	  track->md5_hash = md5_hash_on_file_name (filename);
	  g_free (filename);
      }
      if (track->md5_hash)
      {
	  sei = g_hash_table_lookup (extendedinfohash_md5, track->md5_hash);
      }
  }
  if (sei) /* found info for this id! */
  {
      if (sei->pc_path_locale && !track->pc_path_locale)
	  track->pc_path_locale = g_strdup (sei->pc_path_locale);
      if (sei->pc_path_utf8 && !track->pc_path_utf8)
	  track->pc_path_utf8 = g_strdup (sei->pc_path_utf8);
      if (sei->md5_hash && !track->md5_hash)
	  track->md5_hash = g_strdup (sei->md5_hash);
      if (sei->charset && !track->charset)
	  track->charset = g_strdup (sei->charset);
      if (sei->hostname && !track->hostname)
	  track->hostname = g_strdup (sei->hostname);
      track->oldsize = sei->oldsize;
      track->playcount += sei->playcount;
      /* FIXME: This means that the rating can never be reset to 0
       * by the iPod */
      if (track->rating == 0)
	  track->rating = sei->rating;
      track->transferred = sei->transferred;
      /* don't remove the md5-hash -- there may be duplicates... */
      if (extendedinfohash)
	  g_hash_table_remove (extendedinfohash, &ipod_id);
  }
}


/* Used to free the memory of hash data */
static void hash_delete (gpointer data)
{
    struct track_extended_info *sei = data;

    if (sei)
    {
	C_FREE (sei->pc_path_locale);
	C_FREE (sei->pc_path_utf8);
	C_FREE (sei->md5_hash);
	C_FREE (sei->charset);
	C_FREE (sei->hostname);
	C_FREE (sei->ipod_path);
	g_free (sei);
    }
}

static void destroy_extendedinfohash (void)
{
    if (extendedinfohash)
	g_hash_table_destroy (extendedinfohash);
    if (extendedinfohash_md5)
	g_hash_table_destroy (extendedinfohash_md5);
    extendedinfohash = NULL;
    extendedinfohash_md5 = NULL;
    extendedinfoversion = 0.0;
}

/* Read extended info from "name" and check if "itunes" is the
   corresponding iTunesDB (using the itunes_hash value in "name").
   The data is stored in a hash table with the ipod_id as key.
   This hash table is used by add_track() to fill in missing information */
/* Return TRUE on success, FALSE otherwise */
static gboolean read_extended_info (gchar *name, gchar *itunes)
{
    gchar *md5, buf[PATH_MAX], *arg, *line, *bufp;
    gboolean success = TRUE;
    gboolean expect_hash, hash_matched=FALSE;
    gint len;
    struct track_extended_info *sei = NULL;
    FILE *fp;


    fp = fopen (name, "r");
    if (!fp)
    {
	gtkpod_warning (_("Could not open \"%s\" for reading extended info.\n"),
			name);
	return FALSE;
    }
    md5 = md5_hash_on_file_name (itunes);
    if (!md5)
    {
	g_warning ("Programming error: Could not create hash value from itunesdb\n");
	fclose (fp);
	return FALSE;
    }
    /* Create hash table */
    destroy_extendedinfohash ();
    expect_hash = TRUE; /* next we expect the hash value (checksum) */
    while (success && fgets (buf, PATH_MAX, fp))
    {
	/* allow comments */
	if ((buf[0] == ';') || (buf[0] == '#')) continue;
	arg = strchr (buf, '=');
	if (!arg || (arg == buf))
	{
	    gtkpod_warning (_("Error while reading extended info: %s\n"), buf);
	    continue;
	}
	/* skip whitespace (isblank() is a GNU extension... */
	bufp = buf;
	while ((*bufp == ' ') || (*bufp == 0x09)) ++bufp;
	line = g_strndup (buf, arg-bufp);
	++arg;
	len = strlen (arg); /* remove newline */
	if((len>0) && (arg[len-1] == 0x0a))  arg[len-1] = 0;
	if (expect_hash)
	{
	    if(g_ascii_strcasecmp (line, "itunesdb_hash") == 0)
	    {
		if (strcmp (arg, md5) != 0)
		{
		    hash_matched = FALSE;
		    gtkpod_warning (_("iTunesDB '%s' does not match checksum in extended information file '%s'\ngtkpod will try to match the information using MD5 checksums. If successful, this may take a while.\n\n"), itunes, name);
/*		    success = FALSE;
		    break;*/
		}
		else
		{
		    hash_matched = TRUE;
		}
		expect_hash = FALSE;
	    }
	    else
	    {
		gtkpod_warning (_("%s:\nExpected \"itunesdb_hash=\" but got:\"%s\"\n"), name, buf);
		success = FALSE;
		break;
	    }
	}
	else
	    if(g_ascii_strcasecmp (line, "id") == 0)
	    { /* found new id */
		if (sei)
		{
		    if (sei->ipod_id != 0)
		    { /* normal extended information */
			if (hash_matched)
			{
			    if (!extendedinfohash)
			    {
				extendedinfohash = g_hash_table_new_full (
				    g_int_hash,g_int_equal, NULL,hash_delete);
			    }
			    g_hash_table_insert (extendedinfohash,
						 &sei->ipod_id, sei);
			}
			else if (sei->md5_hash)
			{
			    if (!extendedinfohash_md5)
			    {
				extendedinfohash_md5 = g_hash_table_new_full (
				    g_str_hash,g_str_equal, NULL,hash_delete);
			    }
			    g_hash_table_insert (extendedinfohash_md5,
						 sei->md5_hash, sei);
			}
			else
			{
			    hash_delete ((gpointer)sei);
			}
		    }
		    else
		    { /* this is a deleted track that hasn't yet been
		         removed from the iPod's hard drive */
			Track *track = g_malloc0 (sizeof (Track));
			track->ipod_path = g_strdup (sei->ipod_path);
			pending_deletion = g_list_append (pending_deletion,
							  track);
			hash_delete ((gpointer)sei); /* free sei */
		    }
		    sei = NULL;
		}
		if (strcmp (arg, "xxx") != 0)
		{
		    sei = g_malloc0 (sizeof (struct track_extended_info));
		    sei->ipod_id = atoi (arg);
		}
	    }
	    else if (g_ascii_strcasecmp (line, "version") == 0)
	    { /* found version */
		extendedinfoversion = g_ascii_strtod (arg, NULL);
	    }
	    else if (sei == NULL)
	    {
		gtkpod_warning (_("%s:\nFormat error: %s\n"), name, buf);
		success = FALSE;
		break;
	    }
	    else if (g_ascii_strcasecmp (line, "hostname") == 0)
		sei->hostname = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "filename_locale") == 0)
		sei->pc_path_locale = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "filename_utf8") == 0)
		sei->pc_path_utf8 = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "md5_hash") == 0)
	    {   /* only accept hash value if version is >= 0.53 or
		   PATH_MAX is 4096 -- in 0.53 I changed the MD5 hash
		   routine to using blocks of 4096 Bytes in
		   length. Before it was PATH_MAX, which might be
		   different on different architectures. */
		if ((extendedinfoversion >= 0.53) || (PATH_MAX == 4096))
		    sei->md5_hash = g_strdup (arg);
	    }
	    else if (g_ascii_strcasecmp (line, "charset") == 0)
		sei->charset = g_strdup (arg);
	    else if (g_ascii_strcasecmp (line, "oldsize") == 0)
		sei->oldsize = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "playcount") == 0)
		sei->playcount = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "rating") == 0)
		sei->rating = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "transferred") == 0)
		sei->transferred = atoi (arg);
	    else if (g_ascii_strcasecmp (line, "filename_ipod") == 0)
		sei->ipod_path = g_strdup (arg);
    }
    g_free (md5);
    fclose (fp);
    if (success && !hash_matched && !extendedinfohash_md5)
    {
	gtkpod_warning (_("No MD5 checksums on individual tracks are available.\n\nTo avoid this situation in the future either switch on duplicate detection (will provide MD5 checksums) or avoid using the iPod with programs other than gtkpod.\n\n"), itunes, name);
	success = FALSE;
    }
    if (!success) destroy_extendedinfohash ();
    return success;
}


/* Handle the function "Import iTunesDB" */
/* The import button is disabled once you have imported an existing
   database or exported your new data. Therefore, no specific check
   has to be performed on whether it's OK to import or not. */
void handle_import (void)
{
    gchar *name1, *name2, *cfgdir;
    gboolean success, md5tracks;
    guint32 n;


    /* we should switch off duplicate detection during import --
     * otherwise we mess up the playlists */

    md5tracks = prefs_get_md5tracks ();
    prefs_set_md5tracks (FALSE);

    n = get_nr_of_tracks (); /* how many tracks are already there? */

    block_widgets ();
    if (!prefs_get_offline())
    { /* iPod is connected */
	if (prefs_get_write_extended_info())
	{
	    name1 = g_build_filename (prefs_get_ipod_mount (),
				"iPod_Control/iTunes/iTunesDB.ext", NULL);
	    name2 = g_build_filename (prefs_get_ipod_mount (),
				"iPod_Control/iTunes/iTunesDB", NULL);
	    success = read_extended_info (name1, name2);
	    g_free (name1);
	    g_free (name2);
	    if (!success) 
	    {
		gtkpod_warning (_("Extended info will not be used.\n"));
	    }
	}
	if(itunesdb_parse (prefs_get_ipod_mount ()))
	    gtkpod_statusbar_message(_("iPod Database Successfully Imported"));
	else
	    gtkpod_statusbar_message(_("iPod Database Import Failed"));
    }
    else
    { /* offline - requires extended info */
	if ((cfgdir = prefs_get_cfgdir ()))
	{
	    name1 = g_build_filename (cfgdir, "iTunesDB.ext", NULL);
	    name2 = g_build_filename (cfgdir, "iTunesDB", NULL);
	    success = read_extended_info (name1, name2);
	    g_free (name1);
	    if (!success) 
	    {
		gtkpod_warning (_("Extended info will not be used. If you have non-transferred tracks,\nthese will be lost.\n"));
	    }
	    if(itunesdb_parse_file (name2))
		gtkpod_statusbar_message(
			_("Offline iPod Database Successfully Imported"));
	    else
		gtkpod_statusbar_message(
			_("Offline iPod Database Import Failed"));
	    g_free (name2);
	    g_free (cfgdir);
	}
	else
	{
	    gtkpod_warning (_("Import aborted.\n"));
	    return;
	    release_widgets ();
	}
    }
    /* We need to make sure that the tracks that already existed
       in the DB when we called itunesdb_parse() do not duplicate
       any existing ID */
    renumber_ipod_ids ();

    gtkpod_tracks_statusbar_update();
    if (n != get_nr_of_tracks ())
    { /* Import was successfull, block menu item and button */
	display_disable_gtkpod_import_buttons();
    }
    /* reset duplicate detection -- this will also detect and correct
     * all duplicate tracks currently in the database */
    prefs_set_md5tracks (md5tracks);
    destroy_extendedinfohash (); /* delete hash information (if set up) */
    release_widgets ();
}


/* Like get_track_name_on_disk(), but verifies the track actually exists
   Must g_free return value after use */
gchar *get_track_name_on_disk_verified (Track *track)
{
    gchar *name = NULL;

    if (track)
    {
	name = get_track_name_on_ipod (track);
	if (name)
	{
	    if (!g_file_test (name, G_FILE_TEST_EXISTS))
	    {
		g_free (name);
		name = NULL;
	    }
	}
	if(!name && track->pc_path_locale && (*track->pc_path_locale))
	{
	    if (g_file_test (track->pc_path_locale, G_FILE_TEST_EXISTS))
		name = g_strdup (track->pc_path_locale);
	} 
    }
    return name;
}

/**
 * get_track_name_on_disk
 * Function to retrieve the filename on disk for the specified Track.  It
 * returns the valid filename whether the file has been copied to the ipod,
 * or has yet to be copied.  So it's useful for file operations on a track.
 * @s - The Track data structure we want the on disk file for
 * Returns - the filename for this Track. Must be g_free'd.
 */
gchar* get_track_name_on_disk(Track *s)
{
    gchar *result = NULL;

    if(s)
    {
	result = get_track_name_on_ipod (s);
	if(!result &&
	   (s->pc_path_locale) && (strlen(s->pc_path_locale) > 0))
	{
	    result = g_strdup (s->pc_path_locale);
	} 
    }
    return(result);
}


/* Return the full iPod filename as stored in @s.
   @s: track
   Return value: full filename to @s on the iPod or NULL if no
   filename is set in @s. NOTE: the file does not necessarily
   exist. NOTE: the in itunesdb.c code works around a problem on some
   systems (see below) and might return a filename with different case
   than the original filename. Don't copy it back to @s */
gchar *get_track_name_on_ipod (Track *s)
{
    gchar *result = NULL;

    if(s &&  !prefs_get_offline ())
    {
	result = itunesdb_get_track_name_on_ipod (prefs_get_ipod_mount (), s);
    }
    return(result);
}



/**
 * get_preferred_track_name_format - useful for generating the preferred
 * output filename for any track.  
 * FIXME Eventually this should check your prefs for the displayed
 * attributes in the track model and generate track names based on that
 * @s - The Track reference we're generating the filename for
 * Returns - The preferred filename, you must free it yourself.
 */
gchar *
get_preferred_track_name_format (Track *s)
{
    gchar *result = NULL;
    if (s)
    {
	gchar *artist = charset_track_charset_from_utf8 (s, s->artist);
	gchar *album = charset_track_charset_from_utf8 (s, s->album);
	gchar *title = charset_track_charset_from_utf8 (s, s->title);
	result = g_strdup_printf ("%s-%s-%02d-%s.mp3",
				  artist, album, s->track_nr, title);
	g_free (artist);
	g_free (album);
	g_free (title);
    }
    return (result);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Functions concerning deletion of tracks                      *
 *                                                                  *
\*------------------------------------------------------------------*/


/* in Bytes, minus the space taken by tracks that will be overwritten
 * during copying */
/* @num will be filled with the number of tracks if != NULL */
double get_filesize_of_deleted_tracks (guint32 *num)
{
    double size = 0;
    guint32 n = 0;
    Track *track;
    GList *gl_track;

    for (gl_track = pending_deletion; gl_track; gl_track=gl_track->next)
    {
	track = (Track *)gl_track->data;
	if (track->transferred)   size += track->size;
	++n;
    }
    if (num)  *num = n;
    return size;
}

void mark_track_for_deletion (Track *track)
{
    pending_deletion = g_list_append(pending_deletion, track);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Handle Export of iTunesDB                                   *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Writes extended info (md5 hash, PC-filename...) into file "name".
   "itunes" is the filename of the corresponding iTunesDB */
static gboolean write_extended_info (gchar *name, gchar *itunes)
{
  FILE *fp;
  guint n,i;
  Track *track;
  gchar *md5;

  fp = fopen (name, "w");
  if (!fp)
  {
      gtkpod_warning (_("Could not open \"%s\" for writing extended info.\n"),
		      name);
      return FALSE;
  }
  md5 = md5_hash_on_file_name (itunes);
  if (md5)
  {
      fprintf(fp, "itunesdb_hash=%s\n", md5);
      g_free (md5);
  }
  else
  {
      gtkpod_warning (_("Aborted writing of extended info.\n"));
      fclose (fp);
      return FALSE;
  }
  fprintf (fp, "version=%s\n", VERSION);
  n = get_nr_of_tracks ();
  for (i=0; i<n; ++i)
  {
      track = get_track_by_nr (i);
      fprintf (fp, "id=%d\n", track->ipod_id);
      if (track->hostname)
	  fprintf (fp, "hostname=%s\n", track->hostname);
      if (strlen (track->pc_path_locale) != 0)
	  fprintf (fp, "filename_locale=%s\n", track->pc_path_locale);
      if (strlen (track->pc_path_utf8) != 0)
	  fprintf (fp, "filename_utf8=%s\n", track->pc_path_utf8);
      if (track->md5_hash)
	  fprintf (fp, "md5_hash=%s\n", track->md5_hash);
      if (track->charset)
	  fprintf (fp, "charset=%s\n", track->charset);
      if (!track->transferred && track->oldsize)
	  fprintf (fp, "oldsize=%d\n", track->oldsize);
      fprintf (fp, "transferred=%d\n", track->transferred);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }
  if (prefs_get_offline())
  { /* we are offline and also need to export the list of tracks that
       are to be deleted */
      GList *gl_track;
      for(gl_track = pending_deletion; gl_track; gl_track = gl_track->next)
      {
	  track = (Track*)gl_track->data;
	  fprintf (fp, "id=000\n");  /* our sign for tracks pending
					deletion */
	  fprintf (fp, "filename_ipod=%s\n", track->ipod_path);
	  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
      }
  }
  fprintf (fp, "id=xxx\n");
  fclose (fp);
  return TRUE;
}



#ifdef G_THREADS_ENABLED
/* Threaded remove file */
static gpointer th_remove (gpointer filename)
{
    int result;

    result = remove ((gchar *)filename);
    g_mutex_lock (mutex);
    mutex_data = TRUE; /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return (gpointer)result;
}
/* Threaded copy of ipod track */
static gpointer th_copy (gpointer s)
{
    gboolean result;
    Track *track = (Track *)s;

    result = itunesdb_copy_track_to_ipod (prefs_get_ipod_mount (),
					 track,
					 track->pc_path_locale);
    /* delete old size */
    if (track->transferred) track->oldsize = 0;
    g_mutex_lock (mutex);
    mutex_data = TRUE;   /* signal that thread will end */
    g_cond_signal (cond);
    g_mutex_unlock (mutex);
    return (gpointer)result;
}
#endif 

/* This function is called when the user presses the abort button
 * during flush_tracks() */
static void flush_tracks_abort (gboolean *abort)
{
    *abort = TRUE;
}

/* Flushes all non-transferred tracks to the iPod filesystem
   Returns TRUE on success, FALSE if some error occured */
gboolean flush_tracks (void)
{
  gint count, n, nrs;
  gchar *buf;
  Track  *track;
  gchar *filename = NULL;
  gint rmres;
  gboolean result = TRUE;
  static gboolean abort;
  GtkWidget *dialog, *progress_bar, *label, *image, *hbox;
  time_t diff, start, mins, secs;
  char *progtext = NULL;

#ifdef G_THREADS_ENABLED
  GThread *thread = NULL;
  GTimeVal gtime;
  if (!mutex) mutex = g_mutex_new ();
  if (!cond) cond = g_cond_new ();
#endif

  n = get_nr_of_nontransferred_tracks ();

  if (n==0 && !pending_deletion) return TRUE;

  abort = FALSE;
  /* create the dialog window */
  dialog = gtk_dialog_new_with_buttons (_("Information"),
                                         GTK_WINDOW (gtkpod_window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_NONE,
                                         NULL);

  /* emulate gtk_message_dialog_new */
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
				    GTK_ICON_SIZE_DIALOG);
  label = gtk_label_new (
      _("Press button to abort.\nExport can be continued at a later time."));

  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  /* hbox to put the image+label in */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* Create the progress bar */
  progress_bar = gtk_progress_bar_new ();
  progtext = g_strdup (_("deleting..."));
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
  g_free (progtext);

  /* Indicate that user wants to abort */
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (flush_tracks_abort),
			    &abort);

  /* Add the image/label + progress bar to dialog */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      progress_bar, FALSE, FALSE, 0);
  gtk_widget_show_all (dialog);

  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

  /* lets clean up those pending deletions */
  while (!abort && pending_deletion)
  {
      track = (Track*)pending_deletion->data;
      if((filename = get_track_name_on_ipod(track)))
      {
	  const gchar *mp = prefs_get_ipod_mount ();
	  if(mp && g_strstr_len(filename, strlen(mp), mp))
	  {
#ifdef G_THREADS_ENABLED
	      mutex_data = FALSE;
	      thread = g_thread_create (th_remove, filename, TRUE, NULL);
	      if (thread)
	      {
		  g_mutex_lock (mutex);
		  do
		  {
		      while (widgets_blocked && gtk_events_pending ())
			  gtk_main_iteration ();
		      /* wait a maximum of 20 ms */
		      g_get_current_time (&gtime);
		      g_time_val_add (&gtime, 20000);
		      g_cond_timed_wait (cond, mutex, &gtime);
		  } while(!mutex_data);
		  g_mutex_unlock (mutex);
		  rmres = (gint)g_thread_join (thread);
		  if (rmres == -1) result = FALSE;
	      }
	      else {
		  g_warning ("Thread creation failed, falling back to default.\n");
		  remove (filename);
	      }
#else
	      rmres = remove(filename);
	      if (rmres == -1) result = FALSE;
/*	      fprintf(stderr, "Removed %s-%s(%d)\n%s\n", track->artist,
						    track->title, track->ipod_id,
						    filename);*/
#endif 
	  }
	  g_free(filename);
      }
      free_track(track);
      pending_deletion = g_list_delete_link (pending_deletion, pending_deletion);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  }
  if (abort) result = FALSE;
  if (result == FALSE)
  {
      gtkpod_statusbar_message (_("Some tracks could not be deleted from the iPod. Export aborted!"));
  }
  else
  {
      /* we now have as much space as we're gonna have, copy files to ipod */

      progtext = g_strdup (_("preparing to copy..."));
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar), progtext);
      g_free (progtext);
      while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();

      /* count number of tracks to be transferred */
      if (n != 0)  display_disable_gtkpod_import_buttons();
      count = 0; /* tracks transferred */
      nrs = 0;
      start = time(NULL);
      while (!abort &&  (track = get_track_by_nr (nrs))) {
	  ++nrs;
	  if (!track->transferred)
	  {
#ifdef G_THREADS_ENABLED
	      mutex_data = FALSE;
	      thread = g_thread_create (th_copy, track, TRUE, NULL);
	      if (thread)
	      {
		  g_mutex_lock (mutex);
		  do
		  {
		      while (widgets_blocked && gtk_events_pending ())
			  gtk_main_iteration ();
		      /* wait a maximum of 10 ms */
		      g_get_current_time (&gtime);
		      g_time_val_add (&gtime, 20000);
		      g_cond_timed_wait (cond, mutex, &gtime);
		  } while(!mutex_data);
		  g_mutex_unlock (mutex);
		  result &= (gboolean)g_thread_join (thread);
	      }
	      else {
		  g_warning ("Thread creation failed, falling back to default.\n");
		  result &= itunesdb_copy_track_to_ipod (
		      prefs_get_ipod_mount (),
		      track, track->pc_path_locale);
		  /* delete old size */
		  if (track->transferred) track->oldsize = 0;
	      }
#else
	      result &= itunesdb_copy_track_to_ipod (cfg->ipod_mount,
						    track, track->pc_path_locale);
	      /* delete old size */
	      if (track->transferred) track->oldsize = 0;

#endif 
	      ++count;
	      if (count == 1)  /* we need longer timeout */
		  prefs_set_statusbar_timeout (3*STATUSBAR_TIMEOUT);
	      if (count == n)  /* we need to reset timeout */
		  prefs_set_statusbar_timeout (0);
	      buf = g_strdup_printf (ngettext ("Copied %d of %d new track.",
					       "Copied %d of %d new tracks.", n),
				     count, n);
	      gtkpod_statusbar_message(buf);
	      g_free (buf);

              gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progress_bar),
					    (gdouble) count/n);

	      diff = time(NULL) - start;
	      mins = ((diff*n/count)-diff+5)/60;
	      secs = ((((diff*n/count)-diff+5) & 60) / 5) * 5;
	      /* don't bounce up too quickly (>10% change only) */
/*	      left = ((mins < left) || (100*mins >= 110*left)) ? mins : left;*/
	      progtext = g_strdup_printf (_("%d%% (%d:%02d) left"),
					  count*100/n, (int)mins, (int)secs);
              gtk_progress_bar_set_text(GTK_PROGRESS_BAR (progress_bar),
					progtext);
	      g_free (progtext);
	  }
	  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
      } /* while (gl_track) */
      if (abort)      result = FALSE;   /* negative result if user aborted */
      if (result == FALSE)
	  gtkpod_statusbar_message (_("Some tracks were not written to iPod. Export aborted!"));
  }
  prefs_set_statusbar_timeout (0);
  gtk_widget_destroy (dialog);
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return result;
}


/* used to handle export of database */
void handle_export (void)
{
  gchar *ipt, *ipe, *cft=NULL, *cfe=NULL, *cfgdir;
  gboolean success = TRUE;
  gchar *buf;

  block_widgets (); /* block user input */
  cfgdir = prefs_get_cfgdir ();
  ipt = g_build_filename (prefs_get_ipod_mount (),
			  "iPod_Control/iTunes/iTunesDB", NULL);
  ipe = g_build_filename (prefs_get_ipod_mount (),
			  "iPod_Control/iTunes/iTunesDB.ext", NULL);
  if (cfgdir)
    {
      cft = g_build_filename (cfgdir, "iTunesDB", NULL);
      cfe = g_build_filename (cfgdir, "iTunesDB.ext", NULL);
    }

  if(!prefs_get_offline ())
    {
      /* write tracks to iPod */
      if ((success=flush_tracks ()))
      { /* write iTunesDB to iPod */
	  gtkpod_statusbar_message (_("Now writing iTunesDB. Please wait..."));
	  while (widgets_blocked && gtk_events_pending ())
	      gtk_main_iteration ();
	  if (!(success=itunesdb_write (prefs_get_ipod_mount ())))
	      gtkpod_statusbar_message (_("Error writing iTunesDB to iPod. Export aborted!"));
	  /* else: write extended info (PC filenames, md5 hash) to iPod */
	  else if (prefs_get_write_extended_info ())
	  {
	      if(!(success = write_extended_info (ipe, ipt)))
		  gtkpod_statusbar_message (_("Extended information not written"));
	  }
	  /* else: delete extended information file, if it exists */
	  else if (g_file_test (ipe, G_FILE_TEST_EXISTS))
	  {
	      if (remove (ipe) != 0)
	      {
		  buf = g_strdup_printf (_("Extended information file not deleted: '%s\'"), ipe);
		  gtkpod_statusbar_message (buf);
		  g_free (buf);
	      }
	  }
      }
      /* if everything was successful, copy files to ~/.gtkpod */
      if (success && prefs_get_keep_backups ())
      {
	  if (cfgdir)
	  {
	      gtkpod_statusbar_message (
		  _("Now writing iTunesDB. Please wait..."));
	      while (widgets_blocked && gtk_events_pending ())
	      gtk_main_iteration ();
	      success = itunesdb_cp (ipt, cft);
	      if (success && prefs_get_write_extended_info())
		  success = itunesdb_cp (ipe, cfe);
	  }
	  if ((cfgdir == NULL) || (!success))
	      gtkpod_statusbar_message (_("Backups could not be created!"));
      }
      if (success && !prefs_get_keep_backups() && cfgdir)
	  cleanup_backup_and_extended_files ();
    }
  else
    { /* we are offline -> only write database to ~/.gtkpod */
      /* offline implies "extended information" */
      if (cfgdir)
      {
	  gtkpod_statusbar_message (_("Now writing iTunesDB. Please wait..."));
	  while (widgets_blocked && gtk_events_pending ())
	      gtk_main_iteration ();
	  success = itunesdb_write_to_file (cft);
	  if (success)
	      success = write_extended_info (cfe, cft);
      }
      if ((cfgdir == NULL) || (!success))
      {
	  gtkpod_statusbar_message (_("Export not successful!"));
	  success = FALSE;
      }
    }

  /* indicate that files and/or database is saved */
  if (success)   
  {
      files_saved = TRUE;
      /* block menu item and button */
      display_disable_gtkpod_import_buttons();
      gtkpod_statusbar_message(_("iPod Database Saved"));
  }

  g_free (cfgdir);
  g_free (cft);
  g_free (cfe);
  g_free (ipt);
  g_free (ipe);

  release_widgets (); /* Allow input again */
}


/* make state of "files_saved" available to functions */
gboolean files_are_saved (void)
{
  return files_saved;
}

/* set the state of "files_saved" to FALSE */
void data_changed (void)
{
  files_saved = FALSE;
}
