#ifndef __FILE_CONVERT_H_
#define __FILE_CONVERT_H_

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <gtk/gtk.h>
#include "display_itdb.h"


extern GError *file_convert_pre_copy(Track *track);
extern GError *file_convert_post_copy(Track *track);
extern GError *file_convert_wait_for_conversion(Track *track);

/* extern gchar **cmdline_to_argv(const gchar *cmdline, Track *track); */

#endif
