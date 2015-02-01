#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_OBJECT_H
#define E_MOD_COMP_OBJECT_H

typedef struct _E_Comp_Object E_Comp_Object;

struct _E_Comp_Object
{
   E_Comp_Canvas  *canvas;
   E_Zone         *zone;

   Evas_Object    *img;         // composite image object
   Evas_Object    *shadow;      // shadow object
   Eina_List      *img_mirror;  // extra mirror image objects
   Ecore_X_Image  *xim;         // x image - software fallback

   Evas_Object    *clipper;

   Eina_Bool       native : 1;  // native
   Eina_Bool       needxim : 1; // need new xim

   struct
   {
      Evas_Object *offset;      // offset rectangle object for transparent rectangle
      Evas_Object *rect;        // transparent rectangle object
   } transp;

   struct
   {
      Evas_Object *mask_rect;      // mask rectangle object for blending set_drawable area
   } hwc;
};

EAPI E_Comp_Object *e_mod_comp_obj_add(E_Comp_Win *cw, E_Comp_Canvas *canvas);
EAPI void           e_mod_comp_obj_del(E_Comp_Object *co);

EAPI Eina_List     *e_mod_comp_win_comp_objs_add(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_del(E_Comp_Win *cw, Eina_List *objs);
EAPI void           e_mod_comp_win_comp_objs_move(E_Comp_Win *cw, int x, int y);
EAPI void           e_mod_comp_win_comp_objs_resize(E_Comp_Win *cw, int w, int h);
EAPI void           e_mod_comp_win_comp_objs_img_resize(E_Comp_Win *cw, int w, int h);
EAPI void           e_mod_comp_win_comp_objs_img_init(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_img_deinit(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_xim_free(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_img_pass_events_set(E_Comp_Win *cw, Eina_Bool set);
EAPI void           e_mod_comp_win_comp_objs_pass_events_set(E_Comp_Win *cw, Eina_Bool set);
EAPI void           e_mod_comp_win_comp_objs_img_alpha_set(E_Comp_Win *cw, Eina_Bool alpha);
EAPI void           e_mod_comp_win_comp_objs_img_size_set(E_Comp_Win *cw, int w, int h);
EAPI void           e_mod_comp_win_comp_objs_img_data_update_add(E_Comp_Win *cw, int x, int y, int w, int h);
EAPI void           e_mod_comp_win_comp_objs_needxim_set(E_Comp_Win *cw, Eina_Bool need);
EAPI void           e_mod_comp_win_comp_objs_native_set(E_Comp_Win *cw, Eina_Bool native);
EAPI void           e_mod_comp_win_comp_objs_data_del(E_Comp_Win *cw, const char *key);
EAPI void           e_mod_comp_win_comp_objs_data_set(E_Comp_Win *cw, const char *key, const void *data);
EAPI void           e_mod_comp_win_comp_objs_show(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_force_show(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_hide(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_raise(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_lower(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_comp_objs_stack_above(E_Comp_Win *cw, E_Comp_Win *cw2);
EAPI void           e_mod_comp_win_comp_objs_transparent_rect_update(E_Comp_Win *cw);

EAPI void           e_mod_comp_win_hwcomp_mask_objs_show(E_Comp_Win *cw);
EAPI void           e_mod_comp_win_hwcomp_mask_objs_hide(E_Comp_Win *cw);

#endif
#endif
