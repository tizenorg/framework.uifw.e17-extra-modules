#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_TM_H
#define E_MOD_COMP_EFFECT_TM_H

/* effect functions for taskmanager */
EINTERN Eina_Bool e_mod_comp_effect_tm_bg_show(E_Comp_Win *cw);
EINTERN Eina_Bool e_mod_comp_effect_tm_bg_hide(E_Comp_Win *cw);
EINTERN Eina_Bool e_mod_comp_effect_tm_state_update(E_Comp *c);
EINTERN Eina_Bool e_mod_comp_effect_tm_raise_above(E_Comp_Win *cw, E_Comp_Win *cw2);
EINTERN Eina_Bool e_mod_comp_effect_tm_handler_show_done(E_Comp_Win *cw);
EINTERN Eina_Bool e_mod_comp_effect_tm_handler_hide_done(E_Comp_Win *cw);
EINTERN Eina_Bool e_mod_comp_effect_tm_handler_raise_above_pre(E_Comp_Win *cw, E_Comp_Win *cw2);

#endif
#endif
