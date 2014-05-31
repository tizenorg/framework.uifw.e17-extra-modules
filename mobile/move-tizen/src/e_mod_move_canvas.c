#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* static global variables */
static int _canvas_num = 0;

/* externally accessible functions */
EINTERN E_Move_Canvas *
e_mod_move_canvas_add(E_Move *m,
                      E_Zone *zone)
{
   E_Move_Canvas *canvas;
   Eina_List *l;
   int x, y, w, h;
   E_CHECK_RETURN(m, 0);

   canvas = E_NEW(E_Move_Canvas, 1);
   E_CHECK_GOTO(canvas, error_cleanup);

   canvas->evas = e_manager_comp_evas_get(m->man);
   E_CHECK_GOTO(canvas->evas, error_cleanup);

   if (zone)
     {
        x = zone->x;
        y = zone->y;
        w = zone->w;
        h = zone->h;
     }
   else
     {
        x = 0;
        y = 0;
        w = m->man->w;
        h = m->man->h;
     }

   canvas->x = x;
   canvas->y = y;
   canvas->w = w;
   canvas->h = h;

   canvas->move = m;
   canvas->num = _canvas_num++;

   canvas->zone = zone;

   m->canvases = eina_list_append(m->canvases, canvas);

   return canvas;

error_cleanup:
   if (canvas)
     {
        memset(canvas, 0, sizeof(E_Move_Canvas));
        E_FREE(canvas);
        canvas = NULL;
     }

   EINA_LIST_FOREACH(m->canvases, l, canvas)
     {
        memset(canvas, 0, sizeof(E_Move_Canvas));
        E_FREE(canvas);
     }
   return NULL;
}

EINTERN void
e_mod_move_canvas_del(E_Move_Canvas *canvas)
{
   memset(canvas, 0, sizeof(E_Move_Canvas));
   E_FREE(canvas);
}
