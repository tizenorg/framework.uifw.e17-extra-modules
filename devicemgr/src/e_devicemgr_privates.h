
#ifndef _E_DEVICEMGR_PRIVATE_H_
#define _E_DEVICEMGR_PRIVATE_H_

#include "e.h"

#define E_DEVICEMGR_CFG_PATH  "/opt/share/.e/e/config/samsung/"
#define E_DEVICEMGR_CFG       "module.devicemgr-slp"

/* atoms */
#define STR_ATOM_DEVICEMGR_CFG "_DEVICEMGR_CFG"

typedef enum
{
   DEVICEMGR_CFG_POPUP,
   DEVICEMGR_CFG_DEFAULT_DISPMODE,
} DEVICEMGR_CFG;

typedef struct _E_Devicemgr_Config E_Devicemgr_Config;

/* external variable to store active config */
extern E_Devicemgr_Config *_e_devicemgr_cfg;

/**
 * @brief structure for Devicemgr configuration.
 *
 * @ingroup E_Devicemgr_Config_Group
 */
struct _E_Devicemgr_Config
{
   struct
     {
        int enable;
        /**< enable screen configuration */
        int default_dispmode;
        /**< default display mode */
        Eina_Bool isPopUpEnabled;
        /**< popup enable/disable status */
     } ScrnConf;
};

#endif//_E_DEVICEMGR_PRIVATE_H_

