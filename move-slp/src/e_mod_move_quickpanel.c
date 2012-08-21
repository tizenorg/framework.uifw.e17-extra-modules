#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

struct _E_Move_Quickpanel_Animation_Data
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
static Eina_Bool          _e_mod_move_quickpanel_cb_motion_start(void *data, void *event_info);
static Eina_Bool          _e_mod_move_quickpanel_cb_motion_move(void *data, void *event_info);
static Eina_Bool          _e_mod_move_quickpanel_cb_motion_end(void *data, void *event_info);
static Eina_Bool          _e_mod_move_quickpanel_objs_position_set(E_Move_Border *mb, int x, int y);
static Eina_Bool          _e_mod_move_quickpanel_objs_position_get(E_Move_Border *mb, int *x, int *y);
static Eina_Bool          _e_mod_move_quickpanel_objs_animation_frame(void  *data, double pos);
static Eina_Bool          _e_mod_move_quickpanel_flick_process(E_Move_Border *mb, int angle, Eina_Bool state);
static Eina_Bool          _e_mod_move_quickpanel_dim_objs_apply(E_Move_Border *mb, int x, int y);
static Eina_Bool          _e_mod_move_quickpanel_below_window_set(void);
static void               _e_mod_move_quickpanel_below_window_unset(void);
static Eina_Bool          _e_mod_move_quickpanel_below_window_objs_add(void);
static Eina_Bool          _e_mod_move_quickpanel_below_window_objs_del(void);
static Eina_Bool          _e_mod_move_quickpanel_below_window_objs_move(int x, int y);

/* local subsystem functions */
static Eina_Bool
_e_mod_move_quickpanel_cb_motion_start(void *data,
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
     "[MOVE] ev:%15.15s w:0x%08x %s()\n", "EVAS_OBJ", mb->bd->win, __func__);

   if (e_mod_move_quickpanel_objs_animation_state_get(mb)) goto error_cleanup;

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
   if (!(REGION_INTERSECTS_WITH_ZONE(mb, mb->bd->zone)))
      goto error_cleanup;

   E_CHECK_GOTO(e_mod_move_flick_data_new(mb), error_cleanup);
   e_mod_move_flick_data_init(mb, info->coord.x, info->coord.y);

   e_mod_move_quickpanel_objs_add(mb);

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
_e_mod_move_quickpanel_cb_motion_move(void *data,
                                      void *event_info)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Event_Motion_Info *info;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int angle = 0;
   E_Zone *zone = NULL;
   Eina_Bool click = EINA_FALSE;
   int cx, cy, cw, ch;

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
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   if (info->coord.y < ch)
                     {
                        e_mod_move_quickpanel_objs_move(mb, 0,
                                                        info->coord.y - ch);
                     }
                }
              else
                {
                   e_mod_move_quickpanel_objs_move(mb, 0,
                                                   info->coord.y - mb->h);
                }
           }
         break;
      case  90:
         if (info->coord.x < mb->w)
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   if (info->coord.x < cw)
                     {
                        e_mod_move_quickpanel_objs_move(mb, info->coord.x - cw,
                                                        0);
                     }
                }
              else
                {
                   e_mod_move_quickpanel_objs_move(mb, info->coord.x - mb->w,
                                                   0);
                }
           }
         break;
      case 180:
         if (info->coord.y > (zone->h - mb->h))
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   if (info->coord.y > cy)
                     {
                        e_mod_move_quickpanel_objs_move(mb, 0,
                                                        info->coord.y - cy);
                     }
                }
              else
                {
                   e_mod_move_quickpanel_objs_move(mb, 0, info->coord.y);
                }
           }
         break;
      case 270:
         if (info->coord.x > (zone->w - mb->w))
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   if (info->coord.x > cx)
                     {
                        e_mod_move_quickpanel_objs_move(mb,
                                                        info->coord.x - cx,
                                                        0);
                     }
                }
              else
                {
                   e_mod_move_quickpanel_objs_move(mb, info->coord.x, 0);
                }
           }
         break;
      default :
         break;
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_cb_motion_end(void *data,
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
   int cx, cy, cw, ch;
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
   if (_e_mod_move_quickpanel_flick_process(mb, angle, flick_state))
     {
        return EINA_TRUE;
     }

   switch (angle)
     {
      case   0:
         check_h = mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y < check_h)
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   check_h = ch;
                   if (check_h) check_h /= 2;
                   if (info->coord.y < check_h)
                     {
                        e_mod_move_quickpanel_e_border_move(mb, 0, mb->h * -1);
                        e_mod_move_quickpanel_objs_animation_move(mb, 0, ch * -1);
                     }
                   else
                     {
                        e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                     }
                }
           }
         else
           {
              e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
           }
         break;
      case  90:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x < check_w)
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   check_w = cw;
                   if (check_w) check_w /= 2;
                   if (info->coord.x < check_w)
                     {
                        e_mod_move_quickpanel_e_border_move(mb, mb->w * -1, 0);
                        e_mod_move_quickpanel_objs_animation_move(mb, cw * -1, 0);
                     }
                   else
                     {
                        e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                     }
                }
           }
         else
           {
              e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
           }
         break;
      case 180:
         check_h = mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y > (zone->h - check_h))
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   check_h = ch;
                   if (check_h) check_h /= 2;
                   if (info->coord.y > (cy + check_h))
                     {
                        e_mod_move_quickpanel_e_border_move(mb, 0, zone->h);
                        e_mod_move_quickpanel_objs_animation_move(mb, 0, ch);
                     }
                   else
                     {
                        e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                     }
                }
           }
         else
           {
              e_mod_move_quickpanel_e_border_move(mb, 0, 0);
              e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
           }
         break;
      case 270:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x > (zone->w - check_w))
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                {
                   check_w = cw;
                   if (check_w) check_w /= 2;
                   if (info->coord.x > (cx + check_w))
                     {
                        e_mod_move_quickpanel_e_border_move(mb, zone->w, 0);
                        e_mod_move_quickpanel_objs_animation_move(mb, cw, 0);
                     }
                   else
                     {
                        e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                     }
                }
           }
         else
           {
              e_mod_move_quickpanel_e_border_move(mb, 0, 0);
              e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
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
_e_mod_move_quickpanel_objs_position_set(E_Move_Border *mb,
                                         int            x,
                                         int            y)
{
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   if (!qp_data)
     qp_data = (E_Move_Quickpanel_Data *)e_mod_move_quickpanel_internal_data_add(mb);
   E_CHECK_RETURN(qp_data, EINA_FALSE);
   qp_data->x = x; qp_data->y = y;
   mb->data = qp_data;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_objs_position_get(E_Move_Border *mb,
                                         int           *x,
                                         int           *y)
{
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data, EINA_FALSE);
   *x = qp_data->x; *y = qp_data->y;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_objs_animation_frame(void  *data,
                                            double pos)
{
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   E_Move_Border                    *mb = NULL;
   E_Move_Border                    *find_mb = NULL;
   E_Border                         *bd = NULL;
   E_Zone                           *zone = NULL;
   double                            frame = pos;
   int                               x, y;
   Ecore_X_Window                    win;
   E_Move                           *m = NULL;

   anim_data = (E_Move_Quickpanel_Animation_Data *)data;
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(mb, EINA_FALSE);
   m = mb->m;

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;

   e_mod_move_quickpanel_objs_move(mb, x, y);

   if (pos >= 1.0)
     {
         e_mod_move_quickpanel_objs_del(mb);
         bd = mb->bd;
         zone = bd->zone;
         if (!(REGION_INTERSECTS_WITH_ZONE(mb, mb->bd->zone))) // it work when quickpanel is not equal to zone size
           {
              win = e_mod_move_util_client_xid_get(mb);

              EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
                {
                   if (find_mb->visible)
                     {
                        if (find_mb->bd)
                           ecore_x_e_illume_quickpanel_state_send(find_mb->bd->client.win,
                                                                  ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
                     }
                }

              e_mod_move_quickpanel_dim_hide(mb);
           }
         memset(anim_data, 0, sizeof(E_Move_Quickpanel_Animation_Data));
         E_FREE(anim_data);
         mb->anim_data = NULL;
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_flick_process(E_Move_Border *mb,
                                     int            angle,
                                     Eina_Bool      state)
{
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int x;
   int y;
   E_Zone *zone = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco) // apptray click unset
     {
        if (!mco) continue;
        e_mod_move_event_click_set(mco->event, EINA_FALSE);
     }

   e_mod_move_flick_data_free(mb);

   switch (angle)
     {
      case  90:
         x = mb->w * -1;
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
      case   0:
      default :
         x = 0;
         y = mb->h * -1;
         break;
     }

   e_mod_move_quickpanel_e_border_move(mb, x, y);
   e_mod_move_quickpanel_objs_animation_move(mb, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_dim_objs_apply(E_Move_Border *mb,
                                      int            x,
                                      int            y)
{
   int angle;
   int mx, my;
   int opacity;
   int dim_max;
   E_Zone *zone = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   int cx, cy, cw, ch;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);

   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data->dim_objs, EINA_FALSE);

   angle = mb->angle;
   zone = mb->bd->zone;
   dim_max = mb->m->dim_max_opacity;

   switch (angle)
     {
      case  90:
         if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
           {
              mx = x + cw;
              if (cw)
                opacity = dim_max * mx / cw;
              else
                opacity = dim_max;
           }
         else
           {
              mx = x + mb->w;
              if (mb->w)
                opacity = dim_max * mx / mb->w;
              else
                opacity = dim_max;
           }
         break;
      case 180:
         if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
           {
              my = zone->h - (cy + y);
              if (ch)
                opacity = dim_max * my / ch;
              else
                opacity = dim_max;
           }
         else
           {
              my = zone->h - y;
              if (zone->h)
                opacity = dim_max * my / zone->h;
              else
                opacity = dim_max;
           }
         break;
      case 270:
         if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
           {
              mx = zone->w - (cx + x);
              if (cw)
                opacity = dim_max * mx / cw;
              else
                opacity = dim_max;
           }
         else
           {
              mx = zone->w - x;
              if (zone->w)
                opacity = dim_max * mx / zone->w;
              else
                opacity = dim_max;
           }
         break;
      case   0:
      default :
         if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
           {
              my = y + ch;
              if (ch)
                opacity = dim_max * my / ch;
              else
                opacity = dim_max;
           }
         else
           {
              my = y + mb->h;
              if (mb->h)
                opacity = dim_max * my / mb->h;
              else
                opacity = dim_max;
           }
         break;
     }

   e_mod_move_bd_move_dim_objs_opacity_set(qp_data->dim_objs, opacity);
   qp_data->opacity = opacity;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_below_window_set(void)
{
   E_Move *m;
   E_Move_Border *mb;
   E_Zone *zone = NULL;
   Eina_Bool found = EINA_FALSE;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (!found && TYPE_QUICKPANEL_CHECK(mb))
          {
             found = EINA_TRUE;
             zone = mb->bd->zone;
          }

        if (found)
          {
             if (!TYPE_QUICKPANEL_CHECK(mb)
                 && !TYPE_INDICATOR_CHECK(mb)
                 && !TYPE_APPTRAY_CHECK(mb)
                 && E_INTERSECTS(mb->x, mb->y, mb->w, mb->h,
                                 zone->x, zone->y, zone->w, zone->h)
                 && (mb->visible))
               {
                  L(LT_EVENT_OBJ,
                    "[MOVE] ev:%15.15s w:0x%08x %s() \n", "EVAS_OBJ",
                    mb->bd->win, __func__);
                  mb->animate_move = EINA_TRUE;
               }
          }
     }
   return found;
}

static void
_e_mod_move_quickpanel_below_window_unset(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK(m);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s w:0x%08x %s() \n", "EVAS_OBJ",
          mb->bd->win, __func__);
        mb->animate_move = EINA_FALSE;
     }
}

static Eina_Bool
_e_mod_move_quickpanel_below_window_objs_add(void)
{
   E_Move *m;
   E_Move_Border *mb;
   Eina_Bool mirror = EINA_TRUE;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (mb->animate_move)
          {
             if (!(mb->objs))
               {
                  L(LT_EVENT_OBJ,
                    "[MOVE] ev:%15.15s w:0x%08x %s() \n",
                    "EVAS_OBJ", mb->bd->win, __func__);
                  mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
                  e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
                  e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
                  e_mod_move_bd_move_objs_show(mb);
               }
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_below_window_objs_del(void)
{
   E_Move *m;
   E_Move_Border *mb;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (mb->animate_move)
          {
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s w:0x%08x %s() \n",
               "EVAS_OBJ", mb->bd->win, __func__);
             e_mod_move_bd_move_objs_del(mb, mb->objs);
             mb->objs = NULL;
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_below_window_objs_move(int x,
                                              int y)
{
   E_Move *m;
   E_Move_Border *mb, *qp_mb;
   E_Zone *zone;
   int angle;
   int cx, cy, cw, ch;
   int mx, my;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   qp_mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(qp_mb, EINA_FALSE);
   if (!e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
     return EINA_FALSE;
   angle = qp_mb->angle;
   zone = qp_mb->bd->zone;

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (mb->animate_move)
          {
             switch (angle)
               {
                case   0:
                  mx = mb->x;
                  my = y + ch + mb->y;
                  break;
                case  90:
                  mx = x + cw + mb->x;
                  my = mb->y;
                  break;
                case 180:
                  mx = mb->x;
                  my = y - ch + mb->y;
                  break;
                case 270:
                  mx = x - cw + mb->x;
                  my = mb->y;
                  break;
                default :
                  break;
               }
             L(LT_EVENT_OBJ,
               "[MOVE] ev:%15.15s w:0x%08x %s()  (%d,%d)\n",
               "EVAS_OBJ", mb->bd->win, __func__, mx, my);
             e_mod_move_bd_move_objs_move(mb, mx, my);
          }
     }
   return EINA_TRUE;
}

/* externally accessible functions */
EINTERN void
e_mod_move_quickpanel_ctl_obj_event_setup(E_Move_Border         *mb,
                                          E_Move_Control_Object *mco)
{
   E_CHECK(mb);
   E_CHECK(mco);
   E_CHECK(TYPE_QUICKPANEL_CHECK(mb));

   mco->event = e_mod_move_event_new(mb->bd->client.win, mco->obj);
   E_CHECK(mco->event);

   e_mod_move_event_angle_cb_set(mco->event,
                                 e_mod_move_util_win_prop_angle_get);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _e_mod_move_quickpanel_cb_motion_start, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _e_mod_move_quickpanel_cb_motion_move, mb);
   e_mod_move_event_cb_set(mco->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _e_mod_move_quickpanel_cb_motion_end, mb);
}

EINTERN E_Move_Border *
e_mod_move_quickpanel_base_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_QUICKPANEL_BASE_CHECK(mb)) return mb;
     }
   return NULL;
}

EINTERN E_Move_Border *
e_mod_move_quickpanel_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_QUICKPANEL_CHECK(mb)) return mb;
     }
   return NULL;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_click_get(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }

   return click;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_event_clear(void)
{
   E_Move_Border         *mb = NULL;
   Eina_Bool              click = EINA_FALSE;
   E_Move_Control_Object *mco = NULL;
   Eina_List             *l;

   mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   click = e_mod_move_quickpanel_click_get();
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
e_mod_move_quickpanel_objs_add(E_Move_Border *mb)
{
   E_Move *m = NULL;
   Eina_Bool mirror = EINA_TRUE;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;

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
e_mod_move_quickpanel_objs_del(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   m = mb->m;
   e_mod_move_bd_move_objs_del(mb, mb->objs);

   mb->objs = NULL;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_move(E_Move_Border *mb,
                                int            x,
                                int            y)
{
   int cx, cy, cw, ch;
   E_Move *m = NULL;
   E_Zone *zone = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   m = mb->m;
   zone = mb->bd->zone;

   e_mod_move_bd_move_objs_move(mb, x, y);
   _e_mod_move_quickpanel_objs_position_set(mb, x, y);
   _e_mod_move_quickpanel_dim_objs_apply(mb, x, y);

   // QP UX
   if (m->qp_scroll_with_visible_win)
     {
        if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
          {
             if (E_INTERSECTS(x+cx, y+cy, cw, ch, zone->x, zone->y, zone->w, zone->h))
                _e_mod_move_quickpanel_below_window_objs_move(x, y);
          }
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_animation_move(E_Move_Border *mb,
                                          int            x,
                                          int            y)
{
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   Ecore_Animator *animator = NULL;
   int sx, sy; //start x, start y
   double anim_time = 0.0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   anim_time = mb->m->animation_duration;

   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     {
        e_mod_move_quickpanel_objs_animation_stop(mb);
        e_mod_move_quickpanel_objs_animation_clear(mb);
     }

   anim_data = E_NEW(E_Move_Quickpanel_Animation_Data, 1);
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   if (_e_mod_move_quickpanel_objs_position_get(mb, &sx, &sy))
     {
        anim_data->sx = sx;
        anim_data->sy = sy;
     }
   else
     {
        anim_data->sx = mb->x;
        anim_data->sy = mb->y;
        _e_mod_move_quickpanel_objs_position_set(mb,
                                                 anim_data->sx,
                                                 anim_data->sy);
     }

   anim_data->ex = x;
   anim_data->ey = y;
   anim_data->dx = anim_data->ex - anim_data->sx;
   anim_data->dy = anim_data->ey - anim_data->sy;

   animator = ecore_animator_timeline_add(anim_time,
                                          _e_mod_move_quickpanel_objs_animation_frame,
                                          anim_data);
   if (!animator)
     {
        memset(anim_data, 0, sizeof(E_Move_Quickpanel_Animation_Data));
        E_FREE(anim_data);
        return EINA_FALSE;
     }

   anim_data->animator = animator;
   anim_data->animating = EINA_TRUE;
   mb->anim_data = anim_data;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_animation_state_get(E_Move_Border *mb)
{
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Quickpanel_Animation_Data *)mb->anim_data;
   E_CHECK_RETURN(anim_data->animating, EINA_FALSE);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_animation_stop(E_Move_Border *mb)
{
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Quickpanel_Animation_Data *)mb->anim_data;
   ecore_animator_freeze(anim_data->animator);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_animation_clear(E_Move_Border *mb)
{
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   E_CHECK_RETURN(mb->anim_data, EINA_FALSE);
   anim_data = (E_Move_Quickpanel_Animation_Data *)mb->anim_data;
   ecore_animator_del(anim_data->animator);
   memset(anim_data, 0, sizeof(E_Move_Quickpanel_Animation_Data));
   E_FREE(anim_data);
   mb->anim_data = NULL;

   return EINA_TRUE;
}

EINTERN void*
e_mod_move_quickpanel_internal_data_add(E_Move_Border *mb)
{
   int dim_min;
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), NULL);
   E_CHECK_RETURN(mb->m, NULL);
   dim_min = mb->m->dim_min_opacity;
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   if (!qp_data)
     {
        qp_data = E_NEW(E_Move_Quickpanel_Data, 1);
        E_CHECK_RETURN(qp_data, NULL);
        qp_data->x = mb->x;
        qp_data->y = mb->y;
        qp_data->dim_objs = NULL;
        qp_data->opacity = dim_min;
        mb->data = qp_data;
     }
   return mb->data;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_internal_data_del(E_Move_Border *mb)
{
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   e_mod_move_quickpanel_dim_hide(mb);
   E_FREE(qp_data);
   mb->data = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_e_border_move(E_Move_Border *mb,
                                    int            x,
                                    int            y)
{
   E_Move        *m = NULL;
   E_Border      *bd = NULL;
   E_Zone        *zone = NULL;
   E_Move_Border *find_mb = NULL;
   Ecore_X_Window win;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);
   m = mb->m;
   bd = mb->bd;
   zone = bd->zone;

   if (E_CONTAINS(zone->x, zone->y, zone->w, zone->h, x, y, mb->w, mb->h))// On Screen Case, if zone contains border geometry?
     {
        win = e_mod_move_util_client_xid_get(mb);

        EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
          {
             if (find_mb->visible)
               {
                  if (find_mb->bd)
                     ecore_x_e_illume_quickpanel_state_send(find_mb->bd->client.win,
                                                            ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
               }
          }
     }

   e_border_move(bd, x, y);

   return EINA_TRUE;
}

EINTERN Eina_List*
e_mod_move_quickpanel_dim_show(E_Move_Border *mb)
{
   int dim_min;
   E_Move *m = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), NULL);
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   m = mb->m;
   dim_min = m->dim_min_opacity;
   if (!qp_data)
     {
        qp_data = E_NEW(E_Move_Quickpanel_Data, 1);
        E_CHECK_RETURN(qp_data, NULL);
        // Composite mode set true
        e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_TRUE);

        qp_data->x = mb->x;
        qp_data->y = mb->y;
        qp_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
        e_mod_move_bd_move_dim_objs_show(qp_data->dim_objs);
        qp_data->opacity = dim_min;
        mb->data = qp_data;
        //qp ux
        if (m->qp_scroll_with_visible_win)
          {
             _e_mod_move_quickpanel_below_window_set();
             _e_mod_move_quickpanel_below_window_objs_add();
          }
        e_mod_move_util_rotation_lock(mb->m);
     }
   else
     {
        if (!(qp_data->dim_objs))
          {
             // Composite mode set true
             e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_TRUE);

             qp_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
             if (qp_data->dim_objs)
               {
                  //qp ux
                  if (m->qp_scroll_with_visible_win)
                    {
                       _e_mod_move_quickpanel_below_window_set();
                       _e_mod_move_quickpanel_below_window_objs_add();
                    }
                  e_mod_move_util_rotation_lock(mb->m);
               }
          }
        if (qp_data->dim_objs)
          {
             e_mod_move_bd_move_dim_objs_show(qp_data->dim_objs);
             qp_data->opacity = dim_min;
          }
     }

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   return qp_data->dim_objs;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_dim_hide(E_Move_Border *mb)
{
   int dim_min;
   E_Move *m = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   dim_min =  mb->m->dim_min_opacity;
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data->dim_objs, EINA_FALSE);
   e_mod_move_bd_move_dim_objs_hide(qp_data->dim_objs);
   e_mod_move_bd_move_dim_objs_del(qp_data->dim_objs);
   qp_data->dim_objs = NULL;
   qp_data->opacity = dim_min;
   m = mb->m;
   //qp ux
   if (m->qp_scroll_with_visible_win)
     {
        _e_mod_move_quickpanel_below_window_objs_del();
        _e_mod_move_quickpanel_below_window_unset();
     }

   e_mod_move_util_rotation_unlock(mb->m);
   // Composite mode set false
   e_mod_move_util_compositor_composite_mode_set(mb->m, EINA_FALSE);

   L(LT_EVENT_OBJ,
     "[MOVE] ev:%15.15s w:0x%08x %s()\n",
     "EVAS_OBJ", mb->bd->win, __func__);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_animation_start_position_set(E_Move_Border *mb,
                                                       int             angle)
{
   int x = 0; int y = 0;
   int cx = 0; int cy = 0; int cw = 0; int ch = 0;
   E_Zone *zone = NULL;
   Eina_Bool contents;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   zone = mb->bd->zone;
   angle = ((angle % 360) / 90) * 90;
   contents = e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch);

   switch (angle)
     {
      case  90:
         if (contents)
           x = cw * -1;
         else
           x = mb->w * -1;
         break;
      case 180:
         if (contents)
           y = zone->y + zone->h - cy;
         else
           y = zone->y + zone->h;
         break;
      case 270:
         if (contents)
           x = zone->x + zone->w - cx;
         else
           x = zone->x + zone->w;
         break;
      case   0:
      default :
         if (contents)
           y = ch * -1;
         else
           y = mb->h * -1;
         break;
     }

   _e_mod_move_quickpanel_objs_position_set(mb, x, y);

   return EINA_TRUE;
}

EINTERN E_Move_Event_Cb
e_mod_move_quickpanel_event_cb_get(E_Move_Event_Type type)
{
   if (type == E_MOVE_EVENT_TYPE_MOTION_START)
     return _e_mod_move_quickpanel_cb_motion_start;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_MOVE)
     return _e_mod_move_quickpanel_cb_motion_move;
   else if (type == E_MOVE_EVENT_TYPE_MOTION_END)
     return _e_mod_move_quickpanel_cb_motion_end;
   else
     return NULL;
}
