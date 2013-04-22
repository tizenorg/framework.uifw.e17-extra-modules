#ifndef __E_MOD_MAIN_H__
#define __E_MOD_MAIN_H__

#include "e.h"
#include "e_randr.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XKB.h>

#include "scrnconf_devicemgr.h"
#include "virt_monitor_devicemgr.h"
#include "hib_devicemgr.h"
#include "e_mod_scrnconf.h"
#include "e_mod_drv.h"

#ifndef LockMask
#define LockMask (1<<1)
#define Mod2Mask (1<<4)
#endif
#define CapsLockMask LockMask
#define NumLockMask Mod2Mask

#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2 ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3 ((1 << 16) | 3)
#endif

#ifndef  E_RANDR_NO_12
#define E_RANDR_NO_12      (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12)
#endif

#ifndef MAX_TOUCH
#define MAX_TOUCH 3
#endif

#define INSIDE(x, y, x1, y1, x2, y2)	(x1 <= x && x <= x2 && y1 <= y && y <= y2)

#define E_PROP_DEVICE_LIST "E_DEVICEMGR_DEVICE_LIST"

#define E_PROP_DEVICE_NAME "Device"
#define E_PROP_DEVICEMGR_INPUTWIN "DeviceMgr Input Window"
#define E_PROP_VIRTUAL_TOUCHPAD_INT "Virtual Touchpad Interaction"
#define E_PROP_X_MOUSE_CURSOR_ENABLE "X Mouse Cursor Enable"
#define E_PROP_X_MOUSE_EXIST "X Mouse Exist"
#define E_PROP_X_EXT_KEYBOARD_EXIST "X External Keyboard Exist"
#define E_PROP_HW_KEY_INPUT_STARTED "HW Keyboard Input Started"
#define E_PROP_X_EVDEV_AXIS_LABELS "Axis Labels"
#define E_PROP_XRROUTPUT "X_RR_PROPERTY_REMOTE_CONTROLLER"
#define E_PROP_VIRTUAL_TOUCHPAD "_X_Virtual_Touchpad_"
#define E_PROP_TOUCH_INPUT "X_TouchInput"
#define E_PROP_VIRTUAL_TOUCHPAD_CONFINE_REGION "Evdev Confine Region"
#define E_NEW_MASTER_NAME "New Master"
#define E_VIRTUAL_TOUCHPAD_NAME "Virtual Touchpad"
#define EVDEVMULTITOUCH_PROP_TRANSFORM "EvdevMultitouch Transform Matrix"
#define XATOM_FLOAT "FLOAT"

#define DEVICEMGR_PREFIX "/usr/lib/enlightenment/modules/e17-extra-modules-devicemgr/"

typedef enum _VirtualTouchpad_MsgType
{
   E_VIRTUAL_TOUCHPAD_NEED_TO_INIT,
   E_VIRTUAL_TOUCHPAD_DO_INIT,
   E_VIRTUAL_TOUCHPAD_AREA_INFO,
   E_VIRTUAL_TOUCHPAD_POINTED_WINDOW,
   E_VIRTUAL_TOUCHPAD_WINDOW,
   E_VIRTUAL_TOUCHPAD_MT_BEGIN,
   E_VIRTUAL_TOUCHPAD_MT_END,
   E_VIRTUAL_TOUCHPAD_MT_MATRIX_SET_DONE,
   E_VIRTUAL_TOUCHPAD_CONFINE_SET,
   E_VIRTUAL_TOUCHPAD_CONFINE_UNSET,
   E_VIRTUAL_TOUCHPAD_SHUTDOWN
} VirtualTouchpad_MsgType;

typedef enum
{
   E_DEVICEMGR_HWKEY= 1,
   E_DEVICEMGR_KEYBOARD,
   E_DEVICEMGR_MOUSE,
   E_DEVICEMGR_TOUCHSCREEN
} DeviceMgrDeviceType;

typedef struct _DeviceMgr_Device_Info DeviceMgr_Device_Info;

struct _DeviceMgr_Device_Info
{
   int id;
   const char *name;
   DeviceMgrDeviceType type;
};

typedef struct _DeviceMgr_
{
   Ecore_X_Display* disp;
   Ecore_X_Window rootWin;
   Ecore_X_Window input_window;
   int num_zones;
   Eina_List *zones;

   /* scrn conf configuration */
   Eina_Bool scrnconf_enable;
   Utilx_Scrnconf_Dispmode default_dispmode;
   Eina_Bool isPopUpEnabled;

   Eina_Bool xkb_available;

   /* scrn conf preferred size */
   int hdmi_preferred_w;
   int hdmi_preferred_h;
   int virtual_preferred_w;
   int virtual_preferred_h;

   Ecore_X_Atom atomRROutput;
   Ecore_X_Atom atomAxisLabels;
   Ecore_X_Atom atomXMouseExist;
   Ecore_X_Atom atomXMouseCursorEnable;
   Ecore_X_Atom atomXExtKeyboardExist;
   Ecore_X_Atom atomHWKbdInputStarted;
   Ecore_X_Atom atomDeviceList;
   Ecore_X_Atom atomDeviceName;
   Ecore_X_Atom atomVirtualTouchpadConfineRegion;
   Ecore_X_Atom atomVirtualTouchpad;
   Ecore_X_Atom atomTouchInput;
   Ecore_X_Atom atomInputTransform;
   Ecore_X_Atom atomFloat;
   Ecore_X_Atom atomVirtualTouchpadInt;
   Ecore_X_Atom atomDeviceMgrInputWindow;

   /* scrn conf atoms */
   Ecore_X_Atom atomScrnConfDispModeSet;
   Ecore_X_Atom atomVirtMonReq;
   Ecore_X_Atom atomHibReq;

   /* devicemgr config atom */
   Ecore_X_Atom atomDevMgrCfg;

   int num_touchscreen_devices;
   int num_pointer_devices;
   int num_keyboard_devices;
   int num_hwkey_devices;
   Eina_List *device_list;

   Ecore_Event_Handler *window_property_handler;
   Ecore_Event_Handler *event_generic_handler;
   Ecore_Event_Handler *zone_add_handler;
   Ecore_Event_Handler *zone_del_handler;
   Ecore_Event_Handler *mouse_in_handler;
   Ecore_Event_Handler *randr_crtc_handler;
   Ecore_Event_Handler *randr_output_handler;
   Ecore_Event_Handler *randr_output_property_handler;
   Ecore_Event_Handler *client_message_handler;
   E_Border_Hook *border_move_end_hook;
   E_Border_Hook *border_resize_end_hook;

   //variables to set XRROutputProperty
   RROutput output;
   char rroutput_buf[256];
   int rroutput_buf_len;

   //variables related to XI2
   int xi2_opcode;
   XIEventMask eventmask;

    //XIMasterPointer id(s)
   int vcp_id;
   int vck_id;
   int vcp_xtest_pointer_id;
   int vck_xtest_keyboard_id;
   int new_master_pointer_id;

   int virtual_touchpad_id;
   int virtual_multitouch_id[MAX_TOUCH];
   int virtual_touchpad_area_info[4];
   int virtual_touchpad_pointed_window_info[4];
   int virtual_multitouch_done;
   int virtual_touchpad_cursor_pos[2];

   Ecore_X_Window virtual_touchpad_window;
   Ecore_X_Window virtual_touchpad_pointed_window;

   //input transform matrix
   float tmatrix[9];
} DeviceMgr;

EAPI extern E_Module_Api e_modapi;

EAPI void* e_modapi_init (E_Module* m);
EAPI int e_modapi_shutdown (E_Module* m);
EAPI int e_modapi_save (E_Module* m);

static int _e_devicemgr_init(void);
static void _e_devicemgr_fini(void);

static int _e_devicemgr_xinput_init(void);

static int _e_devicemgr_cb_window_property(void *data, int ev_type, void *ev);
static int _e_devicemgr_cb_event_generic(void *data, int ev_type, void *event);
static int _e_devicemgr_cb_zone_add(void *data, int ev_type, void *event);
static int _e_devicemgr_cb_zone_del(void *data, int ev_type, void *event);
static Eina_Bool _e_devicemgr_cb_mouse_in(void *data, int type, void *event);
static void _e_devicemgr_hook_border_move_end(void *data, void *border);
static void _e_devicemgr_hook_border_resize_end(void *data, void *border);
static Eina_Bool _e_devicemgr_get_zones(void);
static E_Zone* _e_devicemgr_get_nth_zone(int index);

static void _e_devicemgr_update_input_transform_matrix(Eina_Bool reset);
static void _e_devicemgr_init_transform_matrix(void);
static void _e_devicemgr_init_output(void);
static void _e_devicemgr_init_input(void);
static int _e_devicemgr_marshalize_string (char* buf, int num, char* srcs[]);

static int _e_devicemgr_check_device_type(int deviceid, DeviceMgrDeviceType type, const char* devname);
static void _e_devicemgr_xi2_device_changed_handler(XIDeviceChangedEvent *event);
static void _e_devicemgr_xi2_device_hierarchy_handler(XIHierarchyEvent *event);
static void _e_devicemgr_device_enabled(int id, int type);
static void _e_devicemgr_device_disabled(int id, int type);
static void _e_devicemgr_master_pointer_added(int id);
static void _e_devicemgr_master_pointer_removed(int id);
static void _e_devicemgr_slave_switched(int deviceid, int sourceid);
static void _e_devicemgr_device_changed(int deviceid, int sourceid);
static void _e_devicemgr_enable_mouse_cursor(unsigned int val);
static void _e_devicemgr_set_confine_information(int deviceid, E_Zone *zone, Eina_Bool isset, int region[4], Eina_Bool pointer_warp, Eina_Bool confine);
static void _e_devicemgr_set_mouse_exist(unsigned int val, int propset);
static void _e_devicemgr_set_keyboard_exist(unsigned int val, int is_connected);

static int _e_devicemgr_xkb_init(void);
static int _e_devicemgr_get_lockmodifier_mask(void);
static int _e_devicemgr_xkb_set_on(unsigned int mask);
static int _e_devicemgr_lockmodifier_set(void);

static Eina_Bool _e_devicemgr_create_master_device(char* master_name);
static Eina_Bool _e_devicemgr_remove_master_device(int master_id);
static Eina_Bool _e_devicemgr_detach_slave(int slave_id);
static Eina_Bool _e_devicemgr_reattach_slave(int slave_id, int master_id);
static Eina_Bool _e_devicemgr_virtual_touchpad_helper_enable(Eina_Bool is_enable);
static void _e_devicemgr_virtual_multitouch_helper_init(int deviceid);
static void _e_devicemgr_virtual_multitouch_helper_fini(void);
static void _e_devicemgr_show_device_list(unsigned int val);

Eina_Bool e_mod_sf_rotation_init(void);
Eina_Bool e_mod_sf_rotation_deinit(void);
#endif//__E_MOD_MAIN_H__
