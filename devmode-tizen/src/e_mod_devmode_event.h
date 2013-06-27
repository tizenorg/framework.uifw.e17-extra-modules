#ifndef __TSP_EVENT_H__
#define __TSP_EVENT_H__

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
//#include <X11/Xutil.h>

typedef enum
{
   TSP_EVENT_MOUSE_DOWN,
   TSP_EVENT_MOUSE_MOVE,
   TSP_EVENT_MOUSE_UP,
   TSP_EVENT_MOUSE_ANY,
   TSP_EVENT_MOUSE_GENERIC,
   TSP_EVENT_NUM,
} tsp_event_type_e;

#define VIRTUAL_CORE_POINTER  2
#define VIRTUAL_CORE_KEYBOARD 3
#define VCP_XTEST_POINTER     4
#define VCP_XTEST_KEYBOARD    5

void tsp_event_mgr_init(void *user_data);
void tsp_event_mgr_deinit(void);

#endif /* __TSP_EVENT_H__ */