#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_CANVAS_H
#define E_MOD_COMP_CANVAS_H

typedef struct _E_Comp_Canvas E_Comp_Canvas;

typedef enum _E_Nocomp_Mode
{
   E_NOCOMP_MODE_NONE,
   E_NOCOMP_MODE_PREPARE,
   E_NOCOMP_MODE_BEGIN,
   E_NOCOMP_MODE_RUN,
   E_NOCOMP_MODE_END,
   E_NOCOMP_MODE_DISPOSE
} E_Nocomp_Mode;

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
     E_Nocomp_Mode   mode;
     E_Comp_Win     *cw;
     Eina_Bool       force_composite;
     int             comp_ref; // indicates the number of modules using composite mode
     struct {
       E_Comp_Win   *cw;
       Ecore_Timer  *timer;
     } prepare;
     struct {
       E_Comp_Win   *cw;
       Ecore_Timer  *timer;
       int           dmg_updates;
     } end;
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
EINTERN E_Comp_Win    *e_mod_comp_canvas_fullscreen_check(E_Comp_Canvas *canvas);
EINTERN void           e_mod_comp_canvas_nocomp_prepare(E_Comp_Canvas *canvas, E_Comp_Win *cw);
EINTERN Eina_Bool      e_mod_comp_canvas_nocomp_begin(E_Comp_Canvas *canvas);
EINTERN Eina_Bool      e_mod_comp_canvas_nocomp_end(E_Comp_Canvas *canvas);
EINTERN Eina_Bool      e_mod_comp_canvas_nocomp_dispose(E_Comp_Canvas *canvas);

#endif
#endif
