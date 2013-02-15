#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move_atoms.h"
#include "e_mod_move.h"

struct _E_Move_Apptray_Animation_Data
{
   Eina_Bool       animating;
   int             sx;// start x
   int             sy;// start y
   int             ex;// end x
   int             ey;// end y
   int             dx;// distance x
   int             dy;// distance y
   Ecore_Animator *animator;
};

/* local subsystem functions */
static Eina_Bool      _e_mod_move_apptray_cb_motion_start(void *data, void *event_info);
static Eina_Bool      _e_mod_move_apptray_cb_motion_move(void *data, void *event_info);
static Eina_Bool      _e_mod_move_apptray_cb_motion_end(void *data, void *event_info);
static Eina_Bool      _e_mod_move_apptray_objs_position_set(E_Move_Border *mb, int x, int y);
static Eina_Bool      _e_mod_move_apptray_objs_position_get(E_Move_Border *mb, int *x, int *y);
static Eina_Bool      _e_mod_move_apptray_objs_animation_frame(void  *data, double pos);
static Eina_Bool      _e_mod_move_apptray_flick_process(E_Move_Border *mb, int angle, Eina_Bool state);
static Eina_Bool      _e_mod_move_apptray_dim_objs_apply(E_Move_Border *mb, int x, int y);
static Eina_Bool      _e_mod_move_apptray_cb_bg_touch_dn(void *data, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_apptray_cb_bg_touch_up(void *data, int type __UNUSED__, void *event);
static void           _e_mod_move_apptray_bg_touch_win_show(E_Move_Border *mb);
static void           _e_mod_move_apptray_bg_touch_win_hide(E_Move_Border *mb);
static Ecore_X_Window _e_mod_move_apptray_bg_touch_win_get(E_Move_Border *mb);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_apptray_cb_motion_start(void *data,
                                    void *event_info)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;

   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mb || !info) return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   if (e_mod_move_apptray_objs_animation_state_get(mb)) goto error_cleanup;

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

   E_CHECK_GOTO(mb->visible, error_cleanup);
   if (!REGION_INSIDE_ZONE(mb, mb->bd->zone))
      goto error_cleanup;

   E_CHECK_GOTO(e_mod_move_flick_data_new(mb), error_cleanup);
   e_mod_move_flick_data_init(mb, info->coord.x, info->coord.y);

   e_mod_move_apptray_objs_add(mb);

   // apptray_objs_animation_layer_set
   e_mod_move_apptray_objs_animation_layer_set(mb);

   // send apptray to "move start message".
   e_mod_move_apptray_state_send(mb, EINA_TRUE);
   return EINA_TRUE;

error_cleanup:
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_mod_move_apptray_cb_motion_move(void *data,
                                   void *event_info)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;
   Eina_Bool click = EINA_FALSE;
   E_Zone *zone = NULL;

   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mb || !info) return EINA_FALSE;

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
   E_CHECK_RETURN(click, EINA_FALSE);

   switch (angle)
     {
      case   0:
         if (info->coord.y < mb->h)
           e_mod_move_apptray_objs_move(mb, 0, info->coord.y - mb->h);
         break;
      case  90:
         if (info->coord.x < mb->w)
           e_mod_move_apptray_objs_move(mb, info->coord.x - mb->w, 0);
         break;
      case 180:
         if (info->coord.y > (zone->h - mb->h))
           e_mod_move_apptray_objs_move(mb, 0, info->coord.y);
         break;
      case 270:
         if (info->coord.x > (zone->w - mb->w))
           e_mod_move_apptray_objs_move(mb, info->coord.x, 0);
         break;
      default :
         break;
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_cb_motion_end(void *data,
                                  void *event_info)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;
   E_Zone *zone = NULL;
   Eina_Bool click = EINA_FALSE;
   Eina_Bool flick_state = EINA_FALSE;
   int check_w, check_h;

   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mb || !info) return EINA_FALSE;

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
   E_CHECK_GOTO(click, finish);

   e_mod_move_flick_data_update(mb, info->coord.x, info->coord.y);
   flick_state = e_mod_move_flick_state_get(mb);
   if (_e_mod_move_apptray_flick_process(mb, angle, flick_state))
     {
        return EINA_TRUE;
     }

   switch (angle)
     {
      case   0:
         check_h = mb->y + mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y < check_h)
           {
              e_mod_move_apptray_e_border_move(mb, 0, mb->h * -1);
              e_mod_move_apptray_objs_animation_move(mb, 0, mb->h * -1);
           }
         else
           {
              e_mod_move_apptray_objs_animation_move(mb, 0, 0);
           }
         break;
      case  90:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x < check_w)
           {
              e_mod_move_apptray_e_border_move(mb, mb->w * -1, 0);
              e_mod_move_apptray_objs_animation_move(mb, mb->w * -1, 0);
           }
         else
           {
              e_mod_move_apptray_objs_animation_move(mb, 0, 0);
           }
         break;
      case 180:
         check_h = mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y > (zone->h - check_h))
           {
              e_mod_move_apptray_e_border_move(mb, 0, zone->h);
              e_mod_move_apptray_objs_animation_move(mb, 0, zone->h);
           }
         else
           {
              e_mod_move_apptray_objs_animation_move(mb, 0, (zone->h - mb->h));
           }
         break;
      case 270:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x > (zone->w - check_w))
           {
              e_mod_move_apptray_e_border_move(mb, zone->w, 0);
              e_mod_move_apptray_objs_animation_move(mb, zone->w, 0);
           }
         else
           {
              e_mod_move_apptray_objs_animation_move(mb, zone->w - mb->w, 0);
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

finish:
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_objs_position_set(E_Move_Border *mb,
                                      int            x,
                                      int            y)
{
   E_Move_Apptray_Data *at_data = NULL;
   at_data = (E_Move_Apptray_Data *)mb->data;
   if (!at_data)
     at_data = (E_Move_Apptray_Data *)e_mod_move_apptray_internal_data_add(mb);
   E_CHECK_RETURN(at_data, EINA_FALSE);

   at_data->x = x;
   at_data->y = y;
   mb->data = at_data;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_objs_position_get(E_Move_Border *mb,
                                      int           *x,
                                      int           *y)
{
   E_Move_Apptray_Data *at_data = NULL;
   at_data = (E_Move_Apptray_Data *)mb->data;
   E_CHECK_RETURN(at_data, EINA_FALSE);

   *x = at_data->x;
   *y = at_data->y;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_objs_animation_frame(void  *data,
                                         double pos)
{
   E_Move_Apptray_Animation_Data *anim_data = NULL;
   E_Move_Border                 *mb = NULL;
   double                         frame = pos;
   int                            x, y;

   anim_data = (E_Move_Apptray_Animation_Data *)data;
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   mb = e_mod_move_apptray_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;

   e_mod_move_apptray_objs_move(mb, x, y);

   if (pos >= 1.0)
     {
        // apptray_objs_animation_layer_unset
        e_mod_move_apptray_objs_animation_layer_unset(mb);

        if (!(REGION_INSIDE_ZONE(mb, mb->bd->zone)))
          {
             e_border_focus_set(mb->bd, 0, 0);
             e_border_lower(mb->bd);
             e_mod_move_apptray_dim_hide(mb);
          }

        // send apptray to "move end message".
        e_mod_move_apptray_state_send(mb, EINA_FALSE);

        e_mod_move_apptray_objs_del(mb);

        memset(anim_data, 0, sizeof(E_Move_Apptray_Animation_Data));
        E_FREE(anim_data);
        mb->anim_data = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_flick_process(E_Move_Border *mb,
                                  int            angle,
                                  Eina_Bool      state)
{
   E_Move_Control_Object *mco = NULL;
   E_Zone *zone = NULL;
   Eina_List *l;
   int x, y;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   /* apptray click unset */
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90: x = mb->w * -1; y = 0;          break;
      case 180: x = 0;          y = zone->h;    break;
      case 270: x = zone->w;    y = 0;          break;
      case   0:
      default : x = 0;          y = mb->h * -1; break;
     }

   e_mod_move_apptray_e_border_move(mb, x, y);
   e_mod_move_apptray_objs_animation_move(mb, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_dim_objs_apply(E_Move_Border *mb,
                                   int            x,
                                   int            y)
{
   int angle;
   int mx, my;
   int opacity;
   E_Zone *zone = NULL;
   int dim_max = 255;
   E_Move_Apptray_Data *at_data = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);

   at_data = (E_Move_Apptray_Data *)mb->data;
   E_CHECK_RETURN(at_data->dim_objs, EINA_FALSE);

   angle = mb->angle;
   zone = mb->bd->zone;
   dim_max = mb->m->dim_max_opacity;

   switch (angle)
     {
      case  90:
         mx = x + mb->w;
         if (mb->w)
           opacity = dim_max * mx / mb->w;
         else
           opacity = dim_max;
         break;
      case 180:
         my = zone->h - y;
         if (mb->h)
           opacity = dim_max * my / mb->h;
         else
           opacity = dim_max;
         break;
      case 270:
         mx = zone->w - x;
         if (mb->w)
           opacity = dim_max * mx / mb->w;
         else
           opacity = dim_max;
         break;
      case   0:
      default :
         my = y + mb->h;
         if (mb->h)
           opacity = dim_max * my / mb->h;
         else
           opacity = dim_max;
         break;
     }

   e_mod_move_bd_move_dim_objs_opacity_set(at_data->dim_objs, opacity);
   at_data->opacity = opacity;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_cb_bg_touch_dn(void    *data,
                                   int type __UNUSED__,
                                   void    *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_Move_Apptray_Data *at_data = (E_Move_Apptray_Data *)data;

   E_CHECK_RETURN(at_data, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   if (ev->event_window != at_data->bg_touch.win)
     return ECORE_CALLBACK_PASS_ON;

   at_data->bg_touch.click = EINA_TRUE;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_apptray_cb_bg_touch_up(void    *data,
                                   int type __UNUSED__,
                                   void    *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_Move_Apptray_Data      *at_data = (E_Move_Apptray_Data *)data;
   Eina_Bool                 state;
   E_Move_Border            *at_mb = NULL;
   E_Zone                   *zone = NULL;
   Ecore_X_Window            at_win;
   int                       angles[2];
   int                       x, y;
   x = 0; y = 0;

   E_CHECK_RETURN(at_data, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   if (ev->event_window != at_data->bg_touch.win)
     return ECORE_CALLBACK_PASS_ON;

   E_CHECK_RETURN(at_data->bg_touch.click, ECORE_CALLBACK_PASS_ON);

   at_mb = e_mod_move_apptray_find();
   E_CHECK_RETURN(at_mb, ECORE_CALLBACK_PASS_ON);

   at_win = e_mod_move_util_client_xid_get(at_mb);
   if (e_mod_move_util_win_prop_angle_get(at_win, &angles[0], &angles[1]))
     angles[0] %= 360;
   else
     angles[0] = 0;

   zone = at_mb->bd->zone;

   if (REGION_INSIDE_ZONE(at_mb, zone)) state = EINA_TRUE;
   else state = EINA_FALSE;

   if (state && at_mb->visible)
     {
        switch (angles[0])
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

        // send apptray to "move start message".
        e_mod_move_apptray_state_send(at_mb, EINA_TRUE);

        // apptray_objs_animation_layer_set
        e_mod_move_apptray_objs_animation_layer_set(at_mb);

        e_mod_move_apptray_e_border_move(at_mb, x, y);
        e_mod_move_apptray_objs_animation_move(at_mb, x, y);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_move_apptray_bg_touch_win_show(E_Move_Border *mb)
{
   E_Move_Apptray_Data *at_data = NULL;
   E_Border *bd = NULL;
   E_Zone *zone = NULL;

   E_CHECK(mb);
   E_CHECK(TYPE_APPTRAY_CHECK(mb));
   at_data = (E_Move_Apptray_Data *)mb->data;

   bd = mb->bd;
   zone = bd->zone;

   E_CHECK(at_data);

   if (at_data->bg_touch.win) return;

   at_data->bg_touch.win =  ecore_x_window_input_new(0, zone->x, zone->y,
                                                     zone->w, zone->h);
   ecore_x_icccm_title_set(at_data->bg_touch.win, "E MOVE Apptray BG Touch");
   ecore_x_window_show(at_data->bg_touch.win);

   at_data->bg_touch.dn_h = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                                    _e_mod_move_apptray_cb_bg_touch_dn,
                                                    (void*)at_data);
   at_data->bg_touch.up_h = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                                    _e_mod_move_apptray_cb_bg_touch_up,
                                                    (void*)at_data);

   // Stack re-arrange
   ecore_x_window_configure(at_data->bg_touch.win,
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                            zone->x, zone->y,
                            zone->w, zone->h, 0,
                            bd->win, ECORE_X_WINDOW_STACK_BELOW);
}

static void
_e_mod_move_apptray_bg_touch_win_hide(E_Move_Border *mb)
{
   E_Move_Apptray_Data *at_data = NULL;

   E_CHECK(mb);
   E_CHECK(TYPE_APPTRAY_CHECK(mb));
   at_data = (E_Move_Apptray_Data *)mb->data;

   E_CHECK(at_data);

   E_CHECK(at_data->bg_touch.win);

   if (at_data->bg_touch.dn_h)
     {
        ecore_event_handler_del(at_data->bg_touch.dn_h);
        at_data->bg_touch.dn_h = NULL;
     }
   if (at_data->bg_touch.up_h)
     {
        ecore_event_handler_del(at_data->bg_touch.up_h);
        at_data->bg_touch.up_h = NULL;
     }

   ecore_x_window_hide(at_data->bg_touch.win);
   ecore_x_window_free(at_data->bg_touch.win);
   at_data->bg_touch.win = 0;
}

static Ecore_X_Window
_e_mod_move_apptray_bg_touch_win_get(E_Move_Border *mb)
{
   E_Move_Apptray_Data *at_data = NULL;

   E_CHECK_RETURN(mb, 0);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), 0);
   at_data = (E_Move_Apptray_Data *)mb->data;

   E_CHECK_RETURN(at_data, 0);

   E_CHECK_RETURN(at_data->bg_touch.win, 0);

   return at_data->bg_touch.win;
}

/* externally accessible functions */
EINTERN void
e_mod_move_apptray_ctl_obj_event_setup(E_Move_Border         *mb,
                                       E_Move_Control_Object *mco)
{
   E_CHECK(mb);
   E_CHECK(mco);
   E_CHECK(TYPE_APPTRAY_CHECK(mb));

   mco->event = e_mod_move_event_new(mb->bd->client.win, mco->obj);
   E_CHECK(mco->event);

   e_mod_move_event_data_type_set(mco->event, E_MOVE_EVENT_DATA_TYPE_BORDER);
   e_mod_move_event_angle_cb_set(mco->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_apptray_cb_motion_start, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_apptray_cb_motion_move, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_apptray_cb_motion_end, mb);
}

EINTERN E_Move_Border *
e_mod_move_apptray_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_APPTRAY_CHECK(mb)) return mb;
     }
   return NULL;
}

EINTERN Eina_Bool
e_mod_move_apptray_click_get(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_apptray_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_apptray_event_clear(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_apptray_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   click = e_mod_move_apptray_click_get();
   E_CHECK_RETURN(click, EINA_FALSE);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_data_clear(mco->event);
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   if (mb->flick_data) e_mod_move_flick_data_free(mb);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_add(E_Move_Border *mb)
{
   Eina_Bool mirror = EINA_TRUE;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   if (!(mb->objs))
     {
        mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
        e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
        e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
        e_mod_move_bd_move_objs_show(mb);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_del(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   e_mod_move_bd_move_objs_del(mb, mb->objs);
   mb->objs = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_move(E_Move_Border *mb,
                             int            x,
                             int            y)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   e_mod_move_bd_move_objs_move(mb, x, y);
   _e_mod_move_apptray_objs_position_set(mb, x, y);
   _e_mod_move_apptray_dim_objs_apply(mb, x, y);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_raise(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   e_mod_move_bd_move_objs_raise(mb);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_move(E_Move_Border *mb,
                                       int            x,
                                       int            y)
{
   E_Move_Apptray_Animation_Data *anim_data = NULL;
   Ecore_Animator *animator = NULL;
   int sx, sy; //start x, start y
   double anim_time = 0.0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   anim_time = mb->m->animation_duration;

   if (e_mod_move_apptray_objs_animation_state_get(mb))
     {
        e_mod_move_apptray_objs_animation_stop(mb);
        e_mod_move_apptray_objs_animation_clear(mb);
     }

   anim_data = E_NEW(E_Move_Apptray_Animation_Data, 1);
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   if (_e_mod_move_apptray_objs_position_get(mb, &sx, &sy))
     {
        anim_data->sx = sx;
        anim_data->sy = sy;
     }
   else
     {
        anim_data->sx = mb->x;
        anim_data->sy = mb->y;
        _e_mod_move_apptray_objs_position_set(mb,
                                              anim_data->sx,
                                              anim_data->sy);
     }

   anim_data->ex = x;
   anim_data->ey = y;
   anim_data->dx = anim_data->ex - anim_data->sx;
   anim_data->dy = anim_data->ey - anim_data->sy;
   animator = ecore_animator_timeline_add(anim_time,
                                          _e_mod_move_apptray_objs_animation_frame,
                                          anim_data);
   if (!animator)
     {
        memset(anim_data, 0, sizeof(E_Move_Apptray_Animation_Data));
        E_FREE(anim_data);
        return EINA_FALSE;
     }

   anim_data->animator = animator;
   anim_data->animating = EINA_TRUE;
   mb->anim_data = anim_data;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_state_get(E_Move_Border *mb)
{
   E_Move_Apptray_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Apptray_Animation_Data *)mb->anim_data;
   E_CHECK_RETURN(anim_data->animating, EINA_FALSE);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_stop(E_Move_Border *mb)
{
   E_Move_Apptray_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Apptray_Animation_Data *)mb->anim_data;
   ecore_animator_freeze(anim_data->animator);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_clear(E_Move_Border *mb)
{
   E_Move_Apptray_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Apptray_Animation_Data *)mb->anim_data;
   ecore_animator_del(anim_data->animator);
   memset(anim_data, 0, sizeof(E_Move_Apptray_Animation_Data));
   E_FREE(anim_data);
   mb->anim_data = NULL;

   return EINA_TRUE;
}

EINTERN void*
e_mod_move_apptray_internal_data_add(E_Move_Border *mb)
{
   E_Move_Apptray_Data *at_data = NULL;
   int dim_min = 0;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), NULL);
   E_CHECK_RETURN(mb->m, NULL);
   at_data = (E_Move_Apptray_Data *)mb->data;
   dim_min = mb->m->dim_min_opacity;
   if (!at_data)
     {
        at_data = E_NEW(E_Move_Apptray_Data, 1);
        E_CHECK_RETURN(at_data, NULL);
        at_data->x = mb->x;
        at_data->y = mb->y;
        at_data->dim_objs = NULL;
        at_data->opacity = dim_min;
        mb->data = at_data;
     }
   return mb->data;
}

EINTERN Eina_Bool
e_mod_move_apptray_internal_data_del(E_Move_Border *mb)
{
   E_Move_Apptray_Data *at_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   at_data = (E_Move_Apptray_Data *)mb->data;
   e_mod_move_apptray_dim_hide(mb);
   E_FREE(at_data);
   mb->data = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_e_border_move(E_Move_Border *mb,
                                 int            x,
                                 int            y)
{
   E_Zone *zone;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);

   zone = mb->bd->zone;

   /* check whether zone contains border */
   if (E_CONTAINS(zone->x, zone->y,
                  zone->w, zone->h,
                  x, y, mb->w, mb->h))
     {
        e_border_raise(mb->bd);
        e_border_focus_set(mb->bd, 1, 1);
     }
   else
     {
        e_border_focus_set(mb->bd, 0, 0);
     }

   e_border_move(mb->bd, x, y);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_e_border_raise(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   e_border_raise(mb->bd);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_e_border_lower(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   e_border_lower(mb->bd);

   return EINA_TRUE;
}

EINTERN Eina_List*
e_mod_move_apptray_dim_show(E_Move_Border *mb)
{
   int dim_min = 0;
   E_Move_Apptray_Data *at_data = NULL;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), NULL);
   E_CHECK_RETURN(mb->m, NULL);
   at_data = (E_Move_Apptray_Data *)mb->data;
   dim_min = mb->m->dim_min_opacity;
   if (!at_data)
     {
        at_data = E_NEW(E_Move_Apptray_Data, 1);
        E_CHECK_RETURN(at_data, NULL);
        // Composite mode set true
        e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_TRUE);

        at_data->x = mb->x;
        at_data->y = mb->y;
        at_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
        e_mod_move_bd_move_dim_objs_show(at_data->dim_objs);
        at_data->opacity = dim_min;
        mb->data = at_data;
        e_mod_move_util_rotation_lock(mb->m);

        // it is used for apptray input only window
        _e_mod_move_apptray_bg_touch_win_show(mb);
     }
   else
     {
        if (!(at_data->dim_objs))
          {
             // Composite mode set true
             e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_TRUE);

             at_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
             if (at_data->dim_objs)
                e_mod_move_util_rotation_lock(mb->m);
          }
        if (at_data->dim_objs)
          {
             e_mod_move_bd_move_dim_objs_show(at_data->dim_objs);
             at_data->opacity = dim_min;
          }

        // it is used for apptray input only window
        _e_mod_move_apptray_bg_touch_win_show(mb);
     }

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   return at_data->dim_objs;
}

EINTERN Eina_Bool
e_mod_move_apptray_dim_hide(E_Move_Border *mb)
{
   int dim_min = 0;
   E_Move_Apptray_Data *at_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   at_data = (E_Move_Apptray_Data *)mb->data;
   dim_min = mb->m->dim_min_opacity;
   E_CHECK_RETURN(at_data->dim_objs, EINA_FALSE);
   e_mod_move_bd_move_dim_objs_hide(at_data->dim_objs);
   e_mod_move_bd_move_dim_objs_del(at_data->dim_objs);
   at_data->dim_objs = NULL;
   at_data->opacity = dim_min;

   // it is used for apptray input only window
   _e_mod_move_apptray_bg_touch_win_hide(mb);

   e_mod_move_util_rotation_unlock(mb->m);

   // Composite mode set false
   e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_FALSE);

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_restack_post_process(E_Move_Border *mb)
{
   Ecore_X_Window win;
   E_Border      *bd = NULL;
   E_Zone        *zone = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   if ((win = _e_mod_move_apptray_bg_touch_win_get(mb)))
     {
        // Stack re-arrange
        ecore_x_window_configure(win,
                                 ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                 ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                 zone->x, zone->y,
                                 zone->w, zone->h, 0,
                                 bd->win, ECORE_X_WINDOW_STACK_BELOW);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_start_position_set(E_Move_Border *mb,
                                                     int            angle)
{
   E_Zone *zone;
   int x, y;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   angle = ((angle % 360) / 90) * 90;
   zone = mb->bd->zone;

   switch (angle)
     {
      case  90: x = mb->w * -1;        y = 0;                 break;
      case 180: x = 0;                 y = zone->y + zone->h; break;
      case 270: x = zone->x + zone->w; y = 0;                 break;
      case   0:
      default : x = 0;                 y = mb->h * -1;        break;
     }

   _e_mod_move_apptray_objs_position_set(mb, x, y);

   return EINA_TRUE;
}

EINTERN E_Move_Event_Cb
e_mod_move_apptray_event_cb_get(E_Move_Event_Type type)
{
   if (type == E_MOVE_EVENT_TYPE_MOTION_START)
     return _e_mod_move_apptray_cb_motion_start;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_MOVE)
     return _e_mod_move_apptray_cb_motion_move;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_END)
     return _e_mod_move_apptray_cb_motion_end;
   else
     return NULL;
}

EINTERN Eina_Bool
e_mod_move_apptray_state_send(E_Move_Border *mb,
                              Eina_Bool state)
{
   long d[5] = {0L, 0L, 0L, 0L, 0L};
   Ecore_X_Window win;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);
   E_CHECK_RETURN(win, 0);

   if (state) d[0] = 1L;
   else d[0] = 0L;

   ecore_x_client_message32_send
     (win, ATOM_MV_APPTRAY_STATE,
     ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
     d[0], d[1], d[2], d[3], d[4]);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_layer_set(E_Move_Border *mb)
{
   E_Move              *m = e_mod_move_util_get();
   E_Move_Border       *find_mb = NULL;
   E_Move_Border       *indi_mb = NULL;
   E_Move_Border       *state_above_mb = NULL;
   E_Zone              *zone = NULL;
   Ecore_X_Window       win;
   Eina_Bool            found = EINA_FALSE;
   E_Move_Apptray_Data *at_data = NULL;

   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   at_data = (E_Move_Apptray_Data *)mb->data;
   E_CHECK_RETURN(at_data, EINA_FALSE);

   at_data->animation_layer_info.indicator.layer_set = EINA_FALSE;
   at_data->animation_layer_info.state_above.layer_set = EINA_FALSE;
   at_data->animation_layer_info.state_above.win = 0;

   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (find_mb->bd
            && (find_mb->bd->layer == e_mod_move_util_layer_policy_get(E_MOVE_STATE_ABOVE_LAYER))
            && (find_mb->visible))
          {
             win = find_mb->bd->client.win;
             found = EINA_TRUE;
          }

        if (find_mb->bd
            && (find_mb->bd->layer < e_mod_move_util_layer_policy_get(E_MOVE_STATE_ABOVE_LAYER)))
          {
             break;
          }
     }

   if (found)
     {
        at_data->animation_layer_info.state_above.layer_set = EINA_TRUE;
        at_data->animation_layer_info.state_above.win = win;
        state_above_mb = e_mod_move_border_client_find(win);
        if (state_above_mb
            && !(state_above_mb->objs))
          {
             state_above_mb->objs = e_mod_move_bd_move_objs_add(state_above_mb, EINA_TRUE);
             e_mod_move_bd_move_objs_move(state_above_mb, state_above_mb->x, state_above_mb->y);
             e_mod_move_bd_move_objs_resize(state_above_mb, state_above_mb->w, state_above_mb->h);
             e_mod_move_bd_move_objs_show(state_above_mb);
          }

        // apptray mirror object stack below to state_above object .
        e_mod_move_bd_move_objs_stack_below(mb, state_above_mb); // mb : apptray move border

        // indicator window mirror object stack control
        indi_mb = e_mod_move_indicator_find();
        zone = mb->bd->zone;
        if ((indi_mb)
            && (indi_mb->visible)
            && (REGION_INSIDE_ZONE(indi_mb, zone))
            && (!m->elm_indicator_mode))
          {
             at_data->animation_layer_info.indicator.layer_set = EINA_TRUE;
             if (!(indi_mb->objs))
               {
                  indi_mb->objs = e_mod_move_bd_move_objs_add(indi_mb, EINA_TRUE);
                  e_mod_move_bd_move_objs_move(indi_mb, indi_mb->x, indi_mb->y);
                  e_mod_move_bd_move_objs_resize(indi_mb, indi_mb->w, indi_mb->h);
                  e_mod_move_bd_move_objs_show(indi_mb);
               }

             // indicator mirror object stack below to apptray.
             e_mod_move_bd_move_objs_stack_below(indi_mb, mb); // mb : apptray move border
          }
     }
   else
     {
        e_mod_move_apptray_objs_raise(mb);
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_apptray_objs_animation_layer_unset(E_Move_Border *mb)
{
   E_Move_Border       *indi_mb = NULL;
   E_Move_Border       *state_above_mb = NULL;
   E_Move_Apptray_Data *at_data = NULL;
   Ecore_X_Window       win;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);

   at_data = (E_Move_Apptray_Data *)mb->data;
   E_CHECK_RETURN(at_data, EINA_FALSE);

   if (at_data->animation_layer_info.indicator.layer_set)
     {
        if ((indi_mb = e_mod_move_indicator_find())
             && (!indi_mb->animate_move))
          {
             e_mod_move_bd_move_objs_del(indi_mb, indi_mb->objs);
             indi_mb->objs = NULL;
          }
     }

   if (at_data->animation_layer_info.state_above.layer_set)
     {
        win = at_data->animation_layer_info.state_above.win;
        if ((state_above_mb = e_mod_move_border_client_find(win))
            && (!state_above_mb->animate_move))
          {
             e_mod_move_bd_move_objs_del(state_above_mb, state_above_mb->objs);
             state_above_mb->objs = NULL;
          }
     }

   at_data->animation_layer_info.state_above.layer_set = EINA_FALSE;
   at_data->animation_layer_info.indicator.layer_set = EINA_FALSE;
   at_data->animation_layer_info.state_above.win = 0;

   return EINA_TRUE;
}
