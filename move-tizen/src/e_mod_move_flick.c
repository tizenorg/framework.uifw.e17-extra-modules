#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

struct _E_Move_Flick_Data
{
   int    sx; // start x
   int    sy; // start y
   int    mx1; // move x1 (previous x move position)
   int    my1; // move y1 (previous y move position)
   int    mx2; // move x2 (current x move position)
   int    my2; // move y2 (current y move position)
   int    ex; // end x
   int    ey; // end y
   double st; // start time
   double mt1; // move time (previous move time)
   double mt2; // move time (current move time)
   double et; // end time
};

/* local subsystem functions */

/* local subsystem functions */

/* externally accessible functions */

EINTERN E_Move_Flick_Data *
e_mod_move_flick_data_new(E_Move_Border *mb)
{
   E_Move_Flick_Data *flick;
   E_CHECK_RETURN(mb, 0);

   if (mb->flick_data) return mb->flick_data;
   
   flick = E_NEW(E_Move_Flick_Data, 1);
   E_CHECK_RETURN(flick, 0);

   mb->flick_data = flick;
   return flick;
}

EINTERN void
e_mod_move_flick_data_free(E_Move_Border *mb)
{
   E_CHECK(mb);
   E_CHECK(mb->flick_data);
   E_FREE(mb->flick_data);
   mb->flick_data = NULL;
}

EINTERN Eina_Bool
e_mod_move_flick_data_init(E_Move_Border *mb,
                           int            x,
                           int            y)
{
   E_Move_Flick_Data *flick;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);
   flick = mb->flick_data;
   flick->sx = x;
   flick->sy = y;
   flick->mx1 = x;
   flick->my1 = y;
   flick->mx2 = x;
   flick->my2 = y;
   flick->ex = x;
   flick->ey = y;
   flick->st = ecore_time_get();
   flick->mt1 = flick->st;
   flick->mt2 = flick->st;
   flick->et = flick->st;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_flick_data_update(E_Move_Border *mb,
                             int            x,
                             int            y)
{
   E_Move_Flick_Data *flick;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);
   flick = mb->flick_data;
   flick->ex = x;
   flick->ey = y;
   flick->et = ecore_time_get();
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_flick_data_move_pos_update(E_Move_Border *mb,
                                      int            x,
                                      int            y)
{
   E_Move_Flick_Data *flick;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);
   flick = mb->flick_data;
   flick->mx1 = flick->mx2;
   flick->my1 = flick->my2;
   flick->mt1 = flick->mt2;
   flick->mx2 = x;
   flick->my2 = y;
   flick->mt2 = ecore_time_get();
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_flick_state_get(E_Move_Border *mb,
                           Eina_Bool      direction_check)
{
   E_Move_Flick_Data *flick;
   double             flick_speed = 0.0; // todo speed configuration
   double             speed = 0.0;
   int                dx, dy;
   double             dt;
   double             tand = 0.0;
   double             flick_angle = 0.0;
   double             distance = 0.0;
   double             flick_distance = 0.0;
   Eina_Bool          state = EINA_FALSE;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);
   flick = mb->flick_data;
   flick_speed = mb->m->flick_limit.speed;
   flick_angle = mb->m->flick_limit.angle;
   flick_distance = mb->m->flick_limit.distance;

   dx = flick->ex - flick->sx;
   dy = flick->ey - flick->sy;
   dt = flick->et - flick->st;

   E_CHECK_RETURN(dt, EINA_FALSE);

   if (direction_check)
     {
        switch(mb->angle)
          {
           case  90:
           case 270:
              tand = 1.0 * abs(dx) / abs(dy);
              if (tand < flick_angle) return EINA_FALSE;
              break;
           case   0:
           case 180:
           default :
              tand = 1.0 * abs(dy) / abs(dx);
              if (tand < flick_angle) return EINA_FALSE;
              break;
          }
     }

   distance = sqrt((dx * dx) + (dy * dy));
   if (distance < flick_distance) return EINA_FALSE;

   speed = distance / dt;
   if (speed > flick_speed) state = EINA_TRUE;

   return state;
}

EINTERN Eina_Bool
e_mod_move_flick_state_get2(E_Move_Border *mb)
{
   E_Move_Flick_Data *flick;
   Eina_Bool          state = EINA_FALSE;
   E_Zone            *zone = NULL;
   double             flick_distance_rate = 0.0;
   double             check_distance = 0.0;
   int x = 0, y = 0;
   int zone_w = 0, zone_h = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);
   E_CHECK_RETURN(mb->bd->zone, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);

   flick = mb->flick_data;
   x = flick->ex;
   y = flick->ey;

   zone = mb->bd->zone;
   zone_w = zone->w;
   zone_h = zone->h;

   flick_distance_rate = mb->m->flick_limit.distance_rate;

   switch(mb->angle)
     {
      case  90:
         check_distance = zone_w * ( 1.0 - flick_distance_rate);
         if (check_distance > x ) state = EINA_TRUE;
         break;
      case 180:
         check_distance = zone_h * flick_distance_rate;
         if (check_distance < y ) state = EINA_TRUE;
         break;
      case 270:
         check_distance = zone_w * flick_distance_rate;
         if (check_distance < x ) state = EINA_TRUE;
         break;
      case   0:
      default :
         check_distance = zone_h * ( 1.0 - flick_distance_rate);
         if (check_distance > y ) state = EINA_TRUE;
         break;
     }

   return state;
}

EINTERN Eina_Bool
e_mod_move_flick_data_get(E_Move_Border *mb,
                          int           *sx,
                          int           *sy,
                          int           *ex,
                          int           *ey,
                          double        *st,
                          double        *et)
{
   E_Move_Flick_Data *flick;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);
   E_CHECK_RETURN(sx, EINA_FALSE);
   E_CHECK_RETURN(sy, EINA_FALSE);
   E_CHECK_RETURN(ex, EINA_FALSE);
   E_CHECK_RETURN(ey, EINA_FALSE);
   E_CHECK_RETURN(st, EINA_FALSE);
   E_CHECK_RETURN(et, EINA_FALSE);

   flick = mb->flick_data;

   *sx = flick->sx;
   *sy = flick->sy;
   *ex = flick->ex;
   *ey = flick->ey;
   *st = flick->st;
   *et = flick->et;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_flick_scroll_cancel_state_get(E_Move_Border         *mb,
                                         E_Move_Flick_Direction direction)
{
   E_Move_Flick_Data *flick;
   int                angle = 0;
   Eina_Bool          cancel_state = EINA_FALSE;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);

   flick = mb->flick_data;
   angle = mb->angle;

   switch (angle)
     {
      case 90:
         if (direction == E_MOVE_FLICK_DOWN)
           {
              if (flick->mx1 - flick->ex > 0)
                 cancel_state = EINA_TRUE;
           }
         else if (direction == E_MOVE_FLICK_UP)
           {
              if (flick->mx1 - flick->ex < 0)
                 cancel_state = EINA_TRUE;
           }
         break;
      case 180:
         if (direction == E_MOVE_FLICK_DOWN)
           {
              if (flick->my1 - flick->ey < 0)
                 cancel_state = EINA_TRUE;
           }
         else if (direction == E_MOVE_FLICK_UP)
           {
              if (flick->my1 - flick->ey > 0)
                 cancel_state = EINA_TRUE;
           }
         break;
      case 270:
         if (direction == E_MOVE_FLICK_DOWN)
           {
              if (flick->mx1 - flick->ex < 0)
                 cancel_state = EINA_TRUE;
           }
         else if (direction == E_MOVE_FLICK_UP)
           {
              if (flick->mx1 - flick->ex > 0)
                 cancel_state = EINA_TRUE;
           }
         break;
      case 0:
      default:
         if (direction == E_MOVE_FLICK_DOWN)
           {
              if (flick->my1 - flick->ey > 0)
                 cancel_state = EINA_TRUE;
           }
         else if (direction == E_MOVE_FLICK_UP)
           {
              if (flick->my1 - flick->ey < 0)
                 cancel_state = EINA_TRUE;
           }
         break;
     }

   return cancel_state;
}
