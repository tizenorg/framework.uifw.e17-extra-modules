#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e_mod_devmode.h"
#include "e_mod_config.h"

typedef struct _Mod Mod;
struct _Mod
{
   E_Module        *module;

   E_Config_DD     *conf_edd;
   E_Config_DD     *conf_match_edd;
   Config          *conf;

   E_Config_Dialog *config_dialog;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save(E_Module *m);

/**
 * @t_infodtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot TouchInfo
 *
 * Get Touch information
 *
 * @}
 */

#endif
