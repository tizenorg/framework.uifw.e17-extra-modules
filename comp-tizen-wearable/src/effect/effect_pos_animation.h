#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_EXTEND_H
#define E_MOD_COMP_EFFECT_EXTEND_H


/* effect extend functions */
E_Comp_Effect_Ani_Style _effect_mod_pos_launch_type_get(const char *source);
void _effect_mod_pos_launch_make_emission(E_Comp_Win *cw, char *emission, int size);
E_Comp_Effect_Ani_Style _effect_mod_pos_close_type_get(const char *source);
void _effect_mod_pos_close_make_emission(E_Comp_Win *cw, char *emission, int size);

#endif
#endif
