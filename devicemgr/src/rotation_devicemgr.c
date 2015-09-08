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
#include <sensor_internal_deprecated.h>
#include <sensor_auto_rotation.h>
#include <vconf.h>
#include <X11/Xlib.h>
#include "e_devicemgr_privates.h"
#include "rotation_devicemgr.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17_EXTRA_MODULES"

typedef struct _E_DM_Sensor_Rotation E_DM_Sensor_Rotation;

struct _E_DM_Sensor_Rotation
{
   int                             handle;
   Eina_Bool                       started;
   enum auto_rotation_state        state;
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
static int       _ang_get(enum auto_rotation_state state);

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

   h = sf_connect(AUTO_ROTATION_SENSOR);
   if (h < 0)
     {
        ELB(ELBT_ROT, "ERR! sf_connect failed", h);
        goto error;
     }

   r = sf_register_event(h, AUTO_ROTATION_EVENT_CHANGE_STATE,
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
        sf_unregister_event(h, AUTO_ROTATION_EVENT_CHANGE_STATE);
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
                                AUTO_ROTATION_EVENT_CHANGE_STATE);
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
_ang_get(enum auto_rotation_state state)
{
   E_Devicemgr_Config_Rotation *cr = NULL;
   Eina_List *l = NULL;
   int ang = -1, res = -1;

   /* change CW (SensorFW) to CCW(EFL) */
   switch (state)
     {
      case AUTO_ROTATION_DEGREE_0:     ang = 0; break;
      case AUTO_ROTATION_DEGREE_90:    ang = 270; break;
      case AUTO_ROTATION_DEGREE_180:   ang = 180; break;
      case AUTO_ROTATION_DEGREE_270:   ang = 90; break;
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

enum auto_rotation_state
_state_get(int ang)
{
   switch (ang)
     {
      case 0:
         return AUTO_ROTATION_DEGREE_0;
      case 270:
         return AUTO_ROTATION_DEGREE_90;
      case 180:
         return AUTO_ROTATION_DEGREE_180;
      case 90:
         return AUTO_ROTATION_DEGREE_270;
      default:
         return AUTO_ROTATION_DEGREE_UNKNOWN;
     }
}

static void
_sensor_rotation_changed_cb(unsigned int         event_type,
                            sensor_event_data_t *event,
                            void                *data)
{
    int ang = 0;
    enum auto_rotation_state state;
    E_Zone *zone;
    E_Manager *m = e_manager_current_get();

    if (!m) return;
    zone = e_util_zone_current_get(m);

    if (rot.lock) return;
    if (!zone) return;
    if (event_type != AUTO_ROTATION_EVENT_CHANGE_STATE) return;
    if (!event) return;

    state = *((enum auto_rotation_state*)(event->event_data));

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
        rot.state = AUTO_ROTATION_DEGREE_0;
     }
   else
     {
        // connect sensor for auto rotation.
        res = _sensor_connect();
        ELB(ELBT_ROT, "_sensor_connect() res", res);
        if (res)
          {
             sensor_data_t data;
             if (sf_get_data(rot.handle, AUTO_ROTATION_BASE_DATA_SET, &data) < 0)
               {
                  ELB(ELBT_ROT, "ERR! getting rotation failed", NULL);
               }
             else
               {
                  ang = _ang_get(data.values[0]);
                  if (zone) z_ang = e_zone_rotation_get(zone);
                  if ((ang != -1) && (ang != z_ang))
                    {
                       if (zone) e_zone_rotation_set(zone, ang);
                       rot.state = _state_get(ang);
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
