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
};

EINTERN E_Comp_Object *e_mod_comp_obj_add(E_Comp_Win *cw, E_Comp_Canvas *canvas);
EINTERN void           e_mod_comp_obj_del(E_Comp_Object *co);

EINTERN Eina_List     *e_mod_comp_win_comp_objs_add(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_del(E_Comp_Win *cw, Eina_List *objs);
EINTERN void           e_mod_comp_win_comp_objs_move(E_Comp_Win *cw, int x, int y);
EINTERN void           e_mod_comp_win_comp_objs_resize(E_Comp_Win *cw, int w, int h);
EINTERN void           e_mod_comp_win_comp_objs_img_resize(E_Comp_Win *cw, int w, int h);
EINTERN void           e_mod_comp_win_comp_objs_img_init(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_img_deinit(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_xim_free(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_img_pass_events_set(E_Comp_Win *cw, Eina_Bool set);
EINTERN void           e_mod_comp_win_comp_objs_pass_events_set(E_Comp_Win *cw, Eina_Bool set);
EINTERN void           e_mod_comp_win_comp_objs_img_alpha_set(E_Comp_Win *cw, Eina_Bool alpha);
EINTERN void           e_mod_comp_win_comp_objs_img_size_set(E_Comp_Win *cw, int w, int h);
EINTERN void           e_mod_comp_win_comp_objs_img_data_update_add(E_Comp_Win *cw, int x, int y, int w, int h);
EINTERN void           e_mod_comp_win_comp_objs_needxim_set(E_Comp_Win *cw, Eina_Bool need);
EINTERN void           e_mod_comp_win_comp_objs_native_set(E_Comp_Win *cw, Eina_Bool native);
EINTERN void           e_mod_comp_win_comp_objs_data_del(E_Comp_Win *cw, const char *key);
EINTERN void           e_mod_comp_win_comp_objs_data_set(E_Comp_Win *cw, const char *key, const void *data);
EINTERN void           e_mod_comp_win_comp_objs_show(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_force_show(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_hide(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_raise(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_lower(E_Comp_Win *cw);
EINTERN void           e_mod_comp_win_comp_objs_stack_above(E_Comp_Win *cw, E_Comp_Win *cw2);
EINTERN void           e_mod_comp_win_comp_objs_transparent_rect_update(E_Comp_Win *cw);
#endif
#endif
