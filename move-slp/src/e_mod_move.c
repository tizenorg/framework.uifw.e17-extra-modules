#include "e.h"
#include "e_mod_main.h"
#include "e_mod_move.h"
#include "e_mod_data.h"
#include "config.h"
#include "utilX.h"

#define QP_ANI
#define AT_ANI
#define _WND_REQUEST_ANGLE_IDX 0
#define _WND_CURR_ANGLE_IDX    1
#define MAX_OPACITY            128
#define MIN_OPACITY            0

#define STR_ATOM_CM_QUICKPANEL_LAYOUT_POSITION     "_E_COMP_QUICKPANEL_LAYOUT_POSITION"
#define STR_ATOM_CM_WINDOW_INPUT_REGION            "_E_COMP_WINDOW_INPUT_REGION"
#define STR_ATOM_ILLUME_MINI_CONTROLLER_WINDOW     "_E_ILLUME_MINI_CONTROLLER_WINDOW"
#define STR_ATOM_NET_WM_WINDOW_SHOW                "_NET_WM_WINDOW_SHOW"
#define STR_ATOM_ROTATION_LOCK                     "_E_ROTATION_LOCK"

#ifdef  _MAKE_ATOM
# undef _MAKE_ATOM
#endif

#define _MAKE_ATOM(a, s)                              \
   do {                                               \
        a = ecore_x_atom_get(s);                      \
        if (!a)                                       \
          fprintf(stderr,                             \
                  "[E-move] ##s creation failed.\n"); \
   } while(0)

#define E_CHECK(x)           do {if (!(x)) return;    } while(0)
#define E_CHECK_RETURN(x, r) do {if (!(x)) return (r);} while(0)

typedef struct _Anim_Data
{
   E_Move    *m;
   int        start_x;
   int        start_y;
   int        end_x;
   int        end_y;
   int        dist_x;
   int        dist_y;
   int        start_opacity;
   int        end_opacity;
   int        diff_opacity;
   Eina_Bool  is_on_screen;
   Eina_List *qp_transients;
} Anim_Data;

static Anim_Data qp_anim_data;
static Anim_Data apptray_anim_data;

Ecore_X_Atom      ATOM_QUICKPANEL_LAYOUT_POSITION      = 0;
Ecore_X_Atom      ATOM_WINDOW_INPUT_REGION             = 0;
Ecore_X_Atom      ATOM_MINI_CONTROLLER_WINDOW          = 0;
Ecore_X_Atom      ATOM_NET_WM_WINDOW_SHOW              = 0;
Ecore_X_Atom      ATOM_ROTATION_LOCK                   = 0;

static int        APP_TRAY_CHECK_REGION = 0;
static Evas*      e_move_comp_evas;
static E_Move*    _move = NULL;
static Eina_Hash* windows = NULL;
static Eina_Hash* borders = NULL;
static Eina_Hash* move_borders = NULL;

static void _e_mod_move_comp_evas_get(E_Manager* man);
static Eina_Bool      _e_mod_move_property(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__);
static Eina_Bool      _e_mod_move_message(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__);
static Eina_Bool      _e_mod_move_bd_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_show(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_hide(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_move(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_bd_resize(void *data __UNUSED__, int type __UNUSED__, void *event);
static E_Move_Win*    _e_mod_move_win_find(Ecore_X_Window win);
static E_Move_Win*    _e_mod_move_border_client_find(Ecore_X_Window win);
static E_Move_Border* _e_mod_move_border_find(Ecore_X_Window win);
static void           _e_mod_move_set(E_Move *m);
static E_Move*        _e_mod_move_add(E_Manager *man);
static Evas_Object*   _e_mod_move_comp_window_object_get(E_Manager* man, E_Border *bd);
static Evas_Object*   _e_mod_move_comp_input_window_object_get(E_Manager* man, Ecore_X_Window win);
static Eina_Bool      _e_mod_move_comp_window_input_region_set(E_Manager* man, E_Border *bd, int x, int y, int w, int h);
static Eina_Bool      _e_mod_move_comp_window_move_lock(E_Manager* man, E_Border *bd);
static Eina_Bool      _e_mod_move_comp_window_move_unlock(E_Manager* man, E_Border *bd);
static Eina_Bool      _e_mod_move_win_get_prop_angle(Ecore_X_Window win, int *req, int *curr);
static Eina_Bool      _e_mod_move_quickpanel_transients_find(E_Move *m);
static Eina_Bool      _e_mod_move_quickpanel_transients_release(E_Move *m);
static Eina_Bool      _e_mod_move_quickpanel_transients_remove(E_Move_Win *mw);
static Eina_Bool      _e_mod_move_quickpanel_move(E_Move *m, int quickpanel_move_x, int quickpanel_move_y,
                                                  int quickpanel_child_move_x, int quickpanel_child_move_y,
                                                  QuickPanel_Layout_Position layout_position,
                                                  Eina_Bool window_move);
static Eina_Bool      _e_mod_move_app_tray_move(E_Move *m, int app_tray_move_x, int app_tray_move_y, Eina_Bool window_move);
static Eina_Bool      _e_mod_move_prop_window_rotation(Ecore_X_Event_Window_Property *ev);
static Ecore_X_Window _e_mod_move_win_get_client_xid(E_Move_Win *mw);
static Eina_Bool      _e_mod_move_prop_quickpanel_layout(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_window_input_region(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_set_window_input_region(E_Move_Win *mw);
static Eina_Bool      _e_mod_move_set_quickpanel_layout(E_Move_Win *mw);
static void           _e_mod_move_bd_show_internal(E_Border *bd);
static void           _e_mod_move_win_del(E_Move_Win *mw);
static Eina_Bool      e_mod_move_apptray_helper_mouse_up_callback(void *data, int type __UNUSED__, void *event);
static Eina_Bool      e_mod_move_apptray_helper_mouse_down_callback(void *data, int type __UNUSED__, void *event);
static Eina_Bool      e_mod_move_apptray_helper_mouse_move_callback(void *data, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_prop_active_window_change(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_prop_indicator_state_change(Ecore_X_Event_Window_Property *ev);
static Eina_Bool      _e_mod_move_hw_key_down_cb (void *data, int type __UNUSED__, void *event);
static Eina_Bool      _e_mod_move_quickpanel_move_animation(E_Move *m, int quickpanel_move_x, int quickpanel_move_y, Eina_Bool is_on_screen);
static Eina_Bool      _qp_anim_frame(void *data, double pos);
static Eina_Bool      _e_mod_move_apptray_move_animation(E_Move *m, int apptray_move_x, int apptray_move_y, Eina_Bool is_on_screen);
static Eina_Bool      _apptray_anim_frame(void *data, double pos);
static E_Border*      _e_mod_move_bd_find_by_indicator (E_Move *m);
static Eina_Bool      _e_mod_move_lock_screen_check(E_Move *m);
static Eina_Bool      _e_mod_move_mini_controller_check(E_Move *m);
static Eina_Bool      _animator_move(E_Move_Win *mw, int dst_x, int dst_y, double duration, Eina_Bool after_show);
static Eina_Bool      _e_mod_move_msg_window_show(Ecore_X_Event_Client_Message *ev);
static Eina_Bool      _e_mod_move_msg_qp_state(Ecore_X_Event_Client_Message *ev);
static Eina_Bool      e_mod_move_active_win_state_update(E_Move *m);
static Eina_Bool      _e_mod_move_quickpanel_transients_move_lock(E_Move *m, Eina_Bool move_offscreen);

/* indicator mouse callbacks */
static E_Move_Event_State _indicator_cb_motion_check(E_Move_Event *ev, void *data);
static Eina_Bool          _indicator_cb_motion_start(void *data, void *event_info);
static Eina_Bool          _indicator_cb_motion_move(void *data, void *event_info);
static Eina_Bool          _indicator_cb_motion_end(void *data, void *event_info);

static int                _e_mod_move_prop_indicator_state_get(Ecore_X_Window win);
static Eina_Bool          _e_mod_move_tp_indi_show(E_Move *m, E_Border *target);
static Eina_Bool          _e_mod_move_tp_indi_hide(E_Move *m);
static void               _e_mod_move_tp_indi_touch_win_show(E_Move *m);
static void               _e_mod_move_tp_indi_touch_win_hide(E_Move *m);
static void               _e_mod_move_tp_indi_touch_win_stack_update(E_Move *m);
static Eina_Bool          e_mod_move_tp_indi_touch_win_mouse_up_callback(void *data, int type __UNUSED__, void *event);

E_Move* _e_mod_move_get(void);
void    e_mod_move_win_quickpanel_mouse_down_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_quickpanel_mouse_move_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_quickpanel_mouse_up_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_app_tray_mouse_down_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_app_tray_mouse_move_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_app_tray_mouse_up_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_app_tray_restack_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_quickpanel_mini_del_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_win_del_callback(void *data, Evas *e, Evas_Object *obj, void *event_info);
void    e_mod_move_show_apptray_helper(E_Move *m);
void    e_mod_move_hide_apptray_helper(E_Move *m);
void    _e_mod_move_key_handler_enable(E_Move *m);
void    _e_mod_move_key_handler_disable(E_Move *m);
void    _e_mod_move_illume_quickpanel_state_on(E_Move *m);
void    _e_mod_move_illume_quickpanel_state_off(E_Move *m);
void    _e_mod_move_rotate_block_enable(E_Move *m);
void    _e_mod_move_rotate_block_disable(E_Move *m);
void    _e_mod_move_rotation_lock(E_Move *m);
void    _e_mod_move_rotation_unlock(E_Move *m);

static Eina_Bool
_e_mod_move_quickpanel_transients_find(E_Move *m)
{
   E_Move_Win *find_mw = NULL;
   Eina_List *l = NULL;
   E_Border *child_bd = NULL;

   if (!m) return EINA_FALSE;

   if (m->quickpanel)
     {
        EINA_LIST_FOREACH(m->quickpanel->border->transients, l, child_bd)
          {
             if (!child_bd) continue;

             find_mw = _e_mod_move_win_find(child_bd->win);
             if (find_mw)
               {
                  m->quickpanel_child.quickpanel_transients = eina_list_append(m->quickpanel_child.quickpanel_transients, find_mw );
                  find_mw = NULL;
               }
          }
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_transients_release(E_Move *m)
{
   if (!m) return EINA_FALSE;

   m->quickpanel_child.quickpanel_transients = eina_list_free(m->quickpanel_child.quickpanel_transients);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_move(E_Move *m, int quickpanel_move_x, int quickpanel_move_y,
                                       int quickpanel_child_move_x, int quickpanel_child_move_y,
                                       QuickPanel_Layout_Position layout_position,
                                       Eina_Bool window_move)
{
   Eina_List *l = NULL;
   E_Move_Win *find_mw = NULL;

   if (!m) return EINA_FALSE;
   if (!m->quickpanel) return EINA_FALSE;

   if (window_move)
     {
        evas_object_move(m->quickpanel->move_object, quickpanel_move_x, quickpanel_move_y);
        e_border_move(m->quickpanel->border, quickpanel_move_x, quickpanel_move_y);
     }
   else evas_object_move(m->quickpanel->move_object, quickpanel_move_x, quickpanel_move_y);

   if (m->quickpanel_child.quickpanel_transients)
     {
        EINA_LIST_FOREACH(m->quickpanel_child.quickpanel_transients, l, find_mw)
          {
             if (find_mw)
               {
                 if (layout_position == LAYOUT_POSITION_X)
                   {
                      if (window_move)
                        {
                           evas_object_move(find_mw->move_object, find_mw->quickpanel_layout.position_x + quickpanel_child_move_x , quickpanel_child_move_y);
                           e_border_move(find_mw->border, find_mw->quickpanel_layout.position_x + quickpanel_child_move_x , quickpanel_child_move_y);
                        }
                      else evas_object_move(find_mw->move_object, find_mw->quickpanel_layout.position_x + quickpanel_child_move_x , quickpanel_child_move_y);
                   }
                 else
                   {
                      if (window_move)
                        {
                           evas_object_move(find_mw->move_object, quickpanel_child_move_x, find_mw->quickpanel_layout.position_y + quickpanel_child_move_y);
                           e_border_move(find_mw->border, quickpanel_child_move_x, find_mw->quickpanel_layout.position_y + quickpanel_child_move_y);
                        }
                      else evas_object_move(find_mw->move_object, quickpanel_child_move_x, find_mw->quickpanel_layout.position_y + quickpanel_child_move_y);
                   }
               }
          }
     }

   if (m->active_win.animatable)
     {
        int move_x = 0;
        int move_y = 0;
        if (layout_position == LAYOUT_POSITION_X)
          {
             if (m->quickpanel->event_handler.window_rotation_angle == 90)
               move_x = quickpanel_move_x + m->quickpanel->border->w;
             else
                move_x = quickpanel_move_x - m->quickpanel->border->w;

             move_y = quickpanel_move_y;
          }
        else
          {
             move_x = quickpanel_move_x;

             if (m->quickpanel->event_handler.window_rotation_angle == 0)
                move_y = quickpanel_move_y + m->quickpanel->border->h;
             else
                move_y = quickpanel_move_y - m->quickpanel->border->h;
          }
        evas_object_move(m->active_win.obj, move_x , move_y);
     }

   if (m->block_helper)
     {
        int opacity = MAX_OPACITY;
        int move_x = 0;
        int move_y = 0;
        int angle = 0;

        angle = m->quickpanel->event_handler.window_rotation_angle;
        angle %= 360;

        switch (angle)
          {
           case  90:
             move_x = quickpanel_move_x + m->quickpanel->border->w;
             opacity = MAX_OPACITY * move_x / m->quickpanel->border->w;
             break;
           case 180:
             move_y = m->man->h - quickpanel_move_y;
             opacity = MAX_OPACITY * move_y / m->quickpanel->border->h;
             break;
           case 270:
             move_x = m->man->w - quickpanel_move_x;
             opacity = MAX_OPACITY * move_x / m->quickpanel->border->w;
             break;
           case   0:
           default :
             move_y = quickpanel_move_y + m->quickpanel->border->h;
             opacity = MAX_OPACITY * move_y / m->quickpanel->border->h;
             break;
          }
        evas_object_color_set(m->block_helper->obj, 0, 0, 0, opacity);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_app_tray_move(E_Move *m, int app_tray_move_x, int app_tray_move_y, Eina_Bool window_move)
{
   if (!m) return EINA_FALSE;
   if (!m->app_tray) return EINA_FALSE;

   if (window_move)
     {
        evas_object_move(m->app_tray->move_object, app_tray_move_x, app_tray_move_y);
        e_border_move(m->app_tray->border, app_tray_move_x, app_tray_move_y);
     }
   else evas_object_move(m->app_tray->move_object, app_tray_move_x, app_tray_move_y);

   if (m->apptray_helper)
     {
        int opacity = MAX_OPACITY;
        int move_x = 0;
        int move_y = 0;
        int angle = 0;

        angle = m->app_tray->event_handler.window_rotation_angle;
        angle %= 360;

        switch (angle)
          {
           case  90:
             move_x = app_tray_move_x + m->app_tray->border->w;
             opacity = MAX_OPACITY * move_x / m->app_tray->border->w;
             break;
           case 180:
             move_y = m->man->h - app_tray_move_y;
             opacity = MAX_OPACITY * move_y / m->app_tray->border->h;
             break;
           case 270:
             move_x = m->man->w - app_tray_move_x;
             opacity = MAX_OPACITY * move_x / m->app_tray->border->w;
             break;
           case   0:
           default :
             move_y = app_tray_move_y  + m->app_tray->border->h;
             opacity = MAX_OPACITY * move_y / m->app_tray->border->h;
             break;
          }
       evas_object_color_set(m->apptray_helper->obj, 0, 0, 0, opacity);
     }

   return EINA_TRUE;
}

static Evas_Object *
_e_mod_move_comp_window_object_get(E_Manager* man, E_Border *bd)
{
   E_Manager_Comp_Source *comp_data = NULL;
   if (!man) return NULL;
   if (!bd) return NULL;

   if (man->comp)
     {
        comp_data = e_manager_comp_src_get(man, bd->win);
        if (!comp_data) return NULL;
        return e_manager_comp_src_shadow_get(man, comp_data);
     }
   return NULL;
}

static Evas_Object *
_e_mod_move_comp_input_window_object_get(E_Manager* man, Ecore_X_Window win)
{
   E_Manager_Comp_Source *comp_data = NULL;
   if (!man) return NULL;

   if (man->comp)
     {
        comp_data = e_manager_comp_src_get(man, win);
        if (!comp_data) return NULL;
        return e_manager_comp_src_shadow_get(man, comp_data);
     }
   return NULL;
}

static Eina_Bool
_e_mod_move_comp_window_input_region_set(E_Manager* man, E_Border *bd, int x, int y, int w, int h)
{
   E_Manager_Comp_Source *comp_data = NULL;
   if (!man) return EINA_FALSE;
   if (!bd) return EINA_FALSE;

   if (man->comp)
     {
        comp_data = e_manager_comp_src_get(man, bd->win);
        if (!comp_data) return EINA_FALSE;
        return e_manager_comp_src_input_region_set(man, comp_data, x, y, w, h);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_mod_move_comp_window_move_lock(E_Manager* man, E_Border *bd)
{
   E_Manager_Comp_Source *comp_data = NULL;
   if (!man) return EINA_FALSE;
   if (!bd) return EINA_FALSE;

   if (man->comp)
     {
        comp_data = e_manager_comp_src_get(man, bd->win);
        if (!comp_data) return EINA_FALSE;
        return e_manager_comp_src_move_lock(man, comp_data);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_mod_move_comp_window_move_unlock(E_Manager* man, E_Border *bd)
{
   E_Manager_Comp_Source *comp_data = NULL;
   if (!man) return EINA_FALSE;
   if (!bd) return EINA_FALSE;

   if (man->comp)
     {
        comp_data = e_manager_comp_src_get(man, bd->win);
        if (!comp_data) return EINA_FALSE;
        return e_manager_comp_src_move_unlock(man, comp_data);
     }
   return EINA_FALSE;
}

static E_Move *
_e_mod_move_add(E_Manager *man)
{
   E_Move *m;
   m = calloc(1, sizeof(E_Move));
   if (!m) return NULL;

   m->man = man;

   return m;
}

E_Move *
_e_mod_move_get(void)
{
   return _move;
}

static void
_e_mod_move_set(E_Move *m)
{
   if (_move)
     {
        fprintf(stderr,
                "[E17-move] %s(%d) E_Move setup failed.\n",
                __func__, __LINE__);
        return;
     }
   _move = m;
}

static E_Move_Win *
_e_mod_move_win_find(Ecore_X_Window win)
{
   return eina_hash_find(windows, e_util_winid_str_get(win));
}

static E_Move_Win *
_e_mod_move_border_client_find(Ecore_X_Window win)
{
   return eina_hash_find(borders, e_util_winid_str_get(win));
}

static E_Move_Border*
_e_mod_move_border_find(Ecore_X_Window win)
{
   return eina_hash_find(move_borders, e_util_winid_str_get(win));
}

static Eina_Bool
_e_mod_move_bd_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev = event;
   E_Border *bd = NULL;
   E_Move_Border *m_bd = NULL;
   E_Move_Win_Type win_type = WIN_NORMAL;
   E_Move *m = _e_mod_move_get();

   bd = e_border_find_by_window(ev->border->win);
   if ((win_type = e_mod_move_win_type_get(bd)) == WIN_NORMAL)
     {
       return ECORE_CALLBACK_PASS_ON;
     }

   m_bd = calloc(1, sizeof(E_Move_Border));
   if (!m_bd)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   m_bd->bd = bd;
   m_bd->win_type = win_type;

   m->borders = eina_inlist_append(m->borders, EINA_INLIST_GET(m_bd));
   eina_hash_add(move_borders, e_util_winid_str_get(bd->win), m_bd);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Remove *ev = event;
   E_Move *m = _e_mod_move_get();
   E_Move_Border *m_bd = NULL;
   m_bd = _e_mod_move_border_find(ev->border->win);
   if (!m_bd) return ECORE_CALLBACK_PASS_ON;

   m->borders = eina_inlist_remove(m->borders, EINA_INLIST_GET(m_bd));
   eina_hash_del(move_borders, e_util_winid_str_get(m_bd->bd->win), m_bd);
   free(m_bd);

   return ECORE_CALLBACK_PASS_ON;
}

void
_e_mod_move_bd_show_internal(E_Border *bd)
{
   int req_angle = -1;
   int cur_angle = -1;
   E_Move_Win *mw = NULL;

   E_Move_Win_Type win_type = WIN_NORMAL;
   E_Move *m = _e_mod_move_get();

   if (!bd)
     {
        return;
     }

   if (!m)
     {
        return;
     }

   if ((win_type = e_mod_move_win_type_get(bd)) == WIN_NORMAL)
     {
        return;
     }

   mw = calloc(1, sizeof(E_Move_Win));
   if (!mw)
     {
        return;
     }

   mw->m = m;
   mw->border = bd;
   mw->win_type = win_type;
   mw->move_object = _e_mod_move_comp_window_object_get(m->man, bd);

   if (!(mw->move_object))
     {
        free(mw);
        return;
     }

   if (mw->win_type == WIN_INDICATOR)
     {
        mw->event = e_mod_move_event_new(mw->border->client.win,
                                         mw->move_object);
        if (!mw->event)
          {
             E_FREE(mw);
             return;
          }

        e_mod_move_event_angle_cb_set
          (mw->event, _e_mod_move_win_get_prop_angle);

        e_mod_move_event_check_cb_set
          (mw->event, _indicator_cb_motion_check, mw);

        e_mod_move_event_cb_set(mw->event, E_MOVE_EVENT_TYPE_MOTION_START,
                                _indicator_cb_motion_start, mw);
        e_mod_move_event_cb_set(mw->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                                _indicator_cb_motion_move, mw);
        e_mod_move_event_cb_set(mw->event, E_MOVE_EVENT_TYPE_MOTION_END,
                                _indicator_cb_motion_end, mw);
     }
   if (mw->win_type == WIN_QUICKPANEL_BASE)
     {
        mw->event_handler.win_mouse_down_cb = e_mod_move_win_quickpanel_mouse_down_callback;
        mw->event_handler.win_mouse_move_cb = e_mod_move_win_quickpanel_mouse_move_callback;
        mw->event_handler.win_mouse_up_cb = e_mod_move_win_quickpanel_mouse_up_callback;
     }
   if (mw->win_type == WIN_APP_TRAY)
     {
        mw->event_handler.win_mouse_down_cb = e_mod_move_win_app_tray_mouse_down_callback;
        mw->event_handler.win_mouse_move_cb = e_mod_move_win_app_tray_mouse_move_callback;
        mw->event_handler.win_mouse_up_cb = e_mod_move_win_app_tray_mouse_up_callback;
        mw->event_handler.obj_restack_cb = e_mod_move_win_app_tray_restack_callback;
     }
   if (mw->win_type == WIN_QUICKPANEL_MINICONTROLLER)
     mw->event_handler.obj_del_cb = e_mod_move_win_quickpanel_mini_del_callback;
   else
     mw->event_handler.obj_del_cb = e_mod_move_win_del_callback;

   evas_object_data_set(mw->move_object, "move", mw);

   if (mw->event_handler.win_mouse_down_cb)
     {
        evas_object_event_callback_add( mw->move_object, EVAS_CALLBACK_MOUSE_DOWN, mw->event_handler.win_mouse_down_cb, NULL);
        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE) fprintf(stderr, "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",__func__, __LINE__ );
     }
   if (mw->event_handler.win_mouse_up_cb)
     {
        evas_object_event_callback_add( mw->move_object, EVAS_CALLBACK_MOUSE_UP, mw->event_handler.win_mouse_up_cb , NULL);
        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE) fprintf(stderr, "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",__func__, __LINE__ );
     }
   if (mw->event_handler.win_mouse_move_cb)
     {
        evas_object_event_callback_add( mw->move_object, EVAS_CALLBACK_MOUSE_MOVE, mw->event_handler.win_mouse_move_cb, NULL);
        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE) fprintf(stderr, "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",__func__, __LINE__ );
     }
   if (mw->event_handler.obj_restack_cb)
     {
        evas_object_event_callback_add( mw->move_object, EVAS_CALLBACK_RESTACK, mw->event_handler.obj_restack_cb , NULL);
        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE) fprintf(stderr, "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",__func__, __LINE__ );
     }
   if (mw->event_handler.obj_del_cb)
     {
        evas_object_event_callback_add( mw->move_object, EVAS_CALLBACK_DEL, mw->event_handler.obj_del_cb , NULL);
        if (evas_alloc_error() != EVAS_ALLOC_ERROR_NONE) fprintf(stderr, "[MOVE] ERROR: Callback registering failed! Aborting. %s():%d\n",__func__, __LINE__ );
     }

   mw->window_geometry.x = bd->x;
   mw->window_geometry.y = bd->y;
   mw->window_geometry.w = bd->w;
   mw->window_geometry.h = bd->h;

   // set input_region
   _e_mod_move_set_window_input_region(mw);

  _e_mod_move_comp_window_input_region_set(mw->m->man, mw->border, mw->shape_input_region.input_region_x,
                                                                   mw->shape_input_region.input_region_y,
                                                                   mw->shape_input_region.input_region_w,
                                                                   mw->shape_input_region.input_region_h);

   if (mw->win_type == WIN_QUICKPANEL_MINICONTROLLER)
     {
        _e_mod_move_set_quickpanel_layout(mw);
   }


   if (_e_mod_move_win_get_prop_angle(_e_mod_move_win_get_client_xid(mw),
                                      &req_angle,
                                      &cur_angle))
     {
        mw->event_handler.window_rotation_angle = req_angle;
     }

   if (mw->win_type == WIN_INDICATOR) m->indicator = mw;
   if (mw->win_type == WIN_QUICKPANEL_BASE) m->quickpanel = mw;
   if (mw->win_type == WIN_APP_TRAY) m->app_tray = mw;

   mw->state = MOVE_OFF_STAGE;


   if (mw->win_type == WIN_INDICATOR)
     {
        _e_mod_move_comp_window_input_region_set(mw->m->man, mw->border,
                                                 0,
                                                 0,
                                                 mw->window_geometry.w,
                                                 mw->window_geometry.h);
     }

   if (mw->win_type == WIN_APP_TRAY)
     e_border_move(mw->border, -10000, -10000);

   m->wins = eina_inlist_append(m->wins, EINA_INLIST_GET(mw));
   eina_hash_add(borders, e_util_winid_str_get(mw->border->client.win), mw);
   eina_hash_add(windows, e_util_winid_str_get(mw->border->win), mw);
}

static void
_e_mod_move_win_del(E_Move_Win *mw)
{
   if (!mw) return;

   Eina_List *l;
   E_Move_Win *_mw;
   EINA_LIST_FOREACH(qp_anim_data.qp_transients, l, _mw)
     {
        if (!_mw) continue;
        if (mw == _mw)
          {
             qp_anim_data.qp_transients = eina_list_remove(qp_anim_data.qp_transients, mw);
          }
     }

   if ((mw->win_type == WIN_INDICATOR) &&
       (mw->state == MOVE_WORK))
     {
        if ((mw->m->quickpanel) &&
            (mw->m->quickpanel->state == MOVE_WORK))
          {
            int dest_x = 0, dest_y = 0;
            switch (mw->m->quickpanel->event_handler.window_rotation_angle)
              {
               case  90:
                 dest_x = -(mw->m->quickpanel->window_geometry.w);
                 break;
               case 180:
                 dest_y = mw->m->quickpanel->window_geometry.h;
                 break;
               case 270:
                 dest_x = mw->m->quickpanel->window_geometry.w;
                 break;
               case   0:
               default :
                 dest_y = -(mw->m->quickpanel->window_geometry.h);
                 break;
              }
            _e_mod_move_quickpanel_move_animation(mw->m, dest_x, dest_y, EINA_FALSE);
            mw->m->quickpanel->state = MOVE_OFF_STAGE;
            _e_mod_move_illume_quickpanel_state_off(mw->m);
            _e_mod_move_key_handler_disable(mw->m);
          }

        if((mw->m->app_tray) &&
           (mw->m->app_tray->state == MOVE_WORK))
          {
            int dest_x = 0, dest_y = 0;
            switch (mw->m->app_tray->event_handler.window_rotation_angle)
              {
               case  90:
                 dest_x = -(mw->m->app_tray->window_geometry.w);
                 break;
               case 180:
                 dest_y = mw->m->man->h;
                 break;
               case 270:
                 dest_x = mw->m->man->w;
                 break;
               case   0:
               default :
                 dest_y = -(mw->m->app_tray->window_geometry.h);
                 break;
              }
            _e_mod_move_apptray_move_animation(mw->m, dest_x, dest_y, EINA_FALSE);
            mw->m->app_tray->state = MOVE_OFF_STAGE;
            _e_mod_move_key_handler_disable(mw->m);
          }
     }

   if (mw->event)
     {
        e_mod_move_event_free(mw->event);
        mw->event = NULL;
     }

   evas_object_data_del(mw->move_object,"move");

   if (mw->event_handler.win_mouse_down_cb)
     evas_object_event_callback_del(mw->move_object,
                                    EVAS_CALLBACK_MOUSE_DOWN,
                                    mw->event_handler.win_mouse_down_cb);

   if (mw->event_handler.win_mouse_up_cb)
     evas_object_event_callback_del(mw->move_object,
                                    EVAS_CALLBACK_MOUSE_UP,
                                    mw->event_handler.win_mouse_up_cb);

   if (mw->event_handler.win_mouse_move_cb)
     evas_object_event_callback_del(mw->move_object,
                                    EVAS_CALLBACK_MOUSE_MOVE,
                                    mw->event_handler.win_mouse_move_cb);

   if (mw->event_handler.obj_restack_cb)
     evas_object_event_callback_del(mw->move_object,
                                    EVAS_CALLBACK_RESTACK,
                                    mw->event_handler.obj_restack_cb);

   if (mw->event_handler.obj_del_cb)
     evas_object_event_callback_del(mw->move_object,
                                    EVAS_CALLBACK_DEL,
                                    mw->event_handler.obj_del_cb);

   if (mw->win_type == WIN_INDICATOR)
     {
        _e_mod_move_tp_indi_touch_win_hide(mw->m);
        mw->m->indicator = NULL;
     }

   if (mw->win_type == WIN_QUICKPANEL_BASE)
     {
        if (mw->m->quickpanel->state == MOVE_ON_STAGE)
          _e_mod_move_illume_quickpanel_state_off(mw->m);
        mw->m->quickpanel->state = MOVE_OFF_STAGE;
        _e_mod_move_key_handler_disable(mw->m);
        _e_mod_move_rotate_block_disable(mw->m);
        mw->m->quickpanel = NULL;
     }

   if (mw->win_type == WIN_APP_TRAY)
     {
        mw->m->app_tray->state = MOVE_OFF_STAGE;
        e_mod_move_hide_apptray_helper(mw->m);
        _e_mod_move_key_handler_disable(mw->m);
        mw->m->app_tray = NULL;
     }

   if (mw->win_type == WIN_QUICKPANEL_MINICONTROLLER)
     {
        _e_mod_move_quickpanel_transients_remove(mw);
     }

   eina_hash_del(borders, e_util_winid_str_get(mw->border->client.win), mw);
   eina_hash_del(windows, e_util_winid_str_get(mw->border->win), mw);
   mw->m->wins = eina_inlist_remove(mw->m->wins, EINA_INLIST_GET(mw));
   free(mw);
}

static Eina_Bool
_e_mod_move_bd_show(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Show *ev = event;
   E_Border *bd = e_border_find_by_window(ev->border->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;
   _e_mod_move_bd_show_internal(bd);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_hide(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Move_Win *mw = NULL;
   E_Event_Border_Hide *ev = event;
   mw = _e_mod_move_win_find(ev->border->win);
   if (!mw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_move_win_del(mw);
   mw = NULL;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_move(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Move *ev = event;
   E_Border *bd = NULL;
   E_Move_Win *mw = NULL;
   E_Move *m = NULL;
   m = _e_mod_move_get();
   if (!m) return ECORE_CALLBACK_PASS_ON;

   if (m->tp_indi)
     {
        E_Border *indi = _e_mod_move_bd_find_by_indicator(m);
        if (indi)
          {
             evas_object_move(m->tp_indi->obj, indi->x, indi->y);
             evas_object_resize(m->tp_indi->obj, indi->w, indi->h);
             m->tp_indi->input_shape.x = indi->x;
             m->tp_indi->input_shape.y = indi->y;
             m->tp_indi->input_shape.w = indi->w;
             m->tp_indi->input_shape.h = indi->h;
             _e_mod_move_comp_window_input_region_set(m->man, m->tp_indi->target_bd,
                                                      m->tp_indi->input_shape.x,
                                                      m->tp_indi->input_shape.y,
                                                      m->tp_indi->input_shape.w,
                                                      m->tp_indi->input_shape.h);
          }
        else
          _e_mod_move_tp_indi_hide(m);
     }

   bd = e_border_find_by_window(ev->border->win);
   if (!bd)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   mw = _e_mod_move_win_find(ev->border->win);
   if (!mw)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   mw->window_geometry.x = bd->x;
   mw->window_geometry.y = bd->y;

   if (mw->win_type == WIN_INDICATOR)
     {
         _e_mod_move_comp_window_input_region_set(mw->m->man, mw->border,
                                                  0,
                                                  0,
                                                  mw->window_geometry.w,
                                                  mw->window_geometry.h);
     }

   if ((mw->move_lock == EINA_TRUE)
       && (mw->can_unlock == EINA_TRUE))
     {
        mw->move_lock = EINA_FALSE;
        mw->can_unlock = EINA_FALSE;
        _e_mod_move_comp_window_move_unlock(m->man, mw->border);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_bd_resize(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Resize *ev = event;

   E_Border *bd = NULL;
   E_Move_Win *mw = NULL;
   E_Move *m = NULL;
   m = _e_mod_move_get();
   if (!m) return ECORE_CALLBACK_PASS_ON;
   if (m->tp_indi)
     {
        E_Border *indi = _e_mod_move_bd_find_by_indicator(m);
        if (indi)
          {
             evas_object_move(m->tp_indi->obj, indi->x, indi->y);
             evas_object_resize(m->tp_indi->obj, indi->w, indi->h);
             m->tp_indi->input_shape.x = indi->x;
             m->tp_indi->input_shape.y = indi->y;
             m->tp_indi->input_shape.w = indi->w;
             m->tp_indi->input_shape.h = indi->h;
             _e_mod_move_comp_window_input_region_set(m->man, m->tp_indi->target_bd,
                                                      m->tp_indi->input_shape.x,
                                                      m->tp_indi->input_shape.y,
                                                      m->tp_indi->input_shape.w,
                                                      m->tp_indi->input_shape.h);
          }
        else
          _e_mod_move_tp_indi_hide(m);
     }

   bd = e_border_find_by_window(ev->border->win);
   if (!bd)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   mw = _e_mod_move_win_find(ev->border->win);
   if (!mw)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   mw->window_geometry.w = bd->w;
   mw->window_geometry.h = bd->h;

   if (mw->win_type == WIN_INDICATOR)
     {
         _e_mod_move_comp_window_input_region_set(mw->m->man, mw->border,
                                                  0,
                                                  0,
                                                  mw->window_geometry.w,
                                                  mw->window_geometry.h);
     }

   return ECORE_CALLBACK_PASS_ON;
}

Eina_Bool
e_mod_move_init(void)
{
   Eina_List *l = NULL;
   E_Manager *man = NULL;
   E_Move *m = NULL;
   Ecore_X_Window *wins = NULL;
   int i = 0, num = 0;

   if (!e_module_find("comp-slp"))
     {
        printf("[MOVE_MODULE] Error. There is no compositor. Enable 'comp-slp' module first!!!\n");
        return EINA_FALSE;
     }

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
       _e_mod_move_comp_evas_get(man);
       m = _e_mod_move_add(man);
       if (m) _e_mod_move_set(m);
     }

   if (!e_move_comp_evas)
     {
       printf("[MOVE_MODULE] Error. There is no evas of compositor. \n");
       return EINA_FALSE;
     }

   m = _e_mod_move_get();
   if (!m)
     {
        printf("[MOVE_MODULE] Error. There is no E_Move. \n");
        return EINA_FALSE;
     }

   APP_TRAY_CHECK_REGION = (m->man->w)/4;

   windows = eina_hash_string_superfast_new(NULL);
   borders = eina_hash_string_superfast_new(NULL);
   move_borders = eina_hash_string_superfast_new(NULL);

   ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,  _e_mod_move_property,         NULL);
   ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,   _e_mod_move_message,          NULL);
   ecore_event_handler_add(E_EVENT_BORDER_ADD,             _e_mod_move_bd_add,           NULL);
   ecore_event_handler_add(E_EVENT_BORDER_REMOVE,          _e_mod_move_bd_del,           NULL);
   ecore_event_handler_add(E_EVENT_BORDER_SHOW,            _e_mod_move_bd_show,          NULL);
   ecore_event_handler_add(E_EVENT_BORDER_HIDE,            _e_mod_move_bd_hide,          NULL);
   ecore_event_handler_add(E_EVENT_BORDER_MOVE,            _e_mod_move_bd_move,          NULL);
   ecore_event_handler_add(E_EVENT_BORDER_RESIZE,          _e_mod_move_bd_resize,        NULL);

   _MAKE_ATOM(ATOM_QUICKPANEL_LAYOUT_POSITION,       STR_ATOM_CM_QUICKPANEL_LAYOUT_POSITION    );
   _MAKE_ATOM(ATOM_WINDOW_INPUT_REGION,              STR_ATOM_CM_WINDOW_INPUT_REGION           );
   _MAKE_ATOM(ATOM_MINI_CONTROLLER_WINDOW,           STR_ATOM_ILLUME_MINI_CONTROLLER_WINDOW    );
   _MAKE_ATOM(ATOM_NET_WM_WINDOW_SHOW,               STR_ATOM_NET_WM_WINDOW_SHOW               );
   _MAKE_ATOM(ATOM_ROTATION_LOCK,                    STR_ATOM_ROTATION_LOCK );

   wins = ecore_x_window_children_get(m->man->root, &num);

   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             E_Border *bd;
             if ((bd = e_border_find_by_window(wins[i])))
               {
                  if (bd->visible)
                    {
                       _e_mod_move_bd_show_internal(bd);
                    }
               }
          }
        free(wins);
      }

   //load active window
   ecore_x_window_prop_window_get(m->man->root, ECORE_X_ATOM_NET_ACTIVE_WINDOW, &(m->active_win.win), 1);

   return EINA_TRUE;
}

void
e_mod_move_shutdown(void)
{
    if (windows) eina_hash_free(windows);
    if (borders) eina_hash_free(borders);
    if (move_borders) eina_hash_free(move_borders);
}

static void
_e_mod_move_comp_evas_get(E_Manager* man)
{
   if (!man) return;
   if (e_move_comp_evas) return;

   if (man->comp && man->comp->func.evas_get)
     {
        e_move_comp_evas = man->comp->func.evas_get (man->comp->data, man);
     }
}

static Eina_Bool
_e_mod_move_win_get_prop_angle(Ecore_X_Window win,
                               int *req,
                               int *curr)
{
   Eina_Bool res = EINA_FALSE;
   int ret, count;
   int angle[2] = {-1, -1};
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get(win,
                                          ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                          ECORE_X_ATOM_CARDINAL,
                                          32,
                                          &prop_data,
                                          &count);
   if (ret <= 0)
     {
        if (prop_data) free(prop_data);
        return res;
     }
   if (ret && prop_data)
     {
        memcpy (&angle, prop_data, sizeof (int)*count);
        if (count == 2) res = EINA_TRUE;
     }

   if (prop_data) free(prop_data);

   *req  = angle[_WND_REQUEST_ANGLE_IDX];
   *curr = angle[_WND_CURR_ANGLE_IDX];

   if (angle[0] == -1 && angle[1] == -1) res = EINA_FALSE;

   return res;
}

/* indicator mouse motion callbacks */
static E_Move_Event_State
_indicator_cb_motion_check(E_Move_Event *ev,
                           void         *data)
{
   E_Move_Win *mw = (E_Move_Win *)data;
   Eina_List *l, *ll;
   E_Move_Event_Motion_Info *m0, *m1;
   unsigned int cnt = 0;
   E_Move_Event_State res = E_MOVE_EVENT_STATE_PASS;
   int w, h, angle, min_len, min_cnt = 2;

   if (!ev || !mw) goto finish;

   res = e_mod_move_event_state_get(ev);
   angle = e_mod_move_event_angle_get(ev);

   l = e_mod_move_event_ev_queue_get(ev);
   if (!l) goto finish;

   cnt = eina_list_count(l);
   if (cnt < min_cnt) goto finish;

   m0 = eina_list_data_get(l);
   if (!m0) goto finish;

   ll = eina_list_last(l);
   if (!ll) goto finish;

   m1 = eina_list_data_get(ll);
   if (!m1) goto finish;

   if ((m1->cb_type == EVAS_CALLBACK_MOUSE_UP) &&
       (res == E_MOVE_EVENT_STATE_CHECK))
     {
        return E_MOVE_EVENT_STATE_PASS;
     }

   w = abs(m1->coord.x - m0->coord.x);
   h = abs(m1->coord.y - m0->coord.y);
   min_len =
     (mw->window_geometry.w > mw->window_geometry.h) ?
     (mw->window_geometry.h * 2) :
     (mw->window_geometry.w * 2);

   if ((w < min_len) && (h < min_len))
     {
        goto finish;
     }

   switch (angle)
     {
      case  90:
      case 270:
        if (w < h) res = E_MOVE_EVENT_STATE_PASS;
        else res = E_MOVE_EVENT_STATE_HOLD;
        break;
      case 180:
      case   0:
      default :
        if (w > h) res = E_MOVE_EVENT_STATE_PASS;
        else res = E_MOVE_EVENT_STATE_HOLD;
        break;
     }

finish:
   return res;
}

static Eina_Bool
_indicator_cb_motion_start(void *data,
                           void *event_info)
{
   E_Move_Win *mw = (E_Move_Win *)data;
   E_Move_Event_Motion_Info *info;
   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mw || !info) return EINA_FALSE;

   Eina_Bool qp_off_stage = EINA_TRUE;
   Eina_Bool at_off_stage = EINA_TRUE;
   Eina_Bool lock_scrren_enable = EINA_FALSE;
   Eina_Bool mini_control_enable = EINA_FALSE;
   Eina_Bool qp_lock_screen_check = EINA_FALSE;

   mw->event_handler.mouse_clicked = EINA_TRUE;
   mw->state=MOVE_WORK;

   mw->event_handler.window_rotation_angle = e_mod_move_event_angle_get(mw->event);

   if (mw->m->quickpanel)
     if (mw->m->quickpanel->state != MOVE_OFF_STAGE) qp_off_stage = EINA_FALSE;
   if (mw->m->app_tray)
     if (mw->m->app_tray->state != MOVE_OFF_STAGE) at_off_stage = EINA_FALSE;

   lock_scrren_enable = _e_mod_move_lock_screen_check(mw->m);
   mini_control_enable = _e_mod_move_mini_controller_check(mw->m);

   if (lock_scrren_enable)
     if (mini_control_enable) qp_lock_screen_check = EINA_TRUE;
     else qp_lock_screen_check = EINA_FALSE;
   else qp_lock_screen_check = EINA_TRUE;


   if (mw->event_handler.window_rotation_angle == 0)
     {
        if (info->coord.x > APP_TRAY_CHECK_REGION)
          {
             if (mw->m->quickpanel)
               if ((mw->m->quickpanel->state == MOVE_OFF_STAGE) && at_off_stage
                   && qp_lock_screen_check)
                 {
                    mw->m->quickpanel->state=MOVE_WORK;
                    _e_mod_move_quickpanel_transients_find(mw->m);
                    _e_mod_move_quickpanel_transients_move_lock(mw->m, EINA_TRUE);
                    _e_mod_move_rotate_block_enable(mw->m);
                    e_mod_move_active_win_state_update(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
        else
          {
             if (mw->m->app_tray)
               if ((mw->m->app_tray->state == MOVE_OFF_STAGE) && qp_off_stage
                   && !lock_scrren_enable)
                 {
                    mw->m->app_tray->state=MOVE_WORK;
                    e_border_raise(mw->m->app_tray->border);
                    e_mod_move_show_apptray_helper(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
     }
   else if (mw->event_handler.window_rotation_angle == 90)
     {
        if (info->coord.y < mw->m->indicator->window_geometry.h - APP_TRAY_CHECK_REGION)
          {
             if (mw->m->quickpanel)
               if ((mw->m->quickpanel->state == MOVE_OFF_STAGE) && at_off_stage
                   && qp_lock_screen_check)
                 {
                    mw->m->quickpanel->state=MOVE_WORK;
                    _e_mod_move_quickpanel_transients_find(mw->m);
                    _e_mod_move_quickpanel_transients_move_lock(mw->m, EINA_TRUE);
                    _e_mod_move_rotate_block_enable(mw->m);
                    e_mod_move_active_win_state_update(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
        else
          {
             if (mw->m->app_tray)
               if ((mw->m->app_tray->state == MOVE_OFF_STAGE) && qp_off_stage
                   && !lock_scrren_enable)
                 {
                    mw->m->app_tray->state=MOVE_WORK;
                    e_border_raise(mw->m->app_tray->border);
                    e_mod_move_show_apptray_helper(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
     }
   else if (mw->event_handler.window_rotation_angle == 180)
     {
        if (info->coord.x < mw->m->indicator->window_geometry.w - APP_TRAY_CHECK_REGION)
          {
             if (mw->m->quickpanel)
               if ((mw->m->quickpanel->state == MOVE_OFF_STAGE) && at_off_stage
                   && qp_lock_screen_check)
                 {
                    mw->m->quickpanel->state=MOVE_WORK;
                    _e_mod_move_quickpanel_transients_find(mw->m);
                    _e_mod_move_quickpanel_transients_move_lock(mw->m, EINA_TRUE);
                    _e_mod_move_rotate_block_enable(mw->m);
                    e_mod_move_active_win_state_update(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
        else
          {
             if (mw->m->app_tray)
               if ((mw->m->app_tray->state == MOVE_OFF_STAGE) && qp_off_stage
                   && !lock_scrren_enable)
                 {
                    mw->m->app_tray->state=MOVE_WORK;
                    e_border_raise(mw->m->app_tray->border);
                    e_mod_move_show_apptray_helper(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
     }
   else if (mw->event_handler.window_rotation_angle == 270)
     {
        if (info->coord.y > APP_TRAY_CHECK_REGION)
          {
             if (mw->m->quickpanel)
               if ((mw->m->quickpanel->state == MOVE_OFF_STAGE) && at_off_stage
                   && qp_lock_screen_check)
                 {
                    mw->m->quickpanel->state=MOVE_WORK;
                    _e_mod_move_quickpanel_transients_find(mw->m);
                    _e_mod_move_quickpanel_transients_move_lock(mw->m,EINA_TRUE);
                    _e_mod_move_rotate_block_enable(mw->m);
                    e_mod_move_active_win_state_update(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
        else
          {
             if (mw->m->app_tray)
               if ((mw->m->app_tray->state == MOVE_OFF_STAGE) && qp_off_stage
                   && !lock_scrren_enable)
                 {
                    mw->m->app_tray->state=MOVE_WORK;
                    e_border_raise(mw->m->app_tray->border);
                    e_mod_move_show_apptray_helper(mw->m);
                    _e_mod_move_tp_indi_touch_win_stack_update(mw->m);
                 }
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_indicator_cb_motion_move(void *data,
                          void *event_info)
{
   E_Move_Win *mw = (E_Move_Win *)data;
   E_Move_Event_Motion_Info *info;
   info  = (E_Move_Event_Motion_Info *)event_info;
   if (!mw || !info) return EINA_FALSE;
   if (!mw->event_handler.mouse_clicked) return EINA_FALSE;

   if (mw->event_handler.window_rotation_angle == 0)
     {
       if (mw->m->quickpanel)
         if (mw->m->quickpanel->state == MOVE_WORK)
           {
              if (info->coord.y < (mw->m->quickpanel->window_geometry.h - mw->m->indicator->window_geometry.h))
                {
                   _e_mod_move_quickpanel_move(mw->m, 0, info->coord.y - (mw->m->quickpanel->window_geometry.h - mw->m->indicator->window_geometry.h),
                                               0, info->coord.y - (mw->m->quickpanel->window_geometry.h - mw->m->indicator->window_geometry.h), LAYOUT_POSITION_Y,
                                               EINA_FALSE);
                }
          }
       if (mw->m->app_tray)
         if (mw->m->app_tray->state == MOVE_WORK)
           {
             if (info->coord.y < (mw->m->app_tray->window_geometry.h - mw->m->indicator->window_geometry.h))
               {
                 _e_mod_move_app_tray_move(mw->m, 0, info->coord.y - (mw->m->app_tray->window_geometry.h - mw->m->indicator->window_geometry.h), EINA_FALSE);
               }
           }
     }
   else if (mw->event_handler.window_rotation_angle == 90)
     {
       if (mw->m->quickpanel)
         if (mw->m->quickpanel->state == MOVE_WORK)
           {
             if (info->coord.x < (mw->m->quickpanel->window_geometry.w - mw->m->indicator->window_geometry.w))
               {
                  _e_mod_move_quickpanel_move(mw->m, info->coord.x - (mw->m->quickpanel->window_geometry.w - mw->m->indicator->window_geometry.w), 0,
                                              info->coord.x - (mw->m->quickpanel->window_geometry.w - mw->m->indicator->window_geometry.w), 0, LAYOUT_POSITION_X,
                                              EINA_FALSE);
               }
           }
       if (mw->m->app_tray)
         if (mw->m->app_tray->state == MOVE_WORK)
           {
             if (info->coord.x < (mw->m->app_tray->window_geometry.w - mw->m->indicator->window_geometry.w))
               {
                 _e_mod_move_app_tray_move(mw->m, info->coord.x - (mw->m->app_tray->window_geometry.w - mw->m->indicator->window_geometry.w), 0, EINA_FALSE);
               }
           }
     }
   else if (mw->event_handler.window_rotation_angle == 180)
     {
       if (mw->m->quickpanel)
         if (mw->m->quickpanel->state == MOVE_WORK)
           {
             if (info->coord.y > (mw->m->man->h) - (mw->m->quickpanel->window_geometry.h - mw->m->indicator->window_geometry.h))
               {
                  _e_mod_move_quickpanel_move(mw->m, 0, info->coord.y - mw->m->indicator->window_geometry.h,
                                              0, info->coord.y - mw->m->indicator->window_geometry.h, LAYOUT_POSITION_Y,
                                              EINA_FALSE);
               }
           }
       if (mw->m->app_tray)
         if (mw->m->app_tray->state == MOVE_WORK)
           {
             if (info->coord.y > (mw->m->man->h) - (mw->m->app_tray->window_geometry.h - mw->m->indicator->window_geometry.h))
               {
                 _e_mod_move_app_tray_move(mw->m, 0, info->coord.y - mw->m->indicator->window_geometry.h, EINA_FALSE);
               }
           }
     }
   else if (mw->event_handler.window_rotation_angle == 270)
     {
       if (mw->m->quickpanel)
         if (mw->m->quickpanel->state == MOVE_WORK)
           {
             if (info->coord.x > (mw->m->man->w) - (mw->m->quickpanel->window_geometry.w - mw->m->indicator->window_geometry.w))
               {
                  _e_mod_move_quickpanel_move(mw->m, info->coord.x - mw->m->indicator->window_geometry.w, 0,
                                              info->coord.x - mw->m->indicator->window_geometry.w, 0, LAYOUT_POSITION_X,
                                              EINA_FALSE);
               }
           }
       if (mw->m->app_tray)
         if (mw->m->app_tray->state == MOVE_WORK)
           {
             if (info->coord.x > (mw->m->man->w) - (mw->m->app_tray->window_geometry.w - mw->m->indicator->window_geometry.w))
               {
                 _e_mod_move_app_tray_move(mw->m, info->coord.x - mw->m->indicator->window_geometry.w, 0, EINA_FALSE);
               }
           }
     }

   return EINA_TRUE;
}

static Eina_Bool
_indicator_cb_motion_end(void *data,
                         void *event_info)
{
   E_Move_Win *mw = (E_Move_Win *)data;
   E_Move_Event_Motion_Info *info;
   info  = (E_Move_Event_Motion_Info *)event_info;
   int v0, v1, x0, y0, x1, y1;
   if (!mw || !info) return EINA_FALSE;
   if (!mw->event_handler.mouse_clicked)
     {
        goto finish;
     }

   if ((mw->m->quickpanel) &&
       (mw->m->quickpanel->state == MOVE_WORK))
     {
        v0 = v1 = x0 = y0 = x1 = y1 = 0;
        switch (mw->event_handler.window_rotation_angle)
          {
           case  90:
             v0 = info->coord.x;
             v1 = (mw->m->quickpanel->window_geometry.w)/2;
             x1 = -(mw->m->quickpanel->window_geometry.w);
             break;
           case 180:
             v0 = mw->m->man->h - ((mw->m->quickpanel->window_geometry.h)/2);
             v1 = info->coord.y;
             y1 = mw->m->quickpanel->window_geometry.h;
             break;
           case 270:
             v0 = mw->m->man->w - ((mw->m->quickpanel->window_geometry.w)/2);
             v1 = info->coord.x;
             x1 = mw->m->quickpanel->window_geometry.w;
             break;
           case   0:
           default :
             v0 = info->coord.y;
             v1 = (mw->m->quickpanel->window_geometry.h)/2;
             y1 = -(mw->m->quickpanel->window_geometry.h);
             break;
          }

        if (v0 > v1)
          {
             _e_mod_move_quickpanel_move_animation(mw->m, x0, y0, EINA_TRUE);
             mw->m->quickpanel->state = MOVE_ON_STAGE;
             _e_mod_move_illume_quickpanel_state_on(mw->m);
             _e_mod_move_key_handler_enable(mw->m);
          }
        else
          {
             _e_mod_move_quickpanel_move_animation(mw->m, x1, y1, EINA_FALSE);
             mw->m->quickpanel->state = MOVE_OFF_STAGE;
             _e_mod_move_illume_quickpanel_state_off(mw->m);
             _e_mod_move_key_handler_disable(mw->m);
          }
     }

   if ((mw->m->app_tray) &&
       (mw->m->app_tray->state == MOVE_WORK))
     {
        if (mw->m->apptray_helper)
          {
             mw->m->apptray_helper->event.enable = EINA_TRUE;
             mw->m->apptray_helper->event.click = EINA_FALSE;
          }
        v0 = v1 = x0 = y0 = x1 = y1 = 0;
        switch (mw->event_handler.window_rotation_angle)
          {
           case  90:
             v0 = info->coord.x;
             v1 = (mw->m->app_tray->window_geometry.w)/2;
             x1 = -(mw->m->app_tray->window_geometry.w);
             break;
           case 180:
             v0 = mw->m->man->h - ((mw->m->app_tray->window_geometry.h)/2);
             v1 = info->coord.y;
             y0 = mw->m->man->h - mw->m->app_tray->window_geometry.h;
             y1 = mw->m->man->h;
             break;
           case 270:
             v0 = mw->m->man->w - ((mw->m->app_tray->window_geometry.w)/2);
             v1 = info->coord.x;
             x0 = mw->m->man->w - mw->m->app_tray->window_geometry.w;
             x1 = mw->m->man->w;
             break;
           case   0:
           default :
             v0 = info->coord.y;
             v1 = (mw->m->app_tray->window_geometry.h)/2;
             y1 = -(mw->m->app_tray->window_geometry.h);
             break;
          }

        if (v0 > v1)
          {
             _e_mod_move_apptray_move_animation(mw->m, x0, y0, EINA_TRUE);
             mw->m->app_tray->state = MOVE_ON_STAGE;
             _e_mod_move_key_handler_enable(mw->m);
          }
        else
          {
             _e_mod_move_apptray_move_animation(mw->m, x1, y1, EINA_FALSE);
             mw->m->app_tray->state = MOVE_OFF_STAGE;
             _e_mod_move_key_handler_disable(mw->m);
          }
     }

finish:
   mw->event_handler.mouse_clicked = EINA_FALSE;

   if (mw->m->indicator) mw->m->indicator->state = MOVE_ON_STAGE;
   if (mw->m->quickpanel)
     {
        _e_mod_move_quickpanel_transients_release(mw->m);
     }

   return EINA_TRUE;
}

void
e_mod_move_win_quickpanel_mouse_down_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get( obj, "move");
   if ( mw == NULL ) return;

   mw->event_handler.mouse_clicked = EINA_TRUE;
   mw->state=MOVE_WORK;
   _e_mod_move_quickpanel_transients_find(mw->m);
   _e_mod_move_quickpanel_transients_move_lock(mw->m,EINA_FALSE);
   e_mod_move_active_win_state_update(mw->m);
}

void
e_mod_move_win_quickpanel_mouse_move_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *mouse_move_info = event_info;
   E_Move_Win *mw = NULL;
   int indi_w = 0;
   int indi_h = 0;

   mw = evas_object_data_get( obj, "move");
   if ( mw == NULL ) return;

   if (mw->m->indicator)
     {
        indi_w = mw->m->indicator->window_geometry.w;
        indi_h = mw->m->indicator->window_geometry.h;
     }

   if ((mw->event_handler.mouse_clicked == EINA_TRUE) && (mw->state == MOVE_WORK))
     {
        if (mw->event_handler.window_rotation_angle == 0)
          {
             if (mouse_move_info->cur.output.y < (mw->m->quickpanel->window_geometry.h - indi_h))
               {
                   _e_mod_move_quickpanel_move(mw->m, 0, mouse_move_info->cur.output.y - (mw->m->quickpanel->window_geometry.h - indi_h),
                                               0, mouse_move_info->cur.output.y - (mw->m->quickpanel->window_geometry.h - indi_h), LAYOUT_POSITION_Y,
                                               EINA_FALSE);
               }
          }
        if (mw->event_handler.window_rotation_angle == 90)
          {
             if (mouse_move_info->cur.output.x < (mw->m->quickpanel->window_geometry.w - indi_w))
               {
                  _e_mod_move_quickpanel_move(mw->m, mouse_move_info->cur.output.x - (mw->m->quickpanel->window_geometry.w - indi_w), 0,
                                              mouse_move_info->cur.output.x - (mw->m->quickpanel->window_geometry.w - indi_w), 0, LAYOUT_POSITION_X,
                                              EINA_FALSE);
               }
          }
        if (mw->event_handler.window_rotation_angle == 180)
          {
             if (mouse_move_info->cur.output.y > (mw->m->man->h) - (mw->m->quickpanel->window_geometry.h - indi_h))
               {
                  _e_mod_move_quickpanel_move(mw->m, 0, mouse_move_info->cur.output.y - indi_h,
                                              0, mouse_move_info->cur.output.y - indi_h, LAYOUT_POSITION_Y,
                                              EINA_FALSE);
               }
          }
        if (mw->event_handler.window_rotation_angle == 270)
          {
             if (mouse_move_info->cur.output.x > (mw->m->man->w) - (mw->m->quickpanel->window_geometry.w - indi_w))
               {
                  _e_mod_move_quickpanel_move(mw->m, mouse_move_info->cur.output.x - indi_w, 0,
                                              mouse_move_info->cur.output.x - indi_w, 0, LAYOUT_POSITION_X,
                                              EINA_FALSE);
               }
          }
     }
}

void
e_mod_move_win_quickpanel_mouse_up_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *mouse_up_info = event_info;
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get( obj, "move");
   if ( mw == NULL ) return;

   if ((mw->event_handler.mouse_clicked == EINA_TRUE) && (mw->state == MOVE_WORK))
     {
        if (mw->event_handler.window_rotation_angle == 0)
          {
             if (mouse_up_info->output.y > (mw->m->quickpanel->window_geometry.h)/2)
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, 0, 0, 0, 0, LAYOUT_POSITION_Y, EINA_TRUE);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, 0,0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_illume_quickpanel_state_on(mw->m);
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, -10000, -10000, -10000, -10000, LAYOUT_POSITION_Y, EINA_TRUE);
                  _e_mod_move_rotate_block_disable(mw->m);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, 0, (mw->m->quickpanel->window_geometry.h)*-1, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_illume_quickpanel_state_off(mw->m);
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
        if (mw->event_handler.window_rotation_angle == 90)
          {
             if (mouse_up_info->output.x > (mw->m->quickpanel->window_geometry.w)/2)
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, 0, 0, 0, 0, LAYOUT_POSITION_X, EINA_TRUE);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, 0,0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_illume_quickpanel_state_on(mw->m);
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, -10000, -10000, -10000, -10000, LAYOUT_POSITION_X, EINA_TRUE);
                  _e_mod_move_rotate_block_disable(mw->m);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, (mw->m->quickpanel->window_geometry.w)*-1, 0, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_illume_quickpanel_state_off(mw->m);
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
        if (mw->event_handler.window_rotation_angle == 180)
          {
             if (mouse_up_info->output.y < mw->m->man->h - (mw->m->quickpanel->window_geometry.h)/2)
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, 0, mw->m->man->h - (mw->m->quickpanel->window_geometry.h),
                                              0, 0, LAYOUT_POSITION_Y, EINA_TRUE);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, 0,0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_illume_quickpanel_state_on(mw->m);
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, -10000, -10000, -10000, -10000, LAYOUT_POSITION_Y, EINA_TRUE);
                  _e_mod_move_rotate_block_disable(mw->m);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, 0, mw->m->quickpanel->window_geometry.h, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_illume_quickpanel_state_off(mw->m);
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
        if (mw->event_handler.window_rotation_angle == 270)
          {
             if (mouse_up_info->output.x < mw->m->man->w - (mw->m->quickpanel->window_geometry.w)/2)
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, mw->m->man->w - (mw->m->quickpanel->window_geometry.w), 0,
                                              0, 0, LAYOUT_POSITION_X, EINA_TRUE);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, 0,0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_illume_quickpanel_state_on(mw->m);
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef QP_ANI
                  _e_mod_move_quickpanel_move(mw->m, -10000, -10000, -10000, -10000, LAYOUT_POSITION_X, EINA_TRUE);
                  _e_mod_move_rotate_block_disable(mw->m);
#else
                  _e_mod_move_quickpanel_move_animation(mw->m, mw->m->quickpanel->window_geometry.w, 0, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_illume_quickpanel_state_off(mw->m);
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
     }
   mw->event_handler.mouse_clicked = EINA_FALSE;
   _e_mod_move_quickpanel_transients_release(mw->m);
}

void
e_mod_move_win_app_tray_mouse_down_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get( obj, "move");
   if ( mw == NULL ) return;

   mw->event_handler.mouse_clicked = EINA_TRUE;
   mw->state=MOVE_WORK;
}

void
e_mod_move_win_app_tray_mouse_move_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *mouse_move_info = event_info;
   E_Move_Win *mw = NULL;
   int indi_w = 0;
   int indi_h = 0;

   mw = evas_object_data_get( obj, "move");
   if ( mw == NULL ) return;

   if (mw->m->indicator)
     {
        indi_w = mw->m->indicator->window_geometry.w;
        indi_h = mw->m->indicator->window_geometry.h;
     }

   if ((mw->event_handler.mouse_clicked == EINA_TRUE) && (mw->state == MOVE_WORK))
     {
        if (mw->event_handler.window_rotation_angle == 0)
          {
             if (mouse_move_info->cur.output.y < (mw->m->app_tray->window_geometry.h - indi_h))
               _e_mod_move_app_tray_move(mw->m, 0, mouse_move_info->cur.output.y - (mw->m->app_tray->window_geometry.h - indi_h), EINA_FALSE);
          }
        if (mw->event_handler.window_rotation_angle == 90)
          {
             if (mouse_move_info->cur.output.x < (mw->m->app_tray->window_geometry.w - indi_w))
               _e_mod_move_app_tray_move(mw->m, mouse_move_info->cur.output.x - (mw->m->app_tray->window_geometry.w - indi_w), 0, EINA_FALSE);
          }
        if (mw->event_handler.window_rotation_angle == 180)
          {
             if (mouse_move_info->cur.output.y > (mw->m->man->h) - (mw->m->app_tray->window_geometry.h - indi_h))
               _e_mod_move_app_tray_move(mw->m, 0, mouse_move_info->cur.output.y - indi_h, EINA_FALSE);
          }
        if (mw->event_handler.window_rotation_angle == 270)
          {
             if (mouse_move_info->cur.output.x > (mw->m->man->w) - (mw->m->app_tray->window_geometry.w - indi_w))
               _e_mod_move_app_tray_move(mw->m, mouse_move_info->cur.output.x - indi_w, 0, EINA_FALSE);
          }
     }
}

void
e_mod_move_win_app_tray_mouse_up_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *mouse_up_info = event_info;
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get( obj, "move");
   if ( mw == NULL ) return;

   if ((mw->event_handler.mouse_clicked == EINA_TRUE) && (mw->state == MOVE_WORK))
     {
        if (mw->event_handler.window_rotation_angle == 0)
          {
             if (mouse_up_info->output.y > (mw->m->app_tray->window_geometry.h)/2)
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  0, 0, EINA_TRUE);
#else
                  _e_mod_move_apptray_move_animation(mw->m, 0, 0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  -10000, -10000, EINA_TRUE);
                  e_mod_move_hide_apptray_helper(mw->m);
#else
                  _e_mod_move_apptray_move_animation(mw->m, 0, (mw->m->app_tray->window_geometry.h)*-1, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
        if (mw->event_handler.window_rotation_angle == 90)
          {
             if (mouse_up_info->output.x > (mw->m->app_tray->window_geometry.w)/2)
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  0, 0, EINA_TRUE);
#else
                  _e_mod_move_apptray_move_animation(mw->m, 0, 0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  -10000, -10000, EINA_TRUE);
                  e_mod_move_hide_apptray_helper(mw->m);
#else
                  _e_mod_move_apptray_move_animation(mw->m, (mw->m->app_tray->window_geometry.w)*-1, 0, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
        if (mw->event_handler.window_rotation_angle == 180)
          {
             if (mouse_up_info->output.y < mw->m->man->h - (mw->m->app_tray->window_geometry.h)/2)
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  0, mw->m->man->h - (mw->m->app_tray->window_geometry.h), EINA_TRUE);
#else
                  _e_mod_move_apptray_move_animation(mw->m, 0, mw->m->man->h - (mw->m->app_tray->window_geometry.h), EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  -10000, -10000, EINA_TRUE);
                  e_mod_move_hide_apptray_helper(mw->m);
#else
                  _e_mod_move_apptray_move_animation(mw->m, 0, mw->m->man->h, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
        if (mw->event_handler.window_rotation_angle == 270)
          {
             if (mouse_up_info->output.x < mw->m->man->w - (mw->m->app_tray->window_geometry.w)/2)
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m, mw->m->man->w - mw->m->app_tray->window_geometry.w , 0, EINA_TRUE);
#else
                  _e_mod_move_apptray_move_animation(mw->m, mw->m->man->w - mw->m->app_tray->window_geometry.w, 0, EINA_TRUE);
#endif
                  mw->state = MOVE_ON_STAGE;
                  _e_mod_move_key_handler_enable(mw->m);
               }
             else
               {
#ifndef AT_ANI
                  _e_mod_move_app_tray_move(mw->m,  -10000, -10000, EINA_TRUE);
                  e_mod_move_hide_apptray_helper(mw->m);
#else
                  _e_mod_move_apptray_move_animation(mw->m, mw->m->man->w, 0, EINA_FALSE);
#endif
                  mw->state = MOVE_OFF_STAGE;
                  _e_mod_move_key_handler_disable(mw->m);
               }
          }
     }
   mw->event_handler.mouse_clicked = EINA_FALSE;
}

void
e_mod_move_win_app_tray_restack_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get(obj, "move");
   if ( mw == NULL ) return;
   if (mw->m->apptray_helper)
     {
        if (mw->m->tp_indi)
          {
            if (mw->m->tp_indi->touch.obj) evas_object_stack_below(mw->m->tp_indi->touch.obj, mw->m->app_tray->move_object);
            evas_object_stack_below(mw->m->apptray_helper->obj, mw->m->app_tray->move_object);
          }
        else
          evas_object_stack_below(mw->m->apptray_helper->obj, mw->m->app_tray->move_object);
     }
}

static Ecore_X_Window
_e_mod_move_win_get_client_xid(E_Move_Win *mw)
{
   if (!mw) return 0;
   if (mw->border) return mw->border->client.win;
   else return 0; // do not care E_borderless window
}
static Eina_Bool
_e_mod_move_prop_window_rotation(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Win *mw = NULL;
   E_Move *m = NULL;
   int req_angle = -1;
   int cur_angle = -1;

   m = _e_mod_move_get();
   if (!m)
     {
        return EINA_FALSE;
     }
   if (m->tp_indi)
     {
        E_Border *indi = _e_mod_move_bd_find_by_indicator(m);
        if (indi)
          {
             evas_object_move(m->tp_indi->obj, indi->x, indi->y);
             evas_object_resize(m->tp_indi->obj, indi->w, indi->h);
             m->tp_indi->input_shape.x = indi->x;
             m->tp_indi->input_shape.y = indi->y;
             m->tp_indi->input_shape.w = indi->w;
             m->tp_indi->input_shape.h = indi->h;
             _e_mod_move_comp_window_input_region_set(m->man, m->tp_indi->target_bd,
                                                      m->tp_indi->input_shape.x,
                                                      m->tp_indi->input_shape.y,
                                                      m->tp_indi->input_shape.w,
                                                      m->tp_indi->input_shape.h);
          }
        else
          _e_mod_move_tp_indi_hide(m);
     }
   mw = _e_mod_move_border_client_find(ev->win);
   if (!mw)
     {
        return EINA_FALSE;
     }

   if (!_e_mod_move_win_get_prop_angle(_e_mod_move_win_get_client_xid(mw),
                                       &req_angle,
                                       &cur_angle))
     {
         return EINA_FALSE;
     }

   mw->event_handler.window_rotation_angle = req_angle;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_set_quickpanel_layout(E_Move_Win *mw)
{
   unsigned int val[2] = { 0, 0 };
   int ret = -1;

   if (!mw) return EINA_FALSE;

   ret = ecore_x_window_prop_card32_get(_e_mod_move_win_get_client_xid(mw),
                                        ATOM_QUICKPANEL_LAYOUT_POSITION,
                                        val, 2);
   if (ret == -1) return EINA_FALSE;

   mw->quickpanel_layout.position_x = val[0];
   mw->quickpanel_layout.position_y = val[1];

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_quickpanel_layout(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Win *mw = NULL;

   mw = _e_mod_move_border_client_find(ev->win);
   if (!mw) return EINA_FALSE;

   if (!_e_mod_move_set_quickpanel_layout(mw)) return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_set_window_input_region(E_Move_Win *mw)
{
   int ret = -1;
   unsigned int val[4] = { 0, 0, 0, 0};

   if (!mw) return EINA_FALSE;

   ret = ecore_x_window_prop_card32_get(_e_mod_move_win_get_client_xid(mw),
                                        ATOM_WINDOW_INPUT_REGION,
                                        val, 4);
   if (ret == -1) return EINA_FALSE;

   mw->shape_input_region.input_region_x = val[0];
   mw->shape_input_region.input_region_y = val[1];
   mw->shape_input_region.input_region_w = val[2];
   mw->shape_input_region.input_region_h = val[3];

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_prop_window_input_region(Ecore_X_Event_Window_Property *ev)
{
   E_Move_Win *mw = NULL;

   mw = _e_mod_move_border_client_find(ev->win);
   if (!mw) return EINA_FALSE;

  if (!_e_mod_move_set_window_input_region(mw)) return EINA_FALSE;

  _e_mod_move_comp_window_input_region_set(mw->m->man, mw->border, mw->shape_input_region.input_region_x,
                                                                   mw->shape_input_region.input_region_y,
                                                                   mw->shape_input_region.input_region_w,
                                                                   mw->shape_input_region.input_region_h);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_message(void *data  __UNUSED__,
                    int type    __UNUSED__,
                    void       *event)
{
   Ecore_X_Event_Client_Message *ev;
   Ecore_X_Atom t;
   ev = (Ecore_X_Event_Client_Message *)event;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN((ev->format == 32), 0);

   t = ev->message_type;

   if      (t == ATOM_NET_WM_WINDOW_SHOW) _e_mod_move_msg_window_show(ev);
   else if (t == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) _e_mod_move_msg_qp_state(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_property(void *data __UNUSED__,
                     int type __UNUSED__,
                     void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;
   E_Move *m = NULL;
   if (!(m=_e_mod_move_get())) return ECORE_CALLBACK_PASS_ON;

   if (!ev) return ECORE_CALLBACK_PASS_ON;

   if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
     {
        _e_mod_move_prop_window_rotation(ev);
        return ECORE_CALLBACK_PASS_ON;
     }

   if (ATOM_QUICKPANEL_LAYOUT_POSITION
       && ev->atom == ATOM_QUICKPANEL_LAYOUT_POSITION)
     {
        _e_mod_move_prop_quickpanel_layout(ev);

        if (m->quickpanel)
          if (m->quickpanel->state == MOVE_ON_STAGE)
            {
               _e_mod_move_quickpanel_transients_release(m);
               _e_mod_move_quickpanel_transients_find(m);

               if (m->quickpanel->event_handler.window_rotation_angle == 0)
                 _e_mod_move_quickpanel_move(m, 0, 0, 0, 0, LAYOUT_POSITION_Y, EINA_TRUE);
               else if (m->quickpanel->event_handler.window_rotation_angle == 90)
                 _e_mod_move_quickpanel_move(m, 0, 0, 0, 0, LAYOUT_POSITION_X, EINA_TRUE);
              else if (m->quickpanel->event_handler.window_rotation_angle == 180)
                {
                   _e_mod_move_quickpanel_move(m, 0, m->man->h - (m->quickpanel->window_geometry.h),
                                               0, 0, LAYOUT_POSITION_Y, EINA_TRUE);
                }
              else if (m->quickpanel->event_handler.window_rotation_angle == 270)
                {
                   _e_mod_move_quickpanel_move(m, m->man->w - (m->quickpanel->window_geometry.w), 0,
                                               0, 0, LAYOUT_POSITION_X, EINA_TRUE);
                }
            }
        return ECORE_CALLBACK_PASS_ON;
     }

   if (ATOM_WINDOW_INPUT_REGION
       && ev->atom == ATOM_WINDOW_INPUT_REGION)
     {
        _e_mod_move_prop_window_input_region(ev);
        return ECORE_CALLBACK_PASS_ON;
     }

   if (ev->atom == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        _e_mod_move_prop_active_window_change(ev);
        return ECORE_CALLBACK_PASS_ON;
     }

   if (ev->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE)
     {
        _e_mod_move_prop_indicator_state_change(ev);
        return ECORE_CALLBACK_PASS_ON;
     }

   return ECORE_CALLBACK_PASS_ON;
}

void
e_mod_move_show_apptray_helper(E_Move *m)
{
   E_Move_AppTray_Helper *apptray_helper = NULL;
   if (!m) return;
   if (!m->app_tray) return;
   apptray_helper = calloc(1, sizeof(E_Move_AppTray_Helper));
   if (!apptray_helper) return;

   apptray_helper->geo.x = 0;
   apptray_helper->geo.y = 0;
   apptray_helper->geo.w = m->man->w;
   apptray_helper->geo.h = m->man->h;


   apptray_helper->event.up = e_mod_move_apptray_helper_mouse_up_callback;
   apptray_helper->event.down = e_mod_move_apptray_helper_mouse_down_callback;
   apptray_helper->event.move = e_mod_move_apptray_helper_mouse_move_callback;
   apptray_helper->event.click = EINA_FALSE;
   apptray_helper->event.enable = EINA_FALSE;

   apptray_helper->win = ecore_x_window_input_new(0, 0, 0,
                                                  apptray_helper->geo.w,
                                                  apptray_helper->geo.h);
   ecore_x_icccm_title_set(apptray_helper->win, "E MOVE ApptrayHelper");

   m->apptray_helper = apptray_helper;

   ecore_x_window_show(apptray_helper->win);
   apptray_helper->visible = EINA_TRUE;

   apptray_helper->event.up_handler =  ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, apptray_helper->event.up, apptray_helper);
   apptray_helper->event.down_handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, apptray_helper->event.down, apptray_helper);
   apptray_helper->event.move_handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, apptray_helper->event.move, apptray_helper);

   ecore_x_window_configure(apptray_helper->win,
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                            apptray_helper->geo.x, apptray_helper->geo.y,
                            apptray_helper->geo.w, apptray_helper->geo.h, 0,
                            m->app_tray->border->win, ECORE_X_WINDOW_STACK_BELOW);

   apptray_helper->obj = evas_object_rectangle_add(e_move_comp_evas);
   evas_object_color_set(apptray_helper->obj, 0,0,0,MIN_OPACITY);
   evas_object_move(apptray_helper->obj, 0, 0);
   evas_object_resize(apptray_helper->obj, m->man->w, m->man->h);
   evas_object_show(apptray_helper->obj);
   evas_object_stack_below(apptray_helper->obj, m->app_tray->move_object);
   _e_mod_move_rotation_lock(m);
}

void
e_mod_move_hide_apptray_helper(E_Move *m)
{
   if (!m) return;
   if (!m->apptray_helper) return;

   evas_object_hide(m->apptray_helper->obj);
   evas_object_del(m->apptray_helper->obj);

   if (m->apptray_helper->event.up_handler)
     {
        ecore_event_handler_del(m->apptray_helper->event.up_handler);
        m->apptray_helper->event.up_handler = NULL;
     }
   if (m->apptray_helper->event.down_handler)
     {
        ecore_event_handler_del(m->apptray_helper->event.down_handler);
        m->apptray_helper->event.down_handler = NULL;
     }
   if (m->apptray_helper->event.move_handler)
     {
        ecore_event_handler_del(m->apptray_helper->event.move_handler);
        m->apptray_helper->event.move_handler = NULL;
     }

   ecore_x_window_hide(m->apptray_helper->win);
   ecore_x_window_free(m->apptray_helper->win);
   free(m->apptray_helper);
   m->apptray_helper = NULL;
   _e_mod_move_rotation_unlock(m);
}

static Eina_Bool
e_mod_move_apptray_helper_mouse_up_callback(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Move_AppTray_Helper *apptray_helper;

   ev = event;
   E_Move *m = NULL;

   if (!(apptray_helper = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->event_window != apptray_helper->win) return ECORE_CALLBACK_PASS_ON;
   if (!(m=_e_mod_move_get())) return ECORE_CALLBACK_PASS_ON;
   if (apptray_helper->event.click == EINA_FALSE) return ECORE_CALLBACK_PASS_ON;
   apptray_helper->event.click = EINA_FALSE;
   if (m->app_tray)
     if (m->app_tray->state == MOVE_ON_STAGE)
       {
#ifndef AT_ANI
          _e_mod_move_app_tray_move(m, -10000, -10000, EINA_TRUE);
          e_mod_move_hide_apptray_helper(m);
#else
          if (m->app_tray->event_handler.window_rotation_angle == 0)
            _e_mod_move_apptray_move_animation(m, 0, (m->app_tray->window_geometry.h)*-1, EINA_FALSE);
          else if (m->app_tray->event_handler.window_rotation_angle == 90)
            _e_mod_move_apptray_move_animation(m, (m->app_tray->window_geometry.w)*-1, 0, EINA_FALSE);
          else if (m->app_tray->event_handler.window_rotation_angle == 180)
            _e_mod_move_apptray_move_animation(m, 0, m->man->h, EINA_FALSE);
          else if (m->app_tray->event_handler.window_rotation_angle == 270)
            _e_mod_move_apptray_move_animation(m, m->man->w, 0, EINA_FALSE);
#endif
          m->app_tray->state = MOVE_OFF_STAGE;
       }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
e_mod_move_apptray_helper_mouse_down_callback(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Move_AppTray_Helper *apptray_helper;

   ev = event;
   if (!(apptray_helper = data)) return ECORE_CALLBACK_PASS_ON;

   if (ev->event_window != apptray_helper->win) return ECORE_CALLBACK_PASS_ON;
   if (apptray_helper->event.enable) apptray_helper->event.click = EINA_TRUE;
   else apptray_helper->event.click = EINA_FALSE;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
e_mod_move_apptray_helper_mouse_move_callback(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Move *ev;
   E_Move_AppTray_Helper *apptray_helper;

   ev = event;
   if (!(apptray_helper = data)) return ECORE_CALLBACK_PASS_ON;

   if (ev->event_window != apptray_helper->win) return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_prop_active_window_change(Ecore_X_Event_Window_Property *ev)
{
   E_Move *m = NULL;
   E_Border *bd;
   Ecore_X_Window active_win;
   int indicator_state = 0;

   if (!(m=_e_mod_move_get())) return EINA_FALSE;
   if (ev->win != m->man->root) return EINA_FALSE;

   if (!ecore_x_window_prop_window_get(m->man->root, ECORE_X_ATOM_NET_ACTIVE_WINDOW, &active_win, 1)) return EINA_FALSE;

   if (m->tp_indi) _e_mod_move_tp_indi_hide(m);

   if (m->app_tray)
     if (m->app_tray->border->client.win == active_win) m->active_win.win = active_win;

   if (m->active_win.win != active_win)
     {
        m->active_win.win = active_win;
        indicator_state = _e_mod_move_prop_indicator_state_get(active_win);

        if (m->quickpanel)
          if (m->quickpanel->state == MOVE_ON_STAGE)
            {
               _e_mod_move_quickpanel_transients_find(m);
               m->active_win.animatable = EINA_FALSE;
               _e_mod_move_quickpanel_move(m, -10000, -10000, -10000, -10000, LAYOUT_POSITION_Y, EINA_TRUE);
               _e_mod_move_quickpanel_transients_release(m);
               m->quickpanel->state = MOVE_OFF_STAGE;
               _e_mod_move_illume_quickpanel_state_off(m);
               _e_mod_move_rotate_block_disable(m);
            }

        if (m->app_tray)
          if (m->app_tray->state == MOVE_ON_STAGE)
            {
               _e_mod_move_app_tray_move(m, -10000, -10000, EINA_TRUE);
               m->app_tray->state = MOVE_OFF_STAGE;
               e_mod_move_hide_apptray_helper(m);
               if (indicator_state)
                 if ((bd = _e_mod_move_bd_find_by_indicator(m)))
                   e_border_show(bd);
               e_border_lower(m->app_tray->border);
            }

        _e_mod_move_key_handler_disable(m);

        if (!indicator_state)
          {
             E_Border *target = e_border_find_all_by_client_window(active_win);
             if (target)
               {
                  if ((target->x == 0) && (target->y == 0)
                     && (target->w == m->man->w) && (target->h == m->man->h))
                    _e_mod_move_tp_indi_show(m, target);
               }
           }
     }

   return EINA_TRUE;
}

void
_e_mod_move_key_handler_enable(E_Move *m)
{
   E_Move_Key_Helper *key_helper = NULL;
   if (!m) return;

   if (m->key_helper)
     if (m->key_helper->state) return;

   key_helper = calloc(1, sizeof(E_Move_Key_Helper));

   if (!key_helper) return;

   key_helper->win = ecore_x_window_input_new(0, -1, -1, 1, 1);
   ecore_x_icccm_title_set(key_helper->win, "E MOVE KeyHandler");
   ecore_x_window_show(key_helper->win);
   key_helper->key_down_handler = ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _e_mod_move_hw_key_down_cb, key_helper);

   utilx_grab_key (ecore_x_display_get(), key_helper->win, KEY_END, TOP_POSITION_GRAB);
   utilx_grab_key (ecore_x_display_get(), key_helper->win, KEY_SELECT, TOP_POSITION_GRAB);

   key_helper->state = EINA_TRUE;
   m->key_helper = key_helper;
}

void
_e_mod_move_key_handler_disable(E_Move *m)
{
   if (!m) return;
   if (!m->key_helper) return;

   if (m->quickpanel)
     if (m->quickpanel->state == MOVE_ON_STAGE) return;

   if (m->app_tray)
     if (m->app_tray->state == MOVE_ON_STAGE) return;

   utilx_ungrab_key (ecore_x_display_get(), m->key_helper->win, KEY_END);
   utilx_ungrab_key (ecore_x_display_get(), m->key_helper->win, KEY_SELECT);

   if (m->key_helper->key_down_handler)
     {
        ecore_event_handler_del(m->key_helper->key_down_handler);
        m->key_helper->key_down_handler = NULL;
     }

   ecore_x_window_hide(m->key_helper->win);
   ecore_x_window_free(m->key_helper->win);

   free(m->key_helper);
   m->key_helper = NULL;
}

static Eina_Bool
_e_mod_move_hw_key_down_cb (void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev;
   E_Move *m = NULL;
   E_Move_Key_Helper *key_helper = NULL;

   ev = event;

   if (!(key_helper = data)) return ECORE_CALLBACK_PASS_ON;

   if (!(m=_e_mod_move_get())) return ECORE_CALLBACK_PASS_ON;

   if ((strcmp (ev->keyname, KEY_END) == 0) ||
       (strcmp (ev->keyname, KEY_SELECT) == 0))
     {
        if (m->key_helper)
          if (m->key_helper->state == EINA_TRUE)
            {
               if (m->quickpanel)
                 if (m->quickpanel->state == MOVE_ON_STAGE)
                   {
                      _e_mod_move_quickpanel_transients_find(m);
                      _e_mod_move_quickpanel_transients_move_lock(m, EINA_FALSE);
#ifndef QP_ANI
                      _e_mod_move_quickpanel_move(m, -10000, -10000, -10000, -10000, LAYOUT_POSITION_Y, EINA_TRUE);
                      _e_mod_move_rotate_block_disable(m);
#else
                      if (m->quickpanel->event_handler.window_rotation_angle == 0)
                        _e_mod_move_quickpanel_move_animation(m, 0, (m->quickpanel->window_geometry.h)*-1, EINA_FALSE);
                     else if (m->quickpanel->event_handler.window_rotation_angle == 90)
                        _e_mod_move_quickpanel_move_animation(m, (m->quickpanel->window_geometry.w)*-1, 0, EINA_FALSE);
                     else if (m->quickpanel->event_handler.window_rotation_angle == 180)
                        _e_mod_move_quickpanel_move_animation(m, 0, m->quickpanel->window_geometry.h, EINA_FALSE);
                     else if (m->quickpanel->event_handler.window_rotation_angle == 270)
                        _e_mod_move_quickpanel_move_animation(m, m->quickpanel->window_geometry.w, 0, EINA_FALSE);
#endif
                      m->quickpanel->state = MOVE_OFF_STAGE;
                      _e_mod_move_illume_quickpanel_state_off(m);
                      _e_mod_move_quickpanel_transients_release(m);
                   }

               if (m->app_tray)
                 if (m->app_tray->state == MOVE_ON_STAGE)
                   {
#ifndef AT_ANI
                      _e_mod_move_app_tray_move(m, -10000, -10000, EINA_TRUE);
                      e_mod_move_hide_apptray_helper(m);
#else
                     if (m->app_tray->event_handler.window_rotation_angle == 0)
                        _e_mod_move_apptray_move_animation(m, 0, (m->app_tray->window_geometry.h)*-1, EINA_FALSE);
                     else if (m->app_tray->event_handler.window_rotation_angle == 90)
                        _e_mod_move_apptray_move_animation(m, (m->app_tray->window_geometry.w)*-1, 0, EINA_FALSE);
                     else if (m->app_tray->event_handler.window_rotation_angle == 180)
                        _e_mod_move_apptray_move_animation(m, 0, m->man->h, EINA_FALSE);
                     else if (m->app_tray->event_handler.window_rotation_angle == 270)
                        _e_mod_move_apptray_move_animation(m, m->man->w, 0, EINA_FALSE);
#endif
                      m->app_tray->state = MOVE_OFF_STAGE;
                   }

               _e_mod_move_key_handler_disable(m);
            }
     }
   return ECORE_CALLBACK_PASS_ON;
}

void
_e_mod_move_illume_quickpanel_state_on(E_Move *m)
{
   Eina_List *l = NULL;
   E_Move_Win *find_mw = NULL;
   if (!m) return;
   if (!m->quickpanel) return;
   ecore_x_e_illume_quickpanel_state_send(m->quickpanel->border->client.win,
                                          ECORE_X_ILLUME_QUICKPANEL_STATE_ON);

   _e_mod_move_quickpanel_transients_find(m);
   if (m->quickpanel_child.quickpanel_transients)
     {
        EINA_LIST_FOREACH(m->quickpanel_child.quickpanel_transients, l, find_mw)
          {
             if (find_mw)
               {
                  ecore_x_e_illume_quickpanel_state_send(find_mw->border->client.win,ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
               }
           }
     }
   _e_mod_move_quickpanel_transients_release(m);
   if (m->indicator)
     if (m->indicator->border)
       ecore_x_e_illume_quickpanel_state_send(m->indicator->border->client.win, ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
}

void
_e_mod_move_illume_quickpanel_state_off(E_Move *m)
{
   Eina_List *l = NULL;
   E_Move_Win *find_mw = NULL;
   if (!m) return;
   if (!m->quickpanel) return;
   ecore_x_e_illume_quickpanel_state_send(m->quickpanel->border->client.win,
                                          ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
   _e_mod_move_quickpanel_transients_find(m);
   if (m->quickpanel_child.quickpanel_transients)
     {
        EINA_LIST_FOREACH(m->quickpanel_child.quickpanel_transients, l, find_mw)
          {
             if (find_mw)
               {
                  ecore_x_e_illume_quickpanel_state_send(find_mw->border->client.win,ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
               }
           }
     }
   _e_mod_move_quickpanel_transients_release(m);
   if (m->indicator)
     if (m->indicator->border)
       ecore_x_e_illume_quickpanel_state_send(m->indicator->border->client.win, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
}

void
_e_mod_move_rotate_block_enable(E_Move *m)
{
   E_Move_Block_Helper *block_helper = NULL;
   if (!m) return;

   if (m->block_helper)
     if (m->block_helper->state) return;

   block_helper = calloc(1, sizeof(E_Move_Block_Helper));

   if (!block_helper) return;

   block_helper->win = ecore_x_window_input_new(0, 0, 0, m->man->w, m->man->h);
   ecore_x_icccm_title_set(block_helper->win, "E MOVE RotationBlock");
   ecore_x_window_show(block_helper->win);

   ecore_x_window_configure(block_helper->win,
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                            0, 0,
                            m->man->w,m->man->h, 0,
                            m->quickpanel->border->win, ECORE_X_WINDOW_STACK_BELOW);

   block_helper->obj = evas_object_rectangle_add(e_move_comp_evas);
   evas_object_color_set(block_helper->obj, 0,0,0,MIN_OPACITY);
   evas_object_move(block_helper->obj, 0, 0);
   evas_object_resize(block_helper->obj, m->man->w, m->man->h);
   evas_object_show(block_helper->obj);
   evas_object_stack_below(block_helper->obj, m->quickpanel->move_object);

   block_helper->state = EINA_TRUE;
   m->block_helper = block_helper;
   _e_mod_move_rotation_lock(m);
}

void
_e_mod_move_rotate_block_disable(E_Move *m)
{
   if (!m) return;
   if (!m->block_helper) return;

   evas_object_hide(m->block_helper->obj);
   evas_object_del(m->block_helper->obj);
   ecore_x_window_hide(m->block_helper->win);
   ecore_x_window_free(m->block_helper->win);

   free(m->block_helper);
   m->block_helper = NULL;
   _e_mod_move_rotation_unlock(m);
}

static Eina_Bool
_e_mod_move_quickpanel_move_animation(E_Move *m, int quickpanel_move_x, int quickpanel_move_y, Eina_Bool is_on_screen)
{
   Eina_List *l = NULL;
   E_Move_Win *find_mw = NULL;
   E_Border *child_bd = NULL;

   double anim_time = 0.4;
   int x,y,w,h;
   int a,r,g,b;

   if (!m) return EINA_FALSE;
   if (!m->quickpanel) return EINA_FALSE;

   evas_object_geometry_get(m->quickpanel->move_object, &x, &y, &w, &h);
   qp_anim_data.m = m;
   qp_anim_data.start_x = x;
   qp_anim_data.start_y = y;
   qp_anim_data.end_x = quickpanel_move_x;
   qp_anim_data.end_y = quickpanel_move_y;
   qp_anim_data.dist_x = quickpanel_move_x - x;
   qp_anim_data.dist_y = quickpanel_move_y - y;
   qp_anim_data.is_on_screen = is_on_screen;

   if (m->block_helper)
     {
        evas_object_color_get(m->block_helper->obj, &r, &g, &b, &a);
        qp_anim_data.start_opacity = a;
        if (is_on_screen)
          {
             qp_anim_data.end_opacity = MAX_OPACITY;
             qp_anim_data.diff_opacity = MAX_OPACITY - a;
          }
        else
          {
             qp_anim_data.end_opacity = MIN_OPACITY;
             qp_anim_data.diff_opacity = MIN_OPACITY - a;
          }
     }

   EINA_LIST_FOREACH(m->quickpanel->border->transients, l, child_bd)
     {
        if (!child_bd) continue;

        find_mw = _e_mod_move_win_find(child_bd->win);
        if (find_mw)
          {
            qp_anim_data.qp_transients = eina_list_append(qp_anim_data.qp_transients, find_mw );
            find_mw = NULL;
          }
     }

   ecore_animator_timeline_add(anim_time, _qp_anim_frame, &qp_anim_data);

   return EINA_TRUE;
}

static Eina_Bool
_qp_anim_frame(void *data, double pos)
{
   double frame = pos;
   Anim_Data *anim_data = (Anim_Data *)data;
   Eina_List *l = NULL;
   E_Move_Win *find_mw = NULL;
   E_Move *m = _e_mod_move_get();

   int move_x;
   int move_y;

   if (!anim_data->m) return EINA_FALSE;
   if (!anim_data->m->quickpanel) return EINA_FALSE;

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);

   move_x = anim_data->start_x + anim_data->dist_x * frame;
   move_y = anim_data->start_y + anim_data->dist_y * frame;

   evas_object_move(anim_data->m->quickpanel->move_object, move_x, move_y);

   if (anim_data->m->block_helper)
     {
        int opacity;
        opacity =  anim_data->start_opacity + anim_data->diff_opacity * frame;
        evas_object_color_set(anim_data->m->block_helper->obj, 0, 0, 0, opacity);
        if (pos == 1.0)
          evas_object_color_set(anim_data->m->block_helper->obj, 0, 0, 0, anim_data->end_opacity);
     }

   if (pos == 1.0)
     {
        if (anim_data->is_on_screen)
          e_border_move(anim_data->m->quickpanel->border, anim_data->end_x, anim_data->end_y);
        else
          {
             e_border_move(anim_data->m->quickpanel->border, -10000, -10000);
             _e_mod_move_rotate_block_disable(m);
          }
     }

   if (anim_data->qp_transients)
     {
        EINA_LIST_FOREACH(anim_data->qp_transients, l, find_mw)
          {
             if (find_mw)
               {
                 if (pos == 1.0)
                   {
                     evas_object_move(find_mw->move_object, find_mw->quickpanel_layout.position_x + anim_data->end_x , find_mw->quickpanel_layout.position_y + anim_data->end_y);
                     find_mw->can_unlock = EINA_TRUE;
                     if (anim_data->is_on_screen)
                       e_border_move(find_mw->border, find_mw->quickpanel_layout.position_x + anim_data->end_x , find_mw->quickpanel_layout.position_y + anim_data->end_y);
                     else
                       e_border_move(find_mw->border, -10000 , -10000);
                     //_e_mod_move_comp_window_move_unlock(anim_data->m->man, find_mw->border);
                   }
                 else evas_object_move(find_mw->move_object, find_mw->quickpanel_layout.position_x + move_x , find_mw->quickpanel_layout.position_y + move_y);
               }
          }
     }

   if (anim_data->m->active_win.animatable)
     {
        int x = 0;
        int y = 0;
        if (anim_data->dist_x)
          {
             if (anim_data->m->quickpanel->event_handler.window_rotation_angle == 90)
               x = move_x + anim_data->m->quickpanel->border->w;
             else
               x = move_x - anim_data->m->quickpanel->border->w;
             y = move_y;
          }
        else
          {
             x = move_x;
             if (anim_data->m->quickpanel->event_handler.window_rotation_angle == 0)
               y = move_y + anim_data->m->quickpanel->border->h;
             else
               y = move_y - anim_data->m->quickpanel->border->h;
          }
        evas_object_move(anim_data->m->active_win.obj, x, y);
        if (pos == 1.0)
          {
             evas_object_move(anim_data->m->active_win.obj,
                              anim_data->m->active_win.bd->x,
                              anim_data->m->active_win.bd->y);
             _e_mod_move_comp_window_move_unlock(anim_data->m->man, anim_data->m->active_win.bd);
          }
     }

   if (pos == 1.0)
     {
        anim_data->qp_transients = eina_list_free(anim_data->qp_transients);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_apptray_move_animation(E_Move *m, int apptray_move_x, int apptray_move_y, Eina_Bool is_on_screen)
{
   double anim_time = 0.4;
   int x,y,w,h;
   int a,r,g,b;

   if (!m) return EINA_FALSE;
   if (!m->app_tray) return EINA_FALSE;

   evas_object_geometry_get(m->app_tray->move_object, &x, &y, &w, &h);
   apptray_anim_data.m = m;
   apptray_anim_data.start_x = x;
   apptray_anim_data.start_y = y;
   apptray_anim_data.end_x = apptray_move_x;
   apptray_anim_data.end_y = apptray_move_y;
   apptray_anim_data.dist_x = apptray_move_x - x;
   apptray_anim_data.dist_y = apptray_move_y - y;
   apptray_anim_data.is_on_screen = is_on_screen;

   if (m->apptray_helper)
     {
        evas_object_color_get(m->apptray_helper->obj, &r, &g, &b, &a);
        apptray_anim_data.start_opacity = a;

        if (is_on_screen)
          {
             apptray_anim_data.end_opacity = MAX_OPACITY;
             apptray_anim_data.diff_opacity = MAX_OPACITY - a;
          }
        else
          {
             apptray_anim_data.end_opacity = MIN_OPACITY;
             apptray_anim_data.diff_opacity = MIN_OPACITY - a;
          }
     }

   ecore_animator_timeline_add(anim_time, _apptray_anim_frame, &apptray_anim_data);

   return EINA_TRUE;
}

static Eina_Bool
_apptray_anim_frame(void *data, double pos)
{
   double frame = pos;
   Anim_Data *anim_data = (Anim_Data *)data;
   E_Border *bd;

   int move_x;
   int move_y;

   if (!anim_data->m) return EINA_FALSE;
   if (!anim_data->m->app_tray) return EINA_FALSE;

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);

   move_x = anim_data->start_x + anim_data->dist_x * frame;
   move_y = anim_data->start_y + anim_data->dist_y * frame;

   evas_object_move(anim_data->m->app_tray->move_object, move_x, move_y);

   if (anim_data->m->apptray_helper)
     {
        int opacity = MAX_OPACITY;
        opacity =  anim_data->start_opacity + anim_data->diff_opacity * frame;
        evas_object_color_set(anim_data->m->apptray_helper->obj, 0, 0, 0, opacity);
        if (pos == 1.0)
          evas_object_color_set(anim_data->m->apptray_helper->obj, 0, 0, 0, anim_data->end_opacity);
     }

   if (pos == 1.0)
     {
        if (anim_data->is_on_screen)
          {
             e_border_move(anim_data->m->app_tray->border, anim_data->end_x, anim_data->end_y);
             if (anim_data->m->indicator) e_border_hide(anim_data->m->indicator->border,2);

             e_border_focus_set(anim_data->m->app_tray->border, 1, 1);
          }
        else
          {
             e_border_move(anim_data->m->app_tray->border, -10000, -10000);
             e_mod_move_hide_apptray_helper(anim_data->m);
             if ((bd = _e_mod_move_bd_find_by_indicator(anim_data->m))) e_border_show(bd);
             e_border_lower(anim_data->m->app_tray->border);

             e_border_focus_set(anim_data->m->app_tray->border, 0, 0);
          }
     }

   return EINA_TRUE;
}

E_Border *
_e_mod_move_bd_find_by_indicator(E_Move *m)
{
   E_Move_Border *m_bd = NULL;
   EINA_INLIST_FOREACH(m->borders, m_bd)
     {
        if (m_bd->win_type == WIN_INDICATOR) return m_bd->bd;
     }
   return NULL;
}

static Eina_Bool
_e_mod_move_lock_screen_check(E_Move *m)
{
   E_Border *active_bd = NULL;
   Ecore_X_Window active_win = 0;
   const char *name = NULL;
   const char *clas = NULL;

   if (!m) return EINA_FALSE;
   if (ecore_x_window_prop_window_get(m->man->root, ECORE_X_ATOM_NET_ACTIVE_WINDOW, &active_win, 1) == -1)
     return EINA_FALSE;

   active_bd = e_border_find_by_client_window(active_win);
   if (!active_bd) return EINA_FALSE;

   name = active_bd->client.icccm.name;
   clas = active_bd->client.icccm.class;

   if (clas == NULL) return EINA_FALSE;
   if (strncmp(clas,"LOCK_SCREEN",strlen("LOCK_SCREEN")) != 0) return EINA_FALSE;
   if (name == NULL) return EINA_FALSE;
   if (strncmp(name,"LOCK_SCREEN",strlen("LOCK_SCREEN")) != 0) return EINA_FALSE;
   if (active_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NOTIFICATION) return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_mini_controller_check(E_Move *m)
{
   Ecore_X_Window mini_control_win = 0;
   Eina_Bool ret = EINA_FALSE;
   if (!ATOM_MINI_CONTROLLER_WINDOW) return EINA_FALSE;
   if (ecore_x_window_prop_window_get(m->man->root, ATOM_MINI_CONTROLLER_WINDOW, &mini_control_win, 1) == -1)
     return EINA_FALSE;
   if (mini_control_win) ret = EINA_TRUE;
   return ret;
}

static Eina_Bool
_animator_move(E_Move_Win *mw,
               int         dst_x,
               int         dst_y,
               double      duration,
               Eina_Bool   after_show)
{
   Ecore_Timeline_Cb time_cb = NULL;
   Anim_Data *data = NULL;
   int x, y, w, h;

   if (!mw || !mw->m) return EINA_FALSE;

   evas_object_geometry_get(mw->move_object,
                            &x, &y, &w, &h);

   if (mw->win_type == WIN_QUICKPANEL_BASE)
     {
        Eina_List *l;
        E_Move_Win *_mw;
        E_Border *_bd;

        data = &qp_anim_data;
        time_cb = _qp_anim_frame;

        EINA_LIST_FOREACH(mw->border->transients, l, _bd)
          {
             if (!_bd) continue;
             _mw = _e_mod_move_win_find(_bd->win);
             if (!_mw) continue;
             data->qp_transients = eina_list_append(data->qp_transients, _mw);
          }
     }
   else if (mw->win_type == WIN_APP_TRAY)
     {
        data = &apptray_anim_data;
        time_cb = _apptray_anim_frame;
     }
   else
     {
        return EINA_FALSE;
     }

   data->m = mw->m;
   data->start_x = x;
   data->start_y = y;
   data->end_x = dst_x;
   data->end_y = dst_y;
   data->dist_x = dst_x - x;
   data->dist_y = dst_y - y;
   data->is_on_screen = after_show;

   if (mw->m->block_helper || mw->m->apptray_helper)
     {
        int a,r,g,b;

        if (mw->m->block_helper)
          evas_object_color_get(mw->m->block_helper->obj, &r, &g, &b, &a);
        if (mw->m->apptray_helper)
          evas_object_color_get(mw->m->apptray_helper->obj, &r, &g, &b, &a);

        data->start_opacity = a;

        if (after_show)
          {
             data->end_opacity = MAX_OPACITY;
             data->diff_opacity = MAX_OPACITY - a;
          }
        else
          {
             data->end_opacity = MIN_OPACITY;
             data->diff_opacity = MIN_OPACITY - a;
          }
     }

   ecore_animator_timeline_add(duration, time_cb, data);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_window_show(Ecore_X_Event_Client_Message *ev)
{
   E_Move *m = NULL;
   E_Move_Win *mw = NULL;
   int open, angle;
   int sx, sy, ex, ey, w, h, rw, rh;

   E_CHECK_RETURN(ev, 0);
   open = !(!(ev->data.l[1]));
   mw = _e_mod_move_border_client_find(ev->data.l[0]);
   m = _e_mod_move_get();

   if (!mw || !m)
     {
        return EINA_FALSE;
     }

   angle = mw->event_handler.window_rotation_angle;
   angle %= 360;

   w = mw->window_geometry.w;
   h = mw->window_geometry.h;
   rw = m->man->w;
   rh = m->man->h;

   if ((open) && (mw->state == MOVE_OFF_STAGE))
     {
        Eina_Bool qp_on = EINA_FALSE;
        Eina_Bool at_on = EINA_FALSE;
        Eina_Bool res, lock_on, mini_on;
        int angles[2];

        if ((m->quickpanel) && (m->quickpanel->state != MOVE_OFF_STAGE)) qp_on = EINA_TRUE;
        if ((m->app_tray)   && (m->app_tray->state   != MOVE_OFF_STAGE)) at_on = EINA_TRUE;


        lock_on = _e_mod_move_lock_screen_check(m);
        mini_on = _e_mod_move_mini_controller_check(m);

        res = _e_mod_move_win_get_prop_angle(mw->border->client.win,
                                             &angles[0], &angles[1]);
        if ((res) && (angles[0] != angle))
          {
             mw->event_handler.window_rotation_angle = angles[0];
             angle = angles[0];
             angle %= 360;
          }

        if (m->quickpanel == mw)
          {
             int cx, cy;
             QuickPanel_Layout_Position lay;
             if ((lock_on) && (!mini_on)) return EINA_FALSE;
             ex = ey = 0;

             switch (angle)
               {
                case  90: sx = -(w); sy =    0; cx = -(w); cy =    0; lay = LAYOUT_POSITION_X; break;
                case 180: sx =    0; sy =   rh; cx =    0; cy =   rh; lay = LAYOUT_POSITION_Y; break;
                case 270: sx =   rw; sy =    0; cx =   rw; cy =    0; lay = LAYOUT_POSITION_X; break;
                case   0:
                default : sx =    0; sy = -(h); cx =    0; cy = -(h); lay = LAYOUT_POSITION_Y; break;
               }

             _e_mod_move_quickpanel_transients_find(m);
             _e_mod_move_quickpanel_transients_move_lock(m, EINA_FALSE);
             _e_mod_move_rotate_block_enable(m);
             mw->state = MOVE_WORK;
             _e_mod_move_tp_indi_touch_win_stack_update(m);
             m->active_win.animatable = EINA_FALSE;
             _e_mod_move_quickpanel_move(m, sx, sy, cx, cy, lay, EINA_FALSE);
             _e_mod_move_app_tray_move(m, sx, sy, EINA_FALSE);
             e_mod_move_active_win_state_update(m);
             _e_mod_move_illume_quickpanel_state_on(m);
             _e_mod_move_key_handler_enable(m);
             mw->event_handler.mouse_clicked = EINA_FALSE;
             _animator_move(mw, ex, ey, 0.5, EINA_TRUE);
             mw->state = MOVE_ON_STAGE;
             _e_mod_move_quickpanel_transients_release(m);

          }
        else if (m->app_tray == mw)
          {
             if (lock_on) return EINA_FALSE;

             switch (angle)
               {
                case  90: sx = -(w); sy =    0; ex =      0; ey =      0; break;
                case 180: sx =    0; sy =   rh; ex =      0; ey = rh - h; break;
                case 270: sx =   rw; sy =    0; ex = rw - w; ey =      0; break;
                case   0:
                default : sx =    0; sy = -(h); ex =      0; ey =      0; break;
               }

             _e_mod_move_app_tray_move(m, sx, sy, EINA_FALSE);
             e_border_raise(mw->border);
             e_mod_move_show_apptray_helper(m);
             _e_mod_move_key_handler_enable(m);
             _animator_move(mw, ex, ey, 0.5, EINA_TRUE);
             mw->event_handler.mouse_clicked = EINA_FALSE;
             if (m->apptray_helper) m->apptray_helper->event.enable = EINA_TRUE;
             mw->state = MOVE_ON_STAGE;
          }
     }
   else if ((!open) && (mw->state == MOVE_ON_STAGE))
     {
        if (m->quickpanel == mw)
          {
             switch (angle)
               {
                case  90: ex = -(w); ey =    0; break;
                case 180: ex =    0; ey =    h; break;
                case 270: ex =    w; ey =    0; break;
                case   0:
                default : ex =    0; ey = -(h); break;
               }

             _animator_move(mw, ex, ey, 0.4, EINA_FALSE);
             mw->state = MOVE_OFF_STAGE;
             _e_mod_move_illume_quickpanel_state_off(m);
             _e_mod_move_key_handler_disable(m);
             mw->event_handler.mouse_clicked = EINA_FALSE;
             _e_mod_move_quickpanel_transients_release(m);
          }

        else if (m->app_tray == mw)
          {
             switch (angle)
               {
                case  90: ex = -(w); ey =    0; break;
                case 180: ex =    0; ey =   rh; break;
                case 270: ex =   rw; ey =    0; break;
                case   0:
                default : ex =    0; ey = -(h); break;
               }
             _animator_move(mw, ex, ey, 0.4, EINA_FALSE);
             mw->event_handler.mouse_clicked = EINA_FALSE;
             mw->state = MOVE_OFF_STAGE;
             _e_mod_move_key_handler_disable(m);

          }
     }
   else
     {
        fprintf(stderr,
                "[MOVE_MODULE] _NET_WM_WINDOW_SHOW error."
                " w:0x%07x(state:%d) request:%d\n",
                mw->border->client.win, mw->state, open);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_msg_qp_state(Ecore_X_Event_Client_Message *ev)
{
   Ecore_X_Atom qp_state;
   Ecore_X_Window zone_win;
   E_Move *m = NULL;
   E_Move_Win *qp = NULL;
   int open, angle;
   int sx, sy, ex, ey, w, h, rw, rh;

   m = _e_mod_move_get();

   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(m->quickpanel, 0);
   qp = m->quickpanel;

   zone_win = ev->win;
   if (zone_win != ecore_x_e_illume_zone_get(qp->border->client.win))
     return EINA_FALSE;

   qp_state = ev->data.l[0];
   if (qp_state == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF) open = 0;
   else open = 1;

   angle = qp->event_handler.window_rotation_angle;
   angle %= 360;
   w = qp->window_geometry.w;
   h = qp->window_geometry.h;
   rw = m->man->w;
   rh = m->man->h;

   if ((open) && (qp->state == MOVE_OFF_STAGE))
     {
        Eina_Bool res, lock_on, mini_on;
        int angles[2];
        int cx, cy;
        QuickPanel_Layout_Position lay;

        lock_on = _e_mod_move_lock_screen_check(m);
        mini_on = _e_mod_move_mini_controller_check(m);

        res = _e_mod_move_win_get_prop_angle(qp->border->client.win,
                                             &angles[0], &angles[1]);
        if ((res) && (angles[0] != angle))
          {
             qp->event_handler.window_rotation_angle = angles[0];
             angle = angles[0];
             angle %= 360;
          }

        if ((lock_on) && (!mini_on)) return EINA_FALSE;

        ex = ey = 0;

        switch (angle)
          {
           case  90: sx = -(w); sy =    0; cx = -(w); cy =    0; lay = LAYOUT_POSITION_X; break;
           case 180: sx =    0; sy =   rh; cx =    0; cy =   rh; lay = LAYOUT_POSITION_Y; break;
           case 270: sx =   rw; sy =    0; cx =   rw; cy =    0; lay = LAYOUT_POSITION_X; break;
           case   0:
           default : sx =    0; sy = -(h); cx =    0; cy = -(h); lay = LAYOUT_POSITION_Y; break;
          }

        _e_mod_move_quickpanel_transients_find(m);
        _e_mod_move_quickpanel_transients_move_lock(m, EINA_FALSE);
        _e_mod_move_rotate_block_enable(m);
        qp->state = MOVE_WORK;
        _e_mod_move_tp_indi_touch_win_stack_update(m);
        m->active_win.animatable = EINA_FALSE;
        _e_mod_move_quickpanel_move(m, sx, sy, cx, cy, lay, EINA_FALSE);
        _e_mod_move_app_tray_move(m, sx, sy, EINA_FALSE);
        e_mod_move_active_win_state_update(m);
        _e_mod_move_illume_quickpanel_state_on(m);
        _e_mod_move_key_handler_enable(m);
        qp->event_handler.mouse_clicked = EINA_FALSE;
        _animator_move(qp, ex, ey, 0.5, EINA_TRUE);
        qp->state = MOVE_ON_STAGE;
        _e_mod_move_quickpanel_transients_release(m);

     }
   else if ((!open) && (qp->state == MOVE_ON_STAGE))
     {
        switch (angle)
          {
           case  90: ex = -(w); ey =    0; break;
           case 180: ex =    0; ey =    h; break;
           case 270: ex =    w; ey =    0; break;
           case   0:
           default : ex =    0; ey = -(h); break;
          }

        _animator_move(qp, ex, ey, 0.4, EINA_FALSE);
        qp->state = MOVE_OFF_STAGE;
        _e_mod_move_illume_quickpanel_state_off(m);
        _e_mod_move_key_handler_disable(m);
        qp->event_handler.mouse_clicked = EINA_FALSE;
        _e_mod_move_quickpanel_transients_release(m);
     }

   return EINA_TRUE;
}

static E_Move_Event_State
_tp_indi_cb_motion_check(E_Move_Event *ev,
                         void         *data)
{
   E_Move *m = NULL;
   E_Move_TP_Indi *tp_indi = (E_Move_TP_Indi *)data;
   Eina_List *l, *ll;
   E_Move_Event_Motion_Info *m0, *m1;
   unsigned int cnt = 0;
   E_Move_Event_State res = E_MOVE_EVENT_STATE_PASS;
   int w, h, angle, min_len, min_cnt = 2;
   E_Border *indi = NULL;

   m = _e_mod_move_get();
   if (!m || !ev || !tp_indi) goto finish;
   indi = _e_mod_move_bd_find_by_indicator(m);
   if (!indi) goto finish;

   res = e_mod_move_event_state_get(ev);
   angle = e_mod_move_event_angle_get(ev);

   l = e_mod_move_event_ev_queue_get(ev);
   if (!l) goto finish;

   cnt = eina_list_count(l);
   if (cnt < min_cnt) goto finish;

   m0 = eina_list_data_get(l);
   if (!m0) goto finish;

   ll = eina_list_last(l);
   if (!ll) goto finish;

   m1 = eina_list_data_get(ll);
   if (!m1) goto finish;

   if ((m1->cb_type == EVAS_CALLBACK_MOUSE_UP) &&
       (res == E_MOVE_EVENT_STATE_CHECK))
     {
        return E_MOVE_EVENT_STATE_PASS;
     }

   w = abs(m1->coord.x - m0->coord.x);
   h = abs(m1->coord.y - m0->coord.y);
   min_len = (indi->w > indi->h) ? (indi->h * 2) : (indi->w * 2);

   if ((w < min_len) && (h < min_len))
     {
        goto finish;
     }

   switch (angle)
     {
      case  90:
      case 270:
        if (w < h) res = E_MOVE_EVENT_STATE_PASS;
        else res = E_MOVE_EVENT_STATE_HOLD;
        break;
      case 180:
      case   0:
      default :
        if (w > h) res = E_MOVE_EVENT_STATE_PASS;
        else res = E_MOVE_EVENT_STATE_HOLD;
        break;
     }

finish:
   return res;
}

static Eina_Bool
_tp_indi_cb_motion_start(void *data,
                         void *event_info)
{
   E_Move_TP_Indi *tp_indi = (E_Move_TP_Indi *)data;
   E_Move_Event_Motion_Info *info;
   info  = (E_Move_Event_Motion_Info *)event_info;

   if(!tp_indi || !info) return EINA_FALSE;

   tp_indi->mouse_clicked = EINA_TRUE;
   return EINA_TRUE;
}


static Eina_Bool
_tp_indi_cb_motion_move(void *data,
                        void *event_info)
{
   E_Move_TP_Indi *tp_indi = (E_Move_TP_Indi *)data;
   E_Move_Event_Motion_Info *info;
   info  = (E_Move_Event_Motion_Info *)event_info;
   if(!tp_indi || !info) return EINA_FALSE;
   return EINA_TRUE;
}

static Eina_Bool
_tp_indi_cb_motion_end(void *data,
                       void *event_info)
{
   E_Move_TP_Indi *tp_indi = (E_Move_TP_Indi *)data;
   E_Move_Event_Motion_Info *info;
   E_Border *indi = NULL;
   E_Move *m = NULL;
   int angle;
   int d0, d1;

   m = _e_mod_move_get();
   info  = (E_Move_Event_Motion_Info *)event_info;
   if(!m || !tp_indi || !info) return EINA_FALSE;

   indi = _e_mod_move_bd_find_by_indicator(m);
   if (!indi) return EINA_FALSE;

   if (!tp_indi->mouse_clicked)
     {
        goto finish;
     }
   angle = e_mod_move_event_angle_get(tp_indi->event);
   d0 = d1 = 0;
   switch (angle)
     {
      case  90:
        d0 = info->coord.x;
        d1 = indi->w/2;
        break;
      case 180:
        d0 = m->man->h - (indi->h/2);
        d1 = info->coord.y;
        break;
      case 270:
        d0 =  m->man->w - (indi->w/2);
        d1 = info->coord.x;
        break;
      case   0:
      default :
        d0 = info->coord.y;
        d1 = indi->h/2;
        break;
     }

   if (d0 > d1)
     {
        e_border_show(indi);
        _e_mod_move_tp_indi_touch_win_show(m);
     }

finish:
   tp_indi->mouse_clicked = EINA_FALSE;
   return EINA_TRUE;
}

static int
_e_mod_move_prop_indicator_state_get(Ecore_X_Window win)
{
   Ecore_X_Illume_Indicator_State state;
   int show;

   state = ecore_x_e_illume_indicator_state_get(win);
   if (state == ECORE_X_ILLUME_INDICATOR_STATE_ON)
     show = 1;
   else if (state == ECORE_X_ILLUME_INDICATOR_STATE_OFF)
     show = 0;
   else
     show = -1;

   return show;
}

static Eina_Bool
_e_mod_move_tp_indi_show(E_Move *m, E_Border *target)
{
   E_Border *indi;
   Evas_Object *indi_obj = NULL;
   Evas_Object *obj = NULL;
   E_Move_TP_Indi *tp_indi = NULL;
   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(target, 0);
   if (m->tp_indi) return EINA_FALSE;
   tp_indi = calloc(1, sizeof(E_Move_TP_Indi));
   if (!tp_indi) return EINA_FALSE;
   indi = _e_mod_move_bd_find_by_indicator(m);
   E_CHECK_RETURN(indi, 0);
   indi_obj = _e_mod_move_comp_window_object_get(m->man, indi);
   E_CHECK_RETURN(indi_obj, 0);
   obj = evas_object_rectangle_add(e_move_comp_evas);
   //evas_object_color_set(obj, 255,255,255,128); // white
   evas_object_color_set(obj, 0,0,0,0); // transparent
   evas_object_move(obj, indi->x, indi->y);
   evas_object_resize(obj, indi->w, indi->h);
   evas_object_show(obj);
   evas_object_stack_below(obj, indi_obj);
   tp_indi->obj = obj;
   tp_indi->input_shape.x = indi->x;
   tp_indi->input_shape.y = indi->y;
   tp_indi->input_shape.w = indi->w;
   tp_indi->input_shape.h = indi->h;

   tp_indi->target_bd = target;
   _e_mod_move_comp_window_input_region_set(m->man, target,
                                            tp_indi->input_shape.x,
                                            tp_indi->input_shape.y,
                                            tp_indi->input_shape.w,
                                            tp_indi->input_shape.h);

   tp_indi->event = e_mod_move_event_new(target->client.win, obj);
   e_mod_move_event_angle_cb_set(tp_indi->event, _e_mod_move_win_get_prop_angle);
   e_mod_move_event_check_cb_set(tp_indi->event, _tp_indi_cb_motion_check, tp_indi);
   e_mod_move_event_cb_set(tp_indi->event, E_MOVE_EVENT_TYPE_MOTION_START,
                           _tp_indi_cb_motion_start, tp_indi);
   e_mod_move_event_cb_set(tp_indi->event, E_MOVE_EVENT_TYPE_MOTION_MOVE,
                           _tp_indi_cb_motion_move, tp_indi);
   e_mod_move_event_cb_set(tp_indi->event, E_MOVE_EVENT_TYPE_MOTION_END,
                           _tp_indi_cb_motion_end, tp_indi);

   m->tp_indi = tp_indi;
}

static Eina_Bool
_e_mod_move_tp_indi_hide(E_Move *m)
{
   E_CHECK_RETURN(m, 0);
   E_CHECK_RETURN(m->tp_indi, 0);

   e_mod_move_event_free(m->tp_indi->event);
   evas_object_del(m->tp_indi->obj);

   _e_mod_move_comp_window_input_region_set(m->man, m->tp_indi->target_bd,
                                            0, 0, 0, 0);
   _e_mod_move_tp_indi_touch_win_hide(m);
   free(m->tp_indi);
   m->tp_indi = NULL;
}

static Eina_Bool
e_mod_move_active_win_state_update(E_Move *m)
{
   E_Border *bd = NULL;
   E_CHECK_RETURN(m, 0);

   m->active_win.bd = NULL;
   m->active_win.obj = NULL;
   m->active_win.animatable = EINA_FALSE;

   ecore_x_window_prop_window_get(m->man->root, ECORE_X_ATOM_NET_ACTIVE_WINDOW, &(m->active_win.win), 1);
   m->active_win.bd = e_border_find_all_by_client_window(m->active_win.win);
   E_CHECK_RETURN(m->active_win.bd, 0);

   bd = m->active_win.bd;
   m->active_win.obj = _e_mod_move_comp_window_object_get(m->man, bd); ;
   E_CHECK_RETURN(m->active_win.obj, 0);
   _e_mod_move_comp_window_move_lock(m->man,bd);
   if ( (bd->x == 0) && (bd->y == 0) && (bd->w == m->man->w) && (bd->h == m->man->h))
     m->active_win.animatable = EINA_TRUE;
   return EINA_TRUE;
}

static void
_e_mod_move_tp_indi_touch_win_show(E_Move *m)
{
   E_Border *indi;
   E_Move_TP_Indi *tp_indi = NULL;
   E_CHECK(m);
   E_CHECK(m->tp_indi);
   tp_indi = m->tp_indi;
   E_CHECK(!tp_indi->touch.visible);
   tp_indi->touch.visible = EINA_TRUE;
   tp_indi->touch.ev_up = e_mod_move_tp_indi_touch_win_mouse_up_callback;
   tp_indi->touch.win = ecore_x_window_input_new(0, 0, 0,
                                                 m->man->w,
                                                 m->man->h);
   ecore_x_icccm_title_set(tp_indi->touch.win, "E MOVE TP_INDI_TOUCH");
   ecore_x_window_show(tp_indi->touch.win);
   tp_indi->touch.ev_up_handler =  ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, tp_indi->touch.ev_up, tp_indi);
   indi = _e_mod_move_bd_find_by_indicator(m);
   ecore_x_window_configure(tp_indi->touch.win,
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                            0, 0,
                            m->man->w, m->man->h, 0,
                            indi->win, ECORE_X_WINDOW_STACK_BELOW);
   tp_indi->touch.obj = evas_object_rectangle_add(e_move_comp_evas);
   evas_object_color_set(tp_indi->touch.obj, 0,0,0,MAX_OPACITY);
   evas_object_move(tp_indi->touch.obj, 0, 0);
   evas_object_resize(tp_indi->touch.obj, m->man->w, m->man->h);
   evas_object_show(tp_indi->touch.obj);
   evas_object_stack_below(tp_indi->touch.obj, tp_indi->obj);
}

static void
_e_mod_move_tp_indi_touch_win_hide(E_Move *m)
{
   E_Move_TP_Indi *tp_indi = NULL;
   E_CHECK(m);
   E_CHECK(m->tp_indi);
   tp_indi = m->tp_indi;
   E_CHECK(tp_indi->touch.visible);
   evas_object_hide(tp_indi->touch.obj);
   evas_object_del(tp_indi->touch.obj);
   tp_indi->touch.obj = NULL;
   if (tp_indi->touch.ev_up_handler)
     {
        ecore_event_handler_del(tp_indi->touch.ev_up_handler);
        tp_indi->touch.ev_up_handler = NULL;
     }
   ecore_x_window_hide(tp_indi->touch.win);
   ecore_x_window_free(tp_indi->touch.win);
   tp_indi->touch.visible = EINA_FALSE;
}

static void
_e_mod_move_tp_indi_touch_win_stack_update(E_Move *m)
{
   E_Move_TP_Indi *tp_indi = NULL;
   E_CHECK(m);
   E_CHECK(m->tp_indi);
   tp_indi = m->tp_indi;
   E_CHECK(tp_indi->touch.visible);
   E_CHECK(tp_indi->touch.obj);
   E_CHECK(tp_indi->touch.win);

   if (m->quickpanel)
     if (m->quickpanel->state == MOVE_WORK)
       {
          evas_object_stack_below(tp_indi->touch.obj, m->quickpanel->move_object);
          ecore_x_window_configure(tp_indi->touch.win,
                                   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                   0, 0,
                                   m->man->w, m->man->h, 0,
                                   m->quickpanel->border->win, ECORE_X_WINDOW_STACK_BELOW);
       }
   if (m->app_tray)
     if (m->app_tray->state == MOVE_WORK)
       {
          ecore_x_window_configure(tp_indi->touch.win,
                                   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                   0, 0,
                                   m->man->w, m->man->h, 0,
                                   m->app_tray->border->win, ECORE_X_WINDOW_STACK_BELOW);
       }
}

static Eina_Bool
e_mod_move_tp_indi_touch_win_mouse_up_callback(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Move_TP_Indi *tp_indi = NULL;
   E_Border *indi;
   ev = event;
   E_Move *m = NULL;

   if (!(tp_indi = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->event_window != tp_indi->touch.win) return ECORE_CALLBACK_PASS_ON;
   if (!(m=_e_mod_move_get())) return ECORE_CALLBACK_PASS_ON;

   indi = _e_mod_move_bd_find_by_indicator(m);
   if (indi) e_border_hide(indi,2);
   else _e_mod_move_tp_indi_touch_win_hide(m);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_move_prop_indicator_state_change(Ecore_X_Event_Window_Property *ev)
{
   E_Move *m = NULL;
   Ecore_X_Window active_win;
   E_Border *target = NULL;
   int indicator_state = -1;

   if (!(m=_e_mod_move_get())) return EINA_FALSE;
   if (!ecore_x_window_prop_window_get(m->man->root, ECORE_X_ATOM_NET_ACTIVE_WINDOW, &active_win, 1)) return EINA_FALSE;\

   if (ev->win != active_win) return EINA_FALSE;

   if (_e_mod_move_prop_indicator_state_get(active_win) == 0)
     {
        target = e_border_find_all_by_client_window(active_win);
        if (target)
          {
            if ((target->x == 0) && (target->y == 0)
                 && (target->w == m->man->w) && (target->h == m->man->h))
                 _e_mod_move_tp_indi_show(m, target);
          }
     }
   else
     _e_mod_move_tp_indi_hide(m);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_quickpanel_transients_move_lock(E_Move *m, Eina_Bool move_offscreen)
{
   E_Move_Win *find_mw = NULL;
   Eina_List *l = NULL;

   if (!m) return EINA_FALSE;

   if (!m->quickpanel) return EINA_FALSE;

   if (m->quickpanel_child.quickpanel_transients)
     {
        EINA_LIST_FOREACH(m->quickpanel_child.quickpanel_transients, l, find_mw)
          {
             if (find_mw)
               {
                  if (move_offscreen) e_border_move(find_mw->border, -15000, -15000);
                  find_mw->move_lock = EINA_TRUE;
                  _e_mod_move_comp_window_move_lock(m->man, find_mw->border);
               }
          }
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}

void
_e_mod_move_rotation_lock(E_Move *m)
{
   unsigned int val = 1;
   if (!m) return;
   if (m->rotation_lock) return;
   ecore_x_window_prop_card32_set(m->man->root, ATOM_ROTATION_LOCK, &val, 1);
   m->rotation_lock = EINA_TRUE;
}

void
_e_mod_move_rotation_unlock(E_Move *m)
{
   unsigned int val = 0;
   if (!m) return;
   if (!m->rotation_lock) return;
   ecore_x_window_prop_card32_set(m->man->root, ATOM_ROTATION_LOCK, &val, 1);
   m->rotation_lock = EINA_FALSE;
}

void
e_mod_move_win_quickpanel_mini_del_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get(obj, "move");
   if (mw == NULL) return;
   _e_mod_move_quickpanel_transients_remove(mw);
}

void
e_mod_move_win_del_callback(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Move_Win *mw = NULL;
   mw = evas_object_data_get(obj, "move");
   if (mw == NULL) return;
}

static Eina_Bool
_e_mod_move_quickpanel_transients_remove(E_Move_Win *mw)
{
   Eina_List *qp_transients = NULL;
   if (!mw) return EINA_FALSE;

   if (mw->m->quickpanel)
     {
        qp_transients = mw->m->quickpanel_child.quickpanel_transients;
        if (qp_transients)
          {
             mw->m->quickpanel_child.quickpanel_transients = eina_list_remove(qp_transients, mw);
          }
        else
          return EINA_FALSE;
     }
   else
     return EINA_FALSE;

   return EINA_TRUE;
}
