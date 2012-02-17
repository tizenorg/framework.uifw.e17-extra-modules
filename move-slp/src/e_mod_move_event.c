#include "e.h"
#include "e_mod_move_event.h"

struct _E_Move_Event
{
   E_Move_Event_State       state;
   Evas_Object             *obj;
   Eina_List               *queue;

   struct {
     Ecore_X_Window         id;
     int                    angle;
     E_Move_Event_Angle_Cb  fn_angle_get;
   } win;

   struct {
     E_Move_Event_Cb        cb;
     void                  *data;
   } fn[E_MOVE_EVENT_TYPE_LAST];

   struct {
     E_Move_Event_Check_Cb  cb;
     void                  *data;
   } ev_check;
};

/* local subsystem functions */
static E_Move_Event_Motion_Info *_motion_info_new(void *event_info, Evas_Callback_Type type);
static void                      _motion_info_free(E_Move_Event_Motion_Info *motion);
static void                      _ev_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                      _ev_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                      _ev_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static Eina_Bool                 _ev_cb_call(E_Move_Event *ev, E_Move_Event_Motion_Info *motion);
static void                      _event_pass(Ecore_X_Window id, Evas_Object *obj, void *event_info, Evas_Callback_Type type);

/* externally accessible functions */
EINTERN E_Move_Event *
e_mod_move_event_new(Ecore_X_Window  win,
                     Evas_Object    *obj)
{
   E_Move_Event *ev;
   if (!win || !obj) return NULL;

   ev = E_NEW(E_Move_Event, 1);
   if (!ev) return NULL;

   ev->obj = obj;
   ev->win.id = win;

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_DOWN, _ev_cb_mouse_down, ev);
   if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE)
     {
        fprintf(stderr,
                "[E17-MOVE-ERR] %20.20s(%04d) Callback registering failed! "
                "w:0x%07x obj:%p EVAS_CALLBACK_MOUSE_DOWN\n",
                __func__, __LINE__, win, obj);
        E_FREE(ev);
        return NULL;
     }

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_MOVE, _ev_cb_mouse_move, ev);
   if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE)
     {
        evas_object_event_callback_del
          (obj, EVAS_CALLBACK_MOUSE_DOWN, _ev_cb_mouse_down);
        fprintf(stderr,
                "[E17-MOVE-ERR] %20.20s(%04d) Callback registering failed! "
                "w:0x%07x obj:%p EVAS_CALLBACK_MOUSE_MOVE\n",
                __func__, __LINE__, win, obj);
        E_FREE(ev);
        return NULL;
     }

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_UP, _ev_cb_mouse_up, ev);
   if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE)
     {
        evas_object_event_callback_del
          (obj, EVAS_CALLBACK_MOUSE_DOWN, _ev_cb_mouse_down);
        evas_object_event_callback_del
          (obj, EVAS_CALLBACK_MOUSE_MOVE, _ev_cb_mouse_move);
        fprintf(stderr,
                "[E17-MOVE-ERR] %20.20s(%04d) Callback registering failed! "
                "w:0x%07x obj:%p EVAS_CALLBACK_MOUSE_UP\n",
                __func__, __LINE__, win, obj);
        E_FREE(ev);
        return NULL;
     }

   return ev;
}

EINTERN void
e_mod_move_event_free(E_Move_Event *ev)
{
   int i;
   if (!ev) return;

   e_mod_move_event_data_clear(ev);

   if (ev->obj)
     {
        evas_object_event_callback_del
          (ev->obj, EVAS_CALLBACK_MOUSE_DOWN,
          _ev_cb_mouse_down);

        evas_object_event_callback_del
          (ev->obj, EVAS_CALLBACK_MOUSE_MOVE,
          _ev_cb_mouse_move);

        evas_object_event_callback_del
          (ev->obj, EVAS_CALLBACK_MOUSE_UP,
          _ev_cb_mouse_up);
     }

   for (i = 0; i < E_MOVE_EVENT_TYPE_LAST; i++)
     {
        ev->fn[i].cb = NULL;
        ev->fn[i].data = NULL;
     }

   ev->state = E_MOVE_EVENT_STATE_UNKOWN;
   ev->win.id = 0;
   ev->win.angle = 0;
   ev->win.fn_angle_get = NULL;
   ev->ev_check.cb = NULL;
   ev->ev_check.data = NULL;
   ev->obj = NULL;

   E_FREE(ev);
}

EINTERN Eina_Bool
e_mod_move_event_cb_set(E_Move_Event      *ev,
                        E_Move_Event_Type  type,
                        E_Move_Event_Cb    cb,
                        void              *data)
{
   if (!ev) return EINA_FALSE;
   ev->fn[type].cb = cb;
   ev->fn[type].data = data;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_event_angle_cb_set(E_Move_Event          *ev,
                              E_Move_Event_Angle_Cb  cb)
{
   if (!ev) return EINA_FALSE;
   ev->win.fn_angle_get = cb;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_event_check_cb_set(E_Move_Event          *ev,
                              E_Move_Event_Check_Cb  cb,
                              void                  *data)
{
   if (!ev) return EINA_FALSE;
   ev->ev_check.cb = cb;
   ev->ev_check.data = data;
   return EINA_TRUE;
}

EINTERN Eina_List *
e_mod_move_event_ev_queue_get(E_Move_Event *ev)
{
   if (!ev) return NULL;
   return ev->queue;
}

EINTERN E_Move_Event_State
e_mod_move_event_state_get(E_Move_Event *ev)
{
   if (!ev) return E_MOVE_EVENT_STATE_PASS;
   return ev->state;
}

EINTERN Eina_Bool
e_mod_move_event_angle_set(E_Move_Event *ev,
                           int           angle)
{
   if (!ev) return EINA_FALSE;
   ev->win.angle = ((angle % 360) / 90) * 90;
   return EINA_TRUE;
}

EINTERN int
e_mod_move_event_angle_get(E_Move_Event *ev)
{
   if (!ev) return 0;
   return ev->win.angle;
}

EINTERN Eina_Bool
e_mod_move_event_data_clear(E_Move_Event *ev)
{
   if (!ev) return EINA_FALSE;
   if (!ev->queue) return EINA_TRUE;

   E_Move_Event_Motion_Info *info;
   EINA_LIST_FREE(ev->queue, info)
     {
        if (!info) continue;
        if (ev->state == E_MOVE_EVENT_STATE_PASS)
          {
             _event_pass(ev->win.id,
                         ev->obj,
                         info->event_info,
                         info->cb_type);
          }
        else if (ev->state == E_MOVE_EVENT_STATE_HOLD)
          {
             _ev_cb_call(ev, info);
          }
        _motion_info_free(info);
     }

   ev->queue = NULL;
   return EINA_TRUE;
}

/* local subsystem functions */
#ifdef _EV_COPY
# undef _EV_COPY
#endif

#define _EV_COPY(d, t, e)      \
   {                           \
      d = (void *)E_NEW(t, 1); \
      if (!d) {                \
        E_FREE(motion);        \
        return NULL;           \
      }                        \
      memcpy(d, e, sizeof(t)); \
   }

static E_Move_Event_Motion_Info *
_motion_info_new(void               *event_info,
                 Evas_Callback_Type  type)
{
   E_Move_Event_Motion_Info *motion = NULL;
   void                     *data    = NULL;
   Evas_Event_Mouse_Down    *down = NULL;
   Evas_Event_Mouse_Up      *up   = NULL;
   Evas_Event_Mouse_Move    *move = NULL;

   motion = E_NEW(E_Move_Event_Motion_Info, 1);
   if (!motion) return NULL;

   motion->event_info = event_info;
   motion->cb_type = type;

   switch (type)
     {
      case EVAS_CALLBACK_MOUSE_DOWN:
        _EV_COPY(data, Evas_Event_Mouse_Down, event_info);
        down = event_info;
        motion->coord.x = down->output.x;
        motion->coord.y = down->output.y;
        motion->ev_type = E_MOVE_EVENT_TYPE_MOTION_START;
        break;

      case EVAS_CALLBACK_MOUSE_UP:
        _EV_COPY(data, Evas_Event_Mouse_Up, event_info);
        up = event_info;
        motion->coord.x = up->output.x;
        motion->coord.y = up->output.y;
        motion->ev_type = E_MOVE_EVENT_TYPE_MOTION_END;
        break;

      case EVAS_CALLBACK_MOUSE_MOVE:
        _EV_COPY(data, Evas_Event_Mouse_Move, event_info);
        move = event_info;
        motion->coord.x = move->cur.output.x;
        motion->coord.y = move->cur.output.y;
        motion->ev_type = E_MOVE_EVENT_TYPE_MOTION_MOVE;
        break;

      default:
        break;
     }

   if (!data)
     {
        E_FREE(motion);
        motion = NULL;
     }

   return motion;
}

static void
_motion_info_free(E_Move_Event_Motion_Info *motion)
{
   if (!motion) return;
   E_FREE(motion);
}

static void
_ev_cb_mouse_down(void             *data,
                  Evas *e           __UNUSED__,
                  Evas_Object *obj  __UNUSED__,
                  void             *event_info)
{
   E_Move_Event *ev;
   E_Move_Event_Motion_Info *motion;

   ev = (E_Move_Event *)data;
   if (!event_info || !ev) return;

   e_mod_move_event_data_clear(ev);
   ev->state = E_MOVE_EVENT_STATE_CHECK;

   motion = _motion_info_new(event_info,
                             EVAS_CALLBACK_MOUSE_DOWN);
   if (!motion) goto event_pass;

   if (ev->win.fn_angle_get)
     {
        Eina_Bool res;
        int angles[2];
        res = ev->win.fn_angle_get(ev->win.id,
                                   &angles[0],
                                   &angles[1]);
        if (res)
          e_mod_move_event_angle_set(ev, angles[0]);
     }

   if (!ev->ev_check.cb)
     {
        ev->state = E_MOVE_EVENT_STATE_HOLD;
        _ev_cb_call(ev, motion);
        _motion_info_free(motion);
        motion = NULL;
     }

   if (motion)
     ev->queue = eina_list_append(ev->queue, motion);

   return;

event_pass:
   ev->state = E_MOVE_EVENT_STATE_PASS;
   _event_pass(ev->win.id,
               ev->obj,
               event_info,
               EVAS_CALLBACK_MOUSE_DOWN);
}

static void
_ev_cb_mouse_move(void             *data,
                  Evas *e           __UNUSED__,
                  Evas_Object *obj  __UNUSED__,
                  void             *event_info)
{
   E_Move_Event *ev;
   E_Move_Event_Motion_Info *motion = NULL;

   ev = (E_Move_Event *)data;
   if (!event_info || !ev) return;

   switch (ev->state)
     {
      case E_MOVE_EVENT_STATE_CHECK:
        if (!ev->ev_check.cb) goto event_pass;
        motion = _motion_info_new(event_info,
                                  EVAS_CALLBACK_MOUSE_MOVE);
        if (!motion) goto event_pass;

        ev->queue = eina_list_append(ev->queue, motion);

        ev->state = ev->ev_check.cb(ev, ev->ev_check.data);
        if ((ev->state == E_MOVE_EVENT_STATE_PASS) ||
            (ev->state == E_MOVE_EVENT_STATE_HOLD))
          {
             e_mod_move_event_data_clear(ev);
          }
        break;

      case E_MOVE_EVENT_STATE_HOLD:
        motion = _motion_info_new(event_info,
                                  EVAS_CALLBACK_MOUSE_MOVE);
        if (!motion) return;
        _ev_cb_call(ev, motion);
        _motion_info_free(motion);
        motion = NULL;
        break;

      case E_MOVE_EVENT_STATE_PASS:
      default:
        _event_pass(ev->win.id,
                    ev->obj,
                    event_info,
                    EVAS_CALLBACK_MOUSE_MOVE);
        break;
     }

   return;

event_pass:
   ev->state = E_MOVE_EVENT_STATE_PASS;
   e_mod_move_event_data_clear(ev);
   _event_pass(ev->win.id,
               ev->obj,
               event_info,
               EVAS_CALLBACK_MOUSE_MOVE);
}

static void
_ev_cb_mouse_up(void             *data,
                Evas *e           __UNUSED__,
                Evas_Object *obj  __UNUSED__,
                void             *event_info)
{
   E_Move_Event *ev;
   E_Move_Event_Motion_Info *motion = NULL;

   ev = (E_Move_Event *)data;
   if (!event_info || !ev) return;

   switch (ev->state)
     {
      case E_MOVE_EVENT_STATE_CHECK:
        ev->state = E_MOVE_EVENT_STATE_PASS;
        e_mod_move_event_data_clear(ev);
        _event_pass(ev->win.id,
                    ev->obj,
                    event_info,
                    EVAS_CALLBACK_MOUSE_UP);
        break;

      case E_MOVE_EVENT_STATE_HOLD:
        motion = _motion_info_new(event_info,
                                  EVAS_CALLBACK_MOUSE_UP);
        if (!motion) return;
        _ev_cb_call(ev, motion);
        _motion_info_free(motion);
        motion = NULL;
        break;

      case E_MOVE_EVENT_STATE_PASS:
      default:
        _event_pass(ev->win.id,
                    ev->obj,
                    event_info,
                    EVAS_CALLBACK_MOUSE_UP);
        break;
     }

   ev->state = E_MOVE_EVENT_STATE_UNKOWN;
}

static Eina_Bool
_ev_cb_call(E_Move_Event             *ev,
            E_Move_Event_Motion_Info *motion)
{
   E_Move_Event_Type type = motion->ev_type;

   if (!ev->fn[type].cb)
     return EINA_FALSE;

   return ev->fn[type].cb(ev->fn[type].data,
                          motion);
}

static void
_event_pass(Ecore_X_Window      id,
            Evas_Object        *obj,
            void               *event_info,
            Evas_Callback_Type  type)
{
   Evas_Event_Mouse_Down *down = NULL;
   Evas_Event_Mouse_Up   *up   = NULL;
   Evas_Event_Mouse_Move *move = NULL;
   Evas_Coord x = 0, y =0, w, h;

   if (!id || !event_info) return;

   ecore_x_window_geometry_get(id, &x, &y, &w, &h);

   switch (type)
     {
      case EVAS_CALLBACK_MOUSE_DOWN:
        down = event_info;
        ecore_x_mouse_down_send(id,
                                down->canvas.x -x,
                                down->canvas.y -y,
                                down->button);
        break;

      case EVAS_CALLBACK_MOUSE_UP:
        up = event_info;
        ecore_x_mouse_up_send(id,
                              up->canvas.x -x,
                              up->canvas.y -y,
                              up->button);
        break;

      case EVAS_CALLBACK_MOUSE_MOVE:
        move = event_info;
        ecore_x_mouse_move_send(id,
                                move->cur.output.x -x,
                                move->cur.output.y -y);
        break;

      default:
        break;
     }
}
