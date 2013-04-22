#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

struct _E_Move_Flick_Data
{
   int    sx; // start x
   int    sy; // start y
   int    ex; // end x
   int    ey; // end y
   double st; // start time
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
   flick->ex = x;
   flick->ey = y;
   flick->st = ecore_time_get();
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
e_mod_move_flick_state_get(E_Move_Border *mb,
                           Eina_Bool      direction_check)
{
   E_Move_Flick_Data *flick;
   double             flick_speed = 0.0; // todo speed configuration
   double             speed = 0.0;
   int                dx, dy;
   double             dt;
   Eina_Bool          state = EINA_FALSE;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->flick_data, EINA_FALSE);
   flick = mb->flick_data;
   flick_speed = mb->m->flick_speed_limit;

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
              if ((abs(dy)*3) > abs(dx)) return EINA_FALSE;
              break;
           case   0:
           case 180:
           default :
              if ((abs(dx)*3) > abs(dy)) return EINA_FALSE;
              break;
          }
     }

   speed = sqrt((dx * dx) + (dy * dy)) / dt;

   if (speed > flick_speed) state = EINA_TRUE;

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
