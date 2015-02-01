#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_POLICY_H
#define E_MOD_COMP_POLICY_H

EAPI int       e_mod_comp_policy_init(void);
EAPI int       e_mod_comp_policy_shutdown(void);
EAPI Eina_Bool e_mod_comp_policy_app_launch_check(E_Comp_Win *cw);
EAPI Eina_Bool e_mod_comp_policy_app_close_check(E_Comp_Win *cw);
EAPI char     *e_mod_comp_policy_win_shadow_group_get(E_Comp_Win *cw);
EAPI Eina_Bool e_mod_comp_policy_home_app_win_check(E_Comp_Win *cw);

#endif
#endif
