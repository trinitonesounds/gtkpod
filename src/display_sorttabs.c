/* Time-stamp: <2005-07-18 00:52:32 jcs>
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

#include <gdk/gdkkeysyms.h>

#include "display_private.h"
#include "info.h"
#include "prefs.h"
#include "misc.h"
#include "misc_track.h"
#include "context_menus.h"
#include "date_parser.h"
#include "itdb.h"
#include <string.h>
#include <stdlib.h>

/* array with pointers to the sort tabs */
static SortTab *sorttab[SORT_TAB_MAX];
/* pointer to paned elements holding the sort tabs */
static GtkPaned *st_paned[PANED_NUM_ST];
/* compare function to be used for string comparisons */

static void sp_store_sp_entries (gint inst);

/* Drag and drop definitions */
static GtkTargetEntry st_drag_types [] = {
    { DND_GTKPOD_TRACKLIST_TYPE, 0, DND_GTKPOD_TRACKLIST },
    { "text/uri-list", 0, DND_TEXT_URI_LIST },
    { "text/plain", 0, DND_TEXT_PLAIN },
    { "STRING", 0, DND_TEXT_PLAIN }
};

typedef enum {
    IS_INSIDE,  /* track's timestamp is inside the specified interval  */
    IS_OUTSIDE, /* track's timestamp is outside the specified interval */
    IS_ERROR,   /* error parsing date string (or wrong parameters)    */
} IntervalState;

GladeXML *cal_xml;



/* ---------------------------------------------------------------- */
/*                                                                  */
/* Section for filter tab display (callback)                        */
/*                                                                  */
/* ---------------------------------------------------------------- */

/*
 * utility function for appending ipod track for st treeview callback
 */
static void
on_st_dnd_get_track_foreach(GtkTreeModel *tm, GtkTreePath *tp,
			    GtkTreeIter *i, gpointer data)
{
    GList *gl;
    TabEntry *entry=NULL;
    GString *tracklist = (GString *)data;

    g_return_if_fail (tracklist);

    gtk_tree_model_get (tm, i, ST_COLUMN_ENTRY, &entry, -1);
    g_return_if_fail (entry);


    /* add all member tracks of entry to tracklist */
    for (gl=entry->members; gl; gl=gl->next)
    {
	Track *tr = gl->data;
	g_return_if_fail (tr);
	g_string_append_printf (tracklist, "%p\n", tr);
    }
}


/*
 * utility function for appending filenames for st treeview callback
 */
static void
on_st_dnd_get_file_foreach(GtkTreeModel *tm, GtkTreePath *tp,
			   GtkTreeIter *iter, gpointer data)
{
    GList *gl;
    TabEntry *entry=NULL;
    GString *filelist = data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    gtk_tree_model_get (tm, iter, ST_COLUMN_ENTRY, &entry, -1);
    g_return_if_fail (entry);


    /* add all member tracks of entry to tracklist */
    for (gl=entry->members; gl; gl=gl->next)
    {
	gchar *name;
	Track *tr = gl->data;

	g_return_if_fail (tr);
	name = get_file_name_verified (tr);
	if (name)
	{
	    g_string_append_printf (filelist, "file:%s\n", name);
	    g_free (name);
	}
    }
}


static void st_drag_end (GtkWidget *widget,
			 GdkDragContext *dc,
			 gpointer user_data)
{
/*     puts ("st_drag_end"); */
    gtkpod_tracks_statusbar_update ();
}



/*
 * utility function for appending uris for st treeview callback
 */
static void
on_st_dnd_get_uri_foreach(GtkTreeModel *tm, GtkTreePath *tp,
			  GtkTreeIter *iter, gpointer data)
{
    GList *gl;
    TabEntry *entry=NULL;
    GString *filelist = data;

    g_return_if_fail (tm);
    g_return_if_fail (iter);
    g_return_if_fail (data);

    gtk_tree_model_get (tm, iter, ST_COLUMN_ENTRY, &entry, -1);
    g_return_if_fail (entry);


    /* add all member tracks of entry to tracklist */
    for (gl=entry->members; gl; gl=gl->next)
    {
	gchar *name;
	Track *tr = gl->data;

	g_return_if_fail (tr);
	name = get_file_name_verified (tr);
	if (name)
	{
	    gchar *uri = g_filename_to_uri (name, NULL, NULL);
	    if (uri)
	    {
		g_string_append_printf (filelist, "file:%s\n", name);
		g_free (uri);
	    }
	    g_free (name);
	}
    }
}


static void
st_drag_data_get (GtkWidget       *widget,
		  GdkDragContext  *context,
		  GtkSelectionData *data,
		  guint            info,
		  guint            time,
		  gpointer         user_data)
{
    GtkTreeSelection *ts = NULL;
    GString *reply = g_string_sized_new (2000);

    if (!data) return;
    
    ts = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
    if (ts)
    {
	switch (info)
	{
	case DND_GTKPOD_TRACKLIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_st_dnd_get_track_foreach, reply);
	    break;
	case DND_TEXT_URI_LIST:
	    gtk_tree_selection_selected_foreach(ts,
				    on_st_dnd_get_uri_foreach, reply);
	    break;
	case DND_TEXT_PLAIN:
	    gtk_tree_selection_selected_foreach(ts,
				    on_st_dnd_get_file_foreach, reply);
	    break;
	default:
	    g_warning ("Programming error: st_drag_data_get received unknown info type (%d)\n", info);
	    break;
	}
    }
    gtk_selection_data_set(data, data->target, 8, reply->str, reply->len);
    g_string_free (reply, TRUE);
}


/* delete selected entry in sort tab */
static gboolean
on_st_treeview_key_release_event       (GtkWidget       *widget,
					GdkEventKey     *event,
					gpointer         user_data)
{
    guint mods;
    mods = event->state;

    if(!widgets_blocked && (mods & GDK_CONTROL_MASK))
    {
	switch(event->keyval)
	{
/* 	    case GDK_u: */
/* 		gp_do_selected_entry (update_tracks, */
/* 				   st_get_instance_from_treeview ( */
/* 				       GTK_TREE_VIEW (widget))); */
/* 		break; */
	    default:
		break;
	}

    }
  return FALSE;
}


/* callback */
static void
on_st_switch_page                 (GtkNotebook     *notebook,
				   GtkNotebookPage *page,
				   guint            page_num,
				   gpointer         user_data)
{
    gchar *buf;
    gint inst = (gint)user_data;

/*     printf ("switch_page: inst/page: %d/%d\n", inst, page_num); */
    /* set compare function for strings (to speed up sorting) */
    buf = g_strdup_printf ("sort_ign_field_%d", ST_to_T (page_num));
    if (prefs_get_int (buf))
	sorttab[inst]->string_compare_func = compare_string_fuzzy;
    else
	sorttab[inst]->string_compare_func = compare_string;
    g_free (buf);
    space_data_update ();
    st_page_selected (notebook, page_num);
}



/* ---------------------------------------------------------------- */
/*                                                                  */
/* Section for filter tab display (special sort tab)                */
/*                                                                  */
/* ---------------------------------------------------------------- */


/* Remove all members of special sort tab (ST_CAT_SPECIAL) in instance
 * @inst */
static void sp_remove_all_members (guint32 inst)
{
    SortTab *st;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];

    if (!st)  return;

    g_list_free (st->sp_members);
    st->sp_members = NULL;
    g_list_free (st->sp_selected);
    st->sp_selected = NULL;
}


/* Return a pointer to ti_added, ti_modified or ti_played. Returns
   NULL if either inst or item are out of range */
static TimeInfo *sp_get_timeinfo_ptr (guint32 inst, T_item item)
{
    if (inst >= SORT_TAB_MAX)
    {
	fprintf (stderr, "Programming error: st_get_timeinfo_ptr: inst out of range: %d\n", inst);
    }
    else
    {
	SortTab *st = sorttab[inst];
	switch (item)
	{
	case T_TIME_ADDED:
	    return &st->ti_added;
	case T_TIME_PLAYED:
	    return &st->ti_played;
	case T_TIME_MODIFIED:
	    return &st->ti_modified;
	default:
	    fprintf (stderr, "Programming error: st_get_timeinfo_ptr: item invalid: %d\n", item);
	}
    }
    return NULL;
}

/* Update the date interval from the string provided by
   prefs_get_sp_entry() */
/* @inst: instance
   @item: T_TIME_PLAYED, or T_TIME_MODIFIED,
   @force_update: usually the update is only performed if the string
   has changed. TRUE will re-evaluate the string (and print an error
   message again, if necessary */
/* Return value: pointer to the corresponding TimeInfo struct (for
   convenience) or NULL if error occured */
TimeInfo *sp_update_date_interval_from_string (guint32 inst,
					       T_item item,
					       gboolean force_update)
{
    SortTab *st;
    TimeInfo *ti;

    if (inst >= SORT_TAB_MAX) return NULL;

    st = sorttab[inst];
    ti = sp_get_timeinfo_ptr (inst, item);

    if (ti)
    {
	gchar *new_string = prefs_get_sp_entry (inst, item);

	if (force_update || !ti->int_str ||
	    (strcmp (new_string, ti->int_str) != 0))
	{   /* Re-evaluate the interval */
	    g_free (ti->int_str);
	    ti->int_str = g_strdup (new_string);
	    dp2_parse (ti);
	}
    }
    return ti;
}


/* check if @track's timestamp is within the interval given for @item.
 *
 * Return value:
 *
 * IS_ERROR:   error parsing date string (or wrong parameters)
 * IS_INSIDE:  track's timestamp is inside the specified interval
 * IS_OUTSIDE: track's timestamp is outside the specified interval
 */
static IntervalState sp_check_time (guint32 inst, T_item item, Track *track)
{
    TimeInfo *ti;
    IntervalState result = IS_ERROR;

    ti = sp_update_date_interval_from_string (inst, item, FALSE);
    if (ti && ti->valid)
    {
	guint32 stamp = track_get_timestamp (track, item);
	if (stamp && (ti->lower <= stamp) && (stamp <= ti->upper))
	      result = IS_INSIDE;
	else  result = IS_OUTSIDE;
    }
    if (result == IS_ERROR)
    {
	switch (item)
	{
	case T_TIME_PLAYED:
	    gtkpod_statusbar_message (_("'Played' condition ignored because of error."));
	    break;
	case T_TIME_MODIFIED:
	    gtkpod_statusbar_message (_("'Modified' condition ignored because of error."));
	    break;
	case T_TIME_ADDED:
	    gtkpod_statusbar_message (_("'Added' condition ignored because of error."));
	    break;
	default:
	    break;
	}
    }
    return result;
}


/* decide whether or not @track satisfies the conditions specified in
 * the special sort tab of instance @inst.
 * Return value:  TRUE: satisfies, FALSE: does not satisfy */
static gboolean sp_check_track (Track *track, guint32 inst)
{
    gboolean sp_or = prefs_get_sp_or (inst);
    gboolean result, cond, checked=FALSE;

    if (!track) return FALSE;

    /* Initial state depends on logical operation */
    if (sp_or) result = FALSE;  /* OR  */
    else       result = TRUE;   /* AND */

    /* RATING */
    if (prefs_get_sp_cond (inst, T_RATING))
    {
	/* checked = TRUE: at least one condition was checked */
	checked = TRUE;
	cond = prefs_get_sp_rating_n (inst, track->rating/ITDB_RATING_STEP);
	/* If one of the two combinations occur, we can take a
	   shortcut and stop checking the other conditions */
	if (sp_or && cond)       return TRUE;
	if ((!sp_or) && (!cond)) return FALSE;
	/* We don't have to calculate a new 'result' value because for
	   the other two combinations it does not change */
    }

    /* PLAYCOUNT */
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
    {
	guint32 low = prefs_get_sp_playcount_low (inst);
	/* "-1" will translate into about 4 billion because I use
	   guint32 instead of gint32. Since 4 billion means "no upper
	   limit" the logic works fine */
	guint32 high = prefs_get_sp_playcount_high (inst);
	checked = TRUE;
	if ((low <= track->playcount) && (track->playcount <= high))
	    cond = TRUE;
	else
	    cond = FALSE;
	if (sp_or && cond)       return TRUE;
	if ((!sp_or) && (!cond)) return FALSE;
    }
    /* time played */
    if (prefs_get_sp_cond (inst, T_TIME_PLAYED))
    {
	IntervalState result = sp_check_time (inst, T_TIME_PLAYED, track);
	if (sp_or && (result == IS_INSIDE))      return TRUE;
	if ((!sp_or) && (result == IS_OUTSIDE))  return FALSE;
	if (result != IS_ERROR)                  checked = TRUE;
    }
    /* time modified */
    if (prefs_get_sp_cond (inst, T_TIME_MODIFIED))
    {
	IntervalState result = sp_check_time (inst, T_TIME_MODIFIED, track);
	if (sp_or && (result == IS_INSIDE))      return TRUE;
	if ((!sp_or) && (result == IS_OUTSIDE))  return FALSE;
	if (result != IS_ERROR)                  checked = TRUE;
    }
    /* time added */
    if (prefs_get_sp_cond (inst, T_TIME_ADDED))
    {
	IntervalState result = sp_check_time (inst, T_TIME_ADDED, track);
	if (sp_or && (result == IS_INSIDE))      return TRUE;
	if ((!sp_or) && (result == IS_OUTSIDE))  return FALSE;
	if (result != IS_ERROR)                  checked = TRUE;
    }

    if (checked) return result;
    else         return FALSE;
}


/* called by st_add_track(): add a track to ST_CAT_SPECIAL */
static void st_add_track_special (Track *track, gboolean final,
				 gboolean display, guint32 inst)
{
    SortTab *st;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];

    /* Sanity */
    if (!st)  return;

    /* Sanity */
    if (st->current_category != ST_CAT_SPECIAL) return;

    st->final = final;

    if (track != NULL)
    {
	/* Add track to member list */
	st->sp_members = g_list_append (st->sp_members, track);
	/* Check if track is to be passed on to next sort tab */
	if (st->is_go || prefs_get_sp_autodisplay (inst))
	{   /* Check if track matches sort criteria to be displayed */
	    if (sp_check_track (track, inst))
	    {
		st->sp_selected = g_list_append (st->sp_selected, track);
		st_add_track (track, final, display, inst+1);
	    }
	}
    }
    if (!track && final)
    {
	if (st->is_go || prefs_get_sp_autodisplay (inst))
	    st_add_track (NULL, final, display, inst+1);

    }
}


/* Callback for sp_go() */
static void sp_go_cb (gpointer user_data1, gpointer user_data2)
{
    guint32 inst = (guint32)user_data1;
    SortTab *st = sorttab[inst];

#if DEBUG_TIMING
    GTimeVal time;
    g_get_current_time (&time);
    printf ("sp_go_cb enter: %ld.%06ld sec\n",
	    time.tv_sec % 3600, time.tv_usec);
#endif

    space_data_update ();

    /* Sanity */
    if (st == NULL) return;

    /* remember that "Display" was already pressed */
    st->is_go = TRUE;

    /* Clear the sp_selected list */
    g_list_free (st->sp_selected);
    st->sp_selected = NULL;

    /* initialize next instance */
    st_init (-1, inst+1);

    if (st->sp_members)
    {
	GTimeVal time;
	float max_count = REFRESH_INIT_COUNT;
	gint count = max_count - 1;
	float ms;
	GList *gl;

	if (!prefs_get_block_display ())
	{
	    block_selection (inst);
	    g_get_current_time (&time);
	}
	st_enable_disable_view_sort (inst+1, FALSE);
	for (gl=st->sp_members; gl; gl=gl->next)
	{ /* add all member tracks to next instance */
	    Track *track = (Track *)gl->data;
	    if (stop_add <= (gint)inst) break;
	    if (sp_check_track (track, inst))
	    {
		st->sp_selected = g_list_append (st->sp_selected, track);
		st_add_track (track, FALSE, TRUE, inst+1);
	    }
	    --count;
	    if ((count < 0) && !prefs_get_block_display ())
	    {
		gtkpod_tracks_statusbar_update();
		while (gtk_events_pending ())       gtk_main_iteration ();
		ms = get_ms_since (&time, TRUE);
		/* first time takes significantly longer, so we adjust
		   the max_count */
		if (max_count == REFRESH_INIT_COUNT) max_count *= 2.5;
		/* average the new and the old max_count */
		max_count *= (1 + 2 * REFRESH_MS / ms) / 3;
		count = max_count - 1;
#if DEBUG_TIMING
		printf("st_s_c ms: %f mc: %f\n", ms, max_count);
#endif
	    }
	}
	st_enable_disable_view_sort (inst+1, TRUE);
	if (stop_add > (gint)inst)
	    st_add_track (NULL, TRUE, st->final, inst+1);
	if (!prefs_get_block_display ())
	{
	    while (gtk_events_pending ())	  gtk_main_iteration ();
	    release_selection (inst);
	}
    }
    gtkpod_tracks_statusbar_update();
#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("st_selection_changed_cb exit:  %ld.%06ld sec\n",
	    time.tv_sec % 3600, time.tv_usec);
#endif
}

/* save the contents of the entry to prefs */
static void sp_store_sp_entries (gint inst)
{
    SortTab *st;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];

    /* Sanity */
    if (!st || (st->current_category != ST_CAT_SPECIAL)) return;

    prefs_set_sp_entry (inst, T_TIME_PLAYED,
			gtk_entry_get_text (GTK_ENTRY(st->ti_played.entry)));
    prefs_set_sp_entry (inst, T_TIME_MODIFIED,
			gtk_entry_get_text (GTK_ENTRY(st->ti_modified.entry)));
    prefs_set_sp_entry (inst, T_TIME_ADDED,
			gtk_entry_get_text (GTK_ENTRY(st->ti_added.entry)));
}

/* display the members satisfying the conditions specified in the
 * special sort tab of instance @inst */
void sp_go (guint32 inst)
{
    SortTab *st;

#if DEBUG_CB_INIT
    printf ("st_go: inst: %d\n", inst);
#endif

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];

    /* Sanity */
    if (st->current_category != ST_CAT_SPECIAL) return;

    /* check if members are already displayed */
    /* if (st->is_go || prefs_get_sp_autodisplay (inst))  return; */

    /* Make sure the information typed into the entries is actually
     * being used (maybe the user 'forgot' to press enter */
    sp_store_sp_entries (inst);

    /* Instead of handling the selection directly, we add a
       "callback". Currently running display updates will be stopped
       before the sp_go_cb is actually called */
    add_selection_callback (inst, sp_go_cb,
			    (gpointer)inst, NULL);
}


/* called by st_remove_track() */
static void st_remove_track_special (Track *track, guint32 inst)
{
    SortTab *st;
    GList *link;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];

    /* Sanity */
    if (st->current_category != ST_CAT_SPECIAL) return;

    /* Remove track from member list */
    link = g_list_find (st->sp_members, track);
    if (link)
    {   /* only remove track from next sort tab if it was a member of
	   this sort tab (slight performance improvement when being
	   called with non-existing tracks */
	st->sp_members = g_list_delete_link (st->sp_members, link);
	st_remove_track (track, inst+1);
    }
}


/* called by st_track_changed */
static void st_track_changed_special (Track *track,
				     gboolean removed, guint32 inst)
{
    SortTab *st;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];

    /* Sanity */
    if (st->current_category != ST_CAT_SPECIAL) return;

    if (g_list_find (st->sp_members, track))
    {   /* only do anything if @track was a member of this sort tab
	   (slight performance improvement when being called with
	   non-existing tracks */
	if (removed)
	{
	    /* Remove track from member list */
	    st->sp_members = g_list_remove (st->sp_members, track);
	    if (g_list_find (st->sp_selected, track))
	    {   /* only remove from next sort tab if it was passed on */
		st->sp_selected = g_list_remove (st->sp_selected, track);
		st_track_changed (track, TRUE, inst+1);
	    }
	}
	else
	{
	    if (g_list_find (st->sp_selected, track))
	    {   /* track is being passed on to next sort tab */
		if (sp_check_track (track, inst))
		{   /* only changed */
		    st_track_changed (track, FALSE, inst+1);
		}
		else
		{   /* has to be removed */
		    st->sp_selected = g_list_remove (st->sp_selected, track);
		    st_track_changed (track, TRUE, inst+1);
		}
	    }
	    else
	    {   /* track is not being passed on to next sort tab */
		if (sp_check_track (track, inst))
		{   /* add to next sort tab */
		    st->sp_selected = g_list_append (st->sp_selected, track);
		    st_add_track (track, TRUE, TRUE, inst+1);
		}
	    }
	}
    }
}


/* Called when the user changed the sort conditions in the special
 * sort tab */
void sp_conditions_changed (guint32 inst)
{
    SortTab *st;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return;

    st = sorttab[inst];
    /* Sanity */
    if (!st || st->current_category != ST_CAT_SPECIAL) return;

    /* Only redisplay if data is actually being passed on to the next
       sort tab */
    if (st->is_go || prefs_get_sp_autodisplay (inst))
    {
	st_redisplay (inst);
    }
}


/* ---------------------------------------------------------------- */
/*                                                                  */
/* Callbacks for special sort tab display                           */
/*                                                                  */
/* ---------------------------------------------------------------- */

void
on_sp_or_button_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_or (inst, gtk_toggle_button_get_active (togglebutton));
    sp_conditions_changed (inst);
}


void
on_sp_cond_button_toggled            (GtkToggleButton *togglebutton,
				      gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item cond = (guint32)user_data >> SP_SHIFT;

/*     printf ("%d/%d/%d\n",inst,cond,gtk_toggle_button_get_active (togglebutton)); */
    prefs_set_sp_cond (inst, cond,
		       gtk_toggle_button_get_active (togglebutton));
    sp_conditions_changed (inst);
}

void
on_sp_rating_n_toggled                 (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    guint32 n = (guint32)user_data >> SP_SHIFT;

    prefs_set_sp_rating_n (inst, n,
			   gtk_toggle_button_get_active (togglebutton));
    if (prefs_get_sp_cond (inst, T_RATING))
	sp_conditions_changed (inst);
}


void
on_sp_entry_activate             (GtkEditable     *editable,
				  gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item item = (guint32)user_data >> SP_SHIFT;
    gchar *buf = gtk_editable_get_chars(editable,0, -1);

/*    printf ("sp_entry_activate inst: %d, item: %d\n", inst, item);*/

    prefs_set_sp_entry (inst, item, buf);
    g_free (buf);
    sp_update_date_interval_from_string (inst, item, TRUE);
/*     if (prefs_get_sp_autodisplay (inst))  sp_go (inst); */
    sp_go (inst);
}


void
on_sp_cal_button_clicked        (GtkButton       *button,
				 gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    T_item item = (guint32)user_data >> SP_SHIFT;

    cal_open_calendar (inst, item);
}


void
on_sp_go_clicked                       (GtkButton       *button,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    sp_go (inst);
}


void
on_sp_go_always_toggled                (GtkToggleButton *togglebutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;
    gboolean state = gtk_toggle_button_get_active (togglebutton);

    /* display data if autodisplay is turned on */
    if (state)  on_sp_go_clicked (NULL, user_data);
    prefs_set_sp_autodisplay(inst, state);
}

void
on_sp_playcount_low_value_changed      (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_playcount_low (inst,
				gtk_spin_button_get_value (spinbutton));
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
	sp_conditions_changed (inst);
}


void
on_sp_playcount_high_value_changed     (GtkSpinButton   *spinbutton,
					gpointer         user_data)
{
    guint32 inst = (guint32)user_data & SP_MASK;

    prefs_set_sp_playcount_high (inst,
				 gtk_spin_button_get_value (spinbutton));
    if (prefs_get_sp_cond (inst, T_PLAYCOUNT))
	sp_conditions_changed (inst);
}



/* ---------------------------------------------------------------- */
/*                                                                  */
/* Section for sort tab display (normal and general)                */
/*                                                                  */
/* ---------------------------------------------------------------- */

/* return a pointer to the list of members selected in the sort tab
   @inst. For a normal sort tab this is
   sorttab[inst]->current_entry->members, for a special sort tab this
   is sorttab[inst]->sp_selected.
   You must not g_list_free() the returned list */
GList *st_get_selected_members (guint32 inst)
{
    SortTab *st;

    /* Sanity */
    if (inst >= prefs_get_sort_tab_num ())  return NULL;

    st = sorttab[inst];

    /* Sanity */
    if (!st) return NULL;

    if (st->current_category != ST_CAT_SPECIAL)
    {
	if (st->current_entry)    return st->current_entry->members;
	else                      return NULL;
    }
    else
    {
	return st->sp_selected;
    }
}


/* Get the instance of the sort tab that corresponds to
   "notebook". Returns -1 if sort tab could not be found
   and prints error message */
static gint st_get_instance_from_notebook (GtkNotebook *notebook)
{
    gint i;

    for(i=0; i<SORT_TAB_MAX; ++i)
    {
	if (sorttab[i] && (sorttab[i]->notebook == notebook)) return i;
    }
/*  g_warning ("Programming error (st_get_instance_from_notebook): notebook could
    not be found.\n"); function somehow can get called after notebooks got
    destroyed */
    return -1;
}

/* Get the instance of the sort tab that corresponds to
   "treeview". Returns -1 if sort tab could not be found
   and prints error message */
gint st_get_instance_from_treeview (GtkTreeView *tv)
{
    gint i,cat;

    for(i=0; i<SORT_TAB_MAX; ++i)
    {
	for(cat=0; cat<ST_CAT_NUM; ++cat)
	{
	    if (sorttab[i] && (sorttab[i]->treeview[cat] == tv)) return i;
	}
    }
    return -1;
}


/* returns the selected entry */
TabEntry *st_get_selected_entry (gint inst)
{
    TabEntry *result = NULL;

    if ((inst >= 0) && (inst < SORT_TAB_MAX) && sorttab[inst])
/*	return sorttab[inst]->current_entry;*/
/* we can't just return the "->current_entry" because the context
   menus require the selection before "->current_entry" is updated */
    {
	SortTab *st = sorttab[inst];
	GtkTreeView *tv = st->treeview[st->current_category];
	GtkTreeSelection *ts = gtk_tree_view_get_selection (tv);
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (ts, NULL, &iter))
	{
	    gtk_tree_model_get (st->model, &iter,
				ST_COLUMN_ENTRY, &result, -1);
	}
    }
    /* wait until current_entry was updated */
    if (result != sorttab[inst]->current_entry) result=NULL;
    return result;
}


/* Append playlist to the playlist model. */
static void st_add_entry (TabEntry *entry, guint32 inst)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    SortTab *st;

    st = sorttab[inst];
    model = st->model;
    g_return_if_fail (model != NULL);
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			ST_COLUMN_ENTRY, entry, -1);
    st->entries = g_list_append (st->entries, entry);
    if (!entry->master)
    {
	if (!st->entry_hash)
	{
	    st->entry_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}
	g_hash_table_insert (st->entry_hash, entry->name, entry);
    }
}

/* Used by st_remove_entry_from_model() to remove entry from model by calling
   gtk_tree_model_foreach () */
static gboolean st_delete_entry_from_model (GtkTreeModel *model,
					    GtkTreePath *path,
					    GtkTreeIter *iter,
					    gpointer data)
{
  TabEntry *entry;

  gtk_tree_model_get (model, iter, ST_COLUMN_ENTRY, &entry, -1);
  if(entry == (TabEntry *)data)
  {
      gtk_list_store_remove (GTK_LIST_STORE (model), iter);
      return TRUE;
  }
  return FALSE;
}


/* Remove entry from the display model and the sorttab */
static void st_remove_entry_from_model (TabEntry *entry, guint32 inst)
{
    SortTab *st = sorttab[inst];
    GtkTreeModel *model = st->model;
    if (model && entry)
    {
/* 	printf ("entry: %p, cur_entry: %p\n", entry, st->current_entry); */
	if (entry == st->current_entry)
	{
	    GtkTreeSelection *selection = gtk_tree_view_get_selection
		(st->treeview[st->current_category]);
	    st->current_entry = NULL;
	    /* We have to unselect the previous selection */
	    gtk_tree_selection_unselect_all (selection);
	}
	gtk_tree_model_foreach (model, st_delete_entry_from_model, entry);
	st->entries = g_list_remove (st->entries, entry);
	g_list_free (entry->members);
	/* remove entry from hash */
	if (st->entry_hash)
	{
	    TabEntry *hashed_entry =
		(TabEntry *)g_hash_table_lookup (st->entry_hash, entry->name);
	    if (hashed_entry == entry)
		g_hash_table_remove (st->entry_hash, entry->name);
	}
	C_FREE (entry->name);
	g_free (entry);
    }
}

/* Remove all entries from the display model and the sorttab */
/* @clear_sort: reset sorted columns to the non-sorted state */
void st_remove_all_entries_from_model (guint32 inst)
{
  TabEntry *entry;
  SortTab *st = sorttab[inst];
  gint column;
  GtkSortType order;

  if (st)
  {
      if (st->current_entry)
      {
	  GtkTreeSelection *selection = gtk_tree_view_get_selection
	      (st->treeview[st->current_category]);
	  st->current_entry = NULL;
	  /* We may have to unselect the previous selection */
	  gtk_tree_selection_unselect_all (selection);
      }
      while (st->entries != NULL)
      {
	  entry = (TabEntry *)g_list_nth_data (st->entries, 0);
	  st_remove_entry_from_model (entry, inst);
      }
      if (st->entry_hash)  g_hash_table_destroy (st->entry_hash);
      st->entry_hash = NULL;

      if((prefs_get_st_sort () == SORT_NONE) &&
	 gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (st->model),
					       &column, &order))
      { /* recreate track treeview to unset sorted column */
	  if (column >= 0)
	  {
	      st_create_notebook (inst);
	  }
      }
  }
}

/* Remove "entry" from the model (used by delete_entry_ok()). The
 * entry should be empty (otherwise it's not removed).
 * If "entry" is the master entry 'All', the sort tab is redisplayed
 * (it's empty).
 * If the entry is currently selected (usually will be), the next
 * or previous entry will be selected automatically (unless it's the
 * master entry and prefs_get_st_autoselect() says don't select the 'All'
 * entry). If no new entry is selected, the next sort tab will be
 * redisplayed (should be empty) */
void st_remove_entry (TabEntry *entry, guint32 inst)
{
    TabEntry *next=NULL;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    SortTab *st = sorttab[inst];

    if (!entry) return;
    /* is the entry empty (contains no tracks)? */
    if (g_list_length (entry->members) != 0) return;
    /* if the entry is the master entry 'All' -> the tab is empty,
       re-init tab */
    if (entry->master)
    {
	st_init (-1, inst);
	return;
    }

    /* is the entry currently selected? Remember! */
    selection = gtk_tree_view_get_selection (st->treeview[st->current_category]);
#if 0  /* it doesn't make much sense to select the next entry, or? */
    if (sorttab[inst]->current_entry == entry)
    {
	gboolean valid;
	TabEntry *entry2=NULL;
	GtkTreeIter iter2;
	/* what's the next entry (displayed)? */
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{ /* found selected entry -> now chose next one */
	    if (gtk_tree_model_iter_next (st->model, &iter))
	    {
		gtk_tree_model_get(st->model, &iter, ST_COLUMN_ENTRY, &next, -1);
	    }
	    else
	    { /* no next entry, try to find previous one */
		/* There doesn't seem to be a ..._iter_previous()
		 * call... */
		next = NULL;
		valid = gtk_tree_model_get_iter_first(st->model, &iter2);
		while(valid)
		{
		    gtk_tree_model_get(st->model, &iter2, ST_COLUMN_ENTRY, &entry2, -1);
		    if (entry == entry2)   break;  /* found it */
		    iter = iter2;
		    next = entry2;
		    valid = gtk_tree_model_iter_next(st->model, &iter2);
		}
		if (!valid) next = NULL;
	    }
	    /* don't select master entry 'All' until requested to do so */
	    if (next && next->master && !prefs_get_st_autoselect (inst))
		next = NULL;
	}
    }
#endif
    /* remove entry from display model */
    st_remove_entry_from_model (entry, inst);
    /* if we have a next entry, select it. */
    if (next)
    {
	gtk_tree_selection_select_iter (selection, &iter);
    }
}

/* Get the correct name for the entry according to currently
   selected category (page). Do _not_ g_free() the return value! */
static gchar *st_get_entryname (Track *track, guint32 inst)
{
    T_item t_item = ST_to_T (sorttab[inst]->current_category);

    return track_get_item (track, t_item);
}


/* Returns the entry "track" is stored in or NULL. The master entry
   "All" is skipped */
static TabEntry *st_get_entry_by_track (Track *track, guint32 inst)
{
  GList *entries;
  TabEntry *entry;
  guint i;

  if (track == NULL) return NULL;
  entries = sorttab[inst]->entries;
  i=1; /* skip master entry, which is supposed to be at first position */
  while ((entry = (TabEntry *)g_list_nth_data (entries, i)) != NULL)
    {
      if (g_list_find (entry->members, track) != NULL)   break; /* found! */
      ++i;
    }
  return entry;
}


/* Find TabEntry with name "name". Return NULL if no entry was found.
   If "name" is {-1, 0x00}, it returns the master entry. Otherwise
   it skips the master entry. */
static TabEntry *st_get_entry_by_name (gchar *name, guint32 inst)
{
  TabEntry *entry = NULL;
  SortTab *st = sorttab[inst];
  GList *entries = st->entries;

  if (name == NULL) return NULL;
  /* check if we need to return the master entry */
  if ((strlen (name) == 1) && (*name == -1))
  {
      entry = (TabEntry *)g_list_nth_data (entries, 0);
  }
  else
  {
      if (st->entry_hash)
	  entry = g_hash_table_lookup (st->entry_hash, name);
  }
  return entry;
}


/* moves a track from the entry it is currently in to the one it
   should be in according to its tags (if a Tag had been changed).
   Returns TRUE, if track has been moved, FALSE otherwise */
/* 07 Feb 2003: I decided that recategorizing is a bad thing: the
   current code only moves the tracks "up" in the entry list, so it's
   incomplete. More important: it leaves the display in an
   inconsistent state: the tracks are not removed from the
   trackview (this would confuse the user). But if he changes the entry
   again, nothing happens to the tracks displayed, because they are no
   longer members. Merging the two identical entries is no option
   either, because that takes away the possibility to easily "undo"
   what you have just done. It's also not intuitive if you have
   additional tracks appear on the screen. JCS */
static gboolean st_recategorize_track (Track *track, guint32 inst)
{
#if 0
  TabEntry *oldentry, *newentry;
  gchar *entryname;

  oldentry = st_get_entry_by_track (track, inst);
  /*  printf("%d: recat_oldentry: %x\n", inst, oldentry);*/
  /* should not happen: track is not in sort tab */
  if (oldentry == NULL) return FALSE;
  entryname = st_get_entryname (track, inst);
  newentry = st_get_entry_by_name (entryname, inst);
  if (newentry == NULL)
    { /* not found, create new one */
      newentry = g_malloc0 (sizeof (TabEntry));
      newentry->name = g_strdup (entryname);
      newentry->master = FALSE;
      st_add_entry (newentry, inst);
    }
  if (newentry != oldentry)
    { /* track category changed */
      /* add track to entry members list */
      newentry->members = g_list_append (newentry->members, track);
      /* remove track from old entry members list */
      oldentry->members = g_list_remove (oldentry->members, track);
      /*  printf("%d: recat_return_TRUE\n", inst);*/
      return TRUE;
    }
  /*  printf("%d: recat_return_FALSE\n", inst);*/
#endif
  return FALSE;
}



/* called by st_track_changed */
static void st_track_changed_normal (Track *track, gboolean removed, guint32 inst)
{
    SortTab *st;
    TabEntry *master, *entry;

    st = sorttab[inst];
    master = g_list_nth_data (st->entries, 0);
    if (master == NULL) return; /* should not happen */
    /* if track is not in tab, don't proceed (should not happen) */
    if (g_list_find (master->members, track) == NULL) return;
    if (removed)
    {
	/* remove "track" from master entry "All" */
	master->members = g_list_remove (master->members, track);
	/* find entry which other entry contains the track... */
	entry = st_get_entry_by_track (track, inst);
	/* ...and remove it */
	if (entry) entry->members = g_list_remove (entry->members, track);
	if ((st->current_entry == entry) || (st->current_entry == master))
	    st_track_changed (track, TRUE, inst+1);
    }
    else
    {
	if (st->current_entry &&
	    g_list_find (st->current_entry->members, track) != NULL)
	{ /* "track" is in currently selected entry */
	    if (!st->current_entry->master)
	    { /* it's not the master list */
		if (st_recategorize_track (track, inst))
		    st_track_changed (track, TRUE, inst+1);
		else st_track_changed (track, FALSE, inst+1);
	    }
	    else
	    { /* master entry ("All") is currently selected */
		st_recategorize_track (track, inst);
		st_track_changed (track, FALSE, inst+1);
	    }
	}
	else
	{ /* "track" is not in an entry currently selected */
	    if (st_recategorize_track (track, inst))
	    { /* track was moved to a different entry */
		if (st_get_entry_by_track (track, inst) == st->current_entry)
		{ /* this entry is selected! */
		    st_add_track (track, TRUE, TRUE, inst+1);
		}
	    }
	}
    }
}



/* Some tags of a track currently stored in a sort tab have been changed.
   - if not "removed"
     - if the track is in the entry currently selected:
       - remove entry and put into correct category
       - if current entry != "All":
         - if sort category changed:
           - notify next sort tab ("removed")
	 - if sort category did not change:
	   - notify next sort tab ("not removed")
       - if current entry == "All":
         - notify next sort tab ("not removed")
     - if the track is not in the entry currently selected (I don't know
       how that could happen, though):
       - if sort category changed: remove entry and put into correct category
       - if this "correct" category is selected, call st_add_track for next
         instance.
   - if "removed"
     - remove the track from the sort tab
     - if track was in the entry currently selected, notify next instance
       ("removed")
  "removed": track has been removed from sort tab. This is different
  from st_remove_track, because we will not notify the track model if a
  track has been removed: it might confuse the user if the track, whose
  tabs he/she just edited, disappeared from the display */
void st_track_changed (Track *track, gboolean removed, guint32 inst)
{
  if (inst == prefs_get_sort_tab_num ())
    {
      tm_track_changed (track);
      return;
    }
  else if (inst < prefs_get_sort_tab_num ())
  {
      switch (sorttab[inst]->current_category)
      {
      case ST_CAT_ARTIST:
      case ST_CAT_ALBUM:
      case ST_CAT_GENRE:
      case ST_CAT_COMPOSER:
      case ST_CAT_TITLE:
      case ST_CAT_YEAR:
	  st_track_changed_normal (track, removed, inst);
	  break;
      case ST_CAT_SPECIAL:
	  st_track_changed_special (track, removed, inst);
	  break;
      case ST_CAT_NUM:
	  break;
      }
  }
}


/* Reorders the tracks stored in the sort tabs according to the order
 * in the selected playlist. This has to be done e.g. if we change the
 * order in the track view.
 *
 * Right now I simply delete all members of all tab entries, then add
 * the tracks again without having them added to the track view. This
 * is very fast because neither the sort tab display nor the track
 * view display is affected in any way. For my 2459 tracks that takes
 * approx. 1.3 seconds (850 MHz AMD Duron) */
void st_adopt_order_in_playlist (void)
{
    gint inst;
    Playlist *current_playlist = pm_get_selected_playlist ();

#if DEBUG_TIMING
    GTimeVal time;
    g_get_current_time (&time);
    printf ("st_adopt_order_in_playlist enter: %ld.%06ld sec\n",
	    time.tv_sec % 3600, time.tv_usec);
#endif

    /* first delete all tracks in all visible sort tabs */
    for (inst = 0; inst< prefs_get_sort_tab_num (); ++inst)
    {
	SortTab *st = sorttab[inst];
	GList *link;
	for (link=st->entries; link; link=link->next)
	{   /* in each entry delete all tracks */
	    TabEntry *entry = (TabEntry *)link->data;
	    g_list_free (entry->members);
	    entry->members = NULL;
	}
    }

    /* now add the tracks again, without adding them to the track view */
    if (current_playlist)
    {
	GList *link;

	for (link=current_playlist->members; link; link=link->next)
	{
	    st_add_track ((Track *)link->data, FALSE, FALSE, 0);
	}
    }
#if DEBUG_TIMING
    g_get_current_time (&time);
    printf ("st_adopt_order_in_playlist enter: %ld.%06ld sec\n",
	    time.tv_sec % 3600, time.tv_usec);
#endif
}


/* called by st_add_track() */
static void st_add_track_normal (Track *track, gboolean final,
				gboolean display, guint32 inst)
{
    SortTab *st;
    TabEntry *entry, *master_entry, *iter_entry;
    gchar *entryname;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    TabEntry *select_entry = NULL;
    gboolean first = FALSE;

    st = sorttab[inst];
    st->final = final;

/*       if (track)   printf ("%d: add track: %s\n", inst, track->title); */
/*       else        printf ("%d: add track: %p\n", inst, track); */

    if (track != NULL)
    {
	/* add track to "All" (master) entry */
	master_entry = g_list_nth_data (st->entries, 0);
	if (master_entry == NULL)
	{ /* doesn't exist yet -- let's create it */
	    master_entry = g_malloc0 (sizeof (TabEntry));
	    master_entry->name = g_strdup (_("All"));
	    master_entry->master = TRUE;
	    st_add_entry (master_entry, inst);
	    first = TRUE; /* this is the first track */
	}
	master_entry->members = g_list_append (master_entry->members, track);
	/* Check whether entry of same name already exists */
	entryname = st_get_entryname (track, inst);
	entry = st_get_entry_by_name (entryname, inst);
	if (entry == NULL)
	{ /* not found, create new one */
	    entry = g_malloc0 (sizeof (TabEntry));
	    entry->name = g_strdup (entryname);
	    entry->master = FALSE;
	    st_add_entry (entry, inst);
	}
	/* add track to entry members list */
	entry->members = g_list_append (entry->members, track);
	/* add track to next tab if "entry" is selected */
	if (st->current_entry &&
	    ((st->current_entry->master) || (entry == st->current_entry)))
	{
	    st_add_track (track, final, display, inst+1);
	}
	/* check if we should select some entry */
	if (!st->current_entry)
	{
	    if (st->lastselection[st->current_category] == NULL)
	    {
		/* no last selection -- check if we should select "All" */
		/* only select "All" when currently adding the first track */
		if (first && prefs_get_st_autoselect (inst))
		{
		    select_entry = master_entry;
		}
	    }
	    else
	    {
		/* select current entry if it corresponds to the last
		   selection, or last_entry if that's the master entry */
		TabEntry *last_entry = st_get_entry_by_name (
		    st->lastselection[st->current_category], inst);
		if (last_entry &&
		    ((entry == last_entry) || last_entry->master))
		{
		    select_entry = last_entry;
		}
	    }
	}
    }
    /* select "All" if it's the last track added, no entry currently
       selected (including "select_entry", which is to be selected" and
       prefs_get_st_autoselect() allows us to select "All" */
    if (final && !st->current_entry && !select_entry &&
	!st->unselected && prefs_get_st_autoselect (inst))
    { /* auto-select entry "All" */
	select_entry = g_list_nth_data (st->entries, 0);
    }

    if (select_entry)
    {  /* select current select_entry */
/* 	  printf("%d: selecting: %p: %s\n", inst, select_entry, select_entry->name); */
	if (!gtk_tree_model_get_iter_first (st->model, &iter))
	{
	    g_warning ("Programming error: st_add_track: iter invalid\n");
	    return;
	}
	do {
	    gtk_tree_model_get (st->model, &iter,
				ST_COLUMN_ENTRY, &iter_entry,
				-1);
	    if (iter_entry == select_entry)
	    {
		selection = gtk_tree_view_get_selection
		    (st->treeview[st->current_category]);
		/* We may need to unselect the previous selection */
		/* gtk_tree_selection_unselect_all (selection); */
		st->current_entry = select_entry;
		gtk_tree_selection_select_iter (selection, &iter);
		break;
	    }
	} while (gtk_tree_model_iter_next (st->model, &iter));
    }
    else if (!track && final)
    {
	st_add_track (NULL, final, display, inst+1);
    }
}

/* Add track to sort tab. If the track matches the currently
   selected sort criteria, it will be passed on to the next
   sort tab. The last sort tab will pass the track on to the
   track model (currently two sort tabs).
   When the first track is added, the "All" entry is created.
   If prefs_get_st_autoselect(inst) is true, the "All" entry is
   automatically selected, if there was no former selection
   @display: TRUE: add to track model (i.e. display it) */
void st_add_track (Track *track, gboolean final, gboolean display, guint32 inst)
{
  static gint count = 0;

#if DEBUG_ADD_TRACK
  printf ("st_add_track: inst: %d, final: %d, display: %d, track: %p\n",
	  inst, final, display, track);
#endif

  if (inst == prefs_get_sort_tab_num ())
  {  /* just add to track model */
      if ((track != NULL) && display)    tm_add_track_to_track_model (track, NULL);
      if (final || (++count % 20 == 0))
	  gtkpod_tracks_statusbar_update();
  }
  else if (inst < prefs_get_sort_tab_num ())
  {
      switch (sorttab[inst]->current_category)
      {
      case ST_CAT_ARTIST:
      case ST_CAT_ALBUM:
      case ST_CAT_GENRE:
      case ST_CAT_COMPOSER:
      case ST_CAT_TITLE:
      case ST_CAT_YEAR:
	  st_add_track_normal (track, final, display, inst);
	  break;
      case ST_CAT_SPECIAL:
	  st_add_track_special (track, final, display, inst);
	  break;
      case ST_CAT_NUM:
	  break;
      }
  }
}


/* called by st_remove_track() */
static void st_remove_track_normal (Track *track, guint32 inst)
{
    TabEntry *master, *entry;
    SortTab *st = sorttab[inst];

    master = g_list_nth_data (st->entries, 0);
    if (master == NULL) return; /* should not happen! */
    /* remove "track" from master entry "All" */
    master->members = g_list_remove (master->members, track);
    /* find entry which other entry contains the track... */
    entry = st_get_entry_by_track (track, inst);
    /* ...and remove it */
    if (entry) entry->members = g_list_remove (entry->members, track);
    st_remove_track (track, inst+1);
}


/* Remove track from sort tab. If the track matches the currently
   selected sort criteria, it will be passed on to the next
   sort tab (i.e. removed).
   The last sort tab will remove the
   track from the track model (currently two sort tabs). */
/* 02. Feb 2003: bugfix: track is always passed on to the next sort
 * tab: it might have been recategorized, but still be displayed. JCS */
void st_remove_track (Track *track, guint32 inst)
{
    if (inst == prefs_get_sort_tab_num ())
    {
	tm_remove_track (track);
    }
    else if (inst < prefs_get_sort_tab_num ())
    {
	switch (sorttab[inst]->current_category)
	{
	case ST_CAT_ARTIST:
	case ST_CAT_ALBUM:
	case ST_CAT_GENRE:
	case ST_CAT_COMPOSER:
	case ST_CAT_TITLE:
	case ST_CAT_YEAR:
	    st_remove_track_normal (track, inst);
	    break;
	case ST_CAT_SPECIAL:
	    st_remove_track_special (track, inst);
	    break;
	case ST_CAT_NUM:
	    break;
	}
    }
}



/* Init a sort tab: all current entries are removed. The next sort tab
   is initialized as well (st_init (-1, inst+1)).  Set new_category to
   -1 if the current category is to be left unchanged */
/* Normally we do not specifically remember the "All" entry and will
   select "All" in accordance to the prefs settings. */
void st_init (ST_CAT_item new_category, guint32 inst)
{
  if (inst == prefs_get_sort_tab_num ())
  {
      tm_remove_all_tracks ();
      gtkpod_tracks_statusbar_update ();
      return;
  }
  if (inst < prefs_get_sort_tab_num ())
  {
      SortTab *st = sorttab[inst];

      if (st == NULL) return; /* could happen during initialisation */
      sp_store_sp_entries (inst); /* store sp entries (if applicable) */
      st->unselected = FALSE; /* nothing was unselected so far */
      st->final = TRUE;       /* all tracks are added */
      st->is_go = FALSE;      /* did not press "Display" yet (special) */
#if 0
      if (st->current_entry != NULL)
      {
	  ST_CAT_item cat = st->current_category;
	  if (!st->current_entry->master)
	  {
	      C_FREE (st->lastselection[cat]);
	      st->lastselection[cat] = g_strdup (st->current_entry->name);
	  }
/* don't remember entry 'All' */
#if 0
	  else
	  {
	      gchar buf[] = {-1, 0}; /* this is how I mark the "All"
				      * entry as string: should be
				      * illegal UTF8 */
	      C_FREE (st->lastselection[cat]);
	      st->lastselection[cat] = g_strdup (buf);*/
	  }
#endif
      }
#endif
      switch (sorttab[inst]->current_category)
      {
      case ST_CAT_ARTIST:
      case ST_CAT_ALBUM:
      case ST_CAT_GENRE:
      case ST_CAT_COMPOSER:
      case ST_CAT_TITLE:
      case ST_CAT_YEAR:
	  st_remove_all_entries_from_model (inst);
	  break;
      case ST_CAT_SPECIAL:
	  sp_remove_all_members (inst);
	  break;
      case ST_CAT_NUM:
	  break;
      }
      if (new_category != -1)
      {
	  st->current_category = new_category;
	  prefs_set_st_category (inst, new_category);
      }
      st_init (-1, inst+1);
  }
}


static void st_page_selected_cb (gpointer user_data1, gpointer user_data2)
{
  GtkNotebook *notebook = (GtkNotebook *)user_data1;
  guint page = (guint)user_data2;
  guint32 inst = st_get_instance_from_notebook (notebook);
  guint oldpage;
  gboolean is_go;
  GList *copy = NULL;
  SortTab *st;

#if DEBUG_TIMING
  GTimeVal time;
  g_get_current_time (&time);
  printf ("st_page_selected_cb enter (inst: %d, page: %d): %ld.%06ld sec\n",
	  inst, page,
	  time.tv_sec % 3600, time.tv_usec);
#endif

/*  printf("ps%d: cat: %d\n", inst, page);*/

  if (inst == -1) return; /* invalid notebook */
  if (stop_add < (gint)inst)  return;
  st = sorttab[inst];
  /* remember old is_go state and current page */
  is_go = st->is_go;
  oldpage = st->current_category;
  /* re-initialize current instance */
  st_init (page, inst);
  /* write back old is_go state if page hasn't changed (redisplay) */
  if (page == oldpage)  st->is_go = is_go;
  /* Get list of tracks to re-insert */
  copy = display_get_selected_members (inst-1);
  if (copy)
  {
      GTimeVal time;
      float max_count = REFRESH_INIT_COUNT;
      gint count = max_count - 1;
      float ms;
      GList *gl;
      /* block playlist view and all sort tab notebooks <= inst */
      if (!prefs_get_block_display ())
      {
	  block_selection (inst-1);
	  g_get_current_time (&time);
      }
      /* add all tracks previously present to sort tab */
      st_enable_disable_view_sort (inst, FALSE);
      for (gl=copy; gl; gl=gl->next)
      {
	  Track *track = gl->data;
	  if (stop_add < (gint)inst)  break;
	  st_add_track (track, FALSE, TRUE, inst);
	  --count;
	  if ((count < 0) && !prefs_get_block_display ())
	  {
	      gtkpod_tracks_statusbar_update();
	      while (gtk_events_pending ())       gtk_main_iteration ();
	      ms = get_ms_since (&time, TRUE);
	      /* first time takes significantly longer, so we adjust
		 the max_count */
	      if (max_count == REFRESH_INIT_COUNT) max_count *= 2.5;
	      /* average the new and the old max_count */
	      max_count *= (1 + 2 * REFRESH_MS / ms) / 3;
	      count = max_count - 1;
#if DEBUG_TIMING
	      printf("st_p_s ms: %f mc: %f\n", ms, max_count);
#endif
	  }
      }
      st_enable_disable_view_sort (inst, TRUE);
      if (stop_add >= (gint)inst)
      {
	  gboolean final = TRUE;  /* playlist is always complete */
	  /* if playlist is not source, get final flag from
	   * corresponding sorttab */
	  if ((inst > 0) && (sorttab[inst-1])) final = sorttab[inst-1]->final;
	  st_add_track (NULL, final, TRUE, inst);
      }
      if (!prefs_get_block_display ())
      {
	  while (gtk_events_pending ())      gtk_main_iteration ();
	  release_selection (inst-1);
      }
  }
#if DEBUG_TIMING
  g_get_current_time (&time);
  printf ("st_page_selected_cb exit (inst: %d, page: %d):  %ld.%06ld sec\n",
	  inst, page,
	  time.tv_sec % 3600, time.tv_usec);
#endif
}


/* Called when page in sort tab is selected */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the st_page_selected_cb is actually called */
void st_page_selected (GtkNotebook *notebook, guint page)
{
  guint32 inst;

  inst = st_get_instance_from_notebook (notebook);
#if DEBUG_CB_INIT
  printf ("st_page_selected: inst: %d, page: %d\n", inst, page);
#endif
  if (inst == -1) return; /* invalid notebook */
  /* inst-1: changing a page in the first sort tab is like selecting a
     new playlist and so on. Therefore we subtract 1 from the
     instance. */
  add_selection_callback (inst-1, st_page_selected_cb,
			  (gpointer)notebook, (gpointer)page);
}


/* Redisplay the sort tab "inst" */
void st_redisplay (guint32 inst)
{
    if (inst < prefs_get_sort_tab_num ())
    {
	if (sorttab[inst])
	    st_page_selected (sorttab[inst]->notebook,
			      sorttab[inst]->current_category);
    }
}

/* Start sorting */
static void st_sort_inst (guint32 inst, GtkSortType order)
{
    if (inst < prefs_get_sort_tab_num ())
    {
	SortTab *st = sorttab[inst];
	if (st)
	{
	    switch (st->current_category)
	    {
	    case ST_CAT_ARTIST:
	    case ST_CAT_ALBUM:
	    case ST_CAT_GENRE:
	    case ST_CAT_COMPOSER:
	    case ST_CAT_TITLE:
	    case ST_CAT_YEAR:
		if (order != SORT_NONE)
		    gtk_tree_sortable_set_sort_column_id (
			GTK_TREE_SORTABLE (st->model),
			ST_COLUMN_ENTRY, order);
		else if (inst == 0)
		{   /* we only redisplay for st0 because the others
		       are reinitialized automatically */
		    /* and we only redisplay if the tree is actually
		       sorted */
		    gint column;
		    GtkSortType order;
		    if (gtk_tree_sortable_get_sort_column_id
			(GTK_TREE_SORTABLE (st->model), &column, &order))
			st_redisplay (0);
		}
		break;
	    case ST_CAT_SPECIAL:
	    case ST_CAT_NUM:
		break;
	    }
	}
    }
}


void st_sort (GtkSortType order)
{
    gint i;
    for (i=0; i<prefs_get_sort_tab_num (); ++i)
	st_sort_inst (i, order);
}


static void st_selection_changed_cb (gpointer user_data1, gpointer user_data2)
{
  GtkTreeSelection *selection = (GtkTreeSelection *)user_data1;
  guint32 inst = (guint32)user_data2;
  GtkTreeModel *model;
  GtkTreeIter  iter;
  TabEntry *new_entry;
  SortTab *st;

#if DEBUG_TIMING || DEBUG_CB_INIT
  GTimeVal time;
  g_get_current_time (&time);
  printf ("st_selection_changed_cb enter (inst: %d): %ld.%06ld sec\n",
	  inst, time.tv_sec % 3600, time.tv_usec);
#endif

/*   printf("st_s_c_cb %d: entered\n", inst); */
  st = sorttab[inst];
  if (st == NULL) return;
  if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE)
  {
      /* no selection -- unselect current selection (unless
         st->current_entry == NULL -- that means we're in the middle
         of a  st_init() (removing all entries). In that case we don't
	 want to forget our last selection! */
      if (st->current_entry)
      {
	  st->current_entry = NULL;
	  C_FREE (st->lastselection[st->current_category]);
	  st->unselected = TRUE;
      }
	  st_init (-1, inst+1);
  }
  else
  {   /* handle new selection */
      gtk_tree_model_get (model, &iter,
			  ST_COLUMN_ENTRY, &new_entry,
			  -1);
      /* printf("selected instance %d, entry %x (was: %x)\n", inst,
       * new_entry, st->current_entry);*/

      /* initialize next instance */
      st_init (-1, inst+1);
      /* remember new selection */
      st->current_entry = new_entry;
      if (!new_entry->master)
      {
	  C_FREE (st->lastselection[st->current_category]);
	  st->lastselection[st->current_category] = g_strdup (new_entry->name);
      }
      st->unselected = FALSE;

      if (new_entry->members)
      {
	  GTimeVal time;
	  float max_count = REFRESH_INIT_COUNT;
	  gint count = max_count - 1;
	  float ms;
	  GList *gl;
	  if (!prefs_get_block_display ())
	  {
	      block_selection (inst);
	      g_get_current_time (&time);
	  }
	  st_enable_disable_view_sort (inst+1, FALSE);
	  for (gl=new_entry->members; gl; gl=gl->next)
	  { /* add all member tracks to next instance */
	      Track *track = gl->data;
	      if (stop_add <= (gint)inst) break;
	      st_add_track (track, FALSE, TRUE, inst+1);
	      --count;
	      if ((count < 0) && !prefs_get_block_display ())
	      {
		  gtkpod_tracks_statusbar_update();
		  while (gtk_events_pending ())       gtk_main_iteration ();
		  ms = get_ms_since (&time, TRUE);
		  /* first time takes significantly longer, so we adjust
		     the max_count */
		  if (max_count == REFRESH_INIT_COUNT) max_count *= 2.5;
		  /* average the new and the old max_count */
		  max_count *= (1 + 2 * REFRESH_MS / ms) / 3;
		  count = max_count - 1;
#if DEBUG_TIMING
		  printf("st_s_c ms: %f mc: %f\n", ms, max_count);
#endif
	      }
	  }
	  st_enable_disable_view_sort (inst+1, TRUE);
	  if (stop_add > (gint)inst)
	      st_add_track (NULL, TRUE, st->final, inst+1);
	  if (!prefs_get_block_display ())
	  {
	      while (gtk_events_pending ())	  gtk_main_iteration ();
	      release_selection (inst);
	  }
      }
      gtkpod_tracks_statusbar_update();
  }
#if DEBUG_TIMING
  g_get_current_time (&time);
  printf ("st_selection_changed_cb exit:  %ld.%06ld sec\n",
	  time.tv_sec % 3600, time.tv_usec);
#endif
}


/* Callback function called when the selection
   of the sort tab view has changed */
/* Instead of handling the selection directly, we add a
   "callback". Currently running display updates will be stopped
   before the st_selection_changed_cb is actually called */
static void st_selection_changed (GtkTreeSelection *selection,
				  gpointer user_data)
{
#if DEBUG_CB_INIT
    printf("st_s_c enter (inst: %d)\n", (gint)user_data);
#endif
    space_data_update ();
    add_selection_callback ((gint)user_data, st_selection_changed_cb,
			    (gpointer)selection, user_data);
#if DEBUG_CB_INIT
    printf("st_s_c exit (inst: %d)\n", (gint)user_data);
#endif
}


/* Called when editable cell is being edited. Stores new data to
   the entry list and changes all members. */
static void
st_cell_edited (GtkCellRendererText *renderer,
		const gchar         *path_string,
		const gchar         *new_text,
		gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  TabEntry *entry;
  ST_item column;
  gint i, n, inst;
  GList *members;
  SortTab *st;

  inst = (guint32)data;
  st = sorttab[inst];
  model = st->model;
  path = gtk_tree_path_new_from_string (path_string);
  column = (ST_item)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &entry, -1);

  /*printf("Inst %d: st_cell_edited: column: %d  :%lx\n", inst, column, entry);*/

  switch (column)
    {
    case ST_COLUMN_ENTRY:
      /* We only do something, if the name actually got changed */
      if (g_utf8_collate (entry->name, new_text) != 0)
      {
	  iTunesDB *itdb = NULL;
	  /* remove old hash entry if available */
	  TabEntry *hash_entry =
	      g_hash_table_lookup (st->entry_hash, entry->name);
	  if (hash_entry == entry)
	      g_hash_table_remove (st->entry_hash, entry->name);
	  /* replace entry name */
	  if (sorttab[inst]->current_category == ST_CAT_YEAR)
	  {   /* make sure the entry->name is identical to
		 atoi(new_text) */
	      g_free (entry->name);
	      entry->name = g_strdup_printf ("%d", atoi (new_text));
	      g_object_set (G_OBJECT (renderer), "text", entry->name, NULL);
	  }
	  else
	  {
	      g_free (entry->name);
	      entry->name = g_strdup (new_text);
	  }

	  /* re-insert into hash table if the same name doesn't
	     already exist */
	  if (g_hash_table_lookup (st->entry_hash, entry->name) == NULL)
	      g_hash_table_insert (st->entry_hash, entry->name, entry);
	  /* Now we look up all the tracks and change the ID3 Tag as well */
	  /* We make a copy of the current members list, as it may change
             during the process */
	  members = g_list_copy (entry->members);
	  n = g_list_length (members);
	  /* block user input if we write tags (might take a while) */
	  if (prefs_get_id3_write ())   block_widgets ();
	  for (i=0; i<n; ++i)
	  {
	      ExtraTrackData *etr;
	      Track *track = (Track *)g_list_nth_data (members, i);
	      T_item t_item;

	      g_return_if_fail (track);
	      etr = track->userdata;
	      g_return_if_fail (etr);
	      g_return_if_fail (track->itdb);
	      if (!itdb) itdb = track->itdb;

	      t_item = ST_to_T (sorttab[inst]->current_category);

	      if (t_item == T_YEAR)
	      {
		  gint nr = atoi (new_text);
		  if (nr < 0)  nr = 0;
		  track->year = nr;
		  g_free (etr->year_str);
		  etr->year_str = g_strdup_printf ("%d", nr);
	      }
	      else
	      {
		  gchar **itemp_utf8 = track_get_item_pointer (track, t_item);
		  g_return_if_fail (itemp_utf8);
		  g_free (*itemp_utf8);
		  *itemp_utf8 = g_strdup (new_text);
	      }
	      track->time_modified = itdb_time_get_mac_time ();
	      pm_track_changed (track);
	      /* If prefs say to write changes to file, do so */
	      if (prefs_get_id3_write ())
	      {
		  /* T_item tag_id;*/
		  /* should we update all ID3 tags or just the one
		     changed? -- obsoleted in 0.71 */
/*		  if (prefs_get_id3_writeall ()) tag_id = T_ALL;
		  else		                 tag_id = t_item;*/
		  write_tags_to_file (track);
		  while (widgets_blocked && gtk_events_pending ())
		      gtk_main_iteration ();
	      }
	  }
	  g_list_free (members);
	  /* allow user input again */
	  if (prefs_get_id3_write ())   release_widgets ();
	  /* display possible duplicates that have been removed */
	  gp_duplicate_remove (NULL, NULL);
	  /* indicate that data has changed */
	  if (itdb) data_changed (itdb);
      }
      break;
    default:
	break;
    }
  gtk_tree_path_free (path);
}


/* The sort tab entries are stored in a separate list (sorttab->entries)
   and only pointers to the corresponding TabEntry structure are placed
   into the model.
   This function reads the data for the given cell from the list and
   passes it to the renderer. */
static void st_cell_data_func (GtkTreeViewColumn *tree_column,
			       GtkCellRenderer   *renderer,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
  TabEntry *entry;
  gint column;

  column = (gint)g_object_get_data (G_OBJECT (renderer), "column");
  gtk_tree_model_get (model, iter, ST_COLUMN_ENTRY, &entry, -1);

  switch (column)
    {  /* We only have one column, so this code is overkill... */
    case ST_COLUMN_ENTRY:
      if (entry->master)
      {   /* mark the "All" entry */
	  g_object_set (G_OBJECT (renderer),
			"text", entry->name,
			"editable", FALSE,
			"weight", PANGO_WEIGHT_BOLD,
			NULL);
      }
      else
      {
	  g_object_set (G_OBJECT (renderer),
			"text", entry->name,
			"editable", TRUE,
			"weight", PANGO_WEIGHT_NORMAL,
			NULL);
      }
      break;
    }
}

/* Function used to compare two cells during sorting (sorttab view) */
gint st_data_compare_func (GtkTreeModel *model,
			   GtkTreeIter *a,
			   GtkTreeIter *b,
			   gpointer user_data)
{
  TabEntry *entry1;
  TabEntry *entry2;
  GtkSortType order;
  gint corr, colid;
  gint inst;
  SortTab *st;

  inst = (guint32)user_data;

  gtk_tree_model_get (model, a, ST_COLUMN_ENTRY, &entry1, -1);
  gtk_tree_model_get (model, b, ST_COLUMN_ENTRY, &entry2, -1);
  if(gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					   &colid, &order) == FALSE)
      return 0;

  /* We make sure that the "all" entry always stays on top */
  if (order == GTK_SORT_ASCENDING) corr = +1;
  else                             corr = -1;
  if (entry1->master) return (-corr);
  if (entry2->master) return (corr);

  /* compare the two entries */
  /* string_compare_func is set to either compare_string_fuzzy or
     compare_string in on_st_switch_page() which is called
     once before the comparing begins. */

  st = sorttab[inst];

  return st->string_compare_func (entry1->name, entry2->name);
}

/* Stop editing. If @cancel is TRUE, the edited value will be
   discarded (I have the feeling that the "discarding" part does not
   work quite the way intended). */
void st_stop_editing (gint inst, gboolean cancel)
{
    if (inst < prefs_get_sort_tab_num ())
    {
	SortTab *st = sorttab[inst];
	if (st)
	{
	    GtkTreeViewColumn *col;
	    gtk_tree_view_get_cursor (st->treeview[st->current_category],
				      NULL, &col);
	    if (col)
	    {
		if (!cancel && col->editable_widget)
		    gtk_cell_editable_editing_done (col->editable_widget);
		if (col->editable_widget)
		    gtk_cell_editable_remove_widget (col->editable_widget);
	    }
	}
    }
}


/* Compare function to avoid sorting */
static gint st_nosort_comp (GtkTreeModel *model,
			    GtkTreeIter *a,
			    GtkTreeIter *b,
			    gpointer user_data)
{
    return 0;
}



/* Disable sorting of the view during lengthy updates. */
/* @enable: TRUE: enable, FALSE: disable */
void st_enable_disable_view_sort (gint inst, gboolean enable)
{
    static gint disable_count[SORT_TAB_MAX];

    if (inst >= prefs_get_sort_tab_num ())
    {
	tm_enable_disable_view_sort (enable);
	return;
    }

    if (enable)
    {
	disable_count[inst]--;
	if (disable_count[inst] < 0)
	    fprintf (stderr, "Programming error: disable_count < 0\n");
	if (disable_count[inst] == 0)
	{
	    /* Re-enable sorting */
	    if ((prefs_get_st_sort () != SORT_NONE) &&
		prefs_get_disable_sorting ())
	    {
		SortTab *st = sorttab[inst];
		if (st && 
		    (st->current_category != ST_CAT_SPECIAL) &&
		    st->model)
		{
		    if (BROKEN_GTK_TREE_SORT)
		    {
			gtk_tree_sortable_set_sort_func (
			    GTK_TREE_SORTABLE (st->model),
			    ST_COLUMN_ENTRY,
			    st_data_compare_func, (gpointer)inst, NULL);
		    }
		    else
		    {
			gtk_tree_sortable_set_sort_column_id (
			    GTK_TREE_SORTABLE (st->model),
			    ST_COLUMN_ENTRY,
			    prefs_get_st_sort ());
		    }
		}
	    }
	    st_enable_disable_view_sort (inst+1, enable);
	}
    }
    else
    {
	if (disable_count[inst] == 0)
	{
	    /* Disable sorting */
	    if ((prefs_get_st_sort () != SORT_NONE) &&
		prefs_get_disable_sorting ())
	    {
		SortTab *st = sorttab[inst];
		if (st && 
		    (st->current_category != ST_CAT_SPECIAL) &&
		    st->model)
		{
		    if (BROKEN_GTK_TREE_SORT)
		    {
			gtk_tree_sortable_set_sort_func (
			    GTK_TREE_SORTABLE (st->model),
			    ST_COLUMN_ENTRY,
			    st_nosort_comp, NULL, NULL);
		    }
		    else
		    {
			gtk_tree_sortable_set_sort_column_id (
			    GTK_TREE_SORTABLE (st->model),
			    GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
			    prefs_get_st_sort ());
		    }
		}
	    }
	    st_enable_disable_view_sort (inst+1, enable);
	}
	disable_count[inst]++;
    }
}



void st_select_current_position (gint inst, gint x, gint y)
{
    if (inst < prefs_get_sort_tab_num ())
    {
	SortTab *st = sorttab[inst];
	if (st)
	{
	    GtkTreePath *path;
	    GtkTreeView *tv = st->treeview[st->current_category];

	    gtk_tree_view_get_path_at_pos (tv, x, y, &path, NULL, NULL, NULL);
	    if (path)
	    {
		GtkTreeSelection *ts = gtk_tree_view_get_selection (tv);
		gtk_tree_selection_select_path (ts, path);
		gtk_tree_path_free (path);
	    }
	}
    }
}


/* Make the appropriate number of sort tab instances visible */
/* Also: make the menu items "more/less sort tabs" active/inactive as
 * needed */
void st_show_visible (void)
{
    gint i,n;
    GtkWidget *w;

    if (!st_paned[0])  return;

    /* first initialize (clear) all sorttabs */
    n = prefs_get_sort_tab_num ();
    prefs_set_sort_tab_num (SORT_TAB_MAX, FALSE);
    st_init (-1, 0);
    prefs_set_sort_tab_num (n, FALSE);

    /* set the visible elements */
    for (i=0; i<n; ++i)
    {
	gtk_widget_show (GTK_WIDGET (sorttab[i]->notebook));
	if (i < PANED_NUM_ST)	gtk_widget_show (GTK_WIDGET (st_paned[i]));
    }
    /* set the invisible elements */
    for (i=n; i<SORT_TAB_MAX; ++i)
    {
	gtk_widget_hide (GTK_WIDGET (sorttab[i]->notebook));
	if (i < PANED_NUM_ST)	gtk_widget_hide (GTK_WIDGET (st_paned[i]));
    }

    /* activate / deactiveate "less sort tabs" menu item */
    w = glade_xml_get_widget (main_window_xml, "less_sort_tabs");
    if (n == 0) gtk_widget_set_sensitive (w, FALSE);
    else        gtk_widget_set_sensitive (w, TRUE);

    /* activate / deactiveate "more sort tabs" menu item */
    w = glade_xml_get_widget (main_window_xml, "more_sort_tabs");
    if (n == SORT_TAB_MAX) gtk_widget_set_sensitive (w, FALSE);
    else                   gtk_widget_set_sensitive (w, TRUE);

    /* redisplay */
    st_redisplay (0);
}


/* set the paned positions for the visible sort tabs in the prefs
 * structure. This function is called when first creating the paned
 * elements and from within st_arrange_visible_sort_tabs() */
static void st_set_visible_sort_tab_paned (void)
{
    gint i, x, y, p0, num, width;

    num = prefs_get_sort_tab_num ();
    if (num > 0)
    {
	gchar *buf;
	GtkWidget *w;

	gtk_window_get_size (GTK_WINDOW (gtkpod_window), &x, &y);
	buf = g_strdup_printf ("paned%d", PANED_PLAYLIST);
	if((w = glade_xml_get_widget (main_window_xml,  buf)))
	{
	    p0 = gtk_paned_get_position (GTK_PANED (w));
	    width = (x-p0) / num;
	    for (i=0; i<num; ++i)
	    {
		prefs_set_paned_pos (PANED_NUM_GLADE+i, width);
	    }
	}
	g_free (buf);
    }
}


/* Regularily arrange the visible sort tabs */
void st_arrange_visible_sort_tabs (void)
{
    gint i, num;

    num = prefs_get_sort_tab_num ();
    if (num > 0)
    {
	st_set_visible_sort_tab_paned ();
	for (i=0; i<num; ++i)
	{
	    if (prefs_get_paned_pos (PANED_NUM_GLADE + i) != -1)
	    {
		if (st_paned[i])
		    gtk_paned_set_position (
			st_paned[i], prefs_get_paned_pos (PANED_NUM_GLADE+i));
	    }
	}
    }
}



/* Created paned elements for sorttabs */
static void st_create_paned (void)
{
    gint i;

    /* sanity check */
    g_return_if_fail (st_paned[0] == NULL);

    for (i=0; i<PANED_NUM_ST; ++i)
    {
	GtkWidget *paned;

	paned = gtk_hpaned_new ();
	gtk_widget_show (paned);
 	if (i==0)
	{
	    GtkWidget *parent;
	    GtkWidget *dummy;
	    parent = glade_xml_get_widget (main_window_xml, "paned1");
	    dummy = glade_xml_get_widget (main_window_xml, "paned1_dummy");
	    g_return_if_fail (parent);
	    g_return_if_fail (dummy);
	    gtk_widget_destroy (dummy);
	    gtk_paned_pack1 (GTK_PANED (parent), paned, TRUE, TRUE);
	}
	else
	{
	    gtk_paned_pack2 (st_paned[i-1], paned, TRUE, TRUE);
	}
	st_paned[i] = GTK_PANED (paned);
    }
    /* set position of visible paned to decent values if not already
       set */
    if (prefs_get_paned_pos (PANED_NUM_GLADE) == -1)
	st_set_visible_sort_tab_paned ();
}

static gboolean
st_button_press_event(GtkWidget *w, GdkEventButton *e, gpointer data)
{
    if(w && e)
    {
	switch(e->button)
	{
	    case 3:
		st_select_current_position ((gint)data, e->x, e->y);
		st_context_menu_init((gint)data);
		return TRUE;
	    default:
		break;
	}

    }
    return(FALSE);
}

/* Create tracks listview */
static void st_create_listview (gint inst)
{
  GtkTreeModel *model;
  GtkListStore *liststore;
  gint i;
  SortTab *st = sorttab[inst];

  /* create model */
  if (st->model)
  {
      g_object_unref (G_OBJECT (st->model));
      st->model = NULL;
  }
  liststore = gtk_list_store_new (ST_NUM_COLUMNS, G_TYPE_POINTER);
  model = GTK_TREE_MODEL (liststore);
  st->model = model;
  /* set tree views */
  for (i=0; i<ST_CAT_NUM; ++i)
  {
      if (i != ST_CAT_SPECIAL)
      {
	  GtkTreeSelection *selection;
	  GtkTreeView *treeview;
	  GtkCellRenderer *renderer;
	  GtkTreeViewColumn *column;

	  treeview = st->treeview[i];
	  gtk_tree_view_set_model (treeview, model);
	  selection = gtk_tree_view_get_selection (treeview);
	  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	  g_signal_connect (G_OBJECT (selection), "changed",
			    G_CALLBACK (st_selection_changed),
			    (gpointer)inst);
	  /* Add column */
	  renderer = gtk_cell_renderer_text_new ();
	  g_signal_connect (G_OBJECT (renderer), "edited",
			    G_CALLBACK (st_cell_edited),
			    GINT_TO_POINTER(inst));
	  g_object_set_data (G_OBJECT (renderer), "column",
			     (gint *)ST_COLUMN_ENTRY);
	  column = gtk_tree_view_column_new ();
	  gtk_tree_view_column_pack_start (column, renderer, TRUE);
	  column = gtk_tree_view_column_new_with_attributes ("", renderer, NULL);
	  gtk_tree_view_column_set_cell_data_func (column, renderer,
						   st_cell_data_func, NULL, NULL);
	  gtk_tree_view_column_set_sort_column_id (column, ST_COLUMN_ENTRY);
	  gtk_tree_view_column_set_resizable (column, TRUE);
	  gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
	  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (liststore),
					   ST_COLUMN_ENTRY,
					   st_data_compare_func, (gpointer)inst, NULL);
	  gtk_tree_view_append_column (treeview, column);
      }
  }
}


/* Create the "special" page in the notebook and connect all the
   signals */
static void st_create_special (gint inst, GtkWidget *window)
{
      GtkWidget *special; 
      GtkWidget *viewport; 
      GtkWidget *w;
      SortTab   *st = sorttab[inst];
      gint i;
      GladeXML *special_xml;

      special_xml = glade_xml_new (xml_file, "special_sorttab", NULL);
      special = glade_xml_get_widget (special_xml, "special_sorttab");
     
      viewport = glade_xml_get_widget (special_xml, "special_viewport");


      /* according to GTK FAQ: move a widget to a new parent */
      gtk_widget_ref (viewport);
      gtk_container_remove (GTK_CONTAINER (special), viewport);
      gtk_container_add (GTK_CONTAINER (window), viewport);
      gtk_widget_unref (viewport);

      /* Connect the signal handlers and set default value. User data
	 is @inst+(additional data << SP_SHIFT) */
      /* AND/OR button */
      w = glade_xml_get_widget (special_xml, "sp_or_button");
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_or_button_toggled),
			(gpointer)inst);
      if (prefs_get_sp_or (inst))
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      else
      {
	  w = glade_xml_get_widget (special_xml, "sp_and_button");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }

      /* RATING */
      w = glade_xml_get_widget (special_xml, "sp_rating_button");
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_cond_button_toggled),
			(gpointer)((T_RATING<<SP_SHIFT) + inst));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   prefs_get_sp_cond (inst, T_RATING));
      for (i=0; i<=RATING_MAX; ++i)
      {
	  gchar *buf = g_strdup_printf ("sp_rating%d", i);
	  w = glade_xml_get_widget (special_xml, buf);
	  g_signal_connect ((gpointer)w,
			    "toggled", G_CALLBACK (on_sp_rating_n_toggled),
			    (gpointer)((i<<SP_SHIFT) + inst));
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				       prefs_get_sp_rating_n (inst, i));
	  g_free (buf);
      }

      /* PLAYCOUNT */
      w = glade_xml_get_widget (special_xml, "sp_playcount_button");
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_cond_button_toggled),
			(gpointer)((T_PLAYCOUNT<<SP_SHIFT) + inst));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   prefs_get_sp_cond (inst, T_PLAYCOUNT));
      w = glade_xml_get_widget (special_xml, "sp_playcount_low");
      g_signal_connect ((gpointer)w,
			"value_changed",
			G_CALLBACK (on_sp_playcount_low_value_changed),
			(gpointer)inst);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				 prefs_get_sp_playcount_low (inst));
      w = glade_xml_get_widget (special_xml, "sp_playcount_high");
      g_signal_connect ((gpointer)w,
			"value_changed",
			G_CALLBACK (on_sp_playcount_high_value_changed),
			(gpointer)inst);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
				 prefs_get_sp_playcount_high (inst));

      /* PLAYED */
      w = glade_xml_get_widget (special_xml, "sp_played_button");
      st->ti_played.active = w;
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_cond_button_toggled),
			(gpointer)((T_TIME_PLAYED<<SP_SHIFT) + inst));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   prefs_get_sp_cond (inst, T_TIME_PLAYED));
      w = glade_xml_get_widget (special_xml, "sp_played_entry");
      st->ti_played.entry = w;
      gtk_entry_set_text (GTK_ENTRY (w),
			  prefs_get_sp_entry (inst, T_TIME_PLAYED));
      g_signal_connect ((gpointer)w,
			"activate", G_CALLBACK (on_sp_entry_activate),
			(gpointer)((T_TIME_PLAYED<<SP_SHIFT) + inst));
      g_signal_connect ((gpointer)glade_xml_get_widget (special_xml,
						 "sp_played_cal_button"),
			"clicked",
			G_CALLBACK (on_sp_cal_button_clicked),
			(gpointer)((T_TIME_PLAYED<<SP_SHIFT) + inst));

      /* MODIFIED */
      w = glade_xml_get_widget (special_xml, "sp_modified_button");
      st->ti_modified.active = w;
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_cond_button_toggled),
			(gpointer)((T_TIME_MODIFIED<<SP_SHIFT) + inst));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   prefs_get_sp_cond (inst, T_TIME_MODIFIED));
      w = glade_xml_get_widget (special_xml, "sp_modified_entry");
      st->ti_modified.entry = w;
      gtk_entry_set_text (GTK_ENTRY (w),
			  prefs_get_sp_entry (inst, T_TIME_MODIFIED));
      g_signal_connect ((gpointer)w,
			"activate", G_CALLBACK (on_sp_entry_activate),
			(gpointer)((T_TIME_MODIFIED<<SP_SHIFT) + inst));
      g_signal_connect ((gpointer)glade_xml_get_widget (special_xml,
						 "sp_modified_cal_button"),
			"clicked",
			G_CALLBACK (on_sp_cal_button_clicked),
			(gpointer)((T_TIME_MODIFIED<<SP_SHIFT) + inst));

      /* ADDED */
      w = glade_xml_get_widget (special_xml, "sp_added_button");
      st->ti_added.active = w;
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_cond_button_toggled),
			(gpointer)((T_TIME_ADDED<<SP_SHIFT) + inst));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   prefs_get_sp_cond (inst, T_TIME_ADDED));
      w = glade_xml_get_widget (special_xml, "sp_added_entry");
      st->ti_added.entry = w;
      gtk_entry_set_text (GTK_ENTRY (w),
			  prefs_get_sp_entry (inst, T_TIME_ADDED));
      g_signal_connect ((gpointer)w,
			"activate", G_CALLBACK (on_sp_entry_activate),
			(gpointer)((T_TIME_ADDED<<SP_SHIFT) + inst));
      g_signal_connect ((gpointer)glade_xml_get_widget (special_xml,
						 "sp_added_cal_button"),
			"clicked",
			G_CALLBACK (on_sp_cal_button_clicked),
			(gpointer)((T_TIME_ADDED<<SP_SHIFT) + inst));


      g_signal_connect ((gpointer)glade_xml_get_widget (special_xml, "sp_go"),
			"clicked", G_CALLBACK (on_sp_go_clicked),
			(gpointer)inst);
      w = glade_xml_get_widget (special_xml, "sp_go_always");
      g_signal_connect ((gpointer)w,
			"toggled", G_CALLBACK (on_sp_go_always_toggled),
			(gpointer)inst);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   prefs_get_sp_autodisplay (inst));

      /* Safe pointer to tooltips */
      st->sp_tooltips_data = gtk_tooltips_data_get(glade_xml_get_widget (special_xml, "sp_modified_entry"));
      /* Show / don't show tooltips */
      g_return_if_fail (st->sp_tooltips_data);
      if (prefs_get_display_tooltips_main ()) 
	  gtk_tooltips_enable (st->sp_tooltips_data->tooltips);
      else gtk_tooltips_disable (st->sp_tooltips_data->tooltips);
      /* we don't need this any more */
      gtk_widget_destroy (special);
}


/* create the treeview for category @st_cat of instance @inst */
static void st_create_page (gint inst, ST_CAT_item st_cat)
{
  GtkWidget *st0_notebook;
  GtkWidget *st0_window0;
  GtkWidget *st0_label0 = NULL;
  SortTab *st = sorttab[inst];

  /* destroy treeview if already present */
  if (st->treeview[st_cat])
  {
      gtk_widget_destroy (GTK_WIDGET (st->treeview[st_cat]));
      st->treeview[st_cat] = NULL;
  }

  st0_notebook = GTK_WIDGET (st->notebook);

  if (st->window[st_cat])
  {
      st0_window0 = GTK_WIDGET (st->window[st_cat]);
  }
  else
  {   /* create window if not already present */
      st0_window0 = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_show (st0_window0);
      gtk_container_add (GTK_CONTAINER (st0_notebook), st0_window0);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (st0_window0), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      st->window[st_cat] = st0_window0;
  }

  switch (st_cat)
  {
  case ST_CAT_ARTIST:
      st0_label0 = gtk_label_new (_("Artist"));
      break;
  case ST_CAT_ALBUM:
      st0_label0 = gtk_label_new (_("Album"));
      break;
  case ST_CAT_GENRE:
      st0_label0 = gtk_label_new (_("Genre"));
      break;
  case ST_CAT_COMPOSER:
      st0_label0 = gtk_label_new (_("Comp."));
      break;
  case ST_CAT_TITLE:
      st0_label0 = gtk_label_new (_("Title"));
      break;
  case ST_CAT_YEAR:
      st0_label0 = gtk_label_new (_("Year"));
      break;
  case ST_CAT_SPECIAL:
      st0_label0 = gtk_label_new (_("Special"));
      break;
  case ST_CAT_NUM: /* should not happen... */
      g_return_if_reached ();
      break;
  }
  gtk_widget_show (st0_label0);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (st0_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (st0_notebook), st_cat), st0_label0);
  gtk_label_set_justify (GTK_LABEL (st0_label0), GTK_JUSTIFY_LEFT);

  if (st_cat == ST_CAT_SPECIAL)
  {
      /* create special window */
      st_create_special (inst, st0_window0);
  }
  else
  {
      GtkWidget *treeview;
      /* create treeview */
      treeview = gtk_tree_view_new ();
      st->treeview[st_cat] = GTK_TREE_VIEW (treeview);
      gtk_widget_show (treeview);
      gtk_container_add (GTK_CONTAINER (st0_window0), treeview);
      g_signal_connect_after ((gpointer) treeview, "key_release_event",
			      G_CALLBACK (on_st_treeview_key_release_event),
			      NULL);
      gtk_drag_source_set (GTK_WIDGET (treeview), GDK_BUTTON1_MASK,
			   st_drag_types, TGNR (st_drag_types),
			   GDK_ACTION_COPY|GDK_ACTION_MOVE);
      g_signal_connect (G_OBJECT (treeview), "button-press-event",
			G_CALLBACK (st_button_press_event), GINT_TO_POINTER(inst));
      g_signal_connect ((gpointer) treeview, "drag_data_get",
			G_CALLBACK (st_drag_data_get),
			NULL);
      g_signal_connect ((gpointer) treeview, "drag-end",
			G_CALLBACK (st_drag_end),
			NULL);
      gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  }
}

/* create all ST_CAT_NUM treeviews and the special page in sort tab of
 * instance @inst, then set the model */
static void st_create_pages (gint inst)
{
  st_create_page (inst, ST_CAT_ARTIST);
  st_create_page (inst, ST_CAT_ALBUM);
  st_create_page (inst, ST_CAT_GENRE);
  st_create_page (inst, ST_CAT_COMPOSER);
  st_create_page (inst, ST_CAT_TITLE);
  st_create_page (inst, ST_CAT_YEAR);
  st_create_page (inst, ST_CAT_SPECIAL);
  st_create_listview (inst);
}


/* Create notebook and fill in sorttab[@inst] */
void st_create_notebook (gint inst)
{
  GtkWidget *st0_notebook;
  GtkPaned *paned;
  gint i, page;
  SortTab *st = sorttab[inst];

  if (st->notebook)
  {
      gtk_widget_destroy (GTK_WIDGET (st->notebook));
      st->notebook = NULL;
      for (i=0; i<ST_CAT_NUM; ++i)
      {
	  st->treeview[i] = NULL;
	  st->window[i] = NULL;
      }
  }

  /* paned elements exist? */
  if (!st_paned[0])   st_create_paned ();

  st0_notebook = gtk_notebook_new ();
  if (inst < prefs_get_sort_tab_num ()) gtk_widget_show (st0_notebook);
  else                                  gtk_widget_hide (st0_notebook);
  /* which pane? */
  if (inst == SORT_TAB_MAX-1)  i = inst-1;
  else                         i = inst;
  paned = st_paned[i];
  /* how to pack? */
  if (inst == SORT_TAB_MAX-1)
      gtk_paned_pack2 (paned, st0_notebook, TRUE, TRUE);
  else
      gtk_paned_pack1 (paned, st0_notebook, FALSE, TRUE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (st0_notebook), TRUE);
  g_signal_connect ((gpointer) st0_notebook, "switch_page",
                    G_CALLBACK (on_st_switch_page),
                    (gpointer)inst);

  st->notebook = GTK_NOTEBOOK (st0_notebook);
  st_create_pages (inst);
  page = prefs_get_st_category (inst);
  st->current_category = page;
  gtk_notebook_set_current_page (st->notebook, page);
  if (prefs_get_st_sort () != SORT_NONE)
    st_sort_inst (inst, prefs_get_st_sort ());
}


/* Create sort tabs */
void st_create_tabs (void)
{
  gint inst;
/*   gchar *name; */

  /* we count downward here because the smaller sort tabs might try to
     initialize the higher one's -> create the higher ones first */
  for (inst=SORT_TAB_MAX-1; inst>=0; --inst)
  {
	sorttab[inst] = g_malloc0 (sizeof (SortTab));
	sorttab[inst]->string_compare_func = compare_string;
	st_create_notebook (inst);
  }
  st_show_visible ();
}

/* Clean up the memory used by sort tabs (program quit). */
void st_cleanup (void)
{
    gint i,j;
    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	if (sorttab[i] != NULL)
	{
	    sp_store_sp_entries (i);
	    st_remove_all_entries_from_model (i);
	    for (j=0; j<ST_CAT_NUM; ++j)
	    {
		C_FREE (sorttab[i]->lastselection[j]);
	    }
	    g_free (sorttab[i]);
	    sorttab[i] = NULL;
	}
    }
}


/* set the default sizes for the gtkpod main window according to prefs:
   position of the PANED_NUM GtkPaned elements (the width of the
   colums is set when setting up the colums in the listview. Called by
   display_set_default_sizes() */
void st_set_default_sizes (void)
{
    gint i;

    /* GtkPaned elements */
    g_return_if_fail (gtkpod_window);
    /* Elements defined with glade */
    for (i=0; i<PANED_NUM_GLADE; ++i)
    {
	gchar *buf = g_strdup_printf ("paned%d", i);
	GtkWidget *w = glade_xml_get_widget (main_window_xml,  buf);
	g_free (buf);
	g_return_if_fail (w);
	if (prefs_get_paned_pos (i) != -1)
	{
	    gtk_paned_set_position (GTK_PANED (w),
				    prefs_get_paned_pos (i));
	}
    }
    /* Elements defined with display.c (sort tab hpaned) */
    for (i=0; i<PANED_NUM_ST; ++i)
    {
	g_return_if_fail (st_paned[i]);
	if (prefs_get_paned_pos (PANED_NUM_GLADE + i) != -1)
	{
	    gtk_paned_set_position (
		st_paned[i], prefs_get_paned_pos (PANED_NUM_GLADE+i));
	}
    }
}


/* update the cfg structure (preferences) with the current sizes /
   positions (called by display_update_default_sizes():
   position of GtkPaned elements */
void st_update_default_sizes (void)
{
    /* GtkPaned elements */
    if (gtkpod_window)
    {
	gint i;
	/* Elements defined with glade */
	for (i=0; i<PANED_NUM_GLADE; ++i)
	{
	    gchar *buf;
	    GtkWidget *w;
	    buf = g_strdup_printf ("paned%d", i);
	    if((w = glade_xml_get_widget (main_window_xml,  buf)))
	    {
		prefs_set_paned_pos (i,
				     gtk_paned_get_position (GTK_PANED (w)));
	    }
	    g_free (buf);
	}
	/* Elements defined with display.c (sort tab hpaned) */
	for (i=0; i<PANED_NUM_ST; ++i)
	{
	    if (st_paned[i])
		prefs_set_paned_pos (i + PANED_NUM_GLADE,
				     gtk_paned_get_position (st_paned[i]));
	}
    }
}


/* make the tooltips visible or hide it depending on the value set in
 * the prefs (tooltips_main) (called by display_show_hide_tooltips() */
void st_show_hide_tooltips (void)
{
    gint i;

    for (i=0; i<SORT_TAB_MAX; ++i)
    {
	GtkTooltips *tt;
	GtkTooltipsData *ttd;

	g_return_if_fail (sorttab[i]);
	ttd = sorttab[i]->sp_tooltips_data;
	g_return_if_fail (ttd);
	tt = ttd->tooltips;
	g_return_if_fail (tt);

	if (prefs_get_display_tooltips_main ())
	    gtk_tooltips_enable (tt);
	else   gtk_tooltips_disable (tt);
    }
}






/* ---------------------------------------------------------------- */
/*                                                                  */
/*                Section for calendar display                      */
/*                                                                  */
/* ---------------------------------------------------------------- */

/* Strings for 'Category-Combo' */

const gchar *cat_strings[] = {
    N_("Last Played"),
    N_("Last Modified"),
    N_("Added"),
    NULL };

/* enum to access cat_strings */
enum {
    CAT_STRING_PLAYED = 0,
    CAT_STRING_MODIFIED = 1,
    CAT_STRING_ADDED = 2 };

/* typedef to specify lower or upper margin */
typedef enum {
    LOWER_MARGIN,
    UPPER_MARGIN
} MarginType;


/* Set the calendar @calendar, as well as spin buttons @hour and @min
 * according to @mactime. If @mactime is 0, check @no_margin
 * togglebutton, otherwise uncheck it. */
static void cal_set_time_widgets (GtkCalendar *cal,
				  GtkSpinButton *hour,
				  GtkSpinButton *min,
				  GtkToggleButton *no_margin,
				  guint32 mactime)
{
    struct tm *tm;
    time_t tt = time (NULL);

    /* 0, -1 are treated in a special way (no lower/upper margin
     * -> set calendar to current time */
    if ((mactime != 0) && (mactime != -1))
    {
	tt = itdb_time_mac_to_host (mactime);
	if (no_margin)  gtk_toggle_button_set_active (no_margin, FALSE);
    }
    else if (no_margin) gtk_toggle_button_set_active (no_margin, TRUE);

    tm = localtime (&tt);

    if (cal)
    {
	gtk_calendar_select_month (cal, tm->tm_mon, 1900+tm->tm_year);
	gtk_calendar_select_day (cal, tm->tm_mday);
    }

    if (hour)
	gtk_spin_button_set_value (hour, tm->tm_hour);
    if (min)
	gtk_spin_button_set_value (min, tm->tm_min);
}


static void cal_set_time (GtkWidget *cal, MarginType type, guint32 mactime)
{
    GtkCalendar *calendar = NULL;
    GtkSpinButton *hour = NULL;
    GtkSpinButton *min = NULL;
    GtkToggleButton *no_margin = NULL;

    switch (type)
    {
    case LOWER_MARGIN:
	calendar = GTK_CALENDAR (glade_xml_get_widget (cal_xml, "lower_cal"));
	hour = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "lower_hours"));
	min = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "lower_minutes"));
	no_margin = GTK_TOGGLE_BUTTON (glade_xml_get_widget (cal_xml, "no_lower_margin"));
	break;
    case UPPER_MARGIN:
	calendar = GTK_CALENDAR (glade_xml_get_widget (cal_xml, "upper_cal"));
	hour = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "upper_hours"));
	min = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "upper_minutes"));
	no_margin = GTK_TOGGLE_BUTTON (glade_xml_get_widget (cal_xml, "no_upper_margin"));
	break;
    }
    cal_set_time_widgets (calendar, hour, min, no_margin, mactime);
}


/* Extract data from calendar/time.
 *
 * Return value:
 *
 * pointer to 'struct tm' filled with the relevant data or NULL, if
 * the button no_margin was selected.
 *
 * If @tm is != NULL, modify that instead.
 *
 * You must g_free() the retuned value.
 */
static struct tm *cal_get_time (GtkWidget *cal, MarginType type, struct tm *tm)
{
    GtkCalendar *calendar = NULL;
    GtkSpinButton *hour = NULL;
    GtkSpinButton *min = NULL;
    GtkSpinButton *sec = NULL;
    GtkToggleButton *no_margin = NULL;
    GtkToggleButton *no_time = NULL;

    switch (type)
    {
    case LOWER_MARGIN:
	calendar = GTK_CALENDAR (glade_xml_get_widget (cal_xml, "lower_cal"));
	hour = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "lower_hours"));
	min = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "lower_minutes"));
	no_margin = GTK_TOGGLE_BUTTON (glade_xml_get_widget (cal_xml, "no_lower_margin"));
	no_time =  GTK_TOGGLE_BUTTON (glade_xml_get_widget (cal_xml, "lower_time"));
	break;
    case UPPER_MARGIN:
	calendar = GTK_CALENDAR (glade_xml_get_widget (cal_xml, "upper_cal"));
	hour = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "upper_hours"));
	min = GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "upper_minutes"));
	no_margin = GTK_TOGGLE_BUTTON (glade_xml_get_widget (cal_xml, "no_upper_margin"));
	no_time =  GTK_TOGGLE_BUTTON (glade_xml_get_widget (cal_xml, "upper_time"));
	break;
    }

    if (!gtk_toggle_button_get_active (no_margin))
    {
	/* Initialize tm with current time and copy the result of
	 * localtime() to persistent memory that can be g_free()'d */
	time_t tt = time (NULL);
	if (!tm)
	{
	    tm = g_malloc (sizeof (struct tm));
	    memcpy (tm, localtime (&tt), sizeof (struct tm));
	}

	if (calendar)
	{
	    guint year, month, day;
	    gtk_calendar_get_date (calendar, &year, &month, &day);
	    tm->tm_year = year-1900;
	    tm->tm_mon = month;
	    tm->tm_mday = day;
	}
	if (gtk_toggle_button_get_active (no_time))
	{
	    if (hour)
		tm->tm_hour = gtk_spin_button_get_value_as_int (hour);
	    if (min)
		tm->tm_min = gtk_spin_button_get_value_as_int (min);
	    if (sec)
		tm->tm_min = gtk_spin_button_get_value_as_int (sec);
	}
	else
	{   /* use 0:00 for lower and 23:59 for upper margin */
	    switch (type)
	    {
	    case LOWER_MARGIN:
		if (hour) tm->tm_hour = 0;
		if (min) tm->tm_min = 0;
		if (sec) tm->tm_sec = 0;
		break;
	    case UPPER_MARGIN:
		if (hour) tm->tm_hour = 23;
		if (min) tm->tm_min = 59;
		if (sec) tm->tm_sec = 59;
		break;
	    }
	}
    }
    return tm;
}


/* get the category (T_TIME_PLAYED or T_TIME_MODIFIED) selected in the
 * combo */
static T_item cal_get_category (GtkWidget *cal)
{
    const gchar *str;
    GtkWidget *w;
    T_item item;
    gint i = -1;

    w = glade_xml_get_widget (cal_xml, "cat_combo");
    str = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (w)->entry));
    /* Check which string is selected in the combo */
    if (str)
	for (i=0; cat_strings[i]; ++i)
	    if (strcmp (gettext (cat_strings[i]), str) == 0)  break;
    switch (i)
    {
    case CAT_STRING_PLAYED:
	item = T_TIME_PLAYED;
	break;
    case CAT_STRING_MODIFIED:
	item = T_TIME_MODIFIED;
	break;
    case CAT_STRING_ADDED:
	item = T_TIME_ADDED;
	break;
    default:
	fprintf (stderr, "Programming error: cal_get_category () -- item not found.\n");
	/* set to something reasonable at least */
	item = T_TIME_PLAYED;
    }
    return item;
}


/* Returns a string "DD/MM/YYYY HH:MM". Data is taken from
 * @tm. Returns NULL if tm==NULL. You must g_free() the returned
 * string */
static gchar *cal_get_time_string (struct tm *tm)
{
    gchar *str = NULL;

    if (tm)
	str = g_strdup_printf ("%02d/%02d/%04d %d:%02d",
			       tm->tm_mday, tm->tm_mon+1, 1900+tm->tm_year,
			       tm->tm_hour, tm->tm_min);
    return str;
}


/* Extract data from calendar/time and write it to the corresponding
   entry in the specified sort tab */
static void cal_apply_data (GtkWidget *cal)
{
    struct tm *lower, *upper;
    TimeInfo *ti;
    T_item item;
    gint inst;

    lower = cal_get_time (cal, LOWER_MARGIN, NULL);
    upper = cal_get_time (cal, UPPER_MARGIN, NULL);

    /* Get selected instance */
    inst = gtk_spin_button_get_value_as_int (
	GTK_SPIN_BUTTON (glade_xml_get_widget (cal_xml, "sorttab_num_spin"))) - 1;
    /* Get selected category (played, modified or added) */
    item = cal_get_category (cal);
    /* Get pointer to corresponding TimeInfo struct */
    ti = sp_get_timeinfo_ptr (inst, item);

    if (ti)
    {
	GtkToggleButton *act = 	GTK_TOGGLE_BUTTON (ti->active);
	/* is criteria currently checked (active)? */
	gboolean active = gtk_toggle_button_get_active (act);
	gchar *str = NULL;
	gchar *str1 = cal_get_time_string (lower);
	gchar *str2 = cal_get_time_string (upper);

	if (!lower && !upper)
	    if (!active) /* deactivate this criteria */
		gtk_toggle_button_set_active (act, FALSE);
	if (lower && !upper)
	    str = g_strdup_printf ("> %s", str1);
	if (!lower && upper)
	    str = g_strdup_printf ("< %s", str2);
	if (lower && upper)
	    str = g_strdup_printf ("%s < < %s", str1, str2);
	C_FREE (str1);
	C_FREE (str2);

	if (str)
	{   /* set the new string if it's different */
	    if (strcmp (str,
			gtk_entry_get_text (GTK_ENTRY (ti->entry))) != 0)
	    {
		gtk_entry_set_text (GTK_ENTRY (ti->entry), str);
		/* notification that contents have changed */
		g_signal_emit_by_name (ti->entry, "activate");
	    }
	    g_free (str);
	}
	if (!active)
	{   /* activate the criteria */
	    gtk_toggle_button_set_active (act, TRUE);
	}
    }
    g_free (lower);
    g_free (upper);
}



/* Callback for 'Lower/Upper time ' buttons */
static void cal_time_toggled (GtkToggleButton *togglebutton,
			      gpointer         user_data)
{
    gboolean sens = gtk_toggle_button_get_active (togglebutton);

    if ((GtkWidget *)togglebutton == glade_xml_get_widget (cal_xml, "lower_time"))
    {
	gtk_widget_set_sensitive (glade_xml_get_widget (cal_xml, "lower_time_box"), sens);
    }
    if ((GtkWidget *)togglebutton == glade_xml_get_widget (cal_xml, "upper_time"))
    {
	gtk_widget_set_sensitive (glade_xml_get_widget (cal_xml, "upper_time_box"), sens);
    }
}

/* Callback for 'No Lower/Upper Margin' buttons */
static void cal_no_margin_toggled (GtkToggleButton *togglebutton,
				   gpointer         user_data)
{
    gboolean sens = !gtk_toggle_button_get_active (togglebutton);

    if ((GtkWidget *)togglebutton == glade_xml_get_widget (cal_xml, "no_lower_margin"))
    {
	gtk_widget_set_sensitive (glade_xml_get_widget (cal_xml, "lower_cal_box"), sens);
    }
    if ((GtkWidget *)togglebutton == glade_xml_get_widget (cal_xml, "no_upper_margin"))
    {
	gtk_widget_set_sensitive (glade_xml_get_widget (cal_xml, "upper_cal_box"), sens);
    }
}


/* Save the default geometry of the window */
static void cal_save_default_geometry (GtkWindow *cal)
{
    gint x,y;

    gtk_window_get_size (cal, &x, &y);
    prefs_set_size_cal (x, y);
}

/* Callback for 'delete' event */
static gboolean cal_delete_event (GtkWidget       *widget,
				  GdkEvent        *event,
				  gpointer         user_data)
{
    cal_save_default_geometry (GTK_WINDOW (user_data));
    return FALSE;
}

/* Callback for 'Cancel' button */
static void cal_cancel (GtkButton *button, gpointer user_data)
{
    cal_save_default_geometry (GTK_WINDOW (user_data));
    gtk_widget_destroy (user_data);
}

/* Callback for 'Apply' button */
static void cal_apply (GtkButton *button, gpointer user_data)
{
    cal_save_default_geometry (GTK_WINDOW (user_data));
    cal_apply_data (GTK_WIDGET (user_data));
}

/* Callback for 'OK' button */
static void cal_ok (GtkButton *button, gpointer user_data)
{
    cal_apply (button, user_data);
    gtk_widget_destroy (user_data);
}

/* Open a calendar window. Preset the values for instance @inst,
   category @item (time played, time modified or time added) */
void cal_open_calendar (gint inst, T_item item)
{
    SortTab *st;
    GtkWidget *w;
    GtkWidget *cal;
    gchar *str = NULL;
    static GList *catlist = NULL;
    gint defx, defy;
    TimeInfo *ti;

    /* Sanity */
    if (inst >= SORT_TAB_MAX)  return;

    st = sorttab[inst];

    /* Sanity */
    if (!st) return;

    cal_xml = glade_xml_new (xml_file, "calendar_window", NULL);

    glade_xml_signal_autoconnect (cal_xml);

    cal = glade_xml_get_widget (cal_xml, "calendar_window");

    /* Set to saved size */
    prefs_get_size_cal (&defx, &defy);
    gtk_window_set_default_size (GTK_WINDOW (cal), defx, defy);

    /* Set sorttab number */
    w = glade_xml_get_widget (cal_xml, "sorttab_num_spin");
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (w),
			       1, SORT_TAB_MAX);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), inst+1);

    /* Set Category-Combo */
    if (!catlist)
    {   /* create list */
	const gchar **bp = cat_strings;
	while (*bp)   catlist = g_list_append (catlist, gettext (*bp++));
    }
    w = glade_xml_get_widget (cal_xml, "cat_combo");
    gtk_combo_set_popdown_strings (GTK_COMBO (w), catlist);
    switch (item)
    {
    case T_TIME_PLAYED:
	str = gettext (cat_strings[CAT_STRING_PLAYED]);
	break;
    case T_TIME_MODIFIED:
	str = gettext (cat_strings[CAT_STRING_MODIFIED]);
	break;
    case T_TIME_ADDED:
	str = gettext (cat_strings[CAT_STRING_ADDED]);
	break;
    default:
	fprintf (stderr, "Programming error: cal_open_calendar() -- item not found\n");
	break;
    }
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w)->entry), str);

    /* Make sure we use the current contents of the entry */
    sp_store_sp_entries (inst);
    /* set calendar */
    ti = sp_update_date_interval_from_string (inst, item, TRUE);
    /* set the calendar if we have a valid TimeInfo */
    if (ti)
    {
	if (!ti->valid)
	{   /* set to reasonable default */
	    ti->lower = 0;
	    ti->upper = 0;
	}

	/* Lower Margin */
	w = glade_xml_get_widget (cal_xml, "no_lower_margin");
	g_signal_connect (w,
			  "toggled",
			  G_CALLBACK (cal_no_margin_toggled),
			  cal);
	w = glade_xml_get_widget (cal_xml, "lower_time");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
	g_signal_connect (w,
			  "toggled",
			  G_CALLBACK (cal_time_toggled),
			  cal);

	cal_set_time (cal, LOWER_MARGIN, ti->lower);

	/* Upper Margin */
	w = glade_xml_get_widget (cal_xml, "no_upper_margin");
	g_signal_connect (w,
			  "toggled",
			  G_CALLBACK (cal_no_margin_toggled),
			  cal);
	w = glade_xml_get_widget (cal_xml, "upper_time");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
	g_signal_connect (w,
			  "toggled",
			  G_CALLBACK (cal_time_toggled),
			  cal);
	cal_set_time (cal, UPPER_MARGIN, ti->upper);
    }

    /* Connect delete-event */
    g_signal_connect (cal, "delete_event",
		      G_CALLBACK (cal_delete_event), cal);
    /* Connect cancel-button */
    g_signal_connect (glade_xml_get_widget (cal_xml, "cal_cancel"), "clicked",
		      G_CALLBACK (cal_cancel), cal);
    /* Connect apply-button */
    g_signal_connect (glade_xml_get_widget (cal_xml, "cal_apply"), "clicked",
		      G_CALLBACK (cal_apply), cal);
    /* Connect ok-button */
    g_signal_connect (glade_xml_get_widget (cal_xml, "cal_ok"), "clicked",
		      G_CALLBACK (cal_ok), cal);

    gtk_widget_show (cal);
}
