#include "e_illume_private.h"
#include "e_mod_quickpanel.h"
#include "utilX.h"

#define HD_WIDTH (720)
#define HD_HEIGHT (1280)
#define QUICK_PANEL_SCALE_GET(s) ((double)s / HD_HEIGHT)
#define QUICK_PANEL_REL_Y ((double)50 / HD_HEIGHT) //0.05;
#define QUICK_PANEL_THRESHOLD_Y ((double)10 / HD_HEIGHT)//0.15;
#define QUICK_PANEL_THRESHOLD_MIN ((double)30 / HD_WIDTH)//0.0625;

/* for mini controller */
#define MINI_CONTROLLER_PRIORITY 200
#define POPUP_HANDLER_SIZE 50

/* local function prototypes */
static Eina_Bool _e_mod_quickpanel_atom_init (void);

static Eina_Bool _e_mod_quickpanel_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_resize(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_property (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_show(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_zone_set(void *data __UNUSED__, int type __UNUSED__, void *event);

static void _e_mod_quickpanel_cb_post_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_quickpanel_cb_free(E_Illume_Quickpanel *qp);

static void _e_mod_quickpanel_position_update(E_Illume_Quickpanel *qp);

static void _e_quickpanel_handle_input_region_set (Ecore_X_Window win, int x, int y, int w, int h);
static void _e_quickpanel_layout_position_set (Ecore_X_Window win, int x, int y);

static void _e_quickpanel_position_update(E_Illume_Quickpanel *qp);
static int _e_quickpanel_priority_sort_cb (const void* d1, const void* d2);
static int _e_mod_quickpanel_root_angle_get (E_Illume_Quickpanel* qp);

static Ecore_X_Window _e_mod_quickpanel_active_window_get(Ecore_X_Window root);
static void _e_mod_quickpanel_property_root_angle_change(Ecore_X_Event_Window_Property *event);
static void _e_mod_quickpanel_property_active_win_change(Ecore_X_Event_Window_Property *event);

static int _e_mod_quickpanel_bg_layout_add(E_Illume_Quickpanel *qp);
static int _e_mod_quickpanel_bg_layout_del(E_Illume_Quickpanel *qp);

static void _e_mod_quickpanel_send_message (E_Illume_Quickpanel *qp, Ecore_X_Illume_Quickpanel_State state);

static Eina_Bool _e_mod_quickpanel_popup_new (E_Illume_Quickpanel* qp);
static void _e_mod_quickpanel_popup_del (E_Illume_Quickpanel* qp);
static Eina_Bool _e_mod_quickpanel_popup_update (E_Illume_Quickpanel* qp);

static void _e_mod_quickpanel_hib_enter (void);
static void _e_mod_quickpanel_hib_leave (void);

static void _e_mod_quickpanel_check_lock_screen (E_Illume_Quickpanel* qp);
static E_Illume_Quickpanel_Info* _e_mod_quickpanel_current_mini_controller_get (E_Illume_Quickpanel* qp);
static void _e_mod_quickpanel_window_list_set (E_Illume_Quickpanel* qp);

/* local variables */
static Eina_List *_qp_hdls = NULL;
static E_Border_Hook *_qp_hook = NULL;

static Ecore_X_Atom effect_state_atom = 0;
static Ecore_X_Atom hibernation_state_atom = 0;

static Ecore_X_Atom mini_controller_win_atom = 0;
static Ecore_X_Atom quickpanel_list_atom = 0;

static Ecore_X_Atom quickpanel_layout_position_atom = 0;
static Ecore_X_Atom quickpanel_handle_input_region_atom = 0;

int e_mod_quickpanel_init(void)
{
   /* add handlers for messages we are interested in */
   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                               _e_mod_quickpanel_cb_client_message,
                                               NULL));
   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ADD,
                                               _e_mod_quickpanel_cb_border_add,
                                               NULL));
   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_REMOVE,
                                               _e_mod_quickpanel_cb_border_remove,
                                               NULL));

   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_RESIZE,
                                               _e_mod_quickpanel_cb_border_resize,
                                               NULL));

   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                               _e_mod_quickpanel_cb_property,
                                               NULL));

   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_SHOW,
                                               _e_mod_quickpanel_cb_border_show,
                                               NULL));
   _qp_hdls =
      eina_list_append(_qp_hdls,
                       ecore_event_handler_add(E_EVENT_BORDER_ZONE_SET,
                                               _e_mod_quickpanel_cb_border_zone_set,
                                               NULL));

   /* add hook for new borders so we can test for qp borders */
   _qp_hook = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH,
                                _e_mod_quickpanel_cb_post_fetch, NULL);


   /* init atoms */
   _e_mod_quickpanel_atom_init();

   return 1;
}

int e_mod_quickpanel_shutdown(void)
{
   Ecore_Event_Handler *hdl;

   /* delete the event handlers */
   EINA_LIST_FREE(_qp_hdls, hdl)
      ecore_event_handler_del(hdl);

   /* delete the border hook */
   if (_qp_hook) e_border_hook_del(_qp_hook);
   _qp_hook = NULL;

   return 1;
}

E_Illume_Quickpanel *e_mod_quickpanel_new(E_Zone *zone)
{
   E_Illume_Quickpanel *qp;

   /* try to allocate a new quickpanel object */
   qp = E_OBJECT_ALLOC(E_Illume_Quickpanel, E_ILLUME_QP_TYPE,
                       _e_mod_quickpanel_cb_free);
   if (!qp) return NULL;

   /* set quickpanel zone */
   qp->visible = 0;
   qp->zone = zone;
   qp->vert.dir = 0;
   qp->horiz_style = 0;

   qp->item_pos_y = (double)zone->h * QUICK_PANEL_REL_Y;
   qp->threshold_y = (double)zone->h * QUICK_PANEL_THRESHOLD_Y;
   qp->move_x_min = (double)zone->w * QUICK_PANEL_THRESHOLD_MIN;

   qp->scale = QUICK_PANEL_SCALE_GET(qp->zone->h);
   if(qp->scale == 0) qp->scale = 1;

   qp->key_hdl = NULL;

   return qp;
}


void e_mod_quickpanel_show(E_Illume_Quickpanel *qp, int isAni)
{
}


void e_mod_quickpanel_hide(E_Illume_Quickpanel *qp, int isAni)
{
}

/* local functions */
static Eina_Bool _e_mod_quickpanel_atom_init (void)
{
   effect_state_atom = ecore_x_atom_get ("_NET_CM_WINDOW_EFFECT_ENABLE");
   if( !effect_state_atom)
     {
        fprintf (stderr, "[ILLUME2][QP] Critical Error!!! Cannot create _NET_CM_WINDOW_EFFECT_ENABLE Atom...\n");
        return EINA_FALSE;
     }

   hibernation_state_atom = ecore_x_atom_get ("X_HIBERNATION_STATE");
   if( !hibernation_state_atom)
     {
        fprintf (stderr, "[ILLUME2][QP] Critical Error!!! Cannot create X_HIBERNATION_STATE Atom...\n");
        return EINA_FALSE;
     }

   mini_controller_win_atom = ecore_x_atom_get ("_E_ILLUME_MINI_CONTROLLER_WINDOW");
   if( !mini_controller_win_atom)
     {
        fprintf (stderr, "[ILLUME2][QP] Critical Error!!! Cannot create _E_ILLUME_MINI_CONTROLLER_WINDOW Atom...\n");
        return EINA_FALSE;
     }

   quickpanel_list_atom = ecore_x_atom_get ("_E_ILLUME_QUICKPANEL_WINDOW_LIST");
   if( !quickpanel_list_atom)
     {
        fprintf (stderr, "[ILLUME2][QP] Critical Error!!! Cannot create _E_ILLUME_QUICKPANEL_WINDOW_LIST Atom...\n");
        return EINA_FALSE;
     }

   quickpanel_layout_position_atom = ecore_x_atom_get ("_E_COMP_QUICKPANEL_LAYOUT_POSITION");
   if( !quickpanel_layout_position_atom)
     {
        fprintf (stderr, "[ILLUME2][QP] Critical Error!!! Cannot create _E_COMP_QUICKPANEL_LAYOUT_POSITION Atom...\n");
        return EINA_FALSE;
     }

   quickpanel_handle_input_region_atom = ecore_x_atom_get ("_E_COMP_WINDOW_INPUT_REGION");
   if( !quickpanel_handle_input_region_atom)
     {
        fprintf (stderr, "[ILLUME2][QP] Critical Error!!! Cannot create _E_COMP_WINDOW_INPUT_REGION Atom...\n");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool _e_mod_quickpanel_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Client_Message *ev = event;

   if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
     {
        // TODO: do something
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE_TOGGLE)
     {
        // TODO: do something
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_POSITION_UPDATE)
     {
        // TODO: do something
     }
   /* for hibernation state */
   else if (ev->message_type == hibernation_state_atom)
     {
        if (ev->data.l[0] == 1)
           _e_mod_quickpanel_hib_enter();
        else
           _e_mod_quickpanel_hib_leave();
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _e_mod_quickpanel_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev;
   E_Illume_Quickpanel *qp;
   E_Zone *zone;
   E_Illume_Quickpanel_Info *panel = NULL;
   int priority_major;

   ev = event;

   if (e_illume_border_is_indicator (ev->border))
     {
        if ((qp = e_illume_quickpanel_by_zone_get(ev->border->zone)))
          {
             qp->ind = ev->border;
             qp->vert.isize = qp->ind->h;
          }
        return ECORE_CALLBACK_PASS_ON;
     }

   if (e_illume_border_is_quickpanel_popup (ev->border))
     {
        L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d).. QUICKPANEL POPUP... win:%x\n", __func__, __LINE__, ev->border->client.win);
        if ((qp = e_illume_quickpanel_by_zone_get(ev->border->zone)))
          {
             if (qp->borders)
               {
                  E_Border* bd_last = NULL;
                  Eina_List *bd_list;

                  EINA_LIST_FOREACH(qp->borders, bd_list, panel)
                    {
                       if (!panel) continue;
                       bd_last = panel->bd;
                    }
                  L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d).. QUICKPANEL POPUP (win:%x) is placed under qp:%x\n", __func__, __LINE__, ev->border->client.win, bd_last ? bd_last->client.win:(unsigned int)NULL);
                  e_border_stack_below (ev->border, bd_last);
               }
             else if (qp->ind)
               {
                  L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d).. QUICKPANEL POPUP (win:%x) is placed under indicator:%x\n", __func__, __LINE__, ev->border->client.win, qp->ind->client.win);
                  e_border_stack_below (ev->border, qp->ind);
               }
          }

        return ECORE_CALLBACK_PASS_ON;
     }

   if (!ev->border->client.illume.quickpanel.quickpanel)
      return ECORE_CALLBACK_PASS_ON;

   if (!(zone = ev->border->zone)) return ECORE_CALLBACK_PASS_ON;

   /* if this border should be on a different zone, get requested zone */
   if ((int)zone->num != ev->border->client.illume.quickpanel.zone)
     {
        E_Container *con;
        int zn = 0;

        /* find this zone */
        if (!(con = e_container_current_get(e_manager_current_get())))
           return ECORE_CALLBACK_PASS_ON;
        zn = ev->border->client.illume.quickpanel.zone;
        zone = e_util_container_zone_number_get(con->num, zn);
        if (!zone) zone = e_util_container_zone_number_get(con->num, 0);
        if (!zone) return ECORE_CALLBACK_PASS_ON;
     }

   if (!(qp = e_illume_quickpanel_by_zone_get(zone)))
      return ECORE_CALLBACK_PASS_ON;

   // Disable effect of the quickpanel window
   ecore_x_icccm_window_role_set (ev->border->client.win, "NORMAL_WINDOW");

   if (ev->border->zone != zone)
      e_border_zone_set(ev->border, zone);

   int iy;
   e_illume_border_indicator_pos_get(zone, NULL, &iy);

   if ((ev->border->x != zone->x) || (ev->border->y != iy - zone->h))
      e_border_move(ev->border, zone->x, iy - zone->h);

   panel = calloc(1, sizeof(E_Illume_Quickpanel_Info));
   if(panel == NULL)
     {
        //perror("Failed to alloc memory!");
        return ECORE_CALLBACK_PASS_ON;
     }

   /* add this border to QP border collection */
   panel->bd = ev->border;

   /* for mini controller */
   priority_major = ecore_x_e_illume_quickpanel_priority_major_get (panel->bd->client.win);
   if (priority_major == MINI_CONTROLLER_PRIORITY)
     {
        Eina_List* l;
        E_Illume_Quickpanel_Info* temp;

        panel->mini_controller = EINA_TRUE;

        EINA_LIST_FOREACH(qp->borders, l, temp)
          {
             if (!temp) continue;

             if (temp->mini_controller == EINA_TRUE)
               {
                  qp->borders = eina_list_remove_list(qp->borders, l);
                  // add panel to hidden mini controller list
                  qp->hidden_mini_controllers = eina_list_prepend (qp->hidden_mini_controllers, temp);
                  break;
               }
          }

        // set mini controller atom to root window
        ecore_x_window_prop_window_set(qp->zone->container->manager->root, mini_controller_win_atom, &panel->bd->client.win, 1);
     }
   else
     {
        panel->mini_controller = EINA_FALSE;
     }

   qp->borders = eina_list_sorted_insert (qp->borders, _e_quickpanel_priority_sort_cb, panel);
   _e_mod_quickpanel_window_list_set (qp);

   _e_quickpanel_position_update(qp);

   // set transient_for to base window
   if (qp->popup)
     {
        L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d)... TRANSIENT_FOR.. win:%x, transient_win:%x\n", __func__, __LINE__, ev->border->client.win, qp->popup->evas_win);
        ecore_x_icccm_transient_for_set (ev->border->client.win, qp->popup->evas_win);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _e_mod_quickpanel_cb_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Remove *ev;
   E_Illume_Quickpanel *qp;
   E_Zone *zone;
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;
   Eina_Bool is_hidden_mini_controller;
   Eina_Bool update_mini_controller;

   ev = event;

   if (e_illume_border_is_indicator (ev->border))
     {
        if ((qp = e_illume_quickpanel_by_zone_get(ev->border->zone)))
          {
             qp->ind = NULL;

             // qp will be hidden when the indicator is removed
             if (qp->visible) e_mod_quickpanel_hide (qp, 1);
          }
        return ECORE_CALLBACK_PASS_ON;
     }

   if (!ev->border->client.illume.quickpanel.quickpanel)
      return ECORE_CALLBACK_PASS_ON;

   if (!(zone = ev->border->zone)) return ECORE_CALLBACK_PASS_ON;

   /* if this border should be on a different zone, get requested zone */
   if ((int)zone->num != ev->border->client.illume.quickpanel.zone)
     {
        E_Container *con;
        int zn = 0;

        /* find this zone */
        if (!(con = e_container_current_get(e_manager_current_get())))
           return ECORE_CALLBACK_PASS_ON;
        zn = ev->border->client.illume.quickpanel.zone;
        zone = e_util_container_zone_number_get(con->num, zn);
        if (!zone) zone = e_util_container_zone_number_get(con->num, 0);
        if (!zone) return ECORE_CALLBACK_PASS_ON;
     }

   if (!(qp = e_illume_quickpanel_by_zone_get(zone)))
      return ECORE_CALLBACK_PASS_ON;

   is_hidden_mini_controller = EINA_FALSE;
   update_mini_controller = EINA_FALSE;

   /* for mini controller */
   if (qp->hidden_mini_controllers)
     {
        EINA_LIST_FOREACH(qp->hidden_mini_controllers, l, panel)
          {
             if (!panel) continue;

             if (panel->bd == ev->border)
               {
                  qp->hidden_mini_controllers = eina_list_remove(qp->hidden_mini_controllers, panel);
                  free (panel);
                  is_hidden_mini_controller = EINA_TRUE;
                  break;
               }
          }
     }

   /* remove this border to QP border collection */
   if (qp->borders && !is_hidden_mini_controller)
     {
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             if (panel->bd == ev->border)
               {
                  qp->borders = eina_list_remove(qp->borders, panel);

                  if (panel->mini_controller)
                    {
                       // set the flag to update mini controller
                       update_mini_controller = EINA_TRUE;
                    }

                  free(panel);
                  break;
               }
          }
     }

   if (update_mini_controller)
     {
        // add first hidden mini controller to qp's border list
        if (qp->hidden_mini_controllers)
          {
             E_Illume_Quickpanel_Info *new_panel;
             new_panel = eina_list_nth (qp->hidden_mini_controllers, 0);

             if (new_panel)
               {
                  qp->hidden_mini_controllers = eina_list_remove(qp->hidden_mini_controllers, new_panel);

                  // set mini controller atom to root window
                  ecore_x_window_prop_window_set (qp->zone->container->manager->root, mini_controller_win_atom, &new_panel->bd->client.win, 1);

                  qp->borders = eina_list_sorted_insert (qp->borders, _e_quickpanel_priority_sort_cb, new_panel);

                  // set transient_for to base window
                  if (qp->popup && new_panel->bd)
                    {
                       L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d)... TRANSIENT_FOR.. win:%x, transient_win:%x\n", __func__, __LINE__, ev->border->client.win, qp->popup->evas_win);
                       ecore_x_icccm_transient_for_set (new_panel->bd->client.win, qp->popup->evas_win);
                    }
               }
             else
               {
                  // remove mini controller atom
                  ecore_x_window_prop_property_del (qp->zone->container->manager->root, mini_controller_win_atom);
               }
          }
        else
          {
             // remove mini controller atom
             ecore_x_window_prop_property_del (qp->zone->container->manager->root, mini_controller_win_atom);
          }
     }

   _e_mod_quickpanel_window_list_set (qp);

   _e_quickpanel_position_update(qp);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _e_mod_quickpanel_cb_border_resize(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Resize *ev;
   E_Illume_Quickpanel *qp;

   ev = event;
   if (!ev->border->client.illume.quickpanel.quickpanel)
      return ECORE_CALLBACK_PASS_ON;
   if (!(qp = e_illume_quickpanel_by_zone_get(ev->border->zone)))
      return ECORE_CALLBACK_PASS_ON;

   _e_quickpanel_position_update(qp);

   return ECORE_CALLBACK_PASS_ON;
}

static int _e_mod_quickpanel_root_angle_get (E_Illume_Quickpanel* qp)
{
   int ret;
   int count;
   int angle = 0;
   unsigned char *prop_data = NULL;

   if (!qp) return 0;

   ret = ecore_x_window_prop_property_get (qp->zone->container->manager->root, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if (ret && prop_data)
      memcpy (&angle, prop_data, sizeof (int));

   if (prop_data) free (prop_data);

   return angle;
}

static Ecore_X_Window
_e_mod_quickpanel_active_window_get(Ecore_X_Window root)
{
   Ecore_X_Window win;
   int ret;

   ret = ecore_x_window_prop_xid_get(root,
                                     ECORE_X_ATOM_NET_ACTIVE_WINDOW,
                                     ECORE_X_ATOM_WINDOW,
                                     &win, 1);

   if ((ret == 1) && win)
      return win;
   else
      return 0;
}

static void
_e_mod_quickpanel_property_root_angle_change(Ecore_X_Event_Window_Property *event)
{
   E_Illume_Quickpanel *qp;
   E_Zone* zone;
   int old_angle;

   zone = e_util_zone_current_get(e_manager_current_get());
   if (zone)
     {
        qp = e_illume_quickpanel_by_zone_get (zone);
        if (qp)
          {
             old_angle = qp->angle;
             qp->angle = _e_mod_quickpanel_root_angle_get (qp);

             if (qp->angle != old_angle)
               {
                  L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d)... angle:%d\n", __func__, __LINE__, qp->angle);
                  _e_quickpanel_position_update(qp);
               }
          }
     }
}

static void
_e_mod_quickpanel_property_active_win_change(Ecore_X_Event_Window_Property *event)
{
   Ecore_X_Window active_win;
   E_Border* active_bd;
   E_Illume_Quickpanel *qp;

   active_win = _e_mod_quickpanel_active_window_get(event->win);
   if (!active_win) return;

   active_bd = e_border_find_by_client_window(active_win);
   if (!active_bd) return;

   if (!(qp = e_illume_quickpanel_by_zone_get(active_bd->zone)))
     return;

   if (e_illume_border_is_lock_screen(active_bd))
     {
        if (!qp->is_lock)
          {
             L(LT_QUICKPANEL, "[ILLUME2][QP] line:%d.. LOCK SCREEN....\n", __LINE__);
             qp->is_lock = EINA_TRUE;
             _e_quickpanel_position_update(qp);
          }
     }
   else
     {
        if (qp->is_lock)
          {
             L(LT_QUICKPANEL, "[ILLUME2][QP] line:%d.. UNLOCK SCREEN....\n", __LINE__);
             qp->is_lock = EINA_FALSE;
             _e_quickpanel_position_update(qp);
          }
     }
}

static Eina_Bool _e_mod_quickpanel_cb_property(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Property *ev;
   ev = event;

   if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE)
     {
        _e_mod_quickpanel_property_root_angle_change(ev);
     }
   else if (ev->atom == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        _e_mod_quickpanel_property_active_win_change(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _e_mod_quickpanel_cb_border_show(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Show *ev;

   ev = event;
   if (ev->border->client.illume.quickpanel.quickpanel ||
       e_illume_border_is_quickpanel_popup (ev->border))
     {
        e_border_move (ev->border, -ev->border->zone->w, -ev->border->zone->h);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _e_mod_quickpanel_cb_border_zone_set(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Zone_Set *ev;

   ev = event;
   if (ev->border->client.illume.quickpanel.quickpanel ||
       e_illume_border_is_quickpanel_popup(ev->border))
     {
        e_border_move(ev->border, -ev->border->zone->w, -ev->border->zone->h);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void _e_mod_quickpanel_cb_post_fetch(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (bd->client.illume.quickpanel.quickpanel ||
       e_illume_border_is_quickpanel_popup (bd))
     {
        bd->stolen = 1;
     }
}

static void _e_mod_quickpanel_cb_free(E_Illume_Quickpanel *qp)
{
   E_Illume_Quickpanel_Info *panel;
   Eina_List *l;

   if (!qp) return;

   /* delete the animator if it exists */
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;

   /* delete the timer if it exists */
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;

   /* delete the key handler */
   if (qp->key_hdl) ecore_event_handler_del (qp->key_hdl);
   qp->key_hdl = NULL;

   if (qp->borders)
     {
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             qp->borders = eina_list_remove(qp->borders, panel);
             if (panel)
               {
                  panel->bd->stolen = 0;
                  free(panel);
               }
          }
     }

   if (qp->hidden_mini_controllers)
     {
        EINA_LIST_FOREACH(qp->hidden_mini_controllers, l, panel)
          {
             qp->hidden_mini_controllers = eina_list_remove(qp->hidden_mini_controllers, panel);
             free(panel);
          }
     }

   _e_mod_quickpanel_window_list_set (qp);
   _e_mod_quickpanel_popup_del (qp);

   /* free the structure */
   E_FREE(qp);
}

static void _e_mod_quickpanel_position_update(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;
   int iy = 0;

   if (!qp) return;
   if (!qp->visible) return;
   if (!qp->zone) return;

   e_illume_border_indicator_pos_get(qp->zone, NULL, &iy);

   EINA_LIST_FOREACH(qp->borders, l, panel)
     {
        if (!panel) continue;

        // TODO: check for landscape mode
        e_border_move(panel->bd, qp->zone->x, iy);
     }

   qp->vert.dir = 0;
   if ((iy + qp->vert.isize + qp->vert.size) > qp->zone->h) qp->vert.dir = 1;
}

static int _e_mod_quickpanel_bg_layout_del(E_Illume_Quickpanel *qp)
{
   if(qp == NULL) return -1;

   if(qp->ly_base)
     {
        evas_object_del(qp->ly_base);
        qp->ly_base = NULL;
     }
   return 0;
}

static int _e_mod_quickpanel_bg_layout_add(E_Illume_Quickpanel *qp)
{
   if (!qp) return -1;

   qp->ly_base = edje_object_add(qp->evas);
   if (!qp->ly_base) return -1;

   // set background file
   edje_object_scale_set(qp->ly_base, qp->scale);
   edje_object_file_set(qp->ly_base, EDJ_FILE, "e/modules/illume2-slp/quickpanel/base");
   evas_object_resize(qp->ly_base, qp->zone->w, qp->zone->h);

   evas_object_show(qp->ly_base);
   return 0;
}


static void _e_mod_quickpanel_send_message (E_Illume_Quickpanel *qp, Ecore_X_Illume_Quickpanel_State state)
{
   Eina_List* l;
   E_Illume_Quickpanel_Info* panel;

   if (qp == NULL) return;

   EINA_LIST_FOREACH(qp->borders, l, panel)
     {
        if (!panel) continue;
        ecore_x_e_illume_quickpanel_state_send(panel->bd->client.win, state);
     }

   /* send quickpanel state message to the indicator */
   if (qp->ind)
      ecore_x_e_illume_quickpanel_state_send(qp->ind->client.win, state);
}

static Eina_Bool _e_mod_quickpanel_popup_new (E_Illume_Quickpanel* qp)
{
   if (!qp) return EINA_FALSE;

   if (qp->popup) return EINA_TRUE;

   qp->angle = _e_mod_quickpanel_root_angle_get(qp);
   _e_mod_quickpanel_check_lock_screen(qp);

   qp->popup = e_win_new (qp->zone->container);
   if (!qp->popup) return EINA_FALSE;

   ecore_x_icccm_hints_set(qp->popup->evas_win, 0, 0, 0, 0, 0, 0, 0);
   ecore_x_icccm_name_class_set(qp->popup->evas_win, "QUICKPANEL_BASE", "QUICKPANEL_BASE");
   e_win_show(qp->popup);
   e_win_move_resize (qp->popup, qp->zone->x, qp->zone->y - qp->zone->h,
                            qp->zone->w, qp->zone->h);

   e_win_title_set (qp->popup, "quickpanel_base");

   qp->ee = qp->popup->ecore_evas;
   qp->evas = qp->popup->evas;

   ecore_x_netwm_window_type_set(qp->popup->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   // disable effect
   int state = 0;
   ecore_x_window_prop_property_set (qp->popup->evas_win, effect_state_atom, ECORE_X_ATOM_CARDINAL, 32, &state, 1);

   _e_mod_quickpanel_bg_layout_add(qp);

   return EINA_TRUE;
}

static void _e_mod_quickpanel_popup_del (E_Illume_Quickpanel* qp)
{
   if (!qp) return;
   if (!qp->popup) return;

   _e_mod_quickpanel_bg_layout_del(qp);

   e_object_del (E_OBJECT(qp->popup));
   qp->popup = NULL;
}

static Eina_Bool _e_mod_quickpanel_popup_update (E_Illume_Quickpanel* qp)
{
   if (!qp) return EINA_FALSE;
   if (!qp->popup) return EINA_FALSE;

   int w, h, extra_h;
   int tx, ty;
   int handle_x, handle_y, handle_w, handle_h;

   if (qp->is_lock)
     {
        E_Illume_Quickpanel_Info* mini_controller;
        mini_controller = _e_mod_quickpanel_current_mini_controller_get (qp);
        if (mini_controller)
          {
             w = mini_controller->bd->w;
             h = mini_controller->bd->h;
          }
        else
          {
             return EINA_FALSE;
          }
     }
   else
     {
        w = qp->zone->w;
        h = qp->zone->h;
     }

   // check current angle
   if(qp->angle == 0 || qp->angle == 180)
     {
        handle_w = qp->zone->w;
        handle_h = POPUP_HANDLER_SIZE * qp->scale;

        if (qp->ind) extra_h = qp->ind->h + handle_h;
        else extra_h = handle_h;

        if (!qp->is_lock)
          {
             if (qp->angle == 0)
               {
                  handle_x = qp->zone->x;
                  handle_y = qp->zone->h - handle_h;
               }
             else // qp->angle == 180
               {
                  handle_x = qp->zone->x;
                  handle_y = qp->zone->y;
               }

             e_win_resize (qp->popup, w, h);
             evas_object_resize (qp->ly_base, w, h);

             _e_quickpanel_handle_input_region_set (qp->popup->evas_win, handle_x, handle_y, handle_w, handle_h);
          }
        else
          {
             if (qp->angle == 0)
               {
                  tx = qp->zone->x;
                  ty = qp->zone->y;

                  handle_x = qp->zone->x;
                  handle_y = h + extra_h - handle_h;
               }
             else // qp->angle == 180
               {
                  tx = qp->zone->x;
                  ty = qp->zone->h - (h + extra_h);

                  handle_x = qp->zone->x;
                  handle_y = qp->zone->h - (h+ extra_h);
               }

             e_win_resize(qp->popup, w, h + extra_h);
             evas_object_resize (qp->ly_base, w, h + extra_h);

             _e_quickpanel_handle_input_region_set (qp->popup->evas_win, handle_x, handle_y, handle_w, handle_h);
          }
     }
   else
     {
        handle_w = POPUP_HANDLER_SIZE * qp->scale;
        handle_h = qp->zone->h;

        if (qp->ind) extra_h = qp->ind->w + handle_w;
        else extra_h = handle_w;

        if (!qp->is_lock)
          {
             if (qp->angle == 90)
               {
                  handle_x = qp->zone->w - handle_w;
                  handle_y = qp->zone->y;
               }
             else // qp->angle == 270
               {
                  handle_x = qp->zone->x;
                  handle_y = qp->zone->y;
               }

             e_win_resize (qp->popup, w, h);
             evas_object_resize (qp->ly_base, h, w);

             _e_quickpanel_handle_input_region_set (qp->popup->evas_win, handle_x, handle_y, handle_w, handle_h);
          }
        else
          {
             if (qp->angle == 90)
               {
                  handle_x = w + extra_h - handle_w;
                  handle_y = qp->zone->y;
               }
             else // qp->angle == 270
               {
                  handle_x = qp->zone->w - (w + extra_h);
                  handle_y = qp->zone->y;
               }

             e_win_resize(qp->popup, w + extra_h, h);
             evas_object_resize( qp->ly_base, h, w + extra_h);

             _e_quickpanel_handle_input_region_set (qp->popup->evas_win, handle_x, handle_y, handle_w, handle_h);
          }
     }
   L(LT_QUICKPANEL, "[ILLUME2][QP] %s(%d)... angle:%d\n", __func__, __LINE__, qp->angle);
   ecore_evas_rotation_with_resize_set(qp->popup->ecore_evas, qp->angle);

   return EINA_TRUE;
}


static int
_e_quickpanel_priority_sort_cb (const void* d1, const void* d2)
{
   int priority1, priority2, ret;
   const E_Illume_Quickpanel_Info *panel1, *panel2;

   const E_Border* border1 = NULL;
   const E_Border* border2 = NULL;

   if(!d1) return(1);
   if(!d2) return(-1);

   panel1 = d1;
   panel2 = d2;

   border1 = panel1->bd;
   border2 = panel2->bd;

   priority1 = ecore_x_e_illume_quickpanel_priority_major_get (border1->client.win);
   priority2 = ecore_x_e_illume_quickpanel_priority_major_get (border2->client.win);

   ret = priority2 - priority1;
   if (ret == 0)
     {
        /* check minor priority if major priorities are same. */
        priority1 = ecore_x_e_illume_quickpanel_priority_minor_get (border1->client.win);
        priority2 = ecore_x_e_illume_quickpanel_priority_minor_get (border2->client.win);
        ret = priority2 - priority1;
     }

   return ret;
}

static void
_e_quickpanel_handle_input_region_set(Ecore_X_Window win, int x, int y, int w, int h)
{
   unsigned int position_list[4];

   position_list[0] = x;
   position_list[1] = y;
   position_list[2] = w;
   position_list[3] = h;

   L(LT_QUICKPANEL, "[ILLUME2][QP] Position SET... win:%x, x:%d, y:%d, w:%d, h:%d\n", win, x, y, w, h);

   ecore_x_window_prop_card32_set(win,
                                  quickpanel_handle_input_region_atom,
                                  position_list,
                                  4);
}

static void
_e_quickpanel_layout_position_set(Ecore_X_Window win, int x, int y)
{
   unsigned int position_list[2];

   position_list[0] = x;
   position_list[1] = y;

   L(LT_QUICKPANEL, "[ILLUME2][QP] LAYOUT SET... win:%x, x:%d, y:%d\n", win, x, y);

   ecore_x_window_prop_card32_set( win,
                                   quickpanel_layout_position_atom,
                                   position_list,
                                   2);
}

static void
_e_quickpanel_position_update(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;
   int ty = 0;

   if (!qp) return;

   if (qp->borders)
     {
        ty = qp->vert.isize;
        if (qp->vert.dir == 1) ty = 0;

        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             if (qp->is_lock)
               {
                  if (!panel->mini_controller)
                    {
                       _e_quickpanel_layout_position_set(panel->bd->client.win, -qp->zone->w, -qp->zone->h);
                       continue;
                    }
               }

             if (qp->angle == 0)
               {
                  _e_quickpanel_layout_position_set (panel->bd->client.win, 0, ty);
                  ty += panel->bd->h;
               }
             else if (qp->angle == 180)
               {
                  ty += panel->bd->h;
                  _e_quickpanel_layout_position_set (panel->bd->client.win, 0, qp->zone->h - ty);
               }
             else if (qp->angle == 90)
               {
                  _e_quickpanel_layout_position_set (panel->bd->client.win, ty, 0);
                  ty += panel->bd->w;
               }
             else if (qp->angle == 270)
               {
                  ty += panel->bd->w;
                  _e_quickpanel_layout_position_set (panel->bd->client.win, qp->zone->w - ty, 0);
               }
          }
        _e_mod_quickpanel_popup_update (qp);
     }
}

static void _e_mod_quickpanel_hib_enter (void)
{
   efreet_mime_shutdown();
   efreet_shutdown();
}

static void _e_mod_quickpanel_hib_leave (void)
{
   efreet_init();
   efreet_mime_init();
}

static void _e_mod_quickpanel_check_lock_screen (E_Illume_Quickpanel* qp)
{
   Ecore_X_Window active_win;
   E_Border* active_bd;
   int ret;

   if (!qp) return;

   qp->is_lock = EINA_FALSE;

   ret = ecore_x_window_prop_xid_get(qp->zone->container->manager->root,
                                     ECORE_X_ATOM_NET_ACTIVE_WINDOW,
                                     ECORE_X_ATOM_WINDOW,
                                     &active_win, 1);

   if ((ret == 1) && active_win)
     {
        active_bd = e_border_find_by_client_window (active_win);
        if (active_bd)
          {
             if (e_illume_border_is_lock_screen(active_bd))
                qp->is_lock = EINA_TRUE;
          }
     }
}

static E_Illume_Quickpanel_Info* _e_mod_quickpanel_current_mini_controller_get (E_Illume_Quickpanel* qp)
{
   E_Illume_Quickpanel_Info* panel;
   Eina_List* l;

   if (!qp) return NULL;

   if (qp->borders)
     {
        EINA_LIST_FOREACH (qp->borders, l, panel)
          {
             if (!panel) continue;

             if (panel->mini_controller)
               {
                  return panel;
               }
          }
     }

   return NULL;
}

static void _e_mod_quickpanel_window_list_set (E_Illume_Quickpanel* qp)
{
   unsigned int num, i;
   Ecore_X_Window *quickpanels = NULL;
   E_Illume_Quickpanel_Info* panel;
   Eina_List* l;

   if (!qp) return;

   num = eina_list_count (qp->borders);
   if (num < 1)
     {
        ecore_x_window_prop_window_set (qp->zone->container->manager->root, quickpanel_list_atom,
                                        NULL, 0);
        return;
     }

   quickpanels = calloc (num, sizeof(Ecore_X_Window));
   if (!quickpanels)
      return;

   i = 0;
   EINA_LIST_FOREACH (qp->borders, l, panel)
     {
        if (!panel) continue;

        if (panel->bd)
          {
             quickpanels[i++] = panel->bd->client.win;
          }
     }

   ecore_x_window_prop_window_set (qp->zone->container->manager->root, quickpanel_list_atom,
                                   quickpanels, i);

   E_FREE(quickpanels);
}

