/*
|  Copyright (C) 2002 Jorg Schuler <jcsjcs at sourceforge.net>
|  Part of the gtkpod project.
| 
|  URL: 
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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "prefs.h"
#include "support.h"

struct cfg *cfg = NULL;


static void usage (FILE *file)
{
  fprintf(file, _("gtkpod version %s usage:\n"), VERSION);
  fprintf(file, _("  -h, --help:   display this message\n"));
  fprintf(file, _("  -m path:      define the mountpoint of your iPod\n"));
  fprintf(file, _("  --mountpoint: same as \"-m\".\n"));
  fprintf(file, _("  -w:           write changed ID3 tags to file\n"));
  fprintf(file, _("  --writeid3:   same as \"-w\".\n"));
  fprintf(file, _("  -c:           check files automagically for duplicates\n"));
  fprintf(file, _("  --md5:   same as \"-c\".\n"));
}

struct cfg*
cfg_new(void)
{
    struct cfg *mycfg = NULL;
    gchar buf[PATH_MAX], *str;

    mycfg = g_malloc0 (sizeof (struct cfg));
    memset(mycfg, 0, sizeof(struct cfg));
    if(getcwd(buf, PATH_MAX))
    {
	mycfg->last_dir.dir_browse = g_strdup_printf ("%s/", buf);
	mycfg->last_dir.file_browse = g_strdup_printf ("%s/", buf);
    }
    else
    {
	mycfg->last_dir.dir_browse = g_strdup ("~/");
	mycfg->last_dir.file_browse = g_strdup ("~/");
    }
    if((str = getenv("IPOD_MOUNTPOINT")))
    {
	snprintf(buf, PATH_MAX, "%s", str);
	mycfg->ipod_mount = g_strdup_printf("%s/", buf);
    }
    else
    {
	mycfg->ipod_mount = g_strdup ("/mnt");
    }
    return(mycfg);
}

static gchar*
prefs_readline(char *buf, int maxlen, int *last)
{
    gchar *result = NULL;
    int begin = PATH_MAX;
    int i = 0;
    
    if((buf) && (buf[begin] != '#')) /* allow comments */
    {
	for(i = 0;((i < maxlen) && (buf[i] != '\n')); i++)
	{
	    if((begin > i) && ((buf[i] != ' ') || (buf[i] != '\0'))) 
		begin = i;
	}
	if((i > 0) && (i <= maxlen))
	{
	    int len = (i - begin);
	    result = g_malloc0(len + 1);
	    snprintf(result, len + 1, "%s", &buf[begin]);
	    *last +=  i;
	}
    }
    return(result);
}
static void
read_prefs_from_file_desc(FILE *fp)
{
    gchar buf[PATH_MAX];
    int length = 0, i = 0;

    if(fp)
    {
	gchar *tags[] = {
	    "mp=", "id3=", "md5=", "album=", "track=", "genre=", "artist=",
	    "delete_file=", "delete_playlist=", "delete_ipod="
	};
	gchar *line = NULL;
	length = fread(buf, 1, PATH_MAX, fp);
	
	for(i = 0; i < length; i++)
	{
	    if((line = prefs_readline(&buf[i], length - i, &i)))
	    {
		if((g_strstr_len(line, strlen(tags[0]), tags[0])))
		{
		    gchar mount_point[PATH_MAX];
		    snprintf(mount_point, PATH_MAX, "%s", &line[3]);
		    prefs_set_mount_point(mount_point);
		}
		else if((g_strstr_len(line, strlen(tags[1]), tags[1])))
		{
		    prefs_set_writeid3_active((gboolean)atoi(&line[4]));
		}
		else if((g_strstr_len(line, strlen(tags[2]), tags[2])))
		{
		    prefs_set_md5songs_active((gboolean)atoi(&line[4]));
		}
		else if((g_strstr_len(line, strlen(tags[3]), tags[3])))
		{
		    prefs_set_song_list_show_album((gboolean)atoi(&line[6]));
		}
		else if((g_strstr_len(line, strlen(tags[4]), tags[4])))
		{
		    prefs_set_song_list_show_track((gboolean)atoi(&line[6]));
		}
		else if((g_strstr_len(line, strlen(tags[5]), tags[5])))
		{
		    prefs_set_song_list_show_genre((gboolean)atoi(&line[6]));
		}
		else if((g_strstr_len(line, strlen(tags[6]), tags[6])))
		{
		    prefs_set_song_list_show_artist((gboolean)atoi(&line[7]));
		}
		else if((g_strstr_len(line, strlen(tags[7]), tags[7])))
		{
		    prefs_set_song_playlist_deletion((gboolean)atoi(&line[12]));
		}
		else if((g_strstr_len(line, strlen(tags[8]), tags[8])))
		{
		    prefs_set_playlist_deletion((gboolean)atoi(&line[16]));
		}
		else if((g_strstr_len(line, strlen(tags[9]), tags[9])))
		{
		    prefs_set_song_ipod_file_deletion((gboolean)atoi(&line[12]));
		}
		g_free(line);
	    }
	}
    }
}

void
read_prefs_defaults(void)
{
    gchar cfgdir[PATH_MAX], filename[PATH_MAX], *str = NULL;
    FILE *fp = NULL;

    if((str = getenv("HOME")))
    {
	snprintf(cfgdir, PATH_MAX, "%s/.gtkpod", str);
	if(g_file_test(cfgdir, G_FILE_TEST_IS_DIR))
	{
	    snprintf(filename, PATH_MAX, "%s/prefs", cfgdir);
	    if((fp = fopen(filename, "r")))
	    {
		read_prefs_from_file_desc(fp);
		fclose(fp);
	    }
	    else
	    {
		fprintf(stderr, "Unable to open %s for reading\n", filename);
	    }
	}
    }

}
/* Read Preferences and initialise the cfg-struct */
gboolean read_prefs (int argc, char *argv[])
{
  int opt;
  int option_index;
  struct option const options[] =
    {
      { "h",           no_argument,	NULL, GP_HELP },
      { "help",        no_argument,	NULL, GP_HELP },
      { "m",           required_argument,	NULL, GP_MOUNT },
      { "mountpoint",  required_argument,	NULL, GP_MOUNT },
      { "w",           no_argument,	NULL, GP_WRITEID3 },
      { "writeid3",    no_argument,	NULL, GP_WRITEID3 },
      { "c",           no_argument,	NULL, GP_MD5SONGS },
      { "md5",		no_argument,	NULL, GP_MD5SONGS },
      { 0, 0, 0, 0 }
    };
  
  if (cfg != NULL) discard_prefs ();
  
  cfg = cfg_new();
    
  read_prefs_defaults();
  while((opt=getopt_long_only(argc, argv, "", options, &option_index)) != -1) {
    switch(opt) 
      {
      case GP_HELP:
	usage(stdout);
	exit(0);
	break;
      case GP_MOUNT:
	g_free (cfg->ipod_mount);
	cfg->ipod_mount = g_strdup (optarg);
	break;
      case GP_WRITEID3:
	cfg->writeid3 = TRUE;
	break;
      case GP_MD5SONGS:
	cfg->md5songs = TRUE;
	break;
      default:
	fprintf(stderr, _("Unknown option: %s\n"), argv[optind]);
	usage(stderr);
	exit(1);
      }
  }
  return TRUE;
}

static void
write_prefs_to_file_desc(FILE *fp)
{
    if(!fp)
	fp = stderr;
    
    fprintf(fp, "mp=%s\n", cfg->ipod_mount);
    fprintf(fp, "id3=%d\n",cfg->writeid3);
    fprintf(fp, "md5=%d\n",cfg->md5songs);
    fprintf(fp, "album=%d\n",prefs_get_song_list_show_album());
    fprintf(fp, "track=%d\n",prefs_get_song_list_show_track());
    fprintf(fp, "genre=%d\n",prefs_get_song_list_show_genre());
    fprintf(fp, "artist=%d\n",prefs_get_song_list_show_artist());
    fprintf(fp, "delete_file=%d\n",prefs_get_song_playlist_deletion());
    fprintf(fp, "delete_playlist=%d\n",prefs_get_playlist_deletion());
    fprintf(fp, "delete_ipod=%d\n",prefs_get_song_ipod_file_deletion());
}

void 
write_prefs (void)
{
    gchar cfgdir[PATH_MAX], filename[PATH_MAX], *str = NULL;
    FILE *fp = NULL;

    if((str = getenv("HOME")))
    {
	snprintf(cfgdir, PATH_MAX, "%s/.gtkpod/", str);
	if(!g_file_test(cfgdir, G_FILE_TEST_IS_DIR))
	{
	    if(!mkdir(cfgdir, 0755))
	    {
		fprintf(stderr, "Unable to mkdir %s\n, Settings not saved\n",
			cfgdir);
	    }
	}
	snprintf(filename, PATH_MAX, "%s/prefs", cfgdir);
	if((fp = fopen(filename, "w")))
	{
	    write_prefs_to_file_desc(fp);
	    fclose(fp);
	}
	else
	{
	    fprintf(stderr, "Unable to open %s for writing\n", filename);
	}
    }
}


/* Free all memory including the cfg-struct itself. */
void discard_prefs ()
{
    cfg_free(cfg);
    cfg = NULL;
}

void cfg_free(struct cfg *c)
{
    if(c)
    {
      g_free (c->ipod_mount);
      g_free (c->last_dir.dir_browse);
      g_free (c->last_dir.file_browse);
      g_free (c);
    }
}

static gchar *
get_dirname_of_filename(gchar *file)
{
   char *tok = NULL, *buf = NULL, filename[PATH_MAX];
   gchar *result, dump[PATH_MAX];

   snprintf(filename, PATH_MAX, "%s", file);
   buf = (gchar *) malloc((sizeof(gchar) * PATH_MAX) + 1);

   memset(buf, 0, sizeof(buf));
   memset(dump, 0, PATH_MAX);

   if ((tok = strtok(filename, "/")))
   {
      do
      {
         buf = strncat(buf, dump, PATH_MAX);
         snprintf(dump, PATH_MAX, "/%s", tok);
      }
      while ((tok = strtok(NULL, "/")));
   }
   result = g_strdup_printf("%s/",buf);
   g_free(buf);
   return (result);
}

void prefs_set_last_dir_file_browse_for_filename(gchar *file)
{
    if(cfg->last_dir.file_browse) g_free(cfg->last_dir.file_browse);
    cfg->last_dir.file_browse = get_dirname_of_filename(file);
}

void prefs_set_last_dir_dir_browse_for_filename(gchar *file)
{
    if(cfg->last_dir.dir_browse) g_free(cfg->last_dir.dir_browse);
    cfg->last_dir.dir_browse = get_dirname_of_filename(file);
}


void prefs_set_mount_point(const gchar *mp)
{
    if(cfg->ipod_mount) g_free(cfg->ipod_mount);
    cfg->ipod_mount = g_strdup(mp);
}

void prefs_set_md5songs_active(gboolean active)
{
    cfg->md5songs = active;
}

void prefs_set_writeid3_active(gboolean active)
{
    cfg->writeid3 = active;
}

/* song list opts */
void 
prefs_set_song_list_show_all(gboolean val)
{

    if(val)
    {
	cfg->song_list_show.artist = TRUE;
	cfg->song_list_show.album = TRUE;
	cfg->song_list_show.track = TRUE;
	cfg->song_list_show.genre = TRUE;
    }
    else
    {
	cfg->song_list_show.artist = FALSE;
	cfg->song_list_show.album = FALSE;
	cfg->song_list_show.track = FALSE;
	cfg->song_list_show.genre = FALSE;
    }
}

void 
prefs_set_song_list_show_artist(gboolean val)
{
    cfg->song_list_show.artist = val;
}

void 
prefs_set_song_list_show_album(gboolean val)
{
    cfg->song_list_show.album = val;
}
void 
prefs_set_song_list_show_track(gboolean val)
{
    cfg->song_list_show.track = val;
}
void 
prefs_set_song_list_show_genre(gboolean val)
{
    cfg->song_list_show.genre = val;
}
gboolean 
prefs_get_song_list_show_all(void)
{
    if((cfg->song_list_show.artist == FALSE) &&
	(cfg->song_list_show.album == FALSE) &&
	(cfg->song_list_show.track == FALSE) &&
	(cfg->song_list_show.genre == FALSE))
	return(TRUE);
    else
	return(FALSE);
}
gboolean 
prefs_get_song_list_show_artist(void)
{
    return(cfg->song_list_show.artist);
}
gboolean 
prefs_get_song_list_show_album(void)
{
    return(cfg->song_list_show.album);
}
gboolean 
prefs_get_song_list_show_track(void)
{
    return(cfg->song_list_show.track); 
}
gboolean 
prefs_get_song_list_show_genre(void)
{
    return(cfg->song_list_show.genre); 
}
void
prefs_print(void)
{
    FILE *fp = stderr;
    gchar *on = "On";
    gchar *off = "Off";
    
    fprintf(fp, "GtkPod Preferences\n");
    fprintf(fp, "Mount Point:\t%s\n", cfg->ipod_mount);
    fprintf(fp, "Interactive ID3:\t");
    if(cfg->writeid3)
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    
    fprintf(fp, "MD5 Songs:\t");
    if(cfg->md5songs)
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    
    fprintf(fp, "Song List Options:\n");
    fprintf(fp, "  Show All Attributes: ");
    if(prefs_get_song_list_show_all())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Album: ");
    if(prefs_get_song_list_show_album())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Track: ");
    if(prefs_get_song_list_show_track())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Genre: ");
    if(prefs_get_song_list_show_genre())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
    fprintf(fp, "  Show Artist: ");
    if(prefs_get_song_list_show_artist())
	fprintf(fp, "%s\n", on);
    else
	fprintf(fp, "%s\n", off);
}

void 
prefs_set_playlist_deletion(gboolean val)
{
    cfg->deletion.playlist = val;
}

gboolean 
prefs_get_playlist_deletion(void)
{
    return(cfg->deletion.playlist);
}

void 
prefs_set_song_playlist_deletion(gboolean val)
{
    cfg->deletion.song = val;
}

gboolean 
prefs_get_song_playlist_deletion(void)
{
    return(cfg->deletion.song);
}

void 
prefs_set_song_ipod_file_deletion(gboolean val)
{
    cfg->deletion.ipod_file = val;
}

gboolean 
prefs_get_song_ipod_file_deletion(void)
{
    return(cfg->deletion.ipod_file);
}
