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

# define POINT_INSIDE_ZONE(x, y, z) \
 (((x) <= (((z)->x) + ((z)->w))) && \
  ((y) <= (((z)->y) + ((z)->h))) && \
  ((x) >= ((z)->x)) && \
  ((y) >= ((z)->y)))

typedef enum _E_Move_Layer_Policy
{
   E_MOVE_NOTIFICATION_LAYER = 0,
   E_MOVE_INDICATOR_LAYER,
   E_MOVE_QUICKPANEL_LAYER,
   E_MOVE_STATE_ABOVE_LAYER,
   E_MOVE_ACTIVATE_LAYER,
   E_MOVE_FULLSCREEN_LAYER,
   E_MOVE_DIALOG_LAYER,
   E_MOVE_SPLASH_LAYER,
   E_MOVE_SOFTKEY_LAYER,
   E_MOVE_CLIPBOARD_LAYER,
   E_MOVE_KEYBOARD_LAYER,
   E_MOVE_CONFORMANT_LAYER,
   E_MOVE_APP_LAYER,
   E_MOVE_HOME_LAYER,
   E_MOVE_STATE_BELOW_LAYER
} E_Move_Layer_Policy;

typedef enum _E_Move_Scroll_Region_Indicator
{
   E_MOVE_SCROLL_REGION_NONE = 0,
   E_MOVE_SCROLL_REGION_ALWAYS, // Always Indicator Region
   E_MOVE_SCROLL_REGION_QUICKPANEL, // Quickpanel Indicator Region
   E_MOVE_SCROLL_REGION_APPTRAY
} E_Move_Scroll_Region_Indicator;

EINTERN void                           e_mod_move_util_set(E_Move *m, E_Manager *man);
EINTERN E_Move                        *e_mod_move_util_get(void);
EINTERN Eina_Bool                      e_mod_move_util_border_visible_get(E_Move_Border *mb);
EINTERN Ecore_X_Window                 e_mod_move_util_client_xid_get(E_Move_Border *mb);
EINTERN Eina_Bool                      e_mod_move_util_win_prop_angle_get(Ecore_X_Window win, int *req, int *curr);
EINTERN void                           e_mod_move_util_border_hidden_set(E_Move_Border *mb, Eina_Bool hidden);
EINTERN void                           e_mod_move_util_rotation_lock(E_Move *m);
EINTERN void                           e_mod_move_util_rotation_unlock(E_Move *m);
EINTERN Eina_Bool                      e_mod_move_util_compositor_object_visible_get(E_Move_Border *mb);
EINTERN E_Move_Border                 *e_mod_move_util_visible_fullscreen_window_find(void);
EINTERN void                           e_mod_move_util_compositor_composite_mode_set(E_Move *m, Eina_Bool set);
EINTERN void                           e_mod_move_util_fb_move(int angle, int cw, int ch, int x, int y);
EINTERN int                            e_mod_move_util_layer_policy_get(E_Move_Layer_Policy layer);
EINTERN E_Move_Scroll_Region_Indicator e_mod_move_indicator_region_scroll_check(int angle, Evas_Point input);

#endif
#endif
