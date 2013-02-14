#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_WIN_SHAPE_INPUT_H
#define E_MOD_COMP_WIN_SHAPE_INPUT_H

typedef struct _E_Comp_Win_Shape_Input E_Comp_Win_Shape_Input;

/* shape input region functions */
EINTERN E_Comp_Win_Shape_Input *e_mod_comp_win_shape_input_new(E_Comp_Win *cw);
EINTERN void                    e_mod_comp_win_shape_input_free(E_Comp_Win_Shape_Input *input);
EINTERN Eina_Bool               e_mod_comp_win_shape_input_rect_set(E_Comp_Win_Shape_Input *input, int x, int y, int w, int h);
EINTERN Eina_Bool               e_mod_comp_win_shape_input_invalid_set(E_Comp *c, Eina_Bool set);
EINTERN Eina_Bool               e_mod_comp_win_shape_input_update(E_Comp *c);

#endif
#endif
