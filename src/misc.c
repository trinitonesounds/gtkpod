/* -*- coding: utf-8; -*-
|
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "autodetection.h"
#include "charset.h"
#include "clientserver.h"
#include "file_convert.h"
#include "misc.h"
#include "prefs.h"
#include "misc_track.h"
#include "display_photo.h"
#include "stock_icons.h"

#define DEBUG_MISC 0


/* where to find the scripts */
const gchar *SCRIPTDIR = PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "scripts" G_DIR_SEPARATOR_S;


/*------------------------------------------------------------------*\
 *                                                                  *
 *             Miscellaneous                                        *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Copied g_utf8_strcasestr from GtkSourceView */
gchar *utf8_strcasestr (const gchar *haystack, const gchar *needle)
{
	gsize needle_len;
	gsize haystack_len;
	gchar *ret = NULL;
	gchar *p;
	gchar *casefold;
	gchar *caseless_haystack;
	gint i;

	g_return_val_if_fail (haystack != NULL, NULL);
	g_return_val_if_fail (needle != NULL, NULL);

	casefold = g_utf8_casefold (haystack, -1);
	caseless_haystack = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	needle_len = g_utf8_strlen (needle, -1);
	haystack_len = g_utf8_strlen (caseless_haystack, -1);

	if (needle_len == 0)
	{
		ret = (gchar *)haystack;
		goto finally_1;
	}

	if (haystack_len < needle_len)
	{
		ret = NULL;
		goto finally_1;
	}

	p = (gchar*)caseless_haystack;
	needle_len = strlen (needle);
	i = 0;

	while (*p)
	{
		if ((strncmp (p, needle, needle_len) == 0))
		{
			ret = g_utf8_offset_to_pointer (haystack, i);
			goto finally_1;
		}

		p = g_utf8_next_char (p);
		i++;
	}

finally_1:
	g_free (caseless_haystack);

	return ret;
}

/* Calculate the time in ms passed since @old_time. @old_time is
   updated with the current time if @update is TRUE*/
float get_ms_since (GTimeVal *old_time, gboolean update)
{
    GTimeVal new_time;
    float result;

    g_get_current_time (&new_time);
    result = (new_time.tv_sec - old_time->tv_sec) * 1000 +
	(float)(new_time.tv_usec - old_time->tv_usec) / 1000;
    if (update)
    {
	old_time->tv_sec = new_time.tv_sec;
	old_time->tv_usec = new_time.tv_usec;
    }
    return result;
}

/* parse a bunch of track pointers delimited by \n
 * @s - address of the character string we're parsing (gets updated)
 * @track - pointer the track pointer parsed from the string
 * Returns FALSE when the string is empty, TRUE when the string can still be
 *	parsed
 */
gboolean
parse_tracks_from_string(gchar **s, Track **track)
{
    g_return_val_if_fail (track, FALSE);
    *track = NULL;
    g_return_val_if_fail (s, FALSE);

    if(*s)
    {
	gchar *str = *s;
	gchar *strp = strchr (str, '\n');
	int tokens;

	if (strp == NULL)
	{
	    *track = NULL;
	    *s = NULL;
	    return FALSE;
	}
	tokens = sscanf (str, "%p", track);
	++strp;
	if (*strp) *s = strp;
	else       *s = NULL;
	if (tokens == 1) 	return TRUE;
	else                    return FALSE;
    }
    return FALSE;
}

gboolean
parse_artwork_from_string(gchar **s, Artwork **artwork)
{
    g_return_val_if_fail (artwork, FALSE);
    *artwork = NULL;
    g_return_val_if_fail (s, FALSE);

    if(*s)
    {
	gchar *str = *s;
	gchar *strp = strchr (str, '\n');
	int tokens;

	if (strp == NULL)
	{
	    *artwork = NULL;
	    *s = NULL;
	    return FALSE;
	}
	tokens = sscanf (str, "%p", artwork);
	++strp;
	if (*strp) *s = strp;
	else       *s = NULL;
	if (tokens == 1) 	return TRUE;
	else                    return FALSE;
    }
    return FALSE;
}


/***************************************************************************
 * gtkpod.in,out calls
 *
 **************************************************************************/


/* tries to call "/bin/sh @script" with command line options */
static void do_script (const gchar *script, va_list args)
{
    char *str;
    char **argv;
    GPtrArray *ptra = g_ptr_array_sized_new (10);

    /* prepend args with "sh" and the name of the script */
    g_ptr_array_add (ptra, "sh");
    g_ptr_array_add (ptra, (gpointer)script);
    /* add remaining args */
    while ((str = va_arg (args, char *)))
    {
	g_ptr_array_add (ptra, str);
    }
    g_ptr_array_add (ptra, NULL);
    argv = (char **)g_ptr_array_free (ptra, FALSE);

    if (script)
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	case 0: /* child */
	    execv("/bin/sh", argv);
	    exit(0);
	break;
	case -1: /* parent and error */
	break;
	default: /* parent -- let's wait for the child to terminate */
	    tpid = waitpid (pid, &status, 0);
	    /* we could evaluate tpid and status now */
	    break;
	}
    }
    g_free (argv);
}


/* tries to execute "/bin/sh ~/.gtkpod/@script" or
 * "/bin/sh /etc/gtkpod/@script" if the former does not exist. This
 * function accepts command line arguments that must be terminated by
 * NULL. */
void call_script (gchar *script, ...)
{
    gchar *cfgdir;
    va_list args;
    gchar *file;

    if (!script) return;

    cfgdir =  prefs_get_cfgdir ();
    file = g_build_filename (cfgdir, script, NULL);

    va_start (args, script);
    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
	do_script (file, args);
    }
    else
    {
	C_FREE (file);
	file = g_build_filename ("/etc/gtkpod/", script, NULL);
	if (g_file_test (file, G_FILE_TEST_EXISTS))
	{
	    do_script (file, args);
	}
    }
    va_end (args);

    g_free (file);
    g_free (cfgdir);
}



/* Create a NULL-terminated array of strings given in the command
   line. The last argument must be NULL.

   As a special feature, the first argument is split up into
   individual strings to allow the use of "convert-2mp3 -q <special
   settings>". Set the first argument to NULL if you don't want this.

   You must free the returned array with g_strfreev() after use. */
gchar **build_argv_from_strings (const gchar *first_arg, ...)
{
    gchar **argv;
    va_list args;
    const gchar *str;
    GPtrArray *ptra = g_ptr_array_sized_new (20);

    if (first_arg)
    {
	gchar **strings = g_strsplit (first_arg, " ", 0);
	gchar **strp = strings;
	while (*strp)
	{
	    if (**strp)
	    {   /* ignore empty strings */
		g_ptr_array_add (ptra, g_strdup(*strp));
	    }
	    ++strp;
	}
	g_strfreev (strings);
    }

    va_start (args, first_arg);
    do
    {
	str = va_arg (args, const gchar *);
	g_ptr_array_add (ptra, g_strdup (str));
    }
    while (str);

    va_end (args);

    argv = (gchar **)g_ptr_array_free (ptra, FALSE);

    return argv;
}




/* compare @str1 and @str2 case-sensitively or case-insensitively
 * depending on prefs settings */
gint compare_string (const gchar *str1, const gchar *str2)
{
    gint result;
    gchar *sortkey1 = make_sortkey (str1);
    gchar *sortkey2 = make_sortkey (str2);

    result = strcmp (sortkey1, sortkey2);

    g_free (sortkey1);
    g_free (sortkey2);
    return result;
}

struct csfk
{
    gint length;
    gchar *key;
};

static GList *csfk_list = NULL;


/* Returns the sortkey for an entry name.
 *
 * The sort key can be compared with other sort keys using strcmp and
 * it will give the expected result, according to the user
 * settings. Must be regenerated if the user settings change.
 *
 * The caller is responsible of freeing the returned key with g_free.
 */
gchar *
make_sortkey (const gchar *name)
{
    if (prefs_get_int("case_sensitive"))
    {
	return g_utf8_collate_key (name, -1);
    }
    else
    {
	gchar *casefolded = g_utf8_casefold (name, -1);
	gchar *key = g_utf8_collate_key (casefolded, -1);
	g_free (casefolded);
	return key;
    }
}

/* needs to be called everytime the sort_ign_strings in the prefs were
   changed */
void compare_string_fuzzy_generate_keys (void)
{
    GList *gl;
    GList *sort_ign_strings;
    GList *current;

    /* remove old keys */
    for (gl=csfk_list; gl; gl=gl->next)
    {
	struct csfk *csfk = gl->data;
	g_return_if_fail (csfk);
	g_free (csfk->key);
	g_free (csfk);
    }
    g_list_free (csfk_list);
    csfk_list = NULL;

    /* create new keys */
    sort_ign_strings = prefs_get_list("sort_ign_string_");
    current = sort_ign_strings;
    while (current)
    {
	gchar *str = current->data;
	struct csfk *csfk;
	gchar *tempStr;
	
	current = g_list_next(current);

	csfk = g_malloc (sizeof (struct csfk));
	tempStr = g_utf8_casefold (str, -1 );
	csfk->length = g_utf8_strlen (tempStr, -1 );
	csfk->key = g_utf8_collate_key (tempStr, -1 );
	g_free (tempStr);

	csfk_list = g_list_prepend (csfk_list, csfk);
    }
    prefs_free_list(sort_ign_strings);
}

/* Returns a pointer inside the name, possibly skiping a prefix from
 * the list generated by compare_string_fuzzy_generate_keys.
 */
const gchar *
fuzzy_skip_prefix (const gchar *name)
{
    const gchar *result = name;
    const GList *gl;
    gchar *cleanStr;

    /* If the article collations keys have not been generated, 
     * do that first
     */
    if (!csfk_list)
	compare_string_fuzzy_generate_keys ();

    cleanStr = g_utf8_casefold (name, -1);

    for (gl=csfk_list; gl; gl=g_list_next(gl))
    {
	struct csfk *csfk = gl->data;
	gchar *tempStr;

	g_return_val_if_fail (csfk, 0);

    	tempStr = g_utf8_collate_key (cleanStr, csfk->length);
	if (strcmp (tempStr, csfk->key) == 0)
	{
	    /* Found article, bump pointers ahead appropriate distance
	     */
	    result += csfk->length;
	    g_free (tempStr);
	    break;
	}
	g_free (tempStr);
    }

    g_free (cleanStr);

    return result;
}

/* compare @str1 and @str2 case-sensitively or case-insensitively
 * depending on prefs settings, and ignoring certain initial articles 
 * ("the", "le"/"la", etc) */
gint compare_string_fuzzy (const gchar *str1, const gchar *str2)
{
    return compare_string (fuzzy_skip_prefix (str1),
			   fuzzy_skip_prefix (str2));
}

/* compare @str1 and @str2 case-insensitively */
gint compare_string_case_insensitive (const gchar *str1, const gchar *str2)
{
    gchar *string1 = g_utf8_casefold (str1, -1);
    gchar *string2 = g_utf8_casefold (str2, -1);
    gint result = g_utf8_collate (string1, string2);
    g_free (string1);
    g_free (string2);
    return result;
}

/* todo: optionally ignore 'the', 'a,' etc. */
gboolean compare_string_start_case_insensitive (const gchar *haystack, const gchar *needle)
{
  gint cmp = 0;
  gchar *nhaystack = g_utf8_normalize(haystack, -1, G_NORMALIZE_ALL);
  gchar *lhaystack = g_utf8_casefold(nhaystack,-1);
  gchar *nneedle = g_utf8_normalize(needle, -1, G_NORMALIZE_ALL);
  gchar *lneedle = g_utf8_casefold(nneedle,-1);


  cmp = strncmp(lhaystack, lneedle, strlen(lneedle));

  /*
    printf("searched for %s , matching against %s with %d bytes. say=%d\n", 
    lneedle, lhaystack, strlen(lneedle), cmp);
  */
  
  g_free(nhaystack);
  g_free(lhaystack);
  g_free(nneedle);
  g_free(lneedle);
  return cmp;
};


/* ------------------------------------------------------------
------------------------------------------------------------------
--------                                                 ---------
--------                 UTF16 section                   ---------
--------                                                 ---------
------------------------------------------------------------------
   ------------------------------------------------------------ */

/* Get length of utf16 string in number of characters (words) */
guint32 utf16_strlen (gunichar2 *utf16)
{
  guint32 i=0;
  if (utf16)
      while (utf16[i] != 0) ++i;
  return i;
}

/* duplicate a utf16 string */
gunichar2 *utf16_strdup (gunichar2 *utf16)
{
    guint32 len;
    gunichar2 *new = NULL;

    if (utf16)
    {
	len = utf16_strlen (utf16);
	new = g_malloc (sizeof (gunichar2) * (len+1));
	if (new) memcpy (new, utf16, sizeof (gunichar2) * (len+1));
    }
    return new;
}




/*------------------------------------------------------------------*\
 *                                                                  *
 *  Generic functions to handle options in pop-up requesters        *
 *                                                                  *
\*------------------------------------------------------------------*/




/* Set the toggle button to active that is specified by @prefs_string
   (integer value). If no parameter is set in the prefs, use
   @dflt. The corresponding widget names are stored in an array
   @widgets and are member of @win */
void option_set_radio_button (GladeXML *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets,
			      gint dflt)
{
    gint wnum, num=0;
    GtkWidget *w;

    g_return_if_fail (win_xml && prefs_string && widgets);

    /* number of available widgets */
    num=0;
    while (widgets[num]) ++num;

    if (!prefs_get_int_value (prefs_string, &wnum))
	wnum = dflt;

    if ((wnum >= num) || (wnum < 0))
    {
	fprintf (stderr, "Programming error: wnum > num (%d,%d,%s)\n",
		 wnum, num, prefs_string);
	/* set to reasonable default value */
	prefs_set_int (prefs_string, 0);
	wnum = 0;
    }
    w = gtkpod_xml_get_widget (win_xml, widgets[wnum]);
    if (w)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
}


/* Retrieve which toggle button was activated and store the state in
 * the prefs */
gint option_get_radio_button (GladeXML *win_xml,
			      const gchar *prefs_string,
			      const gchar **widgets)
{
    gint i;

    g_return_val_if_fail (win_xml && prefs_string && widgets, 0);

    for (i=0; widgets[i]; ++i)
    {
	GtkWidget *w = gtkpod_xml_get_widget (win_xml, widgets[i]);
	if (w)
	{
	    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		break;
	}
    }
    if (!widgets[i])
    {
	fprintf (stderr, "Programming error: no active toggle button (%s)", prefs_string);
	/* set reasonable default */
	i=0;
    }
    prefs_set_int (prefs_string, i);
    return i;
}


/* Set the current folder to what is stored in the prefs */
void option_set_folder (GtkFileChooser *fc, const gchar *prefs_string)
{
    gchar *folder;

    g_return_if_fail (fc && prefs_string);

    prefs_get_string_value (prefs_string, &folder);
    if (!folder)
	folder = g_strdup (g_get_home_dir ());
    gtk_file_chooser_set_current_folder (fc, folder);
    g_free (folder);
}


/* Retrieve the current folder and write it to the prefs */
/* If @value is != NULL, a copy of the folder is placed into
   @value. It has to be g_free()d after use */
void option_get_folder (GtkFileChooser *fc,
			const gchar *prefs_string,
			gchar **value)
{
    gchar *folder;

    g_return_if_fail (fc && prefs_string);

    folder = gtk_file_chooser_get_current_folder (fc);
    prefs_set_string (prefs_string, folder);

    if (value) *value = folder;
    else       g_free (folder);
}


/* Set the current filename to what is stored in the prefs */
void option_set_filename (GtkFileChooser *fc, const gchar *prefs_string)
{
    gchar *filename;

    g_return_if_fail (fc && prefs_string);

    prefs_get_string_value (prefs_string, &filename);
    if (!filename)
	filename = g_strdup (g_get_home_dir ());
    gtk_file_chooser_set_current_name (fc, filename);
    g_free (filename);
}


/* Retrieve the current filename and write it to the prefs */
/* If @value is != NULL, a copy of the filename is placed into
   @value. It has to be g_free()d after use */
void option_get_filename (GtkFileChooser *fc,
			  const gchar *prefs_string,
			  gchar **value)
{
    gchar *filename;

    g_return_if_fail (fc && prefs_string);

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(fc));
    prefs_set_string (prefs_string, filename);

    if (value) *value = filename;
    else       g_free (filename);
}


/* Set the string entry @name to the prefs value stored in @name or
   to @default if @name is not yet defined. */
void option_set_string (GladeXML *win_xml,
			const gchar *name,
			const gchar *dflt)
{
    gchar *string;
    GtkWidget *entry;

    g_return_if_fail (win_xml && name && dflt);

    prefs_get_string_value (name, &string);

    if (!string)
	string = g_strdup (dflt);

    entry = gtkpod_xml_get_widget (win_xml, name);

    if (entry)
	gtk_entry_set_text(GTK_ENTRY(entry), string);

    g_free (string);
}

/* Retrieve the current content of the string entry @name and write it
 * to the prefs (@name) */
/* If @value is != NULL, a copy of the string is placed into
   @value. It has to be g_free()d after use */
void option_get_string (GladeXML *win_xml,
			const gchar *name,
			gchar **value)
{
    GtkWidget *entry;

    g_return_if_fail (win_xml && name);

    entry = gtkpod_xml_get_widget (win_xml, name);

    if (entry)
    {
	const gchar *str = gtk_entry_get_text (GTK_ENTRY (entry));
	prefs_set_string (name, str);
	if (value) *value = g_strdup (str);
    }
}


/* Set the state of toggle button @name to the prefs value stored in
   @name or to @default if @name is not yet defined. */
void option_set_toggle_button (GladeXML *win_xml,
			       const gchar *name,
			       gboolean dflt)
{
    gboolean active;
    GtkWidget *button;

    g_return_if_fail (win_xml && name);

    if (!prefs_get_int_value (name, &active))
	active = dflt;

    button = gtkpod_xml_get_widget (win_xml, name);

    if (button)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button),
				      active);
}

/* Retrieve the current state of the toggle button @name and write it
 * to the prefs (@name) */
/* Return value: the current state */
gboolean option_get_toggle_button (GladeXML *win_xml,
				   const gchar *name)
{
    gboolean active = FALSE;
    GtkWidget *button;

    g_return_val_if_fail (win_xml && name, active);

    button = gtkpod_xml_get_widget (win_xml, name);

    if (button)
    {
	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button));
	prefs_set_int (name, active);
    }
    return active;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *  Functions to create string/filename from a template             *
 *                                                                  *
\*------------------------------------------------------------------*/




/*
|  Copyright (C) 2004 Ero Carrera <ero at dkbza.org>
|
|  Placed under GPL in agreement with Ero Carrera. (JCS -- 12 March 2004)
*/

/**
 * Check if supported char and return substitute.
 */
static gchar check_char(gchar c)
{
    gint i;
    static const gchar 
	invalid[]={'"', '*', ':', '<', '>', '?', '\\', '|', '/', 0};
    static const gchar
	replace[]={'_', '_', '-', '_', '_', '_', '-',  '-', '-', 0};
    for(i=0; invalid[i]!=0; i++)
	if(c==invalid[i])  return replace[i];
    return c;
}

/**
 * Process a path. It will substitute all the invalid characters.
 * The changes are made within the original string. A pointer to the
 * original string is returned.
 */
static gchar *fix_path(gchar *orig)
{
        if(orig)
	{
	    gchar *op = orig;
	    while (*op)
	    {
		*op = check_char(*op);
		++op;
	    }
	}
	return orig;
}

/* End of code originally supplied by Ero Carrera */


/* Match a list of templates @p separated by ';' with the type of the
   filename. E.g. '%s.mp3;%t.wav' will return '%s.mp3' if @track is an
   mp3 file, or '%t.wav' if @track is a wav file. If no template can
   be matched, an empty string is returned.

   String be freed after use.
*/
static gchar *select_template (Track *track, const gchar *p)
{
    gchar **templates, **tplp;
    gchar *ext = NULL;
    const gchar *tname;
    gchar *result;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, strdup (""));
    etr = track->userdata;
    g_return_val_if_fail (etr, strdup (""));
    if (etr->pc_path_locale && strlen(etr->pc_path_locale))
	tname = etr->pc_path_locale;
    else
	tname = track->ipod_path;
    if (!tname)
    {   /* this should not happen... */
	gchar *buf = get_track_info (track, TRUE);
	gtkpod_warning (_("Could not process '%s' (no filename available)"),
			buf);
	g_free (buf);
    }
    ext = strrchr (tname, '.');        /* pointer to filename extension */

    templates = g_strsplit (p, ";", 0);
    tplp = templates;
    while (*tplp)
    {
	if (strcmp (*tplp, "%o") == 0)
	{   /* this is only a valid extension if the original filename
	       is present */
	    if (etr->pc_path_locale && strlen(etr->pc_path_locale))  break;
	}
	else if (strrchr (*tplp, '.') == NULL)
	{   /* this template does not have an extension and therefore
	     * matches */
	    if (ext)
	    {   /* if we have an extension, add it */
		gchar *str = g_strdup_printf ("%s%s", *tplp, ext);
		g_free (*tplp);
		*tplp = str;
	    }
	    break;
	}
	else if (ext && (strlen (*tplp) >= strlen (ext)))
	{  /* this template is valid if the extensions match */
	    if (strcasecmp (&((*tplp)[strlen (*tplp) - strlen (ext)]),
			    ext) == 0)
		break;
	}
	++tplp;
    }
    result = g_strdup (*tplp);
    g_strfreev (templates);
    return result;
}


/* Return a string for @track built according to @template.

   @is_filename: if TRUE, remove potentially harmful characters.
   @silent: don't print error messages (no gtk_*() calls -- thread
   safe)
*/
gchar *get_string_from_template (Track *track,
				 const gchar *template,
				 gboolean is_filename,
				 gboolean silent)
{
    GString *result;
    gchar *res_utf8;
    const gchar *p;
    gchar *basename = NULL;
    gchar *basename_noext = NULL;
    ExtraTrackData *etr;

    g_return_val_if_fail (track, NULL);
    g_return_val_if_fail (template, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    result = g_string_new ("");

    /* try to get the original filename */
    if (etr->pc_path_utf8)
	basename = g_path_get_basename (etr->pc_path_utf8);
    /* get original filename without extension */
    if (basename)
    {
	gchar *ptr;
	basename_noext = g_strdup (basename);
	ptr = strrchr (basename_noext, '.');
	if (ptr) *ptr = '\0';
    }

    p=template;
    while (*p != '\0') {
	if (*p == '%') {
	    const gchar* tmp = NULL;
	    gchar dummy[100];
	    Playlist *pl;
	    p++;
	    switch (*p) {
	    case 'o':
		if (basename)
		{
		    tmp = basename;
		}
		break;
	    case 'O':
		if (basename_noext)
		{
		    tmp = basename_noext;
		}
		break;
	    case 'p':
		pl = pm_get_selected_playlist ();
		if (pl)
		    tmp = pl->name;
		break;
	    case 'a':
		tmp = track_get_item (track, T_ARTIST);
		break;
	    case 'A':
		tmp = track_get_item (track, T_ALBUM);
		break;
	    case 't':
		tmp = track_get_item (track, T_TITLE);
		break;
	    case 'c':
		tmp = track_get_item (track, T_COMPOSER);
		break;
	    case 'g':
	    case 'G':
		tmp = track_get_item (track, T_GENRE);
		break;
	    case 'C':
		if (track->cds == 0)
		    sprintf (dummy, "%.2d", track->cd_nr);
		else if (track->cds < 10)
		    sprintf(dummy, "%.1d", track->cd_nr);
		else if (track->cds < 100)
		    sprintf (dummy, "%.2d", track->cd_nr);
		else if (track->cds < 1000)
		    sprintf (dummy, "%.3d", track->cd_nr);
		else
		    sprintf (dummy,"%.4d", track->cd_nr);
		tmp = dummy;
		break;
	    case 'T':
		if (track->tracks == 0)
		    sprintf (dummy, "%.2d", track->track_nr);
		else if (track->tracks < 10)
		    sprintf(dummy, "%.1d", track->track_nr);
		else if (track->tracks < 100)
		    sprintf (dummy, "%.2d", track->track_nr);
		else if (track->tracks < 1000)
		    sprintf (dummy, "%.3d", track->track_nr);
		else
		    sprintf (dummy,"%.4d", track->track_nr);
		tmp = dummy;
		break;
	    case 'Y':
		sprintf (dummy, "%4d", track->year);
		tmp = dummy;
		break;
	    case '%':
		tmp = "%";
		break;
	    default:
		if (!silent)
		{
		    gtkpod_warning (_("Unknown token '%%%c' in template '%s'"),
				    *p, template);
		}
		break;
	    }
	    if (tmp)
	    {
		gchar *tmpcp = g_strdup (tmp);
		if (is_filename)
		{
		    /* remove potentially illegal/harmful characters */
		    fix_path (tmpcp);
		    /* strip spaces to avoid problems with vfat */
		    g_strstrip (tmpcp);
		    /* append to current string */
		}
		result = g_string_append (result, tmpcp);
		tmp = NULL;
		g_free (tmpcp);
	    }
	}
	else 
	    result = g_string_append_c (result, *p);
	p++;
    }
    /* get the utf8 version of the filename */
    res_utf8 = g_string_free (result, FALSE);

    if (is_filename)
    {   /* remove white space before the filename extension
	   (last '.') */
	gchar *ext = strrchr (res_utf8, '.');
	gchar *extst = NULL;
	if (ext)
	{
	    extst = g_strdup (ext);
	    *ext = '\0';
	}
	g_strstrip (res_utf8);
	if (extst)
	{
	    /* The following strcat() is safe because g_strstrip()
	       does not increase the original string size. Therefore
	       the result of the strcat() call will not be longer than
	       the original string. */
	    strcat (res_utf8, extst);
	    g_free (extst);
	}
    }

    g_free (basename);
    g_free (basename_noext);

    return res_utf8;
}



/* Return a string for @track built according to @full_template.
   @full_template can contain several templates separated by ';',
   e.g. '%s.mp3;%t.wav'. The correct one is selected using
   select_template() defined above.

   If @is_filename is TRUE, potentially harmful characters are
   replaced in an attempt to create a valid filename.

   If @is_filename is FALSE, the extension (e.g. '.mp3' will be
   removed). */
gchar *get_string_from_full_template (Track *track,
				      const gchar *full_template,
				      gboolean is_filename)
{
    gchar *res_utf8;
    gchar *template;

    g_return_val_if_fail (track, NULL);
    g_return_val_if_fail (full_template, NULL);

    template = select_template (track, full_template);

    if (!template)
    {
	gchar *fn = get_file_name_from_source (track, SOURCE_PREFER_LOCAL);
	gtkpod_warning (_("Template ('%s') does not match file type '%s'\n"), full_template, fn ? fn:"");
	g_free (fn);
	return NULL;
    }

    if (!is_filename)
    {   /* remove an extension, if present ('.???' or '.????'  at the
	   end) */
	gchar *pnt = strrchr (template, '.');
	if (pnt)
	{
	    if (pnt == template+strlen(template)-3)
		*pnt = 0;
	    if (pnt == template+strlen(template)-4)
		*pnt = 0;
	}
    }

    res_utf8 = get_string_from_template (track, template, is_filename, FALSE);

    g_free (template);

    return res_utf8;
}


/**
 * which - run the shell command which, useful for querying default values
 * for executable,
 * @name - the executable we're trying to find the path for
 * Returns the path to the executable, NULL on not found
 */
gchar *which (const gchar *exe)
{
    FILE *fp = NULL;
    gchar *result = NULL;
    gchar buf[PATH_MAX];
    gchar *which_exec = NULL;

    g_return_val_if_fail (exe, NULL);

    memset(&buf[0], 0, PATH_MAX);
    which_exec = g_strdup_printf("which %s", exe);
    if((fp = popen(which_exec, "r")))
    {
        int read_bytes = 0;
        if((read_bytes = fread(buf, sizeof(gchar), PATH_MAX, fp)) > 0)
            result = g_strndup(buf, read_bytes-1);
        pclose(fp);
    }
    g_free(which_exec);
    return(result);
}

/**
 * Recursively make directories.
 *
 * @silent: don't print error messages via gtk (->thread safe)
 *
 * @return FALSE is this is not possible.
 */
gboolean mkdirhier(const gchar *dirname, gboolean silent)
{
    gchar *dn, *p;

    g_return_val_if_fail (dirname && *dirname, FALSE);

    if (strncmp ("~/", dirname, 2) == 0)
	 dn = g_build_filename (g_get_home_dir(), dirname+2, NULL);
    else dn = g_strdup (dirname);

    p = dn;

    do
    {
	++p;
	p = index (p, G_DIR_SEPARATOR);

	if (p)   *p = '\0';

	if (!g_file_test(dn, G_FILE_TEST_EXISTS))
	{
	    if (g_mkdir(dn, 0777) == -1)
	    {
		if (!silent)
		{
		    gtkpod_warning (_("Error creating %s: %s\n"),
				    dn, g_strerror(errno));
		}
		g_free (dn);
		return FALSE;
	    }
	}
	if (p)   *p = G_DIR_SEPARATOR;
    } while (p);

    g_free (dn);
    return TRUE;
}

/**
 * Recursively make directories in the given filename.
 * @return FALSE is this is not possible.
 */
gboolean mkdirhierfile(const gchar *filename)
{
    gboolean result;
    gchar *dirname = g_path_get_dirname (filename);
    result = mkdirhier (dirname, FALSE);
    g_free (dirname);
    return result;
}


/**
 * Convert "~/" to "/home/.../"
 *
 * g_free() return value when no longer needed.
 */
gchar *convert_filename (const gchar *filename)
{
  if (filename)
  {    
    if (strncmp ("~/", filename, 2) == 0)
      return g_build_filename (g_get_home_dir(), filename+2, NULL);
    else 
      return g_strdup (filename);
  }
  
  return NULL;
}


/**
 * get_size_of_directory
 *
 * Determine the total size in bytes of all files in @dir including
 * subdirectories. This function ignores errors in the sense that if a
 * directory or file cannot be accessed, a size of 0 is assumed.
 */
gint64 get_size_of_directory (const gchar *dir)
{
    GDir *gdir;
    const gchar *fname;
    gint64 tsize = 0;

    g_return_val_if_fail (dir, 0);

    gdir = g_dir_open (dir, 0, NULL);

    /* Check for error */
    if (!gdir)
	return 0;

    while ((fname = g_dir_read_name (gdir)))
    {
	gchar *fullname = g_build_filename (dir, fname, NULL);
	if (g_file_test (fullname, G_FILE_TEST_IS_DIR))
	{
	    tsize += get_size_of_directory (fullname);
	}
	else if (g_file_test (fullname, G_FILE_TEST_IS_REGULAR))
	{
	    struct stat statbuf;
	    if (g_stat (fullname, &statbuf) == 0)
	    {   /* OK, add size */
		tsize += statbuf.st_size;
	    }
	}
	g_free (fullname);
    }

    g_dir_close (gdir);

    return tsize;
}



/**
 * Wrapper for glade_xml_new() for cygwin compatibility issues
 *
 **/
GladeXML *gtkpod_xml_new (const gchar *xml_file, const gchar *name)
{
    GladeXML *xml;
	
#ifdef ENABLE_NLS
	xml = glade_xml_new (xml_file, name, GETTEXT_PACKAGE);
#else
	xml = glade_xml_new (xml_file, name, NULL);
#endif

    if (!xml)
		fprintf (stderr, "*** Programming error: Cannot create glade XML: '%s'\n",
				 name);

    return xml;
}


/**
 * Wrapper for gtkpod_xml_get_widget() giving out a warning if widget
 * could not be found.
 *
 **/
GtkWidget *gtkpod_xml_get_widget (GladeXML *xml, const gchar *name)
{
    GtkWidget *w = glade_xml_get_widget (xml, name);

    if (!w)
		fprintf (stderr, "*** Programming error: Widget not found: '%s'\n",
				 name);

    return w;
}

/* ------------------------------------------------------------
 *
 *        Helper functions for pref keys
 *
 * ------------------------------------------------------------ */


/**
 * Helper function to construct prefs key for itdb,
 * e.g. itdb_1_mountpoint...
 * gfree() after use
 *
 **/
gchar *get_itdb_prefs_key (gint index, const gchar *subkey)
{
    g_return_val_if_fail (subkey, NULL);

    return g_strdup_printf ("itdb_%d_%s", index, subkey);
}


/**
 * Helper function to construct prefs key for playlists,
 * e.g. itdb_1_playlist_xxxxxxxxxxx_syncmode...
 *
 * gfree() after use
 **/
gchar *get_playlist_prefs_key (gint index,
			       Playlist *pl, const gchar *subkey)
{
    g_return_val_if_fail (pl, NULL);
    g_return_val_if_fail (subkey, NULL);

    return g_strdup_printf ("itdb_%d_playlist_%llu_%s",
			    index, (unsigned long long)pl->id, subkey);
}


/**
 * Helper function to retrieve the index number of @itdb needed to
 * construct any keys (see above)
 **/
gint get_itdb_index (iTunesDB *itdb)
{
	struct itdbs_head *itdbs_head;

	itdbs_head = gp_get_itdbs_head (gtkpod_window);
	g_return_val_if_fail (itdbs_head, 0);
	
	return g_list_index (itdbs_head->itdbs, itdb);
}


/**
 * Helper function to retrieve a string prefs entry for @itdb.
 * 
 * gfree() after use
 **/
gchar *get_itdb_prefs_string (iTunesDB *itdb, const gchar *subkey)
{
    gchar *key, *value;

    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (subkey, NULL);

    key = get_itdb_prefs_key (get_itdb_index (itdb), subkey);
    value = prefs_get_string (key);
    g_free (key);

    return value;
}


/**
 * Helper function to retrieve a string prefs entry for @playlist.
 * 
 **/
gchar *get_playlist_prefs_string (Playlist *playlist, const gchar *subkey)
{
    gchar *key, *value;

    g_return_val_if_fail (playlist, 0);
    g_return_val_if_fail (subkey, 0);

    key = get_playlist_prefs_key (get_itdb_index (playlist->itdb),
				  playlist, subkey);
    value = prefs_get_string (key);
    g_free (key);

    return value;
}

/**
 * Helper function to retrieve an int prefs entry for @itdb.
 * 
 **/
gint get_itdb_prefs_int (iTunesDB *itdb, const gchar *subkey)
{
    gchar *key;
    gint value;

    g_return_val_if_fail (itdb, 0);
    g_return_val_if_fail (subkey, 0);

    key = get_itdb_prefs_key (get_itdb_index (itdb), subkey);
    value = prefs_get_int (key);
    g_free (key);

    return value;
}

/**
 * Helper function to retrieve an int prefs entry for @playlist.
 * 
 **/
gint get_playlist_prefs_int (Playlist *playlist, const gchar *subkey)
{
    gchar *key;
    gint value;

    g_return_val_if_fail (playlist, 0);
    g_return_val_if_fail (subkey, 0);

    key = get_playlist_prefs_key (get_itdb_index (playlist->itdb),
				  playlist, subkey);
    value = prefs_get_int (key);
    g_free (key);

    return value;
}

/**
 * Helper function to retrieve a string prefs entry for @itdb.
 *
 * Returns TRUE if the key was actually set in the prefs. 
 * 
 * gfree() after use
 **/
gboolean get_itdb_prefs_string_value (iTunesDB *itdb, const gchar *subkey,
				      gchar **value)
{
    gchar *key;
    gboolean result;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (subkey, FALSE);

    key = get_itdb_prefs_key (get_itdb_index (itdb), subkey);
    result = prefs_get_string_value (key, value);
    g_free (key);

    return result;
}


/**
 * Helper function to retrieve an in prefs entry for @itdb.
 * 
 **/
gboolean get_itdb_prefs_int_value (iTunesDB *itdb, const gchar *subkey,
				   gint *value)
{
    gchar *key;
    gboolean result;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (subkey, FALSE);

    key = get_itdb_prefs_key (get_itdb_index (itdb), subkey);
    result = prefs_get_int_value (key, value);
    g_free (key);

    return result;
}


/**
 * Helper function to set a string prefs entry for @itdb.
 * 
 * gfree() after use
 **/
void set_itdb_prefs_string (iTunesDB *itdb,
			    const gchar *subkey, const gchar *value)
{
    gchar *key;

    g_return_if_fail (itdb);
    g_return_if_fail (subkey);

    key = get_itdb_prefs_key (get_itdb_index (itdb), subkey);
    prefs_set_string (key, value);
    g_free (key);
}


/**
 * Helper function to set a string prefs entry for @itdb.
 * 
 * gfree() after use
 **/
void set_itdb_index_prefs_string (gint index,
				  const gchar *subkey, const gchar *value)
{
    gchar *key;

    g_return_if_fail (subkey);

    key = get_itdb_prefs_key (index, subkey);
    prefs_set_string (key, value);
    g_free (key);
}


/**
 * Helper function to set an in prefs entry for @itdb.
 * 
 **/
void set_itdb_prefs_int (iTunesDB *itdb, const gchar *subkey, gint value)
{
    gchar *key;

    g_return_if_fail (itdb);
    g_return_if_fail (subkey);

    key = get_itdb_prefs_key (get_itdb_index (itdb), subkey);
    prefs_set_int (key, value);
    g_free (key);
}


/**
 * Helper function to set an in prefs entry for @itdb.
 * 
 **/
void set_itdb_index_prefs_int (gint index,
			       const gchar *subkey, gint value)
{
    gchar *key;

    g_return_if_fail (subkey);

    key = get_itdb_prefs_key (index, subkey);
    prefs_set_int (key, value);
    g_free (key);
}

/**
 * Helper function to remove all prefs strings for itdb @index and
 * move all subsequent itdb strings forward. You can call this even
 * after your have removed the itdb from itdbs_head.
 * 
 **/
static void remove_itdb_index_prefs (gint index)
{
    struct itdbs_head *itdbs_head;
    gchar *subkey;
    gint i, n;

    itdbs_head = gp_get_itdbs_head (gtkpod_window);
    g_return_if_fail (itdbs_head);

    n = g_list_length (itdbs_head->itdbs);
    subkey = get_itdb_prefs_key (index, "");
    prefs_flush_subkey (subkey);
    g_free (subkey);
    
    for (i=index; i<=n; ++i)
    {
	gchar *from_key = get_itdb_prefs_key (i+1, "");
	gchar *to_key = get_itdb_prefs_key (i, "");
	prefs_rename_subkey (from_key, to_key);
	g_free (from_key);
	g_free (to_key);
    }
}


/**
 * Helper function to remove all prefs strings for @itdb and move all
 * subsequent itdb strings forward. Call this before removing the itdb
 * from itdbs_head.
 * 
 **/
void remove_itdb_prefs (iTunesDB *itdb)
{
    g_return_if_fail (itdb);

    remove_itdb_index_prefs (get_itdb_index (itdb));
}

/* Save all itdb_<index>_* keys to the iPod
 * (<ControlDir>/gtkpod.prefs).
 *
 * Return value: TRUE on succes, FALSE on error
 */
gboolean save_ipod_index_prefs (gint index, const gchar *mountpoint)
{
    TempPrefs *temp_prefs;
    gboolean result = FALSE;
    gchar *subkey, *dir;

    g_return_val_if_fail (mountpoint, FALSE);

    /* isolate all 'itdb_<index>_*' keys */
    subkey = get_itdb_prefs_key (index, "");
    temp_prefs = prefs_create_subset (subkey);

    /* rename to 'itdb_*' */
    temp_prefs_rename_subkey (temp_prefs, subkey, "itdb_");

    /* remove some keys */
    temp_prefs_remove_key (temp_prefs, "itdb_mountpoint");
    temp_prefs_remove_key (temp_prefs, "itdb_name");
    temp_prefs_remove_key (temp_prefs, "itdb_type");

    /* build filename path */
    dir = itdb_get_itunes_dir (mountpoint);
    if (dir)
    {
	GError *error = NULL;
	gchar *path = g_build_filename (dir, "gtkpod.prefs", NULL);
	result = temp_prefs_save (temp_prefs, path, &error);
	if (result == FALSE)
	{
	    gtkpod_warning (_("Writing preferences file '%s' failed (%s).\n\n"),
			    path,
			    error? error->message:_("unspecified error"));
	    g_error_free (error);
	}
	g_free (path);
	g_free (dir);
    }
    else
    {
	gtkpod_warning (_("Writing preferences to the iPod (%s) failed: could not get path to Control Directory.\n\n"),
			mountpoint);
    }

    temp_prefs_destroy (temp_prefs);
    g_free (subkey);

    return result;
}



/* Save all itdb_<index>_* keys to the iPod
 * (<ControlDir>/gtkpod.prefs).
 *
 * Return value: TRUE on succes, FALSE on error
 */
gboolean save_ipod_prefs (iTunesDB *itdb, const gchar *mountpoint)
{
    g_return_val_if_fail (itdb && mountpoint, FALSE);
    return save_ipod_index_prefs (get_itdb_index (itdb), mountpoint);
}


/* Load preferences file from the iPod and merge them into the general
 * prefs system.
 */
static void load_ipod_index_prefs (gint index, const gchar *mountpoint)
{
    gchar *dir;

    g_return_if_fail (mountpoint);

    /* build filename path */
    dir = itdb_get_itunes_dir (mountpoint);
    if (dir)
    {
	TempPrefs *temp_prefs;
	GError *error = NULL;
	gchar *path = g_build_filename (dir, "gtkpod.prefs", NULL);
	temp_prefs = temp_prefs_load (path, &error);
	g_free (path);
	if (temp_prefs)
	{
	    gchar *subkey;
	    subkey = get_itdb_prefs_key (index, "");
	    /* rename 'itdb_*' to 'itdb_<index>_*' */
	    temp_prefs_rename_subkey (temp_prefs, "itdb_", subkey);
	    g_free (subkey);
	    /* merge with real prefs */
	    temp_prefs_apply (temp_prefs);
	    /* destroy temp prefs */
	    temp_prefs_destroy (temp_prefs);
	}
	else
	{
	    /* we ignore errors -- no need to be concerned about them */
	    g_error_free (error);
	}
	g_free (dir);
    }
}



/* Load preferences file from the iPod and merge them into the general
 * prefs system.
 */
void load_ipod_prefs (iTunesDB *itdb, const gchar *mountpoint)
{
    g_return_if_fail (mountpoint);
    load_ipod_index_prefs (get_itdb_index (itdb), mountpoint);
}




/* retrieve offline mode from itdb (convenience function) */
gboolean get_offline (iTunesDB *itdb)
{
    ExtraiTunesDBData *eitdb;

    g_return_val_if_fail (itdb, FALSE);
    eitdb = itdb->userdata;
    g_return_val_if_fail (eitdb, FALSE);

    return eitdb->offline;
}

/* ----------------------------------------------------------------
 *
 * Main program init and shutdown
 *
 * ---------------------------------------------------------------- */

/**
 * gtkpod_init
 *
 * initialize prefs and other services as well as display
 */
void gtkpod_init (int argc, char *argv[])
{
    gchar *progname;

    /* initialize xml_file: if gtkpod is called in the build directory
       (".../src/gtkpod") use the local gtkpod.glade (in the data
       directory), otherwise use
       "PACKAGE_DATA_DIR/PACKAGE/data/gtkpod.glade" */

    progname = g_find_program_in_path (argv[0]);
    if (progname)
    {
	static const gchar *SEPsrcSEPgtkpod = G_DIR_SEPARATOR_S "src" G_DIR_SEPARATOR_S "gtkpod";

	if (!g_path_is_absolute (progname))
	{
	    gchar *cur_dir = g_get_current_dir ();
	    gchar *prog_absolute;

	    if (g_str_has_prefix (progname, "." G_DIR_SEPARATOR_S))
		prog_absolute = g_build_filename (cur_dir,progname+2,NULL);
	    else
		prog_absolute = g_build_filename (cur_dir,progname,NULL);
	    g_free (progname);
	    g_free (cur_dir);
	    progname = prog_absolute;
	}

	if (g_str_has_suffix (progname, SEPsrcSEPgtkpod))
	{
	    gchar *suffix = g_strrstr (progname, SEPsrcSEPgtkpod);
	    if (suffix)
	    {
		*suffix = 0;
		xml_file = g_build_filename (progname, "data", "gtkpod.glade", NULL);
	    }
	}
	g_free (progname);
	if (xml_file && !g_file_test (xml_file, G_FILE_TEST_EXISTS))
	{
	    g_free (xml_file);
	    xml_file = NULL;
	}
    }
    if (!xml_file)
	xml_file = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, "data", "gtkpod.glade", NULL);
    else
    {
	printf ("Using local gtkpod.glade file since program was started from source directory:\n%s\n", xml_file);
    }

    /* Initialisation of libxml */
    LIBXML_TEST_VERSION;

    main_window_xml = gtkpod_xml_new (xml_file, "gtkpod");

    glade_xml_signal_autoconnect (main_window_xml);
  
    gtkpod_window = gtkpod_xml_get_widget (main_window_xml, "gtkpod");

    prefs_init (argc, argv); 

    coverart_init (argv[0]);
    stockid_init (argv[0]);

    file_convert_init ();

    display_create ();

    gtk_widget_show (gtkpod_window);

    init_data (gtkpod_window);   /* setup base data, importing all local
				  * repositories */

    /* stuff to be done before starting gtkpod */
    call_script ("gtkpod.in", NULL);

    autodetection_init ();
	
    server_setup ();   /* start server to accept playcount updates */
}



/**
 * gtkpod_shutdown
 *
 * free memory, shutdown services and call gtk_main_quit ()
 */
void gtkpod_shutdown ()
{
    /* stop accepting requests for playcount updates */
    server_shutdown ();

		/* Change the windows back to track view to ensure the
		 * sorttab state is saved correctly/
		 */
		gphoto_change_to_photo_window (FALSE);
		
    /* Sort column order needs to be stored */
    tm_store_col_order();
  
    /* Update default sizes */
    display_update_default_sizes();

    /* shut down conversion infrastructure */
    file_convert_shutdown ();

    /* Save prefs */
    prefs_save ();

/* FIXME: release memory in a clean way */
#if 0
    remove_all_playlists ();  /* first remove playlists, then tracks!
			       * (otherwise non-existing *tracks may
			       * be accessed) */
    remove_all_tracks ();
#endif
    display_cleanup ();

    prefs_shutdown ();

    xmlCleanupParser();
    xmlMemoryDump();
	
    call_script ("gtkpod.out", NULL);
    gtk_main_quit ();
}
