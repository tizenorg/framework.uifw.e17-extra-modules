#ifndef __E_MOD_MAIN_H__
#define __E_MOD_MAIN_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/Xrandr.h>

#define MAX_MT_DEVICES       3
#define EVDEVMULTITOUCH_PROP_TRANSFORM       "EvdevMultitouch Transform Matrix"
#ifndef XATOM_FLOAT
#define XATOM_FLOAT       "FLOAT"
#endif
#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#define E_PROP_ACCESSIBILITY_ZOOM_UI       "_E_ACC_ENABLE_ZOOM_UI_"
#define E_PROP_ACCESSIBILITY_HIGH_CONTRAST       "_E_ACC_ENABLE_HIGH_CONTRAST_"
#define E_PROP_XRROUTPUT       "X_RR_PROPERTY_REMOTE_CONTROLLER"
#define E_PROP_ACCESSIBILITY_LCD_OFF	"LCD_OFF"

#define E_ACCESSIBILITY_FPS_DEFAULT                30
#define E_ACCESSIBILITY_FPS_EMERGENCY              30
#define E_ACCESSIBILITY_FPS_EMERGENCY_ENHANCED     15

typedef int XFixed;
typedef double XDouble;
#define XDoubleToFixed(f)       ((XFixed) ((f) * 65536))
#define XFixedToDouble(f)       (((XDouble) (f)) / 65536)

enum
{
   FLICK_NORTHWARD = 0,
   FLICK_NORTHEASTWARD,
   FLICK_EASTWARD,
   FLICK_SOUTHEASTWARD,
   FLICK_SOUTHWARD,
   FLICK_SOUTHWESTWARD,
   FLICK_WESTWARD,
   FLICK_NORTHWESTWARD
};

typedef enum
{
   E_ACCESSIBILITY_ZOOM_UI = 1,
   E_ACCESSIBILITY_HIGH_CONTRAST,
   E_ACCESSIBILITY_POWER_SAVING,
   E_ACCESSIBILITY_DARK_SCREEN,
} AccessibilityFeatureType;

typedef enum
{
   ZOOM_OUT,
   ZOOM_SCALE_ADJUST,
   ZOOM_OUT_PROGRESS,
   ZOOM_IN,
   ZOOM_IN_PROGRESS,
   ZOOM_PAN
} ZoomUIStatusType;

/**
 *  HIGH_CONTRAST_NONE : ARGB => ARGB
 *  HIGH_CONTRAST_NEGATIVE : ARGB =>  ARGB XOR 0x00FFFFF
 *  HIGH_CONTRAST_EMERGENCY : ARGB => AYYY( Y = R * 0.2126 + G * 0.7152 + B * 0.0722 )
 */
typedef enum
{
   HIGH_CONTRAST_NONE = 0,
   HIGH_CONTRAST_NEGATIVE,
   HIGH_CONTRAST_EMERGENCY,
   HIGH_CONTRAST_EMERGENCY_ENHANCED
} HighContrastModeType;

typedef enum
{
   POWER_SAVING_NONE = 0,
   POWER_SAVING_CUSTOM,
   POWER_SAVING_ULTRA,
   POWER_SAVING_WEARABLE,
   POWER_SAVING_WEARABLE_ENHANCED
} PowerSavingModeType;

typedef struct
{
   int width;
   int height;
   int offset_x;
   int offset_y;
   int screenWidth;
   int screenHeight;
   double max_scale;
   double min_scale;
   double current_scale;
   double scale_factor;
   int scale_threshold;

   ZoomUIStatusType status;
} structZoomUI;

typedef struct _accessibility_
{
   Ecore_X_Display* disp;
   Ecore_X_Window rootWin;
   Ecore_X_Window accGrabWin;
   E_Zone *zone;

   Ecore_X_Atom atomZoomUI;
   Ecore_X_Atom atomHighContrast;
   Ecore_X_Atom atomRROutput;
   Ecore_X_Atom atomInputTransform;
   Ecore_X_Atom atomFloat;
   Ecore_X_Atom atomLCDOff;

   RROutput output;
   char rroutput_buf[256];
   int rroutput_buf_len;

   int isZoomUIEnabled;
   HighContrastModeType HighContrastMode;
   int DarkScreenMode;
   structZoomUI ZoomUI;

   Ecore_Event_Handler *window_property_handler;
   Ecore_Event_Handler *gesture_tap_handler;
   Ecore_Event_Handler *gesture_pan_handler;
   Ecore_Event_Handler *gesture_pinchrotation_handler;
   Ecore_Event_Handler *gesture_flick_handler;
   Ecore_Event_Handler *gesture_hold_handler;
   Ecore_Event_Handler *randr_output_property_handler;

   //XGesture extension related variable(s)
   Eina_Bool gesture_supported;

   //XInput extension related variable(s)
   int xi2_opcode;
   int touch_deviceid[MAX_MT_DEVICES];
   float tmatrix[9];
} Accessibility;

EAPI extern E_Module_Api e_modapi;

EAPI void* e_modapi_init (E_Module* m);
EAPI int e_modapi_shutdown (E_Module* m);
EAPI int e_modapi_save (E_Module* m);

static int _e_accessibility_init(void);
static void _e_accessibility_fini(void);

static int _e_accessibility_xinput_init(void);

static int _e_accessibility_gesture_tap_handler(void *data, int ev_type, void *ev);
static int _e_accessibility_gesture_pan_handler(void *data, int ev_type, void *ev);
static int _e_accessibility_gesture_pinchrotation_handler(void *data, int ev_type, void *ev);
static int _e_accessibility_gesture_hold_handler(void *data, int ev_type, void *ev);
static int _e_accessibility_gesture_flick_handler(void *data, int ev_type, void *ev);
static int _e_accessibility_cb_window_property(void *data, int ev_type, void *ev);
static int _e_accessibility_cb_output_property (void *data, int type, void *ev);

static void _e_accessibility_do_screencapture(void);
static void _e_accessibility_enable_feature(AccessibilityFeatureType featureType);
static void _e_accessibility_disable_feature(AccessibilityFeatureType featureType);

static void _e_accessibility_ZoomUI_update(void);
static void _e_accessibility_HighContrast_update(Eina_Bool enable);
static void _e_accessibility_DarkScreen_update(Eina_Bool enable);

static int _e_accessibility_get_configuration(void);
static int _e_accessibility_update_configuration(void);
static void _e_accessibility_feature_grab_gestures(AccessibilityFeatureType featureType, Eina_Bool isGrab);
static Eina_Bool _e_accessiblity_gesture_grab_by_event_name(const char *event_name, const int num_finger, Eina_Bool isGrab);
static Eina_Bool _e_accessiblity_gesture_grab(Ecore_X_Gesture_Event_Type eventType, int num_finger, Eina_Bool isGrab);

static E_Zone* _e_accessibility_get_zone(void);

static void _e_accessibility_zoom_in(void);
static void _e_accessibility_zoom_out(void);
static int _e_accessibility_marshalize_string (char* buf, int num, char* srcs[]);
static void _e_accessibility_init_output(void);
static void _e_accessibility_init_input(void);

static void _e_accessibility_update_input_transform_matrix(void);

#endif//__E_MOD_MAIN_H__

