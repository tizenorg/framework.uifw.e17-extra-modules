#ifdef E_TYPEDEFS
#else
#ifndef EFFECT_WIN_ROTATION_H
#define EFFECT_WIN_ROTATION_H

/* window rotation effect and handler functions */
Eina_Bool                    _effect_mod_win_angle_get(E_Comp_Win *cw);

E_Comp_Effect_Zone_Rotation *_effect_mod_zone_rotation_new(E_Comp_Canvas *canvas);
void                         _effect_mod_zone_rotation_free(E_Comp_Effect_Zone_Rotation *zr);
Eina_Bool                    _effect_mod_zone_rotation_begin(E_Comp_Effect_Zone_Rotation *zr);
Eina_Bool                    _effect_mod_zone_rotation_end(E_Comp_Effect_Zone_Rotation *zr);
Eina_Bool                    _effect_mod_zone_rotation_cancel(E_Comp_Effect_Zone_Rotation *zr);
Eina_Bool                    _effect_mod_zone_rotation_do(E_Comp_Effect_Zone_Rotation *zr);
Eina_Bool                    _effect_mod_zone_rotation_clear(E_Comp_Effect_Zone_Rotation *zr);
#endif
#endif
