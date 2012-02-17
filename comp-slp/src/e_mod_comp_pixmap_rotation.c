#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_update.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_pixmap_rotation.h"

struct _E_Comp_Pixmap_Rotation
{
   Eina_Bool       run;
   unsigned int    angle;
   Evas_Object    *shobj;
   Evas_Object    *obj;
   Ecore_X_Pixmap  pixmap;
   Ecore_X_Damage  dmg;
   int             pw, ph;
   Eina_Bool       force;
   Eina_Bool       defer_show;
   Eina_Bool       defer_hide;
   Eina_Bool       angle_changed;
};

/* externally accessible functions */
EINTERN E_Comp_Pixmap_Rotation *
e_mod_comp_pixmap_rotation_new(void)
{
   E_Comp_Pixmap_Rotation *o;
   o = E_NEW(E_Comp_Pixmap_Rotation, 1);
   return o;
}

EINTERN void
e_mod_comp_pixmap_rotation_free(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK(o);
   E_FREE(o);
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_begin(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, 0);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_end(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, 0);
   if (o->pixmap) o->pixmap = 0;
   if (o->dmg)
     {
        ecore_x_damage_subtract(o->dmg, 0, 0);
        ecore_x_damage_free(o->dmg);
        o->dmg = 0;
     }
   o->obj = NULL;
   o->shobj = NULL;
   o->pw = 0;
   o->ph = 0;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_request(E_Comp_Pixmap_Rotation *o,
                                   Ecore_X_Event_Client_Message *ev,
                                   Evas *evas,
                                   Evas_Object *shobj,
                                   Evas_Object *obj,
                                   Ecore_X_Visual win_vis,
                                   int win_w, int win_h)
{
   Ecore_X_Pixmap pixmap;
   Ecore_X_Damage dmg;
   Evas_Native_Surface ns;
   unsigned int angle;
   int pw, ph, res;

   E_CHECK_RETURN(o, 0);
   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(evas, 0);
   E_CHECK_RETURN(shobj, 0);
   E_CHECK_RETURN(obj, 0);
   E_CHECK_RETURN(win_w <= 0, 0);
   E_CHECK_RETURN(win_h <= 0, 0);

   angle = (unsigned int)ev->data.l[0];
   if ((angle % 90) || ((angle / 90) >= 4))
     return EINA_FALSE;

   pixmap = (Ecore_X_Pixmap)ev->data.l[1];
   E_CHECK_RETURN(pixmap, 0);

   ecore_x_pixmap_geometry_get(pixmap, NULL, NULL, &pw, &ph);
   if ((angle == o->angle) && (pixmap == o->pixmap))
     {
        return EINA_TRUE;
     }

   dmg = ecore_x_damage_new
           (pixmap,
           ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);

   res = edje_object_part_swallow
           (shobj, "e.swallow.content", obj);
   E_CHECK_RETURN(res, 0);

   if ((angle == 90) || (angle == 270))
     evas_object_image_size_set(obj, win_h, win_w);
   else if ((angle == 0) || (angle == 180))
     evas_object_image_size_set(obj, win_w, win_h);

   evas_object_image_native_surface_set(obj, NULL);

   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_X11;
   ns.data.x11.visual = win_vis;
   ns.data.x11.pixmap = pixmap;
   evas_object_image_native_surface_set(obj, &ns);

   if (o->pixmap)
     {
        o->pixmap = 0;
        o->pw = 0;
        o->ph = 0;
     }

   if (o->dmg)
     {
        ecore_x_damage_subtract(o->dmg, 0, 0);
        ecore_x_damage_free(o->dmg);
        o->dmg = 0;
     }

   o->angle  = angle;
   o->shobj  = shobj;
   o->obj    = obj;
   o->pixmap = pixmap;
   o->dmg    = dmg;
   o->pw     = pw;
   o->ph     = ph;
   o->angle_changed = EINA_TRUE;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_damage(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, 0);
   E_CHECK_RETURN(o->dmg, 0);
   ecore_x_damage_subtract(o->dmg, 0, 0);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_update(E_Comp_Pixmap_Rotation *o,
                                  E_Update *up,
                                  int win_x, int win_y,
                                  int win_w, int win_h,
                                  int bd_w)
{
   E_Update_Rect *r;

   E_CHECK_RETURN(o, 0);
   E_CHECK_RETURN(up, 0);
   E_CHECK_RETURN(win_w <= 0, 0);
   E_CHECK_RETURN(win_h <= 0, 0);

   evas_object_move(o->shobj, win_x, win_y);
   evas_object_resize
     (o->shobj,
     win_w + (bd_w * 2),
     win_h + (bd_w * 2));

   r = e_mod_comp_update_rects_get(up);
   E_CHECK_RETURN(r, 0);

   e_mod_comp_update_clear(up);
   evas_object_image_data_update_add
     (o->obj, win_x, win_y, win_w, win_h);
   free(r);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_resize(E_Comp_Pixmap_Rotation *o,
                                  Evas *evas,
                                  Evas_Object *shobj,
                                  Evas_Object *obj,
                                  Ecore_X_Visual win_vis,
                                  int win_w, int win_h)
{
   Ecore_X_Pixmap pixmap;
   Ecore_X_Damage dmg;
   Evas_Native_Surface ns;
   unsigned int angle;
   int pw, ph, res;

   E_CHECK_RETURN(o, 0);
   E_CHECK_RETURN(evas, 0);
   E_CHECK_RETURN(shobj, 0);
   E_CHECK_RETURN(obj, 0);
   E_CHECK_RETURN(win_w <= 0, 0);
   E_CHECK_RETURN(win_h <= 0, 0);

   angle = o->angle;
   pixmap = o->pixmap;
   ecore_x_pixmap_geometry_get
     (pixmap, NULL, NULL, &pw, &ph);
   dmg = ecore_x_damage_new
           (pixmap,
           ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
   res = edje_object_part_swallow
           (shobj, "e.swallow.content", obj);
   E_CHECK_RETURN(res, 0);

   evas_object_image_native_surface_set(obj, NULL);

   if ((angle == 90) || (angle == 270))
     evas_object_image_size_set(obj, win_h, win_w);
   else if ((angle == 0) || (angle == 180))
     evas_object_image_size_set(obj, win_w, win_h);

   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_X11;
   ns.data.x11.visual = win_vis;
   ns.data.x11.pixmap = pixmap;
   evas_object_image_native_surface_set(obj, &ns);
   evas_object_image_data_update_add(obj, 0, 0, win_w, win_h);

   if (o->dmg)
     {
        ecore_x_damage_subtract(o->dmg, 0, 0);
        ecore_x_damage_free(o->dmg);
        o->dmg = 0;
     }

   o->angle  = angle;
   o->shobj  = shobj;
   o->obj    = obj;
   o->pixmap = pixmap;
   o->dmg    = dmg;
   o->pw     = pw;
   o->ph     = ph;
   o->angle_changed = EINA_TRUE;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_state_set(E_Comp_Pixmap_Rotation *o,
                                     Eina_Bool state)
{
   E_CHECK_RETURN(o, 0);
   o->run = state;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_state_get(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, 0);
   return o->run;
}

EINTERN Ecore_X_Damage
e_mod_comp_pixmap_rotation_damage_get(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, 0);
   return o->dmg;
}

EINTERN Evas_Object *
e_mod_comp_pixmap_rotation_shobj_get(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, NULL);
   return o->shobj;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_angle_check(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK_RETURN(o, 0);
   return o->angle_changed;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_done_send(Ecore_X_Window win,
                                     Ecore_X_Atom type)
{
   return ecore_x_client_message32_send
            (win, type, ECORE_X_EVENT_MASK_NONE,
            win, 0, 0, 0, 0);
}

EINTERN void
e_mod_comp_pixmap_rotation_effect_show(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK(o);
   switch (o->angle)
     {
      case   0: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,on", "e.rot.0"  ); break;
      case  90: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,on", "e.rot.90" ); break;
      case 180: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,on", "e.rot.180"); break;
      case 270: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,on", "e.rot.270"); break;
      default:
        break;
     }
   o->angle_changed = EINA_FALSE;
}

EINTERN void
e_mod_comp_pixmap_rotation_effect_end(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK(o);
   switch (o->angle)
     {
      case   0: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,off", "e.rot.0"  ); break;
      case  90: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,off", "e.rot.90" ); break;
      case 180: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,off", "e.rot.180"); break;
      case 270: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,visible,off", "e.rot.270"); break;
      default:
        break;
     }
}

EINTERN void
e_mod_comp_pixmap_rotation_effect_request(E_Comp_Pixmap_Rotation *o)
{
   E_CHECK(o);
   switch (o->angle)
     {
      case   0: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,rotation,on", "e.rot.0"  ); break;
      case  90: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,rotation,on", "e.rot.90" ); break;
      case 180: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,rotation,on", "e.rot.180"); break;
      case 270: e_mod_comp_effect_signal_add(NULL, o->shobj, "e,state,rotation,on", "e.rot.270"); break;
      default:
        break;
     }
   o->angle_changed = EINA_FALSE;
}
