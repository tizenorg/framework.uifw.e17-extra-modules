#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp.h"

/* static global variables */
static int _canvas_num = 0;

/* externally accessible functions */
EINTERN E_Comp_Canvas *
e_mod_comp_canvas_add(E_Comp *c,
                      E_Zone *zone)
{
   E_Comp_Canvas *canvas;
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
             ecore_evas_gl_x11_pre_post_swap_callback_set
               (canvas->ee, c, e_mod_comp_pre_swap, e_mod_comp_post_swap);
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
   evas_object_show(canvas->bg_img);
   evas_object_move(canvas->bg_img, 0, 0);
   evas_object_resize(canvas->bg_img, w, h);

   ecore_evas_show(canvas->ee);

   canvas->ee_win = ecore_evas_window_get(canvas->ee);
   canvas->zone = zone;

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

   c->canvases = eina_list_append(c->canvases, canvas);

   return canvas;

error_cleanup:
   if (canvas->ee)
     ecore_evas_free(canvas->ee);

   memset(canvas, 0, sizeof(E_Comp_Canvas));
   E_FREE(canvas);

   return NULL;
}

EINTERN void
e_mod_comp_canvas_del(E_Comp_Canvas *canvas)
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
   if (canvas->bg_img)
     {
        evas_object_del(canvas->bg_img);
        canvas->bg_img = NULL;
     }
   if (canvas->ov)
     {
        e_mod_comp_hw_ov_win_free(canvas->ov);
        canvas->ov = NULL;
        canvas->comp->use_hw_ov = EINA_FALSE;
     }
   ecore_evas_gl_x11_pre_post_swap_callback_set(canvas->ee, NULL, NULL, NULL);
   ecore_evas_manual_render(canvas->ee);
   ecore_evas_free(canvas->ee);
   memset(canvas, 0, sizeof(E_Comp_Canvas));
   E_FREE(canvas);
}
