#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_QUICKPANEL_H
#define E_MOD_MOVE_QUICKPANEL_H

struct _E_Move_Quickpanel_Data
{
   int x;  // current move x position
   int y;  // current move y position
   Eina_List       *dim_objs; // dim evas_obj
   int              opacity; // current dim opacity
};

typedef struct _E_Move_Quickpanel_Data E_Move_Quickpanel_Data;
typedef struct _E_Move_Quickpanel_Animation_Data E_Move_Quickpanel_Animation_Data;

EINTERN void e_mod_move_quickpanel_ctl_obj_event_setup(E_Move_Border *mb, E_Move_Control_Object *mco);
EINTERN E_Move_Border *e_mod_move_quickpanel_base_find(void);
EINTERN E_Move_Border *e_mod_move_quickpanel_find(void);
EINTERN Eina_Bool e_mod_move_quickpanel_click_get(void);
EINTERN Eina_Bool e_mod_move_quickpanel_event_clear(void);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_state_get(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_stop(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_clear(E_Move_Border *mb);
EINTERN void* e_mod_move_quickpanel_internal_data_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_internal_data_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_e_border_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_List* e_mod_move_quickpanel_dim_show(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_dim_hide(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_start_position_set(E_Move_Border *mb, int angle);
EINTERN E_Move_Event_Cb e_mod_move_quickpanel_event_cb_get(E_Move_Event_Type type);

#endif
#endif

