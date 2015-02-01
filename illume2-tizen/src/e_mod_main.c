#include "e_illume_private.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_policy.h"
#include "e_mod_quickpanel.h"
#include "e_mod_log.h"

/* NB: Initially I had done this rewrite with eina_logging enabled, but it
 * degraded performance so much that it was just not worth it. So now this
 * module just uses printfs on the console to report things */

/* external variables */
const char *_e_illume_mod_dir = NULL;
Eina_List *_e_illume_qps = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume2" };

EAPI void *
e_modapi_init(E_Module *m)
{
   Eina_List *ml, *cl, *zl;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   Ecore_X_Window *zones;
   int zcount = 0;

   /* check if illume is loaded and bail out if it is.
    * Illume1 and illume2 both cannot be loaded @ the same time */
   if (e_module_find("illume")) return NULL;
   if (e_module_find("illume2")) return NULL;

   /* set module priority so we load first */
   e_module_priority_set(m, 100);

   /* set module directory variable */
   _e_illume_mod_dir = eina_stringshare_add(m->dir);

   /* try to initialize the config subsystem */
   if (!e_mod_illume_config_init())
     {
        /* clear module directory variable */
        if (_e_illume_mod_dir) eina_stringshare_del(_e_illume_mod_dir);
        _e_illume_mod_dir = NULL;

        return NULL;
     }

   /* try to initialize the policy subsystem */
   if (!e_mod_policy_init())
     {
        /* shutdown the config subsystem */
        e_mod_illume_config_shutdown();

        /* clear module directory variable */
        if (_e_illume_mod_dir) eina_stringshare_del(_e_illume_mod_dir);
        _e_illume_mod_dir = NULL;

        return NULL;
     }

#if ILLUME_LOGGER_BUILD_ENABLE
   if (!e_mod_log_init())
     {
        /* shutdown the policy subsystem */
        e_mod_policy_shutdown();

        /* shutdown the config subsystem */
        e_mod_illume_config_shutdown();

        /* clear module directory variable */
        if (_e_illume_mod_dir) eina_stringshare_del(_e_illume_mod_dir);
        _e_illume_mod_dir = NULL;

        return NULL;
     }
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

   /* initialize the quickpanel subsystem */
   e_mod_quickpanel_init();

   /* loop zones and get count */
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        if (!man) continue;
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             if (!con) continue;
             EINA_LIST_FOREACH(con->zones, zl, zone)
               {
                  if (zone)
                    zcount++;
               }
          }
     }

   /* allocate enough zones */
   zones = calloc(zcount, sizeof(Ecore_X_Window));
   if (!zones)
     {
        /* shutdown quickpanel */
        e_mod_quickpanel_shutdown();

#if ILLUME_LOGGER_BUILD_ENABLE
        /* shutdown the log subsystem */
        e_mod_log_shutdown();
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

        /* shutdown the config subsystem */
        e_mod_illume_config_shutdown();

        /* clear module directory variable */
        if (_e_illume_mod_dir) eina_stringshare_del(_e_illume_mod_dir);
        _e_illume_mod_dir = NULL;

        return NULL;
     }

   zcount = 0;

   /* loop the zones and create quickpanels for each one */
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        if (!man) continue;
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             if (!con) continue;
             EINA_LIST_FOREACH(con->zones, zl, zone)
               {
                  if (!zone) continue;
                  E_Illume_Quickpanel *qp;

                  /* set zone window in list of zones */
                  zones[zcount] = zone->black_win;

                  /* increment zone count */
                  zcount++;

                  /* try to create a new quickpanel for this zone */
                  if (!(qp = e_mod_quickpanel_new(zone))) continue;

                  /* append new qp to list */
                  _e_illume_qps = eina_list_append(_e_illume_qps, qp);
               }
          }
        /* set the zone list on this root. This is needed for some
         * elm apps like elm_indicator so that they know how many
         * indicators to create at startup */
        ecore_x_e_illume_zone_list_set(man->root, zones, zcount);
     }

   /* free zones variable */
   free(zones);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Illume_Quickpanel *qp;

   /* delete the quickpanels */
   EINA_LIST_FREE(_e_illume_qps, qp)
     e_object_del(E_OBJECT(qp));

   /* shutdown the quickpanel subsystem */
   e_mod_quickpanel_shutdown();

#if ILLUME_LOGGER_BUILD_ENABLE
   /* shutdown the log subsystem */
   e_mod_log_shutdown();
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

   /* shutdown the policy subsystem */
   e_mod_policy_shutdown();

   /* shutdown the config subsystem */
   e_mod_illume_config_shutdown();

   /* clear module directory variable */
   if (_e_illume_mod_dir) eina_stringshare_del(_e_illume_mod_dir);
   _e_illume_mod_dir = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return e_mod_illume_config_save();
}
