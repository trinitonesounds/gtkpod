/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

/* This file provides functions for the details window */

#include <stdlib.h>
#include <gtk/gtk.h>
#include "details.h"
#include "fileselection.h"
#include "misc.h"
#include "fetchcover.h"
#include "display_coverart.h"
#include "misc_track.h"
#include "prefs.h"
#include <string.h>
#include <glib/gstdio.h>

/*
void details_close (void);
void details_update_default_sizes (void);
void details_update_track (Track *track);
void details_remove_track (Track *track);
*/

/* List with all detail windows */
static GList *details = NULL;

/* string constants for preferences */
static const gchar *DETAILS_WINDOW_DEFX="details_window_defx";
static const gchar *DETAILS_WINDOW_DEFY="details_window_defy";
static const gchar *DETAILS_WINDOW_NOTEBOOK_PAGE="details_window_notebook_page";

/* enum types */
typedef enum
{
    DETAILS_MEDIATYPE_AUDIO_VIDEO = 0,
    DETAILS_MEDIATYPE_AUDIO,
    DETAILS_MEDIATYPE_MOVIE,
    DETAILS_MEDIATYPE_PODCAST,
    DETAILS_MEDIATYPE_VIDEO_PODCAST,
    DETAILS_MEDIATYPE_AUDIOBOOK,
    DETAILS_MEDIATYPE_MUSICVIDEO,
    DETAILS_MEDIATYPE_TVSHOW,
    DETAILS_MEDIATYPE_MUSICVIDEO_TVSHOW
} DETAILS_MEDIATYPE;

typedef struct
{
    guint32 id;
    const gchar *str;
} ComboEntry;

/* strings for mediatype combobox */
static const ComboEntry mediatype_comboentries[] =
{
    { 0,                                               N_("Audio/Video") },
    { ITDB_MEDIATYPE_AUDIO,                            N_("Audio") },
    { ITDB_MEDIATYPE_MOVIE,                            N_("Video") },
    { ITDB_MEDIATYPE_PODCAST,                          N_("Podcast") },
    { ITDB_MEDIATYPE_PODCAST|ITDB_MEDIATYPE_MOVIE,     N_("Video Podcast") },
    { ITDB_MEDIATYPE_AUDIOBOOK,                        N_("Audiobook") },
    { ITDB_MEDIATYPE_MUSICVIDEO,                       N_("Music Video") },
    { ITDB_MEDIATYPE_TVSHOW,                           N_("TV Show") },
    { ITDB_MEDIATYPE_TVSHOW|ITDB_MEDIATYPE_MUSICVIDEO, N_("TV Show & Music Video") },
    { 0,                                               NULL }
};

/* Declarations */
static void details_set_track (Detail *detail, Track *track);
static void details_free (Detail *detail);
static void details_get_item (Detail *detail, T_item item,
			      gboolean assumechanged);
static void details_get_changes (Detail *detail);
static gboolean details_copy_artwork (Track *frtrack, Track *totrack);
static void details_undo_track (Detail *detail, Track *track);
static void details_update_headline (Detail *detail);



/* Store the window size */
static void details_store_window_state (Detail *detail)
{
    GtkWidget *w;
    gint defx, defy;

    g_return_if_fail (detail);

    gtk_window_get_size (GTK_WINDOW (detail->window), &defx, &defy);
    prefs_set_int (DETAILS_WINDOW_DEFX, defx);
    prefs_set_int (DETAILS_WINDOW_DEFY, defy);

    if ((w = gtkpod_xml_get_widget (detail->xml, "details_notebook")))
    {
	gint page = gtk_notebook_get_current_page (GTK_NOTEBOOK (w));
	prefs_set_int (DETAILS_WINDOW_NOTEBOOK_PAGE, page);
    }
}


/* Query the state of the writethrough checkbox */
gboolean details_writethrough (Detail *detail)
{
    GtkWidget *w;

    g_return_val_if_fail (detail, FALSE);

    w = gtkpod_xml_get_widget (detail->xml,
			       "details_checkbutton_writethrough");
    return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
}

/* ------------------------------------------------------------
 *
 *        Callback functions
 *
 * ------------------------------------------------------------ */

static void details_text_changed (GtkWidget *widget,
				  Detail *detail)
{
    ExtraTrackData *etr;

    g_return_if_fail (detail);
    g_return_if_fail (detail->track);
    etr = detail->track->userdata;
    g_return_if_fail (etr);

    detail->changed = TRUE;
    etr->tchanged = TRUE;
    details_update_buttons (detail);
}


static void details_entry_activate (GtkEntry *entry,
				    Detail *detail)
{
    T_item item;

    g_return_if_fail (entry);

    item = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry),
					       "details_item"));

    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    details_get_item (detail, item, TRUE);

    details_update_headline (detail);
}


static void details_checkbutton_toggled (GtkCheckButton *button,
					 Detail *detail)
{
    T_item item;

    g_return_if_fail (button);

    item = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
					       "details_item"));
    
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    details_get_item (detail, item, FALSE);
}


static void details_combobox_changed (GtkComboBox *combobox,
					 Detail *detail)
{
    T_item item;

    g_return_if_fail (combobox);

    item = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combobox),
					       "details_item"));
    
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    details_get_item (detail, item, FALSE);
}


static void details_writethrough_toggled (GtkCheckButton *button,
					  Detail *detail)
{
    details_update_buttons (detail);
}


/****** Navigation *****/
void details_button_first_clicked (GtkCheckButton *button,
				   Detail *detail)
{
    GList *first;
    g_return_if_fail (detail);

    first = g_list_first (detail->tracks);

    details_get_changes (detail);

    if (first)
	details_set_track (detail, first->data);
}

void details_button_previous_clicked (GtkCheckButton *button,
				      Detail *detail)
{
    gint i;

    g_return_if_fail (detail);

    details_get_changes (detail);

    i = g_list_index (detail->tracks, detail->track);

    if (i > 0)
    {
	details_set_track (detail, g_list_nth_data (detail->tracks, i-1));
    }
}

void details_button_next_clicked (GtkCheckButton *button,
				  Detail *detail)
{
    GList *gl;
    g_return_if_fail (detail);

    details_get_changes (detail);

    gl = g_list_find (detail->tracks, detail->track);

    g_return_if_fail (gl);

    if (gl->next)
	details_set_track (detail, gl->next->data);
}

void details_button_last_clicked (GtkCheckButton *button,
				  Detail *detail)
{
    GList *last;
    g_return_if_fail (detail);

    last = g_list_last (detail->tracks);

    details_get_changes (detail);

    if (last)
	details_set_track (detail, last->data);
}


/****** Thumbnail Control *****/
static void details_button_set_artwork_clicked (GtkButton *button,
						Detail *detail)
{
    gchar *filename;

    g_return_if_fail (detail);
    g_return_if_fail (detail->track);

    filename = fileselection_get_cover_filename ();

    if (filename)
    {
	if (details_writethrough (detail))
	{   /* Set thumbnail for all tracks */
	    GList *gl;
	    for (gl=detail->tracks; gl; gl=gl->next)
	    {
		ExtraTrackData *etr;
		Track *tr = gl->data;
		g_return_if_fail (tr);
		etr = tr->userdata;
		g_return_if_fail (etr);
		gp_track_set_thumbnails (tr, filename);
		etr->tchanged = TRUE;
		etr->tartwork_changed = TRUE;
	    }
	}
	else
	{   /* Only change current track */
	    ExtraTrackData *etr = detail->track->userdata;
	    g_return_if_fail (etr);
	    gp_track_set_thumbnails (detail->track, filename);
	    etr->tchanged = TRUE;
	    etr->tartwork_changed = TRUE;
	}
	detail->changed = TRUE;
	details_update_thumbnail (detail);
    }
    g_free (filename);

    details_update_buttons (detail);
}

static void details_button_remove_artwork_clicked (GtkButton *button,
						   Detail *detail)
{
    g_return_if_fail (detail);
    g_return_if_fail (detail->track);

    if (details_writethrough (detail))
    {   /* Remove thumbnail on all tracks */
	GList *gl;
	for (gl=detail->tracks; gl; gl=gl->next)
	{
	    ExtraTrackData *etr;
	    Track *tr = gl->data;
	    g_return_if_fail (tr);
	    etr = tr->userdata;
	    g_return_if_fail (etr);

	    etr->tchanged |= gp_track_remove_thumbnails (tr);
	    detail->changed |= etr->tchanged;
	}
    }
    else
    {   /* Only change current track */
	ExtraTrackData *etr = detail->track->userdata;
	g_return_if_fail (etr);
	etr->tchanged |= gp_track_remove_thumbnails (detail->track);
	detail->changed |= etr->tchanged;
    }

    details_update_thumbnail (detail);

    details_update_buttons (detail);
}


/****** Window Control *****/
static void details_button_apply_clicked (GtkButton *button,
					  Detail *detail)
{
    GList *gl, *gl_orig;
    gboolean changed = FALSE;
    GList *changed_tracks = NULL;

    g_return_if_fail (detail);

    details_get_changes (detail);

    for (gl=detail->tracks, gl_orig=detail->orig_tracks;
	 gl && gl_orig;
	 gl=gl->next, gl_orig=gl_orig->next)
    {
	Track *tr = gl->data;
	Track *tr_orig = gl_orig->data;
	ExtraTrackData *etr;
	g_return_if_fail (tr);
	g_return_if_fail (tr_orig);

	etr = tr->userdata;
	g_return_if_fail (etr);

	if (etr->tchanged)
	{
	    T_item item;
	    gboolean tr_changed = FALSE;

	    for (item=1; item<T_ITEM_NUM; ++item)
	    {
		tr_changed |= track_copy_item (tr, tr_orig, item);
	    }

	    tr_changed |= details_copy_artwork (tr, tr_orig);

	    if (tr_changed)
	    {
		tr_orig->time_modified = time (NULL);
		pm_track_changed (tr_orig);
	    }

	    if (prefs_get_int("id3_write"))
	    {
		/* add tracks to a list because write_tags_to_file()
		   can remove newly created duplicates which is not a
		   good idea from within a for() loop over tracks */
		changed_tracks = g_list_prepend (changed_tracks, tr_orig);
	    }

	    changed |= tr_changed;
	    etr->tchanged = FALSE;
	}
    }

    detail->changed = FALSE;

    if (changed)
    {
	data_changed (detail->itdb);
    }
    
    if (prefs_get_int("id3_write"))
    {
	if (changed_tracks)
	{
	    for (gl=changed_tracks; gl; gl=gl->next)
	    {
		Track *tr = gl->data;
		write_tags_to_file (tr);
		/* display possible duplicates that have been removed */
	    }
	    gp_duplicate_remove (NULL, NULL);
	}
    }
    g_list_free (changed_tracks);

    details_update_headline (detail);

    details_update_buttons (detail);
}

static void details_button_cancel_clicked (GtkButton *button,
					   Detail *detail)
{
    g_return_if_fail (detail);

    details_store_window_state (detail);

    details = g_list_remove (details, detail);

    details_free (detail);
}


static void details_button_ok_clicked (GtkButton *button,
				       Detail *detail)
{
    g_return_if_fail (detail);

    details_button_apply_clicked (NULL, detail);
    details_button_cancel_clicked (NULL, detail);
}


/* Check if any tracks are still modified and set detail->changed
   accordingly */
static void details_update_changed_state (Detail *detail)
{
    gboolean changed = FALSE;
    GList *gl;

    g_return_if_fail (detail);

    for (gl=detail->tracks; gl; gl=gl->next)
    {
	ExtraTrackData *etr;
	Track *track = gl->data;
	g_return_if_fail (track);
	etr = track->userdata;
	g_return_if_fail (etr);
	changed |= etr->tchanged;
    }

    detail->changed = changed;
}


static void details_button_undo_track_clicked (GtkButton *button,
					       Detail *detail)
{
    g_return_if_fail (detail);

    details_undo_track (detail, detail->track);

    details_update_changed_state (detail);

    details_set_track (detail, detail->track);
}


static void details_button_undo_all_clicked (GtkButton *button,
					     Detail *detail)
{
    GList *gl;

    g_return_if_fail (detail);

    for (gl=detail->tracks; gl; gl=gl->next)
    {
	Track *track = gl->data;
	g_return_if_fail (track);

	details_undo_track (detail, track);
    }

    detail->changed = FALSE;

    details_set_track (detail, detail->track);
}


static void details_delete_event (GtkWidget *widget,
				  GdkEvent *event,
				  Detail *detail)
{
    details_button_cancel_clicked (NULL, detail);
}


/****** Copy artwork data if filename has changed ****** */
static gboolean details_copy_artwork (Track *frtrack, Track *totrack)
{
		gboolean changed = FALSE;
  	ExtraTrackData *fretr, *toetr;

  	g_return_val_if_fail (frtrack, FALSE);
  	g_return_val_if_fail (totrack, FALSE);

    fretr = frtrack->userdata;
    toetr = totrack->userdata;

    g_return_val_if_fail (fretr, FALSE);
    g_return_val_if_fail (toetr, FALSE);

    g_return_val_if_fail (fretr->thumb_path_locale, FALSE);
    g_return_val_if_fail (toetr->thumb_path_locale, FALSE);
	
	if (strcmp (fretr->thumb_path_locale, toetr->thumb_path_locale) != 0
			|| fretr->tartwork_changed == TRUE)
  {
		itdb_artwork_free (totrack->artwork);
		totrack->artwork = itdb_artwork_duplicate (frtrack->artwork);
		totrack->artwork_size = frtrack->artwork_size;
		totrack->artwork_count = frtrack->artwork_count;
		totrack->has_artwork = frtrack->has_artwork;
		g_free (toetr->thumb_path_locale);
		g_free (toetr->thumb_path_utf8);
		toetr->thumb_path_locale = g_strdup (fretr->thumb_path_locale);
		toetr->thumb_path_utf8 = g_strdup (fretr->thumb_path_utf8);
		changed = TRUE;
	}
    /* make sure artwork gets removed, even if both thumb_paths were
       unset ("") */
    if (!frtrack->artwork->thumbnails)
    {
	changed |= gp_track_remove_thumbnails (totrack);
    }
    
    /* Since no data changes affect the coverart display.
     * Need to force a change by calling set covers directly.
     */
     force_update_covers ();
    
    return changed;
}



/****** Undo one track (no display action) ****** */
static void details_undo_track (Detail *detail, Track *track)
{
    gint i;
    T_item item;
    Track *tr_orig;
    ExtraTrackData *etr;

    g_return_if_fail (detail);
    g_return_if_fail (track);

    etr = track->userdata;
    g_return_if_fail (etr);

    i = g_list_index (detail->tracks, track);
    g_return_if_fail (i != -1);

    tr_orig = g_list_nth_data (detail->orig_tracks, i);
    g_return_if_fail (tr_orig);

    for (item=1; item<T_ITEM_NUM; ++item)
    {
	track_copy_item (tr_orig, track, item);
    }

    details_copy_artwork (tr_orig, track);

    etr->tchanged = FALSE;
}


/****** Read out widgets of current track ******/
static void details_get_changes (Detail *detail)
{
    T_item item;

    g_return_if_fail (detail);
    g_return_if_fail (detail->track);

    for (item=1; item<T_ITEM_NUM; ++item)
    {
	details_get_item (detail, item, FALSE);
    }
}


/****** comboentries helper functions ******/

/* Get index from ID (returns -1 if ID could not be found) */
static gint comboentry_index_from_id (const ComboEntry centries[],
				      guint32 id)
{
    gint i;

    g_return_val_if_fail (centries, -1);

    for (i=0; centries[i].str; ++i)
    {
	if (centries[i].id == id)  return i;
    }
    return -1;
}


/* initialize a combobox with the corresponding entry strings */
static void details_setup_combobox (GtkWidget *cb,
				    const ComboEntry centries[])
{
    const ComboEntry *ce = centries;
    GtkCellRenderer *cell;
    GtkListStore *store;

    g_return_if_fail (cb);
    g_return_if_fail (centries);

    /* clear any renderers that may have been set */
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (cb));
    /* set new model */
    store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_combo_box_set_model (GTK_COMBO_BOX (cb), GTK_TREE_MODEL (store));
    g_object_unref (store);

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), cell,
				    "text", 0,
				    NULL);

    while (ce->str != NULL)
    {
	gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _(ce->str));
	++ce;
    }
}


/****** Setup of widgets ******/
static void details_setup_widget (Detail *detail, T_item item)
{
    GtkTextBuffer *tb;
    GtkWidget *w;
    gchar *buf;

    g_return_if_fail (detail);
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    /* Setup label */
    switch (item)
    {
    case T_COMPILATION:
    case T_CHECKED:
    case T_REMEMBER_PLAYBACK_POSITION:
    case T_SKIP_WHEN_SHUFFLING:
	buf = g_strdup_printf ("details_checkbutton_%d", item);
	w = gtkpod_xml_get_widget (detail->xml, buf);
	gtk_button_set_label (GTK_BUTTON (w),
			      gettext (get_t_string (item)));
	g_free (buf);
	break;
    default:
	buf = g_strdup_printf ("details_label_%d", item);
	w = gtkpod_xml_get_widget (detail->xml, buf);
	gtk_label_set_text (GTK_LABEL (w), gettext (get_t_string (item)));
	g_free (buf);
    }

    buf = NULL;
    w = NULL;

    switch (item)
    {
    case T_ALBUM:
    case T_ARTIST:
    case T_TITLE:
    case T_GENRE:
    case T_COMPOSER:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_PC_PATH:
    case T_IPOD_PATH:
    case T_THUMB_PATH:
    case T_IPOD_ID:
    case T_SIZE:
    case T_TRACKLEN:
    case T_STARTTIME:
    case T_STOPTIME:
    case T_BITRATE:
    case T_SAMPLERATE:
    case T_PLAYCOUNT:
    case T_BPM:
    case T_RATING:
    case T_VOLUME:
    case T_SOUNDCHECK:
    case T_CD_NR:
    case T_TRACK_NR:
    case T_YEAR:
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
    case T_TV_SHOW:
    case T_TV_EPISODE:
    case T_TV_NETWORK:
    case T_SEASON_NR:
    case T_EPISODE_NR:
    case T_ALBUMARTIST:
    case T_SORT_ARTIST:
    case T_SORT_TITLE:
    case T_SORT_ALBUM:
    case T_SORT_ALBUMARTIST:
    case T_SORT_COMPOSER:
    case T_SORT_TVSHOW:
	buf = g_strdup_printf ("details_entry_%d", item);
	w = gtkpod_xml_get_widget (detail->xml, buf);
	g_signal_connect (w, "activate",
			  G_CALLBACK (details_entry_activate),
			  detail);
	g_signal_connect (w, "changed",
			  G_CALLBACK (details_text_changed),
			  detail);
	break;
    case T_COMPILATION:
    case T_TRANSFERRED:
    case T_CHECKED:
    case T_REMEMBER_PLAYBACK_POSITION:
    case T_SKIP_WHEN_SHUFFLING:
	buf = g_strdup_printf ("details_checkbutton_%d", item);
	w = gtkpod_xml_get_widget (detail->xml, buf);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (details_checkbutton_toggled),
			  detail);
	break;
    case T_DESCRIPTION:
    case T_SUBTITLE:
    case T_COMMENT:
	buf = g_strdup_printf ("details_textview_%d", item);
	w = gtkpod_xml_get_widget (detail->xml, buf);
	tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
	g_signal_connect (tb, "changed",
			  G_CALLBACK (details_text_changed),
			  detail);
	break;
    case T_MEDIA_TYPE:
	buf = g_strdup_printf ("details_combobox_%d", item);
	w = gtkpod_xml_get_widget (detail->xml, buf);
	details_setup_combobox (w, mediatype_comboentries);
	g_signal_connect (w, "changed",
			  G_CALLBACK (details_combobox_changed),
			  detail);
	break;
    case T_ALL:
    case T_ITEM_NUM:
	/* cannot happen because of assertion above */
	g_return_if_reached ();
    }

    if (w)
    {
	g_object_set_data (G_OBJECT (w),
			   "details_item", GINT_TO_POINTER (item));
    }

    g_free (buf);
}


static void details_set_item (Detail *detail, Track *track, T_item item)
{
    GtkTextBuffer *tb;
    GtkWidget *w = NULL;
    gchar *text;
    gchar *entry, *checkbutton, *textview, *combobox;

    g_return_if_fail (detail);
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    entry = g_strdup_printf ("details_entry_%d", item);
    checkbutton = g_strdup_printf ("details_checkbutton_%d", item);
    textview = g_strdup_printf ("details_textview_%d", item);
    combobox = g_strdup_printf ("details_combobox_%d", item);

    if (track != NULL)
    {
	track->itdb = detail->itdb;
	text = track_get_text (track, item);
	track->itdb = NULL;
	if ((item == T_THUMB_PATH) && (!detail->artwork_ok))
	{
	    gchar *new_text = g_strdup_printf (_("%s (image data corrupted or unreadable)"), text);
	    g_free (text);
	    text = new_text;
	}
    }
    else
    {
	text = g_strdup ("");
    }

    switch (item)
    {
    case T_ALBUM:
    case T_ARTIST:
    case T_TITLE:
    case T_GENRE:
    case T_COMPOSER:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_PC_PATH:
    case T_IPOD_PATH:
    case T_THUMB_PATH:
    case T_IPOD_ID:
    case T_SIZE:
    case T_TRACKLEN:
    case T_STARTTIME:
    case T_STOPTIME:
    case T_BITRATE:
    case T_SAMPLERATE:
    case T_PLAYCOUNT:
    case T_BPM:
    case T_RATING:
    case T_VOLUME:
    case T_SOUNDCHECK:
    case T_CD_NR:
    case T_TRACK_NR:
    case T_YEAR:
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
    case T_TV_SHOW:
    case T_TV_EPISODE:
    case T_TV_NETWORK:
    case T_SEASON_NR:
    case T_EPISODE_NR:
    case T_ALBUMARTIST:
    case T_SORT_ARTIST:
    case T_SORT_TITLE:
    case T_SORT_ALBUM:
    case T_SORT_ALBUMARTIST:
    case T_SORT_COMPOSER:
    case T_SORT_TVSHOW:
	w = gtkpod_xml_get_widget (detail->xml, entry);
	g_signal_handlers_block_by_func (w, details_text_changed, detail);
	gtk_entry_set_text (GTK_ENTRY (w), text);
	g_signal_handlers_unblock_by_func(w, details_text_changed,detail);
	break;
    case T_COMMENT:
    case T_DESCRIPTION:
    case T_SUBTITLE:
	w = gtkpod_xml_get_widget (detail->xml, textview);
	tb= gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
	g_signal_handlers_block_by_func (tb,
					 details_text_changed, detail);
	gtk_text_buffer_set_text (tb, text, -1);
	g_signal_handlers_unblock_by_func (tb,
					   details_text_changed, detail);
	break;
    case T_COMPILATION:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    if (track)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      track->compilation);
	    else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      FALSE);
	}
	break;
    case T_TRANSFERRED:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    if (track)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      track->transferred);
	    else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      FALSE);
	}
	break;
    case T_REMEMBER_PLAYBACK_POSITION:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    if (track)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      track->remember_playback_position);
	    else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      FALSE);
	}
	break;
    case T_SKIP_WHEN_SHUFFLING:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    if (track)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      track->skip_when_shuffling);
	    else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      FALSE);
	}
	break;
    case T_CHECKED:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    if (track)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      !track->checked);
	    else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
					      FALSE);
	}
	break;
    case T_MEDIA_TYPE:
	if ((w = gtkpod_xml_get_widget (detail->xml, combobox)))
	{
	    gint index = -1;
	    if (track)
	    {
		index = comboentry_index_from_id (mediatype_comboentries,
						  track->mediatype);
		if (index == -1)
		{
		    gtkpod_warning (_("Please report unknown mediatype %x\n"),
				    track->mediatype);
		}
	    }
	    gtk_combo_box_set_active (GTK_COMBO_BOX (w), index);
	}
	break;
    case T_ALL:
    case T_ITEM_NUM:
	/* cannot happen because of assertion above */
	g_return_if_reached ();
    }

    g_free (entry);
    g_free (checkbutton);
    g_free (textview);
    g_free (combobox);
    g_free (text);
}



/* assumechanged: normally the other tracks are only changed if a
 * change has been done. assumechanged==TRUE will write the
 * the value to all other tracks even if no change has taken place
 * (e.g. when ENTER is pressed in a text field) */
static void details_get_item (Detail *detail, T_item item,
			      gboolean assumechanged)
{
    GtkWidget *w = NULL;
    gchar *entry, *checkbutton, *textview, *combobox;
    gboolean changed = FALSE;
    ExtraTrackData *etr;
    Track *track;

    g_return_if_fail (detail);
    track = detail->track;
    g_return_if_fail (track);
    g_return_if_fail ((item > 0) && (item < T_ITEM_NUM));

    etr = track->userdata;
    g_return_if_fail (etr);

    entry = g_strdup_printf ("details_entry_%d", item);
    checkbutton = g_strdup_printf ("details_checkbutton_%d", item);
    textview = g_strdup_printf ("details_textview_%d", item);
    combobox = g_strdup_printf ("details_combobox_%d", item);

    switch (item)
    {
    case T_ALBUM:
    case T_ARTIST:
    case T_TITLE:
    case T_GENRE:
    case T_COMPOSER:
    case T_FILETYPE:
    case T_GROUPING:
    case T_CATEGORY:
    case T_PODCASTURL:
    case T_PODCASTRSS:
    case T_SIZE:
    case T_BITRATE:
    case T_SAMPLERATE:
    case T_PLAYCOUNT:
    case T_BPM:
    case T_RATING:
    case T_VOLUME:
    case T_CD_NR:
    case T_TRACK_NR:
    case T_YEAR:
    case T_TIME_ADDED:
    case T_TIME_PLAYED:
    case T_TIME_MODIFIED:
    case T_TIME_RELEASED:
    case T_TRACKLEN:
    case T_STARTTIME:
    case T_STOPTIME:
    case T_SOUNDCHECK:
    case T_TV_SHOW:
    case T_TV_EPISODE:
    case T_TV_NETWORK:
    case T_SEASON_NR:
    case T_EPISODE_NR:
    case T_ALBUMARTIST:
    case T_SORT_ARTIST:
    case T_SORT_TITLE:
    case T_SORT_ALBUM:
    case T_SORT_ALBUMARTIST:
    case T_SORT_COMPOSER:
    case T_SORT_TVSHOW:
	if ((w = gtkpod_xml_get_widget (detail->xml, entry)))
	{
	    const gchar *text;

	    text = gtk_entry_get_text (GTK_ENTRY (w));

	    /* for soundcheck the displayed value is only a rounded
	       figure -> unless 'assumechanged' is set, compare the
	       string to the original one before assuming a change
	       took place */
	    if (!assumechanged &&
		(item == T_SOUNDCHECK))
	    {
		gchar *buf;
		track->itdb = detail->itdb;
		buf = track_get_text (track, item);
		track->itdb = NULL;
		g_return_if_fail (buf);
		if (strcmp (text, buf) != 0)
		    changed = track_set_text (track, text, item);
		g_free (buf);
	    }
	    else
	    {
		changed = track_set_text (track, text, item);
	    }
	    /* redisplay some items to be on the safe side */
	    switch (item)
	    {
	    case T_TRACK_NR:
	    case T_CD_NR:
	    case T_TRACKLEN:
	    case T_STARTTIME:
	    case T_STOPTIME:
	    case T_TIME_ADDED:
	    case T_TIME_PLAYED:
	    case T_TIME_MODIFIED:
	    case T_TIME_RELEASED:
		details_set_item (detail, track, item);
		break;
	    default:
		break;
	    }
	}
	break;
    case T_COMMENT:
    case T_DESCRIPTION:
    case T_SUBTITLE:
	if ((w = gtkpod_xml_get_widget (detail->xml, textview)))
	{
	    gchar *text;
	    GtkTextIter start, end;
	    GtkTextBuffer *tb = gtk_text_view_get_buffer (
		GTK_TEXT_VIEW (w));
	    gtk_text_buffer_get_start_iter  (tb, &start);
	    gtk_text_buffer_get_end_iter  (tb, &end);

	    text = gtk_text_buffer_get_text (tb, &start, &end, TRUE);

	    changed = track_set_text (track, text, item);
	}
	break;
    case T_COMPILATION:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gboolean state;
	    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	    if (track->compilation != state)
	    {
		track->compilation = state;
		changed = TRUE;
	    }
	}
	break;
    case T_REMEMBER_PLAYBACK_POSITION:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gboolean state;
	    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	    if (track->remember_playback_position != state)
	    {
		track->remember_playback_position = state;
		changed = TRUE;
	    }
	}
	break;
    case T_SKIP_WHEN_SHUFFLING:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gboolean state;
	    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

	    if (track->skip_when_shuffling != state)
	    {
		track->skip_when_shuffling = state;
		changed = TRUE;
	    }
	}
	break;
    case T_CHECKED:
	if ((w = gtkpod_xml_get_widget (detail->xml, checkbutton)))
	{
	    gboolean state;
	    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	    if ((state && (track->checked == 1)) ||
		(!state && (track->checked == 0)))
	    {
		changed = TRUE;
		if (state)  track->checked = 0;
		else        track->checked = 1;
	    }
	}
	break;
    case T_MEDIA_TYPE:
	if ((w = gtkpod_xml_get_widget (detail->xml, combobox)))
	{
	    gint active;
	    active = gtk_combo_box_get_active (GTK_COMBO_BOX (w));
	    if (active != -1)
	    {
		guint32 new_mediatype = mediatype_comboentries[active].id;

		if (track->mediatype != new_mediatype)
		{
		    track->mediatype = new_mediatype;
		    changed = TRUE;
		}
	    }
	}
	break;
    case T_TRANSFERRED:
    case T_PC_PATH:
    case T_IPOD_PATH:
    case T_IPOD_ID:
    case T_THUMB_PATH:
	/* These are read-only only */
	break;
	break;
    case T_ALL:
    case T_ITEM_NUM:
	/* cannot happen because of assertion above */
	g_return_if_reached ();
    }

    etr->tchanged |= changed;
    detail->changed |= changed;

/*     if (changed)  printf ("changed (%d)\n", item); */

    /* Check if this has to be copied to the other tracks as well
       (writethrough) */
    if ((changed || assumechanged) &&
	details_writethrough (detail))
    {   /* change for all tracks */
	GList *gl;
	for (gl=detail->tracks; gl; gl=gl->next)
	{
	    Track *gltr = gl->data;
	    g_return_if_fail (gltr);

	    if (gltr != track)
	    {
		ExtraTrackData *gletr= gltr->userdata;
		g_return_if_fail (gletr);

		gletr->tchanged |= track_copy_item (track, gltr, item);
		detail->changed |= gletr->tchanged;
	    }
	}
    }

    g_free (entry);
    g_free (checkbutton);
    g_free (textview);
    g_free (combobox);

    details_update_buttons (detail);
}


/* Render the Apply button insensitive as long as no changes were done */
void details_update_buttons (Detail *detail)
{
    GtkWidget *w;
    gchar *buf;
    ExtraTrackData *etr;
    gboolean apply, undo_track, undo_all, remove_artwork, viewport;
    gboolean prev, next, ok;

    g_return_if_fail (detail);

    if (detail->track)
    {
	gint i;
	etr = detail->track->userdata;
	g_return_if_fail (etr);

	details_update_changed_state (detail);

	apply = detail->changed;
	undo_track = etr->tchanged;
	undo_all = detail->changed;
	ok = TRUE;
	viewport = TRUE;
	if (details_writethrough (detail))
	{
	    GList *gl;
	    remove_artwork = FALSE;
	    for (gl=detail->tracks; gl && !remove_artwork; gl=gl->next)
	    {
		Track *tr = gl->data;
		g_return_if_fail (tr);
		remove_artwork |= (tr->artwork->thumbnails != NULL);
	    }
	}
	else
	{
	    remove_artwork = (detail->track->artwork->thumbnails != NULL);
	}
	i = g_list_index (detail->tracks, detail->track);
	g_return_if_fail (i != -1);
	if (i == 0)  prev = FALSE;
	else         prev = TRUE;
	if (i == (g_list_length (detail->tracks)-1))  next = FALSE;
	else         next = TRUE;

    }
    else
    {
	apply = FALSE;
	undo_track = FALSE;
	undo_all = FALSE;
	ok = FALSE;
	viewport = FALSE;
	remove_artwork = FALSE;
	prev = FALSE;
	next = FALSE;
    }

    w = gtkpod_xml_get_widget (detail->xml, "details_button_apply");
    gtk_widget_set_sensitive (w, apply);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_undo_track");
    gtk_widget_set_sensitive (w, undo_track);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_undo_all");
    gtk_widget_set_sensitive (w, undo_all);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_ok");
    gtk_widget_set_sensitive (w, ok);
    w = gtkpod_xml_get_widget (detail->xml,
			       "details_button_remove_artwork");
    gtk_widget_set_sensitive (w, remove_artwork);
    w = gtkpod_xml_get_widget (detail->xml, "details_viewport");
    gtk_widget_set_sensitive (w, viewport);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_first");
    gtk_widget_set_sensitive (w, prev);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_previous");
    gtk_widget_set_sensitive (w, prev);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_next");
    gtk_widget_set_sensitive (w, next);
    w = gtkpod_xml_get_widget (detail->xml, "details_button_last");
    gtk_widget_set_sensitive (w, next);

    if (detail->track)
    {
	buf = g_strdup_printf (
	    "%d / %d",
	    g_list_index (detail->tracks, detail->track) + 1,
	    g_list_length (detail->tracks));
    }
    else
    {
	buf = g_strdup (_("n/a"));
    }
    w = gtkpod_xml_get_widget (detail->xml, "details_label_index");
    gtk_label_set_text (GTK_LABEL (w), buf);
    g_free (buf);
}

/* Update the displayed thumbnail */
void details_update_thumbnail (Detail *detail)
{
    Thumb *thumb;
    GtkImage *img;

    g_return_if_fail (detail);

    img = GTK_IMAGE (gtkpod_xml_get_widget (detail->xml,
					    "details_image_thumbnail"));

    gtk_image_set_from_pixbuf (img, NULL);

    if (detail->track)
    {
	detail->artwork_ok = TRUE;
	/* Get large cover */
	thumb = itdb_artwork_get_thumb_by_type (detail->track->artwork,
						ITDB_THUMB_COVER_LARGE);
	if (thumb)
	{
	    GdkPixbuf *pixbuf;
	    pixbuf = itdb_thumb_get_gdk_pixbuf (detail->itdb->device,
						thumb);
	    if (pixbuf)
	    {
		gtk_image_set_from_pixbuf (img, pixbuf);
		gdk_pixbuf_unref (pixbuf);
	    }
	    else
	    {
		gtk_image_set_from_stock (img, GTK_STOCK_DIALOG_WARNING,
					  GTK_ICON_SIZE_DIALOG);
		detail->artwork_ok = FALSE;
	    }
	}
	details_set_item (detail,  detail->track, T_THUMB_PATH);
    }

    if (gtk_image_get_storage_type (img) == GTK_IMAGE_EMPTY)
    {
	gtk_image_set_from_stock (img, GTK_STOCK_MISSING_IMAGE,
				  GTK_ICON_SIZE_DIALOG);
    }
}

static void details_update_headline (Detail *detail)
{
    GtkWidget *w;
    gchar *buf;

    g_return_if_fail (detail);

    /* Set Artist/Title label */
    w = gtkpod_xml_get_widget (detail->xml, "details_label_artist_title");

    if (detail->track)
    {
	buf = g_markup_printf_escaped ("<b>%s / %s</b>",
				       detail->track->artist,
				       detail->track->title);
    }
    else
    {
	buf = g_strdup (_("<b>n/a</b>"));
    }
    gtk_label_set_markup (GTK_LABEL (w), buf);
    g_free (buf);
}

/* Set the display to @track */
static void details_set_track (Detail *detail, Track *track)
{
    T_item item;

    g_return_if_fail (detail);

    detail->track = track;

    /* Set thumbnail */
    details_update_thumbnail (detail);

    for (item=1; item<T_ITEM_NUM; ++item)
    {
	details_set_item (detail, track, item);
    }

    details_update_headline (detail);

    details_update_buttons (detail);
}


/* Set the first track of @tracks. If detail->tracks is already set,
 * replace. */
static void details_set_tracks (Detail *detail, GList *tracks)
{
    GList *gl;
    g_return_if_fail (detail);
    g_return_if_fail (tracks);

    if (detail->orig_tracks)
    {
	g_list_free (detail->orig_tracks);
	detail->orig_tracks = NULL;
    }
    if (detail->tracks)
    {
	for (gl=detail->tracks; gl; gl=gl->next)
	{
	    Track *tr = gl->data;
	    g_return_if_fail (tr);
	    itdb_track_free (tr);
	}
	g_list_free (detail->tracks);
	detail->tracks = NULL;
    }

    detail->itdb = ((Track *)tracks->data)->itdb;

    detail->orig_tracks = glist_duplicate (tracks);

    /* Create duplicated list to work on until "Apply" is pressed */
    for (gl=g_list_last (tracks); gl; gl=gl->prev)
    {
	Track *tr_dup;
	ExtraTrackData *etr_dup;
	Track *tr = gl->data;
	g_return_if_fail (tr);
	
	tr_dup = itdb_track_duplicate (tr);
	etr_dup = tr_dup->userdata;
	g_return_if_fail (etr_dup);
	etr_dup->tchanged = FALSE;
	etr_dup->tartwork_changed = FALSE;
	detail->tracks = g_list_prepend (detail->tracks, tr_dup);
    }

    detail->track = NULL;
    detail->changed = FALSE;

    details_set_track (detail, g_list_nth_data (detail->tracks, 0));
}


void details_remove_track_intern (Detail *detail, Track *track)
{
    gint i;
    Track *dis_track;

    g_return_if_fail (detail);
    g_return_if_fail (track);

    i = g_list_index (detail->orig_tracks, track);
    if (i == -1)
	return;  /* track not displayed */

    /* get the copied track */
    dis_track = g_list_nth_data (detail->tracks, i);
    g_return_if_fail (dis_track);

    /* remove tracks */
    detail->orig_tracks = g_list_remove (detail->orig_tracks, track);
    detail->tracks = g_list_remove (detail->tracks, dis_track);

    if (detail->track == dis_track)
    {   /* find new track to display */
	dis_track = g_list_nth_data (detail->tracks, i);
	if ((dis_track == NULL) && (i > 0))
	{
	    dis_track = g_list_nth_data (detail->tracks, i-1);
	}
	/* set new track */
	details_set_track (detail, dis_track);
    }

    details_update_buttons (detail);
}


/* Free memory taken by @detail */
static void details_free (Detail *detail)
{
    g_return_if_fail (detail);

    g_object_unref (detail->xml);

    if (detail->window)
    {
	gtk_widget_destroy (detail->window);
    }

    if (detail->orig_tracks)
    {
	g_list_free (detail->orig_tracks);
    }

    if (detail->tracks)
    {
	GList *gl;
	for (gl=detail->tracks; gl; gl=gl->next)
	{
	    Track *tr = gl->data;
	    g_return_if_fail (tr);
	    itdb_track_free (tr);
	}
	g_list_free (detail->tracks);
    }

    g_free (detail);
}


/* Open the details window and display the selected tracks, starting
 * with the first track */
void details_edit (GList *selected_tracks)
{
    Detail *detail;
    GtkWidget *w;
    gint defx, defy, page;
    T_item i;

    g_return_if_fail (selected_tracks);

    detail = g_malloc0 (sizeof (Detail));

    detail->xml = glade_xml_new (xml_file, "details_window", NULL);
/*  no signals to connect -> comment out */
/*     glade_xml_signal_autoconnect (detail->xml); */
    detail->window = gtkpod_xml_get_widget (detail->xml, "details_window");
    g_return_if_fail (detail->window);

    details = g_list_append (details, detail);

    for (i=1; i<T_ITEM_NUM; ++i)
    {
	details_setup_widget (detail, i);
    }

    /* Navigation */
    w = gtkpod_xml_get_widget (detail->xml, "details_button_first");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_first_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_previous");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_previous_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_next");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_next_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_last");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_last_clicked),
		      detail);

    /* Thumbnail control */
    w = gtkpod_xml_get_widget (detail->xml, "details_button_set_artwork");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_set_artwork_clicked),
		      detail);

		w = gtkpod_xml_get_widget (detail->xml, "details_button_fetch_cover");
    g_signal_connect (w, "clicked",
    			G_CALLBACK (on_fetchcover_fetch_button),
          detail);
		
    w = gtkpod_xml_get_widget (detail->xml, "details_button_remove_artwork");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_remove_artwork_clicked),
		      detail);

    /* Window control */
    w = gtkpod_xml_get_widget (detail->xml, "details_button_apply");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_apply_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_cancel");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_cancel_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_ok");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_ok_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_undo_all");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_undo_all_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml, "details_button_undo_track");
    g_signal_connect (w, "clicked",
		      G_CALLBACK (details_button_undo_track_clicked),
		      detail);

    w = gtkpod_xml_get_widget (detail->xml,
			       "details_checkbutton_writethrough");
    g_signal_connect (w, "toggled",
		      G_CALLBACK (details_writethrough_toggled),
		      detail);

    g_signal_connect (detail->window, "delete_event",
		      G_CALLBACK (details_delete_event), detail);


    details_set_tracks (detail, selected_tracks);

    /* set notebook page */
    w = gtkpod_xml_get_widget (detail->xml, "details_notebook");
    page = prefs_get_int (DETAILS_WINDOW_NOTEBOOK_PAGE);
    if ((page >= 0) && (page <= 3))
	gtk_notebook_set_current_page (GTK_NOTEBOOK (w), page);

    /* set default size */
    defx = prefs_get_int (DETAILS_WINDOW_DEFX);
    defy = prefs_get_int (DETAILS_WINDOW_DEFY);

    if ((defx != 0) && (defy != 0))
/* 	gtk_window_set_default_size (GTK_WINDOW (detail->window), */
/* 				     defx, defy); */
	gtk_window_resize (GTK_WINDOW (detail->window),
			   defx, defy);

    gtk_widget_show (detail->window);
}


/* Use the dimension of the first open window */
void details_update_default_sizes (void)
{
    if (details)
	details_store_window_state (details->data);
}


/* Remove @track from the details window (assuming it was removed
   from the database altogether */
void details_remove_track (Track *track)
{
    GList *gl;

    g_return_if_fail (track);
    for (gl=details; gl; gl=gl->next)
    {
	Detail *detail = gl->data;
	g_return_if_fail (detail);
	details_remove_track_intern (detail, track);
    }
}

/* Returns the detail struct currently selected in details */
Detail *details_get_selected_detail ()
{
        return details->data;
}
