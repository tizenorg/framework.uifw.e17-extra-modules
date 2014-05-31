#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"

typedef enum _E_Comp_Win_Class_Type
{
   E_COMP_WIN_CLASS_TYPE_UNKNOWN = 0,
   E_COMP_WIN_CLASS_TYPE_NORMAL,
   E_COMP_WIN_CLASS_TYPE_MENUSCREEN,
   E_COMP_WIN_CLASS_TYPE_QUICKPANEL_BASE,
   E_COMP_WIN_CLASS_TYPE_QUICKPANEL,
   E_COMP_WIN_CLASS_TYPE_TASKMANAGER,
   E_COMP_WIN_CLASS_TYPE_LIVEMAGAZINE,
   E_COMP_WIN_CLASS_TYPE_LOCKSCREEN,
   E_COMP_WIN_CLASS_TYPE_INDICATOR,
   E_COMP_WIN_CLASS_TYPE_TICKERNOTI,
   E_COMP_WIN_CLASS_TYPE_DEBUGGING_INFO,
   E_COMP_WIN_CLASS_TYPE_APPTRAY,
   E_COMP_WIN_CLASS_TYPE_MINI_APPTRAY,
   E_COMP_WIN_CLASS_TYPE_VOLUME,
   E_COMP_WIN_CLASS_TYPE_BACKGROUND,
   E_COMP_WIN_CLASS_TYPE_SETUP_WIZARD,
   E_COMP_WIN_CLASS_TYPE_ISF,
} E_Comp_Win_Class_Type;

typedef enum _E_Comp_Win_Name_Type
{
   E_COMP_WIN_NAME_TYPE_UNKNOWN = 0,
   E_COMP_WIN_NAME_TYPE_NORMAL,
   E_COMP_WIN_NAME_TYPE_MENUSCREEN,
   E_COMP_WIN_NAME_TYPE_QUICKPANEL_BASE,
   E_COMP_WIN_NAME_TYPE_QUICKPANEL,
   E_COMP_WIN_NAME_TYPE_TASKMANAGER,
   E_COMP_WIN_NAME_TYPE_LIVEMAGAZINE,
   E_COMP_WIN_NAME_TYPE_LOCKSCREEN,
   E_COMP_WIN_NAME_TYPE_INDICATOR,
   E_COMP_WIN_NAME_TYPE_TICKERNOTI,
   E_COMP_WIN_NAME_TYPE_APPTRAY,
   E_COMP_WIN_NAME_TYPE_MINI_APPTRAY,
   E_COMP_WIN_NAME_TYPE_VOLUME,
   E_COMP_WIN_NAME_TYPE_BACKGROUND,
   E_COMP_WIN_NAME_TYPE_SETUP_WIZARD,
   E_COMP_WIN_NAME_TYPE_ISF_KEYBOARD,
   E_COMP_WIN_NAME_TYPE_ISF_SUB,
} E_Comp_Win_Name_Type;

/* local subsystem functions */
static E_Comp_Win_Type _ecore_type_to_e_comp_type(Ecore_X_Window_Type t);

/* local subsystem globals */
static const char *win_class[] =
{
   "NORMAL_WINDOW",
   "MENU_SCREEN",
   "QUICKPANEL_BASE",
   "QUICKPANEL",
   "TASK_MANAGER",
   "LIVE_MAGAZINE",
   "LOCK_SCREEN",
   "lockscreen",
   "INDICATOR",
   "quickpanel",
   "DEBUGGING_INFO",
   "APP_TRAY",
   "MINIAPP_TRAY",
   "volume",
   "BACKGROUND",
   "SETUP_WIZARD",
   "ISF",
};

static const char *win_name[] =
{
   "NORMAL_WINDOW",
   "com.samsung.menu-screen",
   "QUICKPANEL_BASE",
   "QUICKPANEL",
   "TASK_MANAGER",
   "Live Magazine",
   "LOCK_SCREEN",
   "lockscreen",
   "INDICATOR",
   "noti_win",
   "APP_TRAY",
   "MINIAPP_TRAY",
   "volume",
   "BACKGROUND",
   "SETUP_WIZARD",
   // E_COMP_WIN_NAME_TYPE_ISF_KEYBOARD
   "Virtual Keyboard",
   // E_COMP_WIN_NAME_TYPE_ISF_SUB
   "Key Magnifier",
   "Prediction Window",
   "Setting Window",
   "ISF Popup",
};

static E_Comp_Win_Class_Type win_class_vals[] =
{
   E_COMP_WIN_CLASS_TYPE_NORMAL,
   E_COMP_WIN_CLASS_TYPE_MENUSCREEN,
   E_COMP_WIN_CLASS_TYPE_QUICKPANEL_BASE,
   E_COMP_WIN_CLASS_TYPE_QUICKPANEL,
   E_COMP_WIN_CLASS_TYPE_TASKMANAGER,
   E_COMP_WIN_CLASS_TYPE_LIVEMAGAZINE,
   E_COMP_WIN_CLASS_TYPE_LOCKSCREEN,
   E_COMP_WIN_CLASS_TYPE_LOCKSCREEN,
   E_COMP_WIN_CLASS_TYPE_INDICATOR,
   E_COMP_WIN_CLASS_TYPE_TICKERNOTI,
   E_COMP_WIN_CLASS_TYPE_DEBUGGING_INFO,
   E_COMP_WIN_CLASS_TYPE_APPTRAY,
   E_COMP_WIN_CLASS_TYPE_MINI_APPTRAY,
   E_COMP_WIN_CLASS_TYPE_VOLUME,
   E_COMP_WIN_CLASS_TYPE_BACKGROUND,
   E_COMP_WIN_CLASS_TYPE_SETUP_WIZARD,
   E_COMP_WIN_CLASS_TYPE_ISF,
};

static E_Comp_Win_Class_Type win_name_vals[] =
{
   E_COMP_WIN_NAME_TYPE_NORMAL,
   E_COMP_WIN_NAME_TYPE_MENUSCREEN,
   E_COMP_WIN_NAME_TYPE_QUICKPANEL_BASE,
   E_COMP_WIN_NAME_TYPE_QUICKPANEL,
   E_COMP_WIN_NAME_TYPE_TASKMANAGER,
   E_COMP_WIN_NAME_TYPE_LIVEMAGAZINE,
   E_COMP_WIN_NAME_TYPE_LOCKSCREEN,
   E_COMP_WIN_NAME_TYPE_LOCKSCREEN,
   E_COMP_WIN_NAME_TYPE_INDICATOR,
   E_COMP_WIN_NAME_TYPE_TICKERNOTI,
   E_COMP_WIN_NAME_TYPE_APPTRAY,
   E_COMP_WIN_NAME_TYPE_MINI_APPTRAY,
   E_COMP_WIN_NAME_TYPE_VOLUME,
   E_COMP_WIN_NAME_TYPE_BACKGROUND,
   E_COMP_WIN_NAME_TYPE_SETUP_WIZARD,
   E_COMP_WIN_NAME_TYPE_ISF_KEYBOARD,
   E_COMP_WIN_NAME_TYPE_ISF_SUB,
};

static Eina_Hash *class_hash  = NULL;
static Eina_Hash *names_hash  = NULL;

/* externally accessible functions */
EINTERN int
e_mod_comp_win_type_init(void)
{
   int i, n;
   if (!class_hash) class_hash = eina_hash_string_superfast_new(NULL);
   if (!names_hash) names_hash = eina_hash_string_superfast_new(NULL);

   n = (sizeof(win_class) / sizeof(char *));
   for (i = 0; i < n; i++)
     {
        eina_hash_add
          (class_hash, win_class[i],
          &win_class_vals[i]);
     }

   n = (sizeof(win_name) / sizeof(char *));
   for (i = 0; i < n; i++)
     {
        if (i >= E_COMP_WIN_NAME_TYPE_ISF_SUB)
          {
             eina_hash_add
               (names_hash, win_name[i],
               &win_name_vals[E_COMP_WIN_NAME_TYPE_ISF_SUB-1]);
          }
        else
          {
             eina_hash_add
               (names_hash,
               win_name[i], &win_name_vals[i]);
          }
     }

   return 1;
}

EINTERN int
e_mod_comp_win_type_shutdown(void)
{
   if (class_hash) eina_hash_free(class_hash);
   if (names_hash) eina_hash_free(names_hash);

   names_hash = NULL;
   class_hash = NULL;

   return 1;
}

EINTERN Eina_Bool
e_mod_comp_win_type_setup(E_Comp_Win *cw)
{
   Ecore_X_Window_Type    wtype  = ECORE_X_WINDOW_TYPE_UNKNOWN;
   E_Comp_Win_Class_Type  ctype  = E_COMP_WIN_CLASS_TYPE_UNKNOWN;
   E_Comp_Win_Name_Type   ntype  = E_COMP_WIN_NAME_TYPE_UNKNOWN;
   E_Comp_Win_Type        res    = E_COMP_WIN_TYPE_UNKNOWN;
   E_Comp_Win_Class_Type *pctype = NULL;
   E_Comp_Win_Name_Type  *pntype = NULL;
   char *clas = NULL, *name = NULL;

   E_CHECK_RETURN(cw, 0);

   if (cw->bd)
     {
        wtype = cw->bd->client.netwm.type;
        if (cw->bd->internal)
          ecore_x_icccm_name_class_get(cw->bd->client.win, &name, &clas);
        else
          {
             clas = (char *)cw->bd->client.icccm.class;
             name = (char *)cw->bd->client.icccm.name;
          }
     }
   else
     ecore_x_icccm_name_class_get(cw->win, &name, &clas);

   if (clas)
     {
        pctype = eina_hash_find(class_hash, clas);
        if (pctype) ctype = *pctype;
     }
   if (name)
     {
        pntype = eina_hash_find(names_hash, name);
        if (pntype) ntype = *pntype;
     }

   switch (ctype)
     {
      case E_COMP_WIN_CLASS_TYPE_MENUSCREEN:
         res = E_COMP_WIN_TYPE_MENUSCREEN;
         break;
      case E_COMP_WIN_CLASS_TYPE_QUICKPANEL_BASE:
         res = E_COMP_WIN_TYPE_QUICKPANEL_BASE;
         break;
      case E_COMP_WIN_CLASS_TYPE_QUICKPANEL:
         res = E_COMP_WIN_TYPE_QUICKPANEL;
         break;
      case E_COMP_WIN_CLASS_TYPE_TASKMANAGER:
         if (ntype == E_COMP_WIN_NAME_TYPE_TASKMANAGER)
           res = E_COMP_WIN_TYPE_TASKMANAGER;
         break;
      case E_COMP_WIN_CLASS_TYPE_LIVEMAGAZINE:
         if ((ntype == E_COMP_WIN_NAME_TYPE_LIVEMAGAZINE) &&
             (wtype == ECORE_X_WINDOW_TYPE_NORMAL))
           res = E_COMP_WIN_TYPE_LIVEMAGAZINE;
         break;
      case E_COMP_WIN_CLASS_TYPE_LOCKSCREEN:
         if (ntype == E_COMP_WIN_NAME_TYPE_LOCKSCREEN)
           res = E_COMP_WIN_TYPE_LOCKSCREEN;
         break;
      case E_COMP_WIN_CLASS_TYPE_INDICATOR:
         if ((ntype == E_COMP_WIN_NAME_TYPE_INDICATOR) &&
             (wtype == ECORE_X_WINDOW_TYPE_DOCK))
           res = E_COMP_WIN_TYPE_INDICATOR;
         break;
      case E_COMP_WIN_CLASS_TYPE_TICKERNOTI:
         if ((ntype == E_COMP_WIN_NAME_TYPE_TICKERNOTI) &&
             (wtype == ECORE_X_WINDOW_TYPE_NOTIFICATION))
           res = E_COMP_WIN_TYPE_TICKERNOTI;
         break;
      case E_COMP_WIN_CLASS_TYPE_DEBUGGING_INFO:
           res = E_COMP_WIN_TYPE_DEBUGGING_INFO;
         break;
      case E_COMP_WIN_CLASS_TYPE_ISF:
         if (wtype != ECORE_X_WINDOW_TYPE_UTILITY)
           break;
         else if (ntype == E_COMP_WIN_NAME_TYPE_ISF_KEYBOARD)
           res = E_COMP_WIN_TYPE_ISF_KEYBOARD;
         else if (ntype == E_COMP_WIN_NAME_TYPE_ISF_SUB)
           res = E_COMP_WIN_TYPE_ISF_SUB;
         break;
      case E_COMP_WIN_CLASS_TYPE_NORMAL:
         if ((wtype == ECORE_X_WINDOW_TYPE_NORMAL) ||
             (wtype == ECORE_X_WINDOW_TYPE_UNKNOWN))
           res = E_COMP_WIN_TYPE_NORMAL;
         break;
      case E_COMP_WIN_CLASS_TYPE_APPTRAY:
         if (ntype == E_COMP_WIN_NAME_TYPE_APPTRAY)
           res = E_COMP_WIN_TYPE_APPTRAY;
         break;
      case E_COMP_WIN_CLASS_TYPE_MINI_APPTRAY:
         if (ntype == E_COMP_WIN_NAME_TYPE_MINI_APPTRAY)
           res = E_COMP_WIN_TYPE_MINI_APPTRAY;
         break;
      case E_COMP_WIN_CLASS_TYPE_VOLUME:
         if ((ntype == E_COMP_WIN_NAME_TYPE_VOLUME) &&
             (wtype == ECORE_X_WINDOW_TYPE_NOTIFICATION))
           res = E_COMP_WIN_TYPE_VOLUME;
         break;
      case E_COMP_WIN_CLASS_TYPE_BACKGROUND:
         if (ntype == E_COMP_WIN_NAME_TYPE_BACKGROUND)
           res = E_COMP_WIN_TYPE_BACKGROUND;
         break;
      case E_COMP_WIN_CLASS_TYPE_SETUP_WIZARD:
         if (ntype == E_COMP_WIN_NAME_TYPE_SETUP_WIZARD)
         res = E_COMP_WIN_TYPE_SETUP_WIZARD;
         break;
      default:
         break;
     }

   if (res == E_COMP_WIN_TYPE_UNKNOWN)
     res = _ecore_type_to_e_comp_type(wtype);

   if (!cw->bd ||
       (cw->bd && cw->bd->internal))
     {
        if (name) E_FREE(name);
        if (clas) E_FREE(clas);
     }

   cw->win_type = res;
   return EINA_TRUE;
}

EINTERN E_Comp_Win_Type
e_mod_comp_win_type_get(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, E_COMP_WIN_TYPE_UNKNOWN);
   return cw->win_type;
}

EINTERN Eina_Bool
e_mod_comp_win_type_handler_prop(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw;
   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   L(LT_EVENT_X, "COMP|%31s\n", "PROP_WM_CLASS");

   cw = e_mod_comp_border_client_find(ev->win);
   if (cw) return EINA_TRUE;

   cw = e_mod_comp_win_find(ev->win);
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN((!cw->bd), 0);

   e_mod_comp_win_type_setup(cw);
   return EINA_TRUE;
}

/* local subsystem functions */
static E_Comp_Win_Type
_ecore_type_to_e_comp_type(Ecore_X_Window_Type t)
{
   E_Comp_Win_Type r = E_COMP_WIN_TYPE_UNKNOWN;
   switch (t)
     {
      case ECORE_X_WINDOW_TYPE_NORMAL:        r = E_COMP_WIN_TYPE_NORMAL;        break;
      case ECORE_X_WINDOW_TYPE_TOOLTIP:       r = E_COMP_WIN_TYPE_TOOLTIP;       break;
      case ECORE_X_WINDOW_TYPE_COMBO:         r = E_COMP_WIN_TYPE_COMBO;         break;
      case ECORE_X_WINDOW_TYPE_DND:           r = E_COMP_WIN_TYPE_DND;           break;
      case ECORE_X_WINDOW_TYPE_DESKTOP:       r = E_COMP_WIN_TYPE_DESKTOP;       break;
      case ECORE_X_WINDOW_TYPE_TOOLBAR:       r = E_COMP_WIN_TYPE_TOOLBAR;       break;
      case ECORE_X_WINDOW_TYPE_MENU:          r = E_COMP_WIN_TYPE_MENU;          break;
      case ECORE_X_WINDOW_TYPE_SPLASH:        r = E_COMP_WIN_TYPE_SPLASH;        break;
      case ECORE_X_WINDOW_TYPE_DROPDOWN_MENU: r = E_COMP_WIN_TYPE_DROPDOWN_MENU; break;
      case ECORE_X_WINDOW_TYPE_NOTIFICATION:  r = E_COMP_WIN_TYPE_NOTIFICATION;  break;
      case ECORE_X_WINDOW_TYPE_UTILITY:       r = E_COMP_WIN_TYPE_UTILITY;       break;
      case ECORE_X_WINDOW_TYPE_POPUP_MENU:    r = E_COMP_WIN_TYPE_POPUP_MENU;    break;
      case ECORE_X_WINDOW_TYPE_DIALOG:        r = E_COMP_WIN_TYPE_DIALOG;        break;
      default:
        r = E_COMP_WIN_TYPE_NORMAL;
        break;
     }
   return r;
}
