#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_update.h"
#include "e_mod_comp_rotation.h"
#include "config.h"

#include <X11/X.h>
#include <X11/Xutil.h>

struct _E_Comp_Rotation
{
   unsigned int   angle;
   Evas_Object   *shobj;
   Evas_Object   *obj;
   Ecore_X_Pixmap pixmap;
   Ecore_X_Damage damage;
   int            pw, ph;
   Eina_Bool      force;
   Eina_Bool      defer_show;
   Eina_Bool      defer_hide;
   Eina_Bool      angle_changed;
};

E_Comp_Rotation *
e_mod_comp_rotation_new(void)
{
   E_Comp_Rotation *rot = NULL;
   rot = calloc(1, sizeof(E_Comp_Rotation));
   return rot;
}

void
e_mod_comp_rotation_free(E_Comp_Rotation *rot)
{
   if (rot) free(rot);
}

Eina_Bool
e_mod_comp_rotation_begin(E_Comp_Rotation *rot)
{
   if (!rot) return EINA_FALSE;
   return EINA_TRUE;
}

void
e_mod_comp_rotation_show_effect(E_Comp_Rotation *rot)
{
   if (!rot) return;
   switch (rot->angle)
     {
      case   0: edje_object_signal_emit(rot->shobj, "e,state,visible,on", "e.rot.0"  ); break;
      case  90: edje_object_signal_emit(rot->shobj, "e,state,visible,on", "e.rot.90" ); break;
      case 180: edje_object_signal_emit(rot->shobj, "e,state,visible,on", "e.rot.180"); break;
      case 270: edje_object_signal_emit(rot->shobj, "e,state,visible,on", "e.rot.270"); break;
      default:
        break;
     }

   rot->angle_changed = EINA_FALSE;
}

void
e_mod_comp_rotation_end_effect(E_Comp_Rotation *rot)
{
   if (!rot) return;
   switch (rot->angle)
     {
      case   0: edje_object_signal_emit(rot->shobj, "e,state,visible,off", "e.rot.0"  ); break;
      case  90: edje_object_signal_emit(rot->shobj, "e,state,visible,off", "e.rot.90" ); break;
      case 180: edje_object_signal_emit(rot->shobj, "e,state,visible,off", "e.rot.180"); break;
      case 270: edje_object_signal_emit(rot->shobj, "e,state,visible,off", "e.rot.270"); break;
      default:
        break;
     }
}

void
e_mod_comp_rotation_request_effect(E_Comp_Rotation *rot)
{
   if (!rot) return;
   switch (rot->angle)
     {
      case   0: edje_object_signal_emit(rot->shobj, "e,state,rotation,on", "e.rot.0"  ); break;
      case  90: edje_object_signal_emit(rot->shobj, "e,state,rotation,on", "e.rot.90" ); break;
      case 180: edje_object_signal_emit(rot->shobj, "e,state,rotation,on", "e.rot.180"); break;
      case 270: edje_object_signal_emit(rot->shobj, "e,state,rotation,on", "e.rot.270"); break;
      default:
        break;
     }

   rot->angle_changed = EINA_FALSE;
}

Eina_Bool
e_mod_comp_rotation_end(E_Comp_Rotation *rot)
{
   if (!rot) return EINA_FALSE;

   rot->obj = NULL;
   rot->shobj = NULL;

   if (rot->pixmap)
     {
        // Never free the proxy pixmap which is created and removed by the client.
        // ecore_x_pixmap_free(rot->pixmap);
        rot->pixmap = 0;
        rot->pw = 0;
        rot->ph = 0;
     }

   if (rot->damage)
     {
        ecore_x_damage_subtract(rot->damage, 0, 0);
        ecore_x_damage_free(rot->damage);
        rot->damage = 0;
     }

   return EINA_TRUE;
}


Eina_Bool
e_mod_comp_rotation_request(E_Comp_Rotation* rot,
                            Ecore_X_Event_Client_Message *ev,
                            Evas *evas,
                            Evas_Object *shobj,
                            Evas_Object *obj,
                            Ecore_X_Visual win_vis,
                            int win_x __UNUSED__, int win_y __UNUSED__,
                            int win_w, int win_h )
{
   unsigned int    angle;
   Ecore_X_Pixmap  pixmap;
   Ecore_X_Damage  damage;
   int             pw, ph;

   if (!rot || !ev || !evas || !shobj || !obj || win_w <= 0 || win_h <= 0)
     {
        fprintf(stderr, "[E17-comp] %s() failed. invalid args. " \
                "rot:%p ev:%p evas:%p shobj:%p obj:%p %dx%d\n",
                __func__, rot, ev, evas, shobj, obj, win_w, win_h);
        return EINA_FALSE;
     }

   // --------------------------------------------------------------
   // check angle and pixmap
   // --------------------------------------------------------------
   angle = (unsigned int)ev->data.l[0];
   if ((angle % 90) || ((angle / 90) >= 4))
     {
        fprintf(stderr, "[E17-comp] invalid angle:%d\n", angle);
        return EINA_FALSE;
     }

   // --------------------------------------------------------------
   // create proxy pixmap stuffs
   // --------------------------------------------------------------
   pixmap = (Ecore_X_Pixmap)ev->data.l[1];

   if (pixmap == None)
     {
        fprintf(stderr, "[E17-comp] %s() invalid pixmap:NULL\n", __func__ );
        return EINA_FALSE;
     }

   ecore_x_pixmap_geometry_get(pixmap, NULL, NULL, &pw, &ph);

   if ((angle == rot->angle) && (pixmap == rot->pixmap))
     {
        return EINA_TRUE;
     }

   damage = ecore_x_damage_new(pixmap, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);

   // --------------------------------------------------------------
   // swallow evas obj
   // --------------------------------------------------------------
   int res = edje_object_part_swallow(shobj, "e.swallow.content", obj);
   if (!res)
     {
        // TODO: release evas objs
        fprintf(stdout, "[E17-comp] pixmap:0x%08x swallow failed.\n", pixmap);
        return EINA_FALSE;
     }

   if ((angle == 90) || (angle == 270))
     evas_object_image_size_set(obj, win_h, win_w);
   else if ((angle == 0) || (angle == 180))
     evas_object_image_size_set(obj, win_w, win_h);

   evas_object_image_native_surface_set(obj, NULL);

   Evas_Native_Surface ns;
   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_X11;
   ns.data.x11.visual = win_vis;
   ns.data.x11.pixmap = pixmap;
   evas_object_image_native_surface_set(obj, &ns);


   // --------------------------------------------------------------
   // setup
   // --------------------------------------------------------------
   if (rot->pixmap)
     {
        // Never free the proxy pixmap which is created and removed by the client.
        // ecore_x_pixmap_free(rot->pixmap);
        rot->pixmap = 0;
        rot->pw = 0;
        rot->ph = 0;
     }

   if (rot->damage)
     {
        ecore_x_damage_subtract(rot->damage, 0, 0);
        ecore_x_damage_free(rot->damage);
        rot->damage = 0;
     }

   // TODO: if shobj already exists ??
   rot->angle  = angle;
   rot->shobj  = shobj;
   rot->obj    = obj;
   rot->pixmap = pixmap;
   rot->damage = damage;
   rot->pw     = pw;
   rot->ph     = ph;
   rot->angle_changed = EINA_TRUE;

   return EINA_TRUE;
}

Eina_Bool
e_mod_comp_rotation_damage(E_Comp_Rotation *rot)
{
   if (!rot || !rot->damage) return EINA_FALSE;
   ecore_x_damage_subtract(rot->damage, 0, 0);
   return EINA_TRUE;
}

Eina_Bool
e_mod_comp_rotation_resize(E_Comp_Rotation *rot,
                            Evas *evas,
                            Evas_Object *shobj,
                            Evas_Object *obj,
                            Ecore_X_Visual win_vis,
                            int win_x __UNUSED__, int win_y __UNUSED__,
                            int win_w, int win_h )
{
   unsigned int    angle;
   Ecore_X_Pixmap  pixmap;
   Ecore_X_Damage  damage;
   int             pw, ph;

   if (!rot || !evas || !shobj || !obj || win_w <= 0 || win_h <= 0)
     {
        fprintf(stderr, "[E17-comp] %s() failed. invalid args. " \
                "rot:%p evas:%p shobj:%p obj:%p %dx%d\n",
                __func__, rot, evas, shobj, obj, win_w, win_h);
        return EINA_FALSE;
     }

   angle = rot->angle;
   pixmap = rot->pixmap;
   ecore_x_pixmap_geometry_get(pixmap, NULL, NULL, &pw, &ph);
   damage = ecore_x_damage_new(pixmap, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);

   // --------------------------------------------------------------
   // swallow evas obj
   // --------------------------------------------------------------
   int res = edje_object_part_swallow(shobj, "e.swallow.content", obj);
   if (!res)
     {
        // TODO: release evas objs
        fprintf(stdout, "[E17-comp] pixmap:0x%08x swallow failed.\n", pixmap);
        return EINA_FALSE;
     }

   evas_object_image_native_surface_set(obj, NULL);

   if ((angle == 90) || (angle == 270)) evas_object_image_size_set(obj, win_h, win_w);
   else if ((angle == 0) || (angle == 180)) evas_object_image_size_set(obj, win_w, win_h);


   Evas_Native_Surface ns;
   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_X11;
   ns.data.x11.visual = win_vis;
   ns.data.x11.pixmap = pixmap;
   evas_object_image_native_surface_set(obj, &ns);
   evas_object_image_data_update_add(obj, 0, 0, win_w, win_h);


   // --------------------------------------------------------------
   // setup
   // --------------------------------------------------------------
   if (rot->damage)
     {
        ecore_x_damage_subtract(rot->damage, 0, 0);
        ecore_x_damage_free(rot->damage);
        rot->damage = 0;
     }

   rot->angle  = angle;
   rot->shobj  = shobj;
   rot->obj    = obj;
   rot->pixmap = pixmap;
   rot->damage = damage;
   rot->pw     = pw;
   rot->ph     = ph;
   rot->angle_changed = EINA_TRUE;

   return EINA_TRUE;
}

Eina_Bool
e_mod_comp_rotation_update(E_Comp_Rotation *rot,
                           E_Update *up,
                           int win_x, int win_y,
                           int win_w, int win_h,
                           int border_w )
{
   E_Update_Rect *r;

   if (!rot || !up || win_w <= 0 || win_h <= 0)
     {
        fprintf(stderr, "%s(%d) invalid args. rot:%p up:%p win_w:%d win_h:%d\n",
                __func__, __LINE__, rot, up, win_w, win_h);
        return EINA_FALSE;
     }

   evas_object_move(rot->shobj, win_x, win_y);
   evas_object_resize(rot->shobj,
                      win_w + (border_w * 2),
                      win_h + (border_w * 2));

   r = e_mod_comp_update_rects_get(up);
   if (r)
     {
        e_mod_comp_update_clear(up);
        evas_object_image_data_update_add(rot->obj, win_x, win_y, win_w, win_h);
        free(r);
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

Ecore_X_Damage
e_mod_comp_rotation_get_damage(E_Comp_Rotation *rot)
{
   if (!rot) return 0;
   return rot->damage;
}

Evas_Object *
e_mod_comp_rotation_get_shobj(E_Comp_Rotation *rot)
{
   if (!rot) return NULL;
   return rot->shobj;
}

unsigned int
e_mod_comp_rotation_get_angle(E_Comp_Rotation *rot)
{
   if (!rot) return 0;
   return rot->angle;
}

Eina_Bool
e_mod_comp_rotation_angle_is_changed(E_Comp_Rotation *rot)
{
   if (!rot) return EINA_FALSE;
   return rot->angle_changed;
}

Eina_Bool
e_mod_comp_rotation_done_send(Ecore_X_Window win,
                              Ecore_X_Atom type)
{
   static Ecore_X_Display *dpy = NULL;
   XEvent xev;

   if (!dpy) dpy = ecore_x_display_get();

   xev.xclient.type = ClientMessage;
   xev.xclient.display = dpy;
   xev.xclient.window = win;
   xev.xclient.message_type = type;
   xev.xclient.format = 32;
   xev.xclient.data.l[0] = win;
   xev.xclient.data.l[1] = 0;
   xev.xclient.data.l[2] = 0;
   xev.xclient.data.l[3] = 0;
   xev.xclient.data.l[4] = 0;

   XSendEvent(dpy, win, False,
              NoEventMask,
              &xev);
   return EINA_TRUE;
}
