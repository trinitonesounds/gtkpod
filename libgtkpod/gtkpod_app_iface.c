/*
 |  Copyright (C) 2002-2009 Paul Richardson <phantom_sf at users sourceforge net>
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

#include "gtkpod_app_iface.h"
#include <glib/gi18n-lib.h>

static void gtkpod_app_base_init(GtkPodAppInterface* klass) {
    static gboolean initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;
    }
}

GType gtkpod_app_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo
                info = { sizeof(GtkPodAppInterface), (GBaseInitFunc) gtkpod_app_base_init, NULL, NULL, NULL, NULL, 0, 0, NULL };
        type = g_type_register_static(G_TYPE_INTERFACE, "GtkPodAppInterface", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);
    }
    return type;
}

/* Handler to be used when the button should be displayed, but no
   action is required */
void CONF_NULL_HANDLER (gpointer d1, gpointer d2)
{
    return;
}

void gtkpod_statusbar_message(gchar* message, ...) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    va_list args;
    va_start (args, message);

    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->statusbar_message(gtkpod_app, message, args);
    va_end (args);
}

void gtkpod_warning(gchar* message, ...) {
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    va_list args;
    va_start (args, message);

    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_warning(gtkpod_app, message, args);
    va_end (args);
}

void gtkpod_warning_simple (const gchar *format, ...)
{
    va_list arg;
    gchar *text;

    va_start (arg, format);
    text = g_strdup_vprintf (format, arg);
    va_end (arg);

    gtkpod_warning_hig (GTK_MESSAGE_WARNING, _("Warning"), text);
    g_free (text);
}

void gtkpod_warning_hig(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text)
{
    g_return_if_fail (GTKPOD_IS_APP(gtkpod_app));
    GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_warning_hig(gtkpod_app, icon, primary_text, secondary_text);
}

GtkResponseType gtkpod_confirmation(gint id, gboolean modal, const gchar *title, const gchar *label, const gchar *text, const gchar *option1_text, CONF_STATE option1_state, const gchar *option1_key, const gchar *option2_text, CONF_STATE option2_state, const gchar *option2_key, gboolean confirm_again, const gchar *confirm_again_key, ConfHandler ok_handler, ConfHandler apply_handler, ConfHandler cancel_handler, gpointer user_data1, gpointer user_data2) {
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_confirmation(gtkpod_app, id, modal, title, label, text, option1_text, option1_state, option1_key, option2_text, option2_state, option2_key, confirm_again, confirm_again_key, ok_handler, apply_handler, cancel_handler, user_data1, user_data2);
}

gint gtkpod_confirmation_simple(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text) {
    return gtkpod_confirmation_hig(icon, primary_text, secondary_text, accept_button_text, NULL, NULL, NULL);
}

gint gtkpod_confirmation_hig(GtkMessageType icon, const gchar *primary_text, const gchar *secondary_text, const gchar *accept_button_text, const gchar *cancel_button_text, const gchar *third_button_text, const gchar *help_context) {
    return GTKPOD_APP_GET_INTERFACE (gtkpod_app)->gtkpod_confirmation_hig(gtkpod_app, icon, primary_text, secondary_text, accept_button_text, cancel_button_text, third_button_text, help_context);
}
