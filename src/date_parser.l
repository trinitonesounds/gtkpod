/* -*- mode: c -*-
| Time-stamp: <2003-06-13 00:47:24 jcs>
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
*/

/* This parser (dp_parse()) will take one date specification (like
 * 9/6/03, or -5d) and convert it into a timestamp. More details
 * below. */

%{
#include <stdlib.h>
#include "date_parser.h"
#include "misc.h"
#include "support.h"
    static gchar *dp_strp = NULL;
    /* time stamp (result!) */
    static time_t tstamp;
    /* already parsed time? */
    static gboolean parsed_time;
    /* are we reading a lower bound (TRUE) or upper bound (FALSE) */
    static gboolean lower;
    /* did an error occur? */
    static gboolean dp_error;
    /* should relative dates be read strictly (TRUE: no changing of seconds
     * to 0 or 59 when minutes are set) or not. It's only set to FALSE
     * for the "=..." format. */
    static gboolean rel_strict;
    typedef enum {
	DP_SEC,
	DP_MIN,
	DP_HOUR,
	DP_DAY,
	DP_WEEK,
	DP_MONTH,
	DP_YEAR,
	DP_INF
    } RelTime;
    /* Which is the lowest relative time specifier already parsed? */
    RelTime reltime;
    static void dp_reltime (gchar *str, gint32 sign);
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
%}

/* stop parsing after end of string is reached */
%option noyywrap
/* avoid compiler warning: `yyunput' defined but not used */
%option nounput
/* We have several parsers in one executable, so we must use the
   prefix option to avoid name space clashes. Note that this only
   works with flex, but not with lex. */
%option prefix="lexdp"
/* Unfortunately, the prefix option also changes the output filename
   and thereby breaks the automake script. Therefore I define the
   output filename here. However, this is not portable -- on other
   systems the default output filename expected by the automake
   scripts may be lexyy.c or similar. To make the code portable again
   (hopefully), I also define "LEX_OUTPUT_ROOT = lex.yy" in
   Makefile.am */
%option outfile="lex.yy.c"


DIGIT     [0-9]
NUM       {DIGIT}+
/* definitions for time formats */
TIMESHORT {NUM}":"{NUM}            /* e.g. 8:54               */
TIMEFULL  {NUM}":"{NUM}":"{NUM}    /* e.g. 8:54:23            */
DATESHORT {NUM}"/"{NUM}            /* e.g. 9/6 (for June 9th) */
DATEFULL  {NUM}"/"{NUM}"/"{NUM}    /* e.g. 9/6/3  or 9/6/2003 */
RELTIME_S [+-]{TIMESHORT}          /* e.g. -8:54              */
RELTIME_F [+-]{TIMEFULL}           /* e.g. +8:54.23           */
RELTIME   ({NUM}[smhdwMy])+        /* e.g. 5d6m3s             */
SRELTIME  [+-]{RELTIME}            /* e.g. -3M5d              */
%%

{TIMESHORT}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    lt->tm_hour = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_min = strtol (ptr1, &ptr1, 10);
    if (lower)    lt->tm_sec = 0;
    else          lt->tm_sec = 59;
    tstamp = mktime (lt);
    parsed_time = TRUE;
#if DP_DEBUG
    printf ("Time with minutes: '%s'\n", yytext);
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
#endif
}
	
{TIMEFULL}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    lt->tm_hour = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_min = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_sec = strtol (ptr1, &ptr1, 10);
    tstamp = mktime (lt);
    parsed_time = TRUE;
#if DP_DEBUG
    printf ("Time with seconds: '%s'\n", yytext);
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
#endif
}

{DATESHORT}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    if (!parsed_time)
    {
	if (lower)
	{
	    lt->tm_hour = 0;
	    lt->tm_min = 0;
	    lt->tm_sec = 0;
	}
	else
	{
	    lt->tm_hour = 23;
	    lt->tm_min = 59;
	    lt->tm_sec = 59;
	}
    }
    lt->tm_mday = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_mon = strtol (ptr1, &ptr1, 10) - 1;
    tstamp = mktime (lt);
#if DP_DEBUG
    printf ("Date without year: '%s'\n", yytext);
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
#endif
}

{DATEFULL}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    if (!parsed_time)
    {
	if (lower)
	{
	    lt->tm_hour = 0;
	    lt->tm_min = 0;
	    lt->tm_sec = 0;
	}
	else
	{
	    lt->tm_hour = 23;
	    lt->tm_min = 59;
	    lt->tm_sec = 59;
	}
    }
    lt->tm_mday = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_mon = strtol (ptr1, &ptr1, 10) - 1;
    ++ptr1;
    lt->tm_year = strtol (ptr1, &ptr1, 10);
    if (lt->tm_year < 70)
	lt->tm_year += 2000;
    if ((lt->tm_year < 100) && (lt->tm_year >=70))
	lt->tm_year += 1900;
    /* tm_year is years since 1900 */
    lt->tm_year -= 1900;
    tstamp = mktime (lt);
#if DP_DEBUG
    printf ("Date with year: '%s'\n", yytext);
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
#endif
}

{RELTIME_S}     { /* [+-]{TIMESHORT} */
    gchar *ptr1 = yytext;
    gint32 hours, mins, sign;
    if (*ptr1 == '+')  sign = 1;
    else               sign = -1;
    ++ptr1;
    hours = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    mins = strtol (ptr1, &ptr1, 10);
    tstamp += sign * (hours*3600 + mins*60);
    if (DP_MIN < reltime)   reltime = DP_MIN;
#if DP_DEBUG
    printf ("[+-]{TIMESHORT} '%s'\n", yytext);
    printf ("tstamp: %d: %s", (int)tstamp, ctime (&tstamp));
#endif
}

{RELTIME_F}     { /* [+-]{TIMEFULL} */
    gchar *ptr1 = yytext;
    gint32 hours, mins, secs, sign;
    if (*ptr1 == '+')  sign = 1;
    else               sign = -1;
    ++ptr1;
    hours = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    mins = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    secs = strtol (ptr1, &ptr1, 10);
    tstamp += sign * (hours*3600 + mins*60 + secs);
    reltime = DP_SEC;
#if DP_DEBUG
    printf ("[+-]{TIMEFULL} '%s'\n", yytext);
    printf ("tstamp: %d: %s", (int)tstamp, ctime (&tstamp));
#endif
}

{RELTIME}	{ /* ({NUM}[smhdwMy])+ */
#if DP_DEBUG
    printf ("RELTIME: '%s'\n", yytext);
#endif
    /* call reltime with negative sign */
    dp_reltime (yytext, -1);
}

{SRELTIME}	{ /* [+-]{RELTIME} */
#if DP_DEBUG
    printf ("SRELTIME: '%s'\n", yytext);
#endif
    if (*yytext == '+')  dp_reltime (yytext+1, +1);
    else                 dp_reltime (yytext+1, -1);
}

[ \t]*      /* ignore */

.           {
    gtkpod_warning (_("Date format error: unrecognized character: '%s'\n"), yytext );
    dp_error = TRUE;
}

%%

/* handle ({NUM}[smhdwMy])+, assuming @sign */
static void dp_reltime (gchar *str, gint32 sign)
{
    gchar *ptr1 = str;
    gint32 arg;
    time_t secs = 0;

    while (*ptr1)
    {
	arg = strtol (ptr1, &ptr1, 10);
	switch (*ptr1)
	{
	case 's':
	    secs += arg;
	    reltime = DP_SEC;
	    break;
	case 'm':
	    secs += 60*arg;
	    if (DP_MIN < reltime)   reltime = DP_MIN;
	    break;
	case 'h':
	    secs += 3600*arg;
	    if (DP_HOUR < reltime)   reltime = DP_HOUR;
	    break;
	case 'd':
	    secs += 24*3600*arg;
	    if (DP_DAY < reltime)   reltime = DP_DAY;
	    break;
	case 'w':
	    secs += 7*24*3600*arg;
	    if (DP_WEEK < reltime)   reltime = DP_WEEK;
	    break;
	case 'M':
	    secs += 30*7*24*3600*arg;
	    if (DP_MONTH < reltime)   reltime = DP_MONTH;
	    break;
	case 'y':
	    secs += 365*7*24*3600*arg;
	    if (DP_YEAR < reltime)   reltime = DP_YEAR;
	    break;
	}
	++ptr1;
    }
    tstamp += sign*secs;
#if DP_DEBUG
    printf ("secs: %d, tstamp: %d: %s", (int)secs, (int)tstamp, ctime (&tstamp));
#endif
}


/* after reading the date string check if we should round down the
   interval (round if reltime is not supposed to be strict and no
   absolute time string has been parsed */
static void round_reltime (void)
{
    if (!rel_strict && !parsed_time)
    {   /* Round this datestamp to make a lower/upper margin */
	struct tm *lt = localtime (&tstamp);
	switch (reltime)
	{
	case DP_INF:
	    break;
	case DP_YEAR:
	    if (lower) lt->tm_mon = 0;
	    else       lt->tm_mon = 11;
	case DP_MONTH:
	    if (lower) lt->tm_mday = 1;
	    else
	    {
		switch (lt->tm_mon)
		{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
		    lt->tm_mday = 31;
		    break;
		case 2:
		    if ((lt->tm_year % 4) != 0)   lt->tm_mday = 28;
		    else
		    {
			if ((lt->tm_year % 100) != 0) lt->tm_mday = 29;
			else
			{   /* centuries are only leap years if they
			     * are a multiple of 400 */
			    if (((lt->tm_year+300) % 400) == 0)
				 lt->tm_mday = 29;
			    else lt->tm_mday = 28;
			}
		    }
		    break;
		case 4:
		case 6:
		case 9:
		case 11:
		    lt->tm_mday = 30;
		    break;
		}
	    }
	case DP_WEEK:
	    if (reltime == DP_WEEK)
	    {   /* only do this if week was set manually */
		/* tm_wday gives us the number of days since Sunday (0
		   to 6). Let's declare Monday to be the start of the
		   week and Sunday the end. */
		if (lower)
		{   /* Round down to Monday */
		    if (lt->tm_wday == 0)  lt->tm_mday -= 6;
		    else                   lt->tm_mday -= (lt->tm_wday-1);
		}
		else
		{   /* Round up to Sunday */
		    if (lt->tm_wday != 0)  lt->tm_mday += (7-lt->tm_wday);
		}
	    }
	case DP_DAY:
	    if (lower) lt->tm_hour = 0;
	    else       lt->tm_hour = 23;
	case DP_HOUR:
	    if (lower) lt->tm_min = 0;
	    else       lt->tm_min = 59;
	case DP_MIN:
	    if (lower) lt->tm_sec = 0;
	    else       lt->tm_sec = 59;
	case DP_SEC:
	    break;
	}
	tstamp = mktime (lt);
#if DP_DEBUG
	printf ("tstamp: %d: %s", (int)tstamp, ctime (&tstamp));
#endif
    }
}

/* dp_parse() will take one date specification (like 9/6/03, or -5d,
 * for details have a look at the definitions above) and convert it
 * into a timestamp.
 *
 * @dp_str: the string to parse 
 *
 * @result: the parsed timestamp is written into here
 *
 * @lower_margin: if TRUE, absolute time or date values are "rounded
 * down" (e.g. "9/6/03" would compute to "June 6 2003, 0:00:00"),
 * otherwise the are rounded up (e.g. "9/6/03" would compute to "June
 * 6 2003, 23:59:59"). Similarily, "8:05" would either compute to
 * "8:05:00" or "8:05:59" depending on whether the lower or upper
 * boundary is to be determined.
 *
 * @strict: should relative dates be read strictly (TRUE: no changing
 * of seconds to 0 or 59 when minutes are set, etc.) or not. Actually,
 * it's only set to FALSE * for the "=..." format.
 *
 * Return value:
 *
 * TRUE:  no error occured
 * FALSE: error occured
 */
gboolean dp_parse (gchar *dp_str, time_t *result,
		   gboolean lower_margin, gboolean strict)
{
    dp_strp = dp_str;
    /* set timestamp to current time */
    tstamp = time (NULL);
    /* did not yet parse any absolute time string */
    parsed_time = FALSE;
    /* did not yet parse any relative time string */
    reltime = DP_INF;
    /* no error occured (yet) */
    dp_error = FALSE;
    /* set parameters */
    lower = lower_margin;
    rel_strict = strict;
    /* call lexer */
    yylex ();
    /* round relative time up or down, if necessary */
    round_reltime ();
    /* set the result */
    if (result)  *result = tstamp;
    /* return the (inverted) error variable */
    return !dp_error;
}
