#include <Elementary.h>
#include <vconf.h>
#include "e.h"
#include "e_mod_main.h"
#define HISTORY_MAX 8
#define DEBUG_INFO 0

#if DEBUG_INFO
  #define INFO(cov, txt) \
    evas_object_text_text_set(cov->text, txt); \
    INF("%s", txt)
#else
  #define INFO(cov, txt) INF("%s", txt)
#endif

typedef struct
{
   E_Zone         *zone;
   Ecore_X_Window  win;
   Ecore_Timer    *timer;
   Ecore_Timer    *double_down_timer;
   Ecore_Timer    *tap_timer;
   Evas_Object    *info;
   Evas_Object    *text;
   int             x, y, dx, dy, mx, my;
   int             mouse_history[HISTORY_MAX];
   unsigned int    dt;
   Eina_Inarray   *two_finger_move;
   Eina_Inlist    *history;

   Ecore_X_Atom    atom_control_panel_open;
   Ecore_X_Atom    atom_back;
   Ecore_X_Atom    atom_scroll;
   Ecore_X_Atom    atom_app_tray_open;

   Eina_Bool       longpressed : 1;
   Eina_Bool       two_finger_down : 1;
   Eina_Bool       three_finger_down : 1;
   Eina_Bool       mouse_double_down : 1;
   Eina_Bool       lock_screen : 1;
} Cover;

typedef struct
{
   EINA_INLIST;
   int             device;
} Multi;

static int g_enable = 0;
static Ecore_X_Window target_win = 0;
static Ecore_X_Window unfocused_win = 0;

static Eina_List *covers = NULL;
static Eina_List *handlers = NULL;
static Ecore_Event_Handler *property_handler = NULL;
static int multi_device[3];

static void
_mouse_in_win_get(Cover *cov, int x, int y)
{
   E_Border *bd;
   Eina_List *l;
   Ecore_X_Window *skip;
   Ecore_X_Window win = 0;
   Cover *cov2;
   int i;

   skip = alloca(sizeof(Ecore_X_Window) * eina_list_count(covers));
   i = 0;
   EINA_LIST_FOREACH(covers, l, cov2)
     {
        skip[i] = cov2->win;
        i++;
     }
   win = ecore_x_window_shadow_tree_at_xy_with_skip_get
     (cov->zone->container->manager->root, x, y, skip, i);

   if (win != target_win)
     {
        target_win = win;

        bd = e_border_focused_get();
        if (bd && (bd->client.win != target_win))
          unfocused_win = target_win;
        else
          unfocused_win = 0;
     }
}

static unsigned int
_win_angle_get(Ecore_X_Window win)
{
   Ecore_X_Window root;

   if (!win) return 0;

   int ret;
   int count;
   int angle = 0;
   unsigned char *prop_data = NULL;

   root = ecore_x_window_root_get(win);
   ret = ecore_x_window_prop_property_get(root,
       ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                         ECORE_X_ATOM_CARDINAL,
                        32, &prop_data, &count);

   if (ret && prop_data)
      memcpy (&angle, prop_data, sizeof (int));

   if (prop_data) free (prop_data);

   return angle;
}

static void
_target_window_find()
{
   Ecore_X_Window win;
   E_Border *bd;
   unsigned int val;
   int ret;

   /* find proper target window to send a meesage */
   Eina_List *borders, *l;

   win = 0;
   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd->visible) continue;
        if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL) break;

        ret = ecore_x_window_prop_card32_get
           (bd->client.win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL, &val, 1);

       if ((ret >= 0) && (val == 1))
         {
            win = bd->client.win;
            break;
         }
     }

   /* there would be an unfocused window which does not have 'val == 1'
      such as an window of virtual keyboard. if the window is selected by
      _mouse_in_win_get(); previously with the unfocused window, the target
      window should be the unfocused window */
   if (win) target_win = win;
   else
     {
        if (unfocused_win) target_win = unfocused_win;
        else
          {
             bd = e_border_focused_get();
             if (bd) target_win = bd->client.win;
          }
     }
}

static void
_lock_screen_check(Cover *cov)
{
   Ecore_X_Window win;
   E_Border *bd;
   const char *name = NULL;
   const char *clas = NULL;

   Eina_List *borders, *l;

   cov->lock_screen = EINA_FALSE;
   win = 0;
   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd) continue;
        if (!bd->visible) continue;

        name = bd->client.icccm.name;
        clas = bd->client.icccm.class;

        if (clas == NULL || name == NULL) continue;
        if (strncmp(clas,"LOCK_SCREEN",strlen("LOCK_SCREEN"))!= 0) continue;
        if (strncmp(name,"LOCK_SCREEN",strlen("LOCK_SCREEN"))!= 0) continue;

        INF("lock screen is detected");
        cov->lock_screen = EINA_TRUE;

        break;
     }
}

static void
_app_tray_open(Cover *cov)
{
   Ecore_X_Window win;
   E_Border *bd;
   const char *name = NULL;
   const char *clas = NULL;

   Eina_List *borders, *l;

   win = 0;
   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd) continue;
        if (!bd->visible) continue;
        if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL) break;

        name = bd->client.icccm.name;
        clas = bd->client.icccm.class;

        if (clas == NULL || name == NULL) continue;
        if (strncmp(clas,"APP_TRAY",strlen("APP_TRAY"))!= 0) continue;
        if (strncmp(name,"APP_TRAY",strlen("APP_TRAY"))!= 0) continue;

        /* open mini app tray */
        INF("open app tray");
        ecore_x_client_message32_send(bd->client.win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      bd->client.win,
                                      cov->atom_app_tray_open,
                                      0, 0, 0);
        break;
     }
}

static void
_quickpanel_open(void)
{
   Ecore_X_Window win;
   E_Border *bd;
   const char *name = NULL;
   const char *clas = NULL;

   Eina_List *borders, *l;

   win = 0;
   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd) continue;
        if (!bd->visible) continue;
        if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL) break;

        name = bd->client.icccm.name;
        clas = bd->client.icccm.class;

        if (clas == NULL || name == NULL) continue;
        if (strncmp(clas,"QUICKPANEL",strlen("QUICKPANEL"))!= 0) continue;
        if (strncmp(name,"QUICKPANEL",strlen("QUICKPANEL"))!= 0) continue;

        /* open quickpanel */
        INF("open quickpanel");
        ecore_x_e_illume_quickpanel_state_send
          (ecore_x_e_illume_zone_get(bd->client.win),
           ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
        break;
     }
}

static void
_coordinate_calibrate(Ecore_X_Window win, int *x, int *y)
{
   int tx, ty, w, h;
   unsigned int angle;

   if (!x) return;
   if (!y) return;

   angle = _win_angle_get(win);
   ecore_x_window_geometry_get(win, NULL, NULL, &w, &h);

   tx = *x;
   ty = *y;

   switch (angle)
     {
      case 90:
        *x = h - ty;
        *y = tx;
        break;

      case 180:
        *x = w - tx;
        *y = h - ty;
        break;

      case 270:
        *x = ty;
        *y = w - tx;
        break;

      default:
        break;
     }
}

static void
_mouse_win_fake_tap(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   int x, y;

   /* find target window to send message */
   _mouse_in_win_get(cov, ev->root.x, ev->root.y);

   ecore_x_pointer_xy_get(target_win, &x, &y);
   ecore_x_mouse_in_send(target_win, x, y);
   ecore_x_mouse_move_send(target_win, x, y);
   ecore_x_mouse_down_send(target_win, x, y, 1);
   ecore_x_mouse_up_send(target_win, x, y, 1);
   ecore_x_mouse_out_send(target_win, x, y);
}

static void
_message_control_panel_open_send(Cover *cov)
{
   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 cov->atom_control_panel_open,
                                 0, 0, 0);
}

static void
_message_back_send(Cover *cov)
{
   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 cov->atom_back,
                                 0, 0, 0);
}

static void
_message_scroll_send(Cover *cov, int type)
{
   int x, y;

   if (cov->lock_screen) type = type + 3;

   ecore_x_pointer_xy_get(target_win, &x, &y);
   _coordinate_calibrate(target_win, &x, &y);

   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 cov->atom_scroll,
                                 type, x, y);
}

static void
_message_read_send(Cover *cov)
{
   int x, y;

   /* find target window to send message */
   _mouse_in_win_get(cov, cov->x, cov->y);

   ecore_x_pointer_xy_get(target_win, &x, &y);
   _coordinate_calibrate(target_win, &x, &y);

   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ,
                                 x, y, 0);
}

static Eina_Bool
_mouse_longpress(void *data)
{
   Cover *cov = data;
   int distance = 40;
   int dx, dy;
   int x, y;

   cov->timer = NULL;
   dx = cov->x - cov->dx;
   dy = cov->y - cov->dy;
   if (((dx * dx) + (dy * dy)) < (distance * distance))
     {
        cov->longpressed = EINA_TRUE;
        INFO(cov, "longpress");

        if (!cov->mouse_double_down) _message_read_send(cov);
        else
          {
             INFO(cov, "double down and longpress");
             /* send message to start longpress */
             ecore_x_pointer_xy_get(target_win, &x, &y);

             ecore_x_mouse_in_send(target_win, x, y);
             ecore_x_mouse_move_send(target_win, x, y);
             ecore_x_mouse_down_send(target_win, x, y, 1);
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_mouse_double_down(void *data)
{
   Cover *cov = data;
   ecore_timer_del(cov->double_down_timer);
   cov->double_down_timer = NULL;
   return EINA_FALSE;
}

static void
_mouse_double_down_timeout(Cover *cov)
{
   double long_time = 0.5;
   double short_time = 0.3;
   int distance = 40;
   int dx, dy;

   dx = cov->x - cov->dx;
   dy = cov->y - cov->dy;

   if ((cov->double_down_timer) &&
       (((dx * dx) + (dy * dy)) < (distance * distance)))
     {
        /* start double tap and move from here */
        cov->mouse_double_down = EINA_TRUE;

        if (cov->timer)
          {
             ecore_timer_del(cov->timer);
             cov->timer = NULL;
          }
        /* check longpress after double down */
        cov->timer = ecore_timer_add(long_time, _mouse_longpress, cov);
     }

   if (cov->double_down_timer)
     {
        ecore_timer_del(cov->double_down_timer);
        cov->double_down_timer = NULL;
        return;
     }

   cov->double_down_timer = ecore_timer_add(short_time, _mouse_double_down, cov);
}

static Eina_Bool
_mouse_tap(void *data)
{
   Cover *cov = data;
   cov->tap_timer = NULL;

   _message_read_send(cov);

   return EINA_FALSE;
}

static void
_mouse_down(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   double longtime = 0.5;

   cov->dx = ev->x;
   cov->dy = ev->y;
   cov->mx = ev->x;
   cov->my = ev->y;
   cov->x = ev->x;
   cov->y = ev->y;
   cov->dt = ev->timestamp;
   cov->longpressed = EINA_FALSE;
   cov->timer = ecore_timer_add(longtime, _mouse_longpress, cov);

   if (cov->tap_timer)
     {
        ecore_timer_del(cov->tap_timer);
        cov->tap_timer = NULL;
     }

   /* check mouse double down - not two fingers, refer to double click */
   _mouse_double_down_timeout(cov);
}

static void
_circle_draw_check(Cover *cov)
{
   Ecore_Event_Mouse_Move *ev, *t_ev, *b_ev, *l_ev, *r_ev;
   Evas_Coord_Point m_tb, m_lr;
   int count = 0;
   int offset, i;
   int min_x, min_y, max_x, max_y;
   int left = 0, right = 0, top = 0, bottom = 0;
   int distance;

   count = eina_inarray_count(cov->two_finger_move);
   if (count < 10 || count > 60) goto inarray_free;

   offset = count / 8;

   i = 0;
   EINA_INARRAY_FOREACH(cov->two_finger_move, ev)
     {
        if (i == 0)
          {
             min_x = ev->x;
             max_x = ev->x;
             min_y = ev->y;
             max_y = ev->y;
          }

        if (ev->x < min_x)
          {
             min_x = ev->x;
             left = i;
          }

        if (ev->y < min_y)
          {
             min_y = ev->y;
             bottom = i;
          }

        if (ev->x > max_x)
          {
             max_x = ev->x;
             right = i;
          }

        if (ev->y > max_y)
          {
             max_y = ev->y;
             top = i;
          }

        i++;
     }

   t_ev = eina_inarray_nth(cov->two_finger_move, top);
   b_ev = eina_inarray_nth(cov->two_finger_move, bottom);
   m_tb.x = (t_ev->x + b_ev->x) / 2;
   m_tb.y = (t_ev->y + b_ev->y) / 2;


   l_ev = eina_inarray_nth(cov->two_finger_move, left);
   r_ev = eina_inarray_nth(cov->two_finger_move, right);
   m_lr.x = (l_ev->x + r_ev->x) / 2;
   m_lr.y = (l_ev->y + r_ev->y) / 2;

   distance = (int) sqrt(((m_tb.x - m_lr.x) * (m_tb.x - m_lr.x)) + ((m_tb.y - m_lr.y) * (m_tb.y - m_lr.y)));

   i = 0;
   if (top > left) i++;
   if (left > bottom) i++;
   if (bottom > right) i++;
   if (right > top) i++;

   if ((i >= 3) && (distance < 60))
     {
        INFO(cov, "two finger circle draw");
        _message_back_send(cov);
     }

inarray_free:
   eina_inarray_free(cov->two_finger_move);
}

static void
_mouse_up(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   double timeout = 0.15;
   double double_tap_timeout = 0.25;
   int distance = 40;
   int dx, dy;
   int x, y;
   int angle = 0;

   if (cov->three_finger_down)
     {
        cov->three_finger_down = EINA_FALSE;

        dx = ev->x - cov->dx;
        dy = ev->y - cov->dy;

        if (((dx * dx) + (dy * dy)) > (4 * distance * distance)
            && ((ev->timestamp - cov->dt) < (timeout * 1000)))
          {
             /* get root window rotation */
             angle = _win_angle_get(target_win);

             if (abs(dx) < abs(dy))
               {
                  if (dy > 0) /* down */
                    {
                       INFO(cov, "three finger flick down");
                       switch (angle)
                         {
                          case 90:
                          case 180:
                            _app_tray_open(cov);
                            break;

                          case 270:
                          default:
                            _quickpanel_open();
                            break;
                         }
                    }
                  else /* up */
                    {
                       INFO(cov, "three finger flick up");
                       switch (angle)
                         {
                          case 90:
                          case 180:
                            _quickpanel_open();
                          break;

                          case 270:
                          default:
                            _app_tray_open(cov);
                          break;
                         }
                    }
               }
          }
        goto end;
     }

   /* for two finger panning */
   if (cov->two_finger_down)
     {
        cov->two_finger_down = EINA_FALSE;

        _message_scroll_send(cov, 2);
        cov->lock_screen = EINA_FALSE;

        /* to check 2 finger mouse move */
        if (cov->two_finger_move) _circle_draw_check(cov);

        dx = ev->x - cov->dx;
        dy = ev->y - cov->dy;

        if (((dx * dx) + (dy * dy)) < (distance * distance))
          {
             if (ev->double_click)
               {
                  INFO(cov, "two finger double click");
                  _message_control_panel_open_send(cov);
               }
          }
        goto end;
     }

   if (cov->mouse_double_down)
     {
        /* reset double down and moving: action up/down */
        cov->mouse_double_down = EINA_FALSE;

        if (cov->longpressed)
          {
             INFO(cov, "mouse release after longpress");
             ecore_x_pointer_xy_get(target_win, &x, &y);

             ecore_x_mouse_up_send(target_win, x, y, 1);
             ecore_x_mouse_out_send(target_win, x, y);
          }
     }

   /* delete timer which is used for checking longpress */
   if (cov->timer)
     {
        ecore_timer_del(cov->timer);
        cov->timer = NULL;
     }

   if (cov->longpressed)
     {
        cov->longpressed = EINA_FALSE;
        return;
     }

   dx = ev->x - cov->dx;
   dy = ev->y - cov->dy;
   if (((dx * dx) + (dy * dy)) < (distance * distance))
     {
        if (ev->double_click)
          {
             INFO(cov, "double_click");
             ecore_x_e_illume_access_action_activate_send(target_win);
          }
        else
          {
             cov->tap_timer = ecore_timer_add(double_tap_timeout,
                                         _mouse_tap, cov);
          }
     }
   else if (((dx * dx) + (dy * dy)) > (4 * distance * distance)
            && ((ev->timestamp - cov->dt) < (timeout * 1000)))
     {
        /* get root window rotation */
        angle = _win_angle_get(target_win);

        if (abs(dx) > abs(dy)) /* left or right */
          {
             if (dx > 0) /* right */
               {
                  INFO(cov, "single flick right");
                  switch (angle)
                    {
                     case 180:
                     case 270:
                       ecore_x_e_illume_access_action_read_prev_send
                                                        (target_win);
                     break;

                     case 90:
                     default:
                       ecore_x_e_illume_access_action_read_next_send
                                                        (target_win);
                     break;
                    }

               }
             else /* left */
               {
                  INFO(cov, "single flick left");
                  switch (angle)
                    {
                     case 180:
                     case 270:
                       ecore_x_e_illume_access_action_read_next_send
                                                        (target_win);
                     break;

                     case 90:
                     default:
                       ecore_x_e_illume_access_action_read_prev_send
                                                        (target_win);
                     break;
                    }
               }
          }
        else /* up or down */
          {
             if (dy > 0) /* down */
               {
                  INFO(cov, "single flick down");
                  switch (angle)
                    {
                     case 90:
                     case 180:
                       ecore_x_e_illume_access_action_up_send(target_win);
                     break;

                     case 270:
                     default:
                       ecore_x_e_illume_access_action_down_send(target_win);
                     break;
                    }
               }
             else /* up */
               {
                  INFO(cov, "single flick up");
                  switch (angle)
                    {
                     case 90:
                     case 180:
                       ecore_x_e_illume_access_action_down_send(target_win);
                     break;

                     case 270:
                     default:
                       ecore_x_e_illume_access_action_up_send(target_win);
                     break;
                    }
               }
          }
     }

end:
   cov->longpressed = EINA_FALSE;
}

static void
_mouse_move(Cover *cov __UNUSED__, Ecore_Event_Mouse_Move *ev __UNUSED__)
{
   //FIXME: why here, after long press you cannot go below..
   //if (!cov->down) return;

   //FIXME: one finger cannot come here
   //_record_mouse_history(cov, ev);
   if (cov->two_finger_move) eina_inarray_push(cov->two_finger_move, ev);

   _message_scroll_send(cov, 1);
}

static void
_mouse_wheel(Cover *cov __UNUSED__, Ecore_Event_Mouse_Wheel *ev __UNUSED__)
{
   if (ev->z == -1) /* up */
     {
#if ECORE_VERSION_MAJOR >= 1
# if ECORE_VERSION_MINOR >= 8
        ecore_x_e_illume_access_action_up_send(target_win);
# endif
#endif
     }
   else if (ev->z == 1) /* down */
     {
#if ECORE_VERSION_MAJOR >= 1
# if ECORE_VERSION_MINOR >= 8
        ecore_x_e_illume_access_action_down_send(target_win);
# endif
#endif
     }
}

static Eina_Bool
_cb_mouse_down(void    *data __UNUSED__,
               int      type __UNUSED__,
               void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Eina_List *l;
   Cover *cov;
   int i = 0;

   for (i = 0; i < 3; i++)
     {
        if (multi_device[i] == -1)
          {
             multi_device[i] = ev->multi.device;
             break;
          }
        else if (multi_device[i] == ev->multi.device) break;
     }

   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             //XXX change specific number
             if (ev->multi.device == multi_device[0])
               {
                  _target_window_find();
                  _mouse_down(cov, ev);
               }

             if (ev->multi.device == multi_device[1] && !(cov->two_finger_down))
               {
                  /* prevent longpress client message by two finger */
                  if (cov->timer)
                    {
                       ecore_timer_del(cov->timer);
                       cov->timer = NULL;
                    }

                  cov->two_finger_down = EINA_TRUE;

                  _lock_screen_check(cov);
                  _message_scroll_send(cov, 0);

                  /* to check 2 finger mouse move */
                  cov->two_finger_move = eina_inarray_new(sizeof(Ecore_Event_Mouse_Move), 0);
               }

             if (ev->multi.device == multi_device[2] && !(cov->three_finger_down))
               {
                  cov->three_finger_down = EINA_TRUE;

                  if (cov->two_finger_down)
                    {
                       cov->two_finger_down = EINA_FALSE;

                       _message_scroll_send(cov, 2);
                       cov->lock_screen = EINA_FALSE;

                       eina_inarray_free(cov->two_finger_move);
                    }
               }
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_up(void    *data __UNUSED__,
             int      type __UNUSED__,
             void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Eina_List *l;
   Cover *cov;

   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             /* the first finger: 1, from the second finger: 0 */
             if (ev->buttons == 1)
               _mouse_up(cov, ev);
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_move(void    *data __UNUSED__,
               int      type __UNUSED__,
               void    *event)
{
   Ecore_Event_Mouse_Move *ev = event;
   Eina_List *l;
   Cover *cov;
   int x, y;

   EINA_LIST_FOREACH(covers, l, cov)
     {
        cov->x = ev->x;
        cov->y = ev->y;

        if (ev->window == cov->win)
          {
             //if (ev->multi.device == multi_device[0] || ev->multi.device == multi_device[1])
             if (cov->two_finger_down && ev->multi.device == multi_device[1])
               _mouse_move(cov, ev);
             else if (cov->longpressed && /* client message for moving is available only after long press is detected */
                      !(cov->mouse_double_down) && /* mouse move after double down should not send read message */
                      !(cov->two_finger_down) && ev->multi.device == multi_device[0])
               {
                  INFO(cov, "read");
                  _message_read_send(cov);
               }
             else if (cov->mouse_double_down && /* client message for moving is available only after long press is detected */
                      !(cov->two_finger_down) && ev->multi.device == multi_device[0])
               {
                  if (cov->longpressed)
                    {
                       INFO(cov, "move after longpress");
                       /* send message to notify move after longpress */
                       ecore_x_pointer_xy_get(target_win, &x, &y);

                       ecore_x_mouse_move_send(target_win, x, y);
                    }
               }

             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_wheel(void    *data __UNUSED__,
                int      type __UNUSED__,
                void    *event)
{
   Ecore_Event_Mouse_Wheel *ev = event;
   Eina_List *l;
   Cover *cov;

   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             _mouse_wheel(cov, ev);
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Cover *
_cover_new(E_Zone *zone)
{
   Cover *cov;

   cov = E_NEW(Cover, 1);
   if (!cov) return NULL;
   cov->zone = zone;

#if DEBUG_INFO
   Ecore_Evas *ee;
   ee = ecore_evas_new(NULL,
                       zone->container->x + zone->x,
                       zone->container->y + zone->y,
                       zone->w, zone->h,
                       NULL);
   ecore_evas_alpha_set(ee, EINA_TRUE);
   cov->win = (Ecore_X_Window)ecore_evas_window_get(ee);

   /* create infomation */
   Evas *e;
   e = ecore_evas_get(ee);
   cov->info = evas_object_rectangle_add(e);
   evas_object_color_set(cov->info, 255, 255, 255, 100);
   evas_object_move(cov->info, zone->container->x + zone->x, zone->container->y + zone->y);
   evas_object_resize(cov->info, zone->w, 30);
   evas_object_show(cov->info);

   cov->text = evas_object_text_add(e);
   evas_object_text_style_set(cov->text, EVAS_TEXT_STYLE_PLAIN);
   evas_object_text_font_set(cov->text, "DejaVu", 14);
   evas_object_text_text_set(cov->text, "screen-reader module");

   evas_object_color_set(cov->text, 0, 0, 0, 255);
   evas_object_resize(cov->text, (zone->w / 8), 20);
   evas_object_move(cov->text, zone->container->x + zone->x + 5, zone->container->y + zone->y + 5);
   evas_object_show(cov->text);

#else
   cov->win = ecore_x_window_input_new(zone->container->manager->root,
                                       zone->container->x + zone->x,
                                       zone->container->y + zone->y,
                                       zone->w, zone->h);
#endif

   ecore_x_input_multi_select(cov->win);

   ecore_x_icccm_title_set(cov->win, "screen-reader-input");
   ecore_x_netwm_name_set(cov->win, "screen-reader-input");

   ecore_x_window_ignore_set(cov->win, 1);
   ecore_x_window_configure(cov->win,
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                            0, 0, 0, 0, 0,
                            zone->container->layers[8].win,
                            ECORE_X_WINDOW_STACK_ABOVE);
   ecore_x_window_show(cov->win);
   ecore_x_window_raise(cov->win);

   return cov;
}

static void
_covers_init(void)
{
   Eina_List *l, *l2, *l3;
   E_Manager *man;
   int i = 0;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Container *con;
        EINA_LIST_FOREACH(man->containers, l2, con)
          {
             E_Zone *zone;
             EINA_LIST_FOREACH(con->zones, l3, zone)
               {
                  Cover *cov = _cover_new(zone);
                  if (cov)
                    {
                       covers = eina_list_append(covers, cov);
                       for (i = 0; i < HISTORY_MAX; i++) cov->mouse_history[i] = -1;

                       cov->atom_control_panel_open = ecore_x_atom_get("_E_MOD_SCREEN_READER_ACTION_CONTROL_PANEL_OPEN_");
                       cov->atom_back = ecore_x_atom_get("_E_MOD_SCREEN_READER_ACTION_BACK_");
                       cov->atom_scroll = ecore_x_atom_get("_E_MOD_SCREEN_READER_ACTION_SCROLL_");
                       cov->atom_app_tray_open = ecore_x_atom_get("_E_MOD_SCREEN_READER_ACTION_APP_TRAY_OPEN_");
                    }
               }
          }
     }
}

static void
_covers_shutdown(void)
{
   Cover *cov;

   EINA_LIST_FREE(covers, cov)
     {
        ecore_x_window_ignore_set(cov->win, 0);
        ecore_x_window_free(cov->win);
        evas_object_del(cov->info);
        evas_object_del(cov->text);

        if (cov->timer)
          {
             ecore_timer_del(cov->timer);
             cov->timer = NULL;
          }

        if (cov->double_down_timer)
          {
             ecore_timer_del(cov->double_down_timer);
             cov->double_down_timer = NULL;
          }

        if (cov->tap_timer)
          {
             ecore_timer_del(cov->tap_timer);
             cov->tap_timer = NULL;
          }

        free(cov);
     }
}

static Eina_Bool
_cb_zone_add(void    *data __UNUSED__,
             int      type __UNUSED__,
             void    *event __UNUSED__)
{
   _covers_shutdown();
   _covers_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_zone_del(void    *data __UNUSED__,
             int      type __UNUSED__,
             void    *event __UNUSED__)
{
   _covers_shutdown();
   _covers_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_zone_move_resize(void    *data __UNUSED__,
                     int      type __UNUSED__,
                     void    *event __UNUSED__)
{
   _covers_shutdown();
   _covers_init();
   return ECORE_CALLBACK_PASS_ON;
}

static void
_events_init(void)
{
   int i = 0;

   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                        _cb_mouse_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                        _cb_mouse_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                        _cb_mouse_move, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL,
                                        _cb_mouse_wheel, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(E_EVENT_ZONE_ADD,
                                        _cb_zone_add, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(E_EVENT_ZONE_DEL,
                                        _cb_zone_del, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
                                        _cb_zone_move_resize, NULL));

   for (i = 0; i < 3; i++) multi_device[i] = -1;
}

static void
_events_shutdown(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
}

static Eina_Bool
_cb_property_change(void *data __UNUSED__,
                   int   type __UNUSED__,
                   void *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Property *event = ev;

   if (event->atom == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        bd = e_border_focused_get();
        if (bd) target_win = bd->client.win;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_vconf_cb_accessibility_tts_change(keynode_t *node,
                                   void      *data)
{
   int enable = 0;

   if (!node) return;

   enable = vconf_keynode_get_bool(node);
   if (g_enable == enable) return;

   g_enable = enable;
   if (enable)
     {
        INF("[screen reader module] module enable");
        _covers_shutdown();
        _covers_init();
        _events_shutdown();
        _events_init();
     }
   else
     {
        INF("[screen reader module] module disable");
        _covers_shutdown();
        _events_shutdown();
     }

   elm_config_access_set(enable);
   elm_config_all_flush();
   elm_config_save();
}
/***************************************************************************/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Screen Reader"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                            _vconf_cb_accessibility_tts_change,
                            NULL);

   ecore_x_event_mask_set(ecore_x_window_root_first_get(),
                          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
   ecore_x_event_mask_set(ecore_x_window_root_first_get(),
                          ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
   property_handler = ecore_event_handler_add
                       (ECORE_X_EVENT_WINDOW_PROPERTY, _cb_property_change, NULL);

   if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &g_enable) != 0)
     INF("vconf get failed\n");

   if (g_enable)
     {
        _covers_shutdown();
        _covers_init();
        _events_shutdown();
        _events_init();
     }
   else
     {
        _covers_shutdown();
        _events_shutdown();
     }

   elm_config_access_set(g_enable);
   elm_config_all_flush();
   elm_config_save();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   INF("[screen-reader module] module shutdown");
   if (property_handler) ecore_event_handler_del(property_handler);

   _covers_shutdown();
   _events_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
