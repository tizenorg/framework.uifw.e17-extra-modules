#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

struct _E_Move_Indicator_Data
{
   Eina_Bool quickpanel_move;
   Eina_Bool apptray_move;
};

/* local subsystem functions */
static E_Move_Event_State _e_mod_move_indicator_cb_motion_check(E_Move_Event *ev, void *data);
static Eina_Bool          _e_mod_move_indicator_cb_motion_start(void *data, void *event_info);
static Eina_Bool          _e_mod_move_indicator_cb_motion_move(void *data, void *event_info);
static Eina_Bool          _e_mod_move_indicator_cb_motion_end(void *data, void *event_info);
static Eina_Bool          _e_mod_move_indicator_quickpanel_flick_process(E_Move_Border *mb, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool          _e_mod_move_indicator_apptray_flick_process(E_Move_Border *mb, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool          _e_mod_move_indicator_cb_motion_start_internal_apptray_check(E_Move_Border *at_mb);
static Eina_Bool          _e_mod_move_indicator_cb_motion_start_internal_quickpanel_check(E_Move_Border *qp_mb);
static Eina_Bool          _e_mod_move_indicator_apptray_move_set(E_Move_Border *mb, Eina_Bool state);
static Eina_Bool          _e_mod_move_indicator_quickpanel_move_set(E_Move_Border *mb, Eina_Bool state);
static Eina_Bool          _e_mod_move_indicator_apptray_move_get(E_Move_Border *mb);
static Eina_Bool          _e_mod_move_indicator_quickpanel_move_get(E_Move_Border *mb);

/* local subsystem functions */
static E_Move_Event_State
_e_mod_move_indicator_cb_motion_check(E_Move_Event *ev,
                                      void         *data)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   Eina_List *l, *ll;
   E_Move_Event_Motion_Info *m0, *m1;
   unsigned int cnt = 0;
   E_Move_Event_State res = E_MOVE_EVENT_STATE_PASS;
   int w, h, angle, min_len, min_cnt = 2;

   if (!ev || !mb) goto finish;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n", "EVAS_OBJ", mb->bd->win, __func__);

   res = e_mod_move_event_state_get(ev);
   angle = e_mod_move_event_angle_get(ev);

   l = e_mod_move_event_ev_queue_get(ev);
   if (!l) goto finish;

   cnt = eina_list_count(l);
   if (cnt < min_cnt) goto finish;

   m0 = eina_list_data_get(l);
   if (!m0) goto finish;

   ll = eina_list_last(l);
   if (!ll) goto finish;

   m1 = eina_list_data_get(ll);
   if (!m1) goto finish;

   if ((m1->cb_type == EVAS_CALLBACK_MOUSE_UP) &&
       (res == E_MOVE_EVENT_STATE_CHECK))
     {
        return E_MOVE_EVENT_STATE_PASS;
     }

   w = abs(m1->coord.x - m0->coord.x);
   h = abs(m1->coord.y - m0->coord.y);
   min_len = (mb->w > mb->h) ? (mb->h * 2) : (mb->w * 2);

   if ((w < min_len) && (h < min_len))
     {
        goto finish;
     }

   switch (angle)
     {
      case  90:
      case 270:
        if (w < h) res = E_MOVE_EVENT_STATE_PASS;
        else res = E_MOVE_EVENT_STATE_HOLD;
        break;
      case 180:
      case   0:
      default :
        if (w > h) res = E_MOVE_EVENT_STATE_PASS;
        else res = E_MOVE_EVENT_STATE_HOLD;
        break;
     }

finish:
   return res;
}

static Eina_Bool
_e_mod_move_indicator_cb_motion_start(void *data,
                                      void *event_info)
{
   E_Move *m = NULL;
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Border *qp_mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;
   int region = 0;
   Eina_Bool apptray_show = EINA_FALSE;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   if (!m || !mb || !info) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x INDI_MOTION_START (%4d,%4d)\n",
     "EVAS_OBJ", mb->bd->win,
     info->coord.x, info->coord.y);

   E_CHECK_RETURN(e_mod_move_indicator_scrollable_check(), EINA_FALSE);

   _e_mod_move_indicator_apptray_move_set(mb, EINA_FALSE);
   _e_mod_move_indicator_quickpanel_move_set(mb, EINA_FALSE);

   /* check if apptray or quickpanel exists on the current zone */
   at_mb = e_mod_move_apptray_find();
   if ((at_mb) &&
       (REGION_INSIDE_ZONE(at_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x INDI_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "apptray exists. return.");
        return EINA_FALSE;
     }

   qp_mb = e_mod_move_quickpanel_find();
   if ((qp_mb) &&
       (REGION_INSIDE_ZONE(qp_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x INDI_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "quickpanel exists. return.");
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        angle = e_mod_move_event_angle_get(mco->event);
     }
   mb->angle = angle;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_TRUE);
     }

   E_CHECK_GOTO(e_mod_move_flick_data_new(mb), error_cleanup);
   e_mod_move_flick_data_init(mb, info->coord.x, info->coord.y);

   switch (angle)
     {
      case   0:
         region = m->indicator_home_region_ratio * mb->w;
         if (info->coord.x < region) apptray_show = EINA_TRUE;
         break;
      case  90:
         region = mb->h - (m->indicator_home_region_ratio * mb->h);
         if (info->coord.y > region) apptray_show = EINA_TRUE;
         break;
      case 180:
         region = mb->w - (m->indicator_home_region_ratio * mb->w);
         if (info->coord.x > region) apptray_show = EINA_TRUE;
         break;
      case 270:
         region = m->indicator_home_region_ratio * mb->h;
         if (info->coord.y < region) apptray_show = EINA_TRUE;
         break;
      default :
         fprintf(stderr,
                 "[E17-MOVE] Error1 invalid angle:%d win:0x%08x\n",
                 angle, mb->bd->win);
         goto error_cleanup;
         break;
     }

   if (apptray_show)
     {
        if (!_e_mod_move_indicator_cb_motion_start_internal_apptray_check(at_mb))
          {
             goto error_cleanup;
          }
        e_mod_move_apptray_e_border_raise(at_mb);
        _e_mod_move_indicator_apptray_move_set(mb, EINA_TRUE);
        e_mod_move_apptray_objs_animation_start_position_set(at_mb,
                                                             angle);
     }
   else
     {
        if (!_e_mod_move_indicator_cb_motion_start_internal_quickpanel_check(qp_mb))
          {
             goto error_cleanup;
          }
        _e_mod_move_indicator_quickpanel_move_set(mb, EINA_TRUE);
        e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                angle);
     }

   return EINA_TRUE;

error_cleanup:

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }
   _e_mod_move_indicator_apptray_move_set(mb, EINA_FALSE);
   _e_mod_move_indicator_quickpanel_move_set(mb, EINA_FALSE);

   return EINA_FALSE;
}

static Eina_Bool
_e_mod_move_indicator_cb_motion_move(void *data,
                                     void *event_info)
{
   E_Move *m = NULL;
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Border *qp_mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   E_Zone *zone = NULL;
   Eina_List *l;
   int angle = 0;
   Eina_Bool click = EINA_FALSE;
   Eina_Bool need_move = EINA_FALSE;
   Eina_Bool contents = EINA_FALSE;
   int cx, cy, cw, ch;
   int x = 0, y = 0;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   if (!m || !mb || !info) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x INDI_MOTION_MOVE a:%d (%4d,%4d)\n",
     "EVAS_OBJ", mb->bd->win, mb->angle,
     info->coord.x, info->coord.y);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }

   E_CHECK_RETURN(click, EINA_FALSE);

   if (_e_mod_move_indicator_quickpanel_move_get(mb))
     {
        qp_mb = e_mod_move_quickpanel_find();
        E_CHECK_RETURN(qp_mb, EINA_FALSE);

        contents = e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch);

        switch (angle)
          {
           case   0:
              if (info->coord.y < qp_mb->h)
                {
                   if (contents)
                     {
                        if (info->coord.y < ch)
                          {
                             y = info->coord.y - ch;
                             need_move = EINA_TRUE;
                          }
                     }
                   else
                     {
                        y = info->coord.y - qp_mb->h;
                        need_move = EINA_TRUE;
                     }
                }
              break;
           case  90:
              if (info->coord.x < qp_mb->w)
                {
                   if (contents)
                     {
                        if (info->coord.x < cw)
                          {
                             x = info->coord.x - cw;
                             need_move = EINA_TRUE;
                          }
                     }
                   else
                     {
                        x = info->coord.x - qp_mb->w;
                        need_move = EINA_TRUE;
                     }
                }
              break;
           case 180:
              if (info->coord.y > (zone->h - qp_mb->h))
                {
                   if (contents)
                     {
                        if (info->coord.y > cy)
                          {
                             y = info->coord.y - cy;
                             need_move = EINA_TRUE;
                          }
                     }
                   else
                     {
                        y = info->coord.y;
                        need_move = EINA_TRUE;
                     }
                }
              break;
           case 270:
              if (info->coord.x > (zone->w - qp_mb->w))
                {
                   if (contents)
                     {
                        if (info->coord.x > cx)
                          {
                             x = info->coord.x - cx;
                             need_move = EINA_TRUE;
                          }
                     }
                   else
                     {
                        x = info->coord.x;
                        need_move = EINA_TRUE;
                     }
                }
              break;
           default :
              break;
          }

        if (need_move)
          e_mod_move_quickpanel_objs_move(qp_mb, x, y);
     }
   else if (_e_mod_move_indicator_apptray_move_get(mb))
     {
        at_mb = e_mod_move_apptray_find();
        E_CHECK_RETURN(at_mb, EINA_FALSE);

        switch (angle)
          {
           case   0:
              if (info->coord.y < at_mb->h)
                {
                   y = info->coord.y - at_mb->h;
                   need_move = EINA_TRUE;
                }
              break;
           case  90:
              if (info->coord.x < at_mb->w)
                {
                   x = info->coord.x - at_mb->w;
                   need_move = EINA_TRUE;
                }
              break;
           case 180:
              if (info->coord.y > (zone->h - at_mb->h))
                {
                   y = info->coord.y;
                   need_move = EINA_TRUE;
                }
              break;
           case 270:
              if (info->coord.x > (zone->w - at_mb->w))
                {
                   x = info->coord.x;
                   need_move = EINA_TRUE;
                }
              break;
           default :
              break;
          }
        if (need_move)
          e_mod_move_apptray_objs_move(at_mb, x, y);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_cb_motion_end(void *data,
                                    void *event_info)
{
   E_Move *m = NULL;
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Border *qp_mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;
   Eina_Bool click = EINA_FALSE;
   Eina_Bool flick_state = EINA_FALSE;
   E_Zone *zone;
   int cx, cy, cw, ch;
   int check_h, check_w;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   if (!m || !mb || !info) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x ,angle:%d, (%d,%d)  %s()\n",
     "EVAS_OBJ", mb->bd->win, mb->angle, info->coord.x, info->coord.y,
     __func__);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }

   E_CHECK_GOTO(click, error_cleanup);

   e_mod_move_flick_data_update(mb, info->coord.x, info->coord.y);
   flick_state = e_mod_move_flick_state_get(mb);

   if (_e_mod_move_indicator_quickpanel_move_get(mb))
     {
        qp_mb = e_mod_move_quickpanel_find();
        if (_e_mod_move_indicator_quickpanel_flick_process(mb, qp_mb,
                                                           angle, flick_state))
          {
             return EINA_TRUE;
          }
        
     }
   if (_e_mod_move_indicator_apptray_move_get(mb))
     {
        at_mb = e_mod_move_apptray_find();
        if (_e_mod_move_indicator_apptray_flick_process(mb, at_mb,
                                                        angle, flick_state))
          {
             return EINA_TRUE;
          }
     }

   switch (angle)
     {
      case   0:
         if (at_mb)
           {
              check_h = at_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y < check_h)
                {
                   e_mod_move_apptray_e_border_move(at_mb, 0, at_mb->h * -1);
                   e_mod_move_apptray_objs_animation_move(at_mb, 0, at_mb->h * -1);
                }
              else
                {
                   e_mod_move_apptray_e_border_move(at_mb, 0, 0);
                   e_mod_move_apptray_objs_animation_move(at_mb, 0, 0);
                }
           }
         if (qp_mb)
           {
              check_h = qp_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y < check_h)
                {
                   if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                     {
                        check_h = ch;
                        if (check_h) check_h /= 2;
                        if (info->coord.y < check_h)
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, 0, qp_mb->h * -1);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, ch * -1);
                          }
                        else
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                          }
                     }
                }
               else
                {
                   e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                   e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                }
           }
         break;
      case  90:
         if (at_mb)
           {
              check_w = at_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x < check_w)
                {
                   e_mod_move_apptray_e_border_move(at_mb, at_mb->w * -1, 0);
                   e_mod_move_apptray_objs_animation_move(at_mb, at_mb->w * -1, 0);
                }
               else
                {
                   e_mod_move_apptray_e_border_move(at_mb, 0, 0);
                   e_mod_move_apptray_objs_animation_move(at_mb, 0, 0);
                }
           }
         if (qp_mb)
           {
              check_w = qp_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x < check_w)
                {
                   if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                     {
                        check_w = cw;
                        if (check_w) check_w /= 2;
                        if (info->coord.x < check_w)
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, qp_mb->w * -1, 0);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, cw * -1, 0);
                          }
                        else
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                          }
                     }
                }
               else
                {
                   e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                   e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                }
           }
         break;
      case 180:
         if (at_mb)
           {
              check_h = at_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y > (zone->h - check_h))
                {
                   e_mod_move_apptray_e_border_move(at_mb, 0, zone->h);
                   e_mod_move_apptray_objs_animation_move(at_mb, 0, zone->h);
                }
               else
                {
                   e_mod_move_apptray_e_border_move(at_mb, 0, (zone->h - at_mb->h));
                   e_mod_move_apptray_objs_animation_move(at_mb, 0, (zone->h - at_mb->h));
                }
           }
         if (qp_mb)
           {
              check_h = qp_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y > (zone->h - check_h))
                {
                   if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                     {
                        check_h = ch;
                        if (check_h) check_h /= 2;
                        if (info->coord.y > (cy + check_h))
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, 0, zone->h);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, ch);
                          }
                        else
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                          }
                     }
                }
               else
                {
                   e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                   e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                }
           }
         break;
      case 270:
         if (at_mb)
           {
              check_w = at_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x > (zone->w - check_w))
                {
                   e_mod_move_apptray_e_border_move(at_mb, zone->w, 0);
                   e_mod_move_apptray_objs_animation_move(at_mb, zone->w, 0);
                }
               else
                {
                   e_mod_move_apptray_e_border_move(at_mb, zone->w - at_mb->w, 0);
                   e_mod_move_apptray_objs_animation_move(at_mb, zone->w - at_mb->w, 0);
                }
           }
         if (qp_mb)
           {
              check_w = qp_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x > (zone->w - check_w))
                {
                   if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                     {
                        check_w = cw;
                        if (check_w) check_w /= 2;
                        if (info->coord.x > (cx + check_w))
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, zone->w, 0);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, cw, 0);
                          }
                        else
                          {
                             e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                             e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                          }
                     }
                }
               else
                {
                   e_mod_move_quickpanel_e_border_move(qp_mb, 0, 0);
                   e_mod_move_quickpanel_objs_animation_move(qp_mb, 0, 0);
                }
           }
         break;
      default :
         break;
     }

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }


error_cleanup:
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   _e_mod_move_indicator_apptray_move_set(mb, EINA_FALSE);
   _e_mod_move_indicator_quickpanel_move_set(mb, EINA_FALSE);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_quickpanel_flick_process(E_Move_Border *mb,  // mb: indicator
                                               E_Move_Border *mb2, // mb2 : quickpanel
                                               int            angle,
                                               Eina_Bool      state)
{
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   E_Zone *zone = NULL;
   int x = 0;
   int y = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco) // indicator click unset
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   _e_mod_move_indicator_apptray_move_set(mb, EINA_FALSE);
   _e_mod_move_indicator_quickpanel_move_set(mb, EINA_FALSE);
   
   // flick data free
   if (mb->flick_data) e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90:
         x = 0;
         y = 0;
         break;
      case 180:
         x = 0;
         y = zone->h - mb2->h;
         break;
      case 270:
         x = zone->w - mb2->w;
         y = 0;
         break;
      case   0:
      default :
         x = 0;
         y = 0;
         break;
     }

   e_mod_move_quickpanel_e_border_move(mb2, x, y);
   e_mod_move_quickpanel_objs_animation_move(mb2, x, y);

    return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_apptray_flick_process(E_Move_Border *mb,  // mb: indicator
                                            E_Move_Border *mb2, // mb2 : apptray
                                            int            angle,
                                            Eina_Bool      state)
{
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   E_Zone *zone = NULL;
   int x = 0;
   int y = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco) // indicator click unset
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   _e_mod_move_indicator_apptray_move_set(mb, EINA_FALSE);
   _e_mod_move_indicator_quickpanel_move_set(mb, EINA_FALSE);

   // flick data free
   if (mb->flick_data) e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90:
         x = 0;
         y = 0;
         break;
      case 180:
         x = 0;
         y = zone->h - mb2->h;
         break;
      case 270:
         x = zone->w - mb2->w;
         y = 0;
         break;
      case   0:
      default :
         x = 0;
         y = 0;
         break;
     }

   e_mod_move_apptray_e_border_move(mb2, x, y);
   e_mod_move_apptray_objs_animation_move(mb2, x, y);

    return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_cb_motion_start_internal_apptray_check(E_Move_Border *at_mb)
{
   E_CHECK_RETURN(at_mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(at_mb), EINA_FALSE);
   E_CHECK_RETURN(at_mb->visible, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_util_compositor_object_visible_get(at_mb),
                  EINA_FALSE);
   if (e_mod_move_apptray_objs_animation_state_get(at_mb)) return EINA_FALSE;
   
   e_mod_move_apptray_dim_show(at_mb);
   e_mod_move_apptray_objs_add(at_mb);
   //climb up indicator
   e_mod_move_apptray_objs_raise(at_mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_cb_motion_start_internal_quickpanel_check(E_Move_Border *qp_mb)
{
   E_CHECK_RETURN(qp_mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(qp_mb), EINA_FALSE);
   E_CHECK_RETURN(qp_mb->visible, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_util_compositor_object_visible_get(qp_mb),
                  EINA_FALSE);

   if (e_mod_move_quickpanel_objs_animation_state_get(qp_mb)) return EINA_FALSE;

   e_mod_move_quickpanel_dim_show(qp_mb);
   e_mod_move_quickpanel_objs_add(qp_mb);
   
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_apptray_move_set(E_Move_Border *mb,
                                       Eina_Bool      state)
{
   E_Move_Indicator_Data *indi_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   indi_data = (E_Move_Indicator_Data *)mb->data;
   if (!indi_data)
     indi_data = (E_Move_Indicator_Data *)e_mod_move_indicator_internal_data_add(mb);
   E_CHECK_RETURN(indi_data, EINA_FALSE);
   indi_data->apptray_move = state;
   mb->data = indi_data;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_quickpanel_move_set(E_Move_Border *mb,
                                          Eina_Bool      state)
{
   E_Move_Indicator_Data *indi_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   indi_data = (E_Move_Indicator_Data *)mb->data;
   if (!indi_data)
     indi_data = (E_Move_Indicator_Data *)e_mod_move_indicator_internal_data_add(mb);
   E_CHECK_RETURN(indi_data, EINA_FALSE);
   indi_data->quickpanel_move = state;
   mb->data = indi_data;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_apptray_move_get(E_Move_Border *mb)
{
   E_Move_Indicator_Data *indi_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   indi_data = (E_Move_Indicator_Data *)mb->data;
   E_CHECK_RETURN(indi_data, EINA_FALSE);
   return indi_data->apptray_move;
}

static Eina_Bool
_e_mod_move_indicator_quickpanel_move_get(E_Move_Border *mb)
{
   E_Move_Indicator_Data *indi_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   indi_data = (E_Move_Indicator_Data *)mb->data;
   E_CHECK_RETURN(indi_data, EINA_FALSE);
   return indi_data->quickpanel_move;
}

/* externally accessible functions */
EINTERN void
e_mod_move_indicator_ctl_obj_event_setup(E_Move_Border         *mb,
                                         E_Move_Control_Object *mco)
{
   E_CHECK(mb);
   E_CHECK(mco);
   E_CHECK(TYPE_INDICATOR_CHECK(mb));

   mco->event = e_mod_move_event_new(mb->bd->client.win, mco->obj);
   E_CHECK(mco->event);

   e_mod_move_event_angle_cb_set(mco->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_check_cb_set(mco->event,
                                 _e_mod_move_indicator_cb_motion_check, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_indicator_cb_motion_start, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_indicator_cb_motion_move, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_indicator_cb_motion_end, mb);
   e_mod_move_event_send_all_set(mco->event, EINA_TRUE);

   if (e_mod_move_border_shape_input_new(mb))
     e_mod_move_border_shape_input_rect_set(mb, mb->x, mb->y, mb->w, mb->h);
}

EINTERN E_Move_Border *
e_mod_move_indicator_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_INDICATOR_CHECK(mb)) return mb;
     }
   return NULL;
}

EINTERN Eina_Bool
e_mod_move_indicator_click_get(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_indicator_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_indicator_event_clear(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_indicator_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   click = e_mod_move_indicator_click_get();
   E_CHECK_RETURN(click, EINA_FALSE);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_data_clear(mco->event);
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   _e_mod_move_indicator_apptray_move_set(mb, EINA_FALSE);
   _e_mod_move_indicator_quickpanel_move_set(mb, EINA_FALSE);
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   return EINA_TRUE;
}

EINTERN void*
e_mod_move_indicator_internal_data_add(E_Move_Border *mb)
{
   E_Move_Indicator_Data *indi_data = NULL;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), NULL);
   indi_data = (E_Move_Indicator_Data *)mb->data;
   if (!indi_data)
     {
        indi_data = E_NEW(E_Move_Indicator_Data, 1);
        E_CHECK_RETURN(indi_data, NULL);
        indi_data->quickpanel_move = EINA_FALSE;
        indi_data->apptray_move = EINA_FALSE;
        mb->data = indi_data;
     }
   return mb->data;
}

EINTERN Eina_Bool
e_mod_move_indicator_internal_data_del(E_Move_Border *mb)
{
   int angle;
   E_Move_Border *at_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   E_Zone        *zone = NULL;
   int x = 0; int y = 0;

   E_Move_Indicator_Data *indi_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   indi_data = (E_Move_Indicator_Data *)mb->data;

   angle = mb->angle;
   // if indicaor is crashed, then apptray or quickpanel's mirror object hide with animation
   if (_e_mod_move_indicator_quickpanel_move_get(mb))
     {
        qp_mb = e_mod_move_quickpanel_find();
        E_CHECK_GOTO(qp_mb, error_cleanup);
        zone = qp_mb->bd->zone;

        switch (angle)
          {
           case   0:
              x = 0;
              y = qp_mb->h * -1;
              break;
           case  90:
              x = qp_mb->w * -1;
              y = 0;
              break;
           case 180:
              x = 0;
              y = zone->h;
              break;
           case 270:
              x = zone->w;
              y = 0;
              break;
           default :
              x = 0;
              y = qp_mb->h * -1;
              break;
          }

        if (e_mod_move_quickpanel_objs_animation_state_get(qp_mb))
          {
             e_mod_move_quickpanel_objs_animation_stop(qp_mb);
             e_mod_move_quickpanel_objs_animation_clear(qp_mb);
          }
        e_mod_move_quickpanel_objs_add(qp_mb);
        e_mod_move_quickpanel_e_border_move(qp_mb, x, y);
        e_mod_move_quickpanel_objs_animation_move(qp_mb, x, y);
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s Indicator Crash: Hide QuickPanel %s():%d\n",
          "EVAS_OBJ", __func__, __LINE__);
     }

   if (_e_mod_move_indicator_apptray_move_get(mb))
     {
        at_mb = e_mod_move_apptray_find();
        E_CHECK_GOTO(at_mb, error_cleanup);
        zone = at_mb->bd->zone;

        switch (angle)
          {
           case   0:
              x = 0;
              y = at_mb->h * -1;
              break;
           case  90:
              x = at_mb->w * -1;
              y = 0;
              break;
           case 180:
              x = 0;
              y = zone->h;
              break;
           case 270:
              x = zone->w;
              y = 0;
              break;
           default :
              x = 0;
              y = at_mb->h * -1;
              break;
          }
        if (e_mod_move_apptray_objs_animation_state_get(at_mb))
          {
             e_mod_move_apptray_objs_animation_stop(at_mb);
             e_mod_move_apptray_objs_animation_clear(at_mb);
          }
        e_mod_move_apptray_objs_add(at_mb);
        //climb up indicator
        e_mod_move_apptray_objs_raise(at_mb);

        e_mod_move_apptray_e_border_move(at_mb, x, y);
        e_mod_move_apptray_objs_animation_move(at_mb, x, y);
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s Indicator Crash: Hide Apptray %s():%d\n",
          "EVAS_OBJ", __func__, __LINE__);
     }

error_cleanup:
   E_FREE(indi_data);
   mb->data = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_scrollable_check(void)
{
   E_Move_Border *lockscr_mb = NULL;
   E_Move_Border *taskmgr_mb = NULL;
   E_Move_Border *pwlock_mb = NULL;

   // if lockscreen is exist & visible, then do not show  apptray & quickpanel
   if ((lockscr_mb = e_mod_move_lockscreen_find()))
     {
        if (lockscr_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE)
          {
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s  %s %s()\n",
               "EVAS_OBJ","Lockscreen is exist.", __func__);
             return EINA_FALSE;
          }
     }

   // if taskmanage is exist & visible, then do not show  apptray & quickpanel
   if ((taskmgr_mb = e_mod_move_taskmanager_find()))
     {
        if (taskmgr_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE)
          {
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s  %s %s()\n",
               "EVAS_OBJ","TaskManager is exist.", __func__);
             return EINA_FALSE;
          }
     }

   // if pwlock is exist & visible, then do not show  apptray & quickpanel
   if ((pwlock_mb = e_mod_move_pwlock_find()))
     {
        if (pwlock_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE)
          {
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s  %s %s()\n",
               "EVAS_OBJ","PWLOCK is exist.", __func__);
             return EINA_FALSE;
          }
     }

   return EINA_TRUE;
}

EINTERN E_Move_Event_Cb
e_mod_move_indicator_event_cb_get(E_Move_Event_Type type)
{
   if (type == E_MOVE_EVENT_TYPE_MOTION_START)
     return _e_mod_move_indicator_cb_motion_start;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_MOVE)
     return _e_mod_move_indicator_cb_motion_move;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_END)
     return _e_mod_move_indicator_cb_motion_end;
   else
     return NULL;
}

