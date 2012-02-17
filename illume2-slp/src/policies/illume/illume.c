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
   p->funcs.zone_layout = _policy_zone_layout;
   p->funcs.zone_move_resize = _policy_zone_move_resize;
   p->funcs.zone_mode_change = _policy_zone_mode_change;
   p->funcs.zone_close = _policy_zone_close;
   p->funcs.drag_start = _policy_drag_start;
   p->funcs.drag_end = _policy_drag_end;
   p->funcs.focus_back = _policy_focus_back;
   p->funcs.focus_forward = _policy_focus_forward;
   p->funcs.property_change = _policy_property_change;
   p->funcs.resize_start = _policy_resize_start;
   p->funcs.resize_end = _policy_resize_end;

   p->funcs.window_focus_in = _policy_window_focus_in;

   p->funcs.border_restack_request = _policy_border_stack_change;
   p->funcs.border_stack = _policy_border_stack;

   p->funcs.border_post_new_border = _policy_border_post_new_border;
   p->funcs.border_pre_fetch = _policy_border_pre_fetch;
   p->funcs.border_new_border = _policy_border_new_border;

   p->funcs.window_configure_request = _policy_window_configure_request;

   /* for visibility */
   p->funcs.window_create = _policy_window_create;
   p->funcs.window_destroy = _policy_window_destroy;
   p->funcs.window_reparent = _policy_window_reparent;
   p->funcs.window_show = _policy_window_show;
   p->funcs.window_hide = _policy_window_hide;
   p->funcs.window_configure = _policy_window_configure;

   p->funcs.window_sync_draw_done = _policy_window_sync_draw_done;
   p->funcs.quickpanel_state_change = _policy_quickpanel_state_change;

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
   p->funcs.zone_layout = NULL;
   p->funcs.zone_move_resize = NULL;
   p->funcs.zone_mode_change = NULL;
   p->funcs.zone_close = NULL;
   p->funcs.drag_start = NULL;
   p->funcs.drag_end = NULL;
   p->funcs.focus_back = NULL;
   p->funcs.focus_forward = NULL;
   p->funcs.property_change = NULL;
   p->funcs.resize_start = NULL;
   p->funcs.resize_end = NULL;

   p->funcs.window_focus_in = NULL;

   p->funcs.border_restack_request = NULL;
   p->funcs.border_stack = NULL;

   p->funcs.border_post_new_border = NULL;
   p->funcs.border_pre_fetch = NULL;
   p->funcs.border_new_border = NULL;

   p->funcs.window_configure_request = NULL;

   /* for visibility */
   p->funcs.window_create = NULL;
   p->funcs.window_destroy = NULL;
   p->funcs.window_reparent = NULL;
   p->funcs.window_show = NULL;
   p->funcs.window_hide = NULL;
   p->funcs.window_configure = NULL;

   p->funcs.window_sync_draw_done = NULL;
   p->funcs.quickpanel_state_change = NULL;

   _policy_fin();
   return 1;
}
