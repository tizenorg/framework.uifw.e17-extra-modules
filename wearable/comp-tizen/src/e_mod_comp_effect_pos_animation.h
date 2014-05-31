#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_EXTEND_H
#define E_MOD_COMP_EFFECT_EXTEND_H


/* effect extend functions */
EINTERN E_Comp_Effect_Ani_Style e_mod_comp_effect_pos_launch_type_get(const char *source);
EINTERN void e_mod_comp_effect_pos_launch_make_emission(E_Comp_Win *cw, char *emission, int size);
EINTERN E_Comp_Effect_Ani_Style e_mod_comp_effect_pos_close_type_get(const char *source);
EINTERN void e_mod_comp_effect_pos_close_make_emission(E_Comp_Win *cw, char *emission, int size);

#endif
#endif
