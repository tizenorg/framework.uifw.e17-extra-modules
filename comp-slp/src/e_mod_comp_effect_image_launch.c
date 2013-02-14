#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_image_launch.h"

struct _E_Comp_Effect_Image_Launch
{
   Eina_Bool       running : 1;
   Eina_Bool       launch_done : 1; // image launch edje object got effect done or not.
   Evas_Object    *obj;             // image object
   Evas_Object    *shobj;           // image shadow object
   Ecore_Timer    *timeout;         // max time between show, hide image launch
   Ecore_X_Window  win;             // this represent image launch effect's real window id.
   int             w, h;            // width and height of image object
};

/* local subsystem functions */
static void      _show_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _hide_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _launch_timeout(void *data);
static void      _invisible_win_hide(E_Comp_Win *cw, Eina_Bool show);

/* local subsystem globals */

/* externally accessible functions */
EINTERN E_Comp_Effect_Image_Launch *
e_mod_comp_effect_image_launch_new(Evas *e,
                                   int w, int h)
{
   E_Comp_Effect_Image_Launch *eff;
   int ok = 0;
   E_CHECK_RETURN(e, NULL);

   eff = E_NEW(E_Comp_Effect_Image_Launch, 1);
   E_CHECK_RETURN(eff, NULL);

   eff->shobj = edje_object_add(e);
   E_CHECK_RETURN(eff->shobj, NULL);

   ok = edje_object_file_set
          (eff->shobj,
          _comp_mod->conf->shadow_file,
          "fake_effect_twist");
   E_CHECK_GOTO(ok, error);

   evas_object_move(eff->shobj, 0, 0);
   evas_object_resize(eff->shobj, w, h);

   eff->obj = evas_object_image_add(e);
   E_CHECK_GOTO(eff->obj, error);

   edje_object_signal_callback_add
     (eff->shobj, "fake,action,hide,done", "fake",
     _hide_done, eff);

   edje_object_signal_callback_add
     (eff->shobj, "fake,action,show,done", "fake",
     _show_done, eff);

   return eff;

error:
   e_mod_comp_effect_image_launch_free(eff);
   return NULL;
}

EINTERN void
e_mod_comp_effect_image_launch_free(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK(eff);
   if (eff->shobj) evas_object_del(eff->shobj);
   if (eff->obj) evas_object_del(eff->obj);
   E_FREE(eff);
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_handler_message(Ecore_X_Event_Client_Message *ev)
{
   E_Comp_Effect_Image_Launch *eff;
   E_Comp *c = NULL;
   char *file = NULL;

   E_CHECK_RETURN(ev, 0);

   c = e_mod_comp_find(ev->win);
   E_CHECK_RETURN(c, 0);

   eff = c->eff_img;
   E_CHECK_RETURN(eff, 0);

   if (ev->data.l[0] == 0)
     {
        e_mod_comp_effect_image_launch_hide(eff);
     }
   else if (ev->data.l[0] == 1)
     {
        file = ecore_x_window_prop_string_get
                 (ev->win, ATOM_IMAGE_LAUNCH_FILE);
        E_CHECK_RETURN(file, 0);

        e_mod_comp_effect_image_launch_show(eff, file);
        E_FREE(file);
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_show(E_Comp_Effect_Image_Launch *eff,
                                    const char *file)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *bg_cw = NULL;
   Eina_Bool grabbed;
   Evas_Load_Error err;
   Eina_Bool res;

   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->running, 0);
   E_CHECK_RETURN(file, 0);
   E_CHECK_RETURN(c, 0);

   grabbed = e_mod_comp_util_grab_key_set(EINA_TRUE);
   E_CHECK_RETURN(grabbed, 0);

   eff->running = EINA_TRUE;

   evas_object_image_file_set(eff->obj, file, NULL);
   err = evas_object_image_load_error_get(eff->obj);
   E_CHECK_GOTO(err != EVAS_LOAD_ERROR_NONE, error);

   evas_object_image_size_get(eff->obj, &(eff->w), &(eff->h));
   evas_object_image_fill_set(eff->obj, 0, 0, eff->w, eff->h);
   evas_object_image_filled_set(eff->obj, EINA_TRUE);
   res = edje_object_part_swallow
           (eff->shobj, "fake.swallow.content", eff->obj);
   E_CHECK_GOTO(res, error);

   evas_object_show(eff->obj);

   bg_cw = e_mod_comp_util_win_normal_get();
   if (bg_cw)
     {
        _invisible_win_hide(bg_cw, EINA_TRUE);
     }

   evas_object_show(eff->shobj);
   if (c->animatable)
     {
        e_mod_comp_effect_signal_add
          (NULL, eff->shobj,
          "fake,state,visible,on", "fake");
     }
   else
     {
        e_mod_comp_effect_signal_add
          (NULL, eff->shobj,
          "fake,state,visible,on,noeffect", "fake");
     }
   eff->timeout = ecore_timer_add(10.0f, _launch_timeout, eff);
   e_mod_comp_util_screen_input_region_set(EINA_TRUE);
   return EINA_TRUE;

error:
   eff->running = EINA_FALSE;
   if (grabbed)
     e_mod_comp_util_grab_key_set(EINA_FALSE);
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_hide(E_Comp_Effect_Image_Launch *eff)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *bg_cw = NULL;
   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->running, 0);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   // background hide effect
   bg_cw = e_mod_comp_util_win_normal_get();
   if (bg_cw)
     {
        _invisible_win_hide(bg_cw, EINA_FALSE);
     }

   if (c->animatable)
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off", "fake");
   else
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off,noeffect", "fake");

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_window_check(E_Comp_Effect_Image_Launch *eff,
                                            E_Comp_Win *cw)
{
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->running, 0);
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);

   if (!(eff->win)
       && REGION_EQUAL_TO_ROOT(cw)
       && TYPE_NORMAL_CHECK(cw))
     {
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_done_check(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK_RETURN(eff, 0);
   return eff->running;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_window_set(E_Comp_Effect_Image_Launch *eff,
                                          Ecore_X_Window w)
{
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(w, 0);
   eff->win = w;
   return EINA_TRUE;
}

EINTERN void
e_mod_comp_effect_image_launch_disable(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK(eff);
   E_CHECK(eff->running);

   eff->running = EINA_FALSE;
   eff->launch_done = EINA_FALSE;
   eff->win = 0;

   e_mod_comp_util_screen_input_region_set(EINA_FALSE);
   e_mod_comp_util_grab_key_set(EINA_FALSE);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   evas_object_hide(eff->shobj);
   edje_object_part_unswallow(eff->shobj, eff->obj);
}

static void
_show_done(void *data,
           Evas_Object *obj __UNUSED__,
           const char *emission __UNUSED__,
           const char *source __UNUSED__)
{
   E_Comp_Effect_Image_Launch *eff;
   E_Comp_Win *cw;
   eff = (E_Comp_Effect_Image_Launch *)data;
   E_CHECK(eff);

   eff->launch_done = EINA_TRUE;

   e_mod_comp_effect_image_launch_disable(eff);

   E_CHECK(eff->win);
   E_CHECK(eff->running);

   cw = e_mod_comp_win_find(eff->win);
   E_CHECK(cw);

   e_mod_comp_effect_signal_add
     (cw, cw->shobj,
     "e,state,visible,on,noeffect", "e");
}

static void
_hide_done(void *data,
           Evas_Object *obj __UNUSED__,
           const char *emission __UNUSED__,
           const char *source __UNUSED__)
{
   E_Comp_Effect_Image_Launch *eff;
   eff = (E_Comp_Effect_Image_Launch *)data;
   E_CHECK(eff);

   e_mod_comp_util_screen_input_region_set(EINA_FALSE);
   e_mod_comp_util_grab_key_set(EINA_FALSE);

   evas_object_hide(eff->shobj);
   edje_object_part_unswallow(eff->shobj, eff->obj);
}

static Eina_Bool
_launch_timeout(void *data)
{
   E_Comp_Effect_Image_Launch *eff;
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *bg_cw = NULL;
   eff = (E_Comp_Effect_Image_Launch *)data;

   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->running, 0);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   eff->running = EINA_FALSE;
   eff->win = 0;
   eff->launch_done = EINA_FALSE;

   // background hide effect
   bg_cw = e_mod_comp_util_win_normal_get();
   if (bg_cw)
     {
        _invisible_win_hide(bg_cw, EINA_FALSE);
     }

   if (c->animatable)
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off", "fake");
   else
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off,noeffect", "fake");

   return EINA_FALSE;
}

static void
_invisible_win_hide(E_Comp_Win *cw,
                    Eina_Bool show)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *_cw;
   E_CHECK(cw);
   E_CHECK(c);

   EINA_INLIST_FOREACH(c->wins, _cw)
     {
        if ((!_cw) ||
            (_cw->invalid) ||
            (_cw->input_only) ||
            (_cw->win == cw->win) ||
            TYPE_INDICATOR_CHECK(_cw) ||
            !evas_object_visible_get(_cw->shobj))
          {
             continue;
          }

        // do hide window which is not related window animation effect.
        _cw->animate_hide = EINA_TRUE;
        evas_object_hide(_cw->shobj);
     }

   // show background image when background show animation is emitted.
   evas_object_stack_below
     (c->bg_img, evas_object_bottom_get(c->evas));

   cw->animate_hide = EINA_FALSE;

   if (show)
     {
        if (c->animatable)
          e_mod_comp_effect_signal_add
            (cw, cw->shobj,
            "e,state,fake,background,visible,on", "e");
        else
          e_mod_comp_effect_signal_add
            (cw, cw->shobj,
            "e,state,fake,background,visible,on,noeffect", "e");
     }
   else
     {
        if (c->animatable)
          e_mod_comp_effect_signal_add
            (cw, cw->shobj,
            "e,state,background,visible,off", "e");
        else
          e_mod_comp_effect_signal_add
            (cw, cw->shobj,
            "e,state,background,visible,off,noeffect", "e");
     }
}
