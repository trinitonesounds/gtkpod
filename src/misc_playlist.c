/* Time-stamp: <2004-12-06 22:41:57 jcs>
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

#include <string.h>
#include "charset.h"
#include "itunesdb.h"
#include "md5.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"


#define DEBUG_MISC 0


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Add new playlist asking user for name                *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Add a new playlist at position @position. The name for the new
 * playlist is queried from the user. A default (@dflt) name can be
 * provided.
 * Return value: the new playlist or NULL if the dialog was
 * cancelled. */
Playlist *add_new_pl_user_name (gchar *dflt, gint position)
{
    Playlist *result = NULL;
    gchar *name = get_user_string (
	_("New Playlist"),
	_("Please enter a name for the new playlist"),
	dflt? dflt:_("New Playlist"),
	NULL, NULL);
    if (name)
    {
	result = add_new_playlist (name, position, FALSE);
	gtkpod_tracks_statusbar_update ();
    }
    return result;
}


/* Add a new playlist or smart playlist at position @position. The
 * name for the new playlist is queried from the user. A default
 * (@dflt) name can be provided.
 * Return value: none. In the case of smart playlists, the playlist
 * will not be created immediately. */
void add_new_pl_or_spl_user_name (gchar *dflt, gint position)
{
    gboolean is_spl = FALSE;
    gchar *name = get_user_string (
	_("New Playlist"),
	_("Please enter a name for the new playlist"),
	dflt? dflt:_("New Playlist"),
	_("Smart Playlist"), &is_spl);

    if (name)
    {
	if (!is_spl)
	{   /* add standard playlist */
	    add_new_playlist (name, position, FALSE);
	    gtkpod_tracks_statusbar_update ();
	}
	else
	{   /* add smart playlist */
	    spl_edit_new (name, position);
	}
    }
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                     Special Playlist Stuff                       *
 *                                                                  *
\*------------------------------------------------------------------*/

/* used for special playlist creation */
typedef gboolean (*PL_InsertFunc)(Track *track, gpointer userdata);

/* generate_category_playlists: Create a playlist for each category
   @cat (T_ARTIST, T_ALBUM, T_GENRE, T_COMPOSER) */
void generate_category_playlists (T_item cat)
{
    Playlist *master_pl;
    gint i;
    gchar *qualifier;

    /* sanity */
    if ((cat != T_ARTIST) && (cat != T_ALBUM) &&
	(cat != T_GENRE) && (cat != T_COMPOSER)) return;

    /* Initialize the "qualifier". It is used to indicate the category of
       automatically generated playlists */
    switch (cat)
    {
    case T_ARTIST:
	qualifier = _("AR:");
	break;
    case T_ALBUM:
	qualifier = _("AL:");
	break;
    case T_GENRE:
	qualifier = _("GE:");
	break;
    case T_COMPOSER:
	qualifier = _("CO:");
	break;
    case T_YEAR:
	qualifier = _("YE:");
	break;
    default:
	qualifier = NULL;
	break;
    }

    /* sanity */
    if (qualifier == NULL) return;

    /* FIXME: delete all playlists named '[<qualifier> .*]' and
     * remember which playlist was selected if it gets deleted */

    master_pl = get_playlist_by_nr (0);

    for(i = 0; i < get_nr_of_tracks_in_playlist (master_pl) ; i++)
    {
	Track *track = g_list_nth_data (master_pl->members, i);
	Playlist *cat_pl = NULL;
	gint j;
	gchar *category = NULL;
	gchar *track_cat = NULL;
	int playlists_len = get_nr_of_playlists();
	gchar yearbuf[12];

	if (cat == T_YEAR)
	{
	    snprintf (yearbuf, 11, "%d", track->year);
	    track_cat = yearbuf;
	}
	else
	{
	    track_cat = track_get_item_utf8 (track, cat);
	}

	if (track_cat)
	{
	    /* some tracks have empty strings in the genre field */
	    if(track_cat[0] == '\0')
	    {
		category = g_strdup_printf ("[%s %s]",
					    qualifier, _("Unknown"));
	    }
	    else
	    {
		category = g_strdup_printf ("[%s %s]",
					    qualifier, track_cat);
	    }

	    /* look for category playlist */
	    for(j = 1; j < playlists_len; j++)
	    {
		Playlist *pl = get_playlist_by_nr (j);

		if(g_ascii_strcasecmp(pl->name, category) == 0)
		{
		    cat_pl = pl;
		    break;
		}
	    }
	    /* or, create category playlist */
	    if(!cat_pl)
	    {
		cat_pl = add_new_playlist(category, -1, FALSE);
	    }

	    add_track_to_playlist(cat_pl, track, TRUE);
	    C_FREE (category);
	}
    }
    gtkpod_tracks_statusbar_update();
}

/* Generate a new playlist containing all the tracks currently
   displayed */
Playlist *generate_displayed_playlist (void)
{
    GList *tracks = tm_get_all_tracks ();
    Playlist *result = generate_new_playlist (tracks);
    g_list_free (tracks);
    return result;
}


/* Generate a new playlist containing all the tracks currently
   selected */
Playlist *generate_selected_playlist (void)
{
    GList *tracks = tm_get_selected_tracks ();
    Playlist *result = generate_new_playlist (tracks);
    g_list_free (tracks);
    return result;
}


/* Generates a playlist containing a random selection of
   prefs_get_misc_track_nr() tracks in random order from the currently
   displayed tracks */
Playlist *generate_random_playlist (void)
{
    GRand *grand = g_rand_new ();
    Playlist *new_pl = NULL;
    gchar *pl_name, *pl_name1;
    GList *rtracks = NULL;
    GList *tracks = tm_get_all_tracks ();
    gint tracks_max = prefs_get_misc_track_nr();
    gint tracks_nr = 0;


    while (tracks && (tracks_nr < tracks_max))
    {
	/* get random number between 0 and g_list_length()-1 */
	gint rn = g_rand_int_range (grand, 0, g_list_length (tracks));
	GList *random = g_list_nth (tracks, rn);
	rtracks = g_list_append (rtracks, random->data);
	tracks = g_list_delete_link (tracks, random);
	++tracks_nr;
    }
    pl_name1 = g_strdup_printf (_("Random (%d)"), tracks_max);
    pl_name = g_strdup_printf ("[%s]", pl_name1);
    new_pl = generate_playlist_with_name (rtracks, pl_name, TRUE);
    g_free (pl_name1);
    g_free (pl_name);
    g_list_free (tracks);
    g_list_free (rtracks);
    g_rand_free (grand);
    return new_pl;
}


void randomize_current_playlist (void)
{
    Playlist *pl= pm_get_selected_playlist ();

    if (!pl)
    {
	gtkpod_statusbar_message (_("No playlist selected"));
	return;
    }

    if (prefs_get_tm_autostore ())
    {
	prefs_set_tm_autostore (FALSE);
	gtkpod_warning (_("Auto Store of track view disabled.\n\n"));
/* 	sort_window_update (); */
    }

    randomize_playlist (pl);

    st_adopt_order_in_playlist ();
    tm_adopt_order_in_sorttab ();
}


static void not_listed_make_track_list (gpointer key, gpointer track,
					gpointer tracks)
{
    *(GList **)tracks = g_list_append (*(GList **)tracks, (Track *)track);
}

/* Generate a playlist containing all tracks that are not part of any
   playlist.
   For this, playlists starting with a "[" (generated playlists) are
   being ignored. */
Playlist *generate_not_listed_playlist (void)
{
    GHashTable *hash = g_hash_table_new (NULL, NULL);
    GList *gl, *tracks=NULL;
    guint32 pl_nr, i;
    gchar *pl_name;
    Playlist *new_pl, *pl;

    /* Create hash with all track/track pairs */
    pl = get_playlist_by_nr (0);
    if (pl)
    {
	for (gl=pl->members; gl != NULL; gl=gl->next)
	{
	    g_hash_table_insert (hash, gl->data, gl->data);
	}
    }
    /* remove all tracks that are members of other playlists */
    pl_nr = get_nr_of_playlists ();
    for (i=1; i<pl_nr; ++i)
    {
	pl = get_playlist_by_nr (i);
	/* skip playlists starting with a '[' */
	if (pl && pl->name && (pl->name[0] != '['))
	{
	    for (gl=pl->members; gl != NULL; gl=gl->next)
	    {
		g_hash_table_remove (hash, gl->data);
	    }
	}
    }

    g_hash_table_foreach (hash, not_listed_make_track_list, &tracks);
    g_hash_table_destroy (hash);
    hash = NULL;

    pl_name = g_strdup_printf ("[%s]", _("Not Listed"));

    new_pl = generate_playlist_with_name (tracks, pl_name, TRUE);
    g_free (pl_name);
    g_list_free (tracks);
    return new_pl;
}


/* Generate a playlist consisting of the tracks in @tracks
 * with @name name. If @del_old ist TRUE, delete any old playlist with
 * the same name. */
Playlist *generate_playlist_with_name (GList *tracks, gchar *pl_name,
				       gboolean del_old)
{
    Playlist *new_pl=NULL;
    gint n = g_list_length (tracks);
    gchar *str;

    if(n>0)
    {
	gboolean select = FALSE;
	GList *l;
	if (del_old)
	{
	    /* currently selected playlist */
	    Playlist *sel_pl= pm_get_selected_playlist ();

	    /* remove all playlists with named @plname */
	    remove_playlist_by_name (pl_name);
	    /* check if we deleted the selected playlist */
	    if (sel_pl && !playlist_exists (sel_pl))   select = TRUE;
	}

	new_pl = add_new_playlist (pl_name, -1, FALSE);
	for (l=tracks; l; l=l->next)
	{
	    Track *track = (Track *)l->data;
	    add_track_to_playlist (new_pl, track, TRUE);
	}
	str = g_strdup_printf (
	    ngettext ("Created playlist '%s' with %d track.",
		      "Created playlist '%s' with %d tracks.",
		      n), pl_name, n);
	if (new_pl && select)
	{   /* need to select newly created playlist because the old
	     * selection was deleted */
	    pm_select_playlist (new_pl);
	}
    }
    else
    {   /* n==0 */
	str = g_strdup_printf (_("No tracks available, playlist not created"));
    }
    gtkpod_statusbar_message (str);
    gtkpod_tracks_statusbar_update();
    g_free (str);
    return new_pl;
}

/* Generate a playlist named "New Playlist" consisting of the tracks
 * in @tracks. */
Playlist *generate_new_playlist (GList *tracks)
{
    gchar *name = get_user_string (
	_("New Playlist"),
	_("Please enter a name for the new playlist"),
	_("New Playlist"),
	NULL, NULL);
    if (name)
	return generate_playlist_with_name (tracks, name, FALSE);
    return NULL;
}

/* look at the add_ranked_playlist help:
 * BEWARE this function shouldn't be used by other functions */
static GList *create_ranked_glist(gint tracks_nr,PL_InsertFunc insertfunc,
				  GCompareFunc comparefunc,
				  gpointer userdata)
{
   GList *tracks=NULL;
   gint f=0;
   gint i=0;
   Track *track=NULL;

   while ((track=get_next_track(i)))
   {
      i=1; /* for get_next_track() */
      if (track && (!insertfunc || insertfunc (track,userdata)))
      {
	 tracks = g_list_insert_sorted (tracks, track, comparefunc);
	 ++f;
	 if (tracks_nr && (f>tracks_nr))
	 {   /*cut the tail*/
	    tracks = g_list_remove(tracks,
		   g_list_nth_data(tracks, tracks_nr));
	    --f;
	 }
      }
   }
   return tracks;
}
/* Generate or update a playlist named @pl_name, containing
 * @tracks_nr tracks.
 *
 * @str is the playlist's name (no [ or ])
 * @insertfunc: determines which tracks to enter into the new playlist.
 *              If @insertfunc is NULL, all tracks are added.
 * @comparefunc: determines order of tracks
 * @tracks_nr: max. number of tracks in playlist or 0 for no limit.
 *
 * Return value: the newly created playlist
 */
static Playlist *update_ranked_playlist(gchar *str, gint tracks_nr,
				     PL_InsertFunc insertfunc,
				     GCompareFunc comparefunc,
				     gpointer userdata)
{
    Playlist *result = NULL;
    gchar *pl_name = g_strdup_printf ("[%s]", str);
    GList *tracks = create_ranked_glist(tracks_nr,insertfunc,comparefunc,userdata);
    gint f;
    f=g_list_length(tracks);

    if (f != 0)
    /* else generate_playlist_with_name prints something*/
    {
	result = generate_playlist_with_name (tracks, pl_name, TRUE);
    }
    g_list_free (tracks);
    g_free (pl_name);
    return result;
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the most listened (playcount
 * reverse order) tracks. to enter this playlist a track must have been
 * played */

/* Sort Function: determines the order of the generated playlist */

/* NOTE: THE USE OF 'COMP' ARE NECESSARY FOR THE TIME_PLAYED COMPARES
   WHERE A SIGN OVERFLOW MAY OCCUR BECAUSE OF THE 32 BIT UNSIGNED MAC
   TIMESTAMPS. */
static gint Most_Listened_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->rating, a->rating);
	if (result == 0) result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Most_Listened_IF (Track *track, gpointer userdata)
{
    if (track)   return (track->playcount != 0);
    return      FALSE;
}

void most_listened_pl(void)
{
    gint tracks_nr = prefs_get_misc_track_nr();
    gchar *str = g_strdup_printf (_("Most Listened (%d)"), tracks_nr);
    update_ranked_playlist (str, tracks_nr,
			    Most_Listened_IF, Most_Listened_CF, (gpointer)0 );
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing all songs never listened to. */

/* Sort Function: determines the order of the generated playlist */

/* NOTE: THE USE OF 'COMP' ARE NECESSARY FOR THE TIME_PLAYED COMPARES
   WHERE A SIGN OVERFLOW MAY OCCUR BECAUSE OF THE 32 BIT UNSIGNED MAC
   TIMESTAMPS. */
static gint Never_Listened_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->rating, a->rating);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Never_Listened_IF (Track *track, gpointer userdata)
{
    if (track)   return (track->playcount == 0);
    return      FALSE;
}

void never_listened_pl(void)
{
    gint tracks_nr = 0; /* no limit */
    gchar *str = g_strdup_printf (_("Never Listened"));
    update_ranked_playlist (str, tracks_nr,
			    Never_Listened_IF, Never_Listened_CF, (gpointer)0 );
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the most rated (rate
 * reverse order) tracks. */

/* Sort Function: determines the order of the generated playlist */
static gint Most_Rated_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->rating, a->rating);
	if (result == 0) result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->time_played, a->time_played);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Most_Rated_IF (Track *track, gpointer userdata)
{
    if (track) return ((track->playcount != 0) || prefs_get_not_played_track());
    return FALSE;
}

void most_rated_pl(void)
{
    gint tracks_nr = prefs_get_misc_track_nr();
    gchar *str =  g_strdup_printf (_("Best Rated (%d)"), tracks_nr);
    update_ranked_playlist (str, tracks_nr,
			    Most_Rated_IF, Most_Rated_CF, (gpointer)0 );
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate 6 playlists,one for each rating 1..5 and unrated    */


/* Sort Function: determines the order of the generated playlist */
static gint All_Ratings_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->time_played, a->time_played);
    }
    return result;
}


/* Insert function: determines whether a track is entered into the playlist */
static gboolean All_Ratings_IF (Track *track, gpointer user_data)
{
    gint playlist_nr = (gint)user_data;
    if (track) return (track->rating == playlist_nr*20);
    return FALSE;
}


void each_rating_pl(void)
{
	gchar *str = _("Unrated tracks");
	gint playlist_nr;
	for (playlist_nr = 0; playlist_nr < 6; playlist_nr ++ ) {
		if (playlist_nr > 0) 
		{
			str = g_strdup_printf (_("Rated %d"), playlist_nr);
		} 
    	update_ranked_playlist (str, 0,All_Ratings_IF, All_Ratings_CF, (gpointer)playlist_nr);
	}
  	g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the last listened (last time play
 * reverse order) tracks. */

/* Sort Function: determines the order of the generated playlist */
static gint Last_Listened_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->time_played, a->time_played);
	if (result == 0) result = COMP (b->rating, a->rating);
	if (result == 0) result = COMP (b->playcount, a->playcount);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean Last_Listened_IF (Track *track, gpointer userdata)
{
    if (track)   return (track->playcount != 0);
    return      FALSE;
}

void last_listened_pl(void)
{
    gint tracks_nr = prefs_get_misc_track_nr();
    gchar *str = g_strdup_printf (_("Recent (%d)"), tracks_nr);
    update_ranked_playlist (str, tracks_nr,
			    Last_Listened_IF, Last_Listened_CF, (gpointer)0);
    g_free (str);
}


/* ------------------------------------------------------------ */
/* Generate a new playlist containing the tracks listened to since the
 * last time the iPod was connected to a computer (and the playcount
 * file got wiped) */

/* Sort Function: determines the order of the generated playlist */
static gint since_last_CF (gconstpointer aa, gconstpointer bb)
{
    gint result = 0;
    const Track *a = aa;
    const Track *b = bb;

    if (a && b)
    {
	result = COMP (b->recent_playcount, a->recent_playcount);
	if (result == 0) result = COMP (b->time_played, a->time_played);
	if (result == 0) result = COMP (b->playcount, a->playcount);
	if (result == 0) result = COMP (b->rating, a->rating);
    }
    return result;
}

/* Insert function: determines whether a track is entered into the playlist */
static gboolean since_last_IF (Track *track, gpointer userdata)
{
    if (track && (track->recent_playcount != 0))  return TRUE;
    else                                        return FALSE;
}

void since_last_pl(void)
{
    update_ranked_playlist (_("Last Time"), 0,
			    since_last_IF, since_last_CF, (gpointer)0);
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                Find Orphans                                      *
 *                                                                  *
\*------------------------------------------------------------------*/

/******************************************************************************
 * Attempt to do everything at once:
 *  - find dangling links in iTunesDB
 *  - find orphaned files in mounted directory
 * Will be done by creating first a hash of all known in iTunesDB filenames,
 * and then checking every file on HDD in the hash table. If it is present -
 * remove from hashtable, if not present - it is orphaned. If at the end
 * hashtable still has some elements - they're dangling...
 *
 * TODO: instead of using case-sensitive comparison function it might be better
 *  just to convert all filenames to lowercase before doing any comparisons
 * FIX:
 *  offline... when you import db offline and then switch to online mode you still
 *  have offline db loaded and if it is different from IPOD's - then a lot of crap
 *  can happen... didn't check yet
 ******************************************************************************/

#define IPOD_MUSIC_DIRS 20
#define IPOD_CONTROL_DIR "iPod_Control"


/* compare @str1 and @str2 case-sensitively only */
gint str_cmp (gconstpointer str1, gconstpointer str2, gpointer data)
{
	return compare_string_case_insensitive((gchar *)str1, (gchar *)str2);
	/*return strcmp((gchar *)str1, (gchar *)str2);*/
}

static void treeKeyDestroy(gpointer key) { g_free(key); }
static void treeValueDestroy(gpointer value) { }


/* call back function for traversing what is left from the tree -
 * dangling files - files present in DB but not present physically on iPOD.
 * It adds found tracks to the dandling list so user can see what is missing
 * and then decide on what to do with them */
gboolean remove_dangling (gpointer key, gpointer value, gpointer pl_dangling)
{
/*     printf("Found dangling item pointing file %s\n", ((Track*)value)->ipod_path); */
    Track *track = (Track*)value;
    GList **l_dangling = ((GList **)pl_dangling);
    gchar *filehash = NULL;
    gint lind=                 /* to which list belongs */
	(track->pc_path_locale && *track->pc_path_locale &&   /* file is specified */
	 g_file_test (track->pc_path_locale, G_FILE_TEST_EXISTS) && /* file exists */
	 track->md5_hash &&                           /* md5 defined for the track */
	 (!strcmp ((filehash=md5_hash_on_filename (
			track->pc_path_locale, FALSE)), track->md5_hash)));
    /* and md5 of the file is the same as in the track info */
    /* 1 - Original file is present on PC and has the same md5*/
    /* 0 - Doesn't exist */
    l_dangling[lind]=g_list_append(l_dangling[lind], track);
/*    printf( "Dangling %s %s-%s(%d)\n%s\n",_("Track"),
      track->artist, track->title, track->ipod_id, track->pc_path_locale);*/

    g_free(filehash);
    return FALSE;               /* do not stop traversal */
}

guint ntokens(gchar** tokens)
{
    guint n=0;
    while (tokens[n]) n++;
    return n;
}


void process_gtk_events_blocked()
{    while (widgets_blocked && gtk_events_pending ()) gtk_main_iteration ();  }



/* Frees memory busy by the lists containing tracks stored in @user_data1
 * Frees @user_data1 and @user_data2*/
static void
check_db_danglingcancel  (gpointer user_data1, gpointer user_data2)
{
    gint *i=((gint*)user_data2);
    g_list_free((GList *)user_data1);
    if (*i==0)
	gtkpod_statusbar_message (_("Removal of dangling tracks with no files on PC was canceled."));
    else
	gtkpod_statusbar_message (_("Handling of dangling tracks with files on PC was canceled."));
    g_free(i);
}


/* To be called for ok to remove dangling Tracks with with no files linked.
 * Frees @user_data1 and @user_data2*/
static void
check_db_danglingok (gpointer user_data1, gpointer user_data2)
{
    GList *tlist = ((GList *)user_data1);
    GList *l_dangling = tlist;
    gint *i=((gint*)user_data2);
    /* traverse the list and append to the str */
    for (tlist = g_list_first(tlist);
	 tlist != NULL;
	 tlist = g_list_next(tlist))
    {
	Track *track = (Track*)(tlist->data);
	if (*i==0)
	{
/*            printf("Removing track %d\n", track->ipod_id); */
	    remove_track_from_playlist(NULL, track); /* remove track from everywhere */
	    remove_track(track); /* seems to be fine fnct for this case */
	    unmark_track_for_deletion (track); /* otherwise it will try to remove non-existing ipod file */
	}
	else
	{
/*            printf("Handling track %d\n", track->ipod_id); */
	    track->transferred=FALSE; /* yes - we need to transfer it */
/*            g_free(track->ipod_path);      */ /* need to reset ipod's path so it doesn't try to locate it there */
/*            track->ipod_path=NULL;         */ /* zero it out */
/*            update_track_from_file(track); */ /* please update information from the file */
	}
    }
    g_list_free(l_dangling);
    data_changed();
    if (*i==0)
	gtkpod_statusbar_message (_("Dangling tracks with no files on PC were removed."));
    else
	gtkpod_statusbar_message (_("Dangling tracks with files on PC were handled."));
    g_free(i);
}



/* checks iTunesDB for presence of dangling links and checks IPODs Music directory
 * on subject of orphaned files */
void check_db (void)
{

    void glist_list_tracks (GList * tlist, GString * str)
	{
	    Track *track = NULL;

	    if (str==NULL)
	    {
		fprintf(stderr, "Report the bug please: shouldn't be NULL at %s:%d\n",__FILE__,__LINE__);
		return;
	    }
	    /* traverse the list and append to the str */
	    for (tlist = g_list_first(tlist);
		 tlist != NULL;
		 tlist = g_list_next(tlist))
	    {
		track = (Track*)(tlist->data);
		g_string_append_printf
		    (str,"%s(%d) %s-%s -> %s\n",_("Track"),
		     track->ipod_id, track->artist,  track->title,  track->pc_path_utf8);
	    }
	} /* end of glist_list_tracks */

    GTree *files_known = NULL;
    Track *track = NULL;
    GDir  *dir_des = NULL;

    gchar *pathtrack=NULL;
    gchar *ipod_filename = NULL;
    gchar *ipod_dir = NULL;
    gchar *ipod_fulldir = NULL;
    gchar *buf = NULL;
#   define localdebug  0      /* may be later becomes more general verbose param */
    Playlist* pl_orphaned = NULL;
    GList * l_dangling[2] = {NULL, NULL}; /* 2 kinds of dangling tracks: with approp
					   * files and without */
    /* 1 - Original file is present on PC and has the same md5*/
    /* 0 - Doesn't exist */

    gpointer foundtrack ;

    gint  h = 0
	, i
	, norphaned = 0
	, ndangling = 0;

    gchar ** tokens;
    gchar *ipod_path_as_filename = charset_from_utf8 (prefs_get_ipod_mount ());
    const gchar * music[] = {"iPod_Control","Music",NULL,NULL,};

    /* If an iTunesDB exists on the iPod, the user probably is making
       a mistake and we should tell him about it */
    if (!file_itunesdb_read())
    {
	const gchar *itunes_components[] = {"iPod_Control", "iTunes", NULL};
	gchar *itunes_filename = resolve_path(ipod_path_as_filename,
					      itunes_components);
	if (g_file_test (itunes_filename, G_FILE_TEST_EXISTS))
	{
	    GtkWidget *dialog = gtk_message_dialog_new (
		GTK_WINDOW (gtkpod_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_OK_CANCEL,
		_("You did not import the existing iTunesDB. This is most likely incorrect and will result in the loss of the existing database.\n\nPress 'OK' if you want to proceed anyhow or 'Cancel' to abort. If you cancel, you can import the existing database before calling this function again.\n"));
	    gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	    gtk_widget_destroy (dialog);
	    if (result == GTK_RESPONSE_CANCEL)
	    {
		g_free (ipod_path_as_filename);
		g_free (itunes_filename);
		return;
	    }
	}
	g_free (itunes_filename);
    }

    prefs_set_statusbar_timeout (30*STATUSBAR_TIMEOUT);
    block_widgets();

    gtkpod_statusbar_message(_("Creating a tree of known files"));
    gtkpod_tracks_statusbar_update();

    /* put all files in the hash table */
    files_known = g_tree_new_full (str_cmp, NULL,
				   treeKeyDestroy, treeValueDestroy);
    while((track=get_next_track(h)))
    {
        gint ntok=0;
	h=1;
        /* we don't want to report non-transfered files as dandgling */
	if (!track->transferred) continue; 
	tokens = g_strsplit(track->ipod_path,":",(track->ipod_path[0]==':'?4:3));
        ntok=ntokens(tokens);
	if (ntok>=3)
	    pathtrack=g_strdup (tokens[ntok-1]);
	else
	    fprintf(stderr, "Report the bug please: shouldn't be 0 at %s:%d\n",__FILE__,__LINE__);
        if (localdebug)
            fprintf(stdout,"File %s\n", pathtrack);

	fflush(stdout);

	g_tree_insert (files_known, pathtrack, track);
	g_strfreev(tokens);
    }

    gtkpod_statusbar_message(_("Checking iPOD files against known files in DB"));
    gtkpod_tracks_statusbar_update();
    process_gtk_events_blocked();

    for(h=0;h<IPOD_MUSIC_DIRS;h++)
    {
	/* directory name */
	ipod_dir=g_strdup_printf("F%02d",h); /* just directory name */
                    music[2] = ipod_dir;
	ipod_fulldir=resolve_path(ipod_path_as_filename,music); /* full path */

	if(ipod_fulldir && (dir_des=g_dir_open(ipod_fulldir,0,NULL))) {
	    while ((ipod_filename=g_strdup(g_dir_read_name(dir_des))))
		/* we have a file in the directory*/
	    {
		pathtrack=g_strdup_printf("%s%c%s", ipod_dir, ':', ipod_filename);

                if (localdebug) {
                    fprintf(stdout,"Considering %s ", pathtrack);
                    fflush(stdout);
                }

		if ( g_tree_lookup_extended (files_known, pathtrack,
					     &foundtrack, &foundtrack) )
		{ /* file is not orphaned */
		    g_tree_remove(files_known, pathtrack); /* we don't need this any more */
                    if (localdebug) fprintf(stdout," good ");
		}
		else
		{  /* Now deal with orphaned... */
		    gchar *fn_orphaned;
		    gchar *num_str = g_strdup_printf ("F%02d", h);
		    Track *dupl_track;

		    const gchar *dcomps[] =
			{ "iPod_Control", "Music", num_str,
			  ipod_filename, NULL };

		    fn_orphaned = resolve_path (
			prefs_get_ipod_mount(), dcomps);

		    if (!pl_orphaned)
		    {
			gchar *str = g_strdup_printf ("[%s]", _("Orphaned"));
			pl_orphaned = get_newplaylist_by_name(str, FALSE);
			g_free (str);
		    }

		    norphaned++;

                    if (localdebug) fprintf(stdout,"to orphaned ");
		    if ((dupl_track = md5_file_exists (fn_orphaned, TRUE)))
		    {  /* This orphan has already been added again.
			  It will be removed with the next sync */
			Track *track = itunesdb_new_track ();
			gchar *fn_utf8 = charset_to_utf8 (fn_orphaned);
			track->ipod_path = g_strdup_printf (
			    ":iPod_Control:Music:%s:%s",
			    num_str, ipod_filename);
			validate_entries (track);
			mark_track_for_deletion (track);
			gtkpod_warning (_(
			 "The following orphaned file had already "
			 "been added to the iPod again. It will be "
			 "removed with the next sync:\n%s\n\n"),
			 fn_utf8);
			g_free (fn_utf8);
		    }
		    else
		    {
			add_track_by_filename(fn_orphaned, pl_orphaned,
					      FALSE, NULL, NULL);
		    }
		    g_free (fn_orphaned);
		    g_free (num_str);
		}
                if (localdebug) fprintf(stdout," done\n");

		g_free(ipod_filename);
		g_free(pathtrack);
	    }
            g_dir_close(dir_des);
	}
        music[3] = NULL;
        g_free(ipod_dir);
 	g_free(ipod_fulldir);
	process_gtk_events_blocked();
    }

    ndangling=g_tree_nnodes(files_known);
    buf=g_strdup_printf(_("Found %d orphaned and %d dangling files. Processing..."),
			norphaned, ndangling);

    gtkpod_statusbar_message(buf);
    gtkpod_tracks_statusbar_update();

    g_free(buf);

    /* Now lets deal with dangling tracks */
    /* Traverse the tree - leftovers are dangling - put them in two lists */
    g_tree_foreach(files_known, remove_dangling, l_dangling);

    for (i=0;i<2;i++)
    {
	GString * str_dangs = g_string_sized_new(2000);
	gint ndang=0;
	gint *k = g_malloc(sizeof(gint));
	*k=i; /* to pass inside the _confirmation */

	glist_list_tracks(l_dangling[i], str_dangs); /* compose String list of the tracks */
	ndang = g_list_length(l_dangling[i]);
	if (ndang)
	{
	    if (i)
		buf = g_strdup_printf (
		    ngettext ("The following dangling track has a file on PC.\nPress OK to have them transfered from the file on next Sync, CANCEL to leave it as is.",
			      "The following %d dangling tracks have files on PC.\nPress OK to have them transfered from the files on next Sync, CANCEL to leave them as is.",
			      ndang), ndang);
	    else
		buf = g_strdup_printf (
		    ngettext ("The following dangling track doesn't have file on PC. \nPress OK to remove it, CANCEL to leave it as is.",
			      "The following %d dangling tracks do not have files on PC. \nPress OK to remove them, CANCEL to leave them. as is",
			      ndang), ndang);

	    if (gtkpod_confirmation
		((i?CONF_ID_DANGLING1:CONF_ID_DANGLING0), /* we want unique window for each */
		 FALSE,         /* gboolean modal, */
		 _("Dangling Tracks"), /* title */
		 buf,           /* label */
		 str_dangs->str, /* scrolled text */
		 NULL, 0, NULL, /* option 1 */
		 NULL, 0, NULL, /* option 2 */
		 TRUE,          /* gboolean confirm_again, */
		 NULL,          /* ConfHandlerOpt confirm_again_handler,*/
		 check_db_danglingok, /* ConfHandler ok_handler,*/
		 NULL,          /* don't show "Apply" button */
		 check_db_danglingcancel, /* cancel_handler,*/
		 l_dangling[i], /* gpointer user_data1,*/
		 k)             /* gpointer user_data2,*/
		== GTK_RESPONSE_REJECT)
	    {   /* free memory */
		g_list_free(l_dangling[i]);
		g_free(k);
	    }
	    g_free (buf);
	    g_string_free (str_dangs, TRUE);
	}
    }

    if (pl_orphaned) data_changed();
    g_free(ipod_path_as_filename);
    g_tree_destroy(files_known);
    buf=g_strdup_printf(_("Found %d orphaned and %d dangling files. Done."),
			norphaned, ndangling);
    gtkpod_statusbar_message(buf);
    g_free (buf);
    prefs_set_statusbar_timeout (0);
    release_widgets();
}
