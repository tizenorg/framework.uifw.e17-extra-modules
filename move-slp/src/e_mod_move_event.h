#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_EVENT_H
#define E_MOD_MOVE_EVENT_H

typedef struct _E_Move_Event             E_Move_Event;
typedef struct _E_Move_Event_Motion_Info E_Move_Event_Motion_Info;

typedef enum   _E_Move_Event_Type        E_Move_Event_Type;
typedef enum   _E_Move_Event_State       E_Move_Event_State;

typedef Eina_Bool          (*E_Move_Event_Cb)       (void *, void *);
typedef Eina_Bool          (*E_Move_Event_Angle_Cb) (Ecore_X_Window, int *, int *);
typedef E_Move_Event_State (*E_Move_Event_Check_Cb) (E_Move_Event *, void *);

enum _E_Move_Event_Type
{
   E_MOVE_EVENT_TYPE_FIRST = 0,
   E_MOVE_EVENT_TYPE_MOTION_START,
   E_MOVE_EVENT_TYPE_MOTION_MOVE,
   E_MOVE_EVENT_TYPE_MOTION_END,
   E_MOVE_EVENT_TYPE_LAST
};

enum _E_Move_Event_State
{
   E_MOVE_EVENT_STATE_UNKOWN = 0,
   E_MOVE_EVENT_STATE_CHECK,
   E_MOVE_EVENT_STATE_PASS,
   E_MOVE_EVENT_STATE_HOLD
};

struct _E_Move_Event_Motion_Info
{
   Evas_Callback_Type  cb_type;
   E_Move_Event_Type   ev_type;
   void               *event_info;
   Evas_Point          coord;
};

/* event management functions */
EINTERN E_Move_Event       *e_mod_move_event_new(Ecore_X_Window win, Evas_Object *obj);
EINTERN void                e_mod_move_event_free(E_Move_Event *ev);
EINTERN Eina_Bool           e_mod_move_event_cb_set(E_Move_Event *ev, E_Move_Event_Type type, E_Move_Event_Cb cb, void *data);
EINTERN Eina_Bool           e_mod_move_event_angle_cb_set(E_Move_Event *ev, E_Move_Event_Angle_Cb cb);
EINTERN Eina_Bool           e_mod_move_event_check_cb_set(E_Move_Event *ev, E_Move_Event_Check_Cb cb, void *data);
EINTERN Eina_List          *e_mod_move_event_ev_queue_get(E_Move_Event *ev);
EINTERN E_Move_Event_State  e_mod_move_event_state_get(E_Move_Event *ev);
EINTERN Eina_Bool           e_mod_move_event_angle_set(E_Move_Event *ev, int angle);
EINTERN int                 e_mod_move_event_angle_get(E_Move_Event *ev);
EINTERN Eina_Bool           e_mod_move_event_data_clear(E_Move_Event *ev);

#endif
#endif
