#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */

/* externally accessible functions */
EINTERN E_Move_Evas_Object *
e_mod_move_evas_obj_add(E_Move        *m,
                        E_Move_Canvas *canvas)
{
   Evas *evas = NULL;
   E_Move_Evas_Object *meo;

   E_CHECK_RETURN(m, NULL);
   E_CHECK_RETURN(canvas, NULL);

   evas = canvas->evas;
   E_CHECK_RETURN(evas, NULL);

   meo = E_NEW(E_Move_Evas_Object, 1);
   E_CHECK_RETURN(meo, NULL);

   meo->obj = evas_object_rectangle_add(evas);
   meo->canvas = canvas;
   meo->zone = canvas->zone;
   evas_object_data_set(meo->obj, "move_evas_obj", meo->obj);

   return meo;
}

EINTERN void
e_mod_move_evas_obj_del(E_Move_Evas_Object *meo)
{
   E_CHECK(meo);

   if (meo->event)
     {
        e_mod_move_event_free(meo->event);
        meo->event = NULL;
     }

   if (meo->obj)
     evas_object_del(meo->obj);
   memset(meo, 0, sizeof(E_Move_Evas_Object));
   E_FREE(meo);
}

EINTERN Eina_List *
e_mod_move_evas_objs_add(E_Move *m)
{
   Eina_List *l, *objs = NULL;
   E_Move_Canvas *canvas;
   E_Move_Evas_Object *meo;

   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(m->canvases, 0);

   EINA_LIST_FOREACH(m->canvases, l, canvas)
     {
        meo = e_mod_move_evas_obj_add(m, canvas);
        if (!meo)
          {
             e_mod_move_evas_objs_del(objs);
             return NULL;
          }
        objs = eina_list_append(objs, meo);
     }
   return objs;
}

EINTERN void
e_mod_move_evas_objs_del(Eina_List *objs)
{
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FREE(objs, meo)
     {
        e_mod_move_evas_obj_del(meo);
     }
}

EINTERN void
e_mod_move_evas_objs_show(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_show(meo->obj);
     }
}

EINTERN void
e_mod_move_evas_objs_hide(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_hide(meo->obj);
     }
}

EINTERN void
e_mod_move_evas_objs_move(Eina_List *objs,
                          int        x,
                          int        y)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        int zx = 0, zy = 0;
        if (!meo) continue;
        if (!meo->obj) continue;
        if (meo->zone)
          {
             zx = meo->zone->x;
             zy = meo->zone->y;
          }
        evas_object_move(meo->obj, x - zx, y - zy);
     }
}

EINTERN void
e_mod_move_evas_objs_resize(Eina_List *objs,
                            int        w,
                            int        h)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_resize(meo->obj, w, h);
     }
}

EINTERN void
e_mod_move_evas_objs_data_del(Eina_List  *objs,
                              const char *key)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   E_CHECK(key);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_data_del(meo->obj, key);
     }
}

EINTERN void
e_mod_move_evas_objs_data_set(Eina_List  *objs,
                              const char *key,
                              const void *data)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   E_CHECK(key);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_data_set(meo->obj, key, data);
     }
}

EINTERN void
e_mod_move_evas_objs_raise(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_raise(meo->obj);
     }
}

EINTERN void
e_mod_move_evas_objs_lower(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_lower(meo->obj);
     }
}

EINTERN void
e_mod_move_evas_objs_stack_above(Eina_List *objs,
                                 Eina_List *objs2)
{
   Eina_List *l, *ll;
   E_Move_Evas_Object *meo, *meo2;
   E_CHECK(objs);
   E_CHECK(objs2);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        EINA_LIST_FOREACH(objs2, ll, meo2)
          {
             if (meo->zone == meo2->zone)
               evas_object_stack_above(meo->obj, meo2->obj);
          }
     }
}

EINTERN void
e_mod_move_evas_objs_layer_set(Eina_List *objs,
                               short      layer)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_layer_set(meo->obj, layer);
     }
}

EINTERN void
e_mod_move_evas_objs_color_set(Eina_List *objs,
                               int        r,
                               int        g,
                               int        b,
                               int        a)
{
   Eina_List *l;
   E_Move_Evas_Object *meo;
   E_CHECK(objs);

   if (r < 0) r = 0;
   if (r > 255) r = 255;
   if (g < 0) g = 0;
   if (g > 255) g = 255;
   if (b < 0) b = 0;
   if (b > 255) b = 255;
   if (a < 0) a = 0;
   if (a > 255) a = 255;

   EINA_LIST_FOREACH(objs, l, meo)
     {
        if (!meo) continue;
        if (!meo->obj) continue;
        evas_object_color_set(meo->obj, r, g, b, a);
     }
}

