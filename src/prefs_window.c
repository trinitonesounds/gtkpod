#include"prefs.h"
#include"prefs_window.h"
#include<stdio.h>
#include "md5.h"
#include "song.h"

static GtkWidget *prefs_window = NULL;
static struct cfg *tmpcfg = NULL;

/**
 * create_gtk_prefs_window
 * Create, Initialize, and Show the preferences window
 * allocate a static cfg struct for temporary variables
 */
void
prefs_window_create(void)
{
    if(!prefs_window)
    {
	GtkWidget *w = NULL;
	
	if(!tmpcfg)
	{
	    tmpcfg = g_malloc0 (sizeof (struct cfg));
	    memset(tmpcfg, 0, sizeof(struct cfg));
	    tmpcfg->md5songs = cfg->md5songs;
	    tmpcfg->writeid3 = cfg->writeid3;
	    tmpcfg->ipod_mount = g_strdup(cfg->ipod_mount);
	}
	else
	{
	    fprintf(stderr, "tmpcfg is not NULL wtf !!\n");
	}
	
	prefs_window = create_prefs_window();
	if((w = lookup_widget(prefs_window, "cfg_mount_point")))
	{
	    gtk_entry_set_text(GTK_ENTRY(w), g_strdup(tmpcfg->ipod_mount));
	}
	if((w = lookup_widget(prefs_window, "cfg_md5songs")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), tmpcfg->md5songs);
	}
	if((w = lookup_widget(prefs_window, "cfg_writeid3")))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), tmpcfg->writeid3);
	}
	gtk_widget_show(prefs_window);
    }
}
/**
 * cancel_gtk_prefs_window
 * UI has requested preference changes be ignored
 * Frees the tmpcfg variable
 */
void
prefs_window_cancel(void)
{
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    cfg_free(tmpcfg);
    tmpcfg =NULL;
    prefs_window = NULL;
}

/**
 * save_gtkpod_prefs_window
 * UI has requested preferences update(by clicking ok on the prefs window)
 * Frees the tmpcfg variable
 */
void 
prefs_window_save(void)
{
    prefs_set_md5songs_active(tmpcfg->md5songs);
    prefs_set_writeid3_active(tmpcfg->writeid3);
    prefs_set_mount_point(tmpcfg->ipod_mount);
    cfg_free(tmpcfg);
    tmpcfg =NULL;
    if(prefs_window)
	gtk_widget_destroy(prefs_window);
    prefs_window = NULL;
    if(cfg->md5songs)
	unique_file_repository_init(get_song_list());

}

/**
 * prefs_window_set_md5songs_active
 * @val - truth value of whether or not we should use the md5 hash to
 * prevent file duplication. changes temp variable 
 */
void
prefs_window_set_md5songs_active(gboolean val)
{
    tmpcfg->md5songs = val;
}

/**
 * prefs_window_set_writeid3_active
 * @val - truth value of whether or not we should allow id3 tags to be
 * interactively changed, changes temp variable
 */
void
prefs_window_set_writeid3_active(gboolean val)
{
    tmpcfg->writeid3 = val;
}

/**
 * prefs_window_set_mount_point
 * @mp - set the temporary config variable to the mount point specified
 */
void
prefs_window_set_mount_point(const gchar *mp)
{
    if(tmpcfg->ipod_mount) g_free(tmpcfg->ipod_mount);
    tmpcfg->ipod_mount = g_strdup_printf("%s",mp);
}
