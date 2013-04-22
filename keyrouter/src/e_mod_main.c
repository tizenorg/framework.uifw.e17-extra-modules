#include "e.h"
#include "e_mod_main.h"
#include <string.h>

/* this is needed to advertise a label for the module IN the code (not just
 * the .desktop file) but more specifically the api version it was compiled
 * for so E can skip modules that are compiled for an incorrect API version
 * safely) */
 EAPI E_Module_Api e_modapi =
{
	E_MODULE_API_VERSION,
	"Keyrouter Module of Window Manager"
};

KeyRouter keyrouter;

EAPI void*
e_modapi_init (E_Module* m)
{
	if( !_e_keyrouter_init() )
	{
		printf("[keyrouter][%s] Failed @ _e_keyrouter_init()..!\n", __FUNCTION__);
		return NULL;
	}

	//Adding Event Handlers
#ifdef _F_USE_XI_GRABDEVICE_
	keyrouter.e_event_generic_handler = ecore_event_handler_add(ECORE_X_EVENT_GENERIC, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_event_generic, NULL);
#else//_F_USE_XI_GRABDEVICE_
	keyrouter.e_event_generic_handler = ecore_event_handler_add(ECORE_X_EVENT_GENERIC, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_event_generic, NULL);
	keyrouter.e_event_any_handler = ecore_event_handler_add(ECORE_X_EVENT_ANY, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_event_any, NULL);
#endif//_F_USE_XI_GRABDEVICE_
	//e_border_add_handler = ecore_event_handler_add(E_EVENT_BORDER_ADD, _e_keyrouter_cb_e_border_add, NULL);
	keyrouter.e_window_property_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_window_property, NULL);
	keyrouter.e_border_stack_handler = ecore_event_handler_add(E_EVENT_BORDER_STACK, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_e_border_stack, NULL);
	keyrouter.e_border_remove_handler = ecore_event_handler_add(E_EVENT_BORDER_REMOVE, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_e_border_remove, NULL);
	keyrouter.e_window_create_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_window_create, NULL);
	keyrouter.e_window_destroy_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_window_destroy, NULL);
	keyrouter.e_window_configure_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,(Ecore_Event_Handler_Cb)_e_keyrouter_cb_window_configure, NULL);
	keyrouter.e_window_stack_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_window_stack, NULL);
	keyrouter.e_client_message_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, (Ecore_Event_Handler_Cb)_e_keyrouter_cb_client_message, NULL);

	if( !keyrouter.e_window_stack_handler )			printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_WINDOW_STACK handler\n", __FUNCTION__);
	if( !keyrouter.e_window_configure_handler )		printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_WINDOW_CONFIGURE handler\n", __FUNCTION__);
	if( !keyrouter.e_window_destroy_handler )		printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_WINDOW_DESTROY handler\n", __FUNCTION__);
	if( !keyrouter.e_window_create_handler )		printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_WINDOW_CREATE handler\n", __FUNCTION__);
	//if( !e_border_add_handler )			printf("[keyrouter][%s] Failed to add E_EVENT_BORDER_ADD handler\n", __FUNCTION__);
	if( !keyrouter.e_window_property_handler )		printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_WINDOW_PROPERTY handler\n", __FUNCTION__);
	if( !keyrouter.e_border_stack_handler )			printf("[keyrouter][%s] Failed to add E_EVENT_BORDER_STACK handler\n", __FUNCTION__);
	if( !keyrouter.e_border_remove_handler )		printf("[keyrouter][%s] Failed to add E_EVENT_BORDER_REMOVE handler\n", __FUNCTION__);
#ifdef _F_USE_XI_GRABDEVICE_
	if( !keyrouter.e_event_generic_handler )			printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_GENERIC handler\n", __FUNCTION__);
#else//_F_USE_XI_GRABDEVICE_
	if( !keyrouter.e_event_any_handler )			printf("[keyrouter][%s] Failed to add ECORE_X_EVENT_ANY handler\n", __FUNCTION__);
#endif//_F_USE_XI_GRABDEVICE_

	return m;
}

EAPI int
e_modapi_shutdown (E_Module* m)
{
	//Removing Event Handlers
#ifdef _F_USE_XI_GRABDEVICE_
	ecore_event_handler_del(keyrouter.e_event_generic_handler);
#else//_F_USE_XI_GRABDEVICE_
	ecore_event_handler_del(keyrouter.e_event_generic_handler);
	ecore_event_handler_del(keyrouter.e_event_any_handler);
#endif//_F_USE_XI_GRABDEVICE_
	//ecore_event_handler_del(e_border_add_handler);
	ecore_event_handler_del(keyrouter.e_window_property_handler);
	ecore_event_handler_del(keyrouter.e_border_stack_handler);
	ecore_event_handler_del(keyrouter.e_border_remove_handler);
	ecore_event_handler_del(keyrouter.e_window_create_handler);
	ecore_event_handler_del(keyrouter.e_window_destroy_handler);
	ecore_event_handler_del(keyrouter.e_window_configure_handler);
	ecore_event_handler_del(keyrouter.e_window_stack_handler);

	keyrouter.e_window_stack_handler = NULL;
	keyrouter.e_window_configure_handler = NULL;
	keyrouter.e_window_destroy_handler = NULL;
	keyrouter.e_window_create_handler = NULL;
	//keyrouter.e_border_add_handler = NULL;
	keyrouter.e_window_property_handler = NULL;
	keyrouter.e_border_stack_handler = NULL;
	keyrouter.e_border_remove_handler = NULL;
#ifdef _F_USE_XI_GRABDEVICE_
	keyrouter.e_event_generic_handler = NULL;
#else//_F_USE_XI_GRABDEVICE_
	keyrouter.e_event_any_handler = NULL;
#endif//_F_USE_XI_GRABDEVICE_

	_e_keyrouter_fini();

	return 1;
}

EAPI int
e_modapi_save (E_Module* m)
{
	/* Do Something */
	return 1;
}

static void
_e_keyrouter_x_input_init(void)
{
	int event, error;
	int major = 2, minor = 0;

	if( !XQueryExtension(keyrouter.disp, "XInputExtension", &keyrouter.xi2_opcode, &event, &error) )
	{
		printf("[keyrouter][%s] XInput Extension isn't supported.\n", __FUNCTION__);
		keyrouter.xi2_opcode = -1;
		return;
	}

	if( XIQueryVersion(keyrouter.disp, &major, &minor) == BadRequest )
	{
		printf("[keyrouter][%s] Failed to query XI version.\n", __FUNCTION__);
		keyrouter.xi2_opcode = -1;
		return;
	}

	/* Set up MPX events */
	memset(&keyrouter.eventmask_0, 0L, sizeof(XIEventMask));
	keyrouter.eventmask_all.deviceid = keyrouter.eventmask_part.deviceid = XIAllDevices;
	keyrouter.eventmask_all.mask_len = keyrouter.eventmask_part.mask_len = XIMaskLen(XI_RawMotion);
	keyrouter.eventmask_all.mask = calloc(keyrouter.eventmask_all.mask_len, sizeof(char));
	keyrouter.eventmask_part.mask= calloc(keyrouter.eventmask_part.mask_len, sizeof(char));

	/* Events we want to listen for all */
	XISetMask(keyrouter.eventmask_all.mask, XI_HierarchyChanged);
#ifdef _F_ENABLE_MOUSE_POPUP
	XISetMask(keyrouter.eventmask_all.mask, XI_ButtonPress);
	XISetMask(keyrouter.eventmask_all.mask, XI_ButtonRelease);
#endif//_F_ENABLE_MOUSE_POPUP

	/* Events we want to listen for a part */
	XISetMask(keyrouter.eventmask_part.mask, XI_HierarchyChanged);

	/* select XI events for a part */
	XISelectEvents(keyrouter.disp, keyrouter.rootWin, &keyrouter.eventmask_part, 1);

#ifdef _F_USE_XI_GRABDEVICE_
	/* Set up MPX events */
	memset(&keyrouter.eventmask_0, 0L, sizeof(XIEventMask));

	keyrouter.eventmask.deviceid = XIAllDevices;//or XIAllMasterDevices
	keyrouter.eventmask.mask_len = sizeof(keyrouter.mask);
	keyrouter.eventmask.mask = keyrouter.mask;

	/* Events we want to listen for */
	XISetMask(keyrouter.mask, XI_KeyPress);
	XISetMask(keyrouter.mask, XI_KeyRelease);

	gev = (XEvent *)malloc(sizeof(XEvent));
	gcookie = (XGenericEventCookie *)malloc(sizeof(XGenericEventCookie));

	if( !gev || !gcookie )
	{
		fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for XEvent/XGenericEventCookie !\n", __FUNCTION__);
		return;
	}

	gcookie->type = GenericEvent;
	gcookie->extension = xi2_opcode;
	gcookie->display = gev->xany.display = keyrouter.disp;
#endif
}

#ifndef _F_USE_XI_GRABDEVICE_
static int
_e_keyrouter_cb_event_any(void *data, int ev_type, void *event)
{
	XEvent *ev = (XEvent *)event;
	XDeviceKeyEvent *xdevkey = (XDeviceKeyEvent *)ev;

	int type = ev->xcookie.type;

	if( type == keyrouter.DeviceKeyPress )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] DeviceKeyPress (type = %d, keycode = %d)\n", __FUNCTION__, type, xdevkey->keycode);
#endif
		ev->type = KeyPress;
	}
	else if( type == keyrouter.DeviceKeyRelease )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] DeviceKeyRelease (type = %d, keycode = %d)\n", __FUNCTION__, type, xdevkey->keycode);
#endif
		ev->type = KeyRelease;
	}
#ifdef _F_ENABLE_MOUSE_POPUP
	else if( type == ButtonPress && ev->xbutton.button == 3)
	{
#ifdef __DEBUG__
		fprintf(stderr, "[keyrouter][%s] Mouse Right ButtonPress (type = %d, button=%d, x_root=%d, y_root=%d, x=%d, y=%d)\n", __FUNCTION__, type, ev->xbutton.button,
				ev->xbutton.x_root, ev->xbutton.y_root, ev->xbutton.x, ev->xbutton.y);
#endif
		if( !keyrouter.popup )
		{
			keyrouter.popup_rootx = ev->xbutton.x_root;
			keyrouter.popup_rooty = ev->xbutton.y_root;
			keyrouter.rbutton_pressed_on_popup = 0;
			popup_show();
		}

		return 1;
	}
#endif//_F_ENABLE_MOUSE_POPUP
	else
		return 1;

	ev->xany.display = keyrouter.disp;
	ev->xkey.keycode = xdevkey->keycode;
	ev->xkey.time = xdevkey->time;

#ifdef __DEBUG__
	fprintf(stderr, "[keyrouter][%s] called !\n", __FUNCTION__);
	fprintf(stderr, "[keyrouter][%s] keycode=%d, type=%s\n", __FUNCTION__, ev->xkey.keycode, (ev->type==KeyPress) ? "KeyPress" : "KeyRelease");
#endif

	if(!_e_keyrouter_is_waiting_key_list_empty(ev))
		return 1;

	if(_e_keyrouter_is_key_in_ignored_list(ev))
		return 1;

	_e_keyrouter_hwkey_event_handler(ev);

	return 1;
}
#else//_F_USE_XI_GRABDEVICE_
static int
_e_keyrouter_cb_event_generic(void *data, int ev_type, void *event)
{
	Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event;
	XIDeviceEvent *evData = (XIDeviceEvent *)(e->data);

	if( e->extension != keyrouter.xi2_opcode || ( e->evtype != XI_KeyPress && e->evtype != XI_KeyRelease ))
	{
		fprintf(stderr, "[keyrouter][%s] Invalid event !(extension:%d, evtype:%d)\n", __FUNCTION__, e->extension, e->evtype);
		return 1;
	}

	if( !evData )
	{
		fprintf(stderr, "[keyrouter][%s] e->data is NULL !\n", __FUNCTION__);
		return 1;
	}

	if( evData->send_event )
	{
		return 0;
	}

	keyrouter.gcookie->data = (XIDeviceEvent *)malloc(sizeof(XIDeviceEvent));

	if( !keyrouter.gcookie->data )
	{
		fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for XIDeviceEvent !\n", __FUNCTION__);
		return 1;
	}

#ifdef __DEBUG__
	fprintf(stderr, "[keyrouter][%s] evData->deviceid=%d, evData->sourceid=%d\n", __FUNCTION__, evData->deviceid, evData->sourceid);
#endif

	evData->deviceid = 3;//Virtual Core Keyboard
	evData->child = None;

	//KeyPress or KeyRelease
	keyrouter.gev->type = e->evtype;
	keyrouter.gev->xkey.keycode = evData->detail;

	//XI_KeyPress or XI_KeyRelease
	keyrouter.gcookie->evtype = e->evtype;
	keyrouter.gcookie->cookie = e->cookie;
	memcpy(keyrouter.gcookie->data, e->data, sizeof(XIDeviceEvent));

	fprintf(stderr, "[keyrouter][%s] called !\n", __FUNCTION__);

#if 1//def __DEBUG__
	fprintf(stderr, "[keyrouter][%s] cookie->type=%d, cookie->extension=%d, cookie->evtype=%d, cookie->cookie=%d\n", __FUNCTION__, keyrouter.gcookie->type, keyrouter.gcookie->extension, keyrouter.gcookie->evtype, keyrouter.gcookie->cookie);
	switch( keyrouter.gcookie->evtype )
	{
		case XI_KeyPress:
			fprintf(stderr, "[keyrouter][%s] XI_KeyPress!\n", __FUNCTION__);
			break;

		case XI_KeyRelease:
			fprintf(stderr, "[keyrouter][%s] XI_KeyRelease!\n", __FUNCTION__);
			break;
	}
#endif

	DeliverKeyEvents(keyrouter.gev, keyrouter.gcookie);

out:
	if( keyrouter.gcookie->data )
		free( keyrouter.gcookie->data );

	return 1;
}
#endif//_F_USE_XI_GRABDEVICE_

static void
_e_keyrouter_hwkey_event_handler(XEvent *ev)
{
	int i;
	key_event_info* key_data;

	//KeyRelease handling for key composition
	if( ev->type == KeyRelease )
	{
			//deliver the key
			DeliverDeviceKeyEvents(ev, 0);
			
			ResetModKeyInfo();
			return;
	}
	else if( ev->type == KeyPress )	//KeyPress handling for key composition
	{
#if 1//def __DEBUG__
		fprintf(stderr, "\n[31m[keyrouter][%s] KeyPress (keycode:%d)[0m\n", __FUNCTION__, ev->xkey.keycode);
#endif//__DEBUG__

		if(!keyrouter.modkey_set)
		{
			for(i=0 ; i < NUM_KEY_COMPOSITION_ACTIONS ; i++)
			{
				if(!keyrouter.modkey[i].set)
				{
					//check modifier key
					keyrouter.modkey[i].idx_mod = IsModKey(ev, i);
			
					if(keyrouter.modkey[i].idx_mod)
					{
						fprintf(stderr, "\n[35m[keyrouter][%s][%d] Modifier Key ! (keycode=%d)[0m\n", __FUNCTION__, i, ev->xkey.keycode);
						keyrouter.modkey[i].set = 1;
						keyrouter.modkey_set = 1;
						keyrouter.modkey[i].time = ev->xkey.time;
					}
				}
			}

			//deliver the key
			DeliverDeviceKeyEvents(ev, 0);
			return;
		}
		else
		{
			for(i=0 ; i < NUM_KEY_COMPOSITION_ACTIONS ; i++)
			{
				keyrouter.modkey[i].composited = IsKeyComposited(ev, i);
				
				if(keyrouter.modkey[i].composited)
				{
					fprintf(stderr, "\n[35m[keyrouter][%s][%d] Composition Key ! (keycode=%d)[0m\n", __FUNCTION__, i, ev->xkey.keycode);
				
					//Cancel Press : Send cancel key press to Modifier Key-grabbed window(s)
					ev->xkey.keycode = keyrouter.modkey[i].keys[keyrouter.modkey[i].idx_mod-1].keycode;
					DeliverDeviceKeyEvents(ev, keyrouter.cancel_key.keycode);
					//Modifier Release : Send Modifier key release and cancel key release to Modifier Key-grabbed window(s)
					ev->type = KeyRelease;
					ev->xkey.keycode = keyrouter.modkey[i].keys[keyrouter.modkey[i].idx_mod-1].keycode;
					DeliverDeviceKeyEvents(ev, 0);
					//Cancel Release : Send cancel key release to Modifier Key-grabbed window(s)
					ev->type = KeyRelease;
					ev->xkey.keycode = keyrouter.modkey[i].keys[keyrouter.modkey[i].idx_mod-1].keycode;
					DeliverDeviceKeyEvents(ev, keyrouter.cancel_key.keycode);
				
					//Do Action : ex> send ClientMessage to root window
					DoKeyCompositionAction(i, 1);

					if(keyrouter.modkey[i].press_only)
					{
						//Put Modifier/composited keys' release in ignored_key_list to ignore them
						key_data = malloc(sizeof(key_event_info));
						key_data->ev_type = KeyRelease;
						key_data->keycode = keyrouter.modkey[i].keys[0].keycode;
						keyrouter.ignored_key_list = eina_list_append(keyrouter.ignored_key_list, key_data);
						fprintf(stderr, "[keyrouter][%s] ignored key added (keycode=%d, type=%d)\n", __FUNCTION__, key_data->keycode, key_data->ev_type);
						
						key_data = malloc(sizeof(key_event_info));
						key_data->ev_type = KeyRelease;
						key_data->keycode = keyrouter.modkey[i].keys[1].keycode;
						keyrouter.ignored_key_list = eina_list_append(keyrouter.ignored_key_list, key_data);
						fprintf(stderr, "[keyrouter][%s] ignored key added (keycode=%d, type=%d)\n", __FUNCTION__, key_data->keycode, key_data->ev_type);
					}
					else//need to be waited for keys
					{
						//Put Modifier/composited keys' release in waiting_key_list to check them
						key_data = malloc(sizeof(key_event_info));
						key_data->ev_type = KeyRelease;
						key_data->keycode = keyrouter.modkey[i].keys[0].keycode;
						key_data->modkey_index = i;
						keyrouter.waiting_key_list = eina_list_append(keyrouter.waiting_key_list, key_data);
						fprintf(stderr, "[keyrouter][%s] waiting key added (keycode=%d, type=%d)\n", __FUNCTION__, key_data->keycode, key_data->ev_type);
						
						key_data = malloc(sizeof(key_event_info));
						key_data->ev_type = KeyRelease;
						key_data->keycode = keyrouter.modkey[i].keys[1].keycode;
						key_data->modkey_index = i;
						keyrouter.waiting_key_list = eina_list_append(keyrouter.waiting_key_list, key_data);
						fprintf(stderr, "[keyrouter][%s] waiting key added (keycode=%d, type=%d)\n", __FUNCTION__, key_data->keycode, key_data->ev_type);
					}
				
					ResetModKeyInfo();
					return;
				}
			}

			//deliver the key
			DeliverDeviceKeyEvents(ev, 0);
	
			ResetModKeyInfo();
			return;
		}
	}
}

static int
_e_keyrouter_cb_event_generic(void *data, int ev_type, void *event)
{
	Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event;
	XIDeviceEvent *evData = (XIDeviceEvent *)(e->data);
	XEvent xev;

	if( e->extension != keyrouter.xi2_opcode )
	{
		fprintf(stderr, "[keyrouter][%s] Invalid event !(extension:%d, evtype:%d)\n", __FUNCTION__, e->extension, e->evtype);
		return 1;
	}

	if( !evData || evData->send_event )
	{
		fprintf(stderr, "[keyrouter][%s] Generic event data is not available or the event was sent via XSendEvent (and will be ignored) !\n", __FUNCTION__);
		return 1;
	}

	switch( e->evtype )
	{
		case XI_HierarchyChanged:
			_e_keyrouter_xi2_device_hierarchy_handler((XIHierarchyEvent *)evData);
			break;

		case XI_KeyPress:
		case XI_KeyRelease:
			xev.type = e->evtype;
			xev.xkey.keycode = evData->detail;
			xev.xany.display = keyrouter.disp;
			xev.xkey.time = evData->time;

#ifdef __DEBUG__
			fprintf(stderr, "[keyrouter][%s] keycode=%d, type=%s\n", __FUNCTION__, xev.xkey.keycode, (xev.type==XI_KeyPress) ? "XI_KeyPress" : "XI_KeyRelease");
#endif

			if(!_e_keyrouter_is_waiting_key_list_empty(&xev))
				return 1;

			if(_e_keyrouter_is_key_in_ignored_list(&xev))
				return 1;

			_e_keyrouter_hwkey_event_handler(&xev);
			break;

#ifdef _F_ENABLE_MOUSE_POPUP
		case XI_ButtonRelease:
			if( evData->detail != 1 && evData->detail != 3 )
				break;

			if( keyrouter.popup && evData->child != keyrouter.popup->evas_win )
			{
				if( !keyrouter.rbutton_pressed_on_popup )
					popup_destroy();
				else
					keyrouter.rbutton_pressed_on_popup = 0;
			}
			break;
#endif//_F_ENABLE_MOUSE_POPUP
	}

	return 1;
}

static int
_e_keyrouter_cb_window_create(void *data, int ev_type, void *ev)
{
	Ecore_X_Window_Attributes att;
	Ecore_X_Event_Window_Create *e = ev;

	//Check if current window is TOP-level window or not
	if( !e || keyrouter.rootWin != e->parent || !e->win )
		return 1;

	//Setting PropertyChangeMask and SubstructureNotifyMask for TOP-level window (e->win)
	ecore_x_window_attributes_get(e->win, &att);
	XSelectInput(keyrouter.disp, e->win, att.event_mask.mine | PropertyChangeMask | StructureNotifyMask);
	XSync(keyrouter.disp, False);

#ifdef __DEBUG__
	printf("[keyrouter][%s] TOP-level window ! (id=0x%x, parent=0x%x, override=%d)\n", __FUNCTION__, e->win, e->parent, e->override);
#endif

 	int ret = 0, count = 0;
 	int i, result, keycode, grab_mode;
	unsigned int *prop_data = NULL;

	//Get the window property using the atom
	ret = ecore_x_window_prop_property_get (e->win, keyrouter.atomGrabKey, ECORE_X_ATOM_CARDINAL, 32, (unsigned char **)&prop_data, &count);

	if( !ret || !prop_data )
 		goto out;

	//property¸¦ º¸°í list¿¡ add
	for( i=0 ; i < count ; i++ )
	{
		grab_mode = prop_data[i] & GRAB_MODE_MASK;
		keycode = prop_data[i] & (~GRAB_MODE_MASK);

#ifdef __DEBUG__
		printf("[keyrouter][%s] prop_data[%d] = %d, keycode = %d(hex:%x), grab_mode = %d(hex:%x)\n", __FUNCTION__, i, (int)(prop_data[i]), keycode, keycode, grab_mode, grab_mode);
#endif

		_e_keyrouter_update_key_delivery_list(e->win, keycode, grab_mode, 1);
	}

out:
	if(prop_data)	free(prop_data);
	return 1;
 }

static int
_e_keyrouter_cb_window_property(void *data, int ev_type, void *ev)
{
	Ecore_X_Event_Window_Property *e = ev;

	int ret = 0, count = 0;
	unsigned int *prop_data = NULL;

	int res = -1;
	unsigned int ret_val = 0;

#ifdef _F_ENABLE_MOUSE_POPUP
	//check and rotate popup menu
	if( e->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE && e->win == keyrouter.rootWin )
	{
		res = ecore_x_window_prop_card32_get(e->win, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, &ret_val, 1);

		if( res == 1 )
		{
			if( keyrouter.popup_angle != ret_val )
			{
				keyrouter.popup_angle = ret_val;
				if( !keyrouter.popup ) goto out;
				popup_update();
			}
		}

		goto out;
	}
#endif // _F_ENABLE_MOUSE_POPUP

	if( e->atom == keyrouter.atomDeviceStatus && e->win == keyrouter.rootWin )
	{
		res = ecore_x_window_prop_card32_get(e->win, keyrouter.atomDeviceStatus, &ret_val, 1);

		if( res == 1 )
		{
			Device_Status(ret_val);
		}

		goto out;
	}

	if( e->atom == keyrouter.atomGrabStatus && e->win == keyrouter.rootWin )
	{
		res = ecore_x_window_prop_card32_get(e->win, keyrouter.atomGrabStatus, &ret_val, 1);

		if( res == 1 )
		{
			Keygrab_Status(ret_val);
		}

		goto out;
	}

	//See the client window has interesting atom (_atomGrabKey)
	if( e->atom != keyrouter.atomGrabKey)
		goto out;

#ifdef __DEBUG__
	printf("[keyrouter][%s] PropertyNotify received (Client Window = 0x%x, Atom = %d)\n", __FUNCTION__, e->win, e->atom);
#endif

	//Get the window property using the atom
	ret = ecore_x_window_prop_property_get (e->win, e->atom, ECORE_X_ATOM_CARDINAL, 32, (unsigned char **)&prop_data, &count);

	if( !ret || !prop_data )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] Failed to get window property OR window property is empty ! (window = 0x%x)\n", __FUNCTION__, e->win);
#endif
		RemoveWindowDeliveryList(e->win, 0, 0);
		goto out;
	}

#ifdef __DEBUG__
	PrintKeyDeliveryList();
#endif

	RemoveWindowDeliveryList(e->win, 0, 0);

#ifdef __DEBUG__
	PrintKeyDeliveryList();
#endif

	int i;
	int keycode;
	int grab_mode;

	//property¸¦ º¸°í list¿¡ add
	for( i=0 ; i < count ; i++ )
	{
		grab_mode = prop_data[i] & GRAB_MODE_MASK;
		keycode = prop_data[i] & (~GRAB_MODE_MASK);

#ifdef __DEBUG__
		printf("[keyrouter][%s] prop_data[%d] = %d, keycode = %d(hex:%x), grab_mode = %d(hex:%x)\n", __FUNCTION__, i, (int)(prop_data[i]), keycode, keycode, grab_mode, grab_mode);
#endif

		_e_keyrouter_update_key_delivery_list(e->win, keycode, grab_mode, 1);
	}

out:
	if(prop_data)	free(prop_data);
	return 1;
}

static int
_e_keyrouter_cb_e_border_stack(void *data, int ev_type, void *ev)
{
	E_Event_Border_Stack *e = ev;

	if(!e->border)
		return 1;

#ifdef __DEBUG__
	printf("[keyrouter][%s] E_EVENT_BORDER_STACK received (e->border->win = 0x%x, Client window = 0x%x) !\n", __FUNCTION__, e->border->win, e->border->client.win);
#endif

	keyrouter.isWindowStackChanged = 1;

	if( e->type == E_STACKING_ABOVE )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] == TOP == (window : 0x%x)\n", __FUNCTION__, e->border->client.win);
#endif
		AdjustTopPositionDeliveryList(e->border->client.win, 1);
#ifdef __DEBUG__
		PrintKeyDeliveryList();
#endif
	}
	else
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] == NO TOP == (window : 0x%x)\n", __FUNCTION__, e->border->client.win);
#endif
		AdjustTopPositionDeliveryList(e->border->client.win, 0);
#ifdef __DEBUG__
		PrintKeyDeliveryList();
#endif
	}

	return 1;
}

static int
_e_keyrouter_cb_e_border_remove(void *data, int ev_type, void *ev)
{
	int ret = 0;
	unsigned int ret_val = 0;

	E_Event_Border_Remove *e = ev;

	if(!e->border)
		return 1;
	else
	{
		ret = ecore_x_window_prop_card32_get(e->border->client.win, E_ATOM_MANAGED, &ret_val, 1);

		if( ret != 1 )
		{
#ifdef __DEBUG__
			printf("[keyrouter][%s] E_EVENT_BORDER_REMOVE ! Client Window is destroyed !(window : 0x%x)\n", __FUNCTION__, e->border->client.win);
#endif
			keyrouter.isWindowStackChanged = 1;
			RemoveWindowDeliveryList(e->border->client.win, 0, 1);
			return 1;
		}

		if( ret_val == 1 )
		{
#ifdef __DEBUG__
			printf("[keyrouter][%s] E_EVENT_BORDER_REMOVE ! Client Window is destroyed !(window : 0x%x)\n", __FUNCTION__, e->border->client.win);
#endif
			keyrouter.isWindowStackChanged = 1;
			RemoveWindowDeliveryList(e->border->client.win, 0, 1);
			return 1;
		}

		if( ret_val == 0 )
		{
#ifdef __DEBUG__
			printf("[keyrouter][%s] E_EVENT_BORDER_REMOVE ! Client Window is hidden !(window : 0x%x)\n", __FUNCTION__, e->border->client.win);
#endif
			keyrouter.isWindowStackChanged = 1;
			RemoveWindowDeliveryList(e->border->client.win, 0, 1);
			return 1;
		}
	}

	return 1;
}

static int
_e_keyrouter_cb_window_destroy(void *data, int ev_type, void *ev)
{
	E_Border *bd;
	Ecore_X_Event_Window_Destroy *e = ev;

	//Skip for client windows which have border as their parents
	bd = e_border_find_by_client_window(e->win);
	if( bd )
		return 1;

#ifdef __DEBUG__
	printf("[keyrouter][%s] DestroyNotify received (Top-level Window = 0x%x, event_win = 0x%x) !\n", __FUNCTION__, e->win, e->event_win);
#endif

	keyrouter.isWindowStackChanged = 1;
	RemoveWindowDeliveryList(e->win, 0, 1);

	return 1;
}

static int
_e_keyrouter_cb_window_configure(void *data, int ev_type, void *ev)
{
	E_Border *bd;
	Ecore_X_Event_Window_Configure *e = ev;

	//Skip for client windows which have border as their parents
	bd = e_border_find_by_client_window(e->win);
	if( bd )
		return 1;

	if( keyrouter.rootWin != e->event_win )
		return 1;

#ifdef __DEBUG__
	printf("[keyrouter][%s] ConfigureNotify received (Top-level Window = 0x%x, above win = 0x%x, event win = 0x%x, override = %d, from_wm = %d)\n", __FUNCTION__, e->win, e->abovewin, e->event_win, e->override, e->from_wm);
#endif

	keyrouter.isWindowStackChanged = 1;
	AdjustTopPositionDeliveryList(e->win, !!e->abovewin);

	return 1;
}

static int
_e_keyrouter_cb_client_message (void* data, int type, void* event)
{
	int event_type = 0;
	int keycode;
	int cancel = 0;
	Ecore_X_Event_Client_Message* ev;

	ev = event;
	if(ev->message_type != keyrouter.atomHWKeyEmulation) return 1;
	if(ev->format != 8) return 1;

	if( ev->data.b[0] == 'P' )
		event_type = KeyPress;
	else if( ev->data.b[0] == 'R' )
		event_type = KeyRelease;

	if( ev->data.b[1] == 'C' )
		cancel = 1;

	switch(event_type)
	{
		case KeyPress:
		case KeyRelease:
			keycode = XKeysymToKeycode(keyrouter.disp, XStringToKeysym(&ev->data.b[2]));
			_e_keyrouter_do_hardkey_emulation(NULL, event_type, 0, keycode, cancel);
			break;

		default:
			fprintf(stderr, "[keyrouter][cb_client_message] Unknown event type ! (type=%d)\n", event_type);
	}

	return 1;
}

static int
_e_keyrouter_cb_window_stack(void *data, int ev_type, void *ev)
{
	E_Border *bd;
	Ecore_X_Event_Window_Stack *e = ev;

	//Skip for client windows which have border as their parents
	bd = e_border_find_by_client_window(e->win);
	if( bd )
		return 1;

	if( keyrouter.rootWin != e->event_win )
		return 1;

#ifdef __DEBUG__
	printf("[keyrouter][%s] CirculateNotify received (Top-level Window = 0x%x, event_win = 0x%x, detail = %d)\n", __FUNCTION__, e->win, e->event_win, e->detail);
#endif

	keyrouter.isWindowStackChanged = 1;
	AdjustTopPositionDeliveryList(e->win, !e->detail);

	return 1;
}

//e17 bindings functions and action callbacks

static int
_e_keyrouter_modifiers(E_Binding_Modifier modifiers)
{
   int mod = 0;

   if (modifiers & E_BINDING_MODIFIER_SHIFT) mod |= ECORE_EVENT_MODIFIER_SHIFT;
   if (modifiers & E_BINDING_MODIFIER_CTRL) mod |= ECORE_EVENT_MODIFIER_CTRL;
   if (modifiers & E_BINDING_MODIFIER_ALT) mod |= ECORE_EVENT_MODIFIER_ALT;
   if (modifiers & E_BINDING_MODIFIER_WIN) mod |= ECORE_EVENT_MODIFIER_WIN;
   /* see comment in e_bindings on numlock
      if (modifiers & ECORE_X_LOCK_NUM) mod |= ECORE_X_LOCK_NUM;
   */

   return mod;
}

static void _e_keyrouter_do_bound_key_action(XEvent *xev)
{
	Ecore_Event_Key *ev;
	int keycode = xev->xkey.keycode;

	if( !keyrouter.HardKeys[keycode].bind )
	{
		fprintf(stderr, "[keyrouter][do_bound_key_action] bind info of key(%d) is NULL !\n", keycode);
		return;
	}

	ev = malloc(sizeof(Ecore_Event_Key));

	if( !ev )
	{
		fprintf(stderr, "[keyrouter][do_bound_key_action] Failed to allocate memory for Ecore_Event_Key !\n");
		return;
	}

	ev->keyname = (char *)malloc(strlen(keyrouter.HardKeys[keycode].bind->key)+1);
	ev->key = (char *)malloc(strlen(keyrouter.HardKeys[keycode].bind->key)+1);

	if( !ev->keyname || !ev->key )
	{
		free(ev);
		fprintf(stderr, "[keyrouter][do_bound_key_action] Failed to allocate memory for key name !\n");
		return;
	}

	strncpy((char *)ev->keyname, keyrouter.HardKeys[keycode].bind->key, strlen(keyrouter.HardKeys[keycode].bind->key)+1);
	strncpy((char *)ev->key, keyrouter.HardKeys[keycode].bind->key, strlen(keyrouter.HardKeys[keycode].bind->key)+1);

	ev->compose = NULL;
	ev->string = NULL;
	ev->modifiers = 0;
	ev->timestamp = xev->xkey.time;
	ev->event_window = xev->xkey.window;
	ev->root_window = xev->xkey.root;
	ev->same_screen = xev->xkey.same_screen;
	ev->window = xev->xkey.subwindow ? xev->xkey.subwindow : xev->xkey.window;

	if( xev->type == KeyPress )
		e_bindings_key_down_event_handle(keyrouter.HardKeys[keycode].bind->ctxt, NULL, ev);
	else if( xev->type == KeyRelease )
		e_bindings_key_up_event_handle(keyrouter.HardKeys[keycode].bind->ctxt, NULL, ev);

	free(ev);
}

static void _e_keyrouter_xi2_device_hierarchy_handler(XIHierarchyEvent *event)
{
	int i;

	if( event->flags & XIDeviceEnabled || event->flags & XIDeviceDisabled )
	{
		for( i = 0 ; i < event->num_info ; i++ )
		{
			if( event->info[i].flags & XIDeviceEnabled )
			{
				_e_keyrouter_device_add(event->info[i].deviceid, event->info[i].use);
			}
			else if( event->info[i].flags & XIDeviceDisabled )
			{
				_e_keyrouter_device_remove(event->info[i].deviceid, event->info[i].use);
			}
		}
	}
}

static Eina_Bool _e_keyrouter_is_waiting_key_list_empty(XEvent *ev)
{
	int modkey_index;
	Eina_List* l;
	key_event_info* data;
	key_event_info* key_data;

	if( !keyrouter.waiting_key_list )
	{
		return EINA_TRUE;
	}

	fprintf(stderr, "[keyrouter][%s] waiting_key_list is NOT empty !\n", __FUNCTION__);
	fprintf(stderr, "[keyrouter][%s] type=%s, keycode=%d", __FUNCTION__, (ev->xkey.type==KeyPress) ? "KeyPress" : "KeyRelease", ev->xkey.keycode);

	EINA_LIST_FOREACH(keyrouter.waiting_key_list, l, data)
	{
		if( data && ev->type == data->ev_type && ev->xkey.keycode == data->keycode )
		{
			//found !!!
			fprintf(stderr, "[keyrouter][%s] found !!! (keycode:%d, type=%d)\n", __FUNCTION__, data->keycode, data->ev_type);
			goto found;
		}
	}

	fprintf(stderr, "[keyrouter][%s] not found !!! (keycode:%d, type=%d)\n", __FUNCTION__, ev->xkey.keycode, ev->type);

	//If a key press is coming before the waiting_key_list is cleared,
	//put its release into ignored_list.
	if(ev->type == KeyPress)
	{
		key_data = malloc(sizeof(key_event_info));
		key_data->ev_type = KeyRelease;
		key_data->keycode = ev->xkey.keycode;
		keyrouter.ignored_key_list = eina_list_append(keyrouter.ignored_key_list, key_data);
		fprintf(stderr, "[keyrouter][%s] ignored key added (keycode=%d, type=%d)\n", __FUNCTION__, key_data->keycode, key_data->ev_type);
	}
	else if(ev->type == KeyRelease)
	{
		//If a key release which is contained in ignored_key_list is coming,
		//remove it from ignored_key_list.
		fprintf(stderr, "[keyrouter][%s] check a key is exiting in ignored key list (keycode=%d, type=%d)\n", __FUNCTION__, ev->xkey.keycode, ev->type);
		_e_keyrouter_is_key_in_ignored_list(ev);
	}

	return EINA_FALSE;

found:
	modkey_index = data->modkey_index;
	keyrouter.waiting_key_list = eina_list_remove(keyrouter.waiting_key_list, data);
	fprintf(stderr, "[keyrouter][%s][%d] key was remove from waiting_key_list !(keycode:%d, type:%d)\n",
		__FUNCTION__, modkey_index, ev->xkey.keycode, ev->type);

	if(!keyrouter.waiting_key_list)
	{
		fprintf(stderr, "[keyrouter][%s] Waiting conditions are satified !\n", __FUNCTION__);

		//Do Action : ex> send ClientMessage to root window
		DoKeyCompositionAction(modkey_index, 0);
	}

	return EINA_FALSE;
}


static Eina_Bool _e_keyrouter_is_key_in_ignored_list(XEvent *ev)
{
	Eina_List* l;
	key_event_info* data;

	if( !keyrouter.ignored_key_list )
	{
		return EINA_FALSE;
	}

	fprintf(stderr, "[keyrouter][%s] ignored_key_list is NOT empty !\n", __FUNCTION__);

	EINA_LIST_FOREACH(keyrouter.ignored_key_list, l, data)
	{
		if( data && ev->type == data->ev_type && ev->xkey.keycode == data->keycode )
		{
			//found !!!
			fprintf(stderr, "[keyrouter][%s] found !!! (keycode:%d, type=%d)\n", __FUNCTION__, data->keycode, data->ev_type);
			goto found;
		}
	}

	fprintf(stderr, "[keyrouter][%s] not found !!! (keycode:%d, type=%d)\n", __FUNCTION__, ev->xkey.keycode, ev->type);

	return EINA_FALSE;

found:
	keyrouter.ignored_key_list = eina_list_remove(keyrouter.ignored_key_list, data);
	fprintf(stderr, "[keyrouter][%s] key was remove from ignored_list !(keycode:%d, type:%d)\n", __FUNCTION__, ev->xkey.keycode, ev->type);

	return EINA_TRUE;
}

static void _e_keyrouter_device_add(int id, int type)
{
	int ndevices;
	Eina_List* l;
	XIDeviceInfo *info = NULL;
	KeyrouterDeviceType kdtype;
	E_Keyrouter_Device_Info *kdi = NULL;

	EINA_LIST_FOREACH(keyrouter.device_list, l, kdi)
	{
		if( (kdi) && (kdi->type==E_KEYROUTER_HWKEY) && (kdi->id == id) )
		{
			fprintf(stderr, "[keyrouter][%s] Slave key device (id=%d, name=%s) was added !\n", __FUNCTION__, id, kdi->name);
			detachSlave(id);
			return;
		}
	}

	if( type == XISlaveKeyboard )
	{
		info = XIQueryDevice(keyrouter.disp, id, &ndevices);

		if( !info || ndevices <= 0 )
		{
			fprintf(stderr, "[keyrouter][%s] There is no queried XI device. (device id=%d, type=%d)\n", __FUNCTION__, id, type);
			goto out;
		}

		if( strcasestr(info->name, "XTEST" ) )
			goto out;

		if( strcasestr(info->name, "keyboard" ) )
			kdtype = E_KEYROUTER_KEYBOARD;
		else
			kdtype = E_KEYROUTER_HOTPLUGGED;

		E_Keyrouter_Device_Info *data = malloc(sizeof(E_Keyrouter_Device_Info));

		if( !data )
		{
			fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for device info !\n", __FUNCTION__);
			goto out;
		}

		data->type = kdtype;
		data->id = id;
		data->name = eina_stringshare_add(info->name);
		keyrouter.device_list = eina_list_append(keyrouter.device_list, data);

		_e_keyrouter_grab_hwkeys(id);
	}

out:
	if( info )	XIFreeDeviceInfo(info);
}

static void _e_keyrouter_device_remove(int id, int type)
{
	Eina_List* l;
	E_Keyrouter_Device_Info *data;

	if( !keyrouter.device_list )
	{
		fprintf(stderr, "[keyrouter][%s] device list is empty ! something's wrong ! (id=%d, type=%d)\n", __FUNCTION__, id, type);
		goto out;
	}

	EINA_LIST_FOREACH(keyrouter.device_list, l, data)
	{
		if( data && data->id == id )
		{
			switch( data->type )
			{
				case E_KEYROUTER_HWKEY:
					break;

				case E_KEYROUTER_HOTPLUGGED:
				case E_KEYROUTER_KEYBOARD:
					fprintf(stderr, "[keyrouter][%s] Slave hotplugged key|keyboard device (id=%d, name=%s, type=%d) was removed/disabled !\n",
						__FUNCTION__, id, data->name, type);
					keyrouter.device_list = eina_list_remove(keyrouter.device_list, data);
					free(data);
					goto out;

				default:
					fprintf(stderr, "[keyrouter][%s] Unknown type of device ! (id=%d, type=%d, name=%s, device type=%d)\n",
						__FUNCTION__, data->id, type, data->name, data->type);
					keyrouter.device_list = eina_list_remove(keyrouter.device_list, data);
					free(data);
					goto out;
			}
		}
	}

out:
	return;
}

static E_Zone* _e_keyrouter_get_zone()
{
	Eina_List *ml;
	E_Manager *man;
	E_Zone *zone = NULL;

	if( keyrouter.zone )
		return keyrouter.zone;

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

int _e_keyrouter_init()
{
	int ret = 1;
	int grab_result;

	_e_keyrouter_structure_init();
	keyrouter.disp = NULL;
	keyrouter.disp = ecore_x_display_get();

	if( !keyrouter.disp )
	{
		fprintf(stderr, "[32m[keyrouter] Failed to open display..![0m\n");
		ret = 0;
		goto out;
	}

	keyrouter.rootWin = DefaultRootWindow(keyrouter.disp);

	_e_keyrouter_x_input_init();
	InitGrabKeyDevices();
	_e_keyrouter_bindings_init();

	keyrouter.atomDeviceStatus = ecore_x_atom_get(STR_ATOM_DEVICE_STATUS);
	keyrouter.atomGrabStatus = ecore_x_atom_get(STR_ATOM_GRAB_STATUS);
	keyrouter.atomGrabKey = ecore_x_atom_get(STR_ATOM_GRAB_KEY);
	keyrouter.atomGrabExclWin = ecore_x_atom_get(STR_ATOM_GRAB_EXCL_WIN);
	keyrouter.atomGrabORExclWin = ecore_x_atom_get(STR_ATOM_GRAB_OR_EXCL_WIN);
	keyrouter.atomHWKeyEmulation = ecore_x_atom_get(PROP_HWKEY_EMULATION);

	keyrouter.input_window = ecore_x_window_input_new(keyrouter.rootWin, -1, -1, 1, 1);

	if(!keyrouter.input_window)
	{
		fprintf(stderr, "[keyrouter] Failed to create input_window !\n");
	}
	else
	{
		ecore_x_window_prop_property_set(keyrouter.rootWin, keyrouter.atomHWKeyEmulation, ECORE_X_ATOM_WINDOW, 32, &keyrouter.input_window, 1);
	}

	keyrouter.zone = _e_keyrouter_get_zone();
	if( !keyrouter.zone )
	{
		fprintf(stderr, "[keyrouter] Failed to get zone !\n");
		ret = 0;
		goto out;
	}

#ifdef _F_ENABLE_MOUSE_POPUP
	ecore_x_window_button_grab(keyrouter.rootWin, 3, ECORE_X_EVENT_MASK_MOUSE_DOWN |
				 ECORE_X_EVENT_MASK_MOUSE_UP |
				 ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
#endif//_F_ENABLE_MOUSE_POPUP

#ifndef _F_USE_XI_GRABDEVICE_
	grab_result = GrabKeyDevices(keyrouter.rootWin);

	if( !grab_result )
	{
		fprintf(stderr, "[32m[keyrouter] Failed to GrabDevices() ![0m\n");
		ret = 0;
		goto out;
	}

	keyrouter.DeviceKeyPress = keyrouter.nInputEvent[INPUTEVENT_KEY_PRESS];
	keyrouter.DeviceKeyRelease = keyrouter.nInputEvent[INPUTEVENT_KEY_RELEASE];
#else//_F_USE_XI_GRABDEVICE_
	grab_result = GrabXIKeyDevices();

	if( !grab_result )
	{
		fprintf(stderr, "[32m[keyrouter] Failed to GrabXIKeyDevices() ![0m\n");
		ret = 0;
		goto out;
	}
#endif//_F_USE_XI_GRABDEVICE_

	keyrouter.modkey = calloc(2, sizeof(ModifierKey));

	if(!keyrouter.modkey)
	{
		fprintf(stderr, "[32m[keyrouter] Failed to allocate memory for key composition ![0m\n");
		ret = 0;
		goto out;
	}

	InitModKeys();
#ifdef _F_ENABLE_MOUSE_POPUP
	InitHardKeyCodes();
#endif//_F_ENABLE_MOUSE_POPUP
	BuildKeyGrabList(keyrouter.rootWin);

out:
	return ret;
}

void _e_keyrouter_fini()
{
#ifndef _F_USE_XI_GRABDEVICE_
	UngrabKeyDevices();
#else
	UngrabXIKeyDevices();
#endif
}

static void _e_keyrouter_structure_init()
{
	memset(&keyrouter, 0L, sizeof(keyrouter));

	keyrouter.fplog = stderr;
	keyrouter.DeviceKeyPress = -1;
	keyrouter.DeviceKeyRelease = -1;
	keyrouter.xi2_opcode = -1;
	keyrouter.isWindowStackChanged = 1;
	keyrouter.prev_sent_keycode = 0;
	keyrouter.resTopVisibleCheck = 0;
#ifdef _F_ENABLE_MOUSE_POPUP
	keyrouter.popup_angle = 0;
	keyrouter.toggle = 0;
#endif//_F_ENABLE_MOUSE_POPUP
	keyrouter.device_list = NULL;
	keyrouter.ignored_key_list = NULL;
	keyrouter.waiting_key_list = NULL;
	keyrouter.hwkeymap_info_list = NULL;
	keyrouter.num_hwkey_devices = 0;

	keyrouter.atomGrabKey = None;
	keyrouter.atomDeviceStatus = None;
	keyrouter.atomGrabStatus = None;
	keyrouter.atomGrabExclWin = None;
	keyrouter.atomGrabORExclWin = None;
}

static void _e_keyrouter_grab_hwkeys(int devid)
{
	int i, j, k;
	int result;
	int keycode;
	KeySym ksym;

	int min_keycode, max_keycode, keysyms_per_keycode;
	KeySym *keymap = NULL, *origkeymap = NULL;

	XIGrabModifiers modifiers[] = {{XIAnyModifier, 0}};
	int nmods = sizeof(modifiers)/sizeof(modifiers[0]);
	XIEventMask mask;

	if(!keyrouter.hwkeymap_info_list)
	{
		XDisplayKeycodes (keyrouter.disp, &min_keycode, &max_keycode);
		origkeymap = XGetKeyboardMapping (keyrouter.disp, min_keycode,
						  (max_keycode - min_keycode + 1),
						  &keysyms_per_keycode);

		if(!origkeymap)
		{
			fprintf(stderr, "[keyrouter] Failed to get keyboard mapping from X \n");
			return;
		}

		for( i = 0 ; i < NUM_HWKEYS ; i++ )
		{
			if(!HWKeys[i])
				break;

			Eina_Inlist *l = keyrouter.hwkeymap_info_list;

			ksym = XStringToKeysym(HWKeys[i]);

			if((ksym == NoSymbol) ||(ksym == keyrouter.cancel_key.keysym))
				continue;

			hwkeymap_info *hki = NULL;

			hki = calloc(1, sizeof(hwkeymap_info));

			if(!hki)
			{
				fprintf(stderr, "[keyrouter] Failed to allocate memory for HW keymap information !\n");
				XFree(origkeymap);
				return;
			}

			hki->key_name = HWKeys[i];
			hki->key_sym = ksym;
			hki->keycodes = NULL;

			keymap = origkeymap;
			k = 0;
			for (j = min_keycode; j <= max_keycode; j++)
			{
				if(ksym == keymap[0])
					k++;
				keymap += keysyms_per_keycode;
			}

			hki->num_keycodes = k;

			int *keycodes = calloc(1, sizeof(int)*(hki->num_keycodes));

			if(!keycodes)
			{
				fprintf(stderr, "[keyrouter] Failed to allocate memory for keycode array !\n");
				XFree(origkeymap);
				free(hki);
				return;
			}

			keymap = origkeymap;
			k = 0;
			for (j = min_keycode; j <= max_keycode; j++)
			{
				if(ksym == keymap[0])
				{
					keycodes[k] = j;
					k++;
				}
				keymap += keysyms_per_keycode;
			}

			hki->keycodes = keycodes;
			l = eina_inlist_append(l, (Eina_Inlist *)hki);
			keyrouter.hwkeymap_info_list = l;
		}

		if(origkeymap)
			XFree(origkeymap);
	}

	hwkeymap_info *hkinfo = NULL;
	Eina_Inlist *lst = keyrouter.hwkeymap_info_list;

	if(devid)
	{
		mask.deviceid = devid;
		mask.mask_len = XIMaskLen(XI_LASTEVENT);
		mask.mask = calloc(mask.mask_len, sizeof(char));
		XISetMask(mask.mask, XI_KeyPress);
		XISetMask(mask.mask, XI_KeyRelease);
	}

	EINA_INLIST_FOREACH(lst, hkinfo)
	{
		if(!hkinfo->keycodes)
			continue;

		for(k = 0 ; k < hkinfo->num_keycodes ; k++)
		{
			if(devid)
			{
				result = XIGrabKeycode(keyrouter.disp, devid, hkinfo->keycodes[k], keyrouter.rootWin, GrabModeAsync, GrabModeAsync, False, &mask, nmods, modifiers);
#ifdef __DEBUG__
				fprintf(stderr, "[keyrouter][grab_hwkeys][k=%d] %s (keycode:%d) was grabbed (result=%d)!!\n", k, hkinfo->key_name, hkinfo->keycodes[k], result);
#endif /* #ifdef __DEBUG__ */
	
				if(result < 0)
				{
					fprintf(stderr, "[keyrouter][grab_hwkeys] Failed to grab keycode (=%d) !\n", hkinfo->keycodes[k]);
					continue;
				}
			}

			//disable repeat for a key
			_e_keyrouter_set_key_repeat(hkinfo->keycodes[k], 0);//OFF
		}
	}
}

static void
_e_keyrouter_set_key_repeat(int key, int auto_repeat_mode)
{
	XKeyboardControl values;

	values.auto_repeat_mode = auto_repeat_mode;

	if (key != -1)
	{
		values.key = key;
		XChangeKeyboardControl(keyrouter.disp, KBKey | KBAutoRepeatMode, &values);
	}
	else
	{
		XChangeKeyboardControl(keyrouter.disp, KBAutoRepeatMode, &values);
	}
}


static void _e_keyrouter_bindings_init()
{
	int i;
	int keycode;
	KeySym ksym;

	for( i = 0 ; i < NUM_HWKEYS ; i++ )
	{
		if( HWKeys[i] )
		{
			ksym = XStringToKeysym(HWKeys[i]);

			if( ksym )
				keycode = XKeysymToKeycode(keyrouter.disp, ksym);
			else
				keycode = 0;

			if( !keycode || keycode >= 255 )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[keyrouter][%s] Failed to get keycode from keyname(=%s),(i=%d) !\n", __FUNCTION__, HWKeys[i], i);
#endif//__DEBUG__
				continue;
			}

			//get bound key information
			keyrouter.HardKeys[keycode].bind = e_bindings_key_find(HWKeys[i], E_BINDING_MODIFIER_NONE, 1);

			if( !keyrouter.HardKeys[keycode].bind || strcmp(keyrouter.HardKeys[keycode].bind->key, HWKeys[i]) )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[keyrouter][%s] Failed to get bound key using e_bindings_key_find() (keyname=%s)!\n", __FUNCTION__, HWKeys[i]);
#endif//__DEBUG__
				continue;
			}

			//ungrab bound key(s)
			ecore_x_window_key_ungrab(keyrouter.rootWin, keyrouter.HardKeys[keycode].bind->key,
				       _e_keyrouter_modifiers(keyrouter.HardKeys[keycode].bind->mod), keyrouter.HardKeys[keycode].bind->any_mod);

			fprintf(stderr, "[keyrouter][bindings_init] %s (keycode:%d) was bound !!\n", keyrouter.HardKeys[keycode].bind->key, keycode);
		}
		else
			break;
	}
}

static void
_e_keyrouter_update_key_delivery_list(Ecore_X_Window win, int keycode, const int grab_mode, const int IsOnTop)
{
	int c=0;
	int result = 0;
	Eina_Bool found = EINA_FALSE;
	hwkeymap_info *hkinfo = NULL;
	Eina_Inlist *lst;

	if(!keyrouter.hwkeymap_info_list)
		goto grab_a_keycode_only;

	lst = keyrouter.hwkeymap_info_list;
	EINA_INLIST_FOREACH(lst, hkinfo)
	{
		if(hkinfo && hkinfo->keycodes)
		{
			for(c = 0 ; c < hkinfo->num_keycodes ; c++)
			{
				if(hkinfo->keycodes[c] == keycode)
				{
					found = EINA_TRUE;
					break;
				}
			}

			if(!found) continue;

			for(c = 0 ; c < hkinfo->num_keycodes ; c++)
			{
#ifdef __DEBUG__
				PrintKeyDeliveryList();
#endif
				result = AddWindowToDeliveryList(win, hkinfo->keycodes[c], grab_mode, 1);

				if( result )
					fprintf(stderr, "[32m[keyrouter][%s] Failed to add window (0x%x) to delivery list ! keycode=%x, grab_mode=0x%X[0m\n", __FUNCTION__, win, hkinfo->keycodes[c], grab_mode);
#ifdef __DEBUG__
				PrintKeyDeliveryList();
#endif

			}

			return;
		}
	}

	return;

grab_a_keycode_only:

#ifdef __DEBUG__
		PrintKeyDeliveryList();
#endif
		result = AddWindowToDeliveryList(win,  keycode, grab_mode, 1);

		if( result )
			fprintf(stderr, "[32m[keyrouter][%s] Failed to add window (0x%x) to delivery list ! keycode=%x, grab_mode=0x%X[0m\n", __FUNCTION__, win, keycode, grab_mode);

#ifdef __DEBUG__
		PrintKeyDeliveryList();
#endif
}

static int GetItemFromWindow(Window win, const char* atom_name, unsigned int **key_list)
{
	Atom ret_type;
	int ret_format;
	unsigned long nr_item = 0;
	unsigned long sz_remains_data;
	Atom grabKey;

	grabKey = XInternAtom(keyrouter.disp, atom_name, False);

	if (XGetWindowProperty(keyrouter.disp, win, grabKey, 0, 0x7fffffff, False, XA_CARDINAL,
				&ret_type, &ret_format, &nr_item,
				&sz_remains_data, (unsigned char **)key_list) != Success)
	{
		nr_item = 0;
	}

	return nr_item;
}

static void BuildKeyGrabList(Window root)
{
	Window tmp;
	Window *childwins = NULL;
	unsigned int num_children;
	register unsigned int i, j;
	int grab_mode, keycode;
	unsigned int *key_list = NULL;
	int n_items = 0;
    int res;

	XGrabServer(keyrouter.disp);
	res = XQueryTree(keyrouter.disp, root, &tmp, &tmp, &childwins, &num_children);
	XUngrabServer(keyrouter.disp);
    if (0 == res) return;

	for( i=0 ; i < num_children; i++ )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] window = 0x%X\n", __FUNCTION__, (int)(childwins[i]));
#endif
		BuildKeyGrabList(childwins[i]);

		n_items = GetItemFromWindow(childwins[i], STR_ATOM_GRAB_KEY, &key_list);
		if( n_items )
		{
			for( j=0 ; j < n_items ; j++ )
			{
				grab_mode = key_list[j] & GRAB_MODE_MASK;
				keycode = key_list[j] & (~GRAB_MODE_MASK);
#ifdef __DEBUG__
				printf("[keyrouter][%s] window = 0x%x, grab_mode = 0x%X, keycode = %d\n", __FUNCTION__, (int)(childwins[i]), grab_mode, keycode);
#endif
				_e_keyrouter_update_key_delivery_list(childwins[i], keycode, grab_mode, 1);
			}
		}
	}

	if( key_list ) XFree(key_list);
	if( childwins ) XFree(childwins);
}

static void InitGrabKeyDevices()
{
	memset(keyrouter.HardKeys, (int)NULL, sizeof(keyrouter.HardKeys));
}

#ifndef _F_USE_XI_GRABDEVICE_
//Function for getting device pointer through device name
static int GrabKeyDevice(Window win,
		const char* DeviceName,
		const int DeviceID)
{
	int result;
	XEventClass eventList[32];
	XEventClass cls;

	XDevice* pDev = NULL;

	pDev = XOpenDevice(keyrouter.disp, DeviceID);

	if( !pDev )
	{
		fprintf(stderr, "[keyrouter][%s] Fail to open the device (id=%d) !\n", __FUNCTION__, DeviceID);
		goto out;
	}

	/* key events */
	DeviceKeyPress(pDev, keyrouter.nInputEvent[INPUTEVENT_KEY_PRESS], cls);
	if( cls )	eventList[INPUTEVENT_KEY_PRESS] = cls;
	DeviceKeyRelease(pDev, keyrouter.nInputEvent[INPUTEVENT_KEY_RELEASE], cls);
	if( cls )	eventList[INPUTEVENT_KEY_RELEASE] = cls;

	result = XGrabDevice(keyrouter.disp, pDev, win, False, INPUTEVENT_KEY_RELEASE+1, eventList, GrabModeAsync, GrabModeAsync, CurrentTime);

	if( result )
	{
		fprintf(stderr, "[keyrouter][%s] Fail to grab the device (error=%d, id=%d) !\n", __FUNCTION__, result, DeviceID);
		if( pDev )	XCloseDevice(keyrouter.disp, pDev);
		goto out;
	}

	return 1;

out:
	return 0;
}

static void detachSlave(int DeviceID)
{
	XIDetachSlaveInfo detach;
	detach.type = XIDetachSlave;
	detach.deviceid = DeviceID;

	XIChangeHierarchy(keyrouter.disp, (XIAnyHierarchyChangeInfo*)&detach, 1);
}

static int GrabKeyDevices(Window win)
{
	int i, ndevices, result;
	XIDeviceInfo *dev, *info = NULL;
	KeyrouterDeviceType kdtype;

	info = XIQueryDevice(keyrouter.disp, XIAllDevices, &ndevices);

	if( !info )
	{
		fprintf(stderr, "[keyrouter][%s] There is no queried XI device.\n", __FUNCTION__);
		return 0;
	}

	for( i = 0; i < ndevices ; i++ )
	{
		dev = &info[i];
		kdtype = E_KEYROUTER_HWKEY;

		if( XISlaveKeyboard == dev->use )
		{

			if( strcasestr(dev->name, "XTEST" ) )
				continue;

			if( strcasestr(dev->name, "keyboard") )
				kdtype = E_KEYROUTER_KEYBOARD;

			if( kdtype == E_KEYROUTER_HWKEY )
			{
				result = GrabKeyDevice(win, dev->name, dev->deviceid);
				if(!result)
				{
					fprintf(stderr, "[keyrouter][%s] Failed to grab key device(name=%s, result=%d)\n", dev->name, result);
					continue;
				}

				_e_keyrouter_grab_hwkeys(0);
			}
			else
			{
				_e_keyrouter_grab_hwkeys(dev->deviceid);
			}

			E_Keyrouter_Device_Info *data = malloc(sizeof(E_Keyrouter_Device_Info));

			if( data )
			{
				data->id = dev->deviceid;
				data->name = eina_stringshare_add(dev->name);
				data->type = kdtype;
				keyrouter.device_list = eina_list_append(keyrouter.device_list, data);
			}

			if( kdtype == E_KEYROUTER_HWKEY )
			{
				detachSlave(dev->deviceid);
				keyrouter.num_hwkey_devices++;
			}
		}
	}

	XIFreeDeviceInfo(info);

	return 1;
}

static void reattachSlave(int slave, int master)
{
	XIAttachSlaveInfo attach;

	attach.type = XIAttachSlave;
	attach.deviceid = slave;
	attach.new_master = master;

	XIChangeHierarchy(keyrouter.disp, (XIAnyHierarchyChangeInfo*)&attach, 1);
}
static void UngrabKeyDevices()
{
	int i, ndevices;
	XIDeviceInfo *dev, *info = NULL;
	XDevice* pDev = NULL;

	info = XIQueryDevice(keyrouter.disp, XIAllDevices, &ndevices);

	if( !info )
	{
		fprintf(stderr, "[keyrouter][%s] There is no queried XI device.\n", __FUNCTION__);
		return;
	}

	for( i = 0; i < ndevices ; i++ )
	{
		dev = &info[i];

		if( XIFloatingSlave != dev->use )
			continue;

		if( dev->num_classes > 1 )//only for Floated SlaveKeyboard
			continue;

		pDev = XOpenDevice(keyrouter.disp, dev->deviceid);

		if( !pDev )
			continue;

		XUngrabDevice(keyrouter.disp, pDev, CurrentTime);

		reattachSlave(dev->deviceid, 3);//reattach to Virtual Core Keyboard
	}

	XIFreeDeviceInfo(info);
}
#else//_F_USE_XI_GRABDEVICE_
static int GrabXIKeyDevices()
{
	int i, ndevices, result;
	XIDeviceInfo *dev, *info = NULL;

	info = XIQueryDevice(keyrouter.disp, XIAllDevices, &ndevices);

	if( !info )
	{
		fprintf(stderr, "[keyrouter][%s] There is no queried XI device.\n", __FUNCTION__);
		return 0;
	}

	XISelectEvents(keyrouter.disp, keyrouter.rootWin, &keyrouter.eventmask, 1);
	for( i = 0; i < ndevices ; i++ )
	{
		dev = &info[i];

		if( XISlaveKeyboard == dev->use )
		{

			if( strcasestr(dev->name, "keyboard") || strcasestr(dev->name, "XTEST" ) )
				continue;

			result = XIGrabDevice(keyrouter.disp, dev->deviceid, keyrouter.rootWin, CurrentTime, None,
					GrabModeAsync, GrabModeAsync, False, &keyrouter.eventmask);

			if( result )
			{
				fprintf(stderr, "[gesture][%s] Failed to grab xi device (id = %d, result = %d)\n", __FUNCTION__, dev->deviceid, result);
				continue;
			}
		}
	}

	XIFreeDeviceInfo(info);

	return 1;
}

static void UngrabXIKeyDevices()
{
	int i, ndevices, result;
	XIDeviceInfo *dev, *info = NULL;

	info = XIQueryDevice(keyrouter.disp, XIAllDevices, &ndevices);

	if( !info )
	{
		fprintf(stderr, "[keyrouter][%s] There is no queried XI device.\n", __FUNCTION__);
		return 0;
	}

	XISelectEvents(keyrouter.disp, keyrouter.rootWin, &keyrouter.eventmask_0, 1);
	for( i = 0; i < ndevices ; i++ )
	{
		dev = &info[i];
		if( XISlaveKeyboard == dev->use )
		{
			if( strcasestr(dev->name, "keyboard") || strcasestr(dev->name, "XTEST" ) )
				continue;

			result = XIUngrabDevice(keyrouter.disp, dev->deviceid, CurrentTime);

			if( result )
			{
				fprintf(stderr, "[keyrouter][%s] Failed to ungrab xi device (id=%d, result=%d)\n", __FUNCTION__, dev->deviceid, result);
				continue;
			}
		}
	}

	XIFreeDeviceInfo(info);
}
#endif

static void PrintKeyDeliveryList()
{
	int index;

#ifdef __DEBUG__
	printf("[keyrouter][%s] ============ start =================\n", __FUNCTION__);
#endif

	for( index=0 ; index < MAX_HARDKEYS ; index++ )
	{
		const char *keyname;

		if( keyrouter.HardKeys[index].keycode == 0 )//empty
			continue;

		fprintf(keyrouter.fplog, "\n");
		keyname = XKeysymToString(XKeycodeToKeysym(keyrouter.disp, index, 0));

		if( !keyname )
		{
#ifdef __DEBUG__
			fprintf(keyrouter.fplog, "[keyrouter][%s] Failed to get string from xkeysym via xkeycode(%d) !\n", __FUNCTION__, index);
#endif
			continue;
		}

		if( !strncmp(keyname, KEY_VOLUMEDOWN, LEN_KEY_VOLUMEDOWN) )
			fprintf(keyrouter.fplog, "[ KEY_VOLUMEDOWN : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_VOLUMEUP, LEN_KEY_VOLUMEUP) )
			fprintf(keyrouter.fplog, "[ KEY_VOLUMEUP : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_PAUSE, LEN_KEY_PAUSE) )
			fprintf(keyrouter.fplog, "[ KEY_PAUSE : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_SEND, LEN_KEY_SEND) )
			fprintf(keyrouter.fplog, "[ KEY_SEND : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_SELECT, LEN_KEY_SELECT) )
			fprintf(keyrouter.fplog, "[ KEY_SELECT : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_END, LEN_KEY_END) )
			fprintf(keyrouter.fplog, "[ KEY_END : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_POWER, LEN_KEY_POWER) )
			fprintf(keyrouter.fplog, "[ KEY_POWER : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_CAMERA, LEN_KEY_CAMERA) )
			fprintf(keyrouter.fplog, "[ KEY_CAMERA : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_CONFIG, LEN_KEY_CONFIG) )
			fprintf(keyrouter.fplog, "[ KEY_CONFIG : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_MEDIA, LEN_KEY_MEDIA) )
			fprintf(keyrouter.fplog, "[ KEY_MEDIA : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_PLAYCD, LEN_KEY_PLAYCD) )
			fprintf(keyrouter.fplog, "[ KEY_PLAYCD : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_STOPCD, LEN_KEY_STOPCD) )
			fprintf(keyrouter.fplog, "[ KEY_STOPCD : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_PAUSECD, LEN_KEY_PAUSECD) )
			fprintf(keyrouter.fplog, "[ KEY_PAUSECD : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_NEXTSONG, LEN_KEY_NEXTSONG) )
			fprintf(keyrouter.fplog, "[ KEY_NEXTSONG : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_PREVIOUSSONG, LEN_KEY_PREVIOUSSONG) )
			fprintf(keyrouter.fplog, "[ KEY_PREVIOUSSONG : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_REWIND, LEN_KEY_REWIND) )
			fprintf(keyrouter.fplog, "[ KEY_REWIND : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_FASTFORWARD, LEN_KEY_FASTFORWARD) )
			fprintf(keyrouter.fplog, "[ KEY_FASTFORWARD : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_PLAYPAUSE, LEN_KEY_PLAYPAUSE) )
			fprintf(keyrouter.fplog, "[ KEY_PLAYPAUSE : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_MUTE, LEN_KEY_MUTE) )
			fprintf(keyrouter.fplog, "[ KEY_MUTE : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_HOMEPAGE, LEN_KEY_HOMEPAGE) )
			fprintf(keyrouter.fplog, "[ KEY_HOMEPAGE : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_WEBPAGE, LEN_KEY_WEBPAGE) )
			fprintf(keyrouter.fplog, "[ KEY_WEBPAGE : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_MAIL, LEN_KEY_MAIL) )
			fprintf(keyrouter.fplog, "[ KEY_MAIL : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_SCREENSAVER, LEN_KEY_SCREENSAVER) )
			fprintf(keyrouter.fplog, "[ KEY_SCREENSAVER : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_BRIGHTNESSUP, LEN_KEY_BRIGHTNESSUP) )
			fprintf(keyrouter.fplog, "[ KEY_BRIGHTNESSUP : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_BRIGHTNESSDOWN, LEN_KEY_BRIGHTNESSDOWN) )
			fprintf(keyrouter.fplog, "[ KEY_BRIGHTNESSDOWN : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_SOFTKBD, LEN_KEY_SOFTKBD) )
			fprintf(keyrouter.fplog, "[ KEY_SOFTKBD : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_QUICKPANEL, LEN_KEY_QUICKPANEL) )
			fprintf(keyrouter.fplog, "[ KEY_QUICKPANEL : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_TASKSWITCH, LEN_KEY_TASKSWITCH) )
			fprintf(keyrouter.fplog, "[ KEY_TASKSWITCH : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_APPS, LEN_KEY_APPS) )
			fprintf(keyrouter.fplog, "[ KEY_APPS : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_SEARCH, LEN_KEY_SEARCH) )
			fprintf(keyrouter.fplog, "[ KEY_SEARCH : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_VOICE, LEN_KEY_VOICE) )
			fprintf(keyrouter.fplog, "[ KEY_VOICE : %s : %d ]\n", keyname, index);
		else if( !strncmp(keyname, KEY_LANGUAGE, LEN_KEY_LANGUAGE) )
			fprintf(keyrouter.fplog, "[ KEY_LANGUAGE : %s : %d ]\n", keyname, index);
		else
			fprintf(keyrouter.fplog, "[ UNKNOWN : %d ]\n", keyrouter.HardKeys[index].keycode);

		//Print EXCLUSIVE mode of grab
		if( NULL != keyrouter.HardKeys[index].excl_ptr )
			fprintf(keyrouter.fplog, "[32m== EXCLUSIVE : Window(0x%X)[0m\n", (int)(keyrouter.HardKeys[index].excl_ptr->wid));
		else
			fprintf(keyrouter.fplog, "== EXCLUSIVE : None\n");

		//Print OR_EXCLUSIVE mode of grab
		if( NULL != keyrouter.HardKeys[index].or_excl_ptr )
			fprintf(keyrouter.fplog, "[32m== OR_EXCLUSIVE : Window(0x%X)[0m\n", (int)(keyrouter.HardKeys[index].or_excl_ptr->wid));
		else
			fprintf(keyrouter.fplog, "== OR_EXCLUSIVE : None\n");

		//Print TOP_POSITION mode of grab
		if( NULL != keyrouter.HardKeys[index].top_ptr )
		{
			keylist_node* top_ptr;
			top_ptr = keyrouter.HardKeys[index].top_ptr;
			fprintf(keyrouter.fplog, "[32m== TOP_POSITION : ");

			do
			{
				fprintf(keyrouter.fplog, "Window(0x%X) -> ", (unsigned int)(top_ptr->wid));
				top_ptr = top_ptr->next;
			} while( top_ptr );
			fprintf(keyrouter.fplog, "None[0m\n");
		}
		else
		{
			fprintf(keyrouter.fplog, "== TOP_POSITION : None\n");
		}

		//Print SHARED mode of grab
		if( NULL != keyrouter.HardKeys[index].shared_ptr )
		{
			keylist_node* shared_ptr;
			shared_ptr = keyrouter.HardKeys[index].shared_ptr;
			fprintf(keyrouter.fplog, "[32m== SHARED : ");

			do
			{
				fprintf(keyrouter.fplog, "Window(0x%X) -> ", (unsigned int)(shared_ptr->wid));
				shared_ptr = shared_ptr->next;
			} while( shared_ptr );
			fprintf(keyrouter.fplog, "None[0m\n");
		}
		else
		{
			fprintf(keyrouter.fplog, "== SHARED : None\n");
		}
	}

	return;
}

static int RemoveWindowDeliveryList(Window win, int isTopPositionMode, int UnSetExclusiveProperty)
{

	int index;
	int mode_count = 0;

	//Remove win from EXCLUSIVE, TOP_POSITION and SHARED  grab list !
	//If isTopPosition is true, remove win only from TOP_POSITION grab list !
	for( index=0 ; index < MAX_HARDKEYS ; index++ )
	{
		if( keyrouter.HardKeys[index].keycode == 0 )//empty
			continue;

		if( isTopPositionMode )
		{
			//Check & Delete TOP_POSITION mode of grab
			if( NULL != keyrouter.HardKeys[index].top_ptr )
			{
				int flags = 0;
				keylist_node* current;
				keylist_node* next_current;

				current = keyrouter.HardKeys[index].top_ptr;

				if( !current ) continue;

				next_current = current->next;

				do
				{
					if( win == keyrouter.HardKeys[index].top_ptr->wid )
					{
						if( current->next )
							keyrouter.HardKeys[index].top_ptr = current->next;
						else
						{
							keyrouter.HardKeys[index].top_ptr = NULL;
							keyrouter.HardKeys[index].top_tail = NULL;
							mode_count += TOP_GRAB_MODE;
						}

						if( current )
							free(current);
						flags = 1;
#ifdef __DEBUG__
						printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].top_ptr (head) !\n", __FUNCTION__, (int)win, index);
#endif
						break;
					}

					if( NULL == next_current )
					{
						break;
					}

					if( win == next_current->wid )
					{
						if( next_current->next )
							current->next = next_current->next;
						else
						{
							current->next = NULL;
							keyrouter.HardKeys[index].top_tail = current;
						}

						if( next_current )
							free(next_current);
						flags = 1;
#ifdef __DEBUG__
						printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].top_ptr (normal node) !\n", __FUNCTION__, (int)win, index);
#endif
						break;
					}

					current = next_current;
					next_current = next_current->next;

				} while( NULL != next_current );

				if( flags )
				{
					continue;
				}
			}
		}
		else//isTopPositionMode == 0
		{
			//Check & Delete EXCLUSIVE mode of grab
			if( NULL != keyrouter.HardKeys[index].excl_ptr )
			{
				if( win == keyrouter.HardKeys[index].excl_ptr->wid )
				{
#ifdef __DEBUG__
					printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].excl_ptr !\n", __FUNCTION__, (int)win, index);
#endif
					if( UnSetExclusiveProperty )
					{
#ifdef __DEBUG__
						printf("[keyrouter][%s] Before call UnSetExclusiveGrabInfoToRootWindow() !\n", __FUNCTION__);
						UnSetExclusiveGrabInfoToRootWindow(keyrouter.HardKeys[index].keycode, EXCLUSIVE_GRAB);
						printf("[keyrouter][%s] After call UnSetExclusiveGrabInfoToRootWindow() !\n", __FUNCTION__);
#else
						UnSetExclusiveGrabInfoToRootWindow(keyrouter.HardKeys[index].keycode, EXCLUSIVE_GRAB);
#endif
					}

					if( keyrouter.HardKeys[index].excl_ptr )
						free(keyrouter.HardKeys[index].excl_ptr);
					keyrouter.HardKeys[index].excl_ptr = NULL;
					mode_count += EXCL_GRAB_MODE;
					continue;//need to check another keycode
				}
			}

			//Check & Delete OR_EXCLUSIVE mode of grab
			if( NULL != keyrouter.HardKeys[index].or_excl_ptr )
			{
				if( win == keyrouter.HardKeys[index].or_excl_ptr->wid )
				{
#ifdef __DEBUG__
					printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].or_excl_ptr !\n", __FUNCTION__, (int)win, index);
#endif
					if( UnSetExclusiveProperty )
					{
#ifdef __DEBUG__
						printf("[keyrouter][%s] Before call UnSetExclusiveGrabInfoToRootWindow() !\n", __FUNCTION__);
						UnSetExclusiveGrabInfoToRootWindow(keyrouter.HardKeys[index].keycode, OR_EXCLUSIVE_GRAB);
						printf("[keyrouter][%s] After call UnSetExclusiveGrabInfoToRootWindow() !\n", __FUNCTION__);
#else
						UnSetExclusiveGrabInfoToRootWindow(keyrouter.HardKeys[index].keycode, OR_EXCLUSIVE_GRAB);
#endif
					}

					if( keyrouter.HardKeys[index].or_excl_ptr )
						free(keyrouter.HardKeys[index].or_excl_ptr);
					keyrouter.HardKeys[index].or_excl_ptr = NULL;
					mode_count += OR_EXCL_GRAB_MODE;
					continue;//need to check another keycode
				}
			}

			//Check & Delete TOP_POSITION mode of grab
			if( NULL != keyrouter.HardKeys[index].top_ptr )
			{
				int flags = 0;
				keylist_node* current;
				keylist_node* next_current;

				current = keyrouter.HardKeys[index].top_ptr;

				if( !current )
					goto null_top_ptr;

				next_current = current->next;

				do
				{
					if( win == keyrouter.HardKeys[index].top_ptr->wid )
					{
						if( current->next )
							keyrouter.HardKeys[index].top_ptr = current->next;
						else
						{
							keyrouter.HardKeys[index].top_ptr = NULL;
							keyrouter.HardKeys[index].top_tail = NULL;
							mode_count += TOP_GRAB_MODE;
						}

						if( current )
							free(current);
						flags = 1;
#ifdef __DEBUG__
						printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].top_ptr (head) !\n", __FUNCTION__, (int)win, index);
#endif
						break;
					}

					if( NULL == next_current )
					{
						break;
					}

					if( win == next_current->wid )
					{
						if( next_current->next )
							current->next = next_current->next;
						else
						{
							current->next = NULL;
							keyrouter.HardKeys[index].top_tail = current;
						}

						if( next_current )
							free(next_current);
						flags = 1;
#ifdef __DEBUG__
						printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].top_ptr (normal node) !\n", __FUNCTION__, (int)win, index);
#endif
						break;
					}

					current = next_current;
					next_current = next_current->next;

				} while( NULL != next_current );

				if( flags )
				{
					continue;
				}
			}

null_top_ptr:
			//Check & Delete SHARED mode of grab
			if( NULL != keyrouter.HardKeys[index].shared_ptr )
			{
				int flags = 0;
				keylist_node* current = NULL;
				keylist_node* next_current = NULL;

				current = keyrouter.HardKeys[index].shared_ptr;
                                if (current)
                                  {
                                     next_current = current->next;
                                     do
                                       {
                                          if (win == keyrouter.HardKeys[index].shared_ptr->wid)
                                            {
                                               if (current->next)
                                                 keyrouter.HardKeys[index].shared_ptr = current->next;
                                               else
                                                 {
                                                    keyrouter.HardKeys[index].shared_ptr = NULL;
                                                    mode_count += SHARED_GRAB_MODE;
                                                 }

                                               if (current)
                                                 {
                                                    free(current);
                                                    current = NULL;
                                                 }
                                               flags = 1;
#ifdef __DEBUG__
                                               printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].shared_ptr (head) !\n",
                                                      __FUNCTION__, (int)win, index);
#endif
                                               break;
                                            }

                                          if (NULL == next_current)
                                            {
                                               break;
                                            }

                                          if (win == next_current->wid)
                                            {
                                               if (next_current->next)
                                                 current->next = next_current->next;
                                               else
                                                 current->next = NULL;

                                               if (next_current)
                                                 {
                                                    free(next_current);
                                                    next_current = NULL;
                                                 }
                                               flags = 1;
#ifdef __DEBUG_
                                               printf("[keyrouter][%s] window (0x%x) was found and removed @ keyrouter.HardKeys[%d].shared_ptr (normal node) !\n",
                                                      __FUNCTION__, (int)win, index);
#endif
                                               break;
                                            }

                                          current = next_current;
                                          next_current = next_current->next;

                                       } while (next_current);
                                  }

				if( flags )
				{
					continue;
				}
			}
		}
	}

	return 0;
}

static void UnSetExclusiveGrabInfoToRootWindow(int keycode, int grab_mode)
{
	int i;
	int cnt = 0;
	unsigned int *key_list = NULL;
	int *new_key_list = NULL;

	Atom ret_type;
	int ret_format;
	unsigned long nr_item;
	unsigned long sz_remains_data;
	Window ex_grabwin;

	if( grab_mode == EXCLUSIVE_GRAB )
	{
	if( keyrouter.atomGrabExclWin == None )
		keyrouter.atomGrabExclWin = XInternAtom(keyrouter.disp, STR_ATOM_GRAB_EXCL_WIN, False);
		ex_grabwin = keyrouter.atomGrabExclWin;
	}
	else if( grab_mode == OR_EXCLUSIVE_GRAB )
	{
		if( keyrouter.atomGrabORExclWin == None )
			keyrouter.atomGrabORExclWin = XInternAtom(keyrouter.disp, STR_ATOM_GRAB_OR_EXCL_WIN, False);
		ex_grabwin = keyrouter.atomGrabORExclWin;
	}
	else
		return;

	if (XGetWindowProperty(keyrouter.disp, keyrouter.rootWin,
		ex_grabwin, 0, 0x7fffffff, False, XA_CARDINAL,
		&ret_type, &ret_format, &nr_item, &sz_remains_data, (unsigned char **)&key_list) != Success)
	{
		nr_item = 0;
	}

	if (nr_item == 0)
	{
		fprintf(stderr, "[32m[keyrouter][%s] keycode = %d[0m\n", __FUNCTION__, keycode);
		goto out;
	}

	for( i=0 ; i < nr_item ; i++ )
	{
		if( key_list[i] == keycode )//&& grab_mode == EXCLUSIVE_GRAB )
		{
			continue;
		}
		cnt++;
	}

#ifdef __DEBUG__
	fprintf(stderr, "[keyrouter][%s] cnt = %d, nr_item = %d\n", __FUNCTION__, cnt, (int)nr_item);
#endif

	if( 0 < cnt )
	{
		new_key_list = malloc(sizeof(int)*cnt);
		cnt = 0;
	}
	else
		new_key_list = NULL;

	if( !new_key_list )
	{
		//fprintf(stderr, "[32m[keyrouter][%s] Fail to allocation memory for new_key_list ! [0m\n", __FUNCTION__);
		XDeleteProperty(keyrouter.disp, keyrouter.rootWin, ex_grabwin);
		XSync(keyrouter.disp, False);
		goto out;
	}

	for( i=0 ; i < nr_item ; i++ )
	{
		if( key_list[i] == keycode )//&& grab_mode == EXCLUSIVE_GRAB )
		{
			continue;
		}
		else
			new_key_list[cnt++] = key_list[i];
	}

	XChangeProperty(keyrouter.disp, keyrouter.rootWin, ex_grabwin, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)new_key_list, cnt);
	XSync(keyrouter.disp, False);

out:
	if(new_key_list)
		free(new_key_list);
	if( key_list )
		XFree(key_list);
	return;
}

static int AddWindowToDeliveryList(Window win, int keycode, const int grab_mode, const int IsOnTop)
{
	int ret = 0;
	int index = keycode;
	keylist_node *backup_ptr = NULL;

	if( index >= MAX_HARDKEYS )
	{
		fprintf(stderr, "[keyrouter][%s] Error ! index of keyrouter.HardKeys must be smaller than %d (index=%d)!)\n", __FUNCTION__, MAX_HARDKEYS, index);
		ret = -1;
		goto out;
	}

	keylist_node* ptr = NULL;
	keyrouter.HardKeys[index].keycode = keycode;
	switch( grab_mode )
	{
		case EXCLUSIVE_GRAB:
			if( NULL != keyrouter.HardKeys[index].excl_ptr )
			{
				fprintf(stderr, "[keyrouter][%s] keyrouter.HardKeys[%d].Keycode(%d) was EXCLUSIVELY grabbed already by window(0x%x) !\n", __FUNCTION__, index, keycode, (int)(keyrouter.HardKeys[index].excl_ptr->wid));
				ret = -1;
				goto out;
			}

			ptr = (keylist_node*)malloc(sizeof(keylist_node));
			if( !ptr )
			{
				fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for adding excl_ptr!\n", __FUNCTION__);
				ret = -1;
				goto out;
			}

			ptr->wid = win;
			ptr->next = NULL;
			keyrouter.HardKeys[index].excl_ptr = ptr;

#ifdef __DEBUG__
			printf("[keyrouter][%s] window(0x%x) was added to EXCLUSVE mode list (keyrouter.HardKeys[%d]) !\n", __FUNCTION__, (int)win, index);
#endif
			break;

		case OR_EXCLUSIVE_GRAB:
			if( NULL != keyrouter.HardKeys[index].or_excl_ptr )
			{
				fprintf(stderr, "[keyrouter][%s] keyrouter.HardKeys[%d].Keycode(%d) was OR_EXCLUSIVELY grabbed already by window(0x%x) !\n", __FUNCTION__, index, keycode, (unsigned int)(keyrouter.HardKeys[index].or_excl_ptr->wid));
				fprintf(stderr, "[keyrouter][%s] Now it will be overridden by a new window(0x%x) !\n", __FUNCTION__, (unsigned int)win);
				backup_ptr = keyrouter.HardKeys[index].or_excl_ptr;
			}

			ptr = (keylist_node*)malloc(sizeof(keylist_node));
			if( !ptr )
			{
				fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for adding excl_ptr!\n", __FUNCTION__);
				ret = -1;
				goto out;
			}

			ptr->wid = win;
			ptr->next = NULL;
			keyrouter.HardKeys[index].or_excl_ptr = ptr;
			if( backup_ptr )
				free(backup_ptr);

#ifdef __DEBUG__
			printf("[keyrouter][%s] window(0x%x) was added to OR_EXCLUSVE mode list (keyrouter.HardKeys[%d]) !\n", __FUNCTION__, (int)win, index);
#endif
			break;

		case TOP_POSITION_GRAB:
			ptr = (keylist_node*)malloc(sizeof(keylist_node));
			if( !ptr )
			{
				fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for adding top_ptr!\n", __FUNCTION__);
				ret = -1;
				goto out;
			}

#ifdef __DEBUG__
			printf("[keyrouter][%s] window(0x%x) was added to TOP_POSITION mode list (keyrouter.HardKeys[%d]) !\n", __FUNCTION__, (int)win, index);
#endif

			ptr->wid = win;
			ptr->next = NULL;

			if( NULL == keyrouter.HardKeys[index].top_ptr )//µ¥ÀÌÅÍ ¾øÀ½
			{
				keyrouter.HardKeys[index].top_tail = keyrouter.HardKeys[index].top_ptr = ptr;
				break;
			}

			if( IsOnTop )
			{
				ptr->next = keyrouter.HardKeys[index].top_ptr;
				keyrouter.HardKeys[index].top_ptr = ptr;
			}
			else
			{
				keyrouter.HardKeys[index].top_tail->next = ptr;
				keyrouter.HardKeys[index].top_tail = ptr;
			}
			break;

		case SHARED_GRAB:
			ptr = (keylist_node*)malloc(sizeof(keylist_node));
			if( !ptr )
			{
				fprintf(stderr, "[keyrouter][%s] Failed to allocate memory for adding shared_ptr!\n", __FUNCTION__);
				ret = -1;
				goto out;
			}

			ptr->wid = win;
			if( NULL != keyrouter.HardKeys[index].shared_ptr )//µ¥ÀÌÅÍ Á¸Àç
			{
				ptr->next = keyrouter.HardKeys[index].shared_ptr;
				keyrouter.HardKeys[index].shared_ptr = ptr;
			}
			else//µ¥ÀÌÅÍ ¾øÀ½
			{
				ptr->next = NULL;
				keyrouter.HardKeys[index].shared_ptr = ptr;
			}
#ifdef __DEBUG__
			printf("[keyrouter][%s] window(0x%x) was added to SHARED mode list (keyrouter.HardKeys[%d]) !\n", __FUNCTION__, (int)win, index);
#endif
			break;

		default:
			fprintf(stderr, "[keyrouter][%s] Unknown mode of grab ! (grab_mode=0x%X)\n", __FUNCTION__, grab_mode);
			ret = -1;
			break;
	}

out:

	return ret;
}

static int AdjustTopPositionDeliveryList(Window win, int IsOnTop)
{
	Atom ret_type;
	int ret_format;
	unsigned long nr_item = 0;
	unsigned long sz_remains_data;
	unsigned int *key_list = NULL;

	int i, result;
	int grab_mode, keycode;

	if (keyrouter.atomGrabKey == None) {
		keyrouter.atomGrabKey = XInternAtom(keyrouter.disp, STR_ATOM_GRAB_KEY, False);
	}

	if ( Success != (result = XGetWindowProperty(keyrouter.disp, win, keyrouter.atomGrabKey, 0, 0x7fffffff, False, XA_CARDINAL,
				&ret_type, &ret_format, &nr_item,
				&sz_remains_data,(unsigned char **)&key_list) ) )
	{
		fprintf(stderr, "[keyrouter][%s] Failed to get window property from %s ! (result = %d)\n", __FUNCTION__, STR_ATOM_GRAB_KEY, result);
		RemoveWindowDeliveryList(win, 1, 0);
		goto out;
	}

	if( 0 == nr_item )
	{
		RemoveWindowDeliveryList(win, 1, 0);
		goto out;
	}

	RemoveWindowDeliveryList(win, 1, 0);

	for( i=0 ; i < nr_item ; i++ )
	{
		grab_mode = key_list[i] & GRAB_MODE_MASK;

		if( TOP_POSITION_GRAB != grab_mode )
			continue;

		keycode = key_list[i] & (~GRAB_MODE_MASK);
		_e_keyrouter_update_key_delivery_list(win, keycode, TOP_POSITION_GRAB, IsOnTop);
	}

out:
	if( key_list )
			XFree(key_list);
	return 0;
}

static int IsGrabbed(unsigned int keycode)
{
	int index = keycode;

	if( keyrouter.HardKeys[index].keycode == 0 )//empty
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] keyrouter.HardKeys[%d].keycode is 0\n", __FUNCTION__, index);
#endif
		goto out;
	}

	if( keyrouter.HardKeys[index].keycode != keycode )
	{
		fprintf(stderr, "[keyrouter][%s] Error ! (keyrouter.HardKeys[%d].keycode must be equal to keycode(%d) !\n", __FUNCTION__, index, keycode);
		goto out;
	}

	if( NULL != keyrouter.HardKeys[index].excl_ptr )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] keyrouter.HardKeys[%d] is grabbed @ EXCLUSIVE mode by window(0x%x)\n", __FUNCTION__, index, (int)(keyrouter.HardKeys[index].excl_ptr->wid));
#endif
		index |= EXCLUSIVE_GRAB;
	}
	else if( NULL != keyrouter.HardKeys[index].or_excl_ptr )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] keyrouter.HardKeys[%d] is grabbed @ OR_EXCLUSIVE mode by window(0x%x)\n", __FUNCTION__, index, (int)(keyrouter.HardKeys[index].or_excl_ptr->wid));
#endif
		index |= OR_EXCLUSIVE_GRAB;
	}
	else if( NULL != keyrouter.HardKeys[index].top_ptr )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] keyrouter.HardKeys[%d] is grabbed @ TOP_POSITION mode by window(0x%x)\n", __FUNCTION__, index, (int)(keyrouter.HardKeys[index].top_ptr->wid));
#endif
		index |= TOP_POSITION_GRAB;
	}
	else if( NULL != keyrouter.HardKeys[index].shared_ptr )
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] keyrouter.HardKeys[%d] is grabbed @ SHARED mode by window(0x%x)\n", __FUNCTION__, index, (int)(keyrouter.HardKeys[index].shared_ptr->wid));
#endif
		index |= SHARED_GRAB;
	}
	else
	{
#ifdef __DEBUG__
		printf("[keyrouter][%s] keyrouter.HardKeys[%d] has keycode(%d) but not grabbed !\n", __FUNCTION__, index, keycode);
#endif
		index = -1;
	}
	return index;

out:
	return -1;
}

static int IsWindowTopVisibleWithoutInputFocus(Window win, Window focus)
{
	Window root_win, parent_win;
	unsigned int num_children;
	Window *child_list = NULL;

	int i;
	XWindowAttributes win_attributes;
	E_Border *bd;

 	if( !XQueryTree(keyrouter.disp, keyrouter.rootWin, &root_win, &parent_win, &child_list, &num_children) )
	{
		fprintf(stderr, "[32m[keyrouter][%s] Failed to query window tree ![0m\n", __FUNCTION__);
		return 0;
	}

	/* Make the wininfo list */
	for (i = (int)num_children - 1; i >= 0; i--)
	{
		// 1. check map status of window
		/* figure out whether the window is mapped or not */
		XGetWindowAttributes(keyrouter.disp, child_list[i], &win_attributes);

		// 2. return if map status is 0 or border's visible is 0
		if( win_attributes.map_state == 0 )
			continue;

		// 3. check window is border or not
		bd = e_border_find_by_window(child_list[i]);

		if( bd )//child_list[i] is border
		{//if the window is client window, check hint
#ifdef __DEBUG__
			fprintf(stderr, "[32m[keyrouter][%s] bd is NOT NULL!(child_list[%d]=0x%x, bd->win=0x%x, bd->client.win=0x%x, win=0x%x)[0m\n",
				__FUNCTION__, i, (unsigned int)child_list[i], bd->win, bd->client.win, (unsigned int)win);
#endif

			if( !bd->visible )
				continue;
			if( (bd->x >= bd->zone->w) || (bd->y >= bd->zone->h) )
				continue;
			if( ((bd->x + bd->w) <= 0) || ((bd->y + bd->h) <= 0) )
				continue;

			if( bd->client.win == win )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[32m[keyrouter][%s][CLIENTWIN] Target win(0x%x) is above focus win(0x%x)[0m\n", __FUNCTION__, (unsigned int)win, (unsigned int)focus);
#endif
                                XFree(child_list);
				return 1;
			}

			if( bd->client.win == focus )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[32m[keyrouter][%s][CLIENTWIN] Target win(0x%x) is below focus win(0x%x)[0m\n", __FUNCTION__, (unsigned int)win, (unsigned int)focus);
#endif
                                XFree(child_list);
				return 0;
			}
		}
		else//child_list[i] is override-redirected window
		{//if the window is not client window, it will be a border window or a override-redirected window then check the equality of the windows
#ifdef __DEBUG__
			fprintf(stderr, "[32m[keyrouter][%s] bd is NULL!(child_list[%d]=0x%x, win=0x%x)[0m\n",
				__FUNCTION__, i, (unsigned int)child_list[i], (unsigned int)win);
#endif

			if( child_list[i] == win )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[32m[keyrouter][%s][WIN] Target win(0x%x) is above focus win(0x%x)[0m\n", __FUNCTION__, (unsigned int)win, (unsigned int)focus);
#endif
                                XFree(child_list);
				return 1;
			}

			if( child_list[i] == focus )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[32m[keyrouter][%s][WIN] Target win(0x%x) is below focus win(0x%x)[0m\n", __FUNCTION__, (unsigned int)win, (unsigned int)focus);
#endif
                                XFree(child_list);
				return 0;
			}
		}
	}

        XFree(child_list);
	return 0;
}

#ifdef _F_USE_XI_GRABDEVICE_
static void DeliverKeyEvents(XEvent *xev, XGenericEventCookie *cookie)
{
	int index, rel_index, count;
	int revert_to_return;
	Window focus_window;
	keylist_node* ptr = NULL;
	XIDeviceEvent *xiData = (XIDeviceEvent *)cookie->data;

	index = IsGrabbed(xev->xkey.keycode);
	rel_index = index & (~GRAB_MODE_MASK);
	rel_index = xev->xkey.keycode;
	XGetInputFocus(keyrouter.disp, &focus_window, &revert_to_return);

	if( xev->type == KeyRelease )
	{
		switch( keyrouter.HardKeys[rel_index].lastmode )
		{
			case NONE_GRAB_MODE:
				xiData->event = xev->xkey.window = keyrouter.HardKeys[rel_index].lastwid;
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
				fprintf(stderr, "[32m[keyrouter][%s] Non-grabbed key! Deliver KeyRelease/XI_KeyRelease (keycode:%d) to %s window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (xev->xkey.window==focus_window) ? "focus":"", xev->xkey.window);
 				break;

			case EXCL_GRAB_MODE:
				xiData->event = xev->xkey.window = keyrouter.HardKeys[rel_index].lastwid;
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
				fprintf(stderr, "[32m[keyrouter][%s] EXCLUSIVE mode of grab ! Deliver KeyRelease/XI_KeyRelease (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				break;

			case OR_EXCL_GRAB_MODE:
				xiData->event = xev->xkey.window = keyrouter.HardKeys[rel_index].lastwid;
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
				fprintf(stderr, "[32m[keyrouter][%s] OR_EXCLUSIVE mode of grab ! Deliver KeyRelease/XI_KeyRelease (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				break;

			case TOP_GRAB_MODE:
				xiData->event = xev->xkey.window = keyrouter.HardKeys[rel_index].lastwid;
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
				fprintf(stderr, "[32m[keyrouter][%s] TOP_POSITION mode of grab ! Deliver KeyRelease/XI_KeyRelease (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				break;

			case SHARED_GRAB_MODE:
				if( !keyrouter.HardKeys[rel_index].num_shared_wins )
					break;

				for( count = 0 ; count < keyrouter.HardKeys[rel_index].num_shared_wins ; count++ )
				{
					xiData->event = xev->xkey.window = keyrouter.HardKeys[rel_index].shared_wins[count];
					XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
					XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
					fprintf(stderr, "[32m[keyrouter][%s] Non-grabbed or SHARED mode of grab ! Deliver KeyRelease/XI_KeyRelease (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				}
				break;

			default:
				fprintf(stderr, "[[32m[keyrouter][%s] Unknown case (keycode:%d)![0m\n", __FUNCTION__, xev->xkey.keycode);
		}

		return;
	}

	// Is Grabbed ?
	if( index < 0 )//Code for non-grabbed key
	{
		//Deliver to focus window
		xiData->event = xev->xkey.window = focus_window;

		if( xev->type == KeyPress )
		{
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
			fprintf(stderr, "[32m[keyrouter][%s] Non-grabbed key! Deliver KeyPress/XI_KeyPress (keycode:%d) to focus window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)focus_window);

			keyrouter.HardKeys[xev->xkey.keycode].lastwid = xev->xkey.window;
			keyrouter.HardKeys[xev->xkey.keycode].lastmode = NONE_GRAB_MODE;
		}

		return;
	}

	int grab_mode = index & GRAB_MODE_MASK;
	index &= ~GRAB_MODE_MASK;

	switch( grab_mode )
	{
		case EXCLUSIVE_GRAB:
			// Is Grab Mode equal to EXCLUSIVE ?
			xiData->event = xev->xkey.window = keyrouter.HardKeys[index].excl_ptr->wid;
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2

			if( xev->type == KeyPress )
			{
				fprintf(stderr, "[32m[keyrouter][%s] EXCLUSIVE mode of grab ! Deliver KeyPress/XI_KeyPress (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				keyrouter.HardKeys[index].lastwid = xev->xkey.window;
				keyrouter.HardKeys[index].lastmode = EXCL_GRAB_MODE;
			}
			break;

		case OR_EXCLUSIVE_GRAB:
			// Is Grab Mode equal to OR_EXCLUSIVE ?
			xiData->event = xev->xkey.window = keyrouter.HardKeys[index].or_excl_ptr->wid;
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2

			if( xev->type == KeyPress )
			{
				fprintf(stderr, "[32m[keyrouter][%s] OR_EXCLUSIVE mode of grab ! Deliver KeyPress/XI_KeyPress (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				keyrouter.HardKeys[index].lastwid = xev->xkey.window;
				keyrouter.HardKeys[index].lastmode = OR_EXCL_GRAB_MODE;
			}
			break;

		case TOP_POSITION_GRAB:
			if( focus_window != keyrouter.HardKeys[index].top_ptr->wid )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[32m[keyrouter][%s] isWindowStackChanged = %d[0m\n", __FUNCTION__, isWindowStackChanged);
#endif
				if( keyrouter.isWindowStackChanged || (keyrouter.prev_sent_keycode != xev->xkey.keycode) )
					keyrouter.resTopVisibleCheck = IsWindowTopVisibleWithoutInputFocus(keyrouter.HardKeys[index].top_ptr->wid, focus_window);

				keyrouter.prev_sent_keycode = xev->xkey.keycode;

				if( !keyrouter.resTopVisibleCheck )
					goto shared_delivery;

				if( keyrouter.isWindowStackChanged )
					keyrouter.isWindowStackChanged = 0;
			}

			// Is Grab Mode equal to TOP_POSITION ?
			xiData->event = xev->xkey.window = keyrouter.HardKeys[index].top_ptr->wid;
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
			XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2

			if( xev->type == KeyPress )
			{
				fprintf(stderr, "[32m[keyrouter][%s] TOP_POSITION mode of grab ! Deliver KeyPress/XI_KeyPress (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
				keyrouter.HardKeys[index].lastwid = xev->xkey.window;
				keyrouter.HardKeys[index].lastmode = TOP_GRAB_MODE;
			}
			break;

		case SHARED_GRAB:
shared_delivery:
			keyrouter.HardKeys[index].lastwid = None;
			keyrouter.HardKeys[index].lastmode = SHARED_GRAB_MODE;
			for( ptr=keyrouter.HardKeys[index].shared_ptr ; (NULL != ptr) ; ptr=ptr->next )
			{
				xiData->event = xev->xkey.window = ptr->wid;
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
				XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2

				if( xev->type == KeyPress )
					fprintf(stderr, "[32m[keyrouter][%s] SHARED mode of grab ! Deliver KeyPress/XI_KeyPress (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, xev->xkey.keycode, (int)xev->xkey.window);
			}

			for( ptr=keyrouter.HardKeys[index].shared_ptr ; (NULL != ptr) ; ptr=ptr->next )
			{
				if( ptr->wid == focus_window )
				{
					is_focus_window_in_shared_list = 1;
					break;
				}
			}

			if( !is_focus_window_in_shared_list )
			{
				xiData->event = xev->xkey.window = focus_window;
				if( keyrouter.isWindowStackChanged )
					BackedupSharedWins(index, focus_window);

				if( xev->type == KeyPress )
				{
					XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, xev);
					XSendEvent(keyrouter.disp, xev->xkey.window, False, NoEventMask, cookie);//XI2
					fprintf(stderr, "[32m[keyrouter][%s] Deliver KeyPress/XI_KeyPress (keycode:%d) to focus window (0x%x)![0m\n", __FUNCTION__, xev->xkey.keycode, xev->xkey.window);
				}
			}
			else
				if( keyrouter.isWindowStackChanged )
					BackedupSharedWins(index, None);

			if( keyrouter.isWindowStackChanged )
				keyrouter.isWindowStackChanged = 0;
			break;

		default:
			fprintf(stderr, "[32m[keyrouter][%s] Unknown mode of grab (mode = %d, index = %d, keycode = %d)[0m\n", __FUNCTION__, grab_mode, index, xev->xkey.keycode);
 			break;
	}
}
#else//_F_USE_XI_GRABDEVICE_
static void DeliverDeviceKeyEvents(XEvent *xev, int replace_key)
{
	int index;
	int revert_to_return;
	Window focus_window;
	keylist_node* ptr = NULL;

	index = IsGrabbed(xev->xkey.keycode);

	if( index < 0 && keyrouter.HardKeys[xev->xkey.keycode].bind )
	{
		fprintf(stderr, "[keyrouter][DeliverDeviceKeyEvents] key(keycode=%d, name=%s) was bound !\n",
			xev->xkey.keycode, keyrouter.HardKeys[xev->xkey.keycode].bind->key);
		_e_keyrouter_do_bound_key_action(xev);
		return;
	}

	XGetInputFocus(xev->xany.display, &focus_window, &revert_to_return);

	if( replace_key )
		xev->xkey.keycode = replace_key;

	// Is Grabbed ?
	if( index < 0 )//Code for non-grabbed key
	{
		//Deliver to focus window
		xev->xkey.window = focus_window;

		XTestFakeKeyEvent(xev->xany.display, xev->xkey.keycode, (xev->type==KeyPress) ? True : False, CurrentTime);
		fprintf(stderr, "[32m[keyrouter][%s] Non-grabbed key! Deliver %s (keycode:%d) to focus window (0x%x) ![0m\n", __FUNCTION__, (xev->type == KeyPress) ? "KeyPress" : "KeyRelease", xev->xkey.keycode, (int)focus_window);
		return;
	}

	int grab_mode = index & GRAB_MODE_MASK;
	index &= ~GRAB_MODE_MASK;

	switch( grab_mode )
	{
		case EXCLUSIVE_GRAB:
			// Is Grab Mode equal to EXCLUSIVE ?
			xev->xkey.window = keyrouter.HardKeys[index].excl_ptr->wid;
			XSendEvent(xev->xany.display, xev->xkey.window, False, NoEventMask, xev);
			fprintf(stderr, "[32m[keyrouter][%s] EXCLUSIVE mode of grab ! Deliver %s (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, (xev->type == KeyPress) ? "KeyPress" : "KeyRelease", xev->xkey.keycode, (int)xev->xkey.window);
			break;

		case OR_EXCLUSIVE_GRAB:
			// Is Grab Mode equal to OR_EXCLUSIVE ?
			xev->xkey.window = keyrouter.HardKeys[index].or_excl_ptr->wid;
			XSendEvent(xev->xany.display, xev->xkey.window, False, NoEventMask, xev);
			fprintf(stderr, "[32m[keyrouter][%s] OR_EXCLUSIVE mode of grab ! Deliver %s (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, (xev->type == KeyPress) ? "KeyPress" : "KeyRelease", xev->xkey.keycode, (int)xev->xkey.window);
			break;

		case TOP_POSITION_GRAB:
			if( focus_window != keyrouter.HardKeys[index].top_ptr->wid )
			{
#ifdef __DEBUG__
				fprintf(stderr, "[32m[keyrouter][%s] isWindowStackChanged = %d[0m\n", __FUNCTION__, keyrouter.isWindowStackChanged);
#endif
				if( keyrouter.isWindowStackChanged || (keyrouter.prev_sent_keycode != xev->xkey.keycode) )
					keyrouter.resTopVisibleCheck = IsWindowTopVisibleWithoutInputFocus(keyrouter.HardKeys[index].top_ptr->wid, focus_window);

				keyrouter.prev_sent_keycode = xev->xkey.keycode;

				if( !keyrouter.resTopVisibleCheck )
					goto shared_delivery;

				if( keyrouter.isWindowStackChanged )
					keyrouter.isWindowStackChanged = 0;
			}

			// Is Grab Mode equal to TOP_POSITION ?
			xev->xkey.window = keyrouter.HardKeys[index].top_ptr->wid;
			XSendEvent(xev->xany.display, xev->xkey.window, False, NoEventMask, xev);
			fprintf(stderr, "[32m[keyrouter][%s] TOP_POSITION mode of grab ! Deliver %s (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, (xev->type == KeyPress) ? "KeyPress" : "KeyRelease", xev->xkey.keycode, (int)xev->xkey.window);
			break;

		case SHARED_GRAB:
shared_delivery:
			//Deliver to focus_window first
			xev->xkey.window = focus_window;
			XTestFakeKeyEvent(xev->xany.display, xev->xkey.keycode, (xev->type==KeyPress) ? True : False, CurrentTime);
			fprintf(stderr, "[32m[keyrouter][%s] Deliver %s (keycode:%d) to focus window (0x%x)![0m\n", __FUNCTION__, (xev->type == KeyPress) ? "KeyPress" : "KeyRelease", xev->xkey.keycode, (unsigned int)xev->xkey.window);

			//Deliver to shared grabbed window(s)
			for( ptr=keyrouter.HardKeys[index].shared_ptr ; (NULL != ptr) ; ptr=ptr->next )
			{
				if( ptr->wid == focus_window )
					continue;
				xev->xkey.window = ptr->wid;
				XSendEvent(xev->xany.display, xev->xkey.window, False, NoEventMask, xev);
				fprintf(stderr, "[32m[keyrouter][%s] SHARED mode of grab ! Deliver %s (keycode:%d) to window (0x%x) ![0m\n", __FUNCTION__, (xev->type == KeyPress) ? "KeyPress" : "KeyRelease", xev->xkey.keycode, (int)xev->xkey.window);
			}
			break;

		default:
			fprintf(stderr, "[32m[keyrouter][%s] Unknown mode of grab (mode = %d, index = %d, keycode = %d)[0m\n", __FUNCTION__, grab_mode, index, xev->xkey.keycode);
 			break;
	}
}
#endif//_F_USE_XI_GRABDEVICE_

#ifdef _F_ENABLE_MOUSE_POPUP
static void _e_keyrouter_popup_btn_down_cb(void *data)
{
	char *label = (char*)data;
	keyrouter.rbutton_pressed_on_popup = 1;
	_e_keyrouter_do_hardkey_emulation(label, KeyPress, 0, 0, 0);
}

static void _e_keyrouter_popup_btn_up_cb(void *data)
{
	char *label = (char*)data;
	keyrouter.rbutton_pressed_on_popup = 0;
	_e_keyrouter_do_hardkey_emulation(label, KeyRelease, 1, 0, 0);
}
#endif//_F_ENABLE_MOUSE_POPUP

static void _e_keyrouter_do_hardkey_emulation(const char *label, unsigned int key_event, unsigned int on_release, int keycode, int cancel)
{
#ifdef _F_ENABLE_MOUSE_POPUP
	int i;
	char buf[128];
#endif//_F_ENABLE_MOUSE_POPUP
	XEvent xev;

	if( !label )
		goto normal_hardkey_handler;
	else
		goto reserved_hardkey_handler;

normal_hardkey_handler:
	xev.xkey.display = keyrouter.disp;
	xev.xkey.root = keyrouter.rootWin;
	xev.xkey.keycode = keycode;
	xev.xkey.time = CurrentTime;
	xev.xkey.type = key_event;

	if( cancel )
	{
		DeliverDeviceKeyEvents(&xev, keyrouter.cancel_key.keycode);
		xev.xkey.keycode = keycode;
		xev.xkey.time = CurrentTime;
		xev.xkey.type = KeyRelease;
		DeliverDeviceKeyEvents(&xev, 0);
		xev.xkey.keycode = keycode;
		xev.xkey.time = CurrentTime;
		xev.xkey.type = KeyRelease;
		DeliverDeviceKeyEvents(&xev, keyrouter.cancel_key.keycode);
		fprintf(stderr, "[keyrouter][do_hardkey_emulation] HWKeyEmulation Done !\n");
		fprintf(stderr, "...( Cancel KeyPress + KeyRelease(keycode:%d) + Cancel KeyRelease )\n", xev.xkey.keycode);
	}
	else
	{
		DeliverDeviceKeyEvents(&xev, 0);
		fprintf(stderr, "[keyrouter][do_hardkey_emulation] HWKeyEmulation Done !\n");
		fprintf(stderr, "...( %s(keycode=%d )\n", (xev.xkey.type==KeyPress) ? "KeyPress" : "KeyRelease", xev.xkey.keycode);
	}

	return;

reserved_hardkey_handler:
#ifdef _F_ENABLE_MOUSE_POPUP
	for( i = 0 ; i < 3 ; i++ )
	{
		if( !strcmp(label, btns_label[i]) )
		{
			xev.xany.display = keyrouter.disp;
			xev.xkey.keycode = keyrouter.btn_keys[i];
			xev.xkey.time = 0;

			xev.xkey.type = key_event;
			DeliverDeviceKeyEvents(&xev, 0);
			goto out;
		}
	}

	if( on_release && !strcmp(label, btns_label[3]) )//Rotate Screen
	{
		keyrouter.toggle %= 4;
		sprintf(buf, "/usr/bin/vconftool set -t int memory/sensor/10001 %d", keyrouter.toggle+1);
		fprintf(stderr, "[keyrouter][rotation] %s\n", buf);
		system (buf);
	}
#endif//_F_ENABLE_MOUSE_POPUP

out:
	return;
}

#ifdef _F_ENABLE_MOUSE_POPUP
static void InitHardKeyCodes()
{
	keyrouter.btn_keys[0] = XKeysymToKeycode(keyrouter.disp, XStringToKeysym(KEY_VOLUMEUP));
	keyrouter.btn_keys[1] = XKeysymToKeycode(keyrouter.disp, XStringToKeysym(KEY_VOLUMEDOWN));
	keyrouter.btn_keys[2] = XKeysymToKeycode(keyrouter.disp, XStringToKeysym(KEY_SELECT));
}

static void popup_destroy()
{
	if( !keyrouter.popup )
		return;

	ecore_x_pointer_ungrab();
	XISelectEvents(keyrouter.disp, keyrouter.rootWin, &keyrouter.eventmask_part, 1);

	e_popup_hide(keyrouter.popup);
	e_object_del( E_OBJECT (keyrouter.popup));
	keyrouter.popup = NULL;
}

static void popup_update()
{
	popup_destroy();
	popup_show();
}

static void popup_show()
{
	if( keyrouter.popup )
	{
		fprintf(stderr, "[keyrouter][%s] popup is already displayed...\n", __FUNCTION__);
		return;
	}

	if( !keyrouter.zone )
	{
		fprintf(stderr, "[keyrouter][%s] popup couldn't be displayed because zone is null !\n", __FUNCTION__);
		return;
	}

	int i;
	int posx, posy;
	int state = 0;
	static Ecore_X_Atom effect_state_atom = 0;

	switch( keyrouter.popup_angle )
	{
		case 0:		keyrouter.toggle = 3;	break;
		case 90:	keyrouter.toggle = 2;	break;
		case 180:	keyrouter.toggle = 0;	break;
		case 270:	keyrouter.toggle = 1;	break;
	}

	//creating and showing popup
	if( keyrouter.popup_angle == 0 || keyrouter.popup_angle == 180 )
		keyrouter.popup = e_popup_new(keyrouter.zone, 0, 0, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT);
	else
		keyrouter.popup = e_popup_new(keyrouter.zone, 0, 0, POPUP_MENU_HEIGHT, POPUP_MENU_WIDTH);

	if( !keyrouter.popup )
	{
		fprintf(stderr, "[keyrouter][%s] Failed on e_popup_new() !\n", __FUNCTION__);
		return;
	}

	keyrouter.popup_bg = evas_object_rectangle_add(keyrouter.popup->evas);

	if( !keyrouter.popup_bg )
	{
		fprintf(stderr, "[keyrouter][%s] Fail to call evas_object_rectangle_add() !\n", __FUNCTION__);
		popup_destroy();
		return;
	}

	evas_object_resize(keyrouter.popup_bg, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT);
	evas_object_color_set(keyrouter.popup_bg, 0, 0, 0, 255);
	evas_object_show(keyrouter.popup_bg);

	e_popup_name_set(keyrouter.popup, "R-Click Popup");
	e_popup_layer_set(keyrouter.popup, 400);

	posx = keyrouter.popup_rootx;
	posy = keyrouter.popup_rooty;

	if( keyrouter.popup_angle == 0 || keyrouter.popup_angle == 180 )
	{
		if( (keyrouter.popup_rootx+POPUP_MENU_WIDTH) >= keyrouter.zone->w )
		{
			posx -= POPUP_MENU_WIDTH;
			keyrouter.rbutton_pressed_on_popup = 1;
		}
		if( (keyrouter.popup_rooty+POPUP_MENU_HEIGHT) >= keyrouter.zone->h )
		{
			posy -= POPUP_MENU_HEIGHT;
			keyrouter.rbutton_pressed_on_popup = 1;
		}
		e_popup_move(keyrouter.popup, posx, posy);
	}
	else
	{
		if( (keyrouter.popup_rootx+POPUP_MENU_HEIGHT) >= keyrouter.zone->w )
		{
			posx -= POPUP_MENU_HEIGHT;
			keyrouter.rbutton_pressed_on_popup = 1;
		}
		if( (keyrouter.popup_rooty+POPUP_MENU_WIDTH) >= keyrouter.zone->h )
		{
			posy -= POPUP_MENU_WIDTH;
			keyrouter.rbutton_pressed_on_popup = 1;
		}
		e_popup_move(keyrouter.popup, posx, posy);
	}

	e_popup_show(keyrouter.popup);

	XISelectEvents(keyrouter.disp, keyrouter.rootWin, &keyrouter.eventmask_all, 1);

	if( !ecore_x_pointer_grab(keyrouter.popup->evas_win) )
		fprintf(stderr, "[keyrouter][%s] Failed to grab pointer !\n", __FUNCTION__);

	//effect disable for popup
	effect_state_atom = ecore_x_atom_get ("_NET_CM_WINDOW_EFFECT_ENABLE");

	if( !effect_state_atom)
		fprintf(stderr, "[keyrouter][%s] Cannot find _NET_CM_WINDOW_EFFECT_ENABLE atom...\n", __FUNCTION__);

	ecore_x_window_prop_property_set(keyrouter.popup->evas_win, effect_state_atom, ECORE_X_ATOM_CARDINAL, 32, &state, 1);

	for( i = 0 ; i < 4 ; i++ )
	{
		keyrouter.popup_btns[i] = NULL;
		//creating event handlers of contents of popup

		keyrouter.popup_btns[i] = e_widget_button_add(evas_object_evas_get(keyrouter.popup_bg), btns_label[i], NULL, NULL, NULL, NULL);
		evas_object_event_callback_add(keyrouter.popup_btns[i], EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)_e_keyrouter_popup_btn_down_cb, btns_label[i]);
		evas_object_event_callback_add(keyrouter.popup_btns[i], EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)_e_keyrouter_popup_btn_up_cb, btns_label[i]);

		evas_object_move(keyrouter.popup_btns[i], 0, i*30);
		evas_object_resize(keyrouter.popup_btns[i], POPUP_MENU_WIDTH, 30);
		evas_object_show(keyrouter.popup_btns[i]);
	}

	if( keyrouter.popup_angle != 0 )
		ecore_evas_rotation_with_resize_set(keyrouter.popup->ecore_evas, keyrouter.popup_angle);

	return;
}
#endif//_F_ENABLE_MOUSE_POPUP

static void Device_Status(unsigned int val)
{
	if( 1 == val )
	{
		keyrouter.fplog = stderr;
	}
	else if( 2 == val )
	{
		keyrouter.fplog = fopen(KEYROUTER_LOG_FILE, "w+");

		if( keyrouter.fplog == 0 )
		{
			fprintf(stderr, "[keyrouter][Device_Status] Failed to open file (%s) !\n", KEYROUTER_LOG_FILE);
			keyrouter.fplog = (struct FILE *)stderr;
		}
	}

	fprintf(keyrouter.fplog, "\n[keyrouter] - Device Status = Start =====================\n");

	if( keyrouter.device_list )
	{
		Eina_List* l;
		E_Keyrouter_Device_Info *data;

		EINA_LIST_FOREACH(keyrouter.device_list, l, data)
		{
			if( data )
			{
				fprintf(keyrouter.fplog, "Device id : %d Name : %s\n", data->id, data->name);
				switch( data->type )
				{
					case E_KEYROUTER_HWKEY:
						fprintf(keyrouter.fplog, "¦¦Device type : H/W Key Device\n");
						break;

					case E_KEYROUTER_HOTPLUGGED:
						fprintf(keyrouter.fplog, "¦¦Device type : Hotplugged Key Device\n");
						break;

					case E_KEYROUTER_KEYBOARD:
						fprintf(keyrouter.fplog, "¦¦Device type : Hotplugged Keyboard Device\n");
						break;

					default:
						fprintf(keyrouter.fplog, "¦¦Device type : Unknown\n");
				}
			}
		}
	}
	else
	{
		fprintf(keyrouter.fplog, "No input devices...\n");
	}

	fprintf(keyrouter.fplog, "\n[keyrouter] - Device Status = End =====================\n");

	if( keyrouter.fplog != stderr )
	{
		fflush(keyrouter.fplog);
		fclose(keyrouter.fplog);
		keyrouter.fplog = (struct FILE *)stderr;
	}
}

static void Keygrab_Status(unsigned int val)
{
	if( 1 == val )
	{
		keyrouter.fplog = (struct FILE *)stderr;
	}
	else if( 2 == val )
	{
		keyrouter.fplog = fopen(KEYROUTER_LOG_FILE, "w+");

		if( !keyrouter.fplog )
		{
			fprintf(stderr, "[keyrouter][Keygrab_Status] Failed to open file (%s) !\n", KEYROUTER_LOG_FILE);
			keyrouter.fplog = (struct FILE *)stderr;
		}
	}

	fprintf(keyrouter.fplog, "\n[keyrouter] - Grab Status = Start =====================\n");
	PrintKeyDeliveryList();
	fprintf(keyrouter.fplog, "\n[keyrouter] - Grab Status = End =====================\n");

	if( keyrouter.fplog != stderr )
	{
		fflush(keyrouter.fplog);
		fclose(keyrouter.fplog);
		keyrouter.fplog = (struct FILE *)stderr;
	}
}

static void InitModKeys()
{
	int i = 0;

	if(!keyrouter.modkey)
		return;

	keyrouter.modkey[i].keys[0].keysym = XStringToKeysym(KEY_POWER);
	keyrouter.modkey[i].keys[0].keycode = XKeysymToKeycode(keyrouter.disp, keyrouter.modkey[i].keys[0].keysym);
	keyrouter.modkey[i].keys[1].keysym = XStringToKeysym(KEY_VOLUMEDOWN);
	keyrouter.modkey[i].keys[1].keycode = XKeysymToKeycode(keyrouter.disp, keyrouter.modkey[i].keys[1].keysym);
	keyrouter.modkey[i].press_only = EINA_TRUE;

	keyrouter.cancel_key.keysym = XStringToKeysym(KEY_CANCEL);
	keyrouter.cancel_key.keycode = XKeysymToKeycode(keyrouter.disp, keyrouter.cancel_key.keysym);

	fprintf(stderr, "[keyrouter][%s][%d] Modifier Key=%s (keycode:%d)\n", __FUNCTION__, i, KEY_POWER, keyrouter.modkey[i].keys[0].keycode);
	fprintf(stderr, "[keyrouter][%s][%d] Composited Key=%s (keycode:%d)\n", __FUNCTION__, i, KEY_VOLUMEDOWN, keyrouter.modkey[i].keys[1].keycode);
	fprintf(stderr, "[keyrouter][%s][%d] Cancel Key=%s (keycode:%d)\n", __FUNCTION__, i, KEY_CANCEL, keyrouter.cancel_key.keycode);
#ifdef __DEBUG__
	fprintf(stderr, "[keyrouter][%s][%d] modkey.composited=%d, modkey.set=%d, modkey.time=%d\n", __FUNCTION__, i, keyrouter.modkey[i].composited, keyrouter.modkey[i].set, (int)keyrouter.modkey[i].time);
	fprintf(stderr, "[keyrouter][%s][%d] modkey.idx_mod=%d, modkey.idx_comp=%d, press_only=%d\n", __FUNCTION__, i, keyrouter.modkey[i].idx_mod, keyrouter.modkey[i].idx_comp, keyrouter.modkey[i].press_only);
#endif//__DEBUG__
	i++;
	keyrouter.modkey[i].keys[0].keysym = XStringToKeysym(KEY_VOLUMEUP);
	keyrouter.modkey[i].keys[0].keycode = XKeysymToKeycode(keyrouter.disp, keyrouter.modkey[i].keys[0].keysym);
	keyrouter.modkey[i].keys[1].keysym = XStringToKeysym(KEY_VOLUMEDOWN);
	keyrouter.modkey[i].keys[1].keycode = XKeysymToKeycode(keyrouter.disp, keyrouter.modkey[i].keys[1].keysym);
	keyrouter.modkey[i].press_only = EINA_FALSE;

	fprintf(stderr, "[keyrouter][%s][%d] Modifier Key=%s (keycode:%d)\n", __FUNCTION__, i, KEY_VOLUMEUP, keyrouter.modkey[i].keys[0].keycode);
	fprintf(stderr, "[keyrouter][%s][%d] Composited Key=%s (keycode:%d)\n", __FUNCTION__, i, KEY_VOLUMEDOWN, keyrouter.modkey[i].keys[1].keycode);
	fprintf(stderr, "[keyrouter][%s][%d] Cancel Key=%s (keycode:%d)\n", __FUNCTION__, i, KEY_CANCEL, keyrouter.cancel_key.keycode);
#ifdef __DEBUG__
	fprintf(stderr, "[keyrouter][%s][%d] modkey.composited=%d, modkey.set=%d, modkey.time=%d\n", __FUNCTION__, i, keyrouter.modkey[i].composited, keyrouter.modkey[i].set, (int)keyrouter.modkey[i].time);
	fprintf(stderr, "[keyrouter][%s][%d] modkey.idx_mod=%d, modkey.idx_comp=%d, press_only=%d\n", __FUNCTION__, i, keyrouter.modkey[i].idx_mod, keyrouter.modkey[i].idx_comp, keyrouter.modkey[i].press_only);
#endif//__DEBUG__

}

static void ResetModKeyInfo()
{
	int i;

	for(i=0 ; i < NUM_KEY_COMPOSITION_ACTIONS ; i++)
	{
#ifdef __DEBUG__
		fprintf(stderr, "[keyrouter][%s][%d] Current Mod Key Info\n", __FUNCTION__, i);
		fprintf(stderr, "[keyrouter][%s][%d] modkey.set=%d, modkey.composited=%d, modkey.time=%d, modkey.idx_mod=%d, modkey.idx_comp=%d\n",
			__FUNCTION__, i, keyrouter.modkey[i].set, keyrouter.modkey[i].composited, (int)keyrouter.modkey[i].time, keyrouter.modkey[i].idx_mod, keyrouter.modkey[i].idx_comp);
#endif
		keyrouter.modkey[i].set = keyrouter.modkey[i].composited = keyrouter.modkey[i].time = keyrouter.modkey[i].idx_mod = keyrouter.modkey[i].idx_comp = 0;
#ifdef __DEBUG__
		fprintf(stderr, "[keyrouter][%s][%d] Reset Mod Key Info\n", __FUNCTION__, i);
#endif
	}

	keyrouter.modkey_set = 0;
	fprintf(stderr, "[keyrouter][%s][%d] modkey_set=%d\n", __FUNCTION__, i, keyrouter.modkey_set);
}

static int IsModKey(XEvent *ev, int index)
{
	int i, j = index;

	for( i = 0 ; i < NUM_COMPOSITION_KEY ; i++ )
		if( ev->xkey.keycode == keyrouter.modkey[j].keys[i].keycode )
			return (i+1);

	return 0;
}

static int IsCompKey(XEvent *ev, int index)
{
	int i = index;

	if( ev->xkey.keycode == keyrouter.modkey[i].keys[keyrouter.modkey[i].idx_mod%NUM_COMPOSITION_KEY].keycode )
		return 4;

	return 0;
}

static int IsKeyComposited(XEvent *ev, int index)
{
	int i = index;

	if( (ev->xkey.keycode == keyrouter.modkey[i].keys[keyrouter.modkey[i].idx_mod%NUM_COMPOSITION_KEY].keycode)
		&& (ev->xkey.time <= (keyrouter.modkey[i].time + KEY_COMPOSITION_TIME)) )
		return 3;

	return 0;
}

static void DoKeyCompositionAction(int index, int press)
{
	XEvent xev;
	Atom xkey_composition_atom = None;

	int revert_to;
	int i = index;

	xkey_composition_atom = XInternAtom (keyrouter.disp, STR_ATOM_XKEY_COMPOSITION, False);

	xev.xclient.window = keyrouter.rootWin;
	xev.xclient.type = ClientMessage;
	xev.xclient.message_type = xkey_composition_atom;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = keyrouter.modkey[i].keys[0].keycode;
	xev.xclient.data.l[1] = keyrouter.modkey[i].keys[1].keycode;
	xev.xclient.data.l[2] = press;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;

	XSendEvent(keyrouter.disp, keyrouter.rootWin, False, StructureNotifyMask | SubstructureNotifyMask, &xev);
	XSync(keyrouter.disp, False);

	fprintf(stderr, "\n[keyrouter][%s][%d] Do Key Composition Action : ClientMessage to RootWindow(0x%x)!: %s\n", __FUNCTION__, i, keyrouter.rootWin, press ? "Press" : "Release");
}

// End of a file

