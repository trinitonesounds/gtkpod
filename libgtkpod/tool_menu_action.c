/*
| Example code taken from http://www.gtkforums.com/about4215.html
|
| tool_menu_action.c
*/
#include "tool_menu_action.h"

static void tool_menu_action_init (ToolMenuAction *tma);
static void tool_menu_action_class_init (ToolMenuActionClass *klass);

GType
tool_menu_action_get_type (void)
{
    static GType type = 0;
    if (! type)
    {
        static const GTypeInfo type_info =
        {
            sizeof (ToolMenuActionClass),
            NULL,
            NULL,
            (GClassInitFunc) tool_menu_action_class_init,
            NULL,
            NULL,
            sizeof (ToolMenuAction),
            0,
            (GInstanceInitFunc) tool_menu_action_init,
            NULL
        };

        type = g_type_register_static (GTK_TYPE_ACTION, "ToolMenuAction",
                &type_info, 0);
    }

    return type;
}

static void
tool_menu_action_class_init (ToolMenuActionClass *klass)
{
    GtkActionClass *action_class = GTK_ACTION_CLASS (klass);
    action_class->toolbar_item_type = GTK_TYPE_MENU_TOOL_BUTTON;
}

static void
tool_menu_action_init (ToolMenuAction *tma)
{
}

GtkAction *
tool_menu_action_new (const gchar *name, const gchar *label,
        const gchar *tooltip, const gchar *stock_id)
{
    g_return_val_if_fail (name != NULL, NULL);

    return g_object_new (TYPE_TOOL_MENU_ACTION,
            "name", name,
            "label", label,
            "tooltip", tooltip,
            "stock-id", stock_id,
            NULL);
}
