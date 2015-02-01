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
    #define INFO(cov, txt) SECURE_SLOG(LOG_DEBUG, LOG_TAG, "%s -> %x", txt, target_win);
#endif

#define MOUSE_BUTTON_DOWN 0
#define MOUSE_MOVE 1
#define MOUSE_BUTTON_UP 2
#define LONGPRESS_TIME 0.5
#define DOUBLE_DOWN_TIME 0.3
#define RAD2DEG(x) ((x) * 57.295779513)

typedef struct
{
   E_Zone         *zone;
   Ecore_X_Window  win;
   Ecore_X_Window  down_win;
   Ecore_Timer    *timer;
   Ecore_Timer    *double_down_timer;
   Ecore_Timer    *tap_timer;
   Evas_Object    *info;
   Evas_Object    *text;
   int             x, y, dx, dy, mx, my;
   int             mouse_history[HISTORY_MAX];
   unsigned int    dt;
   unsigned int    n_taps;
   Eina_Inarray   *two_finger_move;
   Eina_Inlist    *history;

   Eina_Bool       longpressed : 1;
   Eina_Bool       two_finger_down : 1;
   Eina_Bool       three_finger_down : 1;
   Eina_Bool       mouse_double_down : 1;
} Cover;

typedef struct
{
   EINA_INLIST;
   int             device;
} Multi;

static int zoom_base = 0;
static int g_enable = 0;
#ifdef ENABLE_RAPID_KEY_INPUT
static int rapid_input = 0;
#endif
static Ecore_X_Window target_win = 0;

static Ecore_X_Atom E_MOD_SCREEN_READER_ATOM_CONTROL_PANEL_OPEN;
static Ecore_X_Atom E_MOD_SCREEN_READER_ATOM_APP_TRAY_OPEN;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL;

static Eina_List *covers = NULL;
static Eina_List *handlers = NULL;
static Ecore_Event_Handler *property_handler = NULL;
static Ecore_Event_Handler *border_show_handler = NULL;
static Ecore_Event_Handler *border_move_handler = NULL;
static Ecore_Event_Handler *border_hide_handler = NULL;
static Ecore_Event_Handler *border_stack_handler = NULL;

static void _move_module_enable_set(int enable);
static void _screen_reader_support_check();

static void
_target_window_change(Ecore_X_Window new_target)
{
   if (target_win != new_target)
     {
        ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      target_win,
                                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_DISABLE,
                                      0, 0, 0);

        ecore_x_client_message32_send(new_target, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      new_target,
                                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ENABLE,
                                      0, 0, 0);

        target_win = new_target;
     }
   _screen_reader_support_check();
}

static void
_top_window_get(E_Zone *zone)
{
   E_Border *bd;
   E_Border_List *bl;

   bl = e_container_border_list_last(zone->container);

   while ((bd = e_container_border_list_prev(bl)))
     {
        if (!bd->visible) continue;
        if (E_CONTAINS(zone->x, zone->y, zone->w, zone->h, bd->x, bd->y, bd->w, bd->h))
          {
             _target_window_change(bd->client.win);
             break;
          }
     }
   e_container_border_list_free(bl);
}

static void
_mouse_in_win_get(Cover *cov, int x, int y)
{
   E_Border *bd, *child;
   Eina_List *l;
   Ecore_X_Window *skip;
   Ecore_X_Window win = 0;
   Ecore_X_Window child_win = 0;
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

   bd = e_border_find_all_by_client_window(win);

   if (bd)
     {
        EINA_LIST_FOREACH(bd->transients, l, child)
          {
             if (!strcmp(child->client.icccm.name, "Center Popup"))
               child_win = child->client.win;
          }
     }

   if (child_win)
     _target_window_change(child_win);
   else
     _target_window_change(win);
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
_cov_data_reset(Cover *cov)
{
   cov->n_taps = 0;
   cov->longpressed = EINA_FALSE;
   cov->two_finger_down = EINA_FALSE;
   cov->two_finger_move = EINA_FALSE;
   cov->mouse_double_down = EINA_FALSE;
   cov->three_finger_down = EINA_FALSE;

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
}

static void
_screen_reader_support_check()
{
   int ret;
   unsigned int val;
   Eina_List *l;
   Cover *cov;
   E_Border *bd;
   E_Border_List *bl;
   Eina_Bool supported = EINA_FALSE;

   ret = ecore_x_window_prop_card32_get
      (target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL, &val, 1);

   if ((ret >= 0) && (val == 2))
     {
        /* hide input window */
        EINA_LIST_FOREACH(covers, l, cov)
          {
             bl = e_container_border_list_last(cov->zone->container);

             while ((bd = e_container_border_list_prev(bl)))
               {
                  if (!bd->visible) continue;
                  if (bd->client.win == target_win) break;
                  if (E_CONTAINS(cov->zone->x, cov->zone->y,
                                 cov->zone->w, cov->zone->h,
                                 bd->x, bd->y, bd->w, bd->h))
                    {
                       ecore_x_window_move(cov->win, bd->x, bd->y);
                       ecore_x_window_resize(cov->win, bd->w, bd->h);
                       supported = EINA_TRUE;
                       break;
                    }
               }
             e_container_border_list_free(bl);

             if (!supported)
               ecore_x_window_hide(cov->win);
             _cov_data_reset(cov);
          }

        _move_module_enable_set(EINA_FALSE);
     }
   else
     {
        /* show input window */
        EINA_LIST_FOREACH(covers, l, cov)
          {
             ecore_x_window_move(cov->win,
                                 cov->zone->container->x + cov->zone->x,
                                 cov->zone->container->y + cov->zone->y);
             ecore_x_window_resize(cov->win, cov->zone->w, cov->zone->h);
             ecore_x_window_show(cov->win);
          }

        _move_module_enable_set(EINA_TRUE);
     }
}

#ifdef ENABLE_RAPID_KEY_INPUT
static Eina_Bool
_keyboard_check(Cover *cov)
{
   E_Border *bd = NULL;
   const char *name = NULL;
   const char *clas = NULL;

   Eina_List *borders, *l;

   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd) continue;
        if (!bd->visible) continue;
        if (bd->client.win == target_win) break;
     }

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if (clas == NULL || name == NULL) return EINA_FALSE;
   if (strncmp(clas,"ISF",strlen("ISF"))) return EINA_FALSE;
   if (!strncmp(name,"Setting Window",strlen("Setting Window"))) return EINA_FALSE;

   INF("keyboard is detected");

   return EINA_TRUE;
}
#endif
static void
_app_tray_open(Cover *cov)
{
   E_Border *bd;
   const char *name = NULL;
   const char *clas = NULL;

   Eina_List *borders, *l;

   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd) continue;
        if (!bd->visible) continue;

        /* UTILITY type such as keyboard window could come first, before NORMAL
           type such as app tray, quickpanel window comes */
        if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UTILITY) continue;
        if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL) break;

        name = bd->client.icccm.name;
        clas = bd->client.icccm.class;

        if (clas == NULL || name == NULL) continue;
        if (strncmp(clas,"MINIAPP_TRAY",strlen("MINIAPP_TRAY"))!= 0) continue;
        if (strncmp(name,"MINIAPP_TRAY",strlen("MINIAPP_TRAY"))!= 0) continue;

        /* open mini app tray */
        INF("open app tray");
        ecore_x_client_message32_send(bd->client.win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      bd->client.win,
                                      E_MOD_SCREEN_READER_ATOM_APP_TRAY_OPEN,
                                      0, 0, 0);
        break;
     }
}

static void
_quickpanel_open(void)
{
   E_Border *bd;
   const char *name = NULL;
   const char *clas = NULL;

   Eina_List *borders, *l;

   borders = e_border_client_list();
   EINA_LIST_REVERSE_FOREACH(borders, l, bd)
     {
        if (!bd) continue;
        if (!bd->visible) continue;

        /* UTILITY type such as keyboard window could come first, before NORMAL
           type such as app tray, quickpanel window comes.
           NOTIFICATION: it needs for the lock screen */
        if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UTILITY) continue;
        if ((bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NOTIFICATION)) break;

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

        _target_window_change(bd->client.win);
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

static double
_angle_get(Evas_Coord xx1,
           Evas_Coord yy1,
           Evas_Coord xx2,
           Evas_Coord yy2)
{
   double a, xx, yy, rt = (-1);

   xx = abs(xx2 - xx1);
   yy = abs(yy2 - yy1);

   if (((int)xx) && ((int)yy))
     {
        rt = a = RAD2DEG(atan(yy / xx));
        if (xx1 < xx2)
          {
             if (yy1 < yy2) rt = 360 - a;
             else rt = a;
          }
        else
          {
             if (yy1 < yy2) rt = 180 + a;
             else rt = 180 - a;
          }
     }

   if (rt < 0) /* Do this only if rt is not set */
     {
        if (((int)xx)) /* Horizontal line */
          {
             if (xx2 < xx1) rt = 180;
             else rt = 0.0;
          }
        else
          {  /* Vertical line */
            if (yy2 < yy1) rt = 90;
            else rt = 270;
          }
     }

   /* Now we want to change from:
    *                      90                   0
    * original circle   180   0   We want:  270   90
    *                     270                 180
    */
   rt = 450 - rt;
   if (rt >= 360) rt -= 360;

   return rt;
}

static double
_zoom_get(Evas_Coord xx1,
              Evas_Coord yy1,
              Evas_Coord xx2,
              Evas_Coord yy2)
{
   Evas_Coord diam;
   double rt = 1.0;
   double xx, yy;

   xx = abs(xx2 - xx1);
   yy = abs(yy2 - yy1);
   diam = sqrt((xx * xx) + (yy * yy));

   if (!zoom_base)
     {
        zoom_base = diam;
        return 1.0;
     }

   rt = ((1.0) + ((((float)diam - (float)zoom_base) /
                   (float)zoom_base)));

   return rt;
}

static void
_zoom_check(Cover *cov, double *zoom, double *angle)
{
   Ecore_Event_Mouse_Move *ev;
   Evas_Coord xx1 = 0, yy1 = 0, xx2 = 0, yy2 = 0;
   int i, n = 1;

   if (!cov || !cov->two_finger_move) return;

   EINA_INARRAY_FOREACH(cov->two_finger_move, ev)
     {
        if (ev->multi.device == 0)
          {
             xx1 = ev->root.x;
             yy1 = ev->root.y;
          }
        else
          {
             xx2 = ev->root.x;
             yy2 = ev->root.y;
          }

        if (xx1 && yy1 && xx2 && yy2)
          {
             _coordinate_calibrate(target_win, &xx1, &yy1);
             _coordinate_calibrate(target_win, &xx2, &yy2);
             *zoom = _zoom_get(xx1, yy1, xx2, yy2);
             *angle = _angle_get(xx1, yy1, xx2, yy2);
             for (i = 0; i < n; i++)
               eina_inarray_remove_at(cov->two_finger_move, i);
             return;
          }
        n++;
     }
}

static void
_message_control_panel_open_send(Cover *cov)
{
   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 E_MOD_SCREEN_READER_ATOM_CONTROL_PANEL_OPEN,
                                 0, 0, 0);
}

static void
_message_back_send(Cover *cov)
{
   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_BACK,
                                 0, 0, 0);
}

static void
_message_scroll_send(Cover *cov, int type)
{
   int x, y;
   Ecore_X_Atom atom;

   atom = ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_SCROLL;

   ecore_x_pointer_xy_get(target_win, &x, &y);
   _coordinate_calibrate(target_win, &x, &y);

   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 atom,
                                 type, x, y);
}

static void
_message_zoom_send(Cover *cov, int type)
{
   static double zoom = 1.0, angle = 0.0;
   Ecore_X_Atom atom;

   atom = ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ZOOM;

   _zoom_check(cov, &zoom, &angle);

   if (type == MOUSE_BUTTON_DOWN) zoom = 1.0;
   else if (type == MOUSE_BUTTON_UP) zoom_base = 0;
   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 atom,
                                 type, zoom * 1000, angle * 1000);
}

static void
_message_mouse_send(Cover *cov, int type)
{
   int x, y;

   ecore_x_pointer_xy_get(cov->down_win, &x, &y);
   _coordinate_calibrate(cov->down_win, &x, &y);

   ecore_x_client_message32_send(cov->down_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 cov->down_win,
                                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_MOUSE,
                                 type, x, y);
}

static void
_message_read_send(Cover *cov)
{
   int x, y;

   _mouse_in_win_get(cov, cov->x, cov->y);
   ecore_x_pointer_xy_get(target_win, &x, &y);
   _coordinate_calibrate(target_win, &x, &y);

   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ,
                                 x, y, 0);
}

static void
_message_over_send(Cover *cov, int type)
{
   int x, y;

   _mouse_in_win_get(cov, cov->x, cov->y);
   ecore_x_pointer_xy_get(target_win, &x, &y);
   _coordinate_calibrate(target_win, &x, &y);

   ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 target_win,
                                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_OVER,
                                 type, x, y);
}

static Eina_Bool
_mouse_longpress(void *data)
{
   Cover *cov = data;
   int distance = 40;
   int dx, dy;

   cov->timer = NULL;
   dx = cov->x - cov->dx;
   dy = cov->y - cov->dy;
   if (!cov->mouse_double_down)
     {
        cov->longpressed = EINA_TRUE;
        INFO(cov, "over");
        _message_over_send(cov, MOUSE_BUTTON_DOWN);
     }
   else if (((dx * dx) + (dy * dy)) < (distance * distance))
     {
        cov->longpressed = EINA_TRUE;
        INFO(cov, "longpress");

        /* send message to start longpress,
           keep previous target window to send mouse-up event */
        cov->down_win = target_win;

        _message_mouse_send(cov, MOUSE_BUTTON_DOWN);
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
        cov->timer = ecore_timer_add(LONGPRESS_TIME, _mouse_longpress, cov);
     }

   if (cov->double_down_timer)
     {
        ecore_timer_del(cov->double_down_timer);
        cov->double_down_timer = NULL;
        return;
     }

   cov->double_down_timer = ecore_timer_add(DOUBLE_DOWN_TIME, _mouse_double_down, cov);
}

static Eina_Bool
_mouse_tap(void *data)
{
   Cover *cov = data;
   cov->tap_timer = NULL;

   _message_read_send(cov);

#ifdef ENABLE_RAPID_KEY_INPUT
   if (rapid_input && _keyboard_check(cov))
     ecore_x_e_illume_access_action_activate_send(target_win);
#endif

   return EINA_FALSE;
}

static void
_mouse_down(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   cov->dx = ev->root.x;
   cov->dy = ev->root.y;
   cov->mx = ev->root.x;
   cov->my = ev->root.y;
   cov->x = ev->root.x;
   cov->y = ev->root.y;
   cov->dt = ev->timestamp;
   cov->longpressed = EINA_FALSE;
   cov->timer = ecore_timer_add(LONGPRESS_TIME, _mouse_longpress, cov);
   cov->down_win = 0;

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
             min_x = ev->root.x;
             max_x = ev->root.x;
             min_y = ev->root.y;
             max_y = ev->root.y;
          }

        if (ev->root.x < min_x)
          {
             min_x = ev->root.x;
             left = i;
          }

        if (ev->root.y < min_y)
          {
             min_y = ev->root.y;
             bottom = i;
          }

        if (ev->root.x > max_x)
          {
             max_x = ev->root.x;
             right = i;
          }

        if (ev->root.y > max_y)
          {
             max_y = ev->root.y;
             top = i;
          }

        i++;
     }

   t_ev = eina_inarray_nth(cov->two_finger_move, top);
   b_ev = eina_inarray_nth(cov->two_finger_move, bottom);
   m_tb.x = (t_ev->root.x + b_ev->root.x) / 2;
   m_tb.y = (t_ev->root.y + b_ev->root.y) / 2;


   l_ev = eina_inarray_nth(cov->two_finger_move, left);
   r_ev = eina_inarray_nth(cov->two_finger_move, right);
   m_lr.x = (l_ev->root.x + r_ev->root.x) / 2;
   m_lr.y = (l_ev->root.y + r_ev->root.y) / 2;

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
   int angle = 0;

   if (cov->three_finger_down)
     {
        cov->three_finger_down = EINA_FALSE;

        dx = ev->root.x - cov->dx;
        dy = ev->root.y - cov->dy;

        if (((dx * dx) + (dy * dy)) > (4 * distance * distance)
            && ((ev->timestamp - cov->dt) < (timeout * 3000)))
          {
             /* get root window rotation */
             angle = _win_angle_get(target_win);

             if (abs(dx) > abs(dy)) /* left or right */
               {
                  if (dx > 0) /* right */
                    {
                       INFO(cov, "three finger swipe right");
                       switch (angle)
                         {
                          case 270:
                            _app_tray_open(cov);
                          break;

                          case 90:
                            _quickpanel_open();
                          break;
                         }

                    }
                  else /* left */
                    {
                       INFO(cov, "three finger swipe left");
                       switch (angle)
                         {
                          case 270:
                            _quickpanel_open();
                          break;

                          case 90:
                            _app_tray_open(cov);
                          break;

                         }
                    }
               }
             else /* up or down */
               {
                  if (dy > 0) /* down */
                    {
                       INFO(cov, "three finger swipe down");
                       switch (angle)
                         {
                          case 180:
                          default:
                            _quickpanel_open();
                          break;
                         }
                    }
                  else /* up */
                    {
                       INFO(cov, "three finger swipe up");
                       switch (angle)
                         {
                          case 180:
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

        _message_scroll_send(cov, MOUSE_BUTTON_UP);
        _message_zoom_send(cov, MOUSE_BUTTON_UP);

        /* to check 2 finger mouse move */
        if (cov->two_finger_move) _circle_draw_check(cov);

        dx = ev->root.x - cov->dx;
        dy = ev->root.y - cov->dy;

        if (((dx * dx) + (dy * dy)) < (distance * distance))
          {
             if (ev->double_click && !ev->triple_click)
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
             /* mouse up after longpress */
             _message_mouse_send(cov, MOUSE_BUTTON_UP);
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
        if (!(cov->mouse_double_down) && !(cov->two_finger_down) &&
            ev->multi.device == 0)
          {
             INFO(cov, "over");
             _message_over_send(cov, MOUSE_BUTTON_UP);

#ifdef ENABLE_RAPID_KEY_INPUT
             if (rapid_input && _keyboard_check(cov))
               ecore_x_e_illume_access_action_activate_send(target_win);
#endif
          }

        return;
     }

   dx = ev->root.x - cov->dx;
   dy = ev->root.y - cov->dy;
   if (((dx * dx) + (dy * dy)) < (distance * distance))
     {
        if (ev->double_click)
          {
             if (!ev->triple_click)
               {
#ifdef ENABLE_RAPID_KEY_INPUT
                  if (!rapid_input || !_keyboard_check(cov))
                    {
#endif
                       INFO(cov, "double_click");
                       int x, y;
                       ecore_x_pointer_xy_get(target_win, &x, &y);
                       _coordinate_calibrate(target_win, &x, &y);

                       ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                                     ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                     target_win,
                                                     ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ACTIVATE,
                                                     0, x, y);
#ifdef ENABLE_RAPID_KEY_INPUT
                    }
#endif
               }
          }
        else
          {
             cov->tap_timer = ecore_timer_add(double_tap_timeout,
                                         _mouse_tap, cov);
          }
     }
   else if (((dx * dx) + (dy * dy)) > (4 * distance * distance)
            && ((ev->timestamp - cov->dt) < (timeout * 2000)))
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
                     case 270:
                       ecore_x_e_illume_access_action_up_send(target_win);
                     break;

                     case 90:
                       ecore_x_e_illume_access_action_down_send(target_win);
                     break;

                     case 180:
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
                     case 270:
                       ecore_x_e_illume_access_action_down_send(target_win);
                     break;

                     case 90:
                       ecore_x_e_illume_access_action_up_send(target_win);
                     break;

                     case 180:
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
                       ecore_x_e_illume_access_action_read_prev_send
                                                        (target_win);
                     break;

                     case 270:
                       ecore_x_e_illume_access_action_read_next_send
                                                        (target_win);
                     break;

                     case 180:
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
                       ecore_x_e_illume_access_action_read_next_send
                                                        (target_win);
                     break;

                     case 270:
                       ecore_x_e_illume_access_action_read_prev_send
                                                        (target_win);
                     break;

                     case 180:
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

   _message_scroll_send(cov, MOUSE_MOVE);
   _message_zoom_send(cov, MOUSE_MOVE);
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

   EINA_LIST_FOREACH(covers, l, cov)
     {
        cov->x = ev->root.x;
        cov->y = ev->root.y;

        if (ev->multi.device == 0)
          cov->n_taps = 0;
        /* sometimes the mouse down event has improper multi.device value */
        cov->n_taps++;

        if (ev->window == cov->win)
          {
             //XXX change specific number
             if (ev->multi.device == 0)
               {
                  _mouse_down(cov, ev);
               }
             else if (cov->n_taps == 2 &&
                      !(cov->two_finger_down) &&
                      !(cov->longpressed))
               {
                  /* prevent longpress client message by two finger */
                  if (cov->timer)
                    {
                       ecore_timer_del(cov->timer);
                       cov->timer = NULL;
                    }

                  cov->two_finger_down = EINA_TRUE;

                  /* to check 2 finger mouse move */
                  cov->two_finger_move = eina_inarray_new(sizeof(Ecore_Event_Mouse_Move), 0);
                  _message_scroll_send(cov, MOUSE_BUTTON_DOWN);
                  _message_zoom_send(cov, MOUSE_BUTTON_DOWN);
               }

             else if (cov->n_taps == 3 &&
                      !(cov->three_finger_down) &&
                      !(cov->longpressed))
               {
                  cov->three_finger_down = EINA_TRUE;

                  if (cov->two_finger_down)
                    {
                       cov->two_finger_down = EINA_FALSE;

                       _message_scroll_send(cov, MOUSE_BUTTON_UP);
                       _message_zoom_send(cov, MOUSE_BUTTON_UP);

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
        cov->n_taps--;

        if (ev->window == cov->win)
          {
             /* the first finger: 1, from the second finger: 0 */
             if (ev->buttons == 1) _mouse_up(cov, ev);

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

   EINA_LIST_FOREACH(covers, l, cov)
     {
        cov->x = ev->root.x;
        cov->y = ev->root.y;

        if (ev->window == cov->win)
          {
             //if (ev->multi.device == multi_device[0] || ev->multi.device == multi_device[1])
             if (cov->two_finger_down && cov->n_taps == 2)
               _mouse_move(cov, ev);
             else if (cov->longpressed && /* client message for moving is available only after long press is detected */
                      !(cov->mouse_double_down) && /* mouse move after double down should not send read message */
                      !(cov->two_finger_down) && ev->multi.device == 0)
               {
                  INFO(cov, "over");
                  _message_over_send(cov, MOUSE_MOVE);
               }
             else if (cov->mouse_double_down && /* client message for moving is available only after long press is detected */
                      !(cov->two_finger_down) && ev->multi.device == 0)
               {
                  if (cov->longpressed)
                    {
                       /* send message to notify move after longpress */
                        _message_mouse_send(cov, MOUSE_MOVE);
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
                       cov->n_taps = 0;
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

   if (!g_enable) return ECORE_CALLBACK_PASS_ON;

   if (event->atom == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        bd = e_border_focused_get();
        if (bd)
          {
             _target_window_change(bd->client.win);
             _screen_reader_support_check();
          }
     }

   if (event->atom == ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL &&
       event->win == target_win)
     {
        _screen_reader_support_check();
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_border_show(void *data __UNUSED__,
               int   type __UNUSED__,
               void *event)
{
   E_Border *bd;
   const char *name = NULL;
   const char *clas = NULL;
   E_Event_Border_Show *ev = event;

   if (!g_enable) return ECORE_CALLBACK_PASS_ON;

   if (!ev) return ECORE_CALLBACK_PASS_ON;
   bd = ev->border;

   if (!bd) return ECORE_CALLBACK_PASS_ON;
   if (!bd->visible) return ECORE_CALLBACK_PASS_ON;

   _top_window_get(bd->zone);

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if (clas == NULL || name == NULL) return ECORE_CALLBACK_PASS_ON;

   if (strncmp(clas,"MINIAPP_TRAY",strlen("MINIAPP_TRAY"))!= 0) return ECORE_CALLBACK_PASS_ON;
   if (strncmp(name,"MINIAPP_TRAY",strlen("MINIAPP_TRAY"))!= 0) return ECORE_CALLBACK_PASS_ON;

   ecore_x_client_message32_send(bd->client.win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 bd->client.win,
                                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_DISABLE,
                                 0, 0, 0);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_border_move(void *data __UNUSED__,
               int   type __UNUSED__,
               void *event)
{
   E_Border *bd;
   E_Event_Border_Move *ev = event;

   if (!g_enable) return ECORE_CALLBACK_PASS_ON;

   bd = ev->border;
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   _top_window_get(bd->zone);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_border_hide(void *data __UNUSED__,
               int   type __UNUSED__,
               void *event)
{
   E_Border *bd;
   E_Event_Border_Hide *ev = event;

   if (!g_enable) return ECORE_CALLBACK_PASS_ON;

   bd = ev->border;
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   _top_window_get(bd->zone);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _cb_border_stack(void *data __UNUSED__,
                                  int type __UNUSED__,
                                  void *event)
{
   E_Event_Border_Stack *ev = event;
   E_Border *bd;

   if (!g_enable) return ECORE_CALLBACK_PASS_ON;

   bd = ev->border;
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   _top_window_get(bd->zone);

   return ECORE_CALLBACK_PASS_ON;
}



static void
_move_module_enable_set(int enable)
{
   E_Manager *man = e_manager_current_get();

   if (!man) ERR("cannot get current manager");

   if (enable)
     {
        INF("send enaable message");
        e_msg_send("screen-reader", "enable", 0, E_OBJECT(man), NULL, NULL, NULL);
     }
   else
     {
        INF("send disaable message");
        e_msg_send("screen-reader", "disable", 0, E_OBJECT(man), NULL, NULL, NULL);
     }
}

static void
_mini_application_close(void)
{
   ecore_x_client_message32_send(ecore_x_window_root_first_get(),
                                 E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL,
                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                 0, 0, 0, 0, 0);
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

        if (!target_win)
          {
             Eina_List *l;
             Cover *cov;
             EINA_LIST_FOREACH(covers, l, cov)
                _top_window_get(cov->zone);
          }

        ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      target_win,
                                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ENABLE,
                                      0, 0, 0);

        /* send a message to the move module */
        _move_module_enable_set(enable);

        /* send a message to close mini application */
        _mini_application_close();
     }
   else
     {
        INF("[screen reader module] module disable");
        _covers_shutdown();
        _events_shutdown();

        ecore_x_client_message32_send(target_win, ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      target_win,
                                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_DISABLE,
                                      0, 0, 0);

        /* send a message to the move module */
        _move_module_enable_set(enable);
     }

   elm_config_access_set(enable);
   elm_config_all_flush();
   elm_config_save();
}

#ifdef ENABLE_RAPID_KEY_INPUT
static void
_vconf_cb_accessibility_rapid_key_input_change(keynode_t *node,
                                               void      *data)
{
   if (!node) return;

   rapid_input = vconf_keynode_get_bool(node);
}
#endif
static void
atoms_init(void)
{
   E_MOD_SCREEN_READER_ATOM_CONTROL_PANEL_OPEN = ecore_x_atom_get("_E_MOD_SCREEN_READER_ACTION_CONTROL_PANEL_OPEN_");
   E_MOD_SCREEN_READER_ATOM_APP_TRAY_OPEN = ecore_x_atom_get("_E_MOD_SCREEN_READER_ACTION_APP_TRAY_OPEN_");
   E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL = ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL");
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
#ifdef ENABLE_RAPID_KEY_INPUT
   vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_RAPID_KEY_INPUT,
                            _vconf_cb_accessibility_rapid_key_input_change,
                            NULL);
#endif

   ecore_x_event_mask_set(ecore_x_window_root_first_get(),
                          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
   ecore_x_event_mask_set(ecore_x_window_root_first_get(),
                          ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
   property_handler = ecore_event_handler_add
                       (ECORE_X_EVENT_WINDOW_PROPERTY, _cb_property_change, NULL);
   border_show_handler = ecore_event_handler_add
                         (E_EVENT_BORDER_SHOW, _cb_border_show, NULL);
   border_move_handler = ecore_event_handler_add
                         (E_EVENT_BORDER_MOVE, _cb_border_move, NULL);
   border_hide_handler = ecore_event_handler_add
                         (E_EVENT_BORDER_HIDE, _cb_border_hide, NULL);
   border_stack_handler = ecore_event_handler_add
                         (E_EVENT_BORDER_STACK, _cb_border_stack, NULL);

   if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &g_enable) != 0)
     INF("vconf get failed\n");
#ifdef ENABLE_RAPID_KEY_INPUT
   if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_RAPID_KEY_INPUT, &rapid_input) != 0)
     INF("vconf get failed\n");
#endif

   atoms_init();

   if (g_enable)
     {
        _covers_shutdown();
        _covers_init();
        _events_shutdown();
        _events_init();

        /* send a message to the move module */
        _move_module_enable_set(g_enable);
     }
   else
     {
        _covers_shutdown();
        _events_shutdown();

        /* send a message to the move module */
        _move_module_enable_set(g_enable);
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
   if (border_show_handler) ecore_event_handler_del(border_show_handler);
   if (border_move_handler) ecore_event_handler_del(border_move_handler);
   if (border_hide_handler) ecore_event_handler_del(border_hide_handler);
   if (border_stack_handler) ecore_event_handler_del(border_stack_handler);

   _covers_shutdown();
   _events_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
