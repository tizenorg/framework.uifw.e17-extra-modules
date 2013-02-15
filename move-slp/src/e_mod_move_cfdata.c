#include "e.h"
#include "e_mod_main.h"
#include "e_mod_move_cfdata.h"
#include "e_mod_move_shared_types.h"

EAPI void
e_mod_move_cfdata_edd_init(E_Config_DD **conf_edd)
{
   *conf_edd = E_CONFIG_DD_NEW("Move_Config", Config);
#undef T
#undef D
#define T Config
#define D *conf_edd
   E_CONFIG_VAL(D, T, indicator_always_portrait_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, indicator_always_landscape_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, indicator_quickpanel_portrait_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, indicator_quickpanel_landscape_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, indicator_apptray_portrait_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, indicator_apptray_landscape_region_ratio, DOUBLE);
   E_CONFIG_VAL(D, T, qp_scroll_with_visible_win, UCHAR);
   E_CONFIG_VAL(D, T, qp_scroll_with_clipping, UCHAR);
   E_CONFIG_VAL(D, T, dim_max_opacity, INT);
   E_CONFIG_VAL(D, T, dim_min_opacity, INT);
   E_CONFIG_VAL(D, T, flick_speed_limit, DOUBLE);
   E_CONFIG_VAL(D, T, animation_duration, DOUBLE);
   E_CONFIG_VAL(D, T, event_log, UCHAR);
   E_CONFIG_VAL(D, T, event_log_count, INT);
   E_CONFIG_VAL(D, T, elm_indicator_mode, UCHAR);
   // indicator widget gemometry setting
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].x, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].y, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].w, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].h, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].x, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].y, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].w, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].h, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].x, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].y, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].w, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].h, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].x, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].y, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].w, INT);
   E_CONFIG_VAL(D, T, indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].h, INT);
}

EAPI Config *
e_mod_move_cfdata_config_new(void)
{
   Config *cfg;

   cfg = E_NEW(Config, 1);

   cfg->indicator_always_portrait_region_ratio = 0.10833;
   cfg->indicator_always_landscape_region_ratio = 0.0609;
   cfg->indicator_quickpanel_portrait_region_ratio = 0.8917;
   cfg->indicator_quickpanel_landscape_region_ratio = 0.9390;
   cfg->indicator_apptray_portrait_region_ratio = 1.0;
   cfg->indicator_apptray_landscape_region_ratio = 1.0;
   cfg->qp_scroll_with_visible_win = 1;
   cfg->qp_scroll_with_clipping = 1;
   cfg->dim_max_opacity = 200;
   cfg->dim_min_opacity = 0;
   cfg->flick_speed_limit = 200.0;
   cfg->animation_duration = 0.4;
   cfg->event_log = 1;
   cfg->event_log_count = 600;
   cfg->elm_indicator_mode = 1;
   // indicator widget gemometry setting
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].x = 0;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].y = 0;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].w = 720;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_0].h = 50;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].x = 0;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].y = 0;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].w = 50;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_90].h = 1280;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].x = 0;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].y = 1230;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].w = 720;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_180].h = 50;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].x = 670;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].y = 0;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].w = 50;
   cfg->indicator_widget_geometry[E_MOVE_INDICATOR_WIDGET_ANGLE_270].h = 1280;

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