#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* local subsystem functions */
static void _e_mod_move_cb_comp_object_del(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* local subsystem functions */
static void _e_mod_move_cb_comp_object_del(void        *data,
                                           Evas        *e,
                                           Evas_Object *obj,
                                           void        *event_info)
{
   E_Move_Dim_Object *mdo = (E_Move_Dim_Object *)data;
   E_CHECK(mdo);

   E_CHECK(obj);

   mdo->comp_obj = NULL;
}

/* externally accessible functions */
EINTERN E_Move_Dim_Object *
e_mod_move_dim_obj_add(E_Move_Border *mb,
                       E_Move_Canvas *canvas)
{
   E_Move *m;
   E_Move_Dim_Object *mdo;
   Evas *evas = NULL;
   Evas_Object *comp_obj = NULL;
   E_Manager_Comp_Source *comp_src = NULL;
   Evas_Object *ly = NULL;

   E_CHECK_RETURN(mb, 0);
   m = mb->m;
   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(canvas, 0);

   ly = e_mod_move_util_comp_layer_get(m, "move");
   E_CHECK_RETURN(ly, 0);

   evas = canvas->evas;
   E_CHECK_RETURN(evas, 0);

   mdo = E_NEW(E_Move_Dim_Object, 1);
   E_CHECK_RETURN(mdo, 0);

   if (m->man->comp)
     {
        comp_src = e_manager_comp_src_get(m->man, mb->bd->win);
        E_CHECK_GOTO(comp_src, error_cleanup);
        comp_obj = e_manager_comp_src_shadow_get(m->man, comp_src);
        E_CHECK_GOTO(comp_obj, error_cleanup);

        mdo->obj = evas_object_rectangle_add(evas);
        evas_object_color_set(mdo->obj, 0, 0, 0, m->dim_min_opacity);
        evas_object_move(mdo->obj, canvas->x, canvas->y);
        evas_object_resize(mdo->obj, canvas->w, canvas->h);

        e_layout_pack(ly, mdo->obj);
        e_layout_child_lower(mdo->obj);

        mdo->comp_obj = comp_obj;
        mdo->canvas = canvas;
        mdo->zone = canvas->zone;

        evas_object_data_set(mdo->obj, "move_dim_obj", mdo->obj);

        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE)
          {
             SLOG(LOG_DEBUG, "E17_MOVE_MODULE",
                  "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",
                  __func__, __LINE__ );
          }

        evas_object_event_callback_add(comp_obj, EVAS_CALLBACK_DEL,
                                       _e_mod_move_cb_comp_object_del, mdo);
        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE)
          {
             SLOG(LOG_DEBUG, "E17_MOVE_MODULE",
                  "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",
                  __func__, __LINE__ );
          }

        return mdo;
     }

error_cleanup:
   memset(mdo, 0, sizeof(E_Move_Dim_Object));
   E_FREE(mdo);
   return NULL;
}

EINTERN void
e_mod_move_dim_obj_del(E_Move_Dim_Object *mdo)
{
   E_Move *m = e_mod_move_util_get();
   Evas_Object *ly = NULL;
   E_CHECK(m);
   E_CHECK(mdo);

   ly = e_mod_move_util_comp_layer_get(m, "move");
   E_CHECK(ly);

   if (mdo->comp_obj)
     {
        evas_object_event_callback_del(mdo->comp_obj,
                                       EVAS_CALLBACK_DEL,
                                       _e_mod_move_cb_comp_object_del);
     }

   if (mdo->obj)
     {
        e_layout_unpack(mdo->obj);
        evas_object_del(mdo->obj);
     }
   memset(mdo, 0, sizeof(E_Move_Dim_Object));
   E_FREE(mdo);
}

EINTERN Eina_List *
e_mod_move_bd_move_dim_objs_add(E_Move_Border *mb)
{
   Eina_List *l, *objs = NULL;
   E_Move_Canvas *canvas;
   E_Move_Dim_Object *mdo;

   E_CHECK_RETURN(mb, 0);
   E_CHECK_RETURN(mb->m, 0);
   E_CHECK_RETURN(mb->m->canvases, 0);

   EINA_LIST_FOREACH(mb->m->canvases, l, canvas)
     {
        mdo = e_mod_move_dim_obj_add(mb, canvas);
        if (!mdo)
          {
             e_mod_move_bd_move_dim_objs_del(objs);
             return NULL;
          }
        objs = eina_list_append(objs, mdo);
     }
   return objs;
}

EINTERN void
e_mod_move_bd_move_dim_objs_del(Eina_List *objs)
{
   E_Move_Dim_Object *mdo;
   EINA_LIST_FREE(objs, mdo)
     {
        e_mod_move_dim_obj_del(mdo);
     }
}

EINTERN void
e_mod_move_bd_move_dim_objs_opacity_set(Eina_List *objs,
                                        int        opacity)
{
   E_Move *m = NULL;
   Eina_List *l;
   E_Move_Dim_Object *mdo;
   int r, g, b, a;
   int max_opacity;
   int min_opacity;

   m = e_mod_move_util_get();
   E_CHECK(m);

   min_opacity = m->dim_min_opacity;
   max_opacity = m->dim_max_opacity;

   if (opacity < min_opacity) opacity = min_opacity;
   if (opacity > max_opacity) opacity = max_opacity;

   EINA_LIST_FOREACH(objs, l, mdo)
     {
        if (!mdo) continue;
        if (!mdo->obj) continue;
        evas_object_color_get(mdo->obj, &r, &g, &b, &a);
        evas_object_color_set(mdo->obj, r, g, b, opacity);
     }
}

EINTERN void
e_mod_move_bd_move_dim_objs_show(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Dim_Object *mdo;
   EINA_LIST_FOREACH(objs, l, mdo)
     {
        if (!mdo) continue;
        if (!mdo->obj) continue;
        evas_object_show(mdo->obj);
     }
}

EINTERN void
e_mod_move_bd_move_dim_objs_hide(Eina_List *objs)
{
   Eina_List *l;
   E_Move_Dim_Object *mdo;
   EINA_LIST_FOREACH(objs, l, mdo)
     {
        if (!mdo) continue;
        if (!mdo->obj) continue;
        evas_object_hide(mdo->obj);
     }
}
