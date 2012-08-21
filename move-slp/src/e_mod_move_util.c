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

