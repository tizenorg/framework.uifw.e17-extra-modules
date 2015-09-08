#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_WIN_ROTATION_H
#define E_MOD_COMP_EFFECT_WIN_ROTATION_H

#include <Elementary.h>

typedef struct _E_Comp_Zone_Rotation_Effect_Begin E_Comp_Zone_Rotation_Effect_Begin;
typedef struct _E_Comp_Zone_Rotation_Effect_End   E_Comp_Zone_Rotation_Effect_End;

struct _E_Comp_Zone_Rotation_Effect_Begin
{
   Evas_Object   *o;
   E_Zone        *zone;
   double         src;
   double         target;
   Ecore_X_Image *xim;
   Evas_Object   *img;
   Evas_Object   *xv_img;
   Ecore_X_Pixmap xv_pix;
   Eina_Bool      init;
   E_Comp_Layer  *ly;
};

struct _E_Comp_Zone_Rotation_Effect_End
{
   Evas_Object   *o;
   E_Zone        *zone;
   double         src;
   double         target;
   Evas_Object   *xv_img;
   Ecore_X_Pixmap xv_pix;
   Eina_Bool      init;
   Eina_Bool      send_msg;
};

struct _E_Comp_Effect_Zone_Rotation
{
   E_Comp_Canvas      *canvas;
   Eina_Bool           ready;
   Eina_Bool           run;
   Eina_Bool           xv_use;
   Elm_Transit        *trans_begin;
   Elm_Transit        *trans_end;
   Elm_Transit_Effect *effect_begin;
   Elm_Transit_Effect *effect_end;
};
#endif
#endif
