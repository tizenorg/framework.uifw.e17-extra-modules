#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_FLICK_H
#define E_MOD_MOVE_FLICK_H

typedef struct _E_Move_Flick_Data   E_Move_Flick_Data;

EINTERN E_Move_Flick_Data *e_mod_move_flick_data_new(E_Move_Border *mb);
EINTERN Eina_Bool          e_mod_move_flick_data_init(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool          e_mod_move_flick_data_update(E_Move_Border *mb, int x, int y);
EINTERN Eina_Bool          e_mod_move_flick_state_get(E_Move_Border *mb, Eina_Bool direction_check);
EINTERN Eina_Bool          e_mod_move_flick_state_get2(E_Move_Border *mb);
EINTERN void               e_mod_move_flick_data_free(E_Move_Border *mb);
EINTERN Eina_Bool          e_mod_move_flick_data_get(E_Move_Border *mb, int *sx, int *sy, int *ex, int *ey, double *st, double *et);

#endif
#endif
