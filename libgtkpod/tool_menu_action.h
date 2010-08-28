/*
| Example code taken from http://www.gtkforums.com/about4215.html
|
| tool_menu_action.h
*/
#ifndef __TOOL_MENU_ACTION_H__
#define __TOOL_MENU_ACTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_TOOL_MENU_ACTION                (tool_menu_action_get_type ())
#define TOOL_MENU_ACTION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TOOL_MENU_ACTION, ToolMenuAction))
#define TOOL_MENU_ACTION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TOOL_MENU_ACTION, ToolMenuActionClass))
#define IS_TOOL_MENU_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TOOL_MENU_ACTION))
#define IS_TOOL_MENU_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TOOL_MENU_ACTION))

typedef struct _ToolMenuAction        ToolMenuAction;
typedef struct _ToolMenuActionClass    ToolMenuActionClass;

struct _ToolMenuAction
{
    GtkAction parent_instance;
};

struct _ToolMenuActionClass
{
    GtkActionClass parent_class;
};

GType tool_menu_action_get_type (void);
GtkAction * tool_menu_action_new (const gchar *name, const gchar *label,
        const gchar *tooltip, const gchar *stock_id);

G_END_DECLS

#endif /* __TOOL_MENU_ACTION_H__ */
