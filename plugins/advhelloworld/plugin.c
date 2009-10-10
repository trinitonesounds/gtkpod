/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) phantomjinx 2009 <phantomjinx@goshawk.BIRDS-OF-PREY>
 *
 * plugin.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * plugin.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "plugin.h"

/* Project configuration file */
#include <config.h>

/* Document manager interface */
#include <libanjuta/interfaces/ianjuta-document-manager.h>

/* Parent class. Part of standard class definition */
static gpointer parent_class;

static void
on_hello_action_activate (GtkAction *action, AdvHelloWorldPlugin *plugin) {
    IAnjutaDocument *doc;
    IAnjutaDocumentManager *docman;
    GtkWindow *parent_win;
    gchar *filename;

    /* We are using Anjuta widow as parent for displaying the dialog */
    parent_win = GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell);

    /* Query for object implementing IAnjutaDocumentManager interface */
    docman = anjuta_shell_get_interface (ANJUTA_PLUGIN (plugin)->shell,
            IAnjutaDocumentManager, NULL);

    /* Get current document */
    doc = ianjuta_document_manager_get_current_document(docman, NULL);
    filename = ianjuta_document_get_filename(doc, NULL);

    /* Display the filename */
    anjuta_util_dialog_info(parent_win, "Current filename is: %s", filename);
    g_free(filename);
}

static GtkActionEntry actions[] = {
    {
        "ActionFileHelloWorld",                   /* Action name */
        GTK_STOCK_NEW,                            /* Stock icon, if any */
        N_("_Hello world action"),                /* Display label */
        NULL,                                     /* short-cut */
        N_("This is hello world action"),         /* Tooltip */
        G_CALLBACK (on_hello_action_activate)    /* action callback */
    }
};

#define UI_FILE GTKPOD_UI_DIR"/advhelloworld.ui"

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
    GtkWidget *wid;
    AnjutaUI *ui;
    AdvHelloWorldPlugin *hello_plugin;
    GtkActionGroup* action_group;

    hello_plugin = (AdvHelloWorldPlugin*) plugin;
    ui = anjuta_shell_get_ui (plugin->shell, NULL);

    /* Create hello plugin widget */
    wid = gtk_label_new ("Hello World Plugin!!");
    hello_plugin->widget = wid;

    /* Add our actions */
    action_group = anjuta_ui_add_action_group_entries (ui,
                                                        "ActionGroupHello",
                                                        _("Hello world"),
                                                        actions,
                                                        G_N_ELEMENTS (actions),
                                                        GETTEXT_PACKAGE, TRUE,
                                                        plugin);
    hello_plugin->action_group = action_group;

    /* Merge UI */
    hello_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);

    /* Add widget in Shell. Any number of widgets can be added */
    anjuta_shell_add_widget (plugin->shell, wid,
                             "AdvHelloWorldPlugin",
                             _("AdvHelloWorldPlugin"),
                             GTK_STOCK_ABOUT,
                             ANJUTA_SHELL_PLACEMENT_CENTER,
                             NULL);

    return TRUE; /* FALSE if activation failed */
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
    AnjutaUI *ui;
    AdvHelloWorldPlugin *hello_plugin;

    hello_plugin = (AdvHelloWorldPlugin*) plugin;
    ui = anjuta_shell_get_ui (plugin->shell, NULL);

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget (plugin->shell,
                                hello_plugin->widget,
                                NULL);
    /* Unmerge UI */
    anjuta_ui_unmerge (ui, hello_plugin->uiid);

    /* Remove Action groups */
    anjuta_ui_remove_action_group (ui, hello_plugin->action_group);

    /* FALSE if plugin doesn't want to deactivate */
    return TRUE;
}

static void
adv_hello_world_plugin_instance_init (GObject *obj)
{
    AdvHelloWorldPlugin *plugin = (AdvHelloWorldPlugin*) obj;
    plugin->uiid = 0;
    plugin->widget = NULL;
    plugin->action_group = NULL;
}

static void
adv_hello_world_plugin_class_init (GObjectClass *klass)
{
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

/* This line will change when we implement interfaces */
ANJUTA_PLUGIN_BOILERPLATE (AdvHelloWorldPlugin, adv_hello_world_plugin);

/* This sets up codes to register our plugin */
ANJUTA_SIMPLE_PLUGIN (AdvHelloWorldPlugin, adv_hello_world_plugin);

;
