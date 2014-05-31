#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_WIN_ROTATION_H
#define E_MOD_COMP_EFFECT_WIN_ROTATION_H

/* window rotation effect and handler functions */
EINTERN Eina_Bool                    e_mod_comp_effect_win_angle_get(E_Comp_Win *cw);

EINTERN E_Comp_Effect_Zone_Rotation *e_mod_comp_effect_zone_rotation_new(E_Comp_Canvas *canvas);
EINTERN void                         e_mod_comp_effect_zone_rotation_free(E_Comp_Effect_Zone_Rotation *zr);
EINTERN Eina_Bool                    e_mod_comp_effect_zone_rotation_begin(E_Comp_Effect_Zone_Rotation *zr);
EINTERN Eina_Bool                    e_mod_comp_effect_zone_rotation_end(E_Comp_Effect_Zone_Rotation *zr);
EINTERN Eina_Bool                    e_mod_comp_effect_zone_rotation_cancel(E_Comp_Effect_Zone_Rotation *zr);
EINTERN Eina_Bool                    e_mod_comp_effect_zone_rotation_do(E_Comp_Effect_Zone_Rotation *zr);

#endif
#endif
