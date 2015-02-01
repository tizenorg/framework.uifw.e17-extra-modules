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
   Eina_List       *handle_objs; // Clipping Scroller's Quickpanel Handle Move_Evas_Object.

   struct {
      Eina_List *list;
      Eina_Bool use;
   } stack_above_borders; // Quickpanel's stack above borders. it is used when quickpanel object created.
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
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_move_with_time(E_Move_Border *mb, int x, int y, double time);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_state_get(E_Move_Border *mb);
EINTERN E_Move_Animation_Direction e_mod_move_quickpanel_objs_animation_direction_get(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_stop(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_clear(E_Move_Border *mb);
EINTERN void* e_mod_move_quickpanel_internal_data_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_internal_data_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_e_border_move(E_Move_Border *mb, int x, int y);
EINTERN Eina_List* e_mod_move_quickpanel_dim_show(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_dim_hide(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_animation_start_position_set(E_Move_Border *mb, int angle, Eina_Bool on_screen); // on_screen is true: quickpanel exists inside screen. false: quickpanel exists outside screen.
EINTERN E_Move_Event_Cb e_mod_move_quickpanel_event_cb_get(E_Move_Event_Type type);
EINTERN Eina_Bool e_mod_move_quickpanel_visible_check(void);
EINTERN Eina_Bool e_mod_move_quickpanel_below_window_reset(void);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_clipper_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_clipper_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_objs_clipper_apply(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool e_mod_move_quickpanel_stage_init(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_stage_deinit(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_angle_change_post_job(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_window_input_region_change_post_job(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_quickpanel_anim_state_send(E_Move_Border *mb, Eina_Bool state);
EINTERN void e_mod_move_quickpanel_above_border_process(E_Move_Border *mb, Eina_Bool size, Eina_Bool pos, Eina_Bool stack, Eina_Bool visible);
#endif
#endif

