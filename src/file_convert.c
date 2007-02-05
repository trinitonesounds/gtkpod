#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif
/*
 * TODO:
 *  - see threaded version (conversion launched when adding the file)
 *
 *  ++dump child_stdout if file name not found or file not found 
 *
 *  - redirect stderr display into "debug info" ?? (as in k3b) ??
 *
 *  - what happend if "conversion" change ? i.e. if ogg was converted into mp3 and is now with flac
 *
 *  - how abort this ? we should kill the child_pid ?
 *  - fix non-converted files, i.e. remove them from DB ? or it is currently ok
 *  - fix the g_return_if_fail( <cond>, NULL) into conv_error(xxx) ?
 *
 *  - check memory leak(s): check allocated/free'd stuff (especially on errors)
 *
 */

/*
 * HOWTO add new conversions:
 *  - new xxxfile.[ch] (modify also Makefile.am and configure.in if lib needed)
 *  - new path_conv_xxx in 
 *       gtkpod.glade
 *       prefs_window.c: path_button_names, path_key_names, path_entry_names
 *  - modify file_convert.c::file_convert_pre_copy() to add the new type in switch(converter->type)
 */

#include <string.h>
#include "file_convert.h"
#include "display_itdb.h"
#include <glib.h>
#include <sys/wait.h>       /* for WIF... macros */
#include <glib.h>
#include <glib/gstdio.h>    /* g_unlink (which is currently #define unlink) */
#include <unistd.h>         /* unlink not included in glib/gstdio (strange) */
#include "prefs.h"
#include "misc.h"           /* T_item */
#include "misc_track.h"     /* track_get_item_pointer */

#define DEBUG_CONV
#ifdef DEBUG_CONV
#   define _TO_STR(x) #x
#   define TO_STR(x) _TO_STR(x)
#   define debug(...) do { fprintf(stderr,  __FILE__ ":" TO_STR(__LINE__) ":" __VA_ARGS__); } while(0)
#else
#   define debug(...)
#endif

#define conv_error(...) g_error_new(G_FILE_ERROR, 0, __VA_ARGS__)


/* Internal functions */
static GError *execute_conv (gchar **argv, TrackConv *converter);
static gchar *get_filename_from_stdout(TrackConv *converter);
static gchar *checking_converted_file (TrackConv *converter);
#if 0
static gchar **cmdline_to_argv(const gchar *cmdline, Track *track); /* TODO: could be extern for other needs */
#endif


/* Functions */

/*
 * file_convert_pre_copy()
 * converts a track by launching the external command given in the prefs 
 *
 */
extern GError *file_convert_pre_copy (TrackConv *converter)
{   
    Track *track = NULL;
    ExtraTrackData *etr = NULL;
    GError *error = NULL;                 /* error if any */
    const gchar *type = NULL;             /* xxx (ogg) */
    gchar *convert_command = NULL;        /* from prefs */
    gchar **argv;                         /* for conversion external process */
    gint i;

    /* sanity checks */
    g_return_val_if_fail (converter, NULL);
    g_return_val_if_fail (converter->track, NULL);
    track = converter->track;
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    /* Find the correct script for conversion */
    switch (converter->type) {
        case FILE_TYPE_UNKNOWN:
        case FILE_TYPE_M4P:
        case FILE_TYPE_M4B:
        case FILE_TYPE_M4V:
        case FILE_TYPE_MP4:
        case FILE_TYPE_MOV:
        case FILE_TYPE_MPG:
        case FILE_TYPE_M3U:
        case FILE_TYPE_PLS:
        case FILE_TYPE_IMAGE:
        case FILE_TYPE_DIRECTORY:
            return NULL; /*FIXME: error? */
            break;
        case FILE_TYPE_M4A:
            convert_command = prefs_get_string ("path_conv_m4a");
            break;
        case FILE_TYPE_WAV:
            convert_command = prefs_get_string ("path_conv_wav");
            break;
        case FILE_TYPE_MP3:
            convert_command = prefs_get_string ("path_conv_mp3");
            break;
        case FILE_TYPE_OGG:
	    convert_command = prefs_get_string ("path_conv_ogg");
            break;
        case FILE_TYPE_FLAC:
	    convert_command = prefs_get_string ("path_conv_flac");
            break;
    }
 
    debug("convert_command:%s\n",convert_command);
    /* Check that everything is ok before continuing */
    if (!convert_command ||
        strlen(convert_command)==0) {
        return conv_error(_("No command found to convert `%s'.\n"
                            "Please fill the On-the-fly conversion settings for %s in the Tools preferences"),
                          etr->pc_path_locale,
                          type);
    }
    /* FIXME: check that %s is present (at least) */
   
    /* Build the command args */
    argv = g_malloc0(sizeof(gchar*) * 17);

    argv[0] = g_strdup (convert_command);
    i = 1;
    if (track->artist) {
        argv[i] = g_strdup ("-a");
        ++i;
        argv[i] = g_strdup (track->artist);
        ++i;
    }
    if (track->album) {
        argv[i] = g_strdup ("-A");
        ++i;
        argv[i] = g_strdup (track->album);
        ++i;
    }
    if (track->track_nr) {
        argv[i] = g_strdup ("-T");
        ++i;
        argv[i] = g_strdup_printf("%d", track->track_nr);
        ++i;
    }
    if (track->title) {
        argv[i] = g_strdup ("-t");
        ++i;
        argv[i] = g_strdup (track->title);
        ++i;
    }
    if (track->genre) {
        argv[i] = g_strdup ("-g");
        ++i;
        argv[i] = g_strdup (track->genre);
        ++i;
    }
    if (track->year) {
        argv[i] = g_strdup ("-y");
        ++i;
        argv[i] = g_strdup_printf("%d", track->year);
        ++i;
    }
    if (track->comment) {
        argv[i] = g_strdup ("-c");
        ++i;
        argv[i] = g_strdup (track->comment);
        ++i;
    }
    argv[i] = g_strdup (etr->pc_path_locale);

    argv = g_realloc(argv, (i + 1) * sizeof(gchar*));

    /* Convert the file */
    error = execute_conv (argv, converter); /* frees all cmdline strings */

    return error;
}

/*
 * file_convert_wait_for_conversion()
 *
 * Waiting for converter process to end, then analyse its stdout in order to find the filename created.
 * FIXME: check converted file extension sounds good. 
 *
 */
GError *file_convert_wait_for_conversion (TrackConv *converter)
{
    ExtraTrackData *etr=NULL;
    gint exit_status;
    GPid wait;
    gchar *error_msg=NULL;
    GError *error=NULL;
    gboolean aborted = FALSE;

    /* sanity checks */ 
    g_return_val_if_fail (converter, NULL);
    g_return_val_if_fail (converter->track, NULL);
    etr = converter->track->userdata;
    g_return_val_if_fail (etr, NULL);

    debug("Waiting for child (PID=%d)\n",converter->child_pid);

    /* Waiting for PID and checking Return code */
    wait=waitpid(converter->child_pid,&exit_status,0);
    if (wait == -1)  {
        error_msg=g_strdup ("Weird waitpid sent -1 ! Please report.\n");
    } else {
        if (WIFEXITED(exit_status)){
            debug("seems to be %s: %d...\n",WEXITSTATUS(exit_status)==0?"ok":"ko", WEXITSTATUS(exit_status));
            switch (WEXITSTATUS(exit_status)) {
                case 0: /* OK */ break;
                case 1: error_msg = g_strdup_printf ("Input file not found"); break;
                case 2: error_msg = g_strdup_printf ("Output file could not be created"); break;
                case 3: error_msg = g_strdup_printf ("Cannot get info for file"); break;
                case 4: error_msg = g_strdup_printf ("Cannot exec decoding"); break;
                case 5: error_msg = g_strdup_printf ("Cannot exec encoding"); break;
                case 6: error_msg = g_strdup_printf ("Conversion failed"); break;
                default:
                    error_msg = g_strdup_printf ("Unknow script exit status:%d ",WEXITSTATUS(exit_status));
            }
        }else if (WIFSIGNALED(exit_status)) {
            debug(":0x%X -> %d\n",exit_status,WTERMSIG(exit_status));
            aborted = TRUE;
/*
            error_msg = g_strdup_printf ("Execution failed. Received signal %d (%s)",
                                         WTERMSIG(exit_status),(error?error->message:""));
*/
        }else if (WIFSTOPPED(exit_status)) {
            debug(":0x%X -> %d\n",exit_status,WSTOPSIG(exit_status));
            error_msg = g_strdup_printf ("Execution stopped. Received signal %d (%s)",
                                         WSTOPSIG(exit_status),(error?error->message:""));
        }else if (WIFCONTINUED(exit_status)) {
            /* FIXME: strange to be here no?? */
            debug(":0x%X -> SIGCONT\n",exit_status);
            error_msg = g_strdup_printf ("Execution continued. Received signal SIGCONT (%s)",
                                         (error?error->message:""));
        } else {
            error_msg = g_strdup_printf ("Execution failed (%s)",
                                         (error?error->message:""));
        }
    }
    debug("Error: %s\n",(error_msg?error_msg:"N/A"));

    if(aborted) error = conv_error(_("Aborted.\n"));
    else {
        /* Getting filename from child's STDOUT */
        if (!error_msg)
            error_msg = get_filename_from_stdout (converter);

        /* Checking file exists */
        if (!error_msg)
            error_msg = checking_converted_file (converter);

        /* Checking errors */
        if (error_msg){
            error=conv_error(_("Cannot convert '%s'.\nExecuting `%s'\nGave error: %s\n\n"),
                             etr->pc_path_locale,
                             converter->command_line,
                             error_msg);
            g_free (error_msg);
        }
    }

    /* Freeing data */
    if (converter->child_pid) {
        details_log_clear ();

        if (!g_source_remove (converter->source_id)) {
             error = conv_error(_("Failed to remove watch\n\n"));
        }

        g_spawn_close_pid(converter->child_pid);
        converter->child_pid=0;
    }
    g_free (converter->command_line);
    converter->command_line=NULL;

    /* It's finally over */
    return error;
}


/*
 * file_convert_post_copy()
 * 
 * Remove files created by file_convert_pre_copy() if necessary (TODO).
 *
 */
GError * file_convert_post_copy (TrackConv *converter)
{
    GError *error=NULL;
    ExtraTrackData *etr;
    Track *track;

    /* sanity checks */ 
    g_return_val_if_fail (converter,NULL);
    track = converter->track;
    g_return_val_if_fail (track, NULL);
    etr = track->userdata;
    g_return_val_if_fail (etr, NULL);

    debug("file_convert_post_copy(%s)\n",converter->converted_file);
    /*TODO: check prefs to not remove file */
    if (converter->converted_file)
	g_unlink (converter->converted_file);
    g_free (converter->converted_file);
    converter->converted_file = NULL;
    /*FIXME: restore track->size ????? */
    return error; 
}






/* 
 *
 *
 * Implementation of Internal functions 
 *
 *
 */

/*
 * Called by watch on stderr to update details log
 */
static gboolean execute_conv_cb(GIOChannel *source, GIOCondition condition,  TrackConv *converter)
{
    GIOStatus status;
    gchar buf[255];
    gsize bytes_read;

    if (converter->aborted) return FALSE;

    while (1) {
      status = g_io_channel_read_chars (source, buf, sizeof(buf) - 1, &bytes_read, NULL);
      buf[bytes_read] = '\0';
      switch (status) {

        case G_IO_STATUS_ERROR :
          return FALSE;

        case G_IO_STATUS_EOF :
          details_log_append(buf);
          return FALSE;

        case G_IO_STATUS_NORMAL :
          details_log_append(buf);
          break;

        case G_IO_STATUS_AGAIN :
          return TRUE;
      }
    }

    return FALSE; 
}


/*
 * Call the external command given in @tokens for the given @track
 */
static GError *execute_conv (gchar **argv, TrackConv *converter)
{
    ExtraTrackData *etr = NULL;
    GError *error = NULL;
    gchar *command_line = NULL;
    GError *spawn_error = NULL;
    guint i = 0;
    GPid child_pid;
    gint child_stdout;
    gint child_stderr;
    GIOChannel *gio_channel;

    /* sanity checks */ 
    g_return_val_if_fail (converter, NULL);
    g_return_val_if_fail (converter->track, NULL);
    etr = converter->track->userdata;
    g_return_val_if_fail (etr, NULL);

    g_return_val_if_fail (argv, NULL);

    /* Building command_line string (for output only) */
    for (i = 0; argv[i] != NULL; ++i) 
    {

        gchar *p = command_line;
        if (!p) 
            command_line = g_strdup(argv[i]);                      /* program name (1st token) */
        else {
            command_line = g_strdup_printf("%s \"%s\"",p,argv[i]); /* arg (other tokens) */
            g_free(p);
        }
    }

    debug("Executing `%s'\n",command_line);

    /* Launching conversion */
    if (g_spawn_async_with_pipes(NULL,                      /* working dir (inherit parent) FIXME: use 'track' dir  */
                                 argv,                      /* argv  FIXME: check if windows issue with argv[0] */
                                 NULL,                      /* envp */
                                 G_SPAWN_SEARCH_PATH |      /* flags: use user's $PATH */
                                 G_SPAWN_DO_NOT_REAP_CHILD, /*        keep child alive (for waidpid, g_spwan_close_pid)  */
                                 NULL,                      /* child_setup function */
                                 NULL,                      /* user_data */
                                 &child_pid,                /* child's PID */
                                 NULL,                      /* child's stdin */
                                 &child_stdout,             /* child's stdout */
                                 &child_stderr,             /* child's stderr FIXME: get this for "debug"    (result?) */
                                 &spawn_error)==FALSE)
    {     /* error */
        debug("error: error:%p (message:%s)",error, (error?error->message:"NoErrorMessage"));
        error = conv_error(_("Cannot convert '%s'.\nExecuting `%s'\nGave error: %s\n\n"),
			   etr->pc_path_locale,
			   command_line,
			   (spawn_error&&spawn_error->message)?spawn_error->message:"no explanation. Please report.");
        if (spawn_error)
	{
            g_free (spawn_error);   /* FIXME: memory leak i guess (at
				     * least for message if any). */
	}
        g_free (command_line);
    }
    else
    {
        converter->child_pid = child_pid;
        converter->child_stdout = child_stdout;
        converter->command_line = command_line;
        gio_channel = g_io_channel_unix_new (child_stderr);
        g_io_channel_set_flags (gio_channel, G_IO_FLAG_NONBLOCK, NULL);
        converter->source_id = g_io_add_watch (gio_channel, G_IO_IN, (GIOFunc)execute_conv_cb, converter);
        g_io_channel_set_close_on_unref (gio_channel, TRUE);
        g_io_channel_unref (gio_channel);
        converter->aborted = FALSE;
    }
    /* Freeing data */
    for(i=0; argv[i]!=NULL; ++i){
        g_free (argv[i]);
    }
    g_free (argv);

    return error;
}

/*
 * get_filename_from_stdout()
 */
static gchar *get_filename_from_stdout(TrackConv *converter)
{
    gchar *error=0;
    if (converter->child_stdout<=0) {
        error = g_strdup_printf ("Strange child_stdout: %d\n",converter->child_stdout);
    } else {
        /* find a filename (max size: PATH_MAX) at the end of the STDOUT */
        lseek(converter->child_stdout, PATH_MAX, SEEK_END); /* don't care of error: we assume it's ok, i.e. we're near the end of the stdout */
        gchar buffer[PATH_MAX+1];
        memset(buffer,0x0,sizeof(buffer));
        if (read(converter->child_stdout, buffer,  PATH_MAX) <= 0) {
            error = g_strdup_printf ("Cannot read in child_stdout (%d)\n",converter->child_stdout);
        } else {
            /* find the filename on a line, i.e. after a \n */
            char *end=buffer+strlen(buffer)-1;
            char *begin=rindex(buffer, '\n');       
            if (begin==end) {                /* if STDOUT ends with \n, */
                *end='\0';                   /*    eat this one  */
                begin=rindex(buffer, '\n');  /*    and re-search for a \n */
            }
            if (!begin) 
                begin=buffer;
            else 
                begin++;
            debug("File found: `%s'\n",begin);
            converter->converted_file=g_strdup (begin);
        }
        close (converter->child_stdout);
        converter->child_stdout=-1;
    }
    return error;
}


/*
 *  checking_converted_file()
 */
static gchar *checking_converted_file (TrackConv *converter)
{
    Track *track;
    gchar *error=NULL;
    struct stat stat_file;

    g_return_val_if_fail (converter, NULL);
    track = converter->track;
    g_return_val_if_fail (track, NULL);
      
    /* Fix some stuff for the iPod */
    if (g_file_test(converter->converted_file,G_FILE_TEST_EXISTS) &&
        stat(converter->converted_file,&stat_file)==0){
        converter->old_size=track->size;            /* save real original size */
        track->size=stat_file.st_size;              /* faking size for iPod */
    } else {
        error = g_strdup_printf (_("Converted file `%s' is not found !\n"),
                                    converter->converted_file);
    }
    return error;
}


#if 0
/*
 * cmdline_to_argv()
 *
 * Parses a string and split it into a decent argv (that should be freed) 
 *        while replacing %<x> with their value from the track
 */
static gchar **cmdline_to_argv(const gchar *cmdline, Track *track)
{
    gint  size=1;
    gchar **argv=g_malloc(size*sizeof(gchar *));
    gchar *copy=g_strdup (cmdline);
    gchar *ptr=copy;
    gint  argc=0;
    gchar ptr_plus_index_less_1='\0';
    guint index=0;
    gint  finished=0;
    debug("cmdline_to_argv(\"%s\",%p)\n",cmdline, track);
    while (!finished) {
        /* Percent sign */
        if (*(ptr+index)=='%' && *(ptr+index+1)!='\0'){  
            T_item item = char_to_T (*(ptr+index+1));
            if (item != -1) {
                gchar *tmp;
                const gchar *value=track_get_item (track, item);
                gchar *basename=NULL;
                ptr[index]='\0';
                if (value && ptr_plus_index_less_1 != '\0' && *(ptr+index+1) == 's') {
                    /* relative path to file: keep only basename */
                    basename=g_path_get_basename (value);
                    value=basename;
                }
                tmp=g_strdup_printf ("%s%s%s", ptr, value?value:"", ptr+index+2);
                g_free (ptr);
                ptr=tmp;
                if (value && strlen (value)) {
                    index+=strlen(value);
                    ptr_plus_index_less_1=*(ptr+index-1);
                }
                if (basename)
                    g_free (basename);
            } else {
                /* no item found. Skipping */    /* FIXME: %% should be '%' no ? */
                debug("No item\n");
                ptr_plus_index_less_1=*(ptr+index);
                ++index;
            }

        /* Blank sign */
        } else if (*(ptr+index)==' ' || *(ptr+index) == '\0') {
            gchar *tmp;
            finished = *(ptr+index)=='\0';
            /* Blank: new token to be put in argv */
            if ((argc+1) >= size) { 
                size*=2;
                argv=g_realloc(argv, size*sizeof(gchar *));
                if (!argv)
                    return NULL;/* FIXME: memory Leak ! */
            }
            ptr[index]='\0';
            while (*(ptr+index+1)==' ')
                ++index;
            argv[argc++]=g_strdup (ptr);
            tmp=g_strdup (ptr+index+1);
            g_free (ptr);
            ptr_plus_index_less_1='\0';
            ptr=tmp;
            index=0;

        /* Normal char */
        } else {                            
            ptr_plus_index_less_1=*(ptr+index);
            ++index;
        }
    }
    g_free (ptr);

    if (argc) { /*FIXME: if argc == 1 we have no parameters  */
        argv[argc]=NULL;
    } else {
        g_free (argv);
        argv=NULL;
    }
    /*if (argc+1 > size) */
    /*    argv=g_realloc (argv, (argc+1) * sizeof(gchar *)); // not really usefull ... */
    debug("cmdline_to_argv() -> %p / %d\n",argv,argc);
    return argv;
}
#endif
