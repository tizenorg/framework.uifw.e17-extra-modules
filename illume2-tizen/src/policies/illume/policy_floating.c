#include "e_illume_private.h"
#include "policy.h"
#include "policy_floating.h"

/* resourceD */
#define PROCESS_FOREGROUND 3

/* global event callback function */
static Eina_Bool  _policy_floating_cb_client_message(void   *data, int    type, void   *event);
static Eina_Bool  _policy_floating_cb_bd_resize(void *data, int type, void *event);
static Eina_Bool  _policy_floating_cb_bd_move(void *data, int type, void *event);
static Eina_Bool  _policy_floating_cb_bd_rot_begin(void *data, int type __UNUSED__, void *event);
static Eina_Bool  _policy_floating_cb_bd_rot_end(void *data, int type __UNUSED__, void *event);
static void       _policy_floating_cb_hook_eval_pre_new_border(void *data __UNUSED__, void *data2);
static void       _policy_floating_cb_hook_eval_post_new_border(void *data __UNUSED__, void *data2);

/* general function */
static E_Illume_Floating_Border* _policy_floating_border_list_add(E_Border *bd);
static void                      _policy_floating_border_list_del(E_Border *bd);
static E_Illume_Floating_Border* _policy_floating_get_floating_border(Ecore_X_Window win);

static int        _policy_floating_atom_init(void);
static void       _policy_hints_floating_list_set(void);
/* for close the floating windows */
static void       _policy_floating_close_all(void);
/* for iconify the floating windows */
static void       _policy_floating_iconify(E_Illume_Floating_Border *ft_bd, Eina_Bool iconify);
static void       _policy_floating_iconify_all(Eina_Bool iconify);
static void       _policy_floating_minimize_all(Eina_Bool minimize);

/* for iconify the floating windows */
static Eina_Bool  _policy_floating_cb_pinwin_mouse_move(void   *data, int    type __UNUSED__, void   *event);
static Eina_Bool  _policy_floating_cb_pinwin_mouse_down(void   *data, int    type __UNUSED__, void   *event);
static Eina_Bool  _policy_floating_cb_pinwin_mouse_up(void   *data, int    type __UNUSED__, void   *event);
static Eina_Bool  _policy_floating_pinwin_create(E_Illume_Floating_Border *ft_bd);
static Eina_Bool  _policy_floating_pinwin_destroy(E_Illume_Floating_Border *ft_bd);

static void       _policy_floating_pinoff(E_Illume_Floating_Border *ft_bd);
static void       _policy_floating_pinon(E_Illume_Floating_Border *ft_bd);
static void       _policy_floating_pinon_xy_get(E_Illume_Floating_Border *ft_bd, int *x, int *y);
static void       _policy_floating_pinoff_xy_get(E_Illume_Floating_Border *ft_bd, int *x, int *y);
static void       _policy_floating_pinwin_iconify(E_Illume_Floating_Border *ft_bd, Eina_Bool iconify);

static void       _policy_floating_active_request(Ecore_X_Event_Client_Message *event);

/* for automatically align the floating windows */
static void       _policy_floating_border_placement(E_Border *bd);
static void       _policy_floating_smart_cleanup(Ecore_X_Event_Client_Message *event);
static int        _policy_floating_place_region_smart(E_Desk *desk,
                                                      Eina_List *skiplist,
                                                      int x, int y, int w, int h, int rotation,
                                                      int *rx, int *ry);
static Eina_Bool  _policy_floating_rotation_change_pos(E_Border *bd, int *x, int *y);

/* to wake up freezing windows */
static void       _policy_floating_set_pid_fg(int pid);


/* for controlling app-in-app window */
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_LIST;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST;

static Eina_List *_fw_hdls;
static Eina_List *_fw_hooks;
static Ecore_Idle_Enterer *_idle_enterer;

static Eina_Hash *floating_wins_hash;
static Eina_List *floating_wins;

EINTERN Eina_Bool
policy_floating_init(void)
{
   Eina_Bool ret = EINA_FALSE;

   /* event handlers */
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                               _policy_floating_cb_client_message,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_RESIZE,
                                               _policy_floating_cb_bd_resize,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_MOVE,
                                               _policy_floating_cb_bd_move,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ROTATION_CHANGE_BEGIN,
                                               _policy_floating_cb_bd_rot_begin, NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ROTATION_CHANGE_END,
                                               _policy_floating_cb_bd_rot_end, NULL));

   /* hook functions */
   _fw_hooks =
      eina_list_append(_fw_hooks,
                       e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_NEW_BORDER,
                                         _policy_floating_cb_hook_eval_pre_new_border, NULL));

   _fw_hooks =
      eina_list_append(_fw_hooks,
                       e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER,
                                         _policy_floating_cb_hook_eval_post_new_border, NULL));

   ret = _policy_floating_atom_init();
   if (!ret)
     L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) Failed initializing atoms\n", __func__, __LINE__);

   if (!floating_wins_hash)
     floating_wins_hash = eina_hash_string_superfast_new(NULL);

   return ret;
}

EINTERN void
policy_floating_shutdown(void)
{
   Ecore_Event_Handler *hdl;
   E_Border_Hook *hook;

   EINA_LIST_FREE(_fw_hdls, hdl)
     ecore_event_handler_del(hdl);
   EINA_LIST_FREE(_fw_hooks, hook)
     e_border_hook_del(hook);

   if (_idle_enterer) ecore_idle_enterer_del(_idle_enterer);
   _idle_enterer = NULL;

   if (floating_wins_hash) eina_hash_free(floating_wins_hash);
   floating_wins_hash = NULL;

   if (floating_wins) eina_list_free(floating_wins);
   floating_wins = NULL;
}


EINTERN void
policy_floating_idle_enterer(void)
{
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = NULL;
   Eina_List *l;

   if (!floating_wins) return;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        if (!ft_bd) continue;
        if (!ft_bd->changed) continue;
        ft_bd->changed = 0;

        bd = ft_bd->bd;
        if (!bd) continue;

         L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) idle state, win: 0x%08x\n",
           __func__, __LINE__, bd->client.win);

        if (ft_bd->defer.close)
          {
             e_border_act_close_begin(ft_bd->bd);
             ft_bd->defer.close = 0;
          }
     }
}

EINTERN Eina_List*
policy_floating_get_window_list(void)
{
   return floating_wins;
}

EINTERN void
policy_floating_border_add(E_Border *bd)
{
   EINA_SAFETY_ON_NULL_RETURN(bd);
   E_CHECK(e_illume_border_is_floating(bd));

   if (bd->client.netwm.state.skip_taskbar)
      return;

   _policy_floating_border_list_add(bd);
}

EINTERN void
policy_floating_border_del(E_Border *bd)
{
   EINA_SAFETY_ON_NULL_RETURN(bd);

   // if "ECORE_X_ILLUME_WINDOW_STATE_FLOATING is set in this window,
   // we have to remove property of E_STACKING_ABOVE when its border is deleted.
   bd->client.netwm.state.stacking = E_STACKING_NONE;
   e_hints_window_state_set(bd);

   _policy_floating_border_list_del(bd);
}

EINTERN void
policy_floating_configure_request(Ecore_X_Event_Window_Configure_Request *ev)
{
   E_Illume_Floating_Border *ft_bd;

   ft_bd = _policy_floating_get_floating_border(ev->win);
   E_CHECK(ft_bd);

   if ((ev->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X) ||
       (ev->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y))
     {
        ELBF(ELBT_ILLUME, 0, ev->win,
             "[PLACEMENT] App(0x%08x) decides position by itself. => (%d,%d)",
             ev->win, ev->x, ev->y);
        ft_bd->placed = 1;
     }
}

static Eina_Bool
_policy_floating_check_window_state_change_scenario(Ecore_X_Illume_Window_State from, Ecore_X_Illume_Window_State to)
{
   Eina_Bool ret = EINA_FALSE;

   if( ((from==ECORE_X_ILLUME_WINDOW_STATE_FLOATING) && (to==ECORE_X_ILLUME_WINDOW_STATE_NORMAL))
        ||((from==ECORE_X_ILLUME_WINDOW_STATE_NORMAL) && (to==ECORE_X_ILLUME_WINDOW_STATE_FLOATING)) )
     {
        ret = EINA_TRUE;
     }

   return ret;
}

EINTERN void
policy_floating_window_state_change(E_Border *bd, unsigned int state)
{
   E_Border *indi_bd;
   E_Illume_Border_Info *bd_info;
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(bd);
   if (bd->client.illume.win_state.state == state) return;

   bd_info = policy_get_border_info(bd);
   E_CHECK(bd_info);

   ELBF(ELBT_ILLUME, 0, bd->client.win, "SET WIN_STATE %d->%d size: %dx%d",
        bd->client.illume.win_state.state, state, bd->w, bd->h);

   /**
    * We only deal with below scenario. Other case, just update win_state.state and return it.
    * ECORE_X_ILLUME_WINDOW_STATE_NORMAL -> ECORE_X_ILLUME_WINDOW_STATE_FLOATING
    * ECORE_X_ILLUME_WINDOW_STATE_FLOATING -> ECORE_X_ILLUME_WINDOW_STATE_NORMAL
    **/
   if ( !_policy_floating_check_window_state_change_scenario(bd->client.illume.win_state.state, state) ) return;

   bd->client.illume.win_state.state = state;

   switch(state)
     {
      case ECORE_X_ILLUME_WINDOW_STATE_FLOATING:

         ft_bd = _policy_floating_border_list_add(bd);

         bd_info->used_to_floating = EINA_TRUE;
         bd_info->allow_user_geometry = EINA_TRUE;

         bd->lock_user_size = 0;
         bd->lock_user_location = 0;
         bd->borderless = 0;

         e_hints_window_state_update(bd, ECORE_X_WINDOW_STATE_ABOVE,
                                     ECORE_X_WINDOW_STATE_ACTION_ADD);

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
              if (ft_bd)
                ft_bd->rot_done_wait = policy_border_dep_rotation_set(bd);
           }

         policy_border_illume_handlers_add(bd_info);

         E_Illume_XWin_Info* xwin_info;
         xwin_info = policy_xwin_info_find(bd->win);
         if (xwin_info)
           {
              if (bd->iconic)
                 xwin_info->iconify_by_wm = 0;

              xwin_info->wait_resize = EINA_TRUE;
           }

         break;
      case ECORE_X_ILLUME_WINDOW_STATE_NORMAL:

         e_hints_window_state_update(bd, ECORE_X_WINDOW_STATE_ABOVE, ECORE_X_WINDOW_STATE_ACTION_REMOVE);

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

         ft_bd = _policy_floating_get_floating_border(bd->win);
         if (ft_bd)
           {
              if (!bd->iconic && !bd->visible)
                {
                   e_border_iconify(bd);
                }
           }

         policy_border_dep_rotation_list_del(bd);
         _policy_floating_border_list_del(bd);
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
policy_floating_window_state_update(E_Border *bd,
                                    Ecore_X_Window_State        state,
                                    Ecore_X_Window_State_Action action)
{
   E_Illume_Floating_Border *ft_bd;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   ft_bd = _policy_floating_get_floating_border(bd->win);
   E_CHECK(ft_bd);

   switch (state)
     {
      case ECORE_X_WINDOW_STATE_ICONIFIED:
         if (action != ECORE_X_WINDOW_STATE_ACTION_ADD) return;
         if (bd->lock_client_iconify) return;

         //mini window (floating mode) is changed into icon with this event
         if (ft_bd->pinon) return;

         _policy_floating_pinon(ft_bd);
         _policy_hints_floating_list_set();
         break;

      default:
         break;
     }
}

EINTERN void
_policy_floating_check_max_size(E_Border *bd)
{
   int max;
   int w, h;

   if (!bd) return;
   if (e_border_rotation_is_progress(bd)) return;

   E_Illume_XWin_Info* xwin_info;
   xwin_info = policy_xwin_info_find(bd->win);
   if (xwin_info && xwin_info->wait_resize) return;

   w = bd->w;
   h = bd->h;

   if (bd->zone->w > bd->zone->h)
      max = bd->zone->h;
   else
      max = bd->zone->w;

   if (w > max) w = max;
   if (h > max) h = max;

   if ((w != bd->w) ||
       (h != bd->h))
      e_border_resize(bd, w, h);
}

#define SIZE_EQUAL_TO_ZONE(a, z) \
   ((((a)->w) == ((z)->w)) &&    \
    (((a)->h) == ((z)->h)))
EINTERN void
policy_zone_layout_floating(E_Border *bd)
{
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   ft_bd = _policy_floating_get_floating_border(bd->client.win);
   E_CHECK(ft_bd);

   _policy_floating_check_max_size(bd);

   if (!ft_bd->placed)
     {
        // workaound: we assume that the size of floating window is different with zone.
        // since there is no size information in the floating protocol,
        // we don't know the size of the window which try to change the mode to floating.
        // so we should check out size difference with zone to place the floating window.
        if ((bd->visible) &&
            (!SIZE_EQUAL_TO_ZONE(bd, bd->zone)) &&
            (!e_border_rotation_is_progress(bd)))
          _policy_floating_border_placement(bd);
     }
}

EINTERN void
policy_floating_icon_rotation(E_Border *bd, int rotation)
{
   int new_x, new_y;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (!e_illume_border_is_icon_win(bd)) return;
   if (e_border_rotation_curr_angle_get(bd) == rotation) return;

   ecore_evas_rotation_set(bd->internal_ecore_evas, rotation);

   bd->client.e.state.rot.ang.prev = bd->client.e.state.rot.ang.curr;
   bd->client.e.state.rot.ang.curr = rotation;

   if ((bd->visible) && (!bd->changes.visible))
     {
        if (_policy_floating_rotation_change_pos(bd, &new_x, &new_y))
          e_border_move(bd, new_x, new_y);
     }
}

static int
_policy_floating_atom_init(void)
{
   E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_FLOATING_WINDOW_LIST =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_LIST");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_LIST)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_LIST Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST =
      ecore_x_atom_get("_E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST");
   if (!E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!!"
                 "Cannot create _E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST Atom...\n");
        return 0;
     }

   return 1;
}

static E_Illume_Floating_Border*
_policy_floating_get_floating_border(Ecore_X_Window win)
{
   return eina_hash_find(floating_wins_hash, e_util_winid_str_get(win));
}

static E_Illume_Floating_Border*
_policy_floating_border_list_add(E_Border *bd)
{
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_SAFETY_ON_NULL_GOTO(bd, error);
   if (!e_illume_border_is_floating(bd)) goto error;

   ft_bd = _policy_floating_get_floating_border(bd->client.win);
   if (ft_bd) return ft_bd;

   L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) Foating window is added in list, win: 0x%08x\n",
     __func__, __LINE__, bd->client.win);

   ft_bd = E_NEW(E_Illume_Floating_Border, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ft_bd, NULL);

   memset(ft_bd, 0, sizeof(E_Illume_Floating_Border));
   ft_bd->bd = bd;

   eina_hash_add(floating_wins_hash, e_util_winid_str_get(bd->client.win), ft_bd);
   eina_hash_add(floating_wins_hash, e_util_winid_str_get(bd->bg_win), ft_bd);
   eina_hash_add(floating_wins_hash, e_util_winid_str_get(bd->win), ft_bd);

   floating_wins = eina_list_append(floating_wins, ft_bd);
   _policy_hints_floating_list_set();

   return ft_bd;

error:
   ELB(ELBT_ILLUME, "[FLOATING] COULDN'T CREATE \"ft_bd\"", bd->client.win);
   return NULL;
}

static void
_policy_floating_border_list_del(E_Border *bd)
{
   E_Illume_Floating_Border *ft_bd = NULL;

   ft_bd = _policy_floating_get_floating_border(bd->win);
   if (!ft_bd)
     {
        L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) There is no border in list\n", __func__, __LINE__);
        return;
     }

   L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) Floating window is removed in list, win:0x%08x\n",
     __func__, __LINE__, bd->win);

   _policy_floating_pinwin_destroy(ft_bd);

   floating_wins = eina_list_remove(floating_wins, ft_bd);
   eina_hash_del(floating_wins_hash, e_util_winid_str_get(bd->client.win), ft_bd);
   eina_hash_del(floating_wins_hash, e_util_winid_str_get(bd->bg_win), ft_bd);
   eina_hash_del(floating_wins_hash, e_util_winid_str_get(bd->win), ft_bd);

   memset(ft_bd, 0, sizeof(E_Illume_Floating_Border));
   E_FREE(ft_bd);

   _policy_hints_floating_list_set();
}

static void
_policy_hints_floating_list_set(void)
{
   Eina_List *ml = NULL, *cl = NULL;
   E_Manager *m;
   E_Container *c;
   E_Border_List *bl;
   E_Border *b;
   E_Illume_Floating_Border *ft_bd = NULL;
   Ecore_X_Window *clients = NULL;
   int num = 0, i = 0;
   int n_hide, n_unicon;

   num = eina_list_count(floating_wins);

   L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) Floating window list has being updated\n",
     __func__, __LINE__);

   if (num > 0)
     {
        clients = calloc(num, sizeof(Ecore_X_Window));
        EINA_SAFETY_ON_NULL_RETURN(clients);

        EINA_LIST_FOREACH(e_manager_list(), ml, m)
          {
             EINA_LIST_FOREACH(m->containers, cl, c)
               {
                  Eina_List *zl;
                  E_Zone *zone;
                  E_Illume_Quickpanel *qp;

                  EINA_LIST_FOREACH(c->zones, zl, zone)
                    {
                       i = 0; n_hide = 0; n_unicon = 0;
                       qp = e_illume_quickpanel_by_zone_get(zone);
                       bl = e_container_border_list_first(c);
                       if (bl)
                         {
                            while ((b = e_container_border_list_next(bl)))
                              {
                                 ft_bd = _policy_floating_get_floating_border(b->client.win);
                                 if (!ft_bd) continue;

                                 clients[i++] = b->client.win;
                                 if(ft_bd->hide) n_hide++;
                                 else
                                   {
                                      if(!ft_bd->pinon) n_unicon++;
                                   }
                              }

                            L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) There is =%d floating(%d hidden, %d uniconified) window\n",
                              __func__, __LINE__, i, n_hide, n_unicon);
                            if (i > 0)
                              ecore_x_window_prop_window_set(m->root,
                                                             E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                                             clients, i);
                            else
                              ecore_x_window_prop_window_set(m->root,
                                                             E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                                             NULL, 0);

                            if (n_hide > 0)
                              ecore_x_window_prop_window_set(m->root,
                                                             E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST,
                                                             clients, n_hide);
                            else
                              ecore_x_window_prop_window_set(m->root,
                                                             E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST,
                                                             NULL, 0);

                            if (n_unicon > 0)
                              ecore_x_window_prop_window_set(m->root,
                                                             E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST,
                                                             clients, n_unicon);
                            else
                              ecore_x_window_prop_window_set(m->root,
                                                             E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST,
                                                             NULL, 0);

                            if (qp && qp->bd)
                              {
                                 E_Border *bd = qp->bd;
                                 ecore_x_client_message32_send(bd->client.win,
                                                               E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                                               ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                               i,0,0,0,0);
                                 ecore_x_client_message32_send(bd->client.win,
                                                               E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST,
                                                               ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                               n_hide,0,0,0,0);
                                 ecore_x_client_message32_send(bd->client.win,
                                                               E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST,
                                                               ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                               n_unicon,0,0,0,0);
                              }

                            e_container_border_list_free(bl);
                         }
                    } //end of zones
               } // end of containers
          } // end of managers
        E_FREE(clients);
     }
   else
     {
        EINA_LIST_FOREACH(e_manager_list(), ml, m)
          {
             EINA_LIST_FOREACH(m->containers, cl, c)
               {
                  Eina_List *zl;
                  E_Zone *zone;
                  E_Illume_Quickpanel *qp;

                  EINA_LIST_FOREACH(c->zones, zl, zone)
                    {
                       qp = e_illume_quickpanel_by_zone_get(zone);

                       L(LT_FLOATING, "[ILLUME][FLOATING] %s(%d) There is no floating window\n",
                         __func__, __LINE__);
                       ecore_x_window_prop_window_set(m->root,
                                                      E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                                      NULL, 0);
                       ecore_x_window_prop_window_set(m->root,
                                                      E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST,
                                                      NULL, 0);
                       ecore_x_window_prop_window_set(m->root,
                                                      E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST,
                                                      NULL, 0);
                       if (qp && qp->bd)
                         {
                            E_Border *bd = qp->bd;

                            ecore_x_client_message32_send(bd->client.win,
                                                          E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                                          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                          0,0,0,0,0);
                            ecore_x_client_message32_send(bd->client.win,
                                                          E_ILLUME_ATOM_FLOATING_WINDOW_HIDE_LIST,
                                                          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                          0,0,0,0,0);
                            ecore_x_client_message32_send(bd->client.win,
                                                          E_ILLUME_ATOM_FLOATING_WINDOW_UNICONIFY_LIST,
                                                          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                          0,0,0,0,0);
                         }
                    } //end of zones
               } // end of containers
          } // end of managers
     }

}

static Eina_Bool
_policy_floating_cb_client_message(void   *data __UNUSED__,
                                   int    type __UNUSED__,
                                   void   *event)
{
   Ecore_X_Event_Client_Message *ev = event;
   Eina_Bool set = EINA_FALSE;

   if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL)
     {
        L(LT_FLOATING,
          "[ILLUME][FLOATING] %s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL\n",
          __func__, __LINE__);

        _policy_floating_close_all();
     }
   else if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE)
     {
        L(LT_FLOATING,
          "[ILLUME][FLOATING] %s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE\n",
          __func__, __LINE__);

        set = ev->data.b[0];
        _policy_floating_iconify_all(set);
     }
   else if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN)
     {
        L(LT_FLOATING,
          "[ILLUME][FLOATING] %s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN\n",
          __func__, __LINE__);

        _policy_floating_smart_cleanup(ev);
     }
   else if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL)
     {
        L(LT_FLOATING,
          "[ILLUME][FLOATING] %s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_ICONIFY_ALL\n",
          __func__, __LINE__);

        set = ev->data.b[0];
        _policy_floating_minimize_all(set);
     }
   else if (ev->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        L(LT_FLOATING,
          "[ILLUME][FLOATING] %s(%d) Received message, ECORE_X_ATOM_NET_ACTIVE_WINDOW\n",
          __func__, __LINE__);

        _policy_floating_active_request(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_floating_cb_bd_resize(void *data, int type, void *event)
{
   E_Event_Border_Resize *ev = event;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = ev->border;
   int new_x, new_y;

   ft_bd = _policy_floating_get_floating_border(bd->win);
   E_CHECK_GOTO(ft_bd, end);

   if (e_border_rotation_is_progress(bd))
     {
        ft_bd->rot.geo.curr.w = bd->w;
        ft_bd->rot.geo.curr.h = bd->h;
        ft_bd->rot.resized = 1;
        if (((!ft_bd->rot_done_wait) || (!ft_bd->pinon)) && (ft_bd->placed))
          {
             if (_policy_floating_rotation_change_pos(bd, &new_x, &new_y))
               e_border_move(bd, new_x, new_y);
          }
     }

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_floating_cb_bd_move(void *data, int type, void *event)
{
   E_Event_Border_Move *ev = event;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = ev->border;

   ft_bd = _policy_floating_get_floating_border(bd->win);
   E_CHECK_GOTO(ft_bd, end);

   if (e_border_rotation_is_progress(bd))
     {
        ft_bd->rot.geo.curr.x = bd->x;
        ft_bd->rot.geo.curr.y = bd->y;
     }
end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_floating_cb_bd_rot_begin(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Rotation_Change_Begin *ev = event;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = ev->border;

   ft_bd = _policy_floating_get_floating_border(bd->win);
   E_CHECK_GOTO(ft_bd, end);

   ft_bd->rot.geo.prev.x = bd->x;
   ft_bd->rot.geo.prev.y = bd->y;
   ft_bd->rot.geo.prev.w = bd->w;
   ft_bd->rot.geo.prev.h = bd->h;
   ft_bd->rot.geo.curr.x = bd->x;
   ft_bd->rot.geo.curr.y = bd->y;
   ft_bd->rot.geo.curr.w = bd->w;
   ft_bd->rot.geo.curr.h = bd->h;
   ft_bd->rot.resized = 0;
end:
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_policy_floating_cb_bd_rot_end(void *data, int type __UNUSED__, void *event)
{
   E_Event_Border_Rotation_Change_End *ev = event;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = ev->border;
   E_Win *ewin = NULL;

   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   ft_bd = _policy_floating_get_floating_border(bd->client.win);
   E_CHECK_GOTO(ft_bd, end);

   if ((ft_bd->rot_done_wait) && (ft_bd->pinon))
     {
        int x = 0, y = 0;

        ewin = ft_bd->icon.win;
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

        _policy_floating_pinoff_xy_get(ft_bd, &x, &y);
        e_border_move(bd, x, y);

        ELBF(ELBT_ILLUME, 0, bd->client.win,
             "PIN - OFF(by User) |  x:%d, y:%d",
             x, y);

        ft_bd->pinon = 0;
        ft_bd->rot_done_wait = 0;
     }
   else if (!ft_bd->placed)
     {
        _policy_floating_border_placement(bd);
     }
   else if (!ft_bd->rot.resized)
     {
        int new_x, new_y;

        if ((ft_bd->rot.geo.prev.x == ft_bd->rot.geo.curr.x) &&
            (ft_bd->rot.geo.prev.y == ft_bd->rot.geo.curr.y))
          {
             if (_policy_floating_rotation_change_pos(bd, &new_x, &new_y))
               e_border_move(bd, new_x, new_y);
          }
     }
end:
   return ECORE_CALLBACK_RENEW;
}

static void
_policy_floating_cb_hook_eval_pre_new_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   EINA_SAFETY_ON_NULL_RETURN((bd = data2));
   if (!bd->new_client) return;
   if (bd->placed) return;
   if (bd->re_manage) return;
   if (bd->client.icccm.request_pos) return;
   if (!e_illume_border_is_floating(bd)) return;

   // set the "bd->placed" to avoid e17's major placement policy.
   // floating window will be placed at the border show by illume's policy.
   bd->placed = 1;
}

static void
_policy_floating_cb_hook_eval_post_new_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;
   Eina_List *l;
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN((bd = data2));
   if (!bd->new_client) return;
   if (!bd->internal) return;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        if (ft_bd->icon.win)
          {
             if (ft_bd->icon.win->border == bd)
               {
                  e_border_layer_set(bd, POL_FLOATING_LAYER);
                  break;
               }
          }
     }
}

static Eina_Bool
_policy_floating_cb_pinwin_mouse_down(void   *data,
                                     int    type __UNUSED__,
                                     void *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_Illume_Floating_Border *ft_bd = (E_Illume_Floating_Border *)data;
   E_Win *e_win;
   E_Border *bd;

   EINA_SAFETY_ON_NULL_GOTO(ev, end);
   EINA_SAFETY_ON_NULL_GOTO(ft_bd, end);

   e_win = ft_bd->icon.win;

   EINA_SAFETY_ON_NULL_GOTO(e_win, end);
   bd = e_win->border;

   if ((ev->event_window != e_win->evas_win) &&
       (ev->window != e_win->evas_win)) goto end;

   if ((ev->buttons >= 1) && (ev->buttons <= 3))
     {
        edje_object_signal_emit(ft_bd->icon.edje_icon_layout,  "pin.focused", "illume2");
        ft_bd->icon.mouse.pressed = 1;
        ft_bd->icon.mouse.last_down.x = ev->root.x;
        ft_bd->icon.mouse.last_down.y = ev->root.y;
     }

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_floating_cb_pinwin_mouse_move(void   *data,
                                     int    type __UNUSED__,
                                     void   *event)
{
   Ecore_Event_Mouse_Move *ev = (Ecore_Event_Mouse_Move *)event;
   E_Illume_Floating_Border *ft_bd = (E_Illume_Floating_Border *)data;
   E_Win *e_win;
   E_Border *bd;
   double l;

   EINA_SAFETY_ON_NULL_GOTO(ev, end);
   EINA_SAFETY_ON_NULL_GOTO(ft_bd, end);

   e_win = ft_bd->icon.win;
   EINA_SAFETY_ON_NULL_GOTO(e_win, end);

   bd = e_win->border;

   if ((ev->event_window != e_win->evas_win) &&
       (ev->window != e_win->evas_win)) goto end;

   E_CHECK_GOTO(!ft_bd->icon.moving, end);
   E_CHECK_GOTO(ft_bd->icon.mouse.pressed, end);

   l = sqrt(pow((float)(ft_bd->icon.mouse.last_down.x - ev->root.x), 2) +
            pow((float)(ft_bd->icon.mouse.last_down.y - ev->root.y), 2));

   if (l >= 30.0f)
     {
        bd->moveinfo.down.mx = ev->x;
        bd->moveinfo.down.my = ev->y;

        bd->lock_user_location = 0;

        bd->cur_mouse_action = e_action_find("window_move");
        if (bd->cur_mouse_action)
          {
             if ((!bd->cur_mouse_action->func.end_mouse) &&
                 (!bd->cur_mouse_action->func.end))
                bd->cur_mouse_action = NULL;
             if (bd->cur_mouse_action)
               {
                  e_object_ref(E_OBJECT(bd->cur_mouse_action));
                  bd->cur_mouse_action->func.go(E_OBJECT(bd), NULL);
                  ft_bd->icon.moving = 1;
               }
          }
        if (ft_bd->icon.mouse.handlers.move)
          {
             ecore_event_handler_del(ft_bd->icon.mouse.handlers.move);
             ft_bd->icon.mouse.handlers.move = NULL;
          }
        ft_bd->icon.mouse.pressed = 0;
     }

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_floating_cb_pinwin_mouse_up(void  *data,
                                   int   type __UNUSED__,
                                   void  *event)
{
   Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;
   E_Illume_Floating_Border *ft_bd = (E_Illume_Floating_Border *)data;
   E_Win *e_win;
   E_Border *bd;

   EINA_SAFETY_ON_NULL_GOTO(ev, end);
   EINA_SAFETY_ON_NULL_GOTO(ft_bd, end);

   e_win = ft_bd->icon.win;
   EINA_SAFETY_ON_NULL_GOTO(e_win, end);

   bd = e_win->border;

   if (ev->buttons != 1)      goto end;
   if ((ev->event_window != e_win->evas_win) &&
       (ev->window != e_win->evas_win) &&
       (ev->event_window != bd->win)) goto end;

   if ((ft_bd->icon.moving) &&
       (ev->event_window == bd->win))
     {
        if (!ft_bd->icon.mouse.handlers.move)
          {
             ft_bd->icon.mouse.handlers.move =
                ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                        _policy_floating_cb_pinwin_mouse_move, ft_bd);
          }
        e_grabinput_release(bd->win, bd->win);
        ft_bd->icon.moving = 0;

        edje_object_signal_emit(ft_bd->icon.edje_icon_layout, "pin.losefocus", "illume2");
     }
   else if (!ft_bd->icon.moving)
     {
        edje_object_signal_emit(ft_bd->icon.edje_icon_layout, "pin.losefocus", "illume2");
        edje_object_signal_emit(ft_bd->icon.edje_icon_layout, "pin.sound", "illume2");
        _policy_floating_pinoff(ft_bd);
        _policy_hints_floating_list_set();
     }

   ft_bd->icon.mouse.pressed = 0;

end:
   return ECORE_CALLBACK_PASS_ON;
}

static void
_policy_floating_close_all(void)
{
   E_Illume_Floating_Border *ft_bd = NULL;
   Eina_List *l;
   E_Border *bd;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        if(ft_bd->pinon)
          {
              _policy_floating_pinwin_iconify(ft_bd, 1);
          }
        _policy_floating_iconify(ft_bd, 1);

        if(_e_illume_cfg->dbus.rscd_msg.use_unfreeze_win)
          {
             bd = ft_bd->bd;
             if(bd) _policy_floating_set_pid_fg(bd->client.netwm.pid);
          }
        ft_bd->defer.close = 1;
        ft_bd->changed = 1;
     }
}

void
_policy_floating_set_pid_fg(int pid)
{
   DBusMessage* msg;
   int param_pid;
   int param_act = PROCESS_FOREGROUND;

   if (!dbus_conn) return;
   if (pid <= 1) return;

   // set up msg for resourced
   msg = dbus_message_new_signal(_e_illume_cfg->dbus.rscd_msg.path,
                                 _e_illume_cfg->dbus.rscd_msg.iface,
                                 _e_illume_cfg->dbus.rscd_msg.sig_name);
   if (!msg) return;
   // append the action to do and the pid to do it to
   param_pid = (int)pid;
   if (!dbus_message_append_args(msg,
                                 DBUS_TYPE_INT32, &param_act,
                                 DBUS_TYPE_INT32, &param_pid,
                                 DBUS_TYPE_INVALID))
     {
        dbus_message_unref(msg);
        return;
     }
   // send the message
   if (!e_dbus_message_send(dbus_conn, msg, NULL, 0, NULL))
     {
        dbus_message_unref(msg);
        return;
     }
   // cleanup
   dbus_message_unref(msg);
}

static void
_policy_floating_iconify(E_Illume_Floating_Border *ft_bd,
                        Eina_Bool iconify)
{
   E_Border *bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);
   bd = ft_bd->bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (iconify)
     {
        e_border_hide(bd, 0);
     }
   else
     {
        e_border_show(bd);
     }
}

static void
_policy_floating_iconify_all(Eina_Bool iconify)
{
   Eina_List *l;
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        if(ft_bd->pinon)
          {
             _policy_floating_pinwin_iconify(ft_bd, iconify);
          }
        else
          {
             _policy_floating_iconify(ft_bd, iconify);
          }
          ft_bd->hide = iconify;
     }
     _policy_hints_floating_list_set();
}

static void
_policy_floating_minimize_all(Eina_Bool minimize)
{
   Eina_List *l;
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        if(minimize)
          {
             if (ft_bd->hide) continue;
             if (!ft_bd->pinon) _policy_floating_pinon(ft_bd);
          }
        else
          {
             if (ft_bd->hide) continue;
             if (ft_bd->pinon) _policy_floating_pinoff(ft_bd);
          }
     }
   _policy_hints_floating_list_set();
}

static void
_policy_floating_active_request(Ecore_X_Event_Client_Message *event)
{
   E_Border *bd = NULL;
   E_Illume_Floating_Border *ft_bd = NULL;

   bd = e_border_find_by_client_window(event->win);
   if (!bd) return;

   if (!e_illume_border_is_floating(bd)) return;

   ft_bd = _policy_floating_get_floating_border(bd->win);
   if (!ft_bd) return;

    if (ft_bd->pinon)
     {
        _policy_floating_pinoff(ft_bd);
     }
   else if (!bd->visible)
     {
        e_border_show(bd);
     }

   ft_bd->hide = 0;
   _policy_hints_floating_list_set();
}

static Eina_Bool
_policy_floating_pinwin_create(E_Illume_Floating_Border *ft_bd)
{
   Evas_Object *swallow_obj;
   Evas_Object *layout;
   E_Border *bd;
   E_Win *e_win;
   char buf[PATH_MAX];
   char win_name[PATH_MAX];

   EINA_SAFETY_ON_NULL_GOTO(ft_bd, end);

   bd = ft_bd->bd;
   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   if(!ft_bd->pinon) goto end;

   e_win = e_win_new(bd->zone->container);
   if (!e_win) goto end;

   ft_bd->icon.win = e_win;
   ft_bd->icon.w = 112;  // hardcode's going to get size from layout
   ft_bd->icon.h = 112;  // hardcode's going to get size from layout

   ecore_evas_alpha_set(e_win->ecore_evas, 1);
   e_win->evas_win = ecore_evas_window_get(e_win->ecore_evas);

   ft_bd->icon.edje_icon_layout = edje_object_add(e_win->evas);
   layout = ft_bd->icon.edje_icon_layout;

   memset(buf, 0x00, PATH_MAX);
   snprintf(buf, sizeof(buf), "%s/e-module-illume2-tizen.edj", _e_illume_mod_dir);
   EINA_SAFETY_ON_FALSE_GOTO(edje_object_file_set(layout, buf, "mw_minimize_mode"), end);
   evas_object_resize(layout, 112, 112);

   swallow_obj = e_border_icon_add(bd, e_win->evas);
   if(!swallow_obj)
     {
        memset(buf, 0x00, PATH_MAX);
        swallow_obj = edje_object_add(e_win->evas);
        snprintf(buf, sizeof(buf), "%s/e-module-illume2-tizen.edj", _e_illume_mod_dir);
        EINA_SAFETY_ON_FALSE_GOTO(edje_object_file_set(swallow_obj, buf, "default_icon"), end);
     }
   evas_object_resize(swallow_obj, 102, 102);

   edje_object_part_swallow( layout, "swallow_icon", swallow_obj);
   evas_object_show(layout);

   ft_bd->icon.mouse.handlers.move = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                                             _policy_floating_cb_pinwin_mouse_move, ft_bd);

   ft_bd->icon.mouse.handlers.down = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                                             _policy_floating_cb_pinwin_mouse_down, ft_bd);

   ft_bd->icon.mouse.handlers.up = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                                           _policy_floating_cb_pinwin_mouse_up, ft_bd);

   memset(win_name, 0x00, PATH_MAX);
   snprintf(win_name, sizeof(win_name), "Iconic - %s",  e_border_name_get(bd));
   e_win_title_set(e_win, win_name);
   e_win_name_class_set(e_win, "E", "ICON_WIN");
   e_win_resize(e_win, ft_bd->icon.w, ft_bd->icon.h);

  return EINA_TRUE; // successfully created

end:
   return EINA_FALSE;
}

static Eina_Bool
_policy_floating_pinwin_destroy(E_Illume_Floating_Border *ft_bd)
{
   E_Win *e_win;
   Ecore_Event_Handler *eh;

   e_win = ft_bd->icon.win;
   if (!e_win) goto end;

   EINA_LIST_FREE (ft_bd->handlers, eh)
      ecore_event_handler_del(eh);

   ecore_event_handler_del(ft_bd->icon.mouse.handlers.move);
   ft_bd->icon.mouse.handlers.move = NULL;
   ecore_event_handler_del(ft_bd->icon.mouse.handlers.down);
   ft_bd->icon.mouse.handlers.down = NULL;
   ecore_event_handler_del(ft_bd->icon.mouse.handlers.up);
   ft_bd->icon.mouse.handlers.up = NULL;

   e_object_del(E_OBJECT(e_win));
   ft_bd->icon.win = NULL;

end:
   return ECORE_CALLBACK_PASS_ON;
}

static void
_policy_floating_pinon(E_Illume_Floating_Border *ft_bd)
{
   E_Border *bd;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);

   bd = ft_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!ft_bd->pinon)
     {
        ft_bd->pinon = 1;
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

        if (!ft_bd->icon.win) _policy_floating_pinwin_create(ft_bd);
        if (ft_bd->icon.win)
          {
             int x, y;
             _policy_floating_pinon_xy_get(ft_bd, &x, &y);
             e_win_move(ft_bd->icon.win, x, y);
             e_win_show(ft_bd->icon.win);
             ecore_x_icccm_hints_set(ft_bd->icon.win->border->client.win, 0, 0, 0, 0, 0, 0, 0);
             ft_bd->icon.win->border->client.icccm.accepts_focus = 0;
             e_border_raise(ft_bd->icon.win->border);
             // for supporting dependent rotation to icon window.
             ft_bd->icon.win->border->client.e.fetch.rot.need_rotation = EINA_TRUE;
          }
     }
}

static void
_policy_floating_pinoff(E_Illume_Floating_Border *ft_bd)
{
   E_Border *bd;
   E_Win *ewin;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);
   bd = ft_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (ft_bd->pinon)
     {
        if (!policy_border_dep_rotation_set(bd))
          {
             int x = 0, y = 0;

             ewin = ft_bd->icon.win;
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

             _policy_floating_pinoff_xy_get(ft_bd, &x, &y);
             e_border_move(bd, x, y);

             ELBF(ELBT_ILLUME, 0, bd->client.win,
                  "PIN - OFF(by User) |  x:%d, y:%d",
                  x, y);

             ft_bd->pinon = 0;
          }
        else
          {
             e_hints_window_visible_set(bd);
             ft_bd->rot_done_wait = 1;
          }
     }
}

static void
_policy_floating_pinwin_iconify(E_Illume_Floating_Border *ft_bd,
                               Eina_Bool iconify)
{
   E_Border *ewin_bd = NULL;
   E_Win *ewin;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);

   ewin= ft_bd->icon.win;
   EINA_SAFETY_ON_NULL_RETURN(ewin);

   ewin_bd = ewin->border;
   EINA_SAFETY_ON_NULL_RETURN(ewin_bd);

   if (iconify)
     {
        e_border_hide(ewin_bd, 0);
     }
   else
     {
        e_border_show(ewin_bd);
     }
}

static void
_policy_floating_pinon_xy_get(E_Illume_Floating_Border *ft_bd, int *x, int *y)
{
   E_Border *bd;
   int rot;
   int max_x, max_y;
   int e_x, e_y;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);

   bd = ft_bd->bd;
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
           e_y = bd->y + bd->h - ft_bd->icon.h;
           break;

        case 180:
           e_x = bd->x - (bd->h - ft_bd->icon.h);
           e_y = bd->y - (bd->w - ft_bd->icon.w);
           break;

        case 270:
           e_x = bd->x + bd->w - ft_bd->icon.w;
           e_y = bd->y;
           break;

        case 0:
        default:
           e_x = bd->x;
           e_y = bd->y;
           break;
     }

   // Icons are located in the screen area.
   max_x = bd->zone->w - ft_bd->icon.w;
   max_y = bd->zone->h - ft_bd->icon.h;
   if(e_x < 0) e_x = 0;
   if(e_y < 0) e_y = 0;
   if(e_x > max_x) e_x = max_x;
   if(e_y > max_y) e_y = max_y;

   if (x) *x = e_x;
   if (y) *y = e_y;
}

static void
_policy_floating_pinoff_xy_get(E_Illume_Floating_Border *ft_bd, int *x, int *y)
{
   E_Border *bd, *e_bd;
   int wx, wy, bd_w, bd_h;
   int max_x, max_y;
   int diff_view, ang1, ang2;
   E_Win *e_win;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);

   bd = ft_bd->bd;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   e_win = ft_bd->icon.win;
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

   // floating windows are located in the screen area.
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
   E_Illume_Floating_Border *ft_bd;
   int x2, y2, w2, h2;
   int ok;
   int iw, ih;
   int x0, x00, yy0, y00;

   if (!desk) return 0;
   if (!desk->zone) return 0;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        bd = ft_bd->bd;
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
        else if ((!ft_bd->hide) && (ft_bd->pinon))
          {
            // to calculate intersaction with E_Win(minimized multi-window)
            ewin_bd = NULL;
            if (ft_bd->icon.win) ewin_bd = ft_bd->icon.win->border;
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
   E_Illume_Floating_Border *ft_bd;
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

   // Make the array that is candidate position which is floating window could be placed.
   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        int ok;
        int bx, by, bw, bh;

        bd = ft_bd->bd;
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
             if ((ft_bd->pinon) && (ft_bd->icon.win) && (ft_bd->icon.win->border))
               bd = ft_bd->icon.win->border;
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
                    {
                       x[(*a_w) - 1] = bx;
                       u_x[bx] = 1;
                    }
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
                    {
                       x[(*a_w) - 1] = bx + bw;
                       u_x[bx + bw] = 1;
                    }
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
                    {
                       y[(*a_h) - 1] = by;
                       u_y[by] = 1;
                    }
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
                    {
                       y[(*a_h) - 1] = by + bh;
                       u_y[by + bh] = 1;
                    }
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
_policy_floating_border_placement(E_Border *bd)
{
   E_Illume_Floating_Border *ft_bd;
   Eina_List *skiplist = NULL;
   int new_x, new_y, old_x, old_y;
   int rotation = 0;

   EINA_SAFETY_ON_NULL_RETURN(bd);

   ft_bd = _policy_floating_get_floating_border(bd->win);
   E_CHECK(!e_object_is_del(E_OBJECT(bd)));
   E_CHECK(ft_bd);
   E_CHECK(!ft_bd->placed);

   if (bd->client.icccm.request_pos)
     {
        ELB(ELBT_ILLUME, "[PLACEMENT] PASS BY REQUEST_POS", bd->client.win);
        ft_bd->placed = 1;
        return;
     }

   skiplist = eina_list_append(skiplist, bd);
   rotation = e_border_rotation_curr_angle_get(bd);
   old_x = new_x = bd->x;
   old_y = new_y = bd->y;

   _policy_floating_place_region_smart(bd->desk, skiplist,
                                       bd->x, bd->y, bd->w, bd->h, rotation,
                                       &new_x, &new_y);

   e_border_move(bd, new_x, new_y);
   ft_bd->placed = 1;

   ELBF(ELBT_ILLUME, 0, bd->client.win,
        "[PLACEMENT] win:0x%08x, rot:%d, size:%dx%d, (%d,%d) => (%d,%d)",
        bd->client.win, rotation, bd->w, bd->h, old_x, old_y, new_x, new_y);
}

static void
_policy_floating_smart_cleanup(Ecore_X_Event_Client_Message *event __UNUSED__)
{
   Eina_List *borders = NULL, *l;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *border = NULL;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        border = ft_bd->bd;
        if (!border) continue;
        if (e_object_is_del(E_OBJECT(border))) continue;
        /* Build a list of windows not iconified. */
        if ((!border->iconic) && (!border->lock_user_location))
          {
             int area;
             Eina_List *ll;
             E_Border *bd;

             /* Ordering windows largest to smallest gives better results */
             area = border->w * border->h;
             EINA_LIST_FOREACH(borders, ll, bd)
               {
                  int testarea;

                  testarea = bd->w * bd->h;
                  /* Insert the border if larger than the current border */
                  if (area >= testarea)
                    {
                       borders = eina_list_prepend_relative(borders, border, bd);
                       break;
                    }
               }
             /* Looped over all borders without placing, so place at end */
             if (!ll) borders = eina_list_append(borders, border);
          }
     }

   /* Loop over the borders moving each one using the smart placement */
   EINA_LIST_FREE(borders, border)
     {
        int new_x, new_y;
        int rotation;

        if (e_object_is_del(E_OBJECT(border))) continue;

        rotation = e_border_rotation_curr_angle_get(border);
        _policy_floating_place_region_smart(border->desk, borders, border->x, border->y,
                                  border->w, border->h, rotation,  &new_x, &new_y);

        ELBF(ELBT_ILLUME, 0, border->client.win,
             "AUTO-PLACEMENT(MANUAL) win:0x%08x, rot:%d, size:%dx%d (%d,%d) => (%d,%d)",
             border->client.win, rotation, border->w, border->h, border->x, border->y, new_x, new_y);

        e_border_move(border, new_x, new_y);
     }

   return;
}

/*
 * Get the widest position that is not occupied by another window.
 */
static int
_policy_floating_place_region_smart(E_Desk *desk, Eina_List *skiplist,
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

   // Step 1. Make the array that is candidate position which is floating window could be placed.
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
_policy_floating_rotation_change_pos(E_Border *bd, int *x, int *y)
{
   E_Zone *zone;
   E_Illume_Floating_Border *ft_bd = NULL;
   int zw, zh, next_zw, next_zh;
   int prev_rot, next_rot;
   int wx, wy, ww, wh, out_ww = 0, out_wh = 0;
   int next_w_space, next_h_space;
   int new_x, new_y;
   float upper_space, lower_space;
   float left_space, right_space;
   float x_axis_rate, y_axis_rate;
   Eina_Rectangle prev_geo, next_geo;

   if (!bd) return EINA_FALSE;
   if ((!x) || (!y)) return EINA_FALSE;

   zone = bd->zone;

   if (e_illume_border_is_icon_win(bd))
     {
        prev_geo.x = bd->x;
        prev_geo.y = bd->y;
        prev_geo.w = bd->w;
        prev_geo.h = bd->h;
        next_geo.x = bd->x;
        next_geo.y = bd->y;
        next_geo.w = bd->w;
        next_geo.h = bd->h;
     }
   else
     {
        ft_bd = _policy_floating_get_floating_border(bd->win);
        if (!ft_bd) return EINA_FALSE;

        prev_geo.x = ft_bd->rot.geo.prev.x;
        prev_geo.y = ft_bd->rot.geo.prev.y;
        prev_geo.w = ft_bd->rot.geo.prev.w;
        prev_geo.h = ft_bd->rot.geo.prev.h;
        next_geo.x = ft_bd->rot.geo.curr.x;
        next_geo.y = ft_bd->rot.geo.curr.y;
        next_geo.w = ft_bd->rot.geo.curr.w;
        next_geo.h = ft_bd->rot.geo.curr.h;
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
