#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_PROCESSMGR_H
#define E_MOD_PROCESSMGR_H

#include "e_mod_processmgr_shared_types.h"

EINTERN Eina_Bool      e_mod_processmgr_init(void);
EINTERN void           e_mod_processmgr_shutdown(void);

EINTERN E_ProcessInfo *e_mod_processinfo_find(unsigned int pid);
EINTERN E_WindowInfo  *e_mod_windowinfo_find(Ecore_X_Window win);

#endif
#endif
