#include "e_mod_main.h"
#include "e_mod_eom_privates.h"
#include "e_mod_eom_config.h"
#include "e_mod_eom_server.h"

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "External Output Manager Module of Window Manager"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   if (!eom_server_init(m))
     {
        SLOG(LOG_DEBUG, "EOM", "eom_init() failed\n");
        return NULL;
     }

   SLOG(LOG_DEBUG, "EOM", "eom_init!!!!\n");

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   eom_server_deinit(m);

   SLOG(LOG_DEBUG, "EOM", "eom_deinit!!!!\n");

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}
