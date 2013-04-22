#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_INDICATOR_CONTROLLER_H
#define E_MOD_MOVE_INDICATOR_CONTROLLER_H

struct _E_Move_Indicator_Controller
{
   Ecore_X_Window target_win; // target client window id
   Eina_List     *objs; // list of E_Move_Evas_Object.
};

typedef struct _E_Move_Indicator_Controller E_Move_Indicator_Controller;

EINTERN Eina_Bool e_mod_move_indicator_controller_set(E_Move_Border *target_mb);
EINTERN Eina_Bool e_mod_move_indicator_controller_unset(E_Move *m);
EINTERN Eina_Bool e_mod_move_indicator_controller_update(E_Move *m);
EINTERN Eina_Bool e_mod_move_indicator_controller_state_get(E_Move *m, Ecore_X_Window *win);
EINTERN Eina_Bool e_mod_move_indicator_controller_set_policy_check(E_Move_Border *target_mb);
EINTERN Eina_Bool e_mod_move_indicator_controller_unset_policy_check(E_Move_Border *target_mb);

#endif
#endif
