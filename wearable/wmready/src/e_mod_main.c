#include "e.h"
#include "e_mod_main.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17_EXTRA_MODULES"

static int _e_wmready_init(void);
static void _e_wmready_fin(void);
static void _e_wmready_configure_system(void);
static void _e_wmready_set_ready_flag(void);
static Eina_Bool _e_wmready_cb_timer(void *data);

static Ecore_Timer *_wmready_timer = NULL;

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "I'm Ready Module of Window Manager"
};

EAPI void*
e_modapi_init(E_Module *m)
{
   if (!_e_wmready_init())
     {
        printf("[wmready][%s] Failed @ _e_wmready_init()..!\n", __FUNCTION__);
        return NULL;
     }

   _e_wmready_configure_system();
   _e_wmready_set_ready_flag();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   _e_wmready_fin();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   /* Do Something */
   return 1;
}

static int
_e_wmready_init(void)
{
   int ret = 1;
   return ret;
}

static void
_e_wmready_fin(void)
{
}

static void
_e_wmready_configure_system(void)
{
   // block to save config files
   e_config_save_block_set(1);

   // change power save mode to immediately
   e_powersave_mode_set(E_POWERSAVE_MODE_NONE);

   _wmready_timer = ecore_timer_add(1.0, _e_wmready_cb_timer, NULL);
}

static Eina_Bool
_e_wmready_cb_timer(void *data)
{
   // unblock to save config files
   e_config_save_block_set(0);

   // set power save mode to LOW
   e_powersave_mode_set(E_POWERSAVE_MODE_LOW);

   return ECORE_CALLBACK_CANCEL;
}

static void _e_wmready_set_ready_flag(void)
{
   FILE *_wmready_checker = NULL;

   _wmready_checker = fopen("/tmp/.wm_ready", "wb");
   if (_wmready_checker)
     {
        LOGD("[WM] WINDOW MANAGER is READY!!!");
        fclose(_wmready_checker);
     }
   else
     {
        LOGD("[WM] WINDOW MANAGER is READY. BUT, failed to create .wm_ready file.");
     }
}

