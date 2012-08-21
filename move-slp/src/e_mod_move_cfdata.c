#include "e.h"
#include "e_mod_main.h"
#include "e_mod_move_cfdata.h"

EAPI void
e_mod_move_cfdata_edd_init(E_Config_DD **conf_edd)
{
   *conf_edd = E_CONFIG_DD_NEW("Move_Config", Config);
#undef T
#undef D
#define T Config
#define D *conf_edd
   E_CONFIG_VAL(D, T, indicator_home_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, qp_scroll_with_visible_win, UCHAR);
   E_CONFIG_VAL(D, T, dim_max_opacity, INT);
   E_CONFIG_VAL(D, T, dim_min_opacity, INT);
   E_CONFIG_VAL(D, T, flick_speed_limit, DOUBLE);
   E_CONFIG_VAL(D, T, animation_duration, DOUBLE);
   E_CONFIG_VAL(D, T, event_log, UCHAR);
   E_CONFIG_VAL(D, T, event_log_count, INT);
}

EAPI Config *
e_mod_move_cfdata_config_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);

   cfg->indicator_home_region_ratio = 0.33;
   cfg->qp_scroll_with_visible_win = 1;
   cfg->dim_max_opacity = 200;
   cfg->dim_min_opacity = 0;
   cfg->flick_speed_limit = 200.0;
   cfg->animation_duration = 0.4;
   cfg->event_log = 1;
   cfg->event_log_count = 600;

   return cfg;
}

EAPI void
e_mod_move_cfdata_config_free(Config *cfg)
{
   if (cfg)
     {
        memset(cfg, 0, sizeof(Config));
        free(cfg);
        cfg = NULL;
     }
}
