/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users.sourceforge.net>
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
#include <stdio.h>
#include <string.h>

#include "itdb.h"
#include "misc.h"
#include "display.h"
#include "prefs.h"

static const gchar *SPL_WINDOW_DEFX="spl_window_defx";
static const gchar *SPL_WINDOW_DEFY="spl_window_defy";

static void spl_display_checklimits (GtkWidget *spl_window);
static void spl_update_rule (GtkWidget *spl_window, Itdb_SPLRule *splr);
static void spl_update_rules_from_row (GtkWidget *spl_window, gint row);
static void spl_action_changed (GtkComboBox *combobox, GtkWidget *spl_window);

#define WNLEN 100 /* max length for widget names */
#define XPAD 1    /* padding for g_table_attach () */
#define YPAD 1    /* padding for g_table_attach () */

typedef struct
{
    guint32 id;
    const gchar *str;
} ComboEntry;

GladeXML *spl_window_xml;

static const ComboEntry splat_inthelast_units_comboentries[] =
{
    { ITDB_SPLACTION_LAST_DAYS_VALUE,   N_("days") },
    { ITDB_SPLACTION_LAST_WEEKS_VALUE,  N_("weeks") },
    { ITDB_SPLACTION_LAST_MONTHS_VALUE, N_("months") },
    { 0,                           NULL }
};


static const ComboEntry splfield_units[] =
{
    { ITDB_SPLFIELD_BITRATE,        N_("kbps") },
    { ITDB_SPLFIELD_SAMPLE_RATE,    N_("Hz") },
    { ITDB_SPLFIELD_SIZE,           N_("MB") },
    { ITDB_SPLFIELD_TIME,           N_("secs") },
    { 0,                       NULL }
};


static const ComboEntry splfield_comboentries[] =
{
    { ITDB_SPLFIELD_SONG_NAME,      N_("Title") },
    { ITDB_SPLFIELD_ALBUM,          N_("Album") },
    { ITDB_SPLFIELD_ARTIST,         N_("Artist") },
    { ITDB_SPLFIELD_BITRATE,        N_("Bitrate") },
    { ITDB_SPLFIELD_SAMPLE_RATE,    N_("Samplerate") },
    { ITDB_SPLFIELD_YEAR,           N_("Year") },
    { ITDB_SPLFIELD_GENRE,          N_("Genre") },
    { ITDB_SPLFIELD_KIND,           N_("Kind") },
    { ITDB_SPLFIELD_DATE_MODIFIED,  N_("Date modified") },
    { ITDB_SPLFIELD_TRACKNUMBER,    N_("Track number") },
    { ITDB_SPLFIELD_SIZE,           N_("Size") },
    { ITDB_SPLFIELD_TIME,           N_("Play time") },
    { ITDB_SPLFIELD_COMMENT,        N_("Comment") },
    { ITDB_SPLFIELD_DATE_ADDED,     N_("Date added") },
    { ITDB_SPLFIELD_COMPOSER,       N_("Composer") },
    { ITDB_SPLFIELD_PLAYCOUNT,      N_("Playcount") },
    { ITDB_SPLFIELD_LAST_PLAYED,    N_("Last played") },
    { ITDB_SPLFIELD_DISC_NUMBER,    N_("Disc number") },
    { ITDB_SPLFIELD_RATING,         N_("Rating") },
    { ITDB_SPLFIELD_COMPILATION,    N_("Compilation") },
    { ITDB_SPLFIELD_BPM,            N_("BPM") },
    { ITDB_SPLFIELD_GROUPING,       N_("Grouping") },
    { ITDB_SPLFIELD_PLAYLIST,       N_("Playlist") },
    { ITDB_SPLFIELD_VIDEO_KIND,     N_("Video Kind") },
    { ITDB_SPLFIELD_TVSHOW,         N_("TV Show") },
    { ITDB_SPLFIELD_SEASON_NR,      N_("Season number") },
    { ITDB_SPLFIELD_SKIPCOUNT,      N_("Skip count") },
    { ITDB_SPLFIELD_LAST_SKIPPED,   N_("Last skipped") },
    { ITDB_SPLFIELD_ALBUMARTIST,    N_("Album artist") },
    { 0,                       NULL }
};

static const ComboEntry splaction_ftstring_comboentries[] =
{
    { ITDB_SPLACTION_CONTAINS,         N_("contains") },
    { ITDB_SPLACTION_DOES_NOT_CONTAIN, N_("does not contain") },
    { ITDB_SPLACTION_IS_STRING,        N_("is") },
    { ITDB_SPLACTION_IS_NOT,           N_("is not") },
    { ITDB_SPLACTION_STARTS_WITH,      N_("starts with") },
    { ITDB_SPLACTION_ENDS_WITH,        N_("ends with") },
    { 0,                          NULL }
};

static const ComboEntry splaction_ftint_comboentries[] =
{
    { ITDB_SPLACTION_IS_INT,          N_("is") },
    { ITDB_SPLACTION_IS_NOT_INT,      N_("is not") },
    { ITDB_SPLACTION_IS_GREATER_THAN, N_("is greater than") },
    { ITDB_SPLACTION_IS_LESS_THAN,    N_("is less than") },
    { ITDB_SPLACTION_IS_IN_THE_RANGE, N_("is in the range") },
    { 0,                         NULL }
};

static const ComboEntry splaction_ftdate_comboentries[] =
{
    { ITDB_SPLACTION_IS_INT,             N_("is") },
    { ITDB_SPLACTION_IS_NOT_INT,         N_("is not") },
    { ITDB_SPLACTION_IS_GREATER_THAN,    N_("is after") },
    { ITDB_SPLACTION_IS_LESS_THAN,       N_("is before") },
    { ITDB_SPLACTION_IS_IN_THE_LAST,     N_("in the last") },
    { ITDB_SPLACTION_IS_NOT_IN_THE_LAST, N_("not in the last") },
    { ITDB_SPLACTION_IS_IN_THE_RANGE,    N_("is in the range") },
    { 0,                            NULL }
};

static const ComboEntry splaction_ftboolean_comboentries[] =
{
    { ITDB_SPLACTION_IS_INT,     N_("is set") },
    { ITDB_SPLACTION_IS_NOT_INT, N_("is not set") },
    { 0,                    NULL }
};

static const ComboEntry splaction_ftplaylist_comboentries[] =
{
    { ITDB_SPLACTION_IS_INT,     N_("is") },
    { ITDB_SPLACTION_IS_NOT_INT, N_("is not") },
    { 0,                    NULL }
};

static const ComboEntry splaction_ftbinaryand_comboentries[] =
{
    { ITDB_SPLACTION_BINARY_AND, N_("is") },
    { ITDB_SPLACTION_BINARY_AND, N_("is not") },
    { 0,                    NULL }
};

static const ComboEntry splaction_notsupported_comboentries[] =
{
    { -1,     N_("Not supported") },
    { 0,                    NULL }
};

/* Strings for limittypes */
static const ComboEntry limittype_comboentries[] =
{
    { ITDB_LIMITTYPE_MINUTES, N_("minutes") },
    { ITDB_LIMITTYPE_MB,      N_("MB") },
    { ITDB_LIMITTYPE_SONGS,   N_("tracks") },
    { ITDB_LIMITTYPE_HOURS,   N_("hours") },
    { ITDB_LIMITTYPE_GB,      N_("GB") },
    { 0,                 NULL }
};

/* Strings for limitsort */
static const ComboEntry limitsort_comboentries[] =
{
    { ITDB_LIMITSORT_RANDOM,                N_("random order") },
    { ITDB_LIMITSORT_SONG_NAME,             N_("title") },
    { ITDB_LIMITSORT_ALBUM,                 N_("album") },
    { ITDB_LIMITSORT_ARTIST,                N_("artist") },
    { ITDB_LIMITSORT_GENRE,                 N_("genre") },
    { ITDB_LIMITSORT_MOST_RECENTLY_ADDED,   N_("most recently added") },
    { ITDB_LIMITSORT_LEAST_RECENTLY_ADDED,  N_("least recently added") },
    { ITDB_LIMITSORT_MOST_OFTEN_PLAYED,     N_("most often played") },
    { ITDB_LIMITSORT_LEAST_OFTEN_PLAYED,    N_("least often played") },
    { ITDB_LIMITSORT_MOST_RECENTLY_PLAYED,  N_("most recently played") },
    { ITDB_LIMITSORT_LEAST_RECENTLY_PLAYED, N_("least recently played") },
    { ITDB_LIMITSORT_HIGHEST_RATING,        N_("highest rating") },
    { ITDB_LIMITSORT_LOWEST_RATING,         N_("lowest rating") },
    { 0,                               NULL }
};

/* Strings for Video Kind */
static const ComboEntry videokind_comboentries_is[] =
{
    { ITDB_MEDIATYPE_MOVIE,      N_("Movie") },
    { ITDB_MEDIATYPE_MUSICVIDEO, N_("Music Video") },
    { ITDB_MEDIATYPE_TVSHOW,     N_("TV Show") },
    { 0,                         NULL }
};

/* Strings for Video Kind */
static const ComboEntry videokind_comboentries_is_not[] =
{
    { 0x0e00 | ITDB_MEDIATYPE_MUSICVIDEO | ITDB_MEDIATYPE_TVSHOW,    N_("Movie") },
    { 0x0e00 | ITDB_MEDIATYPE_MOVIE | ITDB_MEDIATYPE_TVSHOW,   N_("Music Video") },
    { 0x0e00 | ITDB_MEDIATYPE_MOVIE | ITDB_MEDIATYPE_MUSICVIDEO,   N_("TV Show") },
    { 0,                                                                    NULL }
};

/* types used for entries (from, to, date...) */
enum entrytype
{
    spl_ET_FROMVALUE = 1,
    spl_ET_FROMVALUE_DATE,  /* fromvalue interpreted as datestamp */
    spl_ET_FROMDATE,
    spl_ET_TOVALUE,
    spl_ET_TOVALUE_DATE,    /* tovalue interpreted as datestamp */
    spl_ET_TODATE,
    spl_ET_INTHELAST,
    spl_ET_STRING,
};

static const gchar *entry_get_string (gchar *str, Itdb_SPLRule *splr,
				      enum entrytype et);

/* Get index from ID (returns -1 if ID could not be found) */
static gint comboentry_index_from_id (const ComboEntry centries[],
				      guint32 id)
{
    gint i;

    g_return_val_if_fail (centries, -1);

    /* handle field types not yet supported */
    if (centries == splaction_notsupported_comboentries)
	return 0;

    for (i=0; centries[i].str; ++i)
    {
	if (centries[i].id == id)  return i;
    }
    return -1;
}


/* Get index in @array from playlist ID @id */
/* returns -1 in case of error */
static gint pl_ids_index_from_id (GArray *pl_ids, guint64 id)
{
    gint index;

    g_return_val_if_fail (pl_ids, -1);

    for (index=0; ; ++index)
    {
	guint64 sid = g_array_index (pl_ids, guint64, index);
	if (sid == id)  return index;
	if (sid == 0)   return -1;
    }
}


/* called by spl_set_combobox() */
static void spl_setup_combobox (GtkComboBox *cb,
				const ComboEntry centries[], gint index,
				GCallback cb_func, gpointer cb_data)
{
    const ComboEntry *old_centries = g_object_get_data (G_OBJECT (cb),
							"spl_centries");

    if ((g_object_get_data (G_OBJECT (cb), "combo_set") == NULL) ||
	(centries != old_centries))
    {   /* the combo has not yet been initialized */
	const ComboEntry *ce = centries;
	GtkCellRenderer *cell;
	GtkListStore *store;

	/* Set the model -- that is you cannot do a
	   gtk_combo_box_new_text()! This gives us the flexibility to
	   expand this function to set some graphic next to the text. */
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (cb, GTK_TREE_MODEL (store));
	g_object_unref (store);

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cb), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cb), cell,
					"text", 0,
					NULL);

	while (ce->str != NULL)
	{
	    gtk_combo_box_append_text (cb, _(ce->str));
	    ++ce;
	}
	g_object_set_data (G_OBJECT (cb), "spl_centries", (gpointer)centries);
	g_object_set_data (G_OBJECT (cb), "combo_set", "set");
	if (cb_func)
	    g_signal_connect (cb, "changed", cb_func, cb_data);
    }
    if (index != -1)
    {
	gtk_combo_box_set_active (cb, index);
    }
}


/* Initialize/update ComboBox @cb with strings from @centries and
   select @id. If @cb_func is != NULL, connect @cb_func to signal
   "changed" with data @cb_data. */
static void spl_set_combobox (GtkComboBox *cb,
			      const ComboEntry centries[], guint32 id,
			      GCallback cb_func, gpointer cb_data)
{
    gint index;

    g_return_if_fail (cb);
    g_return_if_fail (centries);

    index = comboentry_index_from_id (centries, id);

    spl_setup_combobox (cb, centries, index, cb_func, cb_data);

}


/* Callbacks */
static void spl_all_radio_toggled (GtkToggleButton *togglebutton,
				   GtkWidget *spl_window)
{
    Playlist *spl;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    if (gtk_toggle_button_get_active (togglebutton))
    {
	GtkWidget *frame = gtkpod_xml_get_widget (spl_window_xml, "spl_rules_frame");
	g_return_if_fail (frame);
	gtk_widget_set_sensitive (frame, TRUE);
	spl->splpref.checkrules = TRUE;
	spl->splrules.match_operator = ITDB_SPLMATCH_AND;
    }
}

static void spl_any_radio_toggled (GtkToggleButton *togglebutton,
				   GtkWidget *spl_window)
{
    Playlist *spl;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    if (gtk_toggle_button_get_active (togglebutton))
    {
	GtkWidget *frame = gtkpod_xml_get_widget (spl_window_xml, "spl_rules_frame");
	g_return_if_fail (frame);
	gtk_widget_set_sensitive (frame, TRUE);
	spl->splpref.checkrules = TRUE;
	spl->splrules.match_operator = ITDB_SPLMATCH_OR;
    }
}

static void spl_none_radio_toggled (GtkToggleButton *togglebutton,
				    GtkWidget *spl_window)
{
    Playlist *spl;
    GtkWidget *frame;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    frame = gtkpod_xml_get_widget (spl_window_xml, "spl_rules_frame");
    g_return_if_fail (frame);
    if (gtk_toggle_button_get_active (togglebutton))
    {
	gtk_widget_set_sensitive (frame, FALSE);
	spl->splpref.checkrules = FALSE;
    }
}

static void spl_matchcheckedonly_toggled (GtkToggleButton *togglebutton,
					  GtkWidget *spl_window)
{
    Playlist *spl;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    spl->splpref.matchcheckedonly = 
	gtk_toggle_button_get_active (togglebutton);
}

static void spl_liveupdate_toggled (GtkToggleButton *togglebutton,
				    GtkWidget *spl_window)
{
    Playlist *spl;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    spl->splpref.liveupdate = 
	gtk_toggle_button_get_active (togglebutton);
}


static void spl_checklimits_toggled (GtkToggleButton *togglebutton,
				     GtkWidget *spl_window)
{
    Playlist *spl;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    spl->splpref.checklimits =
	gtk_toggle_button_get_active (togglebutton);
    spl_display_checklimits (spl_window);
}


/* The limitvalue has changed */
static void spl_limitvalue_changed (GtkEditable *editable,
				    GtkWidget *spl_window)
{
    Playlist *spl;
    gchar *str;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    str = gtk_editable_get_chars (editable, 0, -1);
    spl->splpref.limitvalue = atol (str);
    g_free (str);
}



/* Limittype has been changed */
static void spl_limittype_changed (GtkComboBox *combobox,
				   GtkWidget *spl_window)
{
    Playlist *spl;
    gint index = gtk_combo_box_get_active (combobox);

    g_return_if_fail (index != -1);
    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    spl->splpref.limittype = limittype_comboentries[index].id;
}


/* Limitsort has been changed */
static void spl_limitsort_changed (GtkComboBox *combobox,
				   GtkWidget *spl_window)
{
    Playlist *spl;
    gint index = gtk_combo_box_get_active (combobox);

    g_return_if_fail (index != -1);
    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    spl->splpref.limitsort = limitsort_comboentries[index].id;
}


/* Rule field has been changed */
static void spl_field_changed (GtkComboBox *combobox, GtkWidget *spl_window)
{
    Playlist *spl;
    Itdb_SPLRule *splr;
    gint index = gtk_combo_box_get_active (combobox);

    g_return_if_fail (index != -1);
    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    splr = g_object_get_data (G_OBJECT (combobox), "spl_rule");
    g_return_if_fail (splr);

    if (splr->field != splfield_comboentries[index].id)
    {   /* data changed */
	splr->field = splfield_comboentries[index].id;
	/* update display */
	spl_update_rule (spl_window, splr);
    }
}


/* Action field has been changed */
static void spl_action_changed (GtkComboBox *combobox, GtkWidget *spl_window)
{
    Playlist *spl;
    Itdb_SPLRule *splr;
    ItdbSPLFieldType ft;
    const ComboEntry *centries;
    gint index = gtk_combo_box_get_active (combobox);

    g_return_if_fail (index != -1);
    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    splr = g_object_get_data (G_OBJECT (combobox), "spl_rule");
    g_return_if_fail (splr);

    centries = g_object_get_data (G_OBJECT (combobox), "spl_centries");
    g_return_if_fail (centries);

    ft = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combobox), "spl_fieldtype"));

/* printf ("(action) value changed: %04x...", (gint)splr->fromvalue); */
    switch (ft)
    {
    case ITDB_SPLFT_BINARY_AND:
	if (splr->field == ITDB_SPLFIELD_VIDEO_KIND)
	{   /* traet Video Kind differently */
	    gint oldindex = GPOINTER_TO_INT (
		g_object_get_data (G_OBJECT (combobox), "spl_binary_and_index"));
	    if (oldindex != index)
	    {   /* data changed */
		/* inverse the video bits */
		splr->fromvalue = (~splr->fromvalue) &
		    (0x0e00 | ITDB_MEDIATYPE_MOVIE | ITDB_MEDIATYPE_MUSICVIDEO | ITDB_MEDIATYPE_TVSHOW);
		spl_update_rule (spl_window, splr);
	    }
	}
	else
	{   /* treat as standard */
	    if (splr->action != centries[index].id)
	    {   /* data changed */
		splr->action = centries[index].id;
		/* update display */
		spl_update_rule (spl_window, splr);
	    }
	}
	break;
    default:
	if (splr->action != centries[index].id)
	{   /* data changed */
	    splr->action = centries[index].id;
	    /* update display */
	    spl_update_rule (spl_window, splr);
	}
	break;
    }
/* printf ("%04x\n", (gint)splr->fromvalue); */
}


/*  field has been changed */
static void spl_videokind_comboentry_changed (GtkComboBox *combobox,
					      GtkWidget *spl_window)
{
    Playlist *spl;
    Itdb_SPLRule *splr;
    const ComboEntry *centries;
    gint index = gtk_combo_box_get_active (combobox);

    g_return_if_fail (index != -1);
    g_return_if_fail (spl_window);

    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    splr = g_object_get_data (G_OBJECT (combobox), "spl_rule");
    g_return_if_fail (splr);

    centries = g_object_get_data (G_OBJECT (combobox), "spl_centries");
    g_return_if_fail (centries);

/* printf ("(entry) value changed: %04x...", (gint)splr->fromvalue); */
    if (splr->fromvalue != centries[index].id)
    {   /* data changed */
	splr->fromvalue = centries[index].id;
    }
/* printf ("%04x\n", (gint)splr->fromvalue); */
}


/* The enter key was pressed inside a rule entry (fromvalue, fromdate,
 * tovalue, todate, string...)  --> redisplay */
static void splr_entry_redisplay (GtkEditable *editable, GtkWidget *spl_window)
{
    Itdb_SPLRule *splr;
    enum entrytype type;
    gchar str[WNLEN];
    const gchar *strp;

    g_return_if_fail (spl_window);
    splr =  g_object_get_data (G_OBJECT (editable), "spl_rule");
    g_return_if_fail (splr);
    type = (enum entrytype)g_object_get_data (
	G_OBJECT (editable), "spl_entrytype");
    g_return_if_fail (type != 0);
    strp = entry_get_string (str, splr, type);
    if (strp)  gtk_entry_set_text (GTK_ENTRY (editable), strp);
}
    


/* The content of a rule entry (fromvalue, fromdate, tovalue, todate,
 * string...)  has changed --> update */
static void splr_entry_changed (GtkEditable *editable,
				GtkWidget *spl_window)
{
    Itdb_SPLRule *splr;
    gchar *str;
    time_t t;
    enum entrytype type;

    g_return_if_fail (spl_window);
    splr =  g_object_get_data (G_OBJECT (editable), "spl_rule");
    g_return_if_fail (splr);
    type = (enum entrytype)g_object_get_data (
	G_OBJECT (editable), "spl_entrytype");
    g_return_if_fail (type != 0);

    str = gtk_editable_get_chars (editable, 0, -1);
    switch (type)
    {
    case spl_ET_FROMVALUE:
	switch (splr->field)
	{
	case ITDB_SPLFIELD_RATING:
	    splr->fromvalue = ITDB_RATING_STEP * atol (str);
	    break;
	case ITDB_SPLFIELD_TIME:
	    splr->fromvalue = 1000 * strtod (str, NULL);
	    break;
	default:
	    splr->fromvalue = atol (str);
	    break;
	}
	break;
    case spl_ET_FROMVALUE_DATE:
	t = time_string_to_fromtime (str);
	if (t != -1)
	    splr->fromvalue = t;
	break;
    case spl_ET_FROMDATE:
	splr->fromdate = atol (str);
	break;
    case spl_ET_TOVALUE:
	switch (splr->field)
	{
	case ITDB_SPLFIELD_RATING:
	    splr->tovalue = ITDB_RATING_STEP * atol (str);
	    break;
	case ITDB_SPLFIELD_TIME:
	    splr->tovalue = 1000 * strtod (str, NULL);
	    break;
	default:
	    splr->tovalue = atol (str);
	    break;
	}
	break;
    case spl_ET_TOVALUE_DATE:
	t = time_string_to_totime (str);
	if (t != -1)
	    splr->tovalue = t;
	break;
    case spl_ET_TODATE:
	splr->todate = atol (str);
	break;
    case spl_ET_INTHELAST:
	splr->fromdate = -atol (str);
	break;
    case spl_ET_STRING:
	g_free (splr->string);
	splr->string = g_strdup (str);
	break;
    default:
	/* must not happen */
	g_free (str);
	g_return_if_fail (FALSE);
	break;
    }
    g_free (str);
}


/* splat_inthelast's fromunits have changed --> update */
static void spl_fromunits_changed (GtkComboBox *combobox,
				   GtkWidget *spl_window)
{
    Itdb_SPLRule *splr;
    gint index = gtk_combo_box_get_active (combobox);

    g_return_if_fail (index != -1);
    g_return_if_fail (spl_window);
    splr =  g_object_get_data (G_OBJECT (combobox), "spl_rule");
    g_return_if_fail (splr);
    splr->fromunits =
	splat_inthelast_units_comboentries[index].id;
}




/* splat_playlist has changed --> update */
static void spl_playlist_changed (GtkComboBox *combobox,
				  GtkWidget *spl_window)
{
    Itdb_SPLRule *splr;
    GArray *pl_ids;
    gint index;

    g_return_if_fail (combobox);
    g_return_if_fail (spl_window);
    splr =  g_object_get_data (G_OBJECT (combobox), "spl_rule");
    g_return_if_fail (splr);
    pl_ids = g_object_get_data (G_OBJECT (combobox), "spl_pl_ids");
    g_return_if_fail (pl_ids);
    index = gtk_combo_box_get_active (combobox);
    g_return_if_fail (index != -1);
    splr->fromvalue = g_array_index (pl_ids, guint64, index);
}


/* Deactivate "minus" (delete rule) button if only one rule is
   displayed, activate the "minus" (delete rule) buttons if more than
   one rule is displayed. This affects only the first button
   (spl_button-0) */
static void spl_check_number_of_rules (GtkWidget *spl_window)
{
    Playlist *spl;
    GtkTable *table;
    gint numrules;
    GtkWidget *button;

    g_return_if_fail (spl_window);

    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    table = g_object_get_data (G_OBJECT (spl_window), "spl_rules_table");
    g_return_if_fail (table);

    numrules = g_list_length (spl->splrules.rules);
    g_return_if_fail (numrules > 0);

    button = g_object_get_data (G_OBJECT (table), "spl_button-0");
    g_return_if_fail (button);
    if (numrules > 1)
	gtk_widget_set_sensitive (button, TRUE);
    else
	gtk_widget_set_sensitive (button, FALSE);
}


static void spl_button_minus_clicked (GtkButton *button,
				      GtkWidget *spl_window)
{
    Playlist *spl;
    Itdb_SPLRule *splr;
    gint row;

    g_return_if_fail (spl_window);

    splr =  g_object_get_data (G_OBJECT (button), "spl_rule");
    g_return_if_fail (splr);

    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    row = g_list_index (spl->splrules.rules, splr);
    g_return_if_fail (row != -1);

    itdb_splr_remove (spl, splr);
    spl_update_rules_from_row (spl_window, row);

    spl_check_number_of_rules (spl_window);
}


static void spl_button_plus_clicked (GtkButton *button,
				     GtkWidget *spl_window)
{
    Playlist *spl;
    Itdb_SPLRule *splr;
    gint row;

    g_return_if_fail (spl_window);

    splr =  g_object_get_data (G_OBJECT (button), "spl_rule");
    g_return_if_fail (splr);

    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    row = g_list_index (spl->splrules.rules, splr);
    g_return_if_fail (row != -1);

    itdb_splr_add_new (spl, row+1);
    spl_update_rules_from_row (spl_window, row+1);

    spl_check_number_of_rules (spl_window);
}


static void spl_store_window_size (GtkWidget *spl_window)
{
    gint defx, defy;

    gtk_window_get_size (GTK_WINDOW (spl_window), &defx, &defy);
    prefs_set_int (SPL_WINDOW_DEFX, defx);
    prefs_set_int (SPL_WINDOW_DEFY, defy);
}

static void spl_cancel (GtkButton *button, GtkWidget *spl_window)
{
    Playlist *spl_dup = g_object_get_data (G_OBJECT (spl_window),
					   "spl_work");
    Playlist *spl_orig = g_object_get_data (G_OBJECT (spl_window),
					    "spl_orig");
    iTunesDB *itdb = g_object_get_data (G_OBJECT (spl_window),
					"spl_itdb");

    g_return_if_fail (spl_dup != NULL);
    g_return_if_fail (spl_orig != NULL);
    g_return_if_fail (itdb != NULL);

    itdb_playlist_free (spl_dup);
    /* does playlist already exist in display? */
    if (!itdb_playlist_exists (itdb, spl_orig))
    {   /* Delete */
	itdb_playlist_free (spl_orig);
    }

    spl_store_window_size (spl_window);

    gtk_widget_destroy (spl_window);

    release_widgets ();
}


static void spl_delete_event (GtkWidget *widget,
			      GdkEvent *event,
			      GtkWidget *spl_window)
{
    spl_cancel (NULL, spl_window);
}


static void spl_ok (GtkButton *button, GtkWidget *spl_window)
{
    GtkWidget *w;
    Playlist *spl_dup;
    Playlist *spl_orig;
    iTunesDB *itdb;
    gint32 pos;

    g_return_if_fail (spl_window_xml != NULL);

    spl_dup = g_object_get_data (G_OBJECT (spl_window), "spl_work");
    spl_orig = g_object_get_data (G_OBJECT (spl_window), "spl_orig");
    pos =  (gint32)GPOINTER_TO_INT(g_object_get_data (G_OBJECT (spl_window), "spl_pos"));
    itdb = g_object_get_data (G_OBJECT (spl_window), "spl_itdb");

    g_return_if_fail (spl_dup != NULL);
    g_return_if_fail (spl_orig != NULL);
    g_return_if_fail (itdb != NULL);

    /* Read out new playlist name */
    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_name_entry")))
    {
	g_free (spl_orig->name);
	spl_orig->name = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
    }

    itdb_spl_copy_rules (spl_orig, spl_dup);

    itdb_playlist_free (spl_dup);

    /* does playlist already exist in itdb? */
    if (!itdb_playlist_exists (itdb, spl_orig))
    {   /* Insert at specified position */
	gp_playlist_add (itdb, spl_orig, pos);
    }

    itdb_spl_update (spl_orig);

    if (pm_get_selected_playlist () == spl_orig)
    {   /* redisplay */
	pm_unselect_playlist (spl_orig);
	pm_select_playlist (spl_orig);
    }

    data_changed (itdb);

    spl_store_window_size (spl_window);

    gtk_widget_destroy (spl_window);

    release_widgets ();
}


/* Display the "checklimits" data correctly */
static void spl_display_checklimits (GtkWidget *spl_window)
{
    Playlist *spl;
    GtkWidget *w;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_checklimits_button")))
    {
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (w), spl->splpref.checklimits);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (spl_checklimits_toggled),
			  spl_window);
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_limitvalue_entry")))
    {
	gchar str[WNLEN];
	snprintf (str, WNLEN, "%d", spl->splpref.limitvalue);
	gtk_entry_set_text (GTK_ENTRY (w), str);
	gtk_widget_set_sensitive (w, spl->splpref.checklimits);
	g_signal_connect (w, "changed",
			  G_CALLBACK (spl_limitvalue_changed),
			  spl_window);
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_limittype_combobox")))
    {
	spl_set_combobox (GTK_COMBO_BOX (w), 
			  limittype_comboentries,
			  spl->splpref.limittype,
			  G_CALLBACK (spl_limittype_changed),
			  spl_window);
	gtk_widget_set_sensitive (w, spl->splpref.checklimits);
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_limitsort_label")))
    {
	gtk_widget_set_sensitive (w, spl->splpref.checklimits);
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_limitsort_combobox")))
    {
	spl_set_combobox (GTK_COMBO_BOX (w), 
			  limitsort_comboentries,
			  spl->splpref.limitsort,
			  G_CALLBACK (spl_limitsort_changed),
			  spl_window);
	gtk_widget_set_sensitive (w, spl->splpref.checklimits);
    }
}



/* from "man strftime 3" ("%x" behaves just like "%c"):

       Some buggy versions of gcc complain about the use of %c:
       warning: `%c' yields only last 2 digits of year in some
       locales.  Of course program- mers are encouraged to use %c, it
       gives the preferred date and time representation. One meets all
       kinds of strange obfuscations to circum- vent this gcc
       problem. A relatively clean one is to add an intermediate
       function
*/
size_t my_strftime(char *s, size_t max, const char  *fmt,  const
		   struct tm *tm)
{
    return strftime(s, max, fmt, tm);
}


/* set @str to a timestring corresponding to mac timestamp @value */
void set_timestring (gchar *str, guint64 value, enum entrytype et)
{
    time_t t;
    gchar *resstr;

    g_return_if_fail (str != NULL);

    t = (time_t)value;
    if (et == spl_ET_FROMVALUE_DATE)
    {
	resstr = time_fromtime_to_string (t);
    }
    else
    {
	resstr = time_totime_to_string (t);
    }
    strncpy (str, resstr, WNLEN);
    str[WNLEN-1] = 0;
    g_free (resstr);
}


/* set the string @str for rule @splr (entrytype: @et) */
/* @str must be WNLEN chars long. Returns a pointer to the string to
 * be used */
const gchar *entry_get_string (gchar *str, Itdb_SPLRule *splr,
			       enum entrytype et)
{
    gchar *strp = str;

    g_return_val_if_fail (str, NULL);
    g_return_val_if_fail (splr, NULL);

    switch (et)
    {
    case spl_ET_FROMVALUE:
	if (splr->fromvalue == ITDB_SPL_DATE_IDENTIFIER)
	    splr->fromvalue = 0;
	switch (splr->field)
	{
	case ITDB_SPLFIELD_RATING:
	    snprintf (str, WNLEN, "%lld", (long long int)(splr->fromvalue / ITDB_RATING_STEP));
	    break;
	case ITDB_SPLFIELD_TIME:
	    snprintf (str, WNLEN, "%.10g", ((gdouble)splr->fromvalue/1000));
	    break;
	default:
	    snprintf (str, WNLEN, "%lld", (long long int)(splr->fromvalue));
	    break;
	}
	break;
    case spl_ET_FROMVALUE_DATE:
	if (splr->fromvalue == ITDB_SPL_DATE_IDENTIFIER)
	    splr->fromvalue = 0;
	set_timestring (str, splr->fromvalue, et);
	break;
    case spl_ET_FROMDATE:
	snprintf (str, WNLEN, "%lld",  (long long int)splr->fromdate);
	break;
    case spl_ET_TOVALUE:
	if (splr->tovalue == ITDB_SPL_DATE_IDENTIFIER)
	    splr->tovalue = 0;
	switch (splr->field )
	{
	case ITDB_SPLFIELD_RATING:
	    snprintf (str, WNLEN, "%lld",  (long long int)(splr->tovalue / ITDB_RATING_STEP));
	    break;
	case ITDB_SPLFIELD_TIME:
	    snprintf (str, WNLEN, "%.10g", ((gdouble)splr->tovalue/1000));
	    break;
	default:
	    snprintf (str, WNLEN, "%lld", (long long int)(splr->tovalue));
	    break;
	}
	break;
    case spl_ET_TOVALUE_DATE:
	if (splr->tovalue == ITDB_SPL_DATE_IDENTIFIER)
	    splr->tovalue = 0;
	set_timestring (str, splr->tovalue, et);
	break;
    case spl_ET_TODATE:
	snprintf (str, WNLEN, "%lld",  (long long int)splr->todate);
	break;
    case spl_ET_INTHELAST:
	snprintf (str, WNLEN, "%lld",  (long long int)-splr->fromdate);
	break;
    case spl_ET_STRING:
/*	gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);*/
	strp = splr->string;
	break;
    default:
	/* must not happen */
	g_return_val_if_fail (FALSE, NULL);
	break;
    }
    return strp;
}



static GtkWidget *hbox_add_entry (GtkWidget *hbox,
				  Itdb_SPLRule *splr,
				  enum entrytype et)
{
    GtkWidget *spl_window;
    GtkWidget *entry;
    gchar str[WNLEN];
    const gchar *strp;

    g_return_val_if_fail (hbox, NULL);

    str[0] = 0;

    spl_window = g_object_get_data (G_OBJECT (hbox), "spl_window");
    g_return_val_if_fail (spl_window, NULL);

    entry = gtk_entry_new ();
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    if (et == spl_ET_STRING)
	  gtk_entry_set_max_length (GTK_ENTRY (entry),
				    ITDB_SPL_STRING_MAXLEN);
    else  gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    strp = entry_get_string (str, splr, et);
    if (strp)  gtk_entry_set_text (GTK_ENTRY (entry), strp);
    g_object_set_data (G_OBJECT (entry), "spl_rule", splr);
    g_object_set_data (G_OBJECT (entry), "spl_entrytype",
		       (gpointer)et);
    g_signal_connect (entry, "changed",
		      G_CALLBACK (splr_entry_changed),
		      spl_window);
    g_signal_connect (entry, "activate",
		      G_CALLBACK (splr_entry_redisplay),
		      spl_window);
    return entry;
}


/* Called to destroy the memory used by the array holding the playlist
   IDs. It is called automatically when the associated object
   (combobox) is destroyed */
void spl_pl_ids_destroy (GArray *array)
{
    g_return_if_fail (array);
    g_array_free (array, TRUE);
}


/* Create the widgets to hold the action data (range, date,
 * string...) */
GtkWidget *spl_create_hbox (GtkWidget *spl_window, Itdb_SPLRule *splr)
{
    GtkWidget *hbox = NULL;
    ItdbSPLActionType at;
    GtkWidget *entry, *label, *combobox;
    gint index;
    GArray *pl_ids = NULL;
    Playlist *spl_orig;
    iTunesDB *itdb;
    GList *gl;


    g_return_val_if_fail (spl_window, NULL);
    g_return_val_if_fail (splr, NULL);

    spl_orig =  g_object_get_data (G_OBJECT (spl_window), "spl_orig");
    g_return_val_if_fail (spl_orig, NULL);

    itdb =  g_object_get_data (G_OBJECT (spl_window), "spl_itdb");
    g_return_val_if_fail (itdb, NULL);

    at = itdb_splr_get_action_type (splr);
    g_return_val_if_fail (at != ITDB_SPLAT_UNKNOWN, NULL);
    g_return_val_if_fail (at != ITDB_SPLAT_INVALID, NULL);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox);
    g_object_set_data (G_OBJECT (hbox), "spl_window", spl_window);

    switch (at)
    {
    case ITDB_SPLAT_STRING:
	entry = hbox_add_entry (hbox, splr, spl_ET_STRING);
	break;
    case ITDB_SPLAT_INT:
	entry = hbox_add_entry (hbox, splr, spl_ET_FROMVALUE);
	/* check for unit */
	index = comboentry_index_from_id (splfield_units, splr->field);
	if (index != -1)
	{
	    label = gtk_label_new (_(splfield_units[index].str));
	    gtk_widget_show (label);
	    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	}
	break;
    case ITDB_SPLAT_DATE:
	entry = hbox_add_entry (hbox, splr, spl_ET_FROMVALUE_DATE);
	break;
    case ITDB_SPLAT_RANGE_INT:
	entry = hbox_add_entry (hbox, splr, spl_ET_FROMVALUE);
	label = gtk_label_new (_("to"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	entry = hbox_add_entry (hbox, splr, spl_ET_TOVALUE),
	/* check for unit */
	index = comboentry_index_from_id (splfield_units, splr->field);
	if (index != -1)
	{
	    label = gtk_label_new (_(splfield_units[index].str));
	    gtk_widget_show (label);
	    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	}
	break;
    case ITDB_SPLAT_RANGE_DATE:
	entry = hbox_add_entry (hbox, splr, spl_ET_FROMVALUE_DATE);
	label = gtk_label_new (_("to"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	entry = hbox_add_entry (hbox, splr, spl_ET_TOVALUE_DATE);
	/* check for unit */
	index = comboentry_index_from_id (splfield_units, splr->field);
	if (index != -1)
	{
	    label = gtk_label_new (_(splfield_units[index].str));
	    gtk_widget_show (label);
	    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	}
	break;
    case ITDB_SPLAT_INTHELAST:
	if (comboentry_index_from_id (
		splat_inthelast_units_comboentries,
		splr->fromunits) == -1)
	{   /* unit currently set is not known */
	    /* adapt to known units */
	    guint64 units = splr->fromunits;
	    splr->fromunits = splat_inthelast_units_comboentries[0].id;
	    splr->fromvalue *= ((double)units)/splr->fromunits;
	}
	entry = hbox_add_entry (hbox, splr, spl_ET_INTHELAST);
	combobox = gtk_combo_box_new ();
	gtk_widget_show (combobox);
	gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
	g_object_set_data (G_OBJECT (combobox), "spl_rule", splr);
	spl_set_combobox (GTK_COMBO_BOX (combobox),
			  splat_inthelast_units_comboentries,
			  splr->fromunits,
			  G_CALLBACK (spl_fromunits_changed),
			  spl_window);
	break;
    case ITDB_SPLAT_PLAYLIST:
	combobox = gtk_combo_box_new_text ();
	gtk_widget_show (combobox);
	gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
	pl_ids = g_array_sized_new (TRUE, TRUE, sizeof (guint64), 
				    itdb_playlists_number (itdb));
	gl=itdb->playlists;
	while (gl && gl->next)
	{
	    Playlist *pl = gl->next->data;
	    g_return_val_if_fail (pl, NULL);
	    if (pl != spl_orig)
	    {
		gtk_combo_box_append_text (GTK_COMBO_BOX (combobox),
					   pl->name);
		g_array_append_val (pl_ids, pl->id);
	    }
	    gl = gl->next;
	}
	g_object_set_data (G_OBJECT (combobox), "spl_rule", splr);
	g_object_set_data_full (G_OBJECT (combobox), "spl_pl_ids",
				pl_ids,
				(GDestroyNotify)spl_pl_ids_destroy);
	if (splr->fromvalue == ITDB_SPL_DATE_IDENTIFIER)
	    splr->fromvalue = g_array_index (pl_ids, guint64, 0);
	index = pl_ids_index_from_id (pl_ids, splr->fromvalue);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), index);
	g_signal_connect (combobox, "changed",
			  G_CALLBACK (spl_playlist_changed),
			  spl_window);
	break;
    case ITDB_SPLAT_BINARY_AND:
	if (splr->field == ITDB_SPLFIELD_VIDEO_KIND)
	{
	    const ComboEntry *use_centries = NULL;
	    combobox = gtk_combo_box_new ();
	    gtk_widget_show (combobox);
	    gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
	    if (comboentry_index_from_id (videokind_comboentries_is,
					  splr->fromvalue) != -1)
	    {
		use_centries = videokind_comboentries_is;
	    }
	    else
	    {
		use_centries = videokind_comboentries_is_not;
	    }
	    g_object_set_data (G_OBJECT (combobox), "spl_rule", splr);
	    spl_set_combobox (GTK_COMBO_BOX (combobox),
			      use_centries,
			      splr->fromvalue,
			      G_CALLBACK (spl_videokind_comboentry_changed),
			      spl_window);
	}
	else
	{   /* not supported: display as standard INT */
	    entry = hbox_add_entry (hbox, splr, spl_ET_FROMVALUE);
	}
	break;
    case ITDB_SPLAT_NONE:
	break;
    case ITDB_SPLAT_UNKNOWN:
    case ITDB_SPLAT_INVALID:
	/* hopefully never reached !! */
	break;
    }
    return hbox;
}


/* Display/update rule @n in @spl_window */
static void spl_update_rule (GtkWidget *spl_window, Itdb_SPLRule *splr)
{
    GtkTable *table;
    Playlist *spl;
    GtkWidget *combobox, *hbox, *button;
    gchar name[WNLEN];
    ItdbSPLFieldType ft;
    ItdbSPLActionType at;
    gint row, index;
    const ComboEntry *centries = NULL;


    g_return_if_fail (spl_window);
    g_return_if_fail (splr);

    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    table = g_object_get_data (G_OBJECT (spl_window), "spl_rules_table");
    g_return_if_fail (table);

    row = g_list_index (spl->splrules.rules, splr);
    g_return_if_fail (row != -1);


    /* Combobox for field */
    /* ------------------ */
    snprintf (name, WNLEN, "spl_fieldcombo%d", row);
    combobox = g_object_get_data (G_OBJECT (table), name);
    if (!combobox)
    {  /* create combo for field */
	combobox = gtk_combo_box_new ();
	gtk_widget_show (combobox);
	gtk_table_attach (table, combobox, 0,1, row,row+1,
			  0,0,         /* expand options */
			  XPAD,YPAD);  /* padding options */
	g_object_set_data (G_OBJECT (table), name, combobox);
    }
    g_object_set_data (G_OBJECT (combobox), "spl_rule", splr);
    spl_set_combobox (GTK_COMBO_BOX (combobox),
		      splfield_comboentries,
		      splr->field,
		      G_CALLBACK (spl_field_changed),
		      spl_window);

    /* Combobox for action */
    /* ------------------- */
    ft = itdb_splr_get_field_type (splr);
    snprintf (name, WNLEN, "spl_actioncombo%d", row);
    combobox = g_object_get_data (G_OBJECT (table), name);

    if (combobox)
    {   /* check if existing combobox is of same type */
	ItdbSPLFieldType old_ft = GPOINTER_TO_INT (
	    g_object_get_data (G_OBJECT (combobox), "spl_fieldtype"));
	if (old_ft != ft)
	{
	    gtk_widget_destroy (combobox);
	    combobox = NULL;
	}
    }

    if (!combobox)
    {  /* create combo for action */
	combobox = gtk_combo_box_new ();
	gtk_widget_show (combobox);
	gtk_table_attach (table, combobox, 1,2, row,row+1,
			  GTK_FILL,0,   /* expand options */
			  XPAD,YPAD);   /* padding options */
	g_object_set_data (G_OBJECT (table), name, combobox);
    }
    g_object_set_data (G_OBJECT (combobox), "spl_rule", splr);
    g_object_set_data (G_OBJECT (combobox), "spl_fieldtype", GINT_TO_POINTER (ft));

    switch (ft)
    {
    case ITDB_SPLFT_STRING:
	centries = splaction_ftstring_comboentries;
	break;
    case ITDB_SPLFT_INT:
	centries = splaction_ftint_comboentries;
	break;
    case ITDB_SPLFT_BOOLEAN:
	centries = splaction_ftboolean_comboentries;
	break;
    case ITDB_SPLFT_DATE:
	centries = splaction_ftdate_comboentries;
	break;
    case ITDB_SPLFT_PLAYLIST:
	centries = splaction_ftplaylist_comboentries;
	break;
    case ITDB_SPLFT_BINARY_AND:
	centries = splaction_ftbinaryand_comboentries;
	break;
    case ITDB_SPLFT_UNKNOWN:
	centries = splaction_notsupported_comboentries;
	break;
    }

    if (centries == NULL)
    {   /* Handle non-supported action types */
	centries = splaction_notsupported_comboentries;
    }

    if (comboentry_index_from_id (centries, splr->action) == -1)
    {   /* Action currently set is not a legal action for the type of
	   field. --> adjust */
	if (centries)    splr->action = centries[0].id;
    }

    if ((splr->field == ITDB_SPLFIELD_VIDEO_KIND) && (ft == ITDB_SPLFT_BINARY_AND))
    {   /* this field needs to be handled differently from everything
	   else */
	if (comboentry_index_from_id (videokind_comboentries_is,
				      splr->fromvalue) != -1)
	{   /* found value to be part of "Is" */
	    index = 0;
	}
	else
	{   /* assume value to be part of "Is Not" */
	    index = 1;
	}
	g_object_set_data (G_OBJECT (combobox), "spl_binary_and_index",
			   GINT_TO_POINTER (index));
	spl_setup_combobox (GTK_COMBO_BOX (combobox), centries, index,
			    G_CALLBACK (spl_action_changed), spl_window);
    }
    else
    {
	spl_set_combobox (GTK_COMBO_BOX (combobox), centries, splr->action,
			  G_CALLBACK (spl_action_changed), spl_window);

    }

    /* input fields (range, string, date...) */
    /* ------------------------------------- */
    at = itdb_splr_get_action_type (splr);
    snprintf (name, WNLEN, "spl_actionhbox%d", row);
    hbox = g_object_get_data (G_OBJECT (table), name);
    if (hbox)
    {
	gtk_widget_destroy (hbox);
	g_object_set_data (G_OBJECT (table), name, NULL);
    }
    if (centries != splaction_notsupported_comboentries)
    {
	g_return_if_fail (at != ITDB_SPLAT_UNKNOWN);
	g_return_if_fail (at != ITDB_SPLAT_INVALID);
	hbox = spl_create_hbox (spl_window, splr);
	gtk_table_attach (table, hbox, 2,3, row,row+1,
			  GTK_FILL,0,   /* expand options */
			  XPAD,YPAD);   /* padding options */
	g_object_set_data (G_OBJECT (table), name, hbox);
    }

    /* +/- buttons */
    /* ----------- */
    snprintf (name, WNLEN, "spl_buttonhbox%d", row);
    hbox = g_object_get_data (G_OBJECT (table), name);
    if (!hbox)
    {
	/* create hbox with buttons */
	hbox = gtk_hbox_new (TRUE, 2);
	gtk_widget_show (hbox);
	g_object_set_data (G_OBJECT (table), name, hbox);
	gtk_table_attach (table, hbox, 3,4, row,row+1,
			  0,0,   /* expand options */
			  XPAD,YPAD);  /* padding options */
    }

    snprintf (name, WNLEN, "spl_button-%d", row);
    button = g_object_get_data (G_OBJECT (table), name);
    if (!button)
    {
	button = gtk_button_new_with_label (_("-"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (spl_button_minus_clicked),
			  spl_window);
	g_object_set_data (G_OBJECT (table), name, button);
    }
    g_object_set_data (G_OBJECT (button), "spl_rule", splr);


    snprintf (name, WNLEN, "spl_button+%d", row);
    button = g_object_get_data (G_OBJECT (table), name);
    if (!button)
    {
	button = gtk_button_new_with_label (_("+"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (spl_button_plus_clicked),
			  spl_window);
	g_object_set_data (G_OBJECT (table), name, button);
	snprintf (name, WNLEN, "spl_button+%d", row);
    }
    g_object_set_data (G_OBJECT (button), "spl_rule", splr);
}

/* Display all rules stored in "spl_work" */
static void spl_display_rules (GtkWidget *spl_window)
{
    Playlist *spl;
    GtkWidget *align, *table;
    GList *gl;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    align = gtkpod_xml_get_widget (spl_window_xml, "spl_rules_table_align");
    g_return_if_fail (align);
    /* Destroy table if it already exists */
    table = g_object_get_data (G_OBJECT (spl_window),
			       "spl_rules_table");
    if (table)  gtk_widget_destroy (table);
    table = gtk_table_new (1, 4, FALSE);
    gtk_widget_show (table);
    gtk_container_add (GTK_CONTAINER (align), table);
    g_object_set_data (G_OBJECT (spl_window),
		       "spl_rules_table", table);

    for (gl=spl->splrules.rules; gl; gl=gl->next)
	spl_update_rule (spl_window, gl->data);

    spl_check_number_of_rules (spl_window);
}


/* destroy widget @wname in row @row of table @table (used by
   spl_update_rules_from_row() */
gboolean splremove (GtkWidget *table, const gchar *wname, gint row)
{
    GtkWidget *w;
    gchar name[WNLEN];
    gboolean removed = FALSE;

    snprintf (name, WNLEN, "%s%d", wname, row);
    w = g_object_get_data (G_OBJECT (table), name);
    if (w)
    {
	gtk_widget_destroy (w);
	g_object_set_data (G_OBJECT (table), name, NULL);
	removed = TRUE;
    }
    return removed;
}


/* Update rules starting in row @row */
static void spl_update_rules_from_row (GtkWidget *spl_window, gint row)
{
    gint i, numrules;
    Playlist *spl;
    GtkWidget *table;
    gboolean removed;

    g_return_if_fail (spl_window);
    spl =  g_object_get_data (G_OBJECT (spl_window), "spl_work");
    g_return_if_fail (spl);
    table = g_object_get_data (G_OBJECT (spl_window), "spl_rules_table");
    g_return_if_fail (table);

    numrules = g_list_length (spl->splrules.rules);

    /* update all rules starting in row @row */
    for (i=row; i<numrules; ++i)
    {
	spl_update_rule (spl_window,
			 g_list_nth_data (spl->splrules.rules, i));
    }
    /* remove rules that do no longer exist */
    for (removed=TRUE; removed==TRUE; ++i)
    {
	removed =  splremove (table, "spl_fieldcombo", i);
	removed |= splremove (table, "spl_actioncombo", i);
	removed |= splremove (table, "spl_actionhbox", i);
	/* remove spl_button+/- BEFORE removing spl_buttonhbox, as
	   removing spl_buttonhbox will destroy the buttons as well --
	   we'd have to g_object_set_data(..., NULL) manually. */
	removed |= splremove (table, "spl_button+", i);
	removed |= splremove (table, "spl_button-", i);
	removed |= splremove (table, "spl_buttonhbox", i);
    }
}


/* Edit a smart playlist. If it is a new smartlist, it will be
 * inserted at position @pos when 'OK' is pressed. */
void spl_edit_all (iTunesDB *itdb, Playlist *spl, gint32 pos)
{
    GtkWidget *spl_window, *w;
    gint defx, defy;
    Playlist *spl_dup;

    g_return_if_fail (spl != NULL);
    g_return_if_fail (spl->is_spl);
    g_return_if_fail (itdb != NULL);

    spl_window_xml = gtkpod_xml_new (xml_file, "spl_window");
    spl_window = gtkpod_xml_get_widget (spl_window_xml, "spl_window");
    
    g_return_if_fail (spl_window != NULL);

    /* Duplicate playlist to work on */
    spl_dup = itdb_playlist_duplicate (spl);

    /* Store pointers to original playlist and duplicate */
    g_object_set_data (G_OBJECT (spl_window), "spl_orig", spl);
    g_object_set_data (G_OBJECT (spl_window), "spl_work", spl_dup);
    g_object_set_data (G_OBJECT (spl_window), "spl_pos",  GINT_TO_POINTER(pos));
    g_object_set_data (G_OBJECT (spl_window), "spl_itdb", itdb);
    /* Set checkboxes and connect signal handlers */
    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_name_entry")))
    {
	if (spl_dup->name)
	    gtk_entry_set_text (GTK_ENTRY (w), spl_dup->name);	
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_all_radio")))
    {
	g_signal_connect (w, "toggled",
			  G_CALLBACK (spl_all_radio_toggled),
			  spl_window);
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (w),
	    (spl_dup->splrules.match_operator == ITDB_SPLMATCH_AND));
    }
    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_any_radio")))
    {
	g_signal_connect (w, "toggled",
			  G_CALLBACK (spl_any_radio_toggled),
			  spl_window);
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (w),
	    (spl_dup->splrules.match_operator == ITDB_SPLMATCH_OR));
    }
    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_none_radio")))
    {
	g_signal_connect (w, "toggled",
			  G_CALLBACK (spl_none_radio_toggled),
			  spl_window);
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (w), !spl_dup->splpref.checkrules);
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_matchcheckedonly_button")))
    {
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (w), spl_dup->splpref.matchcheckedonly);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (spl_matchcheckedonly_toggled),
			  spl_window);
    }

    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_liveupdate_button")))
    {
	gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (w), spl_dup->splpref.liveupdate);
	g_signal_connect (w, "toggled",
			  G_CALLBACK (spl_liveupdate_toggled),
			  spl_window);
    }

    /* Signals for Cancel, OK, Delete */
    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_cancel_button")))
    {
	g_signal_connect (w, "clicked",
		      G_CALLBACK (spl_cancel), spl_window);
    }
    if ((w = gtkpod_xml_get_widget (spl_window_xml, "spl_ok_button")))
    {
	g_signal_connect (w, "clicked",
		      G_CALLBACK (spl_ok), spl_window);
    }
    g_signal_connect (spl_window, "delete_event",
		      G_CALLBACK (spl_delete_event), spl_window);

    spl_display_checklimits (spl_window);

    spl_display_rules (spl_window);

    /* set default size */
    defx = prefs_get_int (SPL_WINDOW_DEFX);
    defy = prefs_get_int (SPL_WINDOW_DEFY);
    if ((defx != 0) && (defy != 0))
	gtk_window_set_default_size (GTK_WINDOW (spl_window), defx, defy);

    gtk_widget_show (spl_window);

    block_widgets ();
}


/* Edit an existing smart playlist */
void spl_edit (Playlist *spl)
{
    g_return_if_fail (spl);
    g_return_if_fail (spl->itdb);
    spl_edit_all (spl->itdb, spl, -1);
}


/* Edit a non-existing smartlist. If successful, it will be entered at
   position @pos. Default name is @name */
void spl_edit_new (iTunesDB *itdb, gchar *name, gint32 pos)
{
    Playlist *spl = gp_playlist_new (name? name:_("New Playlist"), TRUE);

    spl_edit_all (itdb, spl, pos);
}
