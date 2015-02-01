#include "e.h"
#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp.h"
#include "e_mod_comp_screen.h"

/* local subsystem functions */
static int       _screen_angle_get(Ecore_X_Window root);
static void      _screen_lock(E_Comp *c);
static void      _screen_unlock(E_Comp *c);
static Eina_Bool _screen_lock_timeout(void *data);

/* externally accessible functions */
EAPI Eina_Bool
e_mod_comp_screen_rotation_init(E_Comp_Screen_Rotation *r,
                                Ecore_X_Window root,
                                int w, int h)
{
   E_CHECK_RETURN(r, 0);
   E_CHECK_RETURN(root, 0);
   E_CHECK_RETURN((w > 0), 0);
   E_CHECK_RETURN((h > 0), 0);

   r->enabled = EINA_FALSE;
   r->scr_w = w;
   r->scr_h = h;
   r->angle = _screen_angle_get(root);

   if (0 != r->angle)
     {
        r->enabled = EINA_TRUE;
        if (0 != (r->angle % 180))
          {
             r->scr_w = h;
             r->scr_h = w;
          }
     }
   return EINA_TRUE;
}

EAPI Eina_Bool
e_mod_comp_screen_lock_init(E_Comp_Screen_Lock *l)
{
   E_CHECK_RETURN(l, 0);
   l->locked = EINA_FALSE;
   l->timeout = NULL;
   return EINA_FALSE;
}

EAPI Eina_Bool
e_mod_comp_screen_lock_handler_message(Ecore_X_Event_Client_Message *ev)
{
   E_Comp *c;
   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(_comp_mod->conf->use_lock_screen, 0);

   c = e_mod_comp_find(ev->win);
   E_CHECK_RETURN(c, 0);

   if (ev->data.l[0] == 0) _screen_unlock(c);
   else if (ev->data.l[0] == 1) _screen_lock(c);

   return EINA_TRUE;
}

EAPI void
e_mod_comp_screen_lock_func(void *data,
                            E_Manager *man __UNUSED__)
{
   E_Comp *c = (E_Comp *)data;
   E_CHECK(c);
   _screen_lock(c);
}

EAPI void
e_mod_comp_screen_unlock_func(void *data,
                              E_Manager *man __UNUSED__)
{
   E_Comp *c = (E_Comp *)data;
   E_CHECK(c);
   _screen_unlock(c);
}

/* local subsystem functions */
static int
_screen_angle_get(Ecore_X_Window root)
{
   int ret = -1, ang = 0;
   unsigned int val = 0;
   Ecore_X_Display* dpy = ecore_x_display_get();
   E_CHECK_RETURN(dpy, 0);
   E_CHECK_RETURN(ATOM_X_SCREEN_ROTATION, 0);

   ret = ecore_x_window_prop_card32_get
           (root, ATOM_X_SCREEN_ROTATION, &val, 1);
   E_CHECK_RETURN((ret > 0), 0);

   switch (val)
     {
      case  1: ang =   0; break;
      case  2: ang =  90; break;
      case  4: ang = 180; break;
      case  8: ang = 270; break;
      default: ang =   0; break;
     }

   return ang;
}

static void
_screen_lock(E_Comp *c)
{
   Eina_List *l;
   E_Comp_Canvas *canvas;

   E_CHECK(_comp_mod);
   E_CHECK(_comp_mod->conf->use_lock_screen);
   E_CHECK(c->lock.locked != 1);
   E_CHECK(!c->lock.timeout);

   if (!_comp_mod->conf->lock_fps)
     {
        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             if (!canvas) continue;
             if (_comp_mod->conf->use_hwc)
               {
                  e_mod_comp_hwcomp_set_full_composite(canvas->hwcomp);
                  e_mod_comp_hwcomp_update_null_set_drawables(canvas->hwcomp);
               }

             ecore_evas_manual_render_set(canvas->ee, 1);
          }
     }

   c->lock.timeout = ecore_timer_add
                       (_comp_mod->conf->max_lock_screen_time,
                       _screen_lock_timeout, c);

   c->lock.locked = EINA_TRUE;
}

static void
_screen_unlock(E_Comp *c)
{
   Eina_List *l;
   E_Comp_Canvas *canvas;
   E_Comp_Win *cw;

   E_CHECK(_comp_mod);
   E_CHECK(_comp_mod->conf->use_lock_screen);
   E_CHECK(c->lock.locked);
   if (c->lock.timeout)
     {
        ecore_timer_del(c->lock.timeout);
        c->lock.timeout = NULL;
     }

   // remove all cw->update_timeout
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((!cw->visible) || (cw->input_only) || (cw->invalid))
           continue;
        if (cw->counter && cw->update_timeout)
          {
             cw->update = 0;
             e_mod_comp_win_damage_timeout((void*)cw);
          }
     }

   // clear c->update_job
   if (c->update_job)
     {
        ecore_job_del(c->update_job);
        c->update_job = NULL;
        c->render_overflow = 0;
     }

   // clear c->updates
   if (c->updates) e_mod_comp_cb_update(c);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;

        if ((canvas->nocomp.mode == E_NOCOMP_MODE_NONE)
            || (canvas->nocomp.mode == E_NOCOMP_MODE_PREPARE))
          {
             ecore_evas_manual_render(canvas->ee);

             if (!_comp_mod->conf->lock_fps)
               ecore_evas_manual_render_set(canvas->ee, 0);
          }
     }

   c->lock.locked = 0;
}

static Eina_Bool
_screen_lock_timeout(void *data)
{
   E_Comp *c = (E_Comp*)data;
   E_CHECK_RETURN(c, 0);
   _screen_unlock(c);
   return EINA_TRUE;
}
