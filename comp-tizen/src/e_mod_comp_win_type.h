#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_WIN_TYPE_H
#define E_MOD_COMP_WIN_TYPE_H

#define TYPE_NORMAL_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_NORMAL)

#define TYPE_INDICATOR_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_INDICATOR)

#define TYPE_TICKERNOTI_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_TICKERNOTI)

#define TYPE_DEBUGGING_INFO_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_DEBUGGING_INFO)

#define TYPE_TASKMANAGER_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_TASKMANAGER)

#define TYPE_HOME_CHECK(a) \
   (((a)->win_type == E_COMP_WIN_TYPE_MENUSCREEN) || \
    ((a)->win_type == E_COMP_WIN_TYPE_LIVEMAGAZINE))

#define TYPE_QUICKPANEL_CHECK(a) \
   (((a)->win_type == E_COMP_WIN_TYPE_QUICKPANEL_BASE) || \
    ((a)->win_type == E_COMP_WIN_TYPE_QUICKPANEL))

#define TYPE_BG_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_BACKGROUND)

#define TYPE_KEYBOARD_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_ISF_KEYBOARD)

#define TYPE_APPTRAY_CHECK(a) \
   ((a)->win_type == E_COMP_WIN_TYPE_APPTRAY)

typedef enum _E_Comp_Win_Type
{
   E_COMP_WIN_TYPE_UNKNOWN = 0,
   E_COMP_WIN_TYPE_DESKTOP,
   E_COMP_WIN_TYPE_DOCK,
   E_COMP_WIN_TYPE_TOOLBAR,
   E_COMP_WIN_TYPE_MENU,
   E_COMP_WIN_TYPE_UTILITY,
   E_COMP_WIN_TYPE_SPLASH,
   E_COMP_WIN_TYPE_DIALOG,
   E_COMP_WIN_TYPE_NORMAL,
   E_COMP_WIN_TYPE_DROPDOWN_MENU,
   E_COMP_WIN_TYPE_POPUP_MENU,
   E_COMP_WIN_TYPE_TOOLTIP,
   E_COMP_WIN_TYPE_NOTIFICATION,
   E_COMP_WIN_TYPE_COMBO,
   E_COMP_WIN_TYPE_DND,
   /* added type */
   E_COMP_WIN_TYPE_MENUSCREEN,
   E_COMP_WIN_TYPE_QUICKPANEL_BASE,
   E_COMP_WIN_TYPE_QUICKPANEL,
   E_COMP_WIN_TYPE_TASKMANAGER,
   E_COMP_WIN_TYPE_LIVEMAGAZINE,
   E_COMP_WIN_TYPE_LOCKSCREEN,
   E_COMP_WIN_TYPE_INDICATOR,
   E_COMP_WIN_TYPE_TICKERNOTI,
   E_COMP_WIN_TYPE_DEBUGGING_INFO,
   E_COMP_WIN_TYPE_APPTRAY,
   E_COMP_WIN_TYPE_BACKGROUND,
   E_COMP_WIN_TYPE_ISF_KEYBOARD,
   E_COMP_WIN_TYPE_ISF_SUB,
} E_Comp_Win_Type;

/* comp window type functions */
EINTERN int             e_mod_comp_win_type_init(void);
EINTERN int             e_mod_comp_win_type_shutdown(void);
EINTERN Eina_Bool       e_mod_comp_win_type_setup(E_Comp_Win *cw);
EINTERN E_Comp_Win_Type e_mod_comp_win_type_get(E_Comp_Win *cw);
EINTERN Eina_Bool       e_mod_comp_win_type_handler_prop(Ecore_X_Event_Window_Property *ev);

#endif
#endif
