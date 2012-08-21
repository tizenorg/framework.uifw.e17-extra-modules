#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_OBJECT_H
#define E_MOD_MOVE_OBJECT_H

typedef struct _E_Move_Object E_Move_Object;

struct _E_Move_Object
{
   E_Move_Canvas *canvas;
   E_Zone        *zone;

   Evas_Object   *obj;    // compositor's evas(window) object
   Eina_Bool      mirror; // TRUE: use compositor's mirror_obj / FALSE: use compositor's shadow_obj
};

EINTERN E_Move_Object *e_mod_move_obj_add(E_Move_Border *mb, E_Move_Canvas *canvas, Eina_Bool mirror);
EINTERN void           e_mod_move_obj_del(E_Move_Object *mo);

EINTERN Eina_List     *e_mod_move_bd_move_objs_add(E_Move_Border *mb, Eina_Bool mirror);
EINTERN void           e_mod_move_bd_move_objs_del(E_Move_Border *mb, Eina_List *objs);
EINTERN void           e_mod_move_bd_move_objs_move(E_Move_Border *mb, int x, int y);
EINTERN void           e_mod_move_bd_move_objs_resize(E_Move_Border *mb, int w, int h);
EINTERN void           e_mod_move_bd_move_objs_data_del(E_Move_Border *mb, const char *key);
EINTERN void           e_mod_move_bd_move_objs_data_set(E_Move_Border *mb, const char *key, const void *data);
EINTERN void           e_mod_move_bd_move_objs_show(E_Move_Border *mb);
EINTERN void           e_mod_move_bd_move_objs_hide(E_Move_Border *mb);
EINTERN void           e_mod_move_bd_move_objs_raise(E_Move_Border *mb);
EINTERN void           e_mod_move_bd_move_objs_lower(E_Move_Border *mb);
EINTERN void           e_mod_comp_bd_move_objs_stack_above(E_Move_Border *mb, E_Move_Border *mb2);

#endif
#endif
