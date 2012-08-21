#ifndef E_MOD_MOVE_CFDATA_H
#define E_MOD_MOVE_CFDATA_H

typedef struct _Config Config;

struct _Config
{
   double        indicator_home_region_ratio; // indicator's home button region ratio
   unsigned char qp_scroll_with_visible_win; // 1: quickpanel scroll with visible window 0: qp scroll only
   int           dim_max_opacity; // dim max opacity
   int           dim_min_opacity; // dim min opacity
   double        flick_speed_limit; // flick speed limitation
   double        animation_duration; // apptray / quickpanel move animation duration
   unsigned char event_log; // 1 :ecore & evas_object debug event logging  0: do not log event
   int           event_log_count; // ecore & evas_object debug event logging count
};

EAPI void    e_mod_move_cfdata_edd_init(E_Config_DD **conf_edd);
EAPI Config *e_mod_move_cfdata_config_new(void);
EAPI void    e_mod_move_cfdata_config_free(Config *cfg);

#endif
