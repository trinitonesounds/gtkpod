/*
 |  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "misc.h"
#include "misc_track.h"
#include "prefs.h"
#include "gp_private.h"
#include "tools.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib/gi18n-lib.h>

/* Structure to keep all necessary information */
struct nm {
    Track *track; /* track to be normalised */
    GError *error; /* Errors generated during the normalisation */
};

/*pipe's definition*/
enum {
    READ = 0, WRITE = 1
};

enum {
    BUFLEN = 1000,
};

/* ------------------------------------------------------------

 Normalize Volume

 ------------------------------------------------------------ */

#ifdef G_THREADS_ENABLED
static GMutex *mutex = NULL;
static GCond *cond = NULL;
static gboolean mutex_data = FALSE;
#endif

/* Run @command on @track_path.
 *
 * Command may include options, like "mp3gain -q -k %s"
 *
 * %s is replaced by @selected_tracks if present, otherwise @selected_tracks are
 * added at the end.
 *
 * Return value: TRUE if the command ran successfully, FALSE if any
 * error occurred.
 */
gboolean run_exec_on_tracks(const gchar *commandline, GList *selected_tracks, GError **error) {
    gchar *command_full_path = NULL;
    gchar *command = NULL;
    gchar *command_base = NULL;
    gchar *buf;
    GList *tks;
    const gchar *nextarg;
    gboolean success = FALSE;
    gboolean percs = FALSE;
    GPtrArray *args;

    int status, fdnull, ret;
    pid_t tpid;

    g_return_val_if_fail(commandline, FALSE);
    g_return_val_if_fail(selected_tracks, FALSE);

    /* skip whitespace */
    while (g_ascii_isspace (*commandline))
        ++commandline;

    /* find the command itself -- separated by ' ' */
    nextarg = strchr(commandline, ' ');
    if (!nextarg) {
        nextarg = commandline + strlen(commandline);
    }

    command = g_strndup(commandline, nextarg - commandline);

    command_full_path = g_find_program_in_path(command);

    if (!command_full_path) {
        buf =
                g_strdup_printf(_("Could not find '%s'.\nPlease specifiy the exact path in the preference dialog or install the program if it is not installed on your system.\n\n"), command);
        gtkpod_log_error(error, buf);
        g_free(buf);
        goto cleanup;
    }

    command_base = g_path_get_basename(command_full_path);

    /* Create the command line to be used with execv(). */
    args = g_ptr_array_sized_new(strlen(commandline));
    /* add the full path */
    g_ptr_array_add(args, command_full_path);
    /* add the basename */
    g_ptr_array_add(args, command_base);
    /* add the command line arguments */

    commandline = nextarg;

    /* skip whitespace */
    while (g_ascii_isspace (*commandline))
        ++commandline;

    while (*commandline != 0) {
        const gchar *next;

        next = strchr(commandline, ' ');
        /* next argument is everything to the end */
        if (!next)
            next = commandline + strlen(commandline);

        if (strncmp(commandline, "%s", 2) == 0) { /* substitute %s with @selected_tracks */
            for (tks = selected_tracks; tks; tks = tks->next) {
                Track *tr = tks->data;
                g_return_val_if_fail(tr, FALSE);
                gchar *path;
                path = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
                if (path) {
                    g_ptr_array_add(args, path);
                }
            }
            percs = TRUE;
        }
        else {
            g_ptr_array_add(args, g_strndup(commandline, next - commandline));
        }

        /* skip to next argument */
        commandline = next;

        /* skip whitespace */
        while (g_ascii_isspace (*commandline))
            ++commandline;
    }

    /* Add @selected_tracks if "%s" was not present */
    if (!percs) {
        for (tks = selected_tracks; tks; tks = tks->next) {
            Track *tr = tks->data;
            g_return_val_if_fail(tr, FALSE);
            gchar *path;
            path = get_file_name_from_source(tr, SOURCE_PREFER_LOCAL);
            if (path) {
                g_ptr_array_add(args, path);
            }
        }
    }

    /* need NULL pointer */
    g_ptr_array_add(args, NULL);

    tpid = fork();

    switch (tpid) {
    case 0: /* we are the child */
    {
        gchar **argv = (gchar **) args->pdata;
#if 0
        gchar **bufp = argv;
        while (*bufp) {puts (*bufp); ++bufp;}
#endif
        /* redirect output to /dev/null */
        if ((fdnull = open("/dev/null", O_WRONLY | O_NDELAY)) != -1) {
            dup2(fdnull, fileno(stdout));
        }
        execv(argv[0], &argv[1]);
        exit(0);
        break;
    }
    case -1: /* we are the parent, fork() failed  */
        g_ptr_array_free(args, TRUE);
        break;
    default: /* we are the parent, everything's fine */
        tpid = waitpid(tpid, &status, 0);
        g_ptr_array_free(args, TRUE);
        if (WIFEXITED(status))
            ret = WEXITSTATUS(status);
        else
            ret = 2;
        if (ret > 1) {
            buf = g_strdup_printf(_("Execution of '%s' failed.\n\n"), command_full_path);
            gtkpod_log_error(error, buf);
            g_free(buf);
        }
        else {
            success = TRUE;
        }
        break;
    }

    cleanup: g_free(command_full_path);
    g_free(command);
    g_free(command_base);

    return success;
}



/* reread the soundcheck value from the file */
static gboolean nm_get_soundcheck(Track *track, GError **error) {
    gchar *path, *buf;
    gchar *commandline = NULL;
    FileType *filetype;

    g_return_val_if_fail(track, FALSE);

    if (read_soundcheck(track, error))
        return TRUE;

    if (error && *error)
        return FALSE;

    path = get_file_name_from_source(track, SOURCE_PREFER_LOCAL);
    filetype = determine_filetype(path);

    if (!path || !filetype) {
        buf = get_track_info(track, FALSE);
        buf = g_strdup_printf(_("Normalization failed: file not available (%s)."), buf);
        gtkpod_log_error(error, buf);
        g_free(buf);
        return FALSE;
    }

    commandline = filetype_get_gain_cmd(filetype);
    if (commandline) {
        GList *tks = NULL;
        tks = g_list_append(tks, track);
        if (run_exec_on_tracks(commandline, tks, error)) {
            return read_soundcheck(track, error);
        }
    }
    else {
        buf = g_strdup_printf(_("Normalization failed for file %s: file type not supported."
                "To normalize mp3 and aac files ensure the following commands paths have been set in the Tools section"
                "\tmp3 files: mp3gain"
                "\taac files: aacgain"), path);
        gtkpod_log_error(error, buf);
        g_free(buf);
    }

    return FALSE;
}

#ifdef G_THREADS_ENABLED
/* Threaded getTrackGain*/
static gpointer th_nm_get_soundcheck(gpointer data) {
    struct nm *nm = data;
    gboolean success = nm_get_soundcheck(nm->track, &(nm->error));
    g_mutex_lock(mutex);
    mutex_data = TRUE; /* signal that thread will end */
    g_cond_signal(cond);
    g_mutex_unlock(mutex);
    return GUINT_TO_POINTER(success);
}
#endif

/* normalize the newly inserted tracks (i.e. non-transferred tracks) */
void nm_new_tracks(iTunesDB *itdb) {
    GList *tracks = NULL;
    GList *gl;

    g_return_if_fail(itdb);

    for (gl = itdb->tracks; gl; gl = gl->next) {
        Track *track = gl->data;
        g_return_if_fail(track);
        if (!track->transferred) {
            tracks = g_list_append(tracks, track);
        }
    }
    nm_tracks_list(tracks);
    g_list_free(tracks);
}

static void nm_report_errors_and_free(GString *errors) {
    if (errors && errors->len > 0) {
        gtkpod_confirmation(-1, /* gint id, */
        TRUE, /* gboolean modal, */
        _("Normalization Errors"), /* title */
        _("Errors created by track normalisation"), /* label */
        errors->str, /* scrolled text */
        NULL, 0, NULL, /* option 1 */
        NULL, 0, NULL, /* option 2 */
        TRUE, /* gboolean confirm_again, */
        "show_normalization_errors",/* confirm_again_key,*/
        CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
        NULL, /* don't show "Apply" button */
        NULL, /* cancel_handler,*/
        NULL, /* gpointer user_data1,*/
        NULL); /* gpointer user_data2,*/

        g_string_free(errors, TRUE);
    }
}

void nm_tracks_list(GList *list) {
    gint count, succ_count, n;
    gdouble fraction = 0;
    gdouble old_fraction = 0;
    guint32 old_soundcheck;
    gboolean success;
    gchar *progtext = NULL;
    struct nm *nm;
    GString *errors = g_string_new(""); /* Errors generated during the normalisation */

#ifdef G_THREADS_ENABLED
    GThread *thread = NULL;
    GTimeVal gtime;
    if (!mutex)
        mutex = g_mutex_new ();
    if (!cond)
        cond = g_cond_new ();
#endif

    block_widgets();

    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();

    /* count number of tracks to be normalized */
    n = g_list_length(list);
    count = 0; /* tracks processed */
    succ_count = 0; /* tracks normalized */

    if (n == 0) {
        // nothing to do
        return;
    }

    gtkpod_statusbar_reset_progress(100);

    nm = g_malloc0(sizeof(struct nm));
    while (list) {
        nm->track = list->data;
        nm->error = NULL;

        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();

        /* need to know so we can update the display when necessary */
        old_soundcheck = nm->track->soundcheck;

#ifdef G_THREADS_ENABLED
        mutex_data = FALSE;

        thread = g_thread_create (th_nm_get_soundcheck, nm, TRUE, NULL);
        if (thread) {
            g_mutex_lock(mutex);
            do {
                while (widgets_blocked && gtk_events_pending())
                    gtk_main_iteration();
                /* wait a maximum of 10 ms */

                g_get_current_time(&gtime);
                g_time_val_add(&gtime, 20000);
                g_cond_timed_wait(cond, mutex, &gtime);
            }
            while (!mutex_data);
            success = GPOINTER_TO_UINT(g_thread_join (thread));
            g_mutex_unlock(mutex);
        }
        else {
            g_warning("Thread creation failed, falling back to default.\n");
            success = nm_get_soundcheck(nm->track, &(nm->error));

        }
#else
        success = nm_get_soundcheck (nm->track, nm->error);
#endif

        /*normalization part*/
        if (!success) {
            gchar *path = get_file_name_from_source(nm->track, SOURCE_PREFER_LOCAL);

            if (nm->error) {
                errors =
                        g_string_append(errors, g_strdup_printf(_("'%s-%s' (%s) could not be normalized. %s\n"), nm->track->artist, nm->track->title,
                                path ? path : "", nm->error->message));
            }
            else {
                errors =
                        g_string_append(errors, g_strdup_printf(_("'%s-%s' (%s) could not be normalized. Unknown error.\n"), nm->track->artist, nm->track->title,
                                path ? path : ""));
            }

            g_free(path);
        }
        else {
            ++succ_count;
            if (old_soundcheck != nm->track->soundcheck) {
                gtkpod_track_updated(nm->track);
                data_changed(nm->track->itdb);
            }
        }
        /*end normalization*/

        ++count;
        fraction = (gdouble) count / (gdouble) n;
        progtext = g_strdup_printf(_("%d%% (%d tracks left)"), count * 100 / n, n - count);

        gdouble ticks = fraction - old_fraction;
        gtkpod_statusbar_increment_progress_ticks(ticks * 100, progtext);

        old_fraction = fraction;
        g_free(progtext);

        if (fraction == 1) {
            /* All finished */
            gtkpod_statusbar_reset_progress(100);
            gtkpod_statusbar_message(ngettext ("Normalized %d of %d track.", "Normalized %d of %d tracks.", n), count, n);
        }

        while (widgets_blocked && gtk_events_pending())
            gtk_main_iteration();

        list = g_list_next(list);

        if (nm->error)
            g_error_free(nm->error);

    } /*end while*/

    g_free(nm);

    nm_report_errors_and_free(errors);

    gtkpod_statusbar_message(ngettext ("Normalized %d of %d tracks.",
            "Normalized %d of %d tracks.", n), count, n);

    release_widgets();
}

/* ------------------------------------------------------------

 Synchronize Contacts / Calendar

 ------------------------------------------------------------ */

typedef enum {
    SYNC_CONTACTS, SYNC_CALENDAR, SYNC_NOTES
} SyncType;

/* FIXME: tools need to be defined for each itdb separately */
/* replace %i in all strings of argv with prefs_get_ipod_mount() */
static void tools_sync_replace_percent_i(iTunesDB *itdb, gchar **argv) {
    gchar *ipod_mount;
    gint ipod_mount_len;
    gint offset = 0;

    if (!itdb)
        return;

    ipod_mount = get_itdb_prefs_string(itdb, KEY_MOUNTPOINT);

    if (!ipod_mount)
        ipod_mount = g_strdup("");

    ipod_mount_len = strlen(ipod_mount);

    while (argv && *argv) {
        gchar *str = *argv;
        gchar *pi = strstr(str + offset, "%i");
        if (pi) {
            /* len: -2: replace "%i"; +1: trailing 0 */
            gint len = strlen(str) - 2 + ipod_mount_len + 1;
            gchar *new_str = g_malloc0(sizeof(gchar) * len);
            strncpy(new_str, str, pi - str);
            strcpy(new_str + (pi - str), ipod_mount);
            strcpy(new_str + (pi - str) + ipod_mount_len, pi + 2);
            g_free(str);
            str = new_str;
            *argv = new_str;
            /* set offset to point behind the inserted ipod_path in
             case ipod_path contains "%i" */
            offset = (pi - str) + ipod_mount_len;
        }
        else {
            offset = 0;
            ++argv;
        }
    }

    g_free(ipod_mount);
}

/* execute the specified script, giving out error/status messages on
 the way */
static gboolean tools_sync_script(iTunesDB *itdb, SyncType type) {
    gchar *script = NULL;
    gchar *script_path, *buf;
    gchar **argv = NULL;
    GString *script_output;
    gint fdpipe[2]; /*a pipe*/
    gint len;
    pid_t pid;

    switch (type) {
    case SYNC_CONTACTS:
        script = get_itdb_prefs_string(itdb, "path_sync_contacts");
        break;
    case SYNC_CALENDAR:
        script = get_itdb_prefs_string(itdb, "path_sync_calendar");
        break;
    case SYNC_NOTES:
        script = get_itdb_prefs_string(itdb, "path_sync_notes");
        break;
    default:
        fprintf(stderr, "Programming error: tools_sync_script () called with %d\n", type);
        return FALSE;
    }

    /* remove leading and trailing whitespace */
    if (script)
        g_strstrip(script);

    if (!script || (strlen(script) == 0)) {
        gtkpod_warning(_("Please specify the command to be called on the 'Tools' section of the preferences dialog.\n"));
        g_free(script);
        return FALSE;
    }

    argv = g_strsplit(script, " ", -1);

    tools_sync_replace_percent_i(itdb, argv);

    script_path = g_find_program_in_path(argv[0]);
    if (!script_path) {
        gtkpod_warning(_("Could not find the command '%s'.\n\nPlease verify the setting in the 'Tools' section of the preferences dialog.\n\n"), argv[0]);
        g_free(script);
        g_strfreev(argv);
        return FALSE;
    }

    /* set up arg list (first parameter should be basename of script */
    g_free(argv[0]);
    argv[0] = g_path_get_basename(script_path);

    buf = g_malloc(BUFLEN);
    script_output = g_string_sized_new(BUFLEN);

    /*create the pipe*/
    pipe(fdpipe);
    /*then fork*/
    pid = fork();

    /*and cast mp3gain*/
    switch (pid) {
    case -1: /* parent and error, now what?*/
        break;
    case 0: /*child*/
        close(fdpipe[READ]);
        dup2(fdpipe[WRITE], fileno(stdout));
        dup2(fdpipe[WRITE], fileno(stderr));
        close(fdpipe[WRITE]);
        execv(script_path, argv);
        break;
    default: /*parent*/
        close(fdpipe[WRITE]);
        waitpid(pid, NULL, 0); /*wait for script termination */
        do {
            len = read(fdpipe[READ], buf, BUFLEN);
            if (len > 0)
                g_string_append_len(script_output, buf, len);
        }
        while (len > 0);
        close(fdpipe[READ]);
        /* display output in window, if any */
        if (strlen(script_output->str)) {
            gtkpod_warning(_("'%s' returned the following output:\n%s\n"), script_path, script_output->str);
        }
        break;
    } /*end switch*/

    /*free everything left*/
    g_free(script_path);
    g_free(buf);
    g_strfreev(argv);
    g_string_free(script_output, TRUE);
    return TRUE;
}

gboolean tools_sync_all(iTunesDB *itdb) {
    gboolean success;

    success = tools_sync_script(itdb, SYNC_CALENDAR);
    if (success)
        success = tools_sync_script(itdb, SYNC_CONTACTS);
    if (success)
        tools_sync_script(itdb, SYNC_NOTES);
    return success;
}

gboolean tools_sync_contacts(iTunesDB *itdb) {
    return tools_sync_script(itdb, SYNC_CONTACTS);
}

gboolean tools_sync_calendar(iTunesDB *itdb) {
    return tools_sync_script(itdb, SYNC_CALENDAR);
}

gboolean tools_sync_notes(iTunesDB *itdb) {
    return tools_sync_script(itdb, SYNC_NOTES);
}
