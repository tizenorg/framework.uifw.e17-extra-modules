#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */
static Eina_Bool _e_mod_move_indi_ctl_event_clear(E_Move_Indicator_Controller *mic);
static Eina_Bool _e_mod_move_indi_ctl_cb_motion_start(void *data, void *event_info);
static Eina_Bool _e_mod_move_indi_ctl_cb_motion_move(void *data, void *event_info);
static Eina_Bool _e_mod_move_indi_ctl_cb_motion_end(void *data, void *event_info);
static void      _e_mod_move_indi_ctl_obj_event_setup(E_Move_Indicator_Controller *mic, E_Move_Evas_Object *meo);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_indi_ctl_event_clear(E_Move_Indicator_Controller *mic)
{
   Eina_Bool           click = EINA_FALSE;
   E_Move_Evas_Object *meo = NULL;
   Eina_List          *l;

   E_CHECK_RETURN(mic, EINA_FALSE);

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
        click = e_mod_move_event_click_get(meo->event);
     }
   E_CHECK_RETURN(click, EINA_FALSE);

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
        e_mod_move_event_data_clear(meo->event);
        e_mod_move_event_click_set(meo->event, EINA_FALSE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indi_ctl_cb_motion_start(void *data,
                                     void *event_info)
{
   E_Move *m = NULL;
   E_Move_Border *indi_mb = NULL;
   E_Move_Border *target_mb = NULL;
   E_Move_Indicator_Controller *mic = (E_Move_Indicator_Controller *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Evas_Object *meo = NULL;
   Eina_List *l;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();
   if (!mic || !info || !m) return EINA_FALSE;
   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s  (%d,%d) %s()\n", "EVAS_OBJ",
     info->coord.x, info->coord.y, __func__);

   // check indicator and if indicator does not exist, then destroy data, and hide
   target_mb = e_mod_move_border_client_find(mic->target_win);
   if (!target_mb)
     {
        // event clear
        _e_mod_move_indi_ctl_event_clear(mic);
        e_mod_move_evas_objs_del(mic->objs);
        memset(mic, 0, sizeof(E_Move_Indicator_Controller));
        E_FREE(mic);
        m->indicator_controller = NULL;
        return EINA_FALSE;
     }

   indi_mb = e_mod_move_indicator_find();
   if (!indi_mb)
     {
        //e_mod_move_indicator_controller_unset(m);
        return EINA_FALSE;
     }

   // position check & and if position is not at indicator then destroy data, and hide
   if (!E_INSIDE(info->coord.x, info->coord.y,
                 indi_mb->x, indi_mb->y, indi_mb->w, indi_mb->h))
     {
        //e_mod_move_indicator_controller_unset(m);
        if (indi_mb->visible)
          {
             e_border_hide(indi_mb->bd,2);

             e_mod_move_evas_objs_move(mic->objs, indi_mb->x, indi_mb->y);
             e_mod_move_evas_objs_resize(mic->objs, indi_mb->w, indi_mb->h);
             e_mod_move_border_shape_input_rect_set(target_mb,
                                                    indi_mb->x,
                                                    indi_mb->y,
                                                    indi_mb->w,
                                                    indi_mb->h);
          }
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
        e_mod_move_event_click_set(meo->event, EINA_TRUE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indi_ctl_cb_motion_move(void *data,
                                    void *event_info)
{
   E_Move *m = NULL;
   E_Move_Indicator_Controller *mic = (E_Move_Indicator_Controller *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Evas_Object *meo = NULL;
   E_Move_Border *indi_mb = NULL;
   E_Move_Border *target_mb = NULL;
   Eina_List *l;
   Eina_Bool click = EINA_FALSE;
   int angle = 0;
   int d0, d1;
   E_Zone *zone = NULL;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();
   if (!mic || !info || !m) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s  (%d,%d) %s()\n", "EVAS_OBJ",
     info->coord.x, info->coord.y, __func__);

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
       click = e_mod_move_event_click_get(meo->event);
     }
   E_CHECK_RETURN(click, EINA_FALSE);

   // check indicator and if indicator does not exist, then destroy data, and hide
   target_mb = e_mod_move_border_client_find(mic->target_win);
   if (!target_mb)
     {
        // event clear
        _e_mod_move_indi_ctl_event_clear(mic);
        e_mod_move_evas_objs_del(mic->objs);
        memset(mic, 0, sizeof(E_Move_Indicator_Controller));
        E_FREE(mic);
        m->indicator_controller = NULL;
        return EINA_FALSE;
     }

   indi_mb = e_mod_move_indicator_find();
   if (!indi_mb)
     {
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
        angle = e_mod_move_event_angle_get(meo->event);
     }

   zone = indi_mb->bd->zone;
   d0 = d1 = 0;
   switch (angle)
     {
      case  90:
         d0 = info->coord.x;
         d1 = indi_mb->w*2;
         break;
      case 180:
         d0 = zone->h - (indi_mb->h*2);
         d1 = info->coord.y;
         break;
      case 270:
         d0 =  zone->w - (indi_mb->w*2);
         d1 = info->coord.x;
         break;
      case   0:
      default :
         d0 = info->coord.y;
         d1 = indi_mb->h*2;
         break;
     }

   if (d0 > d1)
     {
        e_border_show(indi_mb->bd);

        e_mod_move_evas_objs_move(mic->objs, target_mb->x, target_mb->y);
        e_mod_move_evas_objs_resize(mic->objs, target_mb->w, target_mb->h);
        e_mod_move_border_shape_input_rect_set(target_mb,
                                               0,
                                               0,
                                               target_mb->w,
                                               target_mb->h);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indi_ctl_cb_motion_end(void *data,
                                   void *event_info)
{
   E_Move *m = NULL;
   E_Move_Indicator_Controller *mic = (E_Move_Indicator_Controller *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Evas_Object *meo = NULL;
   E_Move_Border *indi_mb = NULL;
   E_Move_Border *target_mb = NULL;
   Eina_List *l;
   Eina_Bool click = EINA_FALSE;
   int angle = 0;
   int d0, d1;
   E_Zone *zone = NULL;
   Eina_Bool ret = EINA_FALSE;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();
   if (!mic || !info || !m) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s  (%d,%d) %s()\n", "EVAS_OBJ",
     info->coord.x, info->coord.y, __func__);

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
       click = e_mod_move_event_click_get(meo->event);
     }
   E_CHECK_RETURN(click, EINA_FALSE);

   // check indicator and if indicator does not exist, then destroy data, and hide
   target_mb = e_mod_move_border_client_find(mic->target_win);
   if (!target_mb)
     {
        // event clear
        _e_mod_move_indi_ctl_event_clear(mic);
        e_mod_move_evas_objs_del(mic->objs);
        memset(mic, 0, sizeof(E_Move_Indicator_Controller));
        E_FREE(mic);
        m->indicator_controller = NULL;
        return EINA_FALSE;
     }

   indi_mb = e_mod_move_indicator_find();
   E_CHECK_GOTO(indi_mb, finish);

   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
        angle = e_mod_move_event_angle_get(meo->event);
     }

   zone = indi_mb->bd->zone;
   d0 = d1 = 0;
   switch (angle)
     {
      case  90:
         d0 = info->coord.x;
         d1 = indi_mb->w*2;
         break;
      case 180:
         d0 = zone->h - (indi_mb->h*2);
         d1 = info->coord.y;
         break;
      case 270:
         d0 =  zone->w - (indi_mb->w*2);
         d1 = info->coord.x;
         break;
      case   0:
      default :
         d0 = info->coord.y;
         d1 = indi_mb->h*2;
         break;
     }

   if (d0 > d1)
     {
        if (!indi_mb->visible)
          {
             e_border_show(indi_mb->bd);
             e_mod_move_evas_objs_move(mic->objs, target_mb->x, target_mb->y);
             e_mod_move_evas_objs_resize(mic->objs, target_mb->w, target_mb->h);
             e_mod_move_border_shape_input_rect_set(target_mb,
                                                    0,
                                                    0,
                                                    target_mb->w,
                                                    target_mb->h);
             ret = EINA_TRUE;
          }
     }
   else
     {
        if (indi_mb->visible)
          {
             e_border_hide(indi_mb->bd,2);
             //e_mod_move_indicator_controller_unset(m);
             e_mod_move_evas_objs_move(mic->objs, indi_mb->x, indi_mb->y);
             e_mod_move_evas_objs_resize(mic->objs, indi_mb->w, indi_mb->h);
             e_mod_move_border_shape_input_rect_set(target_mb,
                                                    indi_mb->x,
                                                    indi_mb->y,
                                                    indi_mb->w,
                                                    indi_mb->h);
          }
     }

finish:
   EINA_LIST_FOREACH(mic->objs, l, meo)
     {
        if (!meo) continue;
        e_mod_move_event_click_set(meo->event, EINA_FALSE);
     }
   return ret;
}

static void
_e_mod_move_indi_ctl_obj_event_setup(E_Move_Indicator_Controller *mic,
                                     E_Move_Evas_Object          *meo)
{
   E_CHECK(mic);
   E_CHECK(meo);

   meo->event = e_mod_move_event_new(mic->target_win, meo->obj);
   E_CHECK(meo->event);

   e_mod_move_event_angle_cb_set(meo->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_cb_set(meo->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_indi_ctl_cb_motion_start, mic);
   e_mod_move_event_cb_set(meo->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_indi_ctl_cb_motion_move, mic);
   e_mod_move_event_cb_set(meo->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_indi_ctl_cb_motion_end, mic);
   e_mod_move_event_send_all_set(meo->event, EINA_TRUE);
}

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_move_indicator_controller_set(E_Move_Border *target_mb)
{
   E_Move *m = NULL;
   E_Move_Border *indi_mb = NULL;
   Ecore_X_Window win;
   E_Move_Indicator_Controller *mic = NULL;
   E_Move_Evas_Object *meo = NULL;
   Eina_List *l;

   E_CHECK_RETURN(target_mb, EINA_FALSE);
   indi_mb = e_mod_move_indicator_find();
   E_CHECK_RETURN(indi_mb, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   if (e_mod_move_indicator_controller_state_get(m, &win))
     e_mod_move_indicator_controller_unset(m);

   mic = E_NEW(E_Move_Indicator_Controller, 1);
   E_CHECK_RETURN(mic, EINA_FALSE);

   mic->target_win = target_mb->bd->client.win;
   mic->objs = e_mod_move_evas_objs_add(m);
   if (mic->objs)
     {
        e_mod_move_evas_objs_move(mic->objs, indi_mb->x, indi_mb->y);
        e_mod_move_evas_objs_resize(mic->objs, indi_mb->w, indi_mb->h);
        e_mod_move_evas_objs_layer_set(mic->objs, EVAS_LAYER_MAX - 1);
        e_mod_move_evas_objs_color_set(mic->objs, 0, 0, 0, 0);
        e_mod_move_evas_objs_show(mic->objs);
        e_mod_move_evas_objs_raise(mic->objs);

        // Set Input Shape Mask
        if (e_mod_move_border_shape_input_new(target_mb))
          {
             e_mod_move_border_shape_input_rect_set(target_mb,
                                                    indi_mb->x,
                                                    indi_mb->y,
                                                    indi_mb->w,
                                                    indi_mb->h);
          }

        // Set Event Handler
        EINA_LIST_FOREACH(mic->objs, l, meo)
          {
             if (!meo) continue;
             _e_mod_move_indi_ctl_obj_event_setup(mic, meo);
          }

        m->indicator_controller = mic;
     }
   else
     {
        memset(mic, 0, sizeof(E_Move_Indicator_Controller));
        E_FREE(mic);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_controller_unset(E_Move *m)
{
   Ecore_X_Window win;
   E_Move_Indicator_Controller *mic = NULL;
   E_Move_Border *target_mb = NULL;
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_indicator_controller_state_get(m, &win),
                  EINA_FALSE);

   mic = m->indicator_controller;
   E_CHECK_RETURN(mic, EINA_FALSE);

   _e_mod_move_indi_ctl_event_clear(mic);
   e_mod_move_evas_objs_del(mic->objs);

   // input shape mask clear
   if ((target_mb = e_mod_move_border_client_find(mic->target_win)))
     {
        if (target_mb->shape_input)
          {
             e_mod_move_border_shape_input_rect_set(target_mb, 0, 0, 0, 0);
             e_mod_move_border_shape_input_free(target_mb);
          }
     }

   memset(mic, 0, sizeof(E_Move_Indicator_Controller));
   E_FREE(mic);
   m->indicator_controller = NULL;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_controller_update(E_Move *m)
{
   Ecore_X_Window win;
   E_Move_Indicator_Controller *mic = NULL;
   E_Move_Border *indi_mb = NULL;
   E_Move_Border *target_mb = NULL;
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_indicator_controller_state_get(m, &win),
                  EINA_FALSE);

   mic = m->indicator_controller;
   E_CHECK_RETURN(mic, EINA_FALSE);

   indi_mb = e_mod_move_indicator_find();
   E_CHECK_RETURN(indi_mb, EINA_FALSE);
   if (indi_mb->visible) return EINA_FALSE;

   e_mod_move_evas_objs_move(mic->objs, indi_mb->x, indi_mb->y);
   e_mod_move_evas_objs_resize(mic->objs, indi_mb->w, indi_mb->h);

   if ((target_mb = e_mod_move_border_client_find(mic->target_win)))
     {
        e_mod_move_border_shape_input_rect_set(target_mb,
                                               indi_mb->x,
                                               indi_mb->y,
                                               indi_mb->w,
                                               indi_mb->h);
     }
   else
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x  %s %s()\n", "EVAS_OBJ",
          mic->target_win,
          "Indicator Controller Target Window doest not exist!!!!!",
          __func__);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_controller_state_get(E_Move         *m,
                                          Ecore_X_Window *win)
{
   E_Move_Indicator_Controller *mic = NULL;
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(win, EINA_FALSE);

   mic = m->indicator_controller;
   E_CHECK_RETURN(mic, EINA_FALSE);

   *win = mic->target_win;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_controller_set_policy_check(E_Move_Border *target_mb)
{
   E_Move_Border *indi_mb = NULL;
   E_Zone *zone = NULL;
   E_CHECK_RETURN(target_mb, EINA_FALSE);
   if (TYPE_QUICKPANEL_CHECK(target_mb)) return EINA_FALSE;
   if (TYPE_APPTRAY_CHECK(target_mb)) return EINA_FALSE;
   if (TYPE_INDICATOR_CHECK(target_mb)) return EINA_FALSE;

   if (target_mb->visibility != E_MOVE_VISIBILITY_STATE_VISIBLE)
     return EINA_FALSE;

   indi_mb = e_mod_move_indicator_find();
   E_CHECK_RETURN(indi_mb, EINA_FALSE);

   //if (indi_mb->visible)
   if (target_mb->indicator_state != E_MOVE_INDICATOR_STATE_OFF)
      return EINA_FALSE;

   zone = indi_mb->bd->zone;
   if (target_mb->bd->zone != zone)
     return EINA_FALSE;

   if ( (zone->x != target_mb->x)
        || (zone->y != target_mb->y)
        || (zone->w != target_mb->w)
        || (zone->h != target_mb->h))
      return EINA_FALSE;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_controller_unset_policy_check(E_Move_Border *target_mb)
{
   E_Move *m = NULL;
   Ecore_X_Window win;
   Eina_Bool ret = EINA_FALSE;
   E_CHECK_RETURN(target_mb, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   if (e_mod_move_indicator_controller_state_get(m, &win))
     {
        if (win == target_mb->bd->client.win)
           ret = EINA_TRUE;
     }

   return ret;
}
