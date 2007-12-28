/* Time-stamp: <2006-05-01 15:04:54 jcs>
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

#include <string.h>
#include <gtk/gtk.h>
#include "prefs.h"
#include "charset.h"
#include "misc.h"

/* If Japanese auto-conversion is being used, this variable is being
   set with each call of charset_to_utf8(). You can get a copy of its
   value by calling charset_get_auto().
   This variable will only be reset by calling charset_reset_auto(). */
static gchar *auto_charset = NULL;

typedef struct {
	const gchar *descr;
	const gchar *name;
} CharsetInfo;


static const CharsetInfo charset_info[] = { 
    {N_("Arabic (IBM-864)"),                  "IBM864"        },
    {N_("Arabic (ISO-8859-6)"),               "ISO-8859-6"    },
    {N_("Arabic (Windows-1256)"),             "windows-1256"  },
    {N_("Baltic (ISO-8859-13)"),              "ISO-8859-13"   },
    {N_("Baltic (ISO-8859-4)"),               "ISO-8859-4"    },
    {N_("Baltic (Windows-1257)"),             "windows-1257"  },
    {N_("Celtic (ISO-8859-14)"),              "ISO-8859-14"   },
    {N_("Central European (IBM-852)"),        "IBM852"        },
    {N_("Central European (ISO-8859-2)"),     "ISO-8859-2"    },
    {N_("Central European (Windows-1250)"),   "windows-1250"  },
    {N_("Chinese Simplified (GB18030)"),      "gb18030"       },
    {N_("Chinese Simplified (GB2312)"),       "GB2312"        },
    {N_("Chinese Traditional (Big5)"),        "Big5"          },
    {N_("Chinese Traditional (Big5-HKSCS)"),  "Big5-HKSCS"    },
    {N_("Cyrillic (IBM-855)"),                "IBM855"        },
    {N_("Cyrillic (ISO-8859-5)"),             "ISO-8859-5"    },
    {N_("Cyrillic (ISO-IR-111)"),             "ISO-IR-111"    },
    {N_("Cyrillic (KOI8-R)"),                 "KOI8-R"        },
    {N_("Cyrillic (Windows-1251)"),           "windows-1251"  },
    {N_("Cyrillic/Russian (CP-866)"),         "IBM866"        },
    {N_("Cyrillic/Ukrainian (KOI8-U)"),       "KOI8-U"        },
    {N_("English (US-ASCII)"),                "us-ascii"      },
    {N_("Greek (ISO-8859-7)"),                "ISO-8859-7"    },
    {N_("Greek (Windows-1253)"),              "windows-1253"  },
    {N_("Hebrew (IBM-862)"),                  "IBM862"        },
    {N_("Hebrew (Windows-1255)"),             "windows-1255"  },
    {N_("Japanese (automatic detection)"),    GTKPOD_JAPAN_AUTOMATIC},
    {N_("Japanese (EUC-JP)"),                 "EUC-JP"        },
    {N_("Japanese (ISO-2022-JP)"),            "ISO-2022-JP"   },
    {N_("Japanese (Shift_JIS)"),              "Shift_JIS"     },
    {N_("Korean (EUC-KR)"),                   "EUC-KR"        },
    {N_("Nordic (ISO-8859-10)"),              "ISO-8859-10"   },
    {N_("South European (ISO-8859-3)"),       "ISO-8859-3"    },
    {N_("Thai (TIS-620)"),                    "TIS-620"       },
    {N_("Turkish (IBM-857)"),                 "IBM857"        },
    {N_("Turkish (ISO-8859-9)"),              "ISO-8859-9"    },
    {N_("Turkish (Windows-1254)"),            "windows-1254"  },
    {N_("Unicode (UTF-7)"),                   "UTF-7"         },
    {N_("Unicode (UTF-8)"),                   "UTF-8"         },
    {N_("Unicode (UTF-16BE)"),                "UTF-16BE"      },
    {N_("Unicode (UTF-16LE)"),                "UTF-16LE"      },
    {N_("Unicode (UTF-32BE)"),                "UTF-32BE"      },
    {N_("Unicode (UTF-32LE)"),                "UTF-32LE"      },
    {N_("Vietnamese (VISCII)"),               "VISCII"        },
    {N_("Vietnamese (Windows-1258)"),         "windows-1258"  },
    {N_("Visual Hebrew (ISO-8859-8)"),        "ISO-8859-8"    },
    {N_("Western (IBM-850)"),                 "IBM850"        },
    {N_("Western (ISO-8859-1)"),              "ISO-8859-1"    },
    {N_("Western (ISO-8859-15)"),             "ISO-8859-15"   },
    {N_("Western (Windows-1252)"),            "windows-1252"  },
    /*
     * From this point, character sets aren't supported by iconv
     */
/*    {N_("Arabic (IBM-864-I)"),                "IBM864i"              },
    {N_("Arabic (ISO-8859-6-E)"),             "ISO-8859-6-E"         },
    {N_("Arabic (ISO-8859-6-I)"),             "ISO-8859-6-I"         },
    {N_("Arabic (MacArabic)"),                "x-mac-arabic"         },
    {N_("Armenian (ARMSCII-8)"),              "armscii-8"            },
    {N_("Central European (MacCE)"),          "x-mac-ce"             },
    {N_("Chinese Simplified (GBK)"),          "x-gbk"                },
    {N_("Chinese Simplified (HZ)"),           "HZ-GB-2312"           },
    {N_("Chinese Traditional (EUC-TW)"),      "x-euc-tw"             },
    {N_("Croatian (MacCroatian)"),            "x-mac-croatian"       },
    {N_("Cyrillic (MacCyrillic)"),            "x-mac-cyrillic"       },
    {N_("Cyrillic/Ukrainian (MacUkrainian)"), "x-mac-ukrainian"      },
    {N_("Farsi (MacFarsi)"),                  "x-mac-farsi"},
    {N_("Greek (MacGreek)"),                  "x-mac-greek"          },
    {N_("Gujarati (MacGujarati)"),            "x-mac-gujarati"       },
    {N_("Gurmukhi (MacGurmukhi)"),            "x-mac-gurmukhi"       },
    {N_("Hebrew (ISO-8859-8-E)"),             "ISO-8859-8-E"         },
    {N_("Hebrew (ISO-8859-8-I)"),             "ISO-8859-8-I"         },
    {N_("Hebrew (MacHebrew)"),                "x-mac-hebrew"         },
    {N_("Hindi (MacDevanagari)"),             "x-mac-devanagari"     },
    {N_("Icelandic (MacIcelandic)"),          "x-mac-icelandic"      },
    {N_("Korean (JOHAB)"),                    "x-johab"              },
    {N_("Korean (UHC)"),                      "x-windows-949"        },
    {N_("Romanian (MacRomanian)"),            "x-mac-romanian"       },
    {N_("Turkish (MacTurkish)"),              "x-mac-turkish"        },
    {N_("User Defined"),                      "x-user-defined"       },
    {N_("Vietnamese (TCVN)"),                 "x-viet-tcvn5712"      },
    {N_("Vietnamese (VPS)"),                  "x-viet-vps"           },
    {N_("Western (MacRoman)"),                "x-mac-roman"          },
    // charsets whithout posibly translatable names
    {"T61.8bit",                              "T61.8bit"             },
    {"x-imap4-modified-utf7",                 "x-imap4-modified-utf7"},
    {"x-u-escaped",                           "x-u-escaped"          },
    {"windows-936",                           "windows-936"          }
*/
    {NULL, NULL}
};



/* Sets up the charsets to choose from in the "combo". It presets the
   charset stored in cfg->charset (or "System Charset" if none is set
   there

   DEPRECATED - use charset_init_combo_box with GtkComboBox
*/
void charset_init_combo (GtkCombo *combo)
{
    gchar *current_charset;
    gchar *description;
    const CharsetInfo *ci;
    static GList *charsets = NULL; /* list with choices -- takes a while to
				     * initialize, so we only do it once */
    
    current_charset = prefs_get_string("charset");
    if ((current_charset == NULL) || (strlen (current_charset) == 0))
    {
	description = g_strdup (_("System Charset"));
    }
    else
    {
	description = charset_to_description (current_charset);
    }
    if (charsets == NULL)
    { /* set up list with charsets */
	FILE *fp;

	charsets = g_list_append (charsets, _("System Charset"));
	/* now add all the charset descriptions in the list above */
	ci=charset_info;
	while (ci->descr != NULL)
	{
	    charsets = g_list_append (charsets, _(ci->descr));
	    ++ci;
	}
	/* let's add all available charsets returned by "iconv -l" */
	/* The code assumes that "iconv -l" returns a list with the
	   name of one charset in each line, each valid line being
	   terminated by "//".  */
	fp = popen ("iconv -l", "r");
	if (fp)
	{
	    gchar buf[PATH_MAX];
	    /* read one line of output at a time */
	    while (fgets (buf, PATH_MAX, fp))
	    {
		/* only consider lines ending on "//" */
		gchar *bufp = g_strrstr (buf, "//\n");
		if (bufp)
		{  /* add everything before "//" to our charset list */
		    gchar *bufpp = buf;
		    *bufp = 0;  /* shorten string */
		    while ((*bufpp == ' ') || (*bufpp == 0x09))
			++bufpp; /* skip whitespace */
		    if (*bufpp)
			charsets = g_list_append (charsets, g_strdup (bufpp));
		}
	    }
	    pclose (fp);
	}
    }
    /* set pull down items */
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), charsets); 
    /* set standard entry */
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), description);
    g_free (description);
    g_free(current_charset);
}

/* Sets up the charsets to choose from in the "combo". It presets the
   charset stored in cfg->charset (or "System Charset" if none is set
   there */
void charset_init_combo_box (GtkComboBox *combo)
{
    gchar *current_charset;
    gchar *description;
    const CharsetInfo *ci;
	GtkCellRenderer *renderer;
	GtkTreeIter use_iter;
    static GtkListStore *charsets = NULL; /* list with choices -- takes a while to
				     * initialize, so we only do it once */
    
    current_charset = prefs_get_string("charset");
	
    if ((current_charset == NULL) || (strlen (current_charset) == 0))
    {
		description = g_strdup (_("System Charset"));
    }
    else
    {
		description = charset_to_description (current_charset);
    }
	
    if (charsets == NULL)
    { /* set up list with charsets */
		FILE *fp;
		GtkTreeIter iter;

		charsets = gtk_list_store_new (1, G_TYPE_STRING);
		/* now add all the charset descriptions in the list above */
		
		gtk_list_store_append (charsets, &iter);
		gtk_list_store_set (charsets, &iter, 0, _("System Charset"), -1);

		for (ci = charset_info; ci->descr; ci++)
		{
			gtk_list_store_append (charsets, &iter);
			gtk_list_store_set (charsets, &iter, 0, _(ci->descr), -1);
		}
		
		/* let's add all available charsets returned by "iconv -l" */
		/* The code assumes that "iconv -l" returns a list with the
		   name of one charset in each line, each valid line being
		   terminated by "//".  */
		fp = popen ("iconv -l", "r");
		
		if (fp)
		{
			gchar buf[PATH_MAX];
			
			/* read one line of output at a time */
			while (fgets (buf, PATH_MAX, fp))
			{
				/* only consider lines ending on "//" */
				gchar *bufp = g_strrstr (buf, "//\n");
				
				if (bufp)
				{
					/* add everything before "//" to our charset list */
					gchar *bufpp = buf;
					*bufp = 0;  /* shorten string */
					
					while ((*bufpp == ' ') || (*bufpp == 0x09))
						bufpp++; /* skip whitespace */
					
					if (*bufpp)
					{
						gtk_list_store_append (charsets, &iter);
						gtk_list_store_set (charsets, &iter, 0, g_strdup (bufpp));

					}
				}
			}
			
			pclose (fp);
		}
    }
    /* set pull down items */
	renderer = gtk_cell_renderer_text_new ();
	
    gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (charsets));
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "text", 0);
	
	for (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (charsets), &use_iter);
		 gtk_list_store_iter_is_valid (charsets, &use_iter);
		 gtk_tree_model_iter_next (GTK_TREE_MODEL (charsets), &use_iter))
	{
		gchar *cur_desc;
		gtk_tree_model_get (GTK_TREE_MODEL (charsets), &use_iter, 0, &cur_desc, -1);
		
		if(!g_utf8_collate(description, cur_desc))
			break;
	}

	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &use_iter);
	g_object_unref (renderer);
    g_free (description);
    g_free (current_charset);
}

/* returns the charset name belonging to the description "descr"
 * chosen from the combo. Return "NULL" when it could not be found, or
 * if it is the System Default Charset (locale). You must g_free
 * the charset received (unlike g_get_charset ()) */
gchar *charset_from_description (gchar *descr)
{
    const CharsetInfo *ci;

    if (!descr) return NULL; /* sanity! */
    /* check for "System Charset" and return NULL */
    if (g_utf8_collate (descr, _("System Charset")) == 0)   return NULL;
    /* check if description matches one of the descriptions in
     * charset_info[], and if so, return the charset name */
    ci = charset_info;
    while (ci->descr != NULL)
    {
	if (g_utf8_collate (descr, _(ci->descr)) == 0)
	{
	    return g_strdup (ci->name);
	}
	++ci;
    }
    /* OK, it was not in the charset_info[] list. Therefore it must be
     * from the "iconv -l" list. We just return a copy of it */
    return g_strdup (descr);
}


/* Returns the description belonging to the charset "charset"
 * Returns the description (if found) or just a copy of "charset"
 * You must g_free the charset received */
gchar *charset_to_description (gchar *charset)
{
    const CharsetInfo *ci;

    if (!charset) return NULL; /* sanity! */
    /* check if charset matches one of the charsets in
     * charset_info[], and if so, return the description */
    ci = charset_info;
    while (ci->descr != NULL)
    {
	if (compare_string_case_insensitive (charset, ci->name) == 0)
	{
	    return g_strdup (_(ci->descr));
	}
	++ci;
    }
    /* OK, it was not in the charset_info[] list. Therefore it must be
     * from the "iconv -l" list. We just return a copy of it */
    return g_strdup (charset);
}


/* code for automatic detection of Japanese char-subset donated by
   Hiroshi Kawashima */
static const gchar *charset_check_k_code (const gchar *p2)
{
    static const char* charsets[] = {
	    "UTF-8",
	    "EUC-JP",
	    "CP932",
	    "ISO-2022-JP",
	    NULL
    };
    int i;
    gchar *ret;
    gssize len;

    if (p2 == NULL) return NULL;
 
    len = strlen ((gchar*)p2);
    for (i=0; charsets[i]; i++) {
      ret = g_convert ((gchar*)p2,            /* string to convert */
		       len,                  /* length of string  */
		       "UTF-8",              /* to_codeset        */
		       charsets[i],          /* from_codeset      */
		       NULL,                 /* *bytes_read       */
		       NULL,                 /* *bytes_written    */
		       NULL);                /* GError **error    */
      if (ret != NULL) {
        g_free(ret);
        return charsets[i];
      }
    }
    return (NULL);
}

/* same as check_k_code, but defaults to "UTF-8" if no match is found */
static const gchar *charset_check_k_code_with_default (const guchar *p)
{
    const gchar *result=NULL;

    if (p)       result = charset_check_k_code (p);
    if (!result) result = "UTF-8";
    return result;
}

/* return the charset actually used for the "auto detection"
 * feature. So far only Japanese Auto Detecion is implemented */
static gchar *charset_check_auto (const gchar *str)
{
    gchar *charset;
    gchar *result;

    if (str == NULL) return NULL; /* sanity */

    charset = prefs_get_string("charset");

    if (charset && (strcmp (charset, GTKPOD_JAPAN_AUTOMATIC) == 0))
	result = g_strdup(charset_check_k_code (str));
    else
	result = NULL;

    g_free(charset);
    return result;
}

/* See description at the definition of gchar *auto_charset; for
   details */
gchar *charset_get_auto (void)
{
    return g_strdup (auto_charset);
}

void charset_reset_auto (void)
{
    auto_charset = NULL;
}


/* Convert "str" (in the charset specified in cfg->charset) to
 * utf8. If cfg->charset is NULL, "str" is assumed to be in the
 * current locale charset */
/* Must free the returned string yourself */
gchar *charset_to_utf8 (const gchar *str)
{
    gchar *charset;  /* From prefs */
    const gchar *locale_charset; /* Used if prefs doesn't have a charset */
    gchar *result;

    if (str == NULL) return NULL;  /* sanity */
    charset = charset_check_auto (str);
    if (charset)
    {
	g_free(auto_charset);
	auto_charset = g_strdup(charset);
    }
    else
    {
	charset = prefs_get_string("charset");
	if (!charset || !strlen (charset))
	{    /* use standard locale charset */
	    g_free(charset);
	    g_get_charset (&locale_charset);
	    charset = g_strdup(locale_charset);
	}
    }
    
    result = charset_to_charset (charset, "UTF-8", str);
    g_free(charset);
    return result;
}


/* Convert "str" from utf8 to the charset specified in
 * cfg->charset. If cfg->charset is NULL, "str" is converted to the
 * current locale charset */
/* Must free the returned string yourself */
gchar *charset_from_utf8 (const gchar *str)
{
    gchar *charset;
    const gchar *locale_charset;
    gchar *result;

    if (str == NULL) return NULL;  /* sanity */
    charset = prefs_get_string("charset");
    if (!charset || !strlen (charset))
    {   
       /* use standard locale charset */
	g_free(charset);
	g_get_charset (&locale_charset);
	charset = g_strdup(locale_charset);
    }

    result = charset_to_charset ("UTF-8", charset, str);
    g_free(charset);
    return result;
}

/* Convert "str" from utf8 to the charset specified in @s->charset. If
 * this is NULL, try cfg->charset. If cfg->charset is also NULL, "str"
 * is converted to the current locale charset */
/* Must free the returned string yourself */
gchar *charset_track_charset_from_utf8 (Track *s, const gchar *str)
{
    gchar *charset;
    const gchar *locale_charset;
    gchar *result;
    ExtraTrackData *etd;

    g_return_val_if_fail (s, NULL);
    g_return_val_if_fail (s->userdata, NULL);

    if (str == NULL) return NULL;  /* sanity */

    etd = s->userdata;

    if (etd->charset && strlen (etd->charset))
	   charset = g_strdup(etd->charset);
    else   
	charset = prefs_get_string("charset");
   
    if (!charset || !strlen (charset))
    {    /* use standard locale charset */
	g_free(charset);
	g_get_charset (&locale_charset);
	charset = g_strdup(locale_charset);
    }

    result = charset_to_charset ("UTF-8", charset, str);
    g_free(charset);
    return result;
}

/* Convert "str" from "from_charset" to "to_charset", trying to skip
   illegal character as best as possible */
/* Must free the returned string yourself */
gchar *charset_to_charset (const gchar *from_charset,
			   const gchar *to_charset,
			   const gchar *str)
{
    gchar *ret;
    gssize len;
    gsize bytes_read;

    if (!str) return NULL;

    /* Handle automatic selection of Japanese charset */
    if (from_charset && (strcmp (from_charset, GTKPOD_JAPAN_AUTOMATIC) == 0))
	from_charset = charset_check_k_code_with_default (str);
    /* Automatic selection of Japanese charset when encoding to
       Japanese is a bit of a problem... we simply fall back on EUC-JP
       (defined in check_k_code_with_default ) if this situation
       occurs. */
    if (to_charset && (strcmp (to_charset, GTKPOD_JAPAN_AUTOMATIC) == 0))
	to_charset = charset_check_k_code_with_default (NULL);

    len = strlen (str);
    /* do the conversion! */
    ret = g_convert (str,                  /* string to convert */
		     len,                  /* length of string  */
		     to_charset,           /* to_codeset        */
		     from_charset,         /* from_codeset      */
		     &bytes_read,          /* *bytes_read       */
		     NULL,                 /* *bytes_written    */
		     NULL);                /* GError **error    */
    if (ret == NULL)
    { /* We probably had illegal characters. We try to convert as much
       * as possible. "bytes_read" tells how many chars were converted
       * successfully, and we try to fill up with spaces 0x20, which
       * will work for most charsets (except those using 16 Bit or 32
       * Bit representation (0x2020 or 0x20202020 != 0x0020 or
       * 0x00000020) */
	gchar *strc = g_strdup (str);
	gsize br0 = bytes_read;
	gsize br;
	while (!ret && (bytes_read < len))
	{
	    strc[bytes_read] = 0x20;
	    br = bytes_read;
	    ret = g_convert (strc,         /* string to convert */
			     len,          /* length of string  */
			     to_charset,   /* to_codeset        */
			     from_charset, /* from_codeset      */
			     &bytes_read,  /* *bytes_read       */
			     NULL,         /* *bytes_written    */
			     NULL);        /* GError **error    */
	    /* max. nr. of converted Bytes */
	    if (bytes_read > br0)  br0 = bytes_read;
	    /* don't do an infinite loop */
	    if (bytes_read <= br)  bytes_read = br + 1;
	}
	if (ret == NULL)
	{ /* still no valid string */
	    if (br0 != 0)
	    { /* ok, at least something we can use! */
		ret = g_convert (strc,         /* string to convert */
				 br0,          /* length of string  */
				 to_charset,   /* to_codeset        */
				 from_charset, /* from_codeset      */
				 &bytes_read,  /* *bytes_read       */
				 NULL,         /* *bytes_written    */
				 NULL);        /* GError **error    */
	    }
	    if (ret == NULL)
	    { /* Well... what do you think we should return? */
		/* ret = g_strdup (_("gtkpod: Invalid Conversion")); */
		ret = g_strdup ("");
	    }
	}
	g_free (strc);
    }
    return ret;
}
