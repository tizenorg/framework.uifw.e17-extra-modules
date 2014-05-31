#include <Ecore.h>
#include <Ecore_Input.h>
#include "Ecore_X.h"
#include "e_mod_devmode_event.h"
#include "e_mod_devmode.h"
#include "e_mod_devmode_main.h"
#include <string.h>

typedef struct
{
   void                *user_data;

   Ecore_Event_Handler *handler[TSP_EVENT_NUM];
   Eina_List           *pressed_mouse_list;

   int                  xi_opcode;
   int                  xi_event;
   int                  xi_error;
   int                 *slots;
   int                  n_slots;
   unsigned int         device_num;
   unsigned int        *button_pressed;
   unsigned int        *button_moved;
   tsp_mouse_info_s    *last_raw;
} tsp_event_mgr_s;

static tsp_event_mgr_s g_event_mgr;

#if 0
static int xi_opcode = -1;
static int xi_event = -1;
static int xi_error = -1;
static int *slots = NULL;
static int n_slots = 0;
static unsigned int g_device_num = 0;
static unsigned int *button_pressed = NULL;
static unsigned int *button_moved = NULL;
static tsp_mouse_info_s *last_raw = NULL;
#endif

static int
get_finger_index(int deviceid)
{
   int i;
   for (i = 0; i < g_event_mgr.n_slots; i++)
     {
        if (g_event_mgr.slots[i] == deviceid)
          return i;
     }

   return -1;
}

static const char *
type_to_name(int evtype)
{
   const char *name;

   switch (evtype)
     {
      case XI_RawKeyPress:
        name = "RawKeyPress";
        break;

      case XI_RawKeyRelease:
        name = "RawKeyRelease";
        break;

      case XI_RawButtonPress:
        name = "RawButtonPress";
        break;

      case XI_RawButtonRelease:
        name = "RawButtonRelease";
        break;

      case XI_RawMotion:
        name = "RawMotion";
        break;

      case XI_RawTouchBegin:
        name = "RawTouchBegin";
        break;

      case XI_RawTouchUpdate:
        name = "RawTouchUpdate";
        break;

      case XI_RawTouchEnd:
        name = "RawTouchEnd";
        break;

      default:
        name = "unknown event type";
        break;
     }

   return name;
}

static void
get_rawdata_from_event(XIRawEvent *event, int *x, int *y, int *deviceid, int *pressure)
{
   int i;
   double *val, *raw_val;

   if (!event)
     {
        TSP_DEBUG("[devmode-tizen] event is NIL");
        return;
     }

   val = event->valuators.values;
   raw_val = event->raw_values;

   if (!val || !raw_val)
     {
        TSP_DEBUG("[devmode-tizen] [%s] : %d, val : %p, raw_val : %p", __FUNCTION__, __LINE__, val, raw_val);
        return;
     }

   for (i = 0; (i < event->valuators.mask_len * 8) && (i < 2); i++)
     {
        if (event->valuators.mask)
          {
             if (XIMaskIsSet(event->valuators.mask, i))
               {
                  *deviceid = get_finger_index(event->deviceid);
                  if (i == 0)
                    {
                       *x = (int)(*(val + i));
                    }
                  else if (i == 1)
                    {
                       *y = (int)(*(val + i));
                    }
               }
             else
               *deviceid = get_finger_index(event->deviceid);
          }
     }
}

static Eina_Bool
tsp_event_mgr_event_mouse_generic_handler(void *data, int type, void *event_info)
{
   Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event_info;
   int x = -1, y = -1, deviceid = -1, pressure = 1;
   tsp_mouse_info_s info;
   XIRawEvent *raw = NULL;
   XGenericEventCookie *cookie = NULL;

   TSP_FUNC_ENTER();

   if ((!e) || (e->extension != g_event_mgr.xi_opcode))
     return ECORE_CALLBACK_PASS_ON;

   TSP_DEBUG("[devmode-tizen] x generic event's evtype : %d, XI_RawButtonPress : %d, XI_RawButtonRelease : %d, XI_RawMotion : %d", e->evtype, XI_RawButtonPress, XI_RawButtonRelease, XI_RawMotion);

   if (e->evtype != XI_RawButtonPress && e->evtype != XI_RawButtonRelease && e->evtype != XI_RawMotion)
     return ECORE_CALLBACK_PASS_ON;

   raw = (XIRawEvent *)e->data;
   cookie = (XGenericEventCookie *)e->cookie;

   if (!raw || !cookie)
     return ECORE_CALLBACK_PASS_ON;

   if ((raw->deviceid == VIRTUAL_CORE_POINTER) || (raw->deviceid == VCP_XTEST_POINTER))
     return ECORE_CALLBACK_PASS_ON;

   get_rawdata_from_event(raw, &x, &y, &deviceid, &pressure);
   if (deviceid == -1)
     return ECORE_CALLBACK_PASS_ON;

   TSP_DEBUG("[devmode-tizen] x : %d, y : %d, deviceid : %d, preesure : %d", x, y, deviceid, pressure);

   if (e->evtype == XI_RawMotion)
     {
        memset(&info, 0, sizeof(tsp_mouse_info_s));
        info.device = deviceid;
        info.timestamp = ecore_loop_time_get() * 1000.0;
        info.x = x;
        info.y = y;
        info.pressure = pressure;
        memcpy(&(g_event_mgr.last_raw[info.device]), &info, sizeof(tsp_mouse_info_s));
     }

   switch (e->evtype)
     {
      case XI_RawButtonPress: /*XI_RawButtonPress*/
        if ((g_event_mgr.button_pressed[deviceid] == 0) && &(g_event_mgr.last_raw[deviceid]))
          {
             tsp_main_view_mouse_down(g_event_mgr.user_data, &(g_event_mgr.last_raw[deviceid]));
             g_event_mgr.button_pressed[deviceid] = 1;
          }
        break;

      case XI_RawButtonRelease: /*XI_RawButtonRelease*/
        if ((g_event_mgr.button_pressed[deviceid] == 1) && &(g_event_mgr.last_raw[deviceid]))
          {
             tsp_main_view_mouse_up(g_event_mgr.user_data, &(g_event_mgr.last_raw[deviceid]));
             g_event_mgr.button_pressed[deviceid] = 0;
          }
        break;

      case XI_RawMotion: /*XI_RawMotion*/
        if (g_event_mgr.button_pressed[deviceid] == 1)
          {
             tsp_main_view_mouse_move(g_event_mgr.user_data, &info);
          }

        break;

      case XI_RawTouchBegin:
        break;

      case XI_RawTouchUpdate:
        break;

      case XI_RawTouchEnd:
        break;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
tsp_event_mgr_devmode_init(void *data)
{
   XIEventMask *mask = NULL;
   int num_mask = 0;
   int rc;
   Ecore_X_Display *d = NULL;

   TSP_FUNC_ENTER();

   d = ecore_x_display_get();
   if (!d)
     {
        TSP_ERROR("[devmode-tizen] Failed to open display !");
        return EINA_FALSE;
     }

   if (!XQueryExtension(d, "XInputExtension", &g_event_mgr.xi_opcode, &g_event_mgr.xi_event, &g_event_mgr.xi_error))
     {
        TSP_ERROR("[devmode-tizen] No XInput extension is available.");
        return EINA_FALSE;
     }

   Ecore_X_Window xroot = ecore_x_window_root_first_get();

   mask = XIGetSelectedEvents(d, xroot, &num_mask);

   if (mask)
     {
        XISetMask(mask->mask, XI_RawButtonPress);
        XISetMask(mask->mask, XI_RawButtonRelease);
        XISetMask(mask->mask, XI_RawMotion);
#if 0
        XISetMask(mask->mask, XI_RawTouchBegin);
        XISetMask(mask->mask, XI_RawTouchUpdate);
        XISetMask(mask->mask, XI_RawTouchEnd);
#endif

        rc = XISelectEvents(d, xroot, mask, 1);
        if (Success != rc)
          {
             TSP_ERROR("[devmode-tizen] Failed to select XInput extension events !");
             return EINA_FALSE;
          }
     }
   else
     {
        mask = calloc(1, sizeof(XIEventMask));
        if (!mask)
          {
             TSP_ERROR("[devmode-tizen] [%s]:[%d] Failed to allocate memory !", __FUNCTION__, __LINE__);
             return EINA_FALSE;
          }

        mask->deviceid = XIAllDevices;
        mask->mask_len = XIMaskLen(XI_LASTEVENT);
        mask->mask = calloc(mask->mask_len, sizeof(char));
        if (!mask->mask)
          {
             TSP_ERROR("[devmode-tizen] [%s][%d] Failed to allocate memory !", __FUNCTION__, __LINE__);
             free(mask);
             return EINA_FALSE;
          }

        XISetMask(mask->mask, XI_RawButtonPress);
        XISetMask(mask->mask, XI_RawButtonRelease);
        XISetMask(mask->mask, XI_RawMotion);
        XISetMask(mask->mask, XI_RawTouchBegin);
        XISetMask(mask->mask, XI_RawTouchUpdate);
        XISetMask(mask->mask, XI_RawTouchEnd);

        rc = XISelectEvents(d, xroot, mask, 1);
        if (Success != rc)
          {
             if (mask->mask) free(mask->mask);
             if (mask) free(mask);
             TSP_ERROR("[devmode-tizen] Failed to select XInput extension events !");
             return EINA_FALSE;
          }

        if (mask->mask) free(mask->mask);
        if (mask) free(mask);
     }

   return EINA_TRUE;
}

static int
tsp_event_mgr_devmode_get_multi_touch_info()
{
   Ecore_X_Display *d = NULL;
   int i, idx = 0;
   int ndevices = 0;
   XIDeviceInfo *dev, *info = NULL;

   TSP_FUNC_ENTER();

   d = ecore_x_display_get();
   if (!d) return -1;

   if (g_event_mgr.xi_opcode < 0 ) return 0;

   info = XIQueryDevice(d, XIAllDevices, &ndevices);

   if (!info)
     {
        TSP_ERROR("[devmode-tizen] There is no queried XI device.");
        return 0;
     }

   for ( i = 0; i < ndevices; i++ )
     {
        dev = &info[i];

        if ((XISlavePointer == dev->use) || (XIFloatingSlave == dev->use))
          {
             //skip XTEST Pointer and non-touch device(s)
             if ( strcasestr(dev->name, "XTEST") || !strcasestr(dev->name, "touch"))
               continue;
             idx++;
          }
     }

   g_event_mgr.slots = malloc(idx * sizeof(int));
   if (!g_event_mgr.slots)
     {
        TSP_ERROR("[devmode-tizen] The slot of event mgr is 0.");
        if (info) free(info);
        return 0;
     }

   idx = 0;
   for ( i = 0; i < ndevices; i++ )
     {
        dev = &info[i];

        if ((XISlavePointer == dev->use) || (XIFloatingSlave == dev->use))
          {
             //skip XTEST Pointer and non-touch device(s)
             if ( strcasestr(dev->name, "XTEST") || !strcasestr(dev->name, "touch"))
               continue;

             g_event_mgr.slots[idx] = dev->deviceid;
             idx++;
          }
     }

   XIFreeDeviceInfo(info);

   g_event_mgr.n_slots = idx;
   return idx;
}

void
tsp_event_mgr_init(void *user_data)
{
   TSP_FUNC_ENTER();

   memset(&g_event_mgr, 0x0, sizeof(tsp_event_mgr_s));

   g_event_mgr.user_data = user_data;

   if (tsp_event_mgr_devmode_init(user_data) != EINA_TRUE)
     TSP_ERROR("[devmode-tizen] [%s] : %d, failed to event manager init", __FUNCTION__, __FUNCTION__);

   g_event_mgr.device_num = tsp_event_mgr_devmode_get_multi_touch_info();
   g_event_mgr.button_pressed = (unsigned int *)calloc(1, sizeof(unsigned int) * g_event_mgr.device_num);
   g_event_mgr.button_moved = (unsigned int *)calloc(1, sizeof(unsigned int) * g_event_mgr.device_num);
   g_event_mgr.last_raw = (tsp_mouse_info_s *)calloc(1, sizeof(tsp_mouse_info_s) * g_event_mgr.device_num);

   g_event_mgr.handler[TSP_EVENT_MOUSE_GENERIC] = ecore_event_handler_add(ECORE_X_EVENT_GENERIC, tsp_event_mgr_event_mouse_generic_handler, NULL);
}

void
tsp_event_mgr_deinit(void)
{
   TSP_FUNC_ENTER();

   int i = 0;
   for (; i < TSP_EVENT_NUM; i++) {
        if (g_event_mgr.handler[i])
          {
             ecore_event_handler_del(g_event_mgr.handler[i]);
             g_event_mgr.handler[i] = NULL;
          }
     }
   free(g_event_mgr.button_pressed);
   free(g_event_mgr.button_moved);
   free(g_event_mgr.last_raw);

   memset(&g_event_mgr, 0x0, sizeof(tsp_event_mgr_s));
}

