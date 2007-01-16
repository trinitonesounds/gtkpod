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
static GError *execute_conv(gchar **argv,Track *track);
static gchar *get_filename_from_stdout(TrackConv *converter);
static gchar *checking_converted_file (TrackConv *converter, Track *track);
#if 0
static gchar **cmdline_to_argv(const gchar *cmdline, Track *track); /* TODO: could be extern for other needs */
#endif


/* Functions */

/*
 * file_convert_pre_copy()
 * converts a track by launching the external command given in the prefs 
 *
 */
extern GError *file_convert_pre_copy(Track *track)
{   
    ExtraTrackData *etr=NULL;
    TrackConv *converter=NULL;          /* the converter struct */
    GError *error=NULL;                 /* error if any */
    const gchar *type=NULL;             /* xxx (ogg) */
    gchar *conv_str=NULL;               /* path_conv_xxx or ext_conv_xxx */
    gchar *convert_command=NULL;        /* from prefs */
    gchar **argv;                       /* for conversion external process */

    /* sanity checks */ 
    g_return_val_if_fail (track,NULL);
    etr=track->userdata;
    g_return_val_if_fail (etr,NULL);
    converter=etr->conv;
    g_return_val_if_fail (converter,NULL);


    /* Find the correct script for conversion */
    switch (converter->type) {
        case FILE_TYPE_UNKNOWN:
        case FILE_TYPE_MP3:
        case FILE_TYPE_M4A:
        case FILE_TYPE_M4P:
        case FILE_TYPE_M4B:
        case FILE_TYPE_WAV:
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
        case FILE_TYPE_OGG:
            type="ogg";/*FIXME: get it directly for type. */
            break;
        case FILE_TYPE_FLAC:
            type="flac";/*FIXME: get it directly for type. */
            break;
    }
    conv_str=g_strdup_printf("path_conv_%s",type);
    convert_command=prefs_get_string (conv_str);
    g_free(conv_str);
 
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
    argv = g_malloc0(sizeof(gchar*) * 3);
    argv[0] = convert_command;
    argv[1] = etr->pc_path_locale;

    /* Convert the file */

    error=execute_conv (argv, track); /* frees all cmdline strings */

    return error;
}

/*
 * file_convert_wait_for_conversion()
 *
 * Waiting for converter process to end, then analyse its stdout in order to find the filename created.
 * FIXME: check converted file extension sounds good. 
 *
 */
extern GError *file_convert_wait_for_conversion(Track *track)
{
    ExtraTrackData *etr=NULL;
    TrackConv *converter=NULL;
    gint exit_status;
    GPid wait;
    gchar *error_msg=NULL;
    GError *error=NULL;

    /* sanity checks */ 
    g_return_val_if_fail (track,NULL);
    etr=track->userdata;
    g_return_val_if_fail (etr,NULL);
    converter=etr->conv;
    g_return_val_if_fail (converter,NULL);
    g_return_val_if_fail (converter->child_pid>0, NULL);

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
            error_msg = g_strdup_printf ("Execution failed. Received signal %d (%s)",
                                         WTERMSIG(exit_status),(error?error->message:""));
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

    /* Getting filename from child's STDOUT */
    if (!error_msg)
        error_msg = get_filename_from_stdout (converter);

    /* Checking file exists */
    if (!error_msg)
        error_msg = checking_converted_file (converter, track);

    /* Checking errors */
    if (error_msg){
        error=conv_error(_("Cannot convert '%s'.\nExecuting `%s'\nGave error: %s\n\n"),
                         etr->pc_path_locale,
                         converter->command_line,
                         error_msg);
        g_free (error_msg);
    }
    /* Freeing data */
    if (converter->child_pid) {
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
extern GError * file_convert_post_copy(Track *track)
{
    GError *error=NULL;
    ExtraTrackData *etr=NULL;
    TrackConv *converter=NULL;

    /* sanity checks */ 
    g_return_val_if_fail (track,NULL);
    etr=track->userdata;
    g_return_val_if_fail (etr,NULL);
    converter=etr->conv;
    g_return_val_if_fail (converter,NULL);
    g_return_val_if_fail (converter->converted_file, NULL);

    debug("file_convert_post_copy(%s)\n",converter->converted_file);
    /*TODO: check prefs to not remove file */
    g_unlink (converter->converted_file);
    g_free (converter->converted_file);
    converter->converted_file=NULL;
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
 * Call the external command given in @tokens for the given @track
 */
static GError *execute_conv(gchar **argv,Track *track)
{
    ExtraTrackData *etr=NULL;
    TrackConv *converter=NULL;
    GError *error=NULL;
    gchar *command_line=NULL;
    GError *spawn_error=NULL;
    guint i=0;
    GPid child_pid;
    gint child_stdout;

    /* sanity checks */ 
    g_return_val_if_fail (track,NULL);
    etr=track->userdata;
    g_return_val_if_fail (etr,NULL);
    converter=etr->conv;
    g_return_val_if_fail (converter,NULL);

    g_return_val_if_fail (argv!=NULL, NULL);

    /* Building command_line string (for output only) */
    for(i=0; argv[i]!=NULL; ++i) 
    {
        gchar *p=command_line;
        if (!p) 
            command_line=g_strdup(argv[i]);                      /* program name (1st token) */
        else {
            command_line=g_strdup_printf("%s \"%s\"",p,argv[i]); /* arg (other tokens) */
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
                                 NULL,                      /* child's stderr FIXME: get this for "debug"    (result?) */
                                 &spawn_error)==FALSE){     /* error */
        debug("error: error:%p (message:%s)",error, (error?error->message:"NoErrorMessage"));
        error = conv_error(_("Cannot convert '%s'.\nExecuting `%s'\nGave error: %s\n\n"),
                             etr->pc_path_locale,
                             command_line,
                             (spawn_error&&spawn_error->message)?spawn_error->message:"no explanation. Please report.");
        if (spawn_error)
            g_free (spawn_error);   /* FIXME: memory leak i guess (at least for message if any). */
        g_free (command_line);
    } else {
        converter->child_pid=child_pid;
        converter->child_stdout=child_stdout;
        converter->command_line=command_line;
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
static gchar *checking_converted_file (TrackConv *converter, Track *track)
{
    gchar *error=NULL;
    struct stat stat_file;             
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
