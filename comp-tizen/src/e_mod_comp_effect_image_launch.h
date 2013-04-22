#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_IMAGE_LAUNCH_H
#define E_MOD_COMP_EFFECT_IMAGE_LAUNCH_H

typedef struct _E_Comp_Effect_Image_Launch E_Comp_Effect_Image_Launch;

/* image launch effect functions */
EINTERN E_Comp_Effect_Image_Launch *e_mod_comp_effect_image_launch_new(Evas *e, int w, int h);
EINTERN void                        e_mod_comp_effect_image_launch_free(E_Comp_Effect_Image_Launch *eff);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_handler_message(Ecore_X_Event_Client_Message *ev);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_show(E_Comp_Effect_Image_Launch *eff, const char *file);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_hide(E_Comp_Effect_Image_Launch *eff);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_window_check(E_Comp_Effect_Image_Launch *eff, E_Comp_Win *cw);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_running_check(E_Comp_Effect_Image_Launch *eff);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_fake_show_done_check(E_Comp_Effect_Image_Launch *eff);
EINTERN Eina_Bool                   e_mod_comp_effect_image_launch_window_set(E_Comp_Effect_Image_Launch *eff, Ecore_X_Window w);
EINTERN void                        e_mod_comp_effect_image_launch_disable(E_Comp_Effect_Image_Launch *eff);

#endif
#endif
