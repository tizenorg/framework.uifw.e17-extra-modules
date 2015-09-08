#include "e_mod_move_shared_types.h"
#include "e_mod_move.h"
#include "e_mod_move_debug.h"

typedef enum _E_Move_Border_Class_Type
{
   E_MOVE_BORDER_CLASS_TYPE_UNKNOWN = 0,
   E_MOVE_BORDER_CLASS_TYPE_NORMAL,
   E_MOVE_BORDER_CLASS_TYPE_MENUSCREEN,
   E_MOVE_BORDER_CLASS_TYPE_QUICKPANEL_BASE,
   E_MOVE_BORDER_CLASS_TYPE_QUICKPANEL,
   E_MOVE_BORDER_CLASS_TYPE_TASKMANAGER,
   E_MOVE_BORDER_CLASS_TYPE_LIVEMAGAZINE,
   E_MOVE_BORDER_CLASS_TYPE_LOCKSCREEN,
   E_MOVE_BORDER_CLASS_TYPE_INDICATOR,
   E_MOVE_BORDER_CLASS_TYPE_APPTRAY,
   E_MOVE_BORDER_CLASS_TYPE_MINI_APPTRAY,
   E_MOVE_BORDER_CLASS_TYPE_SETUP_WIZARD,
   E_MOVE_BORDER_CLASS_TYPE_APP_SELECTOR,
   E_MOVE_BORDER_CLASS_TYPE_APP_POPUP,
   E_MOVE_BORDER_CLASS_TYPE_SPLIT_LAUNCHER,
   E_MOVE_BORDER_CLASS_TYPE_PWLOCK,
   E_MOVE_BORDER_CLASS_TYPE_BACKGROUND,
   E_MOVE_BORDER_CLASS_TYPE_ISF,
} E_Move_Border_Class_Type;

typedef enum _E_Move_Border_Name_Type
{
   E_MOVE_BORDER_NAME_TYPE_UNKNOWN = 0,
   E_MOVE_BORDER_NAME_TYPE_NORMAL,
   E_MOVE_BORDER_NAME_TYPE_MENUSCREEN,
   E_MOVE_BORDER_NAME_TYPE_QUICKPANEL_BASE,
   E_MOVE_BORDER_NAME_TYPE_QUICKPANEL,
   E_MOVE_BORDER_NAME_TYPE_TASKMANAGER,
   E_MOVE_BORDER_NAME_TYPE_LIVEMAGAZINE,
   E_MOVE_BORDER_NAME_TYPE_LOCKSCREEN,
   E_MOVE_BORDER_NAME_TYPE_INDICATOR,
   E_MOVE_BORDER_NAME_TYPE_APPTRAY,
   E_MOVE_BORDER_NAME_TYPE_MINI_APPTRAY,
   E_MOVE_BORDER_NAME_TYPE_SETUP_WIZARD,
   E_MOVE_BORDER_NAME_TYPE_APP_SELECTOR,
   E_MOVE_BORDER_NAME_TYPE_APP_POPUP,
   E_MOVE_BORDER_NAME_TYPE_SPLIT_LAUNCHER,
   E_MOVE_BORDER_NAME_TYPE_PWLOCK,
   E_MOVE_BORDER_NAME_TYPE_BACKGROUND,
   E_MOVE_BORDER_NAME_TYPE_ISF_KEYBOARD,
   E_MOVE_BORDER_NAME_TYPE_ISF_SUB,
} E_Move_Border_Name_Type;

/* local subsystem functions */
static E_Move_Border_Type _ecore_type_to_e_move_type(Ecore_X_Window_Type t);

/* local subsystem globals */
static const char *border_class[] =
{
   "NORMAL_WINDOW",
   "MENU_SCREEN",
   "QUICKPANEL_BASE",
   "QUICKPANEL",
   "TASK_MANAGER",
   "LIVE_MAGAZINE",
   "LOCK_SCREEN",
   "INDICATOR",
   "APP_TRAY",
   "MINIAPP_TRAY",
   "SETUP_WIZARD",
   "APP_SELECTOR",
   "APP_POPUP",
   "SPLIT_LAUNCHER",
   //"PW_LOCK",
   "pwlock",
   "BACKGROUND",
   "ISF",
};

static const char *border_name[] =
{
   "NORMAL_WINDOW",
   "org.tizen.menu-screen",
   "QUICKPANEL_BASE",
   "QUICKPANEL",
   "TASK_MANAGER",
   "Live Magazine",
   "LOCK_SCREEN",
   "INDICATOR",
   "APP_TRAY",
   "MINIAPP_TRAY",
   "SETUP_WIZARD",
   "APP_SELECTOR",
   "APP_POPUP",
   "SPLIT_LAUNCHER",
   //"PW_LOCK",
   "pwlock",
   "BACKGROUND",
   // E_COMP_WIN_NAME_TYPE_ISF_KEYBOARD
   "Virtual Keyboard",
   // E_COMP_WIN_NAME_TYPE_ISF_SUB
   "Key Magnifier",
   "Prediction Window",
   "Setting Window",
   "ISF Popup",
};

static const char *type_names[] =
{
   "E_MOVE_BORDER_TYPE_UNKNOWN",
   "E_MOVE_BORDER_TYPE_DESKTOP",
   "E_MOVE_BORDER_TYPE_DOCK",
   "E_MOVE_BORDER_TYPE_TOOLBAR",
   "E_MOVE_BORDER_TYPE_MENU",
   "E_MOVE_BORDER_TYPE_UTILITY",
   "E_MOVE_BORDER_TYPE_SPLASH",
   "E_MOVE_BORDER_TYPE_DIALOG",
   "E_MOVE_BORDER_TYPE_NORMAL",
   "E_MOVE_BORDER_TYPE_DROPDOWN_MENU",
   "E_MOVE_BORDER_TYPE_POPUP_MENU",
   "E_MOVE_BORDER_TYPE_TOOLTIP",
   "E_MOVE_BORDER_TYPE_NOTIFICATION",
   "E_MOVE_BORDER_TYPE_COMBO",
   "E_MOVE_BORDER_TYPE_DND",
   "E_MOVE_BORDER_TYPE_MENUSCREEN",
   "E_MOVE_BORDER_TYPE_QUICKPANEL_BASE",
   "E_MOVE_BORDER_TYPE_QUICKPANEL",
   "E_MOVE_BORDER_TYPE_TASKMANAGER",
   "E_MOVE_BORDER_TYPE_LIVEMAGAZINE",
   "E_MOVE_BORDER_TYPE_LOCKSCREEN",
   "E_MOVE_BORDER_TYPE_INDICATOR",
   "E_MOVE_BORDER_TYPE_APPTRAY",
   "E_MOVE_BORDER_TYPE_MINI_APPTRAY",
   "E_MOVE_BORDER_TYPE_SETUP_WIZARD",
   "E_MOVE_BORDER_TYPE_APP_SELECTOR",
   "E_MOVE_BORDER_TYPE_APP_POPUP",
   "E_MOVE_BORDER_TYPE_SPLIT_LAUNCHER",
   "E_MOVE_BORDER_TYPE_PWLOCK",
   "E_MOVE_BORDER_TYPE_BACKGROUND",
   "E_MOVE_BORDER_TYPE_ISF_KEYBOARD",
   "E_MOVE_BORDER_TYPE_ISF_SUB",
};

static E_Move_Border_Class_Type border_class_vals[] =
{
   E_MOVE_BORDER_CLASS_TYPE_NORMAL,
   E_MOVE_BORDER_CLASS_TYPE_MENUSCREEN,
   E_MOVE_BORDER_CLASS_TYPE_QUICKPANEL_BASE,
   E_MOVE_BORDER_CLASS_TYPE_QUICKPANEL,
   E_MOVE_BORDER_CLASS_TYPE_TASKMANAGER,
   E_MOVE_BORDER_CLASS_TYPE_LIVEMAGAZINE,
   E_MOVE_BORDER_CLASS_TYPE_LOCKSCREEN,
   E_MOVE_BORDER_CLASS_TYPE_INDICATOR,
   E_MOVE_BORDER_CLASS_TYPE_APPTRAY,
   E_MOVE_BORDER_CLASS_TYPE_MINI_APPTRAY,
   E_MOVE_BORDER_CLASS_TYPE_SETUP_WIZARD,
   E_MOVE_BORDER_CLASS_TYPE_APP_SELECTOR,
   E_MOVE_BORDER_CLASS_TYPE_APP_POPUP,
   E_MOVE_BORDER_CLASS_TYPE_SPLIT_LAUNCHER,
   E_MOVE_BORDER_CLASS_TYPE_PWLOCK,
   E_MOVE_BORDER_CLASS_TYPE_BACKGROUND,
   E_MOVE_BORDER_CLASS_TYPE_ISF
};

static E_Move_Border_Class_Type border_name_vals[] =
{
   E_MOVE_BORDER_NAME_TYPE_NORMAL,
   E_MOVE_BORDER_NAME_TYPE_MENUSCREEN,
   E_MOVE_BORDER_NAME_TYPE_QUICKPANEL_BASE,
   E_MOVE_BORDER_NAME_TYPE_QUICKPANEL,
   E_MOVE_BORDER_NAME_TYPE_TASKMANAGER,
   E_MOVE_BORDER_NAME_TYPE_LIVEMAGAZINE,
   E_MOVE_BORDER_NAME_TYPE_LOCKSCREEN,
   E_MOVE_BORDER_NAME_TYPE_INDICATOR,
   E_MOVE_BORDER_NAME_TYPE_APPTRAY,
   E_MOVE_BORDER_NAME_TYPE_MINI_APPTRAY,
   E_MOVE_BORDER_NAME_TYPE_SETUP_WIZARD,
   E_MOVE_BORDER_NAME_TYPE_APP_SELECTOR,
   E_MOVE_BORDER_NAME_TYPE_APP_POPUP,
   E_MOVE_BORDER_NAME_TYPE_SPLIT_LAUNCHER,
   E_MOVE_BORDER_NAME_TYPE_PWLOCK,
   E_MOVE_BORDER_NAME_TYPE_BACKGROUND,
   E_MOVE_BORDER_NAME_TYPE_ISF_KEYBOARD,
   E_MOVE_BORDER_NAME_TYPE_ISF_SUB
};

static Eina_Hash *class_hash  = NULL;
static Eina_Hash *names_hash  = NULL;
static Eina_Hash *types_hash = NULL;

/* local subsystem functions */
static E_Move_Border_Type
_ecore_type_to_e_move_type(Ecore_X_Window_Type t)
{
   E_Move_Border_Type r = E_MOVE_BORDER_TYPE_UNKNOWN;
   switch (t)
     {
      case ECORE_X_WINDOW_TYPE_NORMAL:        r = E_MOVE_BORDER_TYPE_NORMAL;        break;
      case ECORE_X_WINDOW_TYPE_TOOLTIP:       r = E_MOVE_BORDER_TYPE_TOOLTIP;       break;
      case ECORE_X_WINDOW_TYPE_COMBO:         r = E_MOVE_BORDER_TYPE_COMBO;         break;
      case ECORE_X_WINDOW_TYPE_DND:           r = E_MOVE_BORDER_TYPE_DND;           break;
      case ECORE_X_WINDOW_TYPE_DESKTOP:       r = E_MOVE_BORDER_TYPE_DESKTOP;       break;
      case ECORE_X_WINDOW_TYPE_TOOLBAR:       r = E_MOVE_BORDER_TYPE_TOOLBAR;       break;
      case ECORE_X_WINDOW_TYPE_MENU:          r = E_MOVE_BORDER_TYPE_MENU;          break;
      case ECORE_X_WINDOW_TYPE_SPLASH:        r = E_MOVE_BORDER_TYPE_SPLASH;        break;
      case ECORE_X_WINDOW_TYPE_DROPDOWN_MENU: r = E_MOVE_BORDER_TYPE_DROPDOWN_MENU; break;
      case ECORE_X_WINDOW_TYPE_NOTIFICATION:  r = E_MOVE_BORDER_TYPE_NOTIFICATION;  break;
      case ECORE_X_WINDOW_TYPE_UTILITY:       r = E_MOVE_BORDER_TYPE_UTILITY;       break;
      case ECORE_X_WINDOW_TYPE_POPUP_MENU:    r = E_MOVE_BORDER_TYPE_POPUP_MENU;    break;
      case ECORE_X_WINDOW_TYPE_DIALOG:        r = E_MOVE_BORDER_TYPE_DIALOG;        break;
      default:
        r = E_MOVE_BORDER_TYPE_NORMAL;
        break;
     }
   return r;
}

/* externally accessible functions */
EINTERN int
e_mod_move_border_type_init(void)
{
   int i, n;
   char type_string[10];
   if (!class_hash) class_hash = eina_hash_string_superfast_new(NULL);
   if (!names_hash) names_hash = eina_hash_string_superfast_new(NULL);
   if (!types_hash) types_hash = eina_hash_string_superfast_new(NULL);

   n = (sizeof(border_class) / sizeof(char *));
   for (i = 0; i < n; i++)
     {
        eina_hash_add
          (class_hash, border_class[i],
          &border_class_vals[i]);
     }

   n = (sizeof(border_name) / sizeof(char *));
   for (i = 0; i < n; i++)
     {
        if (i >= E_MOVE_BORDER_NAME_TYPE_ISF_SUB)
          {
             eina_hash_add
               (names_hash, border_name[i],
               &border_name_vals[E_MOVE_BORDER_NAME_TYPE_ISF_SUB-1]);
          }
        else
          {
             eina_hash_add
               (names_hash,
               border_name[i], &border_name_vals[i]);
          }
     }

   for (i = E_MOVE_BORDER_TYPE_UNKNOWN; i < E_MOVE_BORDER_TYPE_ISF_SUB; i++)
     {
        memset(type_string, 0, sizeof(type_string));
        snprintf(type_string, sizeof(type_string), "%d", i);
        eina_hash_add(types_hash,
                      type_string,
                      type_names[i]);
     }

   return 1;
}

EINTERN int
e_mod_move_border_type_shutdown(void)
{
   if (class_hash) eina_hash_free(class_hash);
   if (names_hash) eina_hash_free(names_hash);
   if (types_hash) eina_hash_free(types_hash);

   names_hash = NULL;
   class_hash = NULL;
   types_hash = NULL;

   return 1;
}

EINTERN Eina_Bool
e_mod_move_border_type_setup(E_Move_Border *mb)
{
   Ecore_X_Window_Type       wtype  = ECORE_X_WINDOW_TYPE_UNKNOWN;
   E_Move_Border_Class_Type  ctype  = E_MOVE_BORDER_CLASS_TYPE_UNKNOWN;
   E_Move_Border_Name_Type   ntype  = E_MOVE_BORDER_NAME_TYPE_UNKNOWN;
   E_Move_Border_Type        res    = E_MOVE_BORDER_TYPE_UNKNOWN;
   E_Move_Border_Class_Type *pctype = NULL;
   E_Move_Border_Name_Type  *pntype = NULL;
   char *clas = NULL, *name = NULL;

   E_CHECK_RETURN(mb, 0);
   E_CHECK_RETURN(mb->bd, 0);

   wtype = mb->bd->client.netwm.type;
   if (mb->bd->internal)
     ecore_x_icccm_name_class_get(mb->bd->client.win, &name, &clas);
   else
     {
        clas = (char *)mb->bd->client.icccm.class;
        name = (char *)mb->bd->client.icccm.name;
     }

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
      case E_MOVE_BORDER_CLASS_TYPE_MENUSCREEN:
         res = E_MOVE_BORDER_TYPE_MENUSCREEN;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_QUICKPANEL_BASE:
         res = E_MOVE_BORDER_TYPE_QUICKPANEL_BASE;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_QUICKPANEL:
         res = E_MOVE_BORDER_TYPE_QUICKPANEL;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_TASKMANAGER:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_TASKMANAGER)
           res = E_MOVE_BORDER_TYPE_TASKMANAGER;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_LIVEMAGAZINE:
         if ((ntype == E_MOVE_BORDER_NAME_TYPE_LIVEMAGAZINE) &&
             (wtype == ECORE_X_WINDOW_TYPE_NORMAL))
           res = E_MOVE_BORDER_TYPE_LIVEMAGAZINE;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_LOCKSCREEN:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_LOCKSCREEN)
//         if ((ntype == E_MOVE_BORDER_NAME_TYPE_LOCKSCREEN) &&
//            (wtype == ECORE_X_WINDOW_TYPE_NOTIFICATION)) // net wm window type is chaged later. so it will be fixed.
           res = E_MOVE_BORDER_TYPE_LOCKSCREEN;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_INDICATOR:
         if ((ntype == E_MOVE_BORDER_NAME_TYPE_INDICATOR) &&
             (wtype == ECORE_X_WINDOW_TYPE_DOCK))
           res = E_MOVE_BORDER_TYPE_INDICATOR;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_ISF:
         if (wtype != ECORE_X_WINDOW_TYPE_UTILITY)
           break;
         else if (ntype == E_MOVE_BORDER_NAME_TYPE_ISF_KEYBOARD)
           res = E_MOVE_BORDER_TYPE_ISF_KEYBOARD;
         else if (ntype == E_MOVE_BORDER_NAME_TYPE_ISF_SUB)
           res = E_MOVE_BORDER_TYPE_ISF_SUB;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_NORMAL:
         if ((wtype == ECORE_X_WINDOW_TYPE_NORMAL) ||
             (wtype == ECORE_X_WINDOW_TYPE_UNKNOWN))
           res = E_MOVE_BORDER_TYPE_NORMAL;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_APPTRAY:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_APPTRAY)
           res = E_MOVE_BORDER_TYPE_APPTRAY;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_MINI_APPTRAY:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_MINI_APPTRAY)
           res = E_MOVE_BORDER_TYPE_MINI_APPTRAY;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_SETUP_WIZARD:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_SETUP_WIZARD)
           res = E_MOVE_BORDER_TYPE_SETUP_WIZARD;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_APP_SELECTOR:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_APP_SELECTOR)
           res = E_MOVE_BORDER_TYPE_APP_SELECTOR;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_APP_POPUP:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_APP_POPUP)
           res = E_MOVE_BORDER_TYPE_APP_POPUP;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_SPLIT_LAUNCHER:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_SPLIT_LAUNCHER)
           res = E_MOVE_BORDER_TYPE_SPLIT_LAUNCHER;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_PWLOCK:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_PWLOCK)
           res = E_MOVE_BORDER_TYPE_PWLOCK;
         break;
      case E_MOVE_BORDER_CLASS_TYPE_BACKGROUND:
         if (ntype == E_MOVE_BORDER_NAME_TYPE_BACKGROUND)
           res = E_MOVE_BORDER_TYPE_BACKGROUND;
         break;
      default:
         break;
     }

   if (res == E_MOVE_BORDER_TYPE_UNKNOWN)
     res = _ecore_type_to_e_move_type(wtype);

   if (mb->bd->internal)
     {
        if (name) E_FREE(name);
        if (clas) E_FREE(clas);
     }

   mb->type = res;
   return EINA_TRUE;
}

EINTERN E_Move_Border_Type
e_mod_move_border_type_get(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, E_MOVE_BORDER_TYPE_UNKNOWN);
   return mb->type;
}

EINTERN Eina_Bool
e_mod_move_border_type_handler_prop(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Border *mb;
   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   L(LT_EVENT_X, "[MOVE] %31s\n", "PROP_WM_CLASS");

   mb = e_mod_move_border_client_find(ev->win);
   if (mb) return EINA_TRUE;

   e_mod_move_border_type_setup(mb);
   return EINA_TRUE;
}

EINTERN const char *
e_mod_move_border_types_name_get(E_Move_Border_Type t)
{
   char type_string[10];
   memset(type_string, 0, sizeof(type_string));
   snprintf(type_string, sizeof(type_string), "%d",t);
   return eina_hash_find(types_hash,
                         type_string);
}

