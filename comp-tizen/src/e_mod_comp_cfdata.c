#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_cfdata.h"

EAPI void
e_mod_comp_cfdata_edd_init(E_Config_DD **conf_edd, E_Config_DD **match_edd)
{
   *match_edd = E_CONFIG_DD_NEW("Comp_Match", Match);
#undef T
#undef D
#define T Match
#define D *match_edd
   E_CONFIG_VAL(D, T, title, STR);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, clas, STR);
   E_CONFIG_VAL(D, T, role, STR);
   E_CONFIG_VAL(D, T, primary_type, INT);
   E_CONFIG_VAL(D, T, borderless, CHAR);
   E_CONFIG_VAL(D, T, dialog, CHAR);
   E_CONFIG_VAL(D, T, accepts_focus, CHAR);
   E_CONFIG_VAL(D, T, vkbd, CHAR);
   E_CONFIG_VAL(D, T, quickpanel, CHAR);
   E_CONFIG_VAL(D, T, argb, CHAR);
   E_CONFIG_VAL(D, T, fullscreen, CHAR);
   E_CONFIG_VAL(D, T, modal, CHAR);
   E_CONFIG_VAL(D, T, shadow_style, STR);

   *conf_edd = E_CONFIG_DD_NEW("Comp_Config", Config);
#undef T
#undef D
#define T Config
#define D *conf_edd
   E_CONFIG_VAL(D, T, shadow_file, STR);
   E_CONFIG_VAL(D, T, shadow_style, STR);
   E_CONFIG_VAL(D, T, effect_file, STR);
   E_CONFIG_VAL(D, T, engine, INT);
   E_CONFIG_VAL(D, T, max_unmapped_pixels, INT);
   E_CONFIG_VAL(D, T, max_unmapped_time, INT);
   E_CONFIG_VAL(D, T, min_unmapped_time, INT);
   E_CONFIG_VAL(D, T, fps_average_range, INT);
   E_CONFIG_VAL(D, T, fps_corner, UCHAR);
   E_CONFIG_VAL(D, T, fps_show, UCHAR);
   E_CONFIG_VAL(D, T, use_shadow, UCHAR);
   E_CONFIG_VAL(D, T, indirect, UCHAR);
   E_CONFIG_VAL(D, T, texture_from_pixmap, UCHAR);
   E_CONFIG_VAL(D, T, lock_fps, UCHAR);
   E_CONFIG_VAL(D, T, loose_sync, UCHAR);
   E_CONFIG_VAL(D, T, efl_sync, UCHAR);
   E_CONFIG_VAL(D, T, grab, UCHAR);
   E_CONFIG_VAL(D, T, vsync, UCHAR);
   E_CONFIG_VAL(D, T, keep_unmapped, UCHAR);
   E_CONFIG_VAL(D, T, send_flush, UCHAR);
   E_CONFIG_VAL(D, T, send_dump, UCHAR);
   E_CONFIG_VAL(D, T, nocomp_fs, UCHAR);
   E_CONFIG_VAL(D, T, use_hwc, UCHAR);
   E_CONFIG_VAL(D, T, smooth_windows, UCHAR);
   E_CONFIG_VAL(D, T, first_draw_delay, DOUBLE);
   E_CONFIG_VAL(D, T, canvas_per_zone, UCHAR);
   E_CONFIG_VAL(D, T, use_lock_screen, UCHAR);
   E_CONFIG_VAL(D, T, default_window_effect, UCHAR);
   E_CONFIG_VAL(D, T, keyboard_effect, UCHAR);
   E_CONFIG_VAL(D, T, xv_rotation_effect, UCHAR);
   E_CONFIG_VAL(D, T, fake_image_launch, UCHAR);
   E_CONFIG_VAL(D, T, fake_launch_layer, INT);
   E_CONFIG_VAL(D, T, lower_layer, INT);
   E_CONFIG_VAL(D, T, defer_raise_effect, UCHAR);
   E_CONFIG_VAL(D, T, max_lock_screen_time, DOUBLE);
   E_CONFIG_VAL(D, T, damage_timeout, DOUBLE);
   E_CONFIG_VAL(D, T, nocomp_begin_timeout, DOUBLE);
   E_CONFIG_VAL(D, T, use_hw_ov, UCHAR);
   E_CONFIG_VAL(D, T, debug_info_show, UCHAR);
   E_CONFIG_VAL(D, T, max_debug_msgs, INT);
   E_CONFIG_VAL(D, T, debug_type_nocomp, UCHAR);
   E_CONFIG_VAL(D, T, debug_type_swap, UCHAR);
   E_CONFIG_VAL(D, T, debug_type_effect, UCHAR);
   E_CONFIG_VAL(D, T, effect_policy_unknown, STR);
   E_CONFIG_VAL(D, T, effect_policy_desktop, STR);
   E_CONFIG_VAL(D, T, effect_policy_dock, STR);
   E_CONFIG_VAL(D, T, effect_policy_toolbar, STR);
   E_CONFIG_VAL(D, T, effect_policy_menu, STR);
   E_CONFIG_VAL(D, T, effect_policy_utility, STR);
   E_CONFIG_VAL(D, T, effect_policy_splash, STR);
   E_CONFIG_VAL(D, T, effect_policy_dialog, STR);
   E_CONFIG_VAL(D, T, effect_policy_normal, STR);
   E_CONFIG_VAL(D, T, effect_policy_videocall, STR);
   E_CONFIG_VAL(D, T, effect_policy_dropdown_menu, STR);
   E_CONFIG_VAL(D, T, effect_policy_popup_menu, STR);
   E_CONFIG_VAL(D, T, effect_policy_tooltip, STR);
   E_CONFIG_VAL(D, T, effect_policy_notification, STR);
   E_CONFIG_VAL(D, T, effect_policy_combo, STR);
   E_CONFIG_VAL(D, T, effect_policy_dnd, STR);
   E_CONFIG_VAL(D, T, effect_policy_menuscreen, STR);
   E_CONFIG_VAL(D, T, effect_policy_quickpanel_base, STR);
   E_CONFIG_VAL(D, T, effect_policy_quickpanel, STR);
   E_CONFIG_VAL(D, T, effect_policy_taskmanager, STR);
   E_CONFIG_VAL(D, T, effect_policy_livemagazine, STR);
   E_CONFIG_VAL(D, T, effect_policy_lockscreen, STR);
   E_CONFIG_VAL(D, T, effect_policy_indicator, STR);
   E_CONFIG_VAL(D, T, effect_policy_tickernoti, STR);
   E_CONFIG_VAL(D, T, effect_policy_debugging_info, STR);
   E_CONFIG_VAL(D, T, effect_policy_apptray, STR);
   E_CONFIG_VAL(D, T, effect_policy_mini_apptray, STR);
   E_CONFIG_VAL(D, T, effect_policy_volume, STR);
   E_CONFIG_VAL(D, T, effect_policy_background, STR);
   E_CONFIG_VAL(D, T, effect_policy_isf_keyboard, STR);
   E_CONFIG_VAL(D, T, effect_policy_isf_sub, STR);
   E_CONFIG_VAL(D, T, effect_policy_setup_wizard, STR);
   E_CONFIG_VAL(D, T, effect_policy_app_popup, STR);
   E_CONFIG_LIST(D, T, match.popups, *match_edd);
   E_CONFIG_LIST(D, T, match.borders, *match_edd);
   E_CONFIG_LIST(D, T, match.overrides, *match_edd);
   E_CONFIG_LIST(D, T, match.menus, *match_edd);
}

EAPI Config *
e_mod_comp_cfdata_config_new(void)
{
   Config *cfg;
   Match *mat;

   cfg = E_NEW(Config, 1);

   cfg->shadow_file = calloc(4096, sizeof(unsigned char));
   if (cfg->shadow_file)
     {
        snprintf((char *)(cfg->shadow_file),
                 4096 * sizeof(unsigned char),
                 "%s/data/themes/shadow.edj",
                 e_prefix_data_get());
        fprintf(stdout,
                "[E17-comp] shadow file path: %s\n",
                cfg->shadow_file);
     }

   cfg->shadow_style = eina_stringshare_add("default");
   cfg->effect_file = eina_stringshare_add("/usr/lib/enlightenment/modules/comp-tizen/effect/common.so");
   cfg->engine = ENGINE_GL;
   cfg->max_unmapped_pixels = 32 * 1024;  // implement
   cfg->max_unmapped_time = 10 * 3600; // implement
   cfg->min_unmapped_time = 5 * 60; // implement
   cfg->fps_average_range = 30;
   cfg->fps_corner = 0;
   cfg->fps_show = 0;
   cfg->use_shadow = 0;
   cfg->indirect = 0;
   cfg->texture_from_pixmap = 1;
   cfg->lock_fps = 0;
   cfg->loose_sync = 1;
   cfg->efl_sync = 1;
   cfg->grab = 0;
   cfg->vsync = 1;
   cfg->keep_unmapped = 1;
   cfg->send_flush = 1; // implement
   cfg->send_dump = 1; // implement
   cfg->nocomp_fs = 0; // buggy
   cfg->use_hwc = 0; // hardware_compositing
   cfg->smooth_windows = 0; // 1 if gl, 0 if not
   cfg->first_draw_delay = 0.15;

   cfg->canvas_per_zone = 1;
   cfg->use_lock_screen = 1;
   cfg->default_window_effect = 0;
   cfg->keyboard_effect = 0;
   cfg->xv_rotation_effect = 0;
   cfg->fake_image_launch = 0;
   cfg->fake_launch_layer = 0;
   cfg->lower_layer = 5;
   cfg->defer_raise_effect = 0;
   cfg->max_lock_screen_time = 2.0;
   cfg->damage_timeout = 10.0;
   cfg->nocomp_begin_timeout = 2.0;
   cfg->use_hw_ov = 0;
   cfg->debug_info_show = 0;
   cfg->max_debug_msgs = 1;
   cfg->debug_type_nocomp = 1;
   cfg->debug_type_swap = 0;
   cfg->debug_type_effect = 0;

   cfg->effect_policy_unknown = eina_stringshare_add("shadow");
   cfg->effect_policy_desktop = eina_stringshare_add("no-effect");
   cfg->effect_policy_dock = eina_stringshare_add("shadow_fade");
   cfg->effect_policy_toolbar = eina_stringshare_add("no-effect");
   cfg->effect_policy_menu = eina_stringshare_add("no-effect");
   cfg->effect_policy_utility = eina_stringshare_add("no-effect");
   cfg->effect_policy_splash = eina_stringshare_add("no-effect");
   cfg->effect_policy_dialog = eina_stringshare_add("dialog");
   cfg->effect_policy_normal = eina_stringshare_add("shadow_fade");
   cfg->effect_policy_videocall = eina_stringshare_add("shadow_fade");
   cfg->effect_policy_dropdown_menu = eina_stringshare_add("no-effect");
   cfg->effect_policy_popup_menu = eina_stringshare_add("shadow");
   cfg->effect_policy_tooltip = eina_stringshare_add("shadow");
   cfg->effect_policy_notification = eina_stringshare_add("dialog");
   cfg->effect_policy_combo = eina_stringshare_add("no-effect");
   cfg->effect_policy_dnd = eina_stringshare_add("no-effect");
   cfg->effect_policy_menuscreen = eina_stringshare_add("home_screen");
   cfg->effect_policy_quickpanel_base = eina_stringshare_add("quickpanel");
   cfg->effect_policy_quickpanel = eina_stringshare_add("quickpanel");
   cfg->effect_policy_taskmanager = eina_stringshare_add("taskmgr");
   cfg->effect_policy_livemagazine = eina_stringshare_add("home_screen");
   cfg->effect_policy_lockscreen = eina_stringshare_add("lockscreen");
   cfg->effect_policy_indicator = eina_stringshare_add("indicator");
   cfg->effect_policy_tickernoti = eina_stringshare_add("dialog_without_dim");
   cfg->effect_policy_debugging_info = eina_stringshare_add("no-effect");
   cfg->effect_policy_apptray = eina_stringshare_add("app_tray");
   cfg->effect_policy_mini_apptray = eina_stringshare_add("app_tray");
   cfg->effect_policy_volume = eina_stringshare_add("dialog_without_dim");
   cfg->effect_policy_background = eina_stringshare_add("no-effect");
   cfg->effect_policy_isf_keyboard = eina_stringshare_add("keyboard");
   cfg->effect_policy_isf_sub = eina_stringshare_add("no-effect");
   cfg->effect_policy_setup_wizard = eina_stringshare_add("no-effect");
   cfg->effect_policy_app_popup = eina_stringshare_add("dialog_without_dim");

   cfg->match.popups = NULL;
   mat = E_NEW(Match, 1);
   cfg->match.popups = eina_list_append(cfg->match.popups, mat);
   mat->name = eina_stringshare_add("shelf");
   mat->shadow_style = eina_stringshare_add("popup");
   mat = E_NEW(Match, 1);
   cfg->match.popups = eina_list_append(cfg->match.popups, mat);
   mat->shadow_style = eina_stringshare_add("popup");

   cfg->match.borders = NULL;

   cfg->match.overrides = NULL;
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->name = eina_stringshare_add("E");
   mat->clas = eina_stringshare_add("Background_Window");
   mat->shadow_style = eina_stringshare_add("none");
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->name = eina_stringshare_add("E");
   mat->clas = eina_stringshare_add("everything");
   mat->shadow_style = eina_stringshare_add("everything");
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_DROPDOWN_MENU;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_POPUP_MENU;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_COMBO;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_TOOLTIP;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   cfg->match.overrides = eina_list_append(cfg->match.overrides, mat);
   mat->shadow_style = eina_stringshare_add("popup");

   cfg->match.menus = NULL;
   mat = E_NEW(Match, 1);
   cfg->match.menus = eina_list_append(cfg->match.menus, mat);
   mat->shadow_style = eina_stringshare_add("menu");
   
   return cfg;
}

static void
_match_list_free(Eina_List *list)
{
   Match *m;

   EINA_LIST_FREE(list, m)
     {
        if (m->title) eina_stringshare_del(m->title);
        if (m->name) eina_stringshare_del(m->name);
        if (m->clas) eina_stringshare_del(m->clas);
        if (m->role) eina_stringshare_del(m->role);
        if (m->shadow_style) eina_stringshare_del(m->shadow_style);
        free(m);
     }
}

EAPI void
e_mod_cfdata_config_free(Config *cfg)
{
   if (cfg->shadow_file) eina_stringshare_del(cfg->shadow_file);
   if (cfg->shadow_style) eina_stringshare_del(cfg->shadow_style);

   if (cfg->effect_file) eina_stringshare_del(cfg->effect_file);

   if (cfg->effect_policy_unknown) eina_stringshare_del(cfg->effect_policy_unknown);
   if (cfg->effect_policy_desktop) eina_stringshare_del(cfg->effect_policy_desktop);
   if (cfg->effect_policy_dock) eina_stringshare_del(cfg->effect_policy_dock);
   if (cfg->effect_policy_toolbar) eina_stringshare_del(cfg->effect_policy_toolbar);
   if (cfg->effect_policy_menu) eina_stringshare_del(cfg->effect_policy_menu);
   if (cfg->effect_policy_utility) eina_stringshare_del(cfg->effect_policy_utility);
   if (cfg->effect_policy_splash) eina_stringshare_del(cfg->effect_policy_splash);
   if (cfg->effect_policy_dialog) eina_stringshare_del(cfg->effect_policy_dialog);
   if (cfg->effect_policy_normal) eina_stringshare_del(cfg->effect_policy_normal);
   if (cfg->effect_policy_videocall) eina_stringshare_del(cfg->effect_policy_videocall);
   if (cfg->effect_policy_dropdown_menu) eina_stringshare_del(cfg->effect_policy_dropdown_menu);
   if (cfg->effect_policy_popup_menu) eina_stringshare_del(cfg->effect_policy_popup_menu);
   if (cfg->effect_policy_tooltip) eina_stringshare_del(cfg->effect_policy_tooltip);
   if (cfg->effect_policy_notification) eina_stringshare_del(cfg->effect_policy_notification);
   if (cfg->effect_policy_combo) eina_stringshare_del(cfg->effect_policy_combo);
   if (cfg->effect_policy_dnd) eina_stringshare_del(cfg->effect_policy_dnd);
   if (cfg->effect_policy_menuscreen) eina_stringshare_del(cfg->effect_policy_menuscreen);
   if (cfg->effect_policy_quickpanel_base) eina_stringshare_del(cfg->effect_policy_quickpanel_base);
   if (cfg->effect_policy_quickpanel) eina_stringshare_del(cfg->effect_policy_quickpanel);
   if (cfg->effect_policy_taskmanager) eina_stringshare_del(cfg->effect_policy_taskmanager);
   if (cfg->effect_policy_livemagazine) eina_stringshare_del(cfg->effect_policy_livemagazine);
   if (cfg->effect_policy_lockscreen) eina_stringshare_del(cfg->effect_policy_lockscreen);
   if (cfg->effect_policy_indicator) eina_stringshare_del(cfg->effect_policy_indicator);
   if (cfg->effect_policy_tickernoti) eina_stringshare_del(cfg->effect_policy_tickernoti);
   if (cfg->effect_policy_debugging_info) eina_stringshare_del(cfg->effect_policy_debugging_info);
   if (cfg->effect_policy_apptray) eina_stringshare_del(cfg->effect_policy_apptray);
   if (cfg->effect_policy_mini_apptray) eina_stringshare_del(cfg->effect_policy_mini_apptray);
   if (cfg->effect_policy_volume) eina_stringshare_del(cfg->effect_policy_volume);
   if (cfg->effect_policy_background) eina_stringshare_del(cfg->effect_policy_background);
   if (cfg->effect_policy_isf_keyboard) eina_stringshare_del(cfg->effect_policy_isf_keyboard);
   if (cfg->effect_policy_isf_sub) eina_stringshare_del(cfg->effect_policy_isf_sub);
   if (cfg->effect_policy_setup_wizard) eina_stringshare_del(cfg->effect_policy_setup_wizard);
   if (cfg->effect_policy_app_popup) eina_stringshare_del(cfg->effect_policy_app_popup);

   _match_list_free(cfg->match.popups);
   _match_list_free(cfg->match.borders);
   _match_list_free(cfg->match.overrides);
   _match_list_free(cfg->match.menus);

   free(cfg);
}

