
#include "e_accessibility_privates.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include <string.h>

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

EAPI void*
e_modapi_init(E_Module* m)
{
   if( !_e_accessibility_init() )
     {
        printf("[e_accessibility][%s] Failed @ _e_accessibility_init()..!\n", __FUNCTION__);
        return NULL;
     }

   e_accessibility.window_property_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, (Ecore_Event_Handler_Cb)_e_accessibility_cb_window_property, NULL);
   if( !e_accessibility.window_property_handler )	printf("[e_accessibility][%s] Failed to add ECORE_X_EVENT_WINDOW_PROPERTY handler\n", __FUNCTION__);

   if (e_accessibility.gesture_supported )
     {
        e_accessibility.gesture_tap_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_TAP, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_tap_handler, NULL);
        e_accessibility.gesture_pan_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_PAN, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_pan_handler, NULL);
        e_accessibility.gesture_pinchrotation_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_pinchrotation_handler, NULL);
        e_accessibility.gesture_hold_handler = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_HOLD, (Ecore_Event_Handler_Cb)_e_accessibility_gesture_hold_handler, NULL);

        if( !e_accessibility.gesture_tap_handler )	printf("[e_accessibility][%s] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_TAP handler\n", __FUNCTION__);
        if( !e_accessibility.gesture_pan_handler )	printf("[e_accessibility][%s] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_PAN handler\n", __FUNCTION__);
        if( !e_accessibility.gesture_pinchrotation_handler )	printf("[e_accessibility][%s] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION handler\n", __FUNCTION__);
        if( !e_accessibility.gesture_hold_handler )	printf("[e_accessibility][%s] Failed to add ECORE_X_EVENT_GESTURE_NOTIFY_HOLD handler\n", __FUNCTION__);
     }

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

        e_accessibility.gesture_tap_handler = NULL;
        e_accessibility.gesture_pan_handler = NULL;
        e_accessibility.gesture_pinchrotation_handler = NULL;
        e_accessibility.gesture_hold_handler = NULL;
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
        fprintf(stderr, "[32m[e_accessibility] Failed to open display..![0m\n");
        ret = 0;
        goto out;
     }

   e_accessibility.rootWin = ecore_x_window_root_first_get();

   e_accessibility.accGrabWin = ecore_x_window_new(e_accessibility.rootWin, -1, -1, 1, 1);

   if( !e_accessibility.accGrabWin )
     {
        fprintf(stderr, "[e_accessibility] Failed to create a new window for grabbing gesture event(s) !\n");
        ret = 0;
        goto out;
     }

   e_accessibility.atomZoomUI = ecore_x_atom_get(E_PROP_ACCESSIBILITY_ZOOM_UI);
   e_accessibility.atomHighContrast = ecore_x_atom_get(E_PROP_ACCESSIBILITY_HIGH_CONTRAST);
   e_accessibility.atomRROutput = ecore_x_atom_get(E_PROP_XRROUTPUT);
   e_accessibility.atomInputTransform = ecore_x_atom_get(EVDEVMULTITOUCH_PROP_TRANSFORM);
   e_accessibility.atomFloat = ecore_x_atom_get(XATOM_FLOAT);

   e_accessibility.zone = _e_accessibility_get_zone();
   if( !e_accessibility.zone )
     {
       fprintf(stderr, "[e_accessibility] Failed to get zone !\n");
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
        fprintf(stderr, "[e_accessibility] X Gesture Extension is not supported !\n");
     }

   res = _e_accessibility_xinput_init();
   if( !res )
     {
        fprintf(stderr, "[e_accessibility] Failed to initialize XInput Extension !\n");
        ret =0;
        goto out;
     }

   _e_accessibility_init_input();
   _e_accessibility_init_output();

   res = _e_accessibility_get_configuration();

   if( !res )
     {
        fprintf(stderr, "[e_accessibility] Failed to get configureation from %s.cfg file!\n", E_ACCESSIBILITY_CFG);
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
}

static int _e_accessibility_xinput_init(void)
{
   int event, error;
   int major = 2, minor = 0;

   if( !XQueryExtension(e_accessibility.disp, "XInputExtension", &e_accessibility.xi2_opcode, &event, &error) )
     {
        printf("[e_accessibility][%s] XInput Extension isn't supported.\n", __FUNCTION__);
        goto fail;
     }

   if( XIQueryVersion(e_accessibility.disp, &major, &minor) == BadRequest )
     {
        printf("[e_accessibility][%s] Failed to query XI version.\n", __FUNCTION__);
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
   fprintf(stderr, "\n[e_accessibility][GE] %d Finger %s Event !\n", e->num_fingers, (e->tap_repeat==2) ? "Double Tap" : "Tap");
   fprintf(stderr, "subevent = %d\n", e->subtype);
   fprintf(stderr, "num of finger=%d\n", e->num_fingers);
   fprintf(stderr, "tap repeat=%d\n", e->tap_repeat);
   fprintf(stderr, "window=0x%x\n", e->win);
   fprintf(stderr, "cx:%d, cy:%d\n", e->cx, e->cy);
   fprintf(stderr, "time=%d, interval=%d\n", e->time, e->interval);
#endif

   //Use only double-tap event(s)
   if( e->tap_repeat != 2 )
      return 1;

   if( e_accessibility.isZoomUIEnabled && e->num_fingers == 2 )//toggle zoom in/out
     {
        if( e_accessibility.ZoomUI.status == ZOOM_OUT )
          {
             e_accessibility.ZoomUI.status = ZOOM_IN;
             fprintf(stderr, "[e_accessibility][tap] current_scale=%.3f\n", e_accessibility.ZoomUI.current_scale);
             if (!_e_accessiblity_gesture_grab_by_event_name("hold", 2, EINA_TRUE))
              {
                 fprintf(stderr, "[e_acc][gesture_tap_handler][Grab] Failed to grab gesture for ZoomUI ! (name=hold, num_finger=2)\n");
              }
             if (!_e_accessiblity_gesture_grab_by_event_name("pan", 2, EINA_TRUE))
              {
                 fprintf(stderr, "[e_acc][gesture_tap_handler][Grab] Failed to grab gesture for ZoomUI ! (name=pan, num_finger=2)\n");
              }
          }
        else
          {
             e_accessibility.ZoomUI.status = ZOOM_OUT;
             fprintf(stderr, "[e_accessibility][tap] current_scale=1.0f\n");
             if (!_e_accessiblity_gesture_grab_by_event_name("hold", 2, EINA_FALSE))
              {
                 fprintf(stderr, "[e_acc][gesture_tap_handler][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=hold, num_finger=2)\n");
              }
             if (!_e_accessiblity_gesture_grab_by_event_name("pan", 2, EINA_FALSE))
              {
                 fprintf(stderr, "[e_acc][gesture_tap_handler][Ungrab] Failed to ungrab gesture for ZoomUI ! (name=pan, num_finger=2)\n");
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

   int width, height;
   int offset_x, offset_y;

   int diff_y;
   static int base_y, cur_y;

   double diff_scale;

#ifdef __DEBUG__
   fprintf(stderr, "\n[e_accessibility][GE] %d Finger Pan Event !\n", e->num_fingers);
   fprintf(stderr, "subevent = %d\n", e->subtype);
   fprintf(stderr, "num of finger=%d\n", e->num_fingers);
   fprintf(stderr, "window=0x%x\n", e->win);
   fprintf(stderr, "direction=%d\n", e->direction);
   fprintf(stderr, "distance=%d\n", e->distance);
   fprintf(stderr, "duration=%d\n", e->duration);
   fprintf(stderr, "dx:%d, dy:%d\n", e->dx, e->dy);
   fprintf(stderr, "time=%d\n", e->time);
#endif

   //Use only two finger pan event(s)
   //Use only when ZoomUI is enabled
   if( e->num_fingers != 2 || !e_accessibility.isZoomUIEnabled )
      return 1;

   //Use only when ZoomUI.status is ZOOM_IN
   if( e_accessibility.ZoomUI.status == ZOOM_OUT )
      return 1;

   switch (e->subtype)
     {
      case ECORE_X_GESTURE_BEGIN:
         cur_y = base_y = e->dy;
         break;

      case ECORE_X_GESTURE_UPDATE:
         if( e_accessibility.ZoomUI.status == ZOOM_SCALE_ADJUST )
           {
              if( 0 == e->dy )
                 return 1;

              cur_y += e->dy;
              diff_y = base_y - cur_y;

              diff_scale = e_accessibility.ZoomUI.scale_factor * (ABS(diff_y)%e_accessibility.ZoomUI.scale_threshold);

              if( diff_scale <= 0 )
                 return 1;

              base_y = cur_y;

              if( diff_y > 0 )//scale up//enlarge zoom area
                {
                   e_accessibility.ZoomUI.current_scale += diff_scale;
                }
              else//scale down//shrink zoom area
                {
                   e_accessibility.ZoomUI.current_scale -= diff_scale;
                }

              if( e_accessibility.ZoomUI.current_scale < e_accessibility.ZoomUI.min_scale )
                 e_accessibility.ZoomUI.current_scale = e_accessibility.ZoomUI.min_scale;
              if( e_accessibility.ZoomUI.current_scale > e_accessibility.ZoomUI.max_scale )
                 e_accessibility.ZoomUI.current_scale = e_accessibility.ZoomUI.max_scale;

              if( e_accessibility.ZoomUI.current_scale < e_accessibility.ZoomUI.min_scale )
                 e_accessibility.ZoomUI.current_scale = e_accessibility.ZoomUI.min_scale;
              else if( e_accessibility.ZoomUI.current_scale > e_accessibility.ZoomUI.max_scale )
                 e_accessibility.ZoomUI.current_scale = e_accessibility.ZoomUI.max_scale;

              width = (int)(e_accessibility.ZoomUI.screenWidth / e_accessibility.ZoomUI.current_scale);
              height = (int)(e_accessibility.ZoomUI.screenHeight / e_accessibility.ZoomUI.current_scale);

              offset_x = e_accessibility.ZoomUI.offset_x + (e_accessibility.ZoomUI.width - width)/2;
              offset_y = e_accessibility.ZoomUI.offset_y + (e_accessibility.ZoomUI.height - height)/2;

              if( offset_x < 0 )
                 offset_x = 0;
              if( offset_y < 0 )
                 offset_y = 0;

              if( (offset_x + width) >= e_accessibility.ZoomUI.screenWidth )
                 offset_x = e_accessibility.ZoomUI.screenWidth - width;
              if( (offset_y + height) >= e_accessibility.ZoomUI.screenHeight )
                 offset_y = e_accessibility.ZoomUI.screenHeight - height;

              if( e_accessibility.ZoomUI.offset_x != offset_x || e_accessibility.ZoomUI.offset_y != offset_y ||
                  e_accessibility.ZoomUI.width != width || e_accessibility.ZoomUI.height != height )
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
           }
         else
           {
              offset_x = e_accessibility.ZoomUI.offset_x -e->dx;
              offset_y = e_accessibility.ZoomUI.offset_y -e->dy;

              if( offset_x < 0 )
                 offset_x = 0;
              if( offset_y < 0 )
                 offset_y = 0;

              if( (offset_x + e_accessibility.ZoomUI.width) >= e_accessibility.ZoomUI.screenWidth )
                 offset_x = e_accessibility.ZoomUI.screenWidth - e_accessibility.ZoomUI.width;
              if( (offset_y + e_accessibility.ZoomUI.height) >= e_accessibility.ZoomUI.screenHeight )
                 offset_y = e_accessibility.ZoomUI.screenHeight - e_accessibility.ZoomUI.height;

              if( e_accessibility.ZoomUI.offset_x != offset_x || e_accessibility.ZoomUI.offset_y != offset_y )
                {
                   e_accessibility.ZoomUI.offset_x = offset_x;
                   e_accessibility.ZoomUI.offset_y = offset_y;
                }
              else
                {
                   return 1;
                }

              _e_accessibility_ZoomUI_update();
           }
         break;

      case ECORE_X_GESTURE_END:
         e_accessibility.tmatrix[0] = (float)e_accessibility.ZoomUI.current_scale;
         e_accessibility.tmatrix[4] = (float)e_accessibility.ZoomUI.current_scale;
         e_accessibility.tmatrix[2] = (float)e_accessibility.ZoomUI.offset_x * e_accessibility.tmatrix[0]  * (-1);
         e_accessibility.tmatrix[5] = (float)e_accessibility.ZoomUI.offset_y * e_accessibility.tmatrix[4]  * (-1);
         _e_accessibility_update_input_transform_matrix();

         //update current information to configuration file
         //if if makes overhead, comment on the following function
         _e_accessibility_update_configuration();

         e_accessibility.ZoomUI.status = ZOOM_IN;
         base_y = cur_y = 0;
         break;

      case ECORE_X_GESTURE_DONE:
         break;
     }

   return 1;
}

static int
_e_accessibility_gesture_pinchrotation_handler(void *data, int ev_type, void *ev)
{
   //Ecore_X_Event_Gesture_Notify_PinchRotation *e = ev;

   return 1;
}

static int
_e_accessibility_gesture_hold_handler(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Gesture_Notify_Hold *e = ev;
   static Eina_Bool isHeld = EINA_FALSE;

#ifdef __DEBUG__
   fprintf(stderr, "\n[e_accessibility][GE] %d Finger Hold Event !\n", e->num_fingers);
   fprintf(stderr, "subevent = %d\n", e->subtype);
   fprintf(stderr, "num of finger=%d\n", e->num_fingers);
   fprintf(stderr, "window=0x%x\n", e->win);
   fprintf(stderr, "cx:%d, cy:%d\n", e->cx, e->cy);
   fprintf(stderr, "time=%d, holdtime=%d\n", e->time, e->hold_time);
#endif

   //Zoom
   if( e->subtype == ECORE_X_GESTURE_BEGIN )
     {
        isHeld = EINA_TRUE;
     }
   else if( isHeld == EINA_TRUE && e->subtype == ECORE_X_GESTURE_END )
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

        if( ret_val != e_accessibility.isHighContrastEnabled )
          {
             e_accessibility.isHighContrastEnabled = ret_val;
             if( e_accessibility.isHighContrastEnabled )//enable
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

      default:
         fprintf(stderr, "[e_accessibility][%s] Unknown feature type (=%d)\n", __FUNCTION__, (int)featureType);
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
           }
         _e_accessibility_update_configuration();
         _e_accessibility_feature_grab_gestures(E_ACCESSIBILITY_ZOOM_UI, EINA_FALSE);
         break;

      case E_ACCESSIBILITY_HIGH_CONTRAST:
         _e_accessibility_HighContrast_update(0);
         _e_accessibility_update_configuration();
         break;

      default:
         fprintf(stderr, "[e_accessibility][%s] Unknown feature type (=%d)\n", __FUNCTION__, (int)featureType);
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
_e_accessibility_HighContrast_update(Eina_Bool enable)
{
   if( enable )
     {
        char* cmds[] = {"e_accessibility", "accessibility", "-n", "1", NULL };
        e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        fprintf(stderr, "[e_accessibility] High Contrast On !\n");
     }
   else
     {
        char* cmds[] = {"e_accessibility", "accessibility", "-n", "0", NULL };
        e_accessibility.rroutput_buf_len = _e_accessibility_marshalize_string (e_accessibility.rroutput_buf, 4, cmds);

        XRRChangeOutputProperty(e_accessibility.disp, e_accessibility.output, e_accessibility.atomRROutput, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)e_accessibility.rroutput_buf, e_accessibility.rroutput_buf_len);
        XSync(e_accessibility.disp, False);

        fprintf(stderr, "[e_accessibility] High Contrast Off !\n");
     }
}

static int
_e_accessibility_get_configuration(void)
{
   if( !e_mod_accessibility_config_init() )
     {
        fprintf(stderr, "[e_acc][get_configuration] Failed @ e_mod_accessibility_config_init()..!\n");
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
   e_accessibility.isHighContrastEnabled = _e_accessibility_cfg->HighContrast.isHighContrastEnabled;

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
        ecore_x_window_prop_card32_set(e_accessibility.rootWin, e_accessibility.atomHighContrast, (unsigned int *)&e_accessibility.isHighContrastEnabled, 1);
        if( e_accessibility.isHighContrastEnabled )
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
   _e_accessibility_cfg->HighContrast.isHighContrastEnabled = e_accessibility.isHighContrastEnabled;

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
                        fprintf(stderr, "[e_acc][feature_grab_gestures][%s] Failed to grab gesture for ZoomUI ! (name=%s, num_finger=%d)\n",
                           (isGrab ? "Grab" : "Ungrab"), cgz->event_name, cgz->num_finger);
                     }
                }
           }
         break;

      default:
         fprintf(stderr, "[e_accessibility][%s] Unknown feature type (=%d)\n", __FUNCTION__, (int)featureType);
     }
}

static Eina_Bool _e_accessiblity_gesture_grab_by_event_name(const char *event_name, const int num_finger, Eina_Bool isGrab)
{
   Ecore_X_Gesture_Event_Type eventType;

   if( strcasestr(event_name, "tapnhold") )
     {
        eventType = ECORE_X_GESTURE_EVENT_TAPNHOLD;
        fprintf(stderr, "[e_accessibility][%s] %d TapNHold gesture will be (un)grabbed !\n", __FUNCTION__, num_finger);
     }
   else if( strcasestr(event_name, "flick") )
     {
        eventType = ECORE_X_GESTURE_EVENT_FLICK;
        fprintf(stderr, "[e_accessibility][%s] %d Flick gesture will be (un)grabbed !\n", __FUNCTION__, num_finger);
     }
   else if( strcasestr(event_name, "pan") )
     {
        eventType = ECORE_X_GESTURE_EVENT_PAN;
        fprintf(stderr, "[e_accessibility][%s] %d Pan gesture will be (un)grabbed !\n", __FUNCTION__, num_finger);
     }
   else if( strcasestr(event_name, "pinchrotation") )
     {
        eventType = ECORE_X_GESTURE_EVENT_PINCHROTATION;
        fprintf(stderr, "[e_accessibility][%s] %d PinchRotation gesture will be (un)grabbed !\n", __FUNCTION__, num_finger);
     }
   else if( strcasestr(event_name, "hold") )
     {
        eventType = ECORE_X_GESTURE_EVENT_HOLD;
        fprintf(stderr, "[e_accessibility][%s] %d Hold gesture will be (un)grabbed !\n", __FUNCTION__, num_finger);
     }
   else if( strcasestr(event_name, "tap") )
     {
        eventType = ECORE_X_GESTURE_EVENT_TAP;
        fprintf(stderr, "[e_accessibility][%s] %d Tap gesture will be (un)grabbed !\n", __FUNCTION__, num_finger);
     }
   else
     {
        fprintf(stderr, "[e_acc][gesture_grab_by_event_name] Unknown event name(=%s)\n", event_name);
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
    	     fprintf(stderr, "[e_accessibility][gesture_grab] Failed to grab gesture event ! (eventType=%d, num_finger=%d)\n", eventType, num_finger);
    	  }
     }
   else
     {
        status = ecore_x_gesture_event_ungrab(e_accessibility.accGrabWin, eventType, num_finger);

	 if (!status)
 	   {
 	      fprintf(stderr, "[e_accessibility][gesture_grab] Failed to ungrab gesture event ! (eventType=%d, num_finger=%d)\n", eventType, num_finger);
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

           if( !strcmp(output_info->name, "LVDS1") )
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

   if( !e_accessibility.output )
     {
        fprintf(stderr, "[e_accessibility][_e_accessibility_init_output] Failed to init output !\n");
     }
}

static void _e_accessibility_init_input(void)
{
   int i, idx = 0;
   int ndevices;
   XIDeviceInfo *dev, *info = NULL;

   if( e_accessibility.xi2_opcode < 0 )
     {
        fprintf(stderr, "[e_accessibility][%s] Failed to initialize input !\n", __FUNCTION__);
        return;
     }

   info = XIQueryDevice(e_accessibility.disp, XIAllDevices, &ndevices);

   if( !info )
     {
        fprintf(stderr, "[e_accessibility][%s] There is no queried XI device.\n", __FUNCTION__);
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
        XIChangeProperty(e_accessibility.disp, e_accessibility.touch_deviceid[0], e_accessibility.atomInputTransform,
                         e_accessibility.atomFloat, 32, PropModeReplace, (unsigned char*)&identity_matrix, 9);
     }
   else
     {
        for( i = 0 ; i < 3 ; i++ )
          {
             XIChangeProperty(e_accessibility.disp, e_accessibility.touch_deviceid[i], e_accessibility.atomInputTransform,
                              e_accessibility.atomFloat, 32, PropModeReplace, (unsigned char*)&e_accessibility.tmatrix[0], 9);
          }
     }
}

