/*
|  Copyright (C) 2002 Corey Donohoe <atmos at atmos.org>
|
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
#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

/**
 * The different types of operations the thread system knows about
 * THREAD_WRITE_ITUNES - handle_export() is called in the background
 * THREAD_READ_ITUNES  - handle_import() is called in the background
 * THREAD_MD5   - 
 * THREAD_ID3   - 
 */
enum {
    THREAD_READ_ITUNES, 
    THREAD_WRITE_ITUNES,
    THREAD_MD5,
    THREAD_ID3,
    THREAD_TYPE_COUNT
};

/* Make simple requests of the system. */
void gtkpod_thread_pool_init(void);
void gtkpod_thread_pool_free(void);
void gtkpod_thread_pool_exec(guint e);

#endif
