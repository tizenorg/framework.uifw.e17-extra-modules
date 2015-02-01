#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_CANVAS_H
#define E_MOD_COMP_CANVAS_H

typedef struct _E_Comp_Canvas E_Comp_Canvas;
typedef struct _E_Comp_Layer  E_Comp_Layer;

//enum for stereoscopic view modes
typedef enum _E_Comp_Stereo_Mode
{
   E_COMP_STEREO_MONO = 0,
   E_COMP_STEREO_HORIZONTAL = 1,
   E_COMP_STEREO_VERTICAL = 2,
   E_COMP_STEREO_INTERLACED = 3,
} E_Comp_Stereo_Mode;

typedef enum _E_Nocomp_Mode
{
   E_NOCOMP_MODE_NONE,
   E_NOCOMP_MODE_PREPARE,
   E_NOCOMP_MODE_BEGIN,
   E_NOCOMP_MODE_RUN,
   E_NOCOMP_MODE_END,
   E_NOCOMP_MODE_DISPOSE
} E_Nocomp_Mode;

struct _E_Comp_Layer
{
   char             *name;
   Evas_Object      *layout;     // e_layout
   Evas_Object      *bg;         // background rectangle object for e_layout
   int               x, y, w, h; // geometry
   E_Comp_Canvas    *canvas;     // parent canvas
   unsigned int      count;      // indicates number of running effect objects, if 0 then layer will be hidden
   Eina_List        *objs;       // list of E_Comp_Effect_Object
   Eina_Bool         need_init;  // EINA_TRUE: need to initialize, EINA_FALSE: already initialized.
};

struct _E_Comp_Canvas
{
   E_Comp           *comp;
   E_Zone           *zone; // NULL if we have a single big canvas for all screens
   Ecore_Evas       *ee;
   Ecore_X_Window    ee_win;
   Evas             *evas;
   Eina_List        *layers; // list of E_Comp_Layer
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
   E_Comp_Effect_Zone_Rotation *zr;
   Eina_Bool         xv_ready[3]; // [0]: XV is running [1]: rotation_end_event was occcured [2]: XV_BYPASS_DONE1 msg was received

   E_Comp_HWComp    *hwcomp;
   E_Comp_Stereo_Mode          stereoscopic_mode; //stereoscopic view mode
};

EAPI E_Comp_Canvas *e_mod_comp_canvas_add(E_Comp *c, E_Zone *zone);
EAPI void           e_mod_comp_canvas_del(E_Comp_Canvas *canvas);
EAPI E_Comp_Win    *e_mod_comp_canvas_fullscreen_check(E_Comp_Canvas *canvas);
EAPI void           e_mod_comp_canvas_nocomp_prepare(E_Comp_Canvas *canvas, E_Comp_Win *cw);
EAPI Eina_Bool      e_mod_comp_canvas_nocomp_begin(E_Comp_Canvas *canvas);
EAPI Eina_Bool      e_mod_comp_canvas_nocomp_end(E_Comp_Canvas *canvas);
EAPI Eina_Bool      e_mod_comp_canvas_nocomp_dispose(E_Comp_Canvas *canvas);
EAPI E_Comp_Layer  *e_mod_comp_canvas_layer_get(E_Comp_Canvas *canvas, const char *name);
EAPI void           e_mod_comp_canvas_stereo_layout_set(E_Comp_Canvas *canvas);

EAPI void                  e_mod_comp_layer_populate(E_Comp_Layer *ly, Evas_Object *o);
EAPI void                  e_mod_comp_layer_populate_above_normal(E_Comp_Layer *ly, Evas_Object *o);
EAPI void                  e_mod_comp_layer_raise_above(E_Comp_Canvas *canvas, Evas_Object *o, E_Border *bd);
EAPI void                  e_mod_comp_layer_lower_below(E_Comp_Canvas *canvas, Evas_Object *o, E_Border *bd);
EAPI void                  e_mod_comp_layer_eval(E_Comp_Layer *ly);
EAPI void                  e_mod_comp_layer_bg_adjust(E_Comp_Layer *ly);
EAPI void                  e_mod_comp_layer_effect_set(E_Comp_Layer *ly, Eina_Bool set);
EAPI Eina_Bool             e_mod_comp_layer_effect_get(E_Comp_Layer *ly);
EAPI E_Comp_Effect_Object *e_mod_comp_layer_effect_obj_get(E_Comp_Layer *ly, Ecore_X_Window win);

#endif
#endif
