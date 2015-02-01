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

struct _E_Comp_Effect_Type
{
   Eina_Bool           animatable : 1; // if this valuse is true then window can show animaton.
   E_Comp_Effect_Style show;           // indicate show effect type
   E_Comp_Effect_Style hide;           // indicate hide effect type
   E_Comp_Effect_Style restack;        // indicate restack effect type
   E_Comp_Effect_Style rotation;       // indicate rotation effect type
   E_Comp_Effect_Style focusin;        // indicate focus in effect type
   E_Comp_Effect_Style focusout;       // indicate focus out effect type
};

struct _E_Comp_Effect_Job
{
   Evas_Object   *o;
   E_Comp_Canvas *canvas;
   Ecore_X_Window win;
   Ecore_X_Window cwin;
   char           emission[1024];
   char           src[1024];
   Eina_Bool      emitted;
   Eina_Bool      effect_obj;
};
#endif
#endif
