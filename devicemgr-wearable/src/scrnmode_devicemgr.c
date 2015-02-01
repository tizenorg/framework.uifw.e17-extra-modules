/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This file is a modified version of BSD licensed file and
 * licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please, see the COPYING file for the original copyright owner and
 * license.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <e.h>
#include <vconf.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include "e_devicemgr_privates.h"
#include "scrnmode_devicemgr.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17_EXTRA_MODULES"

#define BUS_NAME                "org.tizen.system.deviced"
#define DEVICED_OBJ_PATH        "/Org/Tizen/System/DeviceD/Display"
#define DEVICED_INTERFACE       "org.tizen.system.deviced.display"
#define MEMBER_ON               "ALPMOn"
#define MEMBER_OFF              "ALPMOff"

#define DBUS_REPLY_TIMEOUT      (120 * 1000)

typedef struct _E_Scrnmode E_Scrnmode;

struct _E_Scrnmode
{
   E_DBus_Connection                 *conn;
   E_DBus_Signal_Handler             *edbus_scrnmode_on_handler;
   E_DBus_Signal_Handler             *edbus_scrnmode_off_handler;
   Ecore_Timer                       *retry_timer;
   int                               retry_count;
   Display                           *dpy;
   int                               alpm_mode;
};

/* static global variables */
static E_Scrnmode scrnmode;


/* local subsystem functions */
static int            _connect_edbus(void);
static Eina_Bool      _connect_edbus_retry_timeout(void *data);
static void           _scrnmode_on_changed_cb(E_DBus_Object *obj, DBusMessage   *msg);
static void           _scrnmode_off_changed_cb(E_DBus_Object *obj, DBusMessage   *msg);
static int            _set_scrnmode(int state);

/* externally accessible functions */
Eina_Bool
e_mod_scrnmode_init(void)
{
   Eina_Bool ret = EINA_FALSE;
   int angle = -1;
   int screen_rot = 0;

   memset (&scrnmode, 0, sizeof(struct _E_Scrnmode));

   ret = _connect_edbus();
   if (!ret)
      ELBF(ELBT_ROT, 0, 0, "_connect_edbus() fail.");

   scrnmode.dpy = ecore_x_display_get();
   if (!scrnmode.dpy)
     {
        ELBF(ELBT_ROT, 0, 0, "fail to ecore_x_open_display.");
        return EINA_FALSE;
     }

   ELBF(ELBT_ROT, 0, 0, "scrnmode.dpy:%p ", scrnmode.dpy);

   return EINA_TRUE;
}

Eina_Bool
e_mod_scrnmode_deinit(void)
{
   e_dbus_signal_handler_del(scrnmode.conn, scrnmode.edbus_scrnmode_on_handler);
   e_dbus_signal_handler_del(scrnmode.conn, scrnmode.edbus_scrnmode_off_handler);
   e_dbus_connection_close(scrnmode.conn);
   e_dbus_shutdown();

   memset (&scrnmode, 0, sizeof(struct _E_Scrnmode));

   return EINA_TRUE;
}

static int
_connect_edbus(void)
{
   int ret = 0;
   E_DBus_Connection *conn = NULL;
   static E_DBus_Signal_Handler *edbus_scrnmode_handler = NULL;

   if (scrnmode.retry_timer)
     {
        ecore_timer_del(scrnmode.retry_timer);
        scrnmode.retry_timer = NULL;
     }

   ret = e_dbus_init();
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "e_dbus_init error! ret(%d)", ret);
        goto con_error;
     }
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn)
     {
        ELBF(ELBT_ROT, 0, 0, "e_dbus_bus_get error!", ret);
        goto con_error;
     }
   scrnmode.conn = conn;

   edbus_scrnmode_handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_OBJ_PATH,
                                                      DEVICED_INTERFACE, MEMBER_ON,
                                                      (E_DBus_Signal_Cb)_scrnmode_on_changed_cb,  NULL);

   if (!edbus_scrnmode_handler)
     {
        ELBF(ELBT_ROT, 0, 0, "e_dbus_signal_handler_add() error!");
        goto con_error;
     }
   scrnmode.edbus_scrnmode_on_handler = edbus_scrnmode_handler;

   edbus_scrnmode_handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_OBJ_PATH,
                                                      DEVICED_INTERFACE, MEMBER_OFF,
                                                      (E_DBus_Signal_Cb)_scrnmode_off_changed_cb,  NULL);

   if (!edbus_scrnmode_handler)
     {
        ELBF(ELBT_ROT, 0, 0, "e_dbus_signal_handler_add() error!");
        goto con_error;
     }
   scrnmode.edbus_scrnmode_off_handler = edbus_scrnmode_handler;

   scrnmode.retry_count = 0;

   //_get_rotation();
   ELBF(ELBT_ROT, 0, 0, "conneted successfully! scrnmode.conn(%p) scrnmode.edbus_scrnmode_on_handler(%p) scrnmode.edbus_scrnmode_off_handler(%p)",
                          scrnmode.conn, scrnmode.edbus_scrnmode_on_handler, scrnmode.edbus_scrnmode_off_handler);

   return EINA_TRUE;

con_error:
   if (scrnmode.conn)
       e_dbus_connection_close(scrnmode.conn);

   if (scrnmode.retry_count <= 20)
     {
        scrnmode.retry_timer = ecore_timer_add(10.0f,
                                                _connect_edbus_retry_timeout,
                                                NULL);
     }
   return EINA_FALSE;
}

static Eina_Bool
_connect_edbus_retry_timeout(void *data)
{
   Eina_Bool res = EINA_FALSE;

   if (scrnmode.retry_timer)
     {
        ecore_timer_del(scrnmode.retry_timer);
        scrnmode.retry_timer = NULL;
     }
   scrnmode.retry_count++;
   ELBF(ELBT_ROT, 0, 0, "retrying to connect edbus", scrnmode.retry_count);
   res = _connect_edbus();
   if (res)
        ELBF(ELBT_ROT, 0, 0, "_connect_edbus() fail.");

   return ECORE_CALLBACK_CANCEL;
}

static void
_scrnmode_on_changed_cb(E_DBus_Object *obj, DBusMessage   *msg)
{
   DBusError err;
   int val;
   int ret;
   int state=1;

   SECURE_SLOGD("[SCREENMODE] ALPM MODE ON.");
   ret = dbus_message_is_signal(msg, DEVICED_INTERFACE, MEMBER_ON);
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "dbus_message_is_signal error! ret(%d)", ret);
        return;
     }

   dbus_error_init(&err);
   //ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
   //if (!ret)
   //  {
   //     ELBF(ELBT_ROT, 0, 0, "dbus_message_get_args error! ret(%d)", ret);
   //     return;
   //  }

   //state = val;
   _set_scrnmode(state);

   SECURE_SLOGD("[SCREENMODE] ALPM MODE ON DONE.");

   return;
}

static void
_scrnmode_off_changed_cb(E_DBus_Object *obj, DBusMessage   *msg)
{
   DBusError err;
   int val;
   int ret;
   int state=0;

   SECURE_SLOGD("[SCREENMODE] ALPM MODE OFF.");
   ret = dbus_message_is_signal(msg, DEVICED_INTERFACE, MEMBER_OFF);
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "dbus_message_is_signal error! ret(%d)", ret);
        return;
     }

   dbus_error_init(&err);
   //ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
   //if (!ret)
   //  {
   //     ELBF(ELBT_ROT, 0, 0, "dbus_message_get_args error! ret(%d)", ret);
   //     return;
   //  }

   //state = val;
   _set_scrnmode(state);

   SECURE_SLOGD("[SCREENMODE] ALPM MODE OFF DONE.");
   return;
}

static int
_set_scrnmode (int state)
{
   Window root = None;
   XRRScreenResources *res = NULL;
   RROutput rr_output = None;
   static Atom property = None;
   int i;
   char buf[32] = {0,};
   char *p = buf;
   int buf_len = 0;

   if(scrnmode.dpy == NULL)
     {
        ELBF(ELBT_ROT, 0, 0, "scrnmode.dpy(%p).", scrnmode.dpy);
        return EINA_FALSE;
     }

   property = XInternAtom (scrnmode.dpy, "XRR_PROPERTY_SCREEN_MODE", False);
   if (property == None)
     {
        ELBF(ELBT_ROT, 0, 0, "property is none.");
        return EINA_FALSE;
     }

   root = XRootWindow (scrnmode.dpy, 0);
   if (root == None)
     {
        ELBF(ELBT_ROT, 0, 0, "root:%lx.", root);
        return EINA_FALSE;
     }

   res = XRRGetScreenResources (scrnmode.dpy, root);
   if (res == NULL || res->noutput == 0)
     {
        ELBF(ELBT_ROT, 0, 0, "ScreenResources is NULL. res(%p) ", res);
        return EINA_FALSE;
     }

   for (i = 0; i < res->noutput; i++)
     {
        XRROutputInfo *output_info = XRRGetOutputInfo (scrnmode.dpy, res, res->outputs[i]);
        if (output_info)
          {
             if (!strcmp (output_info->name, "LVDS1"))
               {
                   rr_output = res->outputs[i];
                   XRRFreeOutputInfo(output_info);
                   break;
               }
             XRRFreeOutputInfo(output_info);
          }
     }

   if (rr_output == None)
     {
        ELBF(ELBT_ROT, 0, 0, "rr_output:%lx ", rr_output);
        XRRFreeScreenResources (res);
        return EINA_FALSE;
     }

   p += sprintf (p, "%d", state);
   *p = '\0';
   p++;
   buf_len = p - buf;

   XRRChangeOutputProperty (scrnmode.dpy, rr_output, property,
                            XA_CARDINAL, 8, PropModeReplace, buf, buf_len);

   scrnmode.alpm_mode = state;
   ELBF(ELBT_ROT, 0, 0, "scrnmode.alpm_mode(%d) ", scrnmode.alpm_mode);
   return EINA_TRUE;
}

