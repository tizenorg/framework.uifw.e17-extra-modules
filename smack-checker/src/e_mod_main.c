#include "e.h"
#include "e_mod_main.h"
#include "security-server.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17_EXTRA_MODULES"

static int _e_smack_checker_init(void);
static void _e_smack_checker_fin(void);

static Eina_Bool _e_smack_checker_atom_init(void);
static void _e_smack_checker_handlers_add(void);
static Eina_Bool _e_smack_checker_notification_level_check(Ecore_X_Window win);
static Eina_Bool _e_smack_checker_cb_window_property(void *data, int type, void *event);
static int _e_smack_checker_pid_get(Ecore_X_Window win);
static int _e_smack_checker_notification_level_privilege_check(int pid);

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Smack rule check module"
};

#define SM_LABEL_NOTIFICATION_LEVEL "e17::notification"
#define SM_RIGHT_NOTIFICATION_LEVEL "w"

static Eina_List *_e_smack_handlers;
static Ecore_X_Atom E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL;
static Ecore_X_Atom E_SM_CHECKER_ATOM_XCLIENT_PID;
static Ecore_X_Atom E_SM_CHECKER_ATOM_ACCESS_RESULT;

EAPI void*
e_modapi_init(E_Module *m)
{
   if (!_e_smack_checker_init())
     {
        return NULL;
     }

   _e_smack_checker_atom_init();
   _e_smack_checker_handlers_add();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   _e_smack_checker_fin();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   /* Do Something */
   return 1;
}

static int
_e_smack_checker_init(void)
{
   int ret = 1;
   return ret;
}

static void
_e_smack_checker_fin(void)
{
}

static Eina_Bool
_e_smack_checker_atom_init(void)
{
   E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL = ecore_x_atom_get("_E_ILLUME_NOTIFICATION_LEVEL");
   if (!E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL)
     {
        return EINA_FALSE;
     }

   E_SM_CHECKER_ATOM_XCLIENT_PID = ecore_x_atom_get("X_CLIENT_PID");
   if (!E_SM_CHECKER_ATOM_XCLIENT_PID)
     {
        return EINA_FALSE;
     }

   E_SM_CHECKER_ATOM_ACCESS_RESULT = ecore_x_atom_get("_E_NOTIFICATION_LEVEL_ACCESS_RESULT");
   if (!E_SM_CHECKER_ATOM_ACCESS_RESULT)
     {
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_e_smack_checker_handlers_add(void)
{
   _e_smack_handlers =
      eina_list_append(_e_smack_handlers,
                       ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                               _e_smack_checker_cb_window_property,
                                               NULL));
}

static Eina_Bool
_e_smack_checker_notification_level_check(Ecore_X_Window win)
{
   Eina_Bool exist;
   int ret;
   int num;
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get(win, E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &num);
   if (ret) exist = EINA_TRUE;
   else exist = EINA_FALSE;

   if (prop_data) free(prop_data);

   return exist;
}


static Eina_Bool
_e_smack_checker_cb_window_property(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Property *ev;
   ev = event;

   if (ev->atom == E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL)
     {
        int pid = 0;
        int access = 0;
        Eina_Bool level_exist = EINA_FALSE;

        level_exist = _e_smack_checker_notification_level_check(ev->win);
        if (level_exist)
          {
             pid = _e_smack_checker_pid_get(ev->win);
             if (pid > 0)
               {
                  access = _e_smack_checker_notification_level_privilege_check(pid);
                  if (!access)
                    {
                       SLOGI("NOTIFICATION LEVEL Denied.. win:%x(pid:%d)", ev->win, pid);
                       ecore_x_client_message32_send(ev->win,
                                                     E_SM_CHECKER_ATOM_ACCESS_RESULT,
                                                     ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                     0, access, 0, 0, 0);
                       ecore_x_window_prop_property_del(ev->win, E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL);
                       return ECORE_CALLBACK_CANCEL;
                    }
               }
             else
               {
                  SLOGI("NOTIFICATION LEVEL Denied.. win:%x(pid:%d), PID error.", ev->win, pid);
                  ecore_x_window_prop_property_del(ev->win, E_SM_CHECKER_ATOM_NOTIFICATION_LEVEL);
                  return ECORE_CALLBACK_CANCEL;
               }
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static int
_e_smack_checker_pid_get(Ecore_X_Window win)
{
   int pid = 0;
   Eina_Bool ret = EINA_FALSE;

   if (!win) return 0;

   ret = ecore_x_netwm_pid_get(win, &pid);
   if (!ret)
     {
        ecore_x_window_prop_card32_get(win,
                                       E_SM_CHECKER_ATOM_XCLIENT_PID,
                                       &pid,
                                       1);
     }

   return pid;
}

static int
_e_smack_checker_notification_level_privilege_check(int pid)
{
   int access = 0;
   int ret = 0;

   if (pid <= 0) return ret;

   access = security_server_check_privilege_by_pid(pid,
                                                   SM_LABEL_NOTIFICATION_LEVEL,
                                                   SM_RIGHT_NOTIFICATION_LEVEL);

   if (access == SECURITY_SERVER_API_SUCCESS)
     ret = 1;

   return ret;
}

