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
static time_t tstamp, tnow;
static struct tm *lt;
static gboolean parsed_time;
static gboolean lower;
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

DIGIT     [0-9]
NUM       {DIGIT}+
TIMESHORT {NUM}":"{NUM}
TIMEFULL  {NUM}":"{NUM}":"{NUM}
DATESHORT {NUM}"/"{NUM}
DATEFULL  {NUM}"/"{NUM}"/"{NUM}
RELTIME1  [+-]{TIMESHORT}
RELTIME2  [+-]{TIMEFULL}
RELTIME3  {NUM}[smhdwMy]
RELTIME4  [+-]{RELTIME3}
%%

{TIMESHORT}	{
		gchar *ptr1 = yytext;
		gchar *ptr2;
		printf ("Time with minutes: '%s'\n", yytext);
		lt->tm_hour = strtol (ptr1, &ptr2, 10);
		++ptr2;
		ptr1 = ptr2;
		lt->tm_min = strtol (ptr1, &ptr2, 10);
		lt->tm_sec = 0;
		tstamp = mktime (lt);
		parsed_time = TRUE;
		printf ("tnow: %d, tstamp: %d: %s", tnow, tstamp, asctime (lt));
		}
	
{TIMEFULL}	{
		gchar *ptr1 = yytext;
		gchar *ptr2;
		printf ("Time with seconds: '%s'\n", yytext);
		lt->tm_hour = strtol (ptr1, &ptr2, 10);
		++ptr2;
		ptr1 = ptr2;
		lt->tm_min = strtol (ptr1, &ptr2, 10);
		++ptr2;
		ptr1 = ptr2;
		lt->tm_sec = strtol (ptr1, &ptr2, 10);
		tstamp = mktime (lt);
		parsed_time = TRUE;
		printf ("tnow: %d, tstamp: %d: %s", tnow, tstamp, asctime (lt));
        	}

{DATESHORT}	{
		gchar *ptr1 = yytext;
		gchar *ptr2;
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
		lt->tm_mday = strtol (ptr1, &ptr2, 10);
		++ptr2;
		ptr1 = ptr2;
		lt->tm_mon = strtol (ptr1, &ptr2, 10) - 1;
		tstamp = mktime (lt);
		printf ("tnow: %d, tstamp: %d: %s", tnow, tstamp, asctime (lt));
		}

{DATEFULL}	{
		gchar *ptr1 = yytext;
		gchar *ptr2;
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
		lt->tm_mday = strtol (ptr1, &ptr2, 10);
		++ptr2;
		ptr1 = ptr2;
		lt->tm_mon = strtol (ptr1, &ptr2, 10) - 1;
		++ptr2;
		ptr1 = ptr2;
		lt->tm_year = strtol (ptr1, &ptr2, 10);
		if (lt->tm_year < 70)
		    lt->tm_year += 2000;
		if ((lt->tm_year < 100) && (lt->tm_year >=70))
		    lt->tm_year += 1900;
		/* tm_year are years since 1900 */
		lt->tm_year -= 1900;
		tstamp = mktime (lt);
		printf ("tnow: %d, tstamp: %d: %s", tnow, tstamp, asctime (lt));
		}

{RELTIME1}	{
		printf ("RELTIME1: '%s'\n", yytext);
		}

{RELTIME2}	{
		printf ("RELTIME2: '%s'\n", yytext);
		}

{RELTIME3}	{
		printf ("RELTIME3: '%s'\n", yytext);
		}

{RELTIME4}	{
		printf ("RELTIME4: '%s'\n", yytext);
		}

[ \t]*      /* ignore */

.           printf( "Unrecognized character: '%s'\n", yytext );

%%

time_t dp_parse (gchar *dp_str, gboolean lower_margin)
{
    dp_strp = dp_str;
    /* set timestamp to current time */
    tstamp = time (NULL);
    tnow = tstamp;
    lt = localtime (&tnow);
    parsed_time = FALSE;
    lower = lower_margin;
    yylex ();
    return tstamp;
}
