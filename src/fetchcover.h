#ifndef __FETCHCOVER_H__
#define __FETCHCOVER_H__

#include <string.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "display.h"
#include "display_private.h"
#include "details.h"
#include "itdb.h"
#include "display_coverart.h"
#include "prefs.h"


void on_coverart_context_menu_click (GList *tracks);
void on_fetchcover_fetch_button (GtkWidget *widget, gpointer data);

#endif
