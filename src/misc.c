/* Time-stamp: <2004-05-20 23:27:13 JST jcs>
|
|  Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "interface.h"
#include "misc.h"
#include "prefs.h"
#include "support.h"


#define DEBUG_MISC 0

/*------------------------------------------------------------------*\
 *                                                                  *
 *             About Window                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

static GtkWidget *about_window = NULL;

void open_about_window ()
{
  GtkLabel *about_label;
  gchar *label_text;
  GtkTextView *textview;
  GtkTextIter ti;
  GtkTextBuffer *tb;

  if (about_window != NULL) return;
  about_window = create_gtkpod_about_window ();
  about_label = GTK_LABEL (lookup_widget (about_window, "about_label"));
  label_text = g_strdup_printf (_("gtkpod Version %s: Cross-Platform Multi-Lingual Interface to Apple's iPod(tm)."), VERSION);
  gtk_label_set_text (about_label, label_text);
  g_free (label_text);
  {
      gchar *text[] = {_("\
(C) 2002 - 2003\n\
Jorg Schuler (jcsjcs at users dot sourceforge dot net)\n\
Corey Donohoe (atmos at atmos dot org)\n\
\n\
\n"),
		       _("\
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.\n\
\n\
\n"),
		       _("\
Patches were supplied by the following people (list may be incomplete -- please contact me)\n\n"),
		       _("\
Ramesh Dharan: Multi-Edit (edit tags of several tracks in one run)\n"),
		       _("\
Hiroshi Kawashima: Japanese charset autodetecion feature\n"),
		       _("\
Adrian Ulrich: porting of playlist code from mktunes.pl to itunesdb.c\n"),
		       _("\
Walter Bell: correct handling of DND URIs with escaped characters and/or cr/newlines at the end\n"),
		       _("\
Sam Clegg: user defined filenames when exporting tracks from the iPod\n"),
		       _("\
Chris Cutler: automatic creation of various playlist types\n"),
		       _("\
Graeme Wilford: reading and writing of the 'Composer' ID3 tags, progress dialogue during sync\n"),
		       _("\
Edward Matteucci: debugging, special playlist creation, most of the volume normalizing code\n"),
		       _("\
Jens Lautenbach: some optical improvements\n"),
		       _("\
Alex Tribble: iPod eject patch\n"),
		       _("\
Yaroslav Halchenko: Orphaned and dangling tracks handling\n"),
		       _("\
Andrew Huntwork: Filename case sensitivity fix and various other bugfixes\n"),
		       "\n\n",
		       _("\
Ero Carrera: Filename validation and quick sync when copying tracks from the iPod\n"),
		       "\n\n",
		       _("\
Jens Taprogge: Support for LAME's replay gain tag to normalize volume\n"),
		       "\n\n",
		       _("\
Armando Atienza: Support with external playcounts\n"),
		       "\n\n",
		       _("\
D.L. Sharp: Support for m4b files (bookmarkable AAC files)\n"),
		       _("\
Jim Hall: Decent INSTALL file\n"),
		       "\n\n",
		       _("\
This program borrows code from the following projects:\n"),
		       _("\
    gnutools: (mktunes.pl, ported to C) reading and writing of iTunesDB (http://www.gnu.org/software/gnupod/)\n"),
		       _("\
    mp3info:  mp3 playlength detection (http://ibiblio.org/mp3info/)\n"),
		       _("\
    xmms:     dirbrowser, mp3 playlength detection (http://www.xmms.org)\n"),
		       "\n",
		       _("\
The GUI was created with the help of glade-2 (http://glade.gnome.org/).\n"),
		       NULL };
      gchar **strp = text;
      textview = GTK_TEXT_VIEW (lookup_widget (about_window, "credits_textview"));
      tb = gtk_text_view_get_buffer (textview);
      while (*strp)
      {
	  gtk_text_buffer_get_end_iter (tb, &ti);
	  gtk_text_buffer_insert (tb, &ti, *strp, -1);
	  ++strp;
      }
  }

 {
     gchar  *text[] = { _("\
French:   David Le Brun (david at dyn-ns dot net)\n"),
				     _("\
German:   Jorg Schuler (jcsjcs at users dot sourceforge dot net)\n"),
				     _("\
Italian:  Edward Matteucci (edward_matteucc at users dot sourceforge dot net)\n"),
				     _("\
Japanese: Ayako Sano\n"),
				     _("\
Japanese: Kentaro Fukuchi (fukuchi at users dot sourceforge dot net)\n"),
				     NULL };
      gchar **strp = text;
      textview = GTK_TEXT_VIEW (lookup_widget (about_window, "translators_textview"));
      tb = gtk_text_view_get_buffer (textview);
      while (*strp)
      {
	  gtk_text_buffer_get_end_iter (tb, &ti);
	  gtk_text_buffer_insert (tb, &ti, *strp, -1);
	  ++strp;
      }
  }

  gtk_widget_show (about_window);
}

void close_about_window (void)
{
  g_return_if_fail (about_window != NULL);
  gtk_widget_destroy (about_window);
  about_window = NULL;
}




/*------------------------------------------------------------------*\
 *                                                                  *
 *             Miscellaneous                                        *
 *                                                                  *
\*------------------------------------------------------------------*/


/* Calculate the time in ms passed since @old_time. @old_time is
   updated with the current time if @update is TRUE*/
float get_ms_since (GTimeVal *old_time, gboolean update)
{
    GTimeVal new_time;
    float result;

    g_get_current_time (&new_time);
    result = (new_time.tv_sec - old_time->tv_sec) * 1000 +
	(float)(new_time.tv_usec - old_time->tv_usec) / 1000;
    if (update)
    {
	old_time->tv_sec = new_time.tv_sec;
	old_time->tv_usec = new_time.tv_usec;
    }
    return result;
}

/* parse a bunch of ipod ids delimited by \n
 * @s - address of the character string we're parsing (gets updated)
 * @id - pointer the ipod id parsed from the string
 * Returns FALSE when the string is empty, TRUE when the string can still be
 *	parsed
 */
gboolean
parse_ipod_id_from_string(gchar **s, guint32 *id)
{
    if(s && (*s))
    {
	gchar *str = *s;
	gchar *strp = strchr (str, '\n');

	if (strp == NULL)
	{
	    *id = 0;
	    *s = NULL;
	    return FALSE;
	}
	*id = (guint32)atoi(str);
	++strp;
	if (*strp) *s = strp;
	else       *s = NULL;
	return TRUE;
    }
    return FALSE;
}



void cleanup_backup_and_extended_files (void)
{
  gchar *cfgdir, *cft, *cfe;

  cfgdir = prefs_get_cfgdir ();
  /* in offline mode, there are no backup files! */
  if (cfgdir && !prefs_get_offline ())
    {
      cft = g_build_filename (cfgdir, "iTunesDB", NULL);
      cfe = g_build_filename (cfgdir, "iTunesDB.ext", NULL);
      if (!prefs_get_write_extended_info ())
	/* delete extended info file from computer */
	if (g_file_test (cfe, G_FILE_TEST_EXISTS))
	  if (remove (cfe) != 0)
	    gtkpod_warning (_("Could not delete backup file: \"%s\"\n"), cfe);
      if (!prefs_get_keep_backups())
	if(g_file_test (cft, G_FILE_TEST_EXISTS))
	  if (remove (cft) != 0)
	    gtkpod_warning (_("Could not delete backup file: \"%s\"\n"), cft);
      g_free (cft);
      g_free (cfe);
    }
  C_FREE (cfgdir);
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             DND to playlists                                     *
 *                                                                  *
\*------------------------------------------------------------------*/

/* DND: add a list of iPod IDs to Playlist @pl */
void add_idlist_to_playlist (Playlist *pl, gchar *string)
{
    guint32 id = 0;
    gchar *str = g_strdup (string);

    if (!pl) return;
    while(parse_ipod_id_from_string(&str,&id))
    {
	add_trackid_to_playlist(pl, id, TRUE);
    }
    data_changed();
    g_free (str);
}

/* DND: add a list of files to Playlist @pl.  @pl: playlist to add to
   or NULL. If NULL, a "New Playlist" will be created for adding
   tracks and when adding a playlist file, a playlist with the name of the
   playlist file will be added.
   @trackaddfunc: passed on to add_track_by_filename() etc. */
void add_text_plain_to_playlist (Playlist *pl, gchar *str, gint pl_pos,
				 AddTrackFunc trackaddfunc, gpointer data)
{
    gchar **files = NULL, **filesp = NULL;
    Playlist *pl_playlist = pl; /* playlist for playlist file */

    if (!str)  return;

    /*   printf("pl: %x, pl_pos: %d\n%s\n", pl, pl_pos, str);*/

    block_widgets ();

    files = g_strsplit (str, "\n", -1);
    if (files)
    {
	filesp = files;
	while (*filesp)
	{
	    gboolean added = FALSE;
	    gint file_len = -1;

	    gchar *file = NULL;
	    gchar *decoded_file = NULL;

	    file = *filesp;
	    /* file is in uri form (the ones we're looking for are
	       file:///), file can include the \n or \r\n, which isn't
	       a valid character of the filename and will cause the
	       uri decode / file test to fail, so we'll cut it off if
	       its there. */
	    file_len = strlen (file);
	    if (file_len && (file[file_len-1] == '\n'))
	    {
		file[file_len-1] = 0;
		--file_len;
	    }
	    if (file_len && (file[file_len-1] == '\r'))
	    {
		file[file_len-1] = 0;
		--file_len;
	    }

	    decoded_file = filename_from_uri (file, NULL, NULL);
	    if (decoded_file != NULL)
	    {
		if (g_file_test (decoded_file, G_FILE_TEST_IS_DIR))
		{   /* directory */
		    if (!pl)
		    {  /* no playlist yet -- create new one */
			pl = add_new_playlist_user_name (NULL, pl_pos);
			if (!pl)  break; /* while (*filesp) */
		    }
		    add_directory_by_name (decoded_file, pl,
					   prefs_get_add_recursively (),
					   trackaddfunc, data);
		    added = TRUE;
		}
		if (g_file_test (decoded_file, G_FILE_TEST_IS_REGULAR))
		{   /* regular file */
		    gint decoded_len = strlen (decoded_file);
		    if (decoded_len >= 4)
		    {
			if (strcasecmp (&decoded_file[decoded_len-4],
					".mp3") == 0)
			{   /* mp3 file */
			    if (!pl)
			    {  /* no playlist yet -- create new one */
				pl = add_new_playlist_user_name (NULL,
								 pl_pos);
				if (!pl)  break; /* while (*filesp) */
			    }
			    add_track_by_filename (decoded_file, pl,
						  prefs_get_add_recursively (),
						  trackaddfunc, data);
			    added = TRUE;
			}
			else if ((strcasecmp (&decoded_file[decoded_len-4],
					      ".plu") == 0) ||
				 (strcasecmp (&decoded_file[decoded_len-4],
					      ".m3u") == 0))
			{
			    add_playlist_by_filename (decoded_file,
						      pl_playlist,
						      trackaddfunc, data);
			    added = TRUE;
			}
		    }
		}
		g_free (decoded_file);
	    }
	    if (!added)
	    {
		if (strlen (*filesp) != 0)
		    gtkpod_warning (_("drag and drop: ignored '%s'\n"), *filesp);
	    }
	    ++filesp;
	}
	g_strfreev (files);
    }
    /* display log of non-updated tracks */
    display_non_updated (NULL, NULL);
    /* display log updated tracks */
    display_updated (NULL, NULL);
    /* display log of detected duplicates */
    remove_duplicate (NULL, NULL);

    release_widgets ();
}


/* ------------------------------------------------------------
 *
 * Code to "eject" the iPod.
 *
 * Large parts of this code are Copyright (C) 1994-2001 Jeff Tranter
 * (tranter at pobox.com).  Repackaged to only include scsi eject code
 * by Alex Tribble (prat at rice.edu). Integrated into gtkpod by Jorg
 * Schuler (jcsjcs at users.sourceforge.net)
 *
 * ------------------------------------------------------------ */

/* This code does not work with FreeBSD -- let me know if you know a
 * way around it. */

#if (HAVE_LINUX_CDROM_H && HAVE_SCSI_SCSI_H && HAVE_SCSI_SCSI_IOCTL_H)
#include <linux/cdrom.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

static gchar *getdevicename(const gchar *mount)
{
    if(mount) {
	gchar device[256], point[256];
	FILE *fp = fopen("/proc/mounts","r");
	if (!fp)
	    return NULL;
	do {
	    fscanf(fp, "%255s %255s %*s %*s %*s %*s", device, point);
	    if( strcmp(mount,point) == 0 ) {
		fclose(fp);
		return strdup(device);
	    }
	} while(!feof(fp));
	fclose(fp);
    }
    return NULL;
}

/*
 * Eject using CDROMEJECT ioctl. Return TRUE if successful, FALSE otherwise.
 */
static gboolean EjectCdrom(int fd)
{
	int status;

	status = ioctl(fd, CDROMEJECT);
	return (status == 0);
}

/*
 * Eject using SCSI commands. Return TRUE if successful, FALSE otherwise.
 */
static gboolean EjectScsi(int fd)
{
    int status;
    struct sdata {
	int inlen;
	int outlen;
	char cmd[256];
    } scsi_cmd;

    scsi_cmd.inlen = 0;
    scsi_cmd.outlen = 0;
    scsi_cmd.cmd[0] = ALLOW_MEDIUM_REMOVAL;
    scsi_cmd.cmd[1] = 0;
    scsi_cmd.cmd[2] = 0;
    scsi_cmd.cmd[3] = 0;
    scsi_cmd.cmd[4] = 0;
    scsi_cmd.cmd[5] = 0;
    status = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, (void *)&scsi_cmd);
    if (status != 0)	return FALSE;

    scsi_cmd.inlen = 0;
    scsi_cmd.outlen = 0;
    scsi_cmd.cmd[0] = START_STOP;
    scsi_cmd.cmd[1] = 0;
    scsi_cmd.cmd[2] = 0;
    scsi_cmd.cmd[3] = 0;
    scsi_cmd.cmd[4] = 1;
    scsi_cmd.cmd[5] = 0;
    status = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, (void *)&scsi_cmd);
    if (status != 0)	return FALSE;

    scsi_cmd.inlen = 0;
    scsi_cmd.outlen = 0;
    scsi_cmd.cmd[0] = START_STOP;
    scsi_cmd.cmd[1] = 0;
    scsi_cmd.cmd[2] = 0;
    scsi_cmd.cmd[3] = 0;
    scsi_cmd.cmd[4] = 2;
    scsi_cmd.cmd[5] = 0;
    status = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, (void *)&scsi_cmd);
    if (status != 0)	return FALSE;

    status = ioctl(fd, BLKRRPART);
    return (status == 0);
}

static void eject_ipod(gchar *ipod_device)
{
     if(ipod_device)
     {
	  int fd = open (ipod_device, O_RDONLY|O_NONBLOCK);
	  if (fd == -1)
	  {
/*	      fprintf(stderr, "Unable to open '%s' to eject.\n",ipod_device); */
	      /* just ignore the error, i.e. if we don't have write access */
	  }
	  else
	  {
	      /* 2G seems to need EjectCdrom(), 3G EjectScsi()? */
	      if (!EjectCdrom (fd))
		  EjectScsi (fd);
	      close (fd);
	  }
     }
}
#else
static gchar *getdevicename(const gchar *mount)
{
    return NULL;
}
static void eject_ipod(gchar *ipod_device)
{
}
#endif

/* ------------------------------------------------------------
 *                    end of eject code
 * ------------------------------------------------------------ */


/***************************************************************************
 * Mount Calls
 *
 **************************************************************************/
/**
 * mount_ipod - attempt to mount the ipod to prefs_get_ipod_mount() This
 * does not check prefs to see if the current prefs want gtkpod itself to
 * mount the ipod drive, that should be checked before making this call.
 */
void
mount_ipod(void)
{
    const gchar *str = prefs_get_ipod_mount ();
    if(str)
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	    case 0: /* child */
		execl(MOUNT_BIN, "mount", str, NULL);
		exit(0);
		break;
	    case -1: /* parent and error */
		break;
	    default: /* parent -- let's wait for the child to terminate */
		tpid = waitpid (pid, &status, 0);
		/* we could evaluate tpid and status now */
		break;
	}
    }
}

/**
 * umount_ipod - attempt to unmount the ipod from prefs_get_ipod_mount()
 * This does not check prefs to see if the current prefs want gtkpod itself
 * to unmount the ipod drive, that should be checked before making this
 * call.
 * After unmounting, an attempt to send the "eject" signal is
 * made. Errors are ignored.
 */
void
unmount_ipod(void)
{
    const gchar *str = prefs_get_ipod_mount ();
    gchar *ipod_device = getdevicename (prefs_get_ipod_mount());
    if(str)
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	    case 0: /* child */
		execl(UMOUNT_BIN, "umount", str, NULL);
		exit(0);
		break;
	    case -1: /* parent and error */
		break;
	    default: /* parent -- let's wait for the child to terminate */
		tpid = waitpid (pid, &status, 0);
		eject_ipod (ipod_device);
		/* we could evaluate tpid and status now */
		break;
	}
    }
    g_free (ipod_device);
}


/***************************************************************************
 * gtkpod.in,out calls
 *
 **************************************************************************/

/* tries to call "/bin/sh @script" */
static void do_script (gchar *script)
{
    if (script)
    {
	pid_t pid, tpid;
	int status;

	pid = fork ();
	switch (pid)
	{
	case 0: /* child */
	    execl("/bin/sh", "sh", script, NULL);
	    exit(0);
	break;
	case -1: /* parent and error */
	break;
	default: /* parent -- let's wait for the child to terminate */
	    tpid = waitpid (pid, &status, 0);
	    /* we could evaluate tpid and status now */
	    break;
	}
    }
}


/* tries to execute "/bin/sh ~/.gtkpod/@script" or
 * "/bin/sh /etc/gtkpod/@script" if the former does not exist */
void call_script (gchar *script)
{
    gchar *cfgdir;
    gchar *file;

    if (!script) return;

    cfgdir =  prefs_get_cfgdir ();
    file = g_build_filename (cfgdir, script, NULL);
    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
	do_script (file);
    }
    else
    {
	C_FREE (file);
	file = g_build_filename ("/etc/gtkpod/", script, NULL);
	if (g_file_test (file, G_FILE_TEST_EXISTS))
	{
	    do_script (file);
	}
    }
    C_FREE (file);
    C_FREE (cfgdir);
}



/**
 * which - run the shell command which, useful for querying default values
 * for executable,
 * @name - the executable we're trying to find the path for
 * Returns the path to the executable, NULL on not found
 */
gchar*
which(const gchar *exe)
{
    FILE *fp = NULL;
    gchar *result = NULL;
    gchar buf[PATH_MAX];
    gchar *which_exec = NULL;

    memset(&buf[0], 0, PATH_MAX);
    which_exec = g_strdup_printf("which %s", exe);
    if((fp = popen(which_exec, "r")))
    {
	int read_bytes = 0;
	if((read_bytes = fread(buf, sizeof(gchar), PATH_MAX, fp)) > 0)
	    result = g_strndup(buf, read_bytes-1);
	pclose(fp);
    }
    C_FREE(which_exec);
    return(result);
}


/* compare @str1 and @str2 case-sensitively or case-insensitively
 * depending on prefs settings */
gint compare_string (gchar *str1, gchar *str2)
{
    if (prefs_get_case_sensitive ())
	return strcmp (str1, str2);
    else
	return compare_string_case_insensitive (str1, str2);
}


/* compare @str1 and @str2 case-sensitively or case-insensitively
 * depending on prefs settings */
gint compare_string_case_insensitive (gchar *str1, gchar *str2)
{
    gchar *string1 = g_utf8_casefold (str1, -1);
    gchar *string2 = g_utf8_casefold (str2, -1);
    gint result = g_utf8_collate (string1, string2);
    g_free (string1);
    g_free (string2);
    return result;
}


/* ------------------------------------------------------------
------------------------------------------------------------------
--------                                                 ---------
--------                 UTF16 section                   ---------
--------                                                 ---------
------------------------------------------------------------------
   ------------------------------------------------------------ */

/* Get length of utf16 string in number of characters (words) */
guint32 utf16_strlen (gunichar2 *utf16)
{
  guint32 i=0;
  if (utf16)
      while (utf16[i] != 0) ++i;
  return i;
}

/* duplicate a utf16 string */
gunichar2 *utf16_strdup (gunichar2 *utf16)
{
    guint32 len;
    gunichar2 *new = NULL;

    if (utf16)
    {
	len = utf16_strlen (utf16);
	new = g_malloc (sizeof (gunichar2) * (len+1));
	if (new) memcpy (new, utf16, sizeof (gunichar2) * (len+1));
    }
    return new;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *  Generic functions to "do" things on selected playlist / entry   *
 *  / tracks                                                         *
 *                                                                  *
\*------------------------------------------------------------------*/

/* Make a list of all selected tracks and call @do_func with that list
   as argument */
void do_selected_tracks (void (*do_func)(GList *trackids))
{
    GList *selected_trackids = NULL;

    if (!do_func) return;

    /* I'm using ids instead of "Track *" -pointer because it would be
     * possible that a track gets removed during the process */
    selected_trackids = tm_get_selected_trackids();
    do_func (selected_trackids);
    g_list_free (selected_trackids);
}


/* Make a list of all tracks in the currently selected entry of sort
   tab @inst and call @do_func with that list as argument */
void do_selected_entry (void (*do_func)(GList *trackids), gint inst)
{
    GList *selected_trackids = NULL;
    TabEntry *entry;
    GList *gl;

    if (!do_func) return;

    if ((inst < 0) || (inst > prefs_get_sort_tab_num ()))   return;

    entry = st_get_selected_entry (inst);
    if (entry == NULL)
    {  /* no entry selected */
	gtkpod_statusbar_message (_("No entry selected."));
	return;
    }
    for (gl=entry->members; gl; gl=gl->next)
    { /* make a list with all trackids in this entry */
	Track *track = (Track *)gl->data;
	if (track)
	    selected_trackids = g_list_append (selected_trackids,
					      (gpointer)track->ipod_id);
    }
    do_func (selected_trackids);
    g_list_free (selected_trackids);
}


/* Make a list of the tracks in the current playlist and call @do_func
   with that list as argument */
void do_selected_playlist (void (*do_func)(GList *trackids))
{
    GList *selected_trackids = NULL;
    Playlist *pl;
    GList *gl;

    if (!do_func) return;

    pl = pm_get_selected_playlist();
    if (!pl)
    { /* no playlist selected */
	gtkpod_statusbar_message (_("No playlist selected."));
	return;
    }
    for (gl=pl->members; gl; gl=gl->next)
    { /* make a list with all trackids in this entry */
	Track *track = (Track *)gl->data;
	if (track)
	    selected_trackids = g_list_append (selected_trackids,
					      (gpointer)track->ipod_id);
    }
    do_func (selected_trackids);
    g_list_free (selected_trackids);
}


/* return some sensible input about the "track". Yo must free the
 * return string after use. */
gchar *get_track_info (Track *track)
{
    if (!track) return NULL;
    if (track->pc_path_utf8 && strlen(track->pc_path_utf8))
	return g_path_get_basename (track->pc_path_utf8);
    else if ((track->title && strlen(track->title)))
	return g_strdup (track->title);
    else if ((track->album && strlen(track->album)))
	return g_strdup (track->album);
    else if ((track->artist && strlen(track->artist)))
	return g_strdup (track->artist);
    return g_strdup_printf ("iPod ID: %d", track->ipod_id);
}



