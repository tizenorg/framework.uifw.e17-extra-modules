/**
 * @addtogroup Optional_Look
 * @{
 *
 * @defgroup Module_Shot TouchInfo
 *
 * Get touch information
 *
 * @}
 */
#include <e.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <Eina.h>
#include <Evas.h>
#include <Edje.h>
#include <e_manager.h>
#include <e_config.h>

#include "e_mod_devmode.h"
#include "e_mod_devmode_main.h"
#include "e_mod_devmode_event.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

#define USE_E_COMP 1

const char *MODULE_NAME = "devmode-tizen";
EAPI E_Module * devmode_mod = NULL;

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Devmode"
};

static Evas_Object *
__tsp_main_layout_add(Evas_Object *parent)
{
   TSP_CHECK_NULL(parent);
   char edje_path[PATH_MAX];
   sprintf(edje_path, "%s/themes/e_mod_devmode.edj", e_module_dir_get(devmode_mod));

   Evas_Object *layout = edje_object_add(evas_object_evas_get(parent));
   TSP_CHECK_NULL(layout);
   edje_object_file_set(layout, edje_path, "main_layout");

   return layout;
}

static Eina_Bool
_tsp_main_init(TouchInfo *t_info)
{
   /*
    * FIXME : Create evas object from canvas
    */
   Eina_List *managers, *l, *l2, *l3;

   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
        E_Manager *man;

        man = l->data;

        for (l2 = man->containers; l2; l2 = l2->next)
          {
             E_Container *con = NULL;
             con = l2->data;
             t_info->cons = eina_list_append(t_info->cons, con);
#if USE_E_CON
             t_info->canvas = con->bg_evas;
#elif USE_E_WIN
             if (con) t_info->win = e_win_new(con);
             if (t_info->win)
               {
                  e_win_move_resize(t_info->win, con->x, con->y, con->w, con->h);
                  e_win_title_set(t_info->win, "devmode tsp test window");
                  e_win_layer_set(t_info->win, 450);
                  e_win_show(t_info->win);
                  ecore_evas_alpha_set(t_info->win->ecore_evas, EINA_TRUE);
                  t_info->canvas = t_info->win->evas;
                  ecore_x_window_shape_rectangle_set(t_info->win->evas_win, 0, 0, 1, 1);
               }
#elif USE_E_COMP
             if (con->zones)
               {
                  for (l3 = con->zones; l3; l3 = l3->next)
                    {
                       E_Zone *zone;
                       zone = l3->data;
                       if (zone)
                         {
                            t_info->zones = eina_list_append(t_info->zones, zone);

                            /*Always enable comp module*/
                            e_manager_comp_composite_mode_set(man, zone, EINA_TRUE);
                            /*
                             * TODO
                             * While release e_manager_comp_composite_mode_set(, , FALSE)
                             * use list of t_info->zones with EINA_LIST_FOR_EACH
                             */
                            /*
                             * I just worry about if there are many zones, which zone shold be selected???
                             */
                            t_info->x = zone->x;
                            t_info->y = zone->y;
                            t_info->width = zone->w;
                            t_info->height = zone->h;
                         }
                       else
                         {
                            t_info->x = 0;
                            t_info->y = 0;
                            t_info->width = man->w;
                            t_info->height = man->h;
                         }

                       t_info->canvas = e_manager_comp_evas_get(man);
                       if (!t_info->canvas)
                         {
                            TSP_ERROR("[devmode-tizen] Failed to get canvas from comp module");
                            return EINA_FALSE;
                         }
                    }
               }
             else
               {
                  TSP_ERROR("[devmode-tizen] Container doesn't have zones :(");
                  return EINA_FALSE;
               }
#endif
          }
     }

   TSP_DEBUG("[devmode-tizen] viewport's width : %d, height : %d", t_info->width, t_info->height);

   t_info->base = evas_object_rectangle_add(t_info->canvas);
   evas_object_move(t_info->base, 0, 0);
   evas_object_resize(t_info->base, t_info->width, t_info->height);
   evas_object_color_set(t_info->base, 0, 0, 0, 0);
   evas_object_layer_set(t_info->base, EVAS_LAYER_MAX);
   evas_object_pass_events_set(t_info->base, EINA_TRUE);
   evas_object_show(t_info->base);

   t_info->layout_main = __tsp_main_layout_add(t_info->base);
   TSP_CHECK_FALSE(t_info->layout_main);
   evas_object_move(t_info->layout_main, 0, 0);
   //evas_object_size_hint_weight_set(t_info->layout_main, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(t_info->layout_main, t_info->width, t_info->height);
   evas_object_show(t_info->layout_main);

   /* main view */
   tsp_main_view_create(t_info);

   tsp_event_mgr_init(t_info);

   return EINA_TRUE;
}

static void
_tsp_main_config_new(E_Module *m)
{
   TouchInfo *t_info = NULL;
   Mod *mod = NULL;
   if (m) t_info = m->data;
   if (t_info) mod = t_info->module;

   if (mod) mod->conf = e_mod_devmode_cfdata_config_new();
   else
     {
        TSP_ERROR("mod->conf is NIL");
     }
}

static void
_tsp_main_config_free(E_Module *m)
{
   TouchInfo *t_info = NULL;
   Mod *mod = NULL;
   if (m) t_info = m->data;
   if (t_info) mod = t_info->module;

   if (mod)
     {
        e_mod_devmode_cfdata_config_free(mod->conf);
        mod->conf = NULL;
     }
   else
     {
        TSP_ERROR("mod is NIL");
     }
}

static void
_tsp_main_set_win_property(Eina_Bool set)
{
   Ecore_X_Atom x_atom_devmode = 0;
   Ecore_X_Window win = 0;
   unsigned int enable = -1;
   int ret = -1, count = 0;
   unsigned char *prop_data = NULL;

   TSP_FUNC_ENTER();

   x_atom_devmode = ecore_x_atom_get("_E_DEVMODE_ENABLE");
   if (x_atom_devmode == None)
     {
        TSP_ERROR("failed to get atom of _E_DEVMODE_ENABLE\n");
        return;
     }

   win = ecore_x_window_root_first_get();
   if (win <= 0)
     {
        TSP_ERROR("failed to get root window\n");
        return;
     }

   ret = ecore_x_window_prop_property_get(win, x_atom_devmode, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if ( ret && prop_data )
     {
        memcpy(&enable, prop_data, sizeof (unsigned int));
        TSP_DEBUG("devmode-tizen is enabled: %s ", enable ? "true" : "false");
     }

   if (set == EINA_TRUE)
     {
        if (enable == 1) return;
        else enable = 1;
     }
   else
     {
        if (enable == 0) return;
        else enable = 0;
     }

   ecore_x_window_prop_card32_set(win, x_atom_devmode, &enable, 1);

   return 0;
}

EAPI void *
e_modapi_init(E_Module *m)
{
   TSP_FUNC_ENTER();

   //int x, y, w, h;
   TouchInfo *t_info = E_NEW(TouchInfo, 1);
   TSP_CHECK_FALSE(t_info);

   Mod *module = E_NEW(Mod, 1);
   if (!module)
     {
        if (t_info) free(t_info);
        TSP_ERROR("[devmode-tizen] failed to alloc Memory");
        return NULL;
     }

   module->module = m;
   m->data = (void *)t_info;

   t_info->module = (void *)module;
   devmode_mod = m;

   e_mod_devmode_cfdata_edd_init(&(module->conf_edd));
   module->conf = e_config_domain_load("module.devmode-tizen", module->conf_edd);

   if (!module->conf) _tsp_main_config_new(m);

   if (_tsp_main_init(t_info) != EINA_TRUE)
     {
        TSP_ERROR("[devmode-tizen] failed to init main of devmode tizen");
        return NULL;
     }

   _tsp_main_set_win_property(EINA_TRUE);

   return t_info;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   TSP_FUNC_ENTER();

   TouchInfo *t_info = (TouchInfo *)m->data;
   TSP_CHECK_VAL(t_info, 0);

   Mod *mod = t_info->module;

   tsp_event_mgr_deinit();

   tsp_main_view_destroy(t_info);

   tsp_evas_object_del(t_info->base);

   Eina_List *managers, *l, *l2, *l3;

   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
        E_Manager *man;
        man = l->data;

        for (l2 = t_info->cons; l2; l2 = l2->next)
          {
             for (l3 = t_info->zones; l3; l3 = l3->next)
               {
                  E_Zone *zone;
                  zone = l3->data;
                  if (zone)
                    {
                       /*Always enable comp module*/
                       e_manager_comp_composite_mode_set(man, zone, EINA_FALSE);
                    }
               }
          }
     }

   _tsp_main_config_free(m);
   E_CONFIG_DD_FREE(mod->conf_edd);

   SAFE_FREE(mod);
   SAFE_FREE(t_info);
   _tsp_main_set_win_property(EINA_FALSE);

   /*
    * FIXME : Free containers and etc...
    */

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
#if 1
   TouchInfo *t_info = NULL;
   Mod *mod = NULL;
   if (m) t_info = m->data;
   if (t_info) mod = t_info->module;
   if (mod && mod->conf_edd && mod->conf)
     {
        e_config_domain_save("module.devmode-tizen", mod->conf_edd, mod->conf);
     }
   else
     {
        TSP_ERROR("[devmode-tizen] failed to save of conf");
     }
#endif

   return 1;
}

