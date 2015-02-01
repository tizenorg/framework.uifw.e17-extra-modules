#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move_atoms.h"

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

/* local subsystem globals */
static E_Move *_m = NULL;

static void
_e_mod_move_util_set_video_offset (int x, int y)
{
   Ecore_X_Atom property = None;
   char buf[32] = {0,};
   char *p = buf;
   int buf_len = 0;
   Display* dpy;
   Ecore_X_Window root = None;
   XRRScreenResources *res = NULL;
   RROutput rr_output = None;
   int i;

   p += sprintf (p, "%d,%d", x, y);

   *p = '\0';
   p++;

   buf_len = p - buf;

   property = ecore_x_atom_get ("XRR_PROPERTY_VIDEO_OFFSET");
   if (property == None)
   return;

   dpy = ecore_x_display_get ();
   root = ecore_x_window_root_first_get ();

   res = XRRGetScreenResources (dpy, root);
   if (res == NULL || res->noutput == 0)
     {
        fprintf (stderr, "Warning : ScreenResources is None.. %s (%d)\n", __func__, __LINE__);
        return;
     }

   for (i = 0; i < res->noutput; i++)
     {
        XRROutputInfo *output_info = XRRGetOutputInfo (dpy, res, res->outputs[i]);
        if (output_info)
          {
             rr_output = res->outputs[i];
             XRRFreeOutputInfo(output_info);
             break;
          }
     }

   if (rr_output == None)
     {
        fprintf (stderr, "Warning : output is None.. %s (%d)\n", __func__, __LINE__);
        XRRFreeScreenResources (res);
        return;
     }

   XRRChangeOutputProperty (dpy, rr_output, property,
                            XA_CARDINAL, 8, PropModeReplace,
                            (unsigned char*)buf, buf_len);
   XRRFreeScreenResources (res);
   XSync (dpy, 0);
}

/* externally accessible functions */
EINTERN void
e_mod_move_util_set(E_Move        *m,
                    E_Manager *man __UNUSED__)
{
   _m = m;
}

EINTERN E_Move *
e_mod_move_util_get(void)
{
   return _m;
}

EINTERN Eina_Bool
e_mod_move_util_border_visible_get(E_Move_Border *mb)
{
   return EINA_TRUE;
}

EINTERN Ecore_X_Window
e_mod_move_util_client_xid_get(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, 0);
   E_CHECK_RETURN(mb->bd, 0);

   if (e_object_is_del(E_OBJECT(mb->bd))) return 0;

   return mb->bd->client.win;
}

EINTERN Eina_Bool
e_mod_move_util_win_angle_get(Ecore_X_Window win,
                              int           *curr,
                              int           *prev)
{
   E_Border *bd = NULL;

   E_CHECK_RETURN(win, EINA_FALSE);
   E_CHECK_RETURN(curr, EINA_FALSE);
   E_CHECK_RETURN(prev, EINA_FALSE);

   bd = e_border_find_all_by_client_window(win);
   E_CHECK_RETURN(bd, EINA_FALSE);

   if (e_object_is_del(E_OBJECT(bd))) return EINA_FALSE;

   if (e_border_rotation_is_progress(bd))
     {
        *curr = e_border_rotation_next_angle_get(bd);
        *prev = e_border_rotation_curr_angle_get(bd); 
     }
   else
     {
        *curr = e_border_rotation_curr_angle_get(bd);
        *prev = e_border_rotation_prev_angle_get(bd); 
     }

   return EINA_TRUE;
}

EINTERN void
e_mod_move_util_border_hidden_set(E_Move_Border *mb,
                                  Eina_Bool      hidden)
{
   E_Move *m = NULL;
   E_Border *bd = NULL;
   E_Manager_Comp_Source *comp_src = NULL;
   Eina_Bool comp_hidden;

   E_CHECK(mb);
   m = mb->m;

   if (m->man->comp)
     {
        bd = e_border_find_all_by_client_window(mb->client_win);
        E_CHECK_GOTO(bd, error_cleanup);
        comp_src = e_manager_comp_src_get(m->man, bd->win);
        E_CHECK_GOTO(comp_src, error_cleanup);
        comp_hidden = e_manager_comp_src_hidden_get(m->man, comp_src);

        if (comp_hidden == hidden)
          return;
        else
          e_manager_comp_src_hidden_set(m->man, comp_src, hidden);
     }

error_cleanup:
   return;
}

EINTERN void
e_mod_move_util_rotation_lock(E_Move *m)
{
   unsigned int val = 1;
   E_Manager   *man = NULL;
   E_Zone      *zone = NULL;

   E_CHECK(m);

   man = m->man;
   E_CHECK(man);

   zone = e_util_zone_current_get(man);
   E_CHECK(zone);

   e_zone_rotation_block_set(zone, "move-tizen", EINA_TRUE);
   ecore_x_window_prop_card32_set(m->man->root, ATOM_ROTATION_LOCK, &val, 1);
}

EINTERN void
e_mod_move_util_rotation_unlock(E_Move *m)
{
   unsigned int val = 0;
   E_Manager   *man = NULL;
   E_Zone      *zone = NULL;

   E_CHECK(m);

   man = m->man;
   E_CHECK(man);

   zone = e_util_zone_current_get(man);
   E_CHECK(zone);

   e_zone_rotation_block_set(zone, "move-tizen", EINA_FALSE);
   ecore_x_window_prop_card32_set(m->man->root, ATOM_ROTATION_LOCK, &val, 1);
}

EINTERN Eina_Bool
e_mod_move_util_compositor_object_visible_get(E_Move_Border *mb)
{
   // get Evas_Object from Compositor
   E_Move *m = NULL;
   E_Border *bd = NULL;
   Evas_Object *comp_obj = NULL;
   E_Manager_Comp_Source *comp_src = NULL;
   Eina_Bool ret = EINA_FALSE;

   E_CHECK_RETURN(mb, EINA_FALSE);
   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);

   if (m->man->comp)
     {
        bd = e_border_find_all_by_client_window(mb->client_win);
        E_CHECK_RETURN(bd, EINA_FALSE);

        comp_src = e_manager_comp_src_get(m->man, bd->win);
        E_CHECK_RETURN(comp_src, EINA_FALSE);

        comp_obj = e_manager_comp_src_shadow_get(m->man, comp_src);
        E_CHECK_RETURN(comp_obj, EINA_FALSE);
        ret = evas_object_visible_get(comp_obj);
     }

   return ret;
}

EINTERN E_Move_Border *
e_mod_move_util_visible_fullscreen_window_find(void)
{
   E_Move        *m = NULL;
   E_Move_Border *mb = NULL;
   E_Move_Border *ret_mb = NULL;
   E_Zone        *zone = NULL;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        zone = mb->bd->zone;
        if ( (zone->x == mb->x)
             && (zone->y == mb->y)
             && (zone->w == mb->w)
             && (zone->h == mb->h))
          {
             if (mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE)
               ret_mb = mb;
             break;
          }
     }
   return ret_mb;
}

EINTERN void
e_mod_move_util_compositor_composite_mode_set(E_Move   *m,
                                              Eina_Bool set)
{
   E_Zone    *zone = NULL;
   E_Manager *man = NULL;
   E_CHECK(m);
   man = m->man;
   E_CHECK(man);
   E_CHECK(man->comp);

   zone = e_util_zone_current_get(man);
   E_CHECK(zone);
   e_manager_comp_composite_mode_set(man, zone, set);
}

EINTERN void
e_mod_move_util_fb_move(int angle,
                        int cw,
                        int ch,
                        int x,
                        int y)
{
   int fb_x = 0, fb_y = 0;

   switch (angle)
     {
      case   0: fb_x = 0;      fb_y = y + ch; break;
      case  90: fb_x = x + cw; fb_y = 0;      break;
      case 180: fb_x = 0;      fb_y = y - ch; break;
      case 270: fb_x = x - cw; fb_y = 0;      break;
      default :
         break;
     }

   _e_mod_move_util_set_video_offset (fb_x, fb_y);
}

EINTERN int
e_mod_move_util_layer_policy_get(E_Move_Layer_Policy layer)
{
   int ret = 100;
   switch (layer)
     {
      case E_MOVE_QUICKPANEL_LAYER:
      case E_MOVE_NOTIFICATION_LAYER:
      case E_MOVE_INDICATOR_LAYER:
           ret = 300; break;
      case E_MOVE_FULLSCREEN_LAYER:
           ret = 250; break;
      case E_MOVE_STATE_ABOVE_LAYER:
      case E_MOVE_ACTIVATE_LAYER:
      case E_MOVE_DIALOG_LAYER:
      case E_MOVE_SPLASH_LAYER:
      case E_MOVE_SOFTKEY_LAYER:
           ret = 150; break;
      case E_MOVE_CLIPBOARD_LAYER:
      case E_MOVE_KEYBOARD_LAYER:
      case E_MOVE_CONFORMANT_LAYER:
      case E_MOVE_APP_LAYER:
      case E_MOVE_HOME_LAYER:
           ret = 100; break;
      case E_MOVE_STATE_BELOW_LAYER:
           ret = 50; break;
      default:
           break;
     }
  return ret;
}

EINTERN E_Move_Scroll_Region_Indicator
e_mod_move_indicator_region_scroll_check(int        angle,
                                         Evas_Point input)
{
   E_Move *m = NULL;
   int w = 0, h = 0;
   int region_always = 0;
   int region_quickpanel = 0;
   int region_apptray = 0;
   E_Move_Border *indi_mb = NULL;
   E_Move_Scroll_Region_Indicator ret = E_MOVE_SCROLL_REGION_NONE;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, E_MOVE_SCROLL_REGION_NONE);

   if (!m->elm_indicator_mode) // Indicator Window mode
     {
        indi_mb = e_mod_move_indicator_find();
        E_CHECK_RETURN(indi_mb, E_MOVE_SCROLL_REGION_NONE);
        w = indi_mb->w;
        h = indi_mb->h;
     }

   switch (angle)
     {
      case  0:
         if (m->elm_indicator_mode)
           w = m->indicator_widget_geometry[E_MOVE_ANGLE_0].w;

         region_always = (m->indicator_always_region_ratio.portrait * w);
         region_quickpanel = (m->indicator_quickpanel_region_ratio.portrait * w);
         region_apptray = (m->indicator_apptray_region_ratio.portrait * w);

         if (input.x < region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.x < region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      case  90:
         if (m->elm_indicator_mode)
           h = m->indicator_widget_geometry[E_MOVE_ANGLE_90].h;

         region_always = (h - (m->indicator_always_region_ratio.landscape * h));
         region_quickpanel = (h - (m->indicator_quickpanel_region_ratio.landscape * h));
         region_apptray = (h - (m->indicator_apptray_region_ratio.landscape * h));

         if (input.y > region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.y > region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      case  180:
         if (m->elm_indicator_mode)
           w = m->indicator_widget_geometry[E_MOVE_ANGLE_180].w;

         region_always = (w - (m->indicator_always_region_ratio.portrait * w));
         region_quickpanel = (w - (m->indicator_quickpanel_region_ratio.portrait * w));
         region_apptray = (w - (m->indicator_apptray_region_ratio.portrait * w));

         if (input.x > region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.x > region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      case  270:
         if (m->elm_indicator_mode)
           h = m->indicator_widget_geometry[E_MOVE_ANGLE_270].h;

         region_always = (m->indicator_always_region_ratio.landscape * h);
         region_quickpanel = (m->indicator_quickpanel_region_ratio.landscape * h);
         region_apptray = (m->indicator_apptray_region_ratio.landscape * h);

         if (input.y < region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.y < region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      default:
         break;
     }
   return ret;
}

EINTERN Eina_Bool
e_mod_move_panel_scrollable_state_init(E_Move_Panel_Scrollable_State *panel_scrollable_state)
{
   E_CHECK_RETURN(panel_scrollable_state, EINA_FALSE);

   panel_scrollable_state->always = EINA_TRUE;
   panel_scrollable_state->quickpanel = EINA_TRUE;
   panel_scrollable_state->apptray = EINA_TRUE;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_panel_scrollable_state_get(Ecore_X_Window                 win,
                                      E_Move_Panel_Scrollable_State *panel_scrollable_state)
{
   unsigned int *vals = NULL;
   Eina_Bool res = EINA_FALSE;
   int num = 0;

   E_CHECK_RETURN(panel_scrollable_state, EINA_FALSE);
   E_CHECK_RETURN(win, EINA_FALSE);

   num = ecore_x_window_prop_card32_list_get
           (win, ATOM_MV_PANEL_SCROLLABLE_STATE, &vals);
   E_CHECK_GOTO((num == 3), cleanup); // currently, we uses only 3 panel type
   E_CHECK_GOTO(vals, cleanup);

   panel_scrollable_state->always = (vals[0] ? EINA_TRUE : EINA_FALSE);
   panel_scrollable_state->quickpanel = (vals[1] ? EINA_TRUE : EINA_FALSE);
   panel_scrollable_state->apptray = (vals[2] ? EINA_TRUE : EINA_FALSE);
   res = EINA_TRUE;

cleanup:

   if (vals) E_FREE(vals);
   return res;
}

EINTERN Eina_Bool
e_mod_move_panel_scrollable_get(E_Move_Border *mb, E_Move_Panel_Type type)
{
   Eina_Bool res = EINA_FALSE;

   E_CHECK_RETURN(mb, EINA_FALSE);

   switch (type)
     {
      case E_MOVE_PANEL_TYPE_ALWAYS:
        res = mb->panel_scrollable_state.always;
        break;
      case E_MOVE_PANEL_TYPE_QUICKPANEL:
        res = mb->panel_scrollable_state.quickpanel;
        break;
      case E_MOVE_PANEL_TYPE_APPTRAY:
        res = mb->panel_scrollable_state.apptray;
        break;
      case E_MOVE_PANEL_TYPE_NONE:
      default:
        break;
     }

   return res;
}

EINTERN E_Border*
e_mod_move_util_border_find_by_pointer(int x,
                                       int y)
{
   E_Border_List *bl = NULL;
   E_Border      *temp_bd = NULL;
   E_Border      *find_bd = NULL;
   E_Border      *focused_bd = NULL;

   focused_bd = e_border_focused_get();
   E_CHECK_RETURN(focused_bd, NULL);

   bl = e_container_border_list_last(focused_bd->zone->container);

   while ((temp_bd = e_container_border_list_prev(bl)))
     {
       if (!E_INSIDE(x, y, temp_bd->x, temp_bd->y, temp_bd->w, temp_bd->h))
         continue;
       if (!find_bd)
         {
            find_bd = temp_bd;
            break;
         }
     }
   e_container_border_list_free(bl);

   return find_bd;
}

EINTERN int
e_mod_move_util_root_angle_get(void)
{
   E_Move        *m = NULL;
   unsigned char *data = NULL;
   int            ret;
   int            cnt;
   int            angle = 0;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   ret = ecore_x_window_prop_property_get(m->man->root,
                                          ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                                          ECORE_X_ATOM_CARDINAL,
                                          32,
                                          &data,
                                          &cnt);
   if (ret && data) memcpy (&angle, data, sizeof(int));
   if (data) free (data);
   if (angle) angle %= 360;

   return angle;
}

EINTERN void
e_mod_move_mouse_event_send(Ecore_X_Window          id,
                            E_Move_Mouse_Event_Type type,
                            Evas_Point              pt)
{
   E_Border  *bd = NULL;
   Evas_Coord x = 0, y =0, w, h;
   int        button = 1;

   E_CHECK(id);

   bd = e_border_find_all_by_client_window(id);
   if (bd)
     {
        x = bd->x;
        y = bd->y;
        w = bd->w;
        h = bd->h;
     }
   else ecore_x_window_geometry_get(id, &x, &y, &w, &h);

   switch (type)
     {
      case E_MOVE_MOUSE_EVENT_DOWN:
         e_mod_move_util_mouse_down_send(id,
                                         ecore_x_current_time_get(),
                                         pt.x,
                                         pt.y,
                                         pt.x - x,
                                         pt.y - y,
                                         button);
         break;

      case E_MOVE_MOUSE_EVENT_UP:
         e_mod_move_util_mouse_up_send(id,
                                       ecore_x_current_time_get(),
                                       pt.x,
                                       pt.y,
                                       pt.x - x,
                                       pt.y - y,
                                       button);
         break;

      case E_MOVE_MOUSE_EVENT_MOVE:
         e_mod_move_util_mouse_move_send(id,
                                         ecore_x_current_time_get(),
                                         pt.x,
                                         pt.y,
                                         pt.x -x,
                                         pt.y -y);
         break;

      default:
         break;
    }
}

EINTERN Eina_Bool
e_mod_move_util_mouse_down_send(Ecore_X_Window id,
                                unsigned int timestamp,
                                int root_x,
                                int root_y,
                                int x,
                                int y,
                                int button)
{
   XEvent xev;
   Ecore_X_Window root;
   root = ecore_x_window_root_get(id);
   xev.xbutton.type = ButtonPress;
   xev.xbutton.window = id;
   xev.xbutton.root = root;
   xev.xbutton.subwindow = id;
   xev.xbutton.time = timestamp;
   xev.xbutton.x = x;
   xev.xbutton.y = y;
   xev.xbutton.x_root = root_x;
   xev.xbutton.y_root = root_y;
   xev.xbutton.state = 1 << button;
   xev.xbutton.button = button;
   xev.xbutton.same_screen = 1;
   return XSendEvent(ecore_x_display_get(), id, True, ButtonPressMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_move_util_mouse_up_send(Ecore_X_Window id,
                              unsigned int timestamp,
                              int root_x,
                              int root_y,
                              int x,
                              int y,
                              int button)
{
   XEvent xev;
   Ecore_X_Window root;

   root = ecore_x_window_root_get(id);
   xev.xbutton.type = ButtonRelease;
   xev.xbutton.window = id;
   xev.xbutton.root = root;
   xev.xbutton.subwindow = id;
   xev.xbutton.time = timestamp;
   xev.xbutton.x = x;
   xev.xbutton.y = y;
   xev.xbutton.x_root = root_x;
   xev.xbutton.y_root = root_y;
   xev.xbutton.state = 0;
   xev.xbutton.button = button;
   xev.xbutton.same_screen = 1;
   return XSendEvent(ecore_x_display_get(), id, True, ButtonReleaseMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_move_util_mouse_move_send(Ecore_X_Window id,
                                unsigned int timestamp,
                                int root_x,
                                int root_y,
                                int x,
                                int y)
{
   XEvent xev;
   Ecore_X_Window root;

   root = ecore_x_window_root_get(id);
   xev.xmotion.type = MotionNotify;
   xev.xmotion.window = id;
   xev.xmotion.root = root;
   xev.xmotion.subwindow = id;
   xev.xmotion.time = timestamp;
   xev.xmotion.x = x;
   xev.xmotion.y = y;
   xev.xmotion.x_root = root_x;
   xev.xmotion.y_root = root_y;
   xev.xmotion.state = 0;
   xev.xmotion.is_hint = 0;
   xev.xmotion.same_screen = 1;
   return XSendEvent(ecore_x_display_get(), id, True, PointerMotionMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_move_util_prop_indicator_cmd_win_get(Ecore_X_Window *win,
                                           E_Move         *m)
{
   int            ret = -1;
   Ecore_X_Window indi_cmd_win;

   E_CHECK_RETURN(win, EINA_FALSE);
   E_CHECK_RETURN(m, EINA_FALSE);

   ret = ecore_x_window_prop_window_get(m->man->root,
                                        ATOM_INDICATOR_CMD_WIN,
                                        &indi_cmd_win, 1);

   if (ret == -1) return EINA_FALSE;

   *win = indi_cmd_win;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_util_prop_indicator_cmd_win_set(Ecore_X_Window win,
                                           E_Move        *m)
{
   Ecore_X_Window indi_cmd_win;

   E_CHECK_RETURN(win, EINA_FALSE);
   E_CHECK_RETURN(m, EINA_FALSE);

   if (e_mod_move_util_prop_indicator_cmd_win_get(&indi_cmd_win, m))
     {
        if (indi_cmd_win != win )
          ecore_x_window_prop_window_set(m->man->root,
                                         ATOM_INDICATOR_CMD_WIN,
                                         &win, 1);
     }
   else
     {
        ecore_x_window_prop_window_set(m->man->root,
                                       ATOM_INDICATOR_CMD_WIN,
                                       &win, 1);
     }
   return EINA_TRUE;
}

EINTERN Evas_Object*
e_mod_move_util_comp_layer_get(E_Move     *m,
                               const char *name)
{
   E_Manager *man = NULL;
   E_Zone    *zone = NULL;

   E_CHECK_RETURN(m, NULL);
   E_CHECK_RETURN(name, NULL);

   man = m->man;
   E_CHECK_RETURN(man, NULL);

   zone = e_util_zone_current_get(man);
   E_CHECK_RETURN(zone, NULL);

   return e_manager_comp_layer_get(man, zone, name);
}

EINTERN Eina_Bool
e_mod_move_util_screen_input_block(E_Move *m)
{
   E_Zone    *zone = NULL;
   E_Manager *man = NULL;
   int        input_block_id = 0;
   Eina_Bool  ret = EINA_FALSE;

   E_CHECK_RETURN(m, EINA_FALSE);
   if (m->screen_input_block_id) return EINA_FALSE;

   man = m->man;
   E_CHECK_RETURN(man, EINA_FALSE);

   zone = e_util_zone_current_get(man);
   E_CHECK_RETURN(zone, EINA_FALSE);

   input_block_id = e_manager_comp_input_region_id_new(man);
   if (input_block_id)
     {
        e_manager_comp_input_region_id_set(man,
                                           input_block_id,
                                           zone->x, zone->y, zone->w, zone->h);
        m->screen_input_block_id = input_block_id;
        ret = EINA_TRUE;
     }

   return ret;
}

EINTERN Eina_Bool
e_mod_move_util_screen_input_unblock(E_Move *m)
{
   E_Manager *man = NULL;
   Eina_Bool  ret = EINA_FALSE;

   E_CHECK_RETURN(m, EINA_FALSE);

   man = m->man;
   E_CHECK_RETURN(man, EINA_FALSE);

   if (m->screen_input_block_id)
     {
        e_manager_comp_input_region_id_del(man, m->screen_input_block_id);
        m->screen_input_block_id = 0;
        ret = EINA_TRUE;
     }
   return ret;
}
