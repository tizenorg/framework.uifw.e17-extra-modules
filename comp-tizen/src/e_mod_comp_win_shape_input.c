#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include <pixman.h>

// input shape geometry (used on Move Module,
// Compositor receives input evevnt from this region.)
struct _E_Comp_Win_Shape_Input
{
   int x, y, w, h;
};

struct _E_Comp_Shape_Input
{
   int id;
   int x, y, w, h;
};

/* local subsystem functions */
static int
_e_mod_comp_shape_input_new_id_get(E_Comp *c)
{
   int new_id = 1;
   E_Comp_Shape_Input *shape_input = NULL;
   Eina_List *l = NULL;
   Eina_Bool create_id = EINA_FALSE;

   E_CHECK_RETURN(c, 0);

   while (new_id < INT_MAX)
     {
        create_id = EINA_TRUE;

        EINA_LIST_FOREACH(c->shape_inputs, l, shape_input)
          {
             if (shape_input->id == new_id)
               {
                  new_id++;
                  create_id = EINA_FALSE;
                  break;
               }
           }

        if (create_id) break;
     }

   if (create_id) return new_id;
   else return 0;
}

/* externally accessible functions */
EINTERN int
e_mod_comp_shape_input_new(E_Comp *c)
{
   int id = 0;
   E_Comp_Shape_Input *shape_input;
   E_CHECK_RETURN(c, 0);

   id = _e_mod_comp_shape_input_new_id_get(c);
   if (id)
     {
        shape_input = E_NEW(E_Comp_Shape_Input, 1);
        E_CHECK_RETURN(shape_input, 0);

        shape_input->id = id;
        c->shape_inputs = eina_list_append(c->shape_inputs, shape_input);
     }
   return id;
}

EINTERN Eina_Bool
e_mod_comp_shape_input_set(E_Comp *c,
                           int     id,
                           int     x,
                           int     y,
                           int     w,
                           int     h)
{
   Eina_Bool found = EINA_FALSE;
   E_Comp_Shape_Input *shape_input = NULL;
   Eina_List *l = NULL;

   E_CHECK_RETURN(c, EINA_FALSE);
   if ( id <= 0 ) return EINA_FALSE;

   EINA_LIST_FOREACH(c->shape_inputs, l, shape_input)
     {
        if (shape_input->id == id)
          {
             shape_input->x = x;
             shape_input->y = y;
             shape_input->w = w;
             shape_input->h = h;
             found = EINA_TRUE;
             break;
          }
     }

   return found;
}

EINTERN Eina_Bool
e_mod_comp_shape_input_del(E_Comp *c,
                           int     id)
{
   E_Comp_Shape_Input *shape_input = NULL;
   E_Comp_Shape_Input *find_shape_input = NULL;
   Eina_List *l = NULL;
   Eina_Bool found = EINA_FALSE;

   E_CHECK_RETURN(c, EINA_FALSE);
   if ( id <= 0 ) return EINA_FALSE;

   EINA_LIST_FOREACH(c->shape_inputs, l, shape_input)
     {
        if (shape_input->id == id)
          {
             find_shape_input = shape_input;
             found = EINA_TRUE;
             break;
          }
     }

   if (find_shape_input)
     {
        c->shape_inputs = eina_list_remove(c->shape_inputs, find_shape_input);
        memset(find_shape_input, 0, sizeof(E_Comp_Shape_Input));
        E_FREE(find_shape_input);
     }

   return found;
}

EINTERN Eina_Bool
e_mod_comp_win_shape_input_update(E_Comp *c)
{
   E_Comp_Win *_cw = NULL;
   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Shape_Input *shape_input = NULL;

   pixman_region32_t vis_part;
   pixman_region32_t win_part;
   pixman_region32_t cur_part;
   pixman_region32_t res_part;

   pixman_region32_t input_part;
   pixman_region32_t cur_input_part;
   pixman_region32_t res_input_part;
   pixman_region32_t sum_input_part;
   pixman_region32_t comp_input_part;

   pixman_box32_t *input_rects;

   Ecore_X_Rectangle *input_shape_rects = NULL;

   int num_rects = 0;
   int num_input_rects = 0;
   int i = 0;

   pixman_box32_t screen_rect;
   pixman_box32_t window_rect;
   pixman_box32_t input_rect;

   E_CHECK_RETURN(c, EINA_FALSE);
   E_CHECK_RETURN(c->need_shape_merge, EINA_FALSE);

   pixman_region32_init(&vis_part);
   pixman_region32_init(&win_part);
   pixman_region32_init(&cur_part);
   pixman_region32_init(&res_part);
   pixman_region32_init(&input_part);
   pixman_region32_init(&cur_input_part);
   pixman_region32_init(&res_input_part);
   pixman_region32_init(&sum_input_part);
   pixman_region32_init(&comp_input_part);

   screen_rect.x1 = 0;
   screen_rect.y1 = 0;
   screen_rect.x2 = c->man->w;
   screen_rect.y2 = c->man->h;

   pixman_region32_init_rects(&vis_part, &screen_rect, 1);
   pixman_region32_init_rect(&sum_input_part, 0, 0, 0, 0);

   EINA_INLIST_REVERSE_FOREACH(c->wins, _cw)
     {
        if (!_cw) continue;
        if ((!_cw->visible) || (_cw->defer_hide) ||
            (_cw->invalid)  ||
            (!E_INTERSECTS(0, 0, c->man->w, c->man->h,
                           _cw->x, _cw->y, _cw->w, _cw->h)))
          {
             continue;
          }
        if (TYPE_DEBUGGING_INFO_CHECK(_cw)) continue;

        num_rects = 0;

        window_rect.x1 = _cw->x;
        window_rect.y1 = _cw->y;
        window_rect.x2 = window_rect.x1 + _cw->w;
        window_rect.y2 = window_rect.y1 + _cw->h;

        if (_cw->shape_input)
          {
             input_rect.x1 = window_rect.x1 + _cw->shape_input->x;
             input_rect.y1 = window_rect.y1 + _cw->shape_input->y;
             input_rect.x2 = input_rect.x1 + _cw->shape_input->w;
             input_rect.y2 = input_rect.y1 + _cw->shape_input->h;
          }
        else
          {
             input_rect.x1 = window_rect.x1;
             input_rect.y1 = window_rect.y1;
             input_rect.x2 = input_rect.x1;
             input_rect.y2 = input_rect.y1;
          }

        pixman_region32_init_rects(&input_part, &input_rect, 1);
        pixman_region32_init_rect(&cur_input_part, 0, 0, 0, 0);
        pixman_region32_init_rect(&res_input_part, 0, 0, 0, 0);
        pixman_region32_init_rect(&cur_part,       0, 0, 0, 0);
        pixman_region32_init_rects(&win_part, &window_rect, 1);
        pixman_region32_init_rects(&res_part, &screen_rect, 1);

        // current_visible_window = window (intersect) screen visible part
        pixman_region32_intersect(&cur_part, &win_part, &vis_part);

        // current_input = current_visible_window (intersect) input
        pixman_region32_intersect(&cur_input_part, &cur_part, &input_part);

        // result_input = sum_input
        // sum_input = result_input + current_input
        pixman_region32_copy(&res_input_part, &sum_input_part);
        pixman_region32_union(&sum_input_part, &res_input_part, &cur_input_part);

        // result = visible - window
        pixman_region32_subtract(&res_part, &vis_part, &win_part);
        pixman_region32_rectangles(&res_part, &num_rects);

        if (num_rects == 0) break;

        pixman_region32_copy(&vis_part, &res_part);
     }

     EINA_LIST_FOREACH(c->shape_inputs, l, shape_input)
       {
          if (E_INTERSECTS(0, 0, c->man->w, c->man->h,
                           shape_input->x, shape_input->y,
                           shape_input->w, shape_input->h))
            {
               pixman_region32_init_rect(&comp_input_part,
                                         shape_input->x,
                                         shape_input->y,
                                         shape_input->w,
                                         shape_input->h);

               pixman_region32_copy(&res_input_part, &sum_input_part);
               pixman_region32_union(&sum_input_part, &res_input_part, &comp_input_part);
            }
       }

   input_rects = pixman_region32_rectangles(&sum_input_part, &num_input_rects);

   if (num_input_rects)
     {
        input_shape_rects = E_NEW(Ecore_X_Rectangle, num_input_rects);
        for (i = 0; i < num_input_rects; i++)
          {
             input_shape_rects[i].x = input_rects[i].x1;
             input_shape_rects[i].y = input_rects[i].y1;
             input_shape_rects[i].width = input_rects[i].x2 - input_rects[i].x1;
             input_shape_rects[i].height = input_rects[i].y2 - input_rects[i].y1;
          }

        if (_comp_mod->conf->nocomp_fs)
          {
             /* set shape before setting shape input */
             EINA_LIST_FOREACH(c->canvases, l, canvas)
               {
                  if ((canvas) &&
                      (canvas->nocomp.mode == E_NOCOMP_MODE_RUN))
                    {
                       ecore_x_window_shape_rectangles_set
                         (c->win, input_shape_rects, num_input_rects);
                       break;
                    }
               }

             /* for the xv */
             if ((input_shape_rects) && (num_input_rects == 1))
               {
                  canvas = (E_Comp_Canvas *)eina_list_nth(c->canvases, 0);
                  if ((canvas) &&
                      (input_shape_rects[0].x == canvas->x) &&
                      (input_shape_rects[0].y == canvas->y) &&
                      (input_shape_rects[0].width == canvas->w) &&
                      (input_shape_rects[0].height == canvas->h))
                    {
                       ecore_x_window_shape_rectangle_subtract(c->win,
                                                               (canvas->w / 2) - 16,
                                                               (canvas->h / 2) - 16,
                                                               16, 16);
                    }
               }
          }

        ecore_x_window_shape_input_rectangles_set(c->win,
                                                  input_shape_rects,
                                                  num_input_rects);
        if (input_shape_rects) E_FREE(input_shape_rects);
     }
   else
     {
        ecore_x_window_shape_input_rectangle_set(c->win, -1, -1, 1, 1);
     }

   pixman_region32_fini(&vis_part);
   pixman_region32_fini(&win_part);
   pixman_region32_fini(&cur_part);
   pixman_region32_fini(&res_part);
   pixman_region32_fini(&input_part);
   pixman_region32_fini(&cur_input_part);
   pixman_region32_fini(&res_input_part);
   pixman_region32_fini(&sum_input_part);
   pixman_region32_fini(&comp_input_part);

   c->need_shape_merge = EINA_FALSE;
   return EINA_TRUE;
}

EINTERN E_Comp_Win_Shape_Input *
e_mod_comp_win_shape_input_new(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;

   E_CHECK_RETURN(cw, 0);

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co->img) return NULL;
        if (!co->shadow) return NULL;
     }

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        evas_object_pass_events_set(co->img, EINA_FALSE);
        evas_object_pass_events_set(co->shadow, EINA_FALSE);
     }

   return E_NEW(E_Comp_Win_Shape_Input, 1);
}

EINTERN void
e_mod_comp_win_shape_input_free(E_Comp_Win_Shape_Input *input)
{
   E_CHECK(input);
   E_FREE(input);
}

EINTERN Eina_Bool
e_mod_comp_win_shape_input_rect_set(E_Comp_Win_Shape_Input *input,
                                    int                     x,
                                    int                     y,
                                    int                     w,
                                    int                     h)
{
   E_CHECK_RETURN(input, EINA_FALSE);

   if (((input->x == x) && (input->y == y) &&
        (input->w == w) && (input->h == h)))
     {
        return EINA_FALSE;
     }

   input->x = x;
   input->y = y;
   input->w = w;
   input->h = h;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_win_shape_input_invalid_set(E_Comp    *c,
                                       Eina_Bool  set)
{
   E_CHECK_RETURN(c, EINA_FALSE);
   c->need_shape_merge = set;
   return EINA_TRUE;
}
