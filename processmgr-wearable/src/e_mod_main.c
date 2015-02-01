#include "e.h"
#include "e_mod_main.h"
#include "e_mod_processmgr.h"
#include "e_mod_processmgr_dfps.h"

/* module private routines */
Mod *_processmgr_mod = NULL;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Process Manager"
};

/* local subsystem functions */
static void _e_mod_config_new(E_Module *m);
static void _e_mod_config_free(E_Module *m);

/* externally accessible functions */
EAPI void *
e_modapi_init(E_Module *m)
{
   Mod *mod;

   mod = calloc(1, sizeof(Mod));
   m->data = mod;

   mod->module = m;
   e_mod_processmgr_cfdata_edd_init(&(mod->conf_edd));
   mod->conf = e_config_domain_load("module.processmgr", mod->conf_edd);

   if (!mod->conf)
     {
        _e_mod_config_new(m);
        e_config_domain_save("module.processmgr", mod->conf_edd, mod->conf);
     }

   _processmgr_mod = mod;

   if (!e_mod_processmgr_init())
     {
        memset(mod, 0, sizeof(Mod));
        free(mod);
        mod = NULL;
        fprintf(stderr,
                "[E17-ProcessMgr] %s(%d) Failed\n",
                __func__, __LINE__);
     }

   _e_mod_processmgr_dfps_init();

   return mod;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   Mod *mod = m->data;

   if (mod == _processmgr_mod) _processmgr_mod = NULL;
   e_mod_processmgr_shutdown();
   _e_mod_config_free(m);
   E_CONFIG_DD_FREE(mod->conf_edd);

   memset(mod, 0, sizeof(Mod));
   free(mod);
   mod = NULL;

   _e_mod_processmgr_dfps_close();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   Mod *mod = m->data;
   e_config_domain_save("module.processmgr", mod->conf_edd, mod->conf);
   return 1;
}

/* local subsystem functions */
static void
_e_mod_config_new(E_Module *m)
{
   Mod *mod = m->data;
   mod->conf = e_mod_processmgr_cfdata_config_new();
}

static void
_e_mod_config_free(E_Module *m)
{
   Mod *mod = m->data;

   e_mod_processmgr_cfdata_config_free(mod->conf);
   mod->conf = NULL;
}
