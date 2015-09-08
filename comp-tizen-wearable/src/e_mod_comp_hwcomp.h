#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_HWCOMP_H
#define E_MOD_COMP_HWCOMP_H

typedef enum _E_HWComp_Mode
{
   E_HWCOMP_USE_INVALID = -1,
   E_HWCOMP_USE_NOCOMP_MODE,
   E_HWCOMP_USE_HYBRIDCOMP_MODE,
   E_HWCOMP_USE_FULLCOMP_MODE
} E_HWComp_Mode;

typedef struct _E_Comp_HWComp_Update E_Comp_HWComp_Update;
typedef struct _E_Comp_HWComp_Drawable E_Comp_HWComp_Drawable;

struct _E_Comp_HWComp;

struct _E_Comp_HWComp_Drawable
{
   E_Comp_Win      *cw;
   Ecore_X_Drawable d;
   Eina_Bool        set_drawable;   /* if set_drawable is true, this drawable is set */
   int              set_countdown;  /* if set_countdown is 0, this drawable is going to hwc set */
   int              comp_countdown; /* if comp_countdown is 0, this drawable is going to composite */
   Eina_Bool        resized;
   Eina_Bool        region_update;
   int              first_update;

   int              update_count; /* for debugging */
};


struct _E_Comp_HWComp_Update
{
    E_HWComp_Mode            update_mode;       /* hw composite mode */
    unsigned int             num_overlays;      /* # of hw overlays */
    unsigned int             num_drawable;      /* # of the candidate drawables */
    E_Comp_HWComp_Drawable **hwc_drawable;      /* the candidate drawables */
    Eina_Bool                comp_update;       /* flag for ee_win update */

    Eina_Bool                ime_present;
    Eina_Bool                keymag_present;
    Eina_Bool                split_launcher_rect_present;
    Eina_Rectangle           ime_rect;
    Eina_Rectangle           keymag_rect;
    Eina_Rectangle           split_launcher_rect;

    struct _E_Comp_HWComp    *hwcomp;
};

struct _E_Comp_HWComp
{
   E_Comp             *c;
   E_Comp_Canvas      *canvas;

   int                   num_overlays; /* # of hw overlays */
   E_Comp_HWComp_Update *hwc_update;

   Ecore_Timer        *idle_timer;
   Eina_Bool           idle_status;
   Ecore_Idle_Enterer *idle_enterer;

   Eina_Bool           force_composite;
   int                 comp_ref;

   Eina_Bool           miniapp_present;
   int                 screen_width;
   int                 screen_height;

   int                 fullcomp_pending;
   Eina_Bool           force_swap;
};

EAPI E_Comp_HWComp *e_mod_comp_hwcomp_new(E_Comp_Canvas *canvas);
EAPI void           e_mod_comp_hwcomp_free(E_Comp_HWComp *hwcomp);
EAPI void           e_mod_comp_hwcomp_update_composite(E_Comp_HWComp *hwcomp);
EAPI void           e_mod_comp_hwcomp_set_full_composite(E_Comp_HWComp *hwcomp);
EAPI void           e_mod_comp_hwcomp_force_composite_set(E_Comp_HWComp *hwcomp, Eina_Bool set);
EAPI Eina_Bool      e_mod_comp_hwcomp_force_composite_get(E_Comp_HWComp *hwcomp);
EAPI Eina_Bool      e_mod_comp_hwcomp_cb_update(E_Comp_HWComp *hwcomp);
EAPI void           e_mod_comp_hwcomp_process_event(E_Comp *c, Ecore_X_Event_Generic *e);
EAPI void           e_mod_comp_hwcomp_check_win_update(E_Comp_Win *cw, int w, int h);
EAPI void           e_mod_comp_hwcomp_win_update(E_Comp_Win *cw);
EAPI void           e_mod_comp_hwcomp_set_resize(E_Comp_Win *cw);
EAPI void           e_mod_comp_hwcomp_reset_idle_timer(E_Comp_Canvas *canvas);
EAPI void           e_mod_comp_hwcomp_win_del(E_Comp_Win *cw);
EAPI E_HWComp_Mode  e_mod_comp_hwcomp_mode_get(E_Comp_HWComp *hwcomp);
EAPI void           e_mod_comp_hwcomp_update_null_set_drawables(E_Comp_HWComp *hwcomp);

#endif
#endif
