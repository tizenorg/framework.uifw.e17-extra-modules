#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_CANVAS_H
#define E_MOD_COMP_CANVAS_H

typedef struct _E_Comp_Canvas E_Comp_Canvas;

struct _E_Comp_Canvas
{
   E_Comp           *comp;
   E_Zone           *zone; // NULL if we have a single big canvas for all screens
   Ecore_Evas       *ee;
   Ecore_X_Window    ee_win;
   Evas             *evas;
   int               x, y, w, h; // geometry
   int               num;
   struct {
     Evas_Object    *bg;
     Evas_Object    *fg;
     double          frametimes[122];
     int             frameskip;
   } fps;
   struct {
     Eina_Bool       run;
     E_Comp_Win     *cw;
     Eina_Bool       force_composite;
   } nocomp;
   struct {
     Eina_Bool       run;
     int             num;
   } animation;
   Evas_Object      *bg_img;
   Eina_Bool         use_bg_img : 1;
   E_Comp_HW_Ov_Win *ov;
};

EINTERN E_Comp_Canvas *e_mod_comp_canvas_add(E_Comp *c, E_Zone *zone);
EINTERN void           e_mod_comp_canvas_del(E_Comp_Canvas *canvas);

#endif
#endif
