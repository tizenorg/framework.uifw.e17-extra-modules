#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */
static Eina_Bool      _e_mod_move_indicator_widget_apptray_move_set(E_Move_Indicator_Widget *indi_widget, Eina_Bool state);
static Eina_Bool      _e_mod_move_indicator_widget_quickpanel_move_set(E_Move_Indicator_Widget *indi_widget, Eina_Bool state);
static Eina_Bool      _e_mod_move_indicator_widget_apptray_move_get(E_Move_Indicator_Widget *indi_widget);
static Eina_Bool      _e_mod_move_indicator_widget_quickpanel_move_get(E_Move_Indicator_Widget *indi_widget);
static Eina_Bool      _e_mod_move_indicator_widget_move_started_set(E_Move_Indicator_Widget *indi_widget, Eina_Bool state);
static Eina_Bool      _e_mod_move_indicator_widget_move_started_get(E_Move_Indicator_Widget *indi_widget);
static Eina_Bool      _e_mod_move_indicator_widget_cb_motion_start_internal_apptray_check(E_Move_Border *at_mb);
static Eina_Bool      _e_mod_move_indicator_widget_cb_motion_start_internal_quickpanel_check(E_Move_Border *qp_mb);
static Eina_Bool      _e_mod_move_indicator_widget_quickpanel_flick_process(E_Move_Indicator_Widget *indi_widget, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool      _e_mod_move_indicator_widget_apptray_flick_process(E_Move_Indicator_Widget *indi_widget, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool      _e_mod_move_indicator_widget_home_region_release_check(E_Move_Indicator_Widget  *indi_widget, Eina_Bool apptray_move, int angle, Evas_Point pos);
static Eina_Bool      _e_mod_move_indicator_widget_cb_motion_start(void *data, void *event_info);
static Eina_Bool      _e_mod_move_indicator_widget_cb_motion_move(void *data, void *event_info);
static Eina_Bool      _e_mod_move_indicator_widget_cb_motion_end(void *data, void *event_info);
static void           _e_mod_move_indicator_widget_obj_event_setup(E_Move_Indicator_Widget *indicator_widget, E_Move_Widget_Object *mwo);
static Eina_Bool      _e_mod_move_indicator_widget_scrollable_object_movable_check(E_Move_Indicator_Widget *indi_widget, E_Move_Border *mb, Evas_Point pos);
static Eina_Bool      _e_mod_move_indicator_widget_target_window_find_by_pointer(Ecore_X_Window *win, int x, int y);
static Ecore_X_Window _e_mod_move_indicator_widget_event_win_find(void *event_info);
static Eina_Bool      _e_mod_move_indicator_widget_target_window_policy_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_indicator_widget_event_send_policy_check(E_Move_Indicator_Widget *indi_widget, Evas_Point pos);
static void           _e_mod_move_indicator_widget_active_indicator_win_find_and_set(void);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_indicator_widget_apptray_move_set(E_Move_Indicator_Widget *indi_widget,
                                              Eina_Bool                state)
{
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   indi_widget->apptray_move = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_quickpanel_move_set(E_Move_Indicator_Widget *indi_widget,
                                                 Eina_Bool                state)
{
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   indi_widget->quickpanel_move = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_apptray_move_get(E_Move_Indicator_Widget *indi_widget)
{
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   return indi_widget->apptray_move;
}

static Eina_Bool
_e_mod_move_indicator_widget_quickpanel_move_get(E_Move_Indicator_Widget *indi_widget)
{
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   return indi_widget->quickpanel_move;
}

static Eina_Bool
_e_mod_move_indicator_widget_move_started_set(E_Move_Indicator_Widget *indi_widget,
                                              Eina_Bool                state)
{
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   indi_widget->move_started = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_move_started_get(E_Move_Indicator_Widget *indi_widget)
{
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   return indi_widget->move_started;
}

static Eina_Bool
_e_mod_move_indicator_widget_cb_motion_start_internal_apptray_check(E_Move_Border *at_mb)
{
   E_Move        *m;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(at_mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(at_mb), EINA_FALSE);
   E_CHECK_RETURN(at_mb->visible, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_util_compositor_object_visible_get(at_mb),
                  EINA_FALSE);
   if (e_mod_move_apptray_objs_animation_state_get(at_mb)) return EINA_FALSE;

   // check if notification window is on-screen.
   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (TYPE_INDICATOR_CHECK(find_mb)) continue;
        if (find_mb->visible
             && REGION_INTERSECTS_WITH_ZONE(find_mb, at_mb->bd->zone))
          {
              found = EINA_TRUE;
              break;
          }
     }
   if (found
        && (find_mb->bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NOTIFICATION))
     {
        return EINA_FALSE;
     }
   // check if notification window is on-screen.

   e_mod_move_apptray_dim_show(at_mb);
   e_mod_move_apptray_objs_add(at_mb);

   // apptray_objs_animation_layer_set
   e_mod_move_apptray_objs_animation_layer_set(at_mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_cb_motion_start_internal_quickpanel_check(E_Move_Border *qp_mb)
{
   E_Move_Border *mini_apptray_mb = NULL;

   E_CHECK_RETURN(qp_mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(qp_mb), EINA_FALSE);
   E_CHECK_RETURN(qp_mb->visible, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_util_compositor_object_visible_get(qp_mb),
                  EINA_FALSE);

   // Quickpanel is under rotation state.
   // I think there is another exception case.
   // It's posible that WM doesn't send rotation change request yet.
   // In this case the value of wait_for_done is zero,
   // it means quickpanel isn't rotating for now, but going to be rotated.
   if (qp_mb->bd)
     if (qp_mb->bd->client.e.state.rot.wait_for_done) return EINA_FALSE;

   if (e_mod_move_quickpanel_objs_animation_state_get(qp_mb)) return EINA_FALSE;

   mini_apptray_mb = e_mod_move_mini_apptray_find();
   if (e_mod_move_mini_apptray_objs_animation_state_get(mini_apptray_mb))
     return EINA_FALSE;

   if (!(qp_mb->m->qp_scroll_with_clipping))
     e_mod_move_quickpanel_dim_show(qp_mb);

   // Set Composite Mode & Rotation Lock & Make below win's mirror object
   e_mod_move_quickpanel_stage_init(qp_mb);

   e_mod_move_quickpanel_objs_add(qp_mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_quickpanel_flick_process(E_Move_Indicator_Widget  *indi_widget,
                                                      E_Move_Border            *mb2, // mb2 : quickpanel
                                                      int                       angle,
                                                      Eina_Bool                 state)
{
   E_Move                   *m = NULL;
   E_Move_Border            *mb = NULL;
   E_Move_Widget_Object     *mwo = NULL;
   Eina_List                *l;
   E_Zone                   *zone = NULL;
   int mx = 0, my = 0, ax = 0, ay = 0;

   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   m = mb->m;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo) // indicator click unset
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_FALSE);
   _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_FALSE);

   // flick data free
   if (mb->flick_data) e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90:
         mx = 0; my = 0;
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x + mb2->w;
              ay = zone->y + mb2->h;
           }
         else
           {
              ax = mx; ay = my;
           }
         break;
      case 180:
         mx = 0; my = zone->h - mb2->h;
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x; ay = zone->y;
           }
         else
           {
              ax = mx; ay = my;
           }
         break;
      case 270:
         mx = zone->w - mb2->w; my = 0;
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x; ay = zone->y;
           }
         else
           {
              ax = mx; ay = my;
           }
         break;
      case   0:
      default :
         mx = 0; my = 0;
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x + mb2->w;
              ay = zone->y + mb2->h;
           }
         else
           {
              ax = mx; ay = my;
           }
         break;
     }

   e_mod_move_quickpanel_e_border_move(mb2, mx, my);
   e_mod_move_quickpanel_objs_animation_move(mb2, ax, ay);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_apptray_flick_process(E_Move_Indicator_Widget *indi_widget,
                                                   E_Move_Border           *mb2, // mb2 : apptray
                                                   int                      angle,
                                                   Eina_Bool                state)
{
   E_Move_Border            *mb = NULL;
   E_Move_Widget_Object     *mwo = NULL;
   Eina_List                *l;
   E_Zone                   *zone = NULL;
   int x = 0;
   int y = 0;

   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo) // indicator click unset
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_FALSE);
   _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_FALSE);

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
_e_mod_move_indicator_widget_home_region_release_check(E_Move_Indicator_Widget *indi_widget,
                                                       Eina_Bool                apptray_move /* if home button is pressd, then apptray_move set true */,
                                                       int                      angle,
                                                       Evas_Point               pos /* mouse up position */)
{
   E_Move_Border *mb = NULL;
   int            region_check;
   Eina_Bool      ret = EINA_FALSE;
   E_Zone        *zone = NULL;
   E_Move        *m = NULL;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(apptray_move, EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);
   zone = mb->bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   switch (angle)
     {
      case   0:
         region_check = m->indicator_widget_geometry[E_MOVE_ANGLE_0].h;
         if (pos.y < region_check) ret = EINA_TRUE;
         break;
      case  90:
         region_check = m->indicator_widget_geometry[E_MOVE_ANGLE_90].w;
         if (pos.x < region_check) ret = EINA_TRUE;
         break;
      case 180:
         region_check = zone->h - m->indicator_widget_geometry[E_MOVE_ANGLE_180].h;
         if (pos.y > region_check) ret = EINA_TRUE;
         break;
      case 270:
         region_check = zone->w - m->indicator_widget_geometry[E_MOVE_ANGLE_270].w;
         if (pos.x > region_check) ret = EINA_TRUE;
         break;
      default :
         SL(LT_EVENT_OBJ,
            "[MOVE] ev:%15.15s , invalid angle:%d, (%d,%d)  %s()\n",
            "EVAS_OBJ", angle, pos.x, pos.y,
            __func__)
         break;
     }
   return ret;
}

static Eina_Bool
_e_mod_move_indicator_widget_cb_motion_start(void *data,
                                             void *event_info)
{
   E_Move *m = NULL;
   E_Move_Indicator_Widget *indi_widget = (E_Move_Indicator_Widget *)data;

   E_Move_Border *mb = NULL;
   E_Move_Border *ev_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Widget_Object *mwo = NULL;
   Evas_Event_Mouse_Down *mouse_down_event = NULL;
   Eina_Bool clicked = EINA_FALSE;
   Eina_List *l;
   E_Move_Scroll_Region_Indicator scroll_region = E_MOVE_SCROLL_REGION_NONE;
   Ecore_X_Window ev_win = 0;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(indi_widget, EINA_FALSE);

   // clicked window indicator policy check
   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        ev_win = e_mod_move_event_win_get(mwo->event);
     }
   ev_mb = e_mod_move_border_client_find(ev_win);
   E_CHECK_RETURN(_e_mod_move_indicator_widget_target_window_policy_check(ev_mb),
                  EINA_FALSE);

   mb = e_mod_move_border_client_find(indi_widget->win);

   if (!m || !mb || !indi_widget || !info) return EINA_FALSE;

   mouse_down_event = info->event_info;
   E_CHECK_RETURN(mouse_down_event, EINA_FALSE);
   if (mouse_down_event->button != 1)
     return EINA_FALSE;

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        clicked = e_mod_move_event_click_get(mwo->event);
     }
   if (clicked)
     return EINA_FALSE;

   SL(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x INDI_WIDGET_MOTION_START (%4d,%4d)\n",
     "EVAS_OBJ", mb->bd->win,
     info->coord.x, info->coord.y);

   _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_FALSE);
   _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_FALSE);

   /* check if apptray or quickpanel exists on the current zone */
   at_mb = e_mod_move_apptray_find();
   if ((at_mb) &&
       (REGION_INSIDE_ZONE(at_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x INDI_WIDGET_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "apptray exists. return.");
        return EINA_FALSE;
     }

   qp_mb = e_mod_move_quickpanel_find();
   if ((qp_mb) &&
       (REGION_INSIDE_ZONE(qp_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x INDI_WIDGET_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "quickpanel exists. return.");
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_TRUE);
     }

   E_CHECK_GOTO(e_mod_move_flick_data_new(mb), error_cleanup);
   e_mod_move_flick_data_init(mb, info->coord.x, info->coord.y);

   scroll_region = e_mod_move_indicator_region_scroll_check(mb->angle, info->coord);

   if (scroll_region == E_MOVE_SCROLL_REGION_APPTRAY)
     {
        if (e_mod_move_panel_scrollable_get(mb, E_MOVE_PANEL_TYPE_APPTRAY))
          {
             if (_e_mod_move_indicator_widget_cb_motion_start_internal_apptray_check(at_mb))
               {
                  e_mod_move_apptray_e_border_raise(at_mb);
                  _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_TRUE);
                  e_mod_move_apptray_objs_animation_start_position_set(at_mb,
                                                                       mb->angle);
                  // send apptray to "move start message".
                  e_mod_move_apptray_anim_state_send(at_mb, EINA_TRUE);
               }
          }
     }
   else if (scroll_region == E_MOVE_SCROLL_REGION_QUICKPANEL)
     {
        if (e_mod_move_panel_scrollable_get(mb, E_MOVE_PANEL_TYPE_QUICKPANEL))
          {
             if (_e_mod_move_indicator_widget_cb_motion_start_internal_quickpanel_check(qp_mb))
               {
                  _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_TRUE);
                  e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                          mb->angle);
                  // send quickpanel to "move start message".
                  e_mod_move_quickpanel_anim_state_send(qp_mb, EINA_TRUE);
               }
          }
     }

   indi_widget->pos = info->coord; // save mouse click position

   return EINA_TRUE;

error_cleanup:

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_FALSE);
   _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_FALSE);

   return EINA_FALSE;
}

static Eina_Bool
_e_mod_move_indicator_widget_cb_motion_move(void *data,
                                            void *event_info)
{
   E_Move                   *m = NULL;
   E_Move_Indicator_Widget  *indi_widget = (E_Move_Indicator_Widget *)data;
   E_Move_Border            *mb = NULL;
   E_Move_Border            *qp_mb = NULL;
   E_Move_Border            *at_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Widget_Object     *mwo = NULL;
   E_Zone                   *zone = NULL;
   Eina_List                *l;
   Eina_Bool                 click = EINA_FALSE;
   Eina_Bool                 need_move = EINA_FALSE;
   Eina_Bool                 contents = EINA_FALSE;

   int cx, cy, cw, ch;
   int x = 0, y = 0;
   int angle;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(indi_widget->win);

   if (!m || !mb || !info) return EINA_FALSE;

   SL(LT_EVENT_OBJ,
      "[MOVE] ev:%15.15s w:0x%08x INDI_WIDGET_MOTION_MOVE a:%d (%4d,%4d)\n",
      "EVAS_OBJ", mb->bd->win, mb->angle,
      info->coord.x, info->coord.y);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }

   E_CHECK_RETURN(click, EINA_FALSE);

   if (_e_mod_move_indicator_widget_quickpanel_move_get(indi_widget))
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

        if (_e_mod_move_indicator_widget_scrollable_object_movable_check(indi_widget, mb, info->coord))
          {
             if (m->qp_scroll_with_clipping)
                e_mod_move_quickpanel_objs_move(qp_mb,
                                                info->coord.x,
                                                info->coord.y);
             else
               {
                  if (need_move)
                     e_mod_move_quickpanel_objs_move(qp_mb, x, y);
               }
          }
     }
   else if (_e_mod_move_indicator_widget_apptray_move_get(indi_widget))
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

   indi_widget->pos = info->coord; // save mouse move position

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_indicator_widget_cb_motion_end(void *data,
                                           void *event_info)
{
   E_Move                   *m = NULL;
   E_Move_Indicator_Widget  *indi_widget = (E_Move_Indicator_Widget *)data;
   E_Move_Border            *mb = NULL;
   E_Move_Border            *qp_mb = NULL;
   E_Move_Border            *at_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Widget_Object     *mwo = NULL;
   Eina_List                *l;
   E_Zone                   *zone;
   Evas_Event_Mouse_Up      *mouse_up_event;
   Eina_Bool                 click = EINA_FALSE;
   Eina_Bool                 flick_state = EINA_FALSE;
   Eina_Bool                 qp_mv_state = EINA_FALSE;
   Eina_Bool                 at_mv_state = EINA_FALSE;

   int cx, cy, cw, ch;
   int check_h, check_w;
   int angle = 0;
   int mx = 0, my = 0, ax = 0, ay = 0;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(indi_widget->win);

   if (!m || !mb || !info) return EINA_FALSE;

   mouse_up_event = info->event_info;
   E_CHECK_RETURN(mouse_up_event, EINA_FALSE);
   if (mouse_up_event->button != 1)
     return EINA_FALSE;

   SL(LT_EVENT_OBJ,
      "[MOVE] ev:%15.15s w:0x%08x ,angle:%d, (%d,%d)  %s()\n",
      "EVAS_OBJ", mb->bd->win, mb->angle, info->coord.x, info->coord.y,
      __func__);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }
   E_CHECK_GOTO(click, error_cleanup);

   qp_mv_state = _e_mod_move_indicator_widget_quickpanel_move_get(indi_widget);
   at_mv_state = _e_mod_move_indicator_widget_apptray_move_get(indi_widget);
   if (!qp_mv_state && !at_mv_state) goto finish;

   e_mod_move_flick_data_update(mb, info->coord.x, info->coord.y);
   flick_state = e_mod_move_flick_state_get(mb, EINA_TRUE);

   if (qp_mv_state)
     {
        qp_mb = e_mod_move_quickpanel_find();
        if (_e_mod_move_indicator_widget_quickpanel_flick_process(indi_widget, qp_mb,
                                                                  angle, flick_state))
          {
             return EINA_TRUE;
          }
     }
   if (at_mv_state)
     {
        at_mb = e_mod_move_apptray_find();

        // if release position is on indicator's home button then, do not flick.
        if (_e_mod_move_indicator_widget_home_region_release_check(indi_widget,
                                                                   EINA_TRUE, /* upper if phrase check _e_mod_move_indicator_apptray_move_get(mb) value TURE or FALSE. */
                                                                   angle,
                                                                   info->coord))
          {
             flick_state = EINA_FALSE;
          }

        if (_e_mod_move_indicator_widget_apptray_flick_process(indi_widget, at_mb,
                                                               angle, flick_state))
          {
             return EINA_TRUE;
          }
     }

   mx = zone->x;
   my = zone->y;
   ax = mx;
   ay = my;

   switch (angle)
     {
      case   0:
         if (at_mb)
           {
              check_h = at_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y < check_h)
                {
                   my = at_mb->h * -1;
                   ay = my;
                }
           }
         if (qp_mb)
           {
              check_h = qp_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y < check_h)
                {
                   if (m->qp_scroll_with_clipping)
                     {
                        my = qp_mb->h * -1;
                     }
                   else
                     {
                        if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                          {
                             check_h = ch;
                             if (check_h) check_h /= 2;
                             if (info->coord.y < check_h)
                               {
                                  my = qp_mb->h * -1;
                                  ay = ch * -1;
                               }
                          }
                     }
                }
               else
                {
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w,
                        ay = zone->y + qp_mb->h;
                     }
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
                   mx = at_mb->w * -1;
                   ax = mx;
                }
           }
         if (qp_mb)
           {
              check_w = qp_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x < check_w)
                {
                   if (m->qp_scroll_with_clipping)
                     {
                        mx = qp_mb->w * -1;
                     }
                   else
                     {
                        if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                          {
                             check_w = cw;
                             if (check_w) check_w /= 2;
                             if (info->coord.x < check_w)
                               {
                                  mx = qp_mb->w * -1;
                                  ax = cw * -1;
                               }
                          }
                     }
                }
               else
                {
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
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
                   my = zone->h;
                   ay = my;
                }
               else
                {
                   my = zone->h - at_mb->h;
                   ay = my;
                }
           }
         if (qp_mb)
           {
              check_h = qp_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y > (zone->h - check_h))
                {
                   if (m->qp_scroll_with_clipping)
                     {
                        my = zone->h;
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                          {
                             check_h = ch;
                             if (check_h) check_h /= 2;
                             if (info->coord.y > (cy + check_h))
                               {
                                  my = zone->h;
                                  ay = ch;
                               }
                          }
                     }
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
                   mx = zone->w;
                   ax = mx;
                }
               else
                {
                   mx = zone->w - at_mb->w;
                   ax = mx;
                }
           }
         if (qp_mb)
           {
              check_w = qp_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x > (zone->w - check_w))
                {
                   if (m->qp_scroll_with_clipping)
                     {
                        mx = zone->w;
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                          {
                             check_w = cw;
                             if (check_w) check_w /= 2;
                             if (info->coord.x > (cx + check_w))
                               {
                                  mx = zone->w;
                                  ax = cw;
                               }
                          }
                     }
                }
           }
         break;
      default :
         break;
     }

   if (at_mb)
     {
        e_mod_move_apptray_e_border_move(at_mb, mx, my);
        e_mod_move_apptray_objs_animation_move(at_mb, ax, ay);
     }
   else if (qp_mb)
     {
        e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
        if (_e_mod_move_indicator_widget_scrollable_object_movable_check(indi_widget, mb, info->coord))
          e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
        else
          e_mod_move_quickpanel_objs_animation_move_with_time(qp_mb, ax, ay, 0.0000001);
        // time :0.0 calls animation_frame with pos : 0.0  calls once.
        // so I use small time value, it makes animation_frame with pos: 1.0 call once
     }

finish:
   indi_widget->pos = info->coord; // save mouse up position
   _e_mod_move_indicator_widget_move_started_set(indi_widget, EINA_FALSE);

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

error_cleanup:
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_FALSE);
   _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_FALSE);

   return EINA_TRUE;
}

static void
_e_mod_move_indicator_widget_obj_event_setup(E_Move_Indicator_Widget *indicator_widget,
                                             E_Move_Widget_Object    *mwo)
{
   E_CHECK(indicator_widget);
   E_CHECK(mwo);

   mwo->event = e_mod_move_event_new(indicator_widget->win, mwo->obj);
   E_CHECK(mwo->event);

   e_mod_move_event_data_type_set(mwo->event, E_MOVE_EVENT_DATA_TYPE_WIDGET_INDICATOR);
   e_mod_move_event_angle_cb_set(mwo->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_indicator_widget_cb_motion_start,
                           indicator_widget);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_indicator_widget_cb_motion_move,
                           indicator_widget);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_indicator_widget_cb_motion_end,
                           indicator_widget);
   e_mod_move_event_win_find_cb_set(mwo->event,
                                    _e_mod_move_indicator_widget_event_win_find);
   e_mod_move_event_propagate_type_set(mwo->event,
                                       E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY);
}

static Eina_Bool
_e_mod_move_indicator_widget_scrollable_object_movable_check(E_Move_Indicator_Widget *indicator_widget,
                                                             E_Move_Border           *mb,
                                                             Evas_Point              pos)
{
   E_Move *m = NULL;
   int check_val = 0;
   Eina_Bool ret = EINA_FALSE;
   Eina_Bool move_started = EINA_FALSE;
   Eina_Bool position_check = EINA_FALSE;

   E_CHECK_RETURN(indicator_widget, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   move_started = _e_mod_move_indicator_widget_move_started_get(indicator_widget);
   if (move_started) return EINA_TRUE;

   switch (mb->angle)
     {
      case 90:
         check_val = m->indicator_widget_geometry[E_MOVE_ANGLE_90].w;
         if (pos.x > check_val) position_check = EINA_TRUE;
         break;
      case 180:
         check_val = m->indicator_widget_geometry[E_MOVE_ANGLE_180].y;
         if (pos.y < check_val) position_check = EINA_TRUE;
         break;
      case 270:
         check_val = m->indicator_widget_geometry[E_MOVE_ANGLE_270].x;
         if (pos.x < check_val) position_check = EINA_TRUE;
         break;
      case 0:
      default:
         check_val = m->indicator_widget_geometry[E_MOVE_ANGLE_0].h;
         if (pos.y > check_val) position_check = EINA_TRUE;
         break;
     }

   if (position_check)
     {
         _e_mod_move_indicator_widget_move_started_set(indicator_widget, EINA_TRUE);
         ret = EINA_TRUE;
     }

   return ret;
}

static Eina_Bool
_e_mod_move_indicator_widget_target_window_find_by_pointer(Ecore_X_Window *win,
                                                           int x,
                                                           int y)
{
   E_Move        *m = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   Eina_Bool      ret = EINA_FALSE;
   Ecore_X_Window noti_win = 0;
   Eina_Bool      noti_win_saved = EINA_FALSE;

   E_CHECK_RETURN(win, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (!find_mb->bd) continue;

        // finding visible border
        if (!find_mb->visible) continue;

        // finding pointed border
        if (!E_INSIDE(x, y, find_mb->bd->x, find_mb->bd->y,
                            find_mb->bd->w, find_mb->bd->h))
          continue;

        // if notification , alpha, and indicator_state_none then search again below.
        if ((TYPE_NOTIFICATION_CHECK(find_mb) || TYPE_APP_SELECTOR_CHECK(find_mb))
            && (find_mb->argb)
            && (find_mb->indicator_state == E_MOVE_INDICATOR_STATE_NONE))
          {
             if (!noti_win_saved)
               {
                  noti_win = find_mb->bd->client.win;
                  noti_win_saved = EINA_TRUE;
               }
             continue;
          }
        else
          {
             found = EINA_TRUE;
             break;
          }
     }

   if (found)
     {
        if ((find_mb)
            && (find_mb->indicator_state == E_MOVE_INDICATOR_STATE_ON))
          {
             *win = find_mb->bd->client.win;
             ret = EINA_TRUE;
          }
        else
          {
             if (noti_win_saved)
               {
                  *win = noti_win;
                  ret = EINA_TRUE;
               }
             else
               {
                  *win = find_mb->bd->client.win;
                  ret = EINA_TRUE;
               }
          }
     }

   return ret;
}

static Ecore_X_Window
_e_mod_move_indicator_widget_event_win_find(void *event_info)
{
   E_Move_Event_Motion_Info *info = NULL;
   Ecore_X_Window            win = 0, res_win = 0;
   info  = (E_Move_Event_Motion_Info *)event_info;

   E_CHECK_RETURN(info, 0);

   if (_e_mod_move_indicator_widget_target_window_find_by_pointer(&win,
                                                                  info->coord.x,
                                                                  info->coord.y))
     {
         res_win = win;
     }

   SL(LT_EVENT_OBJ,
      "[MOVE] ev:%15.15s INDICATOR_WIDGET_EVENT_WIN_FIND w:0x%08x (%4d,%4d)\n",
      "EVAS_OBJ", res_win, info->coord.x, info->coord.y);

   return res_win;
}

static Eina_Bool
_e_mod_move_indicator_widget_target_window_policy_check(E_Move_Border *mb)
{
   E_Move        *m = e_mod_move_util_get();
   E_Zone        *zone = NULL;
   Eina_Bool      ret = EINA_FALSE;
   E_Border      *bd = NULL;

   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);

   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   if (REGION_EQUAL_TO_ZONE(mb, zone)  // check fullscreen
       && (zone->id == 0)              // change zone->id comparing to bd's profile property (mobile)
       && (mb->indicator_state == E_MOVE_INDICATOR_STATE_ON)
       && (mb->indicator_type == E_MOVE_INDICATOR_TYPE_1))
     {
         ret = EINA_TRUE;
     }

   return ret;
}

static Eina_Bool
_e_mod_move_indicator_widget_event_send_policy_check(E_Move_Indicator_Widget *indi_widget,
                                                     Evas_Point               pos)
{
   int x = 0, y = 0, w = 0, h = 0;
   Eina_Bool ret = EINA_FALSE;

   E_CHECK_RETURN(indi_widget, EINA_FALSE);

   e_mod_move_widget_objs_geometry_get(indi_widget->objs, &x ,&y, &w, &h);

   if (E_INSIDE(pos.x, pos.y, x, y, w, h)) ret = EINA_TRUE;

   return ret;
}

/* find active indicator window window and set property */
static void
_e_mod_move_indicator_widget_active_indicator_win_find_and_set(void)
{
   E_Move                  *m = NULL;
   Ecore_X_Window           target_win;
   E_Move_Border           *target_mb = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);

   if (e_mod_move_indicator_widget_target_window_find(&target_win))
     {
        target_mb = e_mod_move_border_client_find(target_win);
        E_CHECK(target_mb);
        if ((TYPE_NOTIFICATION_CHECK(target_mb) || TYPE_APP_SELECTOR_CHECK(target_mb))
            && (target_mb->argb)
            && (target_mb->indicator_state == E_MOVE_INDICATOR_STATE_NONE))
          {
             ;
          }
        else
           e_mod_move_util_prop_active_indicator_win_set(target_win, m);
     }
}

/* externally accessible functions */

/* set current indicator widget */
EINTERN void
e_mod_move_indicator_widget_set(E_Move_Indicator_Widget *indi_widget)
{
   E_Move *m = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);

   if (m->indicator_widget)
     {
        e_mod_move_indicator_widget_del(m->indicator_widget);
     }

   m->indicator_widget = indi_widget;
}

/* get current indicator widget */
EINTERN E_Move_Indicator_Widget *
e_mod_move_indicator_widget_get(void)
{
   E_Move *m = NULL;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, NULL);

   return m->indicator_widget;
}

/* find indicator widget target window */
EINTERN Eina_Bool
e_mod_move_indicator_widget_target_window_find(Ecore_X_Window *win)
{
   E_Move        *m = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   E_Zone        *zone = NULL;
   Eina_Bool      ret = EINA_FALSE;

   E_CHECK_RETURN(win, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   // fix later
   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (!find_mb->bd) continue;
        // the first OnScreen & FullScreen Window
        zone = find_mb->bd->zone;
        if (find_mb->visible
            && REGION_EQUAL_TO_ZONE(find_mb, zone)  // check fullscreen
            && (zone->id == 0)) // change zone->id comparing to bd's profile property (mobile)
          {
             found = EINA_TRUE;
             break;
          }
     }

   if (found
       && !(TYPE_APPTRAY_CHECK(find_mb))
       && !(TYPE_MINI_APPTRAY_CHECK(find_mb))
       && !(TYPE_QUICKPANEL_CHECK(find_mb)))
     {
        *win = find_mb->bd->client.win;
        ret = EINA_TRUE;
     }

   return ret;
}

/* find indicator widget's target window and apply indicator widget control */
EINTERN void
e_mod_move_indicator_widget_apply(void)
{
   E_Move                  *m = NULL;
   E_Move_Indicator_Widget *indi_widget = NULL;
   Ecore_X_Window           target_win;
   E_Move_Border           *target_mb = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);
   if ((m->screen_reader_state) || (m->setup_wizard_state))
     {
        _e_mod_move_indicator_widget_active_indicator_win_find_and_set();
        return;
     }

   if (e_mod_move_indicator_widget_target_window_find(&target_win))
     {
        target_mb = e_mod_move_border_client_find(target_win);
        // if previous indicator widget is created
        if ((indi_widget = e_mod_move_indicator_widget_get()))
          {
             // if current indicator widget's win is equal to finded win
             // then just return.
             if ((indi_widget->win == target_win)) return;
             else
               {
                  // if current indicator widget's win is not equal to finded win
                  // then del previous indicator_widget and add new indicator widget.
                  e_mod_move_indicator_widget_del(indi_widget);
                  e_mod_move_indicator_widget_set(e_mod_move_indicator_widget_add(target_win));
                  if ((target_mb) &&
                      (TYPE_NOTIFICATION_CHECK(target_mb) || TYPE_APP_SELECTOR_CHECK(target_mb)) &&
                      (target_mb->argb) &&
                      (target_mb->indicator_state == E_MOVE_INDICATOR_STATE_NONE))
                    {
                      ;
                    }
                  else
                    e_mod_move_util_prop_active_indicator_win_set(target_win, m);
               }
          }
        else
          {
             //if previous indicator widget is not creagted
             //then add new indicator widget.
             e_mod_move_indicator_widget_set(e_mod_move_indicator_widget_add(target_win));

             if ((target_mb) &&
                 (TYPE_NOTIFICATION_CHECK(target_mb) || TYPE_APP_SELECTOR_CHECK(target_mb)) &&
                 (target_mb->argb) &&
                 (target_mb->indicator_state == E_MOVE_INDICATOR_STATE_NONE))
               {
                  ;
               }
             else
               e_mod_move_util_prop_active_indicator_win_set(target_win, m);
          }
     }
   else
     {
        // if current window does not require indicator widget
        // and previous indicator widget is created,
        // then del previous indicator_widget
        if ((indi_widget = e_mod_move_indicator_widget_get()))
          {
             e_mod_move_indicator_widget_del(indi_widget);
             e_mod_move_indicator_widget_set(NULL);
          }
     }
}

/* create E_Move_Border related Indicator_Widget */
EINTERN E_Move_Indicator_Widget *
e_mod_move_indicator_widget_add(Ecore_X_Window win)
{
   E_Move                  *m = NULL;
   E_Move_Border           *mb = NULL;
   E_Move_Indicator_Widget *indi_widget = NULL;
   E_Move_Widget_Object    *mwo = NULL;
   Eina_List               *l;
   int                      x;
   int                      y;
   int                      w;
   int                      h;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = e_mod_move_border_client_find(win);
   E_CHECK_RETURN(mb, NULL);

   indi_widget = E_NEW(E_Move_Indicator_Widget, 1);
   E_CHECK_RETURN(indi_widget, NULL);

   indi_widget->win = win;
   indi_widget->objs = e_mod_move_widget_objs_add(m);
   if (indi_widget->objs)
     {
        switch (mb->angle)
          {
             case 90:
                x = m->indicator_widget_geometry[E_MOVE_ANGLE_90].x;
                y = m->indicator_widget_geometry[E_MOVE_ANGLE_90].y;
                w = m->indicator_widget_geometry[E_MOVE_ANGLE_90].w;
                h = m->indicator_widget_geometry[E_MOVE_ANGLE_90].h;
                break;
             case 180:
                x = m->indicator_widget_geometry[E_MOVE_ANGLE_180].x;
                y = m->indicator_widget_geometry[E_MOVE_ANGLE_180].y;
                w = m->indicator_widget_geometry[E_MOVE_ANGLE_180].w;
                h = m->indicator_widget_geometry[E_MOVE_ANGLE_180].h;
                break;
             case 270:
                x = m->indicator_widget_geometry[E_MOVE_ANGLE_270].x;
                y = m->indicator_widget_geometry[E_MOVE_ANGLE_270].y;
                w = m->indicator_widget_geometry[E_MOVE_ANGLE_270].w;
                h = m->indicator_widget_geometry[E_MOVE_ANGLE_270].h;
                break;
             case 0:
             default:
                x = m->indicator_widget_geometry[E_MOVE_ANGLE_0].x;
                y = m->indicator_widget_geometry[E_MOVE_ANGLE_0].y;
                w = m->indicator_widget_geometry[E_MOVE_ANGLE_0].w;
                h = m->indicator_widget_geometry[E_MOVE_ANGLE_0].h;
                break;
          }
        e_mod_move_widget_objs_move(indi_widget->objs, x, y);
        e_mod_move_widget_objs_resize(indi_widget->objs, w, h);
        e_mod_move_widget_objs_layer_set(indi_widget->objs, EVAS_LAYER_MAX-2);
        e_mod_move_widget_objs_color_set(indi_widget->objs, 0, 0, 0, 0);
        e_mod_move_widget_objs_show(indi_widget->objs);
        e_mod_move_widget_objs_raise(indi_widget->objs);

        // Set Input Shape Mask
        if ((indi_widget->input_region_id = e_manager_comp_input_region_id_new(m->man)))
          {
             e_manager_comp_input_region_id_set(m->man,
                                                indi_widget->input_region_id,
                                                x, y, w, h);
          }
        else
          goto error_cleanup;
     }
   else
     {
        goto error_cleanup;
     }

   // Set Event Handler
   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        _e_mod_move_indicator_widget_obj_event_setup(indi_widget, mwo);
     }
   return indi_widget;

error_cleanup:
   if (indi_widget->objs) e_mod_move_widget_objs_del(indi_widget->objs);
   memset(indi_widget, 0, sizeof(E_Move_Indicator_Widget));
   E_FREE(indi_widget);
   return NULL;
}

/* delete indicator_widget */
EINTERN void
e_mod_move_indicator_widget_del(E_Move_Indicator_Widget *indi_widget)
{
   E_Move        *m = NULL;
   E_Move_Border *mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   E_Zone        *zone = NULL;
   int x = 0; int y = 0;
   int mx = 0, my = 0, ax = 0, ay = 0;

   E_CHECK(indi_widget);
   m = e_mod_move_util_get();

   if (e_mod_move_indicator_widget_click_get(indi_widget))
     ecore_x_mouse_up_send(indi_widget->win,
                           indi_widget->pos.x,
                           indi_widget->pos.y,
                           1);

   if ((mb = e_mod_move_border_client_find(indi_widget->win)))
     {
        if (indi_widget->input_region_id)
          e_manager_comp_input_region_id_del(m->man, indi_widget->input_region_id);

        // if indicaor widget is deleted, then apptray or quickpanel's mirror object hide with animation
        if (indi_widget->quickpanel_move)
          {
             qp_mb = e_mod_move_quickpanel_find();
             E_CHECK_GOTO(qp_mb, error_cleanup);
             zone = qp_mb->bd->zone;

             switch (mb->angle)
               {
                case  90:
                   mx = qp_mb->w * -1; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x; ay = zone->y;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case 180:
                   mx = 0; my = zone->h;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case 270:
                   mx = zone->w; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case   0:
                default :
                   mx = 0; my = qp_mb->h * -1;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x; ay = zone->y;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
               }

             if (e_mod_move_quickpanel_objs_animation_state_get(qp_mb))
               {
                  e_mod_move_quickpanel_objs_animation_stop(qp_mb);
                  e_mod_move_quickpanel_objs_animation_clear(qp_mb);
               }
             e_mod_move_quickpanel_objs_add(qp_mb);
             e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
             e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s Indicator Widget Deleted: Hide QuickPanel %s():%d\n",
               "EVAS_OBJ", __func__, __LINE__);
          }

        if (indi_widget->apptray_move)
          {
             at_mb = e_mod_move_apptray_find();
             E_CHECK_GOTO(at_mb, error_cleanup);
             zone = at_mb->bd->zone;

             switch (mb->angle)
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

             // apptray_objs_animation_layer_set
             e_mod_move_apptray_objs_animation_layer_set(at_mb);

             e_mod_move_apptray_e_border_move(at_mb, x, y);
             e_mod_move_apptray_objs_animation_move(at_mb, x, y);
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s Indicator Widget Deleted: Hide Apptray %s():%d\n",
               "EVAS_OBJ", __func__, __LINE__);
          }
     }

error_cleanup:
   if (indi_widget->objs) e_mod_move_widget_objs_del(indi_widget->objs);
   memset(indi_widget, 0, sizeof(E_Move_Indicator_Widget));
   E_FREE(indi_widget);
   if (m) m->indicator_widget = NULL;
}

EINTERN Eina_Bool
e_mod_move_indicator_widget_angle_change(Ecore_X_Window win)
{
   E_Move                  *m = NULL;
   E_Move_Indicator_Widget *indi_widget = NULL;
   Eina_Bool                ret = EINA_FALSE;
   E_Move_Border           *mb = NULL;
   int                      x;
   int                      y;
   int                      w;
   int                      h;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   indi_widget = e_mod_move_indicator_widget_get();
   E_CHECK_RETURN(indi_widget, EINA_FALSE);

   if ((indi_widget->win == win))
     {
        if ((mb = e_mod_move_border_client_find(win)))
          {
             switch (mb->angle)
               {
                  case 90:
                     x = m->indicator_widget_geometry[E_MOVE_ANGLE_90].x;
                     y = m->indicator_widget_geometry[E_MOVE_ANGLE_90].y;
                     w = m->indicator_widget_geometry[E_MOVE_ANGLE_90].w;
                     h = m->indicator_widget_geometry[E_MOVE_ANGLE_90].h;
                     break;
                  case 180:
                     x = m->indicator_widget_geometry[E_MOVE_ANGLE_180].x;
                     y = m->indicator_widget_geometry[E_MOVE_ANGLE_180].y;
                     w = m->indicator_widget_geometry[E_MOVE_ANGLE_180].w;
                     h = m->indicator_widget_geometry[E_MOVE_ANGLE_180].h;
                     break;
                  case 270:
                     x = m->indicator_widget_geometry[E_MOVE_ANGLE_270].x;
                     y = m->indicator_widget_geometry[E_MOVE_ANGLE_270].y;
                     w = m->indicator_widget_geometry[E_MOVE_ANGLE_270].w;
                     h = m->indicator_widget_geometry[E_MOVE_ANGLE_270].h;
                     break;
                  case 0:
                  default:
                     x = m->indicator_widget_geometry[E_MOVE_ANGLE_0].x;
                     y = m->indicator_widget_geometry[E_MOVE_ANGLE_0].y;
                     w = m->indicator_widget_geometry[E_MOVE_ANGLE_0].w;
                     h = m->indicator_widget_geometry[E_MOVE_ANGLE_0].h;
                     break;
                }
              e_mod_move_widget_objs_move(indi_widget->objs, x, y);
              e_mod_move_widget_objs_resize(indi_widget->objs, w, h);

              //change Input Shape Mask
              if (indi_widget->input_region_id)
                {
                   e_manager_comp_input_region_id_set(m->man,
                                                      indi_widget->input_region_id,
                                                      x, y, w, h);
                }
              ret = EINA_TRUE;
           }
     }
   return ret;
}

EINTERN Eina_Bool
e_mod_move_indicator_widget_scrollable_check(void)
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

EINTERN Eina_Bool
e_mod_move_indicator_widget_click_get(E_Move_Indicator_Widget* indi_widget)
{
   Eina_Bool             click = EINA_FALSE;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List             *l;

   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   E_CHECK_RETURN(indi_widget->objs, EINA_FALSE);

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_indicator_widget_event_clear(E_Move_Indicator_Widget* indi_widget)
{
   Eina_Bool             click = EINA_FALSE;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List             *l;
   E_Move_Border         *mb = NULL;

   E_CHECK_RETURN(indi_widget, EINA_FALSE);
   E_CHECK_RETURN(indi_widget->objs, EINA_FALSE);

   click = e_mod_move_indicator_widget_click_get(indi_widget);
   E_CHECK_RETURN(click, EINA_FALSE);

   EINA_LIST_FOREACH(indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_data_clear(mwo->event);
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_indicator_widget_apptray_move_set(indi_widget, EINA_FALSE);
   _e_mod_move_indicator_widget_quickpanel_move_set(indi_widget, EINA_FALSE);

   mb = e_mod_move_border_client_find(indi_widget->win);
   if (mb && mb->flick_data) e_mod_move_flick_data_free(mb);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_widget_state_change(Ecore_X_Window win, Eina_Bool state)
{
   E_Move_Indicator_Widget *indi_widget = NULL;
   E_Move_Border           *mb = NULL;

   if ((indi_widget = e_mod_move_indicator_widget_get()))
     {
        if ((indi_widget->win == win)
            && (!state))
          {
             // indicator state disable -> delete current indicator widget
             e_mod_move_indicator_widget_del(indi_widget);
             e_mod_move_indicator_widget_set(NULL);
          }
     }
   else
     {
        mb = e_mod_move_border_client_find(win);
        if ((mb)
            && (mb->indicator_type == E_MOVE_INDICATOR_TYPE_1)
            && (state))
          e_mod_move_indicator_widget_apply();
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_widget_type_change(Ecore_X_Window win, E_Move_Indicator_Type type)
{
   E_Move_Indicator_Widget *indi_widget = NULL;
   E_Move_Border           *mb = NULL;

   if ((indi_widget = e_mod_move_indicator_widget_get()))
     {
        if ((indi_widget->win == win)
            && (type != E_MOVE_INDICATOR_TYPE_1))
          {
             // indicator type is not type_1 -> delete current indicator widget
             e_mod_move_indicator_widget_del(indi_widget);
             e_mod_move_indicator_widget_set(NULL);
#if 1
             // change later
             e_mod_move_indicator_widget_apply();
#endif
          }
     }
   else
     {
        mb = e_mod_move_border_client_find(win);
        if ((mb)
            && (mb->indicator_state == E_MOVE_INDICATOR_STATE_ON)
            && (type == E_MOVE_INDICATOR_TYPE_1))
          e_mod_move_indicator_widget_apply();
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_indicator_widget_angle_change_post_job(void)
{
   E_Move_Indicator_Widget *indi_widget = NULL;
   E_Move_Border           *mb = NULL;
   E_Border                *bd = NULL;
   E_Zone                  *zone = NULL;
   int                      angle = 0;
   int                      x = 0, y = 0;

   indi_widget = e_mod_move_indicator_widget_get();
   E_CHECK_RETURN(indi_widget, EINA_FALSE);

   mb = e_mod_move_border_client_find(indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);

   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   angle = mb->angle;

   if (e_mod_move_indicator_widget_click_get(indi_widget))
     {
         switch (angle)
           {
            case   0:
               x = zone->x;
               y = zone->y;
               break;
            case  90:
               x = zone->x;
               y = zone->y;
               break;
            case 180:
               x = zone->x + zone->w;
               y = zone->y + zone->h;
               break;
            case 270:
               x = zone->x + zone->w;
               y = zone->y + zone->h;
               break;
            default :
               break;
           }
     }

   if (mb->flick_data)
     e_mod_move_flick_data_init(mb, x, y);

   return EINA_TRUE;
}
