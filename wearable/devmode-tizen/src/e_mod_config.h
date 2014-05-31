#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#include "e_mod_devmode.h"

typedef enum _drawing_option
{
   DRAW_ALL,
} drawing_option;

typedef struct _Config Config;

struct _Config
{
   int drawing_option;
};

EAPI void    e_mod_devmode_cfdata_edd_init(E_Config_DD **conf_edd);
EAPI Config *e_mod_devmode_cfdata_config_new(void);
EAPI void    e_mod_devmode_cfdata_config_free(Config *cfg);

#endif
