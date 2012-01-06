#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_ROTATION_H
#define E_MOD_COMP_ROTATION_H

typedef struct _E_Comp_Rotation E_Comp_Rotation;

E_Comp_Rotation *e_mod_comp_rotation_new                      (void);
void             e_mod_comp_rotation_free                     (E_Comp_Rotation *rot);
Eina_Bool        e_mod_comp_rotation_begin                    (E_Comp_Rotation *rot);
Eina_Bool        e_mod_comp_rotation_end                      (E_Comp_Rotation *rot);
Eina_Bool        e_mod_comp_rotation_request                  (E_Comp_Rotation* rot,
                                                               Ecore_X_Event_Client_Message *ev,
                                                               Evas *evas,
                                                               Evas_Object *shobj,
                                                               Evas_Object *obj,
                                                               Ecore_X_Visual win_vis,
                                                               int win_x, int win_y,
                                                               int win_w, int win_h );
Eina_Bool        e_mod_comp_rotation_damage                   (E_Comp_Rotation *rot);
Eina_Bool        e_mod_comp_rotation_update                   (E_Comp_Rotation *rot,
                                                               E_Update *up,
                                                               int win_x, int win_y,
                                                               int win_w, int win_h,
                                                               int border_w );
Eina_Bool        e_mod_comp_rotation_resize                   (E_Comp_Rotation *rot,
                                                               Evas *evas,
                                                               Evas_Object *shobj,
                                                               Evas_Object *obj,
                                                               Ecore_X_Visual win_vis,
                                                               int win_x, int win_y,
                                                               int win_w, int win_h );
Ecore_X_Damage   e_mod_comp_rotation_get_damage               (E_Comp_Rotation *rot);
Evas_Object     *e_mod_comp_rotation_get_shobj                (E_Comp_Rotation *rot);
unsigned int     e_mod_comp_rotation_get_angle                (E_Comp_Rotation *rot);
Eina_Bool        e_mod_comp_rotation_angle_is_changed         (E_Comp_Rotation *rot);

Eina_Bool        e_mod_comp_rotation_done_send                (Ecore_X_Window win, Ecore_X_Atom type);

void             e_mod_comp_rotation_show_effect              (E_Comp_Rotation *rot);
void             e_mod_comp_rotation_end_effect               (E_Comp_Rotation *rot);
void             e_mod_comp_rotation_request_effect           (E_Comp_Rotation *rot);

#endif
#endif
