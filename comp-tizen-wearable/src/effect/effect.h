#ifdef E_TYPEDEFS
#else
#ifndef EFFECT_H
#define EFFECT_H

EAPI Eina_Bool     e_mod_comp_effect_mod_init(E_Comp *c);
EAPI void          e_mod_comp_effect_mod_shutdown(E_Comp *c);

/* window effect type functions */
E_Comp_Effect_Type *_effect_mod_type_new(void);
void                _effect_mod_type_free(E_Comp_Effect_Type *type);
Eina_Bool           _effect_mod_type_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
Eina_Bool           _effect_mod_state_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
Eina_Bool           _effect_mod_state_get(E_Comp_Effect_Type *type);
void                _effect_mod_state_set(E_Comp_Effect_Type *type, Eina_Bool state);
Eina_Bool           _effect_mod_style_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
E_Comp_Effect_Style _effect_mod_style_get(E_Comp_Effect_Type *type, E_Comp_Effect_Kind kind);

/* window effect functions */
void                _effect_mod_win_show(E_Comp_Win *cw);
void                _effect_mod_win_hide(E_Comp_Win *cw);
void                _effect_mod_win_restack(E_Comp_Win *cw, Eina_Bool v1, Eina_Bool v2);

Eina_Bool           _effect_mod_signal_add(E_Comp_Win *cw, Evas_Object *o, const char *emission, const char *src);
Eina_Bool           _effect_mod_signal_del(E_Comp_Win *cw, Evas_Object *obj, const char *name);
Eina_Bool           _effect_mod_jobs_clean(E_Comp_Win *cw, Evas_Object *obj, const char *name);
Eina_Bool           _effect_mod_signal_flush(void);
Eina_Bool           _effect_mod_animating_set(E_Comp *c, E_Comp_Win *cw, Eina_Bool set);

/* effect object functions */
E_Comp_Effect_Object *_effect_mod_object_new(E_Comp_Layer *ly, E_Comp_Win *cw, Eina_Bool recreate);
void                  _effect_mod_object_free(E_Comp_Effect_Object *o);

void                  _effect_mod_object_win_set(E_Comp_Win *cw, const char *emission);
void                  _effect_mod_above_wins_set(E_Comp_Win *cw, Eina_Bool show);

#endif
#endif
