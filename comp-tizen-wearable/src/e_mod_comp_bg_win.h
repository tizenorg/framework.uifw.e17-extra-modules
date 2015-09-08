#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_BG_WIN_H
#define E_MOD_COMP_BG_WIN_H

typedef struct _E_Comp_BG_Win E_Comp_BG_Win;

/* background window setup and handler functions */
EAPI E_Comp_BG_Win *e_mod_comp_bg_win_new(void);
EAPI void           e_mod_comp_bg_win_free(E_Comp_BG_Win *bg);
EAPI Eina_Bool      e_mod_comp_bg_win_handler_prop(Ecore_X_Event_Window_Property *ev);
EAPI Eina_Bool      e_mod_comp_bg_win_handler_release(E_Comp_Win *cw);
EAPI Eina_Bool      e_mod_comp_bg_win_handler_show(E_Comp_Win *cw);
EAPI Eina_Bool      e_mod_comp_bg_win_handler_update(E_Comp_Win *cw);

#endif
#endif
