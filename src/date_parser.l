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
#include <stdlib.h>
#include "date_parser.h"
    static gchar *dp_strp = NULL;
    static time_t tstamp;
    static gboolean parsed_time;
    static gboolean lower;
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

%option prefix="lexdp"
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
    printf ("Time with minutes: '%s'\n", yytext);
    lt->tm_hour = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_min = strtol (ptr1, &ptr1, 10);
    lt->tm_sec = 0;
    tstamp = mktime (lt);
    parsed_time = TRUE;
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
}
	
{TIMEFULL}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    printf ("Time with seconds: '%s'\n", yytext);
    lt->tm_hour = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_min = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    lt->tm_sec = strtol (ptr1, &ptr1, 10);
    tstamp = mktime (lt);
    parsed_time = TRUE;
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
}

{DATESHORT}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    printf ("Date without year: '%s'\n", yytext);
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
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
}

{DATEFULL}	{
    gchar *ptr1 = yytext;
    struct tm *lt = localtime (&tstamp);
    printf ("Date with year: '%s'\n", yytext);
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
    printf ("tstamp: %d: %s", (int)tstamp, asctime (lt));
}

{RELTIME_S}     { /* [+-]{TIMESHORT} */
    gchar *ptr1 = yytext;
    gint32 hours, mins, sign;
    printf ("[+-]{TIMESHORT} '%s'\n", yytext);
    if (*ptr1 == '+')  sign = 1;
    else               sign = -1;
    ++ptr1;
    hours = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    mins = strtol (ptr1, &ptr1, 10);
    tstamp += sign * (hours*3600 + mins*60);
    printf ("tstamp: %d: %s", (int)tstamp, ctime (&tstamp));
}

{RELTIME_F}     { /* [+-]{TIMEFULL} */
    gchar *ptr1 = yytext;
    gint32 hours, mins, secs, sign;
    printf ("[+-]{TIMEFULL} '%s'\n", yytext);
    if (*ptr1 == '+')  sign = 1;
    else               sign = -1;
    ++ptr1;
    hours = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    mins = strtol (ptr1, &ptr1, 10);
    ++ptr1;
    secs = strtol (ptr1, &ptr1, 10);
    tstamp += sign * (hours*3600 + mins*60 + secs);
    printf ("tstamp: %d: %s", (int)tstamp, ctime (&tstamp));
}

{RELTIME}	{ /* ({NUM}[smhdwMy])+ */
    printf ("RELTIME: '%s'\n", yytext);
    /* call reltime with negative sign */
    dp_reltime (yytext, -1);
}

{SRELTIME}	{ /* [+-]{RELTIME} */
    printf ("SRELTIME: '%s'\n", yytext);
    if (*yytext == '+')  dp_reltime (yytext+1, +1);
    else                 dp_reltime (yytext+1, -1);
}

[ \t]*      /* ignore */

.           printf( "Unrecognized character: '%s'\n", yytext );

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
	    break;
	case 'm':
	    secs += 60*arg;
	    break;
	case 'h':
	    secs += 3600*arg;
	    break;
	case 'd':
	    secs += 24*3600*arg;
	    break;
	case 'w':
	    secs += 7*24*3600*arg;
	    break;
	case 'M':
	    secs += 30*7*24*3600*arg;
	    break;
	case 'y':
	    secs += 365*7*24*3600*arg;
	    break;
	}
	++ptr1;
    }
    tstamp += sign*secs;
    printf ("secs: %d, tstamp: %d: %s", (int)secs, (int)tstamp, ctime (&tstamp));
}

time_t dp_parse (gchar *dp_str, gboolean lower_margin)
{
    dp_strp = dp_str;
    /* set timestamp to current time */
    tstamp = time (NULL);
    parsed_time = FALSE;
    lower = lower_margin;
    yylex ();
    return tstamp;
}
