#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp.h"

#ifdef _F_BORDER_CLIP_TO_ZONE_
#undef _F_BORDER_CLIP_TO_ZONE_
#endif
#define _F_BORDER_CLIP_TO_ZONE_ 0

/* externally accessible functions */
EAPI E_Comp_Object *
e_mod_comp_obj_add(E_Comp_Win    *cw,
                   E_Comp_Canvas *canvas)
{
   E_Comp_Object *co;
   co = E_NEW(E_Comp_Object, 1);
   E_CHECK_RETURN(co, 0);
   if ((!cw->input_only) && (!cw->invalid))
     {
        if (_comp_mod->conf->use_hwc)
          {
             co->hwc.mask_rect = evas_object_rectangle_add(canvas->evas);
#if DEBUG_HWC_COLOR
             evas_object_color_set(co->hwc.mask_rect, 50, 0, 0, 50);
#else
             evas_object_color_set(co->hwc.mask_rect, 0, 0, 0, 0);
#endif
             evas_object_render_op_set(co->hwc.mask_rect, EVAS_RENDER_COPY);
             evas_object_hide(co->hwc.mask_rect);
          }
        co->shadow = edje_object_add(canvas->evas);
        co->img = evas_object_image_filled_add(canvas->evas);

        if ((!co->shadow) || (!co->img))
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|ERROR shadow:%p img:%p evas:%p argb:%d canvas_num:%d",
                  "OBJECT_CREATE", co->shadow, co->img,
                  canvas->evas, cw->argb, canvas->num);
          }

        E_Comp_Layer *ly = e_mod_comp_canvas_layer_get(canvas, "comp");
        if (ly)
          {
             e_mod_comp_layer_populate(ly, co->shadow);
             if (_comp_mod->conf->use_hwc)
               {
                  e_mod_comp_layer_populate(ly, co->hwc.mask_rect);
               }

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|OK!! layer shadow:%p img:%p evas:%p argb:%d canvas_num:%d ly:%p",
                  "OBJECT_CREATE", co->shadow, co->img,
                  canvas->evas, cw->argb, canvas->num, ly);
          }
        else
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|ERROR shadow:%p img:%p evas:%p argb:%d canvas_num:%d ly:%p",
                  "OBJECT_CREATE", co->shadow, co->img,
                  canvas->evas, cw->argb, canvas->num, ly);
          }

        evas_object_image_colorspace_set(co->img, EVAS_COLORSPACE_ARGB8888);
        if (cw->argb) evas_object_image_alpha_set(co->img, 1);
        else evas_object_image_alpha_set(co->img, 0);
     }
   else
     {
        co->shadow = evas_object_rectangle_add(canvas->evas);

        if (!co->shadow)
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|ERROR shadow:%p evas:%p canvas_num:%d",
                  "OBJECT_CREATE", co->shadow, canvas->evas,
                  canvas->num);
          }
        else
          {
             E_Comp_Layer *ly = e_mod_comp_canvas_layer_get(canvas, "comp");
             if (ly)
               e_mod_comp_layer_populate(ly, co->shadow);
          }
        evas_object_color_set(co->shadow, 0, 0, 0, 0);
     }

   co->canvas = canvas;
   co->zone = canvas->zone;
   return co;
}

EAPI void
e_mod_comp_obj_del(E_Comp_Object *co)
{
   E_Comp_Layer *ly = e_mod_comp_canvas_layer_get(co->canvas, "comp");
   if (ly) e_layout_unpack(co->shadow);

   if (_comp_mod->conf->use_hwc)
     {
        if (co->hwc.mask_rect)
          {
             evas_object_del(co->hwc.mask_rect);
             co->hwc.mask_rect = NULL;
          }
      }

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
   if (co->transp.offset)
     {
        evas_object_del(co->transp.offset);
        co->transp.offset = NULL;
     }
   if (co->transp.rect)
     {
        evas_object_del(co->transp.rect);
        co->transp.rect = NULL;
     }
   if (co->shadow)
     {
        evas_object_del(co->shadow);
        co->shadow = NULL;
     }
   E_FREE(co);
}

EAPI Eina_List *
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

EAPI void
e_mod_comp_win_comp_objs_del(E_Comp_Win *cw,
                             Eina_List  *objs)
{
   E_Comp_Object *co;
   EINA_LIST_FREE(objs, co) e_mod_comp_obj_del(co);
}

EAPI void
e_mod_comp_win_comp_objs_move(E_Comp_Win *cw,
                              int         x,
                              int         y)
{
   Eina_List *l;
   E_Comp_Object *co;
   E_Comp_Layer *ly;

   E_Comp_Canvas *canvas, *ly_canvas = NULL;
   E_Comp_Layer *eff_ly;
   E_Comp_Effect_Object *eff_obj = NULL;
   Eina_Bool eff_run = EINA_FALSE;
   canvas = eina_list_nth(cw->c->canvases, 0);
   eff_ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   eff_run = e_mod_comp_layer_effect_get(eff_ly);
   if (eff_run)
     {
        /* do not change position of the mini-mode window effect object
         * during effect to avoid unnecessary move around screen.
         * TODO: Move INSET check code to effect.c file.
         */
        if (!STATE_INSET_CHECK(cw))
          {
             eff_obj = e_mod_comp_layer_effect_obj_get(eff_ly, cw->win);
             if (eff_obj) ly_canvas = eff_ly->canvas;
          }
     }

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

        ly = e_mod_comp_canvas_layer_get(co->canvas, "comp");
        if (ly)
          {
          e_layout_child_move(co->shadow, x - zx, y - zy);
             if (_comp_mod->conf->use_hwc)
               {
                  e_layout_child_move(co->hwc.mask_rect, x - zx, y - zy);
               }
          }
        else
          {
          evas_object_move(co->shadow, x - zx, y - zy);
             if (_comp_mod->conf->use_hwc)
               {
                  evas_object_move(co->hwc.mask_rect, x - zx, y - zy);
               }
          }
#if MOVE_IN_EFFECT
        /* to show moving window while effect, we should also update effect object */
        if (eff_obj)
          {
             if (ly_canvas == co->canvas)
               e_layout_child_move(eff_obj->edje, x -zx, y - zy);
          }
#else
        if ((eff_obj) && (TYPE_QUICKPANEL_CHECK(cw)))
          {
             if (ly_canvas == co->canvas)
               e_layout_child_move(eff_obj->edje, x -zx, y - zy);
          }
#endif

#if _F_BORDER_CLIP_TO_ZONE_
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
                  if (ly)
                    {
                       e_layout_child_move(co->clipper, _x, _y);
                       e_layout_child_resize(co->clipper, _w, _h);
                    }
                  else
                    {
                       evas_object_move(co->clipper, _x, _y);
                       evas_object_resize(co->clipper, _w, _h);
                    }
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

EAPI void
e_mod_comp_win_comp_objs_resize(E_Comp_Win *cw,
                                int         w,
                                int         h)
{
   Eina_List *l;
   E_Comp_Object *co;
   E_Comp_Layer *ly;

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue;
        ly = e_mod_comp_canvas_layer_get(co->canvas, "comp");
        if (ly)
          {
          e_layout_child_resize(co->shadow, w, h);
             if (_comp_mod->conf->use_hwc)
               {
                  e_layout_child_resize(co->hwc.mask_rect, w, h);
               }
          }
        else
          {
          evas_object_resize(co->shadow, w, h);
             if (_comp_mod->conf->use_hwc)
               {
                  evas_object_resize(co->hwc.mask_rect, w, h);
               }
          }
     }
}

EAPI void
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
        evas_object_image_size_set(co->img, w, h);
     }
}

EAPI void
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
        if (!cw->argb)
          evas_object_render_op_set(co->img, EVAS_RENDER_COPY);
        evas_object_image_native_surface_set(co->img, &ns);
        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_native_surface_set(o, &ns);
          }
        evas_object_image_data_update_add(co->img, 0, 0, cw->pw, cw->ph);
        co->native = 1;
     }
}

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
e_mod_comp_win_comp_objs_img_data_update_add(E_Comp_Win *cw,
                                             int         x,
                                             int         y,
                                             int         w,
                                             int         h)
{
   Eina_List *l, *ll;
   E_Comp_Object *co;
   Evas_Object *o;

   E_Comp_Canvas *canvas, *ly_canvas = NULL;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *eff_obj = NULL;
   Eina_Bool eff_run = EINA_FALSE;
   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   eff_run = e_mod_comp_layer_effect_get(ly);
   if (eff_run)
     {
        eff_obj = e_mod_comp_layer_effect_obj_get(ly, cw->win);
        if (eff_obj) ly_canvas = ly->canvas;
     }

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

             /* to show damaged window while effect, we should also update effect object */
             if (eff_obj)
               {
                  if (ly_canvas == co->canvas)
                    evas_object_image_data_update_add(eff_obj->img, x, y, w, h);
               }
          }

        EINA_LIST_FOREACH(co->img_mirror, ll, o)
          {
             evas_object_image_data_update_add(o, x, y, w, h);
          }
     }
}

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
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

EAPI void
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
                    {
                       evas_object_show(co->shadow);

                       if (_comp_mod->conf->use_hwc)
                         {
                            cw->hwc.set_drawable = EINA_TRUE;
                         }
                    }
                  else
                    e_mod_comp_bg_win_handler_update(cw);

                  if (!cw->hidden_override)
                    eff = EINA_TRUE;
               }
          }
     }

   if (eff) cw->c->effect_funcs.win_show(cw);
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
EAPI void
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
                    {
                       evas_object_show(co->shadow);

                       if (_comp_mod->conf->use_hwc)
                         {
                            cw->hwc.set_drawable = EINA_TRUE;
                         }
                    }
                  eff = EINA_TRUE;
               }
             else
               e_mod_comp_bg_win_handler_update(cw);
          }
     }

   if (eff) cw->c->effect_funcs.win_show(cw);
}

EAPI void
e_mod_comp_win_comp_objs_hide(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue;
        evas_object_hide(co->shadow);

        if (_comp_mod->conf->use_hwc)
          {
             evas_object_hide(co->hwc.mask_rect);
             cw->hwc.set_drawable = EINA_FALSE;
          }
     }
}

EAPI void
e_mod_comp_win_comp_objs_raise(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   E_Comp_Layer *ly;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        e_layout_child_raise(co->shadow);
        if (_comp_mod->conf->use_hwc)
          {
             e_layout_child_raise_above(co->hwc.mask_rect, co->shadow);
          }
        ly = e_mod_comp_canvas_layer_get(co->canvas, "comp");
        if (ly) e_mod_comp_layer_bg_adjust(ly);
     }
}

EAPI void
e_mod_comp_win_comp_objs_lower(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   E_Comp_Layer *ly;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        e_layout_child_lower(co->shadow);
        if (_comp_mod->conf->use_hwc)
          {
             e_layout_child_raise_above(co->hwc.mask_rect, co->shadow);
          }

        ly = e_mod_comp_canvas_layer_get(co->canvas, "comp");
        if (ly) e_mod_comp_layer_bg_adjust(ly);
     }
}

EAPI void
e_mod_comp_win_comp_objs_stack_above(E_Comp_Win *cw,
                                     E_Comp_Win *cw2)
{
   Eina_List *l, *ll;
   E_Comp_Object *co, *co2;
   E_Comp_Layer *ly;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        EINA_LIST_FOREACH(cw2->objs, ll, co2)
          {
             if (co->zone == co2->zone)
               {
                  e_layout_child_raise_above(co->shadow,
                                             co2->shadow);
                  if (_comp_mod->conf->use_hwc)
                    {
                       e_layout_child_raise_above(co->hwc.mask_rect, co->shadow);
                       e_layout_child_raise_above(co2->hwc.mask_rect, co2->shadow);
                    }
                  ly = e_mod_comp_canvas_layer_get(co->canvas, "comp");
                  if (ly) e_mod_comp_layer_bg_adjust(ly);
               }
          }
     }
}

EAPI void
e_mod_comp_win_comp_objs_transparent_rect_update(E_Comp_Win *cw)
{
   E_Comp_Object *co;
   Eina_List *l;

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (cw->transp_rect.use)
          {
             if (!co->transp.offset)
               co->transp.offset = evas_object_rectangle_add(co->canvas->evas);
             if (!co->transp.rect)
               co->transp.rect = evas_object_rectangle_add(co->canvas->evas);

             if ((co->transp.offset) && (co->transp.rect))
               {
                  evas_object_size_hint_min_set(co->transp.offset,
                                                cw->transp_rect.x,
                                                cw->transp_rect.y);
                  evas_object_size_hint_min_set(co->transp.rect,
                                                cw->transp_rect.w,
                                                cw->transp_rect.h);
                  evas_object_color_set(co->transp.offset, 0, 0, 0, 0);
                  evas_object_color_set(co->transp.rect, 0, 0, 0, 0);
                  evas_object_render_op_set(co->transp.rect, EVAS_RENDER_COPY);

                  if (co->shadow)
                    {
                       if (edje_object_part_exists(co->shadow, "e.swallow.transp.offset"))
                         {
                            edje_object_part_swallow(co->shadow,
                                                     "e.swallow.transp.offset",
                                                     co->transp.offset);
                         }

                       if (edje_object_part_exists(co->shadow, "e.swallow.transp.rect"))
                         {
                            edje_object_part_swallow(co->shadow,
                                                     "e.swallow.transp.rect",
                                                     co->transp.rect);
                         }
                    }
               }
          }
        else
          {
             if (co->transp.offset)
               {
                  if (co->shadow)
                    {
                       if (edje_object_part_exists(co->shadow, "e.swallow.transp.offset"))
                         {
                            edje_object_part_unswallow(co->shadow,
                                                       co->transp.offset);
                         }
                    }
                  evas_object_size_hint_min_set(co->transp.offset, 0, 0);
                  evas_object_color_set(co->transp.offset, 0, 0, 0, 0);
                  evas_object_del(co->transp.offset);
                  co->transp.offset = NULL;
               }
             if (co->transp.rect)
               {
                  if (co->shadow)
                    {
                       if (edje_object_part_exists(co->shadow, "e.swallow.transp.rect"))
                         {
                            edje_object_part_unswallow(co->shadow,
                                                       co->transp.rect);
                         }
                    }
                  evas_object_size_hint_min_set(co->transp.rect, 0, 0);
                  evas_object_color_set(co->transp.rect, 0, 0, 0, 0);
                  evas_object_render_op_set(co->transp.rect, EVAS_RENDER_BLEND);
                  evas_object_del(co->transp.rect);
                  co->transp.rect = NULL;
               }
          }
     }
}

EAPI void
e_mod_comp_win_hwcomp_mask_objs_show(E_Comp_Win *cw)
{
   E_CHECK(cw);

   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->hwc.mask_rect) continue;
        if (!co->shadow) continue;

        e_layout_child_move(co->hwc.mask_rect, cw->x, cw->y);
        e_layout_child_resize(co->hwc.mask_rect, cw->pw, cw->ph);
        e_layout_child_raise_above(co->hwc.mask_rect, co->shadow);
        evas_object_show(co->hwc.mask_rect);
#if DEBUG_HWC
        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s| Show(x:%d, y:%d, w:%d, h:%d)", "Mask OBJ", cw->x, cw->y, cw->pw, cw->ph);
#endif
     }
}

EAPI void
e_mod_comp_win_hwcomp_mask_objs_hide(E_Comp_Win *cw)
{
   E_CHECK(cw);

   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->hwc.mask_rect) continue;

        evas_object_hide(co->hwc.mask_rect);
#if DEBUG_HWC
        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s| Hide", "Mask OBJ");
#endif
     }
}