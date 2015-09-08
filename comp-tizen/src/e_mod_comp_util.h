#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_UTIL_H
#define E_MOD_COMP_UTIL_H

/* check whether region of given window is equal to root */
#define REGION_EQUAL_TO_ROOT(a)       \
   (((a)->x == 0) && ((a)->y == 0) && \
    (((a)->w) == ((a)->c->man->w)) && \
    (((a)->h) == ((a)->c->man->h)))

#define SIZE_EQUAL_TO_ROOT(a)         \
   ((((a)->w) == ((a)->c->man->w)) && \
    (((a)->h) == ((a)->c->man->h)))

#define REGION_EQUAL_TO_ZONE(a, z)    \
   ((((a)->x) == ((z)->x)) &&         \
    (((a)->y) == ((z)->y)) &&         \
    (((a)->w) == ((z)->w)) &&         \
    (((a)->h) == ((z)->h)))

#define REGION_EQUAL_TO_CANVAS(a, c) \
   ((((a)->x) == ((c)->x)) &&         \
    (((a)->y) == ((c)->y)) &&         \
    (((a)->w) == ((c)->w)) &&         \
    (((a)->h) == ((c)->h)))

#define STATE_INSET_CHECK(a) \
   ((a->bd) && \
    ((a->bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) || \
     (a->bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU)))

#define CLASS_ICONIC_CHECK(a) \
   ((a->bd) && \
    (a->bd->client.icccm.class) && \
    (!strcmp(a->bd->client.icccm.class, "ICON_WIN")))

#define PARENT_FLOAT_CHECK(a) \
      ((a->bd) && \
       (a->bd->parent) && \
       ((a->bd->parent->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) ||\
        (a->bd->parent->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU)))

EAPI void           e_mod_comp_util_set(E_Comp *c, E_Manager *man);
EAPI E_Comp        *e_mod_comp_util_get(void);
EAPI Eina_Bool      e_mod_comp_util_grab_key_set(Eina_Bool grab);
EAPI Eina_Bool      e_mod_comp_util_grab_home_key_set(Eina_Bool grab);
EAPI Eina_Bool      e_mod_comp_util_screen_input_region_set(Eina_Bool set);
EAPI E_Comp_Win    *e_mod_comp_util_win_nocomp_get(E_Comp *c, E_Zone *zone);
#if USE_NOCOMP_DISPOSE
EAPI E_Comp_Win    *e_mod_comp_util_win_nocomp_end_get(E_Comp *c, E_Zone *zone);
#endif
EAPI E_Comp_Win    *e_mod_comp_util_win_normal_get(E_Comp_Win *cw, Eina_Bool opaque);
EAPI E_Comp_Win    *e_mod_comp_util_win_home_get(E_Comp_Win *cw);
EAPI E_Comp_Win    *e_mod_comp_util_win_below_get(E_Comp_Win *cw, Eina_Bool normal_check, Eina_Bool visible_check);
EAPI Eina_Bool      e_mod_comp_util_win_below_check(E_Comp_Win *cw, E_Comp_Win *cw2);
EAPI Eina_Bool      e_mod_comp_util_win_visible_get(E_Comp_Win *cw, Eina_Bool alpha_check);
EAPI Eina_Bool      e_mod_comp_util_win_visible_below_get(E_Comp_Win *cw, E_Comp_Win *cw2);
EAPI Ecore_X_Window e_mod_comp_util_client_xid_get(E_Comp_Win *cw);
EAPI void           e_mod_comp_util_fb_visible_set(Eina_Bool set);
EAPI void           e_mod_comp_util_rr_prop_set(Ecore_X_Atom atom, const char* state);
EAPI Ecore_X_Pixmap e_mod_comp_util_copied_pixmap_get(E_Comp_Win *cw);

#endif
#endif
