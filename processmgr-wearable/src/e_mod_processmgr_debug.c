#include "e_mod_processmgr_shared_types.h"
#include "e_mod_processmgr.h"
#include "e_mod_processmgr_debug.h"

typedef struct _E_Mod_ProcessMgr_Log_Info
{
   int  type;
   char file[256];
} E_Mod_ProcessMgr_Log_Info;

/* externally accessible globals */
EINTERN int logtype = LT_NOTHING;

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_processmgr_debug_info_dump(Eina_Bool   to_file,
                                 const char *name)
{
   E_ProcessMgr   *pm;
   Eina_List      *l;
   FILE           *fs     = stderr;
   char           *f_name = NULL;
   E_WindowInfo   *winfo  = NULL;
   E_ProcessInfo  *pinfo  = NULL;
   Ecore_X_Window *_win   = NULL;
   int             i      = 1;
   E_Border       *bd     = NULL;

   pm = e_mod_processmgr_util_get();
   E_CHECK_RETURN(pm, 0);

   if ((to_file) && (name))
     {
        f_name = E_NEW(char, strlen(name) + sizeof("_processmgr") + 1);
        memcpy(f_name, name, strlen(name));
        strncat(f_name, "_processmgr", sizeof("_processmgr"));
        if ((fs = fopen(f_name, "r+")) == NULL)
          fs = fopen(f_name, "w+");
        if (!fs)
          {
             fprintf(stderr, "can't open %s file.\n", f_name);
             fs = stderr;
             to_file = EINA_FALSE;
          }
        E_FREE(f_name);
     }
/////////////////////////////////////////////////////////////////////////////
   /* do print debug info */
   fprintf(fs, "\n\nB------------------------------------\n");
   fprintf(fs, "    <ProcessManager MODULE> Window INFO  \n");
   fprintf(fs, "-----------------------------------------\n");
   fprintf(fs, "  NO   BORDER  CLIENT_WIN   | PID | \n");
   fprintf(fs, "-----------------------------------------\n");
   EINA_INLIST_REVERSE_FOREACH(pm->wins_list, winfo)
     {
        if (!winfo->win) continue;
        bd = e_border_find_all_by_client_window(winfo->win);
        if (!bd) continue;
        fprintf(fs,
                " %3d 0x%07x 0x%07x | %4d |\n",
                i,
                bd->win,
                winfo->win,
                winfo->pid);
        i++;
     }

   i = 1;
   fprintf(fs, "E----------------------------------------\n");
   fprintf(fs, "\n\nB------------------------------------\n");
   fprintf(fs, "    <ProcessManager MODULE> PID INFO  \n");
   fprintf(fs, "-----------------------------------------\n");
   fprintf(fs, "  NO  PID | Freeze  | Windows | \n");
   fprintf(fs, "-----------------------------------------\n");
   EINA_INLIST_REVERSE_FOREACH(pm->pids_list, pinfo)
     {
        if (!pinfo->pid) continue;
        fprintf(fs,
                " %3d  %4d | %8s | ",
                i,
                pinfo->pid,
                (pinfo->freeze ? "Freeze" :"Thaw"));

        EINA_LIST_FOREACH(pinfo->wins, l, _win)
          {
             fprintf(fs, "0x%07x ", *_win);
          }
        fprintf(fs, "\n");

        i++;
     }
   fprintf(fs, "E----------------------------------------\n");
/////////////////////////////////////////////////////////////////////////////

   if (to_file)
     {
        fflush(fs);
        fclose(fs);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_processmgr_debug_prop_handle(Ecore_X_Event_Window_Property *ev)
{
   Eina_Bool res = EINA_FALSE;
   E_Mod_ProcessMgr_Log_Info info = {LT_NOTHING, {0,}};
   unsigned char* data = NULL;
   int ret, n;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);
#if 1
   ret = ecore_x_window_prop_property_get
           (ev->win, ATOM_CM_LOG, ECORE_X_ATOM_CARDINAL,
           32, &data, &n);
   E_CHECK_GOTO((ret != -1), cleanup);
   E_CHECK_GOTO(((ret > 0) && (data)), cleanup);

   memcpy(&info, data, sizeof(E_Mod_ProcessMgr_Log_Info));
   logtype = info.type;

   fprintf(stdout, "[ProcessMgr] log-type:0x%08x\n", logtype);

   if (logtype == LT_NOTHING)
     {
       ;
     }
   else if (logtype == (LT_CREATE | LT_INFO_SHOW))
     {
       e_mod_processmgr_debug_info_dump(EINA_FALSE, NULL);
     }
   else if ((logtype == LT_DUMP) &&
       (strlen(info.file) > 0))
     {
        e_mod_processmgr_debug_info_dump(EINA_TRUE, info.file);
#if 0
        ecore_x_client_message32_send
          (ev->win, ATOM_MV_LOG_DUMP_DONE,
          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
          0, 0, 0, 0, 0);
#endif
     }

   if (logtype == LT_INFO)
     logtype = LT_ALL;
   else
     logtype = LT_NOTHING;

   res = EINA_TRUE;

cleanup:
   if (data) E_FREE(data);
#endif
   return res;
}
