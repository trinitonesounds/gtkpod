/* 
 * Copyright (C) 2007 Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
 *
 * Sound Juicer - sj-inhibit.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
 */

#ifndef SJ_INHIBIT_H
#define SJ_INHIBIT_H

#include <glib.h>
 
guint sj_inhibit (const gchar * appname, const gchar * reason, guint xid);
void sj_uninhibit (guint cookie);

#endif /* SJ_INHIBIT_H */
