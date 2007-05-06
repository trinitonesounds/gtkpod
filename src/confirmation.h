/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

#ifndef __CONFIRMATION_H__
#define __CONFIRMATION_H__


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

typedef void (*ConfHandler)(gpointer user_data1, gpointer user_data2);

/* states for gtkpod_confirmation options */
typedef enum {
    CONF_STATE_TRUE,
    CONF_STATE_FALSE,
    CONF_STATE_INVERT_TRUE,
    CONF_STATE_INVERT_FALSE,
} CONF_STATE;

GtkResponseType gtkpod_confirmation (gint id,
				     gboolean modal,
				     const gchar *title,
				     const gchar *label,
				     const gchar *text,
				     const gchar *option1_text,
				     CONF_STATE option1_state,
				     const gchar *option1_key,
				     const gchar *option2_text,
				     CONF_STATE option2_state,
				     const gchar *option2_key,
				     gboolean confirm_again,
				     const gchar *confirm_again_key,
				     ConfHandler ok_handler,
				     ConfHandler apply_handler,
				     ConfHandler cancel_handler,
				     gpointer user_data1,
				     gpointer user_data2);

/* predefined IDs for use with gtkpod_confirmation() */
enum {
    CONF_ID_IPOD_DIR = 0,
    CONF_ID_GTKPOD_WARNING,
    CONF_ID_DANGLING0,
    CONF_ID_DANGLING1,
    CONF_ID_SYNC_SUMMARY,
    CONF_ID_TRANSFER
} CONF_ID;

void CONF_NULL_HANDLER (gpointer d1, gpointer d2);

#endif
