#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_H
#define E_MOD_COMP_H

#include "e_mod_comp_shared_types.h"

EAPI Eina_Bool    e_mod_comp_init(void);
EAPI void         e_mod_comp_shutdown(void);
EAPI void         e_mod_comp_shadow_set(void);

EAPI void         e_mod_comp_done_defer(E_Comp_Win *cw);
EAPI Eina_Bool    e_mod_comp_win_add_damage(E_Comp_Win *cw, Ecore_X_Damage dmg);
EAPI Eina_Bool    e_mod_comp_win_del_damage(E_Comp_Win *cw, Ecore_X_Damage dmg);
EAPI E_Comp_Win  *e_mod_comp_win_find(Ecore_X_Window win);
EAPI E_Comp_Win  *e_mod_comp_border_client_find(Ecore_X_Window win);
EAPI Eina_Bool    e_mod_comp_comp_event_src_visibility_send(E_Comp_Win *cw);
EAPI void         e_mod_comp_win_shadow_setup(E_Comp_Win *cw, E_Comp_Object *co);
EAPI void         e_mod_comp_win_cb_setup(E_Comp_Win *cw, E_Comp_Object *co);
EAPI void         e_mod_comp_fps_toggle(void);
EAPI E_Comp      *e_mod_comp_find(Ecore_X_Window win);
EAPI void         e_mod_comp_win_render_queue(E_Comp_Win *cw);
EAPI void         e_mod_comp_render_queue(E_Comp *c);
EAPI Eina_Bool    e_mod_comp_win_damage_timeout(void *data);
EAPI Eina_Bool    e_mod_comp_cb_update(E_Comp *c);
EAPI Evas_Object *e_mod_comp_win_mirror_add(E_Comp_Win *cw);
EAPI void         e_mod_comp_cb_win_mirror_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
EAPI void         e_mod_comp_src_hidden_set_func(void *data, E_Manager *man, E_Manager_Comp_Source *src, Eina_Bool hidden);
EAPI void         e_mod_comp_x_grab_set(E_Comp *c, Eina_Bool grab);
EAPI void         e_mod_comp_composite_mode_set(E_Zone *zone, Eina_Bool set);
EAPI void         e_comp_event_src_visibility_send(Ecore_X_Window win, Eina_Bool visible);

#endif
#endif
