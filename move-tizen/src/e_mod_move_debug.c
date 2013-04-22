#include "e_mod_move_shared_types.h"
#include "e_mod_move.h"
#include "e_mod_move_atoms.h"
#include "e_mod_move_debug.h"

typedef struct _E_Mod_Move_Log_Info
{
   int  type;
   char file[256];
} E_Mod_Move_Log_Info;

/* local subsystem functions */
static void _e_mod_move_debug_borders_info_dump(E_Move *m, FILE *fs);
static void _e_mod_move_debug_borders_visibility_dump(E_Move *m, FILE *fs);
static void _e_mod_move_debug_event_cb_dump(E_Move *m, FILE *fs);
static void _e_mod_move_debug_control_objects_info_dump(E_Move *m, E_Move_Canvas *canvas, FILE *fs);
static void _e_mod_move_debug_indicator_controller_info_dump(E_Move *m, E_Move_Canvas *canvas, FILE *fs);
static void _e_mod_move_debug_canvas_info_dump(E_Move *m, E_Move_Canvas *canvas, FILE *fs);
static void _e_mod_move_debug_evas_stack_dump(E_Move *m, E_Move_Canvas *canvas, FILE *fs);
static void _e_mod_move_debug_dim_objects_info_dump(E_Move *m, E_Move_Canvas *canvas, FILE *fs);
static void _e_mod_move_debug_event_logs_dump(E_Move *m, FILE *fs);
static void _e_mod_move_debug_control_objects_visible_set(E_Move *m, E_Move_Canvas *canvas, Eina_Bool visi);
static void _e_mod_move_debug_widget_objects_visible_set(E_Move *m, E_Move_Canvas *canvas, Eina_Bool visi);
static void _e_mod_move_debug_indicator_controller_objects_visible_set(E_Move *m, E_Move_Canvas *canvas, Eina_Bool visi);
static void _e_mod_move_debug_objects_visible_set(Eina_Bool visi);

/* local subsystem functions */
static void
_e_mod_move_debug_borders_info_dump(E_Move *m,
                                    FILE   *fs)
{
   E_Move_Border *mb;
   Ecore_X_Window bid = 0; // Border Window ID
   Ecore_X_Window cid = 0; // Client Window ID
   char *wname = NULL, *wclas = NULL;
   int pid = 0, i = 0;
   char buf[4096];
   Eina_Bool res;

   fprintf(fs, "\n\nB------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "    <MOVE MODULE> BORDER STACK INFO \n");
   fprintf(fs, "-------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "  NO   BORDER  CLIENT_WIN V    x     y    w    h  | SHAPE_INPUT (x, y, w, h) | CONTENTS ( x, y, w, h ) | PID  WNAME / WCLASS\n");
   fprintf(fs, "-------------------------------------------------------------------------------------------------------------------------------------\n");

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        cid = e_mod_move_util_client_xid_get(mb);
        ecore_x_icccm_name_class_get(cid, &wname, &wclas);
        res = ecore_x_netwm_pid_get(cid, &pid);
        if (!res) pid = 0;

        memset(buf, 0, sizeof(buf));

        if (mb->bd)
          {
             bid = mb->bd->win;
             snprintf(buf, sizeof(buf), "0x%07x", mb->bd->client.win);
          }

        fprintf(fs,
                " %3d 0x%07x %9s %2s %5d %5d %4d %4d |  (%4d,%4d,%4d,%4d )  | (%4d,%4d,%4d,%4d )  | %4d %s / %s\n",
                i,
                bid,
                buf,
                mb->visible ? "v" : " ",
                mb->x, mb->y, mb->w, mb->h,
                mb->shape_input ? mb->shape_input->x : 0,
                mb->shape_input ? mb->shape_input->y : 0,
                mb->shape_input ? mb->shape_input->w : 0,
                mb->shape_input ? mb->shape_input->h : 0,
                mb->contents ? mb->contents->x : 0,
                mb->contents ? mb->contents->y : 0,
                mb->contents ? mb->contents->w : 0,
                mb->contents ? mb->contents->h : 0,
                pid,
                wname ? wname : "",
                wclas ? wclas : "");

        if (wname) free(wname);
        if (wclas) free(wclas);
        wname = wclas = NULL;
        pid = cid = 0;
        i++;
     }
   fprintf(fs, "E------------------------------------------------------------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_borders_visibility_dump(E_Move *m,
                                          FILE   *fs)
{
   E_Move_Border *mb;
   Ecore_X_Window cid = 0;
   Ecore_X_Window bid = 0;
   char *wname = NULL, *wclas = NULL;
   int pid = 0, i = 0;
   char buf[4096];
   char buf2[4096];
   Eina_Bool res;

   fprintf(fs, "\n\nB----------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "   <MOVE MODULE>  VISIBILITY / INDICATOR_STATE / INDICATOR_TYPE Info\n");
   fprintf(fs, "---------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "  NO   BORDER  CLIENT_WIN | V | VISIBILITY_STATE | INDICATOR_STATE | INDICATOR_TYPE | PID  WNAME / WCLASS\n");
   fprintf(fs, "---------------------------------------------------------------------------------------------------------\n");

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        cid = e_mod_move_util_client_xid_get(mb);
        ecore_x_icccm_name_class_get(cid, &wname, &wclas);
        res = ecore_x_netwm_pid_get(cid, &pid);
        if (!res) pid = 0;

        memset(buf, 0, sizeof(buf));
        memset(buf2, 0, sizeof(buf));

        if (mb->bd)
          {
             bid = mb->bd->win;
             snprintf(buf, sizeof(buf), "0x%07x", mb->bd->client.win);
          }

        switch (mb->indicator_type)
          {
           case E_MOVE_INDICATOR_TYPE_NONE:
              snprintf(buf2, sizeof(buf2), "%s", "NONE");
              break;
           case    E_MOVE_INDICATOR_TYPE_1:
              snprintf(buf2, sizeof(buf2), "%s", "TYPE_0");
              break;
           case    E_MOVE_INDICATOR_TYPE_2:
              snprintf(buf2, sizeof(buf2), "%s", "TYPE_1");
              break;
           default:
              break;
          }

        fprintf(fs,
                " %3d 0x%07x %9s | %2s | %16.16s | %15.15s | %14.14s | %4d %s / %s\n",
                i,
                bid,
                buf,
                mb->visible ? "v" : " ",
                mb->visibility == E_MOVE_VISIBILITY_STATE_NONE ? ("None") : (mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE ? "Visible" : "Fully Obscured"),
                mb->indicator_state == E_MOVE_INDICATOR_STATE_NONE ? ("None") : (mb->indicator_state == E_MOVE_INDICATOR_STATE_ON ? "On" : "Off"),
                buf2,
                pid,
                wname ? wname : "",
                wclas ? wclas : "");

        if (wname) free(wname);
        if (wclas) free(wclas);
        wname = wclas = NULL;
        pid = cid = 0;
        i++;
     }
   fprintf(fs, "E------------------------------------------------------------------------------------------------\n");
}

static void _e_mod_move_debug_event_cb_dump(E_Move *m,
                                            FILE   *fs)

{
   E_Move_Event_Cb motion_start_cb = NULL;
   E_Move_Event_Cb motion_move_cb = NULL;
   E_Move_Event_Cb motion_end_cb = NULL;

   fprintf(fs, "\n\nB----------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "    <MOVE MODULE> EVENT_CallBack Pointer Dump \n");
   fprintf(fs, "-----------------------------------------------------------------------------------------------------------------------------------\n");
   motion_start_cb = e_mod_move_indicator_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_START);
   motion_move_cb = e_mod_move_indicator_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_MOVE);
   motion_end_cb = e_mod_move_indicator_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_END);
   fprintf(fs, "[INDICATOR ] Motion_Start: %p     Motion_Move: %p    Motion_End: %p\n", (void *)motion_start_cb, (void *)motion_move_cb, (void *)motion_end_cb);
   motion_start_cb = e_mod_move_apptray_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_START);
   motion_move_cb = e_mod_move_apptray_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_MOVE);
   motion_end_cb = e_mod_move_apptray_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_END);
   fprintf(fs, "[ APPTRAY  ] Motion_Start: %p     Motion_Move: %p    Motion_End: %p\n", (void *)motion_start_cb, (void *)motion_move_cb, (void *)motion_end_cb);
   motion_start_cb = e_mod_move_quickpanel_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_START);
   motion_move_cb = e_mod_move_quickpanel_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_MOVE);
   motion_end_cb = e_mod_move_quickpanel_event_cb_get(E_MOVE_EVENT_TYPE_MOTION_END);
   fprintf(fs, "[QUICKPANEL] Motion_Start: %p     Motion_Move: %p    Motion_End: %p\n", (void *)motion_start_cb, (void *)motion_move_cb, (void *)motion_end_cb);
   fprintf(fs, "E-----------------------------------------------------------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_control_objects_info_dump(E_Move        *m,
                                            E_Move_Canvas *canvas,
                                            FILE          *fs)
{
   Eina_List *l;
   E_Move_Control_Object *mco, *_mco = NULL;
   E_Move_Border *mb, *_mb = NULL;
   int x, y, w, h, i = 1;
   Ecore_X_Window cid = 0;
   char *netwm_name = NULL;
   E_Move_Event_Cb motion_start_cb = NULL;
   E_Move_Event_Cb motion_move_cb = NULL;
   E_Move_Event_Cb motion_end_cb = NULL;

   _e_mod_move_debug_event_cb_dump(m, fs);

   fprintf(fs, "\nB-------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "   <MOVE MODULE>  Controller Object Info\n");
   fprintf(fs, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
   if (canvas->zone)
     {
        fprintf(fs, " canvas->zone:%p num:%d %d,%d %dx%d\n",
                (void *)canvas->zone, canvas->zone->num,
                canvas->zone->x, canvas->zone->y,
                canvas->zone->w, canvas->zone->h);
     }
   fprintf(fs, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

   fprintf(fs, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " NO Border(WIN ID)  ctl_obj  found_o   ex   ey   ew     eh  | V | Motion_Start_CB | Motion_Move_CB | Motion_End_CB |          E_Move_Border_Type         | NETWM_NAME| \n");
   fprintf(fs, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;
   while (o)
     {
        EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
          {
             EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
               {
                  if (!mco) continue;
                  if (mco->canvas != canvas) continue;
                  if (mco->obj == o)
                    {
                       found = 1;
                       _mco = mco;
                       _mb = mb;
                       break;
                    }
               }
          }

        evas_object_geometry_get(o, &x, &y, &w, &h);
        if (found && _mco && _mb && _mb->visible)
          {
             motion_start_cb = e_mod_move_event_cb_get(_mco->event,
                                                       E_MOVE_EVENT_TYPE_MOTION_START);
             motion_move_cb = e_mod_move_event_cb_get(_mco->event,
                                                      E_MOVE_EVENT_TYPE_MOTION_MOVE);
             motion_end_cb = e_mod_move_event_cb_get(_mco->event,
                                                     E_MOVE_EVENT_TYPE_MOTION_END);
             cid = e_mod_move_util_client_xid_get(_mb);
             ecore_x_netwm_name_get(cid, &netwm_name);
             fprintf(fs,
                     " %2d 0x%07x     %p  %p %4d %4d %4d x %4d  | %s |   %p    |    %p  |   %p  |%35.35s  | %s\n",
                     i,
                     _mb->bd->win,
                     (void *)_mco->obj,
                     (void *)o,
                     x, y, w, h,
                     evas_object_visible_get(_mco->obj) ? "v" : "",
                     (void *)motion_start_cb,
                     (void *)motion_move_cb,
                     (void *)motion_end_cb,
                     e_mod_move_border_types_name_get(e_mod_move_border_type_get(_mb)),
                     netwm_name ? netwm_name : "");
          }

        o = evas_object_below_get(o);
        found = 0;
        _mco = NULL;
        _mb = NULL;
        motion_start_cb = NULL;
        motion_move_cb = NULL;
        motion_end_cb = NULL;
        if (netwm_name) free(netwm_name);
        netwm_name = NULL;
        cid = 0;
        i++;
     }
   fprintf(fs, "E----------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_indicator_controller_info_dump(E_Move        *m,
                                                 E_Move_Canvas *canvas,
                                                 FILE          *fs)
{
   Eina_List *l;
   E_Move_Evas_Object *meo, *_meo = NULL;
   int x, y, w, h, i = 1;
   Ecore_X_Window cid = 0;
   Ecore_X_Window target_win;
   char *netwm_name = NULL;
   E_Move_Indicator_Controller *mic = NULL;

   fprintf(fs, "\n\nB----------------------------------------------------------------------------------------------\n");
   fprintf(fs, "   <MOVE MODULE>  Indicator Controller Object Info ( It is used for FullScreen Window ) \n");
   fprintf(fs, "-----------------------------------------------------------------------------------------------\n");
   if (canvas->zone)
     {
        fprintf(fs, " canvas->zone:%p num:%d %d,%d %dx%d\n",
                (void *)canvas->zone, canvas->zone->num,
                canvas->zone->x, canvas->zone->y,
                canvas->zone->w, canvas->zone->h);
     }
   fprintf(fs, "------------------------------------------------------------------------------------------------\n");

   fprintf(fs, "------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " NO TARGET_WIN_ID  indicator_control_obj  found_o   ex   ey   ew     eh  | V |  NETWM_NAME\n");
   fprintf(fs, "------------------------------------------------------------------------------------------------\n");
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;

   if (e_mod_move_indicator_controller_state_get(m, &target_win))
     {
        mic = m->indicator_controller;
        if (mic)
          {
             while (o)
               {
                  EINA_LIST_FOREACH(mic->objs, l, meo)
                    {
                       if (!meo) continue;
                       if (meo->canvas != canvas) continue;
                       if (meo->obj == o)
                         {
                            found = 1;
                            _meo = meo;
                            break;
                         }
                    }

                  evas_object_geometry_get(o, &x, &y, &w, &h);
                  if (found && _meo)
                    {
                       cid = target_win;
                       ecore_x_netwm_name_get(cid, &netwm_name);
                       fprintf(fs,
                               " %2d   0x%07x         %p          %p %4d %4d %4d x %4d   %s   %s\n",
                               i,
                               target_win,
                               (void *)_meo->obj,
                               (void *)o,
                               x, y, w, h,
                               evas_object_visible_get(_meo->obj) ? "v" : "",
                               netwm_name ? netwm_name : "");
                    }

                  o = evas_object_below_get(o);
                  found = 0;
                  _meo = NULL;
                  if (netwm_name) free(netwm_name);
                  netwm_name = NULL;
                  cid = 0;
                  i++;
               }
          }
     }
   fprintf(fs, "E-----------------------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_canvas_info_dump(E_Move        *m,
                                   E_Move_Canvas *canvas,
                                   FILE          *fs)
{
   Eina_List *l;
   E_Move_Object *mo, *_mo = NULL;
   E_Move_Border *mb, *_mb = NULL;
   int x, y, w, h, i = 1;
   Ecore_X_Window cid = 0;
   char *netwm_name = NULL;

   fprintf(fs, "\n\nB-------------------------------------------------------------------------------------\n");
   fprintf(fs, "   <MOVE MODULE>  Mirror Object Info ( It is used for Move Scroll / Animation) \n");
   fprintf(fs, "--------------------------------------------------------------------------------------\n");
   if (canvas->zone)
     {
        fprintf(fs, " canvas->zone:%p num:%d %d,%d %dx%d\n",
                (void *)canvas->zone, canvas->zone->num,
                canvas->zone->x, canvas->zone->y,
                canvas->zone->w, canvas->zone->h);
     }
   fprintf(fs, "--------------------------------------------------------------------------------------\n");

   fprintf(fs, "--------------------------------------------------------------------------------------\n");
   fprintf(fs, " NO Border(WIN ID)  mirror_obj   found_o   ex   ey   ew     eh  | V |  NETWM_NAME\n");
   fprintf(fs, "--------------------------------------------------------------------------------------\n");
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;
   while (o)
     {
        EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
          {
             EINA_LIST_FOREACH(mb->objs, l, mo)
               {
                  if (!mo) continue;
                  if (mo->canvas != canvas) continue;
                  if (mo->obj == o)
                    {
                       found = 1;
                       _mo = mo;
                       _mb = mb;
                       break;
                    }
               }
          }

        evas_object_geometry_get(o, &x, &y, &w, &h);
        if (found && _mo && _mb && _mb->visible)
          {
             cid = e_mod_move_util_client_xid_get(_mb);
             ecore_x_netwm_name_get(cid, &netwm_name);
             fprintf(fs,
                     " %2d 0x%07x     %p      %p %4d %4d %4d x %4d   %s   %s\n",
                     i,
                     _mb->bd->win,
                     (void *)_mo->obj,
                     (void *)o,
                     x, y, w, h,
                     evas_object_visible_get(_mo->obj) ? "v" : "",
                     netwm_name ? netwm_name : "");
          }

        o = evas_object_below_get(o);
        found = 0;
        _mo = NULL;
        _mb = NULL;
        if (netwm_name) free(netwm_name);
        netwm_name = NULL;
        cid = 0;
        i++;
     }
   fprintf(fs, "E-------------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_evas_stack_dump(E_Move        *m,
                                  E_Move_Canvas *canvas,
                                  FILE          *fs)
{
   int x, y, w, h, i = 1;
   short layer;
   char desc[50];

   fprintf(fs, "\n\nB--------------------------------------------------------------------------------\n");
   fprintf(fs, "   < MOVE / COMP MODULE>  Evas Object Stack Dump \n");
   fprintf(fs, "---------------------------------------------------------------------------------\n");
   if (canvas->zone)
     {
        fprintf(fs, " canvas->zone:%p num:%d %d,%d %dx%d\n",
                (void *)canvas->zone, canvas->zone->num,
                canvas->zone->x, canvas->zone->y,
                canvas->zone->w, canvas->zone->h);
     }
   fprintf(fs, "----------------------------------------------------------------------------------\n");

   fprintf(fs, "----------------------------------------------------------------------------------\n");
   fprintf(fs, " NO      obj      | layer |  ex   ey   ew     eh   | V |   Description (Maker)\n");
   fprintf(fs, "----------------------------------------------------------------------------------\n");

   Evas_Object *o = evas_object_top_get(canvas->evas);

   while (o)
     {
        memset(desc, 0, sizeof(desc));
        if (evas_object_data_get(o, "win"))
          {
             if (evas_object_data_get(o, "src"))
                strncpy(desc, "WINDOW (COMP)", strlen("WINDOW (COMP)"));
             else
                strncpy(desc, "NONE ( ? )", strlen("NONE ( ? )"));
          }
        else if (evas_object_data_get(o, "move_ctl_obj"))
           strncpy(desc, "Control Obj (MOVE)", strlen("Control Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_widget_obj"))
           strncpy(desc, "Widget Obj (MOVE)", strlen("Widget Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_dim_obj"))
           strncpy(desc, "Dim Obj (MOVE)", strlen("Dim Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_evas_obj"))
           strncpy(desc, "Move Evas Obj (MOVE)", strlen("Move Evas Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_evas_mirror_obj"))
           strncpy(desc, "Move Evas Mirror Obj (MOVE)", strlen("Move Evas Mirror Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_mirror_obj"))
           strncpy(desc, "Mirror Obj (MOVE)", strlen("Mirror Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_evas_clipper_obj"))
           strncpy(desc, "Move Evas Clipper Obj (MOVE)", strlen("Move Evas Clipper Obj (MOVE)"));
        else if (evas_object_data_get(o, "move_clipper_obj"))
           strncpy(desc, "Move Clipper Obj (MOVE)", strlen("Move Clipper Obj (MOVE)"));
        else
           strncpy(desc, "NONE ( ? )", strlen("NONE ( ? )"));

        evas_object_geometry_get(o, &x, &y, &w, &h);
        layer = evas_object_layer_get(o);
        fprintf(fs,
                " %2d   0x%07x   | %5d | %4d %4d %4d x %4d  | %s |   %s \n",
                i,
                (unsigned int)o,
                layer,
                x, y, w, h,
                evas_object_visible_get(o) ? "v" : " ",
                desc);

        o = evas_object_below_get(o);
        i++;
     }
   fprintf(fs, "E---------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_dim_objects_info_dump(E_Move        *m,
                                        E_Move_Canvas *canvas,
                                        FILE          *fs)
{
   Eina_List *l;
   E_Move_Dim_Object *mdo, *_mdo = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   E_Move_Apptray_Data *at_data = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   Eina_List *at_dim_objs = NULL;
   Eina_List *qp_dim_objs = NULL;

   int x, y, w, h, i = 1;

   fprintf(fs, "\n\nB------------------------------------------------------------------\n");
   fprintf(fs, "   <MOVE MODULE>  Dim Object Info \n");
   fprintf(fs, "-------------------------------------------------------------------\n");
   if (canvas->zone)
     {
        fprintf(fs, " canvas->zone:%p num:%d %d,%d %dx%d\n",
                (void *)canvas->zone, canvas->zone->num,
                canvas->zone->x, canvas->zone->y,
                canvas->zone->w, canvas->zone->h);
     }
   fprintf(fs, "-------------------------------------------------------------------\n");

   fprintf(fs, "-------------------------------------------------------------------\n");
   fprintf(fs, " NO     dim_obj  found_o   ex   ey   ew     eh  | V | DIM_INFO\n");
   fprintf(fs, "-------------------------------------------------------------------\n");
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;

   at_mb = e_mod_move_apptray_find();
   qp_mb = e_mod_move_quickpanel_find();

   if (at_mb) at_data = (E_Move_Apptray_Data *)(at_mb->data);
   if (qp_mb) qp_data = (E_Move_Quickpanel_Data *)(qp_mb->data);
   
   if (at_data) at_dim_objs = at_data->dim_objs;
   if (qp_data) qp_dim_objs = qp_data->dim_objs;

   while (o)
     {
        if (at_dim_objs)
          EINA_LIST_FOREACH(at_dim_objs, l, mdo)
            {
               if (!mdo) continue;
               if (mdo->canvas != canvas) continue;
               if (mdo->obj == o)
                 {
                    found = 1;
                    _mdo = mdo;
                    break;
                 }
            }

        evas_object_geometry_get(o, &x, &y, &w, &h);
        if (found && _mdo)
          {
             fprintf(fs,
                     " %2d   %p  %p %4d %4d %4d x %4d   %s   %s\n",
                     i,
                     (void *)_mdo->obj,
                     (void *)o,
                     x, y, w, h,
                     evas_object_visible_get(_mdo->obj) ? "v" : "",
                     "APPTRAY_DIM");
          }

        o = evas_object_below_get(o);
        found = 0;
        _mdo = NULL;
        i++;
     }

   o = evas_object_top_get(canvas->evas);
   found = 0;
   i = 0;
   while (o)
     {
        if (qp_dim_objs)
          EINA_LIST_FOREACH(qp_dim_objs, l, mdo)
            {
               if (!mdo) continue;
               if (mdo->canvas != canvas) continue;
               if (mdo->obj == o)
                 {
                    found = 1;
                    _mdo = mdo;
                    break;
                 }
            }

        evas_object_geometry_get(o, &x, &y, &w, &h);
        if (found && _mdo)
          {
             fprintf(fs,
                     " %2d   %p  %p %4d %4d %4d x %4d   %s   %s\n",
                     i,
                     (void *)_mdo->obj,
                     (void *)o,
                     x, y, w, h,
                     evas_object_visible_get(_mdo->obj) ? "v" : "",
                     "QUICKPANEL_DIM");
          }

        o = evas_object_below_get(o);
        found = 0;
        _mdo = NULL;
        i++;
     }

   fprintf(fs, "E------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_event_logs_dump(E_Move *m,
                                  FILE   *fs)
{
   Eina_List *l;
   E_Move_Event_Log *log;
   int i = 1;
   char obj_type[20];

   E_CHECK(m);
   E_CHECK(m->ev_log);

   fprintf(fs, "\n\nB----------------------------------------------------------------------------------------\n");
   fprintf(fs, "   <MOVE MODULE> EVENT LOG DUMP \n");
   fprintf(fs, "-----------------------------------------------------------------------------------------\n");
   fprintf(fs, "-----------------------------------------------------------------------------------------\n");
   fprintf(fs, " NO         EVENT_TYPE            WID / OBJ    ( x , y )   Button | Additional Data\n");
   fprintf(fs, "-----------------------------------------------------------------------------------------\n");

   EINA_LIST_FOREACH(m->ev_logs, l, log)
     {
        if ((log->t == E_MOVE_EVENT_LOG_EVAS_OBJECT_MOUSE_DOWN)
             || (log->t == E_MOVE_EVENT_LOG_EVAS_OBJECT_MOUSE_UP))
          {
             memset(obj_type, 0, sizeof(obj_type));
             switch (log->d.eo_m.t)
               {
                case E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_QUICKPANEL:
                   strncpy(obj_type, "QUICKPANEL", sizeof("QUICKPANEL"));
                   break;
                case E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_APPTRAY:
                   strncpy(obj_type, "APPTRAY", sizeof("APPTRAY"));
                   break;
                case E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_INDICATOR:
                   strncpy(obj_type, "INDICATOR", sizeof("INDICATOR"));
                   break;
                case E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_UNKNOWN:
                default:
                   strncpy(obj_type, "UNKNOWN", sizeof("UNKNOWN"));
                   break;
               }
          }

        switch (log->t)
          {
           case E_MOVE_EVENT_LOG_ECORE_SINGLE_MOUSE_DOWN:
              fprintf(fs,
                      " %2d   %23s   w:0x%08x (%4d ,%4d )  btn:%d\n",
                      i,
                      "ECORE_SINGLE_MOUSE_DOWN",
                      log->d.ec_sm.win,
                      log->d.ec_sm.x,
                      log->d.ec_sm.y,
                      log->d.ec_sm.btn);
              break;
           case E_MOVE_EVENT_LOG_ECORE_SINGLE_MOUSE_UP:
              fprintf(fs,
                      " %2d   %23s   w:0x%08x (%4d ,%4d )  btn:%d\n",
                      i,
                      "ECORE_SINGLE_MOUSE_UP",
                      log->d.ec_sm.win,
                      log->d.ec_sm.x,
                      log->d.ec_sm.y,
                      log->d.ec_sm.btn);
              break;
           case E_MOVE_EVENT_LOG_ECORE_MULTI_MOUSE_DOWN:
              fprintf(fs,
                      " %2d   %23s   w:0x%08x (%5.1f,%5.1f)  btn:%d | dev:%d\n",
                      i,
                      "ECORE_MULTI_MOUSE_DOWN",
                      log->d.ec_mm.win,
                      log->d.ec_mm.x,
                      log->d.ec_mm.y,
                      log->d.ec_mm.btn,
                      log->d.ec_mm.dev);
              break;
           case E_MOVE_EVENT_LOG_ECORE_MULTI_MOUSE_UP:
              fprintf(fs,
                      " %2d   %23s   w:0x%08x (%5.1f,%5.1f)  btn:%d | dev:%d\n",
                      i,
                      "ECORE_MULTI_MOUSE_UP",
                      log->d.ec_mm.win,
                      log->d.ec_mm.x,
                      log->d.ec_mm.y,
                      log->d.ec_mm.btn,
                      log->d.ec_mm.dev);
              break;
           case E_MOVE_EVENT_LOG_EVAS_OBJECT_MOUSE_DOWN:
              fprintf(fs,
                      " %2d   %23s   obj:%p (%4d ,%4d )  btn:%d | %10s  eo_geo(%d,%d,%d,%d)\n",
                      i,
                      "EVAS_OBJECT_MOUSE_DOWN",
                      log->d.eo_m.obj,
                      log->d.eo_m.x,
                      log->d.eo_m.y,
                      log->d.eo_m.btn,
                      obj_type,
                      log->d.eo_m.ox,
                      log->d.eo_m.oy,
                      log->d.eo_m.ow,
                      log->d.eo_m.oh);
              break;
           case E_MOVE_EVENT_LOG_EVAS_OBJECT_MOUSE_UP:
              fprintf(fs,
                      " %2d   %23s   obj:%p (%4d ,%4d )  btn:%d | %10s  eo_geo(%d,%d,%d,%d)\n",
                      i,
                      "EVAS_OBJECT_MOUSE_UP",
                      log->d.eo_m.obj,
                      log->d.eo_m.x,
                      log->d.eo_m.y,
                      log->d.eo_m.btn,
                      obj_type,
                      log->d.eo_m.ox,
                      log->d.eo_m.oy,
                      log->d.eo_m.ow,
                      log->d.eo_m.oh);
              break;
           case E_MOVE_EVENT_LOG_UNKOWN:
           default:
              fprintf(fs,
                      " %2d   %23s\n",
                      i,
                      "EVENT_LOG_UNKOWN");
              break;
          }
        i++;
     }
   fprintf(fs, "E----------------------------------------------------------------------------------------\n");
}

static void
_e_mod_move_debug_control_objects_visible_set(E_Move        *m,
                                              E_Move_Canvas *canvas,
                                              Eina_Bool      visi)
{
   Eina_List *l;
   E_Move_Control_Object *mco, *_mco = NULL;
   E_Move_Border *mb, *_mb = NULL;
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;

   while (o)
     {
        EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
          {
             EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
               {
                  if (!mco) continue;
                  if (mco->canvas != canvas) continue;
                  if (mco->obj == o)
                    {
                       found = 1;
                       _mco = mco;
                       _mb = mb;
                       break;
                    }
               }
          }

        if (found && _mco && _mb && _mb->visible)
          {
             if (visi)
               e_mod_move_bd_move_ctl_objs_color_set(_mb, 255, 0, 0, 100);
             else
               e_mod_move_bd_move_ctl_objs_color_set(_mb, 0, 0, 0, 0);
          }

        o = evas_object_below_get(o);
        found = 0;
        _mco = NULL;
        _mb = NULL;
     }
}

static void
_e_mod_move_debug_widget_objects_visible_set(E_Move        *m,
                                             E_Move_Canvas *canvas,
                                             Eina_Bool      visi)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo, *_mwo = NULL;
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;
   E_Move_Indicator_Widget *indi_widget = NULL;
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;

   if ((indi_widget = e_mod_move_indicator_widget_get()))
     {
        while (o)
          {
             EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
               {
                  if (!mwo) continue;
                  if (mwo->canvas != canvas) continue;
                  if (mwo->obj == o)
                    {
                       found = 1;
                       _mwo = mwo;
                       break;
                    }
               }

             if (found && _mwo)
               {
                  if (visi)
                    e_mod_move_evas_objs_color_set(indi_widget->objs,
                                                   0, 0, 255, 100);
                  else
                    e_mod_move_evas_objs_color_set(indi_widget->objs,
                                                   0, 0, 0, 0);
               }

             o = evas_object_below_get(o);
             found = 0;
             _mwo = NULL;
          }
     }

   o = evas_object_top_get(canvas->evas);

   if ((mini_apptray_widget = e_mod_move_mini_apptray_widget_get()))
     {
        while (o)
          {
             EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
               {
                  if (!mwo) continue;
                  if (mwo->canvas != canvas) continue;
                  if (mwo->obj == o)
                    {
                       found = 1;
                       _mwo = mwo;
                       break;
                    }
               }

             if (found && _mwo)
               {
                  if (visi)
                    e_mod_move_evas_objs_color_set(mini_apptray_widget->objs,
                                                   255, 255, 0, 100);
                  else
                    e_mod_move_evas_objs_color_set(mini_apptray_widget->objs,
                                                   0, 0, 0, 0);
               }

             o = evas_object_below_get(o);
             found = 0;
             _mwo = NULL;
          }
     }
}

static void
_e_mod_move_debug_indicator_controller_objects_visible_set(E_Move        *m,
                                                           E_Move_Canvas *canvas,
                                                           Eina_Bool      visi)
{
   Eina_List *l;
   E_Move_Evas_Object *meo, *_meo = NULL;
   Evas_Object *o = evas_object_top_get(canvas->evas);
   Eina_Bool found = 0;
   Ecore_X_Window target_win;
   E_Move_Indicator_Controller *mic = NULL;

   if (e_mod_move_indicator_controller_state_get(m, &target_win))
     {
        mic = m->indicator_controller;
        while (o)
          {
             EINA_LIST_FOREACH(mic->objs, l, meo)
               {
                  if (!meo) continue;
                  if (meo->canvas != canvas) continue;
                  if (meo->obj == o)
                    {
                       found = 1;
                       _meo = meo;
                       break;
                    }
               }

             if (found && _meo)
               {
                  if (visi)
                     e_mod_move_evas_objs_color_set(mic->objs, 0, 255, 0, 100);
                  else
                     e_mod_move_evas_objs_color_set(mic->objs, 0, 0, 0, 0);
               }

             o = evas_object_below_get(o);
             found = 0;
             _meo = NULL;
          }
     }
}

static void
_e_mod_move_debug_objects_visible_set(Eina_Bool visi)
{
   E_Move *m = NULL;
   Eina_List *l = NULL;
   E_Move_Canvas *canvas = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);

   EINA_LIST_FOREACH(m->canvases, l, canvas)
     {
        if (!canvas) continue;
        _e_mod_move_debug_control_objects_visible_set(m, canvas, visi);
        _e_mod_move_debug_widget_objects_visible_set(m, canvas, visi);
        _e_mod_move_debug_indicator_controller_objects_visible_set(m,
                                                                   canvas,
                                                                   visi);
     }
}

/* externally accessible globals */
EINTERN int logtype = LT_NOTHING;
//EINTERN int logtype = LT_ALL;

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_move_debug_info_dump(Eina_Bool   to_file,
                           const char *name)
{
   E_Move *m;
   E_Move_Canvas *canvas;
   Eina_List *l;
   FILE *fs = stderr;
   char *f_name = NULL;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   if ((to_file) && (name))
     {
        f_name = E_NEW(char, strlen(name) + sizeof("_move") + 1);
        memcpy(f_name, name, strlen(name));
        strncat(f_name, "_move", sizeof("_move"));
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

   _e_mod_move_debug_borders_info_dump(m, fs);
   _e_mod_move_debug_borders_visibility_dump(m, fs);

   EINA_LIST_FOREACH(m->canvases, l, canvas)
     {
        if (!canvas) continue;
        _e_mod_move_debug_evas_stack_dump(m, canvas, fs);
        _e_mod_move_debug_control_objects_info_dump(m, canvas, fs);
        _e_mod_move_debug_canvas_info_dump(m, canvas, fs);
        _e_mod_move_debug_dim_objects_info_dump(m, canvas, fs);
        _e_mod_move_debug_indicator_controller_info_dump(m, canvas, fs);
     }

   _e_mod_move_debug_event_logs_dump(m, fs);

   if (to_file)
     {
        fflush(fs);
        fclose(fs);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_debug_prop_handle(Ecore_X_Event_Window_Property *ev)
{
   Eina_Bool res = EINA_FALSE;
   E_Mod_Move_Log_Info info = {LT_NOTHING, {0,}};
   unsigned char* data = NULL;
   int ret, n;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   ret = ecore_x_window_prop_property_get
           (ev->win, ATOM_CM_LOG, ECORE_X_ATOM_CARDINAL,
           32, &data, &n);
   E_CHECK_GOTO((ret != -1), cleanup);
   E_CHECK_GOTO(((ret > 0) && (data)), cleanup);

   memcpy(&info, data, sizeof(E_Mod_Move_Log_Info));
   logtype = info.type;

   fprintf(stdout, "[MOVE] log-type:0x%08x\n", logtype);

   if (logtype == LT_NOTHING)
     {
        _e_mod_move_debug_objects_visible_set(EINA_FALSE);
     }
   else if (logtype == (LT_CREATE | LT_INFO_SHOW))
     {
        e_mod_move_debug_info_dump(EINA_FALSE, NULL);
        _e_mod_move_debug_objects_visible_set(EINA_TRUE);
     }
   else if ((logtype == LT_DUMP) &&
       (strlen(info.file) > 0))
     {
        e_mod_move_debug_info_dump(EINA_TRUE, info.file);
        ecore_x_client_message32_send
          (ev->win, ATOM_MV_LOG_DUMP_DONE,
          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
          0, 0, 0, 0, 0);
     }

   if (logtype == LT_INFO)
     logtype = LT_ALL;
   else
     logtype = LT_NOTHING;

   res = EINA_TRUE;

cleanup:
   if (data) E_FREE(data);
   return res;
}
