#ifndef E_MOD_MOVE_CFDATA_H
#define E_MOD_MOVE_CFDATA_H

typedef struct _Config Config;

struct _Config
{
   double        indicator_always_portrait_region_ratio; // indicator's  portrait always region ratio
   double        indicator_always_landscape_region_ratio; // indicator's landscape always region ratio
   double        indicator_quickpanel_portrait_region_ratio; // indicator's  portrait quickpanel region ratio
   double        indicator_quickpanel_landscape_region_ratio; // indicator's landscape quickpanel region ratio
   double        indicator_apptray_portrait_region_ratio; // indicator's  portrait apptray region ratio
   double        indicator_apptray_landscape_region_ratio; // indicator's landscape apptray region ratio
   unsigned char qp_scroll_with_visible_win; // 1: quickpanel scroll with visible window 0: qp scroll only
   unsigned char qp_scroll_with_clipping; // 1: quickpanel scroll with clipping 0: qp scroll without clipping
   int           dim_max_opacity; // dim max opacity
   int           dim_min_opacity; // dim min opacity

   struct {
      double speed;
      double angle;
      double distance;
      double distance_rate;
   } flick_limit; // indicator / quickpanel / apptray flick limitation

   double        animation_duration; // apptray / quickpanel move animation duration
   unsigned char event_log; // 1 :ecore & evas_object debug event logging  0: do not log event
   int           event_log_count; // ecore & evas_object debug event logging count
   unsigned char elm_indicator_mode; // 1: indicator widget mode / 0: indicator window mode

   struct {
      int x;
      int y;
      int w;
      int h;
   } indicator_widget_geometry[4]; //indicator widget's per angle geometry. [0]: angle 0, [1]: angle 90, [2]: angle 180, [3]: angle 270

   struct {
      int x;
      int y;
      int w;
      int h;
   } mini_apptray_widget_geometry[4]; //mini_apptray widget's per angle geometry. [0]: angle 0, [1]: angle 90, [2]: angle 180, [3]: angle 270
};

EAPI void    e_mod_move_cfdata_edd_init(E_Config_DD **conf_edd);
EAPI Config *e_mod_move_cfdata_config_new(void);
EAPI void    e_mod_move_cfdata_config_free(Config *cfg);

#endif
