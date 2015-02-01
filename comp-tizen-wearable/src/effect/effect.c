#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include <X11/Xlib.h>

#include "effect.h"
#include "effect_image_launch.h"
#include "effect_win_rotation.h"
#include "effect_pos_animation.h"

static Eina_List *effect_jobs = NULL;

/* local subsystem functions */
static E_Comp_Effect_Style   _effect_style_get(Ecore_X_Atom a);
static Eina_Bool             _state_send(E_Comp_Win *cw, Eina_Bool state);
static Eina_Bool             _effect_signal_del_intern(E_Comp_Win *cw, Evas_Object *obj, const char *name, Eina_Bool clean_all);
static E_Border             *_effect_policy_border_transient_for_border_top_get(E_Border *bd);
static void                  _effect_policy_border_transient_for_group_make(E_Border  *bd, Eina_List **list);

static void                  _effect_win_set(E_Comp_Win *cw, const char *emission, Eina_Bool send_ev, Eina_Bool show, Eina_Bool recreate);
static void                  _effect_win_lower(E_Comp_Win *cw, const char *emission, Eina_Bool recreate);
static void                  _effect_below_wins_set(E_Comp_Win *cw, Eina_Bool recreate);
static void                  _effect_below_floating_wins_set(E_Comp_Win *cw, Eina_Bool show, Eina_Bool recreate);
static void                  _effect_home_active_below_wins_set(E_Comp_Win *cw, Eina_Bool show, Eina_Bool   recreate);

static void                  _effect_above_wins_set(E_Comp_Win *cw, Eina_Bool show, Eina_Bool recreate);

static E_Comp_Effect_Object * _effect_object_control_layer_new(E_Comp_Layer *ly, E_Comp_Win *cw);
static void                  _effect_control_layer_set(E_Comp_Win *cw);

static Eina_Bool             _effect_show(E_Comp_Win *cw, Eina_Bool send_ev);
static Eina_Bool             _effect_hide(E_Comp_Win *cw, Eina_Bool send_ev);
static Eina_Bool             _effect_obj_win_set(E_Comp_Effect_Object *o, E_Comp_Win *cw);
static void                  _effect_obj_win_shape_rectangles_apply(E_Comp_Effect_Object *o, E_Comp_Win *cw);
static void                  _effect_obj_effect_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static E_Comp_Effect_Object *_effect_obj_find(E_Comp_Win *cw);
static Eina_Bool             _effect_obj_win_shaped_check(int w, int h, const Ecore_X_Rectangle *rects, int num);
static void                  _effect_role_handle(E_Comp_Effect_Object *o);
static Eina_Bool             _effect_active_except_bd_check(E_Border *bd);

EAPI Eina_Bool
e_mod_comp_effect_mod_init(E_Comp *c)
{
   E_CHECK_RETURN(c, 0);

   c->effect_funcs.type_new = _effect_mod_type_new;
   c->effect_funcs.type_free = _effect_mod_type_free;
   c->effect_funcs.type_setup = _effect_mod_type_setup;
   c->effect_funcs.state_setup = _effect_mod_state_setup;
   c->effect_funcs.state_get = _effect_mod_state_get;
   c->effect_funcs.state_set = _effect_mod_state_set;
   c->effect_funcs.style_setup = _effect_mod_style_setup;
   c->effect_funcs.style_get = _effect_mod_style_get;
   c->effect_funcs.win_show = _effect_mod_win_show;
   c->effect_funcs.win_hide = _effect_mod_win_hide;
   c->effect_funcs.win_restack = _effect_mod_win_restack;
   c->effect_funcs.signal_add = _effect_mod_signal_add;
   c->effect_funcs.signal_del = _effect_mod_signal_del;
   c->effect_funcs.jobs_clean = _effect_mod_jobs_clean;
   c->effect_funcs.signal_flush = _effect_mod_signal_flush;
   c->effect_funcs.animating_set = _effect_mod_animating_set;
   c->effect_funcs.object_new = _effect_mod_object_new;
   c->effect_funcs.object_free = _effect_mod_object_free;
   c->effect_funcs.object_win_set = _effect_mod_object_win_set;
   c->effect_funcs.above_wins_set = _effect_mod_above_wins_set;

   c->effect_funcs.image_launch_new = _effect_mod_image_launch_new;
   c->effect_funcs.image_launch_free = _effect_mod_image_launch_free;
   c->effect_funcs.image_launch_handler_message = _effect_mod_image_launch_handler_message;
   c->effect_funcs.image_launch_show = _effect_mod_image_launch_show;
   c->effect_funcs.image_launch_hide = _effect_mod_image_launch_hide;
   c->effect_funcs.image_launch_window_check = _effect_mod_image_launch_window_check;
   c->effect_funcs.image_launch_running_check = _effect_mod_image_launch_running_check;
   c->effect_funcs.image_launch_fake_show_done_check = _effect_mod_image_launch_fake_show_done_check;
   c->effect_funcs.image_launch_window_set = _effect_mod_image_launch_window_set;
   c->effect_funcs.image_launch_disable = _effect_mod_image_launch_disable;

   c->effect_funcs.win_angle_get = _effect_mod_win_angle_get;
   c->effect_funcs.zone_rotation_new = _effect_mod_zone_rotation_new;
   c->effect_funcs.zone_rotation_free = _effect_mod_zone_rotation_free;
   c->effect_funcs.zone_rotation_begin = _effect_mod_zone_rotation_begin;
   c->effect_funcs.zone_rotation_end = _effect_mod_zone_rotation_end;
   c->effect_funcs.zone_rotation_cancel = _effect_mod_zone_rotation_cancel;
   c->effect_funcs.zone_rotation_do = _effect_mod_zone_rotation_do;
   c->effect_funcs.zone_rotation_clear = _effect_mod_zone_rotation_clear;

   c->effect_funcs.pos_launch_type_get = _effect_mod_pos_launch_type_get;
   c->effect_funcs.pos_launch_make_emission = _effect_mod_pos_launch_make_emission;
   c->effect_funcs.pos_close_type_get = _effect_mod_pos_close_type_get;
   c->effect_funcs.pos_close_make_emission= _effect_mod_pos_close_make_emission;

   return EINA_TRUE;
}

EAPI void
e_mod_comp_effect_mod_shutdown(E_Comp *c)
{
   E_CHECK_RETURN(c, 0);

   c->effect_funcs.type_new = NULL;
   c->effect_funcs.type_free = NULL;
   c->effect_funcs.type_setup = NULL;
   c->effect_funcs.state_setup = NULL;
   c->effect_funcs.state_get = NULL;
   c->effect_funcs.state_set = NULL;
   c->effect_funcs.style_setup = NULL;
   c->effect_funcs.style_get = NULL;
   c->effect_funcs.win_show = NULL;
   c->effect_funcs.win_hide = NULL;
   c->effect_funcs.win_restack = NULL;
   c->effect_funcs.signal_add = NULL;
   c->effect_funcs.signal_del = NULL;
   c->effect_funcs.jobs_clean = NULL;
   c->effect_funcs.signal_flush = NULL;
   c->effect_funcs.animating_set = NULL;
   c->effect_funcs.object_new = NULL;
   c->effect_funcs.object_free = NULL;
   c->effect_funcs.object_win_set = NULL;
   c->effect_funcs.above_wins_set = NULL;

   c->effect_funcs.image_launch_new = NULL;
   c->effect_funcs.image_launch_free = NULL;
   c->effect_funcs.image_launch_handler_message = NULL;
   c->effect_funcs.image_launch_show = NULL;
   c->effect_funcs.image_launch_hide = NULL;
   c->effect_funcs.image_launch_window_check = NULL;
   c->effect_funcs.image_launch_running_check = NULL;
   c->effect_funcs.image_launch_fake_show_done_check = NULL;
   c->effect_funcs.image_launch_window_set = NULL;
   c->effect_funcs.image_launch_disable = NULL;

   c->effect_funcs.win_angle_get = NULL;
   c->effect_funcs.zone_rotation_new = NULL;
   c->effect_funcs.zone_rotation_free = NULL;
   c->effect_funcs.zone_rotation_begin = NULL;
   c->effect_funcs.zone_rotation_end = NULL;
   c->effect_funcs.zone_rotation_cancel = NULL;
   c->effect_funcs.zone_rotation_do = NULL;
   c->effect_funcs.zone_rotation_clear = NULL;

   c->effect_funcs.pos_launch_type_get = NULL;
   c->effect_funcs.pos_launch_make_emission = NULL;
   c->effect_funcs.pos_close_type_get = NULL;
   c->effect_funcs.pos_close_make_emission= NULL;
}

/* externally accessible functions */
E_Comp_Effect_Object *
_effect_mod_object_new(E_Comp_Layer *ly,
                             E_Comp_Win   *cw,
                             Eina_Bool recreate)
{
   E_Comp_Effect_Object *o = NULL, *o2 = NULL;
   Eina_Bool res;
   E_CHECK_RETURN(ly, NULL);
   E_CHECK_RETURN(cw, NULL);

   o2 = _effect_obj_find(cw);
   if ((o2) && (recreate))
     {
        e_layout_unpack(o2->edje);
        o2 = NULL;
     }
   /* TODO: clean up previous effect job */
   E_CHECK_RETURN(!o2, NULL);

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

void
_effect_mod_object_free(E_Comp_Effect_Object *o)
{
   E_CHECK(o);

   if ((o->cwin) && (o->ev_vis))
     e_comp_event_src_visibility_send(o->cwin, o->show);

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

void
_effect_mod_object_win_set(E_Comp_Win *cw, const char *emission)
{
   _effect_win_set(cw, emission, EINA_FALSE, EINA_FALSE, EINA_FALSE);
}

void
_effect_mod_above_wins_set(E_Comp_Win *cw, Eina_Bool show)
{
   _effect_above_wins_set(cw, show, EINA_FALSE);
}

Eina_Bool
_effect_mod_signal_del(E_Comp_Win  *cw,
                             Evas_Object *obj,
                             const char  *name)
{
   return _effect_signal_del_intern(cw, obj, name, EINA_FALSE);
}

Eina_Bool
_effect_mod_jobs_clean(E_Comp_Win  *cw,
                             Evas_Object *obj,
                             const char  *name)
{
   return _effect_signal_del_intern(cw, obj, name, EINA_TRUE);
}

Eina_Bool
_effect_mod_signal_flush(void)
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
                       _effect_mod_animating_set(cw->c, cw, EINA_TRUE);
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

E_Comp_Effect_Type *
_effect_mod_type_new(void)
{
   E_Comp_Effect_Type *t;
   t = E_NEW(E_Comp_Effect_Type, 1);
   E_CHECK_RETURN(t, 0);
   t->animatable = EINA_TRUE;
   return t;
}

void
_effect_mod_type_free(E_Comp_Effect_Type *type)
{
   E_CHECK(type);
   E_FREE(type);
}

Eina_Bool
_effect_mod_type_setup(E_Comp_Effect_Type *type,
                             Ecore_X_Window win)
{
   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   _effect_mod_state_setup(type, win);
   _effect_mod_style_setup(type, win);

   return EINA_TRUE;
}

Eina_Bool
_effect_mod_state_setup(E_Comp_Effect_Type *type,
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

Eina_Bool
_effect_mod_state_get(E_Comp_Effect_Type *type)
{
   E_CHECK_RETURN(type, 0);
   return type->animatable;
}

void
_effect_mod_state_set(E_Comp_Effect_Type *type, Eina_Bool state)
{
   E_CHECK(type);
   type->animatable = state;
}


Eina_Bool
_effect_mod_style_setup(E_Comp_Effect_Type *type,
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

E_Comp_Effect_Style
_effect_mod_style_get(E_Comp_Effect_Type *type,
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

void
_effect_mod_win_restack(E_Comp_Win *cw,
                              Eina_Bool   v1,
                              Eina_Bool   v2)
{
   Eina_Bool animatable = _effect_mod_state_get(cw->eff_type);
   if (!((cw->c->animatable) && (animatable))) return;
   if ((v1) == (v2)) return;

   if ((!v1) && (v2))
     _effect_show(cw, EINA_FALSE);
   else
     _effect_hide(cw, EINA_FALSE);
}

void
_effect_mod_win_show(E_Comp_Win *cw)
{
   Eina_Bool animatable = _effect_mod_state_get(cw->eff_type);
   Eina_Bool res = EINA_FALSE;

   if (cw->c->fake_image_launch)
     {
        if ((_effect_mod_image_launch_window_check(cw->c->eff_img, cw)) &&
            (_effect_mod_image_launch_running_check(cw->c->eff_img)))
          {
             if (_effect_mod_image_launch_fake_show_done_check(cw->c->eff_img))
               {
                  _effect_mod_image_launch_disable(cw->c->eff_img);
                  goto postjob;
               }
             else
               {
                  _effect_mod_image_launch_window_set(cw->c->eff_img, cw->win);
                  goto postjob;
               }
          }
     }

   if ((cw->c->animatable) && (animatable))
     {
        res = _effect_show(cw, EINA_TRUE);
     }

postjob:
   /* for the composite window */
   _effect_mod_signal_add(cw, NULL, "e,state,visible,on,noeffect", "e");
   e_mod_comp_comp_event_src_visibility_send(cw);
   if (!res)
     {
        e_comp_event_src_visibility_send
          (e_mod_comp_util_client_xid_get(cw), EINA_TRUE);
     }
}

void
_effect_mod_win_hide(E_Comp_Win *cw)
{
   Eina_Bool animatable, vis;
   Eina_Bool res = EINA_FALSE;

   animatable = _effect_mod_state_get(cw->eff_type);
   vis = e_mod_comp_util_win_visible_get(cw, EINA_TRUE);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s| CHECK visible:%d cw->visible:%d a:%d", "EFFECT",
        vis, cw->visible, animatable);

   if ((cw->c->animatable) && (animatable) && (vis))
     {
        res = _effect_hide(cw, EINA_TRUE);
     }

   /* for the composite window */
   _effect_mod_signal_add(cw, NULL, "e,state,visible,off,noeffect", "e");
   e_mod_comp_comp_event_src_visibility_send(cw);
   if (!res)
     {
        e_comp_event_src_visibility_send
          (e_mod_comp_util_client_xid_get(cw), EINA_TRUE);
     }
}

Eina_Bool
_effect_mod_signal_add(E_Comp_Win  *cw,
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

Eina_Bool
_effect_mod_animating_set(E_Comp *c,
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

static void
_effect_role_handle(E_Comp_Effect_Object *o)
{
   E_CHECK(o);
   E_CHECK(o->cwin);
   E_CHECK(o->edje);

   char *hints = NULL;
   char *token = NULL;

   hints = ecore_x_window_prop_string_get(o->cwin, ECORE_X_ATOM_WM_WINDOW_ROLE);

   if (hints)
     token = strtok(hints, " ,");
   while (token != NULL)
     {
        if (!strncmp(token, "no-dim", strlen("no-dim")))
          {
             const char *file = NULL, *group = NULL;
             edje_object_file_get(o->edje, &file, &group);
             E_CHECK_GOTO(group, cleanup);

             if (!strcmp(group, "dialog"))
               edje_object_signal_emit(o->edje, "e,state,dim,off", "e");
          }
        token = strtok (NULL, " ,");
     }

cleanup:
   if (hints) free(hints);
}

static Eina_Bool
_effect_obj_win_set(E_Comp_Effect_Object *o,
                    E_Comp_Win           *cw)
{
   E_Comp_Object *co = NULL;
   E_Comp_Layer *ly = NULL;
   const char *file = NULL, *group = NULL;
   int ok = 0, pw = 0, ph = 0;
   Ecore_X_Image *xim;

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

   if (cw->shaped)
     {
        pw = cw->pw;
        ph = cw->ph;
        E_CHECK_GOTO((pw > 0), fail);
        E_CHECK_GOTO((ph > 0), fail);

        unsigned int *pix;
        xim = ecore_x_image_new(pw, ph, cw->vis, cw->depth);
        E_CHECK_GOTO(xim, fail);
        pix = ecore_x_image_data_get(xim, NULL, NULL, NULL);
        evas_object_image_size_set(o->img, pw, ph);
        evas_object_image_data_set(o->img, pix);
        evas_object_image_data_update_add(o->img, 0, 0, pw, ph);
        if (ecore_x_image_get(xim, cw->win, 0, 0, 0, 0, pw, ph))
          {
             pix = ecore_x_image_data_get(xim, NULL, NULL, NULL);
             evas_object_image_data_set(o->img, pix);
             evas_object_image_data_update_add(o->img, 0, 0, pw, ph);
          }
        _effect_obj_win_shape_rectangles_apply(o, cw);
        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
            "%15.15s| SHAPED OBJ_NEW pix:0x%p %dx%d", "EFFECT",
            xim, pw, ph);
     }
   else
     {
        evas_object_image_colorspace_set(o->img, EVAS_COLORSPACE_ARGB8888);
        evas_object_image_smooth_scale_set(o->img, _comp_mod->conf->smooth_windows);
        if (cw->argb) evas_object_image_alpha_set(o->img, 1);
        else evas_object_image_alpha_set(o->img, 0);

        /* set nocomp mode before getting named pixmap */
        E_Comp_Win *nocomp_cw = e_mod_comp_util_win_nocomp_get(cw->c, ly->canvas->zone);
        if (nocomp_cw == cw)
          e_mod_comp_canvas_nocomp_end(ly->canvas);
        if (cw->dmg_updates)
          o->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
        else if (cw->needpix)
          o->pixmap = e_mod_comp_util_copied_pixmap_get(cw);
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
     }

   edje_object_part_swallow(o->edje, "e.swallow.content", o->img);
   if (cw->dmg_updates)
     e_layout_child_move(o->edje, cw->x, cw->y);
   else if (cw->needpix)
     e_layout_child_move(o->edje, cw->resizing.x, cw->resizing.y);
   e_layout_child_resize(o->edje, pw, ph);

   edje_object_signal_callback_add(o->edje, "e,action,show,done", "e", _effect_obj_effect_done, o);
   edje_object_signal_callback_add(o->edje, "e,action,hide,done", "e", _effect_obj_effect_done, o);

   o->win = cw->win;
   o->cwin = e_mod_comp_util_client_xid_get(cw);

   _effect_role_handle(o);

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

static Eina_Bool
_effect_obj_win_shaped_check(int                      w,
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

static void
_effect_obj_win_shape_rectangles_apply(E_Comp_Effect_Object *o,
                                       E_Comp_Win           *cw)
{
   int num;
   Ecore_X_Rectangle *rects = ecore_x_window_shape_rectangles_get(cw->win, &(num));
   int w, h, i, px, py;
   unsigned int *pix, *p;
   unsigned char *spix, *sp;

   if (rects)
     {
        for (i = 0; i < cw->rects_num; i++)
          {
             E_RECTS_CLIP_TO_RECT(rects[i].x,
                                  rects[i].y,
                                  rects[i].width,
                                  rects[i].height,
                                  0, 0, cw->pw, cw->ph);
          }
     }

   if (!_effect_obj_win_shaped_check(cw->pw, cw->ph, rects, num))
     {
        if (rects)
          free (rects);
        rects = NULL;
     }

   if (rects)
     {
        evas_object_image_size_get(o->img, &w, &h);
        evas_object_image_alpha_set(o->img, 1);
        pix = evas_object_image_data_get(o->img, 1);

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
             evas_object_image_data_set(o->img, pix);
             evas_object_image_data_update_add(o->img, 0, 0, w, h);
          }
        free (rects);
     }
   else
     {
        if (cw->shaped)
          {
             evas_object_image_size_get(o->img, &w, &h);

             evas_object_image_size_set(o->img, w, h);
             evas_object_image_alpha_set(o->img, 0);
             pix = evas_object_image_data_get(o->img, 1);

             if (pix)
               {
                  p = pix;
                  for (py = 0; py < h; py++)
                    {
                       for (px = 0; px < w; px++)
                       *p |= 0xff000000;
                    }
               }
             evas_object_image_data_set(o->img, pix);
             evas_object_image_data_update_add(o->img, 0, 0, w, h);
          }
     }
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
_effect_policy_border_transient_for_group_make(E_Border  *bd,
                                        Eina_List **list)
{
   E_Border *child;
   Eina_List *l;

   if (!bd) return;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->transient.raise)
     {
        EINA_LIST_FOREACH(bd->transients, l, child)
          {
             if (!child) continue;
             if (!child->iconic)
               {
                  *list = eina_list_prepend(*list, child);
                  _effect_policy_border_transient_for_group_make(child, list);
               }
          }
     }
}

static E_Border *
_effect_policy_border_transient_for_border_top_get(E_Border *bd)
{
   E_Border *top_border = NULL;
   Eina_List *transient_list = NULL;

   _effect_policy_border_transient_for_group_make(bd, &transient_list);

   if (transient_list)
     {
        Eina_List *l = NULL;
        E_Border *temp_bd;
        E_Border *temp_bd2;
        E_Border_List *bl;

        bl = e_container_border_list_last(bd->zone->container);
        while ((temp_bd = e_container_border_list_prev(bl)))
          {
             if (top_border) break;
             if (temp_bd == bd) break;

             EINA_LIST_FOREACH(transient_list, l, temp_bd2)
               {
                  if (temp_bd == temp_bd2)
                    {
                       top_border = temp_bd2;
                       break;
                    }
               }
          }
        e_container_border_list_free(bl);
     }

   eina_list_free(transient_list);

   return top_border;
}

static void
_effect_win_set(E_Comp_Win *cw,
                const char *emission,
                Eina_Bool   send_ev,
                Eina_Bool   show,
                Eina_Bool   recreate)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;
   E_Comp_Effect_Object *o2 = NULL;
   E_Comp_Effect_Object *o3 = NULL;

   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   if (ly)
     {
        o = _effect_mod_object_new(ly, cw, recreate);
        if (o)
          {
             /* adjust the stack position of the given border to the above of
              * parent if border belongs to parent using 'transient_for'
              */
             if ((cw->bd) && (cw->bd->parent))
               {
                  E_Comp_Win *cw2 = e_mod_comp_win_find(cw->bd->parent->win);
                  if (cw2)
                    {
                       E_Border *top_border = NULL;
                       top_border = _effect_policy_border_transient_for_border_top_get(cw2->bd);
                       if (top_border)
                         {
                            E_Comp_Win *cw3 = e_mod_comp_border_client_find(top_border->client.win);
                            if (cw3)
                               o3 = _effect_obj_find(cw3);
                         }
                       o2 = _effect_obj_find(cw2);

                       if (o3)
                         e_layout_child_raise_above(o->edje, o3->edje);
                       else if (o2)
                         e_layout_child_raise_above(o->edje, o2->edje);
                    }
               }

             if (cw->skip_blend)
               {  // to copy img with the alpha especially camera_preview
                  evas_object_render_op_set(o->img, EVAS_RENDER_COPY);
                  evas_object_render_op_set(o->edje, EVAS_RENDER_COPY);
               }

             evas_object_show(o->img);
             evas_object_show(o->edje);
             _effect_mod_signal_add(NULL, o->edje, emission, "e");

#if USE_SHADOW
             /* add shadow on floating mode window while doing effect */
             if (STATE_INSET_CHECK(cw))
               edje_object_signal_emit(o->edje, "e,state,shadow,on", "e");
#endif

             if (send_ev)
               {
                  o->ev_vis = send_ev;
                  o->show = show;
               }
          }
     }
}

static void
_effect_win_lower(E_Comp_Win *cw,
                  const char *emission,
                  Eina_Bool   recreate)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;

   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");

   if (ly)
     {
        o = _effect_mod_object_new(ly, cw, recreate);
        if (o)
          {
             if (cw->skip_blend)
               {  // to copy img with the alpha especially camera_preview
                  evas_object_render_op_set(o->img, EVAS_RENDER_COPY);
                  evas_object_render_op_set(o->edje, EVAS_RENDER_COPY);
               }

             evas_object_show(o->img);
             evas_object_show(o->edje);
             _effect_mod_signal_add(NULL, o->edje, emission, "e");

             e_layout_child_lower(o->edje);
             e_mod_comp_layer_bg_adjust(ly);
          }
     }
}

static void
_effect_below_wins_set(E_Comp_Win *cw,
                       Eina_Bool   recreate)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;
   Eina_Inlist *l;
   E_Comp_Win *_cw = cw, *nocomp_cw = NULL;
   E_Zone *zone;
   char emission[64];

   if (TYPE_NORMAL_CHECK(cw) || TYPE_CALL_SCREEN_CHECK(cw) || TYPE_MINI_APPTRAY_CHECK(cw))
     _MAKE_EMISSION("e,state,background,visible,on");
   else
     _MAKE_EMISSION("e,state,visible,on,noeffect");

   canvas = eina_list_nth(cw->c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   E_CHECK(ly);

   zone = canvas->zone;
   E_CHECK(zone);

   nocomp_cw = e_mod_comp_util_win_nocomp_get(cw->c, zone);
#if USE_NOCOMP_DISPOSE
   E_Comp_Win *nocomp_end_cw = e_mod_comp_util_win_nocomp_end_get(cw->c, zone);
#endif

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
#if USE_NOCOMP_DISPOSE
        else if ((nocomp_end_cw) && (nocomp_end_cw == _cw))
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(_cw),
                  "%15.15s| ENDED EFFECT_OBJ_NEW", "NOCOMP");
          }
#endif
        else if (!((_cw->pixmap) &&
                   (_cw->pw > 0) && (_cw->ph > 0)))
          {
             continue;
          }

        o = _effect_mod_object_new(ly, _cw, recreate);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             if (_cw->skip_blend)
               {  // to copy img with the alpha especially camera_preview
                  evas_object_render_op_set(o->img, EVAS_RENDER_COPY);
                  evas_object_render_op_set(o->edje, EVAS_RENDER_COPY);
               }
             _effect_mod_signal_add(NULL, o->edje, emission, "e");

#if USE_SHADOW
             /* add shadow on floating mode window while doing effect */
             if (STATE_INSET_CHECK(_cw))
               edje_object_signal_emit(o->edje, "e,state,shadow,on", "e");
#endif

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
_effect_below_floating_wins_set(E_Comp_Win *cw,
                                Eina_Bool show,
                                Eina_Bool   recreate)
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

   while ((l = EINA_INLIST_GET(_cw)->prev))
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);

        if (!(_cw->visible)) continue;
        if ((REGION_EQUAL_TO_ZONE(_cw, zone)) && !(_cw->argb) && (!TYPE_VIDEO_CALL_CHECK(_cw))) break;

        _MAKE_EMISSION("e,state,visible,on,noeffect");

        o = _effect_mod_object_new(ly, _cw, recreate);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             _effect_mod_signal_add(NULL, o->edje, emission, "e");
#if USE_SHADOW
             edje_object_signal_emit(o->edje, "e,state,shadow,on", "e");
#endif

             /* change the stack position of the object to the bottom
              * of layer and also background object too
              */
             e_layout_child_lower(o->edje);
             e_mod_comp_layer_bg_adjust(ly);
          }

     }
}

static void
_effect_home_active_below_wins_set(E_Comp_Win *cw,
                                Eina_Bool show,
                                Eina_Bool   recreate)
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

   while ((l = EINA_INLIST_GET(_cw)->prev))
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);

        if (!(_cw->visible)) continue;
        if (TYPE_HOME_CHECK(_cw)) break;

        if (STATE_INSET_CHECK(_cw))
          _MAKE_EMISSION("e,state,visible,on,noeffect");
        else
          _MAKE_EMISSION("e,state,visible,off");

        if (TYPE_VIDEO_CALL_CHECK(cw) && TYPE_VIDEO_CALL_CHECK(_cw))
          {
             if (REGION_EQUAL_TO_ZONE(_cw, zone))
               _MAKE_EMISSION("e,state,visible,off");
             else
               continue;
          }

        o = _effect_mod_object_new(ly, _cw, recreate);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             _effect_mod_signal_add(NULL, o->edje, emission, "e");
#if USE_SHADOW
             edje_object_signal_emit(o->edje, "e,state,shadow,on", "e");
#endif

             /* change the stack position of the object to the bottom
              * of layer and also background object too
              */
             e_layout_child_lower(o->edje);
             e_mod_comp_layer_bg_adjust(ly);
          }

        if ((REGION_EQUAL_TO_ZONE(_cw, zone)) && !(_cw->argb) && (!TYPE_VIDEO_CALL_CHECK(_cw))) break;
     }
}

static void
_effect_above_wins_set(E_Comp_Win *cw,
                       Eina_Bool   show,
                       Eina_Bool   recreate)
{
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;
   E_Comp_Effect_Object *o;
   Eina_Inlist *l;
   E_Comp_Win *_cw = cw;
   E_Zone *zone;
   char emission[64];
   E_Comp_Win *parent_cw = NULL;

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
              (_cw->pw > 0) && (_cw->ph > 0)))
          continue;

        if ((TYPE_KEYBOARD_SUB_CHECK(_cw)) && (!show)) continue;

        if ((REGION_EQUAL_TO_ZONE(_cw, zone)) && (!_cw->argb)) continue;

        if ((_cw->bd) && (_cw->bd->parent))
          {
             parent_cw = e_mod_comp_win_find(_cw->bd->parent->win);
          }

        if ((parent_cw) && (parent_cw == cw) && (!show))
          _MAKE_EMISSION("e,state,visible,off");
        else
          _MAKE_EMISSION("e,state,visible,on,noeffect");

        if ((TYPE_KEYBOARD_CHECK(_cw)) && (!show))
          {
             if (cw->c->keyboard_effect && (!PARENT_FLOAT_CHECK(_cw)))
               {
                  if (_effect_mod_win_angle_get(_cw))
                    _MAKE_EMISSION("e,state,visible,off,angle,%d", _cw->angle);
                  else
                    _MAKE_EMISSION("e,state,visible,off,angle,0");
               }
             else if (!cw->c->keyboard_effect)
               continue;
          }

        o = _effect_mod_object_new(ly, _cw, recreate);
        if (o)
          {
             evas_object_show(o->img);
             evas_object_show(o->edje);
             _effect_mod_signal_add(NULL, o->edje, emission, "e");

#if USE_SHADOW
             /* add shadow on floating mode window while doing effect */
             if (STATE_INSET_CHECK(_cw))
               edje_object_signal_emit(o->edje, "e,state,shadow,on", "e");
#endif
          }
     }
}

static E_Comp_Effect_Object *
_effect_object_control_layer_new(E_Comp_Layer *ly, E_Comp_Win *cw)
{
   E_Comp_Effect_Object *o = NULL;
   int ok = 0;

   E_CHECK_RETURN(ly, NULL);
   E_CHECK_RETURN(ly->canvas, NULL);
   E_CHECK_RETURN(cw, NULL);

   E_Comp_Layer *effect_ly = e_mod_comp_canvas_layer_get(ly->canvas, "effect");
   E_CHECK_GOTO(effect_ly, fail);

   o = E_NEW(E_Comp_Effect_Object, 1);
   E_CHECK_GOTO(o, fail);

   o->edje = edje_object_add(ly->canvas->evas);
   E_CHECK_GOTO(o->edje, fail);

   e_mod_comp_layer_populate(effect_ly, o->edje);

   evas_object_data_set(o->edje, "comp.effect_obj.ly", effect_ly);

   E_Comp_Object *co = NULL;
   const char *file = NULL, *group = NULL;

   co = eina_list_nth(cw->objs, 0);
   E_CHECK_GOTO(co, fail);

   edje_object_file_get(co->shadow, &file, &group);
   E_CHECK_GOTO(file, fail);
   E_CHECK_GOTO(group, fail);

   ok = edje_object_file_set(o->edje, file, "no-effect");
   E_CHECK_GOTO(ok, fail);

   o->img = evas_object_image_filled_add(ly->canvas->evas);
   E_CHECK_GOTO(o->img, fail);

   evas_object_image_source_set(o->img, ly->layout);
   edje_object_part_swallow(o->edje, "e.swallow.content", o->img);

   e_layout_child_move(o->edje, ly->x, ly->y);
   e_layout_child_resize(o->edje, ly->w, ly->h);

   evas_object_show(o->img);
   evas_object_show(o->edje);

   edje_object_signal_callback_add(o->edje, "e,action,show,done", "e", _effect_obj_effect_done, o);
   edje_object_signal_callback_add(o->edje, "e,action,hide,done", "e", _effect_obj_effect_done, o);

   o->win = 0;
   o->cwin = 0;

   effect_ly->objs = eina_list_append(effect_ly->objs, o);

   return o;

fail:
   if (o)
     {
        if (o->img) evas_object_del(o->img);

        if (o->edje)
          {
             e_layout_unpack(o->edje);
             evas_object_del(o->edje);
          }
        E_FREE(o);
     }
   return NULL;


}


static void
_effect_control_layer_set(E_Comp_Win *cw)
{
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Layer *ctrl_ly = NULL;
   E_Comp_Layer *comp_ly = NULL;
   Eina_Bool ctrl_visible;
   Eina_List *lm = NULL;
   Eina_List *l, *ll;
   Evas_Object *member = NULL;
   Evas_Object *active_shadow = NULL;
   Eina_Bool find_ctrl_ly = EINA_FALSE;
   E_Comp_Win *_cw;
   E_Comp_Win *active_window = NULL;
   E_Comp_Object *co;
   char emission[64];

   canvas = eina_list_nth(cw->c->canvases, 0);
   ctrl_ly = e_mod_comp_canvas_layer_get(canvas, "ly-ctrl");
   E_CHECK(ctrl_ly);

   ctrl_visible = evas_object_visible_get(ctrl_ly->layout);
   E_CHECK(ctrl_visible);

   comp_ly = e_mod_comp_canvas_layer_get(canvas, "comp");
   E_CHECK(comp_ly);

   lm = evas_object_smart_members_get(comp_ly->layout);

   EINA_LIST_REVERSE_FOREACH(lm, ll, member)
     {
        if (member == ctrl_ly->layout)
          {
             find_ctrl_ly = EINA_TRUE;
             continue;
          }
        if ((find_ctrl_ly) && (evas_object_visible_get(member)))
          {
             active_shadow = member;
             break;
          }
     }
   E_CHECK(active_shadow);

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        EINA_LIST_FOREACH(_cw->objs, l, co)
          {
             if (!co) continue;
             if (co->shadow == active_shadow)
               {
                  active_window = _cw;
                  break;
               }
          }
     }
   E_CHECK(active_window);

   E_Comp_Effect_Object *active_window_object = _effect_obj_find(active_window);
   E_CHECK(active_window_object);

   E_Comp_Effect_Object *control_layer_object = _effect_object_control_layer_new(ctrl_ly, active_window);
   E_CHECK(control_layer_object);

   _MAKE_EMISSION("e,state,visible,on,noeffect");
   _effect_mod_signal_add(NULL, control_layer_object->edje, emission, "e");

   e_layout_child_raise_above(control_layer_object->edje, active_window_object->edje);
}

static E_Comp_Effect_Object *
_effect_obj_find(E_Comp_Win *cw)
{
   E_Comp_Canvas *canvas = eina_list_nth(cw->c->canvases, 0);
   E_CHECK_RETURN(canvas, NULL);

   E_Comp_Layer *ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   E_CHECK_RETURN(ly, NULL);

   Eina_List *l;
   E_Comp_Effect_Object *o;
   EINA_LIST_FOREACH(ly->objs, l, o)
     {
        if (!o) continue;
        if (o->win == cw->win)
          {
             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s| OBJ already exists!", "EFFECT");
             return o;
          }
     }

   return NULL;
}

/* send_ev: indicates that comp sends the E_EVENT_COMP_SOURCE_VISIBILITY event
 * when effect is done. show and hide effects will set this to true, but restack
 * effect sets false.
 */
static Eina_Bool
_effect_show(E_Comp_Win *cw,
             Eina_Bool   send_ev)
{
   E_Comp_Effect_Style st;
   char emission[64];
   E_Comp_Win *cw2 = NULL, *cw_below = NULL, *home_cw = NULL;
   E_Comp_Effect_Object *o = NULL;
   Eina_Bool launch, vis, res = EINA_FALSE;
   Eina_Bool recreate = EINA_FALSE;

   o = _effect_obj_find(cw);
   E_CHECK_RETURN(!o, EINA_FALSE);

   /* check effect condition and make emission string */
   st = _effect_mod_style_get(cw->eff_type, E_COMP_EFFECT_KIND_SHOW);
   vis = e_mod_comp_util_win_visible_get(cw, EINA_TRUE);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|SHOW", "EFFECT");

   if (E_COMP_EFFECT_STYLE_DEFAULT == st)
     {
        launch = e_mod_comp_policy_app_launch_check(cw);
        if (launch)
          {
             home_cw = e_mod_comp_util_win_home_get(NULL);

             if ((cw->depth == 32) && (home_cw) && (_effect_obj_find(home_cw)))
               {
                  cw2 = e_mod_comp_util_win_normal_get(NULL, EINA_TRUE);
                  if ((cw2) && (cw2 == cw))
                    {
                      /* when 32bit window(Transient for) is resumed*/
                       _effect_below_wins_set(cw, recreate);
                       ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                            "%15.15s|>FG", "EFFECT");
                       _MAKE_EMISSION("e,state,visible,on");
                       _effect_win_set(cw, emission, send_ev, EINA_TRUE, recreate);
                       _effect_above_wins_set(cw, EINA_TRUE, recreate);
                       _effect_control_layer_set(cw);
                       res = EINA_TRUE;
                    }
                  else if ((cw2) && (cw2 != cw))
                    {
                       /* when a stack of window lowered above 32bit window. */
                       recreate = EINA_TRUE;
                       _MAKE_EMISSION("e,state,visible,on,noeffect");
                       _effect_win_set(cw, emission, send_ev, EINA_TRUE, recreate);

                       _MAKE_EMISSION("e,state,visible,off");
                       _effect_win_set(cw2, emission, send_ev, EINA_TRUE, recreate);

                       _effect_above_wins_set(cw, EINA_TRUE, recreate);
                       _effect_control_layer_set(cw);
                       res = EINA_TRUE;
                    }
               }
             else if (vis)
               {
                  if (cw->eff_launch_style == E_COMP_EFFECT_ANIMATION_NONE)
                    return EINA_FALSE;

                  _effect_below_wins_set(cw, recreate);

                  int full_child_count = 0;
                  E_Comp_Win *full_child_cw = NULL;

                  if ((cw->bd) && (cw->bd->transients))
                    {
                       E_Border *child_border = NULL;
                       E_Zone *zone = cw->bd->zone;
                       Eina_List *l;

                       EINA_LIST_FOREACH(cw->bd->transients, l, child_border)
                         {
                            E_Comp_Win *child_cw = e_mod_comp_border_client_find(child_border->client.win);
                            if ((child_cw) && (zone))
                              {
                                 if ((zone) && (REGION_EQUAL_TO_ZONE(child_cw, zone)))
                                   {
                                      full_child_cw = child_cw;
                                      full_child_count++;
                                   }
                              }
                         }
                    }

                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|>FG(Child : 0x%08x) A : %d", "EFFECT",
                       full_child_cw ? e_mod_comp_util_client_xid_get(full_child_cw): 0,
                       full_child_cw ? full_child_cw->argb : 0);

                  _effect_mod_pos_launch_make_emission(cw, emission, sizeof(emission));
                  //cw->eff_launch_style = 0; // consume, once it has emitted.

                  if ((full_child_count == 1) && full_child_cw)
                    {
                       if (full_child_cw->argb)
                         {
                            _effect_win_set(cw, emission, send_ev, EINA_TRUE, recreate);
                            _effect_win_set(full_child_cw, emission, send_ev, EINA_TRUE, recreate);
                         }
                       else
                         _effect_win_set(full_child_cw, emission, send_ev, EINA_TRUE, recreate);
                    }
                  else
                    _effect_win_set(cw, emission, send_ev, EINA_TRUE, recreate);
                  _effect_above_wins_set(cw, EINA_TRUE, recreate);
                  _effect_control_layer_set(cw);
                  res = EINA_TRUE;
               }
          }
        else if ((TYPE_KEYBOARD_CHECK(cw)) && (vis))
          {
             if (cw->c->keyboard_effect)
               {
                  _effect_below_wins_set(cw, recreate);

                  E_Zone *zone = NULL;
                  if ((cw->bd) && (cw->bd->zone))
                    zone = cw->bd->zone;

                  if ((zone) && (zone->rot.block_count))
                    _MAKE_EMISSION("e,state,visible,on,noeffect");
                  else if (_effect_mod_win_angle_get(cw))
                    _MAKE_EMISSION("e,state,window,angle,%d", cw->angle);
                  else
                    _MAKE_EMISSION("e,state,window,angle,0");

                  _effect_win_set(cw, emission, send_ev, EINA_TRUE, recreate);
                  _effect_above_wins_set(cw, EINA_TRUE, recreate);
                  _effect_control_layer_set(cw);
                  res = EINA_TRUE;
               }
          }
        else if ((STATE_INSET_CHECK(cw)) && (vis))
          {
             cw2 = e_mod_comp_util_win_normal_get(NULL, EINA_TRUE);
             if ((cw2) && (cw2->depth == 32) && (_effect_obj_find(cw2)))
               {
                  recreate = EINA_TRUE;
                  _MAKE_EMISSION("e,state,visible,on,noeffect");
                  _effect_win_set(cw2, emission, EINA_FALSE, EINA_TRUE, EINA_TRUE);
               }

             _effect_below_wins_set(cw, recreate);

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|>FG", "EFFECT");

             _MAKE_EMISSION("e,state,visible,on");
             _effect_win_set(cw, emission, send_ev, EINA_TRUE, recreate);
             _effect_above_wins_set(cw, EINA_TRUE, recreate);
             _effect_control_layer_set(cw);
             res = EINA_TRUE;
          }
        /* in most cases, border show events of home window are generated
         * by pressing the h/w home button. at that moment, home window
         * is invisible thus the wm doesn't need to check visibility for
         * home window. just make app closing effect except lock and
         * setup wizard window.
         */
        else if (TYPE_HOME_CHECK(cw) && (!cw->activate))
          {
             Eina_Bool animatable = EINA_FALSE;
             /* app window hide effect by pressing the h/w home button */
             cw2 = e_mod_comp_util_win_normal_get(NULL, EINA_FALSE);

             if (cw2)
               animatable = _effect_mod_state_get(cw2->eff_type);

             if (!animatable)
               return EINA_FALSE;

             /* in case after hide effect proccessed already */
             if ((cw2) && (_effect_obj_find(cw2)))
               return EINA_FALSE;

             if ((cw2) && (cw2->bd) && (_effect_active_except_bd_check(cw2->bd)))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x except_bd SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

             /* In case of that Notification window show*/
             if ((cw2) &&
                 (cw2->win_type == E_COMP_WIN_TYPE_NOTIFICATION))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x Notification SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

            /* In case of that quickpanel window show*/
             if ((cw2) &&
                 (cw2->win_type == E_COMP_WIN_TYPE_QUICKPANEL))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x quickpanel SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

             /* In case of that no-effect 32bit windows show */
             if ((cw2) &&
                 (cw2->win_type == E_COMP_WIN_TYPE_UTILITY))
               {
                  if (cw2->depth == 32)
                    return EINA_FALSE;
                  else
                    {
                       cw_below = e_mod_comp_util_win_below_get(cw2, EINA_FALSE, EINA_FALSE);
                       if ((cw_below) && (cw_below->depth == 32))
                         return EINA_FALSE;
                    }
               }

            /* In case of minimizing. */
             if ((cw2) && STATE_INSET_CHECK(cw2))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x Minimize SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

             /* In case of pressing home button when top normal window transients for floating window. */
             if ((cw2) && (PARENT_FLOAT_CHECK(cw2)))
               {
                  cw2 = NULL;
               }

             /* When top app window is stacked lower already.
              * it is observed in case of a top window is 32bit windows.
              */
             if ((cw2) && (cw2 == cw))
               {
                  cw_below = e_mod_comp_util_win_below_get(cw, EINA_FALSE, EINA_FALSE);
                  if ((cw_below) && (cw_below->depth == 32))
                    cw2 = cw_below;
                  else
                    return EINA_FALSE;
               }
             if (cw2)
               {
                  /* do nothing, if cw2 is such exceptional windows as lock
                   * and setup wizard window. this case usually happens when
                   * system is booting. (first show of the home window)
                   */
                  res = e_mod_comp_policy_home_app_win_check(cw2);

                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x SKIP:%d", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2),
                       !(res));

                  E_CHECK_RETURN(res, EINA_FALSE);
                  /* not enough to add effect to above wins.
                   * because it can be possible to stack cw2 above floating wins.
                   * typically, the case of that cw transients for any floating windows.
                   */
                  _effect_below_floating_wins_set(cw2, EINA_FALSE, recreate);

                  _MAKE_EMISSION("e,state,visible,on,noeffect");
                  _effect_win_lower(cw, emission, recreate);

                  _MAKE_EMISSION("e,state,visible,off");
                  _effect_win_set(cw2, emission, EINA_FALSE, EINA_FALSE, recreate);

                  /* in order to show 'keyboard hide effect with 'app window hide effect',
                   * 'show' argument should be passed to false
                   */
                  _effect_above_wins_set(cw2, EINA_FALSE, recreate);
                  _effect_control_layer_set(cw);
               }
          }
        else if (TYPE_HOME_CHECK(cw) && (cw->activate))
          {
             Eina_Bool animatable = EINA_FALSE;
             /* app window hide effect by pressing the h/w home button */
             cw2 = e_mod_comp_util_win_normal_get(NULL, EINA_FALSE);

             if (!cw2)
               return EINA_FALSE;

             if (TYPE_HOME_CHECK(cw2))
               return EINA_FALSE;

             if (cw2)
               animatable = _effect_mod_state_get(cw2->eff_type);

             if ((!animatable) && (cw->argb))
               return EINA_FALSE;

             /* in case after hide effect proccessed already */
             if ((cw2) && (_effect_obj_find(cw2)))
               return EINA_FALSE;

             if ((cw2) && (cw2->bd) && (_effect_active_except_bd_check(cw2->bd)))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x except_bd SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

             /* In case of that Notification window show*/
             if ((cw2) &&
                 (cw2->win_type == E_COMP_WIN_TYPE_NOTIFICATION))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x Notification SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

            /* In case of that quickpanel window show*/
             if ((cw2) &&
                 (cw2->win_type == E_COMP_WIN_TYPE_QUICKPANEL))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x quickpanel SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

            /* In case of minimizing. */
             if ((cw2) && STATE_INSET_CHECK(cw2))
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw2),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x Minimize SKIP", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2));
                  return EINA_FALSE;
               }

             /* In case of pressing home button when top normal window transients for floating window. */
             if ((cw2) && (PARENT_FLOAT_CHECK(cw2)))
               {
                  cw2 = NULL;
               }

             if (cw2)
               {
                  /* do nothing, if cw2 is such exceptional windows as lock
                   * and setup wizard window. this case usually happens when
                   * system is booting. (first show of the home window)
                   */
                  res = e_mod_comp_policy_home_app_win_check(cw2);

                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|>BG HOME 0x%08x FG 0x%08x SKIP:%d", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw),
                       e_mod_comp_util_client_xid_get(cw2),
                       !(res));

                  E_CHECK_RETURN(res, EINA_FALSE);
                  /* not enough to add effect to above wins.
                   * because it can be possible to stack cw2 above floating wins.
                   * typically, the case of that cw transients for any floating windows.
                   */
                  _effect_home_active_below_wins_set(cw2, EINA_FALSE, recreate);

                  _MAKE_EMISSION("e,state,visible,on,noeffect");
                  _effect_win_lower(cw, emission, recreate);

                  _MAKE_EMISSION("e,state,visible,off");
                  _effect_win_set(cw2, emission, EINA_FALSE, EINA_FALSE, recreate);

                  /* in order to show 'keyboard hide effect with 'app window hide effect',
                   * 'show' argument should be passed to false
                   */
                  _effect_above_wins_set(cw2, EINA_FALSE, recreate);
                  _effect_control_layer_set(cw);
               }
          }
     }

   return res;
}

static Eina_Bool
_effect_hide(E_Comp_Win *cw,
             Eina_Bool   send_ev)
{
   E_Comp_Effect_Style st;
   char emission[64];
   E_Comp_Win *cw2 = NULL, *home_cw = NULL;
   E_Comp_Effect_Object *o = NULL;
   Eina_Bool close, res = EINA_FALSE;
   Eina_Bool recreate = EINA_FALSE;

   o = _effect_obj_find(cw);

   /* check effect condition and make emission string */
   st = _effect_mod_style_get(cw->eff_type, E_COMP_EFFECT_KIND_HIDE);

   ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
        "%15.15s|HIDE", "EFFECT");

   if (E_COMP_EFFECT_STYLE_DEFAULT == st)
     {
        close = e_mod_comp_policy_app_close_check(cw);

        if ((!close) && (o))
          return EINA_FALSE;

        if (close)
          {
             if (cw->eff_close_style == E_COMP_EFFECT_ANIMATION_NONE)
               return EINA_FALSE;
             /* not enough to add effect to above wins.
              * because it can be possible to stack cw above floating wins.
              * typically, the case of that cw transients for any floating windows.
              * when hide effect runs on a normal window, change to show only below floating windows not all below windows.
              */
             _effect_below_floating_wins_set(cw, EINA_TRUE, recreate);

             /* background window */
             cw2 = e_mod_comp_util_win_normal_get(cw, EINA_TRUE);

             /* when 32bit window hides effect doesn't work,
              * because there are effect object of the window already (home show effect).
              * so it need to be handled exceptionally.
              */
             if ((cw2) && (cw2->depth == 32) && (o))
               {
                  home_cw = e_mod_comp_util_win_home_get(cw);
                  if ((home_cw) && (_effect_obj_find(home_cw)))
                    {
                       recreate = EINA_TRUE;
                       _MAKE_EMISSION("e,state,visible,on,noeffect");
                       _effect_win_set(cw2, emission, send_ev, EINA_FALSE, EINA_TRUE);
                       cw2 = NULL;
                    }
                  else
                    return EINA_FALSE;
               }
             else if(o)
               return EINA_FALSE;

             if (cw2)
               {
                  ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                       "%15.15s|>BG 0x%08x", "EFFECT",
                       e_mod_comp_util_client_xid_get(cw2));

                  _MAKE_EMISSION("e,state,background,visible,off");
                  _effect_win_lower(cw2, emission, recreate);
               }

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|>FG", "EFFECT");

             _effect_mod_pos_close_make_emission(cw, emission, sizeof(emission));

             _effect_win_set(cw, emission, send_ev, EINA_FALSE, recreate);
             _effect_above_wins_set(cw, EINA_FALSE, recreate);
             _effect_control_layer_set(cw);

             res = EINA_TRUE;
          }
        else if (TYPE_KEYBOARD_CHECK(cw))
          {
             if (cw->c->keyboard_effect)
               {
                  _effect_below_wins_set(cw, recreate);

                  E_Zone *zone = NULL;
                  if ((cw->bd) && (cw->bd->zone))
                    zone = cw->bd->zone;

                  if ((zone) && (zone->rot.block_count))
                    _MAKE_EMISSION("e,state,visible,off,noeffect");
                  else if (_effect_mod_win_angle_get(cw))
                    _MAKE_EMISSION("e,state,visible,off,angle,%d", cw->angle);
                  else
                    _MAKE_EMISSION("e,state,visible,off,angle,0");

                  _effect_win_set(cw, emission, send_ev, EINA_FALSE, recreate);
                  _effect_above_wins_set(cw, EINA_FALSE, recreate);
                  _effect_control_layer_set(cw);

                  res = EINA_TRUE;
               }
          }
        else if (STATE_INSET_CHECK(cw))
          {
             _effect_below_wins_set(cw, recreate);

             ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                  "%15.15s|>FG", "EFFECT");

             _MAKE_EMISSION("e,state,visible,off");
             _effect_win_set(cw, emission, send_ev, EINA_FALSE, recreate);
             _effect_above_wins_set(cw, EINA_FALSE, recreate);
             _effect_control_layer_set(cw);
          }
     }

   return res;
}

static Eina_Bool
_effect_active_except_bd_check(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if (!clas) return EINA_FALSE;
   if (!name) return EINA_FALSE;
   if (strncmp(clas,"setting_encrypting_menu", strlen("setting_encrypting_menu")) == 0) return EINA_TRUE;
   if (strncmp(name,"org.tizen.setting.encrypting", strlen("org.tizen.setting.encrypting")) == 0) return EINA_TRUE;

   return EINA_FALSE;
}
