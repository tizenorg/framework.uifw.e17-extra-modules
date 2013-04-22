#include "e_illume_private.h"
#include "e_mod_floating_window.h"

/* global event callback function */
static Eina_Bool _e_mod_floating_cb_idle_enterer(void *data __UNUSED__);
static Eina_Bool _e_mod_floating_cb_border_add(void *data __UNUSED__,
                                              int type __UNUSED__,
                                              void *event);
static Eina_Bool _e_mod_floating_cb_border_del(void *data __UNUSED__,
                                              int type __UNUSED__,
                                              void *event);

static Eina_Bool _e_mod_floating_cb_move_resize_request(void *data __UNUSED__,
                                                          int type __UNUSED__,
                                                          void *event);
static Eina_Bool _e_mod_floating_cb_client_message(void   *data,
                                                     int    type,
                                                     void   *event);
static Eina_Bool _e_mod_floating_cb_window_property(void  *data,
                                                      int   type,
                                                      void  *event);

/* border event callback function */
static Eina_Bool _e_mod_floating_cb_mouse_up(void   *data,
                                              int    type __UNUSED__,
                                              void   *event);
static Eina_Bool _e_mod_floating_cb_mouse_move(void    *data,
                                                int     type __UNUSED__,
                                                void    *event);

// E Border hook
static void _e_mod_floating_cb_hook_resize_begin(void *data __UNUSED__,
                                                 void *data2);

/* general function */
static int _e_mod_floating_atom_init(void);
static void _e_mod_floating_border_list_add(E_Border *bd);
static void _e_mod_floating_border_list_del(E_Border *bd);
static void _e_mod_floating_border_handler_remove(E_Illume_Floating_Border *ft_bd);
static void _e_mod_hints_floating_list_set(void);
static E_Illume_Floating_Border* _e_mod_floating_get_floating_border(Ecore_X_Window win);
static void _e_mod_floating_window_state_change(Ecore_X_Event_Window_Property *ev);

/* for close the floating windows */
static void _e_mod_floating_close_all(void);

/* for iconify the floating windows */
static void _e_mod_floating_iconify(E_Illume_Floating_Border *ft_bd,
                                     Eina_Bool iconify);
static void _e_mod_floating_iconify_all(Eina_Bool iconify);

/* for automatically align the floating windows */
static void _e_mod_floating_smart_cleanup(Ecore_X_Event_Client_Message *event);

/* for top or bottom maximize */
static void _e_mod_floating_maximize_coords_handle(E_Illume_Floating_Border *ft_bd,
                                                     int       x,
                                                     int       y);
static void _e_mod_floating_maximize(E_Border *bd, E_Illume_Maximize max);
static void _e_mod_floating_maximize_internal(E_Border *bd, E_Illume_Maximize max);

/* for controlling app-in-app window */
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL;
static Ecore_X_Atom E_ILLUME_ATOM_FLOATING_WINDOW_LIST;

static Eina_List *_fw_hdls;
static Eina_List *_fw_hooks;
static Ecore_Idle_Enterer *_idle_enterer;

static Eina_Hash *floating_wins_hash;
static Eina_List *floating_wins;

int
e_mod_floating_init(void)
{
   Eina_Bool ret = EINA_FALSE;

   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ADD,
                                               _e_mod_floating_cb_border_add,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_REMOVE,
                                               _e_mod_floating_cb_border_del,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST,
                                               _e_mod_floating_cb_move_resize_request,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                               _e_mod_floating_cb_client_message,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                               _e_mod_floating_cb_window_property,
                                               NULL));

   _fw_hooks =
      eina_list_append(_fw_hooks,
                       e_border_hook_add(E_BORDER_HOOK_RESIZE_BEGIN,
                                         _e_mod_floating_cb_hook_resize_begin,
                                         NULL));

   _idle_enterer = ecore_idle_enterer_add(_e_mod_floating_cb_idle_enterer, NULL);

   ret = _e_mod_floating_atom_init();
   if (!ret)
     L(LT_FLOATING, "%s(%d) Failed initializing atoms\n", __func__, __LINE__);

   if (!floating_wins_hash)
     floating_wins_hash = eina_hash_string_superfast_new(NULL);

   return ret;
}

int
e_mod_floating_shutdown(void)
{
   E_FREE_LIST(_fw_hdls, ecore_event_handler_del);
   E_FREE_LIST(_fw_hooks, e_border_hook_del);

   if (_idle_enterer) ecore_idle_enterer_del(_idle_enterer);
   _idle_enterer = NULL;

   if (floating_wins_hash) eina_hash_free(floating_wins_hash);
   floating_wins_hash = NULL;

   if (floating_wins) eina_list_free(floating_wins);
   floating_wins = NULL;

   return 1;
}

EINTERN Eina_Bool
e_mod_floating_border_is_floating(E_Border *bd)
{
   unsigned int state = bd->client.illume.win_state.state;
   Eina_Bool ret = EINA_FALSE;

   if (state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
     ret = EINA_TRUE;

   return ret;
}

EINTERN Eina_List*
e_mod_floating_get_window_list(void)
{
   return floating_wins;
}

static int
_e_mod_floating_atom_init(void)
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

   return 1;
}

static void
_e_mod_floating_window_state_change(Ecore_X_Event_Window_Property *ev)
{
   E_Border *bd = NULL;
   unsigned int state = 0;

   if (!(bd = e_border_find_by_client_window(ev->win))) return;

   state = ecore_x_e_illume_window_state_get(ev->win);
   switch(state)
     {
      case ECORE_X_ILLUME_WINDOW_STATE_FLOATING:
         L(LT_FLOATING, "%s(%d) State of window is changed to floating, win: 0x%08x\n",
           __func__, __LINE__, ev->win);
         _e_mod_floating_border_list_add(bd);
         break;
      case ECORE_X_ILLUME_WINDOW_STATE_NORMAL:
         L(LT_FLOATING, "%s(%d) State of window is changed to normal, win: 0x%08x\n",
           __func__, __LINE__, ev->win);
         _e_mod_floating_border_list_del(bd);
         break;
     }
}

static E_Illume_Floating_Border*
_e_mod_floating_get_floating_border(Ecore_X_Window win)
{
   return eina_hash_find(floating_wins_hash, e_util_winid_str_get(win));
}

static void
_e_mod_floating_border_list_add(E_Border *bd)
{
   E_Illume_Floating_Border *ft_bd = NULL, *tmp_ft_bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(bd);
   if (!e_mod_floating_border_is_floating(bd)) return;

   L(LT_FLOATING, "%s(%d) Foating window is added in list, win: 0x%08x\n",
     __func__, __LINE__, bd->client.win);

   ft_bd = E_NEW(E_Illume_Floating_Border, 1);
   EINA_SAFETY_ON_NULL_RETURN(ft_bd);

   memset(ft_bd, 0, sizeof(E_Illume_Floating_Border));
   ft_bd->bd = bd;

   floating_wins = eina_list_append(floating_wins, ft_bd);
   tmp_ft_bd = eina_hash_find(floating_wins_hash, e_util_winid_str_get(bd->client.win));
   if (tmp_ft_bd)
     {
        E_Border *bd2 = tmp_ft_bd->bd;

        L(LT_FLOATING, "%s(%d) Something worng!!\n", __func__, __LINE__);
        eina_hash_del(floating_wins_hash,
                      e_util_winid_str_get(bd2->client.win), tmp_ft_bd);
        eina_hash_del(floating_wins_hash,
                      e_util_winid_str_get(bd2->bg_win), tmp_ft_bd);
        eina_hash_del(floating_wins_hash,
                      e_util_winid_str_get(bd2->win), tmp_ft_bd);
     }
   eina_hash_add(floating_wins_hash, e_util_winid_str_get(bd->client.win), ft_bd);
   eina_hash_add(floating_wins_hash, e_util_winid_str_get(bd->bg_win), ft_bd);
   eina_hash_add(floating_wins_hash, e_util_winid_str_get(bd->win), ft_bd);

   _e_mod_hints_floating_list_set();
}

static void
_e_mod_floating_border_list_del(E_Border *bd)
{
   E_Illume_Floating_Border *ft_bd = NULL;

   ft_bd = _e_mod_floating_get_floating_border(bd->win);
   if (!ft_bd)
     {
        L(LT_FLOATING, "%s(%d) There is no border in list", __func__, __LINE__);
        return;
     }

   _e_mod_floating_border_handler_remove(ft_bd);

   L(LT_FLOATING, "%s(%d) Floating window is removed in list, win:0x%08x\n",
     __func__, __LINE__, bd->win);

   floating_wins = eina_list_remove(floating_wins, ft_bd);
   eina_hash_del(floating_wins_hash, e_util_winid_str_get(bd->client.win), ft_bd);
   eina_hash_del(floating_wins_hash, e_util_winid_str_get(bd->bg_win), ft_bd);
   eina_hash_del(floating_wins_hash, e_util_winid_str_get(bd->win), ft_bd);

   memset(ft_bd, 0, sizeof(E_Illume_Floating_Border));
   E_FREE(ft_bd);

   _e_mod_hints_floating_list_set();
}

static void
_e_mod_floating_border_handler_remove(E_Illume_Floating_Border *ft_bd)
{
   EINA_SAFETY_ON_NULL_RETURN(ft_bd);
   EINA_SAFETY_ON_NULL_RETURN(ft_bd->handlers);

   L(LT_FLOATING, "%s(%d) Mouse event handler is removed, win: 0x%08x\n",
     __func__, __LINE__, ft_bd->bd->client.win);

   E_FREE_LIST(ft_bd->handlers, ecore_event_handler_del);
}

static void
_e_mod_hints_floating_list_set(void)
{
   Eina_List *ml = NULL, *cl = NULL;
   E_Manager *m;
   E_Container *c;
   E_Border_List *bl;
   E_Border *b;
   E_Illume_Floating_Border *ft_bd = NULL;
   Ecore_X_Window *clients = NULL;
   int num = 0, i = 0;

   num = floating_wins ? floating_wins->accounting->count : 0;

   L(LT_FLOATING, "%s(%d) Floating window list has being updated\n",
     __func__, __LINE__);

   if (num > 0)
     {
        clients = calloc(num, sizeof(Ecore_X_Window));
        EINA_SAFETY_ON_NULL_RETURN(clients);

        EINA_LIST_FOREACH(e_manager_list(), ml, m)
          {
             EINA_LIST_FOREACH(m->containers, cl, c)
               {
                  bl = e_container_border_list_first(c);
                  while ((b = e_container_border_list_next(bl)))
                    {
                       ft_bd = _e_mod_floating_get_floating_border(b->client.win);
                       if (!ft_bd) continue;

                       clients[i++] = b->client.win;
                    }
                  e_container_border_list_free(bl);
               }
             if (i > 0)
               ecore_x_window_prop_window_set(m->root,
                                              E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                              clients, i);
             else
               ecore_x_window_prop_window_set(m->root,
                                              E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                              NULL, 0);
          }
        E_FREE(clients);
     }
   else
     {
        EINA_LIST_FOREACH(e_manager_list(), ml, m)
          {
             L(LT_FLOATING, "%s(%d) There is no floating window\n",
               __func__, __LINE__);
             ecore_x_window_prop_window_set(m->root,
                                            E_ILLUME_ATOM_FLOATING_WINDOW_LIST,
                                            NULL, 0);
          }
     }
}

static Eina_Bool
_e_mod_floating_cb_border_add(void *data __UNUSED__,
                            int type __UNUSED__,
                            void *event)
{
   E_Event_Border_Add *ev = event;
   E_Border *bd = NULL;

   bd = ev->border;
   EINA_SAFETY_ON_NULL_GOTO(bd, end);
   if (!e_mod_floating_border_is_floating(bd)) goto end;

   _e_mod_floating_border_list_add(bd);

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_floating_cb_border_del(void *data __UNUSED__,
                            int type __UNUSED__,
                            void *event)
{
   E_Event_Border_Remove *ev = event;
   E_Border *bd = NULL;

   bd = ev->border;
   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   _e_mod_floating_border_list_del(bd);

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_floating_cb_move_resize_request(void *data __UNUSED__,
                                        int type __UNUSED__,
                                        void *event)
{
   Ecore_X_Event_Window_Move_Resize_Request *e;
   E_Illume_Floating_Border *ft_bd = NULL;

   e = event;
   ft_bd = _e_mod_floating_get_floating_border(e->win);
   if (!ft_bd)
     {
        L(LT_FLOATING, "%s(%d) No window in floating list, win: 0x%08x\n",
          __func__, __LINE__, e->win);
        goto end;
     }

   if (ft_bd->handlers == NULL)
     {
        L(LT_FLOATING, "%s(%d) Mouse event handler is added, win: 0x%08x\n",
          __func__, __LINE__, e->win);
        ft_bd->handlers = eina_list_append(ft_bd->handlers,
                                           ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                                                   _e_mod_floating_cb_mouse_up,
                                                                   ft_bd));
        ft_bd->handlers = eina_list_append(ft_bd->handlers,
                                           ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                                                   _e_mod_floating_cb_mouse_move,
                                                                   ft_bd));
     }

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_floating_cb_client_message(void   *data __UNUSED__,
                                   int    type __UNUSED__,
                                   void   *event)
{
   Ecore_X_Event_Client_Message *ev = event;
   Eina_Bool iconify = EINA_FALSE;

   if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL)
     {
        L(LT_FLOATING,
          "%s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_CLOSE_ALL\n",
          __func__, __LINE__);

        _e_mod_floating_close_all();
     }
   else if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE)
     {
        L(LT_FLOATING,
          "%s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_CHANGE_VISIBLE\n",
          __func__, __LINE__);

        iconify = ev->data.b[0];
        _e_mod_floating_iconify_all(iconify);
     }
   else if (ev->message_type == E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN)
     {
        L(LT_FLOATING,
          "%s(%d) Received message, E_ILLUME_ATOM_FLOATING_WINDOW_ALIGN\n",
          __func__, __LINE__);

        _e_mod_floating_smart_cleanup(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_floating_cb_window_property(void *data __UNUSED__,
                                    int type __UNUSED__,
                                    void *event)
{
   Ecore_X_Event_Window_Property *ev = event;

   if (ev->atom == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE)
     {
        _e_mod_floating_window_state_change(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_floating_cb_mouse_up(void   *data,
                             int    type __UNUSED__,
                             void   *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = NULL;

   EINA_SAFETY_ON_NULL_GOTO(event, end);
   EINA_SAFETY_ON_NULL_GOTO(data, end);

   ev = event;
   ft_bd = data;
   bd = ft_bd->bd;
   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   if ((ev->event_window != bd->win) &&
       (ev->event_window != bd->event_win) &&
       (ev->window != bd->event_win))
     goto end;

   L(LT_FLOATING, "%s(%d) mouse up state, border: 0x%08x\n",
     __func__, __LINE__, ev->window);

   if (!ft_bd->moving) goto end;
   ft_bd->moving = 0;

   if (e_illume_border_is_fixed(bd)) goto end;

   _e_mod_floating_maximize_coords_handle(ft_bd, ev->root.x, ev->root.y);
   _e_mod_floating_border_handler_remove(ft_bd);

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_floating_cb_mouse_move(void    *data,
                               int     type __UNUSED__,
                               void    *event)
{
   Ecore_Event_Mouse_Move *ev;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = NULL;
   int new_x = 0, new_y = 0;
   int threshold = _e_illume_cfg->floating_control_threshold;

   EINA_SAFETY_ON_NULL_GOTO(event, end);
   EINA_SAFETY_ON_NULL_GOTO(data, end);

   ev = event;
   ft_bd = data;
   bd = ft_bd->bd;
   EINA_SAFETY_ON_NULL_GOTO(bd, end);

   if ((ev->event_window != bd->win) &&
       (ev->event_window != bd->event_win) &&
       (ev->window != bd->event_win))
     goto end;

   if (!bd->moving) goto end;
   ft_bd->moving = 1;

   L(LT_FLOATING, "%s(%d) mouse move state, border: 0x%08x\n",
     __func__, __LINE__, ev->window);

   if (e_illume_border_is_fixed(bd)) goto end;

   if (ft_bd->state.maximize_by_illume == 0) goto end;
   ft_bd->state.maximize_by_illume = 0;

   e_border_unmaximize(bd, E_ILLUME_MAXIMIZE_BOTH);

   new_x = ev->root.x - (bd->w / 2);
   new_y = ev->root.y - threshold;

   if ((bd->moveinfo.down.button >= 1) && (bd->moveinfo.down.button <= 3))
     {
        bd->mouse.last_down[bd->moveinfo.down.button - 1].x = new_x;
        bd->mouse.last_down[bd->moveinfo.down.button - 1].y = new_y;
     }

   e_border_move(bd, new_x, new_y);

end:
   return ECORE_CALLBACK_PASS_ON;
}

/* if user resize the window in maximized by gesture,
 * change this window back into un-maximize state.
 */
static void
_e_mod_floating_cb_hook_resize_begin(void *data __UNUSED__,
                                     void *data2)
{
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(data2);
   bd = data2;
   ft_bd = _e_mod_floating_get_floating_border(bd->win);
   EINA_SAFETY_ON_NULL_RETURN(ft_bd);

   if (ft_bd->state.maximize_by_illume)
     {
        ft_bd->state.maximize_by_illume = 0;
        bd->maximized = 0;
     }
}

static Eina_Bool
_e_mod_floating_cb_idle_enterer(void *data __UNUSED__)
{
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *bd = NULL;
   Eina_List *l;

   if (!floating_wins) return ECORE_CALLBACK_RENEW;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        if (!ft_bd) continue;
        if (!ft_bd->changed) continue;
        ft_bd->changed = 0;

        bd = ft_bd->bd;
        if (!bd) continue;

         L(LT_FLOATING, "%s(%d) idle state, win: 0x%08x\n",
           __func__, __LINE__, bd->client.win);

        if (ft_bd->defer.maximize_top)
          {
             _e_mod_floating_maximize(bd,
                                      E_MAXIMIZE_FULLSCREEN |
                                      E_ILLUME_MAXIMIZE_TOP);
             ft_bd->state.maximize_by_illume = 1;
             ft_bd->defer.maximize_top = 0;
          }
        else if (ft_bd->defer.maximize_bottom)
          {
             _e_mod_floating_maximize(bd,
                                      E_MAXIMIZE_FULLSCREEN |
                                      E_ILLUME_MAXIMIZE_BOTTOM);
             ft_bd->state.maximize_by_illume = 1;
             ft_bd->defer.maximize_bottom = 0;
          }

        if (ft_bd->defer.close)
          {
             e_border_act_close_begin(ft_bd->bd);
             ft_bd->defer.close = 0;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_e_mod_floating_close_all(void)
{
   E_Illume_Floating_Border *ft_bd = NULL;
   Eina_List *l;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        _e_mod_floating_iconify(ft_bd, 1);

        ft_bd->defer.close = 1;
        ft_bd->changed = 1;
     }
}

static void
_e_mod_floating_iconify(E_Illume_Floating_Border *ft_bd,
                        Eina_Bool iconify)
{
   E_Border *bd = NULL;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);
   bd = ft_bd->bd;
   EINA_SAFETY_ON_NULL_RETURN(bd);

   if (iconify)
     {
        if (!bd->iconic)
          e_border_iconify(bd);
     }
   else
     {
        if (bd->iconic)
          e_border_uniconify(bd);
     }
}

static void
_e_mod_floating_iconify_all(Eina_Bool iconify)
{
   Eina_List *l;
   E_Illume_Floating_Border *ft_bd = NULL;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        _e_mod_floating_iconify(ft_bd, iconify);
     }
}

static void
_e_mod_floating_smart_cleanup(Ecore_X_Event_Client_Message *event __UNUSED__)
{
   Eina_List *borders = NULL, *l;
   E_Illume_Floating_Border *ft_bd = NULL;
   E_Border *border = NULL;

   EINA_LIST_FOREACH(floating_wins, l, ft_bd)
     {
        border = ft_bd->bd;
        if (!border) continue;
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

        e_place_zone_region_smart(border->zone, borders, border->x, border->y,
                                  border->w, border->h, &new_x, &new_y);
        e_border_move(border, new_x, new_y);
     }

   return;
}

static void
_e_mod_floating_maximize_coords_handle(E_Illume_Floating_Border *ft_bd,
                                        int        x,
                                        int        y)
{
   E_Border *bd = NULL;
   const int margin = _e_illume_cfg->floating_control_threshold;

   EINA_SAFETY_ON_NULL_RETURN(ft_bd);
   EINA_SAFETY_ON_NULL_RETURN(ft_bd->bd);
   bd = ft_bd->bd;

   if ((y >= 0) && (y <= margin))
     {
        ft_bd->defer.maximize_top = 1;
        ft_bd->changed = 1;
     }
   else if ((y >= (bd->zone->h - margin)) && (y <= (bd->zone->h -1)))
     {
        ft_bd->defer.maximize_bottom = 1;
        ft_bd->changed = 1;
     }
}

static void
_e_mod_floating_maximize(E_Border *bd, E_Illume_Maximize max)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!(max & E_ILLUME_MAXIMIZE_DIRECTION)) max |= E_ILLUME_MAXIMIZE_BOTH;

   if ((bd->shaded) || (bd->shading)) return;
   ecore_x_window_shadow_tree_flush();
   if (bd->fullscreen)
     e_border_unfullscreen(bd);
   if (((bd->maximized & E_ILLUME_MAXIMIZE_DIRECTION) ==
        (max & E_MAXIMIZE_DIRECTION)) ||
       ((bd->maximized & E_ILLUME_MAXIMIZE_DIRECTION) ==
        E_ILLUME_MAXIMIZE_BOTH)) return;
   if (bd->new_client)
     {
        bd->need_maximize = 1;
        bd->maximized &= ~E_ILLUME_MAXIMIZE_TYPE;
        bd->maximized |= max;
        return;
     }

   bd->pre_res_change.valid = 0;
   if (!(bd->maximized & E_ILLUME_MAXIMIZE_HORIZONTAL))
     {
        bd->saved.x = bd->x - bd->zone->x;
        bd->saved.w = bd->w;
     }
   if (!(bd->maximized & E_ILLUME_MAXIMIZE_VERTICAL))
     {
        bd->saved.y = bd->y - bd->zone->y;
        bd->saved.h = bd->h;
     }

   bd->saved.zone = bd->zone->num;
   e_hints_window_size_set(bd);

   e_border_raise(bd);

   _e_mod_floating_maximize_internal(bd, max);

   bd->maximized &= ~E_ILLUME_MAXIMIZE_TYPE;
   bd->maximized |= max;

   e_hints_window_maximized_set(bd, bd->maximized & E_ILLUME_MAXIMIZE_HORIZONTAL,
                                bd->maximized & E_ILLUME_MAXIMIZE_VERTICAL);
   e_remember_update(bd);
}

static void
_e_mod_floating_maximize_internal(E_Border *bd, E_Illume_Maximize max)
{
   int x1, yy1;
   int w, h;

   switch (max & E_ILLUME_MAXIMIZE_TYPE)
     {
      case E_ILLUME_MAXIMIZE_FULLSCREEN:
         w = bd->zone->w;
         h = bd->zone->h;

         e_border_resize_limit(bd, &w, &h);
         x1 = bd->zone->x + (bd->zone->w - w) / 2;
         yy1 = bd->zone->y + (bd->zone->h - h) / 2;

         switch (max & E_ILLUME_MAXIMIZE_DIRECTION)
           {
            case E_ILLUME_MAXIMIZE_BOTH:
               e_border_move_resize(bd, x1, yy1, w, h);
               break;

            case E_ILLUME_MAXIMIZE_VERTICAL:
               e_border_move_resize(bd, bd->x, yy1, bd->w, h);
               break;

            case E_ILLUME_MAXIMIZE_HORIZONTAL:
               e_border_move_resize(bd, x1, bd->y, w, bd->h);
               break;

            case E_ILLUME_MAXIMIZE_BOTTOM:
               e_border_move_resize(bd, bd->zone->x, bd->zone->h / 2, w, h / 2);
               break;

            case E_ILLUME_MAXIMIZE_TOP:
               e_border_move_resize(bd, bd->zone->x, bd->zone->y, w, h / 2);
               break;
           }
         break;
     }
}
