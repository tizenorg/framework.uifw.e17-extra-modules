#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_comp.h"

/* module private routines */
Mod *_comp_mod = NULL;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Composite"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   Mod *mod;
   char buf[4096];

   mod = calloc(1, sizeof(Mod));
   m->data = mod;

   mod->module = m;
   snprintf(buf, sizeof(buf), "%s/e-module-comp-slp.edj", e_module_dir_get(m));
   e_configure_registry_category_add("appearance", 10, _("Look"), NULL,
                                     "preferences-appearance");
   e_configure_registry_item_add("appearance/comp", 120, _("Composite"), NULL,
                                 buf, e_int_config_comp_module);

   mod->conf_match_edd = E_CONFIG_DD_NEW("Comp_Match", Match);
#undef T
#undef D
#define T Match
#define D mod->conf_match_edd
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

   mod->conf_edd = E_CONFIG_DD_NEW("Comp_Config", Config);
#undef T
#undef D
#define T Config
#define D mod->conf_edd
   E_CONFIG_VAL(D, T, use_shadow, UCHAR);
   E_CONFIG_VAL(D, T, shadow_file, STR);
   E_CONFIG_VAL(D, T, shadow_style, STR);
   E_CONFIG_VAL(D, T, engine, INT);
   E_CONFIG_VAL(D, T, indirect, UCHAR);
   E_CONFIG_VAL(D, T, texture_from_pixmap, UCHAR);
   E_CONFIG_VAL(D, T, lock_fps, UCHAR);
   E_CONFIG_VAL(D, T, efl_sync, UCHAR);
   E_CONFIG_VAL(D, T, loose_sync, UCHAR);
   E_CONFIG_VAL(D, T, grab, UCHAR);
   E_CONFIG_VAL(D, T, vsync, UCHAR);
   E_CONFIG_VAL(D, T, keep_unmapped, UCHAR);
   E_CONFIG_VAL(D, T, send_flush, UCHAR);
   E_CONFIG_VAL(D, T, send_dump, UCHAR);
   E_CONFIG_VAL(D, T, nocomp_fs, UCHAR);
   E_CONFIG_VAL(D, T, smooth_windows, UCHAR);
   E_CONFIG_VAL(D, T, max_unmapped_pixels, INT);
   E_CONFIG_VAL(D, T, max_unmapped_time, INT);
   E_CONFIG_VAL(D, T, min_unmapped_time, INT);
   E_CONFIG_VAL(D, T, use_lock_screen, UCHAR);
   E_CONFIG_VAL(D, T, max_lock_screen_time, FLOAT);
   E_CONFIG_VAL(D, T, dri_buff_flip, UCHAR);
   E_CONFIG_VAL(D, T, default_window_effect, UCHAR);
   E_CONFIG_VAL(D, T, fps_average_range, INT);
   E_CONFIG_VAL(D, T, fps_corner, UCHAR);
   E_CONFIG_VAL(D, T, fps_show, UCHAR);
   E_CONFIG_VAL(D, T, damage_timeout, FLOAT);
   E_CONFIG_LIST(D, T, match.popups,    mod->conf_match_edd);
   E_CONFIG_LIST(D, T, match.borders,   mod->conf_match_edd);
   E_CONFIG_LIST(D, T, match.overrides, mod->conf_match_edd);
   E_CONFIG_LIST(D, T, match.menus,     mod->conf_match_edd);

   mod->conf = e_config_domain_load("module.comp-slp", mod->conf_edd);
   if (!mod->conf)
     {
        mod->conf = E_NEW(Config, 1);
        mod->conf->use_shadow = 0;
        mod->conf->shadow_file = NULL;
        mod->conf->shadow_style = eina_stringshare_add("default");
        mod->conf->engine = ENGINE_GL;
        mod->conf->indirect = 0;
        mod->conf->texture_from_pixmap = 1;
        mod->conf->lock_fps = 0;
        mod->conf->efl_sync = 1;
        mod->conf->loose_sync = 1;
        mod->conf->grab = 0;
        mod->conf->vsync = 1;
        mod->conf->keep_unmapped = 1;
        mod->conf->send_flush = 1; // implement
        mod->conf->send_dump = 0; // implement
        mod->conf->nocomp_fs = 0; // buggy
        mod->conf->smooth_windows = 0;
        mod->conf->max_unmapped_pixels =  32 * 1024; // implement
        mod->conf->max_unmapped_time = 10 * 3600; // implement
        mod->conf->min_unmapped_time = 5 * 60; // implement
        mod->conf->use_lock_screen = 1;
        mod->conf->max_lock_screen_time = 2.0f;
        mod->conf->dri_buff_flip = 1;
        mod->conf->default_window_effect = 1;
        mod->conf->damage_timeout = 10.0f;

        /* make shadow file path */
        mod->conf->shadow_file = calloc(4096, sizeof(unsigned char));
        if (mod->conf->shadow_file)
          {
             snprintf((char*)(mod->conf->shadow_file), 4096 * sizeof(unsigned char), "%s/data/themes/shadow.edj", e_prefix_data_get());
             fprintf(stdout, "[E17-comp] shadow file path: %s\n", mod->conf->shadow_file);
          }

        /* save default configuration to comp.cfg file - yigl 100518 */
        e_modapi_save(m);

        // FIXME: add some default matches
     }

   mod->conf->match.popups = NULL;
   mod->conf->match.borders = NULL;
   mod->conf->match.overrides = NULL;
   mod->conf->match.menus = NULL;
   // disable config allocation from open source - yigl 100607
#if 0
   if (!mod->conf) _e_mod_config_new(m);
#endif

   if (!e_config->use_composite)
     {
        e_config->use_composite = 1;
        e_config_save_queue();
     }

   _comp_mod = mod;

   if (!e_mod_comp_init())
     {
        // FIXME: handle if comp init fails
     }

   e_module_priority_set(m, -1000);
   return mod;
}

void
_e_mod_config_new(E_Module *m)
{
   Mod *mod = m->data;
   Match *mat;

   mod->conf = E_NEW(Config, 1);
   mod->conf->use_shadow = 1;
   mod->conf->shadow_file = NULL;
   mod->conf->shadow_style = eina_stringshare_add("default");
   mod->conf->engine = ENGINE_SW;
   mod->conf->indirect = 0;
   mod->conf->texture_from_pixmap = 0;
   mod->conf->lock_fps = 0;
   mod->conf->efl_sync = 1;
   mod->conf->loose_sync = 1;
   mod->conf->grab = 0;
   mod->conf->vsync = 1;
   mod->conf->keep_unmapped = 1;
   mod->conf->send_flush = 1; // implement
   mod->conf->send_dump = 0; // implement
   mod->conf->nocomp_fs = 0; // buggy
   mod->conf->smooth_windows = 0;
   mod->conf->max_unmapped_pixels =  32 * 1024; // implement
   mod->conf->max_unmapped_time = 10 * 3600; // implement
   mod->conf->min_unmapped_time = 5 * 60; // implement
   mod->conf->use_lock_screen = 1;
   mod->conf->max_lock_screen_time = 2.0f;
   mod->conf->dri_buff_flip = 1;
   mod->conf->default_window_effect = 1;
   mod->conf->fps_average_range = 30;
   mod->conf->fps_corner = 0;
   mod->conf->fps_show = 0;
   mod->conf->damage_timeout = 10.0f;

   mod->conf->match.popups = NULL;
   mat = E_NEW(Match, 1);
   mod->conf->match.popups = eina_list_append(mod->conf->match.popups, mat);
   mat->name = eina_stringshare_add("shelf");
   mat->shadow_style = eina_stringshare_add("still");
   mat = E_NEW(Match, 1);
   mod->conf->match.popups = eina_list_append(mod->conf->match.popups, mat);
   mat->shadow_style = eina_stringshare_add("popup");

   mod->conf->match.borders = NULL;

   mod->conf->match.overrides = NULL;
   mat = E_NEW(Match, 1);
   mod->conf->match.overrides = eina_list_append(mod->conf->match.overrides, mat);
   mat->name = eina_stringshare_add("E");
   mat->clas = eina_stringshare_add("Background_Window");
   mat->shadow_style = eina_stringshare_add("none");
   mat = E_NEW(Match, 1);
   mod->conf->match.overrides = eina_list_append(mod->conf->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_DROPDOWN_MENU;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   mod->conf->match.overrides = eina_list_append(mod->conf->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_POPUP_MENU;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   mod->conf->match.overrides = eina_list_append(mod->conf->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_COMBO;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   mod->conf->match.overrides = eina_list_append(mod->conf->match.overrides, mat);
   mat->primary_type = ECORE_X_WINDOW_TYPE_TOOLTIP;
   mat->shadow_style = eina_stringshare_add("menu");
   mat = E_NEW(Match, 1);
   mod->conf->match.overrides = eina_list_append(mod->conf->match.overrides, mat);
   mat->shadow_style = eina_stringshare_add("popup");

   mod->conf->match.menus = NULL;
   mat = E_NEW(Match, 1);
   mod->conf->match.menus = eina_list_append(mod->conf->match.menus, mat);
   mat->shadow_style = eina_stringshare_add("menu");
}

static void
_match_list_free(Eina_List *list)
{
   Match *m;

   EINA_LIST_FREE(list, m)
     {
        if (!m) continue;
        if (m->title) eina_stringshare_del(m->title);
        if (m->name) eina_stringshare_del(m->name);
        if (m->clas) eina_stringshare_del(m->clas);
        if (m->role) eina_stringshare_del(m->role);
        if (m->shadow_style) eina_stringshare_del(m->shadow_style);
        free(m);
     }
}

void
_e_mod_config_free(E_Module *m)
{
   Mod *mod = m->data;

   if (mod->conf->shadow_file) eina_stringshare_del(mod->conf->shadow_file);
   if (mod->conf->shadow_style) eina_stringshare_del(mod->conf->shadow_style);

   _match_list_free(mod->conf->match.popups);
   _match_list_free(mod->conf->match.borders);
   _match_list_free(mod->conf->match.overrides);
   _match_list_free(mod->conf->match.menus);

   free(mod->conf);
   mod->conf = NULL;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Mod *mod = m->data;

   e_mod_comp_shutdown();

   e_configure_registry_item_del("appearance/comp");
   e_configure_registry_category_del("appearance");

   if (mod->config_dialog)
     {
        e_object_del(E_OBJECT(mod->config_dialog));
        mod->config_dialog = NULL;
     }
   _e_mod_config_free(m);

   E_CONFIG_DD_FREE(mod->conf_match_edd);
   E_CONFIG_DD_FREE(mod->conf_edd);
   free(mod);

   if (mod == _comp_mod) _comp_mod = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   Mod *mod = m->data;
   e_config_domain_save("module.comp-slp", mod->conf_edd, mod->conf);
   return 1;
}
