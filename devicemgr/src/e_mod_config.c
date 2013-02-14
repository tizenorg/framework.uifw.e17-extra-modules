
#include "e_devicemgr_privates.h"
#include "e_mod_config.h"
#include "scrnconf_devicemgr.h"

/* local variables */
static E_Config_DD *_devicemgr_conf_edd = NULL;

static void _e_mod_devicemgr_config_free(void);
static void _e_mod_devicemgr_config_new(void);

/* external variables */
E_Devicemgr_Config *_e_devicemgr_cfg = NULL;

int
e_mod_devicemgr_config_init(void)
{
   /* create config structure for module */
   _devicemgr_conf_edd = E_CONFIG_DD_NEW("Devicemgr_Config", E_Devicemgr_Config);

#undef T
#undef D
#define T E_Devicemgr_Config
#define D _devicemgr_conf_edd
   E_CONFIG_VAL(D, T, ScrnConf.enable, UCHAR);
   E_CONFIG_VAL(D, T, ScrnConf.default_dispmode, INT);
   E_CONFIG_VAL(D, T, ScrnConf.isPopUpEnabled, UCHAR);

   /* attempt to load existing configuration */
   _e_devicemgr_cfg = e_config_domain_load(E_DEVICEMGR_CFG, _devicemgr_conf_edd);

   /* create new config if we need to */
   if (!_e_devicemgr_cfg)
     {
        _e_mod_devicemgr_config_new();
        e_mod_devicemgr_config_save();
        fprintf(stderr, "[e_devicemgr][config] Config file for e_devicemgr was made/stored !\n");
     }
   else
     {
        fprintf(stderr, "[e_devicemgr][config] Config file for e_devicemgr was loaded successfully !\n");
     }

   return 1;
}

int
e_mod_devicemgr_config_shutdown(void)
{
   /* free config structure */
   _e_mod_devicemgr_config_free();

   /* free data descriptors */
   E_CONFIG_DD_FREE(_devicemgr_conf_edd);

   return 1;
}

int
e_mod_devicemgr_config_save(void)
{
   return e_config_domain_save(E_DEVICEMGR_CFG, _devicemgr_conf_edd, _e_devicemgr_cfg);
}

/* local functions */
static void
_e_mod_devicemgr_config_free(void)
{
   /* check for config */
   if (!_e_devicemgr_cfg) return;

   /* free config structure */
   E_FREE(_e_devicemgr_cfg);
}

static void
_e_mod_devicemgr_config_new(void)
{
   /* create initial config */
   _e_devicemgr_cfg = E_NEW(E_Devicemgr_Config, 1);
   _e_devicemgr_cfg->ScrnConf.enable = EINA_TRUE;
   _e_devicemgr_cfg->ScrnConf.default_dispmode = UTILX_SCRNCONF_DISPMODE_CLONE;
   _e_devicemgr_cfg->ScrnConf.isPopUpEnabled = EINA_FALSE;
}

