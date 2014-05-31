#include "e_illume.h"
#include "illume.h"
#include "policy.h"

EAPI E_Illume_Policy_Api e_illume_policy_api =
{
   /* version, name, label */
   E_ILLUME_POLICY_API_VERSION, "illume", "Illume"
};

EAPI int
e_illume_policy_init(E_Illume_Policy *p)
{
   /* tell the policy what functions we support */
   p->funcs.border_add = _policy_border_add;
   p->funcs.border_del = _policy_border_del;
   p->funcs.border_focus_in = _policy_border_focus_in;
   p->funcs.border_focus_out = _policy_border_focus_out;
   p->funcs.border_activate = _policy_border_activate;
   p->funcs.border_post_fetch = _policy_border_post_fetch;
   p->funcs.border_post_assign = _policy_border_post_assign;
   p->funcs.border_show = _policy_border_show;
   p->funcs.border_hide = _policy_border_cb_hide;
   p->funcs.border_move = _policy_border_cb_move;
   p->funcs.border_resize = _policy_border_cb_resize;
   p->funcs.border_comp_src_visibility = _policy_border_cb_comp_src_visibility;
   p->funcs.border_rotation_change_begin = _policy_border_cb_rotation_change_begin;
   p->funcs.border_rotation_change_end = _policy_border_cb_rotation_change_end;
   p->funcs.zone_layout = _policy_zone_layout;
   p->funcs.zone_move_resize = _policy_zone_move_resize;
   p->funcs.zone_mode_change = _policy_zone_mode_change;
   p->funcs.zone_rotation_change_begin = _policy_zone_rotation_change_begin;
   p->funcs.zone_close = _policy_zone_close;
   p->funcs.drag_start = _policy_drag_start;
   p->funcs.drag_end = _policy_drag_end;
   p->funcs.focus_back = _policy_focus_back;
   p->funcs.focus_forward = _policy_focus_forward;
   p->funcs.property_change = _policy_property_change;

   p->funcs.window_focus_in = _policy_window_focus_in;

   p->funcs.border_restack_request = _policy_border_stack_change;
   p->funcs.border_stack = _policy_border_stack;
   p->funcs.border_zone_set = _policy_border_zone_set;

   p->funcs.border_post_new_border = _policy_border_post_new_border;
   p->funcs.border_pre_fetch = _policy_border_pre_fetch;
   p->funcs.border_new_border = _policy_border_new_border;
   p->funcs.border_eval_end = _policy_border_eval_end;
#ifdef _F_BORDER_HOOK_PATCH_
   p->funcs.border_del_border = _policy_border_del_border;
#endif

   p->funcs.window_configure_request = _policy_window_configure_request;

   p->funcs.border_iconify_cb = _policy_border_iconify_cb;
   p->funcs.border_uniconify_cb = _policy_border_uniconify_cb;

   /* for visibility */
   p->funcs.window_create = _policy_window_create;
   p->funcs.window_destroy = _policy_window_destroy;
   p->funcs.window_reparent = _policy_window_reparent;
   p->funcs.window_show = _policy_window_show;
   p->funcs.window_hide = _policy_window_hide;
   p->funcs.window_configure = _policy_window_configure;

   p->funcs.window_sync_draw_done = _policy_window_sync_draw_done;
   p->funcs.quickpanel_state_change = _policy_quickpanel_state_change;

   p->funcs.window_desk_set = _policy_window_desk_set;

   p->funcs.window_move_resize_request = _policy_window_move_resize_request;
   p->funcs.window_state_request = _policy_window_state_request;

   p->funcs.module_update = _policy_module_update;

   p->funcs.idle_enterer = _policy_idle_enterer;

   p->funcs.illume_win_state_change_request = _policy_illume_win_state_change_request;
   p->funcs.illume_internal = _policy_illume_internal_client_message;

   if (!_policy_init())
     return 0;

   return 1;
}

EAPI int
e_illume_policy_shutdown(E_Illume_Policy *p)
{
   p->funcs.border_add = NULL;
   p->funcs.border_del = NULL;
   p->funcs.border_focus_in = NULL;
   p->funcs.border_focus_out = NULL;
   p->funcs.border_activate = NULL;
   p->funcs.border_post_fetch = NULL;
   p->funcs.border_post_assign = NULL;
   p->funcs.border_show = NULL;
   p->funcs.border_hide = NULL;
   p->funcs.border_move = NULL;
   p->funcs.border_resize = NULL;
   p->funcs.border_comp_src_visibility = NULL;
   p->funcs.border_rotation_change_begin = NULL;
   p->funcs.border_rotation_change_end = NULL;
   p->funcs.zone_layout = NULL;
   p->funcs.zone_move_resize = NULL;
   p->funcs.zone_mode_change = NULL;
   p->funcs.zone_rotation_change_begin = NULL;
   p->funcs.zone_close = NULL;
   p->funcs.drag_start = NULL;
   p->funcs.drag_end = NULL;
   p->funcs.focus_back = NULL;
   p->funcs.focus_forward = NULL;
   p->funcs.property_change = NULL;

   p->funcs.window_focus_in = NULL;

   p->funcs.border_restack_request = NULL;
   p->funcs.border_stack = NULL;
   p->funcs.border_zone_set = NULL;

   p->funcs.border_post_new_border = NULL;
   p->funcs.border_pre_fetch = NULL;
   p->funcs.border_new_border = NULL;
   p->funcs.border_eval_end = NULL;
#ifdef _F_BORDER_HOOK_PATCH_
   p->funcs.border_del_border = NULL;
#endif
   p->funcs.window_configure_request = NULL;

   p->funcs.border_iconify_cb = NULL;
   p->funcs.border_uniconify_cb = NULL;

   /* for visibility */
   p->funcs.window_create = NULL;
   p->funcs.window_destroy = NULL;
   p->funcs.window_reparent = NULL;
   p->funcs.window_show = NULL;
   p->funcs.window_hide = NULL;
   p->funcs.window_configure = NULL;

   p->funcs.window_sync_draw_done = NULL;
   p->funcs.quickpanel_state_change = NULL;

   p->funcs.window_desk_set = NULL;

   p->funcs.window_move_resize_request = NULL;
   p->funcs.window_state_request = NULL;

   p->funcs.module_update = NULL;

   p->funcs.idle_enterer = NULL;

   p->funcs.illume_win_state_change_request = NULL;
   p->funcs.illume_internal = NULL;

   _policy_fin();
   return 1;
}
