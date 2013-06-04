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

static Eina_Bool _e_mod_floating_cb_client_message(void   *data,
                                                     int    type,
                                                     void   *event);
static Eina_Bool _e_mod_floating_cb_window_property(void  *data,
                                                      int   type,
                                                      void  *event);

/* general function */
static int _e_mod_floating_atom_init(void);
static void _e_mod_floating_border_list_add(E_Border *bd);
static void _e_mod_floating_border_list_del(E_Border *bd);
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
                       ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                               _e_mod_floating_cb_client_message,
                                               NULL));
   _fw_hdls =
      eina_list_append(_fw_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                               _e_mod_floating_cb_window_property,
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
