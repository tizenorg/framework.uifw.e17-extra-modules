#include "e.h"
#include "e_mod_main.h"
#include "e_mod_processmgr_cfdata.h"
#include "e_mod_processmgr_shared_types.h"

EAPI void
e_mod_processmgr_cfdata_edd_init(E_Config_DD **conf_edd)
{
   *conf_edd = E_CONFIG_DD_NEW("ProcessMgr_Config", Config);
#undef T
#undef D
#define T Config
#define D *conf_edd
   E_CONFIG_VAL(D, T, enable, UCHAR);
   E_CONFIG_VAL(D, T, tizen_anr_trigger_timeout, UINT);
   E_CONFIG_VAL(D, T, tizen_anr_confirm_timeout, UINT);
   E_CONFIG_VAL(D, T, tizen_anr_ping_interval_timeout, UINT);
#undef T
#undef D
}

EAPI Config *
e_mod_processmgr_cfdata_config_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);

   cfg->enable = 1;
   cfg->tizen_anr_trigger_timeout = 1;
   cfg->tizen_anr_confirm_timeout = 10;
   cfg->tizen_anr_ping_interval_timeout = 20000;

   return cfg;
}

EAPI void
e_mod_processmgr_cfdata_config_free(Config *cfg)
{
   if (cfg)
     {
        memset(cfg, 0, sizeof(Config));
        free(cfg);
        cfg = NULL;
     }
}
