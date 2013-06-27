#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp.h"

/* static global variables */
static int _canvas_num = 0;

/* local subsystem functions */
static void      _fps_update(E_Comp_Canvas *canvas);
static void      _pre_swap(void *data, Evas *e);
static void      _post_swap(void *data, Evas *e);
static Eina_Bool _nocomp_prepare_timeout(void *data);
static Eina_Bool _nocomp_end_timeout(void *data);

/* externally accessible functions */
EINTERN void
e_mod_comp_layer_eval(E_Comp_Layer *ly)
{
   e_layout_freeze(ly->layout);

   if (!ly->bg)
     {
        ly->bg = evas_object_rectangle_add(ly->canvas->evas);
        /* TODO: make it configurable */
        if (!strncmp(ly->name, "move", strlen("move")))
          evas_object_color_set(ly->bg, 0, 0, 0, 0);
        else
          evas_object_color_set(ly->bg, 0, 0, 0, 255);
        evas_object_show(ly->bg);
        e_layout_pack(ly->layout, ly->bg);
     }
   e_layout_child_move(ly->bg, ly->x, ly->y);
   e_layout_child_resize(ly->bg, ly->w, ly->h);
   e_layout_child_lower(ly->bg);

   evas_object_move(ly->layout, ly->x, ly->y);
   evas_object_resize(ly->layout, ly->w, ly->h);
   e_layout_virtual_size_set(ly->layout, ly->w, ly->h);

   e_layout_thaw(ly->layout);
}

EINTERN void
e_mod_comp_layer_populate(E_Comp_Layer *ly,
                          Evas_Object  *o)
{
   e_layout_pack(ly->layout, o);
}

/* adjust the stack position of the background object to the bottom of layer */
EINTERN void
e_mod_comp_layer_bg_adjust(E_Comp_Layer *ly)
{
   e_layout_freeze(ly->layout);

   if (!ly->bg)
     {
        ly->bg = evas_object_rectangle_add(ly->canvas->evas);
        evas_object_color_set(ly->bg, 0, 0, 0, 255);
        evas_object_show(ly->bg);
        e_layout_pack(ly->layout, ly->bg);
     }

   e_layout_child_lower(ly->bg);

   e_layout_thaw(ly->layout);
}

EINTERN void
e_mod_comp_layer_effect_set(E_Comp_Layer *ly,
                            Eina_Bool     set)
{
   if (strcmp(ly->name, "effect"))
     return;

   if (set)
     {
        ly->count++;
        ly->canvas->animation.run = 1;
        ly->canvas->animation.num++;

        if (!evas_object_visible_get(ly->layout))
          {
             e_mod_comp_composite_mode_set(ly->canvas->zone, EINA_TRUE);
             evas_object_show(ly->layout);
          }
     }
   else
     {
        /* decrease effect count and hide effect layer if it is 0 */
        ly->count--;
        ly->canvas->animation.num--;

        if (ly->count <= 0)
          {
             E_FREE_LIST(ly->objs, e_mod_comp_effect_object_free);
             evas_object_hide(ly->layout);

             ly->canvas->animation.run = 0;
             ly->canvas->animation.num = 0;
             ly->count = 0;

             e_mod_comp_composite_mode_set(ly->canvas->zone, EINA_FALSE);
          }
     }
}

EINTERN Eina_Bool
e_mod_comp_layer_effect_get(E_Comp_Layer *ly)
{
   E_CHECK_RETURN(ly, EINA_FALSE);

   if (strcmp(ly->name, "effect"))
     return EINA_FALSE;

   return ly->canvas->animation.run;
}

EINTERN E_Comp_Effect_Object *
e_mod_comp_layer_effect_obj_get(E_Comp_Layer   *ly,
                                Ecore_X_Window win)
{
   E_Comp_Effect_Object *obj = NULL;
   Eina_List *l;
   E_CHECK_RETURN(ly, NULL);

   if (strcmp(ly->name, "effect"))
     return NULL;

   EINA_LIST_FOREACH(ly->objs, l, obj)
     {
        if (!obj) continue;
        if (obj->win == win)
          {
             return obj;
          }
     }

   return NULL;
}

static void
_ly_intercept_show(void        *data,
                   Evas_Object *obj)
{
   E_Comp_Layer *ly = (E_Comp_Layer *)data;

   ELBF(ELBT_COMP, 0, 0,
        "%15.15s|name:%s layout:%p obj:%p",
        "LY_PRE_SHOW", ly->name, ly->layout, obj);

   /* TODO: an incontrollable effect layer show problem can occasionally occur. */
   if (!strcmp(ly->name, "effect"))
     {
        if (!ly->canvas->animation.run)
          {
             ELBF(ELBT_COMP, 0, 0,
                  "%15.15s|name:%s layout:%p obj:%p SKIP SHOW run:%d",
                  "LY_PRE_SHOW", ly->name, ly->layout, obj,
                  ly->canvas->animation.run);

             return;
          }
     }

   evas_object_show(obj);
}

static void
_ly_intercept_hide(void        *data,
                   Evas_Object *obj)
{
   E_Comp_Layer *ly = (E_Comp_Layer *)data;

   ELBF(ELBT_COMP, 0, 0,
        "%15.15s|name:%s layout:%p obj:%p",
        "LY_PRE_HIDE", ly->name, ly->layout, obj);

   evas_object_hide(obj);
}

/* externally accessible functions */
EINTERN E_Comp_Canvas *
e_mod_comp_canvas_add(E_Comp *c,
                      E_Zone *zone)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   int x, y, w, h;
   E_CHECK_RETURN(c, 0);

   canvas = E_NEW(E_Comp_Canvas, 1);
   E_CHECK_RETURN(canvas, 0);

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
        w = c->man->w;
        h = c->man->h;
     }

   if (_comp_mod->conf->engine == ENGINE_GL)
     {
        int opt[20];
        int opt_i = 0;

        if (_comp_mod->conf->indirect)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_INDIRECT;
             opt_i++;
             opt[opt_i] = 1;
             opt_i++;
          }
        if (_comp_mod->conf->vsync)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_VSYNC;
             opt_i++;
             opt[opt_i] = 1;
             opt_i++;
          }
        if (opt_i > 0)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_NONE;
             canvas->ee = ecore_evas_gl_x11_options_new(NULL, c->win, x, y, w, h, opt);
          }
        if (!canvas->ee)
          canvas->ee = ecore_evas_gl_x11_new(NULL, c->win, x, y, w, h);
        if (canvas->ee)
          {
             c->gl = 1;
             ecore_evas_gl_x11_pre_post_swap_callback_set(canvas->ee,
                                                          c,
                                                          _pre_swap,
                                                          _post_swap);
          }
     }
   if (!canvas->ee)
     {
        if (_comp_mod->conf->engine == ENGINE_GL)
          {
             e_util_dialog_internal
               (_("Compositor Warning"),
               _("Your screen does not support OpenGL.<br>"
                 "Falling back to software engine."));
          }

        canvas->ee = ecore_evas_software_x11_new(NULL, c->win, x, y, w, h);
     }
   if (!canvas->ee)
     {
        e_util_dialog_internal
           (_("Compositor Error"),
            _("Failed to initialize Ecore Evas."));
        goto error_cleanup;
     }

   canvas->x = x;
   canvas->y = y;
   canvas->w = w;
   canvas->h = h;

   canvas->comp = c;
   canvas->num = _canvas_num++;

   ecore_evas_comp_sync_set(canvas->ee, 0);
   canvas->evas = ecore_evas_get(canvas->ee);

   canvas->bg_img = evas_object_rectangle_add(canvas->evas);
   evas_object_color_set(canvas->bg_img, 0, 0, 0, 255);
   evas_object_stack_below(canvas->bg_img, evas_object_bottom_get(canvas->evas));
   evas_object_move(canvas->bg_img, 0, 0);
   evas_object_resize(canvas->bg_img, w, h);
   evas_object_show(canvas->bg_img);

   ecore_evas_show(canvas->ee);

   canvas->ee_win = ecore_evas_window_get(canvas->ee);
   canvas->zone = zone;

   /* TODO: make a configurable list */
   int i;
   const char *names[] = {"comp", "effect", "move"};
   for (i = 0; i < 3; i++)
     {
        ly = E_NEW(E_Comp_Layer, 1);
        E_CHECK_GOTO(ly, error_cleanup);
        ly->name = strdup(names[i]);
        ly->layout = e_layout_add(canvas->evas);
        if (!ly->layout)
          {
             E_FREE(ly);
             goto error_cleanup;
          }

        evas_object_color_set(ly->layout, 255, 255, 255, 255);

        ly->x = 0;
        ly->y = 0;
        ly->w = w;
        ly->h = h;
        ly->canvas = canvas;

        e_mod_comp_layer_eval(ly);

        if (!strcmp(names[i], "comp"))
          evas_object_show(ly->layout);

        evas_object_intercept_show_callback_add(ly->layout, _ly_intercept_show, ly);
        evas_object_intercept_hide_callback_add(ly->layout, _ly_intercept_hide, ly);

        canvas->layers = eina_list_append(canvas->layers, ly);

        ELBF(ELBT_COMP, 0, i, "E_Comp_Layer:%s", names[i]);
     }

   // comp can create only one ecore_evas for H/W overlay window
   // this limit will be removed later
   if ((_comp_mod->conf->use_hw_ov) &&
       ((!zone) || (zone->num == 0)) &&
       (!c->use_hw_ov))
     {
        canvas->ov = e_mod_comp_hw_ov_win_new(c->win, x, y, w, h);
        if (canvas->ov)
          {
             c->use_hw_ov = EINA_TRUE;
             e_mod_comp_hw_ov_win_root_set(canvas->ov, c->man->root);
          }
     }

   canvas->zr = e_mod_comp_effect_zone_rotation_new(canvas);
   E_CHECK_GOTO(canvas->zr, error_cleanup);

   c->canvases = eina_list_append(c->canvases, canvas);

   return canvas;

error_cleanup:
   if (canvas->zr)
     e_mod_comp_effect_zone_rotation_free(canvas->zr);

   EINA_LIST_FREE(canvas->layers, ly)
     {
        free(ly->name);
        evas_object_del(ly->layout);
        E_FREE(ly);
     }
   if (canvas->ee)
     ecore_evas_free(canvas->ee);

   memset(canvas, 0, sizeof(E_Comp_Canvas));
   E_FREE(canvas);

   return NULL;
}

EINTERN void
e_mod_comp_canvas_del(E_Comp_Canvas *canvas)
{
   E_Comp_Layer *ly;

   if (canvas->fps.fg)
     {
        evas_object_del(canvas->fps.fg);
        canvas->fps.fg = NULL;
     }
   if (canvas->fps.bg)
     {
        evas_object_del(canvas->fps.bg);
        canvas->fps.bg = NULL;
     }
   if (canvas->bg_img)
     {
        evas_object_del(canvas->bg_img);
        canvas->bg_img = NULL;
     }
   if (canvas->zr)
     {
        e_mod_comp_effect_zone_rotation_free(canvas->zr);
        canvas->zr = NULL;
     }
   if (canvas->ov)
     {
        e_mod_comp_hw_ov_win_free(canvas->ov);
        canvas->ov = NULL;
        canvas->comp->use_hw_ov = EINA_FALSE;
     }
   ecore_evas_gl_x11_pre_post_swap_callback_set(canvas->ee, NULL, NULL, NULL);
   ecore_evas_manual_render(canvas->ee);

   EINA_LIST_FREE(canvas->layers, ly)
     {
        free(ly->name);
        evas_object_del(ly->layout);
        E_FREE(ly);
     }

   ecore_evas_free(canvas->ee);
   memset(canvas, 0, sizeof(E_Comp_Canvas));
   E_FREE(canvas);
}

EINTERN E_Comp_Layer *
e_mod_comp_canvas_layer_get(E_Comp_Canvas *canvas,
                            const char    *name)
{
   E_Comp_Layer *ly;
   Eina_List *l;

   E_CHECK_RETURN(canvas, NULL);
   E_CHECK_RETURN(name, NULL);

   EINA_LIST_FOREACH(canvas->layers, l, ly)
     {
        if (!strcmp(ly->name, name))
          return ly;
     }

   return NULL;
}

EINTERN E_Comp_Win *
e_mod_comp_canvas_fullscreen_check(E_Comp_Canvas *canvas)
{
   E_Comp *c = canvas->comp;
   E_Comp_Win *cw = NULL;

   if (c->fake_image_launch)
     {
        Eina_Bool res = e_mod_comp_effect_image_launch_running_check(c->eff_img);
        E_CHECK_RETURN(!res, NULL);
     }

   E_CHECK_RETURN(c->wins, NULL);
   E_CHECK_RETURN(_comp_mod->conf->nocomp_fs, NULL);

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((!cw->visible)   ||
            (cw->input_only) ||
            (cw->invalid))
          {
             continue;
          }
        if (!E_INTERSECTS(canvas->x, canvas->y, canvas->w, canvas->h,
                          cw->x, cw->y, cw->w, cw->h))
          {
             continue;
          }
        if (REGION_EQUAL_TO_CANVAS(cw, canvas) &&
            (!cw->argb) &&
            (!cw->shaped) &&
            (cw->dmg_updates >= 1) &&
            (!cw->show_ready) &&
            (cw->show_done) &&
            (cw->use_dri2))
          return cw;
        else
          return NULL;
     }
   return NULL;
}

EINTERN void
e_mod_comp_canvas_nocomp_prepare(E_Comp_Canvas *canvas,
                                 E_Comp_Win    *cw)
{
   E_CHECK(canvas);
   E_CHECK(cw);
   E_CHECK(canvas->nocomp.mode == E_NOCOMP_MODE_NONE);

   canvas->nocomp.mode = E_NOCOMP_MODE_PREPARE;
   canvas->nocomp.prepare.cw = cw;
   canvas->nocomp.prepare.timer = ecore_timer_add(2.0f,
                                                  _nocomp_prepare_timeout,
                                                  canvas);
}

EINTERN Eina_Bool
e_mod_comp_canvas_nocomp_begin(E_Comp_Canvas *canvas)
{
   E_Comp *c = NULL;
   E_Comp_Win *cw = NULL;

   E_CHECK_RETURN(canvas, EINA_FALSE);
   c = canvas->comp;
   E_CHECK_RETURN(c, EINA_FALSE);
   cw = canvas->nocomp.prepare.cw;
   E_CHECK_RETURN(cw, EINA_FALSE);

   ELBF(ELBT_COMP, 0,
        e_mod_comp_util_client_xid_get(cw),
        "NOCOMP_BEGIN canvas:%d dmg:%d",
        canvas->num, cw->dmg_updates);

   e_mod_comp_hw_ov_win_msg_show
     (E_COMP_LOG_TYPE_NOCOMP,
     ">> %d NOCOMP 0x%x dmg:%d",
     canvas->num,
     e_mod_comp_util_client_xid_get(cw),
     cw->dmg_updates);

   ecore_x_grab();

   if (cw->redirected)
     {
        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
     }

   if (cw->damage)
     {
        Ecore_X_Region parts;
        e_mod_comp_win_del_damage(cw, cw->damage);
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }

   e_mod_comp_win_comp_objs_img_deinit(cw);
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
     }

   ecore_x_window_shape_rectangle_subtract(c->win,
                                           canvas->x,
                                           canvas->y,
                                           canvas->w,
                                           canvas->h);
   ecore_x_sync();
   ecore_x_ungrab();

   ecore_evas_manual_render_set(canvas->ee, 1);
   c->nocomp = 1;
   //c->render_overflow = OVER_FLOW;
   canvas->nocomp.mode = E_NOCOMP_MODE_RUN;
   canvas->nocomp.cw = cw;
   canvas->nocomp.prepare.cw = NULL;

   if (canvas->nocomp.prepare.timer)
     {
        ecore_timer_del(canvas->nocomp.prepare.timer);
        canvas->nocomp.prepare.timer = NULL;
     }

   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }

   cw->nocomp = 1;
   cw->pw = 0;
   cw->ph = 0;
   cw->needpix = 1;

   e_mod_comp_win_shape_input_invalid_set(c, 1);
   e_mod_comp_win_render_queue(cw);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_canvas_nocomp_end(E_Comp_Canvas *canvas)
{
   E_Comp *c = NULL;
   E_Comp_Win *cw = NULL;

   E_CHECK_RETURN(_comp_mod->conf->nocomp_fs, EINA_FALSE);

   c = canvas->comp;
   E_CHECK_RETURN(c, EINA_FALSE);

   cw = canvas->nocomp.cw;
   E_CHECK_RETURN(cw, EINA_FALSE);

   ELBF(ELBT_COMP, 0,
        e_mod_comp_util_client_xid_get(cw),
        "NOCOMP_END canvas:%d",
        canvas->num, cw->dmg_updates);

   e_mod_comp_hw_ov_win_msg_show
     (E_COMP_LOG_TYPE_NOCOMP,
     ">> %d COMP 0x%x",
     canvas->num,
     e_mod_comp_util_client_xid_get(cw));

   ecore_x_grab();
   if (!cw->damage)
     {
        cw->damage = ecore_x_damage_new
          (cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        e_mod_comp_win_add_damage(cw, cw->damage);
     }

   ecore_x_window_shape_rectangle_add(c->win,
                                      canvas->x,
                                      canvas->y,
                                      canvas->w,
                                      canvas->h);
   if (!cw->redirected)
     {
        ecore_x_composite_redirect_window(cw->win,
                                          ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 1;
     }
   ecore_x_sync();
   ecore_x_ungrab();

   canvas->nocomp.mode = E_NOCOMP_MODE_END;
   canvas->nocomp.end.cw = cw;
   canvas->nocomp.cw = NULL;
   canvas->nocomp.end.dmg_updates = cw->dmg_updates + 2;

   if (canvas->nocomp.end.timer)
     {
        ecore_timer_del(canvas->nocomp.end.timer);
        canvas->nocomp.end.timer = NULL;
     }
   canvas->nocomp.end.timer = ecore_timer_add(2.0f,
                                              _nocomp_end_timeout,
                                              canvas);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_canvas_nocomp_dispose(E_Comp_Canvas *canvas)
{
   E_Comp *c = NULL;
   E_Comp_Win *cw = NULL;
   Ecore_X_Pixmap pm;
   E_Update_Rect *r;
   int i;

   E_CHECK_RETURN(_comp_mod->conf->nocomp_fs, EINA_FALSE);
   E_CHECK_RETURN((canvas->nocomp.mode == E_NOCOMP_MODE_END), EINA_FALSE);

   c = canvas->comp;
   E_CHECK_RETURN(c, EINA_FALSE);

   cw = canvas->nocomp.end.cw;
   E_CHECK_RETURN(cw, EINA_FALSE);
   E_CHECK_GOTO(cw->win, finish);

   if (canvas->nocomp.end.timer)
     {
        ecore_timer_del(canvas->nocomp.end.timer);
        canvas->nocomp.end.timer = NULL;
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
             if (!((cw->pw == cw->w) && (cw->ph == cw->h)))
               {
                  cw->pw = cw->w;
                  cw->ph = cw->h;
                  cw->pixmap = oldpm;
                  cw->needpix = 1;
                  ecore_x_pixmap_free(pm);
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
               }
             e_mod_comp_update_clear(cw->up);
             free(r);
          }
     }

   cw->nocomp = 0;
   e_mod_comp_win_render_queue(cw);

finish:
   ecore_evas_manual_render_set(canvas->ee, 0);
   canvas->nocomp.end.cw = NULL;
   canvas->nocomp.end.dmg_updates = 0;
   canvas->nocomp.mode = E_NOCOMP_MODE_NONE;
   c->nocomp = 0;

   return EINA_TRUE;
}

/* local subsystem functions */
static void
_fps_update(E_Comp_Canvas *canvas)
{
   char buf[128];
   double fps = 0.0, t, dt;
   int i;
   Evas_Coord x = 0, y = 0, w = 0, h = 0;
   E_Zone *z;

   if (!_comp_mod->conf->fps_show) return;

   t = ecore_time_get();

   if (_comp_mod->conf->fps_average_range < 1)
     _comp_mod->conf->fps_average_range = 30;
   else if (_comp_mod->conf->fps_average_range > 120)
     _comp_mod->conf->fps_average_range = 120;

   dt = t - canvas->fps.frametimes[_comp_mod->conf->fps_average_range - 1];

   if (dt > 0.0) fps = (double)_comp_mod->conf->fps_average_range / dt;
   else fps = 0.0;

   if (fps > 0.0) snprintf(buf, sizeof(buf), "FPS: %1.1f", fps);
   else snprintf(buf, sizeof(buf), "FPS: N/A");

   for (i = 121; i >= 1; i--) canvas->fps.frametimes[i] = canvas->fps.frametimes[i - 1];
   canvas->fps.frametimes[0] = t;
   canvas->fps.frameskip++;

   if (canvas->fps.frameskip >= _comp_mod->conf->fps_average_range)
     {
        canvas->fps.frameskip = 0;
        evas_object_text_text_set(canvas->fps.fg, buf);
     }

   evas_object_geometry_get(canvas->fps.fg, NULL, NULL, &w, &h);

   w += 8;
   h += 8;

   z = canvas->zone;
   if (z)
     {
        switch (_comp_mod->conf->fps_corner)
          {
           case 3: // bottom-right
              x = z->w - w;
              y = z->h - h;
              break;
           case 2: // bottom-left
              x = 0;
              y = z->y + z->h - h;
              break;
           case 1: // top-right
              x = z->w - w;
              y = z->y;
              break;
           default: // 0 // top-left
              x = 0;
              y = z->y;
              break;
          }
     }
   evas_object_move(canvas->fps.bg, x, y);
   evas_object_resize(canvas->fps.bg, w, h);
   evas_object_move(canvas->fps.fg, x + 4, y + 4);
}

static void
_pre_swap(void *data,
          Evas *e)
{
   E_Comp *c = (E_Comp *)data;
   Eina_List *l;
   E_Comp_Canvas *canvas;
   E_CHECK(c);

   e_mod_comp_x_grab_set(c, EINA_FALSE);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        if (canvas->evas != e) continue;

        e_mod_comp_hw_ov_win_msg_show
          (E_COMP_LOG_TYPE_SWAP,
          "%d %d SWAP a%d m%d",
          canvas->num,
          canvas->nocomp.mode,
          canvas->animation.num,
          ecore_evas_manual_render_get(canvas->ee));

        if (_comp_mod->conf->fps_show)
          {
             if (!canvas->fps.bg)
               {
                  canvas->fps.bg = evas_object_rectangle_add(canvas->evas);
                  evas_object_color_set(canvas->fps.bg, 0, 0, 0, 128);
                  evas_object_layer_set(canvas->fps.bg, EVAS_LAYER_MAX);
                  evas_object_show(canvas->fps.bg);
               }
             if (!canvas->fps.fg)
               {
                  canvas->fps.fg = evas_object_text_add(canvas->evas);
                  evas_object_text_font_set(canvas->fps.fg, "Sans", 30);
                  evas_object_text_text_set(canvas->fps.fg, "FPS: 0.0");
                  evas_object_color_set(canvas->fps.fg, 255, 255, 255, 255);
                  evas_object_layer_set(canvas->fps.fg, EVAS_LAYER_MAX);
                  evas_object_show(canvas->fps.fg);
               }
             _fps_update(canvas);
          }
        else
          {
             if (canvas->fps.fg)
               {
                  evas_object_del(canvas->fps.fg);
                  canvas->fps.fg = NULL;
               }
             if (canvas->fps.bg)
               {
                  evas_object_del(canvas->fps.bg);
                  canvas->fps.bg = NULL;
               }
          }
     }
}

static void
_post_swap(void *data,
           Evas *e)
{
   E_Comp *c = (E_Comp *)data;
   E_CHECK(c);
   E_CHECK(_comp_mod->conf->nocomp_fs);

   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Win *cw = NULL;
   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (canvas->evas != e) continue;
        if (canvas->nocomp.mode != E_NOCOMP_MODE_NONE) continue;
        if (canvas->nocomp.force_composite) continue;
        if (canvas->animation.run) continue;

        cw = e_mod_comp_canvas_fullscreen_check(canvas);
        if (!cw) continue;

        e_mod_comp_canvas_nocomp_prepare(canvas, cw);
     }
}

static Eina_Bool
_nocomp_prepare_timeout(void *data)
{
   E_Comp_Canvas *canvas = (E_Comp_Canvas *)data;
   E_Comp_Win *cw;
   E_CHECK_RETURN(canvas, ECORE_CALLBACK_CANCEL);

   if (canvas->nocomp.prepare.timer)
     {
        ecore_timer_del(canvas->nocomp.prepare.timer);
        canvas->nocomp.prepare.timer = NULL;
     }

   if ((canvas->nocomp.force_composite)||
       (canvas->animation.run))
     {
        canvas->nocomp.prepare.cw = NULL;
        canvas->nocomp.mode = E_NOCOMP_MODE_NONE;
        return ECORE_CALLBACK_CANCEL;
     }

   cw = e_mod_comp_canvas_fullscreen_check(canvas);
   if (!cw)
     {
        canvas->nocomp.prepare.cw = NULL;
        canvas->nocomp.mode = E_NOCOMP_MODE_NONE;
        return ECORE_CALLBACK_CANCEL;
     }
   else if (cw != canvas->nocomp.prepare.cw)
     {
        canvas->nocomp.prepare.cw = cw;
        canvas->nocomp.prepare.timer = ecore_timer_add(2.0f,
                                                       _nocomp_prepare_timeout,
                                                       canvas);
        return ECORE_CALLBACK_CANCEL;
     }

   e_mod_comp_canvas_nocomp_begin(canvas);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_nocomp_end_timeout(void *data)
{
   E_Comp_Canvas *canvas = (E_Comp_Canvas *)data;
   if (canvas->nocomp.end.timer)
     {
        ecore_timer_del(canvas->nocomp.end.timer);
        canvas->nocomp.end.timer = NULL;
     }
   e_mod_comp_canvas_nocomp_dispose(canvas);
   return ECORE_CALLBACK_CANCEL;
}
