#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_UTIL_H
#define E_MOD_COMP_UTIL_H

/* check whether region of given window is equal to root */
#define REGION_EQUAL_TO_ROOT(a) \
   (((a)->x == 0) && ((a)->y == 0) && \
    (((a)->w) == ((a)->c->man->w)) && \
    (((a)->h) == ((a)->c->man->h)))

#define SIZE_EQUAL_TO_ROOT(a) \
   ((((a)->w) == ((a)->c->man->w)) && \
    (((a)->h) == ((a)->c->man->h)))

#define STATE_INSET_CHECK(a) \
   ((a->bd) && \
    (a->bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_INSET))

EINTERN void           e_mod_comp_util_set(E_Comp *c, E_Manager *man);
EINTERN E_Comp        *e_mod_comp_util_get(void);
EINTERN Eina_Bool      e_mod_comp_util_grab_key_set(Eina_Bool grab);
EINTERN Eina_Bool      e_mod_comp_util_screen_input_region_set(Eina_Bool set);
EINTERN E_Comp_Win    *e_mod_comp_util_win_normal_get(void);
EINTERN E_Comp_Win    *e_mod_comp_util_win_below_get(E_Comp_Win *cw, Eina_Bool normal_check);
EINTERN Eina_Bool      e_mod_comp_util_win_below_check(E_Comp_Win *cw, E_Comp_Win *cw2);
EINTERN Eina_Bool      e_mod_comp_util_win_visible_get(E_Comp_Win *cw);
EINTERN Ecore_X_Window e_mod_comp_util_client_xid_get(E_Comp_Win *cw);

#endif
#endif
