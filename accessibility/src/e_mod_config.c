
#include "e_accessibility_privates.h"
#include "e_mod_config.h"

/* local variables */
static E_Config_DD *_acc_conf_edd = NULL;
static E_Config_DD *_acc_conf_grab_edd = NULL;

static void _e_mod_accessibility_config_free(void);
static void _e_mod_accessibility_config_new(void);

/* external variables */
E_Accessibility_Config *_e_accessibility_cfg = NULL;

int
e_mod_accessibility_config_init(void)
{
   /* create config structure for zones */
   _acc_conf_grab_edd = E_CONFIG_DD_NEW("Accessibility_Config_Grab", E_Accessibility_Config_Grab);

#undef T
#undef D
#define T E_Accessibility_Config_Grab
#define D _acc_conf_grab_edd
   E_CONFIG_VAL(D, T, event_name, STR);
   E_CONFIG_VAL(D, T, num_finger, INT);
   E_CONFIG_VAL(D, T, desc, STR);

   /* create config structure for module */
   _acc_conf_edd = E_CONFIG_DD_NEW("Accessibility_Config", E_Accessibility_Config);

#undef T
#undef D
#define T E_Accessibility_Config
#define D _acc_conf_edd
   E_CONFIG_VAL(D, T, ZoomUI.scale_factor, DOUBLE);
   E_CONFIG_VAL(D, T, ZoomUI.scale_threshold, INT);
   E_CONFIG_VAL(D, T, ZoomUI.max_scale, DOUBLE);
   E_CONFIG_VAL(D, T, ZoomUI.min_scale, DOUBLE);
   E_CONFIG_VAL(D, T, ZoomUI.current_scale, DOUBLE);
   E_CONFIG_VAL(D, T, ZoomUI.width, INT);
   E_CONFIG_VAL(D, T, ZoomUI.height, INT);
   E_CONFIG_VAL(D, T, ZoomUI.offset_x, INT);
   E_CONFIG_VAL(D, T, ZoomUI.offset_y, INT);
   E_CONFIG_VAL(D, T, ZoomUI.isZoomUIEnabled, UCHAR);
   E_CONFIG_LIST(D, T, ZoomUI.grabs, _acc_conf_grab_edd);
   E_CONFIG_VAL(D, T, HighContrast.HighContrastMode, INT);
   E_CONFIG_VAL(D, T, PowerSaving.PowerSavingMode, INT);
   E_CONFIG_VAL(D, T, DarkScreen.DarkScreenMode, INT);

   /* attempt to load existing configuration */
   _e_accessibility_cfg = e_config_domain_load(E_ACCESSIBILITY_CFG, _acc_conf_edd);

   /* create new config if we need to */
   if (!_e_accessibility_cfg)
     {
        _e_mod_accessibility_config_new();
        e_mod_accessibility_config_save();
        fprintf(stderr, "[e_acc][config] Config file for e_accessibility was made/stored !\n");
     }
   else
     {
        fprintf(stderr, "[e_acc][config] Config file for e_accessibility was loaded successfully !\n");
     }

   return 1;
}

int
e_mod_accessibility_config_shutdown(void)
{
   /* free config structure */
   _e_mod_accessibility_config_free();

   /* free data descriptors */
   E_CONFIG_DD_FREE(_acc_conf_grab_edd);
   E_CONFIG_DD_FREE(_acc_conf_edd);

   return 1;
}

int
e_mod_accessibility_config_save(void)
{
   return e_config_domain_save(E_ACCESSIBILITY_CFG, _acc_conf_edd, _e_accessibility_cfg);
}

/* local functions */
static void
_e_mod_accessibility_config_free(void)
{
   Eina_List *l;
   E_Accessibility_Config_Grab *data;

   /* check for config */
   if (!_e_accessibility_cfg) return;

   /* cleanup any stringshares */
   EINA_LIST_FOREACH(_e_accessibility_cfg->ZoomUI.grabs, l, data)
     {
        if (data)
          {
             eina_stringshare_del(data->event_name);
             eina_stringshare_del(data->desc);
          }
     }
   /* free configured grabs */
   eina_list_free(_e_accessibility_cfg->ZoomUI.grabs);

   /* free config structure */
   E_FREE(_e_accessibility_cfg);
}

static void
_e_mod_accessibility_config_new(void)
{
   E_Accessibility_Config_Grab *cgz_2_tap;

   int screenWidth = 0;
   int screenHeight = 0;
   ecore_x_screen_size_get(ecore_x_default_screen_get(),
                           &screenWidth, &screenHeight);

   if( !screenWidth && !screenHeight )
     {
        screenWidth = DEFAULT_SCREEN_WIDTH;
        screenHeight = DEFAULT_SCREEN_HEIGHT;
     }

   /* create initial config */
   _e_accessibility_cfg = E_NEW(E_Accessibility_Config, 1);
   _e_accessibility_cfg->ZoomUI.scale_factor = 0.25;
   _e_accessibility_cfg->ZoomUI.scale_threshold = 3;
   _e_accessibility_cfg->ZoomUI.max_scale = 7.0;
   _e_accessibility_cfg->ZoomUI.min_scale = 1.2;
   _e_accessibility_cfg->ZoomUI.current_scale = 2.0;
   _e_accessibility_cfg->ZoomUI.width = (int)(screenWidth / _e_accessibility_cfg->ZoomUI.current_scale);
   _e_accessibility_cfg->ZoomUI.height = (int)(screenHeight / _e_accessibility_cfg->ZoomUI.current_scale);
   _e_accessibility_cfg->ZoomUI.offset_x = 0;
   _e_accessibility_cfg->ZoomUI.offset_y = 0;
   _e_accessibility_cfg->ZoomUI.isZoomUIEnabled = EINA_FALSE;
   _e_accessibility_cfg->HighContrast.HighContrastMode = 0;
   _e_accessibility_cfg->PowerSaving.PowerSavingMode = 0;
   _e_accessibility_cfg->DarkScreen.DarkScreenMode = 0;

   /* create config for initial grab */
   /* add grabs config to main config structure */
   cgz_2_tap = E_NEW(E_Accessibility_Config_Grab, 1);

   cgz_2_tap->num_finger = 2;
   cgz_2_tap->event_name = eina_stringshare_add("tap");
   cgz_2_tap->desc = eina_stringshare_add("2 finger tap event for zooming in/out");

   _e_accessibility_cfg->ZoomUI.grabs = eina_list_append(_e_accessibility_cfg->ZoomUI.grabs, cgz_2_tap);
}

