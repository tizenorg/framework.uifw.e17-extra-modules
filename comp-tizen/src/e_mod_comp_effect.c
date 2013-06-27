#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include <X11/Xlib.h>

struct _E_Comp_Effect_Type
{
   Eina_Bool           animatable : 1; // if this valuse is true then window can show animaton.
   E_Comp_Effect_Style show;           // indicate show effect type
   E_Comp_Effect_Style hide;           // indicate hide effect type
   E_Comp_Effect_Style restack;        // indicate restack effect type
   E_Comp_Effect_Style rotation;       // indicate rotation effect type
   E_Comp_Effect_Style focusin;        // indicate focus in effect type
   E_Comp_Effect_Style focusout;       // indicate focus out effect type
};

struct _E_Comp_Effect_Job
{
   Evas_Object   *o;
   E_Comp_Canvas *canvas;
   Ecore_X_Window win;
   Ecore_X_Window cwin;
   char           emission[1024];
   char           src[1024];
   Eina_Bool      emitted;
   Eina_Bool      effect_obj;
};

static Eina_List *effect_jobs = NULL;

/* local subsystem functions */
static E_Comp_Effect_Style _effect_style_get(Ecore_X_Atom a);
static Eina_Bool           _state_send(E_Comp_Win *cw, Eina_Bool state);
static Eina_Bool           _effect_signal_del_intern(E_Comp_Win *cw, Evas_Object *obj, const char *name, Eina_Bool clean_all);

static void                _effect_win_set(E_Comp_Win *cw, const char *emission);
static void                _effect_below_wins_set(E_Comp_Win *cw);
static void                _effect_above_wins_set(E_Comp_Win *cw, Eina_Bool show);
static void                _effect_show(E_Comp_Win *cw);
static void                _effect_hide(E_Comp_Win *cw);
static Eina_Bool           _effect_obj_win_set(E_Comp_Effect_Object *o, E_Comp_Win *cw);
static void                _effect_obj_effect_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool           _effect_obj_find(E_Comp_Win *cw);

/* externally accessible functions */
EINTERN E_Comp_Effect_Object *
e_mod_comp_effect_object_new(E_Comp_Layer *ly,
                             E_Comp_Win   *cw)
{
   E_Comp_Effect_Object *o = NULL;
   Eina_Bool res;
   E_CHECK_RETURN(ly, NULL);
   E_CHECK_RETURN(cw, NULL);

   res = _effect_obj_find(cw);
   /* TODO: clean up previous effect job */
   E_CHECK_RETURN(!res, NULL);

   o = E_NEW(E_Comp_Effect_Object, 1);
   E_CHECK_RETURN(o, NULL);

   o->edje = edje_object_add(ly->canvas->evas);
   E_CHECK_GOTO(o->edje, fail);

   e_mod_comp_layer_populate(ly, o->edje);

   evas_object_data_set(o->edje, "comp.effect_obj.ly", ly);
   evas_object_data_set(o->edje, "comp.effect_obj.cwin",
                        (void *)e_mod_comp_util_client_xid_get(cw));

   res = _effect_obj_win_set(o, cw);
   E_CHECK_GOTO(res, fail);

   ly->objs = eina_list_append(ly->objs, o);

   ELBF(ELBT_COMP, 0, o->cwin,
        "%15.15s| OBJ_NEW %p", "EFFECT", o->edje);

   return o;

fail:
   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s| OBJ_NEW Failed", "EFFECT");
   if (o)
     {
        if (o->edje)
          {
             e_layout_unpack(o->edje);
             evas_object_del(o->edje);
          }
        E_FREE(o);
     }
   return NULL;
}

EINTERN void
e_mod_comp_effect_object_free(E_Comp_Effect_Object *o)
{
   E_CHECK(o);

   if ((o->edje) && (o->img))
     edje_object_part_unswallow(o->edje, o->img);

   if (o->img)
     {
        /* TODO: consider when using an xim */
        evas_object_image_native_surface_set(o->img, NULL);
        evas_object_image_size_set(o->img, 1, 1);
        evas_object_image_data_set(o->img, NULL);
        evas_object_hide(o->img);
        evas_object_del(o->img);
     }

   if (o->pixmap)
     ecore_x_pixmap_free(o->pixmap);

   if (o->edje)
     {
        evas_object_data_del(o->edje, "comp.effect_obj.ly");
        evas_object_data_del(o->edje, "comp.effect_obj.cwin");
        evas_object_hide(o->edje);
        evas_object_del(o->edje);
     }

   ELBF(ELBT_COMP, 0, o->cwin,
        "%15.15s| OBJ_FREE", "EFFECT");

   memset(o, 0, sizeof(E_Comp_Effect_Object));

   E_FREE(o);
}

EINTERN Eina_Bool
e_mod_comp_effect_signal_del(E_Comp_Win  *cw,
                             Evas_Object *obj,
                             const char  *name)
{
   return _effect_signal_del_intern(cw, obj, name, EINA_FALSE);
}

EINTERN Eina_Bool
e_mod_comp_effect_jobs_clean(E_Comp_Win  *cw,
                             Evas_Object *obj,
                             const char  *name)
{
   return _effect_signal_del_intern(cw, obj, name, EINA_TRUE);
}

EINTERN Eina_Bool
e_mod_comp_effect_signal_flush(void)
{
   E_Comp_Effect_Job *job;
   Eina_List *l;
   E_Comp_Win *cw;
   E_Comp_Layer *ly;

   EINA_LIST_FOREACH(effect_jobs, l, job)
     {
        if (!job) continue;
        if (job->emitted) continue;

        if ((job->win) && (job->canvas))
          {
             if ((cw = e_mod_comp_win_find(job->win)))
               {
                  if (cw->animating)
                    {
                       ELBF(ELBT_COMP, 0, job->cwin,
                            "%15.15s|     SIG_PEND:%s", "EFFECT",
                            job->emission);
                       continue;
                    }
                  else
                    {
                       e_mod_comp_effect_animating_set(cw->c, cw, EINA_TRUE);
                       job->emitted = EINA_TRUE;

                       edje_object_signal_emit(job->o, job->emission, job->src);

                       ELBF(ELBT_COMP, 0, job->cwin,
                            "%15.15s|     SIG_EMIT>%s", "EFFECT",
                            job->emission);
                    }
               }
             else
               {
                  ELBF(ELBT_COMP, 0, job->cwin,
                       "%15.15s|     SIG_DEL :%s", "EFFECT",
                       job->emission);

                  /* remove this job if cw was already removed */
                  effect_jobs = eina_list_remove(effect_jobs, job);
                  E_FREE(job);
               }
          }
        else
          {
             edje_object_signal_emit(job->o, job->emission, job->src);

             ELBF(ELBT_COMP, 0, job->cwin,
                  "%15.15s| OBJ SIG_EMIT>%s", "EFFECT",
                  job->emission);

             ly = evas_object_data_get(job->o, "comp.effect_obj.ly");
             if (ly) e_mod_comp_layer_effect_set(ly, EINA_TRUE);

             effect_jobs = eina_list_remove(effect_jobs, job);
             E_FREE(job);
          }
     }

   return EINA_TRUE;
}

EINTERN E_Comp_Effect_Type *
e_mod_comp_effect_type_new(void)
{
   E_Comp_Effect_Type *t;
   t = E_NEW(E_Comp_Effect_Type, 1);
   E_CHECK_RETURN(t, 0);
   t->animatable = EINA_TRUE;
   return t;
}

EINTERN void
e_mod_comp_effect_type_free(E_Comp_Effect_Type *type)
{
   E_CHECK(type);
   E_FREE(type);
}

EINTERN Eina_Bool
e_mod_comp_effect_type_setup(E_Comp_Effect_Type *type,
                             Ecore_X_Window win)
{
   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   e_mod_comp_effect_state_setup(type, win);
   e_mod_comp_effect_style_setup(type, win);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_state_setup(E_Comp_Effect_Type *type,
                              Ecore_X_Window win)
{
   Atom type_ret = 0;
   int ret, size_ret = 0;
   unsigned long num_ret = 0, bytes = 0;
   unsigned char *prop_ret = NULL;
   Ecore_X_Display *dpy;
   Eina_Bool is_ok;

   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   dpy = ecore_x_display_get();
   ret = XGetWindowProperty(dpy, win, ATOM_WINDOW_EFFECT_ENABLE, 0, LONG_MAX,
                            False, ECORE_X_ATOM_CARDINAL, &type_ret, &size_ret,
                            &num_ret, &bytes, &prop_ret);
   if (ret == 0) // success
     {
        if (prop_ret && num_ret)
          {
             type->animatable =  prop_ret[0];
             is_ok = EINA_TRUE;
          }
        else
          is_ok = EINA_FALSE;
     }
   else // error
     {
        is_ok = EINA_FALSE;
     }

   if (prop_ret) XFree(prop_ret);
   return is_ok;
}

EINTERN Eina_Bool
e_mod_comp_effect_state_get(E_Comp_Effect_Type *type)
{
   E_CHECK_RETURN(type, 0);
   return type->animatable;
}

EINTERN Eina_Bool
e_mod_comp_effect_style_setup(E_Comp_Effect_Type *type,
                              Ecore_X_Window win)
{
   Ecore_X_Atom *atoms = NULL;
   Eina_Bool res = EINA_FALSE;
   int num = 0;

   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   num = ecore_x_window_prop_atom_list_get
           (win, ATOM_WINDOW_EFFECT_TYPE, &atoms);
   E_CHECK_GOTO((num == 6), cleanup);
   E_CHECK_GOTO(atoms, cleanup);

   type->show     = _effect_style_get(atoms[0]);
   type->hide     = _effect_style_get(atoms[1]);
   type->restack  = _effect_style_get(atoms[2]);
   type->rotation = _effect_style_get(atoms[3]);
   type->focusin  = _effect_style_get(atoms[4]);
   type->focusout = _effect_style_get(atoms[5]);

   res = EINA_TRUE;

cleanup:
   if (atoms) E_FREE(atoms);
   return res;
}

EINTERN E_Comp_Effect_Style
e_mod_comp_effect_style_get(E_Comp_Effect_Type *type,
                            E_Comp_Effect_Kind kind)
{
   E_Comp_Effect_Style res;
   E_CHECK_RETURN(type, E_COMP_EFFECT_STYLE_NONE);
   switch (kind)
     {
      case E_COMP_EFFECT_KIND_SHOW:     res = type->show;     break;
      case E_COMP_EFFECT_KIND_HIDE:     res = type->hide;     break;
      case E_COMP_EFFECT_KIND_RESTACK:  res = type->restack;  break;
      case E_COMP_EFFECT_KIND_ROTATION: res = type->rotation; break;
      case E_COMP_EFFECT_KIND_FOCUSIN:  res = type->focusin;  break;
      case E_COMP_EFFECT_KIND_FOCUSOUT: res = type->focusout; break;
      default: res = E_COMP_EFFECT_STYLE_NONE;                break;
     }
   return res;
}

#define _MAKE_EMISSION(f, x...) do { snprintf(emission, sizeof(emission), f, ##x); } while(0)

EINTERN void
e_mod_comp_effect_win_restack(E_Comp_Win *cw,
                              Eina_Bool   v1,
                              Eina_Bool   v2)
{
   Eina_Bool animatable = e_mod_comp_effect_state_get(cw->eff_type);
   if (!((cw->c->animatable) && (animatable))) return;
   if ((v1) == (v2)) return;

   if ((!v1) && (v2))
     _effect_show(cw);
   else
     _effect_hide(cw);
}

EINTERN void
e_mod_comp_effect_win_show(E_Comp_Win *cw)
{
   Eina_Bool animatable = e_mod_comp_effect_state_get(cw->eff_type);

   if (cw->c->fake_image_launch)
     {
        if ((e_mod_comp_effect_image_launch_window_check(cw->c->eff_img, cw)) &&
            (e_mod_comp_effect_image_launch_running_check(cw->c->eff_img)))
          {
             if (e_mod_comp_effect_image_launch_fake_show_done_check(cw->c->eff_img))
               {
                  e_mod_comp_effect_image_launch_disable(cw->c->eff_img);
                  goto postjob;
               }
             else
               e_mod_comp_effect_image_launch_window_set(cw->c->eff_img, cw->win);
             return;
          }
     }

   if ((cw->c->animatable) && (animatable))
     {
        _effect_show(cw);
     }

postjob:
   /* for the composite window */
   e_mod_comp_effect_signal_add(cw, NULL, "e,state,visible,on,noeffect", "e");

   /* TODO: message of comp object visibility should be sent after finishing effect */
   e_mod_comp_comp_event_src_visibility_send(cw);
}

EINTERN Eina_Bool
e_mod_comp_effect_win_hide(E_Comp_Win *cw)
{
   Eina_Bool animatable, visible;

   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   visible = e_mod_comp_util_win_visible_get(cw);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s| CHECK visible:%d cw->visible:%d a:%d", "EFFECT",
        visible, cw->visible, animatable);

   if ((cw->c->animatable) && (animatable) &&
       (visible))
     {
        _effect_hide(cw);
     }

   /* for the composite window */
   e_mod_comp_effect_signal_add(cw, NULL, "e,state,visible,off,noeffect", "e");

   /* TODO: message of comp object visibility should be sent after finishing effect */
   e_mod_comp_comp_event_src_visibility_send(cw);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_signal_add(E_Comp_Win  *cw,
                             Evas_Object *o,
                             const char  *emission,
                             const char  *src)

{
   Eina_List *l;
   E_Comp *c = NULL;
   E_Comp_Object *co;
   E_Comp_Effect_Job *job = NULL;
   size_t len;

   E_CHECK_RETURN((cw || o), 0);
   E_CHECK_RETURN(emission, 0);
   E_CHECK_RETURN(src, 0);

   if (cw)
     {
        c = cw->c;

        EINA_LIST_FOREACH(cw->objs, l, co)
          {
             if (!co) continue;
             if (!co->shadow) continue;

             job = E_NEW(E_Comp_Effect_Job, 1);
             if (!job) continue;

             job->o = co->shadow;
             job->win = cw->win;
             job->cwin = e_mod_comp_util_client_xid_get(cw);
             job->canvas = co->canvas;

             len = sizeof(job->emission);
             strncpy(job->emission, emission, len);
             job->emission[len-1] = '\0';

             len = sizeof(job->src);
             strncpy(job->src, src, len);
             job->src[len-1] = '\0';

             effect_jobs = eina_list_append(effect_jobs, job);

             e_mod_comp_hw_ov_win_msg_show
               (E_COMP_LOG_TYPE_EFFECT,
               "SIG_ADD 0x%x %s",
               job->cwin, emission);
          }
     }
   else
     {
        job = E_NEW(E_Comp_Effect_Job, 1);
        if (job)
          {
             job->effect_obj = EINA_TRUE;
             job->o = o;
             job->win = 0;
             job->cwin = (Ecore_X_Window)evas_object_data_get(o, "comp.effect_obj.cwin");

             len = sizeof(job->emission);
             strncpy(job->emission, emission, len);
             job->emission[len-1] = '\0';

             len = sizeof(job->src);
             strncpy(job->src, src, len);
             job->src[len-1] = '\0';

             effect_jobs = eina_list_append(effect_jobs, job);
          }
     }

   if (job)
     {
        ELBF(ELBT_COMP, 0, job->cwin,
             "%15.15s| %s  SIG_ADD:%s", "EFFECT",
             job->effect_obj ? "OBJ" : "   ",
             emission);
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_animating_set(E_Comp *c,
                                E_Comp_Win *cw,
                                Eina_Bool set)
{
   Eina_Bool state;
   E_CHECK_RETURN(c, 0);
   E_CHECK_GOTO(cw, postjob);

   if (set)
     {
        if (cw->animating)
          {
             c->animating--;
             _state_send(cw, EINA_FALSE);
          }
        state = EINA_TRUE;
     }
   else
     {
        state = EINA_FALSE;
     }

   cw->animating = state;
   _state_send(cw, state);

postjob:
   if (set) c->animating++;
   else c->animating--;
   return EINA_TRUE;
}

/* local subsystem functions */
static E_Comp_Effect_Style
_effect_style_get(Ecore_X_Atom a)
{
   E_CHECK_RETURN(a, E_COMP_EFFECT_STYLE_NONE);

   if      (a == ATOM_EFFECT_DEFAULT) return E_COMP_EFFECT_STYLE_DEFAULT;
   else if (a == ATOM_EFFECT_NONE   ) return E_COMP_EFFECT_STYLE_NONE;
   else if (a == ATOM_EFFECT_CUSTOM0) return E_COMP_EFFECT_STYLE_CUSTOM0;
   else if (a == ATOM_EFFECT_CUSTOM1) return E_COMP_EFFECT_STYLE_CUSTOM1;
   else if (a == ATOM_EFFECT_CUSTOM2) return E_COMP_EFFECT_STYLE_CUSTOM2;
   else if (a == ATOM_EFFECT_CUSTOM3) return E_COMP_EFFECT_STYLE_CUSTOM3;
   else if (a == ATOM_EFFECT_CUSTOM4) return E_COMP_EFFECT_STYLE_CUSTOM4;
   else if (a == ATOM_EFFECT_CUSTOM5) return E_COMP_EFFECT_STYLE_CUSTOM5;
   else if (a == ATOM_EFFECT_CUSTOM6) return E_COMP_EFFECT_STYLE_CUSTOM6;
   else if (a == ATOM_EFFECT_CUSTOM7) return E_COMP_EFFECT_STYLE_CUSTOM7;
   else if (a == ATOM_EFFECT_CUSTOM8) return E_COMP_EFFECT_STYLE_CUSTOM8;
   else if (a == ATOM_EFFECT_CUSTOM9) return E_COMP_EFFECT_STYLE_CUSTOM9;

   return E_COMP_EFFECT_STYLE_NONE;
}

static Eina_Bool
_state_send(E_Comp_Win *cw,
            Eina_Bool state)
{
   long d[5] = {0L, 0L, 0L, 0L, 0L};
   Ecore_X_Window win;
   E_CHECK_RETURN(cw, 0);

   win = e_mod_comp_util_client_xid_get(cw);
   E_CHECK_RETURN(win, 0);

   if (state) d[0] = 1L;
   else d[1] = 1L;

   ecore_x_client_message32_send
     (win, ATOM_WINDOW_EFFECT_CLIENT_STATE,
     ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
     d[0], d[1], d[2], d[3], d[4]);

   return EINA_TRUE;
}

static Eina_Bool
_effect_signal_del_intern(E_Comp_Win  *cw,
                          Evas_Object *obj,
                          const char  *name,
                          Eina_Bool    clean_all)
{
   E_Comp_Effect_Job *job;
   Eina_List *l;
   EINA_LIST_FOREACH(effect_jobs, l, job)
     {
        if (!job) continue;
        if (!job->emitted) continue;
        if (!job->win) continue;
        if (!job->canvas) continue;

        Ecore_X_Window win = cw ? cw->win : 0;
        if (job->win == win)
          {
             job->canvas->animation.num--;

             if (job->canvas->animation.num <= 0)
               {
                  job->canvas->animation.run = 0;
                  job->canvas->animation.num = 0;
               }

             ELBF(ELBT_COMP, 0, job->cwin,
                  "%15.15s|     SIG_DEL :%s", "EFFECT",
                  job->emission);

             e_mod_comp_hw_ov_win_msg_show
               (E_COMP_LOG_TYPE_EFFECT,
               "SIG_DEL 0x%x %s",
               job->win, job->emission);

             effect_jobs = eina_list_remove(effect_jobs, job);

             E_FREE(job);

             if (!clean_all) break;
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_effect_obj_win_set(E_Comp_Effect_Object *o,
                    E_Comp_Win           *cw)
{
   E_Comp_Object *co = NULL;
   E_Comp_Layer *ly = NULL;
   const char *file = NULL, *group = NULL;
   int ok = 0, pw = 0, ph = 0;

   ly = evas_object_data_get(o->edje, "comp.effect_obj.ly");
   E_CHECK_RETURN(ly, EINA_FALSE);
   E_CHECK_RETURN(ly->canvas, EINA_FALSE);

   co = eina_list_nth(cw->objs, 0);
   E_CHECK_RETURN(co, EINA_FALSE);

   edje_object_file_get(co->shadow, &file, &group);
   E_CHECK_RETURN(file, EINA_FALSE);
   E_CHECK_RETURN(group, EINA_FALSE);

   ok = edje_object_file_set(o->edje, file, group);
   E_CHECK_RETURN(ok, EINA_FALSE);

   o->img = evas_object_image_filled_add(ly->canvas->evas);
   E_CHECK_RETURN(o->img, EINA_FALSE);

   evas_object_image_colorspace_set(o->img, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_smooth_scale_set(o->img, _comp_mod->conf->smooth_windows);
   if (cw->argb) evas_object_image_alpha_set(o->img, 1);
   else evas_object_image_alpha_set(o->img, 0);

   /* set nocomp mode before getting named pixmap */
   E_Comp_Win *nocomp_cw = e_mod_comp_util_win_nocomp_get(cw->c, ly->canvas->zone);
   if (nocomp_cw == cw)
     e_mod_comp_canvas_nocomp_end(ly->canvas);

   o->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
   E_CHECK_GOTO(o->pixmap, fail);

   ecore_x_pixmap_geometry_get(o->pixmap, NULL, NULL, &pw, &ph);
   E_CHECK_GOTO((pw > 0), fail);
   E_CHECK_GOTO((ph > 0), fail);

   Evas_Native_Surface ns;
   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_X11;
   ns.data.x11.visual = cw->vis;
   ns.data.x11.pixmap = o->pixmap;

   evas_object_image_size_set(o->img, pw, ph);
   evas_object_image_native_surface_set(o->img, &ns);
   evas_object_image_data_update_add(o->img, 0, 0, pw, ph);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s| OBJ_NEW pix:0x%x %dx%d", "EFFECT",
        o->pixmap, pw, ph);

   edje_object_part_swallow(o->edje, "e.swallow.content", o->img);

   e_layout_child_move(o->edje, cw->x, cw->y);
   e_layout_child_resize(o->edje, pw, ph);

   edje_object_signal_callback_add(o->edje, "e,action,show,done", "e", _effect_obj_effect_done, o);
   edje_object_signal_callback_add(o->edje, "e,action,hide,done", "e", _effect_obj_effect_done, o);

   o->win = cw->win;
   o->cwin = e_mod_comp_util_client_xid_get(cw);

   return EINA_TRUE;

fail:
   if (o->img)
     {
        evas_object_del(o->img);
        o->img = NULL;
     }
   if (o->pixmap)
     {
        ecore_x_pixmap_free(o->pixmap);
        o->pixmap = 0;
     }
   return EINA_FALSE;
}

static void
_effect_obj_effect_done(void                 *data,
                        Evas_Object          *obj,
                        const char  *emission __UNUSED__,
                        const char  *source   __UNUSED__)
{
   E_Comp_Effect_Object *o = data;
   E_Comp_Layer *ly;
   E_CHECK(o);
   E_CHECK(obj);

   ly = evas_object_data_get(obj, "comp.effect_obj.ly");
   E_CHECK(ly);

   /* decrease effect count and hide effect layer if it is 0 */
   e_mod_comp_layer_effect_set(ly, EINA_FALSE);
}

static void
_effect_win_set(E_Comp_Win *cw,
                const char *emission)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;

   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   if (ly)
     {
        o = e_mod_comp_effect_object_new(ly, cw);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             e_mod_comp_effect_signal_add(NULL, o->edje, emission, "e");
          }
     }
}

static void
_effect_below_wins_set(E_Comp_Win *cw)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;
   Eina_Inlist *l;
   E_Comp_Win *_cw = cw, *nocomp_cw = NULL;
   E_Zone *zone;
   char emission[64];
   _MAKE_EMISSION("e,state,visible,on,noeffect");

   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   E_CHECK(ly);

   zone = canvas->zone;
   E_CHECK(zone);

   nocomp_cw = e_mod_comp_util_win_nocomp_get(cw->c, zone);

   /* create effect objects until finding a non-alpha full-screen window */
   while ((l = EINA_INLIST_GET(_cw)->prev))
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        if (!(_cw->visible)) continue;
        if (_cw->invalid) continue;
        if (_cw->input_only) continue;
        if (!E_INTERSECTS(zone->x, zone->y, zone->w, zone->h,
                          _cw->x, _cw->y, _cw->w, _cw->h))
          continue;

        /* if nocomp exist, change mode to the composite mode */
        if ((nocomp_cw) && (nocomp_cw == _cw))
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(_cw),
                  "%15.15s| END EFFECT_OBJ_NEW", "NOCOMP");
             e_mod_comp_canvas_nocomp_end(canvas);
          }
        else if (!((_cw->pixmap) &&
                   (_cw->pw > 0) && (_cw->ph > 0) &&
                   (_cw->dmg_updates >= 1)))
          {
             continue;
          }

        o = e_mod_comp_effect_object_new(ly, _cw);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             e_mod_comp_effect_signal_add(NULL, o->edje, emission, "e");

             /* change the stack position of the object to the bottom
              * of layer and also background object too
              */
             e_layout_child_lower(o->edje);
             e_mod_comp_layer_bg_adjust(ly);
          }

        /* found a non-alpha full-screen window */
        if ((REGION_EQUAL_TO_ZONE(_cw, zone)) && !(_cw->argb))
          break;
     }
}

static void
_effect_above_wins_set(E_Comp_Win *cw,
                       Eina_Bool   show)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;
   Eina_Inlist *l;
   E_Comp_Win *_cw = cw;
   E_Zone *zone;
   char emission[64];

   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   E_CHECK(ly);

   zone = canvas->zone;
   E_CHECK(zone);

   while ((l = EINA_INLIST_GET(_cw)->next))
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        if (!(_cw->visible)) continue;
        if (_cw->invalid) continue;
        if (_cw->input_only) continue;
        if (!E_INTERSECTS(zone->x, zone->y, zone->w, zone->h,
                          _cw->x, _cw->y, _cw->w, _cw->h))
          continue;
        if (!((_cw->pixmap) &&
              (_cw->pw > 0) && (_cw->ph > 0) &&
              (_cw->dmg_updates >= 1)))
          continue;

        _MAKE_EMISSION("e,state,visible,on,noeffect");

        if ((TYPE_KEYBOARD_CHECK(_cw)) && (!show))
          {
             if (cw->c->keyboard_effect)
               {
                  if (e_mod_comp_effect_win_angle_get(_cw))
                    _MAKE_EMISSION("e,state,visible,off,angle,%d", _cw->angle);
                  else
                    _MAKE_EMISSION("e,state,visible,off,angle,0");
               }
             else
               continue;
          }

        o = e_mod_comp_effect_object_new(ly, _cw);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             e_mod_comp_effect_signal_add(NULL, o->edje, emission, "e");
          }
     }
}

static Eina_Bool
_effect_obj_find(E_Comp_Win *cw)
{
   E_Comp_Canvas *canvas = eina_list_nth(cw->c->canvases, 0);
   E_CHECK_RETURN(canvas, EINA_FALSE);

   E_Comp_Layer *ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   E_CHECK_RETURN(ly, EINA_FALSE);

   Eina_List *l;
   E_Comp_Effect_Object *o = NULL;
   EINA_LIST_FOREACH(ly->objs, l, o)
     {
        if (!o) continue;
        if (o->win == cw->win)
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s| OBJ already exists!", "EFFECT");
             return EINA_TRUE;
          }
     }

   return EINA_FALSE;
}

static void
_effect_show(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   char emission[64];
   E_Comp_Win *cw2 = NULL;
   Eina_Bool launch, visible;

   Eina_Bool res = _effect_obj_find(cw);
   E_CHECK(!res);

   /* check effect condition and make emission string */
   st = e_mod_comp_effect_style_get(cw->eff_type, E_COMP_EFFECT_KIND_SHOW);
   visible = e_mod_comp_util_win_visible_get(cw);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|SHOW", "EFFECT");

   if (E_COMP_EFFECT_STYLE_DEFAULT == st)
     {
        launch = e_mod_comp_policy_app_launch_check(cw);
        if ((launch) && (visible))
          {
             _effect_below_wins_set(cw);

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|>FG", "EFFECT");

             _MAKE_EMISSION("e,state,visible,on");
             _effect_win_set(cw, emission);
             _effect_above_wins_set(cw, EINA_TRUE);
          }
        else if ((TYPE_KEYBOARD_CHECK(cw)) && (visible))
          {
             if (cw->c->keyboard_effect)
               {
                  _effect_below_wins_set(cw);

                  if (e_mod_comp_effect_win_angle_get(cw))
                    _MAKE_EMISSION("e,state,window,angle,%d", cw->angle);
                  else
                    _MAKE_EMISSION("e,state,window,angle,0");

                  _effect_win_set(cw, emission);
                  _effect_above_wins_set(cw, EINA_TRUE);
               }
          }
        /* don't need to check visibility for home window by home key */
        else if (TYPE_HOME_CHECK(cw))
          {
             /* app window hide effect by pressing the home key */
             cw2 = e_mod_comp_util_win_normal_get(NULL);
             if (cw2)
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));

                  /* background is home */
                  _MAKE_EMISSION("e,state,visible,on,noeffect");
                  _effect_win_set(cw, emission);

                  /* app window will hide */
                  _MAKE_EMISSION("e,state,visible,off");
                  _effect_win_set(cw2, emission);
                  _effect_above_wins_set(cw2, EINA_FALSE);
               }
          }
     }
}

static void
_effect_hide(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   char emission[64];
   E_Comp_Win *cw2 = NULL;
   Eina_Bool close;

   Eina_Bool res = _effect_obj_find(cw);
   E_CHECK(!res);

   /* check effect condition and make emission string */
   st = e_mod_comp_effect_style_get(cw->eff_type, E_COMP_EFFECT_KIND_HIDE);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|HIDE", "EFFECT");

   if (E_COMP_EFFECT_STYLE_DEFAULT == st)
     {
        close = e_mod_comp_policy_app_close_check(cw);
        if (close)
          {
             /* background window */
             cw2 = e_mod_comp_util_win_normal_get(cw);
             if (cw2)
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|>BG 0x%08x", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw2));

                  _MAKE_EMISSION("e,state,visible,on,noeffect");
                  _effect_win_set(cw2, emission);
               }

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|>FG", "EFFECT");

             _MAKE_EMISSION("e,state,visible,off");
             _effect_win_set(cw, emission);
             _effect_above_wins_set(cw, EINA_FALSE);
          }
        else if (TYPE_KEYBOARD_CHECK(cw))
          {
             if (cw->c->keyboard_effect)
               {
                  _effect_below_wins_set(cw);

                  if (e_mod_comp_effect_win_angle_get(cw))
                    _MAKE_EMISSION("e,state,visible,off,angle,%d", cw->angle);
                  else
                    _MAKE_EMISSION("e,state,visible,off,angle,0");

                  _effect_win_set(cw, emission);
                  _effect_above_wins_set(cw, EINA_FALSE);
               }
          }
     }
}
