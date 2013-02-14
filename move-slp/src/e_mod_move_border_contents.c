#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"

/* local subsystem functions */

/* externally accessible functions */
EINTERN E_Move_Border_Contents *
e_mod_move_border_contents_new(E_Move_Border *mb)
{
   E_Move_Border_Contents *contents;
   E_CHECK_RETURN(mb, 0);

   if (mb->contents) return mb->contents;
   
   contents = E_NEW(E_Move_Border_Contents, 1);
   E_CHECK_RETURN(contents, 0);

   mb->contents = contents;
   return contents;
}

EINTERN void
e_mod_move_border_contents_free(E_Move_Border *mb)
{
   E_CHECK(mb);
   E_CHECK(mb->contents);
   E_FREE(mb->contents);
   mb->contents = NULL;
}

EINTERN Eina_Bool
e_mod_move_border_contents_rect_set(E_Move_Border *mb,
                                    int            x,
                                    int            y,
                                    int            w,
                                    int            h)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->contents, EINA_FALSE);

   mb->contents->x = x; mb->contents->y = y;
   mb->contents->w = w; mb->contents->h = h;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_border_contents_rect_get(E_Move_Border *mb,
                                    int           *x,
                                    int           *y,
                                    int           *w,
                                    int           *h)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->contents, EINA_FALSE);
   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   E_CHECK_RETURN(w, EINA_FALSE);
   E_CHECK_RETURN(h, EINA_FALSE);

   *x = mb->contents->x; *y = mb->contents->y;
   *w = mb->contents->w; *h = mb->contents->h;

   return EINA_TRUE;
}
