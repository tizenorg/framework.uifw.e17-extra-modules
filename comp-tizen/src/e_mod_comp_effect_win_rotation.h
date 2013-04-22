#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_WIN_ROTATION_H
#define E_MOD_COMP_EFFECT_WIN_ROTATION_H

typedef struct _E_Comp_Effect_Win_Rotation E_Comp_Effect_Win_Rotation;

/* window rotation effect and handler functions */
EINTERN E_Comp_Effect_Win_Rotation *e_mod_comp_effect_win_rotation_new(void);
EINTERN void                        e_mod_comp_effect_win_rotation_free(E_Comp_Effect_Win_Rotation *r);
EINTERN Eina_Bool                   e_mod_comp_effect_win_roation_run_check(E_Comp_Effect_Win_Rotation *r);
EINTERN Eina_Bool                   e_mod_comp_effect_win_rotation_handler_prop(Ecore_X_Event_Window_Property *ev);
EINTERN Eina_Bool                   e_mod_comp_effect_win_rotation_handler_update(E_Comp_Win *cw);
EINTERN Eina_Bool                   e_mod_comp_effect_win_rotation_handler_release(E_Comp_Win *cw);
EINTERN Eina_Bool                   e_mod_comp_effect_win_angle_get(E_Comp_Win *cw);
EINTERN void                        e_mod_comp_explicit_win_rotation_done(E_Comp_Win *cw);

#endif
#endif
