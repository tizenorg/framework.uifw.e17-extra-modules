#include <utilX.h>
#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move_atoms.h"

/* local subsystem globals */
static E_Move *_m = NULL;

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
   if (mb->bd) return mb->bd->client.win;
   else return 0;
}

#define _WND_REQUEST_ANGLE_IDX 0
#define _WND_CURR_ANGLE_IDX    1

EINTERN Eina_Bool
e_mod_move_util_win_prop_angle_get(Ecore_X_Window win,
                                   int           *req,
                                   int           *curr)
{
   Eina_Bool res = EINA_FALSE;
   int ret, count;
   int angle[2] = {-1, -1};
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get(win,
                                          ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                          ECORE_X_ATOM_CARDINAL,
                                          32,
                                          &prop_data,
                                          &count);
   if (ret <= 0)
     {
        if (prop_data) free(prop_data);
        return res;
     }
   if (ret && prop_data)
     {
        memcpy (&angle, prop_data, sizeof (int)*count);
        if (count == 2) res = EINA_TRUE;
     }

   if (prop_data) free(prop_data);

   *req  = angle[_WND_REQUEST_ANGLE_IDX];
   *curr = angle[_WND_CURR_ANGLE_IDX];

   if (angle[0] == -1 && angle[1] == -1) res = EINA_FALSE;

   return res;
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
   Ecore_X_Display *d = ecore_x_display_get();
   int fb_x = 0, fb_y = 0;
   E_CHECK(d);

   switch (angle)
     {
      case   0: fb_x = 0;      fb_y = y + ch; break;
      case  90: fb_x = x + cw; fb_y = 0;      break;
      case 180: fb_x = 0;      fb_y = y - ch; break;
      case 270: fb_x = x - cw; fb_y = 0;      break;
      default :
         break;
     }

   utilx_set_video_offset(d, fb_x, fb_y);
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
       if (!temp_bd) continue;

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
