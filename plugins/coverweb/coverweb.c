/*
 |  Copyright (C) 2002-2010 Jorg Schuler <jcsjcs at users sourceforge net>
 |                                          Paul Richardson <phantom_sf at users.sourceforge.net>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <webkit/webkit.h>
#include "libgtkpod/prefs.h"
#include "plugin.h"
#include "coverweb.h"

static WebBrowser *browser;

static void activate_uri_entry_cb(GtkWidget* entry, gpointer data) {
    const gchar* uri = gtk_entry_get_text(GTK_ENTRY (entry));
    g_assert (uri);
    webkit_web_view_open(WEBKIT_WEB_VIEW(browser->webview), uri);
}

static void link_hover_cb(WebKitWebView* page, const gchar* title, const gchar* link, gpointer data) {
    /* underflow is allowed */
    gtk_statusbar_pop(browser->statusbar, browser->status_context_id);
    if (link)
        gtk_statusbar_push(browser->statusbar, browser->status_context_id, link);
}

static void progress_change_cb(WebKitWebView* page, gint progress, gpointer data) {
    browser->load_progress = progress;
}

static void load_commit_cb(WebKitWebView* page, WebKitWebFrame* frame, gpointer data) {
    const gchar* uri = webkit_web_frame_get_uri(frame);
    if (uri)
        gtk_entry_set_text(GTK_ENTRY (browser->uri_entry), uri);
}

static void go_back_cb(GtkWidget* widget, gpointer data) {
    webkit_web_view_go_back(WEBKIT_WEB_VIEW(browser->webview));
}

static void go_forward_cb(GtkWidget* widget, gpointer data) {
    webkit_web_view_go_forward(WEBKIT_WEB_VIEW(browser->webview));
}

static void bookmark_menu_item_cb(GtkMenuItem* mi, gpointer data) {
    webkit_web_view_open(WEBKIT_WEB_VIEW(browser->webview), gtk_menu_item_get_label(mi));
}

static void create_browser() {
    browser->browser_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (browser->browser_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    browser->webview = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(browser->browser_window), browser->webview);

    g_signal_connect (G_OBJECT (browser->webview), "load-progress-changed", G_CALLBACK (progress_change_cb), browser);
    g_signal_connect (G_OBJECT (browser->webview), "load-committed", G_CALLBACK (load_commit_cb), browser);
    g_signal_connect (G_OBJECT (browser->webview), "hovering-over-link", G_CALLBACK (link_hover_cb), browser);

    WebKitWebSettings* settings = webkit_web_settings_new();
    g_object_set(G_OBJECT(settings), "enable-private-browsing", FALSE, NULL);
    g_object_set(G_OBJECT(settings), "enable-plugins", FALSE, NULL);
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(browser->webview), settings);

    /* Translators: you may change the web address to get a localized page, if it exists */
    webkit_web_view_open(WEBKIT_WEB_VIEW(browser->webview), _("http://images.google.com"));
}

static void create_statusbar() {
    browser->statusbar = GTK_STATUSBAR (gtk_statusbar_new ());
    browser->status_context_id = gtk_statusbar_get_context_id(browser->statusbar, "Link Hover");
}

void update_bookmark_menu() {
    GList *bookmarks;
    gint i;

    if (browser->bookmark_menu) {
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(browser->bookmark_menu_item), NULL);
        browser->bookmark_menu = NULL;
    }

    browser->bookmark_menu = gtk_menu_new();
    bookmarks = prefs_get_list("coverweb_bookmark_");
    for (i = 0; i < g_list_length(bookmarks); ++i) {
        gchar *bookmark = g_list_nth_data(bookmarks, i);
        GtkWidget *bookitem = gtk_menu_item_new_with_label(bookmark);
        gtk_menu_shell_append(GTK_MENU_SHELL (browser->bookmark_menu), bookitem);
        g_signal_connect (G_OBJECT (bookitem), "activate", G_CALLBACK(bookmark_menu_item_cb), (gpointer) browser);
        gtk_widget_show(bookitem);
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM (browser->bookmark_menu_item), browser->bookmark_menu);
}

static void create_menubar() {
    browser->menubar = gtk_menu_bar_new();
    browser->bookmark_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CDROM, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(browser->bookmark_menu_item), _("Bookmarks"));
    update_bookmark_menu();
    gtk_menu_shell_append(GTK_MENU_SHELL(browser->menubar), browser->bookmark_menu_item);
}

static void create_toolbar() {
    browser->toolbar = gtk_toolbar_new();
#ifndef GTK_ORIENTABLE
    gtk_toolbar_set_orientation(GTK_TOOLBAR (browser->toolbar), GTK_ORIENTATION_HORIZONTAL);
#else
    gtk_orientable_set_orientation(GTK_ORIENTABLE (browser->toolbar), GTK_ORIENTATION_HORIZONTAL);
#endif
    gtk_toolbar_set_style(GTK_TOOLBAR (browser->toolbar), GTK_TOOLBAR_BOTH_HORIZ);

    GtkToolItem* item;
    /* the back button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (go_back_cb), (gpointer) browser);
    gtk_toolbar_insert(GTK_TOOLBAR (browser->toolbar), item, -1);

    /* The forward button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (go_forward_cb), (gpointer) browser);
    gtk_toolbar_insert(GTK_TOOLBAR (browser->toolbar), item, -1);

    /* The URL entry */
    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);

    browser->uri_entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER (item), browser->uri_entry);
    g_signal_connect (G_OBJECT (browser->uri_entry), "activate", G_CALLBACK (activate_uri_entry_cb), (gpointer)browser);
    gtk_toolbar_insert(GTK_TOOLBAR (browser->toolbar), item, -1);

    /* The go button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect_swapped (G_OBJECT (item), "clicked", G_CALLBACK (activate_uri_entry_cb), (gpointer)browser->uri_entry);
    gtk_toolbar_insert(GTK_TOOLBAR (browser->toolbar), item, -1);
}

/**
 *
 * init_web_browser
 *
 * Initialise the webkit browser
 *
 * @parent: Widget to house the browser
 */
WebBrowser *init_web_browser(GtkWidget *parent) {
    browser = g_new0(WebBrowser, 1);

    create_menubar();
    create_toolbar();
    create_browser();
    create_statusbar();

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX (vbox), browser->menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (vbox), browser->toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (vbox), browser->browser_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX (vbox), GTK_WIDGET(browser->statusbar), FALSE, FALSE, 0);

    if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent), vbox);
    else
        gtk_container_add(GTK_CONTAINER (parent), vbox);

    return browser;
}

/**
 *
 * destroy_cover_web
 *
 * destroy the web browser and all associated items.
 */
void destroy_cover_web() {
    if (GTK_IS_WIDGET(browser->window))
        gtk_widget_destroy(browser->window);

    g_free(browser);
    browser = NULL;
}
