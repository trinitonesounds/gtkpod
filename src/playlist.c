/*
|  Copyright (C) 2002 Adrian Ulrich <pab at blinkenlights.ch>
|                2002-2003 Jörg Schuler  <jcsjcs at users.sourceforge.net>
|
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
#include <string.h>
#include "prefs.h"
#include "playlist.h"
#include "display.h"
#include "misc.h"
#include "file.h"
#include "support.h"

GList *playlists;

/* This function stores the new "plitem" in the global list
   and adds it to the display model.
   "plitem" must have "name", "name_utf16", and "type" initialized
   and all other members set to zero.
   If "plitem" is a master playlist, and another MPL
   already exists, only the name of the new playlist is copied
   and a pointer to the original MPL is returned. This pointer
   has to be used for consequent calls of add_track(id)_to_playlist()
   */
/* @position: if != -1, playlist will be inserted at that position */
Playlist *add_playlist (Playlist *plitem, gint position)
{
    Playlist *mpl;

    if (plitem->id == 0)
    {
	GRand *grand = g_rand_new ();
	plitem->id = ((guint64)g_rand_int (grand) << 32) |
	    ((guint64)g_rand_int (grand));
    }
    if (plitem->type == PL_TYPE_MPL)
    {
	/* it's the MPL */
	mpl = get_playlist_by_nr (0);
	if (mpl != NULL)
	{
	    if (mpl->name)       g_free (mpl->name);
	    if (mpl->name_utf16) g_free (mpl->name_utf16);
	    mpl->name = plitem->name;
	    plitem->name = NULL;
	    mpl->name_utf16 = plitem->name_utf16;
	    plitem->name_utf16 = NULL;
	    pl_free (plitem);
	    pm_name_changed (mpl);  /* Tell the model about the
				       name change */
	    return mpl;
	}
    }
    if (position == -1)  playlists = g_list_append (playlists, plitem);
    else  playlists = g_list_insert (playlists, plitem, position);
    pm_add_playlist (plitem, position);
    return plitem;
}

/* Create new default rule */
SPLRule *splr_new (void)
{
    SPLRule *splr = g_malloc0 (sizeof (SPLRule));
    gint value;
    guint64 v64;

    if (!prefs_get_int_value ("splr_field", &value))
	value = SPLFIELD_ARTIST;
    splr->field = value;

    if (!prefs_get_int_value ("splr_action", &value))
	value = SPLACTION_CONTAINS;
    splr->action = value;

    if (prefs_get_string_value ("splr_string", &splr->string))
	splr->string_utf16 = g_utf8_to_utf16 (splr->string, -1,
					      NULL, NULL, NULL);

    if (!prefs_get_int64_value ("splr_fromvalue", &v64))
	v64 = 0;
    splr->fromvalue = v64;

    if (!prefs_get_int64_value ("splr_fromdate", &v64))
	v64 = 0;
    splr->fromdate = v64;

    if (!prefs_get_int64_value ("splr_fromunits", &v64))
	v64 = 0;
    splr->fromunits = v64;

    if (!prefs_get_int64_value ("splr_tovalue", &v64))
	v64 = 0;
    splr->tovalue = v64;

    if (!prefs_get_int64_value ("splr_todate", &v64))
	v64 = 0;
    splr->todate = v64;

    if (!prefs_get_int64_value ("splr_tounits", &v64))
	v64 = 0;
    splr->tounits = v64;

    return splr;
}


/* add smart rule @splr to playlist @pl at position @pos */
void splr_add (Playlist *pl, SPLRule *splr, gint pos)
{
    g_return_if_fail (pl);
    g_return_if_fail (splr);

    pl->splrules.rules = g_list_insert (pl->splrules.rules,
					splr, pos);
}


/* create a new smart rule and insert it at position @pos of playlist
 * @pl. A pointer to the newly created rule is returned. */
SPLRule *splr_add_new (Playlist *pl, gint pos)
{
    SPLRule *splr;

    g_return_val_if_fail (pl, NULL);

    splr = splr_new ();
    splr_add (pl, splr, pos);
    return splr;
}


/* Return a new playlist struct. It is not entered into the playlist
 * GList */
Playlist *pl_new (gchar *plname, gboolean spl)
{
    Playlist *pl;

    g_return_val_if_fail (plname, NULL);

    pl = g_malloc0 (sizeof (Playlist));
    pl->type = PL_TYPE_NORM;
    pl->name = g_strdup (plname);
    pl->size = 0.0;
    pl->name_utf16 = g_utf8_to_utf16 (plname, -1, NULL, NULL, NULL);
    pl->is_spl = spl;
    if (spl)
    {   /* set preferred values */
	gint value;
	if (!prefs_get_int_value ("spl_liveupdate", &value))
	    value = TRUE;
	pl->splpref.liveupdate = value;

	if (!prefs_get_int_value ("spl_checkrules", &value))
	    value = TRUE;
	pl->splpref.checkrules = value;

	if (!prefs_get_int_value ("spl_checklimits", &value))
	    value = FALSE;
	pl->splpref.checklimits = value;

	if (!prefs_get_int_value ("spl_limittype", &value))
	    value = LIMITTYPE_HOURS;
	pl->splpref.limittype = value;

	if (!prefs_get_int_value ("spl_limitsort", &value))
	    value = LIMITSORT_RANDOM;
	pl->splpref.limitsort = value;

	if (!prefs_get_int_value ("spl_limitvalue", &value))
	    value = 2;
	pl->splpref.limitvalue = value;

	if (!prefs_get_int_value ("spl_matchcheckedonly", &value))
	    value = FALSE;
	pl->splpref.matchcheckedonly = value;

	if (!prefs_get_int_value ("splr_match_operator", &value))
	    value = SPLMATCH_AND;
	pl->splrules.match_operator = value;

	/* add at least one rule */
	splr_add_new (pl, 0);
    }
    return pl;
}


/* Creates a new playlist (spl = TRUE: smart playlist) */
Playlist *add_new_playlist (gchar *plname, gint position, gboolean spl)
{
  Playlist *plitem;
  
  if(!plname) {
    g_assert_not_reached();
    return NULL;
  }

  plitem = pl_new (plname, spl);
  data_changed (); /* indicate that data has changed in memory */
  return add_playlist (plitem, position);
}


/* initializes the playlists by creating the master playlist */
void create_mpl (void)
{
  Playlist *plitem;

  if (playlists != NULL) return;  /* already entries! */
  plitem = g_malloc0 (sizeof (Playlist));
  plitem->type = PL_TYPE_MPL;  /* MPL! */
  plitem->name = g_strdup ("gtkpod");
  plitem->size = 0.0;
  plitem->name_utf16 = g_utf8_to_utf16 (plitem->name, -1, NULL, NULL, NULL);
  add_playlist (plitem, -1);
}


/* This function appends the track with id "id" to the
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist
   @display: if TRUE, track is added the display.  Otherwise it's only
   added to memory */
void add_trackid_to_playlist (Playlist *plitem, guint32 id, gboolean display)
{
  Track *track;

  track = get_track_by_id (id);
  /* printf ("id: %4d track: %x\n", id, track); */
  add_track_to_playlist (plitem, track, display);
}

/* This function appends the track "track" to the
   playlist "plitem". It then lets the display model know.
   If "plitem" == NULL, add to master playlist
   @display: if TRUE, track is added the display.  Otherwise it's only
   added to memory */
void add_track_to_playlist (Playlist *plitem, Track *track, gboolean display)
{
  if (track == NULL) return;
  if (plitem == NULL)  plitem = get_playlist_by_nr (0);
  plitem->members = g_list_append (plitem->members, track);
  ++plitem->num;  /* increase track counter */
  /* it's more convenient to store the pointer to the track than
     the ID, because id=track->ipod_id -- it takes more computing
     power to do it the other way round */
  if (display)  pm_add_track (plitem, track, TRUE);
}

/* This function removes the track with id "id" from the
   playlist "plitem". It then lets the display model know.
   No action is taken if "track" is not in the playlist.
   If "plitem" == NULL, remove from master playlist */
gboolean remove_trackid_from_playlist (Playlist *plitem, guint32 id)
{
  Track *track;

  track = get_track_by_id (id);
  /* printf ("id: %4d track: %x\n", id, track); */
  return remove_track_from_playlist (plitem, track);
}

/* This function removes the track "track" from the
   playlist "plitem". It then lets the display model know.
   No action is taken if "track" is not in the playlist.
   If "plitem" == NULL, remove from master playlist.
   If the track is removed from the MPL, it's also removed
   from memory completely

   Return value: FALSE, if track was not a member, TRUE otherwise */
gboolean remove_track_from_playlist (Playlist *plitem, Track *track)
{
    if (track == NULL) return FALSE;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, track) == NULL)
	return FALSE; /* not a member */
    pm_remove_track (plitem, track);
    plitem->members = g_list_remove (plitem->members, track);
    --plitem->num; /* decrease number of tracks */
    if (plitem->type == PL_TYPE_MPL)
    { /* if it's the MPL, we remove the track permanently */
	gint i, n;
	n = get_nr_of_playlists ();
	for (i = 1; i<n; ++i)
	{  /* first we remove the track from all other playlists (i=1:
	    * skip MPL or we loop */
	    remove_track_from_playlist (get_playlist_by_nr (i), track);
	}
	remove_track_from_ipod (track);
    }
    return TRUE; /* track was a member */
}

/* checks if "track" is in playlist "plitem". TRUE, if yes, FALSE
   otherwise */
gboolean track_is_in_playlist (Playlist *plitem, Track *track)
{
    if (track == NULL) return FALSE;
    if (plitem == NULL)  plitem = get_playlist_by_nr (0);
    if (g_list_find (plitem->members, track) == NULL)
	return FALSE; /* not a member */
    return TRUE;
}

/* returns in how many playlists (other than the MPL) @track is a
   member of */
guint32 track_is_in_playlists (Track *track)
{
    guint32 res = 0;

    if (playlists)
    {
	GList *gl;
	for (gl=playlists->next; gl; gl=gl->next)
	{
	    if (track_is_in_playlist ((Playlist *)gl->data, track)) ++res;
	}
    }
    return res;
}

/* Returns the number of playlists
   (including the MPL, which is nr. 0) */
guint32 get_nr_of_playlists (void)
{
  return g_list_length (playlists);
}

/* Returns the n_th playlist.
   Useful for itunesdb: Nr. 0 should be the MLP */
Playlist *get_playlist_by_nr (guint32 n)
{
  return g_list_nth_data (playlists, n);
}

/* Returns the number of tracks in "plitem" */
/* Stupid function... but please use it :-) */
guint32 get_nr_of_tracks_in_playlist (Playlist *plitem)
{
  return plitem->num;
  /* return g_list_length (plitem->members);*/
}

/* Returns the track nur "n" in playlist "plitem" */
/* If "n >= number of tracks in playlist" return NULL */
Track *get_track_in_playlist_by_nr (Playlist *plitem, guint32 n)
{
  return g_list_nth_data (plitem->members, n);
}


/* Returns the playlist with the ID @id or NULL if the ID cannot be
 * found. */
Playlist *get_playlist_by_id (guint64 id)
{
    GList *gl;

    for (gl=playlists; gl; gl=gl->next)
    {
	Playlist *pl = gl->data;
	if (pl->id == id)  return pl;
    }
    return NULL;
}


/**
 *
 * You must free this yourself
 */
/* Remove playlist from the list. First it's removed from any display
   model using remove_playlist_from_model (), then the entry itself
   is removed from the GList *playlists */
void remove_playlist (Playlist *playlist)
{
  pm_remove_playlist (playlist, TRUE);
  data_changed ();
  pl_free(playlist);
}

/* Move @playlist to the position @pos. It will not move the master
 * playlist, and will not move anything before the master playlist */
void move_playlist (Playlist *playlist, gint pos)
{
    if (!playlist) return;
    if (pos == 0)  return; /* don't move before MPL */

    if (playlist->type == PL_TYPE_MPL)  return; /* don't move MPL */
    playlists = g_list_remove (playlists, playlist);
    playlists = g_list_insert (playlists, playlist, pos);
}


/* Free memory of SPLRule @splr */
static void splr_free (SPLRule *splr)
{
    if (splr)
    {
	g_free (splr->string);
	g_free (splr->string_utf16);
	g_free (splr);
    }
}

/* remove @splr from playlist @pl */
void splr_remove (Playlist *pl, SPLRule *splr)
{
    g_return_if_fail (pl);
    g_return_if_fail (splr);

    pl->splrules.rules = g_list_remove (pl->splrules.rules, splr);
    splr_free (splr);
}


/**
 * pl_free - free the associated memory a playlist contains
 * @playlist - the playlist we'd like to free
 */
void
pl_free(Playlist *playlist)
{
  playlists = g_list_remove (playlists, playlist);
  g_free (playlist->name);
  g_free (playlist->name_utf16);
  g_list_free (playlist->members);
  g_list_foreach (playlist->splrules.rules,
		  (GFunc)(splr_free), NULL);
  g_list_free (playlist->splrules.rules);
  g_free (playlist);
}

/* Remove all playlists from the list using remove_playlist () */
void remove_all_playlists (void)
{
  Playlist *playlist;

  while (playlists != NULL)
    {
      playlist = g_list_nth_data (playlists, 0);
      pm_remove_playlist (playlist, FALSE);
      pl_free (playlist);
    }
}

/***
 * playlist_exists(): return TRUE if the playlist @pl exists, FALSE
 * otherwise
 ***/
gboolean playlist_exists (Playlist *pl)
{
    if (g_list_find (playlists, pl))  return TRUE;
    else                              return FALSE;
}


/** Searches through the playlists starting from position @startfrom
 * and looking for playlist with name  @pl_name
 * @return position of found playlist starting from 1.
 * If it wasn't found - returns 0
 */
guint get_playlist_by_name(gchar *pl_name, guint startfrom)
{
    Playlist *pl;
    guint pos = 0
        , i;

    if (startfrom==0) startfrom++;

    for(i = startfrom; i < get_nr_of_playlists(); i++)
    {
        pl = get_playlist_by_nr (i);
        if(pl->name && (strcmp (pl->name, pl_name) == 0))
        {
            pos = i;
            break;
        }
    }
    return pos;
}


/** If playlist @pl_name doesn't exist, then it will be created
 * and added to the tail of playlists, otherwise pointer to an existing
 * playlist will be returned
 */
Playlist* get_newplaylist_by_name (gchar *pl_name, gboolean spl)
{
    Playlist *res = NULL;
    guint plnum = get_playlist_by_name(pl_name, (guint) 0);

    if (!plnum)
        res = add_new_playlist(pl_name,-1, spl);
    else
        res = get_playlist_by_nr(plnum);

    return res;
}


/* FIXME: this is a bit dangerous. . . we delete all
 * playlists with titles @pl_name and return how many
 * pl have been removed.
 ***/
guint remove_playlist_by_name(gchar *pl_name){
    Playlist *pl;
    guint i;
    guint pl_removed=0;

    for(i = 1; i < get_nr_of_playlists(); i++)
    {
        pl = get_playlist_by_nr (i);
        if(pl->name && (strcmp (pl->name, pl_name) == 0))
        {
            remove_playlist(pl);
         /* we just deleted the ith element of playlists, so
          * we must examine the new ith element. */
            pl_removed++;
            i--;
        }
    }
    return pl_removed;
}


/* Randomize the order of the members of the GList @list */
/* Returns a pointer to the new start of the list */
static GList *randomize_glist (GList *list)
{
    gint32 nr = g_list_length (list);

    while (nr > 1)
    {
	/* get random element among the first nr members */
	gint32 rand = g_random_int_range (0, nr);
	GList *gl = g_list_nth (list, rand);
	/* remove it and add it at the end */
	list = g_list_remove_link (list, gl);
	list = g_list_concat (list, gl);
	--nr;
    }
    return list;
}


/* Randomize the order of the members of the playlist @pl */
void randomize_playlist (Playlist *pl)
{
    g_return_if_fail (pl);

    pl->members = randomize_glist (pl->members);
}


/* Duplicate SPLRule @splr */
SPLRule *splr_duplicate (SPLRule *splr)
{
    SPLRule *dup = NULL;
    if (splr)
    {
	dup = g_malloc (sizeof (SPLRule));
	g_assert (dup);
	memcpy (dup, splr, sizeof (SPLRule));

	/* Now copy the strings */
	dup->string = g_strdup (splr->string);
	dup->string_utf16 = utf16_strdup (splr->string_utf16);
    }
    return dup;
}


/* Duplicate an existing playlist */
Playlist *pl_duplicate (Playlist *pl)
{
    Playlist *pl_dup = NULL;

    if (pl)
    {
	GList *gl;
	pl_dup = g_malloc (sizeof (Playlist));
	g_assert (pl_dup);
	memcpy (pl_dup, pl, sizeof (Playlist));
	/* clear list heads */
	pl_dup->members = NULL;
	pl_dup->splrules.rules = NULL;

	/* Now copy strings */
	pl_dup->name = g_strdup (pl->name);
	pl_dup->name_utf16 = utf16_strdup (pl->name_utf16);

	/* Copy members */
	pl_dup->members = glist_duplicate (pl->members);

	/* Copy rules */
	for (gl=pl->splrules.rules; gl; gl=gl->next)
	{
	    SPLRule *splr_dup = splr_duplicate (gl->data);
	    pl_dup->splrules.rules = g_list_append (
		pl_dup->splrules.rules, splr_dup);
		
	}
    }
    return pl_dup;
}


/* copy all relevant information for smart playlist from playlist @src
   to playlist @dest. Already available information is
   overwritten/deleted. */
void pl_copy_spl_rules (Playlist *dest, Playlist *src)
{
    GList *gl;

    g_return_if_fail (dest != NULL);
    g_return_if_fail (src != NULL);
    g_return_if_fail (dest->is_spl);
    g_return_if_fail (src->is_spl);

    /* remove existing rules */
    g_list_foreach (dest->splrules.rules, (GFunc)(splr_free), NULL);
    g_list_free (dest->splrules.rules);

    /* copy general spl settings */
    memcpy (&dest->splpref, &src->splpref, sizeof (SPLPref));
    memcpy (&dest->splrules, &src->splrules, sizeof (SPLRules));
    dest->splrules.rules = NULL;

    /* Copy rules */
    for (gl=src->splrules.rules; gl; gl=gl->next)
    {
	SPLRule *splr_dup = splr_duplicate (gl->data);
	dest->splrules.rules = g_list_append (
	    dest->splrules.rules, splr_dup);
    }
}




/* -------------------------------------------------------------------
 *
 * smart playlist stuff, adapted from source provided by Samuel "Otto"
 * Wood (sam dot wood at gmail dot com). This part can also be used
 * under a FreeBSD license. You can also contact Samuel for a complete
 * copy of his original C++-classes.
 *
 */


/* function to evaluate a rule's truth against a track */
gboolean splr_eval (SPLRule *splr, Track *track)
{
    SPLFieldType ft;
    SPLActionType at;
    gchar *strcomp = NULL;
    gint64 intcomp = 0;
    gboolean boolcomp = FALSE;
    guint32 datecomp = 0;
    Playlist *playcomp = NULL;
    time_t t;
    guint64 mactime;

    g_return_val_if_fail (splr != NULL, FALSE);
    g_return_val_if_fail (track != NULL, FALSE);

    ft = itunesdb_spl_get_field_type (splr->field);
    at = itunesdb_spl_get_action_type (splr->field, splr->action);

    g_return_val_if_fail (at != splat_invalid, FALSE);

    /* find what we need to compare in the track */
    switch (splr->field)
    {
    case SPLFIELD_SONG_NAME:
	strcomp = track->title;
	break;
    case SPLFIELD_ALBUM:
	strcomp = track->album;
	break;
    case SPLFIELD_ARTIST:
	strcomp = track->artist;
	break;
    case SPLFIELD_GENRE:
	strcomp = track->genre;
	break;
    case SPLFIELD_KIND:
	strcomp = track->fdesc;
	break;
    case SPLFIELD_COMMENT:
	strcomp = track->comment;
	break;
    case SPLFIELD_COMPOSER:
	strcomp = track->composer;
	break;
    case SPLFIELD_GROUPING:
	strcomp = track->grouping;
	break;
    case SPLFIELD_BITRATE:
	intcomp = track->bitrate;
	break;
    case SPLFIELD_SAMPLE_RATE:
	intcomp = track->samplerate;
	break;
    case SPLFIELD_YEAR:
	intcomp = track->year;
	break;
    case SPLFIELD_TRACKNUMBER:
	intcomp = track->track_nr;
	break;
    case SPLFIELD_SIZE:
	intcomp = track->size;
	break;
    case SPLFIELD_PLAYCOUNT:
	intcomp = track->playcount;
	break;
    case SPLFIELD_DISC_NUMBER:
	intcomp = track->cd_nr;
	break;
    case SPLFIELD_BPM:
	intcomp = track->BPM;
	break;
    case SPLFIELD_RATING:
	intcomp = track->rating;
	break;
    case SPLFIELD_TIME:
	intcomp = track->tracklen/1000;
	break;
    case SPLFIELD_COMPILATION:
	boolcomp = track->compilation;
	break;
    case SPLFIELD_DATE_MODIFIED:
	datecomp = track->time_modified;
	break;
    case SPLFIELD_DATE_ADDED:
	datecomp = track->time_created;
	break;
    case SPLFIELD_LAST_PLAYED:
	datecomp = track->time_played;
	break;
    case SPLFIELD_PLAYLIST:
	playcomp = get_playlist_by_id (splr->fromvalue);
	break;
    default: /* unknown field type */
	g_return_val_if_fail (FALSE, FALSE);
    }

    /* actually do the comparison to our rule */
    switch (ft)
    {
    case splft_string:
	if(strcomp && splr->string)
	{
	    gint len1 = strlen (strcomp);
	    gint len2 = strlen (splr->string);
	    switch (splr->action)
	    {
	    case SPLACTION_IS_STRING:
		return (strcmp (strcomp, splr->string) == 0);
	    case SPLACTION_IS_NOT:
		return (strcmp (strcomp, splr->string) != 0);
	    case SPLACTION_CONTAINS:
		return (strstr (strcomp, splr->string) != NULL);
	    case SPLACTION_DOES_NOT_CONTAIN:
		return (strstr (strcomp, splr->string) == NULL);
	    case SPLACTION_STARTS_WITH:
		return (strncmp (strcomp, splr->string, len2) == 0);
	    case SPLACTION_ENDS_WITH:
	    if (len2 > len1)  return FALSE;
	    return (strncmp (strcomp+len1-len2,
			     splr->string, len2) == 0);
	    case SPLACTION_DOES_NOT_START_WITH:
		return (strncmp (strcomp, splr->string,
				 strlen (splr->string)) != 0);
	    case SPLACTION_DOES_NOT_END_WITH:
		if (len2 > len1)  return TRUE;
		return (strncmp (strcomp+len1-len2,
				 splr->string, len2) != 0);
	    };
	}
	return FALSE;
    case splft_int:
	switch(splr->action)
	{
	case SPLACTION_IS_INT:
	    return (intcomp == splr->fromvalue);
	case SPLACTION_IS_NOT_INT:
	    return (intcomp != splr->fromvalue);
	case SPLACTION_IS_GREATER_THAN:
	    return (intcomp > splr->fromvalue);
	case SPLACTION_IS_LESS_THAN:
	    return (intcomp < splr->fromvalue);
	case SPLACTION_IS_IN_THE_RANGE:
	    return ((intcomp < splr->fromvalue &&
		     intcomp > splr->tovalue) ||
		    (intcomp > splr->fromvalue &&
		     intcomp < splr->tovalue));
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	    return ((intcomp < splr->fromvalue &&
		     intcomp < splr->tovalue) ||
		    (intcomp > splr->fromvalue &&
		     intcomp > splr->tovalue));
	}
	return FALSE;
    case splft_boolean:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:	    /* aka "is set" */
	    return (boolcomp != 0);
	case SPLACTION_IS_NOT_INT:  /* aka "is not set" */
	    return (boolcomp == 0);
	}
	return FALSE;
    case splft_date:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:
	    return (datecomp == splr->fromvalue);
	case SPLACTION_IS_NOT_INT:
	    return (datecomp != splr->fromvalue);
	case SPLACTION_IS_GREATER_THAN:
	    return (datecomp > splr->fromvalue);
	case SPLACTION_IS_LESS_THAN:
	    return (datecomp < splr->fromvalue);
	case SPLACTION_IS_NOT_GREATER_THAN:
	    return (datecomp <= splr->fromvalue);
	case SPLACTION_IS_NOT_LESS_THAN:
	    return (datecomp >= splr->fromvalue);
	case SPLACTION_IS_IN_THE_LAST:
	    time (&t);
	    t += (splr->fromdate * splr->fromunits);
	    mactime = itunesdb_time_host_to_mac (t);
	    return (datecomp > mactime);
	case SPLACTION_IS_NOT_IN_THE_LAST:
	    time (&t);
	    t += (splr->fromdate * splr->fromunits);
	    mactime = itunesdb_time_host_to_mac (t);
	    return (datecomp <= mactime);
	case SPLACTION_IS_IN_THE_RANGE:
	    return ((datecomp < splr->fromvalue &&
		     datecomp > splr->tovalue) ||
		    (datecomp > splr->fromvalue &&
		     datecomp < splr->tovalue));
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	    return ((datecomp < splr->fromvalue &&
		     datecomp < splr->tovalue) ||
		    (datecomp > splr->fromvalue &&
		     datecomp > splr->tovalue));
	}
	return FALSE;
    case splft_playlist:
	/* if we didn't find the playlist, just exit instead of
	   dealing with it */
	if (playcomp == NULL) return FALSE;

	switch(splr->action)
	{
	case SPLACTION_IS_INT:	  /* is this track in this playlist? */
	    return (track_is_in_playlist (playcomp, track));
	case SPLACTION_IS_NOT_INT:/* NOT in this playlist? */
	    return (!track_is_in_playlist (playcomp, track));
	}
	return FALSE;
    case splft_unknown:
	g_return_val_if_fail (ft != splft_unknown, FALSE);
	return FALSE;
    default: /* new type: warning to change this code */
	g_return_val_if_fail (FALSE, FALSE);
	return FALSE;
    }
    /* we should never make it out of the above switches alive */
    g_return_val_if_fail (FALSE, FALSE);
    return FALSE;
}

/* local functions to help with the sorting of the list of tracks so
 * that we can do limits */
static gint compTitle (Track *a, Track *b)
{
    return strcmp (a->title, b->title);
}
static gint compAlbum (Track *a, Track *b)
{
    return strcmp (a->album, b->album);
}
static gint compArtist (Track *a, Track *b)
{
    return strcmp (a->artist, b->artist);
}
static gint compGenre (Track *a, Track *b)
{
    return strcmp (a->genre, b->genre);
}
static gint compMostRecentlyAdded (Track *a, Track *b)
{
    return b->time_created - a->time_created;
}
static gint compLeastRecentlyAdded (Track *a, Track *b)
{
    return a->time_created - b->time_created;
}
static gint compMostOftenPlayed (Track *a, Track *b)
{
    return b->time_created - a->time_created;
}
static gint compLeastOftenPlayed (Track *a, Track *b)
{
    return a->time_created - b->time_created;
}
static gint compMostRecentlyPlayed (Track *a, Track *b)
{
    return b->time_played - a->time_played;
}
static gint compLeastRecentlyPlayed (Track *a, Track *b)
{
    return a->time_played - b->time_played;
}
static gint compHighestRating (Track *a, Track *b)
{
    return b->rating - a->rating;
}
static gint compLowestRating (Track *a, Track *b)
{
    return a->rating - b->rating;
}

void spl_populate (Playlist *spl, GList *tracks)
{
    GList *gl;
    GList *sel_tracks = NULL;

    g_return_if_fail (spl);

    /* we only can populate smart playlists */
    if (!spl->is_spl) return;

    /* clear this playlist */
    g_list_free (spl->members);
    spl->members = NULL;
    spl->num = 0;
    data_changed ();

    for (gl=tracks; gl ; gl=gl->next)
    {
	Track *t = gl->data;
	g_return_if_fail (t);
	/* skip non-checked songs if we have to do so (this takes care
	   of *all* the match_checked functionality) */
	if (spl->splpref.matchcheckedonly && (t->checked == 0))
	    continue;
	/* first, match the rules */
	if (spl->splpref.checkrules)
	{   /* if we are set to check the rules */
	    /* start with true for "match all",
	       start with false for "match any" */
	    gboolean matchrules;
	    GList *gl;

	    if (spl->splrules.match_operator == SPLMATCH_AND)
		 matchrules = TRUE;
	    else matchrules = FALSE;
	    /* assume everything matches with no rules */
	    if (spl->splrules.rules == NULL) matchrules = TRUE;
	    /* match all rules */
	    for (gl=spl->splrules.rules; gl; gl=gl->next)
	    {
		SPLRule* splr = gl->data;
		gboolean ruletruth = splr_eval (splr, t);
		if (spl->splrules.match_operator == SPLMATCH_AND)
		{
		    if (!ruletruth)
		    {   /* one rule did not match -- we can stop */
			matchrules = FALSE;
			break;
		    }
		}
		else if (spl->splrules.match_operator == SPLMATCH_OR)
		{
		    if (ruletruth)
		    {   /* one rule matched -- we can stop */
			matchrules = TRUE;
			break;
		    }
		}
	    }
	    if (matchrules)
	    {   /* we have a track that matches the ruleset, append to
		 * playlist for now*/
		sel_tracks = g_list_append (sel_tracks, t);
	    }
	}
	else
	{   /* we aren't checking the rules, so just append to
	       playlist */
		sel_tracks = g_list_append (sel_tracks, t);
	}
    }
    /* no reason to go on if nothing matches so far */
    if (g_list_length (sel_tracks) == 0) return;

    /* do the limits */
    if (spl->splpref.checklimits)
    {
	/* use a double because we may need to deal with fractions
	 * here */
	gdouble runningtotal = 0;
	guint32 trackcounter = 0;
	guint32 tracknum = g_list_length (sel_tracks);

/* 	printf("limitsort: %d\n", spl->splpref.limitsort); */

	/* limit to (number) (type) selected by (sort) */
	/* first, we sort the list */
	switch(spl->splpref.limitsort)
	{
	case LIMITSORT_RANDOM:
	    sel_tracks = randomize_glist (sel_tracks);
	    break;
	case LIMITSORT_SONG_NAME:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compTitle);
	    break;
	case LIMITSORT_ALBUM:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compAlbum);
		break;
	case LIMITSORT_ARTIST:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compArtist);
	    break;
	case LIMITSORT_GENRE:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compGenre);
	    break;
	case LIMITSORT_MOST_RECENTLY_ADDED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compMostRecentlyAdded);
	    break;
	case LIMITSORT_LEAST_RECENTLY_ADDED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLeastRecentlyAdded);
	    break;
	case LIMITSORT_MOST_OFTEN_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compMostOftenPlayed);
	    break;
	case LIMITSORT_LEAST_OFTEN_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLeastOftenPlayed);
	    break;
	case LIMITSORT_MOST_RECENTLY_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compMostRecentlyPlayed);
	    break;
	case LIMITSORT_LEAST_RECENTLY_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLeastRecentlyPlayed);
	    break;
	case LIMITSORT_HIGHEST_RATING:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compHighestRating);
	    break;
	case LIMITSORT_LOWEST_RATING:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLowestRating);
	    break;
	default:
	    g_warning ("Programming error: should not reach this point (default of switch (spl->splpref.limitsort)\n");
	    break;
	}
	/* now that the list is sorted in the order we want, we
	   take the top X tracks off the list and insert them into
	   our playlist */

	while ((runningtotal < spl->splpref.limitvalue) &&
	       (trackcounter < tracknum))
	{
	    gdouble currentvalue=0;
	    Track *t = g_list_nth_data (sel_tracks, trackcounter);

/* 	    printf ("track: %d runningtotal: %lf, limitvalue: %d\n", */
/* 		    trackcounter, runningtotal, spl->splpref.limitvalue); */

	    /* get the next song's value to add to running total */
	    switch (spl->splpref.limittype)
	    {
	    case LIMITTYPE_MINUTES:
		currentvalue = (double)(t->tracklen)/(60*1000);
		break;
	    case LIMITTYPE_HOURS:
		currentvalue = (double)(t->tracklen)/(60*60*1000);
		break;
	    case LIMITTYPE_MB:
		currentvalue = (double)(t->size)/(1024*1024);
		break;
	    case LIMITTYPE_GB:
		currentvalue = (double)(t->size)/(1024*1024*1024);
		break;
	    case LIMITTYPE_SONGS:
		currentvalue = 1;
		break;
	    default:
		g_warning ("Programming error: should not reach this point (default of switch (spl->splpref.limittype)\n");
		break;
	    }
	    /* check to see that we won't actually exceed the
	     * limitvalue */
	    if (runningtotal + currentvalue <=
		spl->splpref.limitvalue)
	    {
		runningtotal += currentvalue;
		/* Add the playlist entry */
		add_track_to_playlist (spl, t, FALSE);
	    }
	    /* increment the track counter so we can look at the next
	       track */
	    trackcounter++;
/* 	    printf ("  track: %d runningtotal: %lf, limitvalue: %d\n", */
/* 		    trackcounter, runningtotal, spl->splpref.limitvalue); */
	}	/* end while */
	/* no longer needed */
	g_list_free (sel_tracks);
	sel_tracks = NULL;
    } /* end if limits enabled */
    else
    {   /* no limits, so stick everything that matched the rules into
	   the playlist */
	spl->members = sel_tracks;
	spl->num = g_list_length (sel_tracks);
	sel_tracks = NULL;
    }
}


/* end of code based on Samuel Wood's work */
/* ------------------------------------------------------------------- */
/* functions used by itunesdb (so we can refresh the display during
 * import */
void it_add_trackid_to_playlist (Playlist *plitem, guint32 id)
{
    static Playlist *last_pl = NULL;
    static GTimeVal *time;
    static float max_count = REFRESH_INIT_COUNT;
    static gint count = REFRESH_INIT_COUNT - 1;
    static gint count_s = 0;
    float ms;
    gchar *buf;

    if (!time) time = g_malloc (sizeof (GTimeVal));

    if (plitem != last_pl)
    {
	max_count = REFRESH_INIT_COUNT;
	count = REFRESH_INIT_COUNT - 1;
	count_s = 0; /* nr of tracks in current playlist */
	g_get_current_time (time);
	last_pl = plitem;
    }
    add_trackid_to_playlist (plitem, id, TRUE);
    --count;
    ++count_s;
    if ((count < 0) && widgets_blocked)
    {
	buf = g_strdup_printf (ngettext ("Added %d+ tracks to playlist '%s'",
					 "Added %d+ tracks to playlist '%s'",
					 count_s), count_s, plitem->name);
	gtkpod_statusbar_message(buf);
	g_free (buf);
	while (gtk_events_pending ())  gtk_main_iteration ();
	ms = get_ms_since (time, TRUE);
	/* average the new and the old max_count */
	max_count *= (1 + 2 * REFRESH_MS / ms) / 3;
	count = max_count - 1;
#if DEBUG_TIMING
	printf("it_a_s ms: %f mc: %f\n", ms, max_count);
#endif
    }
}

Playlist *it_add_playlist (Playlist *plitem)
{
    Playlist *pl = add_playlist (plitem, -1);
    gtkpod_tracks_statusbar_update();
    while (widgets_blocked && gtk_events_pending ())  gtk_main_iteration ();
    return pl;
}



