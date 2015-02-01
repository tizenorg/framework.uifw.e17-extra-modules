#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */

/* externally accessible functions */
EINTERN E_Move_Widget_Object *
e_mod_move_widget_obj_add(E_Move        *m,
                          E_Move_Canvas *canvas)
{
   Evas *evas = NULL;
   E_Move_Widget_Object *mwo;

   E_CHECK_RETURN(m, NULL);
   E_CHECK_RETURN(canvas, NULL);

   evas = canvas->evas;
   E_CHECK_RETURN(evas, NULL);

   mwo = E_NEW(E_Move_Widget_Object, 1);
   E_CHECK_RETURN(mwo, NULL);

   mwo->obj = evas_object_rectangle_add(evas);
   mwo->canvas = canvas;
   mwo->zone = canvas->zone;
   evas_object_data_set(mwo->obj, "move_widget_obj", mwo->obj);

   return mwo;
}

EINTERN void
e_mod_move_widget_obj_del(E_Move_Widget_Object *mwo)
{
   E_CHECK(mwo);

   if (mwo->event)
     {
        e_mod_move_event_free(mwo->event);
        mwo->event = NULL;
     }

   if (mwo->obj)
     evas_object_del(mwo->obj);
   memset(mwo, 0, sizeof(E_Move_Widget_Object));
   E_FREE(mwo);
}

EINTERN Eina_List *
e_mod_move_widget_objs_add(E_Move *m)
{
   Eina_List *l, *objs = NULL;
   E_Move_Canvas *canvas;
   E_Move_Widget_Object *mwo;

   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(m->canvases, 0);

   EINA_LIST_FOREACH(m->canvases, l, canvas)
     {
        mwo = e_mod_move_widget_obj_add(m, canvas);
        if (!mwo)
          {
             e_mod_move_widget_objs_del(objs);
             return NULL;
          }
        objs = eina_list_append(objs, mwo);
     }
   return objs;
}

EINTERN void
e_mod_move_widget_objs_del(Eina_List *objs)
{
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FREE(objs, mwo)
     {
        e_mod_move_widget_obj_del(mwo);
     }
}

EINTERN void
e_mod_move_widget_objs_show(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_show(mwo->obj);
     }
}

EINTERN void
e_mod_move_widget_objs_hide(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_hide(mwo->obj);
     }
}

EINTERN void
e_mod_move_widget_objs_move(Eina_List *objs,
                            int        x,
                            int        y)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        int zx = 0, zy = 0;
        if (!mwo) continue;
        if (!mwo->obj) continue;
        if (mwo->zone)
          {
             zx = mwo->zone->x;
             zy = mwo->zone->y;
          }
        evas_object_move(mwo->obj, x - zx, y - zy);
        mwo->geometry.x = x;
        mwo->geometry.y = y;
     }
}

EINTERN void
e_mod_move_widget_objs_resize(Eina_List *objs,
                              int        w,
                              int        h)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_resize(mwo->obj, w, h);
        mwo->geometry.w = w;
        mwo->geometry.h = h;
     }
}

EINTERN void
e_mod_move_widget_objs_data_del(Eina_List  *objs,
                                const char *key)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   E_CHECK(key);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_data_del(mwo->obj, key);
     }
}

EINTERN void
e_mod_move_widget_objs_data_set(Eina_List  *objs,
                                const char *key,
                                const void *data)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   E_CHECK(key);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_data_set(mwo->obj, key, data);
     }
}

EINTERN void
e_mod_move_widget_objs_raise(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_raise(mwo->obj);
     }
}

EINTERN void
e_mod_move_widget_objs_lower(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_lower(mwo->obj);
     }
}

EINTERN void
e_mod_move_widget_objs_stack_above(Eina_List *objs,
                                   Eina_List *objs2)
{
   Eina_List *l, *ll;
   E_Move_Widget_Object *mwo, *mwo2;
   E_CHECK(objs);
   E_CHECK(objs2);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        EINA_LIST_FOREACH(objs2, ll, mwo2)
          {
             if (mwo->zone == mwo2->zone)
               evas_object_stack_above(mwo->obj, mwo2->obj);
          }
     }
}

EINTERN void
e_mod_move_widget_objs_layer_set(Eina_List *objs,
                                 short      layer)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_layer_set(mwo->obj, layer);
     }
}

EINTERN void
e_mod_move_widget_objs_color_set(Eina_List *objs,
                                 int        r,
                                 int        g,
                                 int        b,
                                 int        a)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);

   if (r < 0) r = 0;
   if (r > 255) r = 255;
   if (g < 0) g = 0;
   if (g > 255) g = 255;
   if (b < 0) b = 0;
   if (b > 255) b = 255;
   if (a < 0) a = 0;
   if (a > 255) a = 255;

   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_color_set(mwo->obj, r, g, b, a);
     }
}

EINTERN void
e_mod_move_widget_objs_geometry_get(Eina_List *objs,
                                    int       *x,
                                    int       *y,
                                    int       *w,
                                    int       *h)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;

   E_CHECK(objs);
   E_CHECK(x);
   E_CHECK(y);
   E_CHECK(w);
   E_CHECK(h);

   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        *x = mwo->geometry.x;
        *y = mwo->geometry.y;
        *w = mwo->geometry.w;
        *h = mwo->geometry.h;
     }
}

EINTERN void
e_mod_move_widget_objs_repeat_events_set(Eina_List *objs,
                                         Eina_Bool  repeat)
{
   Eina_List *l;
   E_Move_Widget_Object *mwo;
   E_CHECK(objs);
   EINA_LIST_FOREACH(objs, l, mwo)
     {
        if (!mwo) continue;
        if (!mwo->obj) continue;
        evas_object_repeat_events_set(mwo->obj, repeat);
     }
}
