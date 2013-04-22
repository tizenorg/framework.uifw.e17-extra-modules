#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_MINI_APPTRAY_H
#define E_MOD_MOVE_MINI_APPTRAY_H

struct _E_Move_Mini_Apptray_Data
{
   int              x;  // current move x position
   int              y;  // current move y position
   Eina_List       *dim_objs; // dim evas_obj
   int              opacity; // current dim opacity

   struct {
        struct {
          Eina_Bool layer_set;
        } indicator;
        struct {
          Eina_Bool      layer_set;
          Ecore_X_Window win; // multi window's lowest window id. mirror object is made by this window id. for mini_apptay mirror object stack set.
        } state_above; //if multi window exist, then state_above value set true.
   } animation_layer_info;
};

typedef struct _E_Move_Mini_Apptray_Data E_Move_Mini_Apptray_Data;
typedef struct _E_Move_Mini_Apptray_Animation_Data E_Move_Mini_Apptray_Animation_Data;

EINTERN void e_mod_move_mini_apptray_ctl_obj_event_setup(E_Move_Border *mb, E_Move_Control_Object *mco);
EINTERN E_Move_Border *e_mod_move_mini_apptray_find(void);
EINTERN Eina_Bool e_mod_move_mini_apptray_click_get(void);
EINTERN Eina_Bool e_mod_move_mini_apptray_event_clear(void);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_raise(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_state_get(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_stop(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_clear(E_Move_Border *mb);
EINTERN void* e_mod_move_mini_apptray_internal_data_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_internal_data_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_e_border_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_mini_apptray_e_border_raise(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_e_border_lower(E_Move_Border *mb);
EINTERN Eina_List* e_mod_move_mini_apptray_dim_show(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_dim_hide(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_start_position_set(E_Move_Border *mb, int angle);
EINTERN E_Move_Event_Cb e_mod_move_mini_apptray_event_cb_get(E_Move_Event_Type type);
EINTERN Eina_Bool e_mod_move_mini_apptray_anim_state_send(E_Move_Border *mb, Eina_Bool state);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_layer_set(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_mini_apptray_objs_animation_layer_unset(E_Move_Border *mb);

#endif
#endif
