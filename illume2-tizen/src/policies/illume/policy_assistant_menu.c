#include "e_illume_private.h"
#include "policy.h"
#include "policy_assistant_menu.h"

#define ASSISTANT_MENU_ICON_SIZE_W 106
#define ASSISTANT_MENU_ICON_SIZE_H 106
#define ASSISTANT_MENU_ICON_GRID_X 205
#define ASSISTANT_MENU_ICON_GRID_Y 234

/* global event callback function */
static Eina_Bool  _policy_assistant_menu_cb_client_message(void   *data, int    type, void   *event);
static Eina_Bool  _policy_assistant_menu_cb_bd_resize(void *data, int type, void *event);
static Eina_Bool  _policy_assistant_menu_cb_bd_move(void *data, int type, void *event);
static Eina_Bool  _policy_assistant_menu_cb_bd_rot_begin(void *data, int type __UNUSED__, void *event);
static Eina_Bool  _policy_assistant_menu_cb_bd_rot_end(void *data, int type __UNUSED__, void *event);
static void       _policy_assistant_menu_cb_hook_eval_pre_new_border(void *data __UNUSED__, void *data2);
static void       _policy_assistant_menu_cb_hook_eval_post_new_border(void *data __UNUSED__, void *data2);

/* general function */
static E_Illume_Assistant_Menu_Border* _policy_assistant_menu_border_list_add(E_Border *bd);
static void                      _policy_assistant_menu_border_list_del(E_Border *bd);
static E_Illume_Assistant_Menu_Border* _policy_assistant_menu_get_assistant_menu_border(Ecore_X_Window win);

static int        _policy_assistant_menu_atom_init(void);

/* for iconify the assistant_menu windows */
static Eina_Bool  _policy_assistant_menu_cb_pinwin_mouse_move(void   *data, int    type __UNUSED__, void   *event);
static Eina_Bool  _policy_assistant_menu_cb_pinwin_mouse_down(void   *data, int    type __UNUSED__, void   *event);
static Eina_Bool  _policy_assistant_menu_cb_pinwin_mouse_up(void   *data, int    type __UNUSED__, void   *event);
static Eina_Bool  _policy_assistant_menu_pinwin_create(E_Illume_Assistant_Menu_Border *am_bd);
static Eina_Bool  _policy_assistant_menu_pinwin_destroy(E_Illume_Assistant_Menu_Border *am_bd);
static void       _policy_assistant_menu_pinwin_pos_set(E_Illume_Assistant_Menu_Border *am_bd,int x,int y);

static void       _policy_assistant_menu_pinoff(E_Illume_Assistant_Menu_Border *am_bd);
static void       _policy_assistant_menu_pinon(E_Illume_Assistant_Menu_Border *am_bd);
static void       _policy_assistant_menu_pinon_xy_get(E_Illume_Assistant_Menu_Border *am_bd, int *x, int *y);
static void       _policy_assistant_menu_pinoff_xy_get(E_Illume_Assistant_Menu_Border *am_bd, int *x, int *y);

/* for automatically align the assistant_menu windows */
static void       _policy_assistant_menu_pinon_start_rearrange_animation(void* data,int destx,int desty,int curx,int cury);
static Eina_Bool  _policy_assistant_menu_pinon_rearrange_animation_cb(void *data, double pos);
static void       _policy_assistant_menu_border_placement(E_Border *bd);
static int        _policy_assistant_menu_place_region_smart(E_Desk *desk,
                                                      Eina_List *skiplist,
                                                      int x, int y, int w, int h, int rotation,
                                                      int *rx, int *ry);
static Eina_Bool  _policy_assistant_menu_rotation_change_pos(E_Border *bd, int *x, int *y);

static Eina_List *_amw_hdls;
static Eina_List *_amw_hooks;
static Ecore_Idle_Enterer *_idle_enterer;

static Eina_Hash *assistant_menu_wins_hash;
static Eina_List *assistant_menu_wins;

EINTERN Eina_Bool
policy_assistant_menu_init(void)
{
   Eina_Bool ret = EINA_FALSE;

   /* event handlers */
   _amw_hdls =
      eina_list_append(_amw_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                               _policy_assistant_menu_cb_client_message,
                                               NULL));
   _amw_hdls =
      eina_list_append(_amw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_RESIZE,
                                               _policy_assistant_menu_cb_bd_resize,
                                               NULL));
   _amw_hdls =
      eina_list_append(_amw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_MOVE,
                                               _policy_assistant_menu_cb_bd_move,
                                               NULL));
   _amw_hdls =
      eina_list_append(_amw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ROTATION_CHANGE_BEGIN,
                                               _policy_assistant_menu_cb_bd_rot_begin, NULL));
   _amw_hdls =
      eina_list_append(_amw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ROTATION_CHANGE_END,
                                               _policy_assistant_menu_cb_bd_rot_end, NULL));

   /* hook functions */
   _amw_hooks =
      eina_list_append(_amw_hooks,
                       e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_NEW_BORDER,
                                         _policy_assistant_menu_cb_hook_eval_pre_new_border, NULL));

   _amw_hooks =
      eina_list_append(_amw_hooks,
                       e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER,
                                         _policy_assistant_menu_cb_hook_eval_post_new_border, NULL));

   ret = _policy_assistant_menu_atom_init();
   if (!ret)
     L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) Failed initializing atoms\n", __func__, __LINE__);

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) Initialization is Success!!\n", __func__, __LINE__);

   if (!assistant_menu_wins_hash)
     assistant_menu_wins_hash = eina_hash_string_superfast_new(NULL);

   return ret;
}

EINTERN void
policy_assistant_menu_shutdown(void)
{
   Ecore_Event_Handler *hdl;
   E_Border_Hook *hook;

   EINA_LIST_FREE(_amw_hdls, hdl)
     ecore_event_handler_del(hdl);
   EINA_LIST_FREE(_amw_hooks, hook)
     e_border_hook_del(hook);

   if (_idle_enterer) ecore_idle_enterer_del(_idle_enterer);
   _idle_enterer = NULL;

   if (assistant_menu_wins_hash) eina_hash_free(assistant_menu_wins_hash);
   assistant_menu_wins_hash = NULL;

   if (assistant_menu_wins) eina_list_free(assistant_menu_wins);
   assistant_menu_wins = NULL;
}


EINTERN void
policy_assistant_menu_idle_enterer(void)
{
   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   E_Border *bd = NULL;
   Eina_List *l;

   if (!assistant_menu_wins) return;

   EINA_LIST_FOREACH(assistant_menu_wins, l, am_bd)
     {
        if (!am_bd) continue;
        if (!am_bd->changed) continue;
        am_bd->changed = 0;

        bd = am_bd->bd;
        if (!bd) continue;

         L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) idle state, win: 0x%08x\n",
           __func__, __LINE__, bd->client.win);

        if (am_bd->defer.close)
          {
             e_border_act_close_begin(am_bd->bd);
             am_bd->defer.close = 0;
          }
     }
}

EINTERN Eina_List*
policy_assistant_menu_get_window_list(void)
{
   return assistant_menu_wins;
}

EINTERN void
policy_assistant_menu_border_add(E_Border *bd)
{
   EINA_SAFETY_ON_NULL_RETURN(bd);
   
   E_CHECK(e_illume_border_is_assistant_menu(bd));

#if 0
   if (bd->client.netwm.state.skip_taskbar)
      return;
#endif

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) ADD win: 0x%08x\n",
           __func__, __LINE__, bd->client.win);

   _policy_assistant_menu_border_list_add(bd);
}

EINTERN void
policy_assistant_menu_border_del(E_Border *bd)
{
   EINA_SAFETY_ON_NULL_RETURN(bd);

   E_CHECK(e_illume_border_is_assistant_menu(bd));

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) DEL 111. win: 0x%08x\n",
           __func__, __LINE__, bd->client.win);

   _policy_assistant_menu_border_list_del(bd);
}

EINTERN void
policy_assistant_menu_configure_request(Ecore_X_Event_Window_Configure_Request *ev)
{
   E_Illume_Assistant_Menu_Border *am_bd;

   am_bd = _policy_assistant_menu_get_assistant_menu_border(ev->win);
   E_CHECK(am_bd);

   if ((ev->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X) ||
       (ev->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y))
     {
        ELBF(ELBT_ILLUME, 0, ev->win,
             "[PLACEMENT] App(0x%08x) decides position by itself. => (%d,%d)",
             ev->win, ev->x, ev->y);
        am_bd->placed = 1;
     }
}

static Eina_Bool
_policy_assistant_menu_check_window_state_change_scenario(Ecore_X_Illume_Window_State from, Ecore_X_Illume_Window_State to)
{
   Eina_Bool ret = EINA_FALSE;

   if( ((from==ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU) && (to==ECORE_X_ILLUME_WINDOW_STATE_NORMAL))
        ||((from==ECORE_X_ILLUME_WINDOW_STATE_NORMAL) && (to==ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU)) )
     {
        ret = EINA_TRUE;
     }

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU]  %s(%d) SET WIN_STATE %d->%d  return=%d\n",
        __func__, __LINE__, from, to, ret);

   return ret;
}

EINTERN void
policy_assistant_menu_window_state_change(E_Border *bd, unsigned int state)
{
   E_Border *indi_bd;
   E_Illume_Border_Info *bd_info;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU]  SET WIN_STATE %d->%d size: %dx%d\n",
        bd->client.illume.win_state.state, state, bd->w, bd->h);

   if (bd->client.illume.win_state.state == state) return;

   bd_info = policy_get_border_info(bd);
   E_CHECK(bd_info);

   ELBF(LT_ASSISTANT_MENU, 0, bd->client.win, "SET WIN_STATE %d->%d size: %dx%d",
        bd->client.illume.win_state.state, state, bd->w, bd->h);

   /**
    * We only deal with below scenario. Other case, just update win_state.state and return it.
    * ECORE_X_ILLUME_WINDOW_STATE_NORMAL -> ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU
    * ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU -> ECORE_X_ILLUME_WINDOW_STATE_NORMAL
    **/
   if ( !_policy_assistant_menu_check_window_state_change_scenario(bd->client.illume.win_state.state, state) ) return;

   bd->client.illume.win_state.state = state;

   switch(state)
     {
      case ECORE_X_ILLUME_WINDOW_STATE_ASSISTANT_MENU:

#if 0
         if (!bd->client.netwm.state.skip_taskbar)
            am_bd = _policy_assistant_menu_border_list_add(bd);
#else
         am_bd = _policy_assistant_menu_border_list_add(bd);
#endif

         bd_info->used_to_floating = EINA_TRUE;
         bd_info->allow_user_geometry = EINA_TRUE;

         bd->lock_user_size = 0;
         bd->lock_user_location = 0;
         bd->borderless = 0;

         e_border_layer_set(bd, POL_ASSISTANT_MENU_LAYER);

         if (_e_illume_cfg->use_force_iconify)
           policy_border_uniconify_below_borders(bd);

         if (bd->maximized)
           e_border_unmaximize(bd, E_MAXIMIZE_BOTH);

         indi_bd = e_illume_border_indicator_get(bd->zone);
         if (indi_bd)
           {
              L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
              policy_border_indicator_control(indi_bd);

           }

         if (!bd->new_client)
           {
              policy_border_dep_rotation_list_add(bd);
              if (am_bd)
                am_bd->rot_done_wait = policy_border_dep_rotation_set(bd);
           }

         policy_border_illume_handlers_add(bd_info);
         break;
      case ECORE_X_ILLUME_WINDOW_STATE_NORMAL:

         e_border_layer_set(bd, POL_APP_LAYER);

         if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
           {
              bd->lock_user_size = 1;
              bd->lock_user_location = 1;
           }
         else if (E_ILLUME_BORDER_IS_IN_DESKTOP(bd))
           bd->borderless = 0;

         if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_UTILITY)
           {
              bd_info->allow_user_geometry = EINA_FALSE;
              if ((bd->x != bd->zone->x) || (bd->y != bd->zone->y) ||
                  (bd->w != bd->zone->w) || (bd->h != bd->zone->h))
                {
                   e_border_move_resize(bd, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
                }
           }

         indi_bd = e_illume_border_indicator_get(bd->zone);
         if (indi_bd)
           {
              L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
              policy_border_indicator_control(indi_bd);
           }

         am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
         if (am_bd)
           {
              if (!bd->iconic && !bd->visible)
                {
                   e_border_iconify(bd);
                }
           }

         policy_border_dep_rotation_list_del(bd);
         _policy_assistant_menu_border_list_del(bd);
         policy_border_illume_handlers_remove(bd_info);
         break;
     }

   // update type of rotation
   policy_border_rotation_type_update(bd);

   bd->changes.size = 1;
   bd->changes.pos= 1;
   bd->changed = 1;
}

EINTERN void
policy_assistant_menu_window_state_update(E_Border *bd,
                                    Ecore_X_Window_State        state,
                                    Ecore_X_Window_State_Action action)
{
   E_Illume_Assistant_Menu_Border *am_bd;

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) window_state_update\n",
           __func__, __LINE__);

   EINA_SAFETY_ON_NULL_RETURN(bd);

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
   E_CHECK(am_bd);

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) assistant menu window_state_update! state=%d action=%d lock_client_iconify=%d\n",
     __func__, __LINE__, state, action, bd->lock_client_iconify);

   switch (state)
     {
      case ECORE_X_WINDOW_STATE_ICONIFIED:
         if (action != ECORE_X_WINDOW_STATE_ACTION_ADD) return;
         if (bd->lock_client_iconify) return;

         //mini window (assistant_menu mode) is changed into icon with this event
         if (am_bd->pinon) return;

         _policy_assistant_menu_pinon(am_bd);
         break;

      default:
         break;
     }
}

#define SIZE_EQUAL_TO_ZONE(a, z) \
   ((((a)->w) == ((z)->w)) &&    \
    (((a)->h) == ((z)->h)))
EINTERN void
policy_assistant_menu_zone_layout_assistant_menu(E_Border *bd)
{
   E_Illume_Assistant_Menu_Border *am_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->client.win);
   E_CHECK(am_bd);

   if (!am_bd->placed)
     {
        // workaound: we assume that the size of assistant_menu window is different with zone.
        // since there is no size information in the assistant_menu protocol,
        // we don't know the size of the window which try to change the mode to assistant_menu.
        // so we should check out size difference with zone to place the assistant_menu window.
        if ((bd->visible) &&
            (!SIZE_EQUAL_TO_ZONE(bd, bd->zone)) &&
            (!e_border_rotation_is_progress(bd)))
          _policy_assistant_menu_border_placement(bd);
     }
}

EINTERN void
policy_assistant_menu_icon_rotation(E_Border *bd, int rotation)
{
   int new_x, new_y;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (!e_illume_border_is_assistant_menu(bd)) return;
   if (!e_illume_border_is_icon_win(bd)) return;

   ecore_evas_rotation_set(bd->internal_ecore_evas, rotation);

   if ((bd->client.e.state.rot.ang.prev == rotation) &&
       (bd->client.e.state.rot.ang.curr == rotation))
      return;

   bd->client.e.state.rot.ang.prev = bd->client.e.state.rot.ang.curr;
   bd->client.e.state.rot.ang.curr = rotation;

   if ((bd->visible) && (!bd->changes.visible))
     {
        if (_policy_assistant_menu_rotation_change_pos(bd, &new_x, &new_y))
          e_border_move(bd, new_x, new_y);
     }
}

EINTERN void
policy_assistant_menu_icon_relayout_by_keyboard(E_Border *kbd_bd)
{
   EINA_SAFETY_ON_NULL_RETURN(kbd_bd);

   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   E_Border *bd;
   Eina_List *l;
   Ecore_X_Virtual_Keyboard_State state;
   int dest_x = 0, dest_y = 0;

   EINA_LIST_FOREACH(assistant_menu_wins, l, am_bd)
     {
        if (am_bd->icon.win)
          {
             bd = am_bd->icon.win->border;
             if (am_bd->icon.keyboard_show == 0)
               {
                  am_bd->icon.keyboard_show = 1;
                  evas_object_color_set(am_bd->icon.edje_icon_layout, 255, 255, 255, 128);
                  if (E_INTERSECTS(bd->x, bd->y, bd->w,bd->h,kbd_bd->x, kbd_bd->y,kbd_bd->w,kbd_bd->h))
                    {
                       am_bd->icon.prev_x = bd->x;
                       am_bd->icon.prev_y = bd->y;
                       switch (kbd_bd->client.e.state.rot.ang.curr)
                         {
                          case 0:
                          case 180:
                             dest_x = bd->x;
                             dest_y = kbd_bd->y - bd->h;
                             break;
                          case 90:
                             dest_x = kbd_bd->x - bd->w;
                             dest_y = bd->y;
                             break;
                          case 270:
                             dest_x = kbd_bd->w;
                             dest_y = bd->y;
                             break;
                         }
                       _policy_assistant_menu_pinon_start_rearrange_animation((void *) am_bd,dest_x,dest_y,bd->x, bd->y);
                    }
                  else
                    {
                       am_bd->icon.prev_x = 0;
                       am_bd->icon.prev_y = 0;
                    }
               }
             else
               {
                  am_bd->icon.keyboard_show = 0;
                  evas_object_color_set(am_bd->icon.edje_icon_layout, 255, 255, 255, 255);
                  if (am_bd->icon.prev_x != 0 && am_bd->icon.prev_y != 0)
                    {
                       switch (kbd_bd->client.e.state.rot.ang.curr)
                         {
                          case 0:
                          case 180:
                             _policy_assistant_menu_pinon_start_rearrange_animation((void *) am_bd,bd->x,am_bd->icon.prev_y,bd->x, bd->y);
                             break;
                          case 90:
                          case 270:
                             _policy_assistant_menu_pinon_start_rearrange_animation((void *) am_bd,am_bd->icon.prev_x,bd->y,bd->x, bd->y);
                             break;
                         }
                    }
               }
          }
     }

   return;
}


static int
_policy_assistant_menu_atom_init(void)
{
   /* init assistant_menu specific atom */
   return 1;
}

static E_Illume_Assistant_Menu_Border*
_policy_assistant_menu_get_assistant_menu_border(Ecore_X_Window win)
{
   return eina_hash_find(assistant_menu_wins_hash, e_util_winid_str_get(win));
}

static E_Illume_Assistant_Menu_Border*
_policy_assistant_menu_border_list_add(E_Border *bd)
{
   E_Illume_Assistant_Menu_Border *am_bd = NULL;

   EINA_SAFETY_ON_NULL_GOTO(bd, error);
   if (!e_illume_border_is_assistant_menu(bd)) goto error;

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->client.win);
   if (am_bd) return am_bd;

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) assistant menu window is added in list, win: 0x%08x\n",
     __func__, __LINE__, bd->client.win);

   am_bd = E_NEW(E_Illume_Assistant_Menu_Border, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(am_bd, NULL);

   memset(am_bd, 0, sizeof(E_Illume_Assistant_Menu_Border));
   am_bd->bd = bd;

   eina_hash_add(assistant_menu_wins_hash, e_util_winid_str_get(bd->client.win), am_bd);
   eina_hash_add(assistant_menu_wins_hash, e_util_winid_str_get(bd->bg_win), am_bd);
   eina_hash_add(assistant_menu_wins_hash, e_util_winid_str_get(bd->win), am_bd);

   assistant_menu_wins = eina_list_append(assistant_menu_wins, am_bd);

   return am_bd;

error:
   ELB(ELBT_ILLUME, "[ASSISTANT_MENU] COULDN'T CREATE \"am_bd\"", bd->client.win);
   return NULL;
}

static void
_policy_assistant_menu_border_list_del(E_Border *bd)
{
   E_Illume_Assistant_Menu_Border *am_bd = NULL;

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
   if (!am_bd)
     {
        L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) There is no border in list\n", __func__, __LINE__);
        return;
     }

   L(LT_ASSISTANT_MENU, "[ILLUME][ASSISTANT_MENU] %s(%d) assistant menu window is removed in list, win:0x%08x\n",
     __func__, __LINE__, bd->win);

   _policy_assistant_menu_pinwin_destroy(am_bd);

   assistant_menu_wins = eina_list_remove(assistant_menu_wins, am_bd);
   eina_hash_del(assistant_menu_wins_hash, e_util_winid_str_get(bd->client.win), am_bd);
   eina_hash_del(assistant_menu_wins_hash, e_util_winid_str_get(bd->bg_win), am_bd);
   eina_hash_del(assistant_menu_wins_hash, e_util_winid_str_get(bd->win), am_bd);

   memset(am_bd, 0, sizeof(E_Illume_Assistant_Menu_Border));
   E_FREE(am_bd);
}

static Eina_Bool
_policy_assistant_menu_cb_client_message(void   *data __UNUSED__,
                                   int    type __UNUSED__,
                                   void   *event)
{
   //Ecore_X_Event_Client_Message *ev = event;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_assistant_menu_cb_bd_resize(void *data, int type, void *event)
{
   E_Event_Border_Resize *ev = event;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   E_Border *bd = ev->border;
   int new_x, new_y;

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
   E_CHECK_GOTO(am_bd, end);

   if (e_border_rotation_is_progress(bd))
     {
        am_bd->rot.geo.curr.w = bd->w;
        am_bd->rot.geo.curr.h = bd->h;
        am_bd->rot.resized = 1;
        if (((!am_bd->rot_done_wait) || (!am_bd->pinon)) && (am_bd->placed))
          {
             if (_policy_assistant_menu_rotation_change_pos(bd, &new_x, &new_y))
               e_border_move(bd, new_x, new_y);
          }
     }

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_assistant_menu_cb_bd_move(void *data, int type, void *event)
{
   E_Event_Border_Move *ev = event;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   E_Border *bd = ev->border;

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
   E_CHECK_GOTO(am_bd, end);

   if (e_border_rotation_is_progress(bd))
     {
        am_bd->rot.geo.curr.x = bd->x;
        am_bd->rot.geo.curr.y = bd->y;
     }
end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_assistant_menu_cb_bd_rot_begin(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Rotation_Change_Begin *ev = event;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   E_Border *bd = ev->border;

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
   E_CHECK_GOTO(am_bd, end);

   am_bd->rot.geo.prev.x = bd->x;
   am_bd->rot.geo.prev.y = bd->y;
   am_bd->rot.geo.prev.w = bd->w;
   am_bd->rot.geo.prev.h = bd->h;
   am_bd->rot.geo.curr.x = bd->x;
   am_bd->rot.geo.curr.y = bd->y;
   am_bd->rot.geo.curr.w = bd->w;
   am_bd->rot.geo.curr.h = bd->h;
   am_bd->rot.resized = 0;
end:
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_policy_assistant_menu_cb_bd_rot_end(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Rotation_Change_End *ev = event;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   E_Border *bd = ev->border;
   E_Win *ewin = NULL;

   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->client.win);
   E_CHECK_GOTO(am_bd, end);

   if ((am_bd->rot_done_wait) && (am_bd->pinon))
     {
        int x = 0, y = 0;

        ewin = am_bd->icon.win;
        if (ewin) e_border_hide(ewin->border, 0);

        if (bd->iconic)
          {
             e_border_raise(bd);
             e_border_uniconify(bd);
          }
        else
          {
             e_border_raise(bd);
             e_border_show(bd);

             if (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus)
               e_border_focus_set(bd, 1, 1);
          }

        _policy_assistant_menu_pinoff_xy_get(am_bd, &x, &y);
        e_border_move(bd, x, y);

        ELBF(ELBT_ILLUME, 0, bd->client.win,
             "PIN - OFF(by User) |  x:%d, y:%d",
             x, y);

        am_bd->pinon = 0;
        am_bd->rot_done_wait = 0;
     }
   else if (!am_bd->placed)
     {
        _policy_assistant_menu_border_placement(bd);
     }
   else if (!am_bd->rot.resized)
     {
        int new_x, new_y;

        if ((am_bd->rot.geo.prev.x == am_bd->rot.geo.curr.x) &&
            (am_bd->rot.geo.prev.y == am_bd->rot.geo.curr.y))
          {
             if (_policy_assistant_menu_rotation_change_pos(bd, &new_x, &new_y))
               _policy_assistant_menu_pinwin_pos_set(am_bd,new_x,new_y);
          }
     }
end:
   return ECORE_CALLBACK_RENEW;
}

static void
_policy_assistant_menu_cb_hook_eval_pre_new_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   EINA_SAFETY_ON_NULL_RETURN((bd = data2));
   if (!bd->new_client) return;
   if (bd->placed) return;
   if (bd->re_manage) return;
   if (bd->client.icccm.request_pos) return;
   if (!e_illume_border_is_assistant_menu(bd)) return;

   // set the "bd->placed" to avoid e17's major placement policy.
   // assistant_menu window will be placed at the border show by illume's policy.
   bd->placed = 1;
}

static void
_policy_assistant_menu_cb_hook_eval_post_new_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;
   Eina_List *l;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN((bd = data2));

   if (!bd->new_client) return;
   if (!bd->internal) return;

   EINA_LIST_FOREACH(assistant_menu_wins, l, am_bd)
     {
        if (am_bd->icon.win)
          {
             if (am_bd->icon.win->border == bd)
               {
                  e_border_layer_set(bd, POL_ASSISTANT_MENU_LAYER);
                  break;
               }
          }
     }
}

static Eina_Bool
_policy_assistant_menu_cb_pinwin_mouse_down(void   *data,
                                     int    type __UNUSED__,
                                     void *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_Illume_Assistant_Menu_Border *am_bd = (E_Illume_Assistant_Menu_Border *)data;
   E_Win *e_win;
   E_Border *bd;
   int down_x ,down_y;

   EINA_SAFETY_ON_NULL_GOTO(ev, end);
   EINA_SAFETY_ON_NULL_GOTO(am_bd, end);

   e_win = am_bd->icon.win;

   EINA_SAFETY_ON_NULL_GOTO(e_win, end);
   bd = e_win->border;

   if ((ev->event_window != e_win->evas_win) &&
       (ev->window != e_win->evas_win)) goto end;

   if ((ev->buttons >= 1) && (ev->buttons <= 3))
     {
        edje_object_signal_emit(am_bd->icon.edje_icon_layout,  "pin.focused", "illume2");
        am_bd->icon.mouse.pressed = 1;
        am_bd->icon.mouse.released = 0;
        am_bd->icon.mouse.last_down.x = ev->root.x;
        am_bd->icon.mouse.last_down.y = ev->root.y;
     }

   down_x = ev->root.x - (ASSISTANT_MENU_ICON_SIZE_W/2);
   down_y = ev->root.y - (ASSISTANT_MENU_ICON_SIZE_H/2);
   e_border_move(bd, down_x, down_y);

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_assistant_menu_cb_pinwin_mouse_move(void   *data,
                                     int    type __UNUSED__,
                                     void   *event)
{
   Ecore_Event_Mouse_Move *ev = (Ecore_Event_Mouse_Move *)event;
   E_Illume_Assistant_Menu_Border *am_bd = (E_Illume_Assistant_Menu_Border *)data;
   E_Win *e_win;
   E_Border *bd;
   double l;

   EINA_SAFETY_ON_NULL_GOTO(ev, end);
   EINA_SAFETY_ON_NULL_GOTO(am_bd, end);

   e_win = am_bd->icon.win;
   EINA_SAFETY_ON_NULL_GOTO(e_win, end);

   bd = e_win->border;

   if ((ev->event_window != e_win->evas_win) &&
       (ev->window != e_win->evas_win)) goto end;

   if (!am_bd->icon.moving && am_bd->icon.mouse.pressed)
     {
        l = sqrt(pow((float)(am_bd->icon.mouse.last_down.x - ev->root.x), 2) +
                 pow((float)(am_bd->icon.mouse.last_down.y - ev->root.y), 2));
        if (l >= 30.0f)
           am_bd->icon.moving = 1;
     }

     if (am_bd->icon.moving)
       _policy_assistant_menu_pinwin_pos_set(am_bd,ev->root.x,ev->root.y);

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_assistant_menu_cb_pinwin_mouse_up(void  *data,
                                          int   type __UNUSED__,
                                          void  *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_Illume_Assistant_Menu_Border *am_bd = (E_Illume_Assistant_Menu_Border *)data;
   E_Win *e_win;
   E_Border *bd;

   EINA_SAFETY_ON_NULL_GOTO(ev, end);
   EINA_SAFETY_ON_NULL_GOTO(am_bd, end);

   e_win = am_bd->icon.win;
   EINA_SAFETY_ON_NULL_GOTO(e_win, end);

   bd = e_win->border;

   if (ev->buttons != 1)      goto end;
   if ((ev->event_window != e_win->evas_win) &&
       (ev->window != e_win->evas_win) &&
       (ev->event_window != bd->win)) goto end;

   e_grabinput_release(bd->win, bd->win);
   if (am_bd->icon.moving)
      edje_object_signal_emit(am_bd->icon.edje_icon_layout, "pin.losefocus", "illume2");
   else
     {
        edje_object_signal_emit(am_bd->icon.edje_icon_layout, "pin.losefocus", "illume2");
        edje_object_signal_emit(am_bd->icon.edje_icon_layout, "pin.sound", "illume2");
        _policy_assistant_menu_pinoff(am_bd);
     }

   edje_object_signal_emit(am_bd->icon.edje_icon_layout, "pin.losefocus", "illume2");
   edje_object_signal_emit(am_bd->icon.edje_icon_layout, "pin.sound", "illume2");
   am_bd->icon.moving = 0;
   am_bd->icon.mouse.pressed = 0;
   am_bd->icon.mouse.released = 1;
   _policy_assistant_menu_pinwin_pos_set(am_bd,ev->root.x,ev->root.y);

end:
   return ECORE_CALLBACK_PASS_ON;
}

static void
_policy_assistant_menu_pinwin_pos_set(E_Illume_Assistant_Menu_Border *am_bd,int x,int y)
{
   E_Border *bd;
   E_Border *kbd_bd;
   E_Win *e_win;
   int endx,endy,disx,disy;
   int up_x,up_y;
   int move_x ,move_y;

   E_CHECK(am_bd);
   e_win = am_bd->icon.win;
   E_CHECK(e_win);
   bd = e_win->border;
   E_CHECK(bd);

   if (am_bd->icon.moving)
     {
        if (am_bd->icon.keyboard_show)
          {
             move_x = x - (ASSISTANT_MENU_ICON_SIZE_W/2);
             move_y = y - (ASSISTANT_MENU_ICON_SIZE_H/2);

             kbd_bd = e_illume_border_keyboard_get(bd->zone);
             if (!kbd_bd) return;

             switch (kbd_bd->client.e.state.rot.ang.curr)
               {
                case 0:
                case 180:
                   if ((move_y + ASSISTANT_MENU_ICON_SIZE_H) > kbd_bd->y )
                      move_y = kbd_bd->y - ASSISTANT_MENU_ICON_SIZE_H;
                   break;
                case 90:
                   if ((move_x + ASSISTANT_MENU_ICON_SIZE_W) > kbd_bd->x )
                      move_x = kbd_bd->x  - ASSISTANT_MENU_ICON_SIZE_W;
                   break;
                case 270:
                   if (move_x  < kbd_bd->w)
                      move_x = kbd_bd->w;
                   break;
               }
          }
        else
          {
             move_x = x - (ASSISTANT_MENU_ICON_SIZE_W/2);
             move_y = y - (ASSISTANT_MENU_ICON_SIZE_H/2);
          }
        e_border_move(bd, move_x, move_y);
     }
   else
     {
        up_x = bd->x;
        up_y = bd->y;

        disx = up_x % ASSISTANT_MENU_ICON_GRID_X;
        disy = up_y % ASSISTANT_MENU_ICON_GRID_Y;

        if (disx <= (ASSISTANT_MENU_ICON_GRID_X / 2))
          {
             endx =  up_x - disx;
          }
        else
          {
             endx = up_x - disx + ASSISTANT_MENU_ICON_GRID_X;
          }

        if (disy <= (ASSISTANT_MENU_ICON_GRID_Y / 2))
          {
             endy =  up_y - disy;
          }
        else
          {
             endy = up_y - disy + ASSISTANT_MENU_ICON_GRID_Y;
          }

        if (am_bd->icon.keyboard_show)
          {
             kbd_bd = e_illume_border_keyboard_get(bd->zone);
             if (!kbd_bd) return;

             switch (kbd_bd->client.e.state.rot.ang.curr)
               {
                case 0:
                case 180:
                   if ((endy + ASSISTANT_MENU_ICON_SIZE_H) > kbd_bd->y )
                     {
                        endy = kbd_bd->y - ASSISTANT_MENU_ICON_SIZE_H;
                        up_y = endy;
                     }
                   break;
                case 90:
                   if ((endx + ASSISTANT_MENU_ICON_SIZE_W) > kbd_bd->x )
                     {
                        endx = kbd_bd->x  - ASSISTANT_MENU_ICON_SIZE_W;
                        up_x = endx;
                     }
                   break;
                case 270:
                   if (endx < kbd_bd->w)
                     {
                        endx = kbd_bd->w;
                        up_x = endx;
                     }
               }
          }
        if (am_bd->icon.mouse.released)
           _policy_assistant_menu_pinon_start_rearrange_animation((void*) am_bd,endx,endy,up_x,up_y);
        else
           e_border_move(bd,endx,endy);
     }
}

static void
_policy_assistant_menu_pinon_start_rearrange_animation(void* data,int destx,int desty,int curx,int cury)
{
   E_Illume_Assistant_Menu_Animator *anim_data = malloc(sizeof(E_Illume_Assistant_Menu_Animator));

   if (!anim_data) return;

   anim_data->obj= data;
   anim_data->sx = curx;
   anim_data->sy = cury;
   anim_data->ex = destx;
   anim_data->ey = desty;
   anim_data->dx = anim_data->ex - anim_data->sx;
   anim_data->dy = anim_data->ey - anim_data->sy;
   anim_data->animator = ecore_animator_timeline_add(0.20,_policy_assistant_menu_pinon_rearrange_animation_cb,anim_data);
}

static Eina_Bool
_policy_assistant_menu_pinon_rearrange_animation_cb(void *data, double pos)
{
   E_Illume_Assistant_Menu_Animator *anim_data = (E_Illume_Assistant_Menu_Animator *)data;
   E_Illume_Assistant_Menu_Border *am_bd = (E_Illume_Assistant_Menu_Border *)anim_data->obj;
   int   x, y;
   double frame;
   E_Win *e_win;
   E_Border *bd;

   e_win = am_bd->icon.win;
   bd = e_win->border;

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;

   e_border_move(bd, x, y);

   if (pos >= 1.0)
     {
        ecore_animator_del(anim_data->animator);
        memset(anim_data, 0, sizeof(E_Illume_Assistant_Menu_Animator));
        free(anim_data);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
_policy_assistant_menu_pinwin_create(E_Illume_Assistant_Menu_Border *am_bd)
{
   Evas_Object *swallow_obj;
   Evas_Object *layout;
   E_Border *bd;
   E_Win *e_win;
   char buf[PATH_MAX];
   char win_name[PATH_MAX];

   EINA_SAFETY_ON_NULL_GOTO(am_bd, end);

   bd = am_bd->bd;
   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   if(!am_bd->pinon) goto end;

   e_win = e_win_new(bd->zone->container);
   if (!e_win) goto end;

   am_bd->icon.win = e_win;
   am_bd->icon.w = ASSISTANT_MENU_ICON_SIZE_W;  // hardcode's going to get size from layout
   am_bd->icon.h = ASSISTANT_MENU_ICON_SIZE_H;  // hardcode's going to get size from layout

   ecore_evas_alpha_set(e_win->ecore_evas, 1);
   e_win->evas_win = ecore_evas_window_get(e_win->ecore_evas);

   am_bd->icon.edje_icon_layout = edje_object_add(e_win->evas);
   layout = am_bd->icon.edje_icon_layout;

   memset(buf, 0x00, PATH_MAX);
   snprintf(buf, sizeof(buf), "%s/e-module-illume2-tizen.edj", _e_illume_mod_dir);
   EINA_SAFETY_ON_FALSE_GOTO(edje_object_file_set(layout, buf, "mw_minimize_mode"), end);
   evas_object_resize(layout, ASSISTANT_MENU_ICON_SIZE_W, ASSISTANT_MENU_ICON_SIZE_H);

   swallow_obj = e_border_icon_add(bd, e_win->evas);
   if(!swallow_obj)
     {
        memset(buf, 0x00, PATH_MAX);
        swallow_obj = edje_object_add(e_win->evas);
        snprintf(buf, sizeof(buf), "%s/e-module-illume2-tizen.edj", _e_illume_mod_dir);
        EINA_SAFETY_ON_FALSE_GOTO(edje_object_file_set(swallow_obj, buf, "default_icon"), end);
     }
   evas_object_resize(swallow_obj, ASSISTANT_MENU_ICON_SIZE_W-10, ASSISTANT_MENU_ICON_SIZE_H-10);

   edje_object_part_swallow( layout, "swallow_icon", swallow_obj);
   evas_object_show(layout);

   am_bd->icon.mouse.handlers.move = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                                             _policy_assistant_menu_cb_pinwin_mouse_move, am_bd);

   am_bd->icon.mouse.handlers.down = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                                             _policy_assistant_menu_cb_pinwin_mouse_down, am_bd);

   am_bd->icon.mouse.handlers.up = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                                           _policy_assistant_menu_cb_pinwin_mouse_up, am_bd);

   memset(win_name, 0x00, PATH_MAX);
   snprintf(win_name, sizeof(win_name), "Iconic - %s",  e_border_name_get(bd));
   e_win_title_set(e_win, win_name);
   e_win_name_class_set(e_win, "E", "ICON_WIN");
   e_win_resize(e_win, am_bd->icon.w, am_bd->icon.h);

  return EINA_TRUE; // successfully created

end:
   return EINA_FALSE;
}

static Eina_Bool
_policy_assistant_menu_pinwin_destroy(E_Illume_Assistant_Menu_Border *am_bd)
{
   E_Win *e_win;
   Ecore_Event_Handler *eh;

   e_win = am_bd->icon.win;
   if (!e_win) goto end;

   EINA_LIST_FREE (am_bd->handlers, eh)
      ecore_event_handler_del(eh);

   if (am_bd->icon.mouse.handlers.move != NULL)
     {
        ecore_event_handler_del(am_bd->icon.mouse.handlers.move);
        am_bd->icon.mouse.handlers.move = NULL;
     }

   if (am_bd->icon.mouse.handlers.down != NULL)
     {
        ecore_event_handler_del(am_bd->icon.mouse.handlers.down);
        am_bd->icon.mouse.handlers.down = NULL;
     }

   if (am_bd->icon.mouse.handlers.up != NULL)
     {
        ecore_event_handler_del(am_bd->icon.mouse.handlers.up);
        am_bd->icon.mouse.handlers.up = NULL;
     }

   e_object_del(E_OBJECT(e_win));
   am_bd->icon.win = NULL;

end:
   return ECORE_CALLBACK_PASS_ON;
}

static void
_policy_assistant_menu_pinon(E_Illume_Assistant_Menu_Border *am_bd)
{
   E_Border *bd;

   EINA_SAFETY_ON_NULL_RETURN(am_bd);

   bd = am_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!am_bd->pinon)
     {
        am_bd->pinon = 1;
        e_border_iconify(bd);

        // if window is being moved, should to cancel it.
        if (bd->cur_mouse_action)
          {
             if (bd->cur_mouse_action->func.end_mouse)
               bd->cur_mouse_action->func.end_mouse(E_OBJECT(bd), "", NULL);
             else if (bd->cur_mouse_action->func.end)
               bd->cur_mouse_action->func.end(E_OBJECT(bd), "");
             e_object_unref(E_OBJECT(bd->cur_mouse_action));
             bd->cur_mouse_action = NULL;
          }

        if (!am_bd->icon.win) _policy_assistant_menu_pinwin_create(am_bd);
        if (am_bd->icon.win)
          {
             int x, y;
             _policy_assistant_menu_pinon_xy_get(am_bd, &x, &y);
             e_win_move(am_bd->icon.win, x, y);
             e_win_show(am_bd->icon.win);
             ecore_x_icccm_hints_set(am_bd->icon.win->border->client.win, 0, 0, 0, 0, 0, 0, 0);
             am_bd->icon.win->border->client.icccm.accepts_focus = 0;
             e_border_raise(am_bd->icon.win->border);
             // for supporting dependent rotation to icon window.
             am_bd->icon.win->border->client.e.fetch.rot.need_rotation = EINA_TRUE;
          }
     }
}

static void
_policy_assistant_menu_pinoff(E_Illume_Assistant_Menu_Border *am_bd)
{
   E_Border *bd;
   E_Win *ewin;

   EINA_SAFETY_ON_NULL_RETURN(am_bd);
   bd = am_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (am_bd->pinon)
     {
        if (!policy_border_dep_rotation_set(bd))
          {
             int x = 0, y = 0;

             ewin = am_bd->icon.win;
             if (ewin) e_border_hide(ewin->border, 0);

             if (bd->iconic)
               {
                  e_border_raise(bd);
                  e_border_uniconify(bd);
               }
             else
               {
                  e_border_raise(bd);
                  e_border_show(bd);

                  if (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus)
                    e_border_focus_set(bd, 1, 1);
               }

             _policy_assistant_menu_pinoff_xy_get(am_bd, &x, &y);
             e_border_move(bd, x, y);

             ELBF(ELBT_ILLUME, 0, bd->client.win,
                  "PIN - OFF(by User) |  x:%d, y:%d",
                  x, y);

             am_bd->pinon = 0;
          }
        else
          {
             e_hints_window_visible_set(bd);
             am_bd->rot_done_wait = 1;
          }
     }
}

static void
_policy_assistant_menu_pinon_xy_get(E_Illume_Assistant_Menu_Border *am_bd, int *x, int *y)
{
   E_Border *bd;
   int rot;
   int max_x, max_y;
   int e_x, e_y;

   EINA_SAFETY_ON_NULL_RETURN(am_bd);

   bd = am_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_border_rotation_is_progress(bd))
     rot = e_border_rotation_next_angle_get(bd);
   else
     rot = e_border_rotation_curr_angle_get(bd);

   // let's adjust abnormal point
   switch(rot)
     {
        case 90:
           e_x = bd->x;
           e_y = bd->y + bd->h - am_bd->icon.h;
           break;

        case 180:
           e_x = bd->x - (bd->h - am_bd->icon.h);
           e_y = bd->y - (bd->w - am_bd->icon.w);
           break;

        case 270:
           e_x = bd->x + bd->w - am_bd->icon.w;
           e_y = bd->y;
           break;

        case 0:
        default:
           e_x = bd->x;
           e_y = bd->y;
           break;
     }

   // Icons are located in the screen area.
   max_x = bd->zone->w - am_bd->icon.w;
   max_y = bd->zone->h - am_bd->icon.h;
   if(e_x < 0) e_x = 0;
   if(e_y < 0) e_y = 0;
   if(e_x > max_x) e_x = max_x;
   if(e_y > max_y) e_y = max_y;

   if (x) *x = e_x;
   if (y) *y = e_y;
}

static void
_policy_assistant_menu_pinoff_xy_get(E_Illume_Assistant_Menu_Border *am_bd, int *x, int *y)
{
   E_Border *bd, *e_bd;
   int wx, wy, bd_w, bd_h;
   int max_x, max_y;
   int diff_view, ang1, ang2;
   E_Win *e_win;

   EINA_SAFETY_ON_NULL_RETURN(am_bd);

   bd = am_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   e_win = am_bd->icon.win;
   EINA_SAFETY_ON_NULL_RETURN(e_win);

   e_bd = e_win->border;
   E_OBJECT_CHECK(e_bd);
   E_OBJECT_TYPE_CHECK(e_bd, E_BORDER_TYPE);

   // if bd is hidden, its values are not updated according to curr rotation.
   // calculate bd->w, bd->h that is going to be shown if landscape or portrait view has changed.
   ang1 = e_border_rotation_curr_angle_get(e_bd);
   ang2 = e_border_rotation_curr_angle_get(bd);
   diff_view = ((ang1 == 90 || ang1 == 270) ? 1 : 0) ^ ((ang2 == 90 || ang2 == 270) ? 1 : 0);
   if(diff_view)
     {
        bd_w =  bd->h;
        bd_h =  bd->w;
     }
   else
     {
        bd_w =  bd->w;
        bd_h =  bd->h;
     }
   switch(e_border_rotation_curr_angle_get(e_bd))
     {
      case 90:
         wx = e_bd->x;
         wy = e_bd->y + e_bd->h - bd_h;
         break;

      case 180:
         wx = e_bd->x - (bd_w - e_bd->w);
         wy = e_bd->y - (bd_h - e_bd->h);
         break;

      case 270:
         wx = e_bd->x + e_bd->w - bd_w;
         wy = e_bd->y;
         break;

      case 0:
      default:
         wx = e_bd->x;
         wy = e_bd->y;
         break;
     }

   // assistant_menu windows are located in the screen area.
   max_x = bd->zone->w - bd_w;
   max_y = bd->zone->h - bd_h;
   if(wx < 0) wx = 0;
   if(wy < 0) wy = 0;
   if(wx > max_x) wx = max_x;
   if(wy > max_y) wy = max_y;

   if (x) *x = wx;
   if (y) *y = wy;

}

static int
__sort_cmp(const void *v1, const void *v2)
{
   return (*((int *)v1)) - (*((int *)v2));
}

static int
__sort_cmp_reverse(const void *v1, const void *v2)
{
   return (*((int *)v2)) - (*((int *)v1));
}

static int
__place_coverage_border_add(E_Desk *desk, Eina_List *skiplist, int ar, int x, int y, int w, int h)
{
   Eina_List *l, *ll;
   E_Border *bd, *bd2, *ewin_bd;
   E_Illume_Assistant_Menu_Border *am_bd;
   int x2, y2, w2, h2;
   int ok;
   int iw, ih;
   int x0, x00, yy0, y00;

   if (!desk) return 0;
   if (!desk->zone) return 0;

   EINA_LIST_FOREACH(assistant_menu_wins, l, am_bd)
     {
        bd = am_bd->bd;
        if (!bd) continue;

        ok = 1;
        EINA_LIST_FOREACH(skiplist, ll, bd2)
          {
             if (bd2 == bd)
               {
                  ok = 0;
                  break;
               }
          }
        if ((!bd->iconic) && (bd->visible))
          {
            // to calculate intersaction with multi-window
            x2 = (bd->x - desk->zone->x);
            y2 = (bd->y - desk->zone->y);
            w2 = bd->w;
            h2 = bd->h;
          }
        else if ((!am_bd->hide) && (am_bd->pinon))
          {
            // to calculate intersaction with E_Win(minimized multi-window)
            ewin_bd = NULL;
            if (am_bd->icon.win) ewin_bd = am_bd->icon.win->border;
            if (!ewin_bd) continue;

            x2 = (ewin_bd->x - desk->zone->x);
            y2 = (ewin_bd->y - desk->zone->y);
            w2 = ewin_bd->w;
            h2 = ewin_bd->h;
          }
        else
          {
             continue;
          }


        if ((ok) &&
            (E_INTERSECTS(x, y, w, h, x2, y2, w2, h2)) &&
            ((bd->sticky) || (bd->desk == desk)))
          {
             x0 = x;
             if (x < x2) x0 = x2;
             x00 = (x + w);
             if ((x2 + w2) < (x + w)) x00 = (x2 + w2);
             yy0 = y;
             if (y < y2) yy0 = y2;
             y00 = (y + h);
             if ((y2 + h2) < (y + h)) y00 = (y2 + h2);
             iw = x00 - x0;
             ih = y00 - yy0;
             ar += (iw * ih);
          }
     }
   return ar;
}

static void
__place_get_candidate_positions(E_Desk *desk, Eina_List *skiplist,
                                int *a_w, int *a_h, int **a_x, int **a_y)
{
   int *x, *y;
   char *u_x = NULL, *u_y = NULL;
   Eina_List *l, *ll;
   E_Border *bd, *bd2;
   E_Illume_Assistant_Menu_Border *am_bd;
   int zw, zh;
   int a_alloc_w = 0, a_alloc_h = 0;

   if (!desk) return;
   if ((!a_w) || (!a_h)) return;
   if ((!a_x) || (!a_y)) return;

   zw = desk->zone->w;
   zh = desk->zone->h;

   x = *a_x;
   y = *a_y;
   a_alloc_w = *a_w;
   a_alloc_h = *a_h;

   u_x = calloc((zw) + 1, sizeof(char));
   u_y = calloc((zh) + 1, sizeof(char));

   if (u_x)
     {
        u_x[x[0]] = 1;
        u_x[x[1]] = 1;
     }
   if (u_y)
     {
        u_y[y[0]] = 1;
        u_y[y[1]] = 1;
     }
   // Make the array that is candidate position which is assistant_menu window could be placed.
   EINA_LIST_FOREACH(assistant_menu_wins, l, am_bd)
     {
        int ok;
        int bx, by, bw, bh;

        bd = am_bd->bd;
        if (!bd) continue;

        ok = 1;
        EINA_LIST_FOREACH(skiplist, ll, bd2)
          {
             if (bd2 == bd)
               {
                  ok = 0;
                  break;
               }
          }
        if (!ok) continue;

        if (!bd->visible)
          {
             if ((am_bd->pinon) && (am_bd->icon.win) && (am_bd->icon.win->border))
               bd = am_bd->icon.win->border;
             else continue;
          }

        if (!((bd->sticky) || (bd->desk == desk))) continue;

        bx = bd->x - desk->zone->x;
        by = bd->y - desk->zone->y;
        bw = bd->w;
        bh = bd->h;

        if (E_INTERSECTS(bx, by, bw, bh, 0, 0, zw, zh))
          {
             if (bx < x[0])
               {
                  if (bx < 0) bw += (bx - x[0]);
                  else bw -= (x[0] - bx);
                  bx = x[0];
               }
             if ((bx + bw) > x[1]) bw = x[1] - bx;
             if (bx >= x[1]) continue;
             if (by < y[0])
               {
                  if (by < 0) bh += (by - y[0]);
                  else bh -= (y[0] - by);
                  by = y[0];
               }
             if ((by + bh) > y[1]) bh = y[1] - by;
             if (by >= y[1]) continue;
             if (!u_x[bx])
               {
                  *a_w = *a_w + 1;
                  if (*a_w > a_alloc_w)
                    {
                       a_alloc_w += 32;
                       x = *a_x = (int *)realloc(*a_x, sizeof(int) * a_alloc_w);
                    }
                  if (x)
                    x[(*a_w) - 1] = bx;
                  u_x[bx] = 1;
               }
             if (!u_x[bx + bw])
               {
                  *a_w = *a_w + 1;
                  if (*a_w > a_alloc_w)
                    {
                       a_alloc_w += 32;
                       x = *a_x = (int *)realloc(*a_x, sizeof(int) * a_alloc_w);
                    }
                  if (x)
                    x[(*a_w) - 1] = bx + bw;
                  u_x[bx + bw] = 1;
               }
             if (!u_y[by])
               {
                  *a_h = *a_h + 1;
                  if (*a_h > a_alloc_h)
                    {
                       a_alloc_h += 32;
                       y = *a_y = (int *)realloc(*a_y, sizeof(int) * a_alloc_h);
                    }
                  if (y)
                    y[(*a_h) - 1] = by;
                  u_y[by] = 1;
               }
             if (!u_y[by + bh])
               {
                  *a_h = *a_h + 1;
                  if (*a_h > a_alloc_h)
                    {
                       a_alloc_h += 32;
                       y = *a_y = (int *)realloc(*a_y, sizeof(int) * a_alloc_h);
                    }
                  if (y)
                    y[(*a_h) - 1] = by + bh;
                  u_y[by + bh] = 1;
               }
          }
     }
   free(u_x);
   free(u_y);
}

static void
__place_get_widest_area_position(E_Desk *desk, Eina_List *skiplist, int rotation,
                                 int w, int h, Evas_Coord_Point min, Evas_Coord_Point max,
                                 int a_w, int a_h, int *a_x, int *a_y,
                                 int *rx, int *ry)
{
   Evas_Coord_Point tr, tl, br, bl;
   int index1, index2;
   int i, j;
   int area = 0x7fffffff;

   if (!desk) return;
   if ((!a_x) || (!a_y)) return;
   if ((!rx) || (!ry)) return;

   switch (rotation)
     {
      case 270:
         index1 = a_w;
         index2 = a_h;
         qsort(a_x, a_w, sizeof(int), __sort_cmp_reverse);
         qsort(a_y, a_h, sizeof(int), __sort_cmp);
         break;
      case 90:
         index1 = a_w;
         index2 = a_h;
         qsort(a_x, a_w, sizeof(int), __sort_cmp);
         qsort(a_y, a_h, sizeof(int), __sort_cmp_reverse);
         break;
      default:
      case 0:
         index1 = a_h;
         index2 = a_w;
         qsort(a_x, a_w, sizeof(int), __sort_cmp);
         qsort(a_y, a_h, sizeof(int), __sort_cmp);
         break;
     }

   for (j = 0; j < index1 - 1; j++)
     {
        for (i = 0; i < index2 - 1; i++)
          {
             switch (rotation)
               {
                case 90:
                   tl.x = a_x[j];          tl.y = a_y[i] - h;
                   tr.x = a_x[j];          tr.y = a_y[i + 1];
                   br.x = a_x[j + 1] - w;  br.y = a_y[i + 1];
                   bl.x = a_x[j + 1] - w;  bl.y = a_y[i] - h;
                   break;
                case 270:
                   tl.x = a_x[j] - w;      tl.y = a_y[i];
                   tr.x = a_x[j] - w;      tr.y = a_y[i + 1] - h;
                   br.x = a_x[j + 1];      br.y = a_y[i + 1] - h;
                   bl.x = a_x[j + 1];      bl.y = a_y[i];
                   break;
                default:
                case 0:
                   tl.x = a_x[i];          tl.y = a_y[j];
                   tr.x = a_x[i + 1] - w;  tr.y = a_y[j];
                   br.x = a_x[i + 1] - w;  br.y = a_y[j + 1] - h;
                   bl.x = a_x[i];          bl.y = a_y[j + 1] - h;
                   break;
               }

             if ((tl.x >= min.x) && (tl.x <= max.x) &&
                 (tl.y >= min.y) && (tl.y <= max.y))
               {
                  int ar = 0;

                  ar = __place_coverage_border_add(desk, skiplist, ar,
                                                   tl.x, tl.y, w, h);
                  if (ar < area)
                    {
                       area = ar;
                       *rx = tl.x;
                       *ry = tl.y;
                       if (ar == 0) goto done;
                    }
               }
             if ((tr.x >= min.x) && (tr.x <= max.x) &&
                 (tr.y >= min.y) && (tr.y <= max.y))
               {
                  int ar = 0;

                  ar = __place_coverage_border_add(desk, skiplist, ar,
                                                   tr.x, tr.y, w, h);
                  if (ar < area)
                    {
                       area = ar;
                       *rx = tr.x;
                       *ry = tr.y;
                       if (ar == 0) goto done;
                    }
               }
             if ((br.x >= min.x) && (br.x <= max.x) &&
                 (br.y >= min.y) && (br.y <= max.y))
               {
                  int ar = 0;

                  ar = __place_coverage_border_add(desk, skiplist, ar,
                                                   br.x, br.y, w, h);
                  if (ar < area)
                    {
                       area = ar;
                       *rx = br.x;
                       *ry = br.y;
                       if (ar == 0) goto done;
                    }
               }
             if ((bl.x >= min.x) && (bl.x <= max.x) &&
                 (bl.y >= min.y) && (bl.y <= max.y))
               {
                  int ar = 0;

                  ar = __place_coverage_border_add(desk, skiplist, ar,
                                                   bl.x, bl.y, w, h);
                  if (ar < area)
                    {
                       area = ar;
                       *rx = bl.x;
                       *ry = bl.y;
                       if (ar == 0) goto done;
                    }
               }
          }
     }

done:
   if ((*rx + w) > desk->zone->w) *rx = desk->zone->w - w;
   if (*rx < 0) *rx = 0;
   if ((*ry + h) > desk->zone->h) *ry = desk->zone->h - h;
   if (*ry < 0) *ry = 0;
}

static void
_policy_assistant_menu_border_placement(E_Border *bd)
{
   E_Illume_Assistant_Menu_Border *am_bd;
   Eina_List *skiplist = NULL;
   int new_x, new_y, old_x, old_y;
   int rotation = 0;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
   E_CHECK(!e_object_is_del(E_OBJECT(bd)));
   E_CHECK(am_bd);
   E_CHECK(!am_bd->placed);

   if (bd->client.icccm.request_pos)
     {
        ELB(ELBT_ILLUME, "[PLACEMENT] PASS BY REQUEST_POS", bd->client.win);
        am_bd->placed = 1;
        return;
     }

   skiplist = eina_list_append(skiplist, bd);
   rotation = e_border_rotation_curr_angle_get(bd);
   old_x = new_x = bd->x;
   old_y = new_y = bd->y;

   _policy_assistant_menu_place_region_smart(bd->desk, skiplist,
                                       bd->x, bd->y, bd->w, bd->h, rotation,
                                       &new_x, &new_y);

   e_border_move(bd, new_x, new_y);
   am_bd->placed = 1;

   ELBF(ELBT_ILLUME, 0, bd->client.win,
        "[PLACEMENT] win:0x%08x, rot:%d, size:%dx%d, (%d,%d) => (%d,%d)",
        bd->client.win, rotation, bd->w, bd->h, old_x, old_y, new_x, new_y);
}

/*
 * Get the widest position that is not occupied by another window.
 */
static int
_policy_assistant_menu_place_region_smart(E_Desk *desk, Eina_List *skiplist,
                                   int x, int y, int w, int h, int rotation,
                                   int *rx, int *ry)
{
   int *a_x = NULL, *a_y = NULL;
   int a_w = 0, a_h = 0;
   int zw, zh;
   Evas_Coord_Point min, max;

   const int margin = 15, indi_region = 60;

   if (!desk) return 0;
   if ((!rx) || (!ry)) return 0;
   if ((w <= 0) || (h <= 0))
     {
        ELB(ELBT_ILLUME, "ERROR: Trying to place 0x0 window!!!", (unsigned int)NULL);
        return 0;
     }

   *rx = x;
   *ry = y;

   zw = desk->zone->w;
   zh = desk->zone->h;

   a_x = E_NEW(int, 2);
   a_y = E_NEW(int, 2);
   if (!a_x || !a_y)
     {
        E_FREE(a_x);
        E_FREE(a_y);
        return 0;
     }

   a_w = 2;
   a_h = 2;

   switch (rotation)
     {
      case 90:
         a_x[0] = indi_region;
         a_y[0] = margin;
         a_x[1] = zw - margin;
         a_y[1] = zh - margin;
         break;
      case 270:
         a_x[0] = margin;
         a_y[0] = margin;
         a_x[1] = zw - indi_region;
         a_y[1] = zh - margin;
         break;
      default:
      case 0:
         a_x[0] = margin;
         a_y[0] = indi_region;
         a_x[1] = zw - margin;
         a_y[1] = zh - margin;
         break;
     }

   min.x = a_x[0];
   min.y = a_y[0];
   max.x = a_x[1] - w;
   max.y = a_y[1] - h;

   // Step 1. Make the array that is candidate position which is assistant_menu window could be placed.
   __place_get_candidate_positions(desk, skiplist, &a_w, &a_h, &a_x, &a_y);

   // Step 2. Retrieves the widest position that is not occupied by another window.
   __place_get_widest_area_position(desk, skiplist, rotation, w, h, min, max,
                                    a_w, a_h, a_x, a_y, rx, ry);

   E_FREE(a_x);
   E_FREE(a_y);

   *rx += desk->zone->x;
   *ry += desk->zone->y;
   return 1;
}

static void
__coord_transform_rot_space(E_Zone *zone, Eina_Rectangle geo, int rotation,
                            int *zw, int *zh, int *w, int *h, int *x, int *y)
{
   int tzw, tzh, tw, th, tx, ty;

   switch (rotation)
     {
      case 90:
         tzw = zone->h;
         tzh = zone->w;
         tw = geo.h;
         th = geo.w;
         tx = tzw - (geo.y + tw);
         ty = geo.x;
         break;
      case 180:
         tzw = zone->w;
         tzh = zone->h;
         tw = geo.w;
         th = geo.h;
         tx = tzw - (geo.x + tw);
         ty = tzh - (geo.y + th);
         break;
      case 270:
         tzw = zone->h;
         tzh = zone->w;
         tw = geo.h;
         th = geo.w;
         tx = geo.y;
         ty = tzh - (geo.x + th);
         break;
      default:
      case 0:
         tzw = zone->w;
         tzh = zone->h;
         tx = geo.x;
         ty = geo.y;
         tw = geo.w;
         th = geo.h;
         break;
     }

   if (zw) *zw = tzw;
   if (zh) *zh = tzh;
   if (w) *w = tw;
   if (h) *h = th;
   if (x) *x = tx;
   if (y) *y = ty;
}

static void
__coord_transform_none_rot_space(E_Zone *zone, int w, int h, int rotation, int *x, int *y)
{
   int new_x, new_y;
   int temp_x, temp_y;

   if (x) temp_x = *x;
   else temp_x = 0;

   if (y) temp_y = *y;
   else temp_y = 0;

   switch (rotation)
     {
      case 90:
         new_x = temp_y;
         new_y= zone->h - w - temp_x;
         break;
      case 270:
         new_x = zone->w - h - temp_y;
         new_y = temp_x;
         break;
      case 180:
         new_x = zone->w - w - temp_x;
         new_y = zone->h - h - temp_y;
         break;
      default:
      case 0:
         new_x = temp_x;
         new_y = temp_y;
         break;
     }

   if (x) *x = new_x;
   if (y) *y = new_y;
}

static Eina_Bool
_policy_assistant_menu_rotation_change_pos(E_Border *bd, int *x, int *y)
{
   E_Zone *zone;
   E_Illume_Assistant_Menu_Border *am_bd = NULL;
   int zw, zh, next_zw, next_zh;
   int prev_rot, next_rot;
   int wx, wy, ww, wh, out_ww = 0, out_wh = 0;
   int next_w_space, next_h_space;
   int new_x = 0, new_y = 0;
   float upper_space, lower_space;
   float left_space, right_space;
   float x_axis_rate, y_axis_rate;
   Eina_Rectangle prev_geo, next_geo;

   if (!bd) return EINA_FALSE;
   if ((!x) || (!y)) return EINA_FALSE;

   zone = bd->zone;

   if (e_illume_border_is_icon_win(bd))
     {
        switch (bd->client.e.state.rot.ang.curr)
          {
           case 0:
           case 180:
              new_x = ASSISTANT_MENU_ICON_GRID_X * 3;
              new_y = ASSISTANT_MENU_ICON_GRID_Y * 4;
              break;
           case 90:
              new_x = ASSISTANT_MENU_ICON_GRID_X * 2;
              new_y = 0;
              break;
           case 270:
              new_x = ASSISTANT_MENU_ICON_GRID_X;
              new_y = ASSISTANT_MENU_ICON_GRID_Y * 5;
              break;
          }

        *x = new_x;
        *y = new_y;

        return EINA_TRUE;
     }
   else
     {
        am_bd = _policy_assistant_menu_get_assistant_menu_border(bd->win);
        if (!am_bd) return EINA_FALSE;

        prev_geo.x = am_bd->rot.geo.prev.x;
        prev_geo.y = am_bd->rot.geo.prev.y;
        prev_geo.w = am_bd->rot.geo.prev.w;
        prev_geo.h = am_bd->rot.geo.prev.h;
        next_geo.x = am_bd->rot.geo.curr.x;
        next_geo.y = am_bd->rot.geo.curr.y;
        next_geo.w = am_bd->rot.geo.curr.w;
        next_geo.h = am_bd->rot.geo.curr.h;
     }

   if (e_border_rotation_is_progress(bd))
     {
        prev_rot = e_border_rotation_curr_angle_get(bd);
        next_rot = e_border_rotation_next_angle_get(bd);
     }
   else
     {
        prev_rot = e_border_rotation_prev_angle_get(bd);
        next_rot = e_border_rotation_curr_angle_get(bd);
     }
   if (prev_rot == next_rot) return EINA_FALSE;

   // coordinate transformation to rotation space
   __coord_transform_rot_space(zone, prev_geo, prev_rot, &zw, &zh, &ww, &wh, &wx, &wy);

   // get the size out of screen.
   if (wx < 0)
     out_ww = wx;
   else if ((wx + ww) > zw)
     out_ww = (wx + ww) - zw;
   if (wy < 0)
     out_wh = wy;
   else if ((wy + wh) > zh)
     out_wh = (wy + wh) - zh;

   // get the space in rest of screen.
   upper_space = wy;
   lower_space = zh - wy - wh;
   left_space = wx;
   right_space = zw - wx - ww;

   // get the rate using each space
   if ((left_space <= 0) && (right_space <=0))
     x_axis_rate = 0.5;
   else if (left_space <= 0)
     x_axis_rate = 0;
   else if (right_space <= 0)
     x_axis_rate = 1;
   else
     x_axis_rate = left_space / (left_space + right_space);

   if ((upper_space <= 0) && (lower_space <= 0))
     y_axis_rate = 0.5;
   else if (upper_space <= 0)
     y_axis_rate = 0;
   else if (lower_space <= 0)
     y_axis_rate = 1;
   else
     y_axis_rate = upper_space / (upper_space + lower_space);

   switch (next_rot)
     {
      case 90:
      case 270:
         next_zw = zone->h;
         next_zh = zone->w;
         out_ww = round((float)out_ww * ((float)next_geo.h / (float)ww));
         out_wh = round((float)out_wh * ((float)next_geo.w / (float)wh));
         ww = next_geo.h;
         wh = next_geo.w;
         break;
      default:
      case 0:
      case 180:
         next_zw = zone->w;
         next_zh = zone->h;
         out_ww = round((float)out_ww * ((float)next_geo.w / (float)ww));
         out_wh = round((float)out_wh * ((float)next_geo.h / (float)wh));
         ww = next_geo.w;
         wh = next_geo.h;
         break;
     }

   next_w_space = next_zw - ww;
   next_h_space = next_zh - wh;

   // get the newly coordination.
   if (next_w_space <= 0)
     new_x = 0;
   else
     new_x = round((float)next_w_space * x_axis_rate);
   if (next_h_space <= 0)
     new_y = 0;
   else
     new_y = round((float)next_h_space * y_axis_rate);

   new_x += out_ww;
   new_y += out_wh;

   // coordinate transformation to none rotation space
   __coord_transform_none_rot_space(bd->zone, ww, wh, next_rot, &new_x, &new_y);

   ELBF(ELBT_ROT, 0, bd->client.win,
        "Floating Mode. ANGLE (%d->%d), POS (%d,%d) -> (%d,%d)",
        prev_rot, next_rot, bd->x, bd->y, new_x, new_y);
   ELBF(ELBT_ROT, 0, bd->client.win,
        "\tBASE prev: (%d %d) (%dx%d), curr (%dx%d)",
        prev_geo.x, prev_geo.y, prev_geo.w, prev_geo.h,
        next_geo.w, next_geo.h);


   if (x) *x = new_x;
   if (y) *y = new_y;

   return EINA_TRUE;
}
