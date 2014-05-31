#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_H
#define E_MOD_COMP_EFFECT_H

typedef enum   _E_Comp_Effect_Style  E_Comp_Effect_Style;
typedef enum   _E_Comp_Effect_Kind   E_Comp_Effect_Kind;
typedef struct _E_Comp_Effect_Type   E_Comp_Effect_Type;
typedef struct _E_Comp_Effect_Job    E_Comp_Effect_Job;
typedef enum   _E_Comp_Effect_Ani_Style E_Comp_Effect_Ani_Style;

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

// transitions between windows, work best with particular types of shadow edc
enum _E_Comp_Effect_Ani_Style
{
   E_COMP_EFFECT_ANIMATION_DEFAULT = 0,
   E_COMP_EFFECT_ANIMATION_NONE,
   E_COMP_EFFECT_ANIMATION_SLIDEUP,
   E_COMP_EFFECT_ANIMATION_SLIDEDOWN,
   E_COMP_EFFECT_ANIMATION_SLIDELEFT,
   E_COMP_EFFECT_ANIMATION_SLIDERIGHT,
   E_COMP_EFFECT_ANIMATION_FADE,
   E_COMP_EFFECT_ANIMATION_GROW,
   E_COMP_EFFECT_ANIMATION_SHRINK,
   E_COMP_EFFECT_ANIMATION_CUSTOM1,
   E_COMP_EFFECT_ANIMATION_CUSTOM2,
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

struct _E_Comp_Effect_Object
{
   Evas_Object   *edje;
   Evas_Object   *img;
   Ecore_X_Window win;
   Ecore_X_Window cwin;
   Ecore_X_Pixmap pixmap;
   Eina_Bool      ev_vis; // send E_EVENT_COMP_SOURCE_VISIBILITY event when effect is done
   Eina_Bool      show;   // indicates that object is used to show app launching effect
};

/* window effect type functions */
EINTERN E_Comp_Effect_Type *e_mod_comp_effect_type_new(void);
EINTERN void                e_mod_comp_effect_type_free(E_Comp_Effect_Type *type);
EINTERN Eina_Bool           e_mod_comp_effect_type_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
EINTERN Eina_Bool           e_mod_comp_effect_state_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
EINTERN Eina_Bool           e_mod_comp_effect_state_get(E_Comp_Effect_Type *type);
EINTERN void                e_mod_comp_effect_state_set(E_Comp_Effect_Type *type, Eina_Bool state);
EINTERN Eina_Bool           e_mod_comp_effect_style_setup(E_Comp_Effect_Type *type, Ecore_X_Window win);
EINTERN E_Comp_Effect_Style e_mod_comp_effect_style_get(E_Comp_Effect_Type *type, E_Comp_Effect_Kind kind);

/* window effect functions */
EINTERN void                e_mod_comp_effect_win_show(E_Comp_Win *cw);
EINTERN void                e_mod_comp_effect_win_hide(E_Comp_Win *cw);
EINTERN void                e_mod_comp_effect_win_restack(E_Comp_Win *cw, Eina_Bool v1, Eina_Bool v2);

EINTERN Eina_Bool           e_mod_comp_effect_signal_add(E_Comp_Win *cw, Evas_Object *o, const char *emission, const char *src);
EINTERN Eina_Bool           e_mod_comp_effect_signal_del(E_Comp_Win *cw, Evas_Object *obj, const char *name);
EINTERN Eina_Bool           e_mod_comp_effect_jobs_clean(E_Comp_Win *cw, Evas_Object *obj, const char *name);
EINTERN Eina_Bool           e_mod_comp_effect_signal_flush(void);
EINTERN Eina_Bool           e_mod_comp_effect_animating_set(E_Comp *c, E_Comp_Win *cw, Eina_Bool set);

/* effect object functions */
EINTERN E_Comp_Effect_Object *e_mod_comp_effect_object_new(E_Comp_Layer *ly, E_Comp_Win *cw, Eina_Bool recreate);
EINTERN void                  e_mod_comp_effect_object_free(E_Comp_Effect_Object *o);

EINTERN void                  e_mod_comp_effect_object_win_set(E_Comp_Win *cw, const char *emission);
EINTERN void                  e_mod_comp_effect_above_wins_set(E_Comp_Win *cw, Eina_Bool show);

#endif
#endif
