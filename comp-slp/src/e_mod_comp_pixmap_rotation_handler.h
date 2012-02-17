#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_PIXMAP_ROTATION_HANDLER_H
#define E_MOD_COMP_PIXMAP_ROTATION_HANDLER_H

EINTERN Eina_Bool e_mod_comp_pixmap_rotation_handler_update(E_Comp_Win *cw);
EINTERN Eina_Bool e_mod_comp_pixmap_rotation_handler_release(E_Comp_Win *cw);
EINTERN Eina_Bool e_mod_comp_pixmap_rotation_handler_message(Ecore_X_Event_Client_Message *ev);
EINTERN Eina_Bool e_mod_comp_pixmap_rotation_handler_configure(E_Comp_Win *cw, int w, int h);
EINTERN Eina_Bool e_mod_comp_pixmap_rotation_handler_damage(E_Comp_Win *cw, Eina_Bool dmg);
EINTERN Eina_Bool e_mod_comp_pixmap_rotation_handler_hide(E_Comp_Win *cw);

#endif
#endif
