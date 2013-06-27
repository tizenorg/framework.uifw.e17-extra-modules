#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */
static void _e_mod_move_cb_control_object_del(void *data, Evas *e, Evas_Object *obj, void *event_info __UNUSED__);

/* externally accessible functions */
EINTERN E_Move_Control_Object *
e_mod_move_ctl_obj_add(E_Move_Border *mb,
                       E_Move_Canvas *canvas)
{
   E_Move *m;
   E_Move_Control_Object *mco;

   E_CHECK_RETURN(mb, 0);
   m = mb->m;
   E_CHECK_RETURN(m, 0);

   mco = E_NEW(E_Move_Control_Object, 1);
   E_CHECK_RETURN(mco, 0);

   mco->obj = evas_object_rectangle_add(canvas->evas);
   E_CHECK_GOTO(mco->obj, error_cleanup);
   evas_object_color_set(mco->obj, 0,0,0,0); // set color to fully transparency
   evas_object_layer_set(mco->obj, EVAS_LAYER_MAX-2); // set layer to top. this object always on top layer.
   evas_object_event_callback_add(mco->obj, EVAS_CALLBACK_DEL,
                                  _e_mod_move_cb_control_object_del, mb);
   evas_object_data_set(mco->obj,"move_ctl_obj", mco->obj);
   mco->canvas = canvas;
   mco->zone = canvas->zone;
   return mco;

error_cleanup:
   memset(mco, 0, sizeof(E_Move_Control_Object));
   E_FREE(mco);
   return NULL;
}

EINTERN void
e_mod_move_ctl_obj_del(E_Move_Control_Object *mco)
{
   E_CHECK(mco);

   if (mco->event)
     {
        e_mod_move_event_free(mco->event);
        mco->event = NULL;
     }
   if (mco->obj)
     {
        evas_object_event_callback_del(mco->obj, EVAS_CALLBACK_DEL,
                                       _e_mod_move_cb_control_object_del);
        evas_object_del(mco->obj);
     }
   memset(mco, 0, sizeof(E_Move_Control_Object));
   E_FREE(mco);
}

EINTERN Eina_List *
e_mod_move_bd_move_ctl_objs_add(E_Move_Border *mb)
{
   Eina_List *l, *ctl_objs = NULL;
   E_Move_Canvas *canvas;
   E_Move_Control_Object *mco;

   E_CHECK_RETURN(mb, 0);
   E_CHECK_RETURN(mb->m, 0);
   E_CHECK_RETURN(mb->m->canvases, 0);

   EINA_LIST_FOREACH(mb->m->canvases, l, canvas)
     {
        mco = e_mod_move_ctl_obj_add(mb, canvas);
        if (!mco)
          {
             e_mod_move_bd_move_ctl_objs_del(mb, ctl_objs);
             return NULL;
          }
        ctl_objs = eina_list_append(ctl_objs, mco);
     }
   return ctl_objs;
}

EINTERN void
e_mod_move_bd_move_ctl_objs_del(E_Move_Border *mb,
                                Eina_List     *ctl_objs)
{
   E_Move_Control_Object *mco;
   E_CHECK(ctl_objs);
   EINA_LIST_FREE(ctl_objs, mco)
     {
        e_mod_move_ctl_obj_del(mco);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_move(E_Move_Border *mb,
                                 int            x,
                                 int            y)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        int zx = 0, zy = 0;
        if (!mco) continue;
        if (!mco->obj) continue;
        if (mco->zone)
          {
             zx = mco->zone->x;
             zy = mco->zone->y;
          }
        evas_object_move(mco->obj, x - zx, y - zy);
        mco->geometry.x = x;
        mco->geometry.y = y;
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_resize(E_Move_Border *mb,
                                   int            w,
                                   int            h)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        if (!mco->obj) continue;
        evas_object_resize(mco->obj, w, h);
        mco->geometry.w = w;
        mco->geometry.h = h;
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_data_del(E_Move_Border *mb,
                                     const char    *key)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        evas_object_data_del(mco->obj, key);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_data_set(E_Move_Border *mb,
                                     const char    *key,
                                     const void    *data)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        if (!mco->obj) continue;
        evas_object_data_set(mco->obj, key, data);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_show(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        if (!mco->obj) continue;
        evas_object_show(mco->obj);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_hide(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        if (!mco->obj) continue;
        evas_object_hide(mco->obj);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_raise(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        evas_object_raise(mco->obj);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_lower(E_Move_Border *mb)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        evas_object_lower(mco->obj);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_stack_above(E_Move_Border *mb,
                                        E_Move_Border *mb2)
{
   Eina_List *l, *ll;
   E_Move_Control_Object *mco, *mco2;
   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        EINA_LIST_FOREACH(mb2->ctl_objs, ll, mco2)
          {
             if (mco->zone == mco2->zone)
               evas_object_stack_above(mco->obj, mco2->obj);
          }
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_color_set(E_Move_Border *mb,
                                      int            r,
                                      int            g,
                                      int            b,
                                      int            a)
{
   Eina_List *l;
   E_Move_Control_Object *mco;
   if ( r > 255 ) r = 255;
   if ( r < 0 ) r = 0;
   if ( g > 255 ) g = 255;
   if ( g < 0 ) g = 0;
   if ( b > 255 ) b = 255;
   if ( b < 0 ) b = 0;
   if ( a > 255 ) a = 255;
   if ( a < 0 ) a = 0;

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        if (!mco->obj) continue;
        evas_object_color_set(mco->obj, r, g, b, a);
     }
}

EINTERN void
e_mod_move_bd_move_ctl_objs_geometry_get(E_Move_Border *mb,
                                         int           *x,
                                         int           *y,
                                         int           *w,
                                         int           *h)
{
   Eina_List *l;
   E_Move_Control_Object *mco;

   E_CHECK(mb);
   E_CHECK(x);
   E_CHECK(y);
   E_CHECK(w);
   E_CHECK(h);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (!mco) continue;
        *x = mco->geometry.x;
        *y = mco->geometry.y;
        *w = mco->geometry.w;
        *h = mco->geometry.h;
     }
}

/* local subsystem functions */
static void
_e_mod_move_cb_control_object_del(void            *data,
                                  Evas            *e,
                                  Evas_Object     *obj,
                                  void *event_info __UNUSED__)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_Move_Control_Object *mco;
   Eina_List *l;
   E_CHECK(mb);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "CTL_OBJ_DEL",
     mb->bd->win, mb->bd->client.win);

   EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
     {
        if (mco->obj == obj)
          {
             // remove Move_Object pointer from list;
             mb->ctl_objs = eina_list_remove(mb->ctl_objs, mco);
             // free data
             memset(mco, 0, sizeof(E_Move_Control_Object));
             E_FREE(mco);
             break;
          }
     }
}
