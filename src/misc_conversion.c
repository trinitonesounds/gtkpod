/* Time-stamp: <2004-07-22 00:32:02 jcs>
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
#include <math.h>
#include <string.h>

#include "charset.h"
#include "itunesdb.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"


#define DEBUG_MISC 0


/* translates a TM_COLUMN_... (defined in display.h) into a
 * T_... (defined in track.h). Returns -1 in case a translation is not
 * possible */
T_item TM_to_T (TM_item sm)
{
    switch (sm)
    {
    case TM_COLUMN_TITLE:         return T_TITLE;
    case TM_COLUMN_ARTIST:        return T_ARTIST;
    case TM_COLUMN_ALBUM:         return T_ALBUM;
    case TM_COLUMN_GENRE:         return T_GENRE;
    case TM_COLUMN_COMPOSER:      return T_COMPOSER;
    case TM_COLUMN_TRACK_NR:      return T_TRACK_NR;
    case TM_COLUMN_CD_NR:         return T_CD_NR;
    case TM_COLUMN_IPOD_ID:       return T_IPOD_ID;
    case TM_COLUMN_PC_PATH:       return T_PC_PATH;
    case TM_COLUMN_IPOD_PATH:     return T_IPOD_PATH;
    case TM_COLUMN_TRANSFERRED:   return T_TRANSFERRED;
    case TM_COLUMN_SIZE:          return T_SIZE;
    case TM_COLUMN_TRACKLEN:      return T_TRACKLEN;
    case TM_COLUMN_BITRATE:       return T_BITRATE;
    case TM_COLUMN_SAMPLERATE:    return T_SAMPLERATE;
    case TM_COLUMN_BPM:           return T_BPM;
    case TM_COLUMN_PLAYCOUNT:     return T_PLAYCOUNT;
    case TM_COLUMN_RATING:        return T_RATING;
    case TM_COLUMN_TIME_CREATED:  return T_TIME_CREATED;
    case TM_COLUMN_TIME_PLAYED:   return T_TIME_PLAYED;
    case TM_COLUMN_TIME_MODIFIED: return T_TIME_MODIFIED;
    case TM_COLUMN_VOLUME:        return T_VOLUME;
    case TM_COLUMN_SOUNDCHECK:    return T_SOUNDCHECK;
    case TM_COLUMN_YEAR:          return T_YEAR;
    case TM_NUM_COLUMNS:          return -1;
    }
    return -1;
}


/* translates a ST_CAT_... (defined in display.h) into a
 * T_... (defined in track.h). Returns -1 in case a translation is not
 * possible */
T_item ST_to_T (ST_CAT_item st)
{
    switch (st)
    {
    case ST_CAT_ARTIST:      return T_ARTIST;
    case ST_CAT_ALBUM:       return T_ALBUM;
    case ST_CAT_GENRE:       return T_GENRE;
    case ST_CAT_COMPOSER:    return T_COMPOSER;
    case ST_CAT_TITLE:       return T_TITLE;
    case ST_CAT_YEAR:        return T_YEAR;
    case ST_CAT_SPECIAL:
    case ST_CAT_NUM:         return -1;
    }
    return -1;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                       Timestamp stuff                            *
 *                                                                  *
\*------------------------------------------------------------------*/

/* converts the time stamp @time to a string (max. length:
 * PATH_MAX). You must g_free the return value */
gchar *time_time_to_string (time_t time)
{
    gchar *format = prefs_get_time_format ();

    if (time && format)
    {
	gchar buf[PATH_MAX+1];
	struct tm *tm = localtime (&time);
	size_t size = strftime (buf, PATH_MAX, format, tm);
	buf[size] = 0;
	return g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
    }
    return g_strdup ("--");
}



/* get the timestamp TM_COLUMN_TIME_CREATE/PLAYED/MODIFIED */
time_t time_get_time (Track *track, TM_item tm_item)
{
    guint32 mactime = 0;

    if (track) switch (tm_item)
    {
    case TM_COLUMN_TIME_CREATED:
	mactime = track->time_created;
	break;
    case TM_COLUMN_TIME_PLAYED:
	mactime = track->time_played;
	break;
    case TM_COLUMN_TIME_MODIFIED:
	mactime = track->time_modified;
	break;
    default:
	mactime = 0;
	break;
    }
    return (itunesdb_time_mac_to_host (mactime));
}


/* hopefully obvious */
gchar *time_field_to_string (Track *track, TM_item tm_item)
{
    return (time_time_to_string (time_get_time (track, tm_item)));
}


/* get the timestamp TM_COLUMN_TIME_CREATE/PLAYED/MODIFIED */
void time_set_time (Track *track, time_t time, TM_item tm_item)
{
    guint32 mactime = itunesdb_time_host_to_mac (time);

    if (track) switch (tm_item)
    {
    case TM_COLUMN_TIME_CREATED:
	track->time_created = mactime;
	break;
    case TM_COLUMN_TIME_PLAYED:
	track->time_played = mactime;
	break;
    case TM_COLUMN_TIME_MODIFIED:
	track->time_modified = mactime;
	break;
    default:
	break;
    }
}



/* -------------------------------------------------------------------
 * The following is taken straight out of glib2.0.6 (gconvert.c):
 * g_filename_from_uri uses g_filename_from_utf8() to convert from
 * utf8. However, the user might have selected a different charset
 * inside gtkpod -- we must use gtkpod's charset_from_utf8()
 * instead. That's the only line changed...
 * -------------------------------------------------------------------*/

/* Test of haystack has the needle prefix, comparing case
 * insensitive. haystack may be UTF-8, but needle must
 * contain only ascii. */
static gboolean
has_case_prefix (const gchar *haystack, const gchar *needle)
{
  const gchar *h, *n;

  /* Eat one character at a time. */
  h = haystack;
  n = needle;

  while (*n && *h &&
	 g_ascii_tolower (*n) == g_ascii_tolower (*h))
    {
      n++;
      h++;
    }

  return *n == '\0';
}

static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static gchar *
g_unescape_uri_string (const char *escaped,
		       int         len,
		       const char *illegal_escaped_characters,
		       gboolean    ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c;

  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);

  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
	{
	  /* catch partial escape sequences past the end of the substring */
	  if (in + 3 > in_end)
	    break;

	  c = unescape_character (in + 1);

	  /* catch bad escape sequences and NUL characters */
	  if (c <= 0)
	    break;

	  /* catch escaped ASCII */
	  if (ascii_must_not_be_escaped && c <= 0x7F)
	    break;

	  /* catch other illegal escaped characters */
	  if (strchr (illegal_escaped_characters, c) != NULL)
	    break;

	  in += 2;
	}

      *out++ = c;
    }

  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end || !g_utf8_validate (result, -1, NULL))
    {
      g_free (result);
      return NULL;
    }

  return result;
}

static gboolean
is_escalphanum (gunichar c)
{
  return c > 0x7F || g_ascii_isalnum (c);
}

static gboolean
is_escalpha (gunichar c)
{
  return c > 0x7F || g_ascii_isalpha (c);
}

/* allows an empty string */
static gboolean
hostname_validate (const char *hostname)
{
  const char *p;
  gunichar c, first_char, last_char;

  p = hostname;
  if (*p == '\0')
    return TRUE;
  do
    {
      /* read in a label */
      c = g_utf8_get_char (p);
      p = g_utf8_next_char (p);
      if (!is_escalphanum (c))
	return FALSE;
      first_char = c;
      do
	{
	  last_char = c;
	  c = g_utf8_get_char (p);
	  p = g_utf8_next_char (p);
	}
      while (is_escalphanum (c) || c == '-');
      if (last_char == '-')
	return FALSE;

      /* if that was the last label, check that it was a toplabel */
      if (c == '\0' || (c == '.' && *p == '\0'))
	return is_escalpha (first_char);
    }
  while (c == '.');
  return FALSE;
}

/**
 * g_filename_from_uri:
 * @uri: a uri describing a filename (escaped, encoded in UTF-8).
 * @hostname: Location to store hostname for the URI, or %NULL.
 *            If there is no hostname in the URI, %NULL will be
 *            stored in this location.
 * @error: location to store the error occuring, or %NULL to ignore
 *         errors. Any of the errors in #GConvertError may occur.
 *
 * Converts an escaped UTF-8 encoded URI to a local filename in the
 * encoding used for filenames.
 *
 * Return value: a newly-allocated string holding the resulting
 *               filename, or %NULL on an error.
 **/
gchar *
filename_from_uri (const char *uri,
		   char      **hostname,
		   GError    **error)
{
  const char *path_part;
  const char *host_part;
  char *unescaped_hostname;
  char *result;
  char *filename;
  int offs;
#ifdef G_OS_WIN32
  char *p, *slash;
#endif

  if (hostname)
    *hostname = NULL;

  if (!has_case_prefix (uri, "file:/"))
    {
      g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		   _("The URI '%s' is not an absolute URI using the file scheme"),
		   uri);
      return NULL;
    }

  path_part = uri + strlen ("file:");

  if (strchr (path_part, '#') != NULL)
    {
      g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		   _("The local file URI '%s' may not include a '#'"),
		   uri);
      return NULL;
    }

  if (has_case_prefix (path_part, "///"))
    path_part += 2;
  else if (has_case_prefix (path_part, "//"))
    {
      path_part += 2;
      host_part = path_part;

      path_part = strchr (path_part, '/');

      if (path_part == NULL)
	{
	  g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		       _("The URI '%s' is invalid"),
		       uri);
	  return NULL;
	}

      unescaped_hostname = g_unescape_uri_string (host_part, path_part - host_part, "", TRUE);

      if (unescaped_hostname == NULL ||
	  !hostname_validate (unescaped_hostname))
	{
	  g_free (unescaped_hostname);
	  g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		       _("The hostname of the URI '%s' is invalid"),
		       uri);
	  return NULL;
	}

      if (hostname)
	*hostname = unescaped_hostname;
      else
	g_free (unescaped_hostname);
    }

  filename = g_unescape_uri_string (path_part, -1, "/", FALSE);

  if (filename == NULL)
    {
      g_set_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_BAD_URI,
		   _("The URI '%s' contains invalidly escaped characters"),
		   uri);
      return NULL;
    }

  offs = 0;
#ifdef G_OS_WIN32
  /* Drop localhost */
  if (hostname && *hostname != NULL &&
      g_ascii_strcasecmp (*hostname, "localhost") == 0)
    {
      g_free (*hostname);
      *hostname = NULL;
    }

  /* Turn slashes into backslashes, because that's the canonical spelling */
  p = filename;
  while ((slash = strchr (p, '/')) != NULL)
    {
      *slash = '\\';
      p = slash + 1;
    }

  /* Windows URIs with a drive letter can be like "file://host/c:/foo"
   * or "file://host/c|/foo" (some Netscape versions). In those cases, start
   * the filename from the drive letter.
   */
  if (g_ascii_isalpha (filename[1]))
    {
      if (filename[2] == ':')
	offs = 1;
      else if (filename[2] == '|')
	{
	  filename[2] = ':';
	  offs = 1;
	}
    }
#endif

  /* This is where we differ from glib2.0.6: we use
     gtkpod's charset_from_utf8() instead of glib's
     g_filename_from_utf8() */
  result = charset_from_utf8 (filename + offs);
  g_free (filename);

  return result;
}




gint32 replaygain_to_soundcheck(gdouble replaygain)
{
    gdouble tv;
    gint32 soundcheck = 0;

/*    tv = ((double) replaygain) / (5.0 * log10(2.0));*/
    /* according to Samuel Wood -- thanks! */
    /* exp10() is a gnu extension, therefore I substituted exp10(x)
       with exp(x*log(10)) */
    tv = 1000 * exp (-(double)replaygain * log(10) * 0.1);

    soundcheck =  floor(tv + 0.5);
		
/* 		printf("radio_gain: %i\n", track->radio_gain); */
/* 		printf("soundcheck: %i\n", track->soundcheck); */
    return soundcheck;
}
