#include "e_mod_devmode.h"
#include "e_mod_config.h"

EAPI void
e_mod_devmode_cfdata_edd_init(E_Config_DD **conf_edd)
{
   *conf_edd = E_CONFIG_DD_NEW("Devmode_Config", Config);
#undef T
#undef D
#define T Config
#define D *conf_edd
   E_CONFIG_VAL(D, T, drawing_option, INT);
}

EAPI Config *
e_mod_devmode_cfdata_config_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);

   cfg->drawing_option = DRAW_ALL;

   return cfg;
}

EAPI void
e_mod_devmode_cfdata_config_free(Config *cfg)
{
   if (cfg)
     {
        memset(cfg, 0, sizeof(Config));
        free(cfg);
        cfg = NULL;
     }
}

