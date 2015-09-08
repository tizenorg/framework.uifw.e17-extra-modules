#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"
#include <Elementary.h>

#define MSG_DOMAIN_CONTROL_INDICATOR     0x10001
#define MSG_ID_INDICATOR_REPEAT_EVENT    0x10002
#define MSG_ID_INDICATOR_ROTATION        0x10003
#define MSG_ID_INDICATOR_OPACITY         0X1004
#define MSG_ID_INDICATOR_TYPE            0X1005
#define MSG_ID_INDICATOR_START_ANIMATION 0X10006

static const char *PLUG_KEY = "__E_Plug_Ecore_Evas";

typedef struct _Data_Animation   Data_Animation;
typedef struct _Plug_Indicator   Plug_Indicator;
typedef struct _Effect_Indicator Effect_Indicator;

struct _E_Move_Split_Mode_Indicator_Animation_Data
{
   Eina_Bool       animating;
   int             sx;// start x
   int             sy;// start y
   int             ex;// end x
   int             ey;// end y
   int             dx;// distance x
   int             dy;// distance y
   int             angle; // angle
   Ecore_Animator *animator;
   Eina_Bool       on_screen; // indicator is on screen
};

struct _Data_Animation
{
   Ecore_X_Window xwin;
   double         duration;
};

struct _Plug_Indicator
{
   int                 type; // 0: portrait mode, 1: landscape mode
   Ecore_Evas         *ee;
   Evas_Object        *o;
   Evas_Object        *bg;
   Ecore_Timer        *show_timer;
   int                 x, y, w, h;
   int                 sx, sy;
   int                 deg;
   int                 cx, cy;
   Elm_Transit        *trans;
   Elm_Transit_Effect *eff;
};

struct _Effect_Indicator
{
   Evas_Object *o;
   Evas_Object *bg;
   int          src;
   int          tgt;
   int          deg;
   int          cx, cy;
};

// 0 is portrait mode and 1 is landscape mode
static Plug_Indicator *indi[2] = { NULL, NULL };
static Eina_Bool _plug_indi_mode = EINA_FALSE;

/* local subsystem functions */
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_apptray_move_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, Eina_Bool state);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, Eina_Bool state);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_apptray_move_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_quickpanel_move_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_move_started_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, Eina_Bool state);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_move_started_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_cb_motion_start_internal_apptray_check(E_Move_Border *at_mb);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_cb_motion_start_internal_quickpanel_check(E_Move_Border *qp_mb);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_quickpanel_flick_process(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_apptray_flick_process(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, E_Move_Border *mb2, int angle, Eina_Bool state);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_home_region_release_check(E_Move_Split_Mode_Indicator_Widget  *split_mode_indi_widget, Eina_Bool apptray_move, int angle, Evas_Point pos);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_cb_motion_start(void *data, void *event_info);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_cb_motion_move(void *data, void *event_info);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_cb_motion_end(void *data, void *event_info);
static void           _e_mod_move_split_mode_indicator_widget_obj_event_setup(E_Move_Split_Mode_Indicator_Widget *indicator_widget, E_Move_Widget_Object *mwo);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_scrollable_object_movable_check(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, E_Move_Border *mb, Evas_Point pos);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_target_window_find_by_pointer(Ecore_X_Window *win, int x, int y);
static Ecore_X_Window _e_mod_move_split_mode_indicator_widget_event_win_find(void *event_info);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_target_window_policy_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_position_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, int* x, int* y);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_position_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, int x, int y);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_add(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, const char *svcname);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_show(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_hide(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_resize(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, int w, int h);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_move(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, int x, int y);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_del(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_rotate(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, double angle, int cx, int cy);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_flick_state_get(Evas_Point pos, int angle);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_cb(void *data);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_add(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_del(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_animation_frame(void  *data, double pos);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_animation_move_with_time(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, int x, int y, int angle, Eina_Bool on_screen, double time);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_animation_move(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget, int x, int y, int angle, Eina_Bool on_screen);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_animation_state_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_animation_stop(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_objs_animation_clear(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_hide(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);
static Eina_Bool      _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_show(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget);

static void                _obj_rotate(Evas_Object *o, double deg, Evas_Coord cx, Evas_Coord cy);
static void                _on_trans_end(void *data, Elm_Transit *transit);
static Elm_Transit_Effect *_effect_new(Evas_Object *o, Evas_Object *bg, int src, int tgt, int deg, int cx, int cy);
static void                _effect_op(Elm_Transit_Effect *effect, Elm_Transit *transit, double progress);
static void                _effect_show_free(Elm_Transit_Effect *effect, Elm_Transit *transit);
static void                _effect_hide_free(Elm_Transit_Effect *effect, Elm_Transit *transit);
static void                _effect_prepare(Plug_Indicator *pi, Elm_Transit_Effect_End_Cb end_cb);
static Eina_Bool           _plug_indi_hide(void *data);
static void                _plug_indi_msg_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size);
static Plug_Indicator     *_plug_indi_add(int type, Evas *evas, const char *svcname, void (*func_handle)(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size));
static void                _plug_indi_init(void);
static void                _plug_indi_mode_set(Eina_Bool set);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_split_mode_indicator_widget_apptray_move_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                              Eina_Bool                state)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   split_mode_indi_widget->apptray_move = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_quickpanel_move_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                 Eina_Bool                state)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   split_mode_indi_widget->quickpanel_move = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_apptray_move_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   return split_mode_indi_widget->apptray_move;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_quickpanel_move_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   return split_mode_indi_widget->quickpanel_move;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_move_started_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                              Eina_Bool                state)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   split_mode_indi_widget->move_started = state;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_move_started_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   return split_mode_indi_widget->move_started;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_cb_motion_start_internal_apptray_check(E_Move_Border *at_mb)
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
_e_mod_move_split_mode_indicator_widget_cb_motion_start_internal_quickpanel_check(E_Move_Border *qp_mb)
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
_e_mod_move_split_mode_indicator_widget_quickpanel_flick_process(E_Move_Split_Mode_Indicator_Widget  *split_mode_indi_widget,
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

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   m = mb->m;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo) // indicator click unset
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_FALSE);
   _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_FALSE);

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

   if (!E_INTERSECTS(mx, my, mb2->w, mb2->h, zone->x, zone->y, zone->w, zone->h))
     {
        // mb2: quickpanel, check quickpanel is not on screen.
        // if quickpanel is not on sceen then add indicator image auto hide timer.
        _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_add(split_mode_indi_widget);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_apptray_flick_process(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
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

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb2, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb2), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo) // indicator click unset
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_FALSE);
   _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_FALSE);

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
_e_mod_move_split_mode_indicator_widget_home_region_release_check(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
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
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);
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
_e_mod_move_split_mode_indicator_widget_cb_motion_start(void *data,
                                             void *event_info)
{
   E_Move *m = NULL;
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = (E_Move_Split_Mode_Indicator_Widget *)data;

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

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   // clicked window indicator policy check
   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        ev_win = e_mod_move_event_win_get(mwo->event);
     }
   ev_mb = e_mod_move_border_client_find(ev_win);

   if (ev_mb && ev_mb->bd)
     if ((!ev_mb->bd->client.e.state.ly.curr_ly) && ((ev_mb->bd->client.icccm.accepts_focus) || (ev_mb->bd->client.icccm.take_focus)))
       e_focus_event_mouse_down(ev_mb->bd);

   E_CHECK_RETURN(_e_mod_move_split_mode_indicator_widget_target_window_policy_check(ev_mb),
                  EINA_FALSE);

   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);

   if (!m || !mb || !split_mode_indi_widget || !info) return EINA_FALSE;

   mouse_down_event = info->event_info;
   E_CHECK_RETURN(mouse_down_event, EINA_FALSE);
   if (mouse_down_event->button != 1)
     return EINA_FALSE;

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        clicked = e_mod_move_event_click_get(mwo->event);
     }
   if (clicked)
     return EINA_FALSE;

   SL(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x split_mode_indi_widget_MOTION_START (%4d,%4d)\n",
     "EVAS_OBJ", mb->bd->win,
     info->coord.x, info->coord.y);

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_TRUE);
     }

   // if indicator object is (show or hide) animating then do not scroll quickpanel
   if (_e_mod_move_split_mode_indicator_objs_animation_state_get(split_mode_indi_widget))
     return EINA_FALSE;

   if (!split_mode_indi_widget->plugin_objs_show)
     {
        split_mode_indi_widget->event_forwarding_off = EINA_FALSE;
        return EINA_TRUE;
     }
   else
     {
        if (split_mode_indi_widget->plugin_objs_auto_hide_timer)
          _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_del(split_mode_indi_widget);
     }

   _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_FALSE);
   _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_FALSE);

   /* check if apptray or quickpanel exists on the current zone */
   at_mb = e_mod_move_apptray_find();
   if ((at_mb) &&
       (REGION_INSIDE_ZONE(at_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x split_mode_indi_widget_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "apptray exists. return.");
        return EINA_FALSE;
     }

   qp_mb = e_mod_move_quickpanel_find();
   if ((qp_mb) &&
       (REGION_INSIDE_ZONE(qp_mb, mb->bd->zone)))
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x split_mode_indi_widget_MOTION_START %s\n",
          "EVAS_OBJ", mb->bd->win,
          "quickpanel exists. return.");
        return EINA_FALSE;
     }

   E_CHECK_GOTO(e_mod_move_flick_data_new(mb), error_cleanup);
   e_mod_move_flick_data_init(mb, info->coord.x, info->coord.y);

   scroll_region = e_mod_move_indicator_region_scroll_check(mb->angle, info->coord);

   if (scroll_region == E_MOVE_SCROLL_REGION_APPTRAY)
     {
        if (e_mod_move_panel_scrollable_get(mb, E_MOVE_PANEL_TYPE_APPTRAY))
          {
             if (_e_mod_move_split_mode_indicator_widget_cb_motion_start_internal_apptray_check(at_mb))
               {
                  e_mod_move_apptray_e_border_raise(at_mb);
                  _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_TRUE);
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
             if (_e_mod_move_split_mode_indicator_widget_cb_motion_start_internal_quickpanel_check(qp_mb))
               {
                  _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_TRUE);
                  e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                          mb->angle,
                                                                          EINA_FALSE);
                  // send quickpanel to "move start message".
                  e_mod_move_quickpanel_anim_state_send(qp_mb, EINA_TRUE);
               }
          }
     }

   split_mode_indi_widget->pos = info->coord; // save mouse click position

   return EINA_TRUE;

error_cleanup:

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_FALSE);
   _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_FALSE);

   return EINA_FALSE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_cb_motion_move(void *data,
                                            void *event_info)
{
   E_Move                   *m = NULL;
   E_Move_Split_Mode_Indicator_Widget  *split_mode_indi_widget = (E_Move_Split_Mode_Indicator_Widget *)data;
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
   Ecore_X_Window            ev_win = 0;
   E_Move_Border            *ev_mb = NULL;

   int cx, cy, cw, ch;
   int x = 0, y = 0;
   int angle;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);

   if (!m || !mb || !info) return EINA_FALSE;

   SL(LT_EVENT_OBJ,
      "[MOVE] ev:%15.15s w:0x%08x split_mode_indi_widget_MOTION_MOVE a:%d (%4d,%4d)\n",
      "EVAS_OBJ", mb->bd->win, mb->angle,
      info->coord.x, info->coord.y);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }

   E_CHECK_RETURN(click, EINA_FALSE);

   // for split indicator mode
   if (!split_mode_indi_widget->plugin_objs_show)
     {
        if (_e_mod_move_split_mode_indicator_objs_animation_state_get(split_mode_indi_widget))
          return EINA_FALSE;

        EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
          {
             if (!mwo) continue;
             ev_win = e_mod_move_event_win_get(mwo->event);
          }
        ev_mb = e_mod_move_border_client_find(ev_win);

        if ((ev_mb) && (ev_mb->bd))
          {
              if (ev_mb->bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
                 return EINA_TRUE;
          }

        if (_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_flick_state_get(info->coord, angle))
          {
             _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_show(split_mode_indi_widget);

             EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
               {
                  if (!mwo) continue;
                  e_mod_move_event_click_set(mwo->event, EINA_FALSE);
               }
          }
        return EINA_TRUE;
     }

   if (_e_mod_move_split_mode_indicator_widget_quickpanel_move_get(split_mode_indi_widget))
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

        if (_e_mod_move_split_mode_indicator_widget_scrollable_object_movable_check(split_mode_indi_widget, mb, info->coord))
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
   else if (_e_mod_move_split_mode_indicator_widget_apptray_move_get(split_mode_indi_widget))
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

   e_mod_move_flick_data_move_pos_update(mb, info->coord.x, info->coord.y);
   split_mode_indi_widget->pos = info->coord; // save mouse move position

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_cb_motion_end(void *data,
                                           void *event_info)
{
   E_Move                   *m = NULL;
   E_Move_Split_Mode_Indicator_Widget  *split_mode_indi_widget = (E_Move_Split_Mode_Indicator_Widget *)data;
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
   Ecore_X_Window            target_win = 0;
   Ecore_X_Window            ev_win = 0;
   E_Move_Border            *ev_mb = NULL;

   int cx, cy, cw, ch;
   int check_h, check_w;
   int angle = 0;
   int mx = 0, my = 0, ax = 0, ay = 0;
   Eina_Bool cancel_state = EINA_FALSE;

   info  = (E_Move_Event_Motion_Info *)event_info;
   m = e_mod_move_util_get();

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);

   if (!m || !mb || !info) return EINA_FALSE;

   mouse_up_event = info->event_info;
   E_CHECK_RETURN(mouse_up_event, EINA_FALSE);

   if (split_mode_indi_widget->plugin_objs_show)
     {
        //Disable Event Passing to Client Window
        EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
          {
             if (!mwo) continue;
             e_mod_move_event_propagate_type_set(mwo->event,
                                                 E_MOVE_EVENT_PROPAGATE_TYPE_NONE);
          }
        //Enable Event Passing to below Evas object (Indicator Plugin Object)
        e_mod_move_widget_objs_repeat_events_set(split_mode_indi_widget->objs, EINA_TRUE);

        EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
          {
             if (!mwo) continue;
             target_win = e_mod_move_event_win_get(mwo->event);
          }

        if (!target_win)
          target_win = e_mod_move_util_client_xid_get(mb);

        if (target_win)
          {
             ecore_x_mouse_up_send(target_win,
                                   info->coord.x,
                                   info->coord.y,
                                   mouse_up_event->button);
          }
     }
   else
     split_mode_indi_widget->event_forwarding_off = EINA_TRUE;

   if (mouse_up_event->button != 1)
     return EINA_FALSE;

   SL(LT_EVENT_OBJ,
      "[MOVE] ev:%15.15s w:0x%08x ,angle:%d, (%d,%d)  %s()\n",
      "EVAS_OBJ", mb->bd->win, mb->angle, info->coord.x, info->coord.y,
      __func__);

   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }
   E_CHECK_GOTO(click, error_cleanup);

   if (!split_mode_indi_widget->plugin_objs_show)
     {
        if (_e_mod_move_split_mode_indicator_objs_animation_state_get(split_mode_indi_widget))
          return EINA_FALSE;

        EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
          {
             if (!mwo) continue;
             e_mod_move_event_click_set(mwo->event, EINA_FALSE);
             ev_win = e_mod_move_event_win_get(mwo->event);
          }
        ev_mb = e_mod_move_border_client_find(ev_win);
        if ((ev_mb) && (ev_mb->bd))
          {
             if ((ev_mb->bd->client.e.state.ly.curr_ly) &&
                 ((ev_mb->bd->client.icccm.accepts_focus) || (ev_mb->bd->client.icccm.take_focus)))
               {
                  e_focus_event_mouse_down(ev_mb->bd);
               }

             if (ev_mb->bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
               return EINA_TRUE;
          }

             if (_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_flick_state_get(info->coord, angle))
               _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_show(split_mode_indi_widget);

        return EINA_TRUE;
     }

   qp_mv_state = _e_mod_move_split_mode_indicator_widget_quickpanel_move_get(split_mode_indi_widget);
   at_mv_state = _e_mod_move_split_mode_indicator_widget_apptray_move_get(split_mode_indi_widget);
   if (!qp_mv_state && !at_mv_state) goto finish;

   e_mod_move_flick_data_update(mb, info->coord.x, info->coord.y);
   flick_state = e_mod_move_flick_state_get(mb, EINA_TRUE);
   cancel_state = e_mod_move_flick_scroll_cancel_state_get(mb, E_MOVE_FLICK_DOWN);
   if (cancel_state) flick_state = EINA_FALSE;

   if (qp_mv_state)
     {
        qp_mb = e_mod_move_quickpanel_find();
        if (_e_mod_move_split_mode_indicator_widget_quickpanel_flick_process(split_mode_indi_widget, qp_mb,
                                                                  angle, flick_state))
          {
             return EINA_TRUE;
          }
     }
   if (at_mv_state)
     {
        at_mb = e_mod_move_apptray_find();

        // if release position is on indicator's home button then, do not flick.
        if (_e_mod_move_split_mode_indicator_widget_home_region_release_check(split_mode_indi_widget,
                                                                   EINA_TRUE, /* upper if phrase check _e_mod_move_indicator_apptray_move_get(mb) value TURE or FALSE. */
                                                                   angle,
                                                                   info->coord))
          {
             flick_state = EINA_FALSE;
          }

        if (_e_mod_move_split_mode_indicator_widget_apptray_flick_process(split_mode_indi_widget, at_mb,
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
              if ((cancel_state)
                  || (info->coord.y < check_h))
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
              if ((cancel_state)
                  || (info->coord.x < check_w))
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
              if ((cancel_state)
                  || (info->coord.y > (zone->h - check_h)))
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
              if ((cancel_state)
                  || (info->coord.x > (zone->w - check_w)))
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
        if (_e_mod_move_split_mode_indicator_widget_scrollable_object_movable_check(split_mode_indi_widget, mb, info->coord))
          e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
        else
          e_mod_move_quickpanel_objs_animation_move_with_time(qp_mb, ax, ay, 0.0000001);
        // time :0.0 calls animation_frame with pos : 0.0  calls once.
        // so I use small time value, it makes animation_frame with pos: 1.0 call once

        // for split indicator mode
        if (!E_INTERSECTS(mx, my, qp_mb->w, qp_mb->h, zone->x, zone->y, zone->w, zone->h))
          {
             // check quickpane is not on screen.
             // if quickpanel is not on sceen then add indicator image auto hide timer.
             _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_add(split_mode_indi_widget);
          }
     }

finish:
   split_mode_indi_widget->pos = info->coord; // save mouse up position
   _e_mod_move_split_mode_indicator_widget_move_started_set(split_mode_indi_widget, EINA_FALSE);

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

error_cleanup:
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_FALSE);
   _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_FALSE);

   return EINA_TRUE;
}

static void
_e_mod_move_split_mode_indicator_widget_obj_event_setup(E_Move_Split_Mode_Indicator_Widget *indicator_widget,
                                             E_Move_Widget_Object    *mwo)
{
   E_CHECK(indicator_widget);
   E_CHECK(mwo);

   mwo->event = e_mod_move_event_new(indicator_widget->win, mwo->obj);
   E_CHECK(mwo->event);

   e_mod_move_event_data_type_set(mwo->event, E_MOVE_EVENT_DATA_TYPE_WIDGET_INDICATOR);
   e_mod_move_event_angle_cb_set(mwo->event,
                                 e_mod_move_util_win_angle_get);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_split_mode_indicator_widget_cb_motion_start,
                           indicator_widget);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_split_mode_indicator_widget_cb_motion_move,
                           indicator_widget);
   e_mod_move_event_cb_set(mwo->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_split_mode_indicator_widget_cb_motion_end,
                           indicator_widget);
   e_mod_move_event_win_find_cb_set(mwo->event,
                                    _e_mod_move_split_mode_indicator_widget_event_win_find);
   e_mod_move_event_propagate_type_set(mwo->event,
                                       E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY);
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_scrollable_object_movable_check(E_Move_Split_Mode_Indicator_Widget *indicator_widget,
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

   move_started = _e_mod_move_split_mode_indicator_widget_move_started_get(indicator_widget);
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
         _e_mod_move_split_mode_indicator_widget_move_started_set(indicator_widget, EINA_TRUE);
         ret = EINA_TRUE;
     }

   return ret;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_target_window_find_by_pointer(Ecore_X_Window *win,
                                                                      int x,
                                                                      int y)
{
   E_Move        *m = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   Eina_Bool      ret = EINA_FALSE;
   Ecore_X_Window noti_win = 0;
   Eina_Bool      noti_win_saved = EINA_FALSE;
   Ecore_X_Window *all_wins;
   Ecore_X_Window_Attributes att;
   int index,num;
   int win_x,win_y,win_w,win_h;

   E_CHECK_RETURN(win, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   all_wins = ecore_x_window_children_get(ecore_x_window_root_first_get(), &num);
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   if (all_wins)
     {
        for (index = num-1; index >= 0; index--)
          {
              ecore_x_window_geometry_get(all_wins[index],&win_x, &win_y,&win_w, &win_h);
              if (!E_INSIDE(x, y, win_x, win_y,win_w, win_h))
                {
                    continue;
                }

               if (!ecore_x_window_attributes_get(all_wins[index], &att))
                  continue;

               if (att.input_only && att.viewable)
                 {
                     *win = all_wins[index];
                     found = EINA_FALSE;
                     ret = EINA_TRUE;
                     break;
                 }

               find_mb = e_mod_move_border_find(all_wins[index]);

               if (!find_mb ||!find_mb->bd) continue;

               // finding visible border
               if (!find_mb->visible) continue;

               // if notification , alpha, and indicator_state_none then search again below.
               if ((TYPE_NOTIFICATION_CHECK(find_mb) ||
               TYPE_APP_SELECTOR_CHECK(find_mb) ||
               TYPE_APP_POPUP_CHECK(find_mb))
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
         free(all_wins);
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
             if ((noti_win_saved)
                 && (find_mb->indicator_state != E_MOVE_INDICATOR_STATE_NONE))
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
_e_mod_move_split_mode_indicator_widget_event_win_find(void *event_info)
{
   E_Move_Event_Motion_Info *info = NULL;
   Ecore_X_Window            win = 0, res_win = 0;
   info  = (E_Move_Event_Motion_Info *)event_info;

   E_CHECK_RETURN(info, 0);

   if (_e_mod_move_split_mode_indicator_widget_target_window_find_by_pointer(&win,
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
_e_mod_move_split_mode_indicator_widget_target_window_policy_check(E_Move_Border *mb)
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

   if (REGION_INTERSECTS_WITH_ZONE(mb, zone) // check On Screen
       && (zone->id == 0))                   // change zone->id comparing to bd's profile property (mobile)
     {
         ret = EINA_TRUE;
     }

   return ret;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_position_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                   int                                *x,
                                                   int                                *y)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   *x = split_mode_indi_widget->plugin_objs_geometry.x;
   *y = split_mode_indi_widget->plugin_objs_geometry.y;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_position_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                   int                                x,
                                                   int                                y)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   split_mode_indi_widget->plugin_objs_geometry.x = x;
   split_mode_indi_widget->plugin_objs_geometry.y = y;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_add(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                                  const char                         *svcname)
{
   E_Move *m = NULL;

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(svcname, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   split_mode_indi_widget->plugin_objs = e_mod_move_evas_plugin_objs_add(m, svcname, EINA_FALSE);
   e_mod_move_evas_objs_layer_set(split_mode_indi_widget->plugin_objs, EVAS_LAYER_MAX-2);
   e_mod_move_evas_objs_lower(split_mode_indi_widget->plugin_objs);// for widget object handling event. if image object is upper that event handling object, user could  not scrolle quickpanel.
   e_mod_move_evas_objs_del_cb_set(&(split_mode_indi_widget->plugin_objs));

// set indicator background object
   split_mode_indi_widget->indi_bg_objs = e_mod_move_evas_objs_add(m, NULL, EINA_FALSE);
   e_mod_move_evas_objs_layer_set(split_mode_indi_widget->indi_bg_objs, EVAS_LAYER_MAX-2);
   e_mod_move_evas_objs_stack_below(split_mode_indi_widget->indi_bg_objs, split_mode_indi_widget->plugin_objs);
   e_mod_move_evas_objs_del_cb_set(&(split_mode_indi_widget->indi_bg_objs));
   e_mod_move_evas_objs_color_set(split_mode_indi_widget->indi_bg_objs, 0, 0, 0, 255); // black

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_show(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   e_mod_move_evas_objs_show(split_mode_indi_widget->plugin_objs);
   e_mod_move_evas_objs_show(split_mode_indi_widget->indi_bg_objs);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_hide(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   e_mod_move_evas_objs_hide(split_mode_indi_widget->plugin_objs);
   e_mod_move_evas_objs_hide(split_mode_indi_widget->indi_bg_objs);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_move(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                                   int                                 x,
                                                                   int                                 y)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   e_mod_move_evas_objs_move(split_mode_indi_widget->plugin_objs, x, y);
   e_mod_move_evas_objs_move(split_mode_indi_widget->indi_bg_objs, x, y);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_del(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   if (split_mode_indi_widget->plugin_objs)
     {
        e_mod_move_evas_objs_del(split_mode_indi_widget->plugin_objs);
        split_mode_indi_widget->plugin_objs = NULL;
     }

   if (split_mode_indi_widget->indi_bg_objs)
     {
        e_mod_move_evas_objs_del(split_mode_indi_widget->indi_bg_objs);
        split_mode_indi_widget->indi_bg_objs = NULL;
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_resize(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                                     int                                 w,
                                                                     int                                 h)

{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   e_mod_move_evas_objs_resize(split_mode_indi_widget->plugin_objs, w, h);
   e_mod_move_evas_objs_resize(split_mode_indi_widget->indi_bg_objs, w, h);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_rotate(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                                     double                              angle,
                                                                     int                                 cx,
                                                                     int                                 cy)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   if (angle == 0.0) return EINA_TRUE;
   e_mod_move_evas_objs_rotate_set(split_mode_indi_widget->plugin_objs, angle, cx, cy);
   e_mod_move_evas_objs_rotate_set(split_mode_indi_widget->indi_bg_objs, angle, cx, cy);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_flick_state_get(Evas_Point pos,
                                                                              int        angle)
{
   E_Move    *m = NULL;
   Eina_Bool  state = EINA_FALSE;
   E_Zone    *zone = NULL;
   double     flick_distance_rate = 0.0;
   double     check_distance = 0.0;
   int x = 0, y = 0;
   int zone_w = 0, zone_h = 0;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   zone = e_util_zone_current_get(m->man);
   E_CHECK_RETURN(zone, EINA_FALSE);

   x = pos.x;
   y = pos.y;
   zone_w = zone->w;
   zone_h = zone->h;

   flick_distance_rate = m->flick_limit.distance_rate;

   switch(angle)
     {
      case  90:
         check_distance = zone_w * flick_distance_rate;
         if (check_distance < x ) state = EINA_TRUE;
         break;
      case 180:
         check_distance = zone_h * ( 1.0 - flick_distance_rate);
         if (check_distance > y ) state = EINA_TRUE;
         break;
      case 270:
         check_distance = zone_w * ( 1.0 - flick_distance_rate);
         if (check_distance > x ) state = EINA_TRUE;
         break;
      case   0:
      default :
         check_distance = zone_h * flick_distance_rate;
         if (check_distance < y ) state = EINA_TRUE;
         break;
     }

   return state;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_cb(void *data)
{
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = data;

   E_CHECK_RETURN(split_mode_indi_widget, ECORE_CALLBACK_CANCEL);
   split_mode_indi_widget->plugin_objs_auto_hide_timer = NULL;

   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_hide(split_mode_indi_widget);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_add(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   Ecore_Timer *plugin_objs_auto_hide_timer = NULL;
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   if (split_mode_indi_widget->plugin_objs_auto_hide_timer)
     _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_del(split_mode_indi_widget);

   plugin_objs_auto_hide_timer = ecore_timer_add(3.0, _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_cb, split_mode_indi_widget);
   E_CHECK_RETURN(plugin_objs_auto_hide_timer, EINA_FALSE);

   split_mode_indi_widget->plugin_objs_auto_hide_timer = plugin_objs_auto_hide_timer;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_del(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   if (split_mode_indi_widget->plugin_objs_auto_hide_timer)
     {
        ecore_timer_del(split_mode_indi_widget->plugin_objs_auto_hide_timer);
        split_mode_indi_widget->plugin_objs_auto_hide_timer = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_animation_frame(void  *data,
                                                      double pos)
{
   E_Move_Split_Mode_Indicator_Animation_Data *anim_data = NULL;
   E_Move_Split_Mode_Indicator_Widget         *split_mode_indi_widget = NULL;
   E_Move                                     *m = NULL;
   E_Zone                                     *zone = NULL;
   double                                      frame = pos;
   int                                         x, y, cx, cy;
   double                                      angle = 0.0;
   E_Move_Widget_Object                       *mwo = NULL;
   Eina_List                                  *l;

   anim_data = (E_Move_Split_Mode_Indicator_Animation_Data *)data;
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   zone = e_util_zone_current_get(m->man);
   E_CHECK_RETURN(zone, EINA_FALSE);
   split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get();
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_DECELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;
   angle = anim_data->angle;
   cx = zone->x + zone->w / 2;
   cy = zone->y + zone->h / 2;


//  아래는 indicator plugin object move , rotation set 넣기
   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_move(split_mode_indi_widget, x, y);
   _e_mod_move_split_mode_indicator_objs_position_set(split_mode_indi_widget, x, y);
   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_rotate(split_mode_indi_widget, angle, cx, cy);

   if (pos >= 1.0)
     {
        if (anim_data->on_screen)
          {
             split_mode_indi_widget->plugin_objs_show = EINA_TRUE;
             if (split_mode_indi_widget->event_forwarding_off)
               {
                  //Disable Event Passing to Client Window
                  EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
                    {
                       if (!mwo) continue;
                       e_mod_move_event_propagate_type_set(mwo->event,
                                                           E_MOVE_EVENT_PROPAGATE_TYPE_NONE);
                    }
                  //Enable Event Passing to below Evas object (Indicator Plugin Object)
                  e_mod_move_widget_objs_repeat_events_set(split_mode_indi_widget->objs, EINA_TRUE);
               }
          }
         else
          {
             split_mode_indi_widget->plugin_objs_show = EINA_FALSE;
             _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_hide(split_mode_indi_widget);
             _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_del(split_mode_indi_widget);

             e_mod_move_util_compositor_composite_mode_set(m, EINA_FALSE);

             if (!m->split_indicator_event_forwarding_disable)
             {
                //Enable Event Passing to Client Window
                EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
                  {
                     if (!mwo) continue;
                     e_mod_move_event_propagate_type_set(mwo->event,
                                                         E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY);
                  }

                //Disable Event Passing to below Evas object (Indicator Plugin Object)
                e_mod_move_widget_objs_repeat_events_set(split_mode_indi_widget->objs, EINA_FALSE);
             }
          }

         memset(anim_data, 0, sizeof(E_Move_Split_Mode_Indicator_Animation_Data));
         E_FREE(anim_data);
         split_mode_indi_widget->anim_data = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_animation_move_with_time(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                               int                                 x,
                                                               int                                 y,
                                                               int                                 angle,
                                                               Eina_Bool                           on_screen,
                                                               double                              time)
{
   E_Move_Split_Mode_Indicator_Animation_Data *anim_data = NULL;
   Ecore_Animator *animator = NULL;
   int sx, sy; //start x, start y

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   anim_data = E_NEW(E_Move_Split_Mode_Indicator_Animation_Data, 1);
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   _e_mod_move_split_mode_indicator_objs_position_get(split_mode_indi_widget, &sx, &sy);
   anim_data->sx = sx;
   anim_data->sy = sy;
   anim_data->ex = x;
   anim_data->ey = y;
   anim_data->dx = anim_data->ex - anim_data->sx;
   anim_data->dy = anim_data->ey - anim_data->sy;
   anim_data->angle = angle;
   anim_data->on_screen = on_screen;

   animator = ecore_animator_timeline_add(time,
                                          _e_mod_move_split_mode_indicator_objs_animation_frame,
                                          anim_data);
   if (!animator)
     {
        memset(anim_data, 0, sizeof(E_Move_Split_Mode_Indicator_Animation_Data));
        E_FREE(anim_data);
        return EINA_FALSE;
     }

   anim_data->animator = animator;
   anim_data->animating = EINA_TRUE;
   split_mode_indi_widget->anim_data = anim_data;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_animation_move(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget,
                                                     int                                 x,
                                                     int                                 y,
                                                     int                                 angle,
                                                     Eina_Bool                           on_screen /* indicator exist screen */)
{
   E_Move *m = NULL;
   double anim_time = 0.5;
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   //anim_time = m->split_indicator_animation_duration;
   return _e_mod_move_split_mode_indicator_objs_animation_move_with_time(split_mode_indi_widget, x, y, angle, on_screen, anim_time);
}

static Eina_Bool _e_mod_move_split_mode_indicator_objs_animation_state_get(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move_Split_Mode_Indicator_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(split_mode_indi_widget->anim_data, EINA_FALSE);
   anim_data = split_mode_indi_widget->anim_data;
   E_CHECK_RETURN(anim_data->animating, EINA_FALSE);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_animation_stop(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move_Split_Mode_Indicator_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(split_mode_indi_widget->anim_data, EINA_FALSE);
   anim_data = split_mode_indi_widget->anim_data;
   ecore_animator_freeze(anim_data->animator);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_objs_animation_clear(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move_Split_Mode_Indicator_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(split_mode_indi_widget->anim_data, EINA_FALSE);
   anim_data = split_mode_indi_widget->anim_data;
   ecore_animator_del(anim_data->animator);
   memset(anim_data, 0, sizeof(E_Move_Split_Mode_Indicator_Animation_Data));
   E_FREE(anim_data);
   split_mode_indi_widget->anim_data = NULL;
   return  EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_show(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move               *m = NULL;
   E_Move_Border        *mb = NULL;
   E_Zone               *zone = NULL;
   int                   zone_x;
   int                   zone_y;
   int                   zone_w;
   int                   zone_h;
   int                   sx; // start_x
   int                   sy; // start_y
   int                   ex; // end_x
   int                   ey; // end_y
   int                   split_indi_portrait_w = 720;
   int                   split_indi_landscape_w = 1280;
   int                   split_indi_h = 60;
   int                   angle = 0;

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb =  e_mod_move_border_client_find(split_mode_indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   zone = mb->bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);
   zone_x = zone->x;
   zone_y = zone->y;
   zone_w = zone->w;
   zone_h = zone->h;

   e_mod_move_util_compositor_composite_mode_set(m, EINA_TRUE);
   switch (mb->angle)
     {
      case 90:
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_add(split_mode_indi_widget, "elm_indicator_landscape");
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_resize(split_mode_indi_widget, split_indi_landscape_w, split_indi_h);
         ex = (zone_h - zone_w) / 2 * -1;
         ey = (zone_h - zone_w) / 2;
         sx = ex;
         sy = ey - split_indi_h;
         angle = 270;
         break;
      case 180:
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_add(split_mode_indi_widget, "elm_indicator_portrait");
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_resize(split_mode_indi_widget, split_indi_portrait_w, split_indi_h);
         ex = zone_x;
         ey = zone_y;
         sx = ex;
         sy = ey - split_indi_h;
         angle = 180;
         break;
      case 270:
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_add(split_mode_indi_widget, "elm_indicator_landscape");
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_resize(split_mode_indi_widget, split_indi_landscape_w, split_indi_h);
         ex = (zone_h - zone_w) / 2 * -1;
         ey = (zone_h - zone_w) / 2;
         sx = ex;
         sy = ey - split_indi_h;
         angle = 90;
         break;
      case 0:
      default:
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_add(split_mode_indi_widget, "elm_indicator_portrait");
         _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_resize(split_mode_indi_widget, split_indi_portrait_w, split_indi_h);
         ex = zone_x;
         ey = zone_y;
         sx = ex;
         sy = ey - split_indi_h;
         angle = 0;
         break;
     }

   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_move(split_mode_indi_widget, sx, sy);
   _e_mod_move_split_mode_indicator_objs_position_set(split_mode_indi_widget, sx, sy);

   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_rotate(split_mode_indi_widget, mb->angle, zone_w / 2 , zone_h / 2);
   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_show(split_mode_indi_widget);

   _e_mod_move_split_mode_indicator_objs_animation_move(split_mode_indi_widget,ex, ey, angle, EINA_TRUE);

   //split_mode_indi_widget->plugin_objs_show = EINA_TRUE;
   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_add(split_mode_indi_widget);

   return EINA_TRUE;
}


static Eina_Bool
_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_hide(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move_Border        *mb = NULL;
   int                   split_indi_h = 60;
   E_Zone               *zone = NULL;
   int                   zone_x;
   int                   zone_y;
   int                   zone_w;
   int                   zone_h;
   int                   angle;
   int                   ex;
   int                   ey;

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   if (split_mode_indi_widget->plugin_objs_auto_hide_timer)
     _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_del(split_mode_indi_widget);

   // 애니메에션 중이면 에니메이션 삭제하는 코드 넣기
   if (_e_mod_move_split_mode_indicator_objs_animation_state_get(split_mode_indi_widget))
     {
        _e_mod_move_split_mode_indicator_objs_animation_stop(split_mode_indi_widget);
        _e_mod_move_split_mode_indicator_objs_animation_clear(split_mode_indi_widget);
     }

   E_CHECK_RETURN(split_mode_indi_widget->plugin_objs, EINA_FALSE);
   //E_CHECK_RETURN(split_mode_indi_widget->plugin_objs_show, EINA_FALSE);

   mb =  e_mod_move_border_client_find(split_mode_indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   zone = mb->bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);
   zone_x = zone->x;
   zone_y = zone->y;
   zone_w = zone->w;
   zone_h = zone->h;

   switch (mb->angle)
     {
      case 90:
         ex = (zone_h - zone_w) / 2 * -1;
         ey = (zone_h - zone_w) / 2 - split_indi_h;
         angle = 270;
         break;
      case 180:
         ex = zone_x;
         ey = zone_y - split_indi_h;
         angle = 180;
         break;
      case 270:
         ex = (zone_h - zone_w) / 2 * -1;
         ey = (zone_h - zone_w) / 2 - split_indi_h;
         angle = 90;
         break;
      case 0:
      default:
         ex = zone_x;
         ey = zone_y - split_indi_h;
         angle = 0;
         break;
     }

   return _e_mod_move_split_mode_indicator_objs_animation_move(split_mode_indi_widget,ex, ey, angle, EINA_FALSE);
}

static void
_obj_rotate(Evas_Object *o,
            double       deg,
            Evas_Coord   cx,
            Evas_Coord   cy)
{
   Evas_Map *m = evas_map_new(4);
   evas_map_util_points_populate_from_object(m, o);
   evas_map_util_rotate(m, deg, cx, cy);
   evas_object_map_set(o, m);
   evas_object_map_enable_set(o, EINA_TRUE);
   evas_map_free(m);
}

static void
_on_trans_end(void *data,
              Elm_Transit *transit __UNUSED__)
{
   Plug_Indicator *pi = data;
   if (pi->trans)
     {
        elm_transit_del_cb_set(pi->trans, NULL, NULL);
        pi->trans = NULL;
     }
}

static Elm_Transit_Effect *
_effect_new(Evas_Object *o,
            Evas_Object *bg,
            int          src,
            int          tgt,
            int          deg,
            int          cx,
            int          cy)
{
   Effect_Indicator *ctx = E_NEW(Effect_Indicator, 1);
   E_CHECK_RETURN(ctx, NULL);

   ctx->o = o;
   ctx->bg = bg;
   ctx->src = src;
   ctx->tgt = tgt;
   ctx->deg = deg;
   ctx->cx = cx;
   ctx->cy = cy;

   return ctx;
}

static void
_effect_op(Elm_Transit_Effect *effect,
           Elm_Transit        *transit,
           double              progress)
{
   Effect_Indicator *ctx = (Effect_Indicator *)effect;
   Evas_Coord x, y, w, h;
   double curr;

   evas_object_geometry_get(ctx->o, &x, &y, &w, &h);

   curr = (double)ctx->src + (progress * ((double)ctx->tgt - (double)ctx->src));

   evas_object_move(ctx->o, x, (Evas_Coord)curr);
   evas_object_move(ctx->bg, x, (Evas_Coord)curr);

   _obj_rotate(ctx->bg, (double)ctx->deg, ctx->cx, ctx->cy);
   _obj_rotate(ctx->o, (double)ctx->deg, ctx->cx, ctx->cy);
}

static void
_effect_show_free(Elm_Transit_Effect *effect,
                  Elm_Transit        *transit)
{
   Effect_Indicator *ctx = (Effect_Indicator *)effect;
   E_FREE(ctx);
}

static void
_effect_hide_free(Elm_Transit_Effect *effect,
                  Elm_Transit        *transit)
{
   Effect_Indicator *ctx = (Effect_Indicator *)effect;
   evas_object_hide(ctx->o);
   evas_object_hide(ctx->bg);
   E_FREE(ctx);
}

static void
_effect_prepare(Plug_Indicator           *pi,
                Elm_Transit_Effect_End_Cb end_cb)
{
   if (pi->trans)
     {
        elm_transit_del_cb_set(pi->trans, NULL, NULL);
        elm_transit_del(pi->trans);
     }
   pi->trans = elm_transit_add();
   elm_transit_del_cb_set(pi->trans, _on_trans_end, pi);

   pi->eff = _effect_new(pi->o, pi->bg, pi->sy, pi->y, pi->deg, pi->cx, pi->cy);

   elm_transit_smooth_set(pi->trans, EINA_FALSE);
   elm_transit_duration_set(pi->trans, 0.2f);
   elm_transit_effect_add(pi->trans, _effect_op, pi->eff, end_cb);
   elm_transit_tween_mode_set(pi->trans, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_objects_final_state_keep_set(pi->trans, EINA_FALSE);

   elm_transit_go(pi->trans);

}

static Eina_Bool
_plug_indi_hide(void *data)
{
   Plug_Indicator *pi = (Plug_Indicator *)data;
   E_CHECK_RETURN(pi, ECORE_CALLBACK_CANCEL);

   pi->show_timer = NULL;

   if (!_plug_indi_mode)
     {
        evas_object_hide(pi->o);
        evas_object_hide(pi->bg);
        return ECORE_CALLBACK_CANCEL;
     }

   E_Manager *man = eina_list_nth(e_manager_list(), 0);
   E_Zone *zone = e_util_zone_current_get(man);
   E_CHECK_RETURN(zone, ECORE_CALLBACK_CANCEL);

   switch (pi->deg)
     {
      case  90:
      case 270:
         pi->sx = (zone->h - zone->w) / 2 * (-1);
         pi->sy = (zone->h - zone->w) / 2;
         pi->x = pi->sx;
         pi->y = pi->sy - 60;
         break;
      case   0:
      case 180:
      default :
         pi->sx = zone->x;
         pi->sy = zone->y;
         pi->x = pi->sx;
         pi->y = pi->sy - 60;
         break;
     }

   _effect_prepare(pi, _effect_hide_free);

   return ECORE_CALLBACK_CANCEL;
}


static void
_plug_indi_msg_handle(Ecore_Evas *ee,
                      int         msg_domain,
                      int         msg_id,
                      void       *data,
                      int         size)
{
   if (!_plug_indi_mode) return;
   int zone_angle = 0;

   E_CHECK(msg_domain == MSG_DOMAIN_CONTROL_INDICATOR);
   E_CHECK(msg_id == MSG_ID_INDICATOR_START_ANIMATION);
   E_CHECK(size == (int)sizeof(Data_Animation));

   Plug_Indicator *pi = (Plug_Indicator *)ecore_evas_data_get(ee, PLUG_KEY);
   E_CHECK(pi);

   ELBF(ELBT_MOVE, 0, 0, "%15.15s|type:%d", "PLUG_EE_MSG_HANDLE", pi->type);

   Data_Animation *anim_data = data;
   E_CHECK(anim_data);

   E_Manager *man = eina_list_nth(e_manager_list(), 0);
   E_Zone *zone = e_util_zone_current_get(man);
   E_CHECK(zone);

   E_Move *m = e_mod_move_util_get();
   E_CHECK(m);

   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get();
   E_CHECK(split_mode_indi_widget);

   E_Move_Border *mb = e_mod_move_border_client_find(split_mode_indi_widget->win);
   E_CHECK(mb);

   zone_angle = zone->rot.curr;
   /* check window angle and zone angle */
   if ((pi->show_timer) && (zone_angle != mb->angle))
      return;
   else if ((!pi->show_timer) && (zone_angle != mb->angle))
      zone_angle = mb->angle;

   switch (zone_angle)
     {
      case  90: pi->deg = 270; break;
      case 180: pi->deg = 180; break;
      case 270: pi->deg =  90; break;
      case   0:
      default : pi->deg =   0; break;
     }

   switch (zone_angle)
     {
      case  90:
      case 270:
         pi->x = (zone->h - zone->w) / 2 * (-1);
         pi->y = (zone->h - zone->w) / 2;
         pi->sx = pi->x;
         pi->sy = pi->y - 60;
         break;
      case   0:
      case 180:
      default :
         pi->x = zone->x;
         pi->y = zone->y;
         pi->sx = pi->x;
         pi->sy = pi->y - 60;
         break;
     }

   evas_object_move(pi->bg, pi->sx, pi->sy);
   evas_object_move(pi->o, pi->sx, pi->sy);

   _obj_rotate(pi->bg, (double)pi->deg, pi->cx, pi->cy);
   _obj_rotate(pi->o, (double)pi->deg, pi->cx, pi->cy);

   evas_object_show(pi->bg);
   evas_object_show(pi->o);

   if (pi->show_timer) ecore_timer_del(pi->show_timer);
   pi->show_timer = ecore_timer_add(anim_data->duration, _plug_indi_hide, pi);

   _effect_prepare(pi, _effect_show_free);
}

static Plug_Indicator *
_plug_indi_add(int         type,
               Evas       *evas,
               const char *svcname,
               void        (*func_handle)(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size))
{
   Plug_Indicator *pi = E_NEW(Plug_Indicator, 1);
   E_CHECK_RETURN(pi, NULL);

   pi->type = type;

   pi->bg = evas_object_rectangle_add(evas);
   E_CHECK_GOTO(pi->bg, cleanup);

   evas_object_color_set(pi->bg, 0, 0, 0, 255);

   pi->o = ecore_evas_extn_plug_new(ecore_evas_ecore_evas_get(evas));
   E_CHECK_GOTO(pi->o, cleanup);

   if (ecore_evas_extn_plug_connect(pi->o, svcname, 0, EINA_FALSE))
     {
        pi->ee = ecore_evas_object_ecore_evas_get(pi->o);
        ecore_evas_data_set(pi->ee, PLUG_KEY, pi);
        ecore_evas_callback_msg_handle_set(pi->ee, func_handle);

        E_Manager *man = eina_list_nth(e_manager_list(), 0);
        E_Zone *zone = e_util_zone_current_get(man);
        E_CHECK_GOTO(zone, cleanup);

        if (type == 0)
          pi->w = zone->w;
        else
          pi->w = zone->h;

        pi->h = 60;
        pi->cx = zone->w / 2;
        pi->cy = zone->h / 2;

        evas_object_resize(pi->bg, pi->w, pi->h);
        evas_object_resize(pi->o, pi->w, pi->h);

        return pi;
     }

cleanup:
   if (pi) E_FREE(pi);
   return NULL;
}

static void
_plug_indi_init(void)
{
   E_Move *m;
   E_Move_Canvas *canvas;

   E_CHECK(!indi[0]);

   m = e_mod_move_util_get();
   E_CHECK(m);

   canvas = eina_list_nth(m->canvases, 0);
   E_CHECK(canvas);

   indi[0] = _plug_indi_add(0, canvas->evas, "elm_indicator_portrait",  _plug_indi_msg_handle);
   indi[1] = _plug_indi_add(1, canvas->evas, "elm_indicator_landscape", _plug_indi_msg_handle);
}

static void
_plug_indi_mode_set(Eina_Bool set)
{
   if (_plug_indi_mode == set)
     return;

   _plug_indi_mode = set;

   if (set)
     _plug_indi_init();
}

/* externally accessible functions */

/* set current indicator widget */
EINTERN void
e_mod_move_split_mode_indicator_widget_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move *m = NULL;

   m = e_mod_move_util_get();
   E_CHECK(m);

   if (m->split_mode_indicator_widget)
     {
        e_mod_move_split_mode_indicator_widget_del(m->split_mode_indicator_widget);
     }

   m->split_mode_indicator_widget = split_mode_indi_widget;
}

/* get current indicator widget */
EINTERN E_Move_Split_Mode_Indicator_Widget *
e_mod_move_split_mode_indicator_widget_get(void)
{
   E_Move *m = NULL;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, NULL);

   return m->split_mode_indicator_widget;
}

/* find indicator widget target window */
EINTERN Eina_Bool
e_mod_move_split_mode_indicator_widget_target_window_find(Ecore_X_Window *win)
{
   E_Move        *m = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool      found = EINA_FALSE;
   E_Zone        *zone = NULL;
   Eina_Bool      ret = EINA_FALSE;
   E_Desk_Layout *launcher_ly = NULL;

   E_CHECK_RETURN(win, EINA_FALSE);
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   // fix later
   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (!find_mb->bd) continue;
        // pass keyboard window
        if (TYPE_KEYBOARD_CHECK(find_mb)) continue;

        // skip split launcher & skip border whose desk layout is split_launcher's layout
        if (TYPE_SPLIT_LAUNCHER_CHECK(find_mb))
          {
             launcher_ly = find_mb->bd->client.e.state.ly.curr_ly;
             continue;
          }
        if (launcher_ly && (launcher_ly == find_mb->bd->client.e.state.ly.curr_ly))
          continue;

        zone = find_mb->bd->zone;

        if ((find_mb->visible) && (zone->id == 0)) // check visible, change zone->id comparing to bd's profile property (mobile)
          {
             if (REGION_EQUAL_TO_ZONE(find_mb, zone))
               {
                  if ((TYPE_NOTIFICATION_CHECK(find_mb) ||
                       TYPE_APP_SELECTOR_CHECK(find_mb) ||
                       TYPE_APP_POPUP_CHECK(find_mb)) &&
                      (find_mb->argb) &&
                      (find_mb->indicator_state == E_MOVE_INDICATOR_STATE_NONE))
                    {
                       continue;
                    }
                  else
                    {
                       break;
                    }
               }
             else if ((REGION_INTERSECTS_WITH_ZONE(find_mb, zone)) &&
                      (find_mb->bd->client.e.state.ly.curr_ly)) // check split mode window
               {
                  found = EINA_TRUE;
                  break;
               }
          }
     }

   if (found &&
       !(TYPE_APPTRAY_CHECK(find_mb)) &&
       !(TYPE_MINI_APPTRAY_CHECK(find_mb)) &&
       !(TYPE_QUICKPANEL_CHECK(find_mb)) &&
       (find_mb->indicator_state == E_MOVE_INDICATOR_STATE_OFF))
     {
        *win = find_mb->bd->client.win;
        ret = EINA_TRUE;
     }

   return ret;
}

/* find indicator widget's target window and apply indicator widget control */
EINTERN void
e_mod_move_split_mode_indicator_widget_apply(void)
{
   E_Move                  *m = NULL;
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = NULL;
   Ecore_X_Window           target_win;

   m = e_mod_move_util_get();
   E_CHECK(m);
   if ((m->screen_reader_state) || (m->setup_wizard_state))
     return;

   if (e_mod_move_split_mode_indicator_widget_target_window_find(&target_win))
     {
        _plug_indi_mode_set(EINA_TRUE);

        // if previous indicator widget is created
        if ((split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get()))
          {
             // if current indicator widget's win is equal to finded win
             // then just return.
             if ((split_mode_indi_widget->win == target_win))
               return;
             else
               {
                  // if current indicator widget's win is not equal to finded win
                  // then del previous indicator_widget and add new indicator widget.
                  e_mod_move_split_mode_indicator_widget_del(split_mode_indi_widget);
                  e_mod_move_split_mode_indicator_widget_set(e_mod_move_split_mode_indicator_widget_add(target_win));
               }
          }
        else
          {
             //if previous indicator widget is not created
             //then add new indicator widget.
             e_mod_move_split_mode_indicator_widget_set(e_mod_move_split_mode_indicator_widget_add(target_win));
          }
     }
   else
     {
        _plug_indi_mode_set(EINA_FALSE);

        // if current window does not require indicator widget
        // and previous indicator widget is created,
        // then del previous indicator_widget
        if ((split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get()))
          {
             e_mod_move_split_mode_indicator_widget_del(split_mode_indi_widget);
             e_mod_move_split_mode_indicator_widget_set(NULL);
          }
     }

   if (m->split_indicator_event_forwarding_disable)
     e_mod_move_split_mode_indicator_widget_event_forward_off_set(EINA_TRUE);
}

/* create E_Move_Border related Indicator_Widget */
EINTERN E_Move_Split_Mode_Indicator_Widget *
e_mod_move_split_mode_indicator_widget_add(Ecore_X_Window win)
{
   E_Move                  *m = NULL;
   E_Move_Border           *mb = NULL;
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = NULL;
   E_Move_Widget_Object    *mwo = NULL;
   Eina_List               *l;
   int                      x;
   int                      y;
   int                      w;
   int                      h;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, NULL);

   mb = e_mod_move_border_client_find(win);
   E_CHECK_RETURN(mb, NULL);

   split_mode_indi_widget = E_NEW(E_Move_Split_Mode_Indicator_Widget, 1);
   E_CHECK_RETURN(split_mode_indi_widget, NULL);

   split_mode_indi_widget->win = win;
   split_mode_indi_widget->objs = e_mod_move_widget_objs_add(m);
   if (split_mode_indi_widget->objs)
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

        e_mod_move_widget_objs_move(split_mode_indi_widget->objs, x, y);
        e_mod_move_widget_objs_resize(split_mode_indi_widget->objs, w, h);
        e_mod_move_widget_objs_layer_set(split_mode_indi_widget->objs, EVAS_LAYER_MAX-2);
        e_mod_move_widget_objs_color_set(split_mode_indi_widget->objs, 0, 0, 0, 0);
        //e_mod_move_widget_objs_color_set(split_mode_indi_widget->objs, 0, 255, 0, 100); // for test
        e_mod_move_widget_objs_show(split_mode_indi_widget->objs);
        e_mod_move_widget_objs_raise(split_mode_indi_widget->objs);

        // Set Input Shape Mask
        if ((split_mode_indi_widget->input_region_id = e_manager_comp_input_region_id_new(m->man)))
          {
             e_manager_comp_input_region_id_set(m->man,
                                                split_mode_indi_widget->input_region_id,
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
   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        _e_mod_move_split_mode_indicator_widget_obj_event_setup(split_mode_indi_widget, mwo);
     }
   return split_mode_indi_widget;

error_cleanup:
   if (split_mode_indi_widget->objs)
     {
        e_mod_move_widget_objs_del(split_mode_indi_widget->objs);
        split_mode_indi_widget->objs = NULL;
     }

   memset(split_mode_indi_widget, 0, sizeof(E_Move_Split_Mode_Indicator_Widget));
   E_FREE(split_mode_indi_widget);
   return NULL;
}

/* delete indicator_widget */
EINTERN void
e_mod_move_split_mode_indicator_widget_del(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget)
{
   E_Move        *m = NULL;
   E_Move_Border *mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   E_Zone        *zone = NULL;
   int x = 0; int y = 0;
   int mx = 0, my = 0, ax = 0, ay = 0;

   E_CHECK(split_mode_indi_widget);
   m = e_mod_move_util_get();

   if (e_mod_move_split_mode_indicator_widget_click_get(split_mode_indi_widget))
     ecore_x_mouse_up_send(split_mode_indi_widget->win,
                           split_mode_indi_widget->pos.x,
                           split_mode_indi_widget->pos.y,
                           1);

   if ((mb = e_mod_move_border_client_find(split_mode_indi_widget->win)))
     {
        if (split_mode_indi_widget->input_region_id)
          e_manager_comp_input_region_id_del(m->man, split_mode_indi_widget->input_region_id);

        // if indicaor widget is deleted, then apptray or quickpanel's mirror object hide with animation
        if (split_mode_indi_widget->quickpanel_move)
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

        if (split_mode_indi_widget->apptray_move)
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
   if (split_mode_indi_widget->objs)
     {
        e_mod_move_widget_objs_del(split_mode_indi_widget->objs);
        split_mode_indi_widget->objs = NULL;
     }

   // Clear Animation Timer
   if (split_mode_indi_widget->plugin_objs_auto_hide_timer)
     _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_auto_hide_timer_del(split_mode_indi_widget);

   //_e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_hide(split_mode_indi_widget);
   if (_e_mod_move_split_mode_indicator_objs_animation_state_get(split_mode_indi_widget))
     {
        _e_mod_move_split_mode_indicator_objs_animation_stop(split_mode_indi_widget);
        _e_mod_move_split_mode_indicator_objs_animation_clear(split_mode_indi_widget);
     }
   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_hide(split_mode_indi_widget);
   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_del(split_mode_indi_widget);
   e_mod_move_util_compositor_composite_mode_set(m, EINA_FALSE);

   memset(split_mode_indi_widget, 0, sizeof(E_Move_Split_Mode_Indicator_Widget));
   E_FREE(split_mode_indi_widget);
   if (m) m->split_mode_indicator_widget = NULL;
}

EINTERN Eina_Bool
e_mod_move_split_mode_indicator_widget_angle_change(Ecore_X_Window win)
{
   E_Move                  *m = NULL;
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = NULL;
   Eina_Bool                ret = EINA_FALSE;
   E_Move_Border           *mb = NULL;
   int                      x;
   int                      y;
   int                      w;
   int                      h;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get();
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   if ((split_mode_indi_widget->win == win))
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
              e_mod_move_widget_objs_move(split_mode_indi_widget->objs, x, y);
              e_mod_move_widget_objs_resize(split_mode_indi_widget->objs, w, h);

              // for split_mode_indicator
              if ((split_mode_indi_widget->plugin_objs_show)
                   || (_e_mod_move_split_mode_indicator_objs_animation_state_get(split_mode_indi_widget)))
                {
                   _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_animation_hide(split_mode_indi_widget);
                 // when indicator visible and get rotate then, hide inidicator image.
                 //  _e_mod_move_split_mode_indicator_widget_indicator_plugin_objs_show(split_mode_indi_widget);
                }

              //change Input Shape Mask
              if (split_mode_indi_widget->input_region_id)
                {
                   e_manager_comp_input_region_id_set(m->man,
                                                      split_mode_indi_widget->input_region_id,
                                                      x, y, w, h);
                }
              ret = EINA_TRUE;
           }
     }
   return ret;
}

EINTERN Eina_Bool
e_mod_move_split_mode_indicator_widget_scrollable_check(void)
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
e_mod_move_split_mode_indicator_widget_click_get(E_Move_Split_Mode_Indicator_Widget* split_mode_indi_widget)
{
   Eina_Bool             click = EINA_FALSE;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List             *l;

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(split_mode_indi_widget->objs, EINA_FALSE);

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        click = e_mod_move_event_click_get(mwo->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_split_mode_indicator_widget_event_clear(E_Move_Split_Mode_Indicator_Widget* split_mode_indi_widget)
{
   Eina_Bool             click = EINA_FALSE;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List             *l;
   E_Move_Border         *mb = NULL;

   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);
   E_CHECK_RETURN(split_mode_indi_widget->objs, EINA_FALSE);

   click = e_mod_move_split_mode_indicator_widget_click_get(split_mode_indi_widget);
   E_CHECK_RETURN(click, EINA_FALSE);

   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_data_clear(mwo->event);
        e_mod_move_event_click_set(mwo->event, EINA_FALSE);
     }

   _e_mod_move_split_mode_indicator_widget_apptray_move_set(split_mode_indi_widget, EINA_FALSE);
   _e_mod_move_split_mode_indicator_widget_quickpanel_move_set(split_mode_indi_widget, EINA_FALSE);

   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);
   if (mb && mb->flick_data) e_mod_move_flick_data_free(mb);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_split_mode_indicator_widget_state_change(Ecore_X_Window win, Eina_Bool state)
{
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = NULL;
   E_Move_Border           *mb = NULL;

   if ((split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get()))
     {
        if ((split_mode_indi_widget->win == win)
            && state)
          {
             // indicator state disable -> delete current indicator widget
             e_mod_move_split_mode_indicator_widget_del(split_mode_indi_widget);
             e_mod_move_split_mode_indicator_widget_set(NULL);
          }
        else // just for test
          e_mod_move_split_mode_indicator_widget_apply();
     }
   else
     {
        mb = e_mod_move_border_client_find(win);
        if ((mb) && (!state))
          e_mod_move_split_mode_indicator_widget_apply();
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_split_mode_indicator_widget_angle_change_post_job(void)
{
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = NULL;
   E_Move_Border           *mb = NULL;
   E_Border                *bd = NULL;
   E_Zone                  *zone = NULL;
   int                      angle = 0;
   int                      x = 0, y = 0;

   split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get();
   E_CHECK_RETURN(split_mode_indi_widget, EINA_FALSE);

   mb = e_mod_move_border_client_find(split_mode_indi_widget->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);

   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   angle = mb->angle;

   if (e_mod_move_split_mode_indicator_widget_click_get(split_mode_indi_widget))
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

EINTERN void
e_mod_move_split_mode_indicator_widget_zone_rot_change(void)
{
   int i = 0;
   for (i = 0; i < 2; i++)
     {
        if (indi[i])
          {
             if (indi[i]->trans)
               {
                  elm_transit_del_cb_set(indi[i]->trans, NULL, NULL);
                  elm_transit_del(indi[i]->trans);
               }
             evas_object_hide(indi[i]->o);
             evas_object_hide(indi[i]->bg);

             if (indi[i]->show_timer)
               ecore_timer_del(indi[i]->show_timer);

             indi[i]->show_timer = NULL;
          }
     }
}

EINTERN void
e_mod_move_split_mode_indicator_widget_event_forward_off_set(Eina_Bool set)
{
   E_Move *m = NULL;
   E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget = NULL;
   E_Move_Widget_Object *mwo = NULL;
   Eina_List *l;
   E_Move_Event_Propagate_Type type;

   m = e_mod_move_util_get();
   E_CHECK(m);

   split_mode_indi_widget = e_mod_move_split_mode_indicator_widget_get();
   E_CHECK(split_mode_indi_widget);

   m->split_indicator_event_forwarding_disable = set;

   if (set)
      type = E_MOVE_EVENT_PROPAGATE_TYPE_NONE;
   else
      type = E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY;


   EINA_LIST_FOREACH(split_mode_indi_widget->objs, l, mwo)
     {
        if (!mwo) continue;
        e_mod_move_event_propagate_type_set(mwo->event,type);
     }

   e_mod_move_widget_objs_repeat_events_set(split_mode_indi_widget->objs, set);
}
