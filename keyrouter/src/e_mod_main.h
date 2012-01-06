#ifndef __E_MOD_MAIN_H__
#define __E_MOD_MAIN_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XTest.h>
#include <utilX.h>

//maximum number of hardkeys
#define MAX_HARDKEYS	255

#define POPUP_MENU_WIDTH	95
#define POPUP_MENU_HEIGHT	120

//grab modes
#define NONE_GRAB_MODE	0
#define OR_EXCL_GRAB_MODE	1
#define EXCL_GRAB_MODE		2
#define TOP_GRAB_MODE		3
#define SHARED_GRAB_MODE	4
#define ALL_GRAB_MODE		10

#define STR_ATOM_DEVICE_STATUS		"_DEVICE_STATUS"
#define STR_ATOM_GRAB_STATUS			"_GRAB_STATUS"
#define PROP_X_MOUSE_CURSOR_ENABLE	"X Mouse Cursor Enable"
#define PROP_X_MOUSE_EXIST				"X Mouse Exist"
#define PROP_X_EXT_KEYBOARD_EXIST		"X External Keyboard Exist"
#define PROP_X_EVDEV_AXIS_LABELS		"Axis Labels"

//key composition for screen capture
#define NUM_COMPOSITION_KEY	2
#define KEY_COMPOSITION_TIME 300
#define STR_ATOM_XKEY_COMPOSITION	"_XKEY_COMPOSITION"

#define KEYROUTER_LOG_FILE		"/opt/var/log/keygrab_status.txt"

typedef struct _keylist_node
{
	Window wid;
	struct _keylist_node* next;
} keylist_node;

typedef struct
{
	int keycode;
	char* keyname;
	E_Binding_Key *bind;
	Window lastwid;
	int lastmode;
	keylist_node* or_excl_ptr;
	keylist_node* excl_ptr;
	keylist_node* top_ptr;
	keylist_node* top_tail;
	keylist_node* shared_ptr;
	Window *shared_wins;
	int num_shared_wins;
} GrabbedKey;

//Enumeration for getting KeyClass
enum
{
	INPUTEVENT_KEY_PRESS,
	INPUTEVENT_KEY_RELEASE,
	INPUTEVENT_MAX
};

typedef struct _kinfo
{
	KeySym keysym;
	unsigned int keycode;
} kinfo;

typedef struct _ModifierKey
{
	int set;
	int composited;
	int idx_mod;
	int idx_comp;
	Time time;
	kinfo cancel_key;
	kinfo keys[NUM_COMPOSITION_KEY];
} ModifierKey;

const char *btns_label[] = {
	"Volume Up",
	"Volume Down",
	"Go Home",
	"Rotate"
};

#define NUM_HWKEYS		17
const char *HWKeys[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_CAMERA,
	KEY_CONFIG,
	KEY_POWER,
	KEY_PAUSE,
	KEY_CANCEL,
	KEY_SEND,
	KEY_SELECT,
	KEY_END,
	KEY_PLAYCD,
	KEY_STOPCD,
	KEY_PAUSECD,
	KEY_NEXTSONG,
	KEY_PREVIOUSSONG,
	KEY_REWIND,
	KEY_FASTFORWARD
};

typedef enum
{
	E_KEYROUTER_HWKEY= 1,
	E_KEYROUTER_KEYBOARD,
	E_KEYROUTER_MOUSE,
	E_KEYROUTER_TOUCHSCREEN
} KeyrouterDeviceType;

typedef struct _E_Keyrouter_Device_Info E_Keyrouter_Device_Info;

struct _E_Keyrouter_Device_Info
{
	int id;
	const char *name;
	KeyrouterDeviceType type;
};

//global variables will be the member variables of keyrouter structure
typedef struct _tag_keyrouter
{
	Ecore_X_Display* disp;
	Ecore_X_Window rootWin;

	//screen capture related variables
	ModifierKey modkey;

	//mouse rbutton popup related variables
	int toggle;
	int rbutton_pressed_on_popup;
	int popup_angle;
	int popup_rootx;
	int popup_rooty;

	E_Zone *zone;
	E_Popup     *popup;
	Evas_Object *popup_btns[4];
	Evas_Object* popup_bg;
	unsigned int btn_keys[3];

	//number of connected pointer and keyboard devices
	int num_pointer_devices;
	int num_keyboard_devices;
	int num_hwkey_devices;

	Eina_List *device_list;

	//XInput extension 1 related variables
	int DeviceKeyPress;
	int DeviceKeyRelease;
	int nInputEvent[INPUTEVENT_MAX];

	//XInput extension 2 related variables
	int xi2_opcode;
	XIEventMask eventmask_all;
	XIEventMask eventmask_part;
	XIEventMask eventmask_0;

	GrabbedKey HardKeys[MAX_HARDKEYS];
	int isWindowStackChanged;
	int resTopVisibleCheck;

	struct FILE *fplog;

	//atoms
	Atom atomPointerType;
	Atom atomXMouseExist;
	Atom atomXMouseCursorEnable;
	Atom atomXExtKeyboardExist;
	Atom atomGrabKey;
	Atom atomGrabStatus;
	Atom atomDeviceStatus;
	Atom atomGrabExclWin;
	Atom atomGrabORExclWin;

#ifdef _F_USE_XI_GRABDEVICE_
	XEvent *gev;
	XGenericEventCookie *gcookie;
#endif

	//event handlers
	Ecore_Event_Handler *e_window_property_handler;
	Ecore_Event_Handler *e_border_stack_handler;
	Ecore_Event_Handler *e_border_remove_handler;
	Ecore_Event_Handler *e_window_create_handler;
	Ecore_Event_Handler *e_window_destroy_handler;
	Ecore_Event_Handler *e_window_configure_handler;
	Ecore_Event_Handler *e_window_stack_handler;
#ifdef _F_USE_XI_GRABDEVICE_
	Ecore_Event_Handler *e_event_generic_handler;
#else//_F_USE_XI_GRABDEVICE_
	Ecore_Event_Handler *e_event_generic_handler;
	Ecore_Event_Handler *e_event_any_handler;
#endif//_F_USE_XI_GRABDEVICE_
} KeyRouter;

//function prototypes
EAPI extern E_Module_Api e_modapi;
EAPI void* e_modapi_init (E_Module* m);
EAPI int e_modapi_shutdown (E_Module* m);
EAPI int e_modapi_save (E_Module* m);

static int _e_keyrouter_init();
static void _e_keyrouter_fini();
static void _e_keyrouter_structure_init();
static void _e_keyrouter_bindings_init();
static void _e_keyrouter_x_input_init(void);

//event handlers
#ifdef _F_USE_XI_GRABDEVICE_
static int _e_keyrouter_cb_event_generic(void *data, int ev_type, void *ev);
#else//_F_USE_XI_GRABDEVICE_
static int _e_keyrouter_cb_event_generic(void *data, int ev_type, void *event);
static int _e_keyrouter_cb_event_any(void *data, int ev_type, void *ev);
#endif//_F_USE_XI_GRABDEVICE_
//static int _e_keyrouter_cb_e_border_add(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_property(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_e_border_stack(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_e_border_remove(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_create(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_destroy(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_configure(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_stack(void *data, int ev_type, void *ev);
static void _e_keyrouter_xi2_device_hierarchy_handler(XIHierarchyEvent *event);
static int _e_keyrouter_is_relative_device(int deviceid);
static void _e_keyrouter_device_add(int id, int type);
static void _e_keyrouter_device_remove(int id, int type);
static void _e_keyrouter_set_keyboard_exist(unsigned int val, int is_connected);
static void _e_keyrouter_set_mouse_exist(unsigned int val, int propset);
static void _e_keyrouter_mouse_cursor_enable(unsigned int val);

//e17 bindings functions and action callbacks
static int _e_keyrouter_modifiers(E_Binding_Modifier modifiers);
static void _e_keyrouter_do_bound_key_action(XEvent *xev);

#ifdef _F_USE_XI_GRABDEVICE_
static void DeliverKeyEvents(XEvent *xev, XGenericEventCookie *cookie);
#else//_F_USE_XI_GRABDEVICE_
//static void DeliverDeviceKeyEvents(XEvent *xev);
static void DeliverDeviceKeyEvents(XEvent *xev, int replace_key);
#endif//_F_USE_XI_GRABDEVICE_

static void InitGrabKeyDevices();
#ifdef _F_USE_XI_GRABDEVICE_
static int GrabXIKeyDevices();
static void UngrabXIKeyDevices();
#else//_F_USE_XI_GRABDEVICE_
static int GrabKeyDevices(Window win);
static void UngrabKeyDevices();
#endif//_F_USE_XI_GRABDEVICE_

//functions related to mouse rbutton popup
static void InitHardKeyCodes();
static E_Zone* _e_keyrouter_get_zone();
static void popup_update();
static void popup_show();
static void popup_destroy();
static void _e_keyrouter_do_hardkey_emulation(const char *label, unsigned int key_event, unsigned int on_release);

//functions related to key composition for screen capture
static void InitModKeys();
static void ResetModKeyInfo();
static int IsModKey(XEvent *ev);
static int IsCompKey(XEvent *ev);
static int IsKeyComposited(XEvent *ev);
static void DoKeyCompositionAction();

static void UnSetExclusiveGrabInfoToRootWindow(int keycode, int grab_mode);
static int AdjustTopPositionDeliveryList(Window win, int IsOnTop);
static int AddWindowToDeliveryList(Window win, int keycode, const int grab_mode, const int IsOnTop);
static int RemoveWindowDeliveryList(Window win, int isTopPositionMode, int UnSetExclusiveProperty);
static int GetItemFromWindow(Window win, const char* atom_name, unsigned int **key_list);

static int IsGrabbed(unsigned int keycode);
static void detachSlave(int DeviceID);
static void reattachSlave(int slave, int master);
static void Keygrab_Status(unsigned int val);
static void Device_Status(unsigned int val);
static void PrintKeyDeliveryList();
static void BuildKeyGrabList(Window root);
static int GrabKeyDevice(Window win, const char* DeviceName, const int DeviceID);

#endif//__E_MOD_MAIN_H__

