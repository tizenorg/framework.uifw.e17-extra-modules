#ifndef _E_MOD_EOM_CLONE_H_
#define _E_MOD_EOM_CLONE_H_

#include <X11/Xlib.h>
#include <X11/Xatom.h>

Eina_Bool eom_clone_start (int x, int y, int width, int height, char *profile);
void eom_clone_stop (void);

#endif /* _E_MOD_EOM_CLONE_H_ */
