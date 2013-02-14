#ifndef SCRNCONF_DEVICEMGR_H
#define SCRNCONF_DEVICEMGR_H

#include <X11/Xlib.h>
#include <utilX.h>

#define STR_ATOM_SCRNCONF_DISPMODE_SET "_SCRNCONF_DISPMODE_SET"
#define STR_ATOM_SCRNCONF_INFO         "_SCRNCONF_INFO"

/* scrn conf output */
typedef enum
{
    SC_EXT_OUTPUT_NULL,       /* null */
    SC_EXT_OUTPUT_HDMI,       /* hdmi output */
    SC_EXT_OUTPUT_VIRTUAL,    /* virtual output */
} SC_EXT_OUTPUT;

/* scrn conf resolution */
typedef enum
{
    SC_EXT_RES_NULL,            /* null */
    SC_EXT_RES_1920X1080,       /* 1920 x 1080 */
    SC_EXT_RES_1280X720,        /* 1280 x 720 */
} SC_EXT_RES;

/* send status */
int scrnconf_ext_send_status (Display *dpy, Utilx_Scrnconf_Status sc_stat, Utilx_Scrnconf_Dispmode sc_dispmode);

/* update screen configuration of a external monitor */
int scrnconf_ext_update_get_perperty (Display *dpy, char *str_output, char *str_stat, char *str_res, char *str_dispmode);

#endif // SCRNCONF_DEVICEMGR_H

