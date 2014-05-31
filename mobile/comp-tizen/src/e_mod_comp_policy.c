#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_policy.h"

/* local subsystem functions */

/* local subsystem globals */
static Eina_Hash *shadow_hash = NULL;

/* externally accessible functions */
EINTERN int
e_mod_comp_policy_init(void)
{
   if (!shadow_hash) shadow_hash = eina_hash_string_superfast_new(NULL);

   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_UNKNOWN),        "shadow"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DESKTOP),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DOCK),           "shadow_fade"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TOOLBAR),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MENU),           "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_UTILITY),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_SPLASH),         "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DIALOG),         "dialog"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_NORMAL),         "shadow_fade"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DROPDOWN_MENU),  "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_POPUP_MENU),     "shadow"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TOOLTIP),        "shadow"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_NOTIFICATION),   "dialog"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_COMBO),          "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DND),            "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MENUSCREEN),     "home_screen"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_QUICKPANEL_BASE),"quickpanel"        );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_QUICKPANEL),     "quickpanel"        );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TASKMANAGER),    "taskmgr"           );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_LIVEMAGAZINE),   "home_screen"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_LOCKSCREEN),     "lockscreen"        );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_INDICATOR),      "indicator"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TICKERNOTI),     "dialog_without_dim");
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DEBUGGING_INFO), "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_APPTRAY),        "app_tray"          );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MINI_APPTRAY),   "app_tray"          );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_VOLUME),         "dialog_without_dim");
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_BACKGROUND),     "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_ISF_KEYBOARD),   "keyboard"          );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_ISF_SUB),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_SETUP_WIZARD),   "no-effect"         );

   return 1;
}

EINTERN int
e_mod_comp_policy_shutdown(void)
{
   if (shadow_hash) eina_hash_free(shadow_hash);
   shadow_hash = NULL;
   return 1;
}

EINTERN Eina_Bool
e_mod_comp_policy_app_launch_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(cw->bd->zone, 0);
   if (TYPE_NORMAL_CHECK(cw) && REGION_EQUAL_TO_ZONE(cw, cw->bd->zone))
     return EINA_TRUE;
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_policy_app_close_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(cw->bd->zone, 0);
   if (TYPE_NORMAL_CHECK(cw) && REGION_EQUAL_TO_ZONE(cw, cw->bd->zone))
     return EINA_TRUE;
   return EINA_FALSE;
}

EINTERN char *
e_mod_comp_policy_win_shadow_group_get(E_Comp_Win *cw)
{
   E_Comp_Win_Type type;
   E_CHECK_RETURN(cw, 0);
   type = e_mod_comp_win_type_get(cw);
   return eina_hash_find(shadow_hash, e_util_winid_str_get(type));
}

/* when receiving border show event for the home window,
 * check before running app closing effect for given normal window.
 * exceptional windows: lock, setup wizard
 * otherwise: do app closing effect
 */
EINTERN Eina_Bool
e_mod_comp_policy_home_app_win_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, EINA_FALSE);

   if (TYPE_LOCKSCREEN_CHECK(cw) ||
       TYPE_SETUP_WIZARD_CHECK(cw))
     {
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/* local subsystem functions */
