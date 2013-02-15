#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_ANIMATION_H
#define E_MOD_COMP_ANIMATION_H

#define SWITCHER_DURATION_TRANSLATE (0.33)
#define SWITCHER_DURATION_ROTATE    (0.33)
#define SWITCHER_DURATION_TOP       ((SWITCHER_DURATION_TRANSLATE) + (SWITCHER_DURATION_ROTATE))
#define WINDOW_SPACE                (100)
#define ROTATE_ANGLE_BEGIN          (70)
#define ROTATE_ANGLE_TOP            (-70)
#define ROTATE_ANGLE_LEFT           (-30)

typedef struct _E_Comp_Transfer E_Comp_Transfer;

struct _E_Comp_Transfer
{
   Evas_Object    *obj;
   float           from;
   float           len;
   double          begin_time;
   double          duration;
   Ecore_Animator *animator;
   Eina_Bool       selected;
};

/* transfer functions */
EINTERN E_Comp_Transfer *e_mod_comp_animation_transfer_new(void);
EINTERN void             e_mod_comp_animation_transfer_free(E_Comp_Transfer *tr);
EINTERN Eina_Bool        e_mod_comp_animation_transfer_list_clear(void);

/* animation functions */
EINTERN Eina_Bool        e_mod_comp_animation_on_rotate_top(void *data);
EINTERN Eina_Bool        e_mod_comp_animation_on_rotate_left(void *data);
EINTERN Eina_Bool        e_mod_comp_animation_on_translate(void *data);

#endif
#endif
