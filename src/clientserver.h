/* Time-stamp: <2005-01-03 22:27:42 jcs>
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

#ifndef CLIENTSERVERH_INCLUDED
#define CLIENTSERVERH_INCLUDED 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <sys/file.h>

extern const gchar *SOCKET_TEST;
extern const gchar *SOCKET_PLYC;

void server_setup (void);
void server_shutdown (void);
gboolean client_playcount (gchar *file);
#ifndef HAVE_FLOCK
#include <unistd.h>
#include <fcntl.h>
/* emulate flock on systems that do not have it */
int flock(int fd, int operation);
#ifndef LOCK_EX
#define LOCK_EX 2
#endif
#endif
#endif
