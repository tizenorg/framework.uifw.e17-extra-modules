#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include <pixman.h>


/* local subsystem functions */

/* externally accessible functions */
EINTERN E_Move_Border_Shape_Input *
e_mod_move_border_shape_input_new(E_Move_Border *mb)
{
   E_Move_Border_Shape_Input *input;
   E_CHECK_RETURN(mb, 0);

   if (mb->shape_input) return mb->shape_input;
   
   input = E_NEW(E_Move_Border_Shape_Input, 1);
   E_CHECK_RETURN(input, 0);

   mb->shape_input = input;
   return input;
}

EINTERN void
e_mod_move_border_shape_input_free(E_Move_Border *mb)
{
   E_CHECK(mb);
   E_CHECK(mb->shape_input);
   E_FREE(mb->shape_input);
   mb->shape_input = NULL;
}

EINTERN Eina_Bool
e_mod_move_border_shape_input_rect_set(E_Move_Border *mb,
                                       int            x,
                                       int            y,
                                       int            w,
                                       int            h)
{
   E_Manager_Comp_Source *comp_src = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->shape_input, EINA_FALSE);

   E_CHECK_RETURN(mb->m, EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);

   comp_src = e_manager_comp_src_get(mb->m->man, mb->bd->win);
   E_CHECK_RETURN(comp_src, EINA_FALSE);

   mb->shape_input->x = x; mb->shape_input->y = y;
   mb->shape_input->w = w; mb->shape_input->h = h;

   return e_manager_comp_src_input_region_set(mb->m->man, comp_src, x, y, w, h);
}

EINTERN Eina_Bool
e_mod_move_border_shape_input_rect_get(E_Move_Border *mb,
                                       int           *x,
                                       int           *y,
                                       int           *w,
                                       int           *h)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->shape_input, EINA_FALSE);
   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   E_CHECK_RETURN(w, EINA_FALSE);
   E_CHECK_RETURN(h, EINA_FALSE);

   *x = mb->shape_input->x; *y = mb->shape_input->y;
   *w = mb->shape_input->w; *h = mb->shape_input->h;

   return EINA_TRUE;
}
