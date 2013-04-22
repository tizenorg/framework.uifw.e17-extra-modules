#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_BG_WIN_H
#define E_MOD_COMP_BG_WIN_H

typedef struct _E_Comp_BG_Win E_Comp_BG_Win;

/* background window setup and handler functions */
EINTERN E_Comp_BG_Win *e_mod_comp_bg_win_new(void);
EINTERN void           e_mod_comp_bg_win_free(E_Comp_BG_Win *bg);
EINTERN Eina_Bool      e_mod_comp_bg_win_handler_prop(Ecore_X_Event_Window_Property *ev);
EINTERN Eina_Bool      e_mod_comp_bg_win_handler_release(E_Comp_Win *cw);
EINTERN Eina_Bool      e_mod_comp_bg_win_handler_show(E_Comp_Win *cw);
EINTERN Eina_Bool      e_mod_comp_bg_win_handler_update(E_Comp_Win *cw);

#endif
#endif
