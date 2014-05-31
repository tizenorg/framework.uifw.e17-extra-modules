#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_INDICATOR_H
#define E_MOD_MOVE_INDICATOR_H

typedef struct _E_Move_Indicator_Data E_Move_Indicator_Data;

EINTERN void e_mod_move_indicator_ctl_obj_event_setup(E_Move_Border *mb, E_Move_Control_Object *mco);
EINTERN E_Move_Border *e_mod_move_indicator_find(void);
EINTERN Eina_Bool e_mod_move_indicator_click_get(void);
EINTERN Eina_Bool e_mod_move_indicator_event_clear(void);
EINTERN void *e_mod_move_indicator_internal_data_add(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_indicator_internal_data_del(E_Move_Border *mb);
EINTERN Eina_Bool e_mod_move_indicator_scrollable_check(void);
EINTERN E_Move_Event_Cb e_mod_move_indicator_event_cb_get(E_Move_Event_Type type);
EINTERN Eina_Bool e_mod_move_indicator_e_border_move(E_Move_Border *mb, int x, int y);

#endif
#endif
