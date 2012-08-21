#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_UTIL_H
#define E_MOD_MOVE_UTIL_H

/* check whether region of given window is equal to root */
#define REGION_EQUAL_TO_ROOT(a) \
   (((a)->x == 0) && ((a)->y == 0) && \
    (((a)->w) == ((a)->m->man->w)) && \
    (((a)->h) == ((a)->m->man->h)))

#define SIZE_EQUAL_TO_ROOT(a) \
   ((((a)->w) == ((a)->m->man->w)) && \
    (((a)->h) == ((a)->m->man->h)))

#define REGION_EQUAL_TO_ZONE(a, z) \
   ((((a)->x) == ((z)->x)) && \
    (((a)->y) == ((z)->y)) && \
    (((a)->w) == ((z)->w)) && \
    (((a)->h) == ((z)->h)))

#define REGION_INTERSECTS_WITH_ZONE(a, z) \
   ((((a)->x) < (((z)->x) + ((z)->w))) && \
    (((a)->y) < (((z)->y) + ((z)->h))) && \
    ((((a)->x) + ((a)->w)) > ((z)->x)) && \
    ((((a)->y) + ((a)->h)) > ((z)->y)))

#define REGION_INSIDE_ZONE(a, z) \
   ((((a)->x) >= ((z)->x)) && \
    ((((z)->x) + ((z)->w)) >= (((a)->x) + ((a)->w))) && \
    (((a)->y) >= ((z)->y)) && \
    ((((z)->y) + ((z)->h)) >= (((a)->y) + ((a)->h))))

EINTERN void           e_mod_move_util_set(E_Move *m, E_Manager *man);
EINTERN E_Move        *e_mod_move_util_get(void);
EINTERN Eina_Bool      e_mod_move_util_border_visible_get(E_Move_Border *mb);
EINTERN Ecore_X_Window e_mod_move_util_client_xid_get(E_Move_Border *mb);
EINTERN Eina_Bool      e_mod_move_util_win_prop_angle_get(Ecore_X_Window win, int *req, int *curr);
EINTERN void           e_mod_move_util_border_hidden_set(E_Move_Border *mb, Eina_Bool hidden);
EINTERN void           e_mod_move_util_rotation_lock(E_Move *m);
EINTERN void           e_mod_move_util_rotation_unlock(E_Move *m);
EINTERN Eina_Bool      e_mod_move_util_compositor_object_visible_get(E_Move_Border *mb);
EINTERN E_Move_Border *e_mod_move_util_visible_fullscreen_window_find(void);
EINTERN void           e_mod_move_util_compositor_composite_mode_set(E_Move *m, Eina_Bool set);

#endif
#endif
