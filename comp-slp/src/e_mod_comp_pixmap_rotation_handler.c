#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_pixmap_rotation.h"
#include "e_mod_comp_pixmap_rotation_handler.h"

/* local subsystem functions */
static void _win_unredirect(E_Comp_Win *cw);

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_handler_update(E_Comp_Win *cw)
{
   Evas_Object *o;
   Eina_Bool r;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->pixrot, 0);

   if (cw->needpix)
     {
        cw->needpix = 0;
        cw->pw = cw->w;
        cw->ph = cw->h;
     }

   if (cw->resize_hide)
     cw->resize_hide = EINA_FALSE;

   r = e_mod_comp_pixmap_rotation_update
         (cw->pixrot, cw->up, cw->x, cw->y,
         cw->w, cw->h, cw->border);
   E_CHECK_RETURN(r, 0);
   E_CHECK_RETURN(cw->visible, 0);
   E_CHECK_RETURN(cw->dmg_updates > 0, 0);

   o = e_mod_comp_pixmap_rotation_shobj_get(cw->pixrot);
   E_CHECK_RETURN(o, 0);

   if (!evas_object_visible_get(o))
     {
        evas_object_show(cw->obj);
        evas_object_show(o);
        e_mod_comp_pixmap_rotation_effect_show(cw->pixrot);
     }
   else if (e_mod_comp_pixmap_rotation_angle_check(cw->pixrot))
     {
        e_mod_comp_pixmap_rotation_effect_request(cw->pixrot);
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_handler_release(E_Comp_Win *cw)
{
   Ecore_X_Damage dmg;
   Eina_Bool r;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->pixrot, 0);

   r = e_mod_comp_pixmap_rotation_state_get(cw->pixrot);
   E_CHECK_RETURN(r, 0);

   if (cw->animating)
     {
        cw->delete_me = 1;
        return EINA_TRUE;
     }

   dmg = e_mod_comp_pixmap_rotation_damage_get(cw->pixrot);
   if (dmg) e_mod_comp_win_del_damage(cw, dmg);

   e_mod_comp_pixmap_rotation_done_send
      (e_mod_comp_util_client_xid_get(cw),
      ATOM_CM_PIXMAP_ROTATION_END_DONE);

   e_mod_comp_pixmap_rotation_state_set(cw->pixrot, 0);
   e_mod_comp_pixmap_rotation_end(cw->pixrot);
   e_mod_comp_pixmap_rotation_free(cw->pixrot);
   cw->pixrot = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_handler_message(Ecore_X_Event_Client_Message *ev)
{
   Ecore_X_Atom type;
   Ecore_X_Window win;
   E_Comp_Win *cw = NULL;
   Ecore_X_Damage dmg;
   Eina_Bool r;

   cw = e_mod_comp_border_client_find(ev->win);
   if (!cw)
     {
        cw = e_mod_comp_win_find(ev->win);
        E_CHECK_RETURN(cw, 0);
     }

   type = ev->message_type;
   win = ev->win;

   if (type == ATOM_CM_PIXMAP_ROTATION_BEGIN)
     {
        E_CHECK_RETURN((!cw->pixrot), 0);

        cw->pixrot = e_mod_comp_pixmap_rotation_new();
        E_CHECK_RETURN(cw->pixrot, 0);

        e_mod_comp_pixmap_rotation_state_set(cw->pixrot, 1);

        edje_object_part_unswallow(cw->shobj, cw->obj);
        _win_unredirect(cw);

        e_mod_comp_pixmap_rotation_done_send
           (win, ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE);
     }
   else if (type == ATOM_CM_PIXMAP_ROTATION_END)
     {
        E_CHECK_RETURN(cw->pixrot, 0);
        e_mod_comp_effect_animating_set(cw->c, cw, EINA_FALSE);
        e_mod_comp_pixmap_rotation_handler_release(cw);
     }
   else if (type == ATOM_CM_PIXMAP_ROTATION_REQUEST)
     {
        E_CHECK_RETURN(cw->pixrot, 0);

        r = e_mod_comp_pixmap_rotation_state_get(cw->pixrot);
        E_CHECK_RETURN(r, 0);

        dmg = e_mod_comp_pixmap_rotation_damage_get(cw->pixrot);
        if (dmg) e_mod_comp_win_del_damage(cw, dmg);

        if (cw->obj)
          edje_object_part_unswallow(cw->shobj, cw->obj);
        else
          {
             E_CHECK_RETURN(cw->c, 0);
             cw->obj = evas_object_image_filled_add(cw->c->evas);
             evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);

             if (cw->argb)
               evas_object_image_alpha_set(cw->obj, 1);
             else
               evas_object_image_alpha_set(cw->obj, 0);

             evas_object_show(cw->obj);
             evas_object_pass_events_set(cw->obj, 1);
          }

        r = e_mod_comp_pixmap_rotation_request
              (cw->pixrot, ev, cw->c->evas, cw->shobj,
              cw->obj, cw->vis, cw->w, cw->h);
        E_CHECK_RETURN(r, 0);

        e_mod_comp_update_resize(cw->up, cw->w, cw->h);
        e_mod_comp_update_add(cw->up, 0, 0, cw->w, cw->h);

        cw->native = 1;
        dmg = e_mod_comp_pixmap_rotation_damage_get(cw->pixrot);
        E_CHECK_RETURN(dmg, 0);
        e_mod_comp_win_add_damage(cw, dmg);
        e_mod_comp_pixmap_rotation_done_send
          (win, ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE);
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_handler_configure(E_Comp_Win *cw,
                                             int w, int h)
{
   Ecore_X_Window win;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->pixrot, 0);
   E_CHECK_RETURN(((w == cw->w) && (h == cw->h)), 0);

   win = e_mod_comp_util_client_xid_get(cw);

   /* backup below obj */
   Eina_Bool bottom = EINA_FALSE;

   Evas_Object *below_obj = evas_object_below_get(cw->shobj);
   if (!below_obj)
     {
        if (evas_object_bottom_get(cw->c->evas) == cw->shobj)
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x bd:%s shobj is bottom.\n",
               "PIX_ROT", e_mod_comp_util_client_xid_get(cw),
               cw->bd ? "O" : "X");
             bottom = EINA_TRUE;
          }
     }

   if (cw->obj)
     {
        evas_object_hide(cw->obj);
        evas_object_del(cw->obj);
        cw->obj = NULL;
     }
   if (cw->shobj)
     {
        evas_object_hide(cw->obj);
        evas_object_del(cw->shobj);
        cw->shobj = NULL;
     }

   cw->shobj = edje_object_add(cw->c->evas);
   cw->obj = evas_object_image_filled_add(cw->c->evas);
   evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);

   if (cw->argb)
     evas_object_image_alpha_set(cw->obj, 1);
   else
     evas_object_image_alpha_set(cw->obj, 0);

   e_mod_comp_win_type_setup(cw);
   e_mod_comp_win_shadow_setup(cw);
   e_mod_comp_win_cb_setup(cw);

   evas_object_show(cw->obj);
   evas_object_pass_events_set(cw->obj, 1);
   evas_object_pass_events_set(cw->shobj, 1);

   /* restore stack */
   if (bottom)
     below_obj = evas_object_below_get(cw->shobj);

   evas_object_stack_above(cw->shobj, below_obj);
   L(LT_EVENT_X,
     "[COMP] %31s w:0x%08x bd:%s shobj restore stack.\n",
     "PIX_ROT", e_mod_comp_util_client_xid_get(cw),
     cw->bd ? "O" : "X");

   e_mod_comp_pixmap_rotation_done_send
     (win, ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_handler_damage(E_Comp_Win *cw,
                                          Eina_Bool dmg)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->pixrot, 0);
   E_CHECK_RETURN(dmg, 0);

   e_mod_comp_pixmap_rotation_damage(cw->pixrot);
   cw->dmg_updates++;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_pixmap_rotation_handler_hide(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->pixrot, 0);

   if (!cw->force)
     {
        cw->defer_hide = 1;
        e_mod_comp_pixmap_rotation_effect_end(cw->pixrot);
        cw->pending_count++;
     }
   else
     {
        cw->animating = 0;
        cw->defer_hide = 0;
        e_mod_comp_pixmap_rotation_handler_release(cw);
     }

   return EINA_TRUE;
}

/* local subsystem functions */
static void
_win_unredirect(E_Comp_Win *cw)
{
   E_CHECK(cw);
   E_CHECK(cw->visible);
   E_CHECK(!(cw->input_only));
   E_CHECK(!(cw->invalid));

   if (cw->obj)
     {
        evas_object_hide(cw->obj);
        evas_object_del(cw->obj);
        cw->obj = NULL;
     }
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (cw->native)
     {
        evas_object_image_native_surface_set(cw->obj, NULL);
        cw->native = 0;
     }
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   if (cw->xim)
     {
        evas_object_image_size_set(cw->obj, 1, 1);
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
     }
   if (cw->redirected)
     {
        ecore_x_composite_unredirect_window
          (cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
     }
   if (cw->damage)
     {
        e_mod_comp_win_del_damage(cw, cw->damage);
        ecore_x_damage_subtract(cw->damage, 0, 0);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }
}
