#ifndef _E_MOD_EOM_PRIVATE_H_
#define _E_MOD_EOM_PRIVATE_H_

#undef alloca
#include <e.h>

#include <eom.h>
#include <eom-connect.h>

#define LOG_TAG "EOM"
#include "dlog.h"

#if 1
#undef SLOG
#define SLOG(priority, tag, format, arg...) printf(format, ##arg)
#endif

#define EOM_CHK_RET(cond)       {if (!(cond)) { SLOG(LOG_DEBUG, "EOM", "[%s] : '%s' failed.\n", __func__, #cond); return; }}
#define EOM_CHK_RETV(cond, val) {if (!(cond)) { SLOG(LOG_DEBUG, "EOM", "[%s] : '%s' failed.\n", __func__, #cond); return val; }}
#define EOM_CHK_GOTO(cond, dst) {if (!(cond)) { SLOG(LOG_DEBUG, "EOM", "[%s] : '%s' failed.\n", __func__, #cond); goto dst; }}

#define E_EOM_CFG        "module.eom"
#define STR_LEN            128

typedef struct _E_EOM_Config E_EOM_Config;

/* external variable to store active config */
extern E_EOM_Config *_e_eom_cfg;

struct _E_EOM_Config
{
   struct
   {
      int       enable;
      int       default_dispmode;
      Eina_Bool isPopUpEnabled;
   } ScrnConf;
};

typedef enum _SC_EXT_STATUS
{
   SC_EXT_STATUS_DISCONNECT,
   SC_EXT_STATUS_CONNECT,  /* connect status, but not yet active */
   SC_EXT_STATUS_ACTIVATE, /* active status */
   SC_EXT_STATUS_MAX,
} SC_EXT_STATUS;

/* scrn conf output */
typedef enum
{
    SC_EXT_OUTPUT_ID_NULL,       /* null */
    SC_EXT_OUTPUT_ID_HDMI,       /* hdmi output */
    SC_EXT_OUTPUT_ID_VIRTUAL,    /* virtual output */
} SC_EXT_OUTPUT_ID;

/* scrn conf resolution */
typedef enum
{
   SC_EXT_RES_NULL, /* null */
   SC_EXT_RES_1920X1080, /* 1920 x 1080 */
   SC_EXT_RES_1280X720, /* 1280 x 720 */
   SC_EXT_RES_720X480, /* 720 x 480 */
   SC_EXT_RES_720X576, /* 720 x 576 */
} SC_EXT_RES;

#endif /*_E_MOD_EOM_PRIVATE_H_*/
