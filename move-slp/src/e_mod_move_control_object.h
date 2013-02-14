#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_CONTROL_OBJECT_H
#define E_MOD_MOVE_CONTROL_OBJECT_H

typedef struct _E_Move_Control_Object E_Move_Control_Object;

struct _E_Move_Control_Object
{
   E_Move_Canvas *canvas;
   E_Zone        *zone;

   Evas_Object   *obj;    // compositor's evas(window) object
   E_Move_Event  *event;  // evas object's event handler object
};

EINTERN E_Move_Control_Object *e_mod_move_ctl_obj_add(E_Move_Border *mb, E_Move_Canvas *canvas);
EINTERN void                   e_mod_move_ctl_obj_del(E_Move_Control_Object *mco);

EINTERN Eina_List             *e_mod_move_bd_move_ctl_objs_add(E_Move_Border *mb);
EINTERN void                   e_mod_move_bd_move_ctl_objs_del(E_Move_Border *mb, Eina_List *ctl_objs);
EINTERN void                   e_mod_move_bd_move_ctl_objs_move(E_Move_Border *mb, int x, int y);
EINTERN void                   e_mod_move_bd_move_ctl_objs_resize(E_Move_Border *mb, int w, int h);
EINTERN void                   e_mod_move_bd_move_ctl_objs_data_del(E_Move_Border *mb, const char *key);
EINTERN void                   e_mod_move_bd_move_ctl_objs_data_set(E_Move_Border *mb, const char *key, const void *data);
EINTERN void                   e_mod_move_bd_move_ctl_objs_show(E_Move_Border *mb);
EINTERN void                   e_mod_move_bd_move_ctl_objs_hide(E_Move_Border *mb);
EINTERN void                   e_mod_move_bd_move_ctl_objs_raise(E_Move_Border *mb);
EINTERN void                   e_mod_move_bd_move_ctl_objs_lower(E_Move_Border *mb);
EINTERN void                   e_mod_move_bd_move_ctl_objs_stack_above(E_Move_Border *mb, E_Move_Border *mb2);
EINTERN void                   e_mod_move_bd_move_ctl_objs_color_set(E_Move_Border *mb, int r, int g, int b, int a);

#endif
#endif
