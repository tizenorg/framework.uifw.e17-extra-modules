#include "e_mod_processmgr.h"
#include "e_mod_processmgr_debug.h"
#include "e_mod_processmgr_dfps.h"

#define LOG_TAG "PROCESSMGR"
#include "dlog.h"

/* global variables */
EINTERN Ecore_X_Atom ATOM_CM_LOG  = 0;

static Ecore_X_Atom ATOM_CHECK_ANR_IN_INPUT_EVENT  = 0; /* ANR : Application Not Response */
static Ecore_X_Atom ATOM_ANR_EVENT_WINDOW  = 0; /* ANR : Application Not Response*/

/* static global variables */
static Eina_List     *handlers       = NULL;
static Eina_List     *_pms           = NULL;
static E_ProcessMgr  *_pm            = NULL;

/* static functions */

static E_ProcessMgr  *_e_mod_processmgr_get(void);
static void           _e_mod_processmgr_set(E_ProcessMgr *pm);
static E_ProcessMgr  *_e_mod_processmgr_add(E_Manager *man);
static void           _e_mod_processmgr_del(E_ProcessMgr *pm);

static E_ProcessInfo *_e_mod_processmgr_processinfo_find(E_ProcessMgr *pm, int pid);
static E_WindowInfo  *_e_mod_processmgr_wininfo_find(E_ProcessMgr *pm, Ecore_X_Window win);

static E_WindowInfo  *_e_mod_processmgr_wininfo_add(Ecore_X_Window win);
static Eina_Bool      _e_mod_processmgr_wininfo_del(Ecore_X_Window win);
static E_ProcessInfo *_e_mod_processmgr_processinfo_add(int pid, Ecore_X_Window win);
static Eina_Bool      _e_mod_processmgr_processinfo_del(int pid, Ecore_X_Window win);

static Eina_Bool      _e_mod_processmgr_win_create(void *data __UNUSED__, int type  __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_win_destroy(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_win_reparent(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_win_property(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__);
static Eina_Bool      _e_mod_processmgr_win_message(void *data __UNUSED__, int   type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_win_visibility_change(void *data __UNUSED__, int   type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_bd_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_bd_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_bd_iconify(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_processmgr_bd_uniconify(void *data __UNUSED__, int type  __UNUSED__, void *event);

static Eina_Bool      _e_mod_processmgr_win_prop_pid(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_processmgr_win_msg_deiconify_approve(Ecore_X_Event_Client_Message *ev);

static void           _e_mod_processmgr_send_pid_action(int pid, E_ProcessMgr_Action act);

static Eina_Bool      _e_mod_processmgr_win_freeze_policy_check(Ecore_X_Window win);
static Eina_Bool      _e_mod_processmgr_win_freeze_state_get(Ecore_X_Window win);
static Eina_Bool      _e_mod_processmgr_win_freeze(Ecore_X_Window win);
static Eina_Bool      _e_mod_processmgr_win_thaw(Ecore_X_Window win);

static Eina_Bool      _e_mod_processmgr_win_launch_check(Ecore_X_Window win);
static Eina_Bool      _e_mod_processmgr_win_launch(Ecore_X_Window win);
//////////////////////////////////////////////////////////////////////////

static E_ProcessMgr*
_e_mod_processmgr_get(void)
{
   return _pm;
}

static void
_e_mod_processmgr_set(E_ProcessMgr* pm)
{
  _pm = pm;
}

static E_ProcessInfo *
_e_mod_processmgr_processinfo_find(E_ProcessMgr *pm, int pid)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, NULL);
   return eina_hash_find(pm->pids_hash, e_util_winid_str_get(pid));
}

static E_WindowInfo *
_e_mod_processmgr_wininfo_find(E_ProcessMgr *pm, Ecore_X_Window win)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, NULL);
   return eina_hash_find(pm->wins_hash, e_util_winid_str_get(win));
}

static E_WindowInfo*
_e_mod_processmgr_wininfo_add(Ecore_X_Window win)
{
   E_ProcessMgr *pm = NULL;
   E_WindowInfo *winfo = NULL;
   int           pid = -1;

   if (!win) return NULL;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, NULL);

   if (_e_mod_processmgr_wininfo_find(pm, win))
     return NULL;

   winfo = E_NEW(E_WindowInfo, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, NULL);

   winfo->pm = pm;
   winfo->win = win;

   if (ecore_x_netwm_pid_get(win, &pid))
     {
        winfo->pid = pid;
        _e_mod_processmgr_processinfo_add(pid, win);
     }

   winfo->first_show = EINA_TRUE;
   eina_hash_add(pm->wins_hash, e_util_winid_str_get(winfo->win), winfo);
   pm->wins_list = eina_inlist_append(pm->wins_list, EINA_INLIST_GET(winfo));

   return winfo;
}

static Eina_Bool
_e_mod_processmgr_wininfo_del(Ecore_X_Window win)
{
   E_WindowInfo *winfo = NULL;
   E_ProcessMgr *pm    = NULL;

   if (!win) return EINA_FALSE;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);

   winfo = _e_mod_processmgr_wininfo_find(pm, win);
   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   _e_mod_processmgr_processinfo_del(winfo->pid, win);

   eina_hash_del(pm->wins_hash, e_util_winid_str_get(win), winfo);
   pm->wins_list = eina_inlist_remove(pm->wins_list, EINA_INLIST_GET(winfo));

   if (winfo->anr_trigger_timer != NULL)
     {
        Ecore_X_Window *old_data=NULL;
        LOGD("[PROCESSMGR] delete anr_trigger_timer!");
        old_data = ecore_timer_del(winfo->anr_trigger_timer);
        if (old_data != NULL ) E_FREE(old_data);
        winfo->anr_trigger_timer = NULL;
     }

   if (winfo->anr_confirm_timer != NULL)
     {
        Ecore_X_Window *old_data=NULL;
        LOGD("[PROCESSMGR] delete anr_confirm_timer!");
        old_data = ecore_timer_del(winfo->anr_confirm_timer);
        if (old_data != NULL ) E_FREE(old_data);
        winfo->anr_confirm_timer = NULL;
     }

   E_FREE(winfo);

   return EINA_TRUE;
}

static E_ProcessInfo *
_e_mod_processmgr_processinfo_add(int pid, Ecore_X_Window win)
{
   E_ProcessInfo  *pinfo = NULL;
   E_ProcessMgr   *pm    = NULL;
   Ecore_X_Window *_win  = NULL;

   if (!pid) return NULL;
   if (!win) return NULL;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, NULL);

   pinfo = _e_mod_processmgr_processinfo_find(pm, pid);

   if (!pinfo)
     {
        pinfo = E_NEW(E_ProcessInfo, 1);
        EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, NULL);
        pinfo->pm = pm;
        pinfo->pid = pid;
        pinfo->launch = EINA_TRUE;
        _win = E_NEW(Ecore_X_Window, 1);
        if (!_win)
          {
              E_FREE(pinfo);
              return NULL;
          }
        *_win = win;
        pinfo->wins = eina_list_append(pinfo->wins, _win);

        eina_hash_add(pm->pids_hash, e_util_winid_str_get(pinfo->pid), pinfo);
        pm->pids_list = eina_inlist_append(pm->pids_list, EINA_INLIST_GET(pinfo));
     }
   else
     {
        _win = E_NEW(Ecore_X_Window, 1);
        EINA_SAFETY_ON_NULL_RETURN_VAL(_win, NULL);
        *_win = win;
        pinfo->wins = eina_list_append(pinfo->wins, _win);
     }

   return pinfo;
}

static Eina_Bool
_e_mod_processmgr_processinfo_del(int pid, Ecore_X_Window win)
{
   E_ProcessInfo  *pinfo = NULL;
   E_ProcessMgr   *pm    = NULL;
   Ecore_X_Window *_win  = NULL;
   Eina_List      *l;
   Eina_Bool       found = EINA_FALSE;

   if (!pid) return EINA_FALSE;
   if (!win) return EINA_FALSE;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);

   pinfo = _e_mod_processmgr_processinfo_find(pm, pid);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   EINA_LIST_FOREACH(pinfo->wins, l, _win)
     {
        if (*_win == win)
          {
             found = EINA_TRUE;
             break;
          }
     }

   if (found)
     {
         pinfo->wins = eina_list_remove(pinfo->wins, _win);
         E_FREE(_win);
     }

   if (!(pinfo->wins)) // windows list is null
   //if (eina_list_count(pinfo->wins) == 0) // windows list is null
     {
        eina_hash_del(pm->pids_hash, e_util_winid_str_get(pid), pinfo);
        pm->pids_list = eina_inlist_remove(pm->pids_list, EINA_INLIST_GET(pinfo));
        E_FREE(pinfo);
     }

   return EINA_TRUE;
}


static Eina_Bool
_e_mod_processmgr_win_create(void *data __UNUSED__,
                             int type   __UNUSED__,
                             void      *event)
{
   Ecore_X_Event_Window_Create *ev = event;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_win_destroy(void *data __UNUSED__,
                              int type   __UNUSED__,
                              void      *event)
{
   Ecore_X_Event_Window_Destroy *ev = event;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_win_reparent(void *data __UNUSED__,
                               int   type __UNUSED__,
                               void *event)
{
   //Ecore_X_Event_Window_Reparent *ev = event;
   // process window reparent
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_processmgr_dbus_msg_send(E_Border *bd,
                                int       command)
{
   E_ProcessMgr *pm = NULL;
   DBusConnection *conn;
   DBusMessage* msg;
   Eina_Bool sent;
   dbus_bool_t ret;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN(pm);

   conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
   if (!conn) return;

   LOGE("[PROCESSMGR] pointed_win=%#x Send kill command to the ResourceD! PID=%d Name=%s\n", bd->client.win, bd->client.netwm.pid,bd->client.icccm.name);
   msg = dbus_message_new_signal("/Org/Tizen/ResourceD/Process",
                                 "org.tizen.resourced.process",
                                 "ProcWatchdog");
   if (!msg) return;

   if (!dbus_message_append_args(msg,
                                 DBUS_TYPE_INT32, &bd->client.netwm.pid,
                                 DBUS_TYPE_INT32, &command,
                                 DBUS_TYPE_INVALID))
     {
        dbus_message_unref(msg);
        return;
     }
   // send the message
   if (!dbus_connection_send (conn, msg, NULL))
     {
        dbus_message_unref(msg);
        return;
     }
   // cleanup
   dbus_message_unref(msg);
}

static Eina_Bool
_e_mod_processmgr_anr_ping_confirm_handler(void *data)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_Border *bd=NULL;
   Ecore_X_Window *pointed_win;
   Ecore_X_Window root;
   Ecore_X_Window rwin;
   Ecore_X_Window cwin;
   int x, y, win_x, win_y;
   unsigned int mask;

   pointed_win = (Ecore_X_Window *)data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(pointed_win , ECORE_CALLBACK_CANCEL);

   if (*pointed_win)
      bd = e_border_find_by_window(*pointed_win);
   EINA_SAFETY_ON_NULL_GOTO(bd , finish);

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_GOTO(pm, finish);

   winfo = _e_mod_processmgr_wininfo_find(pm, bd->client.win);
   EINA_SAFETY_ON_NULL_GOTO(winfo, finish);

   winfo->anr_confirm_timer = NULL;

   LOGD("[PROCESSMGR] last_pointed_win=%#x bd->visible=%d\n", *pointed_win, bd->visible);

   if (bd->ping_ok == 0)
     {
        // Add more strict condition.
        root = bd->zone->container->manager->root;
        XQueryPointer(ecore_x_display_get(), root,
                      &rwin, &cwin, &x, &y,
                      &win_x, &win_y, &mask);
        // In this case, we may assume that bd->client.win(pointed_win) is blocked.
        LOGD("[PROCESSMGR] pointed_win=%#x cwin=%#x \n", *pointed_win, cwin);
        if (cwin == *pointed_win && bd->visible)
          {
             LOGD("[PROCESSMGR] pointed_win=%#x is not response!\n", *pointed_win);
             _e_mod_processmgr_dbus_msg_send(bd, SIGTERM);
          }
     }

finish:
   // pointed_win is allocated by E_NEW() in _e_mod_processmgr_anr_ping().
   E_FREE(pointed_win);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_processmgr_anr_ping_begin_handler(void *data)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_Border *bd=NULL;
   Ecore_X_Window *pointed_win;
   Ecore_X_Window root;
   Ecore_X_Window rwin;
   Ecore_X_Window cwin;
   int x, y, win_x, win_y;
   unsigned int mask;

   pointed_win = (Ecore_X_Window *)data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(pointed_win , ECORE_CALLBACK_CANCEL);

   if (*pointed_win)
      bd = e_border_find_by_window(*pointed_win);
   EINA_SAFETY_ON_NULL_GOTO(bd , finish);

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_GOTO(pm, finish);

   winfo = _e_mod_processmgr_wininfo_find(pm, bd->client.win);
   EINA_SAFETY_ON_NULL_GOTO(winfo, finish);

   winfo->anr_trigger_timer = NULL;

   //1 1. Check ev->time and anr_timestamp.
   if (winfo->anr_ping_timestamp && (abs(winfo->anr_timestamp - winfo->anr_ping_timestamp) < _processmgr_mod->conf->tizen_anr_ping_interval_timeout))
     {
        goto finish;
     }

   // Need to check cwin and current pointed window.
   // If that is different, then cwin is destroyed/hidden state. So we don't need to use ping-pong.
   root = bd->zone->container->manager->root;
   XQueryPointer(ecore_x_display_get(), root,
                 &rwin, &cwin, &x, &y,
                 &win_x, &win_y, &mask);

   if (cwin == *pointed_win && bd->visible)
     {
        if (winfo->anr_confirm_timer != NULL)
          {
             Ecore_X_Window *old_data=NULL;
             old_data = ecore_timer_del(winfo->anr_confirm_timer);
             if (old_data != NULL ) E_FREE(old_data);
             winfo->anr_confirm_timer = NULL;
          }

        // bd->ping_ok is used in e_border_ping().
        // But if we disable e_config->ping_clients in e.src, then we can reuse it.
        bd->ping_ok = 0;
        LOGD("[PROCESSMGR] ecore_x_netwm_ping_send to the client_win=%#x\n", bd->client.win);
        ecore_x_netwm_ping_send(bd->client.win);
        winfo->anr_ping_timestamp = winfo->anr_timestamp;

        winfo->anr_confirm_timer = ecore_timer_add(_processmgr_mod->conf->tizen_anr_confirm_timeout, _e_mod_processmgr_anr_ping_confirm_handler, pointed_win);
     }
   else
     {
        // pointed_win is allocated by E_NEW() in _e_mod_processmgr_anr_ping().
        E_FREE(pointed_win);
     }

   return ECORE_CALLBACK_CANCEL;

finish:
   E_FREE(pointed_win);

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_processmgr_anr_ping(Ecore_X_Event_Window_Property *ev)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_Border *bd=NULL;
   Ecore_X_Window root;
   Ecore_X_Window rwin;
   Ecore_X_Window *cwin=NULL;
   int x, y, win_x, win_y;
   unsigned int mask;

   //1 1. Get pointer window.
   cwin = E_NEW(Ecore_X_Window, 1);
   EINA_SAFETY_ON_NULL_RETURN(cwin);

   XQueryPointer(ecore_x_display_get(), ecore_x_window_root_first_get(),
                 &rwin, cwin, &x, &y,
                 &win_x, &win_y, &mask);

   if (*cwin)
     bd = e_border_find_by_window(*cwin);

   EINA_SAFETY_ON_NULL_GOTO(bd , finish);

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_GOTO(pm, finish);

   winfo = _e_mod_processmgr_wininfo_find(pm, bd->client.win);
   EINA_SAFETY_ON_NULL_GOTO(winfo, finish);

   //1 2. Remove old timer and register new timer callback.
   if (winfo->anr_trigger_timer != NULL)
     {
        Ecore_X_Window *old_data=NULL;

        old_data = ecore_timer_del(winfo->anr_trigger_timer);
        if (old_data != NULL ) E_FREE(old_data);
        winfo->anr_trigger_timer = NULL;
     }

   LOGD("[PROCESSMGR] ev_win=%#x  register trigger_timer!  pointed_win=%#x \n", ev->win, *cwin);

   // At this point, if user try to press/release quickly, then e17 deliver ping event repeatly.
   // To prevent this issue, I just use timer.
   winfo->anr_trigger_timer = ecore_timer_add(_processmgr_mod->conf->tizen_anr_trigger_timeout, _e_mod_processmgr_anr_ping_begin_handler, cwin);
   winfo->anr_timestamp = ev->time;

   return ;

finish:
   if (cwin) E_FREE(cwin);
}

static Eina_Bool
_e_mod_processmgr_win_property(void *data  __UNUSED__,
                               int type    __UNUSED__,
                               void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;
   Ecore_X_Atom a = 0;
   if (!ev) return ECORE_CALLBACK_PASS_ON;
   if (!ev->atom) return ECORE_CALLBACK_PASS_ON;
   a = ev->atom;
#if 1
   if (a == ATOM_CM_LOG)
     e_mod_processmgr_debug_prop_handle(ev);
   else if (a == ECORE_X_ATOM_NET_WM_PID)
     _e_mod_processmgr_win_prop_pid(ev);
   else if (a == ATOM_CHECK_ANR_IN_INPUT_EVENT)
     _e_mod_processmgr_anr_ping(ev);
#else
   if (a == ECORE_X_ATOM_NET_WM_PID)
     _e_mod_processmgr_win_prop_pid(ev);
#endif

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_win_message(void *data __UNUSED__,
                              int type   __UNUSED__,
                              void      *event)
{
   Ecore_X_Event_Client_Message *ev;
   Ecore_X_Atom t;

   // process client message
   ev = (Ecore_X_Event_Client_Message *)event;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, ECORE_CALLBACK_PASS_ON);
   if(!(ev->format == 32)) return ECORE_CALLBACK_PASS_ON;

   t = ev->message_type;

   if (t == ECORE_X_ATOM_E_DEICONIFY_APPROVE)
     {
        if ( _e_mod_processmgr_win_msg_deiconify_approve(ev) )
        {
            _e_mod_processmgr_dfps_process_state_change(DFPS_RESUME_STATE, ev->win);
        }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_win_visibility_change(void *data __UNUSED__,
                                        int type   __UNUSED__,
                                        void      *event)
{
   int fully_obscured;
   Ecore_X_Event_Window_Visibility_Change *ev;

   // process window visibilty changed
   ev = (Ecore_X_Event_Window_Visibility_Change *)event;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, ECORE_CALLBACK_PASS_ON);

   fully_obscured = ev->fully_obscured;

   if (fully_obscured)
     {
        ;// fully_obscured case
        _e_mod_processmgr_dfps_process_state_change(DFPS_PAUSE_STATE, ev->win);
     }
   else
     {
        // visible case
        if ((!_e_mod_processmgr_win_launch(ev->win)) &&
            (_e_mod_processmgr_win_freeze_state_get(ev->win)))
          {
             _e_mod_processmgr_win_thaw(ev->win);
          }

        _e_mod_processmgr_dfps_process_state_change(DFPS_RESUME_STATE, ev->win);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_bd_add(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void      *event)
{
   E_Event_Border_Add *ev = event;
   E_Border           *bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev->border, ECORE_CALLBACK_PASS_ON);

   // add function _e_mod_processmgr_bd_add_intern(bd);
   // internal function adds E_ProcessInfo, E_WindowInfo datas
   // control processinfo, windowinfo hash, eina_list
   bd = ev->border;
   _e_mod_processmgr_wininfo_add(bd->client.win);

   // process window create
   _e_mod_processmgr_dfps_process_state_change(DFPS_LAUNCH_STATE, bd->client.win);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_bd_del(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void      *event)
{
   E_Event_Border_Remove *ev = event;
   E_Border              *bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev->border, ECORE_CALLBACK_PASS_ON);

   // add function _e_mod_processmgr_bd_del(mb, ev->border);
   // internal function removes E_ProcessInfo, E_WindowInfo datas
   // control processinfo, windowinfo hash, eina_list
   bd = ev->border;
   _e_mod_processmgr_wininfo_del(bd->client.win);

   // process window destroy
    _e_mod_processmgr_dfps_process_state_change(DFPS_TERMINATED_STATE, bd->client.win);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_bd_iconify(void *data __UNUSED__,
                             int type   __UNUSED__,
                             void      *event)
{
   E_Event_Border_Iconify *ev = event;
   E_Border               *bd = NULL;
   // process border iconify
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev->border, ECORE_CALLBACK_PASS_ON);
   bd = ev->border;

   if ( _e_mod_processmgr_win_freeze_policy_check(bd->client.win))
   {
       _e_mod_processmgr_win_freeze(bd->client.win);

       _e_mod_processmgr_dfps_process_state_change(DFPS_PAUSE_STATE, bd->win);
   }

   return ECORE_CALLBACK_PASS_ON;
}


static Eina_Bool
_e_mod_processmgr_bd_uniconify(void *data __UNUSED__,
                               int type   __UNUSED__,
                               void      *event)
{
   E_Event_Border_Uniconify *ev = event;
   E_Border                 *bd = NULL;

   // process border uniconify
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev->border, ECORE_CALLBACK_PASS_ON);

   bd = ev->border;

   if (_e_mod_processmgr_win_freeze_state_get(bd->client.win))
      _e_mod_processmgr_win_thaw(bd->client.win);

   _e_mod_processmgr_dfps_process_state_change(DFPS_RESUME_STATE, bd->win);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_processmgr_win_prop_pid(Ecore_X_Event_Window_Property *ev)
{
   E_ProcessMgr  *pm    = NULL;
   E_WindowInfo  *winfo = NULL;
   Ecore_X_Window win   = 0;
   int            pid   = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, EINA_FALSE);
   win = ev->win;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);

   winfo = _e_mod_processmgr_wininfo_find(pm, win);
   E_CHECK_RETURN(winfo, EINA_FALSE);

   if (ecore_x_netwm_pid_get(win, &pid))
     {
        // pid property change case
        if (winfo->pid != pid)
          {
             _e_mod_processmgr_processinfo_del(winfo->pid, win);
             _e_mod_processmgr_processinfo_add(pid, win);
             winfo->pid = pid;
          }
     }
   else
     {
        // property delete case
        _e_mod_processmgr_processinfo_del(winfo->pid, win);
        winfo->pid = 0;
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_processmgr_win_msg_deiconify_approve(Ecore_X_Event_Client_Message *ev)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, EINA_FALSE);

   if (_e_mod_processmgr_win_freeze_state_get(ev->win))
     {
        _e_mod_processmgr_win_thaw(ev->win);
     }
   return EINA_TRUE;
}

void
_e_mod_processmgr_send_pid_action(int pid, E_ProcessMgr_Action act)
{
   E_ProcessMgr *pm = NULL;
   DBusConnection *conn;
   DBusMessage* msg;
   int param_pid;
   int param_act;

   pm = _e_mod_processmgr_get();
   EINA_SAFETY_ON_NULL_RETURN(pm);

   conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
   if (!conn) return;

   // set up msg for resourced
   msg = dbus_message_new_signal("/Org/Tizen/ResourceD/Process",
                                 "org.tizen.resourced.process",
                                 "ProcStatus");
   if (!msg) return;
   // append the action to do and the pid to do it to
   param_pid = (int)pid;
   param_act = (int)act;
   if (!dbus_message_append_args(msg,
                                 DBUS_TYPE_INT32, &param_act,
                                 DBUS_TYPE_INT32, &param_pid,
                                 DBUS_TYPE_INVALID))
     {
        dbus_message_unref(msg);
        return;
     }
   // send the message
   if (!dbus_connection_send(conn, msg, NULL))
     {
        dbus_message_unref(msg);
        return;
     }
   // cleanup
   dbus_message_unref(msg);
}

static Eina_Bool
_e_mod_processmgr_win_freeze_policy_check(Ecore_X_Window win)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_ProcessInfo  *pinfo  = NULL;
   Ecore_X_Window *_win   = NULL;
   E_Border       *bd     = NULL;
   Eina_Bool       freeze = EINA_TRUE;
   Eina_List      *l;

   if (!win) return EINA_FALSE;
   pm = _e_mod_processmgr_get();

   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);
   winfo = _e_mod_processmgr_wininfo_find(pm, win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   if (!winfo->pid) return EINA_FALSE;

   pinfo = _e_mod_processmgr_processinfo_find(pm, winfo->pid);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   if (pinfo->freeze) return EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo->wins, EINA_FALSE);

   EINA_LIST_FOREACH(pinfo->wins, l, _win)
     {
        bd = e_border_find_all_by_client_window(*_win);
        EINA_SAFETY_ON_NULL_RETURN_VAL(bd, EINA_FALSE);

        if (!bd->iconic)
          {
             freeze = EINA_FALSE;
             break;
          }
     }

   return freeze;
}

static Eina_Bool
_e_mod_processmgr_win_freeze_state_get(Ecore_X_Window win)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_ProcessInfo  *pinfo  = NULL;

   if (!win) return EINA_FALSE;
   pm = _e_mod_processmgr_get();

   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);
   winfo = _e_mod_processmgr_wininfo_find(pm, win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   if (!winfo->pid) return EINA_FALSE;

   pinfo = _e_mod_processmgr_processinfo_find(pm, winfo->pid);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   return pinfo->freeze;
}

static Eina_Bool
_e_mod_processmgr_win_freeze(Ecore_X_Window win)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_ProcessInfo  *pinfo  = NULL;

   if (!win) return EINA_FALSE;
   pm = _e_mod_processmgr_get();

   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);
   winfo = _e_mod_processmgr_wininfo_find(pm, win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   if (!winfo->pid) return EINA_FALSE;

   pinfo = _e_mod_processmgr_processinfo_find(pm, winfo->pid);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   if (pinfo->freeze) return EINA_FALSE;

   pinfo->freeze = EINA_TRUE;
   _e_mod_processmgr_send_pid_action(winfo->pid, PROCESS_BACKGROUND);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_processmgr_win_thaw(Ecore_X_Window win)
{
   E_ProcessMgr   *pm     = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_ProcessInfo  *pinfo  = NULL;

   if (!win) return EINA_FALSE;
   pm = _e_mod_processmgr_get();

   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);
   winfo = _e_mod_processmgr_wininfo_find(pm, win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   if (!winfo->pid) return EINA_FALSE;

   pinfo = _e_mod_processmgr_processinfo_find(pm, winfo->pid);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   if (!pinfo->freeze) return EINA_FALSE;

   pinfo->freeze = EINA_FALSE;
   _e_mod_processmgr_send_pid_action(winfo->pid, PROCESS_FOREGROUND);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_processmgr_win_launch(Ecore_X_Window win)
{
   E_ProcessMgr  *pm     = NULL;
   E_WindowInfo  *winfo  = NULL;
   E_ProcessInfo *pinfo  = NULL;
   Eina_Bool      launch = EINA_FALSE;

   if (!win) return EINA_FALSE;

   launch = _e_mod_processmgr_win_launch_check(win);
   if (!launch) return EINA_FALSE;

   pm = _e_mod_processmgr_get();

   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);
   winfo = _e_mod_processmgr_wininfo_find(pm, win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   winfo->first_show = EINA_FALSE;


   if (!winfo->pid) return EINA_FALSE;

   pinfo = _e_mod_processmgr_processinfo_find(pm, winfo->pid);

   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   pinfo->launch = EINA_FALSE;

   // window lauch process do not check pinfo->freeze
   _e_mod_processmgr_send_pid_action(winfo->pid, PROCESS_FOREGROUND);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_processmgr_win_launch_check(Ecore_X_Window win)
{
   E_ProcessMgr  *pm     = NULL;
   E_WindowInfo  *winfo  = NULL;
   E_ProcessInfo *pinfo  = NULL;
   Eina_Bool      launch = EINA_FALSE;

   if (!win) return EINA_FALSE;
   pm = _e_mod_processmgr_get();

   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, EINA_FALSE);
   winfo = _e_mod_processmgr_wininfo_find(pm, win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(winfo, EINA_FALSE);

   if (!winfo->first_show) return EINA_FALSE;

   if (!winfo->pid) return EINA_FALSE;

   pinfo = _e_mod_processmgr_processinfo_find(pm, winfo->pid);

   EINA_SAFETY_ON_NULL_RETURN_VAL(pinfo, EINA_FALSE);

   if (!pinfo->launch) return EINA_FALSE;

   launch = EINA_TRUE;
   return launch;
}

static E_ProcessMgr *
_e_mod_processmgr_add(E_Manager *man)
{
   E_ProcessMgr   *pm;
   Ecore_X_Window *wins;
   E_Border       *bd;
   int             i, num;

   pm = E_NEW(E_ProcessMgr, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pm, NULL);

   pm->man = man;

   pm->pids_hash = eina_hash_string_superfast_new(NULL);
   pm->wins_hash = eina_hash_string_superfast_new(NULL);

   wins = ecore_x_window_children_get(man->root, &num);

   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             /* Add WindowInfos, ProcessInfos */
             if ((bd = e_border_find_by_window(wins[i])))
               {
                  _e_mod_processmgr_wininfo_add(bd->client.win);
               }
          }
        free(wins);
     }

   return pm;
}

static void
_e_mod_processmgr_del(E_ProcessMgr *pm)
{
   E_WindowInfo   *winfo = NULL;
   E_ProcessInfo  *pinfo = NULL;
   Ecore_X_Window *_win  = NULL;

   E_CHECK(pm);

   while (pm->wins_list)
     {
        winfo = (E_WindowInfo *)(pm->wins_list);
        eina_hash_del(pm->wins_hash, e_util_winid_str_get(winfo->win), winfo);
        pm->wins_list = eina_inlist_remove(pm->wins_list, EINA_INLIST_GET(winfo));
        E_FREE(winfo);
     }

   while (pm->pids_list)
     {
        pinfo = (E_ProcessInfo *)(pm->pids_list);
        eina_hash_del(pm->pids_hash, e_util_winid_str_get(pinfo->pid), pinfo);
        pm->pids_list = eina_inlist_remove(pm->pids_list, EINA_INLIST_GET(pinfo));

        EINA_LIST_FREE(pinfo->wins, _win) E_FREE(_win);

        E_FREE(pinfo);
     }

   if (pm->pids_hash)
     {
        eina_hash_free(pm->pids_hash);
        pm->pids_hash = NULL;
     }

   if (pm->wins_hash)
     {
        eina_hash_free(pm->wins_hash);
        pm->wins_hash = NULL;
     }

   if (pm->event_win)
     {
        ecore_x_window_free(pm->event_win);
        pm->event_win = NULL;
     }

   e_mod_processmgr_util_set(NULL, pm->man);

   _e_mod_processmgr_set(NULL);

   E_FREE(pm);
}

/* wrapper function for external file */

Eina_Bool
e_mod_processmgr_init(void)
{
   Eina_List *l;
   E_Manager *man;

   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,            _e_mod_processmgr_win_create,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,           _e_mod_processmgr_win_destroy,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT,          _e_mod_processmgr_win_reparent,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,          _e_mod_processmgr_win_property,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,           _e_mod_processmgr_win_message,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE, _e_mod_processmgr_win_visibility_change, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ADD,                     _e_mod_processmgr_bd_add,                NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE,                  _e_mod_processmgr_bd_del,                NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ICONIFY,                 _e_mod_processmgr_bd_iconify,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY,               _e_mod_processmgr_bd_uniconify,          NULL));

   e_dbus_init();

   ATOM_CM_LOG = ecore_x_atom_get ("_E_COMP_LOG");

   ATOM_CHECK_ANR_IN_INPUT_EVENT = ecore_x_atom_get("_CHECK_APPLICATION_NOT_RESPONSE_IN_INPUT_EVENT_");
   ATOM_ANR_EVENT_WINDOW = ecore_x_atom_get("_ANR_EVENT_WINDOW_");

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_ProcessMgr *pm;
        if (!man) continue;
        pm = _e_mod_processmgr_add(man);
        if (pm)
          {
             _pms = eina_list_append(_pms, pm);
             _e_mod_processmgr_set(pm);
             e_mod_processmgr_util_set(pm, man);
          }
     }

   E_ProcessMgr*pm;
   pm = _e_mod_processmgr_get();
   pm->event_win = ecore_x_window_input_new(ecore_x_window_root_first_get(), -1, -1, 1, 1);
   ecore_x_window_prop_property_set(ecore_x_window_root_first_get(), ATOM_ANR_EVENT_WINDOW, ECORE_X_ATOM_WINDOW, 32, &pm->event_win, 1);

   return 1;
}

void
e_mod_processmgr_shutdown(void)
{
   E_ProcessMgr *pm;

   EINA_LIST_FREE(_pms, pm) _e_mod_processmgr_del(pm);

   E_FREE_LIST(handlers, ecore_event_handler_del);

}

/////////////////////////////////////////////////////////////////////////
