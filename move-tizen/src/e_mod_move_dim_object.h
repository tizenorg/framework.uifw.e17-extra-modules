#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_DIM_OBJECT_H
#define E_MOD_MOVE_DIM_OBJECT_H

typedef struct _E_Move_Dim_Object E_Move_Dim_Object;

struct _E_Move_Dim_Object
{
   E_Move_Canvas *canvas;
   E_Zone        *zone;

   Evas_Object   *obj;    // evas(dim) object
   Evas_Object   *comp_obj; // comp's window object, it is used for window stack change
};

EINTERN E_Move_Dim_Object *e_mod_move_dim_obj_add(E_Move_Border *mb, E_Move_Canvas *canvas);
EINTERN void               e_mod_move_dim_obj_del(E_Move_Dim_Object *mdo);

EINTERN Eina_List         *e_mod_move_bd_move_dim_objs_add(E_Move_Border *mb);
EINTERN void               e_mod_move_bd_move_dim_objs_del(Eina_List *objs);
EINTERN void               e_mod_move_bd_move_dim_objs_show(Eina_List *objs);
EINTERN void               e_mod_move_bd_move_dim_objs_hide(Eina_List *objs);
EINTERN void               e_mod_move_bd_move_dim_objs_opacity_set(Eina_List *objs, int opacity);

#endif
#endif
