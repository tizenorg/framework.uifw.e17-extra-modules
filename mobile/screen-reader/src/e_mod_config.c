#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

EAPI void
e_mod_screen_reader_cfdata_edd_init(E_Config_DD **conf_edd)
{
   *conf_edd = E_CONFIG_DD_NEW("Screen_Reader_Config", Config);

#undef T
#undef D
#define T Config
#define D *conf_edd
   E_CONFIG_VAL(D, T, three_finger_swipe_timeout, INT);
#undef T
#undef D
}

EAPI Config *
e_mod_screen_reader_cfdata_config_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);

   cfg->three_finger_swipe_timeout = 450;

   return cfg;
}

EAPI void
e_mod_screen_reader_cfdata_config_free(Config *cfg)
{
   if (cfg) E_FREE(cfg);
}
