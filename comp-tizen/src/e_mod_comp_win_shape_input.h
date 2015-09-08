#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_WIN_SHAPE_INPUT_H
#define E_MOD_COMP_WIN_SHAPE_INPUT_H

typedef struct _E_Comp_Win_Shape_Input E_Comp_Win_Shape_Input;
typedef struct _E_Comp_Shape_Input E_Comp_Shape_Input;

/* shape input region functions */
EAPI E_Comp_Win_Shape_Input *e_mod_comp_win_shape_input_new(E_Comp_Win *cw);
EAPI void                    e_mod_comp_win_shape_input_free(E_Comp_Win_Shape_Input *input);
EAPI Eina_Bool               e_mod_comp_win_shape_input_rect_set(E_Comp_Win_Shape_Input *input, int x, int y, int w, int h);
EAPI Eina_Bool               e_mod_comp_win_shape_input_invalid_set(E_Comp *c, Eina_Bool set);
EAPI Eina_Bool               e_mod_comp_win_shape_input_update(E_Comp *c);
EAPI int                     e_mod_comp_shape_input_new(E_Comp *c);
EAPI Eina_Bool               e_mod_comp_shape_input_set(E_Comp *c, int id, int x, int y, int w, int h);
EAPI Eina_Bool               e_mod_comp_shape_input_del(E_Comp *c, int id);
EAPI void                    e_mod_comp_shape_input_manage(E_Comp *c);
EAPI Eina_Bool               e_mod_comp_shape_input_managed_set(E_Comp *c, int id, Evas_Object *obj, Eina_Bool set);
#endif
#endif
