#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */
static Eina_Bool _e_mod_move_mini_apptray_widget_mini_apptray_move_set(E_Move_Mini_Apptray_Widget *mini_apptray_widget, Eina_Bool state);
static Eina_Bool _e_mod_move_mini_apptray_widget_mini_apptray_move_get(E_Move_Mini_Apptray_Widget *mini_apptray_widget);
static Eina_Bool _e_mod_move_mini_apptray_widget_cb_motion_start_internal_mini_apptray_check(E_Move_Border *mini_apptray_mb);
static Eina_Bool _e_mod_move_mini_apptray_widget_mini_apptray_flick_process(E_Move_Mini_Apptray_Widget *mini_apptray_widget, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool _e_mod_move_mini_apptray_widget_cb_motion_start(void *data, void *event_info);
static Eina_Bool _e_mod_move_mini_apptray_widget_cb_motion_move(void *data, void *event_info);
static Eina_Bool _e_mod_move_mini_apptray_widget_cb_motion_end(void *data, void *event_info);
static void      _e_mod_move_mini_apptray_widget_obj_event_setup(E_Move_Mini_Apptray_Widget *mini_apptray_widget, E_Move_Widget_Object *mwo);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_mini_apptray_widget_mini_apptray_move_set(E_Move_Mini_Apptray_Widget *mini_apptray_widget,
                                                      Eina_Bool                   state)
{
   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   mini_apptray_widget->mini_apptray_move = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_widget_mini_apptray_move_get(E_Move_Mini_Apptray_Widget *mini_apptray_widget)
{
   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   return mini_apptray_widget->mini_apptray_move;
}

static Eina_Bool
_e_mod_move_mini_apptray_widget_cb_motion_start_internal_mini_apptray_check(E_Move_Border *mini_apptray_mb)
{
   E_Move        *m;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(mini_apptray_mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mini_apptray_mb), EINA_FALSE);
   E_CHECK_RETURN(mini_apptray_mb->visible, EINA_FALSE);
   E_CHECK_RETURN(e_mod_move_util_compositor_object_visible_get(mini_apptray_mb),
                  EINA_FALSE);
   if (e_mod_move_mini_apptray_objs_animation_state_get(mini_apptray_mb)) return EINA_FALSE;

   // check if notification window is on-screen.
   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (TYPE_INDICATOR_CHECK(find_mb)) continue;
        if (find_mb->visible
             && REGION_INTERSECTS_WITH_ZONE(find_mb, mini_apptray_mb->bd->zone))
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

   e_mod_move_mini_apptray_dim_show(mini_apptray_mb);
   e_mod_move_mini_apptray_objs_add(mini_apptray_mb);

   // mini_apptray_objs_animation_layer_set
   e_mod_move_mini_apptray_objs_animation_layer_set(mini_apptray_mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_widget_mini_apptray_flick_process(E_Move_Mini_Apptray_Widget *mini_apptray_widget,
                                                           E_Move_Border              *mb2, // mb2 : mini_apptray
                                                           int                         angle,
                                                           Eina_Bool                   state)
{
   E_Move_Border            *mb = NULL;
   E_Move_Widget_Object     *mwo = NULL;
   Eina_List                *l;
   E_Zone                   *zone = NULL;
   int x = 0;
   int y = 0;

   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(mini_apptray_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo) // widget click unset
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_mini_apptray_widget_mini_apptray_move_set(mini_apptray_widget, EINA_FALSE);

   // flick data free
   if (mb->flick_data) e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90:
         x = zone->w - mb2->w;
         y = 0;
         break;
      case 180:
         x = 0;
         y = 0;
         break;
      case 270:
         x = 0;
         y = 0;
         break;
      case   0:
      default :
         x = 0;
         y = zone->h - mb2->h;
         break;
     }

   e_mod_move_mini_apptray_e_border_move(mb2, x, y);
   e_mod_move_mini_apptray_objs_animation_move(mb2, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_widget_cb_motion_start(void *data,
                                                void *event_info)
{
   E_Move *m = NULL;
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = (E_Move_Mini_Apptray_Widget *)data;

   E_Move_Border *mb = NULL;
   E_Move_Border *mini_apptray_mb = NULL;
   E_Move_Event_Motion_Info *info;
   E_Move_Widget_Object *mwo = NULL;
   Evas_Event_Mouse_Down *mouse_down_event = NULL;
   Eina_Bool clicked = EINA_FALSE;
   Eina_List *l;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(mini_apptray_widget->win);

   if (!m || !mb || !mini_apptray_widget || !info) return EINA_FALSE;

   mouse_down_event = info->event_info;
   E_CHECK_RETURN(mouse_down_event, EINA_FALSE);
   if (mouse_down_event->button != 1)
     return EINA_FALSE;

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        clicked = e_mod_move_event_click_get(mwo->event);
     }
   if (clicked)
     return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x MINI_APPTRAY_WIDGET_MOTION_START (%4d,%4d)\n",
     "EVAS_OBJ", mb->bd->win,
     info->coord.x, info->coord.y);

   _e_mod_move_mini_apptray_widget_mini_apptray_move_set(mini_apptray_widget, EINA_FALSE);

   /* check if apptray exists on the current zone */
   mini_apptray_mb = e_mod_move_mini_apptray_find();
   if ((mini_apptray_mb) &&
       (REGION_INSIDE_ZONE(mini_apptray_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x MINI_APPTRAY_WIDGET_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "mini_apptray exists. return.");
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_TRUE);
     }

   E_CHECK_GOTO(e_mod_move_flick_data_new(mb), error_cleanup);
   e_mod_move_flick_data_init(mb, info->coord.x, info->coord.y);

   if (!_e_mod_move_mini_apptray_widget_cb_motion_start_internal_mini_apptray_check(mini_apptray_mb))
     {
        goto error_cleanup;
     }

   e_mod_move_mini_apptray_e_border_raise(mini_apptray_mb);
   _e_mod_move_mini_apptray_widget_mini_apptray_move_set(mini_apptray_widget, EINA_TRUE);
   e_mod_move_mini_apptray_objs_animation_start_position_set(mini_apptray_mb,
                                                             mb->angle);
   // send mini_apptray to "move start message".
   e_mod_move_mini_apptray_anim_state_send(mini_apptray_mb, EINA_TRUE);

   mini_apptray_widget->pos = info->coord; // save mouse click position

   return EINA_TRUE;

error_cleanup:

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_mini_apptray_widget_mini_apptray_move_set(mini_apptray_widget, EINA_FALSE);

   return EINA_FALSE;

}

static Eina_Bool
_e_mod_move_mini_apptray_widget_cb_motion_move(void *data,
                                               void *event_info)
{
   E_Move                      *m = NULL;
   E_Move_Mini_Apptray_Widget  *mini_apptray_widget = (E_Move_Mini_Apptray_Widget *)data;
   E_Move_Border               *mb = NULL;
   E_Move_Event_Motion_Info    *info;
   E_Move_Widget_Object        *mwo = NULL;
   E_Zone                      *zone = NULL;
   Eina_List                   *l;
   Eina_Bool                    click = EINA_FALSE;
   int                          angle;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(mini_apptray_widget->win);

   if (!m || !mb || !info) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x MINI_APPTRAY_WIDGET_MOTION_MOVE a:%d (%4d,%4d)\n",
     "EVAS_OBJ", mb->bd->win, mb->angle,
     info->coord.x, info->coord.y);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }

   E_CHECK_RETURN(click, EINA_FALSE);

// do not work on moving just work on flick action
#if 0
   if (_e_mod_move_mini_apptray_widget_mini_apptray_move_get(mini_apptray_widget))
     {
        mini_apptray_mb = e_mod_move_mini_apptray_find();
        E_CHECK_RETURN(mini_apptray_mb, EINA_FALSE);
// change later for flick_up
// todo flick up geometry
        switch (angle)
          {
           case   0:
              if (info->coord.y > (zone->h - mini_apptray_mb->h))
                {
                   y = info->coord.y;
                   need_move = EINA_TRUE;
                }
              break;
           case  90:
              if (info->coord.x > (zone->w - mini_apptray_mb->w))
                {
                   x = info->coord.x;
                   need_move = EINA_TRUE;
                }
              break;
           case 180:
              if (info->coord.y < mini_apptray_mb->h)
                {
                   y = info->coord.y - mini_apptray_mb->h;
                   need_move = EINA_TRUE;
                }
              break;
           case 270:
              if (info->coord.x < mini_apptray_mb->w)
                {
                   x = info->coord.x - mini_apptray_mb->w;
                   need_move = EINA_TRUE;
                }
              break;
           default :
              break;
          }
        if (need_move)
          e_mod_move_mini_apptray_objs_move(mini_apptray_mb, x, y);
     }
#endif
   mini_apptray_widget->pos = info->coord; // save mouse move position

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_widget_cb_motion_end(void *data,
                                              void *event_info)
{
   E_Move                      *m = NULL;
   E_Move_Mini_Apptray_Widget  *mini_apptray_widget = (E_Move_Mini_Apptray_Widget *)data;
   E_Move_Border               *mb = NULL;
   E_Move_Border               *mini_apptray_mb = NULL;
   E_Move_Event_Motion_Info    *info;
   E_Move_Widget_Object        *mwo = NULL;
   Eina_List                   *l;
   E_Zone                      *zone;
   Evas_Event_Mouse_Up         *mouse_up_event;
   Eina_Bool                    click = EINA_FALSE;
   Eina_Bool                    flick_state = EINA_FALSE;
   int                          angle = 0;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(mini_apptray_widget->win);

   if (!m || !mb || !info) return EINA_FALSE;

   mouse_up_event = info->event_info;
   E_CHECK_RETURN(mouse_up_event, EINA_FALSE);
   if (mouse_up_event->button != 1)
     return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x ,angle:%d, (%d,%d)  %s()\n",
     "EVAS_OBJ", mb->bd->win, mb->angle, info->coord.x, info->coord.y,
     __func__);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }
   E_CHECK_GOTO(click, error_cleanup);

   e_mod_move_flick_data_update(mb, info->coord.x, info->coord.y);
   flick_state = e_mod_move_flick_state_get(mb, EINA_TRUE);

   if (_e_mod_move_mini_apptray_widget_mini_apptray_move_get(mini_apptray_widget))
     {
        mini_apptray_mb = e_mod_move_mini_apptray_find();

        if (_e_mod_move_mini_apptray_widget_mini_apptray_flick_process(mini_apptray_widget,
                                                                       mini_apptray_mb,
                                                                       angle, flick_state))
          {
             return EINA_TRUE;
          }
        else
          {
             // if mini_apptray animation is not called, must destory datas explicit
             if (mini_apptray_mb)
               {
                  e_mod_move_mini_apptray_objs_animation_layer_unset(mini_apptray_mb);
                  e_border_focus_set(mini_apptray_mb ->bd, 0, 0);
                  e_border_lower(mini_apptray_mb ->bd);
                  e_mod_move_mini_apptray_dim_hide(mini_apptray_mb );
                  e_mod_move_mini_apptray_objs_del(mini_apptray_mb );
               }
          }
     }
// just work on flick action. so. currently block.
#if 0
   mx = zone->x;
   my = zone->y;
   ax = mx;
   ay = my;
   switch (angle)
     {
      case   0:
         if (mini_apptray_mb)
           {
              check_h = mini_apptray_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y > (zone->h - check_h))
                {
                   my = zone->h;
                   ay = my;
                }
               else
                {
                   my = zone->h - mini_apptray_mb->h;
                   ay = my;
                }
           }
         break;
      case  90:
         if (mini_apptray_mb)
           {
              check_w = mini_apptray_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x > (zone->w - check_w))
                {
                   mx = zone->w;
                   ax = mx;
                }
               else
                {
                   mx = zone->w - mini_apptray_mb->w;
                   ax = mx;
                }
           }
         break;
      case 180:
         if (mini_apptray_mb)
           {
              check_h = mini_apptray_mb->h;
              if (check_h) check_h /= 2;
              if (info->coord.y < check_h)
                {
                   my = mini_apptray_mb->h * -1;
                   ay = my;
                }
           }
         break;
      case 270:
         if (mini_apptray_mb)
           {
              check_w = mini_apptray_mb->w;
              if (check_w) check_w /= 2;
              if (info->coord.x < check_w)
                {
                   mx = mini_apptray_mb->w * -1;
                   ax = mx;
                }
           }
         break;
      default :
         break;
     }

   if (mini_apptray_mb)
     {
        e_mod_move_mini_apptray_e_border_move(mini_apptray_mb, mx, my);
        e_mod_move_mini_apptray_objs_animation_move(mini_apptray_mb, ax, ay);
     }
#endif

   mini_apptray_widget->pos = info->coord; // save mouse up position

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

error_cleanup:
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   _e_mod_move_mini_apptray_widget_mini_apptray_move_set(mini_apptray_widget, EINA_FALSE);

   return EINA_TRUE;
}

static void
_e_mod_move_mini_apptray_widget_obj_event_setup(E_Move_Mini_Apptray_Widget *mini_apptray_widget,
                                                E_Move_Widget_Object    *mwo)
{
   E_CHECK(mini_apptray_widget);
   E_CHECK(mwo);

   mwo->event = e_mod_move_event_new(mini_apptray_widget->win, mwo->obj);
   E_CHECK(mwo->event);

   // change later ... below function used for just log
   e_mod_move_event_data_type_set(mwo->event, E_MOVE_EVENT_DATA_TYPE_WIDGET_INDICATOR);

   e_mod_move_event_angle_cb_set(mwo->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_mini_apptray_widget_cb_motion_start,
                           mini_apptray_widget);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_mini_apptray_widget_cb_motion_move,
                           mini_apptray_widget);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_mini_apptray_widget_cb_motion_end,
                           mini_apptray_widget);
   e_mod_move_event_send_all_set(mwo->event, EINA_TRUE);
   e_mod_move_event_find_redirect_win_set(mwo->event, EINA_TRUE);
}

/* externally accessible functions */

/* set current mini apptray widget */
EINTERN void
e_mod_move_mini_apptray_widget_set(E_Move_Mini_Apptray_Widget *mini_apptray_widget)
{
   E_Move *m = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);

   if (m->mini_apptray_widget)
     {
        e_mod_move_mini_apptray_widget_del(m->mini_apptray_widget);
     }

   m->mini_apptray_widget = mini_apptray_widget;
}

/* get current mini_apptray widget */
EINTERN E_Move_Mini_Apptray_Widget *
e_mod_move_mini_apptray_widget_get(void)
{
   E_Move *m = NULL;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, NULL);

   return m->mini_apptray_widget;
}

/* find mini_apptray widget target window */
// must update/change this function ( for mini_apptray policy )
EINTERN Eina_Bool
e_mod_move_mini_apptray_widget_target_window_find(Ecore_X_Window *win)
{
   E_Move        *m = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   E_Zone        *zone = NULL;
   Eina_Bool      ret = EINA_FALSE;

   E_CHECK_RETURN(win, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

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
       && !(TYPE_INDICATOR_CHECK(find_mb))
       && !(TYPE_APPTRAY_CHECK(find_mb))
       && !(TYPE_MINI_APPTRAY_CHECK(find_mb))
       && !(TYPE_QUICKPANEL_CHECK(find_mb))
       && (find_mb->mini_apptray_state != E_MOVE_MINI_APPTRAY_STATE_OFF))
     {
        *win = find_mb->bd->client.win;
        ret = EINA_TRUE;
     }

   return ret;
}

/* find mini_apptray widget's target window and apply mini_apptray widget control */
EINTERN void
e_mod_move_mini_apptray_widget_apply(void)
{
   E_Move         *m = NULL;
   Ecore_X_Window  target_win;
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);

   E_CHECK(e_mod_move_mini_apptray_find());

   if (e_mod_move_mini_apptray_widget_target_window_find(&target_win))
     {
        // if previous mini_apptray_widget is created
        if ((mini_apptray_widget = e_mod_move_mini_apptray_widget_get()))
          {
             // if current mini_apptray_widget's win is equal to finded win
             // then just return.
             if ((mini_apptray_widget->win == target_win)) return;
             else
               {
                  // if current mini_apptray_widget's win is not equal to finded win
                  // then del previous mini_apptray_widget and add new mini_apptray_widget.
                  e_mod_move_mini_apptray_widget_del(mini_apptray_widget);

                  e_mod_move_mini_apptray_widget_set(e_mod_move_mini_apptray_widget_add(target_win));
               }
          }
        else
          {
             //if previous mini_apptray_widget is not creagted
             //then add new mini_apptray_widget.
             e_mod_move_mini_apptray_widget_set(e_mod_move_mini_apptray_widget_add(target_win));
          }
     }
   else
     {
        // if current window does not require mini_apptray_widget
        // and previous mini_apptray_widget is created,
        // then del previous mini_apptray_widget
        if ((mini_apptray_widget = e_mod_move_mini_apptray_widget_get()))
          {
             e_mod_move_mini_apptray_widget_del(mini_apptray_widget);
             e_mod_move_mini_apptray_widget_set(NULL);
          }
     }
}

/* create E_Move_Border related Mini_Apptray_Widget */
EINTERN E_Move_Mini_Apptray_Widget *
e_mod_move_mini_apptray_widget_add(Ecore_X_Window win)
{
   E_Move                     *m = NULL;
   E_Move_Border              *mb = NULL;
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;
   E_Move_Widget_Object       *mwo = NULL;
   Eina_List                  *l;
   int                         x;
   int                         y;
   int                         w;
   int                         h;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   E_CHECK_RETURN(e_mod_move_mini_apptray_find(), EINA_FALSE);

   mb = e_mod_move_border_client_find(win);
   E_CHECK_RETURN(mb, NULL);

   mini_apptray_widget = E_NEW(E_Move_Mini_Apptray_Widget, 1);
   E_CHECK_RETURN(mini_apptray_widget, NULL);

   mini_apptray_widget->win = win;
   mini_apptray_widget->objs = e_mod_move_widget_objs_add(m);
   if (mini_apptray_widget->objs)
     {
        switch (mb->angle)
          {
             case 90:
                x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].x;
                y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].y;
                w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].w;
                h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].h;
                break;
             case 180:
                x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].x;
                y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].y;
                w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].w;
                h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].h;
                break;
             case 270:
                x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].x;
                y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].y;
                w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].w;
                h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].h;
                break;
             case 0:
             default:
                x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].x;
                y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].y;
                w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].w;
                h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].h;
                break;
          }
        e_mod_move_widget_objs_move(mini_apptray_widget->objs, x, y);
        e_mod_move_widget_objs_resize(mini_apptray_widget->objs, w, h);
        e_mod_move_widget_objs_layer_set(mini_apptray_widget->objs, EVAS_LAYER_MAX);
        e_mod_move_widget_objs_color_set(mini_apptray_widget->objs, 0, 0, 0, 0);
        e_mod_move_widget_objs_show(mini_apptray_widget->objs);
        e_mod_move_widget_objs_raise(mini_apptray_widget->objs);

        // Set Input Shape Mask
        switch (e_mod_move_util_root_angle_get())
          {
           case  90:
           case 180:
           case 270:
              // currently, support angle 0 only. because, application is not ready yet.
              break;
           case   0:
           default :
              e_manager_comp_input_region_set(m->man,
                                              m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].x,
                                              m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].y,
                                              m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].w,
                                              m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].h);
              break;
          }
     }
   else
     {
        goto error_cleanup;
     }

   // Set Event Handler
   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        _e_mod_move_mini_apptray_widget_obj_event_setup(mini_apptray_widget, mwo);
     }
   return mini_apptray_widget;

error_cleanup:
   if (mini_apptray_widget->objs) e_mod_move_widget_objs_del(mini_apptray_widget->objs);
   memset(mini_apptray_widget, 0, sizeof(E_Move_Mini_Apptray_Widget));
   E_FREE(mini_apptray_widget);
   return NULL;
}

/* delete mini_apptray_widget */
EINTERN void
e_mod_move_mini_apptray_widget_del(E_Move_Mini_Apptray_Widget *mini_apptray_widget)
{
   E_Move        *m = NULL;
   E_Move_Border *mb = NULL;
   E_Move_Border *mini_apptray_mb = NULL;
   E_Zone        *zone = NULL;
   int x = 0; int y = 0;

   E_CHECK(mini_apptray_widget);
   m = e_mod_move_util_get();

   if (e_mod_move_mini_apptray_widget_click_get(mini_apptray_widget))
     ecore_x_mouse_up_send(mini_apptray_widget->win,
                           mini_apptray_widget->pos.x,
                           mini_apptray_widget->pos.y,
                           1);

   if ((mb = e_mod_move_border_client_find(mini_apptray_widget->win)))
     {
        // compositor's input region free
        e_manager_comp_input_region_set(m->man, 0, 0, 0, 0);

        // if mini_apptray_widget is deleted, then mini_apptray's mirror object hide with animation
        if (mini_apptray_widget->mini_apptray_move)
          {
             mini_apptray_mb = e_mod_move_mini_apptray_find();
             E_CHECK_GOTO(mini_apptray_mb, error_cleanup);
             zone = mini_apptray_mb->bd->zone;

// following geometry will be changed. for flick-down effect
             switch (mb->angle)
               {
                case   0:
                   x = 0;
                   y = mini_apptray_mb->h * -1;
                   break;
                case  90:
                   x = mini_apptray_mb->w * -1;
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
                   y = mini_apptray_mb->h * -1;
                   break;
               }
             if (e_mod_move_mini_apptray_objs_animation_state_get(mini_apptray_mb))
               {
                  e_mod_move_mini_apptray_objs_animation_stop(mini_apptray_mb);
                  e_mod_move_mini_apptray_objs_animation_clear(mini_apptray_mb);
               }
             e_mod_move_mini_apptray_objs_add(mini_apptray_mb);

             // mini_apptray_objs_animation_layer_set
             e_mod_move_mini_apptray_objs_animation_layer_set(mini_apptray_mb);

             e_mod_move_mini_apptray_e_border_move(mini_apptray_mb, x, y);
             e_mod_move_mini_apptray_objs_animation_move(mini_apptray_mb, x, y);
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s Mini_Apptray Widget Deleted: Hide Mini Apptray %s():%d\n",
               "EVAS_OBJ", __func__, __LINE__);
          }
     }

error_cleanup:
   if (mini_apptray_widget->objs) e_mod_move_widget_objs_del(mini_apptray_widget->objs);
   memset(mini_apptray_widget, 0, sizeof(E_Move_Mini_Apptray_Widget));
   E_FREE(mini_apptray_widget);
   if (m) m->mini_apptray_widget = NULL;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_widget_angle_change(Ecore_X_Window win)
{
   E_Move                     *m = NULL;
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;
   Eina_Bool                   ret = EINA_FALSE;
   E_Move_Border              *mb = NULL;
   int                         x;
   int                         y;
   int                         w;
   int                         h;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mini_apptray_widget = e_mod_move_mini_apptray_widget_get();
   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);

   if ((mini_apptray_widget->win == win))
     {
        if ((mb = e_mod_move_border_client_find(win)))
          {
             switch (mb->angle)
               {
                  case 90:
                     x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].x;
                     y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].y;
                     w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].w;
                     h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].h;
                     break;
                  case 180:
                     x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].x;
                     y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].y;
                     w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].w;
                     h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].h;
                     break;
                  case 270:
                     x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].x;
                     y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].y;
                     w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].w;
                     h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].h;
                     break;
                  case 0:
                  default:
                     x = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].x;
                     y = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].y;
                     w = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].w;
                     h = m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].h;
                     break;
                }
              e_mod_move_widget_objs_move(mini_apptray_widget->objs, x, y);
              e_mod_move_widget_objs_resize(mini_apptray_widget->objs, w, h);

              // Set Input Shape Mask
              switch (e_mod_move_util_root_angle_get())
                {
                 case  90:
                 case 180:
                 case 270:
                    // currently, support angle 0 only. because, application is not ready yet.
                    break;
                 case   0:
                 default :
                    e_manager_comp_input_region_set(m->man,
                                                    m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].x,
                                                    m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].y,
                                                    m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].w,
                                                    m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].h);
                    break;
                }

              ret = EINA_TRUE;
           }
     }
   return ret;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_widget_scrollable_check(void)
{
   E_Move_Border *lockscr_mb = NULL;
   E_Move_Border *taskmgr_mb = NULL;
   E_Move_Border *pwlock_mb = NULL;

   // if lockscreen is exist & visible, then do not show  apptray
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
   // if taskmanage is exist & visible, then do not show  apptray
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

   // if pwlock is exist & visible, then do not show  apptray
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
e_mod_move_mini_apptray_widget_click_get(E_Move_Mini_Apptray_Widget* mini_apptray_widget)
{
   Eina_Bool             click = EINA_FALSE;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List             *l;

   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   E_CHECK_RETURN(mini_apptray_widget->objs, EINA_FALSE);

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_widget_event_clear(E_Move_Mini_Apptray_Widget* mini_apptray_widget)
{
   Eina_Bool             click = EINA_FALSE;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List             *l;
   E_Move_Border         *mb = NULL;

   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);
   E_CHECK_RETURN(mini_apptray_widget->objs, EINA_FALSE);

   click = e_mod_move_mini_apptray_widget_click_get(mini_apptray_widget);
   E_CHECK_RETURN(click, EINA_FALSE);

   EINA_LIST_FOREACH(mini_apptray_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_data_clear(mwo->event);
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_mini_apptray_widget_mini_apptray_move_set(mini_apptray_widget, EINA_FALSE);

   mb = e_mod_move_border_client_find(mini_apptray_widget->win);
   if (mb && mb->flick_data) e_mod_move_flick_data_free(mb);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_widget_state_change(Ecore_X_Window win, Eina_Bool state)
{
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;

   if ((mini_apptray_widget = e_mod_move_mini_apptray_widget_get()))
     {
        if ((mini_apptray_widget->win == win)
            && (!state))
          {
             // mini_apptray_state disable -> delete current mini_apptray_widget
             e_mod_move_mini_apptray_widget_del(mini_apptray_widget);
             e_mod_move_mini_apptray_widget_set(NULL);
          }
     }
   else
     {
        if (state) e_mod_move_mini_apptray_widget_apply();
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_widget_angle_change_post_job(void)
{
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;
   E_Move_Border              *mb = NULL;
   E_Border                   *bd = NULL;
   E_Zone                     *zone = NULL;
   int                         angle = 0;
   int                         x = 0, y = 0;

   mini_apptray_widget = e_mod_move_mini_apptray_widget_get();
   E_CHECK_RETURN(mini_apptray_widget, EINA_FALSE);

   mb = e_mod_move_border_client_find(mini_apptray_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);

   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   angle = mb->angle;

   if (e_mod_move_mini_apptray_widget_click_get(mini_apptray_widget))
     {
         switch (angle)
           {
            case   0:
               x = zone->x + zone->w;
               y = zone->y + zone->h;
               break;
            case  90:
               x = zone->x + zone->w;
               y = zone->y + zone->h;
               break;
            case 180:
               x = zone->x;
               y = zone->y;
               break;
            case 270:
               x = zone->x;
               y = zone->y;
               break;
            default :
               break;
           }
     }

   if (mb->flick_data)
     e_mod_move_flick_data_init(mb, x, y);

   return EINA_TRUE;
}
