/* Time-stamp: <2005-11-13 16:29:27 jcs>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "charset.h"
#include "clientserver.h"
#include "confirmation.h"
#include "info.h"
#include "file.h"
#include "itdb.h"
#include "md5.h"
#include "misc.h"
#include "misc_track.h"
#include "mp3file.h"
#include "mp4file.h"
#include "prefs.h"
#include "wavfile.h"

/* Determine the type of a file. 
 *
 * Currently this is done by checking the suffix of the filename. An improved
 * version should probably only fall back to that method if everything else
 * fails.
 * -jlt
 */

gint determine_file_type (gchar *path)
{
	gchar *path_utf8, *suf;
	gint type = FILE_TYPE_UNKNOWN;
	
	g_return_val_if_fail (path, type);
	
    	path_utf8 = charset_to_utf8 (path);
	suf = strrchr (path_utf8, '.');
	if (suf)
	{
	    /* since we are exclusively checking for equality strcasecmp
	     * should be sufficient */
	    if (g_strcasecmp (suf, ".mp3") == 0) type = FILE_TYPE_MP3;
	    else if (g_strcasecmp (suf, ".m4a") == 0) type = FILE_TYPE_M4A;
	    else if (g_strcasecmp (suf, ".m4p") == 0) type = FILE_TYPE_M4P;
	    else if (g_strcasecmp (suf, ".m4b") == 0) type = FILE_TYPE_M4B;
	    else if (g_strcasecmp (suf, ".wav") == 0) type = FILE_TYPE_WAV;
	    else if (g_strcasecmp (suf, ".m3u") == 0) type = FILE_TYPE_M3U;
	    else if (g_strcasecmp (suf, ".pls") == 0) type = FILE_TYPE_PLS;
	}

	g_free(path_utf8);
	return type;
}


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
/* Return value: playlist to which the files were added to or NULL on
 * error */
Playlist *
add_playlist_by_filename (iTunesDB *itdb, gchar *plfile,
			  Playlist *plitem, gint plitem_pos,
			  AddTrackFunc addtrackfunc, gpointer data)
{
    gchar *bufp, *plfile_utf8;
    gchar *dirname = NULL, *plname = NULL;
    gchar buf[PATH_MAX];
    gint type = FILE_TYPE_UNKNOWN; /* type of playlist file */
    gint line, tracks;
    FILE *fp;
    gboolean error;

    g_return_val_if_fail (plfile, FALSE);
    g_return_val_if_fail (itdb, FALSE);

    if (g_file_test (plfile, G_FILE_TEST_IS_DIR))
    {
	gtkpod_warning (_("'%s' is a directory, not a playlist file.\n\n"),
			plfile);
	return FALSE;  /* definitely not! */
    }

    plfile_utf8 = charset_to_utf8 (plfile);

    plname = g_path_get_basename (plfile_utf8);
    g_free (plfile_utf8);

    bufp = g_strrstr (plname, "."); /* find last occurence of '.' */
    if (bufp)
    {
	*bufp = 0;          /* truncate playlist name */
	type = determine_file_type(plfile);
	switch (type)
	{
	case FILE_TYPE_MP3:
	case FILE_TYPE_M4A:
	case FILE_TYPE_M4P:
	case FILE_TYPE_M4B:
	case FILE_TYPE_WAV:
	    gtkpod_warning (_("'%s' is a not a known playlist file.\n\n"),
			      plfile);
	    g_free(plname);
	    return FALSE;
	case FILE_TYPE_M3U:
	case FILE_TYPE_PLS:
	    break;
	case FILE_TYPE_UNKNOWN:
	    /* assume MISC (M3U like) style */
	    type = FILE_TYPE_MISC;
	    break;
	}
    }

    /* attempt to open playlist file */
    if (!(fp = fopen (plfile, "r")))
    {
	gtkpod_warning (_("Cannot open '%s' for reading.\n\n"));
	g_free (plname);
	return FALSE;  /* definitely not! */
    }
    /* create playlist (if none is specified) */
    if (!plitem)  plitem = gp_playlist_add_new (itdb, plname,
						FALSE, plitem_pos);
    C_FREE (plname);
    g_return_val_if_fail (plitem, NULL);

    /* need dirname if playlist file contains relative paths */
    dirname = g_path_get_dirname (plfile);

    /* for now assume that all playlist files will be line-based
       all of these are line based -- add different code for different
       playlist files */
    line = -1; /* nr of line being read */
    tracks = 0; /* nr of tracks added */
    error = FALSE;
    while (!error && fgets (buf, PATH_MAX, fp))
    {
	gchar *bufp = buf;
	gchar *filename = NULL;
	gint len = strlen (bufp); /* remove newline */

	++line;
	if (len == 0) continue;   /* skip empty lines */
	if (bufp[len-1] == 0x0a)  bufp[len-1] = 0;
	switch (type)
	{
	case FILE_TYPE_MISC:
	    /* skip whitespace */
	    while (isspace (*bufp)) ++bufp;
	    /* assume comments start with ';' or '#' */
	    if ((*bufp == ';') || (*bufp == '#')) break;
	    /* assume the rest of the line is a filename */
	    filename = concat_dir_if_relative (dirname, bufp);
	    break;
	case FILE_TYPE_M3U:
	    /* comments start with '#' */
	    if (*bufp == '#') break;
	    /* assume the rest of the line is a filename */
	    filename = concat_dir_if_relative (dirname, bufp);
	    break;
	case FILE_TYPE_PLS:
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
		if (bufp)
		{
		    ++bufp;
		    filename = concat_dir_if_relative (dirname, bufp);
		}
	    }
	    break;
	}
	if (filename)
	{
	    /* do not allow to add directories! */
	    if (g_file_test (filename, G_FILE_TEST_IS_DIR))
	    {
		gtkpod_warning (_("Skipping '%s' because it is a directory.\n"), filename);
	    }
	    /* do not allow to add playlist file recursively */
	    else if (strcmp (plfile, filename) == 0)
	    {
		gtkpod_warning (_("Skipping '%s' to avoid adding playlist file recursively\n"), filename);
	    }
	    else if (add_track_by_filename (itdb, filename, plitem,
					    prefs_get_add_recursively (),
					    addtrackfunc, data))
	    {
		++tracks;
	    }
	    g_free (filename);
	}
    }
    fclose (fp);
    C_FREE (dirname);

    /* I don't think it's too interesting to pop up the list of
       duplicates -- but we should reset the list. */
    gp_duplicate_remove (NULL, (void *)-1);

    if (!error) return plitem;
    return NULL;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Add Dir                                                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Add all files in directory and subdirectories.
   If @name is a regular file, just add that.
   If @plitem != NULL, add tracks also to Playlist @plitem
   @descend: TRUE: add recursively
             FALSE: don't enter subdirectories */
/* Not nice: the return value has not much meaning. TRUE: all files
 * were added successfully. FALSE: some files could not be
   added (e.g: duplicates)  */
/* @addtrackfunc: if != NULL this will be called instead of
   "add_track_to_playlist () -- used for dropping tracks at a specific
   position in the track view */
gboolean add_directory_by_name (iTunesDB *itdb, gchar *name,
				Playlist *plitem, gboolean descend,
				AddTrackFunc addtrackfunc, gpointer data)
{
  gboolean result = TRUE;

  g_return_val_if_fail (itdb, FALSE);
  g_return_val_if_fail (name, FALSE);

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
		      result &= add_directory_by_name (
			  itdb, nextfull, plitem,
			  descend, addtrackfunc, data);
		  g_free (nextfull);
	      }
	  } while (next != NULL);
	  g_dir_close (dir);
      }
      release_widgets ();
  }
  else
  {
      result = add_track_by_filename (itdb, name, plitem,
				      descend, addtrackfunc, data);
  }
  return result;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *      Fill in track struct with data from file                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* parse the file with name @filename (UTF8) and fill extract the tags
 * to @trackas specified in @template. @track can be NULL if you just
 * want to verify the template */
static gboolean parse_filename_with_template (Track *track,
					      gchar *filename,
					      const gchar *template)
{
    GList *tokens=NULL, *gl;
    gchar *tpl, *fn;
    gchar *sps, *sp, *str;
    gboolean parse_error = FALSE;

    if (!template || !filename) return FALSE;

#ifdef DEBUG
    printf ("fn: '%s', tpl: '%s'\n", filename, template);
#endif

    /* If the template starts with a '%.' (except for '%%') we add a
       '/' in front so that the field has a boundary */
    if ((template[0] == '%') && (template[1] != '%'))
	 tpl = g_strdup_printf ("%c%s", G_DIR_SEPARATOR, template);
    else tpl = g_strdup (template);

    fn = g_strdup (filename);

    /* We split the template into tokens. Each token starting with a
       '%' and two chars long specifies a field ('%.'), otherwise it's
       normal text (used to separate two fields).
       "/%a - %t.mp3" would become ".mp3"  "%t"  " - "  "%a"  "/" */
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
    /* add what's left */
    if (sps[0] != 0)
	tokens = g_list_prepend (tokens, g_strdup (sps));

    /* If the "last" token does not contain a '.' (like in ".mp3"),
       remove the filename extension ("somefile.mp3" -> "somefile")
       because no extension was given in the template */
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

    /* use the tokens to parse the filename */
    gl = tokens;
    while (gl)
    {
	gchar *token = gl->data;
	/* remember: all tokens starting with '%' and two chars long
	   specify a field */
	if ((token[0] == '%') && (strlen (token) == 2))
	{   /* handle tag item */
	    GList *gln = gl->next;
	    if (gln)
	    {
		gchar *itm;
		gchar *next_token = gln->data;
		/* find next token so we can determine where the
		   current field ends */
		gchar *fnp = g_strrstr (fn, next_token);

		if (!fnp)
		{
		    parse_error = TRUE;
		    break;
		}

		/* truncate the filename (for the next token) */
		fnp[0] = 0;
		/* adjust fnp to point to the start of the field */
		fnp = fnp + strlen (next_token);
#ifdef DEBUG
		printf ("%s: '%s'\n", token, fnp);
#endif
		itm = g_strstrip (g_strdup (fnp));
		switch (token[1])
		{
		case 'a': /* artist */
		    if (track &&
			(!track->artist || prefs_get_parsetags_overwrite ()))
		    {
			g_free (track->artist);
			track->artist = itm;
		    }
		    break;
		case 'A': /* album */
		    if (track &&
			(!track->album || prefs_get_parsetags_overwrite ()))
		    {
			g_free (track->album);
			track->album = itm;
		    }
		    break;
		case 'c': /* composer */
		    if (track &&
			(!track->composer || prefs_get_parsetags_overwrite ()))
		    {
			g_free (track->composer);
			track->composer = itm;
		    }
		    break;
		case 't': /* title */
		    if (track &&
			(!track->title || prefs_get_parsetags_overwrite ()))
		    {
			g_free (track->title);
			track->title = itm;
		    }
		    break;
		case 'g': /* genre */
		case 'G': /* genre */
		    if (track &&
			(!track->genre || prefs_get_parsetags_overwrite ()))
		    {
			g_free (track->genre);
			track->genre = itm;
		    }
		    break;
		case 'T': /* track */
		    if (track &&
			((track->track_nr == 0) ||
			 prefs_get_parsetags_overwrite ()))
			track->track_nr = atoi (itm);
		    g_free (itm);
		    break;
		case 'C': /* CD */
		    if (track &&
			(track->cd_nr == 0 || prefs_get_parsetags_overwrite ()))
			track->cd_nr = atoi (itm);
		    g_free (itm);
		    break;
		case 'Y': /* year */
		    if (track &&
			(track->year == 0 || prefs_get_parsetags_overwrite ()))
			track->year = atoi (itm);
		    g_free (itm);
		    break;
		case '*': /* placeholder to skip a field */
		    g_free (itm);
		    break;
		default:
		    g_free (itm);
		    gtkpod_warning (_("Unknown token '%s' in template '%s'\n"),
				    token, template);
		    parse_error = TRUE;
		    break;
		}
		if (parse_error) break;
		gl = gln->next;
	    }
	    else
	    { /* if (gln)...else... */
		break;
	    }
	}
	else
	{   /* skip text */
	    gchar *fnp = g_strrstr (fn, token);
	    if (!fnp)
	    {
		parse_error = TRUE;
		break;  /* could not match */
	    }
	    if (fnp - fn + strlen (fnp) != strlen (fn))
	    {
		parse_error = TRUE;
		break; /* not at the last position */
	    }
	    fnp[0] = 0;
	    gl = gl->next;
	}
    }

    g_free (fn);
    g_free (tpl);
    g_list_foreach (tokens, (GFunc)g_free, NULL);
    g_list_free (tokens);
    return (!parse_error);
}

/* parse the filename of @track and extract the tags as specified in
   prefs_get_parsetags_template(). Several templates can be separated
   with the "," character. */
static void parse_filename (Track *track)
{
    ExtraTrackData *etr;
    const gchar *template;
    gchar **templates, **tplp;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    template = prefs_get_parsetags_template ();
    if (!template) return;
    templates = g_strsplit (template, ";", 0);
    tplp = templates;
    while (*tplp)
    {
	if (parse_filename_with_template (NULL, etr->pc_path_utf8, *tplp))
	    break;
	++tplp;
    }
    if (*tplp)  parse_filename_with_template (track, etr->pc_path_utf8, *tplp);
    g_strfreev (templates);
}

/* Set entry "column" (TM_COLUMN_TITLE etc) according to filename */
/* TODO: make the TAG extraction more intelligent -- if possible, this
   should be user configurable. */
static void set_entry_from_filename (Track *track, gint column)
{
    ExtraTrackData *etr;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    if (prefs_get_autosettags (column) &&
	etr->pc_path_utf8 && strlen (etr->pc_path_utf8))
    {
	switch (column)
	{
	case TM_COLUMN_TITLE:
	    g_free (track->title);
	    track->title = g_path_get_basename (etr->pc_path_utf8);
	    break;
	case TM_COLUMN_ALBUM:
	    g_free (track->album);
	    track->album = g_path_get_basename (etr->pc_path_utf8);
	    break;
	case TM_COLUMN_ARTIST:
	    g_free (track->artist);
	    track->artist = g_path_get_basename (etr->pc_path_utf8);
	    break;
	case TM_COLUMN_GENRE:
	    g_free (track->genre);
	    track->genre = g_path_get_basename (etr->pc_path_utf8);
	    break;
	case TM_COLUMN_COMPOSER:
	    g_free (track->composer);
	    track->composer = g_path_get_basename (etr->pc_path_utf8);
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
    const gchar *charset = prefs_get_charset ();
    ExtraTrackData *etr;

    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    C_FREE (etr->charset);
    if (!charset || !strlen (charset))
    {    /* use standard locale charset */
	g_get_charset (&charset);
    }
    /* only set charset if it's not GTKPOD_JAPAN_AUTOMATIC */
    if (charset && (strcmp (charset, GTKPOD_JAPAN_AUTOMATIC) != 0))
    {
	etr->charset = g_strdup (charset);
    }
}


/* Copy "new" info read from file to an old Track structure.
   Return value: a pointer to the track the data was copied to. */
Track *copy_new_info (Track *from, Track *to)
{
    ExtraTrackData *efrom, *eto;

    g_return_val_if_fail (from, NULL);
    g_return_val_if_fail (to, NULL);
    efrom = from->userdata;
    eto = to->userdata;
    g_return_val_if_fail (efrom, NULL);
    g_return_val_if_fail (eto, NULL);

    C_FREE (to->album);
    C_FREE (to->artist);
    C_FREE (to->title);
    C_FREE (to->genre);
    C_FREE (to->comment);
    C_FREE (to->composer);
    C_FREE (to->filetype);
    C_FREE (eto->pc_path_utf8);
    C_FREE (eto->pc_path_locale);
    C_FREE (eto->charset);
    /* copy strings */
    to->album = g_strdup (from->album);
    to->artist = g_strdup (from->artist);
    to->title = g_strdup (from->title);
    to->genre = g_strdup (from->genre);
    to->comment = g_strdup (from->comment);
    to->composer = g_strdup (from->composer);
    to->filetype = g_strdup (from->filetype);
    eto->pc_path_utf8 = g_strdup (efrom->pc_path_utf8);
    eto->pc_path_locale = g_strdup (efrom->pc_path_locale);
    eto->charset = g_strdup (efrom->charset);
    to->size = from->size;
    to->tracklen = from->tracklen;
    to->cd_nr = from->cd_nr;
    to->cds = from->cds;
    to->track_nr = from->track_nr;
    to->tracks = from->tracks;
    to->bitrate = from->bitrate;
    to->samplerate = from->samplerate;
    to->soundcheck = from->soundcheck;
    eto->peak_signal = efrom->peak_signal;
    eto->radio_gain = efrom->radio_gain;
    eto->audiophile_gain = efrom->audiophile_gain;
    eto->peak_signal_set = efrom->peak_signal_set;
    eto->radio_gain_set = efrom->radio_gain_set;
    eto->audiophile_gain_set = efrom->audiophile_gain_set;
    to->time_added = from->time_added;
    to->time_modified = from->time_modified;
    to->year = from->year;
    g_free (eto->year_str);
    eto->year_str = g_strdup_printf ("%d", to->year);

    return to;
}

/* Updates mserv data (rating only) of @track using filename @name to
   look up mserv information.
   Return TRUE if successfully updated (or disabled), FALSE if not */
gboolean update_mserv_data_from_file (gchar *name, Track *track)
{
    gboolean success=TRUE;

    if (!name || !track) return FALSE;

    if (g_file_test (name, G_FILE_TEST_IS_DIR)) return FALSE;
    if (!g_file_test (name, G_FILE_TEST_EXISTS))
    {
	gchar *buf = g_strdup_printf (_("Local filename not valid (%s)"), name);
	display_mserv_problems (track, buf);
	g_free (buf);
	return FALSE;
    }

    if (prefs_get_mserv_use())
    {
	/* try getting the user's rating from the mserv db */
	const gchar *music_root = prefs_get_path(PATH_MSERV_MUSIC_ROOT);
	const gchar *trackinfo_root = prefs_get_path (PATH_MSERV_TRACKINFO_ROOT);
	/* music_root and trackinfo_root guaranteed to be != NULL */
	g_return_val_if_fail (music_root && trackinfo_root, FALSE);

	success = FALSE;
        /* printf("mroot %s troot %s fname %s\n", music_root, trackinfo_root, name); */

        /* first, check if the file is in the mserv music directory */
	if (*music_root == 0 || strstr(name, music_root))
	{
	    gchar *infoname = g_strdup_printf ("%s%c%s.trk",
					   trackinfo_root,
					   G_DIR_SEPARATOR,
					   &(name[strlen(music_root)]));
	    /* printf("trying %s\n", infoname); */
	    FILE *fp = fopen (infoname, "r");
	    if (fp)
	    {
		/* printf("opened\n");*/
		gchar buff[PATH_MAX];
		const gchar *username = prefs_get_mserv_username ();
		guint usernamelen;
		g_return_val_if_fail (username, (fclose (fp), FALSE));
		usernamelen = strlen (username);
		while (fgets(buff, PATH_MAX, fp))
		{
		    /* printf("username %s (%d) read %s\n",
		     * prefs_get_mserv_username(), usernamelen,
		     * buff);*/
		    if (strncmp(buff, username, usernamelen) == 0
			&& buff[usernamelen] == (gchar)'=')
		    {
			/* found it */
			track->rating = atoi(&buff[usernamelen+1]) * ITDB_RATING_STEP;
			/* printf("found it, = %d\n",
			   orig_track->rating/ITDB_RATING_STEP); */
			success = TRUE;
			break; /* while(fgets(... */
		    }
		}
		fclose(fp);
		if (!success)
		{
		    gchar *buf = g_strdup_printf (_("No information found for user '%s' in '%s'"), prefs_get_mserv_username(), infoname);
		    display_mserv_problems (track, buf);
		    g_free (buf);
		}
	    }
	    else
	    {
		gchar *buf = g_strdup_printf (_("mserv data file (%s) not available for track (%s)"), infoname, name);
		display_mserv_problems (track, buf);
		g_free (buf);
	    }
	    g_free (infoname);
	}
	else
	{
	    gchar *buf = g_strdup_printf (_("Track (%s) not in mserv music root directory (%s)"), name, music_root);
	    display_mserv_problems (track, buf);
	    g_free (buf);
	}
    }

    while (widgets_blocked && gtk_events_pending ())
	gtk_main_iteration ();

    return success;
}


/* Fills the supplied @orig_track with data from the file @name. If
 * @or_track is NULL, a new track struct is created. The entries
 * pc_path_utf8 and pc_path_locale are not changed if an entry already
 * exists */
/* Returns NULL on error, a pointer to the Track otherwise */
Track *get_track_info_from_file (gchar *name, Track *orig_track)
{
    Track *track = NULL;
    Track *nti = NULL;
    gint len;
    gchar *name_utf8 = NULL;

    g_return_val_if_fail (name, NULL);

    if (g_file_test (name, G_FILE_TEST_IS_DIR)) return NULL;

    name_utf8 = charset_to_utf8 (name);

    if (!g_file_test (name, G_FILE_TEST_EXISTS))
    {
	gtkpod_warning (_("The following track could not be processed (file does not exist): '%s'\n"), name_utf8);
	g_free (name_utf8);
	return NULL;
    }

    /* reset the auto detection charset (see explanation in charset.c) */
    charset_reset_auto ();

    /* check for filetype */
    len = strlen (name);
    if (len < 4) return NULL;

    switch (determine_file_type(name))
    {
    case FILE_TYPE_MP3:
	nti = mp3_get_file_info (name);
	break;
    case FILE_TYPE_M4A:
    case FILE_TYPE_M4P:
    case FILE_TYPE_M4B:
	nti = mp4_get_file_info (name);
	break;
    case FILE_TYPE_WAV:
	nti = wav_get_file_info (name);
	break;
    case FILE_TYPE_UNKNOWN:
	gtkpod_warning (_("The following track could not be processed (filetype unknown): '%s'\n"), name_utf8);
	g_free (name_utf8);
	return NULL;
    case FILE_TYPE_M3U:
    case FILE_TYPE_PLS:
	break;
    }

    if (nti)
    {
	ExtraTrackData *enti=nti->userdata;
	struct stat filestat;

	g_return_val_if_fail (enti, NULL);

	if (enti->charset == NULL)
	{   /* Fill in currently used charset. Try if auto_charset is
	     * set first. If not, use the currently set charset. */
	    enti->charset = charset_get_auto ();
	    if (enti->charset == NULL)
		update_charset_info (nti);
	}
	/* set path file information */
	enti->pc_path_utf8 = charset_to_utf8 (name);
	enti->pc_path_locale = g_strdup (name);
	/* set length of file */
	stat (name, &filestat);
	nti->size = filestat.st_size; /* get the filesize in bytes */
	if (nti->bitrate == 0)
	{  /* estimate bitrate */
	    if (nti->tracklen)
		nti->bitrate = nti->size * 8 / nti->tracklen;
	}
	if (enti->radio_gain_set) 
	    nti->soundcheck = 
		replaygain_to_soundcheck (enti->radio_gain);
	/* Set unset strings (album...) from filename */
	set_unset_entries_from_filename (nti);
	/* Make sure all strings are initialized -- that way we don't
	   have to worry about it when we are handling the
	   strings. Also, validate_entries() will fill in the utf16
	   strings if that hasn't already been done. */
	/* exception: md5_hash, charset and hostname: these may be NULL. */
	/* Set modification date to *now* */
	nti->time_modified = itdb_time_get_mac_time ();
	/* Set added date to *now* */
	nti->time_added = nti->time_modified;

	gp_track_validate_entries (nti);

	if (orig_track)
	{ /* we need to copy all information over to the original
	   * track */
	    track = copy_new_info (nti, orig_track);
	    itdb_track_free (nti);
	    nti = NULL;
	}
	else
	{ /* just use nti */
	    track = nti;
	    nti = NULL;
	}

	update_mserv_data_from_file (name, track);
    }
    else
    {
	gtkpod_warning (_("The following track could not be processed (filetype is known but analysis failed): '%s'\n"), name_utf8);
    }	

    while (widgets_blocked && gtk_events_pending ())
	gtk_main_iteration ();

    g_free (name_utf8);
    return track;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Update Track Data from File                                  *
 *                                                                  *
\*------------------------------------------------------------------*/


/* reads info from file and updates the ID3 tags of
   @selected_tracks. */
void update_tracks (GList *selected_tracks)
{
    GList *gl;
    iTunesDB *itdb = NULL;

    if (selected_tracks == NULL)
    {
	gtkpod_statusbar_message(_("Nothing to update"));
	return;
    }

    block_widgets ();
    for (gl=selected_tracks; gl; gl=gl->next)
    {
	Track *track = gl->data;
	g_return_if_fail (track);

	/* update_track_from_file() may possibly remove tracks from
	   the database, so we need to check if the track we are
	   referencing to is still valid. To do so we first have to
	   secure a valid pointer to an itdb. Since the first track in
	   selected_tracks is always valid, we take that one. */
	if (!itdb)
	    itdb = track->itdb;
	g_return_if_fail (itdb);
	if (g_list_find (itdb->tracks, track))
	{
	    gchar *buf = g_strdup_printf (_("Updating %s"),
					  get_track_info (track, TRUE));
	    gtkpod_statusbar_message (buf);
	    g_free (buf);
	    update_track_from_file (track->itdb, track);
	}
    }
    release_widgets ();
    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log of updated tracks */
    display_updated (NULL, NULL);
    /* display log of problems with mserv data */
    display_mserv_problems (NULL, NULL);
    /* display log of detected duplicates */
    gp_duplicate_remove (NULL, NULL);
    gtkpod_statusbar_message(_("Updated selected tracks with info from file."));
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *      Update mserv Data from File                                 *
 *                                                                  *
\*------------------------------------------------------------------*/


/* reads info from file and updates the ID3 tags of
   @selected_tracks. */
void mserv_from_file_tracks (GList *selected_tracks)
{
    GList *gl;

    if (selected_tracks == NULL)
    {
	gtkpod_statusbar_message(_("Nothing to update"));
	return;
    }

    block_widgets ();
    for (gl = selected_tracks; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	gchar *buf;
	Track *track = gl->data;
	g_return_if_fail (track);
	etr = track->userdata;
	g_return_if_fail (etr);
	buf = g_strdup_printf (_("Retrieving mserv data %s"), get_track_info (track, TRUE));
	gtkpod_statusbar_message (buf);
	g_free (buf);
	if (etr->pc_path_locale && *etr->pc_path_locale)
	    update_mserv_data_from_file (etr->pc_path_locale, track);
	else
	    display_mserv_problems (track, _("no filename available"));
    }
    release_widgets ();
    /* display log of problems with mserv data */
    display_mserv_problems (NULL, NULL);
    gtkpod_statusbar_message(_("Updated selected tracks with data from mserv."));
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
    iTunesDB *itdb;

    g_return_if_fail (plitem);
    g_return_if_fail (track);
    itdb = plitem->itdb;
    g_return_if_fail (itdb);

    /* only add if @track isn't already a member of the current
       playlist */
    if (!itdb_playlist_contains_track (plitem, track))
	gp_playlist_add_track (plitem, track, TRUE);
}


/* Synchronize the directory @key (gchar *dir). Called by
 * sync_tracks().
 * @user_data: selected playlist */
static void sync_dir (gpointer key,
		      gpointer value,
		      gpointer user_data)
{
    gchar *dir = (gchar *)key;
    gchar *charset = (gchar *)value;
    Playlist *pl = (Playlist *)user_data;
    iTunesDB *itdb;
    gchar *buf = NULL;
    gchar *dir_utf8 = NULL;
    /* old charset */
    gchar *prefs_charset = NULL;

    g_return_if_fail (dir);
    if (!g_file_test (dir, G_FILE_TEST_IS_DIR))  g_return_if_reached();
    g_return_if_fail (pl);
    itdb = pl->itdb;
    g_return_if_fail (itdb);

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
    add_directory_by_name (itdb, dir, pl, FALSE, sync_addtrackfunc, NULL);

    /* reset charset */
    if (!prefs_get_update_charset () && charset)
	prefs_set_charset (prefs_charset);
    g_free (prefs_charset);
}



/* cancel handler for sync_remove */
/* @user_data1 is NULL, @user_data2 are the selected tracks */
static void sync_remove_cancel (struct DeleteData *dd)
{
    g_return_if_fail (dd);

    g_list_free (dd->selected_tracks);
    g_free (dd);

    gtkpod_statusbar_message (_("Syncing completed. No files deleted."));
}


struct SyncData
{
    GHashTable *hash_tosync;
    GHashTable *hash_removed;
    Playlist *pl;  /* currently selected playlist */
};

static void sync_dir_ok (gpointer user_data1, gpointer user_data2)
{
    struct SyncData *sd = user_data1;
    /* state of "update existing" */
    gboolean update;
    iTunesDB *itdb;

    g_return_if_fail (sd);
    g_return_if_fail (sd->pl);
    itdb = sd->pl->itdb;
    g_return_if_fail (itdb);

    /* set "update existing" to TRUE */
    update = prefs_get_update_existing();
    prefs_set_update_existing (TRUE);

    /* sync all dirs stored in the hash */
    g_hash_table_foreach (sd->hash_tosync, sync_dir, sd->pl);

    /* reset "update existing" */
    prefs_set_update_existing (update);

    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log updated tracks */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    gp_duplicate_remove (NULL, NULL);

    if (prefs_get_sync_remove ())
    {   /* remove tracks that are no longer present in the dirs */
	GList *tracklist = NULL;
	struct DeleteData *dd;
	GList *gl;

	/* add all tracks that are no longer present in the dirs */
	for (gl=itdb->tracks; gl; gl=gl->next)
	{
	    ExtraTrackData *etr;
	    Track *tr = gl->data;
	    g_return_if_fail (tr);
	    etr = tr->userdata;
	    g_return_if_fail (etr);

	    if (etr->pc_path_locale && *etr->pc_path_locale)
	    {
		gchar *dirname = g_path_get_dirname (etr->pc_path_locale);
		if (g_hash_table_lookup (sd->hash_tosync, dirname) &&
		    !g_file_test (etr->pc_path_locale,G_FILE_TEST_EXISTS))
		{
		    tracklist = g_list_append (tracklist, tr);
		}
		else if (g_hash_table_lookup (sd->hash_removed, dirname))
		{
		    tracklist = g_list_append (tracklist, tr);
		}
		g_free (dirname);
	    }
	}

	dd = g_malloc0 (sizeof (struct DeleteData));
	dd->pl = sd->pl;
	dd->itdb = itdb;
	dd->selected_tracks = tracklist;
	dd->deleteaction = -1;
	if (itdb->usertype & GP_ITDB_TYPE_LOCAL)
	    dd->deleteaction = DELETE_ACTION_DATABASE;
	if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	    dd->deleteaction = DELETE_ACTION_IPOD;
	g_return_if_fail (dd->deleteaction != -1);

	if (tracklist)
	{
	    gchar *label, *title;
	    GString *str;
	    GtkResponseType response;

	    /* populate "title" and "label" */
	    delete_populate_settings (dd,
				      &label, &title,
				      NULL, NULL, NULL);

	    /* create a list of tracks */
	    str = g_string_sized_new (2000);
	    for(gl=tracklist; gl; gl=gl->next)
	    {
		ExtraTrackData *etr;
		Track *tr = gl->data;
		g_return_if_fail (tr);
		etr = tr->userdata;
		g_return_if_fail (etr);
		g_string_append_printf (str, "%s (%s-%s)\n",
					etr->pc_path_utf8,
					tr->artist, tr->title);
	    }
	    /* open window */
	    response = gtkpod_confirmation
		(-1,                   /* gint id, */
		 TRUE,                /* gboolean modal, */
		 title,                /* title */
		 label,                /* label */
		 str->str,             /* scrolled text */
		 _("Never delete any files when syncing"),/* opt 1 text */
		 CONF_STATE_INVERT_FALSE,                 /* opt 1 state */
		 prefs_set_sync_remove,                   /* opt 1 handler */
		 NULL, 0, NULL,        /* option 2 */
		 prefs_get_sync_remove_confirm(), /* gboolean confirm_again, */
		 prefs_set_sync_remove_confirm,   /* confirm_again_handler,*/
		 CONF_NULL_HANDLER,    /* ConfHandler ok_handler,*/
		 NULL,                 /* don't show "Apply" button */
		 CONF_NULL_HANDLER,    /* cancel_handler,*/
		 NULL,                 /* gpointer user_data1,*/
		 NULL);                /* gpointer user_data2,*/

	    switch (response)
	    {
	    case GTK_RESPONSE_OK:
		delete_track_ok (dd);
		break;
	    default:
		sync_remove_cancel (dd);
		break;
	    }

	    g_free (label);
	    g_string_free (str, TRUE);
	}
	else sync_remove_cancel (dd);
    }
    else
    {
	gtkpod_statusbar_message (_("Syncing completed."));
    }
    /* destroy the hash tables */
    g_hash_table_destroy (sd->hash_tosync);
    g_hash_table_destroy (sd->hash_removed);
    g_free (sd);
}


/* sync dirs cancelled */
static void sync_dir_cancel (gpointer user_data1, gpointer user_data2)
{
    struct SyncData *sd = user_data1;

    if (sd)
    {
	g_hash_table_destroy (sd->hash_tosync);
	g_hash_table_destroy (sd->hash_removed);
	g_free (sd);
    }
    gtkpod_statusbar_message (_("Syncing aborted"));
}


/* Sync all directories referred to by the pc_path_locale entries in
   the selected tracks */
void sync_tracks (GList *selected_tracks)
{
    GHashTable *hash_tosync;
    GHashTable *hash_removed;
    struct SyncData *sd;
    GList *gl;
    guint32 dirnum = 0;
    /* Append @key (gchar *dir) to GString *user_data. Charset to use is
     * @value. If @value is not set, use the charset specified in the
     * preferences */
    void sync_add_dir_to_string  (gpointer key,
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

    if (selected_tracks == NULL)
    {
	gtkpod_statusbar_message (_("No tracks in selection"));
	return;
    }

    /* Create a hash to keep the directory names ("key", and "value"
       to be freed with g_free). key is dirname, value is charset used
       */
    hash_tosync = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    hash_removed = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    /* Get the dirs of all tracks selected and enter them into the hash
       table if the dir exists. Using a hash table automatically
       removes duplicates */
    for (gl = selected_tracks; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	Track *track = gl->data;
	g_return_if_fail (track);
	etr = track->userdata;
	g_return_if_fail (etr);

	if (etr->pc_path_locale && *etr->pc_path_locale)
	{
	    gchar *dirname = g_path_get_dirname (etr->pc_path_locale);

	    ++dirnum;
	    if (g_file_test (dirname, G_FILE_TEST_IS_DIR))
	    {
		if (etr->charset && *etr->charset)
		{   /* charset set -- just insert the entry */
		    gchar *charset = g_strdup (etr->charset);
		    g_hash_table_insert (hash_tosync, dirname, charset);
		}
		else
		{   /* no charset -- make sure we don't replace a dir
		     * entry that had the charset set */
		    if (!g_hash_table_lookup (hash_tosync, dirname))
		    {
			g_hash_table_insert (hash_tosync, dirname, NULL);
		    }
		    else
		    {
			g_free (dirname);
		    }
		}
	    }
	    else
	    {
		if (g_file_test (dirname, G_FILE_TEST_EXISTS))
		{
		    gtkpod_warning (_("'%s' is not a directory. Ignored.\n"),
				    dirname);
		    g_free (dirname);
		}
		else
		{   /* add to directories to be removed */
		    if (etr->charset && *etr->charset)
		    {   /* charset set -- just insert the entry */
			gchar *charset = g_strdup (etr->charset);
			g_hash_table_insert (hash_removed, dirname, charset);
		    }
		    else
		    {   /* no charset -- make sure we don't replace a
			 * dir entry that had the charset set */
			if (!g_hash_table_lookup (hash_removed, dirname))
			{
			    g_hash_table_insert (hash_removed, dirname, NULL);
			}
			else
			{
			    g_free (dirname);
			}
		    }
		}
	    }
	}
    }
    if ((g_hash_table_size (hash_tosync) +
	 g_hash_table_size (hash_removed)) == 0)
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
	if (g_hash_table_size (hash_removed) != 0)
	{
	    g_string_printf (str, 
			     _("The (former) contents of the following directories will be removed from the database:\n\n"));
	    g_hash_table_foreach (hash_removed,
				  sync_add_dir_to_string, str);
	    g_string_append_printf (str, "\n\n");
	}
	if (g_hash_table_size (hash_tosync) != 0)
	{
	    g_string_append_printf (str, 
			     _("The following directories will be synchronized:\n\n"));
	    g_hash_table_foreach (hash_tosync,
				  sync_add_dir_to_string, str);
	}
	sd = g_malloc (sizeof (struct SyncData));
	sd->hash_tosync = hash_tosync;
	sd->hash_removed = hash_removed;
	sd->pl = pm_get_selected_playlist ();
	if (gtkpod_confirmation (
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
		NULL,                   /* don't show "Apply" */
		sync_dir_cancel,        /* cancel_handler,*/
		sd,                     /* gpointer user_data1,*/
		NULL)                   /* gpointer user_data2,*/
	    != GTK_RESPONSE_ACCEPT)
	{ /* creation failed */
	    g_hash_table_destroy (sd->hash_tosync);
	    g_hash_table_destroy (sd->hash_removed);
	    g_free (sd);
	}
	g_string_free (str, TRUE);
    }
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
		CONF_NULL_HANDLER,  /* ConfHandler ok_handler,*/
		NULL,               /* don't show "Apply" button */
		NULL,               /* cancel_handler,*/
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
       buf = get_track_info (track, TRUE);
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
		CONF_NULL_HANDLER,  /* ConfHandler ok_handler,*/
		NULL,               /* don't show "Apply" button */
		NULL,               /* cancel_handler,*/
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
       buf = get_track_info (track, TRUE);
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


/* Logs tracks (@track) that could not be updated with info from mserv
   database for some reason. Pop up a window with the log by calling
   with @track = NULL, or remove the log by calling with @track = -1.
   @txt (if available) is added as an explanation as to why it was
   impossible to retrieve mserv information */
void display_mserv_problems (Track *track, gchar *txt)
{
   gchar *buf;
   static gint track_nr = 0;
   static GString *str = NULL;

   if ((track == NULL) && str)
   {
       if (prefs_get_mserv_use() &&
	   prefs_get_mserv_report_probs() && str->len)
       { /* Some tracks have had problems. Print a notice */
	   buf = g_strdup_printf (
	       ngettext ("No mserv information could be retrieved for the following track",
			 "No mserv information could be retrieved for the following %d tracks",
			 track_nr), track_nr);
	   gtkpod_confirmation
	       (-1,                 /* gint id, */
		FALSE,              /* gboolean modal, */
		_("mserv data retrieval problem"),   /* title */
		buf,                /* label */
		str->str,           /* scrolled text */
		NULL, 0, NULL,          /* option 1 */
		NULL, 0, NULL,          /* option 2 */
		TRUE,               /* gboolean confirm_again, */
		prefs_set_mserv_report_probs,/* confirm_again_handler,*/
		CONF_NULL_HANDLER,  /* ConfHandler ok_handler,*/
		NULL,               /* don't show "Apply" button */
		NULL,               /* cancel_handler,*/
		NULL,               /* gpointer user_data1,*/
		NULL);              /* gpointer user_data2,*/
	   g_free (buf);
       }
       display_mserv_problems ((void *)-1, NULL);
   }

   if (track == (void *)-1)
   { /* clean up */
       if (str)       g_string_free (str, TRUE);
       str = NULL;
       track_nr = 0;
       gtkpod_tracks_statusbar_update();
   }
   else if (prefs_get_mserv_use() &&
	    prefs_get_mserv_report_probs() && track)
   {
       /* add info about it to str */
       buf = get_track_info (track, TRUE);
       if (!str)
       {
	   track_nr = 0;
	   str = g_string_sized_new (2000); /* used to keep record */
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
void update_track_from_file (iTunesDB *itdb, Track *track)
{
    ExtraTrackData *etr;
    Track *oldtrack;
    gchar *prefs_charset = NULL;
    gchar *trackpath = NULL;
    gint32 oldsize = 0;
    gboolean charset_set;

    g_return_if_fail (itdb);
    g_return_if_fail (track);
    etr = track->userdata;
    g_return_if_fail (etr);

    /* remember size of track on iPod */
    if (track->transferred) oldsize = track->size;
    else                    oldsize = 0;

    /* remember if charset was set */
    if (etr->charset)  charset_set = TRUE;
    else               charset_set = FALSE;

    if (!prefs_get_update_charset () && charset_set)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used when first importing the track */
	prefs_set_charset (etr->charset);
    }

    if (etr->pc_path_locale)
    {   /* need to copy because we cannot pass track->pc_path_locale to
	get_track_info_from_file () since track->pc_path gets g_freed
	there */
	trackpath = g_strdup (etr->pc_path_locale);
    }

    if (!(etr->pc_path_locale && *etr->pc_path_locale))
    { /* no path available */
	display_non_updated (track, _("no filename available"));
    }
    else if (get_track_info_from_file (trackpath, track))
    { /* update successfull */
	/* remove track from md5 hash and reinsert it
	   (hash value may have changed!) */
	gchar *oldhash = etr->md5_hash;
	md5_track_remove (track);
	/* need to remove the old value manually! */
	etr->md5_hash = NULL;
	oldtrack = md5_track_exists_insert (itdb, track);
	if (oldtrack) { /* track exists, remove old track
			  and register the new version */
	    md5_track_remove (oldtrack);
	    gp_duplicate_remove (track, oldtrack);
	    md5_track_exists_insert (itdb, track);
	}
	/* track may have to be copied to iPod on next export */
	/* since it will copied under the same name as before, we
	   don't have to manually remove it */
	if (oldhash && etr->md5_hash)
	{   /* do we really have to copy the track again? */
	    if (strcmp (oldhash, etr->md5_hash) != 0)
	    {
		track->transferred = FALSE;
		data_changed (itdb);
	    }
	}
	else
	{   /* no hash available -- copy! */
	    track->transferred = FALSE;
	    data_changed (itdb);
	}
	/* set old size if track has to be transferred (for free space
	 * calculation) */
	if (!track->transferred) etr->oldsize = oldsize;
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

/* Append file @fname to the list of tracks.
   @fname is in the current locale
   @plitem: if != NULL, add track to plitem as well (unless it's the MPL)
   descend: TRUE:  add directories recursively
            FALSE: add contents of directories passed but don't descend
                   into its subdirectories */
/* @addtrackfunc: if != NULL this will be called instead of
   "add_track_to_playlist () -- used for dropping tracks at a specific
   position in the track view */
gboolean add_track_by_filename (iTunesDB *itdb, gchar *fname,
				Playlist *plitem, gboolean descend,
			       AddTrackFunc addtrackfunc, gpointer data)
{
  static gint count = 0; /* do a gtkpod_tracks_statusbar_update() every
			    10 tracks */
  Track *oldtrack;
  gchar str[PATH_MAX];
  gchar *basename;
  Playlist *mpl;
  gboolean result = TRUE;

  g_return_val_if_fail (fname, FALSE);
  g_return_val_if_fail (itdb, FALSE);
  mpl = itdb_playlist_mpl (itdb);
  g_return_val_if_fail (mpl, FALSE);

  if (!plitem)  plitem = mpl;

  if (g_file_test (fname, G_FILE_TEST_IS_DIR))
  {
      return add_directory_by_name (itdb, fname, plitem, descend, addtrackfunc, data);
  }

  /* check if file is a playlist */
  switch (determine_file_type(fname)) {
	  case FILE_TYPE_M3U:
	  case FILE_TYPE_PLS:
	      if (add_playlist_by_filename (itdb, fname, plitem, -1,
					    addtrackfunc, data))
		  return TRUE;
	      return FALSE;
	  case FILE_TYPE_MP3:
	  case FILE_TYPE_M4A:
	  case FILE_TYPE_M4P:
	  case FILE_TYPE_M4B:
	  case FILE_TYPE_UNKNOWN:
		  break;
  }

  /* print a message about which file is being processed */
  basename = g_path_get_basename (fname);
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
  oldtrack = gp_track_by_filename (itdb, fname);
  /* If a track already exists in the database, either update it or
     just add it to the current playlist (if it doesn't already exist) */
  if (oldtrack)
  {
      if (prefs_get_update_existing ())
      {   /* update the information */
	  update_track_from_file (itdb, oldtrack);
      }
      /* add to current playlist if it's not already in there */
      if (!itdb_playlist_is_mpl (plitem))
      {
	  if (!itdb_playlist_contains_track (plitem, oldtrack))
	  {
	      if (addtrackfunc)
		  addtrackfunc (plitem, oldtrack, data);
	      else
		  gp_playlist_add_track (plitem, oldtrack, TRUE);
	  }
      }
  }
  else  /* oldtrack == NULL */
  {   /* OK, the same filename does not already exist */
      Track *track = get_track_info_from_file (fname, NULL);
      if (track)
      {
          Track *added_track = NULL;
	  ExtraTrackData *etr = track->userdata;
	  g_return_val_if_fail (etr, FALSE);

	  track->id = 0;
	  track->transferred = FALSE;

	  /* is 'fname' on the iPod? -- if yes mark as transfered, if
	   * it's in the ipod_control directory */
	  if (itdb->usertype & GP_ITDB_TYPE_IPOD)
	  {
	      g_return_val_if_fail (itdb->mountpoint, FALSE);
	      if (strstr (fname, itdb->mountpoint) == fname)
	      {   /* Yes */
		  /* is 'fname' in the iPod_Control directory? */
		  gchar *fname_i = fname + strlen (itdb->mountpoint);
		  gchar *fname_l;
		  if (*fname_i == G_DIR_SEPARATOR) ++fname_i;
		  fname_l = g_ascii_strdown (fname_i, -1);
		  if (strstr (fname_l, "ipod_control") == fname_l)
		  {   /* Yes */
		      track->transferred = TRUE;
		      track->ipod_path = g_strdup_printf (
			  "%c%s", G_DIR_SEPARATOR, fname_i);
		      itdb_filename_fs2ipod (track->ipod_path);
		  }
		  g_free (fname_l);
	      }
	  }

	  if (gethostname (str, PATH_MAX-2) == 0)
	  {
	      str[PATH_MAX-1] = 0;
	      etr->hostname = g_strdup (str);
	  }
	  /* add_track may return pointer to a different track if an
	     identical one (MD5 checksum) was found */
	  added_track = gp_track_add (itdb, track);
	  g_return_val_if_fail (added_track, FALSE);

	  /* set flags to 'podcast' if adding to podcast list */
	  if (itdb_playlist_is_podcasts (plitem))
	      gp_track_set_flags_podcast (added_track);

	  if (addtrackfunc)
	  {
	      if (itdb_playlist_is_mpl (plitem))
	      {   /* add track to master playlist if it wasn't a
		     duplicate */
		  if (added_track == track)
		      addtrackfunc (plitem, added_track, data);
	      }
	      else
	      {   /* add track to master playlist if it wasn't a
		   * duplicate and plitem is not the podcasts playlist
		   */
		  if (added_track == track)
		  {
		      if (!itdb_playlist_is_podcasts (plitem))
			  gp_playlist_add_track (mpl, added_track, TRUE);
		  }
		  /* add track to specified playlist */
		  addtrackfunc (plitem, added_track, data);
	      }
	  }
	  else  /* no addtrackfunc */
	  {
	      if (itdb_playlist_is_mpl (plitem))
	      {
		  /* add track to master playlist if it wasn't a
		   * duplicate */
		  if (added_track == track)
		      gp_playlist_add_track (plitem, added_track,
					     TRUE);
	      }
	      else
	      {
		  /* add track to master playlist if it wasn't a
		   * duplicate and plitem is not the podcasts playlist
		   */
		  if (added_track == track)
		  {
		      if (!itdb_playlist_is_podcasts (plitem))
			  gp_playlist_add_track (mpl, added_track, TRUE);
		  }
		  /* add track to specified playlist */
		  gp_playlist_add_track (plitem, added_track, TRUE);
	      }
	  }
	  /* indicate that non-transferred files exist */
	  data_changed (itdb);
	  ++count;
	  if (count >= 10)  /* update every ten tracks added */
	  {
	      gtkpod_tracks_statusbar_update();
	      count = 0;
	  }
      }
      else
      { /* !track */
	  result = FALSE;
      }
  }
  while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
  return result;
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
        switch (determine_file_type(name)) {
	    case FILE_TYPE_MP3:
		return mp3_write_file_info (name, track);
	    case FILE_TYPE_M4A:
	    case FILE_TYPE_M4P:
	    case FILE_TYPE_M4B:
		return mp4_write_file_info (name, track);
	    case FILE_TYPE_WAV:
		return wav_write_file_info (name, track);
	    case FILE_TYPE_M3U:
	    case FILE_TYPE_PLS:
	    case FILE_TYPE_UNKNOWN:
		break;
	}
    }
    return FALSE;
}


/* Write tags to file */
gboolean write_tags_to_file (Track *track)
{
    ExtraTrackData *etr;
    iTunesDB *itdb;
    gchar *ipod_fullpath;
    gchar *prefs_charset = NULL;
    Track *oldtrack;
    gboolean track_charset_set;

    g_return_val_if_fail (track, FALSE);
    etr = track->userdata;
    g_return_val_if_fail (etr, FALSE);
    itdb = track->itdb;
    g_return_val_if_fail (itdb, FALSE);

    /* if we are to use the charset used when first importing
       the track, change the prefs settings temporarily */
    if (etr->charset)  track_charset_set = TRUE;
    else               track_charset_set = FALSE;
    if (!prefs_get_write_charset () && track_charset_set)
    {   /* we should use the initial charset for the update */
	if (prefs_get_charset ())
	{   /* remember the charset originally set */
	    prefs_charset = g_strdup (prefs_get_charset ());
	}
	/* use the charset used when first importing the track */
	prefs_set_charset (etr->charset);
    }
    else
    {   /* we should update the track->charset information */
	update_charset_info (track);
    }

    if (etr->pc_path_locale && (strlen (etr->pc_path_locale) > 0))
    {
	if (file_write_info (
		etr->pc_path_locale, track) == FALSE)
	{
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    etr->pc_path_locale);
	}
    }
    if (!prefs_get_offline () &&
	track->transferred &&
	track->ipod_path &&
	(g_utf8_strlen (track->ipod_path, -1) > 0))
    {
	/* need to get ipod filename */
	ipod_fullpath = get_file_name_on_ipod (track);
	if (file_write_info (
		ipod_fullpath, track) == FALSE)
	{
	    gtkpod_warning (_("Couldn't change tags of file: %s\n"),
			    ipod_fullpath);
	}
	g_free (ipod_fullpath);
    }
    /* remove track from md5 hash and reinsert it (hash value has changed!) */
    md5_track_remove (track);
    C_FREE (etr->md5_hash);  /* need to remove the old value manually! */
    oldtrack = md5_track_exists_insert (itdb, track);
    if (oldtrack) { /* track exists, remove and register the new version */
	md5_track_remove (oldtrack);
	gp_duplicate_remove (track, oldtrack);
	md5_track_exists_insert (itdb, track);
    }

    if (!prefs_get_write_charset () && track_charset_set)
    {   /* reset charset */
	prefs_set_charset (prefs_charset);
    }
    g_free (prefs_charset);
    return TRUE;
}


/**
 * get_file_name
 * Function to retrieve the filename on disk for the specified Track.  It
 * returns the valid filename whether the file has been copied to the ipod,
 * or has yet to be copied.  So it's useful for file operations on a track.
 * @s - The Track data structure we want the on disk file for
 * Returns - the filename for this Track. Must be g_free'd.
 */
gchar* get_file_name(Track *tr)
{
    ExtraTrackData *etr;
    gchar *result = NULL;

    g_return_val_if_fail (tr, result);
    etr = tr->userdata;
    g_return_val_if_fail (etr, result);

    if (tr->itdb)
	result = get_file_name_on_ipod (tr);
    if(!result &&
       (etr->pc_path_locale) && (strlen(etr->pc_path_locale) > 0))
    {
	result = g_strdup (etr->pc_path_locale);
    }
    return result;
}


/* Return the full iPod filename as stored in @s.
   @s: track
   Return value: full filename to @s on the iPod or NULL if no
   filename is set in @s. NOTE: the file does not necessarily
   exist. NOTE: the in itunesdb.c code works around a problem on some
   systems (see below) and might return a filename with different case
   than the original filename. Don't copy it back to @s */
gchar *get_file_name_on_ipod (Track *tr)
{
    gchar *result = NULL;

    if(tr &&  !prefs_get_offline ())
    {
	result = itdb_filename_on_ipod (tr);
    }
    return(result);
}


/* Return the full path of track @tr on the local harddisk or NULL if
 * no path was stored. Return value must be g_free'd when it is no
 * longer needed */
gchar *get_file_name_on_harddisk (Track *tr)
{
    ExtraTrackData *etr;
    gchar *result = NULL;

    g_return_val_if_fail (tr, NULL);
    etr = tr->userdata;
    g_return_val_if_fail (etr, result);

    if ((etr->pc_path_locale) && (strlen(etr->pc_path_locale) > 0))
    {
	result = g_strdup (etr->pc_path_locale);
    }
    return result;
}


/* Like get_file_name(), but verifies the track actually exists.  Must
   g_free return value after use */
gchar *get_file_name_verified (Track *track)
{
    gchar *name = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    if (!prefs_get_offline ())
    {
	name = get_file_name_on_ipod (track);
	if (name)
	{
	    if (!g_file_test (name, G_FILE_TEST_EXISTS))
	    {
		g_free (name);
		name = NULL;
	    }
	}
    }
    if(!name && etr->pc_path_locale && (*etr->pc_path_locale))
    {
	if (g_file_test (etr->pc_path_locale, G_FILE_TEST_EXISTS))
	    name = g_strdup (etr->pc_path_locale);
    }
    return name;
}


/* Get file name from source @source */
gchar *get_file_name_from_source (Track *track, FileSource source)
{
    gchar *result = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    switch (source)
    {
    case SOURCE_PREFER_LOCAL:
	if (etr->pc_path_locale && (*etr->pc_path_locale))
	{
	    if (g_file_test (etr->pc_path_locale, G_FILE_TEST_EXISTS))
	    {
		result = g_strdup (etr->pc_path_locale);
	    }
	}
	if (!result) result = get_file_name_on_ipod (track);
	break;
    case SOURCE_LOCAL:
	if (etr->pc_path_locale && (*etr->pc_path_locale))
	{
	    if (g_file_test (etr->pc_path_locale, G_FILE_TEST_EXISTS))
	    {
		result = g_strdup (etr->pc_path_locale);
	    }
	}
	break;
    case SOURCE_IPOD:
	result = get_file_name_on_ipod (track);
	break;
    }
    return result;
}






/* There seems to be a problem with some distributions
      (kernel versions or whatever -- even identical version
      numbers don't don't show identical behaviour...): even
      though vfat is supposed to be case insensitive, a
     difference is made between upper and lower case under
     some special circumstances. As in
     "/iPod_Control/Music/F00" and "/iPod_Control/Music/f00
     "... If the former filename does not exist, we try to find an
     existing case insensitive match for each component of the
     filename.  If we can find such a match, we return it.  Otherwise,
     we return NULL.*/
     
   /* We start by assuming that the ipod mount point exists.  Then,
    * for each component c of track->ipod_path, we try to find an
    * entry d in good_path that is case-insensitively equal to c.  If
    * we find d, we append d to good_path and make the result the new
    * good_path.  Otherwise, we quit and return NULL.  @root: in local
    * encoding, @components: in utf8
 */
gchar * resolve_path(const gchar *root,const gchar * const * components) {
  gchar *good_path = g_strdup(root);
  guint32 i;

  if (!root) return NULL;
    
  for(i = 0 ; components[i] ; i++) {
    GDir *cur_dir;
    gchar *component_as_filename;
    gchar *test_path;
    gchar *component_stdcase;
    const gchar *dir_file=NULL;

    /* skip empty components */
    if (strlen (components[i]) == 0) continue;
    component_as_filename = 
      g_filename_from_utf8(components[i],-1,NULL,NULL,NULL);
    test_path = g_build_filename(good_path,component_as_filename,NULL);
    g_free(component_as_filename);
    if(g_file_test(test_path,G_FILE_TEST_EXISTS)) {
      /* This component does not require fixup */
      g_free(good_path);
      good_path = test_path;
      continue;
    }
    g_free(test_path);
    component_stdcase = g_utf8_casefold(components[i],-1);
    /* Case insensitively compare the current component with each entry
     * in the current directory. */

    cur_dir = g_dir_open(good_path,0,NULL);
    if (cur_dir) while ((dir_file = g_dir_read_name(cur_dir)))
    {
	gchar *file_utf8 = g_filename_to_utf8(dir_file,-1,NULL,NULL,NULL);
	gchar *file_stdcase = g_utf8_casefold(file_utf8,-1);
	gboolean found = !g_utf8_collate(file_stdcase,component_stdcase);
	gchar *new_good_path;
	g_free(file_stdcase);
	if(!found)
	{
	    /* This is not the matching entry */
	    g_free(file_utf8);
	    continue;
	}
      
	new_good_path = dir_file ? g_build_filename(good_path,dir_file,NULL) : NULL;
	g_free(good_path);
	good_path= new_good_path;
	/* This is the matching entry, so we can stop searching */
	break;
    }
    
    if(!dir_file) {
      /* We never found a matching entry */
      g_free(good_path);
      good_path = NULL;
    }

    g_free(component_stdcase);
    if (cur_dir) g_dir_close(cur_dir);
    if(!good_path || !g_file_test(good_path,G_FILE_TEST_EXISTS))
      break; /* We couldn't fix this component, so don't try later ones */
  }
    
  if(good_path && g_file_test(good_path,G_FILE_TEST_EXISTS))
    return good_path;
          
  return NULL;
}




/* ------------------------------------------------------------

        Reading of offline playcount file

   ------------------------------------------------------------ */

/* Read the ~/.gtkpod/offline_playcount file and adjust the
   playcounts. The tracks will first be matched by their md5 sum, if
   that fails, by their filename.
   If tracks could not be matched, the user will be queried whether to
   forget about them or write them back into the offline_playcount
   file. */
void parse_offline_playcount (void)
{
    gchar *cfgdir = prefs_get_cfgdir ();
    gchar *offlplyc = g_strdup_printf (
	"%s%c%s", cfgdir, G_DIR_SEPARATOR, "offline_playcount");

    if (g_file_test (offlplyc, G_FILE_TEST_EXISTS))
    {
	FILE *file = fopen (offlplyc, "r+");
	size_t len = 0;  /* how many bytes are written */
	gchar *buf;
	GString *gstr, *gstr_filenames;
	if (!file)
	{
	    gtkpod_warning (_("Could not open '%s' for reading and writing.\n"),
		       offlplyc);
	    g_free (offlplyc);
	    return;
	}
	if (flock (fileno (file), LOCK_EX) != 0)
	{
	    gtkpod_warning (_("Could not obtain lock on '%s'.\n"), offlplyc);
	    fclose (file);
	    g_free (offlplyc);
	    return;
	}
	buf = g_malloc (2*PATH_MAX);
	gstr = g_string_sized_new (PATH_MAX);
	gstr_filenames = g_string_sized_new (PATH_MAX);
	while (fgets (buf, 2*PATH_MAX, file))
	{
	    gchar *buf_utf8 = charset_to_utf8 (buf);
	    gchar *md5=NULL;
	    gchar *filename=NULL;
	    gchar *ptr1, *ptr2;
	    /* skip strings that do not start with "PLCT:" */
	    if (strncmp (buf, SOCKET_PLYC, strlen (SOCKET_PLYC)) != 0)
	    {
		gtkpod_warning (_("Malformed line in '%s': %s\n"), offlplyc, buf);
		goto cont;
	    }
	    /* start of MD5 string */
	    ptr1 = buf + strlen (SOCKET_PLYC);
	    /* end of MD5 string */
	    ptr2 = strchr (ptr1, ' ');
	    if (ptr2 == NULL)
	    {   /* error! */
		gtkpod_warning (_("Malformed line in '%s': %s\n"),
				offlplyc, buf_utf8);
		goto cont;
	    }
	    if (ptr1 != ptr2)    md5 = g_strndup (ptr1, ptr2-ptr1);
	    /* start of filename */
	    ptr1 = ptr2 + 1;
	    /* end of filename string */
	    ptr2 = strchr (ptr1, '\n');
	    if (ptr2 == NULL)
	    {   /* error! */
		gtkpod_warning (_("Malformed line in '%s': %s\n"),
				offlplyc, buf_utf8);
		goto cont;
	    }
	    if (ptr1 != ptr2)
	    {
		filename = g_strndup (ptr1, ptr2-ptr1);
	    }
	    else
	    {   /* error! */
		gtkpod_warning (_("Malformed line in '%s': %s\n"),
				offlplyc, buf_utf8);
		goto cont;
	    }
	    if (gp_increase_playcount (md5, filename, 1) == FALSE)
	    {   /* didn't find the track -> store */
		gchar *filename_utf8 = charset_to_utf8 (filename);
/* 		if (gstr->len == 0) */
/* 		{ */
/* 		    gtkpod_warning (_("Couldn't find track for playcount adjustment:\n")); */
/* 		} */
		g_string_append (gstr_filenames, filename_utf8);
		g_string_append (gstr_filenames, "\n");
		g_free (filename_utf8);
		g_string_append (gstr, buf);
	    }
	  cont:
	    g_free (buf_utf8);
	    g_free (md5);
	    g_free (filename);
	}

	/* rewind file pointer to beginning */
	rewind (file);
	if (gstr->len != 0)
	{
	    gint result = gtkpod_confirmation
	    (-1,                    /* gint id, */
	     TRUE,                  /* gboolean modal, */
	     _("Remove offline playcounts?"), /* title */
	     _("Some tracks played offline could not be found in the iTunesDB. Press 'OK' to remove them from the offline playcount file, 'Cancel' to keep them."),   /* label */
	     gstr_filenames->str,   /* scrolled text */
	     NULL, 0, NULL,         /* option 1 */
	     NULL, 0, NULL,         /* option 2 */
	     TRUE,                  /* confirm_again, */
	     NULL,                  /* confirm_again_handler,*/
	     CONF_NULL_HANDLER,     /* ConfHandler ok_handler,*/
	     NULL,                  /* don't show "Apply" button */
	     CONF_NULL_HANDLER,     /* cancel_handler,*/
	     NULL,                  /* gpointer user_data1,*/
	     NULL);                 /* gpointer user_data2,*/

	    if (result != GTK_RESPONSE_OK)
	    {
		len = fwrite (gstr->str, sizeof (gchar), gstr->len, file);
		if (len != gstr->len)
		{
		    gtkpod_warning (_("Error writing to '%s'.\n"), offlplyc);
		}
	    }
	}
	ftruncate (fileno (file), len);
	fclose (file);
	g_string_free (gstr, TRUE);
	g_string_free (gstr_filenames, TRUE);
	g_free (buf);
    }
    g_free (cfgdir);
    g_free (offlplyc);
}


/* ------------------------------------------------------------

        Reading of gain tags

   ------------------------------------------------------------ */
/* Set the gain value in @track. Return value: TRUE, if gain could be set */
gboolean get_gain (Track *track) 
{
    gchar *path = get_file_name_verified (track);
    gboolean result = FALSE;

    if (path)
    {
	switch (determine_file_type (path))
	{
	case FILE_TYPE_MP3: 
	    result = mp3_get_gain (path, track);
	    break;
	case FILE_TYPE_M4A: /* FIXME */
	case FILE_TYPE_M4P: /* FIXME */
	case FILE_TYPE_M4B: /* FIXME */
	case FILE_TYPE_WAV: /* FIXME */
	case FILE_TYPE_UNKNOWN: 
	    gtkpod_warning (
		_("Normalization failed: file type not supported.\n"));
	    break;
	case FILE_TYPE_M3U: 
	case FILE_TYPE_PLS: 
	    break;
	}
	g_free (path);
    }
    else
    {
	gtkpod_warning (
	    _("Normalization failed: file not available.\n"));
    }
    return result;
}


