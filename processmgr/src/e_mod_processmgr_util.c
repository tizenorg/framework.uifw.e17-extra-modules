#include "e_mod_processmgr_shared_types.h"
#include "e_mod_processmgr_debug.h"

/* local subsystem globals */
static E_ProcessMgr *_pm = NULL;

/* externally accessible functions */
EINTERN void
e_mod_processmgr_util_set(E_ProcessMgr  *pm,
                          E_Manager *man __UNUSED__)
{
   _pm = pm;
}

EINTERN E_ProcessMgr *
e_mod_processmgr_util_get(void)
{
   return _pm;
}
