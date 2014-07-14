#include "e_mod_eom_privates.h"
#include "e_mod_eom_config.h"
#include "e_mod_eom_server.h"

/* local variables */
static E_Config_DD *_eom_conf_edd = NULL;

/* external variables */
E_EOM_Config *_e_eom_cfg = NULL;

/* local functions */
static void
_eom_config_new(void)
{
   /* create initial config */
   _e_eom_cfg = E_NEW(E_EOM_Config, 1);
   _e_eom_cfg->ScrnConf.default_dispmode = EOM_OUTPUT_MODE_CLONE;
   _e_eom_cfg->ScrnConf.isPopUpEnabled = EINA_FALSE;
}

static void
_eom_config_free(void)
{
   /* check for config */
   if (!_e_eom_cfg) return;

   /* free config structure */
   E_FREE(_e_eom_cfg);
}

int
eom_config_init(void)
{
   /* create config structure for module */
   _eom_conf_edd = E_CONFIG_DD_NEW("EOM_Config", E_EOM_Config);

#undef T
#undef D
#define T E_EOM_Config
#define D _eom_conf_edd
   E_CONFIG_VAL(D, T, ScrnConf.enable, UCHAR);
   E_CONFIG_VAL(D, T, ScrnConf.default_dispmode, INT);
   E_CONFIG_VAL(D, T, ScrnConf.isPopUpEnabled, UCHAR);

   /* attempt to load existing configuration */
   _e_eom_cfg = e_config_domain_load(E_EOM_CFG, _eom_conf_edd);

   /* create new config if we need to */
   if (!_e_eom_cfg)
     {
        _eom_config_new();
        eom_config_save();
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cfg] Config file for e_eom was made/stored !\n");
     }
   else
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cfg] Config file for e_eom was loaded successfully !\n");
     }

   return 1;
}

int
eom_config_shutdown(void)
{
   /* free config structure */
   _eom_config_free();

   /* free data descriptors */
   E_CONFIG_DD_FREE(_eom_conf_edd);

   return 1;
}

int
eom_config_save(void)
{
   return e_config_domain_save(E_EOM_CFG, _eom_conf_edd, _e_eom_cfg);
}
