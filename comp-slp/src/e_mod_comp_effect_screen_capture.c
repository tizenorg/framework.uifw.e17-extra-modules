#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_screen_capture.h"

struct _E_Comp_Effect_Screen_Capture
{
   Evas_Object *obj;
   struct {
      int x, y, w, h;
   } geom;
};

/* local subsystem functions */
static void _show_done(void *data, Evas_Object *obj, const char *emission, const char *source);

/* screen capture effect functions */
EINTERN E_Comp_Effect_Screen_Capture *
e_mod_comp_effect_screen_capture_new(Evas *evas,
                                     int w, int h)
{
   E_Comp_Effect_Screen_Capture *cap;
   int ok;

   E_CHECK_RETURN(evas, 0);
   E_CHECK_RETURN((w <= 0), 0);
   E_CHECK_RETURN((h <= 0), 0);
   E_CHECK_RETURN(_comp_mod, 0);
   E_CHECK_RETURN(_comp_mod->conf, 0);

   cap = E_NEW(E_Comp_Effect_Screen_Capture, 1);
   E_CHECK_RETURN(cap, 0);

   cap->geom.x = 0;
   cap->geom.y = 0;
   cap->geom.w = w;
   cap->geom.h = h;

   cap->obj = edje_object_add(evas);
   E_CHECK_GOTO(cap->obj, error);

   ok = edje_object_file_set(cap->obj,
                             _comp_mod->conf->shadow_file,
                             "capture_effect");
   E_CHECK_GOTO(ok, error);

   evas_object_move(cap->obj, 0, 0);
   evas_object_resize(cap->obj, w, h);

   edje_object_signal_callback_add
     (cap->obj, "img,capture,show,done", "img", _show_done, cap);

   return cap;

error:
   if (cap)
     {
        if (cap->obj)
          {
             e_mod_comp_debug_edje_error_get(cap->obj, 0);
             evas_object_del(cap->obj);
             cap->obj = NULL;
          }
        E_FREE(cap);
     }
   return NULL;
}

EINTERN void
e_mod_comp_effect_screen_capture_free(E_Comp_Effect_Screen_Capture *cap)
{
   E_CHECK(cap);
   if (evas_object_visible_get(cap->obj))
     evas_object_hide(cap->obj);
   E_FREE(cap);
}

EINTERN Eina_Bool
e_mod_comp_effect_screen_capture_handler_message(Ecore_X_Event_Client_Message *ev)
{
   E_Comp *c;
   Evas_Object *o;
   E_CHECK_RETURN(ev, 0);

   c = e_mod_comp_find(ev->win);
   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(c->animatable, 0);
   E_CHECK_RETURN(c->eff_cap, 0);

   o = c->eff_cap->obj;
   E_CHECK_RETURN(o, 0);

   evas_object_show(o);
   evas_object_raise(o);
   e_mod_comp_effect_signal_add
     (NULL, o, "img,state,capture,on", "img");

   return EINA_TRUE;
}

/* local subsystem functions */
static void
_show_done(void        *data,
           Evas_Object *obj      __UNUSED__,
           const char  *emission __UNUSED__,
           const char  *source   __UNUSED__)
{
   E_Comp_Effect_Screen_Capture *cap;
   cap = (E_Comp_Effect_Screen_Capture *)data;
   E_CHECK(cap);

   e_mod_comp_effect_animating_set(NULL, NULL, EINA_FALSE);

   if (evas_object_visible_get(cap->obj))
     evas_object_hide(cap->obj);
}
