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

%option prefix="lexdp2"
%option outfile="lex.yy.c"

DATECHARS [^<>=]+
%%

"<"{DATECHARS} {
    printf ("<: '%s'\n", yytext);
}

">"{DATECHARS} {
    printf (">: '%s'\n", yytext);
}

"="{DATECHARS} {
    printf ("=: '%s'\n", yytext);
}

{DATECHARS}"<"[ \t]*"d"[ \t]*"<"{DATECHARS} |
{DATECHARS}"<"[ \t]*"<"{DATECHARS} {
    printf ("<<: '%s'\n", yytext);
}

{DATECHARS}">"[ \t]*"d"[ \t]*">"{DATECHARS} |
{DATECHARS}">"[ \t]*">"{DATECHARS} {
    printf (">>: '%s'\n", yytext);
}

[ \t]*      /* ignore */

.           printf( "Unrecognized character: '%s'\n", yytext );

%%

void dp2_parse (gchar *dp_str)
{
    dp_strp = dp_str;
    /* set timestamp to current time */
    yylex ();
}
