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

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"


#define UI_FILE ANJUTA_DATA_DIR"/ui/helloworld.ui"


#define GLADE_FILE ANJUTA_DATA_DIR"/glade/helloworld.glade"


static gpointer parent_class;


static void
on_sample_action_activate (GtkAction *action, AnjutaFoobarPlugin *plugin)
{
	GObject *obj;
	IAnjutaEditor *editor;
	IAnjutaDocumentManager *docman;

	/* Query for object implementing IAnjutaDocumentManager interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
									  "IAnjutaDocumentManager", NULL);
	docman = IANJUTA_DOCUMENT_MANAGER (obj);
	editor = IANJUTA_EDITOR (ianjuta_document_manager_get_current_document (docman, NULL));

	/* Do whatever with plugin */
	anjuta_util_dialog_info (GTK_WINDOW (ANJUTA_PLUGIN (plugin)->shell),
							 "Document manager pointer is: '0x%X'\n"
							 "Current Editor pointer is: 0x%X", docman,
							 editor);
}

static GtkActionEntry actions_file[] = {
	{
		"ActionFileSample",                       /* Action name */
		GTK_STOCK_NEW,                            /* Stock icon, if any */
		N_("_Sample action"),                     /* Display label */
		NULL,                                     /* short-cut */
		N_("Sample action"),                      /* Tooltip */
		G_CALLBACK (on_sample_action_activate)    /* action callback */
	}
};


static gboolean
helloworld_activate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;


	GtkWidget *wid;
	GladeXML *gxml;

	AnjutaFoobarPlugin *helloworld;

	DEBUG_PRINT ("%s", "AnjutaFoobarPlugin: Activating AnjutaFoobarPlugin plugin ...");
	helloworld = (AnjutaFoobarPlugin*) plugin;

	/* Add all UI actions and merge UI */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);

	helloworld->action_group =
		anjuta_ui_add_action_group_entries (ui, "ActionGroupFilehelloworld",
											_("Sample file operations"),
											actions_file,
											G_N_ELEMENTS (actions_file),
											GETTEXT_PACKAGE, TRUE,
											plugin);
	helloworld->uiid = anjuta_ui_merge (ui, UI_FILE);


	/* Add plugin widgets to Shell */
	gxml = glade_xml_new (GLADE_FILE, "top_widget", NULL);
	wid = glade_xml_get_widget (gxml, "top_widget");
	helloworld->widget = wid;
	anjuta_shell_add_widget (plugin->shell, wid,
							 "AnjutaFoobarPluginWidget", _("AnjutaFoobarPlugin widget"), NULL,
							 ANJUTA_SHELL_PLACEMENT_BOTTOM, NULL);
	g_object_unref (gxml);

	return TRUE;
}

static gboolean
helloworld_deactivate (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;

	DEBUG_PRINT ("%s", "AnjutaFoobarPlugin: Dectivating AnjutaFoobarPlugin plugin ...");

	anjuta_shell_remove_widget (plugin->shell, ((AnjutaFoobarPlugin*)plugin)->widget,
								NULL);


	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	anjuta_ui_remove_action_group (ui, ((AnjutaFoobarPlugin*)plugin)->action_group);
	anjuta_ui_unmerge (ui, ((AnjutaFoobarPlugin*)plugin)->uiid);

	return TRUE;
}

static void
helloworld_finalize (GObject *obj)
{
	/* Finalization codes here */
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
helloworld_dispose (GObject *obj)
{
	/* Disposition codes */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
helloworld_instance_init (GObject *obj)
{
	AnjutaFoobarPlugin *plugin = (AnjutaFoobarPlugin*)obj;

	plugin->uiid = 0;


	plugin->widget = NULL;

}

static void
helloworld_class_init (GObjectClass *klass)
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = helloworld_activate;
	plugin_class->deactivate = helloworld_deactivate;
	klass->finalize = helloworld_finalize;
	klass->dispose = helloworld_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (AnjutaFoobarPlugin, helloworld);
ANJUTA_SIMPLE_PLUGIN (AnjutaFoobarPlugin, helloworld);
