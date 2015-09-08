#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_BORDER_CONTENTS_H
#define E_MOD_MOVE_BORDER_CONTENTS_H

typedef struct _E_Move_Border_Contents E_Move_Border_Contents;

struct _E_Move_Border_Contents
{
   int x, y, w, h;
};

/* window contents region functions */
EINTERN E_Move_Border_Contents *e_mod_move_border_contents_new(E_Move_Border *mb);
EINTERN void                    e_mod_move_border_contents_free(E_Move_Border *mb);
EINTERN Eina_Bool               e_mod_move_border_contents_rect_set(E_Move_Border *mb, int x, int y, int w, int h);
EINTERN Eina_Bool               e_mod_move_border_contents_rect_get(E_Move_Border *mb, int *x, int *y, int *w, int *h);

#endif
#endif
