#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"

typedef struct _E_Comp_Log
{
   E_Comp_Log_Type  type;
   int              repeat;
   Evas_Object     *o;
   Evas_Object     *bg;
   char             str[256];
} E_Comp_Log;

struct _E_Comp_HW_Ov_Win
{
   Ecore_X_Window  ee_win;
   Ecore_Evas     *ee;
   Evas           *evas;
   Evas_Object    *bg;
   int             x, y, w, h;

   Ecore_X_Window  root;

   Eina_Bool       on;

   // debugging info
   struct {
     int           num;
     Eina_List    *list;
   } info;
};

/* local subsystem globals */
static E_Comp_HW_Ov_Win *ov_win = NULL;

/* local subsystem functions */
static void      _hw_ov_win_power_set(E_Comp_HW_Ov_Win *ov, Eina_Bool set);
static Eina_Bool _hw_ov_win_update(E_Comp_HW_Ov_Win *ov, E_Comp_Win *cw);

/* externally accessible functions */
EAPI E_Comp_HW_Ov_Win *
e_mod_comp_hw_ov_win_new(Ecore_X_Window parent,
                         int            x,
                         int            y,
                         int            w,
                         int            h)
{
   E_Comp_HW_Ov_Win *ov = NULL;
   Eina_Bool res = EINA_FALSE;

   E_CHECK_GOTO(parent, finish);

   ov = E_NEW(E_Comp_HW_Ov_Win, 1);
   E_CHECK_GOTO(ov, finish);

   ov->ee = ecore_evas_software_x11_new(NULL, parent, x, y, w, h);
   E_CHECK_GOTO(ov->ee, finish);

   ov->ee_win = ecore_evas_window_get(ov->ee);;
   E_CHECK_GOTO(ov->ee_win, finish);

   ov->evas = ecore_evas_get(ov->ee);
   E_CHECK_GOTO(ov->evas, finish);

   ecore_x_composite_redirect_window(ov->ee_win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
   ecore_evas_comp_sync_set(ov->ee, 0);
   ecore_evas_show(ov->ee);

   ov->bg = evas_object_rectangle_add(ov->evas);
   E_CHECK_GOTO(ov->bg, finish);

   evas_object_render_op_set(ov->bg, EVAS_RENDER_COPY);
   evas_object_layer_set(ov->bg, EVAS_LAYER_MIN);
   evas_object_color_set(ov->bg, 0, 0, 0, 0);
   evas_object_move(ov->bg, 0, 0);
   evas_object_resize(ov->bg, w, h);
   evas_object_show(ov->bg);
   _hw_ov_win_power_set(ov, EINA_FALSE);

   // comp has to unset shape input mask for the H/W overlay window
   // to receive input event.
   ecore_x_window_shape_input_rectangle_set(ov->ee_win, -1, -1, 1, 1);

   ov->x = x;
   ov->y = y;
   ov->w = w;
   ov->h = h;

   ov_win = ov;

   res = EINA_TRUE;

finish:
   if (!res && ov)
     {
        if (ov->ee)
          {
             ecore_evas_free(ov->ee);
             ov->ee = NULL;
          }
        E_FREE(ov);
        ov = NULL;
     }
   return ov;
}

EAPI void
e_mod_comp_hw_ov_win_msg_config_update(void)
{
   E_Comp_Log *log;

   E_CHECK(ov_win);
   E_CHECK(_comp_mod->conf->use_hw_ov);
   E_CHECK(_comp_mod->conf->debug_info_show);

   EINA_LIST_FREE(ov_win->info.list, log)
     {
        evas_object_hide(log->o);
        evas_object_hide(log->bg);
        evas_object_del(log->o);
        evas_object_del(log->bg);
        memset(log, 0, sizeof(E_Comp_Log));
        E_FREE(log);
     }
}

#ifdef _LOG_TYPE_CHECK
# undef _LOG_TYPE_CHECK
#endif
# define _LOG_TYPE_CHECK(a, b)          \
  ((_comp_mod->conf->debug_type_##a) && \
   (E_COMP_LOG_TYPE_##b == type))

EAPI void
e_mod_comp_hw_ov_win_msg_show(E_Comp_Log_Type  type,
                              const char      *f,
                              ...)
{
   E_Comp_Log *log, *prev_log;
   Evas_Object *bg, *o;
   Evas_Coord w = 0, h = 0;
   Eina_List *l;
   int num, y;
   char buf[256], str[256];
   va_list args;

   E_CHECK(ov_win);
   E_CHECK(_comp_mod->conf->use_hw_ov);
   E_CHECK(_comp_mod->conf->debug_info_show);
   E_CHECK(_comp_mod->conf->max_debug_msgs >= 1);
   E_CHECK(type > E_COMP_LOG_TYPE_DEFAULT);
   E_CHECK(type < E_COMP_LOG_TYPE_MAX);

   if (!(_LOG_TYPE_CHECK(nocomp, NOCOMP) ||
         _LOG_TYPE_CHECK(swap, SWAP) ||
         _LOG_TYPE_CHECK(effect, EFFECT)))
     {
        return;
     }

   va_start(args, f);
   vsprintf(buf, f, args);
   va_end(args);

   /* check if previous log is same type, just increment repeat value */
   num = eina_list_count(ov_win->info.list);
   if (num >= 1)
     {
        log = eina_list_nth(ov_win->info.list, num - 1);
        if ((log) && (log->type == type) &&
            (!strcmp(buf, log->str)))
          {
             sprintf(str, "%04d|%s %d",
                     ov_win->info.num++,
                     log->str,
                     log->repeat++);
             evas_object_text_text_set(log->o, str);
             return;
          }
     }

   log = E_NEW(E_Comp_Log, 1);
   E_CHECK(log);

   log->type = type;
   strcpy(log->str, buf);
   sprintf(str, "%04d|%s", ov_win->info.num++, log->str);

   if (num < _comp_mod->conf->max_debug_msgs)
     {
        bg = evas_object_rectangle_add(ov_win->evas);
        evas_object_layer_set(bg, EVAS_LAYER_MAX);
        evas_object_render_op_set(bg, EVAS_RENDER_COPY);
        evas_object_color_set(bg, 30, 30, 30, 128);
        evas_object_show(bg);

        o = evas_object_text_add(ov_win->evas);
        evas_object_text_font_set(o, "Sans", 28);
        evas_object_text_text_set(o, str);
        evas_object_layer_set(o, EVAS_LAYER_MAX);
        evas_object_render_op_set(o, EVAS_RENDER_COPY);
        evas_object_color_set(o, 200, 200, 200, 255);
        evas_object_show(o);

        log->bg = bg;
        log->o = o;
     }
   else
     {
        prev_log = eina_list_nth(ov_win->info.list, 0);
        if (!prev_log)
          {
             E_FREE(log);
             return;
          }
        log->bg = prev_log->bg;
        log->o = prev_log->o;
        ov_win->info.list = eina_list_remove(ov_win->info.list, prev_log);
        E_FREE(prev_log);

        evas_object_text_text_set(log->o, str);
     }

   ov_win->info.list = eina_list_append(ov_win->info.list, log);

   y = ov_win->h - 40;
   EINA_LIST_REVERSE_FOREACH(ov_win->info.list, l, log)
     {
        if (!log) continue;
        evas_object_geometry_get(log->o, NULL, NULL, &w, &h);
        evas_object_move(log->bg, 0, y - 1);
        evas_object_resize(log->bg, w + 2, h + 2);
        evas_object_move(log->o, 1, y);
        y -= (h+2);
     }
}

EAPI void
e_mod_comp_hw_ov_win_free(E_Comp_HW_Ov_Win *ov)
{
   E_Comp_Log *log;
   E_CHECK(ov);

   _hw_ov_win_power_set(ov, EINA_FALSE);

   EINA_LIST_FREE(ov->info.list, log)
     {
        evas_object_hide(log->o);
        evas_object_hide(log->bg);
        evas_object_del(log->o);
        evas_object_del(log->bg);
        memset(log, 0, sizeof(E_Comp_Log));
        E_FREE(log);
     }
   if (ov->bg)
     {
        evas_object_del(ov->bg);
        ov->bg = NULL;
     }
   if (ov->ee)
     {
        ecore_evas_free(ov->ee);
        ov->ee = NULL;
     }
   memset(ov, 0, sizeof(E_Comp_HW_Ov_Win));
   E_FREE(ov);

   ov_win = NULL;
}

EAPI void
e_mod_comp_hw_ov_win_show(E_Comp_HW_Ov_Win *ov,
                          E_Comp_Win       *cw)
{
   E_CHECK(ov);
   E_CHECK(cw);
   E_CHECK(cw->visible);
   E_CHECK(cw->dmg_updates >= 1);

   _hw_ov_win_update(ov, cw);
}

EAPI void
e_mod_comp_hw_ov_win_hide(E_Comp_HW_Ov_Win *ov,
                          E_Comp_Win       *cw)
{
   E_CHECK(ov);
   E_CHECK(cw);
   E_CHECK(!cw->visible);

   _hw_ov_win_power_set(ov, EINA_FALSE);

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
   cw->ov.w = 0;
   cw->ov.h = 0;
}

EAPI void
e_mod_comp_hw_ov_win_obj_show(E_Comp_HW_Ov_Win *ov,
                              E_Comp_Win       *cw)
{
   Eina_Bool visible;

   if (cw->ov_obj)
     {
        visible = evas_object_visible_get(cw->ov_obj);
        if (!visible)
          {
             Eina_Bool res = _hw_ov_win_update(ov, cw);
             if (res)
               {
                  evas_object_show(cw->ov_obj);
                  _hw_ov_win_power_set(ov, EINA_TRUE);
               }
          }
     }
}

EAPI void
e_mod_comp_hw_ov_win_obj_hide(E_Comp_HW_Ov_Win *ov,
                              E_Comp_Win       *cw)
{
   Eina_Bool visible;
   if (cw->ov_obj)
     {
        visible = evas_object_visible_get(cw->ov_obj);
        if (visible)
          {
             _hw_ov_win_power_set(ov, EINA_FALSE);
             evas_object_hide(cw->ov_obj);
          }
     }
}



EAPI Eina_Bool
e_mod_comp_hw_ov_win_update(E_Comp_HW_Ov_Win *ov,
                            E_Comp_Win       *cw)
{
   E_CHECK_RETURN(ov, EINA_FALSE);
   E_CHECK_RETURN(cw, EINA_FALSE);

   Eina_Bool res = _hw_ov_win_update(ov, cw);
   if (!res)
     {
        // we will be trying to get image again at next update.
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
        e_mod_comp_win_render_queue(cw);
     }

   return res;
}

EAPI void
e_mod_comp_hw_ov_win_root_set(E_Comp_HW_Ov_Win *ov,
                              Ecore_X_Window    root)
{
   E_CHECK(ov);
   ov->root = root;
}

/* local subsystem functions */
static void
_hw_ov_win_power_set(E_Comp_HW_Ov_Win *ov,
                     Eina_Bool         set)
{
   if (set)
     {
        if (!ov->on)
          {
             // power on
             e_mod_comp_util_fb_visible_set(set);
             ov->on = EINA_TRUE;
          }
     }
   else
     {
        if (ov->on)
          {
             // power off
             e_mod_comp_util_fb_visible_set(set);
             ov->on = EINA_FALSE;
          }
     }
}

static Eina_Bool
_hw_ov_win_update(E_Comp_HW_Ov_Win *ov,
                  E_Comp_Win       *cw)
{
   Eina_Bool visible = EINA_FALSE;
   Eina_Bool get = EINA_FALSE;
   unsigned char *pix = NULL;

   if (!cw->pixmap || cw->pw <= 0 || cw->ph <= 0 || cw->needpix)
     {
        cw->ov.w = 0;
        cw->ov.h = 0;
        if (cw->ov_xim) ecore_x_image_free(cw->ov_xim);
        cw->ov_xim = NULL;
        return EINA_FALSE;
     }

   if (cw->pixmap && cw->pw > 0 && cw->ph > 0)
     {
        if (!cw->ov_xim)
          {
             cw->ov_xim = ecore_x_image_new(cw->pw, cw->ph, cw->vis, cw->depth);
             if (!cw->ov_xim) return EINA_FALSE;

             cw->ov.w = cw->pw;
             cw->ov.h = cw->ph;
          }
        else if (cw->ov.w != cw->pw || cw->ov.h != cw->ph)
          {
             if (cw->ov_xim) ecore_x_image_free(cw->ov_xim);
             cw->ov_xim = ecore_x_image_new(cw->pw, cw->ph, cw->vis, cw->depth);
             if (!cw->ov_xim)
               {
                  cw->ov.w = 0;
                  cw->ov.h = 0;
                  return EINA_FALSE;
               }
             cw->ov.w = cw->pw;
             cw->ov.h = cw->ph;
          }
     }

   if (cw->pixmap && cw->pw > 0 && cw->ph > 0 && cw->ov_xim &&
       !cw->ov_obj && cw->visible)
     {
        cw->ov_obj = evas_object_image_filled_add(ov->evas);
        if (!cw->ov_obj) return EINA_FALSE;
        evas_object_image_content_hint_set(cw->ov_obj, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
        evas_object_image_colorspace_set(cw->ov_obj, EVAS_COLORSPACE_ARGB8888);
        evas_object_image_size_set(cw->ov_obj, cw->pw, cw->ph);
        if (cw->argb) evas_object_image_alpha_set(cw->ov_obj, 1);
        else evas_object_image_alpha_set(cw->ov_obj, 0);
        evas_object_image_fill_set(cw->ov_obj, 0, 0, cw->pw, cw->ph);
        evas_object_resize(cw->ov_obj, cw->pw, cw->ph);
        evas_object_render_op_set(cw->ov_obj, EVAS_RENDER_COPY);
        evas_object_color_set(cw->ov_obj, 255, 255, 255, 255);
        evas_object_show(cw->ov_obj);
        _hw_ov_win_power_set(ov, EINA_TRUE);
     }

   if (cw->ov_obj && cw->visible)
     {
        visible = evas_object_visible_get(cw->ov_obj);
        if (!visible)
          {
             evas_object_show(cw->ov_obj);
             _hw_ov_win_power_set(ov, EINA_TRUE);
          }
     }

   if (cw->ov_xim && cw->ov_obj && cw->visible)
     {
        pix = ecore_x_image_data_get(cw->ov_xim, NULL, NULL, NULL);
        evas_object_image_data_set(cw->ov_obj, pix);
        evas_object_image_size_set(cw->ov_obj, cw->pw, cw->ph);

        get = ecore_x_image_get(cw->ov_xim, cw->pixmap,
                                0, 0, 0, 0,
                                cw->pw, cw->ph);

        if (!get)
          {
             e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
             return EINA_FALSE;
          }

        evas_object_move(cw->ov_obj, cw->x, cw->y);
        evas_object_resize(cw->ov_obj, cw->pw, cw->ph);

        int bpl, rows, bpp;
        pix = ecore_x_image_data_get(cw->ov_xim, &bpl, &rows, &bpp);
        if (pix)
          {
             evas_data_argb_unpremul((unsigned int *)pix,
                                     (unsigned int)bpl * (unsigned int)rows / bpp);
             evas_object_image_data_set(cw->ov_obj, pix);
             evas_object_image_data_update_add(cw->ov_obj, 0, 0, cw->pw, cw->ph);
          }
     }
   return EINA_TRUE;
}
