/* 
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-extracting.h
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
 * Authors: Ross Burton <ross@burtonini.com>
 */

#ifndef SJ_EXTRACTING_H
#define SJ_EXTRACTING_H

#include <gtk/gtk.h>
#include "sj-structures.h"

void on_extract_activate (GtkWidget *button, gpointer user_data);
char *filepath_parse_pattern (const char* pattern, const TrackDetails *track);
void on_progress_cancel_clicked (GtkWidget *button, gpointer user_data);

#endif /* SJ_EXTRACTING_H */
