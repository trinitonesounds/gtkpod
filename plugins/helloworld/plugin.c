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

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
    HelloWorldPlugin *hello_plugin = (HelloWorldPlugin*) plugin;

    /* Create hello plugin widget */
    hello_plugin->widget = gtk_label_new ("Hello World Plugin!!");
    gtk_widget_show_all(hello_plugin->widget);
    /* Add widget in Shell. Any number of widgets can be added */
    anjuta_shell_add_widget (plugin->shell, hello_plugin->widget,
                             "AnjutaHelloWorldPlugin",
                             _("Hello world plugin"),
                             GTK_STOCK_ABOUT,
                             ANJUTA_SHELL_PLACEMENT_CENTER,
                             NULL);

    g_printf("Helloworld is activated\n");

    return TRUE; /* FALSE if activation failed */
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
    HelloWorldPlugin *hello_plugin = (HelloWorldPlugin*) plugin;

    /* Remove widgets from Shell */
    anjuta_shell_remove_widget (plugin->shell, hello_plugin->widget, NULL);

    return TRUE; /* FALSE if plugin doesn't want to deactivate */
}

static void
hello_world_plugin_instance_init (GObject *obj)
{
}

static void
hello_world_plugin_class_init (GObjectClass *klass)
{
    AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);
    plugin_class->activate = activate_plugin;
    plugin_class->deactivate = deactivate_plugin;
}

ANJUTA_PLUGIN_BOILERPLATE (HelloWorldPlugin, hello_world_plugin);
ANJUTA_SIMPLE_PLUGIN (HelloWorldPlugin, hello_world_plugin);
