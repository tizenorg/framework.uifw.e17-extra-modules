#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"

/* externally accessible functions */
EINTERN E_Move_Border *
e_mod_move_pwlock_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_PWLOCK_CHECK(mb)) return mb;
     }
   return NULL;
}

