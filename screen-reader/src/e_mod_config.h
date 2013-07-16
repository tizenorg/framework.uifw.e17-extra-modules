#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

typedef struct _Config Config;

struct _Config
{
   int three_finger_swipe_timeout;
};

EAPI void    e_mod_screen_reader_cfdata_edd_init(E_Config_DD **conf_edd);
EAPI Config *e_mod_screen_reader_cfdata_config_new(void);
EAPI void    e_mod_screen_reader_cfdata_config_free(Config *cfg);

#endif
