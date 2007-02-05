#ifndef __FILE_CONVERT_H_
#define __FILE_CONVERT_H_

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <gtk/gtk.h>
#include <itdb.h>
#include <file.h>

typedef struct
{
    Track   *track;                /* Track to convert */
    gchar   *converted_file;       /* PC filename of the "mp3" file 
                                      != NULL if the file exists   */
    gint32   old_size;             /* size of the original file    */
    FileType type;                 /* type of the original file    */
    GPid     child_pid;            /* PID of conversion process    */
    gint     child_stdout;         /* STDOUT of conversion process */
    gchar    *command_line;        /* used for conversion */
    gint     source_id;
    gboolean aborted;
} TrackConv;


GError *file_convert_pre_copy (TrackConv *converter);
GError *file_convert_post_copy (TrackConv *converter);
GError *file_convert_wait_for_conversion (TrackConv *converter);

/* extern gchar **cmdline_to_argv(const gchar *cmdline, Track *track); */

#endif
