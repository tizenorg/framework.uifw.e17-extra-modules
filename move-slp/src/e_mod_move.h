#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_H
#define E_MOD_MOVE_H

#include "e_mod_move_shared_types.h"

EINTERN Eina_Bool      e_mod_move_init(void);
EINTERN void           e_mod_move_shutdown(void);

EINTERN E_Move_Border *e_mod_move_border_find(Ecore_X_Window win);
EINTERN E_Move_Border *e_mod_move_border_client_find(Ecore_X_Window win);
EINTERN E_Move        *e_mod_move_find(Ecore_X_Window win);
EINTERN void           e_mod_move_border_del(E_Move_Border * mb);

#endif
#endif
