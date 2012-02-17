#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_tm.h"

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

/* local subsystem functions */
static E_Comp_Effect_Style _effect_style_get(Ecore_X_Atom a);
static void                _bg_show(E_Comp_Win *cw);
static void                _bg_hide(E_Comp_Win *cw);
static void                _effect_stage_enable(E_Comp_Win *cw, E_Comp_Win *cw2);
static Eina_Bool           _state_send(E_Comp_Win *cw, Eina_Bool state);
static Eina_Bool           _effect_animating_check(const char *emission);

/* externally accessible functions */
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
   Eina_Bool res = EINA_FALSE;
   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   res = e_mod_comp_effect_state_setup(type, win);
   E_CHECK_RETURN(res, 0);
   res = e_mod_comp_effect_style_setup(type, win);
   E_CHECK_RETURN(res, 0);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_state_setup(E_Comp_Effect_Type *type,
                              Ecore_X_Window win)
{
   unsigned int val = 0;
   int ret;

   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   ret = ecore_x_window_prop_card32_get
           (win, ATOM_WINDOW_EFFECT_ENABLE, &val, 1);
   E_CHECK_RETURN((ret > 0), 0);
   type->animatable = val;

   return EINA_TRUE;
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
   int num = 0;

   E_CHECK_RETURN(type, 0);
   E_CHECK_RETURN(win, 0);

   num = ecore_x_window_prop_atom_list_get
           (win, ATOM_WINDOW_EFFECT_TYPE, &atoms);
   E_CHECK_RETURN((num == 6), 0);
   E_CHECK_RETURN(atoms, 0);

   type->show     = _effect_style_get(atoms[0]);
   type->hide     = _effect_style_get(atoms[1]);
   type->restack  = _effect_style_get(atoms[2]);
   type->rotation = _effect_style_get(atoms[3]);
   type->focusin  = _effect_style_get(atoms[4]);
   type->focusout = _effect_style_get(atoms[5]);

   if (atoms) E_FREE(atoms);
   return EINA_TRUE;
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
   Eina_Bool img_launch, done;
   E_Comp_Effect_Style st;
   Eina_Bool animatable, launch;
   Evas_Object *o;
   E_CHECK(cw);
   E_CHECK(cw->c);

   cw->first_show_worked = EINA_TRUE;

   img_launch = e_mod_comp_effect_image_launch_window_check(cw->c->eff_img, cw);
   done = e_mod_comp_effect_image_launch_done_check(cw->c->eff_img);
   if ((img_launch) && !(done))
     {
        e_mod_comp_effect_image_launch_window_set(cw->c->eff_img, cw->win);
        return;
     }

   o = cw->shobj;
   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_SHOW);

   if (img_launch && done)
     {
        e_mod_comp_effect_signal_add
           (cw, o, "e,state,visible,on,noeffect", "e");
        e_mod_comp_effect_image_launch_disable(cw->c->eff_img);
        goto postjob;
     }

   if (!cw->c->animatable ||
       !animatable)
     {
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,on,noeffect", "e");
        goto postjob;
     }

   switch (st)
     {
      case E_COMP_EFFECT_STYLE_DEFAULT:
        launch = e_mod_comp_policy_app_launch_check(cw);
        if (TYPE_TASKMANAGER_CHECK(cw))
          e_mod_comp_effect_tm_bg_show(cw);
        else if (launch) _bg_show(cw);
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,on", "e");
        break;
      case E_COMP_EFFECT_STYLE_NONE:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,on,noeffect", "e");
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM0:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,on,custom0", "e");
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM1:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,on,custom1", "e");
        break;
      default:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,on", "e");
        break;
     }

postjob:
   e_mod_comp_comp_event_src_visibility_send(cw);
}

EINTERN Eina_Bool
e_mod_comp_effect_win_hide(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   Eina_Bool animatable, close;
   Evas_Object *o;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN((!cw->input_only), 0);
   E_CHECK_RETURN((!cw->invalid), 0);

   o = cw->shobj;
   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_HIDE);

   if ((!cw->c->animatable) ||
       (!animatable))
     {
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,off,noeffect", "e");
        goto postjob;
     }

   switch (st)
     {
      case E_COMP_EFFECT_STYLE_DEFAULT:
        close = e_mod_comp_policy_app_close_check(cw);
        if (TYPE_TASKMANAGER_CHECK(cw))
          e_mod_comp_effect_tm_bg_hide(cw);
        else if (!cw->animating && close)
          _bg_hide(cw);
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,off", "e");
        break;
      case E_COMP_EFFECT_STYLE_NONE:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,off,noeffect", "e");
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM0:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,off,custom0", "e");
        break;
      case E_COMP_EFFECT_STYLE_CUSTOM1:
        e_mod_comp_effect_signal_add
          (cw, o, "e,state,visible,off,custom1", "e");
        break;
      default:
        e_mod_comp_effect_signal_add
           (cw, o, "e,state,visible,off", "e");
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
     "[COMP] %18.18s w:0x%08x %s tm:%d tm->obscured:%d\n",
     "EFF", e_mod_comp_util_client_xid_get(cw),
     "RAISE_ABOVE", cw->c->switcher,
     cw->c->switcher_obscured);

   while (_cw->defer_hide || !(REGION_EQUAL_TO_ROOT(_cw)) ||
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
        evas_object_stack_above(cw->shobj, cw2->shobj);
        _effect_stage_enable(_cw, cw);
        e_mod_comp_effect_signal_add
          (_cw, _cw->shobj,
          "e,state,background,visible,on", "e");
        e_mod_comp_effect_signal_add
          (cw, cw->shobj,
          "e,state,visible,on", "e");
     }
   else if (e_mod_comp_policy_app_close_check(_cw))
     {
        cw->defer_raise = EINA_TRUE;
        _cw->defer_raise = EINA_TRUE;
        _effect_stage_enable(cw, _cw);
        e_mod_comp_effect_signal_add
          (_cw, _cw->shobj, "e,state,raise_above,off", "e");
        e_mod_comp_effect_signal_add
          (cw, cw->shobj, "e,state,background,visible,off", "e");
     }
   else
     {
        evas_object_stack_above(cw->shobj, cw2->shobj);
        e_mod_comp_effect_signal_add
          (cw, cw->shobj, "e,state,visible,on", "e");
     }
}

EINTERN void
e_mod_comp_effect_win_lower(E_Comp_Win *cw,
                            E_Comp_Win *cw2)
{
   Eina_Bool close = EINA_FALSE;
   E_CHECK(cw);
   E_CHECK(cw2);

   cw->defer_raise = EINA_TRUE;

   close = e_mod_comp_policy_app_close_check(cw);
   if (close)
     {
        _effect_stage_enable(cw2, cw);
        e_mod_comp_effect_signal_add
          (cw2, cw2->shobj, "e,state,background,visible,off", "e");
     }

   e_mod_comp_effect_signal_add
     (cw, cw->shobj, "e,state,raise_above,off", "e");
}

EINTERN void
e_mod_comp_effect_disable_stage(E_Comp *c,
                                E_Comp_Win *cw)
{
   E_Comp_Win *_cw;
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

        if ((!_cw->animate_hide) &&
            evas_object_visible_get(_cw->shobj))
          {
             continue;
          }

        if ((!_cw->first_show_worked) &&
            !evas_object_visible_get(_cw->shobj))
          {
             continue;
          }

        _cw->animate_hide = EINA_FALSE;
        evas_object_show(_cw->shobj);
     }
}

#define _CHECK(c, s, e) {                    \
   if (!strncmp(s, e, strlen(s)))            \
     {                                       \
        L(LT_EFFECT,                         \
          "[COMP] %18.18s w:0x%08x %s\n",    \
          "SIGNAL",                          \
          e_mod_comp_util_client_xid_get(c), \
          e);                                \
     }                                       \
}

EINTERN Eina_Bool
e_mod_comp_effect_signal_add(E_Comp_Win *cw,
                             Evas_Object *o,
                             const char *emission,
                             const char *src)

{
   E_Comp *c = NULL;
   E_CHECK_RETURN(o, 0);
   E_CHECK_RETURN(emission, 0);
   E_CHECK_RETURN(src, 0);

   if (cw) c = cw->c;

   if (_effect_animating_check(emission))
     e_mod_comp_effect_animating_set(c, cw, EINA_TRUE);

   edje_object_signal_emit(o, emission, src);
   if (!(logtype & LT_EFFECT)) return EINA_TRUE;

   _CHECK(cw, "e,state,visible,on",             emission);
   _CHECK(cw, "e,state,visible,on,noeffect",    emission);
   _CHECK(cw, "e,state,visible,on,custom0",     emission);
   _CHECK(cw, "e,state,visible,on,custom1",     emission);
   _CHECK(cw, "e,state,visible,off",            emission);
   _CHECK(cw, "e,state,visible,off,noeffect",   emission);
   _CHECK(cw, "e,state,visible,off,custom0",    emission);
   _CHECK(cw, "e,state,visible,off,custom1",    emission);
   _CHECK(cw, "e,state,background,visible,on",  emission);
   _CHECK(cw, "e,state,background,visible,off", emission);
   _CHECK(cw, "e,state,raise_above,off",        emission);
   _CHECK(cw, "e,state,raise_above_post,on",    emission);
   _CHECK(cw, "e,state,switcher_top,on",        emission);
   _CHECK(cw, "e,state,switcher,on",            emission);
   _CHECK(cw, "e,state,shadow,on",              emission);
   _CHECK(cw, "e,state,shadow,off",             emission);
   _CHECK(cw, "e,state,focus,on",               emission);
   _CHECK(cw, "e,state,urgent,on",              emission);
   _CHECK(cw, "e,state,window,rotation,90",     emission);
   _CHECK(cw, "e,state,window,rotation,180",    emission);
   _CHECK(cw, "e,state,window,rotation,0",      emission);
   _CHECK(cw, "e,state,window,rotation,-180",   emission);
   _CHECK(cw, "e,state,window,rotation,-90",    emission);
   _CHECK(cw, "e,state,rotation,on",            emission);
   _CHECK(cw, "img,state,capture,on",           emission);

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
        if (!cw->animating)
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

static void
_bg_show(E_Comp_Win *cw)
{
   E_Comp_Win *bg_cw = NULL;
   bg_cw = e_mod_comp_util_win_below_get(cw, 0);
   E_CHECK(bg_cw);

   _effect_stage_enable(cw, bg_cw);
   e_mod_comp_effect_signal_add
     (bg_cw, bg_cw->shobj,
     "e,state,background,visible,on", "e");
}

static void
_bg_hide(E_Comp_Win *cw)
{
   E_Comp_Win *bg_cw = NULL;
   bg_cw = e_mod_comp_util_win_below_get(cw, 0);
   E_CHECK(bg_cw);

   _effect_stage_enable(cw, bg_cw);
   e_mod_comp_effect_signal_add
     (bg_cw, bg_cw->shobj,
     "e,state,background,visible,off", "e");
}

static void
_effect_stage_enable(E_Comp_Win *cw,
                     E_Comp_Win *cw2)
{
   E_Comp_Win *_cw = cw;
   Eina_Inlist *l;
   E_CHECK(cw);
   E_CHECK(cw2);

   // do hide window which is not related window animation effect.
   while ((l = EINA_INLIST_GET(_cw)->prev) != NULL)
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        E_CHECK(_cw);
        if ((_cw->invalid) || (_cw->input_only) ||
            (_cw->win == cw->win) ||
            (_cw->win == cw2->win) ||
            TYPE_INDICATOR_CHECK(_cw) ||
            !evas_object_visible_get(_cw->shobj))
          {
             continue;
          }

        _cw->animate_hide = EINA_TRUE;
        evas_object_hide(_cw->shobj);
     }

   evas_object_stack_below
     (cw->c->bg_img,
     evas_object_bottom_get(cw->c->evas));

   evas_object_show(cw->shobj);
   evas_object_show(cw2->shobj);

   cw->animate_hide = EINA_FALSE;
   cw2->animate_hide = EINA_FALSE;
   cw->effect_stage = EINA_TRUE;
   cw->c->effect_stage = EINA_TRUE;
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
   else if (_STR_CHECK(emission, "e,state,urgent,on"))
     ret = EINA_FALSE;
   else if (_STR_CHECK(emission, "e,state,raise_above_post,on"))
     ret = EINA_FALSE;
   else
     ret = EINA_TRUE;

   return ret;
}
