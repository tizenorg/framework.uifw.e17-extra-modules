#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_EVAS_OBJECT_H
#define E_MOD_MOVE_EVAS_OBJECT_H

typedef struct _E_Move_Evas_Object E_Move_Evas_Object;

struct _E_Move_Evas_Object
{
   E_Move_Canvas *canvas;
   E_Zone        *zone;
   Evas_Object   *obj;
   E_Move_Event  *event;  // evas object's event handler object
};

EINTERN E_Move_Evas_Object *e_mod_move_evas_obj_add(E_Move *m, E_Move_Canvas *canvas);
EINTERN void                e_mod_move_evas_obj_del(E_Move_Evas_Object *meo);

EINTERN Eina_List          *e_mod_move_evas_objs_add(E_Move *m);
EINTERN void                e_mod_move_evas_objs_del(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_show(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_hide(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_move(Eina_List *objs, int x, int y);
EINTERN void                e_mod_move_evas_objs_resize(Eina_List *objs, int w, int h);
EINTERN void                e_mod_move_evas_objs_data_del(Eina_List *objs, const char *key);
EINTERN void                e_mod_move_evas_objs_data_set(Eina_List *objs, const char *key, const void *data);
EINTERN void                e_mod_move_evas_objs_raise(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_lower(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_stack_above(Eina_List *objs, Eina_List *objs2);
EINTERN void                e_mod_move_evas_objs_layer_set(Eina_List *objs, short l);
EINTERN void                e_mod_move_evas_objs_color_set(Eina_List *objs, int r, int g, int b, int a);

#endif
#endif
