#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_ANIMATION_H
#define E_MOD_COMP_ANIMATION_H

#include "e_mod_comp_data.h"

#if COMP_LOGGER_BUILD_ENABLE
extern int comp_logger_type                        ;
extern Ecore_X_Atom ATOM_CM_LOG                    ;
#endif

#define SWITCHER_DURATION_TOP 0.66 // sum of SWITCHER_DURATION_TRANSLATE and SWITCHER_DURATION_ROTATE
#define SWITCHER_DURATION_TRANSLATE 0.33
#define SWITCHER_DURATION_ROTATE 0.33
#define WINDOW_SPACE 100
#define ROTATE_ANGLE_BEGIN 70
#define ROTATE_ANGLE_TOP -70
#define ROTATE_ANGLE_LEFT -30

struct _E_Comp_Transfer {
   Evas_Object* obj;
   float from, len;
   double begin_time;
   double duration;
   Ecore_Animator *animator;
   Eina_Bool selected;
};

void                      _e_mod_comp_done_defer                   (E_Comp_Win *cw);
E_Comp                   *_e_mod_comp_get                          (void);
Eina_Bool                 on_animate_rotate_top                    (void *data);
Eina_Bool                 on_animate_rotate_left                   (void *data);
Eina_Bool                 on_animate_translate                     (void *data);

#endif
#endif
