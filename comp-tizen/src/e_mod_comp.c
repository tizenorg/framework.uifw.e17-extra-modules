#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include <dlog.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define LOG_TAG "E17_EXTRA_MODULES"

#define OVER_FLOW 2

//////////////////////////////////////////////////////////////////////////
//
// TODO (no specific order):
//   1. abstract evas object and compwin so we can duplicate the object N times
//      in N canvases - for winlist, everything, pager etc. too
//   2. implement "unmapped composite cache" -> N pixels worth of unmapped
//      windows to be fully composited. only the most active/recent.
//   3. for unmapped windows - when window goes out of unmapped comp cache
//      make a miniature copy (1/4 width+height?) and set property on window
//      with pixmap id
//   8. obey transparent property
//   9. shortcut lots of stuff to draw inside the compositor - shelf,
//      wallpaper, efm - hell even menus and anything else in e (this is what
//      e18 was mostly about)
//  10. fullscreen windows need to be able to bypass compositing *seems buggy*
//
//////////////////////////////////////////////////////////////////////////

/* static global variables */
static Eina_List *handlers      = NULL;
static Eina_List *handlers_hook = NULL;
static Eina_List *compositors   = NULL;
static Eina_Hash *windows       = NULL;
static Eina_Hash *borders       = NULL;
static Eina_Hash *damages       = NULL;

/* static functions */
static void         _e_mod_comp_render_queue(E_Comp *c);
static Eina_Bool    _e_mod_comp_effect_init(E_Comp *c);
static void         _e_mod_comp_win_damage(E_Comp_Win *cw, int x, int y, int w, int h, Eina_Bool dmg);
static void         _e_mod_comp_win_del(E_Comp_Win *cw);
static void         _e_mod_comp_win_real_hide(E_Comp_Win *cw);
static void         _e_mod_comp_win_hide(E_Comp_Win *cw);
static void         _e_mod_comp_win_configure(E_Comp_Win *cw, int x, int y, int w, int h, int border);
static Eina_Bool    _e_mod_comp_win_damage_timeout(void *data);
static void         _e_mod_comp_win_raise(E_Comp_Win *cw);
static void         _e_mod_comp_win_lower(E_Comp_Win *cw);
static E_Comp_Win  *_e_mod_comp_win_find(Ecore_X_Window win);
static E_Comp_Win  *_e_mod_comp_border_client_find(Ecore_X_Window win);
static Eina_Bool    _e_mod_comp_cb_update(E_Comp *c);
static Eina_Bool    _e_mod_comp_win_is_border(E_Comp_Win *cw);
static void         _e_mod_comp_cb_pending_after(void *data, E_Manager *man, E_Manager_Comp_Source *src);
static E_Comp      *_e_mod_comp_find(Ecore_X_Window root);
static void         _e_mod_comp_win_render_queue(E_Comp_Win *cw);
static Evas_Object *_e_mod_comp_win_mirror_add(E_Comp_Win *cw);
static void         _e_mod_comp_cb_win_mirror_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void         _e_mod_comp_src_hidden_set_func(void *data, E_Manager *man, E_Manager_Comp_Source *src, Eina_Bool hidden);
static Eina_Bool    _e_mod_comp_prop_window_use_dri2_get(Ecore_X_Window win);
static Eina_Bool    _e_mod_comp_prop_use_dri2(Ecore_X_Event_Window_Property *ev);
static void         _e_mod_comp_composite_mode_set(void *data, E_Manager *man, E_Zone *zone, Eina_Bool set);
static void         _e_comp_event_src_visibility_free(void *data, void *event);
static void         _e_mod_comp_win_transparent_rect_update(E_Comp_Win *cw);
static Eina_Bool    _e_mod_comp_prop_window_role(Ecore_X_Event_Window_Property *ev);
static void         _e_mod_comp_window_role_handle(E_Comp_Win *cw);
static Eina_Bool    _e_mod_comp_win_rr_output_prop_set(Ecore_X_Window win, unsigned int_mode);
static Eina_Bool    _e_mod_comp_prop_3d_win_mode(Ecore_X_Event_Window_Property *ev);
static Eina_Bool    _e_mod_comp_win_stereo_layout_set(Ecore_X_Window win, unsigned int mode);
static void         _e_mod_comp_active_request_handle(Ecore_X_Event_Client_Message *ev);

///////////////////////////////////////////////////////////////////////////////////
EAPI void
e_comp_event_src_visibility_send(Ecore_X_Window win,
                                 Eina_Bool      visible)
{
   E_Event_Comp_Source_Visibility *ev;
   ev = E_NEW(E_Event_Comp_Source_Visibility, 1);
   if (ev)
     {
        ev->win = win;
        ev->visible = visible;
        ecore_event_add(E_EVENT_COMP_SOURCE_VISIBILITY, ev, (Ecore_End_Cb)_e_comp_event_src_visibility_free, NULL);
     }
}

EAPI Eina_Bool
e_mod_comp_comp_event_src_visibility_send(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->man, 0);

   cw->pending_count++;

   e_manager_comp_event_src_visibility_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_win_is_border(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   if (cw->bd) return EINA_TRUE;
   else return EINA_FALSE;
}

EAPI void
e_mod_comp_fps_toggle(void)
{
   if (_comp_mod)
     {
        Eina_List *l;
        E_Comp *c;

        if (_comp_mod->conf->fps_show)
          {
             _comp_mod->conf->fps_show = 0;
          }
        else
          {
             _comp_mod->conf->fps_show = 1;
          }
        e_config_save_queue();
        EINA_LIST_FOREACH(compositors, l, c) _e_mod_comp_cb_update(c);
     }
}

EAPI Eina_Bool
e_mod_comp_win_add_damage(E_Comp_Win *cw,
                          Ecore_X_Damage dmg)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(dmg, 0);
   return eina_hash_add(damages, e_util_winid_str_get(dmg), cw);
}

EAPI Eina_Bool
e_mod_comp_win_del_damage(E_Comp_Win *cw,
                          Ecore_X_Damage dmg)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(dmg, 0);
   return eina_hash_del(damages, e_util_winid_str_get(dmg), cw);
}

EAPI void
e_mod_comp_composite_mode_set(E_Zone   *zone,
                              Eina_Bool set)
{
   _e_mod_comp_composite_mode_set(e_mod_comp_util_get(),
                                  NULL, zone, set);
}

static void
_e_mod_comp_cb_pending_after(void *data __UNUSED__,
                             E_Manager *man __UNUSED__,
                             E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_Comp *c = (E_Comp *)data;

   cw->pending_count--;

   if (!cw->delete_pending) return;
   if (cw->pending_count == 0)
     {
        E_Comp_Canvas *canvas = NULL;
        Eina_List *l = NULL;
        if (_comp_mod->conf->nocomp_fs && c)
          {
             EINA_LIST_FOREACH(c->canvases, l, canvas)
               {
                  if (canvas->nocomp.end.cw == cw)
                    {
                       if (canvas->nocomp.end.timer)
                         {
                            ecore_timer_del(canvas->nocomp.end.timer);
                            canvas->nocomp.end.timer = NULL;
                         }
                       e_mod_comp_canvas_nocomp_dispose(canvas);
                       break;
                    }
               }
          }
        free(cw);
     }
}

static Eina_Bool
_e_mod_comp_effect_init(E_Comp *c)
{
   if (!(c->effect_handle = dlopen(_comp_mod->conf->effect_file, (RTLD_NOW | RTLD_GLOBAL))))
     {
        /* cannot open the .so file, bail out */
        ELBF(ELBT_COMP, 0, 0,
            "Cannot open effect so: %s", ecore_file_file_get(_comp_mod->conf->effect_file));
        return 0;
     }

   c->effect_funcs.init = dlsym(c->effect_handle, "e_mod_comp_effect_mod_init");
   c->effect_funcs.shutdown = dlsym(c->effect_handle, "e_mod_comp_effect_mod_shutdown");

   /* check that policy supports needed functions */
   if ((!c->effect_funcs.init) || (!c->effect_funcs.shutdown))
     {
        ELBF(ELBT_COMP, 0, 0,
            "Effect does not support needed functions: %s", ecore_file_file_get(_comp_mod->conf->effect_file));
        return 0;
     }

   /* try to initialize the effect */
   if (!c->effect_funcs.init(c))
     {
        /* init failed, bail out */
        ELBF(ELBT_COMP, 0, 0,
            "Effect failed to initialize: %s", ecore_file_file_get(_comp_mod->conf->effect_file));
        return 0;
     }

   return EINA_TRUE;
}

static inline Eina_Bool
_e_mod_comp_shaped_check(int                      w,
                         int                      h,
                         const Ecore_X_Rectangle *rects,
                         int                      num)
{
   if ((!rects) || (num < 1)) return EINA_FALSE;
   if (num > 1) return EINA_TRUE;
   if ((rects[0].x == 0) && (rects[0].y == 0) &&
       ((int)rects[0].width == w) && ((int)rects[0].height == h))
     return EINA_FALSE;
   return EINA_TRUE;
}

static inline Eina_Bool
_e_mod_comp_win_shaped_check(const E_Comp_Win        *cw,
                             const Ecore_X_Rectangle *rects,
                             int                      num)
{
   return _e_mod_comp_shaped_check(cw->w, cw->h, rects, num);
}

static void
_e_mod_comp_win_shape_rectangles_apply(E_Comp_Win              *cw,
                                       const Ecore_X_Rectangle *rects,
                                       int                      num)
{
   Eina_List *l, *ll;
   Evas_Object *o;
   E_Comp_Object *co;
   int i;

   if (!_e_mod_comp_win_shaped_check(cw, rects, num))
     {
        rects = NULL;
     }
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (rects)
          {
             unsigned int *pix, *p;
             unsigned char *spix, *sp;
             int w, h, px, py;

             evas_object_image_size_get(co->img, &w, &h);
             if ((w > 0) && (h > 0))
               {
                  if (co->native) return;
                  evas_object_image_native_surface_set(co->img, NULL);
                  evas_object_image_alpha_set(co->img, 1);
                  EINA_LIST_FOREACH(co->img_mirror, ll, o)
                    {
                       evas_object_image_native_surface_set(o, NULL);
                       evas_object_image_alpha_set(o, 1);
                    }

                  pix = evas_object_image_data_get(co->img, 1);
                  if (pix)
                    {
                       spix = calloc(w * h, sizeof(unsigned char));
                       if (spix)
                         {
                            for (i = 0; i < num; i++)
                              {
                                 int rx, ry, rw, rh;

                                 rx = rects[i].x; ry = rects[i].y;
                                 rw = rects[i].width; rh = rects[i].height;
                                 E_RECTS_CLIP_TO_RECT(rx, ry, rw, rh, 0, 0, w, h);
                                 sp = spix + (w * ry) + rx;
                                 for (py = 0; py < rh; py++)
                                   {
                                      for (px = 0; px < rw; px++)
                                        {
                                           *sp = 0xff; sp++;
                                        }
                                      sp += w - rw;
                                   }
                              }
                            sp = spix;
                            p = pix;
                            for (py = 0; py < h; py++)
                              {
                                 for (px = 0; px < w; px++)
                                   {
                                      unsigned int mask, imask;
                                      mask = ((unsigned int)(*sp)) << 24;
                                      imask = mask >> 8;
                                      imask |= imask >> 8;
                                      imask |= imask >> 8;
                                      *p = mask | (*p & imask);
                                      sp++;
                                      p++;
                                   }
                              }
                            free(spix);
                         }
                       evas_object_image_data_set(co->img, pix);
                       evas_object_image_data_update_add(co->img, 0, 0, w, h);
                       EINA_LIST_FOREACH(co->img_mirror, ll, o)
                         {
                            evas_object_image_data_set(o, pix);
                            evas_object_image_data_update_add(o, 0, 0, w, h);
                         }
                    }
               }
          }
        else
          {
             if (cw->shaped)
               {
                  unsigned int *pix, *p;
                  int w, h, px, py;

                  evas_object_image_size_get(co->img, &w, &h);
                  if ((w > 0) && (h > 0))
                    {
                       if (co->native) return;
                       evas_object_image_alpha_set(co->img, 0);
                       EINA_LIST_FOREACH(co->img_mirror, l, o)
                         {
                            evas_object_image_alpha_set(o, 1);
                         }
                       pix = evas_object_image_data_get(co->img, 1);
                       if (pix)
                         {
                            p = pix;
                            for (py = 0; py < h; py++)
                              {
                                 for (px = 0; px < w; px++)
                                    *p |= 0xff000000;
                              }
                         }
                       evas_object_image_data_set(co->img, pix);
                       evas_object_image_data_update_add(co->img, 0, 0, w, h);
                       EINA_LIST_FOREACH(co->img_mirror, ll, o)
                         {
                            evas_object_image_data_set(o, pix);
                            evas_object_image_data_update_add(o, 0, 0, w, h);
                         }
                    }
               }
             // dont need to fix alpha chanel as blending
             // should be totally off here regardless of
             // alpha channel content
          }
     }
}

static void
_e_mod_comp_win_update(E_Comp_Win *cw)
{
   E_Update_Rect *r;
   int i;

   e_mod_comp_x_grab_set(cw->c, EINA_TRUE);
   cw->update = 0;

   if (cw->argb)
     {
        if (cw->rects)
          {
             free(cw->rects);
             cw->rects = NULL;
             cw->rects_num = 0;
          }
     }
   else
     {
        if (cw->shape_changed)
          {
             if (cw->rects)
               {
                  free(cw->rects);
                  cw->rects = NULL;
                  cw->rects_num = 0;
               }
             ecore_x_pixmap_geometry_get(cw->win, NULL, NULL, &(cw->w), &(cw->h));
             cw->rects = ecore_x_window_shape_rectangles_get(cw->win, &(cw->rects_num));
             if (cw->rects)
               {
                  for (i = 0; i < cw->rects_num; i++)
                    {
                       E_RECTS_CLIP_TO_RECT(cw->rects[i].x,
                                            cw->rects[i].y,
                                            cw->rects[i].width,
                                            cw->rects[i].height,
                                            0, 0, cw->w, cw->h);
                    }
               }
             if (!_e_mod_comp_win_shaped_check(cw, cw->rects, cw->rects_num))
               {
                  free(cw->rects);
                  cw->rects = NULL;
                  cw->rects_num = 0;
               }
             if ((cw->rects) && (!cw->shaped))
               {
                  cw->shaped = 1;
               }
             else if ((!cw->rects) && (cw->shaped))
               {
                  cw->shaped = 0;
               }
          }
     }

   if ((cw->needpix) && (cw->dmg_updates <= 0))
     {
        e_mod_comp_x_grab_set(cw->c, EINA_FALSE);
        return;
     }

   if ((!cw->pixmap) || (cw->needpix))
     {
        Ecore_X_Pixmap pm = 0;
        // named pixmap of the nocomp window will be updated in
        // nocomp_dispose function after the damage event
        if (cw->nocomp)
          {
             e_mod_comp_x_grab_set(cw->c, EINA_FALSE);
             return;
          }
        pm = ecore_x_composite_name_window_pixmap_get(cw->win);
        if (pm)
          {
             Ecore_X_Pixmap oldpm;
             cw->needpix = 0;
             e_mod_comp_win_comp_objs_needxim_set(cw, 1);
             oldpm = cw->pixmap;
             cw->pixmap = pm;
             if (cw->pixmap)
               {
                  ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
                  // pixmap's size is not equal with window's size case
                  if (!((cw->pw == (cw->w + cw->border * 2)) &&
                       (cw->ph == (cw->h + cw->border *2))))
                    {
                       cw->pw = cw->w;
                       cw->ph = cw->h;
                       cw->pixmap = oldpm;
                       cw->needpix = 1;
                       ecore_x_pixmap_free(pm);
                       e_mod_comp_x_grab_set(cw->c, EINA_FALSE);
                       return;
                    }
                  if ((cw->pw > 0) && (cw->ph > 0))
                    e_mod_comp_win_comp_objs_img_resize(cw, cw->pw, cw->ph);
               }
             else
               {
                  cw->pw = 0;
                  cw->ph = 0;
               }
             if ((cw->pw <= 0) || (cw->ph <= 0))
               {
                  e_mod_comp_win_comp_objs_img_deinit(cw);
                  if (cw->pixmap)
                    {
                       ecore_x_pixmap_free(cw->pixmap);
                       cw->pixmap = 0;
                    }
                  cw->pw = 0;
                  cw->ph = 0;
               }
             ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
             e_mod_comp_win_comp_objs_native_set(cw, 0);
             e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
             e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
             if (oldpm) ecore_x_pixmap_free(oldpm);
          }
     }

   if (!((cw->pw > 0) && (cw->ph > 0)))
     {
        e_mod_comp_x_grab_set(cw->c, EINA_FALSE);
        return;
     }

   e_mod_comp_win_comp_objs_move(cw, cw->x, cw->y);
   e_mod_comp_win_comp_objs_resize(cw, cw->pw, cw->ph);

   //update old geometry due to finish resize
   cw->resizing.x = cw->x;
   cw->resizing.y = cw->y;

   if ((cw->c->gl)
       && (_comp_mod->conf->texture_from_pixmap)
       && (!cw->shaped)
       && (!cw->rects))
     {
        e_mod_comp_win_comp_objs_img_size_set(cw, cw->pw, cw->ph);
        e_mod_comp_win_comp_objs_img_init(cw);

        r = e_mod_comp_update_rects_get(cw->up);
        if (r)
          {
             for (i = 0; r[i].w > 0; i++)
               {
                  int x, y, w, h;
                  x = r[i].x; y = r[i].y;
                  w = r[i].w; h = r[i].h;
                  e_mod_comp_win_comp_objs_img_data_update_add(cw, x, y, w, h);

                  /* check update region */
                  if (_comp_mod->conf->use_hwc)
                    {
                       e_mod_comp_hwcomp_check_win_update(cw, w, h);
                    }
               }
             e_mod_comp_update_clear(cw->up);
             free(r);
          }
     }
   else
     {
        Eina_List *l, *ll;
        E_Comp_Object *co;
        Evas_Object *o;
        EINA_LIST_FOREACH(cw->objs, l, co)
          {
             if (co->native)
               {
                  evas_object_image_native_surface_set(co->img, NULL);
                  EINA_LIST_FOREACH(co->img_mirror, ll, o)
                    {
                       evas_object_image_native_surface_set(o, NULL);
                    }
                  co->native = 0;
               }
             if (co->needxim)
               {
                  co->needxim = 0;
                  if (co->xim)
                    {
                       evas_object_image_size_set(co->img, 1, 1);
                       evas_object_image_data_set(co->img, NULL);
                       EINA_LIST_FOREACH(co->img_mirror, ll, o)
                         {
                            evas_object_image_size_set(o, 1, 1);
                            evas_object_image_data_set(o, NULL);
                         }
                       ecore_x_image_free(co->xim);
                       co->xim = NULL;
                    }
               }
             if (!co->xim)
               {
                  if ((co->xim = ecore_x_image_new(cw->pw, cw->ph, cw->vis, cw->depth)))
                    e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
               }
          }
        r = e_mod_comp_update_rects_get(cw->up);
        if (r)
          {
             EINA_LIST_FOREACH(cw->objs, l, co)
               {
                  if (co->xim)
                    {
                       unsigned int *pix;

                       pix = ecore_x_image_data_get(co->xim, NULL, NULL, NULL);
                       evas_object_image_data_set(co->img, pix);
                       evas_object_image_size_set(co->img, cw->pw, cw->ph);
                       EINA_LIST_FOREACH(co->img_mirror, ll, o)
                         {
                            evas_object_image_data_set(o, pix);
                            evas_object_image_size_set(o, cw->pw, cw->ph);
                         }

                       e_mod_comp_update_clear(cw->up);
                       for (i = 0; r[i].w > 0; i++)
                         {
                            int x, y, w, h;
                            x = r[i].x; y = r[i].y;
                            w = r[i].w; h = r[i].h;
                            if (!ecore_x_image_get(co->xim, cw->pixmap, x, y, x, y, w, h))
                              {
                                 e_mod_comp_update_add(cw->up, x, y, w, h);
                                 cw->update = 1;
                              }
                            else
                              {
                                 // why do we neeed these 2? this smells wrong
                                 pix = ecore_x_image_data_get(co->xim, NULL, NULL, NULL);
                                 evas_object_image_data_set(co->img, pix);
                                 evas_object_image_data_update_add(co->img, x, y, w, h);
                                 EINA_LIST_FOREACH(co->img_mirror, ll, o)
                                   {
                                      evas_object_image_data_set(o, pix);
                                      evas_object_image_data_update_add(o, x, y, w, h);
                                   }
                              }
                         }
                    }
               }
             free(r);
             if (cw->shaped)
               {
                  _e_mod_comp_win_shape_rectangles_apply(cw, cw->rects, cw->rects_num);
               }
             else
               {
                  if (cw->shape_changed)
                    _e_mod_comp_win_shape_rectangles_apply(cw, cw->rects, cw->rects_num);
               }
             cw->shape_changed = 0;
          }
     }

   e_mod_comp_win_comp_objs_show(cw);

   if (_comp_mod->conf->use_hwc)
     {
        if (cw->hwc.set_drawable)
          e_mod_comp_hwcomp_win_update(cw);
     }

   e_mod_comp_x_grab_set(cw->c, EINA_FALSE);
}

static void
_e_mod_comp_win_transparent_rect_update(E_Comp_Win *cw)
{
   Ecore_X_Window win;
   unsigned int val[4] = { 0, }; // offset and size
   int ret = -1;

   win = e_mod_comp_util_client_xid_get(cw);

   ret = ecore_x_window_prop_card32_get
           (win, ATOM_CM_TRANSPARENT_RECT, val, 4);
   if (ret <= 0)
     {
        cw->transp_rect.use = EINA_FALSE;
        cw->transp_rect.x = 0;
        cw->transp_rect.y = 0;
        cw->transp_rect.w = 0;
        cw->transp_rect.h = 0;
     }
   else
     {
        cw->transp_rect.use = EINA_TRUE;
        cw->transp_rect.x = val[0];
        cw->transp_rect.y = val[1];
        cw->transp_rect.w = val[2];
        cw->transp_rect.h = val[3];
     }

   e_mod_comp_win_comp_objs_transparent_rect_update(cw);
}

// set XRandr RROutput property according to viewmode
static Eina_Bool
_e_mod_comp_win_rr_output_prop_set(Ecore_X_Window win, unsigned int mode)
{
   XRRScreenResources *resources = NULL;
   Atom view_mode;
   RROutput output = 0;
   unsigned int op_num;
   Data_Rrprop prop_data;
   Ecore_X_Display *x_disp = ecore_x_display_get();

   if (win)
       resources = XRRGetScreenResources(x_disp, win);
   else
       resources = XRRGetScreenResources(x_disp, ecore_x_window_root_first_get());
   if ((!resources) || (resources->noutput <= 0))
      return EINA_FALSE;

   for (op_num = 0; op_num < resources->noutput; op_num++)
    {
      output = resources->outputs[op_num];
      view_mode = ATOM_CM_STEREOSCOPIC_MODE;
      if (view_mode == None)
        {
           printf("RR property is not suppported \n");
           return EINA_FALSE;
        }

       if (win)
         {
           prop_data.win = win;
         }
       else
        {
          prop_data.win = NULL;
        }

       prop_data.value = mode;
       XRRChangeOutputProperty(x_disp, output, view_mode, XA_INTEGER, 32,
            PropModeReplace, (unsigned char *)&prop_data, 2);
     }
    XRRFreeScreenResources(resources);
    return EINA_TRUE;
}

// interfacing of compositor with stereoscopic mode (3D App, 2D App & Multimedia content)
static Eina_Bool
_e_mod_comp_win_stereo_layout_set(Ecore_X_Window win, unsigned int mode)
{
   unsigned int view_mode = 0;
   int ret = -1;
   E_Comp_Win *cw = NULL;
   E_Zone *zone = NULL;

   cw = _e_mod_comp_win_find(win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(win);
        E_CHECK_RETURN(cw, 0);
     }

   ret = ecore_x_window_prop_card32_get
      (win, ATOM_CM_3D_APP_WIN, &view_mode , 1);
   if (ret <= 0)
     {
        cw->win3d = EINA_FALSE;
        if (mode < 4) /*2D app and 3D multimedia content*/
          {
             E_Comp *c = (E_Comp *)(cw->c);
             E_Comp_Canvas *canvas = NULL;
             Eina_List *l;

             E_CHECK_RETURN(c, NULL);

             zone = cw->bd->zone;

             EINA_LIST_FOREACH(c->canvases, l, canvas)
              {
                if (!canvas) continue;

                if(canvas->zone == zone) canvas->stereoscopic_mode = mode;

                e_mod_comp_canvas_stereo_layout_set(canvas); 
              }
          }
     }
   else
     {
        if (view_mode > 0)      /*3D application window case */
          {
            cw->win3d = EINA_TRUE;
            //do nothing in case of 3D app, 3D app will take care
          }
     }
   return EINA_TRUE;
}

// added handler for RR Output property notification (instead of API calling from device manager)
static int
_e_mod_comp_cb_rr_output_prop(void *data, int type, void *ev)
{
   if (type == ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY)
    {
      Ecore_X_Event_Randr_Output_Property_Notify *event = (Ecore_X_Event_Randr_Output_Property_Notify *)ev;
      /* available information:
         struct _Ecore_X_Event_Randr_Output_Property_Notify
         {
         Ecore_X_Window                win;
         Ecore_X_Randr_Output          output;
         Ecore_X_Atom                  property;
         Ecore_X_Time                  time;
         Ecore_X_Randr_Property_Change state;
         };
       */

      char *atom_name = ecore_x_atom_name_get(event->property);

      // checking and retrieving 3D stereoscopic mode
      if ((atom_name) && (strcmp(atom_name, "_E_COMP_STEREOSCOPIC_MODE") == 0))
        {
          Ecore_X_Display *x_disp = ecore_x_display_get();
          Atom actual_type;
          int actual_format;
          unsigned int value;
          unsigned long nitems;
          unsigned long bytes_after;
          Data_Rrprop *prop_data;


          if (XRRGetOutputProperty(x_disp, event->output, ATOM_CM_STEREOSCOPIC_MODE,
               0, 4, False, False, None, &actual_type, &actual_format,
               &nitems, &bytes_after, &prop_data) != Success)
            {
              fprintf(stderr,"DeviceManager :: stereoMode not valid \n");
              if (atom_name) free(atom_name);
              return EINA_FALSE;
            }

          if ((actual_type != XA_INTEGER) || (nitems != 2) || (actual_format != 32))
            {
              fprintf(stderr,"DeviceManager :: stereoMode prop value not correct \n");
              if (atom_name) free(atom_name);
              return EINA_FALSE;
            }

          value = ((Data_Rrprop *)prop_data)->value;

          // taking specific action in compositor based on stereoscopic mode
          if (((Data_Rrprop *)prop_data)->win != NULL)
             _e_mod_comp_win_stereo_layout_set(((Data_Rrprop *)prop_data)->win, value);
       }

      if (atom_name) free(atom_name);
    }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_cb_delayed_update_timer(void *data)
{
   E_Comp *c = data;
   _e_mod_comp_render_queue(c);
   c->new_up_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_comp_cb_update(E_Comp *c)
{
   E_Comp_Win *cw;
   Eina_List *l;
   E_Comp_Canvas *canvas = NULL;
   Eina_List *new_updates = NULL;
   Eina_List *update_done = NULL;

   c->update_job = NULL;

   if (_comp_mod->conf->nocomp_fs)
     {
        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             if (canvas->nocomp.mode != E_NOCOMP_MODE_RUN) continue;
             cw = e_mod_comp_canvas_fullscreen_check(canvas);
             if ((!cw) || (cw != canvas->nocomp.cw) ||
                 (canvas->nocomp.force_composite))
               {
                  ELBF(ELBT_COMP, 0,
                       cw ? e_mod_comp_util_client_xid_get(cw) : 0,
                       "NOCOMP_END nocomp.cw:0x%08x canvas:%d",
                       e_mod_comp_util_client_xid_get(canvas->nocomp.cw),
                       canvas->num);

                  e_mod_comp_canvas_nocomp_end(canvas);
               }
          }
     }

   e_mod_comp_x_grab_set(c, EINA_TRUE);

   EINA_LIST_FREE(c->updates, cw)
     {
        if (!cw) continue;
        if (_comp_mod->conf->efl_sync)
          {
             if (((cw->counter) && (cw->drawme)) || (!cw->counter))
               {
                  _e_mod_comp_win_update(cw);
                  if (cw->drawme)
                    {
                       update_done = eina_list_append(update_done, cw);
                       cw->drawme = 0;
                    }
               }
             else
               cw->update = 0;
          }
        else
          _e_mod_comp_win_update(cw);
        if (cw->update)
          new_updates = eina_list_append(new_updates, cw);
     }

   if (_comp_mod->conf->efl_sync)
     {
        EINA_LIST_FREE(update_done, cw)
          {
             if (!cw) continue;
             ecore_x_sync_counter_inc(cw->counter, 1);
             cw->sync_info.val++;

             ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|done:%d val:%d", "INC",
                  cw->sync_info.done_count, cw->sync_info.val);
          }
     }
   e_mod_comp_win_shape_input_update(c);
   e_mod_comp_x_grab_set(c, EINA_FALSE);

   if (new_updates)
     {
        if (c->new_up_timer) ecore_timer_del(c->new_up_timer);
        c->new_up_timer =
          ecore_timer_add(0.001, _e_mod_comp_cb_delayed_update_timer, c);
     }
   c->updates = new_updates;
   if (!c->animating) c->render_overflow--;

   c->effect_funcs.signal_flush();

   if (_comp_mod->conf->use_hwc)
     {
        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             if (!canvas->hwcomp) continue;

             if (!e_mod_comp_hwcomp_cb_update(canvas->hwcomp))
               return ECORE_CALLBACK_RENEW;
          }
     }

   if (c->render_overflow <= 0)
     {
        c->render_overflow = 0;
        if (c->render_animator) c->render_animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_mod_comp_cb_job(void *data)
{
   _e_mod_comp_cb_update(data);
}

static Eina_Bool
_e_mod_comp_cb_animator(void *data)
{
   return _e_mod_comp_cb_update(data);
}

static void
_e_comp_event_src_visibility_free(void *data __UNUSED__,
                                  void      *event)
{
   E_Event_Comp_Source_Visibility *ev = event;
   E_FREE(ev);
}

static void
_e_mod_comp_render_queue(E_Comp *c)
{
   E_CHECK(c);

   if (_comp_mod->conf->lock_fps)
     {
        if (c->render_animator)
          {
             c->render_overflow = OVER_FLOW;
             return;
          }
        c->render_animator = ecore_animator_add(_e_mod_comp_cb_animator, c);
     }
   else
     {
        if (c->update_job)
          {
             ecore_job_del(c->update_job);
             c->update_job = NULL;
             c->render_overflow = 0;
          }
        c->update_job = ecore_job_add(_e_mod_comp_cb_job, c);
     }
}

static void
_e_mod_comp_win_render_queue(E_Comp_Win *cw)
{
   E_CHECK(cw);
   E_CHECK(cw->c);

   _e_mod_comp_render_queue(cw->c);
}

static E_Comp *
_e_mod_comp_find(Ecore_X_Window root)
{
   Eina_List *l;
   E_Comp *c;
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        if (c->man->root == root) return c;
     }
   return NULL;
}

static E_Comp_Win *
_e_mod_comp_win_find(Ecore_X_Window win)
{
   return eina_hash_find(windows, e_util_winid_str_get(win));
}

static E_Comp_Win *
_e_mod_comp_border_client_find(Ecore_X_Window win)
{
   return eina_hash_find(borders, e_util_winid_str_get(win));
}

/* wrapper function for external file */
EAPI E_Comp_Win *
e_mod_comp_win_find(Ecore_X_Window win)
{
   return _e_mod_comp_win_find(win);
}

EAPI E_Comp_Win *
e_mod_comp_border_client_find(Ecore_X_Window win)
{
   return _e_mod_comp_border_client_find(win);
}

EAPI E_Comp *
e_mod_comp_find(Ecore_X_Window win)
{
   E_CHECK_RETURN(win, 0);
   return _e_mod_comp_find(win);
}

EAPI void
e_mod_comp_render_queue(E_Comp *c)
{
    E_CHECK(c);
   _e_mod_comp_render_queue(c);
}

EAPI void
e_mod_comp_win_render_queue(E_Comp_Win *cw)
{
   E_CHECK(cw);
   _e_mod_comp_win_render_queue(cw);
}

EAPI Eina_Bool
e_mod_comp_win_damage_timeout(void *data)
{
   E_CHECK_RETURN(data, 0);
   return _e_mod_comp_win_damage_timeout(data);
}

EAPI Eina_Bool
e_mod_comp_cb_update(E_Comp *c)
{
   E_CHECK_RETURN(c, 0);
   return _e_mod_comp_cb_update(c);
}

EAPI Evas_Object *
e_mod_comp_win_mirror_add(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   return _e_mod_comp_win_mirror_add(cw);
}

EAPI void
e_mod_comp_cb_win_mirror_del(void        *data,
                             Evas        *e,
                             Evas_Object *obj,
                             void        *event_info)
{
   E_CHECK(data);
   _e_mod_comp_cb_win_mirror_del(data,
                                 e,
                                 obj,
                                 event_info);
}

EAPI void
e_mod_comp_src_hidden_set_func(void                  *data,
                               E_Manager             *man,
                               E_Manager_Comp_Source *src,
                               Eina_Bool              hidden)
{
   _e_mod_comp_src_hidden_set_func(data, man, src, hidden);
}

static E_Comp_Win *
_e_mod_comp_win_damage_find(Ecore_X_Damage damage)
{
   return eina_hash_find(damages, e_util_winid_str_get(damage));
}

static Eina_Bool
_e_mod_comp_win_is_borderless(E_Comp_Win *cw)
{
   if (!cw->bd) return 1;
   if ((cw->bd->client.border.name) &&
       (!strcmp(cw->bd->client.border.name, "borderless")))
     return 1;
   return 0;
}

static Eina_Bool
_e_mod_comp_win_damage_timeout(void *data)
{
   E_Comp_Win *cw = data;
   E_CHECK_RETURN(cw, 0);

   if (!cw->update)
     {
        if (cw->update_timeout)
          {
             ecore_timer_del(cw->update_timeout);
             cw->update_timeout = NULL;
          }
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|%dx%d p:%dx%d dmg:%d NO_DRAWDONE", "DMG_TIMEOUT",
        cw->w, cw->h, cw->pw, cw->ph, cw->dmg_updates);

   cw->drawme = 1;
   _e_mod_comp_win_render_queue(cw);
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_comp_object_del(void *data,
                       void *obj)
{
   E_Comp_Win *cw = (E_Comp_Win *)data;
   E_CHECK(cw);

   _e_mod_comp_win_render_queue(cw);

   if (obj == cw->bd)
     {
        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s|", "OBJECT_DEL");

        if (cw->counter)
          {
             Ecore_X_Window _w = e_mod_comp_util_client_xid_get(cw);
             ecore_x_e_comp_sync_cancel_send(_w);
             ecore_x_sync_counter_inc(cw->counter, 1);
             cw->sync_info.val++;

             ELBF(ELBT_COMP, 1, _w, "%15.15s", "SYNC_CANCEL");
          }
        if (cw->bd) eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
        cw->bd = NULL;
        e_mod_comp_win_comp_objs_data_del(cw, "border");
     }
   else if (obj == cw->pop)
     {
        cw->pop = NULL;
        e_mod_comp_win_comp_objs_data_del(cw, "popup");
     }
   else if (obj == cw->menu)
     {
        cw->menu = NULL;
        e_mod_comp_win_comp_objs_data_del(cw, "menu");
     }
   if (cw->dfn)
     {
        e_object_delfn_del(obj, cw->dfn);
        cw->dfn = NULL;
     }
}

EAPI void
e_mod_comp_done_defer(E_Comp_Win *cw)
{
   E_CHECK(cw);
   cw->c->effect_funcs.animating_set(cw->c, cw, EINA_FALSE);

   cw->force = 1;
   if (cw->defer_hide)
     {
        ELBF(ELBT_COMP, 0,
             e_mod_comp_util_client_xid_get(cw),
             "EDJ_DONE Force win to hide bd:%d",
             cw->bd);

        _e_mod_comp_win_hide(cw);
     }
   cw->force = 1;
   if (cw->delete_me)
     {
        ELBF(ELBT_COMP, 0,
             e_mod_comp_util_client_xid_get(cw),
             "EDJ_DONE Force win to del bd:%d",
             cw->bd);

        _e_mod_comp_win_del(cw);
     }
   else cw->force = 0;
}

static void
_e_mod_comp_show_done(void        *data,
                      Evas_Object *obj,
                      const char  *emission __UNUSED__,
                      const char  *source   __UNUSED__)
{
   E_Comp_Win *cw = (E_Comp_Win *)data;
   E_CHECK(cw);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|SHOW_DONE", "SIGNAL");

   cw->c->effect_funcs.signal_del(cw, obj, "show,done");

   cw->launched = EINA_TRUE;
   cw->show_done = EINA_TRUE;
   e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_hide_done(void        *data,
                      Evas_Object *obj,
                      const char  *emission __UNUSED__,
                      const char  *source   __UNUSED__)
{
   E_Comp_Win *cw = (E_Comp_Win *)data;
   E_CHECK(cw);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|HIDE_DONE", "SIGNAL");

   cw->c->effect_funcs.signal_del(cw, obj, "hide,done");

   cw->show_done = EINA_FALSE;
   e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_win_sync_setup(E_Comp_Win *cw,
                           Ecore_X_Window win)
{
   if (!_comp_mod->conf->efl_sync) return;
   if (cw->bd)
     {
        if (_e_mod_comp_win_is_borderless(cw) ||
            (_comp_mod->conf->loose_sync))
          {
             cw->counter = ecore_x_e_comp_sync_counter_get(win);
          }
        else
          {
             ecore_x_e_comp_sync_cancel_send(win);
             cw->counter = 0;
          }
     }
   else
     cw->counter = ecore_x_e_comp_sync_counter_get(win);

   if (cw->counter)
     {
        if (cw->bd)
          {
             E_Comp_Win *client_cw = _e_mod_comp_win_find(win);
             if (client_cw &&
                 client_cw->counter == cw->counter)
               {
                  ecore_x_sync_counter_inc(cw->counter, 1);
                  cw->sync_info.val++;
                  return;
               }
          }

        ecore_x_e_comp_sync_begin_send(win);
        ecore_x_sync_counter_inc(cw->counter, 1);
        cw->sync_info.val++;
     }
}

static void
_e_mod_comp_win_shadow_setup_error_get(E_Comp_Win  *cw,
                                       Evas_Object *o,
                                       const char  *msg,
                                       const char  *file)
{
   Ecore_X_Window win = e_mod_comp_util_client_xid_get(cw);

   fprintf(stdout,
           "[E17-comp] EDC file ERROR win:0x%08x o:%p %s FILE:%s\n",
           win, o, msg, file);

   ELBF(ELBT_COMP, 0, win,
        "%15.15s|ERROR o:%p %s",
        "EDC", o, msg);

   ELBF(ELBT_COMP, 0, win,
        "%15.15s|ERROR FILE:%s",
        "EDC", file);

   e_mod_comp_debug_edje_error_get(o, win);
}

EAPI void
e_mod_comp_win_shadow_setup(E_Comp_Win    *cw,
                            E_Comp_Object *co)
{
   Evas_Object *o;
   int ok = 0;
   char buf[PATH_MAX];
   Eina_List *l;

   evas_object_image_smooth_scale_set(co->img, _comp_mod->conf->smooth_windows);
   EINA_LIST_FOREACH(co->img_mirror, l, o)
     {
        evas_object_image_smooth_scale_set(o, _comp_mod->conf->smooth_windows);
     }

   if (_comp_mod->conf->shadow_file)
     {
        ok = edje_object_file_set
          (co->shadow, _comp_mod->conf->shadow_file,
          e_mod_comp_policy_win_shadow_group_get(cw));

        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s|GROUP:%s", "EDC",
             e_mod_comp_policy_win_shadow_group_get(cw));
     }

   if (!ok)
     {
        _e_mod_comp_win_shadow_setup_error_get
          (cw, co->shadow, "SHADOW_FILE",
          _comp_mod->conf->shadow_file);

        if (_comp_mod->conf->shadow_style)
          {
             snprintf(buf, sizeof(buf), "e/comp/%s",
                      _comp_mod->conf->shadow_style);

             ok = e_theme_edje_object_set(co->shadow,
                                          "base/theme/borders",
                                          buf);
          }
        if (!ok)
          {
             _e_mod_comp_win_shadow_setup_error_get
               (cw, co->shadow, "STYLE",
               _comp_mod->conf->shadow_style);

             ok = e_theme_edje_object_set(co->shadow,
                                          "base/theme/borders",
                                          "e/comp/default");
          }
     }
   // fallback to local shadow.edj - will go when default theme supports this
   if (!ok)
     {
        _e_mod_comp_win_shadow_setup_error_get
          (cw, co->shadow, "STYLE_DEFAULT",
          "e/comp/default");

        snprintf(buf, sizeof(buf), "%s/shadow.edj",
                 e_module_dir_get(_comp_mod->module));
        ok = edje_object_file_set(co->shadow, buf, "shadow");

        if (!ok)
          {
             _e_mod_comp_win_shadow_setup_error_get
               (cw, co->shadow, "LOCAL", buf);
          }
     }
   if (!edje_object_part_swallow(co->shadow,
                                 "e.swallow.content",
                                 co->img))
     {
        fprintf(stdout,
                "[E17-comp] EDC swallow ERROR win:0x%08x %s(%d) o:%p img:%p\n",
                cw->win, __func__, __LINE__, co->shadow, co->img);

        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s|SWALLOW_ERROR o:%p img:%p",
             "EDC", co->shadow, co->img);
     }
   e_mod_comp_debug_edje_error_get
     (co->shadow, e_mod_comp_util_client_xid_get(cw));
}

static Eina_Bool
_e_mod_comp_shadow_set(E_Comp_Win *cw,
                       Eina_Bool  set)
{
#if USE_SHADOW
   Eina_List *l;
   E_Comp_Object *co;

   if (!cw) return EINA_FALSE;

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if ((co->shadow) && (co->img))
          {
             if (set)
               edje_object_signal_emit(co->shadow, "e,state,shadow,on", "e");
             else
               edje_object_signal_emit(co->shadow, "e,state,shadow,off", "e");
          }
     }
#endif
   return EINA_TRUE;
}

static void
_e_mod_comp_cb_win_mirror_del(void            *data,
                              Evas            *e,
                              Evas_Object     *obj,
                              void *event_info __UNUSED__)
{
   E_Comp_Win *cw = (E_Comp_Win *)data;
   E_Comp_Object *co;
   Eina_List *l;
   E_CHECK(cw);

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (co->canvas->evas == e)
          {
             co->img_mirror = eina_list_remove(co->img_mirror, obj);
             break;
          }
     }
}

static Evas_Object *
_e_mod_comp_win_mirror_add(E_Comp_Win *cw)
{
   Evas_Object *o, *smart_parent_obj;
   Eina_List *l;
   E_Comp_Object *co;
   E_Zone *z;

   E_CHECK_RETURN(cw->c, 0);

   if (cw->bd) z = cw->bd->zone;
   else z = e_util_zone_current_get(cw->c->man);
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if ((_comp_mod->conf->canvas_per_zone) &&
            ((co->zone) != (z)))
          continue;

        o = evas_object_image_filled_add(co->canvas->evas);
        evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
        co->img_mirror = eina_list_append(co->img_mirror, o);
        evas_object_image_smooth_scale_set(o, _comp_mod->conf->smooth_windows);

        evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                       _e_mod_comp_cb_win_mirror_del, cw);

        if ((cw->pixmap) && (cw->pw > 0) && (cw->ph > 0))
          {
             unsigned int *pix;
             Eina_Bool alpha;
             int w, h;

             alpha = evas_object_image_alpha_get(co->img);
             evas_object_image_size_get(co->img, &w, &h);

             evas_object_image_alpha_set(o, alpha);

             if (cw->shaped)
               {
                  pix = evas_object_image_data_get(co->img, 0);
                  evas_object_image_data_set(o, pix);
                  evas_object_image_size_set(o, w, h);
                  evas_object_image_data_set(o, pix);
                  evas_object_image_data_update_add(o, 0, 0, w, h);
               }
             else
               {
                  if (co->native)
                    {
                       Evas_Native_Surface ns;

                       ns.version = EVAS_NATIVE_SURFACE_VERSION;
                       ns.type = EVAS_NATIVE_SURFACE_X11;
                       ns.data.x11.visual = cw->vis;
                       ns.data.x11.pixmap = cw->pixmap;
                       evas_object_image_size_set(o, w, h);
                       evas_object_image_native_surface_set(o, &ns);
                       evas_object_image_data_update_add(o, 0, 0, w, h);
                    }
                  else
                    {
                       if (!co->xim)
                         {
                            evas_object_del(o);
                            return NULL;
                         }
                       pix = ecore_x_image_data_get(co->xim, NULL, NULL, NULL);
                       evas_object_image_data_set(o, pix);
                       evas_object_image_size_set(o, w, h);
                       evas_object_image_data_set(o, pix);
                       evas_object_image_data_update_add(o, 0, 0, w, h);
                    }
               }
             evas_object_image_size_set(o, w, h);
             evas_object_image_data_update_add(o, 0, 0, w, h);
          }
        smart_parent_obj = evas_object_smart_parent_get(co->shadow);
        if (!smart_parent_obj)
          evas_object_stack_above(o, co->shadow);
        else
          evas_object_stack_above(o, smart_parent_obj);
        return o;
     }
   return NULL;
}

static E_Comp_Win *
_e_mod_comp_win_add(E_Comp        *c,
                    Ecore_X_Window win)
{
   Ecore_X_Window_Attributes att;
   E_Comp_Win *cw;
   Eina_List *l;
   E_Comp_Object *co;

   cw = E_NEW(E_Comp_Win, 1);
   E_CHECK_RETURN(cw, 0);
   cw->win = win;
   cw->c = c;
   cw->opacity = 255.0;
   cw->bd = e_border_find_by_window(cw->win);
   e_mod_comp_x_grab_set(c, EINA_TRUE);
   if (cw->bd)
     {
        eina_hash_add(borders, e_util_winid_str_get(cw->bd->client.win), cw);
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->bd), _e_mod_comp_object_del, cw);
     }
   else if ((cw->pop = e_popup_find_by_window(cw->win)))
     {
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->pop),
                                     _e_mod_comp_object_del, cw);
        cw->show_ready = 1;
     }
   else if ((cw->menu = e_menu_find_by_window(cw->win)))
     {
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->menu),
                                     _e_mod_comp_object_del, cw);
        cw->show_ready = 1;
     }
   else
     {
        char *netwm_title = NULL;

        cw->title = ecore_x_icccm_title_get(cw->win);
        if (ecore_x_netwm_name_get(cw->win, &netwm_title))
          {
             if (cw->title) free(cw->title);
             cw->title = netwm_title;
          }
        ecore_x_icccm_name_class_get(cw->win, &cw->name, &cw->clas);
        cw->role = ecore_x_icccm_window_role_get(cw->win);
        if (!ecore_x_netwm_window_type_get(cw->win, &cw->primary_type))
          cw->primary_type = ECORE_X_WINDOW_TYPE_UNKNOWN;
     }

   e_mod_comp_win_type_setup(cw);
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   if (!ecore_x_window_attributes_get(cw->win, &att))
     {
        LOGW("[CW Creating Fail!!(Attributes get)] Win:%08x, BD:%p, POP:%p, MENU:%p", e_mod_comp_util_client_xid_get(cw), cw->bd, cw->pop, cw->menu);
        if (cw->bd)
          {
             eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
             if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->bd), cw->dfn);
          }
        else if (cw->pop)
          {
             if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->pop), cw->dfn);
          }
        else if (cw->menu)
          {
             if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->menu), cw->dfn);
          }

        if (cw->title) free(cw->title);
        if (cw->name) free(cw->name);
        if (cw->clas) free(cw->clas);
        if (cw->role) free(cw->role);
        free(cw);

        e_mod_comp_x_grab_set(c, EINA_FALSE);
        return NULL;
     }

   if ((!att.input_only) &&
       ((att.depth != 24) && (att.depth != 32)))
     {
        printf("WARNING: window 0x%x not 24/32bpp -> %ibpp\n",
               cw->win, att.depth);
        cw->invalid = 1;
     }
   cw->input_only = att.input_only;
   cw->override = att.override;
   cw->vis = att.visual;
   cw->depth = att.depth;
   cw->argb = ecore_x_window_argb_get(cw->win);

   // setting default value for 3D win and stereoscpic mode
   cw->win3d = EINA_FALSE;
   //cw->c->stereoscopic_mode = 0;

   eina_hash_add(windows, e_util_winid_str_get(cw->win), cw);
   cw->inhash = 1;
   if ((!cw->input_only) && (!cw->invalid))
     {
        Ecore_X_Rectangle *rects;
        int num;

        cw->damage = ecore_x_damage_new
          (cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
        cw->objs = e_mod_comp_win_comp_objs_add(cw);
        if (!cw->objs)
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|ERROR", "OBJECT_ADD");

             LOGW("[CW Creating Fail!!(Object Add fail)] Win:%08x, BD:%p, POP:%p, MENU:%p", e_mod_comp_util_client_xid_get(cw), cw->bd, cw->pop, cw->menu);
             if (cw->bd)
               {
                  eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
                  if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->bd), cw->dfn);
               }
             else if (cw->pop)
               {
                  if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->pop), cw->dfn);
               }
             else if (cw->menu)
               {
                  if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->menu), cw->dfn);
               }
             eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);

             if (cw->title) free(cw->title);
             if (cw->name) free(cw->name);
             if (cw->clas) free(cw->clas);
             if (cw->role) free(cw->role);
             free(cw);

             e_mod_comp_x_grab_set(c, EINA_FALSE);
             return NULL;
          }

        E_Comp_Canvas *canvas;
        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             if (!canvas) continue;
             if (canvas->use_bg_img) continue;
             evas_object_lower(canvas->bg_img);
          }

        EINA_LIST_FOREACH(cw->objs, l, co)
          {
             if (!co) continue;
             e_mod_comp_win_shadow_setup(cw, co);
             e_mod_comp_win_cb_setup(cw, co);
             if (co->img) evas_object_show(co->img);
          }

        _e_mod_comp_win_transparent_rect_update(cw);

        ecore_x_window_shape_events_select(cw->win, 1);
        rects = ecore_x_window_shape_rectangles_get(cw->win, &num);
        if (rects)
          {
             int i;
             for (i = 0; i < num; i++)
               E_RECTS_CLIP_TO_RECT(rects[i].x, rects[i].y,
                                    rects[i].width, rects[i].height,
                                    0, 0, att.w, att.h);
             if (_e_mod_comp_shaped_check(att.w, att.h, rects, num))
               cw->shape_changed = 1;

             free(rects);
          }

        if (cw->bd) e_mod_comp_win_comp_objs_data_set(cw, "border", cw->bd);
        else if (cw->pop)
          e_mod_comp_win_comp_objs_data_set(cw, "popup", cw->pop);
        else if (cw->menu)
          e_mod_comp_win_comp_objs_data_set(cw, "menu", cw->menu);

        e_mod_comp_win_comp_objs_img_pass_events_set(cw, 1);

        cw->pending_count++;
        e_manager_comp_event_src_add_send
          (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
     }
   else
     {
        cw->objs = e_mod_comp_win_comp_objs_add(cw);
        if (!cw->objs)
          {
             LOGW("[CW Creating Fail!!(Input only Object Add fail)] Win:%08x, BD:%p, POP:%p, MENU:%p", e_mod_comp_util_client_xid_get(cw), cw->bd, cw->pop, cw->menu);
             if (cw->bd)
               {
                  eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
                  if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->bd), cw->dfn);
               }
             else if (cw->pop)
               {
                  if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->pop), cw->dfn);
               }
             else if (cw->menu)
               {
                  if (cw->dfn) e_object_delfn_del(E_OBJECT(cw->menu), cw->dfn);
               }
             eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);

             if (cw->title) free(cw->title);
             if (cw->name) free(cw->name);
             if (cw->clas) free(cw->clas);
             if (cw->role) free(cw->role);
             free(cw);

             e_mod_comp_x_grab_set(c, EINA_FALSE);
             return NULL;
          }
     }
   e_mod_comp_win_comp_objs_pass_events_set(cw, 1);
   e_mod_comp_win_comp_objs_data_set(cw, "win",
                               (void *)((unsigned long)cw->win));
   e_mod_comp_win_comp_objs_data_set(cw, "src", cw);

   c->wins_invalid = 1;
   c->wins = eina_inlist_append(c->wins, EINA_INLIST_GET(cw));
   cw->up = e_mod_comp_update_new();
   if (cw->up)
     {
        e_mod_comp_update_tile_size_set(cw->up, 32, 32);
        // for software:
        e_mod_comp_update_policy_set
           (cw->up, E_UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH);
     }
   if (((!cw->input_only) && (!cw->invalid)) && (cw->override))
     {
        cw->redirected = 1;
        cw->dmg_updates = 0;
     }

   cw->eff_type = cw->c->effect_funcs.type_new();
   if (cw->eff_type)
     {
        cw->c->effect_funcs.type_setup
          (cw->eff_type,
          e_mod_comp_util_client_xid_get(cw));
     }
   if ((!cw->input_only) && (!cw->invalid))
     _e_mod_comp_window_role_handle(cw);

   e_mod_comp_x_grab_set(c, EINA_FALSE);
   cw->launched = EINA_FALSE;

   if (_comp_mod->conf->use_hwc)
     {
        cw->hwc.set_drawable = EINA_FALSE;
     }

   return cw;
}

static void
_e_mod_comp_win_del(E_Comp_Win *cw)
{
   int pending_count;
   E_Comp_Canvas *canvas;
   E_Comp_Win *nocomp_cw = NULL;
   E_Zone *zone = NULL;

   canvas = eina_list_nth(cw->c->canvases, 0);
   if (canvas) zone = canvas->zone;

   if (zone) nocomp_cw = e_mod_comp_util_win_nocomp_get(cw->c, zone);
   if ((canvas) && (nocomp_cw) && (nocomp_cw == cw))
     {
        e_mod_comp_canvas_nocomp_end(canvas);
#if USE_NOCOMP_DISPOSE
        e_mod_comp_canvas_nocomp_dispose(canvas);
#endif
     }

   if (_comp_mod->conf->use_hwc && (canvas))
     {
       e_mod_comp_hwcomp_reset_idle_timer(canvas);
       e_mod_comp_hwcomp_win_del(cw);
     }

   if (cw->animating)
     cw->c->effect_funcs.animating_set(cw->c, cw, EINA_FALSE);

   if (cw->shape_input)
     {
        e_mod_comp_win_shape_input_free(cw->shape_input);
        cw->shape_input = NULL;
     }

   if ((!cw->input_only) && (!cw->invalid))
     {
        cw->pending_count++;
        e_manager_comp_event_src_del_send
          (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
     }

   e_mod_comp_update_free(cw->up);
   e_mod_comp_bg_win_handler_release(cw);

   if (cw->rects)
     {
        free(cw->rects);
        cw->rects = NULL;
     }
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (cw->dfn)
     {
        if (cw->bd)
          {
             eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
             e_object_delfn_del(E_OBJECT(cw->bd), cw->dfn);
             cw->bd = NULL;
          }
        else if (cw->pop)
          {
             e_object_delfn_del(E_OBJECT(cw->pop), cw->dfn);
             cw->pop = NULL;
          }
        else if (cw->menu)
          {
             e_object_delfn_del(E_OBJECT(cw->menu), cw->dfn);
             cw->menu = NULL;
          }
        cw->dfn = NULL;
     }
   if (cw->pixmap)
     {
        e_mod_comp_win_comp_objs_img_deinit(cw);

        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
        e_mod_comp_win_comp_objs_xim_free(cw);
     }
   if (cw->redirected)
     {
        cw->redirected = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   if (cw->update)
     {
        cw->update = 0;
        cw->c->updates = eina_list_remove(cw->c->updates, cw);
     }
   e_mod_comp_win_comp_objs_del(cw, cw->objs);
   if (cw->inhash)
     eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);
   if (cw->damage)
     {
        Ecore_X_Region parts;
        eina_hash_del(damages, e_util_winid_str_get(cw->damage), cw);
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }
   if (cw->title) free(cw->title);
   if (cw->name) free(cw->name);
   if (cw->clas) free(cw->clas);
   if (cw->role) free(cw->role);
   if (cw->eff_type)
     {
        cw->c->effect_funcs.type_free(cw->eff_type);
        cw->eff_type = NULL;
     }
   if (cw->ov_obj)
     {
        evas_object_hide(cw->ov_obj);
        evas_object_del(cw->ov_obj);
        cw->ov_obj = NULL;
     }
   if (cw->ov_xim)
     {
        ecore_x_image_free(cw->ov_xim);
        cw->ov_xim = NULL;
     }
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   pending_count = cw->pending_count;
   memset(cw, 0, sizeof(E_Comp_Win));
   cw->pending_count = pending_count;
   cw->delete_pending = 1;
   if (cw->pending_count > 0) return;
   free(cw);
}

static void
_e_mod_comp_win_prop_check(E_Comp_Win *cw)
{
   Ecore_X_Window win;
   Ecore_X_Sync_Counter counter;

   E_CHECK(cw);
   win = e_mod_comp_util_client_xid_get(cw);

   // ECORE_X_ATOM_E_COMP_SYNC_COUNTER
   counter = ecore_x_e_comp_sync_counter_get(win);
   if (cw->counter != counter)
     {
        if (cw->counter)
          {
             ecore_x_e_comp_sync_cancel_send(win);
             ecore_x_sync_counter_inc(cw->counter, 1);
             cw->sync_info.val++;
          }
        cw->counter = counter;
        if (cw->counter)
          {
             ecore_x_sync_counter_inc(cw->counter, 1);
             ecore_x_e_comp_sync_begin_send(win);
             cw->sync_info.val = 1;
          }
     }
}

static void
_e_mod_comp_win_show(E_Comp_Win *cw)
{
   Ecore_X_Window win;

   // if win_hide was showed then immediatly win_show() function is called. case.
   if (cw->defer_hide == 1) cw->defer_hide = 0;
   if (cw->visible) return;
   cw->visible = 1;
   _e_mod_comp_win_configure(cw,
                             cw->hidden.x, cw->hidden.y,
                             cw->w, cw->h,
                             cw->border);
   if ((cw->input_only) || (cw->invalid)) return;

   if (cw->bd)
     _e_mod_comp_win_sync_setup(cw, cw->bd->client.win);
   else
     _e_mod_comp_win_sync_setup(cw, cw->win);

   if (cw->real_hid)
     {
        cw->real_hid = 0;
        e_mod_comp_win_comp_objs_img_deinit(cw);
        if (cw->pixmap)
          {
             ecore_x_pixmap_free(cw->pixmap);
             cw->pixmap = 0;
             cw->pw = 0;
             cw->ph = 0;
             ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
          }
        e_mod_comp_win_comp_objs_xim_free(cw);
        if (cw->redirected)
          {
             cw->redirected = 0;
             cw->pw = 0;
             cw->ph = 0;
          }
     }

   if ((!cw->redirected) || (!cw->pixmap))
     {
        if (!cw->pixmap)
          cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
        if (cw->pixmap)
          ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
        else
          {
             cw->pw = 0;
             cw->ph = 0;
          }
        if ((cw->pw <= 0) || (cw->ph <= 0))
          {
             if (cw->pixmap)
               {
                  ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
                  cw->needpix = 1;
               }
          }
        cw->redirected = 1;

        e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
        e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);

        e_mod_comp_win_comp_objs_img_size_set(cw, cw->pw, cw->ph);
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
     }

   if (cw->dmg_updates >= 1)
     {
        if ((cw->c->gl) && (_comp_mod->conf->texture_from_pixmap))
          {
             if (!cw->pixmap)
               cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);

             if (cw->pixmap)
               {
                  ecore_x_pixmap_geometry_get(cw->pixmap,
                                              NULL, NULL,
                                              &(cw->pw), &(cw->ph));
               }
             else
               {
                  cw->pw = 0;
                  cw->ph = 0;
               }

             if ((cw->pw <= 0) || (cw->ph <= 0))
               {
                  if (cw->pixmap)
                    ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
                  cw->needpix = 1;
               }
             else
               {
                  ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
                  e_mod_comp_win_comp_objs_img_size_set(cw, cw->pw, cw->ph);
                  e_mod_comp_win_comp_objs_img_init(cw);
               }
            }

        if (cw->pixmap)
          {
             cw->defer_hide = 0;
             if (!cw->hidden_override)
               e_mod_comp_win_comp_objs_force_show(cw);
          }
     }
   e_mod_comp_bg_win_handler_show(cw);

   win = e_mod_comp_util_client_xid_get(cw);
   cw->use_dri2 = _e_mod_comp_prop_window_use_dri2_get(win);

   e_mod_comp_win_shape_input_invalid_set(cw->c, 1);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_real_hide(E_Comp_Win *cw)
{
   if (cw->bd)
     {
        _e_mod_comp_win_hide(cw);
        return;
     }
   cw->real_hid = 1;
   _e_mod_comp_win_hide(cw);
}

static void
_e_mod_comp_win_hide(E_Comp_Win *cw)
{
   Ecore_X_Window _w;

   if ((!cw->visible) && (!cw->defer_hide)) return;

   e_mod_comp_win_shape_input_invalid_set(cw->c, 1);

   if ((cw->input_only) || (cw->invalid)) return;

   if (!cw->force)
     {
        cw->defer_hide = 1;
        if (cw->bd)
          {
             if (cw->bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_NORMAL)
               _e_mod_comp_shadow_set(cw, EINA_FALSE);
          }
        cw->c->effect_funcs.win_hide(cw);
        cw->visible = 0;
        return;
     }

   cw->visible = 0;

   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        cw->c->effect_funcs.signal_del(cw,
                                     co->shadow,
                                     "hide,done");
     }

   cw->defer_hide = 0;
   cw->force = 0;
   e_mod_comp_win_comp_objs_hide(cw);

   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (_comp_mod->conf->keep_unmapped)
     {
        goto finish;
     }

   e_mod_comp_win_comp_objs_img_deinit(cw);
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
     }

   e_mod_comp_win_comp_objs_xim_free(cw);

   if (cw->redirected)
     {
        cw->redirected = 0;
        cw->pw = 0;
        cw->ph = 0;
     }

finish:
   _e_mod_comp_win_render_queue(cw);
   _w = e_mod_comp_util_client_xid_get(cw);
   if (_comp_mod->conf->send_flush) ecore_x_e_comp_flush_send(_w);
   if (_comp_mod->conf->send_dump) ecore_x_e_comp_dump_send(_w);
}

static void
_e_mod_comp_win_raise_above(E_Comp_Win *cw,
                            E_Comp_Win *cw2)
{
   Eina_Bool v1, v2;

   v1 = e_mod_comp_util_win_visible_get(cw, EINA_TRUE);

   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append_relative(cw->c->wins,
                                             EINA_INLIST_GET(cw),
                                             EINA_INLIST_GET(cw2));

   v2 = e_mod_comp_util_win_visible_get(cw, EINA_TRUE);
   cw->c->effect_funcs.win_restack(cw, v1, v2);

   e_mod_comp_win_comp_objs_stack_above(cw, cw2);

   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_raise(E_Comp_Win *cw)
{
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append(cw->c->wins, EINA_INLIST_GET(cw));

   e_mod_comp_win_comp_objs_raise(cw);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_lower(E_Comp_Win *cw)
{
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_prepend(cw->c->wins, EINA_INLIST_GET(cw));

   e_mod_comp_win_comp_objs_lower(cw);
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_configure(E_Comp_Win *cw,
                          int x, int y,
                          int w, int h,
                          int border)
{
   Eina_Bool geo_changed = EINA_FALSE;
   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;

   if (!((w == cw->w) && (h == cw->h)))
     {
        cw->w = w;
        cw->h = h;
        cw->needpix = 1;
        cw->dmg_updates = 0;
        geo_changed = EINA_TRUE;

        /* reset dmg_updates of canvas.nocomp.end if this window
         * is waiting for damage events before it is running with
		 * the comp mode. (E_NOCOMP_MODE_DISPOSE exactly)
         */
#if USE_NOCOMP_DISPOSE
        if (_comp_mod->conf->nocomp_fs)
          {
             EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
               {
                  if ((canvas->nocomp.mode == E_NOCOMP_MODE_END) &&
                      (canvas->nocomp.end.cw == cw))
                    {
                       ELBF(ELBT_COMP, 0,
                            e_mod_comp_util_client_xid_get(cw),
                            "NOCOMP dmg:%d nocomp.end.dmg_updates:%d->2",
                            canvas->num, cw->dmg_updates,
                            canvas->nocomp.end.dmg_updates);

                       canvas->nocomp.end.dmg_updates = 2;
                    }
               }
          }
#endif
        if (_comp_mod->conf->use_hwc)
          {
             e_mod_comp_hwcomp_set_resize(cw);
          }
     }

   if (!cw->visible)
     {
        cw->hidden.x = x;
        cw->hidden.y = y;
        cw->border = border;
     }
   else
     {
        if (!((x == cw->x) && (y == cw->y)))
          {
             if (cw->needpix)
               {
                  cw->resizing.x = cw->x;
                  cw->resizing.y = cw->y;
               }

             cw->x = x;
             cw->y = y;
             geo_changed = EINA_TRUE;
             if (!cw->needpix)
               e_mod_comp_win_comp_objs_move(cw, cw->x, cw->y);
          }
        cw->hidden.x = x;
        cw->hidden.y = y;
     }

   if (cw->border != border)
     {
        cw->border = border;
        geo_changed = EINA_TRUE;
        e_mod_comp_win_comp_objs_resize(cw,
                                  cw->pw + (cw->border * 2),
                                  cw->ph + (cw->border * 2));
     }
   cw->hidden.w = cw->w;
   cw->hidden.h = cw->h;
   if (geo_changed)
     {
        e_mod_comp_win_shape_input_invalid_set(cw->c, 1);

        if (_comp_mod->conf->nocomp_fs)
          {
             EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
               {
                  if ((canvas->nocomp.mode == E_NOCOMP_MODE_RUN) &&
                      (canvas->nocomp.cw == cw))
                    {
                       ELBF(ELBT_COMP, 0,
                            e_mod_comp_util_client_xid_get(cw),
                            "NOCOMP_END RESIZE canvas:%d dmg:%d",
                            canvas->num, cw->dmg_updates);

                       e_mod_comp_canvas_nocomp_end(canvas);
                    }
               }
          }
     }
   if ((cw->input_only) || (cw->invalid) || (cw->needpix)) return;
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_damage(E_Comp_Win *cw,
                       int         x,
                       int         y,
                       int         w,
                       int         h,
                       Eina_Bool   dmg)
{
   if ((cw->input_only) || (cw->invalid)) return;
   if ((dmg) && (cw->damage))
     {
        Ecore_X_Region parts;
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        cw->dmg_updates++;
     }

#if USE_NOCOMP_DISPOSE
   if ((_comp_mod->conf->nocomp_fs) &&
       (cw->nocomp))
     {
        EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
          {
             if (canvas->nocomp.mode != E_NOCOMP_MODE_END) continue;
             if (canvas->nocomp.end.cw != cw) continue;
             if (canvas->nocomp.end.dmg_updates > cw->dmg_updates) continue;
             e_mod_comp_canvas_nocomp_dispose(canvas);
          }
     }
#endif

   e_mod_comp_update_add(cw->up, x, y, w, h);
   if (dmg)
     {
        if (cw->counter)
          {
             if (!cw->update_timeout)
               cw->update_timeout = ecore_timer_add
                   (_comp_mod->conf->damage_timeout,
                   _e_mod_comp_win_damage_timeout, cw);
             return;
          }
     }

   if ((dmg) &&
       (cw->dmg_updates <= 1))
     {
        if (!(cw->needpix))
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|SKIP FIRST DMG. bd:%s", "DMG",
                  cw->bd ? "O" : "X");
             return;
          }
     }

   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_reshape(E_Comp_Win *cw)
{
   if (cw->shape_changed) return;
   cw->shape_changed = 1;
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   e_mod_comp_update_add(cw->up, 0, 0, cw->w, cw->h);
   _e_mod_comp_win_render_queue(cw);
}

static Eina_Bool
_e_mod_comp_create(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   Ecore_X_Event_Window_Create *ev = event;
   Eina_List *l;
   E_Comp_Win *cw;
   E_Comp_Canvas *canvas;
   E_Comp *c = _e_mod_comp_find(ev->parent);
   if (!c) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_find(ev->win)) return ECORE_CALLBACK_PASS_ON;
   if (c->win == ev->win) return ECORE_CALLBACK_PASS_ON;
   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (canvas->ee_win == ev->win) return ECORE_CALLBACK_PASS_ON;
     }
   ELBF(ELBT_COMP, 0, ev->win, "%15.15s|", "X_CREATE");
   cw = _e_mod_comp_win_add(c, ev->win);
   if (cw)
     _e_mod_comp_win_configure(cw,
                               ev->x, ev->y,
                               ev->w, ev->h,
                               ev->border);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_destroy(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   Ecore_X_Event_Window_Destroy *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|w:0x%08x|bd:%d cw:%p", "X_DESTROY",
        ev->win, _e_mod_comp_win_is_border(cw), cw);
   if (!cw->c->nocomp && cw->animating) cw->delete_me = 1;
   else _e_mod_comp_win_del(cw);
   return ECORE_CALLBACK_PASS_ON;
}

/* Compositor must check the client window's property that want to be shown.
 * TODO:Compositor will check the property of ECORE_X_ATOM_E_COMP_SYNC_COUNTER for now,
 * but the other properties may also need to check later.
 */
static Eina_Bool
_e_mod_comp_show_request(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void       *event)
{
   Ecore_X_Event_Window_Show_Request *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|w:0x%08x|bd:%d cw:%p", "X_SHOW_REQUEST",
        ev->win, _e_mod_comp_win_is_border(cw), cw);
   _e_mod_comp_win_prop_check(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_show(void *data __UNUSED__,
                 int type   __UNUSED__,
                 void      *event)
{
   Ecore_X_Event_Window_Show *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->visible) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_is_border(cw)) return ECORE_CALLBACK_PASS_ON;
   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|w:0x%08x|bd:%d cw:%p", "X_SHOW",
        ev->win, _e_mod_comp_win_is_border(cw), cw);
   _e_mod_comp_win_show(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_hide(void *data __UNUSED__,
                 int type   __UNUSED__,
                 void      *event)
{
   Ecore_X_Event_Window_Hide *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (!cw->visible) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_is_border(cw)) return ECORE_CALLBACK_PASS_ON;
   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|w:0x%08x|bd:%d cw:%p", "X_HIDE",
        ev->win, _e_mod_comp_win_is_border(cw), cw);
   _e_mod_comp_win_real_hide(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_reparent(void *data __UNUSED__,
                     int   type __UNUSED__,
                     void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|w:0x%08x|bd:%d cw:%p TO parent:0x%08x", "X_REPARENT",
        ev->win, _e_mod_comp_win_is_border(cw), cw, ev->parent);
   if (ev->parent != cw->c->man->root)
     {
        ELBF(ELBT_COMP, 1, e_mod_comp_util_client_xid_get(cw),
             "DEL 0x%08x", ev->win);
        _e_mod_comp_win_del(cw);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_configure(void *data __UNUSED__,
                      int   type __UNUSED__,
                      void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   Eina_Bool need_shape_merge = EINA_FALSE;
   if (!cw) return ECORE_CALLBACK_PASS_ON;

   ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|%4d %4d %3dx%3d", "X_CONFIGURE",
        ev->x, ev->y, ev->w, ev->h);

   if (ev->abovewin == 0)
     {
        if (EINA_INLIST_GET(cw)->prev)
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|", "LOWER");
             SLOGI("%15.15s|0x%08x", "LOWER", e_mod_comp_util_client_xid_get(cw));

             _e_mod_comp_win_lower(cw);
             need_shape_merge = EINA_TRUE;
          }
     }
   else
     {
        E_Comp_Win *cw2 = _e_mod_comp_win_find(ev->abovewin);
        if (cw2)
          {
             E_Comp_Win *cw3 = (E_Comp_Win *)(EINA_INLIST_GET(cw)->prev);
             if (cw3 != cw2)
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|above:0x%08x", "RAISE_ABOVE",
                       e_mod_comp_util_client_xid_get(cw2));
                  SLOGI("(0x%08x)%15.15s|above:0x%08x", e_mod_comp_util_client_xid_get(cw), "RAISE_ABOVE",
                       e_mod_comp_util_client_xid_get(cw2));

                  _e_mod_comp_win_raise_above(cw, cw2);
                  need_shape_merge = EINA_TRUE;
               }
          }
     }

   if (need_shape_merge)
     {
        e_mod_comp_win_shape_input_invalid_set(cw->c, 1);
        _e_mod_comp_win_render_queue(cw);
     }

#if OPTIMIZED_HWC
        if (_comp_mod->conf->use_hwc && cw->hwc.resize_pending)
          {
             cw->hwc.resize_pending = 0;
          }
#endif

  if (!((cw->x == ev->x) && (cw->y == ev->y)) &&
      ((cw->w == ev->w) && (cw->h == ev->h)) &&
      _e_mod_comp_win_is_border(cw))
    {
       return ECORE_CALLBACK_PASS_ON;
    }

   if (!((cw->x == ev->x) && (cw->y == ev->y) &&
         (cw->w == ev->w) && (cw->h == ev->h) &&
         (cw->border == ev->border)))
     {
        _e_mod_comp_win_configure(cw,
                                  ev->x, ev->y,
                                  ev->w, ev->h,
                                  ev->border);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_stack(void *data __UNUSED__,
                  int type __UNUSED__,
                  void *event)
{
   Ecore_X_Event_Window_Stack *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (ev->detail == ECORE_X_WINDOW_STACK_ABOVE) _e_mod_comp_win_raise(cw);
   else _e_mod_comp_win_lower(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_prop_effect_state(Ecore_X_Event_Window_Property *ev __UNUSED__)
{
   E_Comp *c;
   unsigned int val;
   int ret;

   c = e_mod_comp_util_get();
   E_CHECK_RETURN(c, 0);

   ret = ecore_x_window_prop_card32_get
           (c->man->root, ATOM_EFFECT_ENABLE, &val, 1);
   E_CHECK_RETURN((ret >= 0), 0);

   if (val != 0)
     {
        c->animatable = EINA_TRUE;
        c->keyboard_effect = EINA_TRUE;
        if (_comp_mod->conf->default_window_effect != 1)
          {
             _comp_mod->conf->default_window_effect = 1;
             e_config_domain_save("module.comp-tizen",
                                  _comp_mod->conf_edd,
                                  _comp_mod->conf);

          }
     }
   else
     {
        c->animatable = EINA_FALSE;
        c->keyboard_effect = EINA_FALSE;
        if (_comp_mod->conf->default_window_effect != 0)
          {
             _comp_mod->conf->default_window_effect = 0;
             e_config_domain_save("module.comp-tizen",
                                  _comp_mod->conf_edd,
                                  _comp_mod->conf);
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_window_effect_state(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   Ecore_X_Window w = 0;
   E_CHECK_RETURN(ev, 0);

   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   E_CHECK_RETURN(cw->eff_type, 0);

   w = e_mod_comp_util_client_xid_get(cw);
   cw->c->effect_funcs.state_setup(cw->eff_type, w);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_effect_style(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   Ecore_X_Window w = 0;
   E_CHECK_RETURN(ev, 0);

   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   E_CHECK_RETURN(cw->eff_type, 0);

   w = e_mod_comp_util_client_xid_get(cw);
   cw->c->effect_funcs.style_setup(cw->eff_type, w);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_opacity(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   unsigned int val = 0;
   int ret = -1;

   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   ret = ecore_x_window_prop_card32_get
           (ev->win, ECORE_X_ATOM_NET_WM_WINDOW_OPACITY, &val, 1);
   E_CHECK_RETURN((ret > 0), 0);

   cw->opacity = (val >> 24);

   Eina_List *l;
   E_Comp_Object *co;
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        evas_object_color_set(co->shadow,
                              cw->opacity,
                              cw->opacity,
                              cw->opacity,
                              cw->opacity);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_sync_counter(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   cw = _e_mod_comp_win_find(ev->win);
   if (!cw) cw = _e_mod_comp_border_client_find(ev->win);

   Ecore_X_Sync_Counter counter = ecore_x_e_comp_sync_counter_get(ev->win);
   if (cw)
     {
        if (cw->counter != counter)
          {
             Ecore_X_Window _w = e_mod_comp_util_client_xid_get(cw);
             if (cw->counter)
               {
                  ecore_x_e_comp_sync_cancel_send(_w);
                  ecore_x_sync_counter_inc(cw->counter, 1);
                  cw->sync_info.val++;
               }
             cw->counter = counter;
             if (cw->counter)
               {
                  ecore_x_sync_counter_inc(cw->counter, 1);
                  ecore_x_e_comp_sync_begin_send(_w);
                  cw->sync_info.val = 1;
               }
          }
     }
   else
     {
        if (counter) ecore_x_sync_counter_inc(counter, 1);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_window_use_dri2_get(Ecore_X_Window win)
{
   unsigned int val = 0;
   int ret = ecore_x_window_prop_card32_get(win,
                                            ATOM_X_WIN_USE_DRI2,
                                            &val,
                                            1);
   if (ret == -1) return EINA_FALSE;

   return val ? EINA_TRUE : EINA_FALSE;
}

static Eina_Bool
_e_mod_comp_prop_use_dri2(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   Ecore_X_Window win;
   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   win = e_mod_comp_util_client_xid_get(cw);
   cw->use_dri2 = _e_mod_comp_prop_window_use_dri2_get(win);

   return EINA_TRUE;
}

/* TODO: change getting x prop code to e border event */
static void
_e_mod_comp_prop_illume_win_state(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = _e_mod_comp_border_client_find(ev->win);
   E_CHECK(cw);

   unsigned int state = ecore_x_e_illume_window_state_get(ev->win);

   if (state == ECORE_X_ILLUME_WINDOW_STATE_NORMAL)
     {
        _e_mod_comp_shadow_set(cw, EINA_FALSE);
        if (_comp_mod->conf->use_hwc)
          e_mod_comp_hwcomp_set_resize(cw);
     }
   else if ((state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) ||
            (state == ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU))
     {
        _e_mod_comp_shadow_set(cw, EINA_TRUE);
        if (_comp_mod->conf->use_hwc)
          e_mod_comp_hwcomp_set_resize(cw);
     }
}

static Eina_Bool
_e_mod_comp_prop_transparent_rect(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   _e_mod_comp_win_transparent_rect_update(cw);

   return EINA_TRUE;
}

// for interfacing of 3D setreoscopic mode with compositor
/* one can change the property as below
   unsigned int view_mode;
   Atom StereoMode;
   StereoMode = XInternAtom(disp, "_E_COMP_3D_APP_WIN", False);
   if (StereoMode == None)
      return ;
   XChangeProperty(disp, win, StereoMode, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&view_mode, 1);
*/
static Eina_Bool
_e_mod_comp_prop_3d_win_mode(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   unsigned int view_mode = 0;
   int ret = 0;

   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
       cw = _e_mod_comp_border_client_find(ev->win);
       E_CHECK_RETURN(cw, 0);
     }

   //retrieving view mode
   ret = ecore_x_window_prop_card32_get
                (ev->win, ATOM_CM_3D_APP_WIN, &view_mode , 1);
   if (view_mode < 4)
     {
       cw->win3d = EINA_TRUE;
       // set XRandr RROutput property according to viewmode
       _e_mod_comp_win_rr_output_prop_set(ev->win, view_mode);
     }

   return EINA_TRUE;
}

//X client message handling for stereo mode change (from 2D Apps)
static void
_e_mod_comp_msg_stereo_mode_changed(Ecore_X_Event_Client_Message *ev)
{
   unsigned int res = 0, view_mode = 0;
   Ecore_X_Window win;

   res = ev->data.l[0];
   win = ev->data.l[1];

   switch(res)
     {
       case 0:
         view_mode = E_COMP_STEREO_MONO;
         break;

       case 1:
         view_mode = E_COMP_STEREO_INTERLACED;
         break;

       case 2:
         view_mode = E_COMP_STEREO_VERTICAL;
         break;

       case 3:
         view_mode = E_COMP_STEREO_HORIZONTAL;
         break;

       default:
         break;
     }

   if (view_mode < 4)
     {
       // set XRandr RROutput property according to viewmode
       _e_mod_comp_win_rr_output_prop_set(win, view_mode);
     }
}

static void
_e_mod_comp_window_role_handle(E_Comp_Win *cw)
{
   E_CHECK(cw);
   Ecore_X_Window win;
   E_Comp_Object *co;
   Eina_List *l;
   char *hints = NULL;
   char *token = NULL;

   win = e_mod_comp_util_client_xid_get(cw);

   hints = ecore_x_window_prop_string_get(win, ECORE_X_ATOM_WM_WINDOW_ROLE);

   if (hints)
     token = strtok(hints, " ,");
   while (token != NULL)
     {
        if (!strncmp (token, "no-effect", strlen("no-effect")))
          {
             cw->c->effect_funcs.state_set(cw->eff_type, 0);
          }
        else if (!strncmp(token, "no-dim", strlen("no-dim")))
          {
             char* group = e_mod_comp_policy_win_shadow_group_get(cw);
             if (group && !strcmp(group, "dialog"))
               {
                  EINA_LIST_FOREACH(cw->objs, l, co)
                    {
                       if (!co) continue;
                       if ((co->shadow) && (co->img))
                         {
                            edje_object_signal_emit(co->shadow, "e,state,dim,off", "e");
                         }
                    }
               }
          }
        token = strtok (NULL, " ,");
     }

   free(hints);
}


static Eina_Bool
_e_mod_comp_prop_window_role(Ecore_X_Event_Window_Property *ev)
{

   E_Comp_Win *cw = NULL;
   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   _e_mod_comp_window_role_handle(cw);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_property(void *data __UNUSED__,
                     int type __UNUSED__,
                     void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;
   Ecore_X_Atom a = 0;
   if (!ev) return ECORE_CALLBACK_PASS_ON;
   if (!ev->atom) return ECORE_CALLBACK_PASS_ON;
   if (!e_mod_comp_atoms_name_get(ev->atom)) return ECORE_CALLBACK_PASS_ON;
   a = ev->atom;

   ELBF(ELBT_COMP_RENDER, 0, ev->win,
        "%15.15s|atom:%s", "X_PROP",
        e_mod_comp_atoms_name_get(a));

   if      (a == ECORE_X_ATOM_E_COMP_SYNC_COUNTER  ) _e_mod_comp_prop_sync_counter(ev);
   else if (a == ATOM_X_WIN_USE_DRI2               ) _e_mod_comp_prop_use_dri2(ev);
   else if (a == ATOM_EFFECT_ENABLE                ) _e_mod_comp_prop_effect_state(ev);
   else if (a == ATOM_WINDOW_EFFECT_ENABLE         ) _e_mod_comp_prop_window_effect_state(ev);
   else if (a == ATOM_WINDOW_EFFECT_TYPE           ) _e_mod_comp_prop_effect_style(ev);
   else if (a == ECORE_X_ATOM_NET_WM_WINDOW_OPACITY) _e_mod_comp_prop_opacity(ev);
   else if (a == ECORE_X_ATOM_WM_CLASS             ) e_mod_comp_win_type_handler_prop(ev);
   else if (a == ATOM_NET_CM_WINDOW_BACKGROUND     ) e_mod_comp_bg_win_handler_prop(ev);
   else if (a == ATOM_CM_LOG                       ) e_mod_comp_debug_prop_handle(ev);
   else if (a == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE) _e_mod_comp_prop_illume_win_state(ev);
   else if (a == ATOM_CM_TRANSPARENT_RECT          ) _e_mod_comp_prop_transparent_rect(ev);
   else if (a == ECORE_X_ATOM_WM_WINDOW_ROLE       ) _e_mod_comp_prop_window_role(ev);
   else if (a == ATOM_CM_3D_APP_WIN                       ) _e_mod_comp_prop_3d_win_mode(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_msg_sync_draw_done(Ecore_X_Event_Client_Message *ev)
{
   E_Comp_Win *cw = NULL;
   int v = 0, w = 0, h = 0;
   cw = _e_mod_comp_border_client_find(ev->data.l[0]);
   v = ev->data.l[1];
   w = ev->data.l[2];
   h = ev->data.l[3];
   if (cw)
     {
        cw->sync_info.version = v;
        if (!cw->bd)
          {
             ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|v:%d %dx%d ERR NO cw->bd", "DRAW_DONE", v, w, h);
             return EINA_FALSE;
          }
        if ((Ecore_X_Window)(ev->data.l[0]) != cw->bd->client.win)
          {
             ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|v:%d %dx%d ERR != 0x%08x", "DRAW_DONE",
                  v, w, h, ev->data.l[0]);
             return EINA_FALSE;
          }
     }
   else
     {
        cw = _e_mod_comp_win_find(ev->data.l[0]);
        if (!cw || (ev->data.l[0] != (int)cw->win))
          {
             Ecore_X_Sync_Counter counter = ecore_x_e_comp_sync_counter_get(ev->win);
             ecore_x_e_comp_sync_cancel_send(ev->win);
             if (counter) ecore_x_sync_counter_inc(counter, 1);

             /*
              * FIXME : Is that a right aproach?
              */
             if (_comp_mod->conf->use_hwc)
               {
                  Eina_List *l, *ll;
                  E_Comp *c;
                  E_Comp_Canvas *canvas;
                  E_Comp_HWComp *hwcomp;

                  EINA_LIST_FOREACH (compositors, l, c)
                    {
                       if (!c) return EINA_FALSE;
                       EINA_LIST_FOREACH (c->canvases, ll, canvas)
                         {
                            if (!canvas) return EINA_FALSE;
                            if (!canvas->hwcomp) return EINA_FALSE;

                            hwcomp = canvas->hwcomp;
                            if (ev->data.l[0] == canvas->ee_win)
                              {
                                 if (c->lock.locked)
                                   {
                                      if (ecore_evas_manual_render_get(canvas->ee) == EINA_FALSE)
                                        ecore_evas_manual_render_set(canvas->ee, 1);
                                   }
                                 else
                                   e_mod_comp_hwcomp_update_composite (hwcomp);
                              }
                         }
                    }
               }

             ELBF(ELBT_COMP_RENDER, 0, ev->data.l[0],
                  "%15.15s|v:%d %dx%d ERR NO cw", "DRAW_DONE", v, w, h);
             return EINA_FALSE;
          }
     }
   if (!cw->counter)
     {
        cw->counter = ecore_x_e_comp_sync_counter_get(e_mod_comp_util_client_xid_get(cw));
        if (cw->counter)
          {
             ecore_x_sync_counter_inc(cw->counter, 1);
             ecore_x_e_comp_sync_begin_send(e_mod_comp_util_client_xid_get(cw));
             cw->sync_info.val = 1;
             cw->sync_info.done_count = 1;
          }
        ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s|v:%d %dx%d WARN NO counter", "DRAW_DONE", v, w, h);
        return EINA_FALSE;
     }

   if ((_comp_mod->conf->efl_sync) &&
       (_comp_mod->conf->nocomp_fs) &&
       (cw->nocomp))
     {
        ecore_x_sync_counter_inc(cw->counter, 1);
        cw->update = 0;
        cw->drawme = 0;
        cw->sync_info.done_count++;
        cw->sync_info.val++;
        ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s|v:%d %dx%d NOCOMP INC", "DRAW_DONE", v, w, h);
        return EINA_TRUE;
     }

   if (!cw->update)
     {
        if (cw->update_timeout)
          {
             ecore_timer_del(cw->update_timeout);
             cw->update_timeout = NULL;
          }
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }

   cw->drawme = 1;
   cw->sync_info.done_count++;

   ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|v:%d %dx%d", "DRAW_DONE", v, w, h);
   _e_mod_comp_win_render_queue(cw);
   return EINA_TRUE;
}

static void
_e_mod_comp_active_request_handle(Ecore_X_Event_Client_Message *ev)
{
   E_Comp_Win *cw = NULL;
   cw = _e_mod_comp_win_find(ev->win);
   if (!cw) cw = _e_mod_comp_border_client_find(ev->win);

   if (!cw) return;
   if (!cw->c->animatable) return;

   if (TYPE_HOME_CHECK(cw))
     {
        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|", "HOME ACTIVE");

        cw->activate = EINA_TRUE;
     }
}

static Eina_Bool
_e_mod_comp_hib_leave(Ecore_X_Event_Client_Message *ev)
{
   E_Comp *c;
   Config *cfg;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN((ev->data.l[0] == 0), 0);
   E_CHECK_RETURN(_comp_mod, 0);

   c = _e_mod_comp_find(ev->win);
   E_CHECK_RETURN(c, 0);

   cfg = e_config_domain_load("module.comp-tizen",
                              _comp_mod->conf_edd);
   E_CHECK_RETURN(cfg, 0);

   if (cfg->default_window_effect != c->animatable)
     {
        if (cfg->default_window_effect)
          {
             c->animatable = EINA_TRUE;
             _comp_mod->conf->default_window_effect = 1;
          }
        else
          {
             c->animatable = EINA_FALSE;
             _comp_mod->conf->default_window_effect = 0;
          }
        ecore_x_window_prop_card32_set
           (c->man->root, ATOM_EFFECT_ENABLE,
           (unsigned int *)(&(_comp_mod->conf->default_window_effect)), 1);
     }

   if (cfg->shadow_file) eina_stringshare_del(cfg->shadow_file);
   if (cfg->shadow_style) eina_stringshare_del(cfg->shadow_style);
   if (cfg->effect_file) eina_stringshare_del(cfg->effect_file);

   if (cfg->effect_policy_unknown) eina_stringshare_del(cfg->effect_policy_unknown);
   if (cfg->effect_policy_desktop) eina_stringshare_del(cfg->effect_policy_desktop);
   if (cfg->effect_policy_dock) eina_stringshare_del(cfg->effect_policy_dock);
   if (cfg->effect_policy_toolbar) eina_stringshare_del(cfg->effect_policy_toolbar);
   if (cfg->effect_policy_menu) eina_stringshare_del(cfg->effect_policy_menu);
   if (cfg->effect_policy_utility) eina_stringshare_del(cfg->effect_policy_utility);
   if (cfg->effect_policy_splash) eina_stringshare_del(cfg->effect_policy_splash);
   if (cfg->effect_policy_dialog) eina_stringshare_del(cfg->effect_policy_dialog);
   if (cfg->effect_policy_normal) eina_stringshare_del(cfg->effect_policy_normal);
   if (cfg->effect_policy_videocall) eina_stringshare_del(cfg->effect_policy_videocall);
   if (cfg->effect_policy_dropdown_menu) eina_stringshare_del(cfg->effect_policy_dropdown_menu);
   if (cfg->effect_policy_popup_menu) eina_stringshare_del(cfg->effect_policy_popup_menu);
   if (cfg->effect_policy_tooltip) eina_stringshare_del(cfg->effect_policy_tooltip);
   if (cfg->effect_policy_notification) eina_stringshare_del(cfg->effect_policy_notification);
   if (cfg->effect_policy_combo) eina_stringshare_del(cfg->effect_policy_combo);
   if (cfg->effect_policy_dnd) eina_stringshare_del(cfg->effect_policy_dnd);
   if (cfg->effect_policy_menuscreen) eina_stringshare_del(cfg->effect_policy_menuscreen);
   if (cfg->effect_policy_quickpanel_base) eina_stringshare_del(cfg->effect_policy_quickpanel_base);
   if (cfg->effect_policy_quickpanel) eina_stringshare_del(cfg->effect_policy_quickpanel);
   if (cfg->effect_policy_taskmanager) eina_stringshare_del(cfg->effect_policy_taskmanager);
   if (cfg->effect_policy_livemagazine) eina_stringshare_del(cfg->effect_policy_livemagazine);
   if (cfg->effect_policy_lockscreen) eina_stringshare_del(cfg->effect_policy_lockscreen);
   if (cfg->effect_policy_indicator) eina_stringshare_del(cfg->effect_policy_indicator);
   if (cfg->effect_policy_tickernoti) eina_stringshare_del(cfg->effect_policy_tickernoti);
   if (cfg->effect_policy_debugging_info) eina_stringshare_del(cfg->effect_policy_debugging_info);
   if (cfg->effect_policy_apptray) eina_stringshare_del(cfg->effect_policy_apptray);
   if (cfg->effect_policy_mini_apptray) eina_stringshare_del(cfg->effect_policy_mini_apptray);
   if (cfg->effect_policy_volume) eina_stringshare_del(cfg->effect_policy_volume);
   if (cfg->effect_policy_background) eina_stringshare_del(cfg->effect_policy_background);
   if (cfg->effect_policy_isf_keyboard) eina_stringshare_del(cfg->effect_policy_isf_keyboard);
   if (cfg->effect_policy_isf_sub) eina_stringshare_del(cfg->effect_policy_isf_sub);
   if (cfg->effect_policy_setup_wizard) eina_stringshare_del(cfg->effect_policy_setup_wizard);
   if (cfg->effect_policy_app_popup) eina_stringshare_del(cfg->effect_policy_app_popup);

   free(cfg);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_xv_rot_begin(Ecore_X_Event_Client_Message* ev)
{
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   E_CHECK_RETURN(ev, 0);

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if ((canvas->zr) && (canvas->xv_ready[0]))
               canvas->comp->effect_funcs.zone_rotation_begin(canvas->zr);
             break;
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_xv_rot_end(Ecore_X_Event_Client_Message *ev)
{
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   E_CHECK_RETURN(ev, 0);

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if (canvas->zr)
               {
                  if (canvas->xv_ready[0])
                    {
                       canvas->xv_ready[2] = EINA_TRUE;
                       if (!canvas->xv_ready[1])
                         break;
                    }
                  Eina_Bool res = canvas->comp->effect_funcs.zone_rotation_end(canvas->zr);
                  if (res)
                    canvas->comp->effect_funcs.zone_rotation_do(canvas->zr);
                  else
                    canvas->comp->effect_funcs.zone_rotation_clear(canvas->zr);
               }
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_message(void *data __UNUSED__,
                    int   type __UNUSED__,
                    void *event)
{
   Ecore_X_Event_Client_Message *ev;
   Ecore_X_Atom t;
   E_Comp *c;
   ev = (Ecore_X_Event_Client_Message *)event;
   c = e_mod_comp_util_get();

   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN((ev->format == 32), 0);

   t = ev->message_type;

   if      (t == ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE         ) _e_mod_comp_msg_sync_draw_done(ev);
   else if (t == ECORE_X_ATOM_NET_ACTIVE_WINDOW            ) _e_mod_comp_active_request_handle(ev);
   else if (t == ATOM_CM_LOCK_SCREEN                        ) e_mod_comp_screen_lock_handler_message(ev);
   else if (t == ATOM_X_HIBERNATION_STATE                   ) _e_mod_comp_hib_leave(ev);
   else if (t == ATOM_IMAGE_LAUNCH_FILE                     ) c->effect_funcs.image_launch_handler_message(ev);
   else if (t == ATOM_XV_BYPASS_DONE0                       ) _e_mod_comp_xv_rot_begin(ev);
   else if (t == ATOM_XV_BYPASS_DONE1                       ) _e_mod_comp_xv_rot_end(ev);
   else if (t == ATOM_CM_MSG_STEREO_MODE           ) _e_mod_comp_msg_stereo_mode_changed(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_shape(void *data __UNUSED__,
                  int type __UNUSED__,
                  void *event)
{
   Ecore_X_Event_Window_Shape *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (ev->type != ECORE_X_SHAPE_BOUNDING) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_reshape(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_damage(void *data __UNUSED__,
                   int type __UNUSED__,
                   void *event)
{
   Ecore_X_Event_Damage *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_damage_find(ev->damage);

   if (!cw)
     {
        ELBF(ELBT_COMP_RENDER, 0, ev->drawable,
             "%15.15s|%4d %4d %3dx%3d ERR NO cw (ev->damage)", "DMG",
             ev->area.x, ev->area.y, ev->area.width, ev->area.height);
        cw = _e_mod_comp_border_client_find(ev->drawable);

        if (!cw)
          {
             ELBF(ELBT_COMP_RENDER, 0, ev->drawable,
                  "%15.15s|%4d %4d %3dx%3d ERR NO cw (ev->drawable)", "DMG",
                  ev->area.x, ev->area.y, ev->area.width, ev->area.height);
             return ECORE_CALLBACK_PASS_ON;
          }
     }

   ELBF(ELBT_COMP_RENDER, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|%4d %4d %3dx%3d", "DMG",
        ev->area.x, ev->area.y, ev->area.width, ev->area.height);

   _e_mod_comp_win_damage(cw,
                          ev->area.x, ev->area.y,
                          ev->area.width, ev->area.height, 1);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_damage_win(void *data __UNUSED__,
                       int   type __UNUSED__,
                       void *event)
{
   Ecore_X_Event_Window_Damage *ev = event;
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if (ev->win == canvas->ee_win)
               {
                  // expose on comp win - init win or some other bypass win did it
                  _e_mod_comp_render_queue(c);
                  break;
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_randr(void *data  __UNUSED__,
                  int type    __UNUSED__,
                  void *event __UNUSED__)
{
   Eina_List *l;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   ELB(ELBT_COMP, "E_CONTNR_RESIZE", 0);

   if (_comp_mod->conf->canvas_per_zone)
     return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        canvas = eina_list_data_get(c->canvases);
        if (!canvas) continue;
        ecore_evas_resize(canvas->ee,
                          c->man->w,
                          c->man->h);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_add(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   E_Event_Border_Add *ev = event;
   Eina_List *l;
   E_Comp_Win *cw;
   E_Comp_Canvas *canvas;
   E_Comp* c = _e_mod_comp_find(ev->border->zone->container->manager->root);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x", "BD_ADD", ev->border->win);

   if (!c) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_find(ev->border->win)) return ECORE_CALLBACK_PASS_ON;
   if (c->win == ev->border->win) return ECORE_CALLBACK_PASS_ON;
   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (canvas->ee_win == ev->border->win) return ECORE_CALLBACK_PASS_ON;
     }
   cw = _e_mod_comp_win_add(c, ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;

   _e_mod_comp_win_configure
      (cw, ev->border->x, ev->border->y,
      ev->border->w, ev->border->h,
      ev->border->client.initial_attributes.border);

   if (ev->border->internal && ev->border->visible)
     _e_mod_comp_win_show(cw);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_del(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   E_Event_Border_Remove *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x cw:%p", "BD_DEL", ev->border->win, cw);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->bd == ev->border)
     {
        if (e_mod_comp_policy_app_close_check(cw))
          cw->c->effect_funcs.signal_add(cw, NULL, "e,state,visible,off,noeffect", "e");

        _e_mod_comp_object_del(cw, ev->border);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_show(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Show *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x cw:%p", "BD_SHOW", ev->border->win, cw);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_show(cw);

   unsigned int state = ev->border->client.illume.win_state.state;
   if (state == ECORE_X_ILLUME_WINDOW_STATE_NORMAL)
     _e_mod_comp_shadow_set(cw, EINA_FALSE);
   else if ((state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) ||
            (state == ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU))
     _e_mod_comp_shadow_set(cw, EINA_TRUE);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_hide(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Hide *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x cw:%p", "BD_HIDE", ev->border->win, cw);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (!cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_hide(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_move(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Move *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|cw[%dx%d] hidden[%d,%d] ev[%d,%d]", "BD_MOVE",
        cw->x, cw->y, cw->hidden.x, cw->hidden.y,
        ev->border->x, ev->border->y);
   SLOGI("%15.15s|(0x%08x)cw[%dx%d] hidden[%d,%d] ev[%d,%d]", "BD_MOVE", ev->border->client.win,
        cw->x, cw->y, cw->hidden.x, cw->hidden.y,
        ev->border->x, ev->border->y);

   if (_comp_mod->conf->use_hwc)
     {
        if ((STATE_INSET_CHECK(cw)) ||
             CLASS_ICONIC_CHECK(cw))
          cw->hwc.geo_changed = 3;
     }

   if (!((cw->x == ev->border->x) &&
         (cw->y == ev->border->y)) &&
       (cw->visible))
     {
        _e_mod_comp_win_configure
          (cw, ev->border->x, ev->border->y,
          ev->border->w, ev->border->h, 0);
#if OPTIMIZED_HWC
        if (_comp_mod->conf->use_hwc && !cw->hwc.resize_pending)
          {
             cw->hwc.resize_pending = 1;
          }
#endif
     }
   else if (!((cw->hidden.x == ev->border->x) &&
              (cw->hidden.y == ev->border->y)) &&
            (!cw->visible))
     {
        _e_mod_comp_win_configure
          (cw, ev->border->x, ev->border->y,
          ev->border->w, ev->border->h, 0);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_resize(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void      *event)
{
   E_Event_Border_Resize *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|%4dx%4d", "BD_RESIZE",
        ev->border->w, ev->border->h);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if ((cw->w == ev->border->w) && (cw->h == ev->border->h))
     return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_configure
     (cw, ev->border->x, ev->border->y,
     ev->border->w, ev->border->h, cw->border);

#if OPTIMIZED_HWC
   if (_comp_mod->conf->use_hwc)
     {
        cw->hwc.resize_pending =  1;
     }
#endif

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_iconify(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void      *event)
{
   E_Event_Border_Iconify *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x", "BD_ICONIFY", ev->border->win);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_uniconify(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void      *event)
{
   E_Event_Border_Uniconify *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x", "BD_UNICONIFY", ev->border->win);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_focus_in(void *data __UNUSED__,
                        int type   __UNUSED__,
                        void      *event)
{
   E_Event_Border_Focus_In *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x", "BD_FOCUS_IN", ev->border->win);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_focus_out(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void      *event)
{
   E_Event_Border_Focus_Out *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);

   ELBF(ELBT_COMP, 0, ev->border->client.win,
        "%15.15s|bd:0x%08x", "BD_FOCUS_OUT", ev->border->win);

   if (!cw) return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_stack(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void      *event)
{
   E_Event_Border_Stack *ev = event;
   E_Comp_Win *cw, *cw2 = NULL;
   if (!ev->border) return ECORE_CALLBACK_PASS_ON;

   cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;

   if (ev->stack)
     {
        cw2 = _e_mod_comp_win_find(ev->stack->win);
        /* there is no cw yet, thus put off doing it. */
        if (!cw2) return ECORE_CALLBACK_PASS_ON;
     }

   if (ev->type == E_STACKING_BELOW)
     {
        if (cw2)
          {
             /* LOWER BELOW
              * if user selects back icon of the application, then this event
              * is occured first. thus we use this event for showing application
              * hide effect.
              * visible -> invisible
              */

             /* check visibility of cw before and after */
             Eina_Bool v1 = EINA_FALSE, v2 = EINA_FALSE;
             E_Comp *c = cw->c;
             E_Comp_Win *cw3 = NULL;

             v1 = e_mod_comp_util_win_visible_get(cw, EINA_TRUE);
             if (v1)
               {
                  cw3 = (E_Comp_Win *)(EINA_INLIST_GET(cw)->prev);

                  c->wins = eina_inlist_remove(c->wins, EINA_INLIST_GET(cw));
                  c->wins = eina_inlist_prepend_relative(c->wins,
                                                         EINA_INLIST_GET(cw),
                                                         EINA_INLIST_GET(cw2));

                  v2 = e_mod_comp_util_win_visible_get(cw, EINA_TRUE);

                  /* restore c->wins */
                  if (cw3)
                    {
                       c->wins = eina_inlist_remove(c->wins, EINA_INLIST_GET(cw));
                       c->wins = eina_inlist_append_relative(c->wins,
                                                             EINA_INLIST_GET(cw),
                                                             EINA_INLIST_GET(cw3));
                    }
                  else
                    {
                       c->wins = eina_inlist_remove(c->wins, EINA_INLIST_GET(cw));
                       c->wins = eina_inlist_prepend(c->wins, EINA_INLIST_GET(cw));
                    }
               }

             /* TODO: try to check visibility of cw without changing c->wins */
             //v2 = e_mod_comp_util_win_visible_below_get(cw, cw2);

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|LOWER_BELOW 0x%08x v:%d->%d", "BD_STACK",
                  e_mod_comp_util_client_xid_get(cw2), v1, v2);

             /* do hide effect if visibility of cw will be changed to invisible from visible */
             if ((v1) && (!v2))
               cw->c->effect_funcs.win_restack(cw, EINA_TRUE, EINA_FALSE);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_key_down(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void      *event)
{
   Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
   E_CHECK_RETURN(ev, 0);

   if ((!strcmp(ev->keyname, "Home")) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_ALT))
     {
        if (_comp_mod)
          {
             _e_mod_config_free(_comp_mod->module);
             _e_mod_config_new(_comp_mod->module);
             e_config_save();
             e_module_disable(_comp_mod->module);
             e_config_save();
             e_sys_action_do(E_SYS_RESTART, NULL);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

//////////////////////////////////////////////////////////////////////////
static Evas *
_e_mod_comp_evas_get_func(void          *data,
                          E_Manager *man __UNUSED__)
{
   E_Comp *c = (E_Comp *)data;
   Eina_List *l;
   E_Comp_Canvas *canvas;
   E_Zone *z;
   E_CHECK_RETURN(c, 0);

   z = e_util_zone_current_get(c->man);
   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!_comp_mod->conf->canvas_per_zone)
          return canvas->evas;
        else if ((canvas->zone) && (canvas->zone == z))
          return canvas->evas;
     }
   return NULL;
}

static void
_e_mod_comp_update_func(void          *data,
                        E_Manager *man __UNUSED__)
{
   E_Comp *c = (E_Comp *)data;
   E_CHECK(c);
   _e_mod_comp_render_queue(c);
}

static E_Manager_Comp_Source *
_e_mod_comp_src_get_func(void           *data __UNUSED__,
                         E_Manager      *man __UNUSED__,
                         Ecore_X_Window  win)
{
   return (E_Manager_Comp_Source *)_e_mod_comp_win_find(win);
}

static const Eina_List *
_e_mod_comp_src_list_get_func(void          *data,
                              E_Manager *man __UNUSED__)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Win *cw;
   Eina_List *l;
   E_Comp_Object *co;
   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(c->wins, 0);

   // source list should always be updated if multi-canvas is enabled.
   if (_comp_mod->conf->canvas_per_zone)
     {
        if (c->wins_invalid) c->wins_invalid = 0;
        if (c->wins_list)
          {
             eina_list_free(c->wins_list);
             c->wins_list = NULL;
          }
        E_Zone *z = e_util_zone_current_get(c->man);
        EINA_INLIST_FOREACH(c->wins, cw)
          {
             EINA_LIST_FOREACH(cw->objs, l, co)
               {
                  if ((co->shadow) && (co->img) && (z == co->zone))
                    c->wins_list = eina_list_append(c->wins_list, cw);
               }
          }
     }
   else
     {
        if (c->wins_invalid)
          {
             c->wins_invalid = 0;
             if (c->wins_list)
               {
                  eina_list_free(c->wins_list);
                  c->wins_list = NULL;
               }
             EINA_INLIST_FOREACH(c->wins, cw)
               {
                  EINA_LIST_FOREACH(cw->objs, l, co)
                    {
                       if ((co->shadow) && (co->img))
                         c->wins_list = eina_list_append(c->wins_list, cw);
                    }
               }
          }
     }
   return c->wins_list;
}

static Evas_Object *
_e_mod_comp_src_image_get_func(void                  *data,
                               E_Manager *man         __UNUSED__,
                               E_Manager_Comp_Source *src)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   Eina_List *l;
   E_Comp_Object *co;
   E_Zone *z;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);

   if (cw->bd) z = cw->bd->zone;
   else z = e_util_zone_current_get(c->man);
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!_comp_mod->conf->canvas_per_zone)
          return co->img;
        else if ((co->zone) && (co->zone == z))
          return co->img;
     }
   return NULL;
}

static Evas_Object *
_e_mod_comp_src_shadow_get_func(void                  *data,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   Eina_List *l;
   E_Comp_Object *co;
   E_Zone *z;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);

   if (cw->bd) z = cw->bd->zone;
   else z = e_util_zone_current_get(c->man);
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!_comp_mod->conf->canvas_per_zone)
          return co->shadow;
        else if ((co->zone) && (co->zone == z))
          return co->shadow;
     }
   return NULL;
}

static Evas_Object *
_e_mod_comp_src_image_mirror_add_func(void *data             __UNUSED__,
                                      E_Manager *man         __UNUSED__,
                                      E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_GOTO(_comp_mod->conf->nocomp_fs, end);

   EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
     {
        if (canvas->nocomp.mode != E_NOCOMP_MODE_RUN) continue;
        if (cw != canvas->nocomp.cw) continue;

        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s| END MIRROR_ADD", "NOCOMP");

        e_mod_comp_canvas_nocomp_end(canvas);
     }
end:
   return _e_mod_comp_win_mirror_add(cw);
}

static Eina_Bool
_e_mod_comp_src_visible_get_func(void *data             __UNUSED__,
                                 E_Manager *man         __UNUSED__,
                                 E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   return cw->visible;
}

static void
_e_mod_comp_src_hidden_set_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src,
                                Eina_Bool              hidden)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   Eina_List *l;
   E_Comp_Object *co;
   E_CHECK(cw);
   E_CHECK(cw->c);
   if (cw->hidden_override == hidden) return;
   cw->hidden_override = hidden;
   if (cw->bd)
     e_border_comp_hidden_set(cw->bd,
                              cw->hidden_override);
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (cw->visible)
          {
             if (cw->hidden_override)
               evas_object_hide(co->shadow);
             else if (!cw->bd ||
                      (cw->bd->visible && cw->show_done))
               evas_object_show(co->shadow);
          }
        else
          {
             if (cw->hidden_override)
               evas_object_hide(co->shadow);
          }
     }
}

static Eina_Bool
_e_mod_comp_src_hidden_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   return cw->hidden_override;
}

static E_Popup *
_e_mod_comp_src_popup_get_func(void *data             __UNUSED__,
                               E_Manager *man         __UNUSED__,
                               E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   return cw->pop;
}

static E_Border *
_e_mod_comp_src_border_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   return cw->bd;
}

static Ecore_X_Window
_e_mod_comp_src_window_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   return cw->win;
}

static Eina_Bool
_e_mod_comp_src_input_region_set_func(void *data             __UNUSED__,
                                      E_Manager *man         __UNUSED__,
                                      E_Manager_Comp_Source *src,
                                      int                    x,
                                      int                    y,
                                      int                    w,
                                      int                    h)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   Eina_Bool res = EINA_FALSE;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);

   if (!cw->shape_input)
     cw->shape_input = e_mod_comp_win_shape_input_new(cw);
   E_CHECK_RETURN(cw->shape_input, 0);

   res = e_mod_comp_win_shape_input_rect_set(cw->shape_input, x, y, w, h);
   E_CHECK_RETURN(res, 0);

   e_mod_comp_win_shape_input_invalid_set(cw->c, 1);
   _e_mod_comp_win_render_queue(cw);
   return EINA_TRUE;
}

static int
_e_mod_comp_input_region_new_func(void *data     __UNUSED__,
                                  E_Manager *man __UNUSED__)
{
   E_Comp *c = e_mod_comp_util_get();
   E_CHECK_RETURN(c, 0);

   return e_mod_comp_shape_input_new(c);
}

static Eina_Bool
_e_mod_comp_input_region_set_func(void *data     __UNUSED__,
                                  E_Manager *man __UNUSED__,
                                  int            id,
                                  int            x,
                                  int            y,
                                  int            w,
                                  int            h)
{
   E_Comp *c = e_mod_comp_util_get();
   Eina_Bool res = EINA_FALSE;
   E_CHECK_RETURN(c, EINA_FALSE);

   if (id <= 0) return EINA_FALSE;

   res = e_mod_comp_shape_input_set(c, id, x, y, w, h);
   if (res)
     {
        e_mod_comp_win_shape_input_invalid_set(c, 1);
        _e_mod_comp_render_queue(c);
     }

   return res;
}

static Eina_Bool
_e_mod_comp_input_region_managed_set_func(void *data          __UNUSED__,
                                          E_Manager *man      __UNUSED__,
                                          int                 id,
                                          Evas_Object        *obj,
                                          Eina_Bool           set)
{
   E_Comp *c = e_mod_comp_util_get();
   Eina_Bool res = EINA_FALSE;
   E_CHECK_RETURN(c, EINA_FALSE);

   if (id <= 0) return EINA_FALSE;

   res = e_mod_comp_shape_input_managed_set(c, id, obj, set);
   if (res)
     {
        e_mod_comp_win_shape_input_invalid_set(c, 1);
        _e_mod_comp_render_queue(c);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_input_region_del_func(void *data     __UNUSED__,
                                  E_Manager *man __UNUSED__,
                                  int id)
{
   E_Comp *c = e_mod_comp_util_get();
   Eina_Bool res = EINA_FALSE;
   E_CHECK_RETURN(c, 0);

   if (id <= 0) return EINA_FALSE;

   res = e_mod_comp_shape_input_del(c, id);
   if (res)
     {
        e_mod_comp_win_shape_input_invalid_set(c, 1);
        _e_mod_comp_render_queue(c);
     }
   return res;
}

static void
_e_mod_comp_composite_mode_set(void           *data,
                               E_Manager *man __UNUSED__,
                               E_Zone        *zone,
                               Eina_Bool      set)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Canvas *canvas;
   Eina_List *l;
   E_CHECK(c);

   if (_comp_mod->conf->use_hwc)
     {
        EINA_LIST_FOREACH(c->canvases, l, canvas)
         {
            if (!canvas) continue;
            if (canvas->zone != zone) continue;

            if (canvas->hwcomp)
              e_mod_comp_hwcomp_force_composite_set(canvas->hwcomp, set);

            if (set)
              _e_mod_comp_render_queue(c);
         }
     }

   E_CHECK(_comp_mod->conf->nocomp_fs);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        if (canvas->zone == zone)
          {
             if (set)
               {
                  canvas->nocomp.comp_ref++;

                  e_mod_comp_hw_ov_win_msg_show
                    (E_COMP_LOG_TYPE_NOCOMP,
                    ">> %d COMP REQUEST ref:%d", canvas->num,
                    canvas->nocomp.comp_ref);

                  if (!canvas->nocomp.force_composite)
                    {
                       canvas->nocomp.force_composite = set;
                       e_mod_comp_hw_ov_win_msg_show
                         (E_COMP_LOG_TYPE_NOCOMP,
                         ">> %d COMP REQUEST", canvas->num);

                       if (canvas->nocomp.cw)
                         {
                            canvas->nocomp.cw->update = 1;
                            c->updates = eina_list_append(c->updates, canvas->nocomp.cw);
                            _e_mod_comp_win_render_queue(canvas->nocomp.cw);
                         }
                       else
                         {
                            _e_mod_comp_render_queue(c);
                         }
                    }
               }
             else
               {
                  canvas->nocomp.comp_ref--;
                  if (canvas->nocomp.comp_ref < 0)
                    canvas->nocomp.comp_ref = 0;

                  e_mod_comp_hw_ov_win_msg_show
                    (E_COMP_LOG_TYPE_NOCOMP,
                    ">> %d NOCOMP REQUEST ref:%d", canvas->num,
                    canvas->nocomp.comp_ref);

                  if ((canvas->nocomp.comp_ref == 0) &&
                      (canvas->nocomp.force_composite))
                    {
                       canvas->nocomp.force_composite = EINA_FALSE;
                       e_mod_comp_hw_ov_win_msg_show
                         (E_COMP_LOG_TYPE_NOCOMP,
                         ">> %d NOCOMP REQUEST", canvas->num);
                    }
               }
             break;
          }
     }
}

static Eina_Bool
_e_mod_comp_composite_mode_get(void          *data,
                               E_Manager *man __UNUSED__,
                               E_Zone        *zone)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Canvas *canvas;
   Eina_List *l;

   E_CHECK_RETURN(_comp_mod->conf->nocomp_fs, 1);
   E_CHECK_RETURN(c, 1);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        if (canvas->zone == zone)
          {
             if (canvas->nocomp.mode == E_NOCOMP_MODE_RUN)
               return EINA_FALSE;
             else
               return EINA_TRUE;
          }
     }
   return EINA_TRUE;
}

static void
_e_mod_comp_src_shadow_show_func(void                  *data,
                                 E_Manager *man         __UNUSED__,
                                 E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK(cw);
   E_CHECK(cw->c);

   _e_mod_comp_win_show(cw);
}

static void
_e_mod_comp_src_shadow_hide_func(void                  *data,
                                 E_Manager *man         __UNUSED__,
                                 E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   E_CHECK(cw);
   E_CHECK(cw->c);

   _e_mod_comp_win_hide(cw);
}

static Evas_Object *
_e_mod_comp_layer_get_func(void           *data,
                           E_Manager *man __UNUSED__,
                           E_Zone        *zone,
                           const char    *name)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Layer *ly = NULL;
   Eina_List *l;

   E_CHECK_RETURN(c, NULL);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        if (canvas->zone == zone)
          {
             ly = e_mod_comp_canvas_layer_get(canvas, name);
             if ((ly) && (!strcmp(name, "ly-ctrl")) && (ly->need_init))
               {
                  E_Comp_Layer *comp_ly = e_mod_comp_canvas_layer_get(canvas, "comp");
                  if (comp_ly)
                    {
                       ly->need_init = EINA_FALSE;

                       e_layout_pack(comp_ly->layout, ly->layout);
                       e_mod_comp_layer_populate_above_normal(comp_ly, ly->layout);
                       e_mod_comp_layer_bg_adjust(ly);
                    }
               }

             if (ly) return ly->layout;
             break;
          }
     }

   return NULL;
}

static void
_e_mod_comp_layer_raise_above_func(void          *data,
                                   E_Manager     *man __UNUSED__,
                                   E_Zone        *zone,
                                   const char    *name,
                                   E_Border      *bd)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Layer *ly = NULL;
   Eina_List *l;

   E_CHECK(c);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        if (canvas->zone == zone)
          {
             ly = e_mod_comp_canvas_layer_get(canvas, name);
             if (!ly) return;
             e_mod_comp_layer_raise_above(canvas, ly->layout, bd);
          }
     }
}

static void
_e_mod_comp_layer_lower_below_func(void          *data,
                                   E_Manager     *man __UNUSED__,
                                   E_Zone        *zone,
                                   const char    *name,
                                   E_Border      *bd)
{
   E_Comp *c = (E_Comp *)data;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Layer *ly = NULL;
   Eina_List *l;

   E_CHECK(c);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        if (canvas->zone == zone)
          {
             ly = e_mod_comp_canvas_layer_get(canvas, name);
             if (!ly) return;
             e_mod_comp_layer_lower_below(canvas, ly->layout, bd);
          }
     }
}

static E_Comp *
_e_mod_comp_add(E_Manager *man)
{
   E_Comp *c;
   Ecore_X_Window *wins;
   Ecore_X_Window_Attributes att;
   Eina_Bool res;
   int i, num;

   E_Container *con;
   E_Zone *zone;
   E_Comp_Canvas *canvas;
   Eina_List *l, *ll;

   c = E_NEW(E_Comp, 1);
   E_CHECK_RETURN(c, NULL);

   res = ecore_x_screen_is_composited(man->num);
   if (res)
     {
        e_util_dialog_internal
           (_("Compositor Error"),
           _("Another compositor is already running<br>"
             "on your screen."));
        free(c);
        return NULL;
     }

   c->cm_selection = ecore_x_window_input_new(man->root, 0, 0, 1, 1);
   if (!c->cm_selection)
     {
        free(c);
        return NULL;
     }
   ecore_x_screen_is_composited_set(man->num, c->cm_selection);

   ecore_x_e_comp_sync_supported_set(man->root, _comp_mod->conf->efl_sync);

   c->man = man;
   c->win = ecore_x_composite_render_window_enable(man->root);
   if (!c->win)
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your screen does not support the compositor<br>"
            "overlay window. This is needed for it to<br>"
            "function."));
        E_FREE(c);
        return NULL;
     }

   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(c->win, &att);

   if ((att.depth != 24) && (att.depth != 32))
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your screen is not in 24/32bit display mode.<br>"
            "This is required to be your default depth<br>"
            "setting for the compositor to work properly."));
        ecore_x_composite_render_window_disable(c->win);
        E_FREE(c);
        return NULL;
     }

   if (c->man->num == 0) e_alert_composite_win = c->win;

   e_mod_comp_screen_lock_init(&(c->lock));

   if (!_e_mod_comp_effect_init(c))
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _(" Effect failed to initialize."));
        ecore_x_composite_render_window_disable(c->win);
        E_FREE(c);
        return NULL;
     }

   if (_comp_mod->conf->canvas_per_zone)
     {
        EINA_LIST_FOREACH(man->containers, l, con)
          {
             EINA_LIST_FOREACH(con->zones, ll, zone)
               {
                  if (!zone) continue;
                  canvas = e_mod_comp_canvas_add(c, zone);
                  if (!canvas)
                    {
                       ecore_x_composite_render_window_disable(c->win);
                       E_FREE(c);
                       return NULL;
                    }

               }
          }
     }
   else
     {
        canvas = e_mod_comp_canvas_add(c, NULL);
        if (!canvas)
          {
             ecore_x_composite_render_window_disable(c->win);
             E_FREE(c);
             return NULL;
          }
     }

   if (_comp_mod->conf->keyboard_effect) c->keyboard_effect = EINA_TRUE;
   if (_comp_mod->conf->defer_raise_effect) c->defer_raise_effect = EINA_TRUE;
   if (_comp_mod->conf->fake_image_launch) c->fake_image_launch = EINA_TRUE;

   ecore_x_composite_redirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);

   wins = ecore_x_window_children_get(c->man->root, &num);
   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             E_Comp_Win *cw;
             int x, y, w, h, border;
             char *wname = NULL, *wclass = NULL;

             ecore_x_icccm_name_class_get(wins[i], &wname, &wclass);
             if ((man->initwin == wins[i]) ||
                 ((wname) && (wclass) && (!strcmp(wname, "E")) &&
                  (!strcmp(wclass, "Init_Window"))))
               {
                  free(wname);
                  free(wclass);
                  ecore_x_window_reparent(wins[i], c->win, 0, 0);
                  ecore_x_sync();
                  continue;
               }
             if (wname) free(wname);
             if (wclass) free(wclass);
             wname = wclass = NULL;
             cw = _e_mod_comp_win_add(c, wins[i]);
             if (!cw) continue;
             if (i == _comp_mod->conf->lower_layer)
               c->lower_win = cw;
             ecore_x_window_geometry_get(cw->win, &x, &y, &w, &h);
             border = ecore_x_window_border_width_get(cw->win);
             if (wins[i] == c->win) continue;
             _e_mod_comp_win_configure(cw, x, y, w, h, border);
             if (ecore_x_window_visible_get(wins[i]))
               _e_mod_comp_win_show(cw);
          }
        free(wins);
     }

   ecore_x_window_key_grab
     (c->man->root, "Home",
     ECORE_EVENT_MODIFIER_SHIFT |
     ECORE_EVENT_MODIFIER_CTRL |
     ECORE_EVENT_MODIFIER_ALT, 0);
   ecore_x_window_key_grab
     (c->man->root, "F",
     ECORE_EVENT_MODIFIER_SHIFT |
     ECORE_EVENT_MODIFIER_CTRL |
     ECORE_EVENT_MODIFIER_ALT, 0);

   c->comp.data                      = c;
   c->comp.func.evas_get             = _e_mod_comp_evas_get_func;
   c->comp.func.update               = _e_mod_comp_update_func;
   c->comp.func.src_get              = _e_mod_comp_src_get_func;
   c->comp.func.src_list_get         = _e_mod_comp_src_list_get_func;
   c->comp.func.src_image_get        = _e_mod_comp_src_image_get_func;
   c->comp.func.src_shadow_get       = _e_mod_comp_src_shadow_get_func;
   c->comp.func.src_image_mirror_add = _e_mod_comp_src_image_mirror_add_func;
   c->comp.func.src_visible_get      = _e_mod_comp_src_visible_get_func;
   c->comp.func.src_hidden_set       = _e_mod_comp_src_hidden_set_func;
   c->comp.func.src_hidden_get       = _e_mod_comp_src_hidden_get_func;
   c->comp.func.src_window_get       = _e_mod_comp_src_window_get_func;
   c->comp.func.src_border_get       = _e_mod_comp_src_border_get_func;
   c->comp.func.src_popup_get        = _e_mod_comp_src_popup_get_func;
   c->comp.func.screen_lock          = e_mod_comp_screen_lock_func;
   c->comp.func.screen_unlock        = e_mod_comp_screen_unlock_func;
   c->comp.func.src_input_region_set = _e_mod_comp_src_input_region_set_func;
   c->comp.func.input_region_new     = _e_mod_comp_input_region_new_func;
   c->comp.func.input_region_set     = _e_mod_comp_input_region_set_func;
   c->comp.func.input_region_managed_set = _e_mod_comp_input_region_managed_set_func;
   c->comp.func.input_region_del         = _e_mod_comp_input_region_del_func;
   c->comp.func.src_move_lock            = NULL; /* deprecated */
   c->comp.func.src_move_unlock          = NULL; /* deprecated */
   c->comp.func.composite_mode_set       = _e_mod_comp_composite_mode_set;
   c->comp.func.composite_mode_get       = _e_mod_comp_composite_mode_get;
   c->comp.func.src_shadow_show          = _e_mod_comp_src_shadow_show_func;
   c->comp.func.src_shadow_hide          = _e_mod_comp_src_shadow_hide_func;
   c->comp.func.layer_get            = _e_mod_comp_layer_get_func;
   c->comp.func.layer_raise_above    = _e_mod_comp_layer_raise_above_func;
   c->comp.func.layer_lower_below    = _e_mod_comp_layer_lower_below_func;

   e_manager_comp_set(c->man, &(c->comp));
   return c;
}

static void
_e_mod_comp_del(E_Comp *c)
{
   E_Comp_Win *cw;
   E_Comp_Canvas *canvas;

   e_manager_comp_set(c->man, NULL);

   ecore_x_window_key_ungrab
     (c->man->root, "F",
     ECORE_EVENT_MODIFIER_SHIFT |
     ECORE_EVENT_MODIFIER_CTRL |
     ECORE_EVENT_MODIFIER_ALT, 0);
   ecore_x_window_key_ungrab
     (c->man->root, "Home",
     ECORE_EVENT_MODIFIER_SHIFT |
     ECORE_EVENT_MODIFIER_CTRL |
     ECORE_EVENT_MODIFIER_ALT, 0);

   e_mod_comp_x_grab_set(c, EINA_FALSE);

   if((c->fake_image_launch) && (c->eff_img))
     c->effect_funcs.image_launch_free(c->eff_img);

   while (c->wins)
     {
        cw = (E_Comp_Win *)(c->wins);
        if (cw->counter)
          {
             ecore_x_sync_counter_free(cw->counter);
             cw->counter = 0;
             cw->sync_info.val = 0;
          }
        cw->force = 1;
        _e_mod_comp_win_hide(cw);
        cw->force = 1;
        _e_mod_comp_win_del(cw);
     }

   EINA_LIST_FREE(c->canvases, canvas) e_mod_comp_canvas_del(canvas);
   c->canvases = NULL;

   ecore_x_composite_unredirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
   ecore_x_composite_render_window_disable(c->win);
   if (c->man->num == 0) e_alert_composite_win = 0;
   if (c->render_animator) ecore_animator_del(c->render_animator);
   if (c->new_up_timer) ecore_timer_del(c->new_up_timer);
   if (c->update_job) ecore_job_del(c->update_job);
   if (c->wins_list) eina_list_free(c->wins_list);

   ecore_x_window_free(c->cm_selection);
   ecore_x_e_comp_sync_supported_set(c->man->root, 0);
   ecore_x_screen_is_composited_set(c->man->num, 0);

   if (c->shape_inputs) eina_list_free(c->shape_inputs);
   if (c->shape_inputs_mo) eina_list_free(c->shape_inputs_mo);

   c->effect_funcs.shutdown(c);

   free(c);
}

///////////////////////////////////////////////////////////////////////////////////////////
static Eina_Bool
_e_mod_comp_zone_move_resize(void *data __UNUSED__,
                             int type   __UNUSED__,
                             void      *event)
{
   E_Event_Zone_Move_Resize *ev = (E_Event_Zone_Move_Resize *)event;
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;
   if (!(ev && ev->zone))
     return ECORE_CALLBACK_PASS_ON;

   if (!_comp_mod->conf->canvas_per_zone)
     return ECORE_CALLBACK_PASS_ON;

   ELB(ELBT_COMP, "E_ZONE_MOVE_RESIZE", ev->zone->id);

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if (canvas->zone != ev->zone) continue;
             if (!((ev->zone->x == canvas->x) &&
                   (ev->zone->y == canvas->y)))
               {
                  canvas->x = ev->zone->x;
                  canvas->y = ev->zone->y;
                  ecore_evas_move(canvas->ee,
                                  canvas->x,
                                  canvas->y);
               }
             if (!((ev->zone->w == canvas->w) &&
                   (ev->zone->h == canvas->h)))
               {
                  canvas->w = ev->zone->w;
                  canvas->h = ev->zone->h;
                  ecore_evas_resize(canvas->ee,
                                    canvas->w,
                                    canvas->h);
               }
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_zone_add(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void      *event)
{
   E_Event_Zone_Add *ev = (E_Event_Zone_Add *)event;
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;
   E_Comp_Win *cw;
   E_Comp_Object *co;
   Eina_Bool found = EINA_FALSE;
   if ((!ev) || (!ev->zone) ||
       (!ev->zone->container))
     {
        return ECORE_CALLBACK_PASS_ON;
     }
   if (!_comp_mod->conf->canvas_per_zone)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   EINA_LIST_FOREACH(compositors, l, c)
     {
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (canvas->zone == ev->zone)
               {
                  found = EINA_TRUE;
                  break;
               }
          }
     }

   if (found) return ECORE_CALLBACK_PASS_ON;

   ELB(ELBT_COMP, "E_ZONE_ADD", ev->zone->id);

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if ((!c) || (c->man != ev->zone->container->manager))
          {
             continue;
          }

        canvas = e_mod_comp_canvas_add(c, ev->zone);
        if (!canvas) return ECORE_CALLBACK_PASS_ON;

        ecore_x_window_shape_rectangle_add(c->win,
                                           canvas->zone->x,
                                           canvas->zone->y,
                                           canvas->zone->w,
                                           canvas->zone->h);
        ecore_x_sync();

        EINA_INLIST_FOREACH(c->wins, cw)
          {
             Eina_Bool found = EINA_FALSE;
             Eina_List *lll;
             E_Comp_Canvas *_canvas;
             EINA_LIST_FOREACH(c->canvases, lll, _canvas)
               {
                  if (_canvas->ee_win == cw->win)
                    {
                       found = 1;
                       break;
                    }
               }

             co = e_mod_comp_obj_add(cw, canvas);
             if (!co) continue;

             cw->objs = eina_list_append(cw->objs, co);

             if ((!cw->input_only) && (!cw->invalid))
               {
                  e_mod_comp_win_shadow_setup(cw, co);
                  e_mod_comp_win_cb_setup(cw, co);
                  evas_object_show(co->img);

                  _e_mod_comp_win_transparent_rect_update(cw);

                  if (cw->bd) evas_object_data_set(co->shadow, "border", cw->bd);
                  else if (cw->pop)
                    evas_object_data_set(co->shadow, "popup", cw->pop);
                  else if (cw->menu)
                    evas_object_data_set(co->shadow, "menu", cw->menu);

                  evas_object_pass_events_set(co->img, 1);
               }

             evas_object_pass_events_set(co->shadow, 1);
             evas_object_data_set(co->shadow, "win",
                                  (void *)((unsigned long)cw->win));
             evas_object_data_set(co->shadow, "src", cw);

             int zx = 0, zy = 0;
             zx = co->zone->x;
             zy = co->zone->y;
             evas_object_move(co->shadow, cw->x - zx, cw->y - zy);
             evas_object_resize(co->shadow, cw->pw, cw->ph);

             if (cw->visible)
               {
                  evas_object_image_size_set(co->img, cw->pw, cw->ph);
                  Evas_Object *o;
                  EINA_LIST_FOREACH(co->img_mirror, ll, o)
                    {
                       evas_object_image_size_set(o, cw->pw, cw->ph);
                    }
                  if ((cw->dmg_updates >= 1))
                    {
                       cw->defer_hide = 0;
                       if (!cw->hidden_override)
                         evas_object_show(co->shadow);
                       edje_object_signal_emit(co->shadow, "e,state,visible,on", "e");
                       if (!cw->animating)
                         {
                            cw->c->animating++;
                         }
                       cw->animating = 1;
                       _e_mod_comp_win_render_queue(cw);

                       cw->pending_count++;
                       e_manager_comp_event_src_visibility_send
                         (cw->c->man, (E_Manager_Comp_Source *)cw,
                         _e_mod_comp_cb_pending_after, cw->c);
                    }
               }
          } // end of eina_inlist
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_zone_del(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void      *event)
{
   E_Event_Zone_Del *ev = (E_Event_Zone_Del *)event;
   Eina_List *l, *ll, *lll;
   E_Comp *c;
   E_Comp_Canvas *canvas;
   E_Comp_Win *cw;
   E_Comp_Object *co;
   if (!ev || !ev->zone)
     {
        return ECORE_CALLBACK_PASS_ON;
     }
   if (!_comp_mod->conf->canvas_per_zone)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   ELB(ELBT_COMP, "E_ZONE_DEL", ev->zone->id);

   EINA_LIST_FOREACH(compositors, l, c)
     {
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (canvas->zone != ev->zone) continue;
             EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
               {
                  EINA_LIST_FOREACH(cw->objs, lll, co)
                    {
                       if (co->zone != ev->zone) continue;
                       cw->objs = eina_list_remove(cw->objs, co);
                       e_mod_comp_obj_del(co);
                    }
               }

             ecore_x_window_shape_rectangle_subtract(c->win,
                                                     canvas->zone->x,
                                                     canvas->zone->y,
                                                     canvas->zone->w,
                                                     canvas->zone->h);
             ecore_x_sync();

             c->canvases = eina_list_remove(c->canvases, canvas);
             e_mod_comp_canvas_del(canvas);
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_zone_rot_begin(void *data __UNUSED__,
                           int type   __UNUSED__,
                           void      *event)
{
   E_Event_Zone_Rotation_Change_Begin *ev = (E_Event_Zone_Rotation_Change_Begin *)event;
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   if (!(ev && ev->zone))
     return ECORE_CALLBACK_PASS_ON;

   ELBF(ELBT_COMP, 0, ev->zone->num, "%15.15s|", "ZONE_ROT_BEGIN");

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if (canvas->zone != ev->zone) continue;
             if (canvas->zr)
               c->effect_funcs.zone_rotation_begin(canvas->zr);
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_zone_rot_cancel(void *data __UNUSED__,
                            int type   __UNUSED__,
                            void      *event)
{
   E_Event_Zone_Rotation_Change_Cancel *ev = (E_Event_Zone_Rotation_Change_Cancel *)event;
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   if (!(ev && ev->zone))
     return ECORE_CALLBACK_PASS_ON;

   ELBF(ELBT_COMP, 0, ev->zone->num, "%15.15s|", "ZONE_ROT_CANCEL");

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if (canvas->zone != ev->zone) continue;
             if (canvas->zr)
               {
                  canvas->xv_ready[0] = EINA_FALSE;
                  c->effect_funcs.zone_rotation_cancel(canvas->zr);
               }
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_zone_rot_end(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void      *event)
{
   E_Event_Zone_Rotation_Change_End *ev = (E_Event_Zone_Rotation_Change_End *)event;
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Canvas *canvas;

   if (!(ev && ev->zone))
     return ECORE_CALLBACK_PASS_ON;

   ELBF(ELBT_COMP, 0, ev->zone->num, "%15.15s|", "ZONE_ROT_END");

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_LIST_FOREACH(c->canvases, ll, canvas)
          {
             if (!canvas) continue;
             if (canvas->zone != ev->zone) continue;
             if (canvas->zr)
               {
                  if (canvas->xv_ready[0])
                    {
                       canvas->xv_ready[1] = EINA_TRUE;
                       if (!canvas->xv_ready[2])
                         break;
                    }
                  Eina_Bool res = c->effect_funcs.zone_rotation_end(canvas->zr);
                  if (res)
                    c->effect_funcs.zone_rotation_do(canvas->zr);
                  else
                    c->effect_funcs.zone_rotation_clear(canvas->zr);

               }
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

//////////////////////////////////////////////////////////////////////////

static Eina_Bool
_e_mod_comp_hwc_configure(void *data __UNUSED__,
                          int ev_type   __UNUSED__,
                          void *event)
{
   Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event;
   Eina_List *l = NULL;
   E_Comp *c = NULL;

   if (!e) return ECORE_CALLBACK_PASS_ON;

   /* FIXME : No problem regarding timing issue??? */
   if (_comp_mod->conf->use_hwc)
    {
       EINA_LIST_FOREACH(compositors, l, c)
         {
            if (!c) continue;
            e_mod_comp_hwcomp_process_event (c, e);
         }
     }

   return ECORE_CALLBACK_PASS_ON;
}

//////////////////////////////////////////////////////////////////////////

Eina_Bool
e_mod_comp_init(void)
{
   Eina_List *l;
   E_Manager *man;
   unsigned int effect = 0;
   int res = 0;

   windows = eina_hash_string_superfast_new(NULL);
   borders = eina_hash_string_superfast_new(NULL);
   damages = eina_hash_string_superfast_new(NULL);

   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,         _e_mod_comp_create,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,        _e_mod_comp_destroy,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST,   _e_mod_comp_show_request,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW,           _e_mod_comp_show,             NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE,           _e_mod_comp_hide,             NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT,       _e_mod_comp_reparent,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,      _e_mod_comp_configure,        NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK,          _e_mod_comp_stack,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,       _e_mod_comp_property,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,        _e_mod_comp_message,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,          _e_mod_comp_shape,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_DAMAGE_NOTIFY,         _e_mod_comp_damage,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DAMAGE,         _e_mod_comp_damage_win,       NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,                _e_mod_comp_key_down,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,            _e_mod_comp_randr,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ADD,                  _e_mod_comp_bd_add,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE,               _e_mod_comp_bd_del,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_SHOW,                 _e_mod_comp_bd_show,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_HIDE,                 _e_mod_comp_bd_hide,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_MOVE,                 _e_mod_comp_bd_move,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_RESIZE,               _e_mod_comp_bd_resize,        NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ICONIFY,              _e_mod_comp_bd_iconify,       NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY,            _e_mod_comp_bd_uniconify,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN,             _e_mod_comp_bd_focus_in,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT,            _e_mod_comp_bd_focus_out,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_STACK,                _e_mod_comp_bd_stack,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,            _e_mod_comp_zone_move_resize, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_ADD,                    _e_mod_comp_zone_add,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_DEL,                    _e_mod_comp_zone_del,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_ROTATION_CHANGE_BEGIN,  _e_mod_comp_zone_rot_begin,   NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_ROTATION_CHANGE_CANCEL, _e_mod_comp_zone_rot_cancel,  NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_ROTATION_CHANGE_END,    _e_mod_comp_zone_rot_end,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_GENERIC,               _e_mod_comp_hwc_configure,    NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY, _e_mod_comp_cb_rr_output_prop, NULL));

   handlers_hook = eina_list_append(handlers_hook, e_border_hook_add(E_BORDER_HOOK_AUX_HINT_EVAL,     e_mod_comp_aux_hint_eval,     NULL));

   res = e_mod_comp_atoms_init();
   E_CHECK_RETURN(res, 0);

   res = e_mod_comp_win_type_init();
   E_CHECK_RETURN(res, 0);

   res = e_mod_comp_policy_init();
   E_CHECK_RETURN(res, 0);

   if (!ecore_x_composite_query())
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your X Display does not support the XComposite extension<br>"
            "or Ecore was built without XComposite support.<br>"
            "Note that for composite support you will also need<br>"
            "XRender and XFixes support in X11 and Ecore."));
        return 0;
     }
   if (!ecore_x_damage_query())
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your screen does not support the XDamage extension<br>"
            "or Ecore was built without XDamage support."));
        return 0;
     }

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Comp *c;
        if (!man) continue;
        c = _e_mod_comp_add(man);
        if (c)
          {
             int w, h;
             if (c->fake_image_launch)
               {
                  c->evas = _e_mod_comp_evas_get_func(c, c->man);
                  ecore_x_window_size_get(c->win, &w, &h);
                  c->eff_img = c->effect_funcs.image_launch_new(c->evas, w, h);
               }

             compositors = eina_list_append(compositors, c);
             e_mod_comp_util_set(c, man);
             ecore_animator_frametime_set(1.0f/60.0f);

             if (_comp_mod->conf->default_window_effect) effect = 1;
             ecore_x_window_prop_card32_set
               (c->man->root, ATOM_EFFECT_ENABLE,
               &effect, 1);
             ecore_x_window_prop_property_set
               (c->man->root, ATOM_OVERAY_WINDOW,
               ECORE_X_ATOM_WINDOW, 32, &c->win, 1);
          }
     }
   ecore_x_sync();

   res = e_mod_comp_aux_hint_init();
   E_CHECK_RETURN(res, 0);

   return 1;
}

void
e_mod_comp_shutdown(void)
{
   E_Comp *c;

   EINA_LIST_FREE(compositors, c) _e_mod_comp_del(c);

   E_FREE_LIST(handlers, ecore_event_handler_del);

   E_FREE_LIST(handlers_hook, e_border_hook_del);

   if (damages) eina_hash_free(damages);
   if (windows) eina_hash_free(windows);
   if (borders) eina_hash_free(borders);
   damages = NULL;
   windows = NULL;
   borders = NULL;

   e_mod_comp_aux_hint_shutdown();;
   e_mod_comp_policy_shutdown();
   e_mod_comp_win_type_shutdown();
   e_mod_comp_atoms_shutdown();

   e_mod_comp_util_set(NULL, NULL);
}

void
e_mod_comp_shadow_set(void)
{
   Eina_List *l, *ll;
   E_Comp *c;
   E_Comp_Win *cw;
   E_Comp_Object *co;

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        EINA_INLIST_FOREACH(c->wins, cw)
          {
             e_mod_comp_win_type_setup(cw);
             EINA_LIST_FOREACH(cw->objs, ll, co)
               {
                  if (!co) continue;
                  if ((co->shadow) && (co->img))
                    e_mod_comp_win_shadow_setup(cw, co);
               }
             c->effect_funcs.win_show(cw);
             _e_mod_comp_win_transparent_rect_update(cw);
          }
     }
}

EAPI void
e_mod_comp_win_cb_setup(E_Comp_Win    *cw,
                        E_Comp_Object *co)
{
   edje_object_signal_callback_add(co->shadow, "e,action,show,done", "e", _e_mod_comp_show_done, cw);
   edje_object_signal_callback_add(co->shadow, "e,action,hide,done", "e", _e_mod_comp_hide_done, cw);
}

EAPI void
e_mod_comp_x_grab_set(E_Comp *c,
                      Eina_Bool grab)
{
   E_CHECK(_comp_mod->conf->grab);
   E_CHECK(c);
   E_CHECK((c->grabbed != grab));
   if (grab)
     ecore_x_grab();
   else
     ecore_x_ungrab();
   c->grabbed = grab;
}
