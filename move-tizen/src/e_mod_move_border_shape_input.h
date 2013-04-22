#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_BORDER_SHAPE_INPUT_H
#define E_MOD_MOVE_BORDER_SHAPE_INPUT_H

typedef struct _E_Move_Border_Shape_Input E_Move_Border_Shape_Input;

struct _E_Move_Border_Shape_Input
{
   int x, y, w, h;
};

/* shape input region functions */
EINTERN E_Move_Border_Shape_Input *e_mod_move_border_shape_input_new(E_Move_Border *mb);
EINTERN void                       e_mod_move_border_shape_input_free(E_Move_Border *mb);
EINTERN Eina_Bool                  e_mod_move_border_shape_input_rect_set(E_Move_Border *mb, int x, int y, int w, int h);
EINTERN Eina_Bool                  e_mod_move_border_shape_input_rect_get(E_Move_Border *mb, int *x, int *y, int *w, int *h);

#endif
#endif
