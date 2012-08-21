#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */
static void _e_mod_move_cb_comp_object_del(void *data, Evas *e, Evas_Object *obj, void *event_info __UNUSED__);
static void _e_mod_move_cb_comp_mirror_object_del(void *data, Evas *e, Evas_Object *obj, void *event_info __UNUSED__);

/* externally accessible functions */
EINTERN E_Move_Object *
e_mod_move_obj_add(E_Move_Border *mb,
                   E_Move_Canvas *canvas,
                   Eina_Bool      mirror)
{
   // get Evas_Object from Compositor
   E_Move *m;
   E_Move_Object *mo;
   E_Manager_Comp_Source *comp_src = NULL;

   E_CHECK_RETURN(mb, 0);
   m = mb->m;
   E_CHECK_RETURN(m, 0);

   mo = E_NEW(E_Move_Object, 1);
   E_CHECK_RETURN(mo, 0);

   if (m->man->comp)
     {
        comp_src = e_manager_comp_src_get(m->man, mb->bd->win);
        E_CHECK_GOTO(comp_src, error_cleanup);

        if (mirror)
          {
             mo->obj = e_manager_comp_src_image_mirror_add(m->man, comp_src);
             E_CHECK_GOTO(mo->obj, error_cleanup);

             evas_object_event_callback_add(mo->obj, EVAS_CALLBACK_DEL,
                                            _e_mod_move_cb_comp_mirror_object_del,
                                            mb);
             mo->mirror = mirror;
             evas_object_data_set(mo->obj,"move_mirror_obj", mo->obj);
             e_mod_move_util_border_hidden_set(mb, EINA_TRUE);
          }
        else
          {
             mo->obj = e_manager_comp_src_shadow_get(m->man, comp_src);
             E_CHECK_GOTO(mo->obj, error_cleanup);
             evas_object_data_set(mo->obj,"comp_shadow_obj", mo->obj);

             evas_object_event_callback_add(mo->obj, EVAS_CALLBACK_DEL,
                                            _e_mod_move_cb_comp_object_del, mb);
          }
        mo->canvas = canvas;
        mo->zone = canvas->zone;
        return mo;
     }

error_cleanup:
   memset(mo, 0, sizeof(E_Move_Object));
   E_FREE(mo);
   return NULL;
}

EINTERN void
e_mod_move_obj_del(E_Move_Object *mo)
{
   E_CHECK(mo);

   if (mo->obj)
     {
        if (mo->mirror)
          {
             evas_object_event_callback_del(mo->obj, EVAS_CALLBACK_DEL,
                                            _e_mod_move_cb_comp_mirror_object_del);
             evas_object_del(mo->obj);
          }
        else
          {
             evas_object_event_callback_del(mo->obj, EVAS_CALLBACK_DEL,
                                            _e_mod_move_cb_comp_object_del);
          }

     }
   memset(mo, 0, sizeof(E_Move_Object));
   E_FREE(mo);
}

EINTERN Eina_List *
e_mod_move_bd_move_objs_add(E_Move_Border *mb,
                            Eina_Bool      mirror)
{
   Eina_List *l, *objs = NULL;
   E_Move_Canvas *canvas;
   E_Move_Object *mo;

   E_CHECK_RETURN(mb, 0);
   E_CHECK_RETURN(mb->m, 0);
   E_CHECK_RETURN(mb->m->canvases, 0);

   EINA_LIST_FOREACH(mb->m->canvases, l, canvas)
     {
        mo = e_mod_move_obj_add(mb, canvas, mirror); //Third Value - EINA_TRUE: Get Mirror Object
                                                     //Third Value - EINA_FALSE: Get Shadow Object
        if (!mo)
          {
             e_mod_move_bd_move_objs_del(mb, objs);
             return NULL;
          }
        objs = eina_list_append(objs, mo);
     }
   return objs;
}

EINTERN void
e_mod_move_bd_move_objs_del(E_Move_Border *mb,
                            Eina_List     *objs)
{
   E_Move_Object *mo;
   EINA_LIST_FREE(objs, mo)
     {
        if (mo->mirror) e_mod_move_util_border_hidden_set(mb, EINA_FALSE);
        e_mod_move_obj_del(mo);
     }
}

EINTERN void
e_mod_move_bd_move_objs_move(E_Move_Border *mb,
                             int            x,
                             int            y)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        int zx = 0, zy = 0;
        if (!mo) continue;
        if (!mo->obj) continue;
        if (mo->zone)
          {
             zx = mo->zone->x;
             zy = mo->zone->y;
          }
        evas_object_move(mo->obj, x - zx, y - zy);
     }
}

EINTERN void
e_mod_move_bd_move_objs_resize(E_Move_Border *mb,
                               int            w,
                               int            h)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        if (!mo->obj) continue;
        evas_object_resize(mo->obj, w, h);
     }
}

EINTERN void
e_mod_move_bd_move_objs_data_del(E_Move_Border *mb,
                                 const char    *key)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        evas_object_data_del(mo->obj, key);
     }
}

EINTERN void
e_mod_move_bd_move_objs_data_set(E_Move_Border *mb,
                                 const char    *key,
                                 const void    *data)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        if (!mo->obj) continue;
        evas_object_data_set(mo->obj, key, data);
     }
}

EINTERN void
e_mod_move_bd_move_objs_show(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        if (!mo->obj) continue;
        evas_object_show(mo->obj);
     }
}

EINTERN void
e_mod_move_bd_move_objs_hide(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        if (!mo->obj) continue;
        evas_object_hide(mo->obj);
     }
}

EINTERN void
e_mod_move_bd_move_objs_raise(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        evas_object_raise(mo->obj);
     }
}

EINTERN void
e_mod_move_bd_move_objs_lower(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Object *mo;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (!mo) continue;
        evas_object_lower(mo->obj);
     }
}

EINTERN void
e_mod_comp_bd_move_objs_stack_above(E_Move_Border *mb,
                                    E_Move_Border *mb2)
{
   Eina_List *l, *ll;
   E_Move_Object *mo, *mo2;
   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        EINA_LIST_FOREACH(mb2->objs, ll, mo2)
          {
             if (mo->zone == mo2->zone)
               evas_object_stack_above(mo->obj, mo2->obj);
          }
     }
}

/* local subsystem functions */
static void
_e_mod_move_cb_comp_object_del(void            *data,
                               Evas            *e,
                               Evas_Object     *obj,
                               void *event_info __UNUSED__)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Object *mo;
   Eina_List *l;
   E_CHECK(mb);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "COMP_OBJ_DEL",
     mb->bd->win, mb->bd->client.win);

   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (mo->obj == obj)
          {
             // remove Move_Object pointer from list;
             mb->objs = eina_list_remove(mb->objs, mo);
             // free data
             memset(mo, 0, sizeof(E_Move_Object));
             E_FREE(mo);
             break;
          }
     }

   if (!mb->objs) e_mod_move_border_del(mb);
}

static void
_e_mod_move_cb_comp_mirror_object_del(void            *data,
                                      Evas            *e,
                                      Evas_Object     *obj,
                                      void *event_info __UNUSED__)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Object *mo;
   Eina_List *l;
   E_CHECK(mb);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "COMP_MIRROR_OBJ_DEL",
     mb->bd->win, mb->bd->client.win);

   EINA_LIST_FOREACH(mb->objs, l, mo)
     {
        if (mo->obj == obj)
          {
             // remove Move_Object pointer from list;
             mb->objs = eina_list_remove(mb->objs, mo);
             // free data
             memset(mo, 0, sizeof(E_Move_Object));
             E_FREE(mo);
             break;
          }
     }

   e_mod_move_util_border_hidden_set(mb, EINA_FALSE);
}
