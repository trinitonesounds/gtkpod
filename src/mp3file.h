/* mp3file.h
   Accessing the metadata of a raw MPEG stream, header file
   Copyright (C) 2001 Linus Walleij

This file is part of the GNOMAD package.

GNOMAD is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

You should have received a copy of the GNU General Public License
along with GNOMAD; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA. 

*/

#ifndef MP3FILEH_INCLUDED
#define MP3FILEH_INCLUDED 1

guint32 length_from_file(gchar *path, guint32 filesize);

#endif
