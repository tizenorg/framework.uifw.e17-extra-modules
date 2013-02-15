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
static Eina_Bool          _e_mod_move_quickpanel_objs_moving_distance_rate_get(E_Move_Border *mb, int angle, double *rate);
static Eina_Bool          _e_mod_move_quickpanel_objs_animation_frame(void  *data, double pos);
static Eina_Bool          _e_mod_move_quickpanel_flick_process(E_Move_Border *mb, int angle, Eina_Bool state);
static Eina_Bool          _e_mod_move_quickpanel_dim_objs_apply(E_Move_Border *mb, int x, int y);
static Eina_Bool          _e_mod_move_quickpanel_below_window_set(void);
static void               _e_mod_move_quickpanel_below_window_unset(void);
static Eina_Bool          _e_mod_move_quickpanel_below_window_objs_add(void);
static Eina_Bool          _e_mod_move_quickpanel_below_window_objs_del(void);
static Eina_Bool          _e_mod_move_quickpanel_below_window_objs_move(int x, int y);
static Eina_Bool          _e_mod_move_quickpanel_handle_objs_add(E_Move_Border *mb);
static Eina_Bool          _e_mod_move_quickpanel_handle_objs_del(E_Move_Border *mb);
static Eina_Bool          _e_mod_move_quickpanel_handle_objs_size_update(E_Move_Border *mb, int w, int h);
static Eina_Bool          _e_mod_move_quickpanel_handle_objs_move(E_Move_Border *mb, int x, int y);
static Eina_Bool          _e_mod_move_quickpanel_objs_check_on_screen(E_Move_Border *mb, int x, int y);
static Eina_Bool          _e_mod_move_quickpanel_objs_outside_movable_pos_get(E_Move_Border *mb, int *x, int *y);
static Eina_Bool          _e_mod_move_quickpanel_animation_change_with_angle(E_Move_Border *mb);
static Eina_Bool          _e_mod_move_quickpanel_fb_move_change_with_angle(E_Move_Border *mb);

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
   E_Move *m = NULL;
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

   m = mb->m;
   angle = mb->angle;
   zone = mb->bd->zone;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        click = e_mod_move_event_click_get(mco->event);
     }
   E_CHECK_RETURN(click, EINA_FALSE);

   if (m->qp_scroll_with_clipping)
     e_mod_move_quickpanel_objs_move(mb, info->coord.x, info->coord.y);
   else
     {
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
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_cb_motion_end(void *data,
                                     void *event_info)
{
   E_Move *m = NULL;
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

   m = mb->m;
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
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_e_border_move(mb, 0, mb->h * -1);
                   e_mod_move_quickpanel_objs_animation_move(mb, zone->x, zone->y);
                }
              else
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
           }
         else
           {
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_objs_animation_move(mb,
                                                             zone->x + mb->w,
                                                             zone->y + mb->h);
                }
              else
                {
                   e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                }
           }
         break;
      case  90:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x < check_w)
           {
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_e_border_move(mb, mb->w * -1, 0);
                   e_mod_move_quickpanel_objs_animation_move(mb, zone->x, zone->y);
                }
              else
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
           }
         else
           {
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_objs_animation_move(mb,
                                                             zone->x + mb->w,
                                                             zone->y + mb->h);
                }
              else
                {
                   e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                }
           }
         break;
      case 180:
         check_h = mb->h;
         if (check_h) check_h /= 2;
         if (info->coord.y > (zone->h - check_h))
           {
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_e_border_move(mb, 0, zone->h);
                   e_mod_move_quickpanel_objs_animation_move(mb,
                                                             zone->x + mb->w,
                                                             zone->y + mb->h);
                }
              else
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
           }
         else
           {
              e_mod_move_quickpanel_e_border_move(mb, 0, 0);
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_objs_animation_move(mb,
                                                             zone->x,
                                                             zone->y);
                }
              else
                {
                   e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
                }
           }
         break;
      case 270:
         check_w = mb->w;
         if (check_w) check_w /= 2;
         if (info->coord.x > (zone->w - check_w))
           {
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_e_border_move(mb, zone->w, 0);
                   e_mod_move_quickpanel_objs_animation_move(mb,
                                                             zone->x + mb->w,
                                                             zone->y + mb->h);
                }
              else
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
           }
         else
           {
              e_mod_move_quickpanel_e_border_move(mb, 0, 0);
              if (m->qp_scroll_with_clipping)
                {
                   e_mod_move_quickpanel_objs_animation_move(mb, zone->x, zone->y);
                }
              else
                {
                   e_mod_move_quickpanel_objs_animation_move(mb, 0, 0);
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
_e_mod_move_quickpanel_objs_moving_distance_rate_get(E_Move_Border *mb,
                                                     int            angle,
                                                     double        *rate)
{
   E_Move *m = NULL;
   E_Border *bd = NULL;
   E_Zone *zone = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   int x = 0, y = 0;
   int cx = 0, cy = 0, cw = 0, ch = 0;
   double dist_rate = 0.0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(rate, EINA_FALSE);
   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data, EINA_FALSE);
   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   if (!_e_mod_move_quickpanel_objs_position_get(mb, &x, &y))
     return EINA_FALSE;

   switch (angle)
     {
      case   0:
         if (m->qp_scroll_with_clipping)
           dist_rate = (double)y / (zone->y + zone->h);
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                dist_rate = (double)(y + ch) / (zone->y + zone->h);
              else
                dist_rate = (double)(y + mb->h) / (zone->y + zone->h);
           }
         break;
      case  90:
         if (m->qp_scroll_with_clipping)
           dist_rate = (double)x / (zone->x + zone->w);
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                dist_rate = (double)(x + cw) / (zone->x + zone->w);
              else
                dist_rate = (double)(x + mb->w) / (zone->x + zone->w);
           }
         break;
      case 180:
         if (m->qp_scroll_with_clipping)
           dist_rate = 1.0 - ((double)y / (zone->y + zone->h));
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                dist_rate = 1.0 - ((double)(y + cy) / (zone->y + zone->h));
              else
                dist_rate = 1.0 - ((double)y / (zone->y + zone->h));
           }
         break;
      case 270:
         if (m->qp_scroll_with_clipping)
           dist_rate = 1.0 - ((double)x / (zone->x + zone->w));
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                dist_rate = 1.0 - ((double)(x + cx) / (zone->x + zone->w));
              else
                dist_rate = 1.0 - ((double)x / (zone->x + zone->w));
           }
         break;
      default :
         return EINA_FALSE;
         break;
     }

   *rate = dist_rate;

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
   Eina_Bool                         mv_ret = EINA_FALSE;

   anim_data = (E_Move_Quickpanel_Animation_Data *)data;
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(mb, EINA_FALSE);
   m = mb->m;

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;

   // When Animation is ended, if new window is added or window stack changed, then apply new info
   if (pos >= 1.0
       && m->qp_scroll_with_clipping)
     e_mod_move_quickpanel_below_window_reset();

   e_mod_move_quickpanel_objs_move(mb, x, y);

   if (pos >= 1.0)
     {
         bd = mb->bd;
         zone = bd->zone;
         mv_ret = e_mod_move_quickpanel_objs_move(mb, anim_data->ex, anim_data->ey);

         if (!mv_ret) // objs move fail case
           {
              // while quickpanel is animating, if quickpanel angle changed, qp_objs move could fail. so qp_objs move again with changed angle.
              if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                                zone->x, zone->y, zone->w, zone->h))
                {
                   if (_e_mod_move_quickpanel_objs_outside_movable_pos_get(mb,
                                                                           &x,
                                                                           &y))
                     e_mod_move_quickpanel_objs_move(mb, x, y);
                }
           }

         e_mod_move_quickpanel_objs_del(mb);
         if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                           zone->x, zone->y, zone->w, zone->h)) // it work when quickpanel is not equal to zone size
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
                   else
                     {
                        if (find_mb->bd && find_mb->bd->iconic)
                           ecore_x_e_illume_quickpanel_state_send(find_mb->bd->client.win,
                                                                  ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
                     }
                }

              if (!m->qp_scroll_with_clipping)
                {
                   // if dim use case, hold below windows until dim  is on screen
                   // dim hide calls deletion of below window mirror object
                   e_mod_move_quickpanel_dim_hide(mb);
                }
              // Set No Composite Mode & Rotation UnLock & Destroy below win's mirror object
              e_mod_move_quickpanel_stage_deinit(mb);
           }

         // if scroll with clipping use case, hold below windows until only animation is working
         if (m->qp_scroll_with_visible_win &&
             m->qp_scroll_with_clipping)
           {
              _e_mod_move_quickpanel_below_window_objs_del();
              _e_mod_move_quickpanel_below_window_unset();
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
   E_Move *m = NULL;
   E_Move_Control_Object *mco = NULL;
   Eina_List *l;
   int mx = 0, my = 0, ax = 0, ay = 0;
   E_Zone *zone = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(state, EINA_FALSE);

   m = mb->m;
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
         mx = mb->w * -1; my = 0;
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
              ax = zone->x + mb->w;
              ay = zone->y + mb->h;
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
              ax = zone->x + mb->w;
              ay = zone->y + mb->h;
           }
         else
           {
              ax = mx; ay = my;
           }
         break;
      case   0:
      default :
         mx = 0; my = mb->h * -1;
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

   e_mod_move_quickpanel_e_border_move(mb, mx, my);
   e_mod_move_quickpanel_objs_animation_move(mb, ax, ay);

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
   int cx = 0, cy = 0, cw = 0, ch = 0;
   int mx = 0, my = 0;
   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   qp_mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(qp_mb, EINA_FALSE);

   if (!m->qp_scroll_with_clipping
        && !e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
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
                  if (m->qp_scroll_with_clipping)
                    my = y + mb->y;
                  else
                    my = y + ch + mb->y;
                  break;
                case  90:
                  if (m->qp_scroll_with_clipping)
                    mx = x + mb->x;
                  else
                    mx = x + cw + mb->x;
                  my = mb->y;
                  break;
                case 180:
                  mx = mb->x;
                  if (m->qp_scroll_with_clipping)
                    my = mb->y - (zone->h - y);
                  else
                    my = y - ch + mb->y;
                  break;
                case 270:
                  if (m->qp_scroll_with_clipping)
                    mx = mb->x - (zone->w - x);
                  else
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

   // if qp_scroll_with_clipping case, make cw / ch data for e_mod_move_util_fb_move()
   if (m->qp_scroll_with_clipping)
     {
        switch (angle)
          {
           case 180:
              ch = zone->h;
              break;
           case 270:
              cw = zone->w;
              break;
           default:
              break;
          }
     }

   e_mod_move_util_fb_move(angle, cw, ch, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_handle_objs_add(E_Move_Border *mb)
{
   E_Move *m = NULL;
   int hx = 0, hy = 0, hw = 0, hh = 0;
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data, EINA_FALSE);

   if (!e_mod_move_border_shape_input_rect_get(mb, &hx, &hy, &hw, &hh))
     return EINA_FALSE;

   m = mb->m;
   qp_data->handle_objs = e_mod_move_evas_objs_add(m, mb->bd);

   E_CHECK_RETURN(qp_data->handle_objs, EINA_FALSE);

   e_mod_move_evas_objs_raise(qp_data->handle_objs);
   e_mod_move_evas_objs_move(qp_data->handle_objs, mb->x, mb->y);
   e_mod_move_evas_objs_resize(qp_data->handle_objs, mb->w, mb->h);
   e_mod_move_evas_objs_show(qp_data->handle_objs);

   e_mod_move_evas_objs_clipper_add(qp_data->handle_objs);
   e_mod_move_evas_objs_clipper_move(qp_data->handle_objs, mb->x + hx, mb->y + hy);
   e_mod_move_evas_objs_clipper_resize(qp_data->handle_objs, hw, hh);
   e_mod_move_evas_objs_clipper_show(qp_data->handle_objs);

   e_mod_move_evas_objs_del_cb_set(&(qp_data->handle_objs));

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_handle_objs_del(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data, EINA_FALSE);

   m = mb->m;
   E_CHECK_RETURN(qp_data->handle_objs,EINA_FALSE);

   e_mod_move_evas_objs_clipper_hide(qp_data->handle_objs);
   e_mod_move_evas_objs_clipper_del(qp_data->handle_objs);
   e_mod_move_evas_objs_hide(qp_data->handle_objs);
   e_mod_move_evas_objs_del(qp_data->handle_objs);

   qp_data->handle_objs = NULL;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_handle_objs_size_update(E_Move_Border *mb,
                                               int            w,
                                               int            h)
{
   E_Move_Quickpanel_Data *qp_data = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data, EINA_FALSE);

   E_CHECK_RETURN(qp_data->handle_objs,EINA_FALSE);

   e_mod_move_evas_objs_clipper_resize(qp_data->handle_objs, w, h);

   return EINA_TRUE;
}

static Eina_Bool _e_mod_move_quickpanel_handle_objs_move(E_Move_Border *mb,
                                                         int            x,
                                                         int            y)
{
   int hx = 0, hy = 0, hw = 0, hh = 0; // handle's x, y, w, h
   E_Zone *zone = NULL;
   E_Move *m = NULL;
   E_Move_Quickpanel_Data *qp_data = NULL;
   int angle =0;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   qp_data = (E_Move_Quickpanel_Data *)mb->data;
   E_CHECK_RETURN(qp_data, EINA_FALSE);

   m = mb->m;
   E_CHECK_RETURN(qp_data->handle_objs,EINA_FALSE);

   if (!e_mod_move_border_shape_input_rect_get(mb, &hx, &hy, &hw, &hh))
     return EINA_FALSE;

   angle = mb->angle;
   zone = mb->bd->zone;
   switch (angle)
     {
      case 90:
         e_mod_move_evas_objs_move(qp_data->handle_objs, x - mb->w + hw, zone->y);
         e_mod_move_evas_objs_clipper_move(qp_data->handle_objs, x, zone->y); 
         break;
      case 180:
         e_mod_move_evas_objs_move(qp_data->handle_objs, zone->x, y - hh);
         e_mod_move_evas_objs_clipper_move(qp_data->handle_objs, zone->x, y - hh);
         // y - hh 's -hh means overlaping app's indicator region
         break;
      case 270:
         e_mod_move_evas_objs_move(qp_data->handle_objs, x - hw, zone->y);
         e_mod_move_evas_objs_clipper_move(qp_data->handle_objs, x - hw, zone->y);
         // x - hw 's -hw means overlaping app's indicator region
         break;
      case 0:
      default:
         e_mod_move_evas_objs_move(qp_data->handle_objs, zone->x, y - mb->h + hh);
         e_mod_move_evas_objs_clipper_move(qp_data->handle_objs, zone->x, y); 
         break;
     }

   return EINA_TRUE;
}

// this function check moving objs go over screen or not
static Eina_Bool
_e_mod_move_quickpanel_objs_check_on_screen(E_Move_Border *mb,
                                            int            x,
                                            int            y)
{
   E_Zone *zone = NULL;
   Eina_Bool ret = EINA_FALSE;
   int hx = 0, hy = 0, hw = 0, hh = 0; // handle's x, y, w, h

   E_CHECK_RETURN(mb, FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), FALSE);

   if (!e_mod_move_border_shape_input_rect_get(mb, &hx, &hy, &hw, &hh))
     return EINA_FALSE;

   zone = mb->bd->zone;
   switch (mb->angle)
     {
       // don't go over screen
      case 90:
         if (x + hw <= zone->x + zone->w) ret = EINA_TRUE;
         break;
      case 180:
         if (y - hh >= zone->y) ret = EINA_TRUE;
         break;
      case 270:
         if (x - hw >= zone->x) ret = EINA_TRUE;
         break;
      case 0:
      default:
         if (y + hh <= zone->y + zone->h) ret = EINA_TRUE;
         break;
     }
   return ret;
}

static Eina_Bool
_e_mod_move_quickpanel_animation_change_with_angle(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_Border *bd = NULL;
   int ax = 0, ay = 0; // animation x, animation y
   int px = 0, py = 0; // position x, position y
   int cx = 0, cy = 0, cw = 0, ch = 0;
   int curr_angle = 0, prev_angle = 0;
   double dist_rate = 0.0;
   E_Zone *zone = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);
   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   if (!e_mod_move_util_win_prop_angle_get(bd->client.win,
                                           &curr_angle,
                                           &prev_angle))
     return EINA_FALSE;

   if (!e_mod_move_quickpanel_objs_animation_state_get(mb))
     return EINA_FALSE;

   if (!_e_mod_move_quickpanel_objs_moving_distance_rate_get(mb, prev_angle, &dist_rate))
     return EINA_FALSE;

   // new_position set with distance rate
   switch (curr_angle)
     {
      case  90:
         if (m->qp_scroll_with_clipping)
           px = (dist_rate * (zone->x + zone->w));
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                px = (dist_rate * (zone->x + zone->w)) - cw;
              else
                px = (dist_rate * (zone->x + zone->w)) - mb->w;
           }
         break;
      case 180:
         if (m->qp_scroll_with_clipping)
           py = (1.0 - dist_rate) * (zone->y + zone->h);
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                py = (1.0 - dist_rate) * (zone->y + zone->h) - cy;
              else
                py = (1.0 - dist_rate) * (zone->y + zone->h);
           }
         break;
      case 270:
         if (m->qp_scroll_with_clipping)
           px = (1.0 - dist_rate) * (zone->x + zone->w);
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                px = (1.0 - dist_rate) * (zone->x + zone->w) - ch;
              else
                px = (1.0 - dist_rate) * (zone->x + zone->w);
           }
         break;
      case   0:
      default :
         if (m->qp_scroll_with_clipping)
           py = (dist_rate * (zone->y + zone->h));
         else
           {
              if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
                py = (dist_rate * (zone->y + zone->h)) - ch;
              else
                py = (dist_rate * (zone->y + zone->h)) - mb->h;
           }
         break;
     }
   _e_mod_move_quickpanel_objs_position_set(mb, px, py);

   // set info for new animation
   switch(e_mod_move_quickpanel_objs_animation_direction_get(mb))
     {
        case E_MOVE_ANIMATION_DIRECTION_INSIDE:
              switch (curr_angle)
                {
                 case 180:
                 case 270:
                    if (m->qp_scroll_with_clipping)
                      {
                        ax = zone->x;
                        ay = zone->y;
                      }
                    break;
                 case   0:
                 case  90:
                 default :
                    if (m->qp_scroll_with_clipping)
                      {
                        ax = zone->x + mb->w;
                        ay = zone->y + mb->h;
                     }
                    break;
                }
           break;
        case E_MOVE_ANIMATION_DIRECTION_OUTSIDE:
           {
              switch (curr_angle)
                {
                 case  90:
                    if (m->qp_scroll_with_clipping)
                      {
                         ax = zone->x; ay = zone->y;
                      }
                    else
                      {
                         ax = mb->w * -1;
                      }
                    break;
                 case 180:
                    if (m->qp_scroll_with_clipping)
                      {
                         ax = zone->x + mb->w;
                         ay = zone->y + mb->h;
                      }
                    else
                      {
                         ay = zone->h;
                      }
                    break;
                 case 270:
                    if (m->qp_scroll_with_clipping)
                      {
                         ax = zone->x + mb->w;
                         ay = zone->y + mb->h;
                      }
                    else
                      {
                         ax = zone->w;
                      }
                    break;
                 case   0:
                 default :
                    if (m->qp_scroll_with_clipping)
                      {
                         ax = zone->x; ay = zone->y;
                      }
                    else
                      {
                         ay = mb->h * -1;
                      }
                    break;
                }
           }
           break;
        case E_MOVE_ANIMATION_DIRECTION_NONE:
        default:
           return EINA_FALSE;
     }

   e_mod_move_quickpanel_objs_animation_stop(mb);
   e_mod_move_quickpanel_objs_animation_clear(mb);
   e_mod_move_quickpanel_objs_add(mb);

   //prevent handler object flickering, if input region update, then show handler again.
   _e_mod_move_quickpanel_handle_objs_size_update(mb, 0, 0);

   e_mod_move_quickpanel_objs_animation_move(mb, ax, ay);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_fb_move_change_with_angle(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_Border *bd = NULL;
   int ax = 0, ay = 0;
   int cx = 0, cy = 0, cw = 0, ch = 0;
   int curr_angle = 0, prev_angle = 0;
   E_Zone *zone = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);
   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   if (!e_mod_move_util_win_prop_angle_get(bd->client.win,
                                           &curr_angle,
                                           &prev_angle))
     return EINA_FALSE;

   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     return EINA_FALSE;

   if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                     zone->x, zone->y, zone->w, zone->h)) // it work when quickpanel exists inside zone
     return EINA_FALSE;

   // if none-clipping scroll and none-content-rect then return, because there are not scroll info
   if (!m->qp_scroll_with_clipping
        && !e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
     return EINA_FALSE;

   switch (curr_angle)
     {
      case  90:
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x + mb->w;
              ay = zone->y + mb->h;
           }
         break;
      case 180:
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x; ay = zone->y;
           }
         else
           {
              ay = zone->h - mb->h;
           }
         break;
      case 270:
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x; ay = zone->y;
           }
         else
           {
              ax = zone->w - mb->w;
           }
         break;
      case   0:
      default :
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x + mb->w;
              ay = zone->y + mb->h;
           }
         break;
     }

   if (m->qp_scroll_with_clipping)
     {
        switch (curr_angle)
          {
           case 180:
              ch = zone->h;
              break;
           case 270:
              cw = zone->w;
              break;
           default:
              break;
          }
     }

   e_mod_move_util_fb_move(curr_angle, cw, ch, ax, ay);

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

   e_mod_move_event_data_type_set(mco->event, E_MOVE_EVENT_DATA_TYPE_BORDER);
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

        if (m->qp_scroll_with_clipping)
          {
             e_mod_move_quickpanel_objs_clipper_add(mb);
             _e_mod_move_quickpanel_handle_objs_add(mb);

             // make below window's mirror object for animation
             if (m->qp_scroll_with_visible_win)
               {
                  _e_mod_move_quickpanel_below_window_set();
                  _e_mod_move_quickpanel_below_window_objs_add();
               }
          }
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

   if (m->qp_scroll_with_clipping)
     {
        e_mod_move_quickpanel_objs_clipper_del(mb);
        _e_mod_move_quickpanel_handle_objs_del(mb);
     }

   e_mod_move_bd_move_objs_del(mb, mb->objs);

   mb->objs = NULL;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_move(E_Move_Border *mb,
                                int            x,
                                int            y)
{
   E_Move *m = NULL;
   E_Zone *zone = NULL;
   int cx = 0, cy = 0, cw = 0, ch = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;
   zone = mb->bd->zone;

   _e_mod_move_quickpanel_objs_position_set(mb, x, y);

   // QP UX
   if (m->qp_scroll_with_clipping)
     {
        e_mod_move_bd_move_objs_move(mb, zone->x, zone->y); // in general, zone->x:0, zone->y: 0
        e_mod_move_quickpanel_objs_clipper_apply(mb, x, y); // move clipper

        // check handle & below window don't go over screen
        if (!_e_mod_move_quickpanel_objs_check_on_screen(mb, x , y))
          return EINA_FALSE;

        _e_mod_move_quickpanel_handle_objs_move(mb, x, y);; // move handle (mirror with clipper) object

        if (m->qp_scroll_with_visible_win)
          {
             if (POINT_INSIDE_ZONE(x, y, zone))
               _e_mod_move_quickpanel_below_window_objs_move(x, y);
          }
     }
   else
     {
        e_mod_move_bd_move_objs_move(mb, x, y);
        _e_mod_move_quickpanel_dim_objs_apply(mb, x, y);

        if (m->qp_scroll_with_visible_win)
          {
             if (e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch))
               {
                  if (E_INTERSECTS(x+cx, y+cy, cw, ch, zone->x, zone->y, zone->w, zone->h))
                     _e_mod_move_quickpanel_below_window_objs_move(x, y);
               }
          }
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_animation_move_with_time(E_Move_Border *mb,
                                                    int            x,
                                                    int            y,
                                                    double         anim_time)
{
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   Ecore_Animator *animator = NULL;
   int sx, sy; //start x, start y
   int angle = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);

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
        // below cases are quickpanel is on screen case, so set start position with quickpanel's screen position
        if (mb->m->qp_scroll_with_clipping)
          {
             angle = mb->angle;
             switch (angle)
               {
                 case 0:
                 case 90:
                 default:
                     anim_data->sx = mb->x + mb->w;
                     anim_data->sy = mb->y + mb->h;
                     break;
                 case 180:
                 case 270:
                     anim_data->sx = mb->x;
                     anim_data->sy = mb->y;
                     break;
               }
          }
        else
          {
             anim_data->sx = mb->x;
             anim_data->sy = mb->y;
          }
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
e_mod_move_quickpanel_objs_animation_move(E_Move_Border *mb,
                                          int            x,
                                          int            y)
{
   double anim_time = 0.0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   anim_time = mb->m->animation_duration;
   return e_mod_move_quickpanel_objs_animation_move_with_time(mb, x, y, anim_time);
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

EINTERN E_Move_Animation_Direction
e_mod_move_quickpanel_objs_animation_direction_get(E_Move_Border *mb)
{
   E_Border *bd = NULL;
   E_Zone *zone = NULL;
   E_Move_Quickpanel_Animation_Data *anim_data = NULL;
   E_CHECK_RETURN(mb, E_MOVE_ANIMATION_DIRECTION_NONE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), E_MOVE_ANIMATION_DIRECTION_NONE);
   bd= mb->bd;
   E_CHECK_RETURN(bd, E_MOVE_ANIMATION_DIRECTION_NONE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, E_MOVE_ANIMATION_DIRECTION_NONE);
   E_CHECK_RETURN(mb->anim_data, E_MOVE_ANIMATION_DIRECTION_NONE);
   anim_data = (E_Move_Quickpanel_Animation_Data *)mb->anim_data;

   if (E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                    zone->x, zone->y, zone->w, zone->h))
     {
        // animation's end position is inside zone.
        return E_MOVE_ANIMATION_DIRECTION_INSIDE;
     }
   else
     {
        // animation's end position is outside zone.
        return E_MOVE_ANIMATION_DIRECTION_OUTSIDE;
     }
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

   if (mb->m->qp_scroll_with_clipping) // "scroll with clipping" uses handle objs so delete here.
     {
        _e_mod_move_quickpanel_handle_objs_del(mb);
     }
   else // general quickpanel scroll uses dim objs so delete here.
     {
        e_mod_move_quickpanel_dim_hide(mb);
     }

   // Set No Composite Mode & Rotation UnLock & Destroy below win's mirror object
   e_mod_move_quickpanel_stage_deinit(mb);

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
             else
               {
                  if (find_mb->bd && find_mb->bd->iconic)
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

        qp_data->x = mb->x;
        qp_data->y = mb->y;
        qp_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);
        e_mod_move_bd_move_dim_objs_show(qp_data->dim_objs);
        qp_data->opacity = dim_min;
        mb->data = qp_data;
     }
   else
     {
        if (!(qp_data->dim_objs))
          qp_data->dim_objs = e_mod_move_bd_move_dim_objs_add(mb);

        if (qp_data->dim_objs)
          {
             e_mod_move_bd_move_dim_objs_show(qp_data->dim_objs);
             qp_data->opacity = dim_min;
          }
     }

   //qp ux
   if (m->qp_scroll_with_visible_win)
     {
        _e_mod_move_quickpanel_below_window_set();
        _e_mod_move_quickpanel_below_window_objs_add();
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
   E_Move *m = NULL;
   Eina_Bool contents;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;
   zone = mb->bd->zone;
   angle = ((angle % 360) / 90) * 90;
   contents = e_mod_move_border_contents_rect_get(mb, &cx, &cy ,&cw, &ch);

   if (m->qp_scroll_with_clipping)
     {
       switch (angle)
         {
          case 0:
          case 90:
          default:
             x = mb->x; y = mb->y;
             break;
          case 180:
          case 270:
             x = mb->x + mb->w; y = mb->y + mb->h;
             break;
          }
     }
   else
     {
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

EINTERN Eina_Bool
e_mod_move_quickpanel_visible_check(void)
{
   E_Move_Border *mb;
   E_Zone        *zone;
   mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->visible, EINA_FALSE);

   zone = mb->bd->zone;
   E_CHECK_RETURN(REGION_INSIDE_ZONE(mb, zone), EINA_FALSE);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_below_window_reset(void)
{
   E_Move *m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   if (m->qp_scroll_with_visible_win)
     {
        // remove below window objs & unset
        _e_mod_move_quickpanel_below_window_objs_del();
        _e_mod_move_quickpanel_below_window_unset();
        // add below window set & objs
        _e_mod_move_quickpanel_below_window_set();
        _e_mod_move_quickpanel_below_window_objs_add();
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_clipper_add(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   e_mod_move_bd_move_objs_clipper_add(mb);
   e_mod_move_bd_move_objs_clipper_move(mb, mb->x, mb->y);
   e_mod_move_bd_move_objs_clipper_resize(mb, mb->w, mb->h);
   e_mod_move_bd_move_objs_clipper_show(mb);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_clipper_del(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   e_mod_move_bd_move_objs_clipper_hide(mb);
   e_mod_move_bd_move_objs_clipper_del(mb);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_objs_clipper_apply(E_Move_Border *mb, int x, int y)
{
   int cx = 0, cy = 0, cw = 0, ch = 0; // clip_x, clip_y, clip_w, clip_h
   int angle = 0;
   E_Zone *zone = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   zone = mb->bd->zone;
   angle = mb->angle;

   switch (angle)
     {
        case 90:
           cw = x;
           ch = mb->h;
           break;

        case 180:
           cy = y;
           cw = mb->w;
           ch = zone->h - y;
           break;

        case 270:
           cx = x;
           cw = zone->w - x;
           ch = mb->h;
           break;

        case 0:
        default:
           cw = mb->w;
           ch = y;
           break;
     }
   e_mod_move_bd_move_objs_clipper_move(mb, cx, cy);
   e_mod_move_bd_move_objs_clipper_resize(mb, cw, ch);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_stage_init(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;

   // Composite mode set true
   e_mod_move_util_compositor_composite_mode_set(m, EINA_TRUE);
   e_mod_move_util_rotation_lock(m);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_stage_deinit(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;

   //qp ux
   if (m->qp_scroll_with_visible_win)
     {
        _e_mod_move_quickpanel_below_window_objs_del();
        _e_mod_move_quickpanel_below_window_unset();
     }

   e_mod_move_util_rotation_unlock(m);

   // Composite mode set false
   e_mod_move_util_compositor_composite_mode_set(m, EINA_FALSE);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_angle_change_post_job(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     return _e_mod_move_quickpanel_animation_change_with_angle(mb);
   else
     return _e_mod_move_quickpanel_fb_move_change_with_angle(mb);
}

static Eina_Bool
_e_mod_move_quickpanel_objs_outside_movable_pos_get(E_Move_Border *mb,
                                                    int           *x,
                                                    int           *y)
// while quickpanel is animating,
// if quickpanel angle changed, qp_objs move could fail. so qp_objs move again with changed angle.
{
   E_Move *m = NULL;
   int ax = 0, ay = 0;
   int angle = 0;
   E_Zone *zone = NULL;
   E_Border *bd = NULL;
   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);
   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);
   angle = mb->angle;

   switch (angle)
     {
      case  90:
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x; ay = zone->y;
           }
         else
           {
              ax = mb->w * -1;
           }
         break;
      case 180:
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x + mb->w;
              ay = zone->y + mb->h;
           }
         else
           {
              ay =  zone->h;
           }
         break;
      case 270:
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x + mb->w;
              ay = zone->y + mb->h;
           }
         else
           {
              ax = zone->w;
           }
         break;
      case   0:
      default :
         if (m->qp_scroll_with_clipping)
           {
              ax = zone->x; ay = zone->y;
           }
         else
           {
              ay = mb->h * -1;;
           }
         break;
     }
   *x = ax;
   *y = ay;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_quickpanel_window_input_region_change_post_job(E_Move_Border *mb)
{
   E_Move *m = NULL;
   int hx = 0, hy = 0, hw = 0, hh = 0; // handle's x, y, w, h

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);

   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);

   if (m->qp_scroll_with_clipping)
     {
        if (e_mod_move_border_shape_input_rect_get(mb, &hx, &hy, &hw, &hh))
          _e_mod_move_quickpanel_handle_objs_size_update(mb, hw, hh);
     }

   return EINA_TRUE;
}
