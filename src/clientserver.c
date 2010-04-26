/*
 |  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

#include "config.h"
#include "clientserver.h"
#include "sha1.h"
#include "misc.h"
#include "gp_itdb.h"
#include "prefs.h"
#include <glib/gi18n-lib.h>
#include <errno.h>
#include <fcntl.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static gint ssock = -1;
static struct sockaddr_un *saddr = NULL;
static guint inp_handler;

const gchar *SOCKET_TEST = "TEST:";
const gchar *SOCKET_PLYC = "PLYC:";

#ifndef HAVE_FLOCK
/* emulate flock on systems that do not have it */
int flock(int fd, int operation)
{
    struct flock f;
    memset(&f, 0, sizeof (f));

    switch (operation)
    {
        case LOCK_EX:
        f.l_type = F_WRLCK;
        return fcntl(fd, F_SETLKW, &f);
        default:
        g_warning ("*** flock operation '%d' not implemented.\n",
                operation);
        return -1;
    }
}
#endif

/* set the path to the socket name */
static void set_path(struct sockaddr_un *saddr) {
    snprintf(saddr->sun_path, sizeof(saddr->sun_path), "%s%sgtkpod-%s", g_get_tmp_dir(), G_DIR_SEPARATOR_S, g_get_user_name());
}

/* checks if the socket "/tmp/gtkpod-<user>" is already being used.
 Return value:
 TRUE: socket already being used
 FALSE: socket not being used */
static gboolean socket_used() {
    struct sockaddr_un *server;
    gboolean result = FALSE;

    server = g_malloc0(sizeof(struct sockaddr_un));
    set_path(server);
    if (g_file_test(server->sun_path, G_FILE_TEST_EXISTS)) {
        gint csock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (csock != -1) {
            server->sun_family = AF_UNIX;
            if (connect(csock, (struct sockaddr *) server, sizeof(struct sockaddr_un)) != -1) {
                size_t socket_test_len = strlen(SOCKET_TEST);
                if (write(csock, SOCKET_TEST, socket_test_len) == (ssize_t) socket_test_len)
                    result = TRUE;
            }
            close(csock);
        }
    }
    g_free(server);
    return result;
}

/* append the filename <file> to ~/.gtkpod/offline_playcount */
static gboolean register_playcount(gchar *file) {
    if (file && *file) {
        gchar *cfgdir = prefs_get_cfgdir();

        if (cfgdir) {
            gchar *offlplyc = g_strdup_printf("%s%c%s", cfgdir, G_DIR_SEPARATOR, "offline_playcount");
            int fd = open(offlplyc, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
                    |S_IWOTH);
            if (fd != -1) {
                if (flock(fd, LOCK_EX) == 0) {
                    gchar *sha1 = sha1_hash_on_filename(file, TRUE);
                    write(fd, SOCKET_PLYC, strlen(SOCKET_PLYC));
                    if (sha1)
                        write(fd, sha1, strlen(sha1));
                    write(fd, " ", 1);
                    write(fd, file, strlen(file));
                    write(fd, "\n", 1);
                    g_free(sha1);
                }
                else {
                    fprintf(stderr, "couldn't lock %s\n", file);
                }
                close(fd);
            }
            else {
                fprintf(stderr, "couldn't open %s\n", file);
            }
            g_free(offlplyc);
            g_free(cfgdir);
        }
    }
    return TRUE;
}

void received_message(gpointer data, gint source, GdkInputCondition condition) {
    gint csock, rval;
    gchar *buf;
    /*    printf("received message\n");*/

    buf = g_malloc(PATH_MAX);
    while ((csock = accept(source, 0, 0)) != -1) {
        do {
            bzero(buf, PATH_MAX);
            if ((rval = read(csock, buf, PATH_MAX)) < 0) {
                fprintf(stderr, "server: read error: %s", strerror(errno));
                continue;
            }
            else if (rval == 0) {
                /* forget about it */
            }
            else {
                if (strncmp(buf, SOCKET_TEST, strlen(SOCKET_TEST)) == 0) {
                    continue; /* skip socket tests */
                }
                if (strncmp(buf, SOCKET_PLYC, strlen(SOCKET_PLYC)) == 0) {
                    gchar *file = buf + strlen(SOCKET_PLYC);
                    if (gp_increase_playcount(NULL, file, 1) == FALSE) { /* didn't find the track --> write to
                     offline_playcount */
                        register_playcount(file);
                    }
                }
            }
        }
        while (rval > 0);
        close(csock);
    }
    g_free(buf);
}

void server_setup(void) {
    if (ssock != -1)
        return; /* already opened */
    if (socket_used()) { /* we are not the first instance of gtkpod -- the socket is
     already being used, so we pass */
        gtkpod_warning(_("Another instance of gtkpod was detected. Playcount server not started.\n"));
        return;
    }
    ssock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ssock != -1) {
        if (saddr == NULL) {
            saddr = g_malloc0(sizeof(struct sockaddr_un));
            saddr->sun_family = AF_UNIX;
        }
        set_path(saddr);
        unlink(saddr->sun_path);
        if (bind(ssock, (struct sockaddr *) saddr, sizeof(struct sockaddr_un)) != -1) {
            listen(ssock, 5);
            /* socket must be non-blocking -- otherwise
             received_message() will block */
            fcntl(ssock, F_SETFL, O_NONBLOCK);
            inp_handler = gtk_input_add_full(ssock, GDK_INPUT_READ, received_message, NULL, NULL, NULL);
        }
        else {
            fprintf(stderr, "server: bind error: %s", strerror(errno));
            close(ssock);
            ssock = -1;
        }
    }
    else {
        fprintf(stderr, "server: socket error: %s", strerror(errno));
    }
}

void server_shutdown(void) {
    if (ssock != -1) {
        gtk_input_remove(inp_handler);
        close(ssock);
        ssock = -1;
    }
    if (saddr) {
        if (strlen(saddr->sun_path) != 0)
            unlink(saddr->sun_path);
        g_free(saddr);
        saddr = 0;
    }
}

/* Increment the playcount of <file> by one. Either connect to a
 running instance of gtkpod and transfer the filename, or write the
 name to ~/.gtkpod/offline_playcounts.
 Return value: TRUE on success, FALSE if a non-recoverable error
 occurred */
gboolean client_playcount(gchar *file) {
    if (socket_used()) { /* send filename to currently running gtkpod instance */
        struct sockaddr_un *server;

        server = g_malloc0(sizeof(struct sockaddr_un));
        set_path(server);
        if (g_file_test(server->sun_path, G_FILE_TEST_EXISTS)) {
            gint csock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (csock != -1) {
                server->sun_family = AF_UNIX;
                if (connect(csock, (struct sockaddr *) server, sizeof(struct sockaddr_un)) != -1) {
                    gchar *buf = g_strdup_printf("%s%s", SOCKET_PLYC, file);
                    size_t buf_len = strlen(buf);
                    if (write(csock, buf, buf_len) != (ssize_t) buf_len) {
                        fprintf(stderr, "Error communicating to server. Playcount registered in offline database.\n");
                        register_playcount(file);
                    }
                    g_free(buf);
                }
                close(csock);
            }
        }
        g_free(server);
    }
    else { /* write filename to ~/.gtkpod/offline_playcounts */
        register_playcount(file);
    }
    return TRUE;
}

/* print out the "sha1" hash of a filename <file> */
gboolean print_sha1_hash(gchar *file) {
    if (file && *file) {
        gchar *hash = sha1_hash_on_filename(file, TRUE);
        if (hash)
            fprintf(stdout, "%s\n", hash);
        g_free(hash);
    }
    return TRUE;
}

