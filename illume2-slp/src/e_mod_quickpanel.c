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

static void _e_mod_quickpanel_cb_post_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_quickpanel_cb_free(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_slide(E_Illume_Quickpanel *qp, int visible, double len);
static Eina_Bool _e_mod_quickpanel_cb_animate(void *data);
static void _e_mod_quickpanel_position_update(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_animate_down(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_animate_up(E_Illume_Quickpanel *qp);

static void _e_quickpanel_position_update(E_Illume_Quickpanel *qp);
static int _e_quickpanel_priority_sort_cb (const void* d1, const void* d2);
static void _e_quickpanel_after_animate (E_Illume_Quickpanel* qp);
static int _e_mod_quickpanel_root_angle_get (E_Illume_Quickpanel* qp);

static int _e_mod_quickpanel_bg_layout_add(E_Illume_Quickpanel *qp);
static int _e_mod_quickpanel_bg_layout_del(E_Illume_Quickpanel *qp);

static void _e_mod_quickpanel_cb_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_mod_quickpanel_cb_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_mod_quickpanel_cb_event_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _e_mod_quickpanel_send_message (E_Illume_Quickpanel *qp, Ecore_X_Illume_Quickpanel_State state);

static void _e_mod_quickpanel_border_show (E_Illume_Quickpanel* qp, E_Border* bd);
static void _e_mod_quickpanel_border_hide (E_Border* bd);

static Eina_Bool _e_mod_quickpanel_popup_new (E_Illume_Quickpanel* qp);
static void _e_mod_quickpanel_popup_del (E_Illume_Quickpanel* qp);
static Eina_Bool _e_mod_quickpanel_popup_update (E_Illume_Quickpanel* qp);

static Eina_Bool _e_mod_quickpanel_cb_key_down (void *data, int type __UNUSED__, void *event);

static void _e_mod_quickpanel_hib_enter (void);
static void _e_mod_quickpanel_hib_leave (void);

static void _e_mod_quickpanel_check_lock_screen (E_Illume_Quickpanel* qp);
static E_Illume_Quickpanel_Info* _e_mod_quickpanel_current_mini_controller_get (E_Illume_Quickpanel* qp);
static void _e_mod_quickpanel_window_list_set (E_Illume_Quickpanel* qp);

static void _e_mod_quickpanel_extra_panel_show(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_extra_panel_hide(E_Illume_Quickpanel *qp);

/* local variables */
static Eina_List *_qp_hdls = NULL;
static E_Border_Hook *_qp_hook = NULL;

static Ecore_X_Atom effect_state_atom = 0;
static Ecore_X_Atom hibernation_state_atom = 0;

static Ecore_X_Atom mini_controller_win_atom = 0;
static Ecore_X_Atom quickpanel_list_atom = 0;

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
   char str_scale[128];

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

   /* FIXME */
   snprintf(str_scale, 127, "%lf", qp->scale);
   e_util_env_set ("ELM_SCALE", str_scale);

   qp->key_hdl = NULL;

   return qp;
}


void e_mod_quickpanel_show(E_Illume_Quickpanel *qp, int isAni)
{
   int duration;

   if (!qp) return;

   if (qp->ani_type == E_ILLUME_QUICKPANEL_ANI_HIDE)
     {
        if (qp->visible) qp->visible = 0;
        else qp->visible = 1;
     }
   else if (qp->ani_type == E_ILLUME_QUICKPANEL_ANI_SHOW)
     {
        return;
     }

   /* delete the animator if it exists */
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;

   /* delete any existing timer */
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;

   /* if it's already visible, or has no borders to show, then get out */
   //if ((qp->visible) || (!qp->borders)) return;
   if (qp->visible)
     {
        qp->ani_type = E_ILLUME_QUICKPANEL_ANI_NONE;
        return;
     }

   if (_e_mod_quickpanel_popup_new(qp) == EINA_FALSE) return;

   qp->ani_type = E_ILLUME_QUICKPANEL_ANI_SHOW;

   if (qp->angle == 0 || qp->angle == 180)
      qp->popup_len = qp->popup->h;
   else
      qp->popup_len = qp->popup->w;

   duration = _e_illume_cfg->animation.quickpanel.duration;

   /* grab the height of the indicator */
   if (!qp->ind) qp->vert.isize = 0;
   else
     {
        if (qp->angle == 0 || qp->angle == 180)
          {
             qp->vert.isize = qp->ind->h;
          }
        else
          {
             qp->vert.isize = qp->ind->w;
          }
     }

   /* check animation duration */
   if (duration <= 0 || !isAni)
     {
        Eina_List *l;
        E_Illume_Quickpanel_Info *panel;
        int ny = 0, nx = 0;

        ny = qp->vert.isize;
        if (qp->vert.dir == 1) ny = 0;

        _e_quickpanel_after_animate (qp);

        // FIXME: shouldnt have special case - just call anim with adjust @ end
        // also don't use e_border_fx_offset but actuall;y move bd
        /* if we are not animating, just show the borders */
        if (qp->horiz_style)
          {
             EINA_LIST_FOREACH(qp->borders, l, panel)
               {
                  if (!panel) continue;
                  if (!panel->bd->visible) _e_mod_quickpanel_border_show(qp, panel->bd);
                  if (qp->vert.dir == 0)
                    {
                       e_border_fx_offset(panel->bd, qp->horiz.adjust + nx, ny);
                       nx += panel->bd->w;
                    }
                  else
                    {
                       nx -= panel->bd->w;
                       e_border_fx_offset(panel->bd, qp->horiz.adjust + nx, ny);
                    }
               }
          }
        else
          {
             EINA_LIST_FOREACH(qp->borders, l, panel)
               {
                  if (!panel) continue;
                  if (!panel->bd->visible) _e_mod_quickpanel_border_show(qp, panel->bd);
                  if (qp->vert.dir == 0)
                    {
                       if (qp->angle == 0)
                         {
                            e_border_move (panel->bd, qp->horiz.adjust, ny);
                            ny += panel->bd->h;
                         }
                       else if (qp->angle == 180)
                         {
                            ny += panel->bd->h;
                            e_border_move (panel->bd, qp->horiz.adjust, qp->zone->h - ny);
                         }
                       else if (qp->angle == 90)
                         {
                            e_border_move (panel->bd, ny, qp->horiz.adjust);
                            ny += panel->bd->w;
                         }
                       else if (qp->angle == 270)
                         {
                            ny += panel->bd->w;
                            e_border_move (panel->bd, qp->zone->w - ny, qp->horiz.adjust);
                         }
                    }
                  else
                    {
                       ny -= panel->bd->h;
                       e_border_fx_offset(panel->bd, qp->horiz.adjust, ny);
                    }
               }
          }
     }
   else
     {
        if (!qp->popup->visible)
          {
             e_popup_show(qp->popup);

             if (qp->ind)
               {
                  /* restack popup win under the bottom quickpanel win */
                  ecore_x_window_configure (qp->popup->evas_win,
                                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING | ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                            0, 0, 0, 0, 0,
                                            qp->ind->win, ECORE_X_WINDOW_STACK_BELOW);
               }
          }
        _e_mod_quickpanel_slide(qp, 1, (double)duration / 1000.0);
     }
}


void e_mod_quickpanel_hide(E_Illume_Quickpanel *qp, int isAni)
{
   int duration;

   if (!qp) return;
   if (!qp->popup) return;

   if (qp->ani_type == E_ILLUME_QUICKPANEL_ANI_SHOW)
     {
        if (qp->visible) qp->visible = 0;
        else qp->visible = 1;
     }
   else if (qp->ani_type == E_ILLUME_QUICKPANEL_ANI_HIDE)
     {
        return;
     }

   /* delete the animator if it exists */
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;

   /* delete the timer if it exists */
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;

   /* if it's not visible, we can't hide it */
   if (!qp->visible)
     {
        qp->ani_type = E_ILLUME_QUICKPANEL_ANI_NONE;
        return;
     }

   qp->ani_type = E_ILLUME_QUICKPANEL_ANI_HIDE;

   if (qp->angle == 0 || qp->angle == 180)
      qp->popup_len = qp->popup->h;
   else
      qp->popup_len = qp->popup->w;

   duration = _e_illume_cfg->animation.quickpanel.duration;

   _e_mod_quickpanel_extra_panel_hide(qp);

   if (duration <= 0 || !isAni)
     {
        qp->vert.adjust = 0;
        _e_quickpanel_after_animate (qp);
     }
   else
     {
        _e_mod_quickpanel_slide(qp, 0, (double)duration / 1000.0);
     }
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

   return EINA_TRUE;
}

static Eina_Bool _e_mod_quickpanel_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Client_Message *ev = event;
   E_Zone *zone;
   E_Illume_Quickpanel *qp;
   E_Border *bd;

   if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
     {
        if ((zone = e_util_zone_window_find(ev->win)))
          {
             if ((qp = e_illume_quickpanel_by_zone_get(zone)))
               {
                  if (ev->data.l[0] == (int)ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF)
                     e_mod_quickpanel_hide(qp, 1);
                  else
                     e_mod_quickpanel_show(qp, 1);
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE_TOGGLE)
     {
        if ((zone = e_util_zone_window_find(ev->win)))
          {
             if ((qp = e_illume_quickpanel_by_zone_get(zone)))
               {
                  if (qp->visible)
                     e_mod_quickpanel_hide(qp, 1);
                  else
                     e_mod_quickpanel_show(qp, 1);
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_POSITION_UPDATE)
     {
        if (!(bd = e_border_find_by_client_window(ev->win)))
           return ECORE_CALLBACK_PASS_ON;
        if (!(qp = e_illume_quickpanel_by_zone_get(bd->zone)))
           return ECORE_CALLBACK_PASS_ON;
        _e_mod_quickpanel_position_update(qp);
     }
   /* for hibernation state */
   else if (ev->message_type == hibernation_state_atom)
     {
        if (ev->data.l[0] == 1)
           _e_mod_quickpanel_hib_enter();
        else
           _e_mod_quickpanel_hib_leave();
     }
   else if (ev->message_type == ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE)
     {
        Eina_List *l;
        E_Illume_Quickpanel_Info *panel;

        if (!(bd = e_border_find_by_client_window(ev->win)))
           return ECORE_CALLBACK_PASS_ON;
        if (!(qp = e_illume_quickpanel_by_zone_get(bd->zone)))
           return ECORE_CALLBACK_PASS_ON;

        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             if (panel->bd->client.win == ev->win)
               {
                  panel->draw_done = EINA_TRUE;
                  break;
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}


static void _e_mod_quickpanel_size_adjust(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;

   // FIXME: copy & pasted in _e_mod_quickpanel_cb_border_resize() too - make fn
   qp->horiz.size = 0;
   if (qp->horiz_style)
     {
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             if (qp->is_lock)
               {
                  if (!panel->mini_controller) continue;
               }

             //if (panel->bd->h > qp->vert.size) qp->vert.size = bd->h;
             qp->horiz.size += panel->bd->w;
          }
     }
   else
     {
        qp->vert.size = qp->vert.isize;

        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             if (qp->is_lock)
               {
                  if (!panel->mini_controller) continue;
               }

             if (qp->angle == 0 || qp->angle == 180)
               {
                  if (panel->bd->w > qp->horiz.size) qp->horiz.size = panel->bd->w;
                  qp->vert.size += panel->bd->h;
               }
             else
               {
                  if (panel->bd->w > qp->horiz.size) qp->horiz.size = panel->bd->w;
                  qp->vert.size += panel->bd->w;
               }
          }
     }
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
   panel->draw_done = EINA_FALSE;
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
                  _e_mod_quickpanel_border_hide(temp->bd);
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

   if (!qp->visible)
     {
        ecore_x_e_illume_quickpanel_state_send(panel->bd->client.win, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
        if (qp->ind) e_border_stack_below (ev->border, qp->ind);
        /* hide this border */
        _e_mod_quickpanel_border_hide(ev->border);
     }
   else
     {
        _e_mod_quickpanel_size_adjust(qp);
        _e_quickpanel_position_update (qp);
        if (!qp->is_lock || panel->mini_controller)
           _e_mod_quickpanel_border_show (qp, panel->bd);
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

                  if (qp->visible) _e_mod_quickpanel_border_show (qp, new_panel->bd);
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

   _e_mod_quickpanel_size_adjust(qp);
   _e_quickpanel_position_update (qp);

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

   _e_mod_quickpanel_size_adjust(qp);
   _e_quickpanel_position_update (qp);

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

static Eina_Bool _e_mod_quickpanel_cb_property (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Property *ev;

   ev = event;
   if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE)
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
                  if (qp->visible)
                    {
                       old_angle = qp->angle;
                       qp->angle = _e_mod_quickpanel_root_angle_get (qp);

                       if (qp->angle != old_angle)
                         {
                            _e_mod_quickpanel_popup_update (qp);
                         }
                    }
               }
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void _e_mod_quickpanel_cb_post_fetch(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (!bd->client.illume.quickpanel.quickpanel) return;
   bd->stolen = 1;
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

static void _e_mod_quickpanel_slide(E_Illume_Quickpanel *qp, int visible, double len)
{
   if (!qp) return;
   qp->start = ecore_loop_time_get();
   qp->len = len;
   qp->vert.adjust_start = qp->vert.adjust;
   qp->vert.adjust_end = 0;
   if (qp->vert.dir == 0)
     {
        if (visible) qp->vert.adjust_end = qp->vert.size;
     }
   else
     {
        if (visible) qp->vert.adjust_end = -qp->vert.size;
     }

   if (!qp->animator)
      qp->animator = ecore_animator_add(_e_mod_quickpanel_cb_animate, qp);
   _e_mod_quickpanel_cb_animate(qp);
}

static void _e_quickpanel_after_animate (E_Illume_Quickpanel* qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;

   if (!qp) return;

   qp->animator = NULL;

   if (qp->visible)
     {
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;
             if (panel->bd->visible) _e_mod_quickpanel_border_hide(panel->bd);
          }
        qp->visible = 0;

        e_popup_hide(qp->popup);

        // send message to quickpanel borders
        _e_mod_quickpanel_send_message (qp, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);

        _e_mod_quickpanel_popup_del (qp);
     }
   else
     {
        qp->visible = 1;

        if (!qp->popup->visible)
          {
             e_popup_show(qp->popup);

             if (qp->ind)
               {
                  /* restack popup win under the bottom quickpanel win */
                  ecore_x_window_configure (qp->popup->evas_win,
                                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING | ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                            0, 0, 0, 0, 0,
                                            qp->ind->win, ECORE_X_WINDOW_STACK_BELOW);
               }
          }

        _e_mod_quickpanel_extra_panel_show(qp);

        // send message to quickpanel borders
        _e_mod_quickpanel_send_message (qp, ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
     }

   qp->down = 0;
   qp->ani_type = E_ILLUME_QUICKPANEL_ANI_NONE;
}

static Eina_Bool _e_mod_quickpanel_cb_animate(void *data)
{
   E_Illume_Quickpanel *qp;
   double t, v, diff = 1.0;

   if (!(qp = data)) return ECORE_CALLBACK_CANCEL;
   t = (ecore_loop_time_get() - qp->start);
   if (t > qp->len) t = qp->len;
   if (qp->len > 0.0)
     {
        v = (t / qp->len);
        v = (1.0 - v);
        v = (v * v * v * v);
        v = (1.0 - v);
     }
   else
     {
        t = qp->len;
        v = 1.0;
     }

   diff = 1.0 - v;
   qp->vert.adjust = ((qp->vert.adjust_end * v) +
                      (qp->vert.adjust_start * diff));

   if (qp->vert.dir == 0) _e_mod_quickpanel_animate_down(qp);
   else _e_mod_quickpanel_animate_up(qp);

   if (qp->visible)
     {
        int x, y;

        if (qp->angle == 0)
          {
             x = qp->zone->x;
             y = qp->zone->y - qp->popup_len + (qp->popup_len * diff);
          }
        else if (qp->angle == 180)
          {
             x = qp->zone->x;
             y = qp->zone->h - qp->zone->y - (qp->popup_len * diff);
          }
        else if (qp->angle == 90)
          {
             x = qp->zone->x - qp->popup_len + (qp->popup_len * diff);
             y = qp->zone->y;
          }
        else if (qp->angle == 270)
          {
             x = qp->zone->w - qp->zone->x - (qp->popup_len * diff);
             y = qp->zone->y;
          }
        else
          {
             // error!!!
             x = qp->zone->x;
             y = qp->zone->y - qp->popup_len + (qp->popup_len * diff);
          }
        e_popup_move(qp->popup, x, y);
     }
   else
     {
        int x, y;

        if (qp->angle == 0)
          {
             x = qp->zone->x;
             y = qp->zone->y - qp->popup_len + (qp->popup_len * v);
          }
        else if (qp->angle == 180)
          {
             x = qp->zone->x;
             y = qp->zone->h - qp->zone->y - (qp->popup_len * v);
          }
        else if (qp->angle == 90)
          {
             x = qp->zone->x - qp->popup_len + (qp->popup_len * v);
             y = qp->zone->y;
          }
        else if (qp->angle == 270)
          {
             x = qp->zone->w - qp->zone->x - (qp->popup_len * v);
             y = qp->zone->y;
          }
        else
          {
             // error!!!
             x = qp->zone->x;
             y = qp->zone->y - qp->popup_len + (qp->popup_len * v);
          }
        e_popup_move(qp->popup, x, y);
     }

   if (t == qp->len)
     {
        _e_quickpanel_after_animate (qp);
        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
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

static void _e_mod_quickpanel_animate_down(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;
   int pbh = 0, pbw = 0;

   if (!qp) return;

   if (qp->horiz_style)
     {
        int p = 0;

        if (qp->vert.size > 0)
          {
             p = (qp->vert.adjust * (qp->vert.size + qp->vert.isize))
                / qp->vert.size + qp->item_pos_y;
          }

        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             /* don't adjust borders that are being deleted */
             if (e_object_is_del(E_OBJECT(panel->bd))) continue;

             if (qp->is_lock)
               {
                  if (!panel->mini_controller) continue;
               }

             e_border_move(panel->bd, qp->horiz.adjust + pbw, p - qp->vert.size);

             pbw += qp->zone->w;

             if (qp->down == 0)
               {
                  if (!qp->visible)
                    {
                       if (!panel->bd->visible)
                         {
                            _e_mod_quickpanel_border_show(qp, panel->bd);
                         }
                    }
               }
          }
     }
   else
     {
        int p = 0;
        int h = 0;

        if (qp->vert.size > 0)
           p = (qp->vert.adjust * (qp->vert.size + qp->vert.isize))
              / qp->vert.size;
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             /* don't adjust borders that are being deleted */
             if (e_object_is_del(E_OBJECT(panel->bd))) continue;

             if (qp->is_lock)
               {
                  if (!panel->mini_controller) continue;
               }

             if (qp->angle == 0)
               {
                  h = p - qp->vert.size + pbh;
                  e_border_move(panel->bd, qp->horiz.adjust, h);
                  pbh += panel->bd->h;
               }
             else if (qp->angle == 180)
               {
                  pbh += panel->bd->h;
                  h = qp->zone->h - p + qp->vert.size - pbh;
                  e_border_move(panel->bd, qp->horiz.adjust, h);
               }
             else if (qp->angle == 90)
               {
                  h = p - qp->vert.size + pbh;
                  e_border_move(panel->bd, h, qp->horiz.adjust);
                  pbh += panel->bd->w;
               }
             else if (qp->angle == 270)
               {
                  pbh += panel->bd->w;
                  h = qp->zone->w - p + qp->vert.size - pbh;
                  e_border_move(panel->bd, h, qp->horiz.adjust);
               }

             if (qp->down == 0)
               {
                  if (!qp->visible)
                    {
                       if (!panel->bd->visible)
                         {
                            _e_mod_quickpanel_border_show(qp, panel->bd);
                         }
                    }
               }
          }
     }
}

static void _e_mod_quickpanel_animate_up(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;
   int pbh = 0, pbw = 0;

   if (!qp) return;
   // FIXME: repeat of whats in _e_mod_quickpanel_animate_down() but for sliding
   // vertially up form an indicator @ screen bottom. fix to make it 1 fn
   pbh = qp->vert.size;

   if (qp->horiz_style)
     {
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             /* don't adjust borders that are being deleted */
             if (e_object_is_del(E_OBJECT(panel->bd))) continue;
             pbw -= panel->bd->w;
             e_border_fx_offset(panel->bd, (qp->horiz.adjust + pbw), qp->vert.adjust);

             if (!qp->visible)
               {
                  if (!panel->bd->visible) _e_mod_quickpanel_border_show(qp, panel->bd);
               }
             else
               {
                  if (panel->bd->visible) _e_mod_quickpanel_border_hide(panel->bd);
               }
          }
     }
   else
     {
        EINA_LIST_FOREACH(qp->borders, l, panel)
          {
             if (!panel) continue;

             /* don't adjust borders that are being deleted */
             if (e_object_is_del(E_OBJECT(panel->bd))) continue;
             pbh -= panel->bd->h;
             e_border_fx_offset(panel->bd, qp->horiz.adjust, (qp->vert.adjust + pbh));

             if (!qp->visible)
               {
                  if (!panel->bd->visible) _e_mod_quickpanel_border_show(qp, panel->bd);
               }
             else
               {
                  if (panel->bd->visible) _e_mod_quickpanel_border_hide(panel->bd);
               }
          }
     }
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
   Evas_Object *eo;

   qp->ly_base = edje_object_add(qp->evas);
   if (!qp->ly_base) return -1;

   // set background file
   edje_object_scale_set(qp->ly_base, qp->scale);
   edje_object_file_set(qp->ly_base, EDJ_FILE, "e/modules/illume2-slp/quickpanel/base");
   evas_object_resize(qp->ly_base, qp->zone->w, qp->zone->h);

   /* Add quickpanel handle bar object */
   eo = (Evas_Object*)edje_object_part_object_get(qp->ly_base, "quickpanel_handle_event");
   if (!eo) goto failed;
   evas_object_event_callback_add(eo, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_mod_quickpanel_cb_event_mouse_down, qp);
   evas_object_event_callback_add(eo, EVAS_CALLBACK_MOUSE_UP,
                                  _e_mod_quickpanel_cb_event_mouse_up, qp);
   evas_object_event_callback_add(eo, EVAS_CALLBACK_MOUSE_MOVE,
                                  _e_mod_quickpanel_cb_event_mouse_move, qp);

   e_popup_edje_bg_object_set(qp->popup, qp->ly_base);

   evas_object_show(qp->ly_base);
   return 0;
failed:
   _e_mod_quickpanel_bg_layout_del(qp);
   return -1;
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

static void _e_mod_quickpanel_cb_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   E_Illume_Quickpanel *qp = data;
   if (ev->button != 1) return;
   // do not allow mouse event while the quickpanel is showing/hiding
   if (qp->animator) return;

   qp->down = 1;
   qp->down_x = ev->canvas.x;
   qp->down_y = ev->canvas.y;
   qp->down_adjust = qp->horiz.adjust;
   qp->dragging = 0;

   qp->hide_trigger = 0;
}


static void _e_mod_quickpanel_cb_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   E_Illume_Quickpanel *qp = data;
   if (ev->button != 1) return;
   if (qp->down == 0) return;

   qp->down = 0;

   // do not allow mouse event while the quickpanel is showing/hiding
   if (qp->animator) return;

   int dy;
   if (qp->hide_trigger == 1)
     {
        dy = qp->down_y - ev->canvas.y;
        // check again delta y position.
        if(dy > qp->threshold_y)
          {
             // we need to hide quickpanel
             if (qp->visible) e_mod_quickpanel_hide(qp, 1);
          }

        qp->hide_trigger = 0;
        return;
     }
}

static void _e_mod_quickpanel_cb_event_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   E_Illume_Quickpanel *qp = data;

   // do not allow mouse event while the quickpanel is showing/hiding
   if (qp->animator) return;

   if (!qp->down) return;

   if (!qp->dragging)
     {
        int dx, dy;

        dx = ev->cur.canvas.x - qp->down_x;
        dy = ev->cur.canvas.y - qp->down_y;

        if (((dx * dx) > (qp->move_x_min * qp->move_x_min)) && (!qp->hide_trigger))
          {
             qp->dragging = 1;
          }
        else
          {
             if(!qp->hide_trigger)
               {
                  dy = qp->down_y - ev->cur.canvas.y;
                  if(dy > qp->threshold_y)
                    {
                       qp->hide_trigger = 1;
                    }
               }
             return;
          }
     }
}

static void _e_mod_quickpanel_border_show (E_Illume_Quickpanel* qp, E_Border* bd)
{
   if (!qp) return;

   /* make sure we have a border */
   if (!bd) return;

   e_border_show (bd);
   if (qp->ind)
     {
        e_border_stack_below (bd, qp->ind);
     }
}


static void _e_mod_quickpanel_border_hide (E_Border* bd)
{
   /* make sure we have a border */
   if (!bd) return;
   e_border_hide (bd, 2);
}


static Eina_Bool _e_mod_quickpanel_popup_new (E_Illume_Quickpanel* qp)
{
   if (!qp) return EINA_FALSE;

   if (qp->popup) return EINA_TRUE;

   qp->angle = _e_mod_quickpanel_root_angle_get(qp);
   _e_mod_quickpanel_check_lock_screen(qp);

   // FIXME: qp->zone->h height - should adapt to MAX of qp->zone->h or
   // max height needed to fit talles qp child
   qp->popup = e_popup_new (qp->zone,
                            qp->zone->x, qp->zone->y - qp->zone->h,
                            qp->zone->w, qp->zone->h);
   if (!qp->popup) return EINA_FALSE;

   e_popup_name_set(qp->popup, "quickpanel");
   e_popup_layer_set(qp->popup, 151);
   qp->ee = qp->popup->ecore_evas;
   qp->evas = qp->popup->evas;

   ecore_x_netwm_window_type_set(qp->popup->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   // disable effect
   int state = 0;
   ecore_x_window_prop_property_set (qp->popup->evas_win, effect_state_atom, ECORE_X_ATOM_CARDINAL, 32, &state, 1);

   _e_mod_quickpanel_bg_layout_add(qp);
   if (!_e_mod_quickpanel_popup_update (qp))
     {
        // send message to quickpanel borders
        _e_mod_quickpanel_send_message (qp, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
        _e_mod_quickpanel_popup_del (qp);
        return EINA_FALSE;
     }
   _e_mod_quickpanel_size_adjust(qp);

   if (!qp->key_hdl)
     {
        qp->key_hdl = ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _e_mod_quickpanel_cb_key_down, qp);
     }
   utilx_grab_key (ecore_x_display_get(), qp->popup->evas_win, KEY_END, TOP_POSITION_GRAB);
   utilx_grab_key (ecore_x_display_get(), qp->popup->evas_win, KEY_SELECT, TOP_POSITION_GRAB);

   return EINA_TRUE;
}

static void _e_mod_quickpanel_popup_del (E_Illume_Quickpanel* qp)
{
   if (!qp) return;
   if (!qp->popup) return;

   if (qp->key_hdl)
     {
        ecore_event_handler_del (qp->key_hdl);
        qp->key_hdl = NULL;
     }
   utilx_ungrab_key (ecore_x_display_get(), qp->popup->evas_win, KEY_END);
   utilx_ungrab_key (ecore_x_display_get(), qp->popup->evas_win, KEY_SELECT);

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
        // FIXME: POPUP_HANDLER_SIZE will be changed
        if (qp->ind) extra_h = qp->ind->h + POPUP_HANDLER_SIZE * qp->scale;
        else extra_h = POPUP_HANDLER_SIZE * qp->scale;

        if (!qp->is_lock)
          {
             e_popup_resize (qp->popup, w, h);
             evas_object_resize (qp->ly_base, w, h);
          }
        else
          {
             if (qp->angle == 0)
                e_popup_resize (qp->popup, w, h + extra_h);
             else
               {
                  tx = qp->zone->x;
                  ty = qp->zone->h - (h + extra_h);
                  e_popup_move_resize (qp->popup, tx, ty, w, h + extra_h);
               }

             evas_object_resize (qp->ly_base, w, h + extra_h);
          }
     }
   else
     {
        // FIXME: POPUP_HANDLER_SIZE will be changed
        if (qp->ind) extra_h = qp->ind->w + POPUP_HANDLER_SIZE * qp->scale;
        else extra_h = POPUP_HANDLER_SIZE * qp->scale;

        if (!qp->is_lock)
          {
             e_popup_resize (qp->popup, w, h);
             evas_object_resize (qp->ly_base, h, w);
          }
        else
          {
             if (qp->angle == 90)
                e_popup_resize (qp->popup, w + extra_h, h);
             else
               {
                  tx = qp->zone->w - (w + extra_h);
                  ty = qp->zone->y;
                  e_popup_move_resize (qp->popup, tx, ty, w + extra_h, h);
               }

             evas_object_resize( qp->ly_base, h, w + extra_h);
          }
     }
   ecore_evas_rotation_with_resize_set(qp->popup->ecore_evas, qp->angle);

   return EINA_TRUE;
}

static Eina_Bool _e_mod_quickpanel_cb_key_down (void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key*ev;
   E_Illume_Quickpanel *qp;

   ev = event;
   if (!(qp = data)) return ECORE_CALLBACK_PASS_ON;

   if ((strcmp (ev->keyname, KEY_END) == 0) ||
       (strcmp (ev->keyname, KEY_SELECT) == 0))
     {
        e_illume_quickpanel_hide (qp->zone, 1);
     }

   return ECORE_CALLBACK_PASS_ON;
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
_e_quickpanel_position_update(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;
   int ty = 0;

   if (!qp) return;

   if (qp->borders)
     {
        if (qp->visible)
          {
             int nx = 0;

             ty = qp->vert.isize;
             if (qp->vert.dir == 1) ty = 0;

             // FIXME: shouldnt have special case - just call anim with adjust @ end
             // also don't use e_border_fx_offset but actuall;y move bd
             /* if we are not animating, just show the borders */
             if (qp->horiz_style)
               {
                  EINA_LIST_FOREACH(qp->borders, l, panel)
                    {
                       if (!panel) continue;

                       if (qp->is_lock)
                         {
                            if (!panel->mini_controller) continue;
                         }

                       if (!panel->bd->visible) _e_mod_quickpanel_border_show(qp, panel->bd);
                       if (qp->vert.dir == 0)
                         {
                            e_border_fx_offset(panel->bd, qp->horiz.adjust + nx, ty);
                            nx += panel->bd->w;
                         }
                       else
                         {
                            nx -= panel->bd->w;
                            e_border_fx_offset(panel->bd, qp->horiz.adjust + nx, ty);
                         }
                    }
               }
             else
               {
                  EINA_LIST_FOREACH(qp->borders, l, panel)
                    {
                       if (!panel) continue;

                       if (qp->is_lock)
                         {
                            if (!panel->mini_controller) continue;
                         }

                       if (!panel->bd->visible) _e_mod_quickpanel_border_show(qp, panel->bd);
                       if (qp->vert.dir == 0)
                         {
                            if (qp->angle == 0)
                              {
                                 e_border_move (panel->bd, qp->horiz.adjust, ty);
                                 ty += panel->bd->h;
                              }
                            else if (qp->angle == 180)
                              {
                                 ty += panel->bd->h;
                                 e_border_move (panel->bd, qp->horiz.adjust, qp->zone->h - ty);
                              }
                            else if (qp->angle == 90)
                              {
                                 e_border_move (panel->bd, ty, qp->horiz.adjust);
                                 ty += panel->bd->w;
                              }
                            else if (qp->angle == 270)
                              {
                                 ty += panel->bd->w;
                                 e_border_move (panel->bd, qp->zone->w - ty, qp->horiz.adjust);
                              }
                         }
                       else
                         {
                            ty -= panel->bd->h;
                            e_border_fx_offset(panel->bd, qp->horiz.adjust, ty);
                         }
                    }
               }

             _e_mod_quickpanel_popup_update (qp);
             qp->vert.adjust = ty;
          }
        else
          {
             qp->vert.adjust = 0;
          }
     }

   if (qp->animator)
     {
        if (qp->ani_type == E_ILLUME_QUICKPANEL_ANI_SHOW)
          {
             qp->vert.adjust_end = qp->vert.size;
          }
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

static void
_e_mod_quickpanel_extra_panel_show(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Border *mini_bd;
   E_Illume_Quickpanel_Info *panel;
   int x,y,w,h;

   if (!qp) return;

   mini_bd = NULL;
   EINA_LIST_FOREACH(qp->borders, l, panel)
     {
        if (!panel) continue;

        if (panel->mini_controller == EINA_TRUE)
          {
             mini_bd = panel->bd;
             break;
          }
     }

   if (!mini_bd) return;

   x = mini_bd->x;
   y = mini_bd->y;
   w = mini_bd->w;
   h = mini_bd->h;

   if (qp->hidden_mini_controllers)
     {
        EINA_LIST_FOREACH(qp->hidden_mini_controllers, l, panel)
          {
             if (!panel) continue;
             if (!panel->bd) continue;

             e_border_move_resize(panel->bd, x, y, w, h);
             e_border_show(panel->bd);
             e_border_stack_below(panel->bd, mini_bd);
          }
     }

}

static void
_e_mod_quickpanel_extra_panel_hide(E_Illume_Quickpanel *qp)
{
   Eina_List *l;
   E_Illume_Quickpanel_Info *panel;

   if (!qp) return;

   if (qp->hidden_mini_controllers)
     {
        EINA_LIST_FOREACH(qp->hidden_mini_controllers, l, panel)
          {
             if (!panel) continue;
             if (!panel->bd) continue;

             e_border_hide(panel->bd, 2);
          }
     }

}

