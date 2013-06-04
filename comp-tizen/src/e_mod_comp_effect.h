#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_H
#define E_MOD_COMP_EFFECT_H

typedef enum   _E_Comp_Effect_Style E_Comp_Effect_Style;
typedef enum   _E_Comp_Effect_Kind  E_Comp_Effect_Kind;
typedef struct _E_Comp_Effect_Type  E_Comp_Effect_Type;
typedef struct _E_Comp_Effect_Job   E_Comp_Effect_Job;

enum _E_Comp_Effect_Style
{
   E_COMP_EFFECT_STYLE_DEFAULT = 0,
   E_COMP_EFFECT_STYLE_NONE,
   E_COMP_EFFECT_STYLE_CUSTOM0,
   E_COMP_EFFECT_STYLE_CUSTOM1,
   E_COMP_EFFECT_STYLE_CUSTOM2,
   E_COMP_EFFECT_STYLE_CUSTOM3,
   E_COMP_EFFECT_STYLE_CUSTOM4,
   E_COMP_EFFECT_STYLE_CUSTOM5,
   E_COMP_EFFECT_STYLE_CUSTOM6,
   E_COMP_EFFECT_STYLE_CUSTOM7,
   E_COMP_EFFECT_STYLE_CUSTOM8,
   E_COMP_EFFECT_STYLE_CUSTOM9
};

enum _E_Comp_Effect_Kind
{
   E_COMP_EFFECT_KIND_SHOW = 0,
   E_COMP_EFFECT_KIND_HIDE,
   E_COMP_EFFECT_KIND_RESTACK,
   E_COMP_EFFECT_KIND_ROTATION,
   E_COMP_EFFECT_KIND_FOCUSIN,
   E_COMP_EFFECT_KIND_FOCUSOUT
};

/* window effect type functions */
EINTERN E_Comp_Effect_Type *e_mod_comp_effect_type_new(void);
EINTERN void                e_mod_comp_effect_type_free(E_Comp_Effect_Type *type);
EINTERN Eina_Bool           e_mod_comp_effect_type_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
EINTERN Eina_Bool           e_mod_comp_effect_state_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
EINTERN Eina_Bool           e_mod_comp_effect_state_get(E_Comp_Effect_Type *type);
EINTERN Eina_Bool           e_mod_comp_effect_style_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
EINTERN E_Comp_Effect_Style e_mod_comp_effect_style_get(E_Comp_Effect_Type *type, E_Comp_Effect_Kind kind);

/* window effect functions */
EINTERN void                e_mod_comp_effect_win_show(E_Comp_Win *cw);
EINTERN Eina_Bool           e_mod_comp_effect_win_hide(E_Comp_Win *cw);
EINTERN void                e_mod_comp_effect_win_restack(E_Comp_Win *cw, E_Comp_Win *cw2);
EINTERN void                e_mod_comp_effect_win_lower(E_Comp_Win *cw, E_Comp_Win *cw2);
EINTERN void                e_mod_comp_effect_mirror_handler_hide(E_Comp_Win *cw, E_Comp_Win *cw2);
EINTERN void                e_mod_comp_effect_disable_stage(E_Comp *c, E_Comp_Win *cw);
EINTERN Eina_Bool           e_mod_comp_effect_signal_add(E_Comp_Win *cw, Evas_Object *o, const char *emission, const char *src);
EINTERN Eina_Bool           e_mod_comp_effect_signal_del(E_Comp_Win *cw, Evas_Object *obj, const char *name);
EINTERN Eina_Bool           e_mod_comp_effect_jobs_clean(E_Comp_Win *cw, Evas_Object *obj, const char *name);
EINTERN Eina_Bool           e_mod_comp_effect_signal_flush(void);
EINTERN Eina_Bool           e_mod_comp_effect_animating_set(E_Comp *c, E_Comp_Win *cw, Eina_Bool set);

#endif
#endif
