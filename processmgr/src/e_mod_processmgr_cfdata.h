#ifndef E_MOD_PROCESSMGR_CFDATA_H
#define E_MOD_PROCESSMGR_CFDATA_H

typedef struct _Config Config;

struct _Config
{
   unsigned char enable; // 1: module enable  0: module disable 
   unsigned int tizen_anr_trigger_timeout; /* After input event, we will wait 1 second for triggering ping timer.  unit:sec*/
   unsigned int tizen_anr_confirm_timeout; /* After ping message, we will wait 5 seconds for checking deadlock status. unit:sec*/
   unsigned int tizen_anr_ping_interval_timeout; /* Once it sent ping then it will not be sent during 15 seconds. unit:ms*/
};

EAPI void    e_mod_processmgr_cfdata_edd_init(E_Config_DD **conf_edd);
EAPI Config *e_mod_processmgr_cfdata_config_new(void);
EAPI void    e_mod_processmgr_cfdata_config_free(Config *cfg);

#endif
