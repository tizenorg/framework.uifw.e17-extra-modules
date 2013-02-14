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
   E_CHECK(m);
   ecore_x_window_prop_card32_set(m->man->root, ATOM_ROTATION_LOCK, &val, 1);
}

EINTERN void
e_mod_move_util_rotation_unlock(E_Move *m)
{
   unsigned int val = 0;
   E_CHECK(m);
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
           w = m->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].w;

         region_always = (m->indicator_always_region_ratio.portrait * w);
         region_quickpanel = (m->indicator_quickpanel_region_ratio.portrait * w);
         region_apptray = (m->indicator_apptray_region_ratio.portrait * w);

         if (input.x < region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.x < region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      case  90:
         if (m->elm_indicator_mode)
           h = m->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].h;

         region_always = (h - (m->indicator_always_region_ratio.landscape * h));
         region_quickpanel = (h - (m->indicator_quickpanel_region_ratio.landscape * h));
         region_apptray = (h - (m->indicator_apptray_region_ratio.landscape * h));

         if (input.y > region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.y > region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      case  180:
         if (m->elm_indicator_mode)
           w = m->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].w;

         region_always = (w - (m->indicator_always_region_ratio.portrait * w));
         region_quickpanel = (w - (m->indicator_quickpanel_region_ratio.portrait * w));
         region_apptray = (w - (m->indicator_apptray_region_ratio.portrait * w));

         if (input.x > region_always) ret = E_MOVE_SCROLL_REGION_NONE; // Always scroll region
         else if ( input.x > region_quickpanel ) ret = E_MOVE_SCROLL_REGION_QUICKPANEL;
         else  ret = E_MOVE_SCROLL_REGION_NONE; // Apptray scroll region
         break;
      case  270:
         if (m->elm_indicator_mode)
           h = m->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].h;

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
