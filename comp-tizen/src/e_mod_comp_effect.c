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
   char           emission[1024];
   char           src[1024];
   Eina_Bool      emitted;
   Eina_Bool      wait_previous_effect_done;
};

static Eina_List   *effect_jobs = NULL;

/* local subsystem functions */
static E_Comp_Effect_Style _effect_style_get(Ecore_X_Atom a);
static void                _effect_stage_enable(E_Comp_Win *cw, E_Comp_Win *cw2);
static Eina_Bool           _state_send(E_Comp_Win *cw, Eina_Bool state);
static Eina_Bool           _effect_animating_check(const char *emission);
static Ecore_X_Window      _comp_win_xid_get(E_Comp_Win *cw);
static Eina_Bool           _effect_signal_del_intern(E_Comp_Win *cw, Evas_Object *obj, const char *name, Eina_Bool clean_all);

/* externally accessible functions */
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
                       job->wait_previous_effect_done = EINA_TRUE;
                       continue;
                    }
                  else
                    {
                       if (strncmp(job->emission, "e,state,raise_above_post,on", sizeof("e,state,raise_above_post,on")) == 0)
                         {
                            // special contonl this signal
                            edje_object_signal_emit(job->o, job->emission, job->src);

                            ELBF(ELBT_COMP, 0, job->win,
                                 "%15.15s|EMIT %s", "SIGNAL",
                                 job->emission);

                            effect_jobs = eina_list_remove(effect_jobs, job);
                            E_FREE(job);
                         }
                       else
                         {
                            e_mod_comp_effect_animating_set(cw->c, cw, EINA_TRUE);
                            job->canvas->animation.run = 1;
                            job->canvas->animation.num++;
                            job->emitted = EINA_TRUE;

                            if (job->wait_previous_effect_done)
                              {
                                 // do check effectable & signal emit
                                 // or send_done and do not send signal emit
                                 if ((strncmp(job->emission, "e,state,visible,on", sizeof("e,state,visible,on")) == 0) ||
                                     /* e,state,window,angle is used for keyboard show effect */
                                     (strncmp(job->emission, "e,state,window,angle", sizeof("e,state,window,angle")) == 0) ||
                                     (strncmp(job->emission, "e,state,visible,off", sizeof("e,state,visible,off")) == 0))
                                   {
                                      // do effect signal emit
                                      edje_object_signal_emit(job->o, job->emission, job->src);

                                      ELBF(ELBT_COMP, 0, job->win,
                                           "%15.15s|EMIT %s", "SIGNAL",
                                           job->emission);
                                   }
                                 else if (strncmp(job->emission, "e,state,window,rotation,", sizeof("e,state,window,rotation")) == 0)
                                   {
                                      e_mod_comp_explicit_win_rotation_done(cw);
                                   }
                                 else if (strncmp(job->emission, "e,state,raise_above,on", sizeof("e,state,raise_above,on")) == 0)
                                   {
                                      e_mod_comp_explicit_raise_above_show_done(cw);
                                   }
                                 else if (strncmp(job->emission, "e,state,raise_above,off", sizeof("e,state,raise_above,off")) == 0)
                                   {
                                      e_mod_comp_explicit_raise_above_hide_done(cw);
                                   }
                              }
                            else
                              {
                                 edje_object_signal_emit(job->o, job->emission, job->src);

                                 ELBF(ELBT_COMP, 0, job->win,
                                      "%15.15s|EMIT %s", "SIGNAL",
                                      job->emission);
                              }
                         }
                    }
               }
             else
               {
                  // if cw is not exist then clear effect job
                  effect_jobs = eina_list_remove(effect_jobs, job);
                  E_FREE(job);
               }
          }
        else
          { // effect for not window ex) capture. etc...
             edje_object_signal_emit(job->o, job->emission, job->src);

             ELBF(ELBT_COMP, 0, job->win,
                  "%15.15s|EMIT %s", "SIGNAL",
                  job->emission);

             effect_jobs = eina_list_remove(effect_jobs, job);
             E_FREE(job);
          }
     }

   return 1;
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

EINTERN void
e_mod_comp_effect_win_show(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   Eina_Bool animatable, launch;
   E_Comp_Win *bg_cw = NULL;
   char emission[64];
   E_CHECK(cw);
   E_CHECK(cw->c);

   cw->first_show_worked = EINA_TRUE;

   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_SHOW);

   if (cw->c->fake_image_launch)
     {
        if ((e_mod_comp_effect_image_launch_window_check(cw->c->eff_img, cw)) &&
            (e_mod_comp_effect_image_launch_running_check(cw->c->eff_img)))
          {
             if (e_mod_comp_effect_image_launch_fake_show_done_check(cw->c->eff_img))
               {
                  e_mod_comp_effect_image_launch_disable(cw->c->eff_img);
                  e_mod_comp_effect_signal_add(cw, NULL, "e,state,visible,on,noeffect", "e");
                  goto postjob;
               }
             else
               e_mod_comp_effect_image_launch_window_set(cw->c->eff_img, cw->win);
             return;
          }
     }

   if (!cw->c->animatable ||
       !animatable)
     {
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,on,noeffect", "e");
        goto postjob;
     }

   switch (st)
     {
      case E_COMP_EFFECT_STYLE_DEFAULT:
        launch = e_mod_comp_policy_app_launch_check(cw);
        if (launch)
          {
             if (!(cw->launched))
               {
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,on", "e");
               }
             else
               {
                 e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,on,noeffect", "e");
               }
             goto postjob;
          }

        if (TYPE_KEYBOARD_CHECK(cw))
          {
             if (cw->c->keyboard_effect)
               {
                  if (e_mod_comp_effect_win_angle_get(cw))
                    {
                       snprintf(emission, sizeof(emission),
                                "e,state,window,angle,%d", cw->angle);
                       e_mod_comp_effect_signal_add(cw, NULL, emission, "e");
                    }
                  else
                     e_mod_comp_effect_signal_add
                        (cw, NULL, "e,state,window,angle,0", "e");
               }
             else
                e_mod_comp_effect_signal_add
                   (cw, NULL, "e,state,visible,on,noeffect", "e");
          }
        else
          e_mod_comp_effect_signal_add
            (cw, NULL, "e,state,visible,on,noeffect", "e");
        break;
      case E_COMP_EFFECT_STYLE_NONE:
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,on,noeffect", "e");
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM0:
        if (TYPE_INDICATOR_CHECK(cw))
          {
             switch (cw->angle)
               {
                case 90:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,on,custom0,90", "e");
                  break;
                case 180:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,on,custom0,180", "e");
                  break;
                case 270:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,on,custom0,270", "e");
                  break;
                case 0:
                default:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,on,custom0,0", "e");
                  break;
               }
          }
        else
          {
             e_mod_comp_effect_signal_add
               (cw, NULL, "e,state,visible,on,custom0", "e");
          }
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM1:
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,on,custom1", "e");
        break;
      default:
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,on", "e");
        break;
     }

postjob:
   e_mod_comp_comp_event_src_visibility_send(cw);
}

EINTERN Eina_Bool
e_mod_comp_effect_win_hide(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   Eina_Bool animatable;
   Eina_Bool close = EINA_FALSE;
   char emission[64];
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN((!cw->input_only), 0);
   E_CHECK_RETURN((!cw->invalid), 0);

   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_HIDE);

   if ((!cw->c->animatable) ||
       (!animatable))
     {
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,off,noeffect", "e");
        goto postjob;
     }

   switch (st)
     {
      case E_COMP_EFFECT_STYLE_DEFAULT:
        if (TYPE_KEYBOARD_CHECK(cw))
          {
             if (cw->c->keyboard_effect)
               {
                  if (e_mod_comp_effect_win_angle_get(cw))
                    {
                       snprintf(emission, sizeof(emission), "e,state,visible,off,angle,%d", cw->angle);
                       e_mod_comp_effect_signal_add(cw, NULL, emission, "e");
                    }
                  else
                    e_mod_comp_effect_signal_add
                       (cw, NULL, "e,state,visible,off,angle,0", "e");
               }
             else
                e_mod_comp_effect_signal_add
                   (cw, NULL, "e,state,visible,off,noeffect", "e");
          }
        else
          ;
        break;
      case E_COMP_EFFECT_STYLE_NONE:
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,off,noeffect", "e");
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM0:
        if (TYPE_INDICATOR_CHECK(cw))
          {
             switch (cw->angle)
               {
                case 90:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,off,custom0,90", "e");
                  break;
                case 180:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,off,custom0,180", "e");
                  break;
                case 270:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,off,custom0,270", "e");
                  break;
                case 0:
                default:
                  e_mod_comp_effect_signal_add
                    (cw, NULL, "e,state,visible,off,custom0,0", "e");
                  break;
               }
          }
        else
          {
             e_mod_comp_effect_signal_add
               (cw, NULL, "e,state,visible,off,custom0", "e");
          }
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM1:
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,visible,off,custom1", "e");
        break;
      default:
        e_mod_comp_effect_signal_add
           (cw, NULL, "e,state,visible,off", "e");
        break;
     }

postjob:
   e_mod_comp_comp_event_src_visibility_send(cw);
   return EINA_TRUE;
}

EINTERN void
e_mod_comp_effect_win_restack(E_Comp_Win *cw,
                              E_Comp_Win *cw2)
{
   E_Comp_Win *_cw = cw2;
   Eina_Inlist *l;

   E_CHECK(cw);
   E_CHECK(cw2);

   L(LT_EFFECT,
     "[COMP] %18.18s w:0x%08x %s\n",
     "EFF", e_mod_comp_util_client_xid_get(cw),
     "RAISE_ABOVE");

   while (_cw->defer_hide || !(REGION_EQUAL_TO_ROOT(_cw)) ||
          _cw->input_only || _cw->invalid ||
          !(TYPE_NORMAL_CHECK(_cw) || TYPE_HOME_CHECK(_cw) ||
            TYPE_TASKMANAGER_CHECK(_cw)))
     {
        l = EINA_INLIST_GET(_cw)->prev;
        E_CHECK(l);
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        E_CHECK(_cw);
     }
   E_CHECK(_cw->visible);

   if (e_mod_comp_policy_app_launch_check(cw))
     {
        e_mod_comp_win_comp_objs_stack_above(cw, cw2);
        if (cw->launched)
           e_mod_comp_effect_signal_add
             (cw, NULL, "e,state,raise_above,on", "e");     }
   else if (e_mod_comp_policy_app_close_check(_cw))
     {
        if (cw->c->defer_raise_effect)
          {
             cw->defer_raise = EINA_TRUE;
             _cw->defer_raise = EINA_TRUE;
          }
        else
          e_mod_comp_win_comp_objs_stack_above(cw, cw2);

        e_mod_comp_effect_mirror_handler_hide(cw, _cw);
     }
   else
     {
        e_mod_comp_win_comp_objs_stack_above(cw, cw2);
        e_mod_comp_effect_signal_add
          (cw, NULL, "e,state,raise_above,on", "e");
     }
}

EINTERN void
e_mod_comp_effect_win_lower(E_Comp_Win *cw,
                            E_Comp_Win *cw2)
{
   E_CHECK(cw);
   E_CHECK(cw2);

   cw->defer_raise = EINA_TRUE;

   if (e_mod_comp_policy_app_close_check(cw))
     e_mod_comp_effect_signal_add
       (cw, NULL, "e,state,raise_above,off", "e");
}

EINTERN void
e_mod_comp_effect_mirror_handler_hide(E_Comp_Win *cw,
                            E_Comp_Win *cw2)
{
   E_Manager_Comp_Source *src = NULL;
   E_Comp_Object *co;
   E_Comp *c;
   Eina_Bool res;
   Eina_Inlist *l;

   E_CHECK(cw);
   E_CHECK(cw2);

   c = cw->c;
   E_CHECK(c);
   E_CHECK(c->mirror_handler);

   src = e_manager_comp_src_get(c->man, cw2->win);
   c->mirror_obj = e_manager_comp_src_image_mirror_add(c->man, src);

   res = edje_object_part_swallow(c->mirror_handler, "e.swallow.content", c->mirror_obj);
   E_CHECK(res);

   evas_object_show(c->mirror_obj);
   evas_object_show(c->mirror_handler);

   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (!co->shadow) continue ;
          evas_object_stack_above(c->mirror_handler, co->shadow);
     }

   e_mod_comp_effect_signal_add
     (NULL, c->mirror_handler, "e,state,mirror,visible,off", "e");

}


EINTERN void
e_mod_comp_effect_disable_stage(E_Comp *c,
                                E_Comp_Win *cw)
{
   E_Comp_Win *_cw;
   E_Comp_Object *co;
   Eina_List *l;
   E_CHECK(c);

   if (cw)
     {
        if (!cw->effect_stage) return;
        cw->effect_stage = 0;
     }

   E_CHECK(c->effect_stage);
   c->effect_stage = EINA_FALSE;

   EINA_INLIST_FOREACH(c->wins, _cw)
     {
        if (!_cw) continue;
        if ((_cw->invalid) || (_cw->input_only) ||
            (!_cw->visible))
          {
             // Do recovery state of window visibility
             // when was window animation effect occured,
             // unrelated window was hided.
             _cw->animate_hide = EINA_FALSE;
             continue;
          }

        EINA_LIST_FOREACH(_cw->objs, l, co)
          {
             if (!co) continue;
             if (!_cw->animate_hide &&
                 evas_object_visible_get(co->shadow))
               {
                  continue;
               }
             if (!_cw->first_show_worked &&
                 !evas_object_visible_get(co->shadow))
               {
                  continue;
               }
             if (!cw->hidden_override && cw->show_done)
               evas_object_show(co->shadow);
          }
        _cw->animate_hide = 0;
     }
}

#define _CHECK(c, s, e) {                       \
   if (!strncmp(s, e, strlen(s)))               \
     {                                          \
        ELBF(ELBT_COMP, 0,                      \
             e_mod_comp_util_client_xid_get(c), \
             "%15.15s|%s", "SIGNAL", e);        \
     }                                          \
}

EINTERN Eina_Bool
e_mod_comp_effect_signal_add(E_Comp_Win *cw,
                             Evas_Object *o,
                             const char *emission,
                             const char *src)

{
   Eina_List *l;
   E_Comp *c = NULL;
   E_Comp_Object *co;
   E_Comp_Effect_Job *job;
   size_t len;

   E_CHECK_RETURN((cw || o), 0);
   E_CHECK_RETURN(emission, 0);
   E_CHECK_RETURN(src, 0);

   if (cw)
     {
        c = cw->c;

        E_CHECK_GOTO(_effect_animating_check(emission), finish);

        EINA_LIST_FOREACH(cw->objs, l, co)
          {
             if (!co) continue;
             if (!co->shadow) continue;

             job = E_NEW(E_Comp_Effect_Job, 1);
             if (!job) continue;

             job->o = co->shadow;
             job->win = _comp_win_xid_get(cw);
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
               e_mod_comp_util_client_xid_get(cw),
               emission);
          }
     }
   else
     {
        job = E_NEW(E_Comp_Effect_Job, 1);
        if (job)
          {
             job->o = o;

             len = sizeof(job->emission);
             strncpy(job->emission, emission, len);
             job->emission[len-1] = '\0';

             len = sizeof(job->src);
             strncpy(job->src, src, len);
             job->src[len-1] = '\0';

             effect_jobs = eina_list_append(effect_jobs, job);
          }
     }

   e_mod_comp_effect_signal_flush();

finish:

   if (!(logtype & LT_EFFECT)) return EINA_TRUE;

   _CHECK(cw, "e,state,visible,on",             emission);
   _CHECK(cw, "e,state,visible,on,noeffect",    emission);
   _CHECK(cw, "e,state,visible,on,custom0",     emission);
   _CHECK(cw, "e,state,visible,on,custom0,0",   emission);
   _CHECK(cw, "e,state,visible,on,custom0,90",  emission);
   _CHECK(cw, "e,state,visible,on,custom0,180", emission);
   _CHECK(cw, "e,state,visible,on,custom0,270", emission);
   _CHECK(cw, "e,state,visible,on,custom1",     emission);
   _CHECK(cw, "e,state,visible,off",            emission);
   _CHECK(cw, "e,state,visible,off,noeffect",   emission);
   _CHECK(cw, "e,state,visible,off,custom0",    emission);
   _CHECK(cw, "e,state,visible,off,custom0,0",  emission);
   _CHECK(cw, "e,state,visible,off,custom0,90", emission);
   _CHECK(cw, "e,state,visible,off,custom0,180",emission);
   _CHECK(cw, "e,state,visible,off,custom0,270",emission);
   _CHECK(cw, "e,state,visible,off,custom1",    emission);
   _CHECK(cw, "e,state,raise_above,off",        emission);
   _CHECK(cw, "e,state,raise_above_post,on",    emission);
   _CHECK(cw, "e,state,switcher_top,on",        emission);
   _CHECK(cw, "e,state,switcher,on",            emission);
   _CHECK(cw, "e,state,shadow,on",              emission);
   _CHECK(cw, "e,state,shadow,off",             emission);
   _CHECK(cw, "e,state,focus,on",               emission);
   _CHECK(cw, "e,state,focus,off",              emission);
   _CHECK(cw, "e,state,urgent,on",              emission);
   _CHECK(cw, "e,state,urgent,off",             emission);
   _CHECK(cw, "e,state,window,rotation,90",     emission);
   _CHECK(cw, "e,state,window,rotation,180",    emission);
   _CHECK(cw, "e,state,window,rotation,0",      emission);
   _CHECK(cw, "e,state,window,rotation,-180",   emission);
   _CHECK(cw, "e,state,window,rotation,-90",    emission);
   _CHECK(cw, "e,state,rotation,on",            emission);
   _CHECK(cw, "img,state,capture,on",           emission);
   _CHECK(cw, "e,state,window,angle,0",         emission);
   _CHECK(cw, "e,state,window,angle,90",        emission);
   _CHECK(cw, "e,state,window,angle,180",       emission);
   _CHECK(cw, "e,state,window,angle,270",       emission);
   _CHECK(cw, "e,state,visible,off,angle,0",    emission);
   _CHECK(cw, "e,state,visible,off,angle,90",   emission);
   _CHECK(cw, "e,state,visible,off,angle,180",  emission);
   _CHECK(cw, "e,state,visible,off,angle,270",  emission);

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

#define _STR_CHECK(s1,s2) (!(strncmp(s1, s2, strlen(s2))))

static Eina_Bool
_effect_animating_check(const char *emission)
{
   Eina_Bool ret = EINA_TRUE;
   E_CHECK_RETURN(emission, 0);

   if (_STR_CHECK(emission, "e,state,shadow,on"))
     ret = EINA_FALSE;
   else if (_STR_CHECK(emission, "e,state,shadow,off"))
     ret = EINA_FALSE;
   else if (_STR_CHECK(emission, "e,state,focus,on"))
     ret = EINA_FALSE;
   else if (_STR_CHECK(emission, "e,state,focus,off"))
     ret = EINA_FALSE;
   else if (_STR_CHECK(emission, "e,state,urgent,on"))
     ret = EINA_FALSE;
   else if (_STR_CHECK(emission, "e,state,urgent,off"))
     ret = EINA_FALSE;
   else
     ret = EINA_TRUE;

   return ret;
}

static Ecore_X_Window
_comp_win_xid_get(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   return cw->win;
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

        if (job->win == _comp_win_xid_get(cw))
          {
             job->canvas->animation.num--;

             if (job->canvas->animation.num <= 0)
               {
                  job->canvas->animation.run = 0;
                  job->canvas->animation.num = 0;
               }

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
