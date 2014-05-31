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
#include <sensor.h>
#include <vconf.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include "e_devicemgr_privates.h"
#include "sf_rotation_devicemgr.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17_EXTRA_MODULES"

#if 1

#define BUS_NAME                "org.tizen.system.coord"
#define DEVICED_OBJ_PATH        "/Org/Tizen/System/Coord/Rotation"
#define DEVICED_INTERFACE       "org.tizen.system.coord.rotation"
#define ROTATION_GET_METHOD     "Degree"
#define ROTATION_GET_SIGNAL     "Changed"

#define DBUS_REPLY_TIMEOUT      (120 * 1000)

typedef struct _E_DM_Sensor_Rotation E_DM_Sensor_Rotation;

struct _E_DM_Sensor_Rotation
{
   E_DBus_Connection                 *conn;
   E_DBus_Signal_Handler             *edbus_rot_handler;
   Ecore_Timer                       *retry_timer;
   int                               retry_count;
   Display                           *dpy;
   int                               touch_device_id;
   int                               screen_rotation;
};

/* static global variables */
static E_DM_Sensor_Rotation rot;
static Ecore_X_Atom ATOM_DEVICE_ROTATION_ANGLE = 0;


extern void _e_devicemgr_set_touch_rotation_angle(int deviceid, int angle);
/* local subsystem functions */
static int            _connect_edbus(void);
static Eina_Bool      _connect_edbus_retry_timeout(void *data);
static void           _rotation_changed_cb(E_DBus_Object *obj, DBusMessage   *msg);
static void           _set_rotation(int screen_rotation);
static void           _set_touch_rotation (int screen_rotation);
static int            _get_rotation(void);
static int            _get_angle(enum accelerometer_rotate_state state);
static DBusMessage*  _dbus_method_sync_with_reply(const char *dest, const char *path,
                                                                 const char *interface, const char *method,
                                                                 const char *sig, char *param[]);
/* externally accessible functions */
Eina_Bool
e_mod_sf_rotation_init(int touch_device_id)
{
   Eina_Bool ret = EINA_FALSE;
   int angle = -1;
   int screen_rot = 0;

   memset (&rot, 0, sizeof(struct _E_DM_Sensor_Rotation));

   ret = _connect_edbus();
   if (!ret)
      ELBF(ELBT_ROT, 0, 0, "_connect_edbus() fail.");

   rot.dpy = ecore_x_display_get();
   rot.touch_device_id = touch_device_id;
   if (!rot.dpy)
     {
        ELBF(ELBT_ROT, 0, 0, "fail to ecore_x_open_display.");
        return EINA_FALSE;
     }

#ifdef _F_WEARABLE_PROFILE_
   ret = vconf_get_int(VCONFKEY_SETAPPL_SCREENROTATION_DEG_INT, &screen_rot);
   if (ret)
     {
        ELBF(ELBT_ROT, 0, 0,
                      "ERR! SCREENROTATION get failed. "
                      "ret:%d screen_rot:%d ", ret, screen_rot);
     }

   /* change CW (setting value) to CCW(x video driver) */
   switch (screen_rot)
     {
      case 0:   angle = 0; break;
      case 1:   angle = 3; break;
      case 2:   angle = 2; break;
      case 3:   angle = 1; break;
      default:
         ELBF(ELBT_ROT, 0, 0, "Unknown state screen_rot(%d)", screen_rot);
        break;
     }

   if (angle == -1)
     {
        ELBF(ELBT_ROT, 0, 0, "invlid rotation value.  angle:%d, screen_rot:%d ", angle, screen_rot);
        _set_rotation(0);
     }
   else
        _set_rotation(angle);
#else  // MOBILE_PROFILE
    angle = 0;
    _set_rotation(angle);
#endif //_F_WEARABLE_PROFILE_


   ELBF(ELBT_ROT, 0, 0, "angle:%d, screen_rot:%d rot.dpy:%p ", angle, screen_rot, rot.dpy);

   return EINA_TRUE;
}

Eina_Bool
e_mod_sf_rotation_deinit(void)
{
   e_dbus_signal_handler_del(rot.conn, rot.edbus_rot_handler);
   e_dbus_connection_close(rot.conn);
   e_dbus_shutdown();

   memset (&rot, 0, sizeof(struct _E_DM_Sensor_Rotation));

   return EINA_TRUE;
}

static int
_connect_edbus(void)
{
   int ret = 0;
   E_DBus_Connection *conn = NULL;
   static E_DBus_Signal_Handler *edbus_rot_handler = NULL;

   if (rot.retry_timer)
     {
        ecore_timer_del(rot.retry_timer);
        rot.retry_timer = NULL;
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
   rot.conn = conn;

   edbus_rot_handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_OBJ_PATH,
                                                      DEVICED_INTERFACE, ROTATION_GET_SIGNAL,
                                                      (E_DBus_Signal_Cb)_rotation_changed_cb,  NULL);

   if (!edbus_rot_handler)
     {
        ELBF(ELBT_ROT, 0, 0, "e_dbus_signal_handler_add() error!");
        goto con_error;
     }
   rot.edbus_rot_handler = edbus_rot_handler;

   rot.retry_count = 0;

   //_get_rotation();
   ELBF(ELBT_ROT, 0, 0, "conneted successfully! rot.conn(%p) rot.edbus_rot_handler(%p)", rot.conn, rot.edbus_rot_handler);

   return EINA_TRUE;

con_error:
   if (rot.conn)
       e_dbus_connection_close(rot.conn);

   if (rot.retry_count <= 20)
     {
        rot.retry_timer = ecore_timer_add(10.0f,
                                          _connect_edbus_retry_timeout,
                                          NULL);
     }
   return EINA_FALSE;
}

static Eina_Bool
_connect_edbus_retry_timeout(void *data)
{
   Eina_Bool res = EINA_FALSE;

   if (rot.retry_timer)
     {
        ecore_timer_del(rot.retry_timer);
        rot.retry_timer = NULL;
     }
   rot.retry_count++;
   ELBF(ELBT_ROT, 0, 0, "retrying to connect edbus", rot.retry_count);
   res = _connect_edbus();
   if (res)
        ELBF(ELBT_ROT, 0, 0, "_connect_edbus() fail.");

   return ECORE_CALLBACK_CANCEL;
}

static void
_rotation_changed_cb(E_DBus_Object *obj, DBusMessage   *msg)
{
   DBusError err;
   int val;
   int ret;
   int ang = 0;
   enum accelerometer_rotate_state state;

   ret = dbus_message_is_signal(msg, DEVICED_INTERFACE, ROTATION_GET_SIGNAL);
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "dbus_message_is_signal error! ret(%d)", ret);
        return;
     }

   dbus_error_init(&err);
   ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "dbus_message_get_args error! ret(%d)", ret);
        return;
     }

   state = (enum accelerometer_rotate_state)val;
   ang = _get_angle(state);
   if (ang < 0)
     {
        ELBF(ELBT_ROT, 0, 0, "invliad state:%d angle:%d", state, ang);
        return;
     }
   SECURE_SLOGD("[ROTATION] ROT_CHANGE, state:%d angle:%d", state, ang);
   ELBF(ELBT_ROT, 0, 0, "ROT_EV state:%d angle:%d", state, ang);

   _set_rotation(ang);

   //_get_rotation();
}

static int _get_rotation(void)
{
   DBusError err;
   DBusMessage *msg;
   int ret, status;
   msg = _dbus_method_sync_with_reply(BUS_NAME, DEVICED_OBJ_PATH, DEVICED_INTERFACE,
                                      ROTATION_GET_METHOD, NULL, NULL);
   if (!msg)
     {
        ELBF(ELBT_ROT, 0, 0, "deviced_dbus_method_sync_with_reply error!");
        return -1;
     }
   dbus_error_init(&err);

   ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &status, DBUS_TYPE_INVALID);
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "dbus_message_get_args error!");
        return -1;
     }
   dbus_message_unref(msg);
   dbus_error_free(&err);

   ELBF(ELBT_ROT, 0, 0, "rotation status:%d", status);
   return status;
}


static void
_set_rotation (int screen_rotation)
{
   Window root = None;
   XRRScreenResources *res = NULL;
   RROutput rr_output = None;
   static Atom property = None;
   int i;
   char buf[32] = {0,};
   char *p = buf;
   int buf_len = 0;

   if(rot.dpy == NULL)
     {
        ELBF(ELBT_ROT, 0, 0, "rot.dpy(%p).", rot.dpy);
        return EINA_FALSE;
     }

   property = XInternAtom (rot.dpy, "XRR_PROPERTY_SCREEN_ROTATE", False);
   if (property == None)
     {
        ELBF(ELBT_ROT, 0, 0, "property is none.");
        return EINA_FALSE;    
     }

   root = XRootWindow (rot.dpy, 0);
   if (root == None)
     {
        ELBF(ELBT_ROT, 0, 0, "root:%lx.", root);
        return EINA_FALSE;
     }

   res = XRRGetScreenResources (rot.dpy, root);
   if (res == NULL || res->noutput == 0)
     {
        ELBF(ELBT_ROT, 0, 0, "ScreenResources is NULL. res(%p) ", res);
        return EINA_FALSE;
     }

   for (i = 0; i < res->noutput; i++)
     {
        XRROutputInfo *output_info = XRRGetOutputInfo (rot.dpy, res, res->outputs[i]);
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

   p += sprintf (p, "%d", screen_rotation);
   *p = '\0';
   p++;
   buf_len = p - buf;

   XRRChangeOutputProperty (rot.dpy, rr_output, property,
                            XA_CARDINAL, 8, PropModeReplace, buf, buf_len);

   _set_touch_rotation(screen_rotation);

   rot.screen_rotation = screen_rotation;
   ELBF(ELBT_ROT, 0, 0, "rot.screen_rotation(%d) ", rot.screen_rotation);
   return EINA_TRUE;
}

static void
_set_touch_rotation (int screen_rotation)
{
   int ang = 0;
   switch (screen_rotation)
     {
        case 0:   ang = 0; break;
        case 1:   ang = 1; break;
        case 2:   ang = 3; break;
        case 3:   ang = 2; break;
        default:
           ELB(ELBT_ROT, "invalid screen_rotation(%d)", screen_rotation);
           return;
           break;
     }

   _e_devicemgr_set_touch_rotation_angle(rot.touch_device_id, ang);
   ELBF(ELBT_ROT, 0, 0, "touch_device_id(%d) ang(%d)", rot.touch_device_id, ang);

   return;
}

static int
_get_angle(enum accelerometer_rotate_state state)
{
   int ang = -1;

   /* change rotation value from Coord to EFL */
   switch (state)
     {
        case ROTATION_EVENT_0:     ang = 0; break;
        case ROTATION_EVENT_90:    ang = 3; break;
        case ROTATION_EVENT_180:   ang = 2; break;
        case ROTATION_EVENT_270:   ang = 1; break;
        default:
           ELBF(ELBT_ROT, 0, 0, "Unknown state(%d)", state);
           break;
     }

   return ang;
}

static int
append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
   char *ch;
   int i;
   int int_type;
   uint64_t int64_type;
   DBusMessageIter arr;
   struct dbus_byte *bytes;

   if (!sig || !param)
      return 0;

   for (ch = (char*)sig, i = 0; *ch != '\0'; ++i, ++ch) {
      switch (*ch) {
         case 'i':
            int_type = atoi(param[i]);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
            break;
         case 'u':
            int_type = strtoul(param[i], NULL, 10);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
            break;
         case 't':
            int64_type = atoll(param[i]);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
            break;
         case 's':
            dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
            break;
         default:
            return -EINVAL;
      }
   }

   return 0;
}

static DBusMessage *
_dbus_method_sync_with_reply(const char *dest, const char *path,
                             const char *interface, const char *method,
                             const char *sig, char *param[])
{
   DBusConnection *conn;
   DBusMessage *msg;
   DBusMessageIter iter;
   DBusMessage *reply;
   DBusError err;
   int r;

   conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

   if (!conn) {
      ELBF(ELBT_ROT, 0, 0, "conn is NULL");
      return NULL;
   }

   msg = dbus_message_new_method_call(dest, path, interface, method);
   if (!msg) {
      ELBF(ELBT_ROT, 0, 0, "dbus_message_new_method_call(%s:%s-%s)",path, interface, method);
      return NULL;
   }

   dbus_message_iter_init_append(msg, &iter);
   r = append_variant(&iter, sig, param);
   if (r < 0) {
      ELBF(ELBT_ROT, 0, 0, "dbus_message_iter_init_append error.");
      dbus_message_unref(msg);
      return NULL;
   }

   dbus_error_init(&err);

   reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);
   if (!reply) {
      ELBF(ELBT_ROT, 0, 0, "dbus_connection_send_with_reply_and_block error!");
   }

   if (dbus_error_is_set(&err)) {
      ELBF(ELBT_ROT, 0, 0, "dbus_connection_send error(%s:%s)", err.name, err.message);
      dbus_error_free(&err);
      reply = NULL;
   }

   dbus_message_unref(msg);
   return reply;
}


#else
//////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _E_DM_Sensor_Rotation E_DM_Sensor_Rotation;

struct _E_DM_Sensor_Rotation
{
   int                             handle;
   Eina_Bool                       started;
   enum accelerometer_rotate_state state;
   Ecore_Timer                    *retry_timer;
   int                             retry_count;
   Eina_Bool                       lock;
   Eina_Bool                       connected;
};

/* static global variables */
static E_DM_Sensor_Rotation rot;
static Ecore_X_Atom ATOM_DEVICE_ROTATION_ANGLE = 0;

/* local subsystem functions */
static Eina_Bool _sensor_connect(void);
static Eina_Bool _sensor_disconnect(void);
static Eina_Bool _sensor_connect_retry_timeout(void *data);
static void      _sensor_rotation_changed_cb(unsigned int event_type, sensor_event_data_t *event, void *data);
static void      _vconf_cb_lock_change(keynode_t *node, void *data);
static void      _sensor_rotation_set(int ang);
static int       _ang_get(enum accelerometer_rotate_state state);

/* externally accessible functions */
Eina_Bool 
e_mod_sf_rotation_init(void)
{
   int r = 0, lock = 0;
   Eina_Bool res = EINA_FALSE;

   rot.connected = EINA_FALSE;
   rot.retry_count = 0;
   res = _sensor_connect();
   if (res)
     {
        r = vconf_get_bool(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, &lock);
        if (r)
          {
             ELBF(ELBT_ROT, 0, 0,
                  "ERR! AUTO_ROTATE_SCREEN_BOOL get failed. "
                  "r:%d lock:%d", r, lock);
          }
        else
          {
             rot.lock = !lock;
             vconf_notify_key_changed(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL,
                                      _vconf_cb_lock_change,
                                      NULL);
             ELBF(ELBT_ROT, 0, 0,
                  "AUTO_ROTATE_SCREEN_BOOL get succeeded. "
                  "lock:%d rot.locK%d", lock, rot.lock);
          }
     }
   _sensor_rotation_set(0);
   return EINA_TRUE;
}

Eina_Bool 
e_mod_sf_rotation_deinit(void)
{
   vconf_ignore_key_changed(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, _vconf_cb_lock_change);
   _sensor_disconnect();
   return EINA_TRUE;
}

/* local subsystem functions */
static Eina_Bool
_sensor_connect(void)
{
   int h, r, lock = 0;
   if (rot.connected) return EINA_TRUE;

   if (rot.retry_timer)
     {
        ecore_timer_del(rot.retry_timer);
        rot.retry_timer = NULL;
     }

   rot.handle = -1;
   rot.started = EINA_FALSE;

   h = sf_connect(ACCELEROMETER_SENSOR);
   if (h < 0)
     {
        ELB(ELBT_ROT, "ERR! sf_connect failed", h);
        goto error;
     }

   r = sf_register_event(h, ACCELEROMETER_EVENT_ROTATION_CHECK,
                         NULL, _sensor_rotation_changed_cb, NULL);
   if (r < 0)
     {
        ELB(ELBT_ROT, "ERR! sf_register_event failed", r);
        sf_disconnect(h);
        goto error;
     }

   r = sf_start(h, 0);
   if (r < 0)
     {
        ELB(ELBT_ROT, "ERR! sf_start failed", r);
        sf_unregister_event(h, ACCELEROMETER_EVENT_ROTATION_CHECK);
        sf_disconnect(h);
        goto error;
     }

   rot.handle = h;
   rot.started = EINA_TRUE;
   rot.retry_count = 0;
   rot.lock = EINA_FALSE;
   rot.connected = EINA_TRUE;

   ELB(ELBT_ROT, "sf_connect succeeded", h);
   return EINA_TRUE;

error:
   if (rot.retry_count <= 20)
     {
        rot.retry_timer = ecore_timer_add(10.0f,
                                          _sensor_connect_retry_timeout,
                                          NULL);
     }
   return EINA_FALSE;
}

static Eina_Bool
_sensor_disconnect(void)
{
   int r;
   if (!rot.connected) return EINA_TRUE;

   rot.lock = EINA_FALSE;

   if (rot.retry_timer)
     {
        ecore_timer_del(rot.retry_timer);
        rot.retry_timer = NULL;
     }

   rot.retry_count = 0;

   if (rot.handle < 0)
     {
        ELB(ELBT_ROT, "ERR! invalid handle", rot.handle);
        goto error;
     }

   if (rot.started)
     {
        r = sf_unregister_event(rot.handle,
                                ACCELEROMETER_EVENT_ROTATION_CHECK);
        if (r < 0)
          {
             ELB(ELBT_ROT, "ERR! sf_unregister_event failed", r);
             goto error;
          }
        r = sf_stop(rot.handle);
        if (r < 0)
          {
             ELB(ELBT_ROT, "ERR! sf_stop failed", r);
             goto error;
          }
        rot.started = EINA_TRUE;
     }

   r = sf_disconnect(rot.handle);
   if (r < 0)
     {
        ELB(ELBT_ROT, "ERR! sf_disconnect failed", r);
        goto error;
     }

   rot.handle = -1;
   rot.connected = EINA_FALSE;
   ELB(ELBT_ROT, "sf_disconnect succeeded", NULL);
   return EINA_TRUE;
error:
   return EINA_FALSE;
}

static Eina_Bool
_sensor_connect_retry_timeout(void *data)
{
   int r = 0, lock = 0;
   Eina_Bool res = EINA_FALSE;

   if (rot.retry_timer)
     {
        ecore_timer_del(rot.retry_timer);
        rot.retry_timer = NULL;
     }
   rot.retry_count++;
   ELB(ELBT_ROT, "retrying to connect sensor", rot.retry_count);
   res = _sensor_connect();
   if (res)
     {
        r = vconf_get_bool(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, &lock);
        if (r)
          {
             ELBF(ELBT_ROT, 0, 0,
                  "ERR! AUTO_ROTATE_SCREEN_BOOL get failed. "
                  "r:%d lock:%d", r, lock);
          }
        else
          {
             rot.lock = !lock;
             vconf_notify_key_changed(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL,
                                      _vconf_cb_lock_change,
                                      NULL);
             ELBF(ELBT_ROT, 0, 0,
                  "AUTO_ROTATE_SCREEN_BOOL get succeeded. "
                  "lock:%d rot.locK%d", lock, rot.lock);
          }
     }

   return ECORE_CALLBACK_CANCEL;
}

static int
_ang_get(enum accelerometer_rotate_state state)
{
   E_Devicemgr_Config_Rotation *cr = NULL;
   Eina_List *l = NULL;
   int ang = -1, res = -1;

   /* change CW (SensorFW) to CCW(EFL) */
   switch (state)
     {
      case ROTATION_EVENT_0:     ang = 0; break;
      case ROTATION_EVENT_90:    ang = 270; break;
      case ROTATION_EVENT_180:   ang = 180; break;
      case ROTATION_EVENT_270:   ang = 90; break;
      default:
         ELB(ELBT_ROT, "Unknown state", state);
        break;
     }

   EINA_LIST_FOREACH(_e_devicemgr_cfg->rotation, l, cr)
     {
        if (!cr) continue;
        if (cr->angle == ang)
          {
             if (cr->enable)
               res = ang;
             break;
          }
     }

   return res;
}

static void
_sensor_rotation_changed_cb(unsigned int         event_type,
                            sensor_event_data_t *event,
                            void                *data)
{
    enum accelerometer_rotate_state state;
    E_Manager *m = e_manager_current_get();
    E_Zone *zone = e_util_zone_current_get(m);
    int ang = 0;

    if (rot.lock) return;
    if (!zone) return;
    if (event_type != ACCELEROMETER_EVENT_ROTATION_CHECK) return;
    if (!event) return;

    state = *((enum accelerometer_rotate_state*)(event->event_data));

    ang = _ang_get(state);

    SECURE_SLOGD("[ROTATION] SENSOR ROT_CHANGE, state:%d angle:%d", state, ang);
    ELBF(ELBT_ROT, 0, 0, "ROT_EV state:%d angle:%d", state, ang);

    e_zone_rotation_set(zone, ang);

    rot.state = state;
    _sensor_rotation_set(ang);
}

static void
_vconf_cb_lock_change(keynode_t *node,
                      void      *data)
{
   E_Manager *m = NULL;
   E_Zone *zone = NULL;
   int lock = 0, z_ang = -1, ang = -1;
   Eina_Bool res = EINA_FALSE;
   if (!node)
     {
        ELB(ELBT_ROT, "ERR! node is NULL", 0);
        return;
     }

   m = e_manager_current_get();
   if (m) zone = e_util_zone_current_get(m);

   lock = !vconf_keynode_get_bool(node);
   ELBF(ELBT_ROT, 0, 0, "ROT LOCK: %d->%d", rot.lock, lock);

   if (lock)
     {
        // disconnect sensor for reducing the current sinking.
        _sensor_disconnect();
        if (zone) e_zone_rotation_set(zone, 0);
        rot.state = ROTATION_EVENT_0;
     }
   else
     {
        // connect sensor for auto rotation.
        res = _sensor_connect();
        ELB(ELBT_ROT, "_sensor_connect() res", res);
        if (res)
          {
             enum accelerometer_rotate_state state;
             if (sf_check_rotation(&state) < 0)
               {
                  ELB(ELBT_ROT, "ERR! getting rotation failed", state);
               }
             else
               {
                  ang = _ang_get(state);
                  if (zone) z_ang = e_zone_rotation_get(zone);
                  if ((ang != -1) && (ang != z_ang))
                    {
                       if (zone) e_zone_rotation_set(zone, ang);
                       rot.state = state;
                       _sensor_rotation_set(ang);
                    }
               }
          }
     }

   rot.lock = lock;
}

static void
_sensor_rotation_set(int ang)
{
   Ecore_X_Window root = ecore_x_window_root_first_get();
   unsigned int val = (unsigned int)ang;

   if (!ATOM_DEVICE_ROTATION_ANGLE)
     ATOM_DEVICE_ROTATION_ANGLE = ecore_x_atom_get("_E_DEVICE_ROTATION_ANGLE");

   ecore_x_window_prop_card32_set(root,
                                  ATOM_DEVICE_ROTATION_ANGLE,
                                  &val, 1);
}
#endif
