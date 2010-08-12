/* Time-stamp: <2006-05-15 21:59:36 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|                                          Paul Richardson <phantom_sf at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  URL: http://www.gtkpod.org/
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

#ifndef __REPOSITORY_H__
#define __REPOSITORY_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libgtkpod/itdb.h"
#include "libgtkpod/prefs.h"

struct _RepositoryView {
    GladeXML *xml;           /* XML info                             */
    GtkWidget *window; /* pointer to repository window         */
    GtkComboBox *repository_combo_box; /* pointer to repository combo */
    GtkComboBox *playlist_combo_box; /* pointer to playlist combo */
    iTunesDB *itdb; /* currently displayed repository       */
    gint itdb_index; /* index number of itdb                 */
    Playlist *playlist; /* currently displayed playlist         */
    Playlist *next_playlist; /* playlist to display next (or NULL)   */
    TempPrefs *temp_prefs; /* changes made so far                  */
    TempPrefs *extra_prefs; /* changes to non-prefs items (e.g.
     live-update                          */
};

typedef struct _RepositoryView RepositoryView;

struct _CreateRepWindow {
    GladeXML *xml; /* XML info                             */
    GtkWidget *window; /* pointer to repository window         */
};

typedef struct _CreateRepWindow CreateRepWindow;

GtkWidget *repository_xml_get_widget(GladeXML *xml, const gchar *name);

#define GET_WIDGET(x, a) repository_xml_get_widget (x, a)

#define IPOD_MODEL_COMBO "ipod_model_combo"
#define IPOD_MODEL_ENTRY "ipod_model_entry--not-a-glade-name"
#define KEY_BACKUP "filename"

/* Columns for the model_combo tree model */
enum {
    COL_POINTER, COL_STRING
};
void set_cell(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);

void repository_init_model_number_combo (GtkComboBox *cb);
void repository_combo_populate(GtkComboBox *combo_box);
void playlist_cb_cell_data_func_pix(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
void playlist_cb_cell_data_func_text(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

void open_repository_editor(iTunesDB *itdb, Playlist *playlist);
void destroy_repository_editor();
void display_create_repository_dialog();

extern const gchar *SELECT_OR_ENTER_YOUR_MODEL;

gboolean repository_ipod_init (iTunesDB *itdb);
void repository_ipod_init_set_model (iTunesDB *itdb, const gchar *old_model);

#endif
