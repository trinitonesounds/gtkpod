/*
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
*/

%{
/* Set to "1" if debugging output is desired */
#define DP_DEBUG 1
#include <stdlib.h>
#include "date_parser.h"
#include "misc.h"
#include "support.h"
/* Will be set to TRUE if error occured */
    static gboolean dp_error;
/* Will be set to TRUE if construct (< date, = date...) was not
   recognized */
    static gboolean dp_construct_error;
/* Variable to hold lower and upper limit */
    static time_t lower_stamp, upper_stamp;
/* Pointer to input string to be parsed */
    static gchar *dp_strp = NULL;
/* We don't read from a stream but from a string buffer. This macro
   will copy a maximum of @max_size chars from dp_strp to @buf,
   writing the number of chars copied into @result. If no characters
   are copied, YY_NULL is written into @result. */
#define YY_INPUT(buf,result,max_size) \
    { \
    if (!dp_strp || !dp_strp[0]) result = YY_NULL; \
    else \
      { \
        gint i; \
        for (i=0; (i<max_size && *dp_strp); ++i)  buf[i] = *dp_strp++; \
        result = i; \
      } \
    }
    static void dp_ll (gchar *str);
    static void dp_LL (gchar *str);
%}

/* stop parsing after end of string is reached */
%option noyywrap
/* avoid compiler warning: `yyunput' defined but not used */
%option nounput
/* We have several parsers in one executable, so we must use the
   prefix option to avoid name space clashes. Note that this only
   works with flex, but not with lex. */
%option prefix="lexdp2"
/* Unfortunately, the prefix option also changes the output filename
   and thereby breaks the automake script. Therefore I define the
   output filename here. However, this is not portable -- on other
   systems the default output filename expected by the automake
   scripts may be lexyy.c or similar. To make the code portable again
   (hopefully), I also define "LEX_OUTPUT_ROOT = lex.yy" in
   Makefile.am */
%option outfile="lex.yy.c"

DATECHARS [^<>=]+


%%

^"<"{DATECHARS}$ {
    gchar *strp;
#if DP_DEBUG
    printf ("<: '%s'\n", yytext);
#endif
    lower_stamp = 0;
    strp = strchr (yytext, '<');
    if (strp)  upper_stamp = dp_parse (strp+1, FALSE);
    else       dp_error = TRUE;
}

^">"{DATECHARS}$ {
    gchar *strp;
#if DP_DEBUG
    printf (">: '%s'\n", yytext);
#endif
    lower_stamp = -1;
    upper_stamp = -1;
    strp = strchr (yytext, '>');
    if (strp) lower_stamp = dp_parse (strp+1, TRUE);
    else      dp_error = TRUE;
}

^"="{DATECHARS}$ {
    gchar *strp;
#if DP_DEBUG
    printf ("=: '%s'\n", yytext);
#endif
    strp = strchr (yytext, '=');
    if (strp)
    {
	lower_stamp = dp_parse (strp+1, TRUE);
	upper_stamp = dp_parse (strp+1, FALSE);
    }
    else      dp_error = TRUE;
    
}

^{DATECHARS}"<"[ \t]*"d"[ \t]*"<"{DATECHARS}$ {
    dp_ll (yytext);
}

^{DATECHARS}"<"[ \t]*"<"{DATECHARS}$ {
    dp_ll (yytext);
}

^{DATECHARS}">"[ \t]*"d"[ \t]*">"{DATECHARS}$ {
    dp_LL (yytext);
}

^{DATECHARS}">"[ \t]*">"{DATECHARS}$ {
    dp_LL (yytext);
}

[\n]        /* ignore */

.           {
    dp_error = TRUE;
    dp_construct_error = TRUE;
}

%%


/* pattern " < < " */
static void dp_ll (gchar *str)
{
    gchar *strp;
#if DP_DEBUG
    printf ("<<: '%s'\n", yytext);
#endif
    strp = strchr (str, '<');
    if (strp)
    {
	*strp = 0;
	lower_stamp = dp_parse (str, TRUE);
	strp = strchr (strp+1, '<');
	if (strp)  upper_stamp = dp_parse (strp+1, FALSE);
	else       dp_error = TRUE;
    }
    else  dp_error = TRUE;
}


/* pattern " > > " */
static void dp_LL (gchar *str)
{
    gchar *strp;
#if DP_DEBUG
    printf (">>: '%s'\n", yytext);
#endif
    strp = strchr (str, '>');
    if (strp)
    {
	*strp = 0;
	upper_stamp = dp_parse (str, FALSE);
	strp = strchr (strp+1, '>');
	if (strp)  lower_stamp = dp_parse (strp+1, TRUE);
	else       dp_error = TRUE;
    }
    else  dp_error = TRUE;
}



void dp2_parse (gchar *dp_str)
{
    /* for the 'end of line ("$") rule to work, we need a "\n" at the
       end of the string... */
    gchar *str = g_strdup_printf ("%s\n", dp_str);
    /* set string to parse */
    dp_strp = str;
    /* no error occured (yet) */
    dp_error = FALSE;
    dp_construct_error = FALSE;
    yylex ();
    if (dp_construct_error)
	gtkpod_warning ("Date parser: did not recognize construct:\n   '%s'\n",
			dp_str);
    g_free (str);
}
