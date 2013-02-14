#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_SCREEN_CAPTURE_H
#define E_MOD_COMP_EFFECT_SCREEN_CAPTURE_H

typedef struct _E_Comp_Effect_Screen_Capture E_Comp_Effect_Screen_Capture;

/* screen capture effect functions */
EINTERN E_Comp_Effect_Screen_Capture *e_mod_comp_effect_screen_capture_new(Evas *evas, int w, int h);
EINTERN void                          e_mod_comp_effect_screen_capture_free(E_Comp_Effect_Screen_Capture *cap);
EINTERN Eina_Bool                     e_mod_comp_effect_screen_capture_handler_message(Ecore_X_Event_Client_Message *ev);

#endif
#endif
