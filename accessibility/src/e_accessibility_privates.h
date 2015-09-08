
#ifndef _E_ACCESSIBILITY_PRIVATE_H_
#define _E_ACCESSIBILITY_PRIVATE_H_

#include "e.h"

#define E_ACCESSIBILITY_CFG_PATH       "/home/apps/.e/e/config/samsung/"
#define E_ACCESSIBILITY_CFG       "module.accessibility-tizen"

#define DEFAULT_SCREEN_WIDTH       720
#define DEFAULT_SCREEN_HEIGHT       1280

typedef struct _E_Accessibility_Config E_Accessibility_Config;
typedef struct _E_Accessibility_Config_Grab E_Accessibility_Config_Grab;

/* external variable to store active config */
extern E_Accessibility_Config *_e_accessibility_cfg;

/**
 * @brief structure for Accessibility configuration.
 *
 * @ingroup E_Accessibility_Config_Group
 */
struct _E_Accessibility_Config
{
   struct
     {
        double scale_factor;
        /**< scale factor */
        int scale_threshold;
        /**< scale threshold */
        double max_scale;
        /**< maximun scale factor */
        double min_scale;
        /**< minimum scale factor */
        double current_scale;
        /**< current scale factor */

        int width;
        /**< width to be magnified */
        int height;
        /**< height to be magnified */
        int offset_x;
        /**< top left x coordinates of magnified area  */
        int offset_y;
        /**< top left y coordinates of magnified area  */
        Eina_Bool isZoomUIEnabled;
        /**< ZoomUI enable/disable status */
        Eina_List *grabs;
        /** < gesture event list to be grabbed */
     } ZoomUI;

   struct
     {
        int HighContrastMode;
        /**< HighContrast enable/disable status */
     } HighContrast;

   struct
     {
        int PowerSavingMode;
     } PowerSaving;

   struct
     {
        int DarkScreenMode;
     } DarkScreen;

   struct
   	{
   		Eina_Bool isPalmGestureEnabled;
		/**< Palm gesture enable/disable status */
		Eina_List *grabs;
		/** < gesture event list to be grabbed */
	} PalmGesture;
};

/**
 * @brief structure for Accessibility Grab configuration.
 *
 * @ingroup E_Accessibility_Config_Group
 */
struct _E_Accessibility_Config_Grab
{
   const char *event_name;
   /**< gesture event name what you want to grab  */
   int num_finger;
   /**< integer specifying number of finger what you're interested in */
   const char *desc;
   /**< purpose of grabbing event  */
};

#endif//_E_ACCESSIBILITY_PRIVATE_H_

