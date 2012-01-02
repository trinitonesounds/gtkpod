/* 
 * Copyright (C) 2003-2005 Ross Burton <ross@burtonini.com>
 *
 * Sound Juicer - sj-util.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "sj-util.h"

/* Taken from #511367, will be in GLib soon */
gboolean
make_directory_with_parents (GFile         *file,
		                    GCancellable  *cancellable,
		                    GError       **error)
{
  gboolean result;
  GFile *parent_file, *work_file;
  GList *list = NULL, *l;
  GError *my_error = NULL;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;
  
  result = g_file_make_directory (file, cancellable, &my_error);
  if (result || my_error->code != G_IO_ERROR_NOT_FOUND) 
    {
      if (my_error)
        g_propagate_error (error, my_error);
      return result;
    }
  
  work_file = file;
  
  while (!result && my_error->code == G_IO_ERROR_NOT_FOUND) 
    {
      g_clear_error (&my_error);
    
      parent_file = g_file_get_parent (work_file);
      if (parent_file == NULL)
        break;
      result = g_file_make_directory (parent_file, cancellable, &my_error);
    
      if (!result && my_error->code == G_IO_ERROR_NOT_FOUND)
        list = g_list_prepend (list, parent_file);

      work_file = parent_file;
    }

  for (l = list; result && l; l = l->next)
    {
      result = g_file_make_directory ((GFile *) l->data, cancellable, &my_error);
    }
  
  /* Clean up */
  while (list != NULL) 
    {
      g_object_unref ((GFile *) list->data);
      list = g_list_remove (list, list->data);
    }

  if (!result) 
    {
      g_propagate_error (error, my_error);
      return result;
    }
  
  return g_file_make_directory (file, cancellable, error);
}

/* Pass NULL to use g_free */
void
g_list_deep_free (GList *l, GFunc free_func)
{
  g_return_if_fail (l != NULL);
  if (free_func == NULL) free_func = (GFunc)g_free;
  g_list_foreach (l, free_func, NULL);
  g_list_free (l);
}

GFile *
sj_get_default_music_directory (void)
{
	const char *dir;
	GFile *file;

	dir = g_get_user_special_dir (G_USER_DIRECTORY_MUSIC);
	if (dir == NULL) {
		dir = g_get_home_dir ();
	}
	file = g_file_new_for_path (dir);
	return file;
}

void
sj_add_default_dirs (GtkFileChooser *dialog)
{
	const char *dir;

	dir = g_get_user_special_dir (G_USER_DIRECTORY_MUSIC);
	if (dir) {
		gtk_file_chooser_add_shortcut_folder (dialog, dir, NULL);
	}
}

