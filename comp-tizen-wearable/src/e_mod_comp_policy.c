#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_policy.h"

/* local subsystem functions */

/* local subsystem globals */
static Eina_Hash *shadow_hash = NULL;

/* externally accessible functions */
EAPI int
e_mod_comp_policy_init(void)
{
   if (!shadow_hash) shadow_hash = eina_hash_string_superfast_new(NULL);

   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_UNKNOWN), _comp_mod->conf->effect_policy_unknown);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DESKTOP), _comp_mod->conf->effect_policy_desktop);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DOCK), _comp_mod->conf->effect_policy_dock);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TOOLBAR), _comp_mod->conf->effect_policy_toolbar);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MENU), _comp_mod->conf->effect_policy_menu);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_UTILITY), _comp_mod->conf->effect_policy_utility);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_SPLASH), _comp_mod->conf->effect_policy_splash);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DIALOG), _comp_mod->conf->effect_policy_dialog);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_NORMAL), _comp_mod->conf->effect_policy_normal);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_VIDEOCALL), _comp_mod->conf->effect_policy_videocall);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DROPDOWN_MENU), _comp_mod->conf->effect_policy_dropdown_menu);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_POPUP_MENU), _comp_mod->conf->effect_policy_popup_menu);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TOOLTIP), _comp_mod->conf->effect_policy_tooltip);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_NOTIFICATION), _comp_mod->conf->effect_policy_notification);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_COMBO), _comp_mod->conf->effect_policy_combo);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DND), _comp_mod->conf->effect_policy_dnd);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MENUSCREEN), _comp_mod->conf->effect_policy_menuscreen);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_QUICKPANEL_BASE), _comp_mod->conf->effect_policy_quickpanel_base);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_QUICKPANEL), _comp_mod->conf->effect_policy_quickpanel);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TASKMANAGER), _comp_mod->conf->effect_policy_taskmanager);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_LIVEMAGAZINE), _comp_mod->conf->effect_policy_livemagazine);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_LOCKSCREEN), _comp_mod->conf->effect_policy_lockscreen);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_INDICATOR), _comp_mod->conf->effect_policy_indicator);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TICKERNOTI), _comp_mod->conf->effect_policy_tickernoti);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DEBUGGING_INFO), _comp_mod->conf->effect_policy_debugging_info);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_APPTRAY), _comp_mod->conf->effect_policy_apptray);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MINI_APPTRAY), _comp_mod->conf->effect_policy_mini_apptray);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_VOLUME), _comp_mod->conf->effect_policy_volume);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_BACKGROUND), _comp_mod->conf->effect_policy_background);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_ISF_KEYBOARD), _comp_mod->conf->effect_policy_isf_keyboard);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_ISF_SUB), _comp_mod->conf->effect_policy_isf_sub);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_SETUP_WIZARD), _comp_mod->conf->effect_policy_setup_wizard);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_APP_POPUP), _comp_mod->conf->effect_policy_app_popup);
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_APP_CALLSCREEN), _comp_mod->conf->effect_policy_app_callscreen);

   return 1;
}

EAPI int
e_mod_comp_policy_shutdown(void)
{
   if (shadow_hash) eina_hash_free(shadow_hash);
   shadow_hash = NULL;
   return 1;
}

EAPI Eina_Bool
e_mod_comp_policy_app_launch_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(cw->bd->zone, 0);
   if (TYPE_NORMAL_CHECK(cw) || TYPE_CALL_SCREEN_CHECK(cw))
     {
        if (REGION_EQUAL_TO_ZONE(cw, cw->bd->zone))
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

EAPI Eina_Bool
e_mod_comp_policy_app_close_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(cw->bd->zone, 0);
   if (TYPE_NORMAL_CHECK(cw) || TYPE_CALL_SCREEN_CHECK(cw))
     {
        if (REGION_EQUAL_TO_ZONE(cw, cw->bd->zone))
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

EAPI char *
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
EAPI Eina_Bool
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
