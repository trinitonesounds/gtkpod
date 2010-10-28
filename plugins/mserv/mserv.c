/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include "libgtkpod/prefs.h"
#include "libgtkpod/misc_track.h"
#include "libgtkpod/misc.h"
#include "mserv.h"
#include "plugin.h"

/* Updates mserv data (rating only) of @track using filename @name to
 look up mserv information.
 Return TRUE if successfully updated (or disabled), FALSE if not */
gboolean update_mserv_data_from_file(gchar *name, Track *track) {
    gboolean success = TRUE;

    if (!name || !track)
        return FALSE;

    if (g_file_test(name, G_FILE_TEST_IS_DIR))
        return FALSE;
    if (!g_file_test(name, G_FILE_TEST_EXISTS)) {
        gchar *buf = g_strdup_printf(_("Local filename not valid (%s)"), name);
        display_mserv_problems(track, buf);
        g_free(buf);
        return FALSE;
    }

    if (prefs_get_int("mserv_use")) {
        /* try getting the user's rating from the mserv db */
        gchar *music_root = prefs_get_string("path_mserv_music_root");
        gchar *trackinfo_root = prefs_get_string("path_mserv_trackinfo_root");

        /* we expect music_root and trackinfo_root to be initialized */
        if (!music_root)
            music_root = g_strdup("");
        if (!trackinfo_root)
            trackinfo_root = g_strdup("");

        success = FALSE;
        /* printf("mroot %s troot %s fname %s\n", music_root, trackinfo_root, name); */

        /* first, check if the file is in the mserv music directory */
        if (*music_root == 0 || strstr(name, music_root)) {
            gchar *infoname =
                    g_strdup_printf("%s%c%s.trk", trackinfo_root, G_DIR_SEPARATOR, &(name[strlen(music_root)]));
            /* printf("trying %s\n", infoname); */
            FILE *fp = fopen(infoname, "r");
            if (fp) {
                /* printf("opened\n");*/
                gchar buff[PATH_MAX];
                gchar *username = prefs_get_string("mserv_username");
                guint usernamelen;
                g_return_val_if_fail (username, (fclose (fp), FALSE));
                usernamelen = strlen(username);
                while (fgets(buff, PATH_MAX, fp)) {
                    /* printf("username %s (%d) read %s\n",
                     * prefs_get_string("mserv_username"), usernamelen,
                     * buff);*/
                    if (strncmp(buff, username, usernamelen) == 0 && buff[usernamelen] == (gchar) '=') {
                        /* found it */
                        track->rating = atoi(&buff[usernamelen + 1]) * ITDB_RATING_STEP;
                        /* printf("found it, = %d\n",
                         orig_track->rating/ITDB_RATING_STEP); */
                        success = TRUE;
                        break; /* while(fgets(... */
                    }
                }
                fclose(fp);
                g_free(username);
                if (!success) {
                    gchar *username = prefs_get_string("mserv_username");
                    gchar *buf = g_strdup_printf(_("No information found for user '%s' in '%s'"), username, infoname);
                    display_mserv_problems(track, buf);
                    g_free(buf);
                    g_free(username);
                }
            }
            else {
                gchar *buf = g_strdup_printf(_("mserv data file (%s) not available for track (%s)"), infoname, name);
                display_mserv_problems(track, buf);
                g_free(buf);
            }
            g_free(infoname);
        }
        else {
            gchar *buf = g_strdup_printf(_("Track (%s) not in mserv music root directory (%s)"), name, music_root);
            display_mserv_problems(track, buf);
            g_free(buf);
        }
        g_free(music_root);
        g_free(trackinfo_root);
    }

    while (widgets_blocked && gtk_events_pending())
        gtk_main_iteration();

    return success;
}

/* Logs tracks (@track) that could not be updated with info from mserv
 database for some reason. Pop up a window with the log by calling
 with @track = NULL, or remove the log by calling with @track = -1.
 @txt (if available) is added as an explanation as to why it was
 impossible to retrieve mserv information */
void display_mserv_problems(Track *track, gchar *txt) {
    gchar *buf;
    static gint track_nr = 0;
    static GString *str = NULL;

    if ((track == NULL) && str) {
        if (prefs_get_int("mserv_use") && prefs_get_int("mserv_report_probs") && str->len) { /* Some tracks have had problems. Print a notice */
            buf
                    = g_strdup_printf(ngettext("No mserv information could be retrieved for the following track", "No mserv information could be retrieved for the following %d tracks", track_nr), track_nr);
            gtkpod_confirmation(-1, /* gint id, */
            FALSE, /* gboolean modal, */
            _("mserv data retrieval problem"), /* title */
            buf, /* label */
            str->str, /* scrolled text */
            NULL, 0, NULL, /* option 1 */
            NULL, 0, NULL, /* option 2 */
            TRUE, /* gboolean confirm_again, */
            "mserv_report_probs",/* confirm_again_key,*/
            CONF_NULL_HANDLER, /* ConfHandler ok_handler,*/
            NULL, /* don't show "Apply" button */
            NULL, /* cancel_handler,*/
            NULL, /* gpointer user_data1,*/
            NULL); /* gpointer user_data2,*/
            g_free(buf);
        }
        display_mserv_problems((void *) -1, NULL);
    }

    if (track == (void *) -1) { /* clean up */
        if (str)
            g_string_free(str, TRUE);
        str = NULL;
        track_nr = 0;
        gtkpod_tracks_statusbar_update();
    }
    else if (prefs_get_int("mserv_use") && prefs_get_int("mserv_report_probs") && track) {
        /* add info about it to str */
        buf = get_track_info(track, TRUE);
        if (!str) {
            track_nr = 0;
            str = g_string_sized_new(2000); /* used to keep record */
        }
        if (txt)
            g_string_append_printf(str, "%s (%s)\n", buf, txt);
        else
            g_string_append_printf(str, "%s\n", buf);
        g_free(buf);
        ++track_nr; /* count tracks */
    }
}

/*------------------------------------------------------------------*\
 *                                                                  *
 *      Update mserv Data from File                                 *
 *                                                                  *
 \*------------------------------------------------------------------*/

/* reads info from file and updates the ID3 tags of
 @selected_tracks. */
void mserv_from_file_tracks(GList *selected_tracks) {
    GList *gl;

    if (selected_tracks == NULL) {
        gtkpod_statusbar_message(_("Nothing to update"));
        return;
    }

    block_widgets();
    for (gl = selected_tracks; gl; gl = gl->next) {
        ExtraTrackData *etr;
        Track *track = gl->data;
        gchar *buf;
        g_return_if_fail (track);
        etr = track->userdata;
        g_return_if_fail (etr);

        buf = get_track_info(track, TRUE);
        gtkpod_statusbar_message (_("Retrieving mserv data %s"), buf);
        g_free(buf);
        if (etr->pc_path_locale && *etr->pc_path_locale)
            update_mserv_data_from_file(etr->pc_path_locale, track);
        else
            display_mserv_problems(track, _("no filename available"));
    }
    release_widgets();
    /* display log of problems with mserv data */
    display_mserv_problems(NULL, NULL);
    gtkpod_statusbar_message(_("Updated selected tracks with data from mserv."));
}
