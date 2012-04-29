#include "e.h"
#include "e_mod_main.h"
#include "e_mod_move_win_type.h"

static Eina_Bool _e_move_init(void);
static void _e_move_shutdown(void);

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Window Move Helper"
};

EAPI void*
e_modapi_init(E_Module* m)
{
   if (!_e_move_init())
     {
        printf("[move][%s] Failed @ _e_move_init()..!\n", __FUNCTION__);
        return NULL;
     }

   return m;
}

EAPI int
e_modapi_shutdown(E_Module* m)
{
   _e_move_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module* m)
{
   return 1;
}

static Eina_Bool
_e_move_init(void)
{
   if (!e_mod_move_init())
     {
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_e_move_shutdown(void)
{
   e_mod_move_shutdown();
}
