#ifndef E_MOD_PROCESSMGR_DFPS_H
#define E_MOD_PROCESSMGR_DFPS_H

#include "e.h"
#include <Ecore_X.h>
#include <X11/extensions/Xrandr.h>

typedef enum _dfps_app_state {
    DFPS_LAUNCH_STATE,
    DFPS_RESUME_STATE,
    DFPS_PAUSE_STATE,
    DFPS_TERMINATED_STATE,
} dfps_app_state;

typedef struct _dfps_entry
{
    unsigned int	index;
    char*			package;
    char*			name;
    unsigned int	dfps;
    unsigned int	pid;
} dfps_entry;

typedef struct _dfpsConfig
{
    Ecore_X_Display* disp;
    Ecore_X_Window rootWin;

    Ecore_X_Atom atomRROutput;

    RROutput output;

    char rroutput_buf[256];
    int rroutput_buf_len;

    E_DBus_Connection *conn;
    E_DBus_Signal_Handler *edbus_dfps_handler;
} dfpsConfig;

#define E_PROP_XRROUTPUT	"X_RR_PROPERTY_REMOTE_CONTROLLER"

#define DYNAMICFPS_CFG_FILE	"/usr/etc/dfps/DynamicFPS.xml"

EINTERN int _e_mod_processmgr_dfps_init(void);
EINTERN int _e_mod_processmgr_dfps_close(void);

EINTERN void _e_mod_processmgr_dfps_process_state_change(dfps_app_state state, Ecore_X_Window win);

#endif
