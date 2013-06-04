#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move_atoms.h"
#include "e_mod_move.h"

struct _E_Move_Mini_Apptray_Animation_Data
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
static Eina_Bool      _e_mod_move_mini_apptray_cb_motion_start(void *data, void *event_info);
static Eina_Bool      _e_mod_move_mini_apptray_cb_motion_move(void *data, void *event_info);
static Eina_Bool      _e_mod_move_mini_apptray_cb_motion_end(void *data, void *event_info);
static Eina_Bool      _e_mod_move_mini_apptray_objs_position_set(E_Move_Border *mb, int x, int y);
static Eina_Bool      _e_mod_move_mini_apptray_objs_position_get(E_Move_Border *mb, int *x, int *y);
static Eina_Bool      _e_mod_move_mini_apptray_objs_animation_frame(void  *data, double pos);
static Eina_Bool      _e_mod_move_mini_apptray_flick_process(E_Move_Border *mb, int angle, Eina_Bool state);
static Eina_Bool      _e_mod_move_mini_apptray_dim_objs_apply(E_Move_Border *mb, int x, int y);
static Eina_Bool      _e_mod_move_mini_apptray_ctl_event_send_policy_check(E_Move_Border *mb, Evas_Point pos);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_mini_apptray_cb_motion_start(void *data,
                                         void *event_info)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Evas_Event_Mouse_Down *mouse_down_event = NULL;
   Eina_Bool clicked = EINA_FALSE;
   Eina_List *l;
   int angle = 0;

   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mb || !info) return EINA_FALSE;

   mouse_down_event = info->event_info;
   E_CHECK_RETURN(mouse_down_event, EINA_FALSE);
   if (mouse_down_event->button != 1)
     return EINA_FALSE;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        clicked = e_mod_move_event_click_get(mco->event);
     }
   if (clicked)
     return EINA_FALSE;

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   if (e_mod_move_mini_apptray_objs_animation_state_get(mb)) goto error_cleanup;

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

   e_mod_move_mini_apptray_objs_add(mb);

   // send mini_apptray to "move start message".
   e_mod_move_mini_apptray_anim_state_send(mb, EINA_TRUE);
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
_e_mod_move_mini_apptray_cb_motion_move(void *data,
                                        void *event_info)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;
   Eina_Bool click = EINA_FALSE;
   E_Zone *zone = NULL;
   int x = 0, y = 0;
   int cx, cy, cw, ch;
   Eina_Bool contents_region = EINA_FALSE;

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

   if (_e_mod_move_mini_apptray_ctl_event_send_policy_check(mb, info->coord))
     {
        EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
          {
             if (!mco) continue;
             e_mod_move_event_propagate_type_set(mco->event, E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY);
          }
     }
   else
     {
        EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
          {
             if (!mco) continue;
             e_mod_move_event_propagate_type_set(mco->event, E_MOVE_EVENT_PROPAGATE_TYPE_NONE);
          }
     }

   contents_region = e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch);

   switch (angle)
     {
      case   0:
         if (info->coord.y > (zone->h - mb->h))
           {
              if (contents_region)
                {
                   if (info->coord.y > cy) y =  info->coord.y - cy;
                }
              else
                {
                   y =  info->coord.y;
                }
           }
         break;
      case  90:
         if (info->coord.x > (zone->w - mb->w))
           {
              if (contents_region)
                {
                   if (info->coord.x > cx) x = info->coord.x - cx;
                }
              else
                {
                   x = info->coord.x;
                }
           }
         break;
      case 180:
         if (info->coord.y < mb->h)
           {
              if (contents_region)
                {
                   if (info->coord.y < ch) y = info->coord.y - ch;
                }
              else
                {
                   y = info->coord.y - mb->h;
                }
           }
         break;
      case 270:
         if (info->coord.x < mb->w)
           {
              if (contents_region)
                {
                   if (info->coord.x < cw) x = info->coord.x - cw;
                }
              else
                {
                   x = info->coord.x - mb->w;
                }
           }
         break;
      default :
         break;
     }

   e_mod_move_mini_apptray_objs_move(mb, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_cb_motion_end(void *data,
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
   Evas_Event_Mouse_Up *mouse_up_event;
   int check_w, check_h;

   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mb || !info) return EINA_FALSE;

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

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }
   E_CHECK_GOTO(click, finish);

   // event send all set
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_propagate_type_set(mco->event, E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY);
     }

   e_mod_move_flick_data_update(mb, info->coord.x, info->coord.y);
   flick_state = e_mod_move_flick_state_get(mb, EINA_FALSE);
   if (_e_mod_move_mini_apptray_flick_process(mb, angle, flick_state))
     {
        return EINA_TRUE;
     }

   switch (angle)
     {
      case   0:
         check_h = mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y > (zone->h - check_h))
           {
              e_mod_move_mini_apptray_e_border_move(mb, 0, zone->h);
              e_mod_move_mini_apptray_objs_animation_move(mb, 0, zone->h);
           }
         else
           {
              e_mod_move_mini_apptray_objs_animation_move(mb, 0, (zone->h - mb->h));
           }
         break;
      case  90:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x > (zone->w - check_w))
           {
              e_mod_move_mini_apptray_e_border_move(mb, zone->w, 0);
              e_mod_move_mini_apptray_objs_animation_move(mb, zone->w, 0);
           }
         else
           {
              e_mod_move_mini_apptray_objs_animation_move(mb, zone->w - mb->w, 0);
           }
         break;
      case 180:
         check_h = mb->y + mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y < check_h)
           {
              e_mod_move_mini_apptray_e_border_move(mb, 0, mb->h * -1);
              e_mod_move_mini_apptray_objs_animation_move(mb, 0, mb->h * -1);
           }
         else
           {
              e_mod_move_mini_apptray_objs_animation_move(mb, 0, 0);
           }
         break;
      case 270:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x < check_w)
           {
              e_mod_move_mini_apptray_e_border_move(mb, mb->w * -1, 0);
              e_mod_move_mini_apptray_objs_animation_move(mb, mb->w * -1, 0);
           }
         else
           {
              e_mod_move_mini_apptray_objs_animation_move(mb, 0, 0);
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
_e_mod_move_mini_apptray_objs_position_set(E_Move_Border *mb,
                                           int            x,
                                           int            y)
{
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   if (!mini_apptray_data)
     mini_apptray_data = (E_Move_Mini_Apptray_Data *)e_mod_move_mini_apptray_internal_data_add(mb);
   E_CHECK_RETURN(mini_apptray_data, EINA_FALSE);

   mini_apptray_data->x = x;
   mini_apptray_data->y = y;
   mb->data = mini_apptray_data;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_objs_position_get(E_Move_Border *mb,
                                           int           *x,
                                           int           *y)
{
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   E_CHECK_RETURN(mini_apptray_data, EINA_FALSE);

   *x = mini_apptray_data->x;
   *y = mini_apptray_data->y;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_objs_animation_frame(void  *data,
                                              double pos)
{
   E_Move_Mini_Apptray_Animation_Data *anim_data = NULL;
   E_Move_Border                      *mb = NULL;
   double                              frame = pos;
   int                                 x, y;

   anim_data = (E_Move_Mini_Apptray_Animation_Data *)data;
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   mb = e_mod_move_mini_apptray_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;

   e_mod_move_mini_apptray_objs_move(mb, x, y);

   if (pos >= 1.0)
     {
        if (!(REGION_INSIDE_ZONE(mb, mb->bd->zone)))
          {
             e_border_focus_set(mb->bd, 0, 0);
             e_mod_move_mini_apptray_dim_hide(mb);
          }

        // send mini_apptray to "move end message".
        e_mod_move_mini_apptray_anim_state_send(mb, EINA_FALSE);

        e_mod_move_mini_apptray_objs_del(mb);

        memset(anim_data, 0, sizeof(E_Move_Mini_Apptray_Animation_Data));
        E_FREE(anim_data);
        mb->anim_data = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_flick_process(E_Move_Border *mb,
                                       int            angle,
                                       Eina_Bool      state)
{
   E_Move_Control_Object *mco = NULL;
   E_Zone *zone = NULL;
   Eina_List *l;
   int x, y;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   /* mini_apptray click unset */
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90: x = zone->w;    y = 0;          break;
      case 180: x = 0;          y = mb->h * -1; break;
      case 270:    x = mb->w * -1; y = 0;       break;
      case   0:
      default : x = 0;          y = zone->h;    break;
     }

   e_mod_move_mini_apptray_e_border_move(mb, x, y);
   e_mod_move_mini_apptray_objs_animation_move(mb, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_dim_objs_apply(E_Move_Border *mb,
                                        int            x,
                                        int            y)
{
   int angle;
   int mx, my;
   int opacity;
   E_Zone *zone = NULL;
   int dim_max = 255;
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);

   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   E_CHECK_RETURN(mini_apptray_data->dim_objs, EINA_FALSE);

   angle = mb->angle;
   zone = mb->bd->zone;
   dim_max = mb->m->dim_max_opacity;

   switch (angle)
     {
      case  90:
         mx = zone->w - x;
         if (mb->w)
           opacity = dim_max * mx / mb->w;
         else
           opacity = dim_max;
         break;
      case 180:
         my = y + mb->h;
         if (mb->h)
           opacity = dim_max * my / mb->h;
         else
           opacity = dim_max;
         break;
      case 270:
         mx = x + mb->w;
         if (mb->w)
           opacity = dim_max * mx / mb->w;
         else
           opacity = dim_max;
         break;
      case   0:
      default :
         my = zone->h - y;
         if (mb->h)
           opacity = dim_max * my / mb->h;
         else
           opacity = dim_max;
         break;
     }

   e_mod_move_bd_move_dim_objs_opacity_set(mini_apptray_data->dim_objs, opacity);
   mini_apptray_data->opacity = opacity;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_apptray_ctl_event_send_policy_check(E_Move_Border *mb,
                                                     Evas_Point     pos)
{
   int x = 0, y = 0, w = 0, h = 0;
   Eina_Bool ret = EINA_FALSE;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   e_mod_move_bd_move_ctl_objs_geometry_get(mb, &x ,&y, &w, &h);

   if (E_INSIDE(pos.x, pos.y, x, y, w, h)) ret = EINA_TRUE;

   return ret;
}

/* externally accessible functions */
EINTERN void
e_mod_move_mini_apptray_ctl_obj_event_setup(E_Move_Border         *mb,
                                            E_Move_Control_Object *mco)
{
   E_CHECK(mb);
   E_CHECK(mco);
   E_CHECK(TYPE_MINI_APPTRAY_CHECK(mb));

   mco->event = e_mod_move_event_new(mb->bd->client.win, mco->obj);
   E_CHECK(mco->event);

   e_mod_move_event_data_type_set(mco->event, E_MOVE_EVENT_DATA_TYPE_BORDER);
   e_mod_move_event_angle_cb_set(mco->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_mini_apptray_cb_motion_start, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_mini_apptray_cb_motion_move, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_mini_apptray_cb_motion_end, mb);
   e_mod_move_event_propagate_type_set(mco->event,
                                       E_MOVE_EVENT_PROPAGATE_TYPE_IMMEDIATELY);
}

EINTERN E_Move_Border *
e_mod_move_mini_apptray_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_MINI_APPTRAY_CHECK(mb)) return mb;
     }
   return NULL;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_click_get(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_mini_apptray_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_event_clear(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_mini_apptray_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   click = e_mod_move_mini_apptray_click_get();
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
e_mod_move_mini_apptray_objs_add(E_Move_Border *mb)
{
   Eina_Bool mirror = EINA_TRUE;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   if (!(mb->objs))
     {
        mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
        e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
        e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
        e_mod_move_bd_move_objs_show(mb);
        if (mb->objs) e_mod_move_util_rotation_lock(mb->m);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_add_with_pos(E_Move_Border *mb,
                                          int            x,
                                          int            y)
{
   Eina_Bool mirror = EINA_TRUE;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   if (!(mb->objs))
     {
        mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
        e_mod_move_bd_move_objs_move(mb, x, y);
        e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
        e_mod_move_bd_move_objs_show(mb);
        if (mb->objs) e_mod_move_util_rotation_lock(mb->m);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_del(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   e_mod_move_bd_move_objs_del(mb, mb->objs);
   e_mod_move_util_rotation_unlock(mb->m);
   mb->objs = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_move(E_Move_Border *mb,
                                  int            x,
                                  int            y)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   e_mod_move_bd_move_objs_move(mb, x, y);
   _e_mod_move_mini_apptray_objs_position_set(mb, x, y);
   _e_mod_move_mini_apptray_dim_objs_apply(mb, x, y);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_raise(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   e_mod_move_bd_move_objs_raise(mb);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_move(E_Move_Border *mb,
                                            int            x,
                                            int            y)
{
   E_Move_Mini_Apptray_Animation_Data *anim_data = NULL;
   Ecore_Animator *animator = NULL;
   int sx, sy; //start x, start y
   double anim_time = 0.0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   anim_time = mb->m->animation_duration;

   if (e_mod_move_mini_apptray_objs_animation_state_get(mb))
     {
        e_mod_move_mini_apptray_objs_animation_stop(mb);
        e_mod_move_mini_apptray_objs_animation_clear(mb);
     }

   anim_data = E_NEW(E_Move_Mini_Apptray_Animation_Data, 1);
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   if (_e_mod_move_mini_apptray_objs_position_get(mb, &sx, &sy))
     {
        anim_data->sx = sx;
        anim_data->sy = sy;
     }
   else
     {
        anim_data->sx = mb->x;
        anim_data->sy = mb->y;
        _e_mod_move_mini_apptray_objs_position_set(mb,
                                                   anim_data->sx,
                                                   anim_data->sy);
     }

   anim_data->ex = x;
   anim_data->ey = y;
   anim_data->dx = anim_data->ex - anim_data->sx;
   anim_data->dy = anim_data->ey - anim_data->sy;
   animator = ecore_animator_timeline_add(anim_time,
                                          _e_mod_move_mini_apptray_objs_animation_frame,
                                          anim_data);
   if (!animator)
     {
        memset(anim_data, 0, sizeof(E_Move_Mini_Apptray_Animation_Data));
        E_FREE(anim_data);
        return EINA_FALSE;
     }

   anim_data->animator = animator;
   anim_data->animating = EINA_TRUE;
   mb->anim_data = anim_data;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_state_get(E_Move_Border *mb)
{
   E_Move_Mini_Apptray_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Mini_Apptray_Animation_Data *)mb->anim_data;
   E_CHECK_RETURN(anim_data->animating, EINA_FALSE);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_stop(E_Move_Border *mb)
{
   E_Move_Mini_Apptray_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Mini_Apptray_Animation_Data *)mb->anim_data;
   ecore_animator_freeze(anim_data->animator);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_clear(E_Move_Border *mb)
{
   E_Move_Mini_Apptray_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Mini_Apptray_Animation_Data *)mb->anim_data;
   ecore_animator_del(anim_data->animator);
   memset(anim_data, 0, sizeof(E_Move_Mini_Apptray_Animation_Data));
   E_FREE(anim_data);
   mb->anim_data = NULL;

   return EINA_TRUE;
}

EINTERN void*
e_mod_move_mini_apptray_internal_data_add(E_Move_Border *mb)
{
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   int dim_min = 0;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), NULL);
   E_CHECK_RETURN(mb->m, NULL);
   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   dim_min = mb->m->dim_min_opacity;
   if (!mini_apptray_data)
     {
        mini_apptray_data = E_NEW(E_Move_Mini_Apptray_Data, 1);
        E_CHECK_RETURN(mini_apptray_data, NULL);
        mini_apptray_data->x = mb->x;
        mini_apptray_data->y = mb->y;
        mini_apptray_data->dim_objs = NULL;
        mini_apptray_data->opacity = dim_min;
        mb->data = mini_apptray_data;
     }
   return mb->data;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_internal_data_del(E_Move_Border *mb)
{
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   e_mod_move_mini_apptray_dim_hide(mb);
   E_FREE(mini_apptray_data);
   mb->data = NULL;
   e_mod_move_mini_apptray_widget_apply();// disable/destory mini_apptray_widget related datas
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_e_border_move(E_Move_Border *mb,
                                      int            x,
                                      int            y)
{
   E_Zone *zone;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);

   zone = mb->bd->zone;

   /* check whether zone contains border */
   if (E_CONTAINS(zone->x, zone->y,
                  zone->w, zone->h,
                  x, y, mb->w, mb->h))
     {
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
e_mod_move_mini_apptray_e_border_raise(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   e_border_raise(mb->bd);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_e_border_lower(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   e_border_lower(mb->bd);

   return EINA_TRUE;
}

EINTERN Eina_List*
e_mod_move_mini_apptray_dim_show(E_Move_Border *mb)
{
   int dim_min = 0;
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), NULL);
   E_CHECK_RETURN(mb->m, NULL);
   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   dim_min = mb->m->dim_min_opacity;
   if (!mini_apptray_data)
     {
        mini_apptray_data = E_NEW(E_Move_Mini_Apptray_Data, 1);
        E_CHECK_RETURN(mini_apptray_data, NULL);
        // Composite mode set true
        e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_TRUE);

        mini_apptray_data->x = mb->x;
        mini_apptray_data->y = mb->y;
        mini_apptray_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
        e_mod_move_bd_move_dim_objs_show(mini_apptray_data->dim_objs);
        mini_apptray_data->opacity = dim_min;
        mb->data = mini_apptray_data;
     }
   else
     {
        if (!(mini_apptray_data->dim_objs))
          {
             // Composite mode set true
             e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_TRUE);

             mini_apptray_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
          }
        if (mini_apptray_data->dim_objs)
          {
             e_mod_move_bd_move_dim_objs_show(mini_apptray_data->dim_objs);
             mini_apptray_data->opacity = dim_min;
          }
     }

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   return mini_apptray_data->dim_objs;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_dim_hide(E_Move_Border *mb)
{
   int dim_min = 0;
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   dim_min = mb->m->dim_min_opacity;
   E_CHECK_RETURN(mini_apptray_data->dim_objs, EINA_FALSE);
   e_mod_move_bd_move_dim_objs_hide(mini_apptray_data->dim_objs);
   e_mod_move_bd_move_dim_objs_del(mini_apptray_data->dim_objs);
   mini_apptray_data->dim_objs = NULL;
   mini_apptray_data->opacity = dim_min;

   // Composite mode set false
   e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_FALSE);

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", ((mb->bd) ? mb->bd->win : NULL), __func__);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_start_position_set(E_Move_Border *mb,
                                                          int            angle)
{
   E_Zone *zone;
   int x, y;
   int cx, cy, cw, ch;
   Eina_Bool contents_region = EINA_FALSE;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   angle = ((angle % 360) / 90) * 90;
   zone = mb->bd->zone;
   contents_region = e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch);

// change later
// todo flick-up position
   switch (angle)
     {
      case  90:
         if (contents_region) { x = zone->x + zone->w - cx; y = 0; }
         else                 { x = zone->x + zone->w;      y = 0; }
         break;
      case 180:
         if (contents_region) { x = 0; y = mb->h * -1 + (mb->h - ch); }
         else                 { x = 0; y = mb->h * -1;                }
         break;
      case 270:
         if (contents_region) { x = mb->w * -1 + (mb->w - cw); y = 0; }
         else                 { x = mb->w * -1;                y = 0; }
         break;
      case   0:
      default :
         if (contents_region) { x = 0; y = zone->y + zone->h - cy; }
         else                 { x = 0; y = zone->y + zone->h;      }
         break;
     }

   _e_mod_move_mini_apptray_objs_position_set(mb, x, y);

   return EINA_TRUE;
}

EINTERN E_Move_Event_Cb
e_mod_move_mini_apptray_event_cb_get(E_Move_Event_Type type)
{
   if (type == E_MOVE_EVENT_TYPE_MOTION_START)
     return _e_mod_move_mini_apptray_cb_motion_start;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_MOVE)
     return _e_mod_move_mini_apptray_cb_motion_move;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_END)
     return _e_mod_move_mini_apptray_cb_motion_end;
   else
     return NULL;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_anim_state_send(E_Move_Border *mb,
                                        Eina_Bool state)
{
   long d[5] = {0L, 0L, 0L, 0L, 0L};
   Ecore_X_Window win;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);
   E_CHECK_RETURN(win, 0);

   if (state) d[0] = 1L;
   else d[0] = 0L;

   ecore_x_client_message32_send
     (win, ATOM_MV_MINI_APPTRAY_STATE,
     ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
     d[0], d[1], d[2], d[3], d[4]);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_layer_set(E_Move_Border *mb)
{
   E_Move                  *m = e_mod_move_util_get();
   E_Move_Border           *find_mb = NULL;
   E_Move_Border           *state_above_mb = NULL;
   Ecore_X_Window           win;
   Eina_Bool                found = EINA_FALSE;
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;

   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   E_CHECK_RETURN(mini_apptray_data, EINA_FALSE);

   mini_apptray_data->animation_layer_info.state_above.layer_set = EINA_FALSE;
   mini_apptray_data->animation_layer_info.state_above.win = 0;

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
        mini_apptray_data->animation_layer_info.state_above.layer_set = EINA_TRUE;
        mini_apptray_data->animation_layer_info.state_above.win = win;
        state_above_mb = e_mod_move_border_client_find(win);
        if (state_above_mb
            && !(state_above_mb->objs))
          {
             state_above_mb->objs = e_mod_move_bd_move_objs_add(state_above_mb, EINA_TRUE);
             e_mod_move_bd_move_objs_move(state_above_mb, state_above_mb->x, state_above_mb->y);
             e_mod_move_bd_move_objs_resize(state_above_mb, state_above_mb->w, state_above_mb->h);
             e_mod_move_bd_move_objs_show(state_above_mb);
          }

        // mini_apptray mirror object stack below to state_above object .
        e_mod_move_bd_move_objs_stack_below(mb, state_above_mb); // mb : mini_apptray move border
     }
   else
     {
        e_mod_move_mini_apptray_objs_raise(mb);
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_mini_apptray_objs_animation_layer_unset(E_Move_Border *mb)
{
   E_Move_Border            *state_above_mb = NULL;
   E_Move_Mini_Apptray_Data *mini_apptray_data = NULL;
   Ecore_X_Window            win;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   mini_apptray_data = (E_Move_Mini_Apptray_Data *)mb->data;
   E_CHECK_RETURN(mini_apptray_data, EINA_FALSE);

   if (mini_apptray_data->animation_layer_info.state_above.layer_set)
     {
        win = mini_apptray_data->animation_layer_info.state_above.win;
        if ((state_above_mb = e_mod_move_border_client_find(win))
            && (!state_above_mb->animate_move))
          {
             e_mod_move_bd_move_objs_del(state_above_mb, state_above_mb->objs);
             state_above_mb->objs = NULL;
          }
     }

   mini_apptray_data->animation_layer_info.state_above.layer_set = EINA_FALSE;
   mini_apptray_data->animation_layer_info.state_above.win = 0;

   return EINA_TRUE;
}
