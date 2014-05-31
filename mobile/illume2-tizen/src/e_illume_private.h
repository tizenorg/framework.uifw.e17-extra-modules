#ifndef E_ILLUME_PRIVATE_H
# define E_ILLUME_PRIVATE_H

# include "e_illume.h"
# include "e_illume_log.h"

/* define policy object type */
# define E_ILLUME_POLICY_TYPE 0xE0b200b

# define POL_NUM_OF_LAYER 10

/* define layer values here so we don't have to grep through code to change */
/* layer level 10 (450~) */
# define POL_QUICKPANEL_LAYER 450

/* layer level 9 (400~449) */
# define POL_NOTIFICATION_LAYER_HIGH 400

/* layer level 8 (350~399) */
# define POL_NOTIFICATION_LAYER_NORMAL 350

/* layer level 7 (300~349) */
# define POL_NOTIFICATION_LAYER 300
# define POL_INDICATOR_LAYER 300
# define POL_NOTIFICATION_LAYER_LOW 300

/* layer level 6 (250~299) */
# define POL_FULLSCREEN_LAYER 250

/* layer level 5 (200~249) */
# define POL_APPTRAY_LAYER 200

/* layer level 4 (150~199) */
# define POL_STATE_ABOVE_LAYER 150
# define POL_ACTIVATE_LAYER 150
# define POL_DIALOG_LAYER 150
# define POL_SPLASH_LAYER 150
# define POL_SOFTKEY_LAYER 150

/* layer level 3 (100~149) */
# define POL_CLIPBOARD_LAYER 100
# define POL_KEYBOARD_LAYER 100
# define POL_CONFORMANT_LAYER 100
# define POL_APP_LAYER 100
# define POL_HOME_LAYER 100

/* layer level 2 (50~99) */
# define POL_STATE_BELOW_LAYER 50

/* layer level 1 (0~49) */

/* external variable to store list of quickpanels */
extern Eina_List *_e_illume_qps;

/* external variable to store active config */
extern E_Illume_Config *_e_illume_cfg;

/* external variable to store module directory */
extern const char *_e_illume_mod_dir;

/* external event for policy changes */
extern int E_ILLUME_POLICY_EVENT_CHANGE;

#endif
