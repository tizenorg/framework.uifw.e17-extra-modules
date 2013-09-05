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
static Eina_List     *handlers       = NULL;
static Eina_List     *moves          = NULL;
static Eina_Hash     *border_clients = NULL;
static Eina_Hash     *borders        = NULL;
static Eina_List     *e_border_hooks = NULL;
static E_Msg_Handler *e_msg_handler  = NULL;

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

static E_Move_Border *_e_mod_move_bd_new_intern(E_Move *m, E_Border *bd);
static void           _e_mod_move_bd_add_intern(E_Move_Border *mb);
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
static Eina_Bool      _e_mod_move_prop_mini_apptray_state_get(Ecore_X_Window win, E_Move_Mini_Apptray_State *state);
static Eina_Bool      _e_mod_move_prop_indicator_type_get(Ecore_X_Window win, E_Move_Indicator_Type *type);
static Eina_Bool      _e_mod_move_prop_fullscreen_indicator_show_state_get(Ecore_X_Window win, Eina_Bool *state);
static Eina_Bool      _e_mod_move_prop_indicator_geometry_get(Ecore_X_Window win, E_Move *m);

static Eina_Bool      _e_mod_move_prop_window_input_region(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_window_contents_region(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_active_window(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_indicator_state(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_mini_apptray_state(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_indicator_type(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_fullscreen_indicator_show_state(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_rotate_window_angle(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_rotate_root_angle(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_panel_scrollable_state(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_indicator_geometry(Ecore_X_Event_Window_Property *ev);

static Eina_Bool      _e_mod_move_msg_window_show(Ecore_X_Event_Client_Message *ev);
static Eina_Bool      _e_mod_move_msg_qp_state(Ecore_X_Event_Client_Message *ev);

static Eina_Bool      _e_mod_move_outside_zone_border_position_update(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_prop_active_window_internal_quickpanel_move(E_Move_Border *mb, int x, int y);
static Eina_Bool      _e_mod_move_msg_window_show_internal_apptray_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_msg_window_show_internal_quickpanel_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_msg_window_show_internal_mini_apptray_check(E_Move_Border *mb);
static Eina_Bool      _e_mod_move_msg_qp_state_internal_quickpanel_check(E_Move_Border *mb);

static void           _e_mod_move_bd_new_hook(void *data __UNUSED__, void *data2);
static void           _e_mod_move_bd_del_hook(void *data __UNUSED__, void *data2);

static void           _e_mod_move_e_msg_handler(void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata);
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
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   if (ev->multi.device == 0) // single mouse down
     {
        SECURE_SLOGD("%23s   w:0x%08x (%4d ,%4d )  btn:%d\n",
                     "ECORE_SINGLE_MOUSE_DOWN",
                     ev->window,
                     ev->x,
                     ev->y,
                     ev->buttons);
     }
   else if (ev->multi.device > 0) // multi mouse down
     {
        SECURE_SLOGD("%23s   w:0x%08x (%5.1f,%5.1f)  btn:%d | dev:%d\n",
                     "ECORE_MULTI_MOUSE_DOWN",
                     ev->window,
                     ev->multi.x,
                     ev->multi.y,
                     ev->buttons,
                     ev->multi.device);
     }
   else
     {
        SECURE_SLOGD("%23s\n","EVENT_LOG_UNKOWN");
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_mouse_btn_up(void    *data,
                         int type __UNUSED__,
                         void    *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);

   if (ev->multi.device == 0) // single mouse up
     {
        SECURE_SLOGD("%23s   w:0x%08x (%4d ,%4d )  btn:%d\n",
                     "ECORE_SINGLE_MOUSE_UP",
                     ev->window,
                     ev->x,
                     ev->y,
                     ev->buttons);
     }
   else if (ev->multi.device > 0) // multi mouse up
     {
        SECURE_SLOGD("%23s   w:0x%08x (%5.1f,%5.1f)  btn:%d | dev:%d\n",
                     "ECORE_MULTI_MOUSE_UP",
                     ev->window,
                     ev->multi.x,
                     ev->multi.y,
                     ev->buttons,
                     ev->multi.device);
     }
   else
     {
        SECURE_SLOGD("%23s\n","EVENT_LOG_UNKOWN");
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

   if (a == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE     )
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
   else if (a == ATOM_MV_FULLSCREEN_INDICATOR_SHOW        )
     _e_mod_move_prop_fullscreen_indicator_show_state(ev);
   else if (a == ATOM_MV_PANEL_SCROLLABLE_STATE           )
     _e_mod_move_prop_panel_scrollable_state(ev);
   else if (a == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE  )
     _e_mod_move_prop_rotate_root_angle(ev);
   else if (a == ATOM_MV_INDICATOR_GEOMETRY               )
     _e_mod_move_prop_indicator_geometry(ev);
   else if (a == ECORE_X_ATOM_E_ILLUME_INDICATOR_TYPE_MODE)
     _e_mod_move_prop_indicator_type(ev);
   else if (a == ATOM_MV_MINI_APPTRAY_STATE)
     _e_mod_move_prop_mini_apptray_state(ev);

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


   if (t == ATOM_WM_WINDOW_SHOW)
     {
        SLOG(LOG_DEBUG, "E17_MOVE_MODULE",
             "[e17:X_CLIENT_MESSAGE] w:0x%08x atom:%s",
             ev->win,e_mod_move_atoms_name_get(t));
        _e_mod_move_msg_window_show(ev);
     }
   else if (t == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) _e_mod_move_msg_qp_state(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_visibility_change(void *data __UNUSED__,
                              int type   __UNUSED__,
                              void      *event)
{
   Ecore_X_Window win;
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
     }
   else
     {
        mb->visibility = E_MOVE_VISIBILITY_STATE_VISIBLE;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_add(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   E_Event_Border_Add *ev = event;
   E_Move_Border *mb = NULL;
   E_Move *m = e_mod_move_util_get();

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_ADD",
     ev->border->win, ev->border->client.win);

   E_CHECK_RETURN(m, ECORE_CALLBACK_PASS_ON);

   mb = _e_mod_move_border_find(ev->border->win);
   E_CHECK_RETURN(mb, ECORE_CALLBACK_PASS_ON);

   _e_mod_move_bd_add_intern(mb);

   _e_mod_move_bd_move_resize_intern(mb,
                                     ev->border->x, ev->border->y,
                                     ev->border->w, ev->border->h);

   if (ev->border->internal && ev->border->visible)
     _e_mod_move_bd_show_intern(mb);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_del(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void      *event)
{
   E_Move                *m  = e_mod_move_util_get();
   E_Move_Border         *mb = NULL;
   E_Event_Border_Remove *ev = event;

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_DEL",
     ev->border->win, ev->border->client.win);

   // func call flow.
   // BD_DEL_HOOK -> BD_HIDE -> BD_DEL -> E_MSG_Handler ['comp.manager':'visibility.src']
   //
   // "e_msg config.src" event is received after "e_border del" event
   // and move module's e_boder del handler delete E_Move_Boder structure. (_e_mod_move_object_del() )
   // so implicit call widget apply fuctions ( indicator widget, mini_apptray widget)
   E_CHECK_RETURN(m, ECORE_CALLBACK_PASS_ON);
   if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();
   e_mod_move_mini_apptray_widget_apply();

   mb = _e_mod_move_border_find(ev->border->win);
   E_CHECK_RETURN(mb, ECORE_CALLBACK_PASS_ON);
   _e_mod_move_object_del(mb, ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_show(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void      *event)
{
   E_Event_Border_Show *ev = event;
   E_Move_Border *mb = NULL;

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);
   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_SHOW",
     ev->border->win, ev->border->client.win);

   mb = _e_mod_move_border_find(ev->border->win);
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
   E_Move_Border *mb = NULL;

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);
   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_HIDE",
     ev->border->win, ev->border->client.win);

   mb = _e_mod_move_border_find(ev->border->win);
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
   E_Move *m = e_mod_move_util_get();
   E_Move_Border *mb = NULL;

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);

   ELBF(ELBT_MOVE, 0, ev->border->win,
        "%15.15s| BD_MOVE w:0x%08x c:0x%08x ev[%d,%d]",
        "MOVE", ev->border->win, ev->border->client.win, ev->border->x, ev->border->y);

   mb = _e_mod_move_border_find(ev->border->win);
   E_CHECK_RETURN(m, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(mb, ECORE_CALLBACK_PASS_ON);

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

   // if compositor send config.src "e_msg" event before BD_Move "E Event", then indicator widget could not apply correctly.
   // so explicit function (indicator_widget_apply() )call is required on BD_RESIZE_Event Handler.
   // if Move module's stack info equals to E_Stack info then following explicit call should remove.
   if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();

   e_mod_move_mini_apptray_widget_apply();

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_resize(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void      *event)
{
   E_Event_Border_Resize *ev = event;
   E_Move_Border *mb = NULL;
   E_Move *m = e_mod_move_util_get();

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);
   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x ev:%4dx%4d\n",
     "BD_RESIZE", ev->border->win, ev->border->client.win,
     ev->border->w, ev->border->h);

   mb = _e_mod_move_border_find(ev->border->win);
   E_CHECK_RETURN(m, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(mb, ECORE_CALLBACK_PASS_ON);

   if ((mb->w == ev->border->w) && (mb->h == ev->border->h))
     return ECORE_CALLBACK_PASS_ON;
   _e_mod_move_bd_move_resize_intern
     (mb, mb->x, mb->y,
     ev->border->w, ev->border->h);

   // if compositor send config.src "e_msg" event before BD_RESIZE "E Event", then indicator widget could not apply correctly.
   // so explicit function (indicator_widget_apply() )call is required on BD_RESIZE_Event Handler.
   // if Move module's stack info equals to E_Stack info then following explicit call should remove.
   if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();

   e_mod_move_mini_apptray_widget_apply();

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

   E_CHECK_RETURN(ev, ECORE_CALLBACK_PASS_ON);
   E_CHECK_RETURN(ev->border, ECORE_CALLBACK_PASS_ON);
   if (ev->type == E_STACKING_ABOVE)
     {
        if (ev->stack)
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w1:0x%08x c1:0x%08x w2:0x%08x c2:0x%08x\n",
               "BD_RAISE_ABOVE", ev->border->win, ev->border->client.win,
               ev->stack->win, ev->stack->client.win);
             ELBF(ELBT_MOVE, 0, ev->border->win,
                  "%15.15s| BD_RAISE_ABOVE w1:0x%08x c1:0x%08x w2:0x%08x c2:0x%08x",
                  "MOVE", ev->border->win, ev->border->client.win,
                  ev->stack->win, ev->stack->client.win);
          }
        else
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n",
               "BD_LOWER", ev->border->win, ev->border->client.win);
             ELBF(ELBT_MOVE, 0, ev->border->win,
                  "%15.15s| BD_LOWER w:0x%08x c:0x%08x",
                  "MOVE", ev->border->win, ev->border->client.win);
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
             ELBF(ELBT_MOVE, 0, ev->border->win,
                  "%15.15s| BD_LOWER_BELOW w1:0x%08x c1:0x%08x w2:0x%08x c2:0x%08x",
                  "MOVE", ev->border->win, ev->border->client.win,
                  ev->stack->win, ev->stack->client.win);
          }
        else
          {
             L(LT_EVENT_BD,
               "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n",
               "BD_RAISE", ev->border->win, ev->border->client.win);
             ELBF(ELBT_MOVE, 0, ev->border->win,
                  "%15.15s| BD_RAISE w:0x%08x c:0x%08x",
                  "MOVE", ev->border->win, ev->border->client.win);
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

static void
_e_mod_move_bd_new_hook(void *data __UNUSED__,
                        void      *data2)
{
   // "e_border_add event" could follow after "e_border_stack evnet" (e17-core does not ensure e_border event order)
   // but e_border_new hook handler is called before other e_border events. (e17-core ensures e_border_hook hander order)
   E_Border      *bd = data2;
   E_Move        *m  = e_mod_move_util_get();

   E_CHECK(bd);
   E_CHECK(m);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_NEW_HOOK",
     bd->win, bd->client.win);
   ELBF(ELBT_MOVE, 0, bd->win,
        "%15.15s| BD_NEW_HOOK w:0x%08x c:0x%08x",
        "MOVE", bd->win, bd->client.win);

   if (_e_mod_move_border_find(bd->win)) return;

   _e_mod_move_bd_new_intern(m, bd);
}

static void
_e_mod_move_bd_del_hook(void *data __UNUSED__,
                        void      *data2)
{
   E_Border      *bd = data2;
   E_Move_Border *mb = NULL;
   E_CHECK(bd);

   L(LT_EVENT_BD,
     "[MOVE] ev:%15.15s w:0x%08x c:0x%08x\n", "BD_DEL_HOOK",
     bd->win, bd->client.win);
   ELBF(ELBT_MOVE, 0, bd->win,
        "%15.15s| BD_DEL_HOOK w:0x%08x c:0x%08x",
        "MOVE", bd->win, bd->client.win);

   // border is not processed by _e_border_eval()
   //  - case: bd_new -> bd_del
   //    : this case "bd_del e_event" does not occure. e17 only calls bd_del hook.
   //  - general flow: bd_new() -> _e_border_eval() -> bd_del()
   //    :  _e_border_eval() makes new_client flag to zero.
   if (bd->new_client)
     {
        mb = _e_mod_move_border_find(bd->win);
        E_CHECK(mb);
        _e_mod_move_object_del(mb, bd);
     }
}

static void
_e_mod_move_e_msg_handler(void       *data,
                          const char *name,
                          const char *info,
                          int         val,
                          E_Object   *obj,
                          void       *msgdata)
{
   E_Move *m = e_mod_move_util_get();
   E_Manager *man = (E_Manager *)obj;
   E_Manager_Comp_Source *src = (E_Manager_Comp_Source *)msgdata;
   E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;
   E_Move_Indicator_Widget *indi_widget = NULL;

   L(LT_EVENT_X, "[MOVE] ev:E_MSG '%s':'%s'\n", name, info);
   E_CHECK(m);

   // handle only comp.manager msg
   if (!strncmp(name, "comp.manager", sizeof("comp.manager")))
     {
        if (!strncmp(info, "resize.comp", sizeof("resize.comp")))
          {
             L(LT_EVENT_X,
               "[MOVE] ev:E_MSG %15.15s w:0x%08x manager: %p, comp_src: %p\n",
               info, e_manager_comp_src_window_get(man,src), man, src);
          }
        else if (!strncmp(info, "add.src", sizeof("add.src")))
          {
             L(LT_EVENT_X,
               "[MOVE] ev:E_MSG %15.15s w:0x%08x manager: %p, comp_src: %p\n",
               info, e_manager_comp_src_window_get(man,src), man, src);
          }
        else if (!strncmp(info, "del.src", sizeof("del.src")))
          {
             L(LT_EVENT_X,
               "[MOVE] ev:E_MSG %15.15s w:0x%08x manager: %p, comp_src: %p\n",
               info, e_manager_comp_src_window_get(man,src), man, src);
          }
        else if (!strncmp(info, "config.src", sizeof("config.src")))
          {
             L(LT_EVENT_X,
               "[MOVE] ev:E_MSG %15.15s w:0x%08x manager: %p, comp_src: %p\n",
               info, e_manager_comp_src_window_get(man,src), man, src);
             // if Quickpanel is OnScreen then move quickpanel's below window with animation position.
             if (e_mod_move_quickpanel_visible_check()
                 && !m->qp_scroll_with_clipping
                 && !e_mod_move_quickpanel_objs_animation_state_get(e_mod_move_quickpanel_find()))
               {
                  e_mod_move_quickpanel_below_window_reset();
                  e_mod_move_quickpanel_objs_move(e_mod_move_quickpanel_find(), 0, 0);
               }

             // if indicator widget mode, then apply indicator widget visibility, position
             if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();

             e_mod_move_mini_apptray_widget_apply();
          }
        else if (!strncmp(info, "visibility.src", sizeof("visibility.src")))
          {
             L(LT_EVENT_X,
               "[MOVE] ev:E_MSG %15.15s w:0x%08x manager: %p, comp_src: %p\n",
               info, e_manager_comp_src_window_get(man,src), man, src);
             // if Quickpanel is OnScreen then move quickpanel's below window with animation position.
             if (e_mod_move_quickpanel_visible_check()
                 && !m->qp_scroll_with_clipping
                 && !e_mod_move_quickpanel_objs_animation_state_get(e_mod_move_quickpanel_find()))
               {
                  e_mod_move_quickpanel_below_window_reset();
                  e_mod_move_quickpanel_objs_move(e_mod_move_quickpanel_find(), 0, 0);
               }

             // if indicator widget mode, then apply indicator widget visibility, position
             if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();

             e_mod_move_mini_apptray_widget_apply();
          }
     }
   else if (!strncmp(name, "screen-reader", sizeof("screen-reader")))
     {
        if (!strncmp(info, "enable", sizeof("enable")))
          {
             L(LT_EVENT_X, "[MOVE] ev:E_MSG [%s:%s]\n", name, info);
             if (!m->screen_reader_state) // if screen-reader is not activate then work.
               {
                  m->screen_reader_state = EINA_TRUE;

                  indi_widget = e_mod_move_indicator_widget_get();
                  if (indi_widget) e_mod_move_indicator_widget_del(indi_widget);

                  mini_apptray_widget = e_mod_move_mini_apptray_widget_get();
                  if (mini_apptray_widget) e_mod_move_mini_apptray_widget_del(mini_apptray_widget);
               }
          }
        else if (!strncmp(info, "disable", sizeof("disable")))
          {
             L(LT_EVENT_X, "[MOVE] ev:E_MSG [%s:%s]\n", name, info);
             if (m->screen_reader_state) // if screen-reader is activate then work.
               {
                  m->screen_reader_state = EINA_FALSE;
                  if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();
                  e_mod_move_mini_apptray_widget_apply();
               }
          }
     }
}

static E_Move_Border *
_e_mod_move_bd_new_intern(E_Move   *m,
                          E_Border *bd)
{
   E_Move_Border *mb;

   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(bd, 0);

   mb = E_NEW(E_Move_Border, 1);
   E_CHECK_RETURN(mb, 0);

   mb->bd = bd;
   mb->m = m;
   mb->client_win = bd->client.win;

   eina_hash_add(border_clients, e_util_winid_str_get(mb->bd->client.win), mb);
   mb->dfn = e_object_delfn_add(E_OBJECT(mb->bd), _e_mod_move_object_del, mb);

   eina_hash_add(borders, e_util_winid_str_get(mb->bd->win), mb);
   mb->inhash = 1;
   m->borders = eina_inlist_append(m->borders, EINA_INLIST_GET(mb));

   return mb;
}

static void
_e_mod_move_bd_add_intern(E_Move_Border *mb)
{
   Ecore_X_Window            win;
   Eina_Bool                 indicator_state;
   Eina_Bool                 fullscreen_indicator_show_state;
   E_Move_Mini_Apptray_State mini_apptray_state = E_MOVE_MINI_APPTRAY_STATE_NONE;
   int x = 0; int y = 0; int w = 0; int h = 0;
   int                       angles[2];
   E_Move                   *m = e_mod_move_util_get();
   E_Border                 *bd = NULL;

   E_CHECK(mb);
   E_CHECK(m);

   bd = mb->bd;
   E_CHECK(bd);

   e_mod_move_border_type_setup(mb);
   mb->argb = ecore_x_window_argb_get(bd->win);

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

   // check client window's indicator type
   _e_mod_move_prop_indicator_type_get(win, &mb->indicator_type);

   // check client window's fullscreen indicator show state
   if (_e_mod_move_prop_fullscreen_indicator_show_state_get(win,
                                                            &fullscreen_indicator_show_state))
     {
        if (fullscreen_indicator_show_state)
          mb->fullscreen_indicator_show_state = E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_ON;
        else
          mb->fullscreen_indicator_show_state = E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_OFF;
     }
   else
     {
        mb->fullscreen_indicator_show_state = E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_NONE;
     }

   // Check Mini Apptray State
   if (_e_mod_move_prop_mini_apptray_state_get(win, &mini_apptray_state))
     {
        mb->mini_apptray_state = mini_apptray_state;
     }

   // Check Window's Angle Property
   if (e_mod_move_util_win_prop_angle_get(win, &angles[0], &angles[1]))
     angles[0] %= 360;
   else
     angles[0] = 0;
   mb->angle = angles[0];

   // add visibility initial value
   mb->visibility = E_MOVE_VISIBILITY_STATE_NONE;

   // add internal data
   mb->data = _e_mod_move_bd_internal_data_add(mb);

   // panel scrollable init
   e_mod_move_panel_scrollable_state_init(&(mb->panel_scrollable_state));
   e_mod_move_panel_scrollable_state_get(win, &(mb->panel_scrollable_state));

   // if current window is Indicator, then get indicator_geometry_property.
   if (TYPE_INDICATOR_CHECK(mb))
     {
        e_mod_move_util_prop_indicator_cmd_win_set(win, m);
        _e_mod_move_prop_indicator_geometry_get(win, m);
     }
   else if (TYPE_SETUP_WIZARD_CHECK(mb))
     {
        E_Move_Mini_Apptray_Widget *mini_apptray_widget = NULL;
        E_Move_Indicator_Widget *indi_widget = NULL;

        if (!m->setup_wizard_state)
          {
             m->setup_wizard_state = EINA_TRUE;
             indi_widget = e_mod_move_indicator_widget_get();
             if (indi_widget) e_mod_move_indicator_widget_del(indi_widget);
             mini_apptray_widget = e_mod_move_mini_apptray_widget_get();
             if (mini_apptray_widget) e_mod_move_mini_apptray_widget_del(mini_apptray_widget);
          }
     }
}

static void
_e_mod_move_bd_del_intern(E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_CHECK(mb);

   m = e_mod_move_util_get();
   E_CHECK(m);

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

   if (mb->shape_input) e_mod_move_border_shape_input_free(mb);
   if (mb->contents) e_mod_move_border_contents_free(mb);
   if (mb->anim_data) _e_mod_move_bd_anim_data_del(mb);
   if (mb->data) _e_mod_move_bd_internal_data_del(mb);
   if (mb->flick_data) e_mod_move_flick_data_free(mb);
   _e_mod_move_bd_obj_del(mb);
   _e_mod_move_ctl_obj_del(mb);
   m->borders = eina_inlist_remove(m->borders, EINA_INLIST_GET(mb));

   if (TYPE_SETUP_WIZARD_CHECK(mb))
     {
        if ((e_mod_move_setup_wizard_find() == NULL))
          {
             m->setup_wizard_state = EINA_FALSE;
             if (m->elm_indicator_mode) e_mod_move_indicator_widget_apply();
             e_mod_move_mini_apptray_widget_apply();
          }
     }

   memset(mb, 0, sizeof(E_Move_Border));
   free(mb);
}

static void
_e_mod_move_bd_show_intern(E_Move_Border *mb)
{
   E_Border *bd = NULL;
   E_Zone   *zone = NULL;

   E_CHECK(mb);
   bd = mb->bd;
   E_CHECK(bd);
   zone = bd->zone;
   E_CHECK(zone);

   if (mb->visible) return;
   mb->visible = 1;

   e_mod_move_bd_move_ctl_objs_show(mb);

   // If Apptray is shown, then move out of screen.
   if (TYPE_APPTRAY_CHECK(mb))
     e_mod_move_apptray_e_border_move(mb, -10000, -10000);
   // If MiniApptray is shown, then move out of screen.
   if (TYPE_MINI_APPTRAY_CHECK(mb))
     e_mod_move_mini_apptray_e_border_move(mb, -10000, -10000);
   // If Indicator Window is shown by user application, then move out of screen.
   // later indicator window is controlled by window manager
   if (TYPE_INDICATOR_CHECK(mb))
     e_mod_move_indicator_e_border_move(mb, -10000, -10000);
   // If Quickpanel is shown, then move out of screen.
   if (TYPE_QUICKPANEL_CHECK(mb))
     {
        e_mod_move_quickpanel_e_border_move(mb, -10000, -10000);
        ecore_x_e_illume_quickpanel_state_set(zone->black_win, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
     }
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
   E_Move *m = NULL;
   int shape_input_x = 0; int shape_input_y = 0;
   int shape_input_w = 0; int shape_input_h = 0;

   E_CHECK(mb);

   m = e_mod_move_util_get();
   E_CHECK(m);

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

   _e_mod_move_bd_del_intern(mb);
}

static void
_e_mod_move_bd_obj_del(E_Move_Border *mb)
{
   E_CHECK(mb);
   if (TYPE_QUICKPANEL_CHECK(mb))
     e_mod_move_quickpanel_objs_del(mb);
   else if (TYPE_MINI_APPTRAY_CHECK(mb))
     e_mod_move_mini_apptray_objs_del(mb);
   else
     e_mod_move_bd_move_objs_del(mb, mb->objs);
}

static void
_e_mod_move_ctl_obj_add(E_Move_Border *mb)
{
   E_CHECK(mb);
   if (TYPE_INDICATOR_CHECK(mb)
       || TYPE_APPTRAY_CHECK(mb)
       || TYPE_QUICKPANEL_CHECK(mb)
       || TYPE_MINI_APPTRAY_CHECK(mb))
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
   else if (TYPE_MINI_APPTRAY_CHECK(mb))
     e_mod_move_mini_apptray_ctl_obj_event_setup(mb, mco);
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
   else if (TYPE_MINI_APPTRAY_CHECK(mb))
     ret = e_mod_move_mini_apptray_internal_data_add(mb);
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
   else if (TYPE_MINI_APPTRAY_CHECK(mb))
     ret = e_mod_move_mini_apptray_internal_data_del(mb);
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
   else if (TYPE_MINI_APPTRAY_CHECK(mb))
     {
        if (e_mod_move_mini_apptray_objs_animation_state_get(mb))
          {
             e_mod_move_mini_apptray_objs_animation_stop(mb);
             e_mod_move_mini_apptray_objs_animation_clear(mb);
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
_e_mod_move_prop_mini_apptray_state_get(Ecore_X_Window             win,
                                        E_Move_Mini_Apptray_State *state)
{
   int                       ret = -1;
   unsigned int              mini_apptray_state = 0;
   Eina_Bool                 ret_val = EINA_FALSE;
   E_Move_Mini_Apptray_State ret_state = E_MOVE_MINI_APPTRAY_STATE_NONE;

   E_CHECK_RETURN(state, EINA_FALSE);

   ret = ecore_x_window_prop_card32_get(win,
                                        ATOM_MV_MINI_APPTRAY_STATE,
                                        &mini_apptray_state,
                                        1);

   if (ret == -1)
     ret_val = EINA_FALSE;
   else
     {
        ret_val = EINA_TRUE;
        if (mini_apptray_state == 0)
          ret_state = E_MOVE_MINI_APPTRAY_STATE_OFF;
        else if (mini_apptray_state == 1)
          ret_state = E_MOVE_MINI_APPTRAY_STATE_ON;
        else
          ret_state = E_MOVE_MINI_APPTRAY_STATE_NONE;
     }

   if (ret_val) *state = ret_state;

   return ret_val;
}

static Eina_Bool
_e_mod_move_prop_indicator_type_get(Ecore_X_Window         win,
                                    E_Move_Indicator_Type *type)
{
   Ecore_X_Illume_Indicator_Type_Mode indicator_type;
   E_Move_Indicator_Type              ret_type;

   E_CHECK_RETURN(type, EINA_FALSE);

   indicator_type = ecore_x_e_illume_indicator_type_get(win);

   if (indicator_type == ECORE_X_ILLUME_INDICATOR_TYPE_1)
     ret_type = E_MOVE_INDICATOR_TYPE_1;
   else if (indicator_type == ECORE_X_ILLUME_INDICATOR_TYPE_2)
     ret_type = E_MOVE_INDICATOR_TYPE_2;
   else
     ret_type = E_MOVE_INDICATOR_TYPE_NONE;

   *type = ret_type;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_fullscreen_indicator_show_state_get(Ecore_X_Window win,
                                                     Eina_Bool     *state)
{
   int          ret = -1;
   unsigned int fullscreen_indicator_show_state = 0;
   Eina_Bool    ret_state = EINA_FALSE;
   Eina_Bool    ret_val = EINA_FALSE;

   E_CHECK_RETURN(state, EINA_FALSE);

   ret = ecore_x_window_prop_card32_get(win,
                                        ATOM_MV_FULLSCREEN_INDICATOR_SHOW,
                                        &fullscreen_indicator_show_state,
                                        1);

   if (ret == -1)
     ret_val = EINA_FALSE;
   else
     {
        ret_val = EINA_TRUE;
        if (fullscreen_indicator_show_state > 0)
          ret_state = EINA_TRUE;
        else
          ret_state = EINA_FALSE;
     }

   if (ret_val) *state = ret_state;

   return ret_val;
}

static Eina_Bool
_e_mod_move_prop_indicator_geometry_get(Ecore_X_Window win,
                                        E_Move        *m)
{
   int            ret = -1;
   unsigned int   val[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

   E_CHECK_RETURN(m, EINA_FALSE);

   ret = ecore_x_window_prop_card32_get(win,
                                        ATOM_MV_INDICATOR_GEOMETRY,
                                        val, 16);
   if (ret == -1) return EINA_FALSE;

   m->indicator_widget_geometry[E_MOVE_ANGLE_0  ].x = val[0];
   m->indicator_widget_geometry[E_MOVE_ANGLE_0  ].y = val[1];
   m->indicator_widget_geometry[E_MOVE_ANGLE_0  ].w = val[2];
   m->indicator_widget_geometry[E_MOVE_ANGLE_0  ].h = val[3];
   m->indicator_widget_geometry[E_MOVE_ANGLE_90 ].x = val[4];
   m->indicator_widget_geometry[E_MOVE_ANGLE_90 ].y = val[5];
   m->indicator_widget_geometry[E_MOVE_ANGLE_90 ].w = val[6];
   m->indicator_widget_geometry[E_MOVE_ANGLE_90 ].h = val[7];
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].x = val[8];
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].y = val[9];
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].w = val[10];
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].h = val[11];
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].x = val[12];
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].y = val[13];
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].w = val[14];
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].h = val[15];

   return EINA_TRUE;
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

   if (TYPE_QUICKPANEL_CHECK(mb))
     e_mod_move_quickpanel_window_input_region_change_post_job(mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_window_contents_region(Ecore_X_Event_Window_Property *ev)
{
   E_Move *m = NULL;
   E_Move_Border *mb = NULL;
   Ecore_X_Window win;
   int x = 0; int y = 0; int w = 0; int h = 0;

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (!mb->contents) e_mod_move_border_contents_new(mb);
   E_CHECK_RETURN(mb->contents, EINA_FALSE);

   m = mb->m;
   win = e_mod_move_util_client_xid_get(mb);

   if (!_e_mod_move_prop_window_contents_region_get(win, &x, &y, &w, &h))
     return EINA_FALSE;

   e_mod_move_border_contents_rect_set(mb, x, y, w, h);

   if ( TYPE_QUICKPANEL_CHECK(mb)
        && REGION_INSIDE_ZONE(mb, mb->bd->zone)
        && !m->qp_scroll_with_clipping
        && (mb->visible))
     {
        e_mod_move_quickpanel_objs_move(mb, 0, 0);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_active_window_internal_quickpanel_move(E_Move_Border *mb,
                                                        int            x,
                                                        int            y)
{
   E_Move *m = NULL;
   E_Zone *zone = NULL;
   int     angle = 0;
   int     ax = 0, ay = 0; // animation x, y

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb),EINA_FALSE);

   m = mb->m;

   if (e_mod_move_quickpanel_click_get()) // if apptray process event, do event clear
     e_mod_move_quickpanel_event_clear();
   if (e_mod_move_quickpanel_objs_animation_state_get(mb))
     {
        e_mod_move_quickpanel_objs_animation_stop(mb);
        e_mod_move_quickpanel_objs_animation_clear(mb);
     }
   e_mod_move_quickpanel_objs_add(mb);
   e_mod_move_quickpanel_e_border_move(mb, x, y);

   if (m->qp_scroll_with_clipping)
     {
        zone = mb->bd->zone;
        angle = mb->angle;

        switch (angle)
          {
           case 0:
           case 90:
           default:
              ax = zone->x; ay = zone->y;
              break;
           case 180:
           case 270:
              ax = zone->x + mb->w; ay = zone->y + mb->h;
              break;
          }
     }
   else
     {
        ax = x; ay = y;
     }

   e_mod_move_quickpanel_objs_animation_move(mb, ax, ay);

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
   Eina_Bool      indicator_state;
   E_Move_Border *mb = NULL;
   E_Move        *m = NULL;

   E_CHECK_RETURN(ev, EINA_FALSE);
   win = ev->win;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (_e_mod_move_prop_indicator_state_get(ev->win, &indicator_state))
     {
        if (indicator_state)
          {
             mb->indicator_state = E_MOVE_INDICATOR_STATE_ON;
          }
        else
          {
             mb->indicator_state = E_MOVE_INDICATOR_STATE_OFF;
          }
        // control indicator widget state.
        if (m->elm_indicator_mode)
          e_mod_move_indicator_widget_state_change(ev->win, indicator_state);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_mini_apptray_state(Ecore_X_Event_Window_Property *ev)
{
   Ecore_X_Window            win;
   E_Move_Mini_Apptray_State mini_apptray_state = E_MOVE_MINI_APPTRAY_STATE_NONE;
   E_Move_Border            *mb = NULL;
   E_Move                   *m = NULL;

   E_CHECK_RETURN(ev, EINA_FALSE);
   win = ev->win;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (_e_mod_move_prop_mini_apptray_state_get(ev->win, &mini_apptray_state))
     {
        if (mb->mini_apptray_state != mini_apptray_state)
          {
             mb->mini_apptray_state = mini_apptray_state;
             e_mod_move_mini_apptray_widget_apply();
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_indicator_type(Ecore_X_Event_Window_Property *ev)
{
   Ecore_X_Window win;
   E_Move_Border *mb = NULL;
   E_Move        *m = NULL;
   E_Move_Indicator_Type indicator_type = E_MOVE_INDICATOR_TYPE_NONE;

   E_CHECK_RETURN(ev, EINA_FALSE);
   win = ev->win;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (_e_mod_move_prop_indicator_type_get(ev->win, &indicator_type))
     {
        mb->indicator_type = indicator_type;

        // control indicator widget state
        if (m->elm_indicator_mode)
          e_mod_move_indicator_widget_type_change(ev->win, indicator_type);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_fullscreen_indicator_show_state(Ecore_X_Event_Window_Property *ev)
{
   Ecore_X_Window win;
   Eina_Bool      fullscreen_indicator_show_state;
   E_Move_Border *mb = NULL;
   E_Move        *m = NULL;

   E_CHECK_RETURN(ev, EINA_FALSE);
   win = ev->win;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   if (_e_mod_move_prop_fullscreen_indicator_show_state_get(ev->win,
                                                            &fullscreen_indicator_show_state))
     {
        if (fullscreen_indicator_show_state)
          {
             mb->fullscreen_indicator_show_state = E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_ON;
          }
        else
          {
             mb->fullscreen_indicator_show_state = E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_OFF;
          }
     }

   return EINA_TRUE;

}

static Eina_Bool
_e_mod_move_prop_rotate_window_angle(Ecore_X_Event_Window_Property *ev)
{
   E_Move        *m = NULL;
   E_Move_Border *mb = NULL;
   Ecore_X_Window win;
   int            angles[2];

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = _e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);
   if (e_mod_move_util_win_prop_angle_get(win, &angles[0], &angles[1]))
     angles[0] %= 360;
   else
     angles[0] = 0;

   mb->angle = angles[0];

   if (!m->elm_indicator_mode)
     {
        if (TYPE_INDICATOR_CHECK(mb))
          _e_mod_move_outside_zone_border_position_update(mb);
     }

   // indicator widget angle change method
   if (m->elm_indicator_mode)
     {
        e_mod_move_indicator_widget_angle_change(ev->win);
        e_mod_move_indicator_widget_angle_change_post_job();
     }

   e_mod_move_mini_apptray_widget_angle_change(ev->win);
   e_mod_move_mini_apptray_widget_angle_change_post_job();

   if (TYPE_QUICKPANEL_CHECK(mb))
     e_mod_move_quickpanel_angle_change_post_job(mb);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_rotate_root_angle(Ecore_X_Event_Window_Property *ev)
{
   E_Move        *m = NULL;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_panel_scrollable_state(Ecore_X_Event_Window_Property *ev)
{
   Ecore_X_Window win;
   E_Move_Border *mb = NULL;

   E_CHECK_RETURN(ev, EINA_FALSE);
   win = ev->win;

   mb = _e_mod_move_border_client_find(win);
   E_CHECK_RETURN(mb, EINA_FALSE);

   return e_mod_move_panel_scrollable_state_get(win, &(mb->panel_scrollable_state));
}

static Eina_Bool
_e_mod_move_prop_indicator_geometry(Ecore_X_Event_Window_Property *ev)
{
   E_Move *m = NULL;
   E_Move_Border *mb = NULL;
   Ecore_X_Window win;

   E_CHECK_RETURN(ev, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   mb = e_mod_move_border_client_find(ev->win);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_INDICATOR_CHECK(mb), EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);

   return _e_mod_move_prop_indicator_geometry_get(win, m);
}

static Eina_Bool
_e_mod_move_msg_window_show_internal_quickpanel_check(E_Move_Border *mb)
{
   E_Move *m = e_mod_move_util_get();;
   E_CHECK_RETURN(m, EINA_FALSE);

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_QUICKPANEL_CHECK(mb), EINA_FALSE);
   if ((!m->elm_indicator_mode) && e_mod_move_indicator_click_get()) // if indicator process event, do event clear
     e_mod_move_indicator_event_clear();
   if ((m->elm_indicator_mode)
       && e_mod_move_indicator_widget_click_get(e_mod_move_indicator_widget_get())) // if indicator widget process event, do event clear
     {
        e_mod_move_indicator_widget_event_clear(e_mod_move_indicator_widget_get());
     }
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
   E_Move *m = e_mod_move_util_get();;
   E_CHECK_RETURN(m, EINA_FALSE);

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_APPTRAY_CHECK(mb), EINA_FALSE);
   if ((!m->elm_indicator_mode) && e_mod_move_indicator_click_get()) // if indicator process event, do event clear
     e_mod_move_indicator_event_clear();
   if ((m->elm_indicator_mode)
       && e_mod_move_indicator_widget_click_get(e_mod_move_indicator_widget_get())) // if indicator widget process event, do event clear
     {
        e_mod_move_indicator_widget_event_clear(e_mod_move_indicator_widget_get());
     }
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
_e_mod_move_msg_window_show_internal_mini_apptray_check(E_Move_Border *mb)
{
   E_Move *m = e_mod_move_util_get();;
   E_CHECK_RETURN(m, EINA_FALSE);

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_MINI_APPTRAY_CHECK(mb), EINA_FALSE);

   if (e_mod_move_mini_apptray_widget_click_get(e_mod_move_mini_apptray_widget_get())) // if mini_apptray widget process event, do event clear
     {
        e_mod_move_mini_apptray_widget_event_clear(e_mod_move_mini_apptray_widget_get());
     }
   if (e_mod_move_mini_apptray_click_get()) // if mini_apptray process event, do event clear
     e_mod_move_mini_apptray_event_clear();
   if (e_mod_move_mini_apptray_objs_animation_state_get(mb))
     {
        e_mod_move_mini_apptray_objs_animation_stop(mb);
        e_mod_move_mini_apptray_objs_animation_clear(mb);
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
   E_Move_Border *mini_at_mb = NULL;
   E_Move_Border *fullscr_mb = NULL;
   Ecore_X_Window win;
   E_Zone        *zone = NULL;
   Eina_Bool      state;
   Eina_Bool      comp_obj_visible = EINA_FALSE;
   int open, angles[2];
   int x, y;
   int mx = 0, my = 0, ax = 0, ay = 0; // move x / y , animation x / y
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

   at_mb = e_mod_move_apptray_find();
   qp_mb = e_mod_move_quickpanel_find();
   mini_at_mb = e_mod_move_mini_apptray_find();
   zone = mb->bd->zone;

   comp_obj_visible = e_mod_move_util_compositor_object_visible_get(mb);

   if (open)
     {
        if (mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE) state = EINA_TRUE;
        else state = EINA_FALSE;
     }
   else
     {
         if (REGION_INSIDE_ZONE(mb, zone)) state = EINA_TRUE;
         else state = EINA_FALSE;
     }
   if ((!m->elm_indicator_mode)
       && e_mod_move_indicator_click_get())
     {
        L(LT_EVENT_X,
          "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Indicator Is Scrolling\n",
          "X_CLIENT_MESSAGE", win, state, open);
        return EINA_FALSE;
     }

   if ((m->elm_indicator_mode)
       && e_mod_move_indicator_widget_click_get(e_mod_move_indicator_widget_get()))
     {
        L(LT_EVENT_X,
          "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Indicator Widget Is Scrolling\n",
          "X_CLIENT_MESSAGE", win, state, open);
        return EINA_FALSE;
     }

   if (e_mod_move_quickpanel_click_get())
     {
        L(LT_EVENT_X,
          "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Quickpanel Is Scrolling\n",
          "X_CLIENT_MESSAGE", win, state, open);
        return EINA_FALSE;
     }

   if (e_mod_move_apptray_click_get())
     {
        L(LT_EVENT_X,
          "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Apptray Is Scrolling\n",
          "X_CLIENT_MESSAGE", win, state, open);
        return EINA_FALSE;
     }

   if (e_mod_move_mini_apptray_click_get())
     {
        L(LT_EVENT_X,
          "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, MINI_Apptray Is Scrolling\n",
          "X_CLIENT_MESSAGE", win, state, open);
        return EINA_FALSE;
     }

   if ((open) && (!state) && (comp_obj_visible))  // Open Case
     {
        if (mb == qp_mb) // Quickpanel Case
          {
             if ((at_mb) && (at_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE))
               {
                  L(LT_EVENT_X,
                    "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Apptray in On Screen\n",
                    "X_CLIENT_MESSAGE", win, state, open);
                  return EINA_FALSE;
               }

             if ((mini_at_mb) && (mini_at_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE))
               {
                  L(LT_EVENT_X,
                    "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Mini Apptray in On Screen\n",
                    "X_CLIENT_MESSAGE", win, state, open);
                  return EINA_FALSE;
               }

             if ((fullscr_mb = e_mod_move_util_visible_fullscreen_window_find()))
               {
                  E_CHECK_RETURN(e_mod_move_panel_scrollable_get(fullscr_mb, E_MOVE_PANEL_TYPE_QUICKPANEL), EINA_FALSE);
               }

             switch (angles[0])
               {
                case  90:
                   mx = 0; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case 180:
                   mx = 0; my = zone->h - qp_mb->h;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x; ay = zone->y;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case 270:
                   mx = zone->w - qp_mb->w; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x; ay = zone->y;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case   0:
                default :
                   mx = 0; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
               }
             _e_mod_move_msg_window_show_internal_quickpanel_check(qp_mb);

             if (!m->qp_scroll_with_clipping)
               e_mod_move_quickpanel_dim_show(qp_mb);

             // Set Composite Mode & Rotation Lock & Make below win's mirror object
             e_mod_move_quickpanel_stage_init(qp_mb);

             e_mod_move_quickpanel_objs_add(qp_mb);

             // send quickpanel to "move start message".
             e_mod_move_quickpanel_anim_state_send(qp_mb, EINA_TRUE);

             e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
             e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                     angles[0],
                                                                     EINA_FALSE);
             e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Quickpanel Show %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
        else if (mb == at_mb) // Apptray Case
          {
             if ((qp_mb) && (qp_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE))
               {
                  L(LT_EVENT_X,
                    "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Quickpanel in On Screen\n",
                    "X_CLIENT_MESSAGE", win, state, open);
                  return EINA_FALSE;
               }

             if ((fullscr_mb = e_mod_move_util_visible_fullscreen_window_find()))
               {
                  E_CHECK_RETURN(e_mod_move_panel_scrollable_get(fullscr_mb, E_MOVE_PANEL_TYPE_APPTRAY), EINA_FALSE);
               }

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
             // apptray_objs_animation_layer_set
             e_mod_move_apptray_objs_animation_layer_set(at_mb);

             // send apptray to "move start message".
             e_mod_move_apptray_anim_state_send(at_mb, EINA_TRUE);

             e_mod_move_apptray_e_border_move(at_mb, x, y);
             e_mod_move_apptray_objs_animation_start_position_set(at_mb,
                                                                  angles[0]);
             e_mod_move_apptray_objs_animation_move(at_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Apptray Show %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
        else if (mb == mini_at_mb) // MINI_Apptray Case
          {
             if ((qp_mb) && (qp_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE))
               {
                  L(LT_EVENT_X,
                    "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Quickpanel in On Screen\n",
                    "X_CLIENT_MESSAGE", win, state, open);
                  return EINA_FALSE;
               }

             if ((at_mb) && (at_mb->visibility == E_MOVE_VISIBILITY_STATE_VISIBLE))
               {
                  L(LT_EVENT_X,
                    "[MOVE] ev:%15.15s _NET_WM_WINDOW_SHOW error. w:0x%07x(state:%d) request:%d, Apptray in On Screen\n",
                    "X_CLIENT_MESSAGE", win, state, open);
                  return EINA_FALSE;
               }

             switch (angles[0])
               {
                case  90:
                   x = zone->w - mini_at_mb->w;
                   y = 0;
                   break;
                case 180:
                   x = 0;
                   y = 0;
                   break;
                case 270:
                   x = 0;
                   y = 0;
                   break;
                case   0:
                default :
                   x = 0;
                   y = zone->h - mini_at_mb->h;
                   break;
               }

             _e_mod_move_msg_window_show_internal_mini_apptray_check(mini_at_mb);
             e_mod_move_mini_apptray_dim_show(mini_at_mb);

             if (REGION_INSIDE_ZONE(mini_at_mb, mini_at_mb->bd->zone))
               {
                  e_mod_move_mini_apptray_e_border_move(mini_at_mb, -10000, -10000);
                  e_mod_move_mini_apptray_objs_add_with_pos(mini_at_mb, -10000, -10000);
               }
             else
                e_mod_move_mini_apptray_objs_add(mini_at_mb);

             // send mini_apptray to "move start message".
             SLOG(LOG_DEBUG, "E17_MOVE_MODULE","[e17:X_CLIENT_MESSAGE:ApptrayShow:ANIMATION_START]");
             e_mod_move_mini_apptray_anim_state_send(mini_at_mb, EINA_TRUE);

             e_mod_move_mini_apptray_e_border_move(mini_at_mb, x, y);
             e_mod_move_mini_apptray_objs_animation_start_position_set(mini_at_mb,
                                                                       angles[0]);
             e_mod_move_mini_apptray_objs_animation_move(mini_at_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Mini_Apptray Show %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
     }
   else if ((!open) && (state)) // Close Case
     {
        if (mb == qp_mb) // Quickpanel case
          {
             switch (angles[0])
               {
                case  90:
                   mx = qp_mb->w * -1; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x; ay = zone->y;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case 180:
                   mx = 0; my = zone->h;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case 270:
                   mx = zone->w; my = 0;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x + qp_mb->w;
                        ay = zone->y + qp_mb->h;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
                case   0:
                default :
                   mx = 0; my = qp_mb->h * -1;
                   if (m->qp_scroll_with_clipping)
                     {
                        ax = zone->x; ay = zone->y;
                     }
                   else
                     {
                        ax = mx; ay = my;
                     }
                   break;
               }
             _e_mod_move_msg_window_show_internal_quickpanel_check(qp_mb);
             e_mod_move_quickpanel_objs_add(qp_mb);

             // send quickpanel to "move start message".
             e_mod_move_quickpanel_anim_state_send(qp_mb, EINA_TRUE);

             e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
             e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                     angles[0],
                                                                     EINA_TRUE);
             e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
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

             // send apptray to "move start message".
             e_mod_move_apptray_anim_state_send(at_mb, EINA_TRUE);

             e_mod_move_apptray_e_border_move(at_mb, x, y);
             SLOG(LOG_DEBUG, "E17_MOVE_MODULE","[e17:X_CLIENT_MESSAGE:ApptrayHide:ANIMATION_START]");
             e_mod_move_apptray_objs_animation_move(at_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Apptray Hide %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
        else if (mb == mini_at_mb) // Mini_Apptray Case
          {
             switch (angles[0])
               {
                case  90:
                   x = zone->w;
                   y = 0;
                   break;
                case 180:
                   x = 0;
                   y = mini_at_mb->h * -1;
                   break;
                case 270:
                   x = mini_at_mb->w * -1;
                   y = 0;
                   break;
                case   0:
                default :
                   x = 0;
                   y = zone->h;
                   break;
               }
             _e_mod_move_msg_window_show_internal_mini_apptray_check(mini_at_mb);
             e_mod_move_mini_apptray_objs_add(mini_at_mb);

             // send apptray to "move start message".
             e_mod_move_mini_apptray_anim_state_send(mini_at_mb, EINA_TRUE);

             e_mod_move_mini_apptray_e_border_move(mini_at_mb, x, y);
             e_mod_move_mini_apptray_objs_animation_move(mini_at_mb, x, y);
             L(LT_EVENT_X,
               "[MOVE] ev:%15.15s Nini_Apptray Hide %s():%d\n",
               "X_CLIENT_MESSAGE", __func__, __LINE__);
          }
     }
   else
     {
        SLOG(LOG_DEBUG, "E17_MOVE_MODULE",
             "[MOVE_MODULE] _NET_WM_WINDOW_SHOW error."
             " w:0x%07x(state:%d) request:%d\n",
             win, state, open);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_qp_state_internal_quickpanel_check(E_Move_Border *mb)
{
   E_Move *m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   if ((!m->elm_indicator_mode) && e_mod_move_indicator_click_get()) // if indicator process event, do event clear
     e_mod_move_indicator_event_clear();
   if ((m->elm_indicator_mode)
       && e_mod_move_indicator_widget_click_get(e_mod_move_indicator_widget_get())) // if indicator widget process event, do event clear
     {
        e_mod_move_indicator_widget_event_clear(e_mod_move_indicator_widget_get());
     }
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
   E_Move_Border *fullscr_mb = NULL;
   Ecore_X_Window win;
   Eina_Bool      state;
   Eina_Bool      comp_obj_visible = EINA_FALSE;
   E_Zone        *zone;
   Eina_Bool      scrolling_state = EINA_FALSE;
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

   if ((fullscr_mb = e_mod_move_util_visible_fullscreen_window_find()))
     {
        E_CHECK_RETURN(e_mod_move_panel_scrollable_get(fullscr_mb, E_MOVE_PANEL_TYPE_QUICKPANEL), EINA_FALSE);
     }

   comp_obj_visible = e_mod_move_util_compositor_object_visible_get(qp_mb);

   if (((!m->elm_indicator_mode) && e_mod_move_indicator_click_get())
       || ((m->elm_indicator_mode)
           && e_mod_move_indicator_widget_click_get(e_mod_move_indicator_widget_get()))
       || (e_mod_move_quickpanel_click_get()))
     {
        scrolling_state = EINA_TRUE;
     }

   zone = qp_mb->bd->zone;
   if (REGION_INSIDE_ZONE(qp_mb, zone)) state = EINA_TRUE;
   else state = EINA_FALSE;

   if ((open) && ((!state) || scrolling_state) && (comp_obj_visible)) // Quickpanel Open
     {
        switch (angles[0])
          {
           case  90:
              mx = 0;
              my = 0;
              if (m->qp_scroll_with_clipping)
                {
                   ax = zone->x + qp_mb->w;
                   ay = zone->y + qp_mb->h;
                }
              else
                {
                   ax = mx; ay = my;
                }
              break;
           case 180:
              mx = 0;
              my = zone->h - qp_mb->h;
              if (m->qp_scroll_with_clipping)
                {
                   ax = zone->x; ay = zone->y;
                }
              else
                {
                   ax = mx; ay = my;
                }
              break;
           case 270:
              mx = zone->w - qp_mb->w;
              my = 0;
              if (m->qp_scroll_with_clipping)
                {
                   ax = zone->x; ay = zone->y;
                }
              else
                {
                   ax = mx; ay = my;
                }
              break;
           case   0:
           default :
              mx = 0;
              my = 0;
              if (m->qp_scroll_with_clipping)
                {
                   ax = zone->x + qp_mb->w;
                   ay = zone->y + qp_mb->h;
                }
              else
                {
                   ax = mx; ay = my;
                }
              break;
          }
        _e_mod_move_msg_qp_state_internal_quickpanel_check(qp_mb);

        if (!m->qp_scroll_with_clipping)
          e_mod_move_quickpanel_dim_show(qp_mb);

        // Set Composite Mode & Rotation Lock & Make below win's mirror object
        e_mod_move_quickpanel_stage_init(qp_mb);

        e_mod_move_quickpanel_objs_add(qp_mb);
        // send quickpanel to "move start message".
        if (!scrolling_state) e_mod_move_quickpanel_anim_state_send(qp_mb, EINA_TRUE);

        e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);
        if (!scrolling_state)
          e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                  angles[0],
                                                                  EINA_FALSE);
        e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
     }
   else if ((!open) && (state || scrolling_state))// Quickpanel Close
     {
        switch (angles[0])
          {
           case  90:
              if (m->qp_scroll_with_clipping)
                {
                   mx = qp_mb->w * -1; my = 0;
                   ax = zone->x; ay = zone->y;
                }
              else
                {
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
                }
              break;
           case 180:
              if (m->qp_scroll_with_clipping)
                {
                   mx = 0; my = zone->h;
                   ax = zone->x + qp_mb->w;
                   ay = zone->y + qp_mb->h;
                }
              else
                {
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
                }
              break;
           case 270:
              if (m->qp_scroll_with_clipping)
                {
                   mx = zone->w; my = 0;
                   ax = zone->x + qp_mb->w;
                   ay = zone->y + qp_mb->h;
                }
              else
                {
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
                }
              break;
           case   0:
           default :
              if (m->qp_scroll_with_clipping)
                {
                   mx = 0; my = qp_mb->h * -1;
                   ax = zone->x; ay = zone->y;
                }
              else
                {
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
                }
              break;
          }
        _e_mod_move_msg_qp_state_internal_quickpanel_check(qp_mb);
        e_mod_move_quickpanel_objs_add(qp_mb);

        // send quickpanel to "move start message".
        if (!scrolling_state) e_mod_move_quickpanel_anim_state_send(qp_mb, EINA_TRUE);

        e_mod_move_quickpanel_e_border_move(qp_mb, mx, my);

        if (!scrolling_state)
          e_mod_move_quickpanel_objs_animation_start_position_set(qp_mb,
                                                                  angles[0],
                                                                  EINA_TRUE);
        e_mod_move_quickpanel_objs_animation_move(qp_mb, ax, ay);
     }
   else
     {
		 SLOG(LOG_DEBUG, "E17_MOVE_MODULE",
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
   E_Zone *zone;
   Ecore_X_Window *wins;
   int i, num;

   E_Move_Canvas *canvas;
   E_Border *bd;

   zone = e_util_zone_current_get(man);
   E_CHECK_RETURN(zone, NULL);

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

   m->indicator_always_region_ratio.portrait = _move_mod->conf->indicator_always_portrait_region_ratio;
   m->indicator_always_region_ratio.landscape = _move_mod->conf->indicator_always_landscape_region_ratio;
   m->indicator_quickpanel_region_ratio.portrait = _move_mod->conf->indicator_quickpanel_portrait_region_ratio;
   m->indicator_quickpanel_region_ratio.landscape = _move_mod->conf->indicator_quickpanel_landscape_region_ratio;
   m->indicator_apptray_region_ratio.portrait = _move_mod->conf->indicator_apptray_portrait_region_ratio;
   m->indicator_apptray_region_ratio.landscape = _move_mod->conf->indicator_apptray_landscape_region_ratio;
   m->qp_scroll_with_visible_win = _move_mod->conf->qp_scroll_with_visible_win;
   m->qp_scroll_with_clipping = _move_mod->conf->qp_scroll_with_clipping;
   m->flick_limit.speed = _move_mod->conf->flick_limit.speed;
   m->flick_limit.angle = _move_mod->conf->flick_limit.angle;
   m->flick_limit.distance = _move_mod->conf->flick_limit.distance;
   m->flick_limit.distance_rate = _move_mod->conf->flick_limit.distance_rate;
   m->animation_duration = _move_mod->conf->animation_duration;
   m->dim_max_opacity = _move_mod->conf->dim_max_opacity;
   m->dim_min_opacity = _move_mod->conf->dim_min_opacity;
   m->elm_indicator_mode = _move_mod->conf->elm_indicator_mode;
   wins = ecore_x_window_children_get(m->man->root, &num);

   // indicator widget gemometry setting
   m->indicator_widget_geometry[E_MOVE_ANGLE_0].x
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_0].x;
   m->indicator_widget_geometry[E_MOVE_ANGLE_0].y
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_0].y;
   m->indicator_widget_geometry[E_MOVE_ANGLE_0].w
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_0].w;
   m->indicator_widget_geometry[E_MOVE_ANGLE_0].h
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_0].h;
   m->indicator_widget_geometry[E_MOVE_ANGLE_90].x
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_90].x;
   m->indicator_widget_geometry[E_MOVE_ANGLE_90].y
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_90].y;
   m->indicator_widget_geometry[E_MOVE_ANGLE_90].w
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_90].w;
   m->indicator_widget_geometry[E_MOVE_ANGLE_90].h
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_90].h;
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].x
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_180].x;
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].y
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_180].y;
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].w
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_180].w;
   m->indicator_widget_geometry[E_MOVE_ANGLE_180].h
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_180].h;
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].x
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_270].x;
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].y
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_270].y;
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].w
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_270].w;
   m->indicator_widget_geometry[E_MOVE_ANGLE_270].h
      = _move_mod->conf->indicator_widget_geometry[E_MOVE_ANGLE_270].h;

   // miniapp_tray widget gemometry setting
   m->apptray_launch_by_flickup = _move_mod->conf->apptray_launch_by_flickup;

   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].x
      = zone->x;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].y
      = zone->h - _move_mod->conf->apptray_widget_size.portrait;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].w
      = zone->w;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_0].h
      = _move_mod->conf->apptray_widget_size.portrait;

   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].x
      = zone->w - _move_mod->conf->apptray_widget_size.landscape;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].y
      = zone->y;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].w
      = _move_mod->conf->apptray_widget_size.landscape;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_90].h
      =  zone->h;

   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].x
      = zone->x;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].y
      = zone->y;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].w
      = zone->w;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_180].h
      = _move_mod->conf->apptray_widget_size.portrait;

   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].x
      = zone->x;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].y
      = zone->y;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].w
      = _move_mod->conf->apptray_widget_size.landscape;
   m->mini_apptray_widget_geometry[E_MOVE_ANGLE_270].h
      = zone->h;

   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             E_Move_Border *mb;
             if ((bd = e_border_find_by_window(wins[i])))
               {
                  mb = _e_mod_move_bd_new_intern(m, bd);
                  if (!mb) continue;

                  _e_mod_move_bd_add_intern(mb);
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
   E_Move_Border *mb;
   E_Move_Canvas *canvas;

   E_CHECK(m);
   while (m->borders)
     {
        mb = (E_Move_Border *)(m->borders);
        _e_mod_move_bd_hide_intern(mb);
        _e_mod_move_bd_del_intern(mb);
     }

   EINA_LIST_FREE(m->canvases, canvas) e_mod_move_canvas_del(canvas);
   m->canvases = NULL;

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

   e_border_hooks = eina_list_append(e_border_hooks, e_border_hook_add(E_BORDER_HOOK_NEW_BORDER,         _e_mod_move_bd_new_hook,       NULL));
   e_border_hooks = eina_list_append(e_border_hooks, e_border_hook_add(E_BORDER_HOOK_DEL_BORDER,         _e_mod_move_bd_del_hook,       NULL));

   e_msg_handler = e_msg_handler_add(_e_mod_move_e_msg_handler, NULL);

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

   E_FREE_LIST(e_border_hooks, e_border_hook_del);

   if (e_msg_handler) e_msg_handler_del(e_msg_handler);

   if (borders) eina_hash_free(borders);
   if (border_clients) eina_hash_free(border_clients);
   borders = NULL;
   border_clients = NULL;

   e_mod_move_border_type_shutdown();
   e_mod_move_atoms_shutdown();

   e_mod_move_util_set(NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////
