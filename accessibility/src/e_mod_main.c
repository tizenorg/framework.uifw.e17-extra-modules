
#include "e_accessibility_privates.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include <string.h>
#include "dlog.h"
#include <vconf.h>

#define BUS_NAME                "org.tizen.system.deviced"
#define DEVICED_OBJ_PATH        "/Org/Tizen/System/DeviceD/Display"
#define DEVICED_INTERFACE       "org.tizen.system.deviced.display"
#define DARK_SCREEN_MEMBER      "PowerkeyDoublePressed"

char *strcasestr(const char *s, const char *find);
Accessibility e_accessibility;

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Accessibility Module of Window Manager"
};
#define LOG_TAG	"ACCESSIBILITY"

static void
_e_accessibility_vconf_powersaving_cb(keynode_t *node, void *data)
{
    int power_saving_mode = POWER_SAVING_NONE;

    power_saving_mode = vconf_keynode_get_int (node);
    SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] node(%s), power_saving_mode(%d) e_accessibility.PowerSavingMode(%d)!\n",
                    vconf_keynode_get_name(node), power_saving_mode, e_accessibility.PowerSavingMode);

    if( power_saving_mode == POWER_SAVING_NONE )
    {
        if( e_accessibility.PowerSavingMode != POWER_SAVING_NONE )
        {
            e_accessibility.PowerSavingMode = POWER_SAVING_NONE;
            //ecore_x_window_prop_card32_set(e_accessibility.rootWin, e_accessibility.atomHighContrast, (unsigned int *)&e_accessibility.HighContrastMode, 1);
            _e_accessibility_disable_feature(E_ACCESSIBILITY_POWER_SAVING);
        }
    }
    else if( power_saving_mode == POWER_SAVING_CUSTOM )
    {
        if( e_accessibility.PowerSavingMode != POWER_SAVING_CUSTOM )
        {
            e_accessibility.PowerSavingMode = POWER_SAVING_CUSTOM;
            //ecore_x_window_prop_card32_set(e_accessibility.rootWin, e_accessibility.atomHighContrast, (unsigned int *)&e_accessibility.HighContrastMode, 1);
            _e_accessibility_enable_feature(E_ACCESSIBILITY_POWER_SAVING);
        }
    }
    else if( power_saving_mode == POWER_SAVING_ULTRA )
    {
        if( e_accessibility.PowerSavingMode != POWER_SAVING_ULTRA )
        {
            e_accessibility.PowerSavingMode = POWER_SAVING_ULTRA;
            //ecore_x_window_prop_card32_set(e_accessibility.rootWin, e_accessibility.atomHighContrast, (unsigned int *)&e_accessibility.HighContrastMode, 1);
            _e_accessibility_enable_feature(E_ACCESSIBILITY_POWER_SAVING);
        }
    }
    else
    {
        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] invalid power_saving_mode(%d).\n",power_saving_mode);
    }
    return;
}

static void
_e_accessibility_vconf_powersaving_custom_cb(keynode_t *node, void *data)
{
    int value = -1;

    value = vconf_keynode_get_bool (node);
    SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] node(%s), value(%d), e_accessibility.PowerSavingMode(%d)!\n",
                    vconf_keynode_get_name(node), value, e_accessibility.PowerSavingMode);

    if( e_accessibility.PowerSavingMode == POWER_SAVING_CUSTOM )
    {
        _e_accessibility_enable_feature(E_ACCESSIBILITY_POWER_SAVING);
    }

    return;
}

EAPI void*
e_modapi_init(E_Module* m)
{
   if( !_e_accessibility_init() )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY", "[e_accessibility] Failed @ _e_accessibility_init()..!\n");
        return NULL;
     }

   e_accessibility.window_property_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, (Ecore_Event_Handler_Cb)_e_accessibility_cb_window_property, NULL);
   if( !e_accessibility.window_property_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_WINDOW_PROPERTY handler\n");

   if (e_accessibility.gesture_supported )
     {
        e_accessibility.gesture_tap_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_TAP, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_tap_handler, NULL);
        e_accessibility.gesture_pan_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_PAN, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_pan_handler, NULL);
        e_accessibility.gesture_pinchrotation_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_pinchrotation_handler, NULL);
        e_accessibility.gesture_hold_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_HOLD, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_hold_handler, NULL);
        e_accessibility.gesture_flick_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_FLICK, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_flick_handler, NULL);
        e_accessibility.randr_output_property_handler = ecore_event_handler_add (ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY, (Ecore_Event_Handler_Cb)_e_accessibility_cb_output_property, NULL);

        if( !e_accessibility.gesture_tap_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_TAP handler\n");
        if( !e_accessibility.gesture_pan_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_PAN handler\n");
        if( !e_accessibility.gesture_pinchrotation_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION handler\n");
        if( !e_accessibility.gesture_hold_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_HOLD handler\n");
        if( !e_accessibility.gesture_flick_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_FLICK handler\n");
        if( !e_accessibility.randr_output_property_handler )	SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility] Failed to add ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY handler\n");
     }

   vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE, _e_accessibility_vconf_powersaving_cb, NULL);
   vconf_notify_key_changed(VCONFKEY_SETAPPL_WINDOW_GRAYTONE, _e_accessibility_vconf_powersaving_custom_cb, NULL);
   vconf_notify_key_changed(VCONFKEY_SETAPPL_WINDOW_FPS, _e_accessibility_vconf_powersaving_custom_cb, NULL);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module* m)
{
   ecore_event_handler_del(e_accessibility.window_property_handler);
   e_accessibility.window_property_handler = NULL;

   if (e_accessibility.gesture_supported )
     {
        if (e_accessibility.gesture_tap_handler) ecore_event_handler_del(e_accessibility.gesture_tap_handler);
        if (e_accessibility.gesture_pan_handler) ecore_event_handler_del(e_accessibility.gesture_pan_handler);
        if (e_accessibility.gesture_pinchrotation_handler) ecore_event_handler_del(e_accessibility.gesture_pinchrotation_handler);
        if (e_accessibility.gesture_hold_handler) ecore_event_handler_del(e_accessibility.gesture_hold_handler);
        if (e_accessibility.gesture_flick_handler) ecore_event_handler_del(e_accessibility.gesture_flick_handler);
        if (e_accessibility.randr_output_property_handler) ecore_event_handler_del(e_accessibility.randr_output_property_handler);

        e_accessibility.gesture_tap_handler = NULL;
        e_accessibility.gesture_pan_handler = NULL;
        e_accessibility.gesture_pinchrotation_handler = NULL;
        e_accessibility.gesture_hold_handler = NULL;
        e_accessibility.gesture_flick_handler = NULL;
        e_accessibility.randr_output_property_handler = NULL;
     }

   _e_accessibility_fini();

   return 1;
}

EAPI int
e_modapi_save(E_Module* m)
{
   /* Do Something */
   return e_mod_accessibility_config_save();
}

static int _e_accessibility_init(void)
{
   int res, ret = 1;

   memset(&e_accessibility, 0, sizeof(Accessibility));

   e_accessibility.disp = ecore_x_display_get();

   if( !e_accessibility.disp )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[32m[e_accessibility] Failed to open display..![0m\n");
        ret = 0;
        goto out;
     }

   _e_accessibility_init_edbus();

   e_accessibility.rootWin = ecore_x_window_root_first_get();

   e_accessibility.accGrabWin = ecore_x_window_new(e_accessibility.rootWin, -1, -1, 1, 1);

   if( !e_accessibility.accGrabWin )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] Failed to create a new window for grabbing gesture event(s) !\n");
        ret = 0;
        goto out;
     }

   e_accessibility.atomZoomUI = ecore_x_atom_get(E_PROP_ACCESSIBILITY_ZOOM_UI);
   e_accessibility.atomHighContrast = ecore_x_atom_get(E_PROP_ACCESSIBILITY_HIGH_CONTRAST);
   e_accessibility.atomRROutput = ecore_x_atom_get(E_PROP_XRROUTPUT);
   e_accessibility.atomInputTransform = ecore_x_atom_get(EVDEVMULTITOUCH_PROP_TRANSFORM);
   e_accessibility.atomFloat = ecore_x_atom_get(XATOM_FLOAT);
   e_accessibility.atomLCDOff = ecore_x_atom_get(E_PROP_ACCESSIBILITY_LCD_OFF);

   e_accessibility.zone = _e_accessibility_get_zone();
   if( !e_accessibility.zone )
     {
       SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] Failed to get zone !\n");
       ret = 0;
       goto out;
     }

   ecore_x_screen_size_get(ecore_x_default_screen_get(),
                           &e_accessibility.ZoomUI.screenWidth,
                           &e_accessibility.ZoomUI.screenHeight);

   if( !e_accessibility.ZoomUI.screenWidth && !e_accessibility.ZoomUI.screenHeight )
     {
        e_accessibility.ZoomUI.screenWidth = DEFAULT_SCREEN_WIDTH;
        e_accessibility.ZoomUI.screenHeight = DEFAULT_SCREEN_HEIGHT;
     }

   e_accessibility.gesture_supported = ecore_x_gesture_supported();
   if (!e_accessibility.gesture_supported)
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] X Gesture Extension is not supported !\n");
     }

   res = _e_accessibility_xinput_init();
   if( !res )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] Failed to initialize XInput Extension !\n");
        ret =0;
        goto out;
     }

   _e_accessibility_init_input();
   _e_accessibility_init_output();

   res = _e_accessibility_get_configuration();

   if( !res )
     {
        SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility] Failed to get configureation from %s.cfg file!\n", E_ACCESSIBILITY_CFG);
        ret =0;
        goto out;
     }

out:
   return ret;
}

static void _e_accessibility_fini(void)
{
   /* shutdown the config subsystem */
   e_mod_accessibility_config_shutdown();

   _e_accessibility_deinit_edbus();
}

static int _e_accessibility_xinput_init(void)
{
   int event, error;
   int major = 2, minor = 0;

   if( !XQueryExtension(e_accessibility.disp, "XInputExtension", &e_accessibility.xi2_opcode, &event, &error) )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] XInput Extension isn't supported.\n");
        goto fail;
     }

   if( XIQueryVersion(e_accessibility.disp, &major, &minor) == BadRequest )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] Failed to query XI version.\n");
        goto fail;
     }

   return 1;

fail:
   e_accessibility.xi2_opcode = -1;
   return 0;
}

static int
_e_accessibility_gesture_tap_handler(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Gesture_Notify_Tap *e = ev;

#ifdef __DEBUG__
   SLOG(LOG_INFO, "ACCESSIBILITY",  "\n[e_accessibility][GE] %d Finger %s Event !\n", e->num_fingers, (e->tap_repeat==2) ? "Double Tap" : "Tap");
   SLOG(LOG_INFO, "ACCESSIBILITY",  "subevent = %d\n", e->subtype);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "num of finger=%d\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "tap repeat=%d\n", e->tap_repeat);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "window=0x%x\n", e->win);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "cx:%d, cy:%d\n", e->cx, e->cy);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "time=%d, interval=%d\n", e->time, e->interval);
#endif

   //Use only double-tap event(s)
   if( e->tap_repeat != 2 )
      return 1;

   if( e_accessibility.isZoomUIEnabled && e->num_fingers == 2 )//toggle zoom in/out
     {
        if( e_accessibility.ZoomUI.status == ZOOM_OUT )
          {
             e_accessibility.ZoomUI.status = ZOOM_IN;
             SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility][tap] current_scale=%.3f\n", e_accessibility.ZoomUI.current_scale);
             if (!_e_accessiblity_gesture_grab_by_event_name("pinchrotation", 2, EINA_TRUE))
              {
                 SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][gesture_tap_handler][Grab] Failed to grab gesture for ZoomUI ! (name=pinchrotation, num_finger=2)\n");
              }
             if (!_e_accessiblity_gesture_grab_by_event_name("pan", 2, EINA_TRUE))
              {
                 SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][gesture_tap_handler][Grab] Failed to grab gesture for ZoomUI ! (name=pan, num_finger=2)\n");
              }
          }
        else
          {
             e_accessibility.ZoomUI.status = ZOOM_OUT;
             SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility][tap] current_scale=1.0f\n");
             if (!_e_accessiblity_gesture_grab_by_event_name("pinchrotation", 2, EINA_FALSE))
              {
                 SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][gesture_tap_handler][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=pinchrotation, num_finger=2)\n");
              }
             if (!_e_accessiblity_gesture_grab_by_event_name("pan", 2, EINA_FALSE))
              {
                 SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][gesture_tap_handler][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=pan, num_finger=2)\n");
              }
          }

        _e_accessibility_ZoomUI_update();
        _e_accessibility_update_input_transform_matrix();
     }

   return 1;
}

static int
_e_accessibility_gesture_pan_handler(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Gesture_Notify_Pan *e = ev;

   int offset_x, offset_y;
   static int retry_count = 0;

#ifdef __DEBUG__
   SLOG(LOG_INFO, "ACCESSIBILITY",  "\n[e_accessibility][GE] %d Finger Pan Event !\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "subevent = %d\n", e->subtype);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "num of finger=%d\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "window=0x%x\n", e->win);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "direction=%d\n", e->direction);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "distance=%d\n", e->distance);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "duration=%d\n", e->duration);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "dx:%d, dy:%d\n", e->dx, e->dy);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "time=%d\n", e->time);
#endif

   //Use only two finger pan event(s)
   //Use only when ZoomUI.status is ZOOM_IN
   if ((e->num_fingers != 2) || (e_accessibility.ZoomUI.status == ZOOM_OUT))
      return 1;

   switch (e->subtype)
     {
      case ECORE_X_GESTURE_BEGIN:
        break;

      case ECORE_X_GESTURE_UPDATE:
        retry_count++;
        if ((e_accessibility.ZoomUI.status != ZOOM_PAN) && (retry_count >= 10)) e_accessibility.ZoomUI.status = ZOOM_PAN;

        offset_x = e_accessibility.ZoomUI.offset_x -e->dx;
        offset_y = e_accessibility.ZoomUI.offset_y -e->dy;

        if (offset_x < 0) offset_x = 0;
        if (offset_y < 0) offset_y = 0;

        if ((offset_x + e_accessibility.ZoomUI.width) >= e_accessibility.ZoomUI.screenWidth)
          offset_x = e_accessibility.ZoomUI.screenWidth - e_accessibility.ZoomUI.width;
        if ((offset_y + e_accessibility.ZoomUI.height) >= e_accessibility.ZoomUI.screenHeight)
          offset_y = e_accessibility.ZoomUI.screenHeight - e_accessibility.ZoomUI.height;

        if (e_accessibility.ZoomUI.offset_x != offset_x || e_accessibility.ZoomUI.offset_y != offset_y)
          {
             e_accessibility.ZoomUI.offset_x = offset_x;
             e_accessibility.ZoomUI.offset_y = offset_y;
          }
        else
          {
             return 1;
          }

        _e_accessibility_ZoomUI_update();
        break;

      case ECORE_X_GESTURE_END:
        e_accessibility.tmatrix[0] = (float)e_accessibility.ZoomUI.current_scale;
        e_accessibility.tmatrix[4] = (float)e_accessibility.ZoomUI.current_scale;
        e_accessibility.tmatrix[2] = (float)e_accessibility.ZoomUI.offset_x * e_accessibility.tmatrix[0]  * (-1);
        e_accessibility.tmatrix[5] = (float)e_accessibility.ZoomUI.offset_y * e_accessibility.tmatrix[4]  * (-1);
        _e_accessibility_update_input_transform_matrix();

        e_accessibility.ZoomUI.status = ZOOM_IN;
        retry_count = 0;

        //update current information to configuration file
        //if if makes overhead, comment on the following function
        _e_accessibility_update_configuration();
        break;

      case ECORE_X_GESTURE_DONE:
        break;
     }

   return 1;
}

static int
_e_accessibility_gesture_pinchrotation_handler(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Gesture_Notify_PinchRotation *e = ev;

   int width, height;
   int offset_x, offset_y;
   int diff_cx, diff_cy;

   static int base_dist;
   static int base_cx;
   static int base_cy;
   static int retry_count = 0;
   static int diff_dist = 0;
   static int acc_evcount = 0;
   static int sign_toggle_count = 0;
   static int prev_diff_dist = 0;
   static double base_scale;
   double diff_scale;

#ifdef __DEBUG__
   SLOG(LOG_INFO, "ACCESSIBILITY",  "\n[e_accessibility][GE] %d Finger pinchrotation Event !\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "subevent = %d\n", e->subtype);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "num of finger=%d\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "window=0x%x\n", e->win);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "zoom=%.3f\n", e->zoom);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "distance=%d\n", e->distance);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "angle=%.3f\n", e->angle);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "cx:%d, cy:%d\n", e->cx, e->cy);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "time=%d\n", e->time);
#endif

   if ((e->num_fingers != 2) || (e_accessibility.ZoomUI.status == ZOOM_OUT))
      return 1;

   switch (e->subtype)
     {
      case ECORE_X_GESTURE_BEGIN:
        base_dist = e->distance;
        base_cx = e->cx;
        base_cy = e->cy;
        base_scale = e_accessibility.ZoomUI.current_scale;
        break;

      case ECORE_X_GESTURE_UPDATE:
        prev_diff_dist = diff_dist;
        diff_dist = e->distance - base_dist;
        diff_cx = ABS(base_cx - e->cx);
        diff_cy = ABS(base_cy - e->cy);
        base_cx = e->cx;
        base_cy = e->cy;
        acc_evcount++;

        if ((e_accessibility.ZoomUI.status == ZOOM_IN) && (retry_count++ < 10))
	   {
	      if (ABS(diff_dist) >= 7) e_accessibility.ZoomUI.status = ZOOM_SCALE_ADJUST;
	      else if ((diff_cx+diff_cy) >= 10) e_accessibility.ZoomUI.status = ZOOM_PAN;
	      return 1;
	   }

        if (((diff_cx+diff_cy) >= 15)) return 1;
        if (ABS(diff_dist) < 5) return 1;
        if ((prev_diff_dist * diff_dist) < 0) sign_toggle_count++;
        if ((acc_evcount < 30) && (acc_evcount >= 10) && (sign_toggle_count >=3))
          {
             e_accessibility.ZoomUI.status = ZOOM_PAN;
             return 1;
          }

        diff_scale = e_accessibility.ZoomUI.scale_factor * (ABS(diff_dist)%e_accessibility.ZoomUI.scale_threshold);
        diff_scale = (double)diff_scale*0.5;

        base_dist = e->distance;
        e_accessibility.ZoomUI.current_scale = base_scale * e->zoom;

        if (e_accessibility.ZoomUI.current_scale < e_accessibility.ZoomUI.min_scale)
          e_accessibility.ZoomUI.current_scale = e_accessibility.ZoomUI.min_scale;
        if (e_accessibility.ZoomUI.current_scale > e_accessibility.ZoomUI.max_scale)
          e_accessibility.ZoomUI.current_scale = e_accessibility.ZoomUI.max_scale;

        width = (int)(e_accessibility.ZoomUI.screenWidth / e_accessibility.ZoomUI.current_scale);
        height = (int)(e_accessibility.ZoomUI.screenHeight / e_accessibility.ZoomUI.current_scale);

        offset_x = e_accessibility.ZoomUI.offset_x + ((e_accessibility.ZoomUI.width - width)/2);// - e->cx;
        offset_y = e_accessibility.ZoomUI.offset_y + ((e_accessibility.ZoomUI.height - height)/2);// - e->cy;

        if (offset_x < 0) offset_x = 0;
        if (offset_y < 0) offset_y = 0;

        if ((offset_x + width) >= e_accessibility.ZoomUI.screenWidth)
          offset_x = e_accessibility.ZoomUI.screenWidth - width;
        if ((offset_y + height) >= e_accessibility.ZoomUI.screenHeight)
          offset_y = e_accessibility.ZoomUI.screenHeight - height;

        if (e_accessibility.ZoomUI.offset_x != offset_x || e_accessibility.ZoomUI.offset_y != offset_y ||
             e_accessibility.ZoomUI.width != width || e_accessibility.ZoomUI.height != height)
          {
             e_accessibility.ZoomUI.offset_x = offset_x;
             e_accessibility.ZoomUI.offset_y = offset_y;
             e_accessibility.ZoomUI.width = width;
             e_accessibility.ZoomUI.height = height;
          }
        else
          {
             return 1;
          }

        _e_accessibility_ZoomUI_update();
        break;

      case ECORE_X_GESTURE_END:
        _e_accessibility_update_input_transform_matrix();

        e_accessibility.ZoomUI.status = ZOOM_IN;
        retry_count = sign_toggle_count = acc_evcount = diff_dist =0;

        //update current information to configuration file
        //if if makes overhead, comment on the following function
        _e_accessibility_update_configuration();
        break;

      case ECORE_X_GESTURE_DONE:
        break;
     }

   return 1;
}

static int
_e_accessibility_gesture_hold_handler(void *data, int ev_type, void *ev)
{
	Ecore_X_Event_Gesture_Notify_Hold *e = ev;
	static Eina_Bool isHeld = EINA_FALSE;

#ifdef __DEBUG__
	   SLOG(LOG_INFO, "ACCESSIBILITY",  "\n[e_accessibility][GE] %d Finger Hold Event !\n", e->num_fingers);
	   SLOG(LOG_INFO, "ACCESSIBILITY",  "subevent = %d\n", e->subtype);
	   SLOG(LOG_INFO, "ACCESSIBILITY",  "num of finger=%d\n", e->num_fingers);
	   SLOG(LOG_INFO, "ACCESSIBILITY",  "window=0x%x\n", e->win);
	   SLOG(LOG_INFO, "ACCESSIBILITY",  "cx:%d, cy:%d\n", e->cx, e->cy);
	   SLOG(LOG_INFO, "ACCESSIBILITY",  "time=%d, holdtime=%d\n", e->time, e->hold_time);
#endif
	   if( (e->num_fingers == 2) && (e->subtype == ECORE_X_GESTURE_BEGIN) )
		 {
			isHeld = EINA_TRUE;
		 }
	   else if( (e->num_fingers == 2) && (isHeld == EINA_TRUE) && (e->subtype == ECORE_X_GESTURE_END) )
		 {
			if( e_accessibility.isZoomUIEnabled && e_accessibility.ZoomUI.status == ZOOM_IN )
			  {
				 e_accessibility.ZoomUI.status = ZOOM_SCALE_ADJUST;
			  }
			isHeld = EINA_FALSE;
		 }
	   return 1;
}

static int
_e_accessibility_gesture_flick_handler(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Gesture_Notify_Flick *e = ev;
   int angle=-1;
   int ret;
   ret = ecore_x_window_prop_card32_get(e_accessibility.rootWin,  ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, &angle, 1);
   if(!ret)
   	{
		SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility][GE] Fail to get property !\n");
		return 0;
   	}
   if( angle < 0)
	{
		SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_accessibility][GE] False get angle !\n");
		return 0;
	}
#ifdef __DEBUG__
   SLOG(LOG_INFO, "ACCESSIBILITY",  "\n[e_accessibility][GE] %d Finger Flick Event !\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "subevent = %d\n", e->subtype);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "num of finger=%d\n", e->num_fingers);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "window=0x%x\n", e->win);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "distance=%d, direction=%d\n", e->distance, e->direction);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "time=%d, duration=%d\n", e->time, e->duration);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "angle=%.1f\n", e->angle);
   SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility][GE] device angle: %d \n", angle);
#endif
   return 1;
}

static void
_e_accessibility_do_screencapture()
{
   XEvent xev;
   Ecore_X_Atom atomXKeyComposition;

   Ecore_X_Window win;
   int revert_to;

   atomXKeyComposition = ecore_x_atom_get("_XKEY_COMPOSITION");
   win = ecore_x_window_focus_get();

   SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility] win=0x%x\n", win);

   if( !win )
   	win = e_accessibility.rootWin;

   ecore_x_client_message32_send(e_accessibility.rootWin, atomXKeyComposition, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
   	124,//power keycode
   	122,//volume down keycode
   	0, 0, 0);
}

static int
_e_accessibility_cb_output_property (void *data, int type, void *ev)
{
    Ecore_X_Event_Randr_Output_Property_Notify *event = (Ecore_X_Event_Randr_Output_Property_Notify *)ev;

    if( type != ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY) return EINA_TRUE;

    if( (e_accessibility.atomLCDOff == event->property) && (e_accessibility.output == event->output) )
      {
	 if( (e_accessibility.isZoomUIEnabled) && (e_accessibility.ZoomUI.status == ZOOM_IN) )
	   {
	      e_accessibility.ZoomUI.status = ZOOM_OUT;

	      if (!_e_accessiblity_gesture_grab_by_event_name("pinchrotation", 2, EINA_FALSE))
	        {
	           SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=pinchrotation, num_finger=2)\n");
	        }
	      if (!_e_accessiblity_gesture_grab_by_event_name("pan", 2, EINA_FALSE))
	        {
	           SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=pan, num_finger=2)\n");
	        }
	      if (!_e_accessiblity_gesture_grab_by_event_name("flick", 0, EINA_TRUE))
	        {
	           SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=flick, num_finger=0)\n");
	        }
	 
	      _e_accessibility_ZoomUI_update();
	      _e_accessibility_update_input_transform_matrix();
	   }
      }

   return EINA_TRUE;
}


static int
_e_accessibility_cb_window_property(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Property *e = ev;

   int res = -1;
   unsigned int ret_val = 0;

   if( !e || e->win != e_accessibility.rootWin )
     {
        return 1;
     }

   //check and enable/disable accessibility feature(s)
   if( e->atom == e_accessibility.atomZoomUI )	//Zoom UI
     {
        res = ecore_x_window_prop_card32_get(e->win, e_accessibility.atomZoomUI, &ret_val, 1);

        if( res != 1 )
           goto out;

	if( ret_val != e_accessibility.isZoomUIEnabled )
          {
             e_accessibility.isZoomUIEnabled = ret_val;
             if( e_accessibility.isZoomUIEnabled )//enable
               {
                  _e_accessibility_enable_feature(E_ACCESSIBILITY_ZOOM_UI);
               }
             else//disable
               {
                  _e_accessibility_disable_feature(E_ACCESSIBILITY_ZOOM_UI);
               }
          }
     }
   else if( e->atom == e_accessibility.atomHighContrast )//High Contrast
     {
        res = ecore_x_window_prop_card32_get(e->win, e_accessibility.atomHighContrast, &ret_val, 1);

        if( res != 1 )
           goto out;

        SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility] ret_val(%d) ,e_accessibility.HighContrastMode(%d)\n", ret_val ,e_accessibility.HighContrastMode);
        if( ret_val != e_accessibility.HighContrastMode )
          {
             if( ret_val > HIGH_CONTRAST_EMERGENCY )
                 ret_val = HIGH_CONTRAST_NONE;

             e_accessibility.HighContrastMode = ret_val;
             if( e_accessibility.HighContrastMode )//enable
               {
                  _e_accessibility_enable_feature(E_ACCESSIBILITY_HIGH_CONTRAST);
               }
             else//disable
               {
                  _e_accessibility_disable_feature(E_ACCESSIBILITY_HIGH_CONTRAST);
               }
          }
     }
out:
   return 1;
}

static void
_e_accessibility_enable_feature(AccessibilityFeatureType featureType)
{
	switch( featureType )
	{
		case E_ACCESSIBILITY_ZOOM_UI:
			_e_accessibility_update_configuration();
			_e_accessibility_feature_grab_gestures(E_ACCESSIBILITY_ZOOM_UI, EINA_TRUE);
			break;
		case E_ACCESSIBILITY_HIGH_CONTRAST:
			_e_accessibility_HighContrast_update(1);
			_e_accessibility_update_configuration();
			break;
		case E_ACCESSIBILITY_POWER_SAVING:
			_e_accessibility_PowerSaving_update(1);
			_e_accessibility_update_configuration();
			break;
		case E_ACCESSIBILITY_DARK_SCREEN:
			_e_accessibility_DarkScreen_update(1);
			_e_accessibility_update_configuration();
			break;
		default:
			SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility] Unknown feature type (=%d)\n", (int)featureType);
     }
}

static void
_e_accessibility_disable_feature(AccessibilityFeatureType featureType)
{
   switch( featureType )
     {
      case E_ACCESSIBILITY_ZOOM_UI:
         e_accessibility.isZoomUIEnabled = 0;
         if( e_accessibility.ZoomUI.status == ZOOM_IN )
           {
              e_accessibility.ZoomUI.status = ZOOM_OUT;
              _e_accessibility_ZoomUI_update();
              _e_accessibility_update_input_transform_matrix();
			  if (!_e_accessiblity_gesture_grab_by_event_name("flick", 0, EINA_TRUE))
              {
                 SLOG(LOG_WARN, "ACCESSIBILITY", "[e_acc][gesture_tap_handler][Grab] Failed to grab gesture for ZoomUI ! (name=flick, num_finger=0)\n");
              }
           }
         _e_accessibility_update_configuration();
         _e_accessibility_feature_grab_gestures(E_ACCESSIBILITY_ZOOM_UI, EINA_FALSE);
         break;

      case E_ACCESSIBILITY_HIGH_CONTRAST:
         _e_accessibility_HighContrast_update(0);
         _e_accessibility_update_configuration();
         break;
      case E_ACCESSIBILITY_POWER_SAVING:
         _e_accessibility_PowerSaving_update(0);
         _e_accessibility_update_configuration();
         break;
      case E_ACCESSIBILITY_DARK_SCREEN:
         _e_accessibility_DarkScreen_update(0);
         _e_accessibility_update_configuration();
         break;
      default:
         SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility] Unknown feature type (=%d)\n", (int)featureType);
     }
}

static void
_e_accessibility_ZoomUI_update(void)
{
   if( e_accessibility.ZoomUI.status == ZOOM_IN )
     {
        _e_accessibility_zoom_in();
     }
   else if( e_accessibility.ZoomUI.status == ZOOM_OUT )
     {
        _e_accessibility_zoom_out();
     }
   else
     {
        _e_accessibility_zoom_in();
     }
}

static void
_e_accessibility_PowerSaving_update(Eina_Bool enable)
{
   int is_gray_tone=0;
   int is_fps=0;
   if( enable )
     {
        if( e_accessibility.PowerSavingMode == POWER_SAVING_CUSTOM )
        {
           vconf_get_bool(VCONFKEY_SETAPPL_WINDOW_GRAYTONE, &is_gray_tone);
           vconf_get_bool(VCONFKEY_SETAPPL_WINDOW_FPS, &is_fps);
           SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] e_accessibility.PowerSavingMode = custom! is_gray_tone(%d) is_fps(%d) \n", is_gray_tone, is_fps);
           if(is_gray_tone == 1 && is_fps ==1)
           {
              char* cmds[] = {"e_accessibility", "accessibility", "-m", "3", NULL };
              e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
           }
           else if(is_gray_tone == 1 && is_fps ==0)
           {
              char* cmds[] = {"e_accessibility", "accessibility", "-m", "1", NULL };
              e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
           }
           else if(is_gray_tone == 0 && is_fps ==1)
           {
              char* cmds[] = {"e_accessibility", "accessibility", "-m", "2", NULL };
              e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
           }
           else if(is_gray_tone == 0 && is_fps ==0)
           {
              char* cmds[] = {"e_accessibility", "accessibility", "-m", "0", NULL };
              e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
           }
        }
        else if( e_accessibility.PowerSavingMode == POWER_SAVING_ULTRA )
        {
           char* cmds[] = {"e_accessibility", "accessibility", "-m", "3", NULL };
           e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
        }

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility] PowerSaving On(mode:%d) !\n",e_accessibility.PowerSavingMode);

     }
   else
     {
        char* cmds[] = {"e_accessibility", "accessibility", "-m", "0", NULL };
        e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] PowerSaving Off !\n");
     }
}

static void
_e_accessibility_HighContrast_update(Eina_Bool enable)
{
   if( enable )
     {
        if( e_accessibility.HighContrastMode == HIGH_CONTRAST_NEGATIVE )
        {
           char* cmds[] = {"e_accessibility", "accessibility", "-n", "1", NULL };
           e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
        }
        else if( e_accessibility.HighContrastMode == HIGH_CONTRAST_EMERGENCY )
        {
           char* cmds[] = {"e_accessibility", "accessibility", "-n", "2", NULL };
           e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);
        }

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility] High Contrast On !\n");
     }
   else
     {
        char* cmds[] = {"e_accessibility", "accessibility", "-n", "0", NULL };
        e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] High Contrast Off !\n");
     }
}

static void
_e_accessibility_DarkScreen_update(Eina_Bool enable)
{
   if( enable )
     {
       char* cmds[] = {"e_accessibility", "accessibility", "-d", "1", NULL };
       e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        SLOG(LOG_INFO, "ACCESSIBILITY",  "[e_accessibility] Dark screen On !\n");
     }
   else
     {
        char* cmds[] = {"e_accessibility", "accessibility", "-d", "0", NULL };
        e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] Dark screen Off !\n");
     }
}

static int
_e_accessibility_get_configuration(void)
{

   if( !e_mod_accessibility_config_init() )
     {
        SLOG(LOG_WARN, "ACCESSIBILITY",  "[e_acc][get_configuration] Failed @ e_mod_accessibility_config_init()..!\n");
        return 0;
     }

   e_accessibility.ZoomUI.scale_factor = _e_accessibility_cfg->ZoomUI.scale_factor;
   e_accessibility.ZoomUI.scale_threshold = _e_accessibility_cfg->ZoomUI.scale_threshold;
   e_accessibility.ZoomUI.max_scale = _e_accessibility_cfg->ZoomUI.max_scale;
   e_accessibility.ZoomUI.min_scale = _e_accessibility_cfg->ZoomUI.min_scale;
   e_accessibility.ZoomUI.current_scale = _e_accessibility_cfg->ZoomUI.current_scale;

   e_accessibility.ZoomUI.width = _e_accessibility_cfg->ZoomUI.width;
   e_accessibility.ZoomUI.height = _e_accessibility_cfg->ZoomUI.height;
   e_accessibility.ZoomUI.offset_x = _e_accessibility_cfg->ZoomUI.offset_x;
   e_accessibility.ZoomUI.offset_y = _e_accessibility_cfg->ZoomUI.offset_y;

   memset(e_accessibility.tmatrix, 0, sizeof(e_accessibility.tmatrix));
   e_accessibility.tmatrix[8] = 1.0f;
   e_accessibility.tmatrix[0] = (float)e_accessibility.ZoomUI.current_scale;
   e_accessibility.tmatrix[4] = (float)e_accessibility.ZoomUI.current_scale;
   e_accessibility.tmatrix[2] = (float)e_accessibility.ZoomUI.offset_x * e_accessibility.tmatrix[0]  * (-1);
   e_accessibility.tmatrix[5] = (float)e_accessibility.ZoomUI.offset_y * e_accessibility.tmatrix[4]  * (-1);

   e_accessibility.isZoomUIEnabled = _e_accessibility_cfg->ZoomUI.isZoomUIEnabled;
   e_accessibility.HighContrastMode = _e_accessibility_cfg->HighContrast.HighContrastMode;
   e_accessibility.PowerSavingMode = _e_accessibility_cfg->PowerSaving.PowerSavingMode;
   e_accessibility.DarkScreenMode = _e_accessibility_cfg->DarkScreen.DarkScreenMode;

   if( e_accessibility.PowerSavingMode )
     {
        _e_accessibility_enable_feature(E_ACCESSIBILITY_POWER_SAVING);
     }

   if( e_accessibility.DarkScreenMode )
     {
        _e_accessibility_enable_feature(E_ACCESSIBILITY_DARK_SCREEN);
     }

   if( e_accessibility.atomZoomUI )
     {
        ecore_x_window_prop_card32_set(e_accessibility.rootWin, e_accessibility.atomZoomUI, (unsigned int *)&e_accessibility.isZoomUIEnabled, 1);
        if( e_accessibility.isZoomUIEnabled)
          {
             _e_accessibility_feature_grab_gestures(E_ACCESSIBILITY_ZOOM_UI, EINA_TRUE);
          }
     }

   if( e_accessibility.atomHighContrast )
     {
        ecore_x_window_prop_card32_set(e_accessibility.rootWin, e_accessibility.atomHighContrast, (unsigned int *)&e_accessibility.HighContrastMode, 1);
        if( e_accessibility.HighContrastMode )
          {
             _e_accessibility_enable_feature(E_ACCESSIBILITY_HIGH_CONTRAST);
          }
     }

   return 1;
}

static int
_e_accessibility_update_configuration(void)
{
   _e_accessibility_cfg->ZoomUI.scale_factor = e_accessibility.ZoomUI.scale_factor;
   _e_accessibility_cfg->ZoomUI.scale_threshold = e_accessibility.ZoomUI.scale_threshold;
   _e_accessibility_cfg->ZoomUI.max_scale = e_accessibility.ZoomUI.max_scale;
   _e_accessibility_cfg->ZoomUI.min_scale = e_accessibility.ZoomUI.min_scale;
   _e_accessibility_cfg->ZoomUI.current_scale = e_accessibility.ZoomUI.current_scale;

   _e_accessibility_cfg->ZoomUI.width = e_accessibility.ZoomUI.width;
   _e_accessibility_cfg->ZoomUI.height = e_accessibility.ZoomUI.height;
   _e_accessibility_cfg->ZoomUI.offset_x = e_accessibility.ZoomUI.offset_x;
   _e_accessibility_cfg->ZoomUI.offset_y = e_accessibility.ZoomUI.offset_y;

   _e_accessibility_cfg->ZoomUI.isZoomUIEnabled = e_accessibility.isZoomUIEnabled;
   _e_accessibility_cfg->HighContrast.HighContrastMode = e_accessibility.HighContrastMode;
   _e_accessibility_cfg->PowerSaving.PowerSavingMode = e_accessibility.PowerSavingMode;
   _e_accessibility_cfg->DarkScreen.DarkScreenMode = e_accessibility.DarkScreenMode;

   e_mod_accessibility_config_save();

   return 1;
}

static void _e_accessibility_feature_grab_gestures(AccessibilityFeatureType featureType, Eina_Bool isGrab)
{
   Eina_List *l;
   E_Accessibility_Config_Grab *cgz;

   switch( featureType )
     {
      case E_ACCESSIBILITY_ZOOM_UI:
         EINA_LIST_FOREACH(_e_accessibility_cfg->ZoomUI.grabs, l, cgz)
           {
              if (cgz)
                {
                   if (!_e_accessiblity_gesture_grab_by_event_name(cgz->event_name, cgz->num_finger, isGrab))
                     {
                        SLOG(LOG_WARN, "ACCESSIBILITY", "[e_acc][feature_grab_gestures][%s] Failed to grab gesture for ZoomUI ! (name=%s, num_finger=%d)\n",
                           (isGrab ? "Grab" : "Ungrab"), cgz->event_name, cgz->num_finger);
                     }
                }
           }
         break;

      default:
         SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility] Unknown feature type (=%d)\n", (int)featureType);
     }
}

static Eina_Bool _e_accessiblity_gesture_grab_by_event_name(const char *event_name, const int num_finger, Eina_Bool isGrab)
{
   Ecore_X_Gesture_Event_Type eventType;

    if( strcasestr(event_name, "pan") )
     {
        eventType = ECORE_X_GESTURE_EVENT_PAN;
        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] %d Pan gesture will be (un)grabbed !\n", num_finger);
     }
   else if( strcasestr(event_name, "pinchrotation") )
     {
        eventType = ECORE_X_GESTURE_EVENT_PINCHROTATION;
        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] %d PinchRotation gesture will be (un)grabbed !\n", num_finger);
     }
    else if( strcasestr(event_name, "tap") )
     {
        eventType = ECORE_X_GESTURE_EVENT_TAP;
        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] %d Tap gesture will be (un)grabbed !\n", num_finger);
     }
	else if( strcasestr(event_name, "flick") )
     {
        eventType = ECORE_X_GESTURE_EVENT_FLICK;
        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] %d Flick gesture will be (un)grabbed !\n", num_finger);
     }
	else if( strcasestr(event_name, "hold") )
     {
        eventType = ECORE_X_GESTURE_EVENT_HOLD;
        SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] %d Hold gesture will be (un)grabbed !\n", num_finger);
     }
   else
     {
        SLOG(LOG_WARN, "ACCESSIBILITY", "[e_acc]Unknown event name(=%s)\n", event_name);
        return EINA_FALSE;
     }

   return _e_accessiblity_gesture_grab(eventType, num_finger, isGrab);
}

static Eina_Bool _e_accessiblity_gesture_grab(Ecore_X_Gesture_Event_Type eventType, int num_finger, Eina_Bool isGrab)
{
   Eina_Bool status = EINA_FALSE;

   if (isGrab)
     {
        status = ecore_x_gesture_event_grab(e_accessibility.accGrabWin, eventType, num_finger);

        if (!status)
    	  {
	     SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility]Failed to grab gesture event ! (eventType=%d, num_finger=%d)\n", eventType, num_finger);
    	  }
     }
   else
     {
        status = ecore_x_gesture_event_ungrab(e_accessibility.accGrabWin, eventType, num_finger);

	 if (!status)
 	   {
	      SLOG(LOG_WARN, "ACCESSIBILITY", "[e_accessibility] Failed to ungrab gesture event ! (eventType=%d, num_finger=%d)\n", eventType, num_finger);
 	   }
     }

   return status;
}

static E_Zone* _e_accessibility_get_zone(void)
{
   Eina_List *ml;
   E_Manager *man;
   E_Zone* zone = NULL;

   if (e_accessibility.zone)
      return e_accessibility.zone;

   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        if (man)
          {
             Eina_List *cl;
             E_Container *con;

             EINA_LIST_FOREACH(man->containers, cl, con)
               {
                  if (con)
                    {
                       Eina_List *zl;
                       E_Zone *z;

                       EINA_LIST_FOREACH(con->zones, zl, z)
                         {
                            if (z) zone = z;
                         }
                    }
               }
          }
     }

   return zone;
}

static void _e_accessibility_zoom_in(void)
{
   char info[64];

   sprintf(info, "%dx%d+%d+%d", e_accessibility.ZoomUI.width, e_accessibility.ZoomUI.height,
           e_accessibility.ZoomUI.offset_x, e_accessibility.ZoomUI.offset_y);

   char* cmds[] = {"e_accessibility", "accessibility", "-scale", "1", info, NULL };
   e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 5, cmds);

   XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
   XSync(e_accessibility.disp, False);
}

static void _e_accessibility_zoom_out()
{
   char* cmds[] = {"e_accessibility", "accessibility", "-scale", "0", NULL };
   e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

   XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
   XSync(e_accessibility.disp, False);
}

static int _e_accessibility_marshalize_string (char* buf, int num, char* srcs[])
{
   int i;
   char * p = buf;

   for (i=0; i<num; i++)
     {
        p += sprintf (p, srcs[i]);
        *p = '\0';
        p++;
     }

   *p = '\0';
   p++;

   return (p - buf);
}

static void _e_accessibility_init_output(void)
{
   int i;

   XRRScreenResources* res = XRRGetScreenResources (e_accessibility.disp, e_accessibility.rootWin);
   e_accessibility.output = 0;

   XRROutputInfo *output_info = NULL;

   if( res && (res->noutput != 0) )
   {

       for( i = 0; i < res->noutput; i++ )
       {
           output_info = XRRGetOutputInfo(e_accessibility.disp, res, res->outputs[i]);

           if ((output_info) && !(strcmp(output_info->name, "LVDS1")))
           {
               e_accessibility.output = res->outputs[i];

               if ( output_info )
                   XRRFreeOutputInfo(output_info);
               break;
           }

           if ( output_info )
               XRRFreeOutputInfo(output_info);
       }
   }
   if (res)
     XRRFreeScreenResources(res);

   if( !e_accessibility.output )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] Failed to init output !\n");
     }
}

static void _e_accessibility_init_input(void)
{
   int i, idx = 0;
   int ndevices;
   XIDeviceInfo *dev, *info = NULL;

   if( e_accessibility.xi2_opcode < 0 )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] Failed to initialize input !\n");
        return;
     }

   info = XIQueryDevice(e_accessibility.disp, XIAllDevices, &ndevices);

   if( !info )
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] There is no queried XI device.\n");
        return;
     }

   for( i = 0; i < ndevices ; i++ )
     {
        dev = &info[i];

        if( XISlavePointer == dev->use  || XIFloatingSlave == dev->use )
          {
             //skip XTEST Pointer
             if( strcasestr(dev->name, "XTEST" ) || !strcasestr(dev->name, "touch") )
                continue;

             e_accessibility.touch_deviceid[idx] = dev->deviceid;
             idx++;
          }
     }

   XIFreeDeviceInfo(info);

}

static void _e_accessibility_update_input_transform_matrix(void)
{
   int i;
   static float identity_matrix[] = { 1.0f, 0, 0, 0, 1.0f, 0, 0, 0, 1.0f };

   if( e_accessibility.ZoomUI.status == ZOOM_OUT )
     {
        for( i = 0 ; i < 3 ; i++ )
          {
             XIChangeProperty(e_accessibility.disp, e_accessibility.touch_deviceid[i], e_accessibility.atomInputTransform,
                              e_accessibility.atomFloat, 32, PropModeReplace, (unsigned char*)&identity_matrix, 9);
          }
        XSync(e_accessibility.disp, False);
     }
   else
     {
        for( i = 0 ; i < 3 ; i++ )
          {
             XIChangeProperty(e_accessibility.disp, e_accessibility.touch_deviceid[i], e_accessibility.atomInputTransform,
                              e_accessibility.atomFloat, 32, PropModeReplace, (unsigned char*)&e_accessibility.tmatrix[0], 9);
          }
        XSync(e_accessibility.disp, False);
     }
}

static void _e_accessibility_init_edbus(void)
{
   Eina_Bool ret = EINA_FALSE;

   ret = _e_accessibility_connect_edbus();
   if (!ret)
      SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] _connect_edbus fail!\n");

   return EINA_TRUE;
}

static Eina_Bool _e_accessibility_deinit_edbus(void)
{
   e_dbus_signal_handler_del(e_accessibility.conn, e_accessibility.edbus_dark_screen_handler);
   e_dbus_connection_close(e_accessibility.conn);
   e_dbus_shutdown();

   e_accessibility.conn = NULL;
   e_accessibility.edbus_dark_screen_handler = NULL;
   e_accessibility.retry_count = 0;

   if (e_accessibility.retry_timer)
     {
        ecore_timer_del(e_accessibility.retry_timer);
        e_accessibility.retry_timer = NULL;
     }

   SLOG(LOG_INFO, "ACCESSIBILITY",  "deinit edbus.");

   return EINA_TRUE;
}
static int _e_accessibility_connect_edbus(void)
{
   int ret = 0;
   E_DBus_Connection *conn = NULL;
   static E_DBus_Signal_Handler *edbus_dark_screen_handler = NULL;

   if (e_accessibility.retry_timer)
     {
        ecore_timer_del(e_accessibility.retry_timer);
        e_accessibility.retry_timer = NULL;
     }

   ret = e_dbus_init();
   if (!ret)
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] e_dbus_init error! ret(%d)\n", ret);
        goto con_error;
     }
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn)
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] e_dbus_bus_get error\n", ret);
        goto con_error;
     }
   e_accessibility.conn = conn;

   edbus_dark_screen_handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_OBJ_PATH,
                                                         DEVICED_INTERFACE, DARK_SCREEN_MEMBER,
                                                         (E_DBus_Signal_Cb)_e_accessibility_dark_screen_handler,  NULL);

   if (!edbus_dark_screen_handler)
     {
        SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] e_dbus_signal_handler_add error\n", ret);
        goto con_error;
     }
   e_accessibility.edbus_dark_screen_handler = edbus_dark_screen_handler;

   e_accessibility.retry_count = 0;

   SLOG(LOG_INFO, "ACCESSIBILITY",  "conneted successfully! conn(%p) edbus_dark_screen_handler(%p)",
                          e_accessibility.conn, e_accessibility.edbus_dark_screen_handler);

   return EINA_TRUE;

con_error:
   if (e_accessibility.conn)
       e_dbus_connection_close(e_accessibility.conn);

   if (e_accessibility.retry_count <= 20)
     {
        e_accessibility.retry_timer = ecore_timer_add(10.0f,
                                                        _e_accessibility_retry_edbus_connection,
                                                        NULL);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_accessibility_retry_edbus_connection(void *data)
{
   Eina_Bool res = EINA_FALSE;

   if (e_accessibility.retry_timer)
     {
        ecore_timer_del(e_accessibility.retry_timer);
        e_accessibility.retry_timer = NULL;
     }
   e_accessibility.retry_count++;
   ELBF(ELBT_ROT, 0, 0, "retrying to connect edbus", e_accessibility.retry_count);
   res = _e_accessibility_connect_edbus();
   if (res)
      SLOG(LOG_ERROR, "ACCESSIBILITY",  "[e_accessibility] _connect_edbus fail!\n");

   return ECORE_CALLBACK_CANCEL;
}
static void
_e_accessibility_dark_screen_handler(E_DBus_Object *obj, DBusMessage   *msg)
{
   DBusError err;
   int val;
   int ret;
   int state=1;

   ret = dbus_message_is_signal(msg, DEVICED_INTERFACE, DARK_SCREEN_MEMBER);
   if (!ret)
     {
        ELBF(ELBT_ROT, 0, 0, "dbus_message_is_signal error! ret(%d)", ret);
        return;
     }

   dbus_error_init(&err);

   //SECURE_SLOGD("[e_accessibility] Dark screen mode is changed.");

   if( e_accessibility.DarkScreenMode == 0 )
   {
       e_accessibility.DarkScreenMode = 1;
       _e_accessibility_enable_feature(E_ACCESSIBILITY_DARK_SCREEN);
   }
   else if( e_accessibility.DarkScreenMode == 1 )
   {
       e_accessibility.DarkScreenMode = 0;
       _e_accessibility_disable_feature(E_ACCESSIBILITY_DARK_SCREEN);
   }
   else
   {
       SLOG(LOG_INFO, "ACCESSIBILITY", "[e_accessibility] invalid DarkScreenMode(%d).\n", e_accessibility.DarkScreenMode);
   }

   SLOG(LOG_INFO, "ACCESSIBILITY",  "dark screen mode is changed(%d)", e_accessibility.DarkScreenMode);

   return;
}
