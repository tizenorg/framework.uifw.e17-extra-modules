#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp.h"

#define _F_BORDER_CLIP_TO_ZONE_ 1

/* externally accessible functions */
EINTERN E_Comp_Object *
e_mod_comp_obj_add(E_Comp_Win    *cw,
                   E_Comp_Canvas *canvas)
{
   E_Comp_Object *co;
   co = E_NEW(E_Comp_Object, 1);
   E_CHECK_RETURN(co, 0);
   if ((!cw->input_only) && (!cw->invalid))
     {
        co->shadow = edje_object_add(canvas->evas);
        co->img = evas_object_image_filled_add(canvas->evas);
        evas_object_image_colorspace_set(co->img, EVAS_COLORSPACE_ARGB8888);
        if (cw->argb) evas_object_image_alpha_set(co->img, 1);
        else evas_object_image_alpha_set(co->img, 0);
     }
   else
     {
        co->shadow = evas_object_rectangle_add(canvas->evas);
        evas_object_color_set(co->shadow, 0, 0, 0, 0);
     }

   co->canvas = canvas;
   co->zone = canvas->zone;
   return co;
}

EINTERN void
e_mod_comp_obj_del(E_Comp_Object *co)
{
   if (co->img_mirror)
     {
        Evas_Object *o;
        EINA_LIST_FREE(co->img_mirror, o)
          {
             if (co->xim) evas_object_image_data_set(o, NULL);
             evas_object_event_callback_del(o, EVAS_CALLBACK_DEL,
                                            e_mod_comp_cb_win_mirror_del);
             evas_object_del(o);
          }
     }
   if (co->xim)
     {
        evas_object_image_data_set(co->img, NULL);
        ecore_x_image_free(co->xim);
        co->xim = NULL;
     }
   if (co->clipper)
     {
        evas_object_del(co->clipper);
        co->clipper = NULL;
     }
   if (co->img)
     {
        evas_object_del(co->img);
        co->img = NULL;
     }
   if (co->shadow)
     {
        evas_object_del(co->shadow);
        co->shadow = NULL;
     }
   E_FREE(co);
}

EINTERN Eina_List *
e_mod_comp_win_comp_objs_add(E_Comp_Win *cw)
{
   Eina_List *l, *objs = NULL;
   E_Comp_Canvas *canvas;
   E_Comp_Object *co;

   EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
     {
        co = e_mod_comp_obj_add(cw, canvas);
        if (!co)
          {
             e_mod_comp_win_comp_objs_del(cw, objs);
             return NULL;
          }
        objs = eina_list_append(objs, co);
     }
   return objs;
}

EINTERN void
e_mod_comp_win_comp_objs_del(E_Comp_Win *cw,
                             Eina_List  *objs)
{
   E_Comp_Object *co;
   EINA_LIST_FREE(objs, co) e_mod_comp_obj_del(co);
}

EINTERN void
e_mod_comp_win_comp_objs_move(E_Comp_Win *cw,
                              int         x,
                              int         y)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        int zx = 0, zy = 0;
        if (!co) continue;
        if (!co->shadow) continue;
        if (co->zone)
          {
             zx = co->zone->x;
             zy = co->zone->y;
          }
        evas_object_move(co->shadow, x - zx, y - zy);

#ifdef _F_BORDER_CLIP_TO_ZONE_
        if ((cw->visible) && (cw->bd) && (co->zone) &&
            (!cw->input_only) && (!cw->invalid) &&
            E_INTERSECTS(co->zone->x, co->zone->y,
                         co->zone->w, co->zone->h,
                         cw->x, cw->y, cw->w, cw->h))
          {
             if (!(E_CONTAINS(co->zone->x, co->zone->y,
                              co->zone->w, co->zone->h,
                              cw->x, cw->y, cw->w, cw->h)))
               {
                  int _x, _y, _w, _h;
                  _x = x - zx; _y = y - zy;
                  _w = cw->pw; _h = cw->ph;

                  E_RECTS_CLIP_TO_RECT(_x, _y, _w, _h,
                                       0, 0, co->zone->w, co->zone->h);
                  if (!co->clipper)
                    {
                       co->clipper = evas_object_rectangle_add(co->canvas->evas);
                       evas_object_clip_set(co->shadow, co->clipper);
                       /* check to see if a given object exists on the zone which is
                        * equal to its border. if it is, the clipper will be shown.
                        * otherwise, the clipper will be invisible.
                        */
                       if (cw->bd->zone == co->zone)
                         evas_object_show(co->clipper);
                    }
                  evas_object_move(co->clipper, _x, _y);
                  evas_object_resize(co->clipper, _w, _h);
               }
             else
               {
                  if (co->clipper)
                    {
                       evas_object_hide(co->clipper);
                       evas_object_clip_unset(co->shadow);
                       evas_object_del(co->clipper);
                       co->clipper = NULL;
                    }
               }
          }
#endif
     }
}

EINTERN void
e_mod_comp_win_comp_objs_resize(E_Comp_Win *cw,
                                int         w,
                                int         h)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue;
        evas_object_resize(co->shadow, w, h);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_resize(E_Comp_Win *cw,
                                    int         w,
                                    int         h)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->img) continue;
        evas_object_resize(co->img, w, h);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_init(E_Comp_Win *cw)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (co->native) continue;
        Evas_Native_Surface ns;
        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_X11;
        ns.data.x11.visual = cw->vis;
        ns.data.x11.pixmap = cw->pixmap;
        evas_object_image_native_surface_set(co->img, &ns);
        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_native_surface_set(o, &ns);
          }
        evas_object_image_data_update_add(co->img, 0, 0, cw->pw, cw->ph);
        co->native = 1;
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_deinit(E_Comp_Win *cw)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->native) continue;
        evas_object_image_native_surface_set(co->img, NULL);
        co->native = 0;
        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             if (!o) continue;
             evas_object_image_native_surface_set(o, NULL);
          }
     }
}

EINTERN void
e_mod_comp_win_comp_objs_xim_free(E_Comp_Win *cw)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->xim) continue;
        evas_object_image_size_set(co->img, 1, 1);
        evas_object_image_data_set(co->img, NULL);
        ecore_x_image_free(co->xim);
        co->xim = NULL;
        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_size_set(o, 1, 1);
             evas_object_image_data_set(o, NULL);
          }
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_pass_events_set(E_Comp_Win *cw,
                                             Eina_Bool   set)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->img) continue;
        evas_object_pass_events_set(co->img, set);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_pass_events_set(E_Comp_Win *cw,
                                         Eina_Bool   set)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue;
        evas_object_pass_events_set(co->shadow, set);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_alpha_set(E_Comp_Win *cw,
                                       Eina_Bool   alpha)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->img) continue;
        evas_object_image_alpha_set(co->img, alpha);
        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_alpha_set(o, alpha);
          }
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_size_set(E_Comp_Win *cw,
                                      int         w,
                                      int         h)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->img) continue;
        evas_object_image_size_set(co->img, w, h);
        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_size_set(o, w, h);
          }
     }
}

EINTERN void
e_mod_comp_win_comp_objs_img_data_update_add(E_Comp_Win *cw,
                                             int         x,
                                             int         y,
                                             int         w,
                                             int         h)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->img) continue;

        /* viewport culling:
         * Do nothing if an object is not located on E_Zone.
         * A given object should not be marked if it is located on
         * outer viewport. The evas doesn't perform viewport culling
         * correctly.
         */
        if ((cw->visible) &&
            !((cw->input_only) && (cw->invalid)) &&
            E_INTERSECTS(co->zone->x, co->zone->y,
                         co->zone->w, co->zone->h,
                         cw->x, cw->y, cw->w, cw->h))
          {
             evas_object_image_data_update_add(co->img, x, y, w, h);
          }

        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_data_update_add(o, x, y, w, h);
          }
     }
}

EINTERN void
e_mod_comp_win_comp_objs_needxim_set(E_Comp_Win *cw,
                                     Eina_Bool   need)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (need && co->xim) co->needxim = EINA_TRUE;
     }
}

EINTERN void
e_mod_comp_win_comp_objs_native_set(E_Comp_Win *cw,
                                    Eina_Bool   native)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        co->native = native;
     }
}

EINTERN void
e_mod_comp_win_comp_objs_data_del(E_Comp_Win *cw,
                                  const char *key)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        evas_object_data_del(co->shadow, key);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_data_set(E_Comp_Win *cw,
                                  const char *key,
                                  const void *data)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue;
        evas_object_data_set(co->shadow, key, data);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_show(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   Eina_Bool eff = EINA_FALSE;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if ((!cw->update) &&
            (cw->visible) &&
            (cw->dmg_updates >= 1))
          {
             if (!evas_object_visible_get(co->shadow) &&
                 !(cw->animate_hide))
               {
                  if (!cw->hidden_override)
                    evas_object_show(co->shadow);
                  else
                    e_mod_comp_bg_win_handler_update(cw);

                  if (!cw->hidden_override)
                    eff = EINA_TRUE;
               }
          }
     }

   if (eff) e_mod_comp_effect_win_show(cw);
}

/**
 * It is possible that the WM can receive a border show event immediatly
 * after getting a border hide event such as indicator hide/show.
 * In that case, if we use e_mod_comp_win_objs_show() function then we
 * don't append showing EDJE signal because a shadow object of given window
 * is still visible state. The hide_done callback can make object to hide.
 * Thus the WM has to append showing EDJE signal into the job queue
 * to show a shadow object of given window.
 */
EINTERN void
e_mod_comp_win_comp_objs_force_show(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   Eina_Bool eff = EINA_FALSE;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!cw->animate_hide)
          {
             if (!cw->hidden_override)
               {
                  if (!evas_object_visible_get(co->shadow))
                    evas_object_show(co->shadow);
                  eff = EINA_TRUE;
               }
             else
               e_mod_comp_bg_win_handler_update(cw);
          }
     }

   if (eff) e_mod_comp_effect_win_show(cw);
}

EINTERN void
e_mod_comp_win_comp_objs_hide(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue;
        evas_object_hide(co->shadow);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_raise(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        evas_object_raise(co->shadow);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_lower(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        evas_object_lower(co->shadow);
     }
}

EINTERN void
e_mod_comp_win_comp_objs_stack_above(E_Comp_Win *cw,
                                     E_Comp_Win *cw2)
{
   Eina_List *l, *ll;
   E_Comp_Object *co, *co2;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        EINA_LIST_FOREACH(cw2->objs, ll, co2)
          {
             if (co->zone == co2->zone)
               {
                  evas_object_stack_above(co->shadow,
                                          co2->shadow);
               }
          }
     }
}
