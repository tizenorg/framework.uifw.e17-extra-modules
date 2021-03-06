#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_BORDER_TYPE_H
#define E_MOD_MOVE_BORDER_TYPE_H

#define TYPE_NORMAL_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_NORMAL)

#define TYPE_INDICATOR_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_INDICATOR)

#define TYPE_QUICKPANEL_BASE_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_QUICKPANEL_BASE)

#define TYPE_QUICKPANEL_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_QUICKPANEL)

#define TYPE_APPTRAY_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_APPTRAY)

#define TYPE_LOCKSCREEN_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_LOCKSCREEN)

#define TYPE_TASKMANAGER_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_TASKMANAGER)

#define TYPE_PWLOCK_CHECK(a) \
   ((a)->type == E_MOVE_BORDER_TYPE_PWLOCK)

typedef enum _E_Move_Border_Type
{
   E_MOVE_BORDER_TYPE_UNKNOWN = 0,
   E_MOVE_BORDER_TYPE_DESKTOP,
   E_MOVE_BORDER_TYPE_DOCK,
   E_MOVE_BORDER_TYPE_TOOLBAR,
   E_MOVE_BORDER_TYPE_MENU,
   E_MOVE_BORDER_TYPE_UTILITY,
   E_MOVE_BORDER_TYPE_SPLASH,
   E_MOVE_BORDER_TYPE_DIALOG,
   E_MOVE_BORDER_TYPE_NORMAL,
   E_MOVE_BORDER_TYPE_DROPDOWN_MENU,
   E_MOVE_BORDER_TYPE_POPUP_MENU,
   E_MOVE_BORDER_TYPE_TOOLTIP,
   E_MOVE_BORDER_TYPE_NOTIFICATION,
   E_MOVE_BORDER_TYPE_COMBO,
   E_MOVE_BORDER_TYPE_DND,
   /* added type */
   E_MOVE_BORDER_TYPE_MENUSCREEN,
   E_MOVE_BORDER_TYPE_QUICKPANEL_BASE,
   E_MOVE_BORDER_TYPE_QUICKPANEL,
   E_MOVE_BORDER_TYPE_TASKMANAGER,
   E_MOVE_BORDER_TYPE_LIVEMAGAZINE,
   E_MOVE_BORDER_TYPE_LOCKSCREEN,
   E_MOVE_BORDER_TYPE_INDICATOR,
   E_MOVE_BORDER_TYPE_APPTRAY,
   E_MOVE_BORDER_TYPE_PWLOCK,
   E_MOVE_BORDER_TYPE_BACKGROUND,
   E_MOVE_BORDER_TYPE_ISF_KEYBOARD,
   E_MOVE_BORDER_TYPE_ISF_SUB,
} E_Move_Border_Type;

/* move border type functions */
EINTERN int                e_mod_move_border_type_init(void);
EINTERN int                e_mod_move_border_type_shutdown(void);
EINTERN Eina_Bool          e_mod_move_border_type_setup(E_Move_Border *mb);
EINTERN E_Move_Border_Type e_mod_move_border_type_get(E_Move_Border *mb);
EINTERN Eina_Bool          e_mod_move_border_type_handler_prop(Ecore_X_Event_Window_Property *ev);
EINTERN const char        *e_mod_move_border_types_name_get(E_Move_Border_Type t);

#endif
#endif
