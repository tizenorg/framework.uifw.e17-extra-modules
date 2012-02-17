#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_PIXMAP_ROTATION_H
#define E_MOD_COMP_PIXMAP_ROTATION_H

typedef struct _E_Comp_Pixmap_Rotation E_Comp_Pixmap_Rotation;

EINTERN E_Comp_Pixmap_Rotation *e_mod_comp_pixmap_rotation_new(void);
EINTERN void                    e_mod_comp_pixmap_rotation_free(E_Comp_Pixmap_Rotation *o);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_begin(E_Comp_Pixmap_Rotation *o);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_end(E_Comp_Pixmap_Rotation *o);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_request(E_Comp_Pixmap_Rotation *o,
                                                                   Ecore_X_Event_Client_Message *ev,
                                                                   Evas *evas,
                                                                   Evas_Object *shobj, Evas_Object *obj,
                                                                   Ecore_X_Visual win_vis,
                                                                   int win_w, int win_h);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_damage(E_Comp_Pixmap_Rotation *o);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_update(E_Comp_Pixmap_Rotation *o,
                                                                  E_Update *up,
                                                                  int win_x, int win_y,
                                                                  int win_w, int win_h,
                                                                  int bd_w);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_resize(E_Comp_Pixmap_Rotation *o,
                                                                  Evas *evas,
                                                                  Evas_Object *shobj,
                                                                  Evas_Object *obj,
                                                                  Ecore_X_Visual win_vis,
                                                                  int win_w, int win_h);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_state_set(E_Comp_Pixmap_Rotation *o,
                                                                     Eina_Bool state);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_state_get(E_Comp_Pixmap_Rotation *o);
EINTERN Ecore_X_Damage          e_mod_comp_pixmap_rotation_damage_get(E_Comp_Pixmap_Rotation *o);
EINTERN Evas_Object            *e_mod_comp_pixmap_rotation_shobj_get(E_Comp_Pixmap_Rotation *o);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_angle_check(E_Comp_Pixmap_Rotation *o);
EINTERN Eina_Bool               e_mod_comp_pixmap_rotation_done_send(Ecore_X_Window win, Ecore_X_Atom type);
EINTERN void                    e_mod_comp_pixmap_rotation_effect_show(E_Comp_Pixmap_Rotation *o);
EINTERN void                    e_mod_comp_pixmap_rotation_effect_end(E_Comp_Pixmap_Rotation *o);
EINTERN void                    e_mod_comp_pixmap_rotation_effect_request(E_Comp_Pixmap_Rotation *o);

#endif
#endif
