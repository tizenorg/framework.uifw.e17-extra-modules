#ifdef E_TYPEDEFS
#else
#ifndef EFFECT_IMAGE_LAUNCH_H
#define EFFECT_IMAGE_LAUNCH_H

/* image launch effect functions */
 E_Comp_Effect_Image_Launch *_effect_mod_image_launch_new(Evas *e, int w, int h);
 void                        _effect_mod_image_launch_free(E_Comp_Effect_Image_Launch *eff);
 Eina_Bool                   _effect_mod_image_launch_handler_message(Ecore_X_Event_Client_Message *ev);
 Eina_Bool                   _effect_mod_image_launch_show(E_Comp_Effect_Image_Launch *eff, const char *file);
 Eina_Bool                   _effect_mod_image_launch_hide(E_Comp_Effect_Image_Launch *eff);
 Eina_Bool                   _effect_mod_image_launch_window_check(E_Comp_Effect_Image_Launch *eff, E_Comp_Win *cw);
 Eina_Bool                   _effect_mod_image_launch_running_check(E_Comp_Effect_Image_Launch *eff);
 Eina_Bool                   _effect_mod_image_launch_fake_show_done_check(E_Comp_Effect_Image_Launch *eff);
 Eina_Bool                   _effect_mod_image_launch_window_set(E_Comp_Effect_Image_Launch *eff, Ecore_X_Window w);
 void                        _effect_mod_image_launch_disable(E_Comp_Effect_Image_Launch *eff);

#endif
#endif
