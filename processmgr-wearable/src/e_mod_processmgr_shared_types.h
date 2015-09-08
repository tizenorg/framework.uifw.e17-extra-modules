#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_PROCESSMGR_SHARED_TYPES_H
#define E_MOD_PROCESSMGR_SHARED_TYPES_H


typedef struct _E_ProcessMgr  E_ProcessMgr;
typedef struct _E_ProcessInfo E_ProcessInfo;
typedef struct _E_WindowInfo  E_WindowInfo;

#include "e.h"
#include "e_mod_main.h"
#include "e_mod_processmgr_util.h"

EINTERN extern Ecore_X_Atom ATOM_CM_LOG; 

typedef enum _E_ProcessMgr_Action
{
   PROCESS_LAUNCH = 0,
   PROCESS_RESUME = 1,
   PROCESS_TERMINATE = 2,
   PROCESS_FOREGROUND = 3,
   PROCESS_BACKGROUND = 4
} E_ProcessMgr_Action;

struct _E_ProcessMgr
{
   E_Manager         *man;
   Eina_Hash         *pids_hash; 
   Eina_Hash         *wins_hash; 
   Eina_Inlist       *pids_list;
   Eina_Inlist       *wins_list;
   Ecore_X_Window event_win; /* ANR event window : gesture driver will send msg to the event_win  that related on touch press/release */
};

struct _E_ProcessInfo
{
   EINA_INLIST;
   E_ProcessMgr *pm;
   int           pid;
   Eina_List    *wins;
   Eina_Bool     freeze; /* process freeze/thaw request value */
   Eina_Bool     launch; /* process launching check, first foreground check */ 
};

struct _E_WindowInfo
{
   EINA_INLIST;
   E_ProcessMgr  *pm;
   Ecore_X_Window win;
   int            pid;
   Eina_Bool      override; /* override rediret */
   Eina_Bool      visibility; /* window visibility nofity */
   Eina_Bool      iconic; /* iconic state */
   Eina_Bool      first_show; /* first show check, it is used for launching check */
   Ecore_X_Time anr_timestamp; /* ANR timestamp : Timestamp for ANR request. */
   Ecore_X_Time anr_ping_timestamp; /* ANR timestamp : Timestamp for ANR ping request. */
   Ecore_Timer *anr_trigger_timer; /* ANR trigger timer : In anr_trigger_timer, it will send ping msg to the pointed window. */
   Ecore_Timer *anr_confirm_timer; /* ANR confirm timer : In anr_confirm_timer, it will check pong msg and send dbus-signal to the resourced. */
};

#endif
#endif
