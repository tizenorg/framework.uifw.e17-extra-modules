#ifndef __E_MOD_MAIN_H__
#define __E_MOD_MAIN_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xrandr.h>
#include <utilX.h>

//maximum number of hardkeys
#define MAX_HARDKEYS	255

//grab modes
#define NONE_GRAB_MODE	0
#define OR_EXCL_GRAB_MODE	1
#define EXCL_GRAB_MODE		2
#define TOP_GRAB_MODE		3
#define SHARED_GRAB_MODE	4
#define ALL_GRAB_MODE		10

#define STR_ATOM_DEVICE_STATUS		"_DEVICE_STATUS"
#define STR_ATOM_GRAB_STATUS			"_GRAB_STATUS"
#define PROP_HWKEY_EMULATION			"_HWKEY_EMULATION"

//key composition for screen capture
#define NUM_KEY_COMPOSITION_ACTIONS	2
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

typedef struct _key_event_info
{
	int ev_type;
	int keycode;
	int modkey_index;
} key_event_info;

#if 0
typedef struct _key_comp_action
{
	int type;//property, clientmessage or exec
	union
	{
		struct {
			Ecore_X_Atom atom;
			Ecore_X_Window win;
		} p;
		struct {
			Ecore_X_Atom atom;
			Ecore_X_Window win;
		} c;
		struct {
			char *cmd;
			char *args;
		} e;
	} u;
} key_comp_action;
#endif

typedef struct _ModifierKey
{
	int set;
	int composited;
	int idx_mod;
	int idx_comp;
	Time time;
	Eina_Bool press_only;
	//key_comp_action action;
	kinfo keys[NUM_COMPOSITION_KEY];
} ModifierKey;
typedef struct ModifierKey *ModifierKeyPtr;

#define NUM_HWKEYS		33
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
	KEY_MEDIA,
	KEY_PLAYCD,
	KEY_STOPCD,
	KEY_PAUSECD,
	KEY_NEXTSONG,
	KEY_PREVIOUSSONG,
	KEY_REWIND,
	KEY_FASTFORWARD,
	KEY_PLAYPAUSE,
	KEY_MUTE,
	KEY_HOMEPAGE,
	KEY_WEBPAGE,
	KEY_MAIL,
	KEY_SCREENSAVER,
	KEY_BRIGHTNESSUP,
	KEY_BRIGHTNESSDOWN,
	KEY_SOFTKBD,
	KEY_QUICKPANEL,
	KEY_TASKSWITCH,
	KEY_APPS,
	KEY_SEARCH,
	KEY_VOICE,
	KEY_LANGUAGE
};

typedef enum
{
	E_KEYROUTER_HWKEY= 1,
	E_KEYROUTER_HOTPLUGGED,
	E_KEYROUTER_KEYBOARD,
	E_KEYROUTER_NONE
} KeyrouterDeviceType;

typedef struct _E_Keyrouter_Device_Info E_Keyrouter_Device_Info;

struct _E_Keyrouter_Device_Info
{
	int id;
	const char *name;
	KeyrouterDeviceType type;
};

typedef struct _hwkeymap_info
{
	EINA_INLIST;
	const char* key_name;
	KeySym key_sym;
	int num_keycodes;
	int *keycodes;
} hwkeymap_info;

//global variables will be the member variables of keyrouter structure
typedef struct _tag_keyrouter
{
	Ecore_X_Display* disp;
	Ecore_X_Window rootWin;
	Ecore_X_Window input_window;

	//screen capture related variables
	kinfo cancel_key;
	int modkey_set;
	ModifierKey *modkey;

	E_Zone *zone;

	//number of connected pointer and keyboard devices
	int num_hwkey_devices;

	Eina_List *device_list;
	Eina_List *ignored_key_list;
	Eina_List *waiting_key_list;
	Eina_Inlist *hwkeymap_info_list;

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
	int prev_sent_keycode;

	//atoms
	Atom atomHWKeyEmulation;
	Atom atomGrabKey;
	Atom atomGrabStatus;
	Atom atomDeviceStatus;
	Atom atomGrabExclWin;
	Atom atomGrabORExclWin;

	//event handlers
	Ecore_Event_Handler *e_client_message_handler;
	Ecore_Event_Handler *e_window_property_handler;
	Ecore_Event_Handler *e_border_stack_handler;
	Ecore_Event_Handler *e_border_remove_handler;
	Ecore_Event_Handler *e_window_create_handler;
	Ecore_Event_Handler *e_window_destroy_handler;
	Ecore_Event_Handler *e_window_configure_handler;
	Ecore_Event_Handler *e_window_stack_handler;
	Ecore_Event_Handler *e_event_generic_handler;
	Ecore_Event_Handler *e_event_any_handler;
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
static void _e_keyrouter_grab_hwkeys(int devid);
static void _e_keyrouter_set_key_repeat(int key, int auto_repeat_mode);
static void _e_keyrouter_hwkey_event_handler(XEvent *ev);

//event handlers
static int _e_keyrouter_cb_event_generic(void *data, int ev_type, void *event);
static int _e_keyrouter_cb_event_any(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_property(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_e_border_stack(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_e_border_remove(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_create(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_destroy(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_configure(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_window_stack(void *data, int ev_type, void *ev);
static int _e_keyrouter_cb_client_message (void* data, int type, void* event);
static void _e_keyrouter_xi2_device_hierarchy_handler(XIHierarchyEvent *event);
static Eina_Bool _e_keyrouter_is_waiting_key_list_empty(XEvent *ev);
static Eina_Bool _e_keyrouter_is_key_in_ignored_list(XEvent *ev);
static void _e_keyrouter_device_add(int id, int type);
static void _e_keyrouter_device_remove(int id, int type);
static void _e_keyrouter_update_key_delivery_list(Ecore_X_Window win, int keycode, const int grab_mode, const int IsOnTop);

//e17 bindings functions and action callbacks
static int _e_keyrouter_modifiers(E_Binding_Modifier modifiers);
static void _e_keyrouter_do_bound_key_action(XEvent *xev);

static void DeliverDeviceKeyEvents(XEvent *xev, int replace_key);

static void InitGrabKeyDevices();
static int GrabKeyDevices(Window win);
static void UngrabKeyDevices();

//functions related to mouse rbutton popup
static E_Zone* _e_keyrouter_get_zone();
static void _e_keyrouter_do_hardkey_emulation(const char *label, unsigned int key_event, unsigned int on_release, int keycode, int cancel);

//functions related to key composition for screen capture
static void InitModKeys();
static void ResetModKeyInfo();
static int IsModKey(XEvent *ev, int index);
static int IsCompKey(XEvent *ev, int index);
static int IsKeyComposited(XEvent *ev, int index);
static void DoKeyCompositionAction(int index, int press);

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

