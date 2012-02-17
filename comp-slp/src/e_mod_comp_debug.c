#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"

typedef struct _E_Mod_Comp_Log_Info
{
   int  type;
   char file[256];
} E_Mod_Comp_Log_Info;

/* externally accessible globals */
EINTERN int logtype = LT_NOTHING;

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_comp_debug_info_dump(Eina_Bool to_file,
                           const char *name)
{
   E_Comp *c;
   E_Comp_Win *cw;
   double val;
   const char *file, *group;
   int x, y, w, h, i = 1;
   FILE *fs = stderr;

   c = e_mod_comp_util_get();
   E_CHECK_RETURN(c, 0);

   if ((to_file) && (name))
     {
        fs = fopen(name, "w");
        if (!fs)
          {
             fprintf(stderr, "can't open %s file.\n", name);
             fs = stderr;
             to_file = EINA_FALSE;
          }
     }

   fprintf(fs, "B-------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " No   WinID        shobj    x    y    w    h   ex   ey   ew   eh W S O    clipper         shower        swallow         group      st \n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " c->bg_img : %s 0x%08x\n", evas_object_visible_get(c->bg_img) ? "O" : "X", (unsigned int)c->bg_img);
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        edje_object_file_get(cw->shobj, &file, &group);
        evas_object_geometry_get(cw->shobj, &x, &y, &w, &h);
        fprintf(fs,
                " %2d 0x%07x 0x%08x %4d %4d %4d %4d %4d %4d %4d %4d %s %s %s %10.10s %.1f %10.10s %.1f %10.10s %.1f %15.15s %d\n",
                i,
                e_mod_comp_util_client_xid_get(cw),
                (unsigned int)cw->shobj,
                cw->x, cw->y, cw->w, cw->h,
                x, y, w, h,
                cw->visible ? "O" : "X",
                evas_object_visible_get(cw->shobj) ? "O" : "X",
                evas_object_visible_get(cw->obj) ? "O" : "X",
                edje_object_part_state_get(cw->shobj, "clipper", &val), val,
                edje_object_part_state_get(cw->shobj, "shower", &val), val,
                edje_object_part_state_get(cw->shobj, "e.swallow.content", &val), val,
                group, cw->effect_stage);
        i++;
     }

   i = 1;
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if (!cw) break;
        file = group = NULL;
        edje_object_file_get(cw->shobj,
                             &file, &group);
        fprintf(fs,
                " %2d 0x%07x %s\n",
                i, e_mod_comp_util_client_xid_get(cw),
                file);
        i++;
     }

   i = 1;
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " No     WinID      shobj        obj    found_o    x    y    w    h   ex   ey   ew   eh | W S O\n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   Evas_Object *o = evas_object_top_get(c->evas);
   Eina_Bool found = 0;
   Ecore_X_Window cid = 0;
   char *wname = NULL, *wclas = NULL;
   int pid = 0;
   while (o)
     {
        EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
          {
             if (!cw) break;
             if (cw->shobj == o)
               {
                  found = 1;
                  cid = e_mod_comp_util_client_xid_get(cw);
                  ecore_x_icccm_name_class_get(cid, &wname, &wclas);
                  ecore_x_netwm_pid_get(cid, &pid);
                  break;
               }
          }

        evas_object_geometry_get(o, &x, &y, &w, &h);

        if (found && cw->visible)
          {
             fprintf(fs,
                     " %2d 0x%07x 0x%08x 0x%08x 0x%08x %4d,%4d %4dx%4d %4d,%4d %4dx%4d | %s %s %s %4d, %s, %s\n",
                     i,
                     e_mod_comp_util_client_xid_get(cw),
                     (unsigned int)cw->shobj,
                     (unsigned int)cw->obj,
                     (unsigned int)o,
                     cw->x, cw->y, cw->w, cw->h,
                     x, y, w, h,
                     cw->visible ? "O" : "X",
                     evas_object_visible_get(cw->shobj) ? "O" : "X",
                     evas_object_visible_get(o) ? "O" : "X",
                     pid, wname, wclas);
          }
        else if (o == c->bg_img)
          {
             fprintf(fs,
                     " %2d %31.31s 0x%08x %19.19s %4d,%4d %4dx%4d |     %s <-- bg_img\n",
                     i, " ", (unsigned int)o, " ", x, y, w, h, evas_object_visible_get(o) ? "O" : "X");
          }
        else if (evas_object_visible_get(o))
          {
             if (found)
               {
                  fprintf(fs,
                          " %2d 0x%07x 0x%08x 0x%08x 0x%08x %4d,%4d %4dx%4d %4d,%4d %4dx%4d | %s %s %s %4d, %s, %s\n",
                          i,
                          e_mod_comp_util_client_xid_get(cw),
                          (unsigned int)cw->shobj,
                          (unsigned int)cw->obj,
                          (unsigned int)o,
                          cw->x, cw->y, cw->w, cw->h,
                          x, y, w, h,
                          cw->visible ? "O" : "X",
                          evas_object_visible_get(cw->shobj) ? "O" : "X",
                          evas_object_visible_get(o) ? "O" : "X",
                          pid, wname, wclas);
               }
             else
               {
                  fprintf(fs,
                          " %2d %31.31s 0x%08x %19.19s %4d,%4d %4dx%4d |     %s\n",
                          i, " ", (unsigned int)o, " ", x, y, w, h,
                          evas_object_visible_get(o) ? "O" : "X");
               }
          }

        o = evas_object_below_get(o);

        if (wname) free(wname);
        if (wclas) free(wclas);
        wname = wclas = NULL;
        pid = cid = 0;
        found = 0;
        i++;
     }

   i = 1;
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " No   WinID        shobj    x    y    w    h   ex   ey   ew   eh W S O  InputOnly  Override  Valid (WinID exists in XServer)  title\n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   Ecore_X_Window_Attributes att;
   Eina_Bool valid = EINA_TRUE;
   char *title = NULL;
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
        valid = EINA_TRUE;
        if (!ecore_x_window_attributes_get(cw->win, &att)) valid = EINA_FALSE;
        title = NULL;
        title = ecore_x_icccm_title_get(cw->win);
        evas_object_geometry_get(cw->shobj, &x, &y, &w, &h);
        fprintf(fs,
                " %2d 0x%07x 0x%08x %4d %4d %4d %4d %4d %4d %4d %4d %s %s %s %6s %10s %12s\t%s\n",
                i,
                e_mod_comp_util_client_xid_get(cw),
                (unsigned int)cw->shobj,
                cw->x, cw->y, cw->w, cw->h,
                x, y, w, h,
                cw->visible ? "O" : "X",
                evas_object_visible_get(cw->shobj) ? "O" : "X",
                evas_object_visible_get(cw->obj) ? "O" : "X",
                cw->input_only ? "O" : "X",
                cw->override ? "O" : "X",
                valid ? "O" : "X",
                title ? title : "");
        if (title) free(title);
        i++;
     }
   fprintf(fs, "E-------------------------------------------------------------------------------------------------------------------------------------\n");
   fflush(fs);

   if (to_file) fclose(fs);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_debug_edje_error_get(Evas_Object *o,
                                Ecore_X_Window win)
{
   int err = EDJE_LOAD_ERROR_NONE;
   E_CHECK_RETURN(o, 0);

   if ((err = edje_object_load_error_get(o)) != EDJE_LOAD_ERROR_NONE)
     {
        switch (err)
          {
           case EDJE_LOAD_ERROR_GENERIC:                    fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_GENERIC! win:0x%08x\n",                    win); break;
           case EDJE_LOAD_ERROR_DOES_NOT_EXIST:             fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_DOES_NOT_EXIST! win:0x%08x\n",             win); break;
           case EDJE_LOAD_ERROR_PERMISSION_DENIED:          fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_PERMISSION_DENIED! win:0x%08x\n",          win); break;
           case EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED: fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED! win:0x%08x\n", win); break;
           case EDJE_LOAD_ERROR_CORRUPT_FILE:               fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_CORRUPT_FILE! win:0x%08x\n",               win); break;
           case EDJE_LOAD_ERROR_UNKNOWN_FORMAT:             fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_UNKNOWN_FORMAT! win:0x%08x\n",             win); break;
           case EDJE_LOAD_ERROR_INCOMPATIBLE_FILE:          fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_INCOMPATIBLE_FILE! win:0x%08x\n",          win); break;
           case EDJE_LOAD_ERROR_UNKNOWN_COLLECTION:         fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_UNKNOWN_COLLECTION! win:0x%08x\n",         win); break;
           case EDJE_LOAD_ERROR_RECURSIVE_REFERENCE:        fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_RECURSIVE_REFERENCE! win:0x%08x\n",        win); break;
           default:
              break;
          }
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_debug_prop_handle(Ecore_X_Event_Window_Property *ev)
{
   Eina_Bool res = EINA_FALSE;
   E_Mod_Comp_Log_Info info = {LT_NOTHING, {0,}};
   unsigned char* data = NULL;
   int ret, n;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   ret = ecore_x_window_prop_property_get
           (ev->win, ATOM_CM_LOG, ECORE_X_ATOM_CARDINAL,
           32, &data, &n);
   E_CHECK_GOTO((ret != -1), cleanup);
   E_CHECK_GOTO(((ret > 0) && (data)), cleanup);

   memcpy(&info, data, sizeof(E_Mod_Comp_Log_Info));
   logtype = info.type;

   fprintf(stdout, "[COMP] logtupe:0x%08x\n", logtype);

   if (logtype == LT_CREATE)
     {
        e_mod_comp_debug_info_dump(EINA_FALSE, NULL);
        e_mod_comp_fps_toggle();
     }

   if ((logtype == LT_DUMP) &&
       (strlen(info.file) > 0))
     {
        e_mod_comp_debug_info_dump(EINA_TRUE, info.file);
        ecore_x_client_message32_send
          (ev->win, ATOM_CM_LOG_DUMP_DONE,
          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
          0, 0, 0, 0, 0);
     }

   res = EINA_TRUE;

cleanup:
   if (data) E_FREE(data);
   return res;
}
