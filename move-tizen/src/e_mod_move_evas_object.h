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
   Evas_Object   *clipper; // clipping evas object
   Evas_Object   *edje; // edje evas object
   E_Move_Event  *event;  // evas object's event handler object
   Eina_Bool      layer_pack; // normal obj is packed in move layer, but  some obj is not packed in move layer.
};

EINTERN E_Move_Evas_Object *e_mod_move_evas_obj_add(E_Move *m, E_Move_Canvas *canvas, E_Border *bd, Eina_Bool layer_pack);
EINTERN E_Move_Evas_Object *e_mod_move_evas_plugin_obj_add(E_Move *m, E_Move_Canvas *canvas, const char *svcname, Eina_Bool layer_pack);
// bd NULL case : create rectangle object
// bd indicates e_border case: create bd's mirror object from compositor

EINTERN void                e_mod_move_evas_obj_del(E_Move_Evas_Object *meo);

EINTERN Eina_List          *e_mod_move_evas_objs_add(E_Move *m, E_Border *bd, Eina_Bool layer_pack);
EINTERN Eina_List          *e_mod_move_evas_plugin_objs_add(E_Move *m, const char *svcname, Eina_Bool layer_pack);
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
EINTERN void                e_mod_move_evas_objs_stack_below(Eina_List *objs, Eina_List *objs2);
EINTERN void                e_mod_move_evas_objs_layer_set(Eina_List *objs, short l);
EINTERN void                e_mod_move_evas_objs_color_set(Eina_List *objs, int r, int g, int b, int a);
EINTERN void                e_mod_move_evas_objs_rotate_set(Eina_List *objs, double degree, Evas_Coord cx, Evas_Coord cy);

EINTERN void                e_mod_move_evas_objs_del_cb_set(Eina_List **objs_ptr);
EINTERN void                e_mod_move_evas_objs_clipper_add(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_clipper_del(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_clipper_show(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_clipper_hide(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_clipper_move(Eina_List *objs, int x, int y);
EINTERN void                e_mod_move_evas_objs_clipper_resize(Eina_List *objs, int w, int h);

EINTERN void                e_mod_move_evas_objs_edje_add(Eina_List *objs, const char *file_name, const char *group_name, const char *swallow_part_name); // include objs swallow process
EINTERN void                e_mod_move_evas_objs_edje_del(Eina_List *objs); // include objs unswallow process
EINTERN void                e_mod_move_evas_objs_edje_show(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_edje_hide(Eina_List *objs);
EINTERN void                e_mod_move_evas_objs_edje_move(Eina_List *objs, int x, int y);
EINTERN void                e_mod_move_evas_objs_edje_resize(Eina_List *objs, int w, int h);
EINTERN void                e_mod_move_evas_objs_edje_signal_emit(Eina_List *objs, const char *emission, const char *source);

#endif
#endif
