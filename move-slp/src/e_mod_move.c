#include "e_mod_move.h"
#include "e_mod_move_atoms.h"
#include "e_mod_move_debug.h"

typedef enum   _E_Move_Stacking        E_Move_Stacking;
enum _E_Move_Stacking
{
   E_MOVE_STACKING_NONE = 0,
   E_MOVE_STACKING_RAISE,
   E_MOVE_STACKING_LOWER,
   E_MOVE_STACKING_RAISE_ABOVE,
   E_MOVE_STACKING_LOWER_BELOW
};

/* static global variables */
static Eina_List *handlers       = NULL;
static Eina_List *moves          = NULL;
static Eina_Hash *border_clients = NULL;
static Eina_Hash *borders        = NULL;

/* static functions */
static E_Move        *_e_mod_move_add(E_Manager *man);
static void           _e_mod_move_del(E_Move *m);
static E_Move_Border *_e_mod_move_border_find(Ecore_X_Window win);
static E_Move_Border *_e_mod_move_border_client_find(Ecore_X_Window win);
static E_Move        *_e_mod_move_find(Ecore_X_Window root);
static Eina_Bool      _e_mod_move_mouse_btn_dn(void *data, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_mouse_btn_up(void *data, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_property(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__);
static Eina_Bool      _e_mod_move_message(void *data __UNUSED__, int   type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_visibility_change(void *data __UNUSED__, int   type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_show(void *data __UNUSED__,int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_hide(void *data __UNUSED__,int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_move(void *data __UNUSED__,int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_resize(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_stack(void *data __UNUSED__, int type __UNUSED__, void *event);

static E_Move_Border *_e_mod_move_bd_add_intern(E_Move *m, E_Border *bd);
static void           _e_mod_move_bd_del_intern(E_Move_Border *mb);
static void           _e_mod_move_bd_show_intern(E_Move_Border *mb);
static void           _e_mod_move_bd_hide_intern(E_Move_Border *mb);
static void           _e_mod_move_bd_move_resize_intern(E_Move_Border *mb, int x, int y, int w, int h);
static void           _e_mod_move_bd_stack_intern(E_Move_Border *mb1, E_Move_Border *mb2, E_Move_Stacking type);
static void           _e_mod_move_object_del(void *data, void *obj);

static void           _e_mod_move_bd_obj_del(E_Move_Border *mb);
static void           _e_mod_move_ctl_obj_add(E_Move_Border *mb);
static void           _e_mod_move_ctl_obj_del(E_Move_Border *mb);
static void           _e_mod_move_ctl_obj_event_setup(E_Move_Border *mb, E_Move_Control_Object *mco);

static void          *_e_mod_move_bd_internal_data_add(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_bd_internal_data_del(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_bd_anim_data_del(E_Move_Border *mb);

static Eina_Bool      _e_mod_move_prop_window_input_region_get(Ecore_X_Window win, int *x, int *y, int *w, int *h);
static Eina_Bool      _e_mod_move_prop_window_contents_region_get(Ecore_X_Window win, int *x, int *y, int *w, int *h);
static Eina_Bool      _e_mod_move_prop_active_window_get(Ecore_X_Window *active_win);
static Eina_Bool      _e_mod_move_prop_indicator_state_get(Ecore_X_Window win, Eina_Bool *state);

static Eina_Bool      _e_mod_move_prop_window_input_region(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_window_contents_region(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_active_window(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_indicator_state(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_rotate_window_angle(Ecore_X_Event_Window_Property *ev);

static Eina_Bool      _e_mod_move_msg_window_show(Ecore_X_Event_Client_Message *ev);
static Eina_Bool      _e_mod_move_msg_qp_state(Ecore_X_Event_Client_Message *ev);

static Eina_Bool      _e_mod_move_outside_zone_border_position_update(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_prop_active_window_internal_quickpanel_move(E_Move_Border *mb, int x, int y);
static Eina_Bool      _e_mod_move_msg_window_show_internal_apptray_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_msg_window_show_internal_quickpanel_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_msg_qp_state_internal_quickpanel_check(E_Move_Border *mb);

//////////////////////////////////////////////////////////////////////////

static E_Move_Border *
_e_mod_move_border_find(Ecore_X_Window win)
{
   return eina_hash_find(borders, e_util_winid_str_get(win));
}

static E_Move_Border *
_e_mod_move_border_client_find(Ecore_X_Window win)
{
   return eina_hash_find(border_clients, e_util_winid_str_get(win));
}

static E_Move *
_e_mod_move_find(Ecore_X_Window root)
{
   Eina_List *l;
   E_Move *m;
   EINA_LIST_FOREACH(moves, l, m)
     {
        if (!m) continue;
        if (m->man->root == root) return m;
     }
   return NULL;
}

static Eina_Bool
_e_mod_move_mouse_btn_dn(void    *data,
                         int type __UNUSED__,
                         void    *event)
{
   E_Move *m;
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   m = e_mod_move_util_get();
   if ((m) && (m->ev_log))
     {
        E_Move_Event_Log *log = NULL;

        log = E_NEW(E_Move_Event_Log, 1);
        if (log)
          {
             if (ev->multi.device == 0) // single mouse down
               {
                  log->t = E_MOVE_EVENT_LOG_ECORE_SINGLE_MOUSE_DOWN;
                  log->d.ec_sm.win = ev->window;
                  log->d.ec_sm.x = ev->x;
                  log->d.ec_sm.y = ev->y;
                  log->d.ec_sm.btn = ev->buttons;
               }
             else if (ev->multi.device > 0) // multi mouse down
               {
                  log->t = E_MOVE_EVENT_LOG_ECORE_MULTI_MOUSE_DOWN;
                  log->d.ec_mm.win = ev->window;
                  log->d.ec_mm.x = ev->multi.x;
                  log->d.ec_mm.y = ev->multi.y;
                  log->d.ec_mm.btn = ev->buttons;
                  log->d.ec_mm.dev = ev->multi.device;
               }
             else
               {
                  log->t = E_MOVE_EVENT_LOG_UNKOWN;
               }
             // list check and append
             if (eina_list_count(m->ev_logs) >= m->ev_log_cnt)
               {
                  // if log list is full, delete first log
                  E_Move_Event_Log *first_log = (E_Move_Event_Log*)eina_list_nth(m->ev_logs, 0);
                  m->ev_logs = eina_list_remove(m->ev_logs, first_log);
               }
             m->ev_logs = eina_list_append(m->ev_logs, log);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_mouse_btn_up(void    *data,
                         int type __UNUSED__,
                         void    *event)
{
   E_Move *m;
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   m = e_mod_move_util_get();
   if ((m) && (m->ev_log))
     {
        E_Move_Event_Log *log = NULL;

        log = E_NEW(E_Move_Event_Log, 1);
        if (log)
          {
             if (ev->multi.device == 0) // single mouse up
               {
                  log->t = E_MOVE_EVENT_LOG_ECORE_SINGLE_MOUSE_UP;
                  log->d.ec_sm.win = ev->window;
                  log->d.ec_sm.x = ev->x;
                  log->d.ec_sm.y = ev->y;
                  log->d.ec_sm.btn = ev->buttons;
               }
             else if (ev->multi.device > 0) // multi mouse up
               {
                  log->t = E_MOVE_EVENT_LOG_ECORE_MULTI_MOUSE_UP;
                  log->d.ec_mm.win = ev->window;
                  log->d.ec_mm.x = ev->multi.x;
                  log->d.ec_mm.y = ev->multi.y;
                  log->d.ec_mm.btn = ev->buttons;
                  log->d.ec_mm.dev = ev->multi.device;
               }
             else
               {
                  log->t = E_MOVE_EVENT_LOG_UNKOWN;
               }
             // list check and append
             if (eina_list_count(m->ev_logs) >= m->ev_log_cnt)
               {
                  // if log list is full, delete first log
                  E_Move_Event_Log *first_log = (E_Move_Event_Log*)eina_list_nth(m->ev_logs, 0);
                  m->ev_logs = eina_list_remove(m->ev_logs, first_log);
               }
             m->ev_logs = eina_list_append(m->ev_logs, log);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_property(void *data  __UNUSED__,
                     int type    __UNUSED__,
                     void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;
   Ecore_X_Atom a = 0;
   if (!ev) return ECORE_CALLBACK_PASS_ON;
   if (!ev->atom) return ECORE_CALLBACK_PASS_ON;
   if (!e_mod_move_atoms_name_get(ev->atom)) return ECORE_CALLBACK_PASS_ON;
   a = ev->atom;

   L(LT_EVENT_X,
     "[MOVE] ev:%15.15s w:0x%08x atom:%s\n",
     "X_PROPERTY", ev->win,
     e_mod_move_atoms_name_get(a));

   if (a == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
     _e_mod_move_prop_rotate_window_angle(ev);
   else if (a == ATOM_CM_LOG                              )
     e_mod_move_debug_prop_handle(ev);
   else if (a == ATOM_CM_WINDOW_INPUT_REGION              )
     _e_mod_move_prop_window_input_region(ev);
   else if (a == ATOM_CM_WINDOW_CONTENTS_REGION           )
     _e_mod_move_prop_window_contents_region(ev);
   else if (a == ECORE_X_ATOM_NET_ACTIVE_WINDOW           )
     _e_mod_move_prop_active_window(ev);
   else if (a == ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE    )
     _e_mod_move_prop_indicator_state(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_message(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   Ecore_X_Event_Client_Message *ev;
   Ecore_X_Atom t;
   ev = (Ecore_X_Event_Client_Message *)event;

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN((ev->format == 32), ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(e_mod_move_atoms_name_get(ev->message_type),
                  ECORE_CALLBACK_PASS_ON);

   t = ev->message_type;

   L(LT_EVENT_X,
     "[MOVE] ev:%15.15s w:0x%08x atom:%s\n",
     "X_CLIENT_MESSAGE", ev->win,
     e_mod_move_atoms_name_get(t));

   if      (t == ATOM_WM_WINDOW_SHOW                   ) _e_mod_move_msg_window_show(ev);
   else if (t == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) _e_mod_move_msg_qp_state(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_visibility_change(void *data __UNUSED__,
                              int type   __UNUSED__,
                              void      *event)
{
   Ecore_X_Window win;
   Ecore_X_Window target_win;
   E_Move_Border *mb = NULL;
   int fully_obscured;
   Ecore_X_Event_Window_Visibility_Change *ev;
   ev = (Ecore_X_Event_Window_Visibility_Change *)event;

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   L(LT_EVENT_X,
     "[MOVE] ev:%15.15s w:0x%08x fully_obscured:%d\n",
     "X_VISIBILITY_CHANGE", ev->win,
     ev->fully_obscured);

   win = ev->win;
   fully_obscured = ev->fully_obscured;

   mb = _e_mod_move_border_client_find(win);
   E_CHECK_RETURN(mb, ECORE_CALLBACK_PASS_ON);

   if (fully_obscured)
     {
        mb->visibility = E_MOVE_VISIBILITY_STATE_FULLY_OBSCURED;

        if (e_mod_move_indicator_controller_unset_policy_check(mb))
          e_mod_move_indicator_controller_unset(mb->m);

     }
   else
     {
        mb->visibility = E_MOVE_VISIBILITY_STATE_VISIBLE;

        if (e_mod_move_indicator_controller_set_policy_check(mb))
          {
             if (e_mod_move_indicator_controller_state_get(mb->m, &target_win))
               e_mod_move_indicator_controller_unset(mb->m);

             e_mod_move_indicator_controller_set(mb);
          }

     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_add(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   E_Event_Border_Add *ev = event;
   E_Move_Border *mb;
   E_Move* m = _e_mod_move_find(ev->border->zone->container->manager->root);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_ADD",
     ev->border->win, ev->border->client.win);

   if (!m) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_move_border_find(ev->border->win)) return ECORE_CALLBACK_PASS_ON;

   mb = _e_mod_move_bd_add_intern(m, ev->border);
   if (mb)
     _e_mod_move_bd_move_resize_intern
       (mb, ev->border->x, ev->border->y,
       ev->border->w, ev->border->h);

   if (ev->border->internal && ev->border->visible)
     _e_mod_move_bd_show_intern(mb);

   if (TYPE_INDICATOR_CHECK(mb))
     {
        E_Move_Border *fullscr_mb = NULL;
        Ecore_X_Window target_win;

        if (e_mod_move_indicator_controller_state_get(m, &target_win))
          e_mod_move_indicator_controller_unset(m);

        if ((fullscr_mb = e_mod_move_util_visible_fullscreen_window_find()))
          {
             if (e_mod_move_indicator_controller_set_policy_check(fullscr_mb))
               e_mod_move_indicator_controller_set(fullscr_mb);
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_del(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   E_Event_Border_Remove *ev = event;
   E_Move_Border *mb = _e_mod_move_border_find(ev->border->win);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_DEL",
     ev->border->win, ev->border->client.win);

   if (!mb) return ECORE_CALLBACK_PASS_ON;
   if (mb->bd == ev->border) _e_mod_move_object_del(mb, ev->border);
   _e_mod_move_bd_del_intern(mb);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_show(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Show *ev = event;
   E_Move_Border *mb = _e_mod_move_border_find(ev->border->win);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_SHOW",
     ev->border->win, ev->border->client.win);

   if (!mb) return ECORE_CALLBACK_PASS_ON;
   if (mb->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_move_bd_show_intern(mb);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_hide(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Hide *ev = event;
   E_Move_Border *mb = _e_mod_move_border_find(ev->border->win);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_HIDE",
     ev->border->win, ev->border->client.win);

   if (!mb) return ECORE_CALLBACK_PASS_ON;
   if (!mb->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_move_bd_hide_intern(mb);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_move(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Move *ev = event;
   E_Move_Border *mb = _e_mod_move_border_find(ev->border->win);
   if (!mb) return ECORE_CALLBACK_PASS_ON;

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x v:%d \
      mb[%d,%d] -> ev[%d,%d]\n", "BD_MOVE",
     ev->border->win, ev->border->client.win, mb->visible,
     mb->x, mb->y, ev->border->x, ev->border->y);

   if (!((mb->x == ev->border->x) &&
         (mb->y == ev->border->y)))
     {
        _e_mod_move_bd_move_resize_intern
          (mb, ev->border->x, ev->border->y,
          mb->w, mb->h);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_resize(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void      *event)
{
   E_Event_Border_Resize *ev = event;
   E_Move_Border *mb = _e_mod_move_border_find(ev->border->win);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x ev:%4dx%4d\n",
     "BD_RESIZE", ev->border->win, ev->border->client.win,
     ev->border->w, ev->border->h);

   if (!mb) return ECORE_CALLBACK_PASS_ON;
   if ((mb->w == ev->border->w) && (mb->h == ev->border->h))
     return ECORE_CALLBACK_PASS_ON;
   _e_mod_move_bd_move_resize_intern
     (mb, mb->x, mb->y,
     ev->border->w, ev->border->h);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_stack(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void      *event)
{
   E_Event_Border_Stack *ev = event;
   E_Move_Border *mb1 = NULL;
   E_Move_Border *mb2 = NULL;

   if (ev->type == E_STACKING_ABOVE)
     {
        if (ev->stack)
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w1:0x%08x c1:0x%08x w2:0x%08x c2:0x%08x\n",
               "BD_RAISE_ABOVE", ev->border->win, ev->border->client.win,
               ev->stack->win, ev->stack->client.win);
          }
        else
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n",
               "BD_LOWER", ev->border->win, ev->border->client.win);
          }
     }
   else if (ev->type == E_STACKING_BELOW)
     {
        if (ev->stack)
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w1:0x%08x c1:0x%08x w2:0x%08x c2:0x%08x\n",
               "BD_LOWER_BELOW", ev->border->win, ev->border->client.win,
               ev->stack->win, ev->stack->client.win);
          }
        else
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n",
               "BD_RAISE", ev->border->win, ev->border->client.win);
          }
     }

   mb1 = _e_mod_move_border_find(ev->border->win);
   if (!mb1) return ECORE_CALLBACK_PASS_ON;

   if (ev->stack)
     {
        mb2 = _e_mod_move_border_find(ev->stack->win);
     }

   if (ev->type == E_STACKING_ABOVE )
     {
        if (mb2)
          _e_mod_move_bd_stack_intern(mb1,mb2, E_MOVE_STACKING_RAISE_ABOVE);
        else
          _e_mod_move_bd_stack_intern(mb1,NULL, E_MOVE_STACKING_LOWER);
     }
   else if ( ev->type == E_STACKING_BELOW)
     {
        if (mb2)
          _e_mod_move_bd_stack_intern(mb1,mb2, E_MOVE_STACKING_LOWER_BELOW);
        else
          _e_mod_move_bd_stack_intern(mb1,NULL, E_MOVE_STACKING_RAISE);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static E_Move_Border *
_e_mod_move_bd_add_intern(E_Move   *m,
                          E_Border *bd)
{
   E_Move_Border *mb;
   Ecore_X_Window win;
   Eina_Bool      indicator_state;
   int x = 0; int y = 0; int w = 0; int h = 0;

   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(bd, 0);

   mb = E_NEW(E_Move_Border, 1);
   E_CHECK_RETURN(mb, 0);

   mb->bd = bd;
   mb->m = m;
   mb->client_win = bd->client.win;

   eina_hash_add(border_clients, e_util_winid_str_get(mb->bd->client.win), mb);
   mb->dfn = e_object_delfn_add(E_OBJECT(mb->bd), _e_mod_move_object_del, mb);

   e_mod_move_border_type_setup(mb);

   eina_hash_add(borders, e_util_winid_str_get(mb->bd->win), mb);
   mb->inhash = 1;
   m->borders = eina_inlist_append(m->borders, EINA_INLIST_GET(mb));

   // Add Move Control Object
   _e_mod_move_ctl_obj_add(mb);
   // Check Window's Input region property and Change Contol Object size with Input Region
   win = e_mod_move_util_client_xid_get(mb);
   if (_e_mod_move_prop_window_input_region_get(win, &x, &y, &w, &h))
     {
        if (e_mod_move_border_shape_input_new(mb))
          {
             e_mod_move_border_shape_input_rect_set(mb, x, y, w, h);
             e_mod_move_bd_move_ctl_objs_resize(mb, w, h);
             e_mod_move_bd_move_ctl_objs_move(mb, mb->x + x, mb->y + y);
          }
     }

   // Check Window's Contents region property
   if (_e_mod_move_prop_window_contents_region_get(win, &x, &y, &w, &h))
     {
        if (e_mod_move_border_contents_new(mb))
          e_mod_move_border_contents_rect_set(mb, x, y, w, h);
     }

   // check client window's indicator state
   if (_e_mod_move_prop_indicator_state_get(win, &indicator_state))
     {
        if (indicator_state)
          mb->indicator_state = E_MOVE_INDICATOR_STATE_ON;
        else
          mb->indicator_state = E_MOVE_INDICATOR_STATE_OFF;
     }
   else
     {
        mb->indicator_state = E_MOVE_INDICATOR_STATE_NONE;
     }

   // add visibility initial value
   mb->visibility = E_MOVE_VISIBILITY_STATE_NONE;

   // add internal data
   mb->data = _e_mod_move_bd_internal_data_add(mb);

   return mb;
}

static void
_e_mod_move_bd_del_intern(E_Move_Border *mb)
{
   Ecore_X_Window target_win;
   E_CHECK(mb);

   if (mb->dfn)
     {
        if (mb->bd)
          {
             if (mb->inhash)
               eina_hash_del(borders, e_util_winid_str_get(mb->bd->win), mb);
             eina_hash_del(border_clients, e_util_winid_str_get(mb->bd->client.win), mb);
             e_object_delfn_del(E_OBJECT(mb->bd), mb->dfn);
             mb->dfn = NULL;
             mb->bd = NULL;
          }
     }

   // if indicator destroy and indicator controller is visible then destroy indicator controller
   if (TYPE_INDICATOR_CHECK(mb))
     {
        if (e_mod_move_indicator_controller_state_get(mb->m, &target_win))
          e_mod_move_indicator_controller_unset(mb->m);
     }

   // if indicator controller target win is destroy then destroy indicator controller
   if (e_mod_move_indicator_controller_state_get(mb->m, &target_win))
     {
        if (mb->client_win == target_win)
          e_mod_move_indicator_controller_unset(mb->m);
     }

   if (mb->shape_input) e_mod_move_border_shape_input_free(mb);
   if (mb->contents) e_mod_move_border_contents_free(mb);
   if (mb->anim_data) _e_mod_move_bd_anim_data_del(mb);
   if (mb->data) _e_mod_move_bd_internal_data_del(mb);
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   _e_mod_move_bd_obj_del(mb);
   _e_mod_move_ctl_obj_del(mb);
   mb->m->borders = eina_inlist_remove(mb->m->borders, EINA_INLIST_GET(mb));
   memset(mb, 0, sizeof(E_Move_Border));
   free(mb);
}

static void
_e_mod_move_bd_show_intern(E_Move_Border *mb)
{
   E_CHECK(mb);

   if (mb->visible) return;
   mb->visible = 1;

   e_mod_move_bd_move_ctl_objs_show(mb);

   // If Apptray is showed, then move out of screen.
   if (TYPE_APPTRAY_CHECK(mb))
     e_mod_move_apptray_e_border_move(mb, -10000, -10000);
}

static void
_e_mod_move_bd_hide_intern(E_Move_Border *mb)
{
   E_CHECK(mb);
   if (!mb->visible) return;

   mb->visible = 0;
   e_mod_move_bd_move_ctl_objs_hide(mb);
}

static void
_e_mod_move_bd_move_resize_intern(E_Move_Border *mb,
                                  int            x,
                                  int            y,
                                  int            w,
                                  int            h)
{
   int shape_input_x = 0; int shape_input_y = 0;
   int shape_input_w = 0; int shape_input_h = 0;

   E_CHECK(mb);

   if (!((w == mb->w) && (h == mb->h)))
     {
        mb->w = w;
        mb->h = h;

        if (TYPE_INDICATOR_CHECK(mb) && (mb->shape_input != NULL))
           e_mod_move_border_shape_input_rect_set(mb, 0, 0, mb->w, mb->h);

        if (TYPE_INDICATOR_CHECK(mb))
          e_mod_move_bd_move_ctl_objs_resize(mb, mb->w, mb->h);
     }

   if (!((x == mb->x) && (y == mb->y)))
     {
        mb->x = x;
        mb->y = y;

        if (e_mod_move_border_shape_input_rect_get(mb,
                                                   &shape_input_x,
                                                   &shape_input_y,
                                                   &shape_input_w,
                                                   &shape_input_h))
          {
             e_mod_move_bd_move_ctl_objs_move(mb,
                                              mb->x + shape_input_x,
                                              mb->y + shape_input_y);
          }
     }

   // update indicator_contoller's evas_object size & shape mask region
   if (TYPE_INDICATOR_CHECK(mb))
     {
        Ecore_X_Window target_win;
        if (e_mod_move_indicator_controller_state_get(mb->m, &target_win))
           e_mod_move_indicator_controller_update(mb->m);
     }
}

static void
_e_mod_move_bd_stack_intern(E_Move_Border  *mb1,
                            E_Move_Border  *mb2,
                            E_Move_Stacking type)
{
   E_CHECK(mb1);
   switch (type)
     {
      case E_MOVE_STACKING_RAISE_ABOVE:
         E_CHECK(mb2);
         mb1->m->borders = eina_inlist_remove(mb1->m->borders,
                                              EINA_INLIST_GET(mb1));
         mb2->m->borders = eina_inlist_append_relative(mb2->m->borders,
                                                       EINA_INLIST_GET(mb1),
                                                       EINA_INLIST_GET(mb2));
         break;
      case E_MOVE_STACKING_LOWER_BELOW:
         E_CHECK(mb2);
         mb1->m->borders = eina_inlist_remove(mb1->m->borders,
                                              EINA_INLIST_GET(mb1));
         mb2->m->borders = eina_inlist_prepend_relative(mb2->m->borders,
                                                        EINA_INLIST_GET(mb1),
                                                        EINA_INLIST_GET(mb2));
         break;
      case E_MOVE_STACKING_RAISE:
         mb1->m->borders = eina_inlist_remove(mb1->m->borders,
                                              EINA_INLIST_GET(mb1));
         mb1->m->borders = eina_inlist_append(mb1->m->borders,
                                              EINA_INLIST_GET(mb1));
         break;
      case E_MOVE_STACKING_LOWER:
         mb1->m->borders = eina_inlist_remove(mb1->m->borders,
                                              EINA_INLIST_GET(mb1));
         mb1->m->borders = eina_inlist_prepend(mb1->m->borders,
                                               EINA_INLIST_GET(mb1));
         break;
      case E_MOVE_STACKING_NONE:
      default:
         break;
     }

   if (TYPE_APPTRAY_CHECK(mb1))
     e_mod_move_apptray_restack_post_process(mb1);
}

static void
_e_mod_move_object_del(void *data,
                       void *obj)
{
   E_Move_Border *mb = (E_Move_Border *)data;
   E_CHECK(mb);
   E_CHECK(mb->bd);

   if (obj == mb->bd)
     {
        if (mb->inhash)
          eina_hash_del(borders, e_util_winid_str_get(mb->bd->win), mb);
        eina_hash_del(border_clients,
                      e_util_winid_str_get(mb->bd->client.win), mb);
        e_mod_move_bd_move_objs_data_del(mb, "move_bd");
        mb->bd = NULL;
     }
   if (mb->dfn)
     {
        e_object_delfn_del(obj, mb->dfn);
        mb->dfn = NULL;
     }
}

static void
_e_mod_move_bd_obj_del(E_Move_Border *mb)
{
   E_CHECK(mb);
   e_mod_move_bd_move_objs_del(mb, mb->objs);
}

static void
_e_mod_move_ctl_obj_add(E_Move_Border *mb)
{
   E_CHECK(mb);
   if (TYPE_INDICATOR_CHECK(mb)
       || TYPE_APPTRAY_CHECK(mb)
       || TYPE_QUICKPANEL_CHECK(mb))
     {
        E_Move_Control_Object *mco = NULL;
        Eina_List *l;

        mb->ctl_objs = e_mod_move_bd_move_ctl_objs_add(mb);

        if (!mb->ctl_objs) return;

        EINA_LIST_FOREACH(mb->ctl_objs, l, mco)
          {
             if (!mco) continue;
             _e_mod_move_ctl_obj_event_setup(mb, mco);
          }
     }
}

static void
_e_mod_move_ctl_obj_del(E_Move_Border *mb)
{
   E_CHECK(mb);
   e_mod_move_bd_move_ctl_objs_del(mb, mb->ctl_objs);
}

static void
_e_mod_move_ctl_obj_event_setup(E_Move_Border         *mb,
                                E_Move_Control_Object *mco)
{
   E_CHECK(mb);
   E_CHECK(mco);
   /* ADD OBJ Event handler ( evas object move / down/ up event handler, with E_MOVE_EVENT ) */
   if (TYPE_INDICATOR_CHECK(mb))
     e_mod_move_indicator_ctl_obj_event_setup(mb, mco);
   else if (TYPE_APPTRAY_CHECK(mb))
     e_mod_move_apptray_ctl_obj_event_setup(mb, mco);
   else if (TYPE_QUICKPANEL_CHECK(mb))
     e_mod_move_quickpanel_ctl_obj_event_setup(mb, mco);
}

static void*
_e_mod_move_bd_internal_data_add(E_Move_Border *mb)
{
   void *ret = NULL;
   E_CHECK_RETURN(mb, NULL);
   if (TYPE_APPTRAY_CHECK(mb))
     ret = e_mod_move_apptray_internal_data_add(mb);
   else if (TYPE_QUICKPANEL_CHECK(mb))
     ret = e_mod_move_quickpanel_internal_data_add(mb);
   else if (TYPE_INDICATOR_CHECK(mb))
     ret = e_mod_move_indicator_internal_data_add(mb);
   return ret;
}

static Eina_Bool
_e_mod_move_bd_internal_data_del(E_Move_Border *mb)
{
   Eina_Bool ret = EINA_FALSE;
   E_CHECK_RETURN(mb, EINA_FALSE);
   if (TYPE_APPTRAY_CHECK(mb))
     ret = e_mod_move_apptray_internal_data_del(mb);
   else if (TYPE_QUICKPANEL_CHECK(mb))
     ret = e_mod_move_quickpanel_internal_data_del(mb);
   else if (TYPE_INDICATOR_CHECK(mb))
     ret = e_mod_move_indicator_internal_data_del(mb);
   return ret;
}

static Eina_Bool
_e_mod_move_bd_anim_data_del(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   if (TYPE_APPTRAY_CHECK(mb))
     {
        if (e_mod_move_apptray_objs_animation_state_get(mb))
          {
             e_mod_move_apptray_objs_animation_stop(mb);
             e_mod_move_apptray_objs_animation_clear(mb);
          }
     }
   else if (TYPE_QUICKPANEL_CHECK(mb))
     {
        if (e_mod_move_quickpanel_objs_animation_state_get(mb))
          {
             e_mod_move_quickpanel_objs_animation_stop(mb);
             e_mod_move_quickpanel_objs_animation_clear(mb);
          }
     }
   return EINA_TRUE;;
}

static Eina_Bool
_e_mod_move_prop_window_input_region_get(Ecore_X_Window win,
                                         int           *x,
                                         int           *y,
                                         int           *w,
                                         int           *h)
{
   int ret = -1;
   unsigned int val[4] = { 0, 0, 0, 0};

   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   E_CHECK_RETURN(w, EINA_FALSE);
   E_CHECK_RETURN(h, EINA_FALSE);

   ret = ecore_x_window_prop_card32_get(win,
                                        ATOM_CM_WINDOW_INPUT_REGION,
                                        val, 4);
   if (ret == -1) return EINA_FALSE;

   *x = val[0];
   *y = val[1];
   *w = val[2];
   *h = val[3];

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_window_contents_region_get(Ecore_X_Window win,
                                            int           *x,
                                            int           *y,
                                            int           *w,
                                            int           *h)
{
   int ret = -1;
   unsigned int val[4] = { 0, 0, 0, 0};

   E_CHECK_RETURN(x, EINA_FALSE);
   E_CHECK_RETURN(y, EINA_FALSE);
   E_CHECK_RETURN(w, EINA_FALSE);
   E_CHECK_RETURN(h, EINA_FALSE);

   ret = ecore_x_window_prop_card32_get(win,
                                        ATOM_CM_WINDOW_CONTENTS_REGION,
                                        val, 4);
   if (ret == -1) return EINA_FALSE;

   *x = val[0];
   *y = val[1];
   *w = val[2];
   *h = val[3];

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_active_window_get(Ecore_X_Window *active_win)
{
   int            ret = -1;
   Ecore_X_Window win;
   E_Move        *m = NULL;

   E_CHECK_RETURN(active_win, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   ret = ecore_x_window_prop_window_get(m->man->root,
                                        ECORE_X_ATOM_NET_ACTIVE_WINDOW,
                                        &win, 1);

   if (ret == -1) return EINA_FALSE;

   *active_win = win;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_indicator_state_get(Ecore_X_Window win,
                                     Eina_Bool     *state)
{
   Eina_Bool ret = EINA_FALSE;
   Eina_Bool ret_state = EINA_FALSE;
   Ecore_X_Illume_Indicator_State indicator_state;

   E_CHECK_RETURN(state, EINA_FALSE);

   indicator_state = ecore_x_e_illume_indicator_state_get(win);

   if (indicator_state == ECORE_X_ILLUME_INDICATOR_STATE_ON)
     {
        ret_state = EINA_TRUE;
        ret = EINA_TRUE;
     }
   else if (indicator_state == ECORE_X_ILLUME_INDICATOR_STATE_OFF)
     {
        ret_state = EINA_FALSE;
        ret = EINA_TRUE;
     }
   else
     ret = EINA_FALSE;

   if (ret) *state = ret_state;

   return ret;
}

static Eina_Bool
_e_mod_move_prop_window_input_region(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Border *mb = NULL;
   Ecore_X_Window win;
   int x = 0; int y = 0; int w = 0; int h = 0;

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (!mb->shape_input) e_mod_move_border_shape_input_new(mb);
   E_CHECK_RETURN(mb->shape_input, EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);

   if (!_e_mod_move_prop_window_input_region_get(win, &x, &y, &w, &h))
     return EINA_FALSE;

   e_mod_move_border_shape_input_rect_set(mb, x, y, w, h);
   if (e_mod_move_border_shape_input_rect_get(mb, &x, &y, &w, &h))
     {
        e_mod_move_bd_move_ctl_objs_resize(mb, w, h);
        e_mod_move_bd_move_ctl_objs_move(mb, mb->x + x, mb->y + y);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_window_contents_region(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Border *mb = NULL;
   Ecore_X_Window win;
   int x = 0; int y = 0; int w = 0; int h = 0;

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (!mb->contents) e_mod_move_border_contents_new(mb);
   E_CHECK_RETURN(mb->contents, EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);

   if (!_e_mod_move_prop_window_contents_region_get(win, &x, &y, &w, &h))
     return EINA_FALSE;

   e_mod_move_border_contents_rect_set(mb, x, y, w, h);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_active_window_internal_quickpanel_move(E_Move_Border *mb,
                                                        int            x,
                                                        int            y)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb),EINA_FALSE);
   if (e_mod_move_quickpanel_click_get()) // if apptray process event, do event clear
     e_mod_move_quickpanel_event_clear();
   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     {
        e_mod_move_quickpanel_objs_animation_stop(mb);
        e_mod_move_quickpanel_objs_animation_clear(mb);
     }
   e_mod_move_quickpanel_objs_add(mb);
   e_mod_move_quickpanel_e_border_move(mb, x, y);
   e_mod_move_quickpanel_objs_animation_move(mb, x, y);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_active_window(Ecore_X_Event_Window_Property *ev)
{
   Ecore_X_Window active_win;
   E_Move_Border *qp_mb = NULL;
   E_Move_Border *active_mb = NULL;
   int            angles[2];
   Ecore_X_Window win;
   E_Zone         *zone = NULL;
   int            x, y;
   x = 0; y = 0;
   L(LT_EVENT_X,
     "[MOVE] ev:%15.15s w:0x%07x %s():%d\n",
     "X_PROPERTY",ev->win, __func__, __LINE__);

   if (_e_mod_move_prop_active_window_get(&active_win))
     {
        active_mb = _e_mod_move_border_client_find(active_win);
        E_CHECK_RETURN(active_mb, EINA_FALSE);

        if ((qp_mb = e_mod_move_quickpanel_find()))
          {
             if ( (qp_mb->visible)  // quickpanel is vislble
                  && (REGION_INSIDE_ZONE(qp_mb, qp_mb->bd->zone)) //quickpanel is on screen
                  && (REGION_INSIDE_ZONE(active_mb, qp_mb->bd->zone))) // active_win is on quickpanel's zone
               {
                  // do hide quickpanel
                  // if it is do animation, do stop animation and delete animation
                  win = e_mod_move_util_client_xid_get(qp_mb);
                  if (e_mod_move_util_win_prop_angle_get(win, &angles[0], &angles[1]))
                     angles[0] %= 360;
                  else
                     angles[0] = 0;
                  zone = qp_mb->bd->zone;

                  switch (angles[0])
                    {
                     case   0:
                        x = 0;
                        y = qp_mb->h * -1;
                        break;
                     case  90:
                        x = qp_mb->w * -1;
                        y = 0;
                        break;
                     case 180:
                        x = 0;
                        y = zone->h;
                        break;
                     case 270:
                        x = zone->w;
                        y = 0;
                        break;
                     default :
                        break;
                    }
                  _e_mod_move_prop_active_window_internal_quickpanel_move(qp_mb,
                                                                           x,
                                                                           y);
                  L(LT_EVENT_X,
                    "[MOVE] ev:%15.15s Apptray Hide %s():%d\n",
                    "X_PROPERTY", __func__, __LINE__);
               }
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_indicator_state(Ecore_X_Event_Window_Property *ev)
{
   Ecore_X_Window win;
   Ecore_X_Window target_win;
   Eina_Bool      indicator_state;
   E_Move_Border *mb = NULL;

   E_CHECK_RETURN(ev, EINA_FALSE);
   win = ev->win;

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (_e_mod_move_prop_indicator_state_get(ev->win, &indicator_state))
     {
        if (indicator_state)
          {
             mb->indicator_state = E_MOVE_INDICATOR_STATE_ON;

             if (e_mod_move_indicator_controller_unset_policy_check(mb))
               e_mod_move_indicator_controller_unset(mb->m);

          }
        else
          {
             mb->indicator_state = E_MOVE_INDICATOR_STATE_OFF;

             if (e_mod_move_indicator_controller_set_policy_check(mb))
               {
                  if (e_mod_move_indicator_controller_state_get(mb->m, &target_win))
                    e_mod_move_indicator_controller_unset(mb->m);
                  e_mod_move_indicator_controller_set(mb);
               }

          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_rotate_window_angle(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Border *mb = NULL;
   Ecore_X_Window win;
   int            angles[2];
   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);
   if (e_mod_move_util_win_prop_angle_get(win, &angles[0], &angles[1]))
     angles[0] %= 360;
   else
     angles[0] = 0;

   mb->angle = angles[0];

   if (TYPE_INDICATOR_CHECK(mb))
     _e_mod_move_outside_zone_border_position_update(mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_window_show_internal_quickpanel_check(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   if (e_mod_move_indicator_click_get()) // if indicator process event, do event clear
     e_mod_move_indicator_event_clear();
   if (e_mod_move_quickpanel_click_get()) // if quickpanel process event, do event clear
     e_mod_move_quickpanel_event_clear();
   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     {
       e_mod_move_quickpanel_objs_animation_stop(mb);
       e_mod_move_quickpanel_objs_animation_clear(mb);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_window_show_internal_apptray_check(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   if (e_mod_move_indicator_click_get()) // if indicator process event, do event clear
     e_mod_move_indicator_event_clear();
   if (e_mod_move_apptray_click_get()) // if apptray process event, do event clear
     e_mod_move_apptray_event_clear();
   if (e_mod_move_apptray_objs_animation_state_get(mb))
     {
        e_mod_move_apptray_objs_animation_stop(mb);
        e_mod_move_apptray_objs_animation_clear(mb);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_window_show(Ecore_X_Event_Client_Message *ev)
{
   E_Move        *m = NULL;
   E_Move_Border *mb = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   E_Move_Border *indi_mb = NULL;
   Ecore_X_Window win;
   E_Zone        *zone = NULL;
   Eina_Bool      state;
   Eina_Bool      comp_obj_visible = EINA_FALSE;
   int open, angles[2];
   int x, y;
   x = 0; y = 0;

   E_CHECK_RETURN(ev, EINA_FALSE);

   open = !(!(ev->data.l[1]));
   mb = _e_mod_move_border_client_find(ev->data.l[0]);
   E_CHECK_RETURN(mb, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);


   // if it is do animation, do stop animation and delete animation
   // must do animation check, and delete animation

   win = e_mod_move_util_client_xid_get(mb);
   if (e_mod_move_util_win_prop_angle_get(win, &angles[0], &angles[1]))
     angles[0] %= 360;
   else
     angles[0] = 0;

   indi_mb = e_mod_move_indicator_find();
   at_mb = e_mod_move_apptray_find();
   qp_mb = e_mod_move_quickpanel_find();
   zone = mb->bd->zone;

   comp_obj_visible = e_mod_move_util_compositor_object_visible_get(mb);

   if (REGION_INSIDE_ZONE(mb, zone)) state = EINA_TRUE;
   else state = EINA_FALSE;

   if ((open) && (!state) && (comp_obj_visible))  // Open Case
     {
        E_CHECK_RETURN(e_mod_move_indicator_scrollable_check(), EINA_FALSE);

        if (mb == qp_mb) // Quickpanel Case
          {
             switch (angles[0])
               {
                case   0:
                   x = 0;
                   y = 0;
                   break;
                case  90:
                   x = 0;
                   y = 0;
                   break;
                case 180:
                   x = 0;
                   y = zone->h - qp_mb->h;
                   break;
                case 270:
                   x = zone->w - qp_mb->w;
                   y = 0;
                   break;
                default :
                   x = 0;
                   y = 0;
                   break;
               }
             _e_mod_move_msg_window_show_internal_quickpanel_check(qp_mb);
             e_mod_move_quickpanel_dim_show(qp_mb);
             e_mod_move_quickpanel_objs_add(qp_mb);
             e_mod_move_quickpanel_e_border_move(qp_mb, x, y);
             e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                     angles[0]);
             e_mod_move_quickpanel_objs_animation_move(qp_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Quickpanel Show %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
        else if (mb == at_mb) // Apptray Case
          {
             switch (angles[0])
               {
                case   0:
                   x = 0;
                   y = 0;
                   break;
                case  90:
                   x = 0;
                   y = 0;
                   break;
                case 180:
                   x = 0;
                   y = zone->h - at_mb->h;
                   break;
                case 270:
                   x = zone->w - at_mb->w;
                   y = 0;
                   break;
                default :
                   x = 0;
                   y = 0;
                   break;
               }
             _e_mod_move_msg_window_show_internal_apptray_check(at_mb);
             e_mod_move_apptray_dim_show(at_mb);
             e_mod_move_apptray_objs_add(at_mb);
             //climb up indicator
             e_mod_move_apptray_objs_raise(at_mb);

             e_mod_move_apptray_e_border_move(at_mb, x, y);
             e_mod_move_apptray_objs_animation_start_position_set(at_mb,
                                                                  angles[0]);
             e_mod_move_apptray_objs_animation_move(at_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Apptray Show %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
     }
   else if ((!open) && (state)) // Close Case
     {
        if (mb == qp_mb) // Quickpanel case
          {
             switch (angles[0])
               {
                case   0:
                   x = 0;
                   y = qp_mb->h * -1;
                   break;
                case  90:
                   x = qp_mb->w * -1;
                   y = 0;
                   break;
                case 180:
                   x = 0;
                   y = zone->h;
                   break;
                case 270:
                   x = zone->w;
                   y = 0;
                   break;
                default :
                   x = 0;
                   y = qp_mb->h * -1;
                   break;
               }
             _e_mod_move_msg_window_show_internal_quickpanel_check(qp_mb);
             e_mod_move_quickpanel_objs_add(qp_mb);
             e_mod_move_quickpanel_e_border_move(qp_mb, x, y);
             e_mod_move_quickpanel_objs_animation_move(qp_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Quickpanel Hide %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
        else if (mb == at_mb) // Apptray Case
          {
             switch (angles[0])
               {
                case   0:
                   x = 0;
                   y = at_mb->h * -1;
                   break;
                case  90:
                   x = at_mb->w * -1;
                   y = 0;
                   break;
                case 180:
                   x = 0;
                   y = zone->h;
                   break;
                case 270:
                   x = zone->w;
                   y = 0;
                   break;
                default :
                   x = 0;
                   y = at_mb->h * -1;
                   break;
               }
             _e_mod_move_msg_window_show_internal_apptray_check(at_mb);
             e_mod_move_apptray_objs_add(at_mb);
             e_mod_move_apptray_e_border_move(at_mb, x, y);
             e_mod_move_apptray_objs_animation_move(at_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Apptray Hide %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
     }
   else
     {
        fprintf(stderr,
                "[MOVE_MODULE] _NET_WM_WINDOW_SHOW error."
                " w:0x%07x(state:%d) request:%d\n",
                win, state, open);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_qp_state_internal_quickpanel_check(E_Move_Border *mb)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   if (e_mod_move_indicator_click_get()) // if indicator process event, do event clear
     e_mod_move_indicator_event_clear();
   if (e_mod_move_quickpanel_click_get()) // if quickpanel process event, do event clear
     e_mod_move_quickpanel_event_clear();
   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     {
        e_mod_move_quickpanel_objs_animation_stop(mb);
        e_mod_move_quickpanel_objs_animation_clear(mb);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_qp_state(Ecore_X_Event_Client_Message *ev)
{
   Ecore_X_Atom   qp_state;
   Ecore_X_Window zone_win;
   E_Move        *m = NULL;
   E_Move_Border *qp_mb = NULL;
   Ecore_X_Window win;
   Eina_Bool      state;
   Eina_Bool      comp_obj_visible = EINA_FALSE;
   E_Zone        *zone;
   int cx, cy, cw, ch;
   int open, angles[2];
   int ax = 0; // animation x
   int ay = 0; // animation y
   int mx = 0; // border move x
   int my = 0; // border move y

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   qp_mb = e_mod_move_quickpanel_find();
   E_CHECK_RETURN(qp_mb, EINA_FALSE);

   zone_win = ev->win;
   win = e_mod_move_util_client_xid_get(qp_mb);

   if (zone_win != ecore_x_e_illume_zone_get(win)) return EINA_FALSE;

   qp_state = ev->data.l[0];
   if (qp_state == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF) open = 0;
   else open = 1;

   if (e_mod_move_util_win_prop_angle_get(win, &angles[0], &angles[1]))
     angles[0] %= 360;
   else
     angles[0] = 0;

   comp_obj_visible = e_mod_move_util_compositor_object_visible_get(qp_mb);

   zone = qp_mb->bd->zone;
   if (REGION_INSIDE_ZONE(qp_mb, zone)) state = EINA_TRUE;
   else state = EINA_FALSE;

   if ((open) && (!state) && (comp_obj_visible)) // Quickpanel Open
     {
        E_CHECK_RETURN(e_mod_move_indicator_scrollable_check(), EINA_FALSE);

        switch (angles[0])
          {
           case   0:
              ax = 0;
              ay = 0;
              mx = ax;
              my = ay;
              break;
           case  90:
              ax = 0;
              ay = 0;
              mx = ax;
              my = ay;
              break;
           case 180:
              ax = 0;
              ay = zone->h - qp_mb->h;
              mx = ax;
              my = ay;
              break;
           case 270:
              ax = zone->w - qp_mb->w;
              ay = 0;
              mx = ax;
              my = ay;
              break;
           default :
              ax = 0;
              ay = 0;
              mx = ax;
              my = ay;
              break;
          }
        _e_mod_move_msg_qp_state_internal_quickpanel_check(qp_mb);
        e_mod_move_quickpanel_dim_show(qp_mb);
        e_mod_move_quickpanel_objs_add(qp_mb);
        e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
        e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                angles[0]);
        e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
     }
   else if ((!open) && (state))// Quickpanel Close
     {
        switch (angles[0])
          {
           case  90:
              if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                {
                   ax = cw * -1;
                   ay = 0;
                   mx = qp_mb->w * -1;
                   my = 0;
                }
              else
                {
                   ax = qp_mb->w * -1;
                   ay = 0;
                   mx = ax;
                   my = ay;
                }
              break;
           case 180:
              if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                {
                   ax = 0;
                   ay = ch;
                   mx = ax;
                   my = zone->h;
                }
              else
                {
                   ax = 0;
                   ay = zone->h;
                   mx = ax;
                   my = ay;
                }
              break;
           case 270:
              if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                {
                   ax = cw;
                   ay = 0;
                   mx = zone->w;
                   my = ay;
                }
              else
                {
                   ax = zone->w;
                   ay = 0;
                   mx = ax;
                   my = ay;
                }
              break;
           case   0:
           default :
              if (e_mod_move_border_contents_rect_get(qp_mb, &cx, &cy ,&cw, &ch))
                {
                   ax = 0;
                   ay = ch * -1;
                   mx = 0;
                   my = qp_mb->h * -1;
                }
              else
                {
                   ax = 0;
                   ay = qp_mb->h * -1;
                   mx = ax;
                   my = ay;
                }
              break;
          }
        _e_mod_move_msg_qp_state_internal_quickpanel_check(qp_mb);
        e_mod_move_quickpanel_objs_add(qp_mb);
        e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
        e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
     }
   else
     {
        fprintf(stderr,
                "[MOVE_MODULE] _E_ILLUME_QUICKPANEL_STATE error."
                " w:0x%07x(state:%d) request:%d\n",
                win, state, open);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_outside_zone_border_position_update(E_Move_Border *mb)
{
   E_Zone *zone = NULL;
   E_Move_Border *at_mb = NULL;
   E_Move_Border *qp_mb = NULL;
   int angle;
   int x, y;

   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);
   angle = mb->angle;
   zone = mb->bd->zone;
   x = -10000;
   y = -10000;
   if ((at_mb = e_mod_move_apptray_find()))
     {
        if ((zone == at_mb->bd->zone)
            && !REGION_INSIDE_ZONE(at_mb, zone))
          {
             e_mod_move_apptray_e_border_move(at_mb, x, y);
          }
     }
   if ((qp_mb = e_mod_move_quickpanel_find()))
     {
        if ((zone == qp_mb->bd->zone)
            && !REGION_INSIDE_ZONE(qp_mb, zone))
          {
             e_mod_move_quickpanel_e_border_move(qp_mb, x, y);
          }
     }
   return EINA_TRUE;
}

static E_Move *
_e_mod_move_add(E_Manager *man)
{
   E_Move *m;
   Ecore_X_Window *wins;
   int i, num;

   E_Move_Canvas *canvas;
   E_Border *bd;

   m = E_NEW(E_Move, 1);
   E_CHECK_RETURN(m, NULL);

   m->man = man;

   canvas = e_mod_move_canvas_add(m, NULL);
   if (!canvas)
     {
        memset(m, 0, sizeof(E_Move));
        E_FREE(m);
        return NULL;
     }

   m->indicator_home_region_ratio = _move_mod->conf->indicator_home_region_ratio;
   m->qp_scroll_with_visible_win = _move_mod->conf->qp_scroll_with_visible_win;
   m->flick_speed_limit = _move_mod->conf->flick_speed_limit;
   m->animation_duration = _move_mod->conf->animation_duration;
   m->dim_max_opacity = _move_mod->conf->dim_max_opacity;
   m->dim_min_opacity = _move_mod->conf->dim_min_opacity;
   m->ev_log = _move_mod->conf->event_log;
   m->ev_log_cnt = _move_mod->conf->event_log_count;
   wins = ecore_x_window_children_get(m->man->root, &num);
   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             E_Move_Border *mb;
             if ((bd = e_border_find_by_window(wins[i])))
               {
                  mb = _e_mod_move_bd_add_intern(m, bd);
                  if (!mb) continue;

                  _e_mod_move_bd_move_resize_intern(mb, bd->x, bd->y, bd->w, bd->h);

                  if (bd->visible)
                    _e_mod_move_bd_show_intern(mb);
               }
          }
        free(wins);
     }

   return m;
}

static void
_e_mod_move_del(E_Move *m)
{
   Eina_List *l;
   E_Move_Border *mb;
   E_Move_Canvas *canvas;
   E_Move_Event_Log *log;

   E_CHECK(m);
   while (m->borders)
     {
        mb = (E_Move_Border *)(m->borders);
        _e_mod_move_bd_hide_intern(mb);
        _e_mod_move_bd_del_intern(mb);
     }

   EINA_LIST_FREE(m->canvases, canvas) e_mod_move_canvas_del(canvas);
   m->canvases = NULL;

   EINA_LIST_FOREACH(m->ev_logs, l, log)
     {
        memset(log, 0, sizeof(E_Move_Event_Log));
        E_FREE(log);
     }
   eina_list_free(m->ev_logs);

   if (m->borders_list) eina_list_free(m->borders_list);

   free(m);
}

/* wrapper function for external file */
EINTERN E_Move_Border *
e_mod_move_border_find(Ecore_X_Window win)
{
   return _e_mod_move_border_find(win);
}

EINTERN E_Move_Border *
e_mod_move_border_client_find(Ecore_X_Window win)
{
   return _e_mod_move_border_client_find(win);
}

EINTERN E_Move *
e_mod_move_find(Ecore_X_Window win)
{
   E_CHECK_RETURN(win, 0);
   return _e_mod_move_find(win);
}

EINTERN void
e_mod_move_border_del(E_Move_Border * mb)
{
   E_CHECK(mb);
   _e_mod_move_object_del(mb, mb->bd);
   _e_mod_move_bd_del_intern(mb);
}

Eina_Bool
e_mod_move_init(void)
{
   Eina_List *l;
   E_Manager *man;
   int res = 0;

   borders = eina_hash_string_superfast_new(NULL);
   border_clients = eina_hash_string_superfast_new(NULL);

   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,          _e_mod_move_mouse_btn_dn,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,            _e_mod_move_mouse_btn_up,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,          _e_mod_move_property,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,           _e_mod_move_message,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE, _e_mod_move_visibility_change, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ADD,                     _e_mod_move_bd_add,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE,                  _e_mod_move_bd_del,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_SHOW,                    _e_mod_move_bd_show,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_HIDE,                    _e_mod_move_bd_hide,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_MOVE,                    _e_mod_move_bd_move,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_RESIZE,                  _e_mod_move_bd_resize,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_STACK,                   _e_mod_move_bd_stack,          NULL));

   res = e_mod_move_atoms_init();
   E_CHECK_RETURN(res, 0);

   res = e_mod_move_border_type_init();
   E_CHECK_RETURN(res, 0);

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Move *m;
        if (!man) continue;
        m = _e_mod_move_add(man);
        if (m)
          {
             moves = eina_list_append(moves, m);
             e_mod_move_util_set(m, man);
          }
     }
   return 1;
}

void
e_mod_move_shutdown(void)
{
   E_Move *m;

   EINA_LIST_FREE(moves, m) _e_mod_move_del(m);

   E_FREE_LIST(handlers, ecore_event_handler_del);

   if (borders) eina_hash_free(borders);
   if (border_clients) eina_hash_free(border_clients);
   borders = NULL;
   border_clients = NULL;

   e_mod_move_border_type_shutdown();
   e_mod_move_atoms_shutdown();

   e_mod_move_util_set(NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////
