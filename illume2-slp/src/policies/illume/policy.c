#include "e_illume_private.h"
#include "policy_util.h"
#include "policy.h"
#include "dlog.h"

#if 1 // for visibility
#include <X11/Xlib.h>
#endif

/* NB: DIALOG_USES_PIXEL_BORDER is an experiment in setting dialog windows
 * to use the 'pixel' type border. This is done because some dialogs,
 * when shown, blend into other windows too much. Pixel border adds a
 * little distinction between the dialog window and an app window.
 * Disable if this is not wanted */
//#define DIALOG_USES_PIXEL_BORDER 1

/* for screen lock */
#define SCREEN_LOCK_TIMEOUT 2.5

/* for debugging */
#define ILLUME2_DEBUG  0
#if ILLUME2_DEBUG
#define ILLUME2_TRACE  printf
#else
#define ILLUME2_TRACE(...)
#endif

typedef struct _E_Illume_Print_Info
{
   unsigned int type;
   char file_name[256];
} E_Illume_Print_Info;


#define COMP_MODULE_CONTROL


/*****************************/
/* local function prototypes */
/*****************************/
static void _policy_border_set_focus(E_Border *bd);
static void _policy_border_move(E_Border *bd, int x, int y);
static void _policy_border_resize(E_Border *bd, int w, int h);
static void _policy_border_show_below(E_Border *bd);
static void _policy_zone_layout_update(E_Zone *zone);
static void _policy_zone_layout_indicator(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_quickpanel(E_Border *bd);
static void _policy_zone_layout_keyboard(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_fullscreen(E_Border *bd);
static void _policy_zone_layout_dialog(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_sliding_win(E_Border *bd, E_Illume_Config_Zone *cz);

static int _policy_window_rotation_angle_get(Ecore_X_Window win);
static Ecore_X_Window _policy_active_window_get(Ecore_X_Window root);
static int _policy_border_indicator_state_get(E_Border *bd);

static void _policy_layout_quickpanel_rotate (E_Illume_Quickpanel* qp, int angle);

static E_Illume_Border_Info* _policy_get_border_info (E_Border* bd);
static E_Illume_Border_Info* _policy_add_border_info_list (E_Border* bd);
static void _policy_delete_border_info_list (E_Border* bd);
static int _policy_compare_cb_border (E_Illume_Border_Info* data1, E_Illume_Border_Info* data2);

static void _policy_zone_layout_app_single_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone* cz);
static void _policy_zone_layout_app_dual_top_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone* cz);
static void _policy_zone_layout_app_dual_left_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone* cz);
static void _policy_zone_layout_app_dual_custom_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone* cz);

static int _policy_border_get_notification_level (Ecore_X_Window win);
static void _policy_border_update_notification_stack (E_Border* bd, int level, Eina_Bool check_focus);

/* for screen lock */
static void _policy_request_screen_lock (E_Manager* man);
static void _policy_request_screen_unlock (E_Manager* man);
static void  _policy_border_add_block_list (E_Border* bd);
static void  _policy_border_remove_block_list (E_Border* bd);

static void _policy_change_quickpanel_layer (E_Illume_Quickpanel* qp, E_Border* indi_bd, int layer, int level);
static void _policy_change_indicator_layer (E_Border* indi_bd, int layer, int level);

/* for property change */
static void _policy_property_window_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_keyboard_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_virtual_keyboard_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_sliding_win_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_sliding_win_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_enlightenment_scale_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_rotate_win_angle_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_active_win_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_win_type_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_rotate_root_angle_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_notification_level_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_overlay_win_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_window_opaque_change (Ecore_X_Event_Window_Property *event);

static int _policy_property_window_opaque_get (Ecore_X_Window win);


/* for debugging */
void _policy_border_list_print (Ecore_X_Window win);

#if ILLUME_LOGGER_BUILD_ENABLE
static Eina_Bool _policy_property_illume_log_change (Ecore_X_Event_Window_Property *ev);
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

#ifdef COMP_MODULE_CONTROL
static void _policy_property_composite_module_change (Ecore_X_Event_Window_Property *ev);
#endif

#if 1 // for visibility
static void _policy_manage_xwins (E_Manager* man);
static E_Illume_XWin_Info* _policy_xwin_info_find (Ecore_X_Window win);
static Eina_Bool _policy_xwin_info_add (Ecore_X_Window win);
static Eina_Bool _policy_xwin_info_delete (Ecore_X_Window win);

static void _policy_send_visibility_notify (Ecore_X_Window win, int visibility);
static void _policy_calculate_visibility (Ecore_X_Window win);
#endif // visibility

static E_Illume_XWin_Info* _policy_find_next_visible_window (E_Illume_XWin_Info* xwin_info);
static void _policy_root_angle_set (Ecore_X_Window root, Ecore_X_Window win);
static void _policy_change_root_angle_by_border_angle (E_Border* bd);
static void _policy_indicator_angle_change (E_Border* indi_bd, int angle);


/*******************/
/* local variables */
/*******************/

/* for active/deactive message */
static Ecore_X_Window g_active_win = 0;
static Ecore_X_Window g_active_pid = 0;

/* for rotation */
int g_root_angle = 0;
Ecore_X_Window g_rotated_win = 0;

/* for focus stack */
static Eina_List *_pol_focus_stack;

/* for border information */
static Eina_List* e_border_info_list = NULL;

/* for screen lock */
static E_Illume_Screen_Lock_Info* g_screen_lock_info;
static Ecore_X_Atom E_ILLUME_ATOM_SCREEN_LOCK;

/* for notification level */
static Ecore_X_Atom E_ILLUME_ATOM_NOTIFICATION_LEVEL;

/* for active/deactive message */
static Ecore_X_Atom E_ILLUME_ATOM_ACTIVATE_WINDOW;
static Ecore_X_Atom E_ILLUME_ATOM_DEACTIVATE_WINDOW;

/* for visibility */
static Ecore_X_Atom E_ILLUME_ATOM_OVERAY_WINDOW;
static Ecore_X_Atom E_ILLUME_ATOM_WINDOW_OPAQUE;

/* for debugging */
static Ecore_X_Atom E_ILLUME_ATOM_STACK_DISPLAY;
static Ecore_X_Atom E_ILLUME_ATOM_STACK_DISPLAY_DONE;

/* for logger */
#if ILLUME_LOGGER_BUILD_ENABLE
static int _illume_logger_type = LT_NOTHING;
static Ecore_X_Atom E_ILLUME_ATOM_ILLUME_LOG;
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

 #ifdef COMP_MODULE_CONTROL
static Ecore_X_Atom E_ILLUME_ATOM_COMP_MODULE_ENABLED;
#endif

#if 1 // for visibility
static Eina_Hash* _e_illume_xwin_info_hash = NULL;
static Eina_Inlist* _e_illume_xwin_info_list = NULL;
static Ecore_X_Window _e_overlay_win = 0;
static int _g_root_width;
static int _g_root_height;
#endif

/* local functions */
static void
_policy_border_set_focus(E_Border *bd)
{
   if (!bd) return;

   /* if focus is locked out then get out */
   if (bd->lock_focus_out) return;

   /* make sure the border can accept or take focus */
   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
     {
        /* check E's focus settings */
        if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
            ((bd->parent) &&
             ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
              ((bd->parent->focused) &&
               (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))))
          {
             /* if the border was hidden due to layout, we need to unhide */
             if (!bd->visible) e_illume_border_show(bd);

             /* if the border is iconified then uniconify */
             if (bd->iconic)
               {
                  /* if the user is allowed to uniconify, then do it */
                  if (!bd->lock_user_iconify) e_border_uniconify(bd);
               }

             /* if we can raise the border do it */
             if (!bd->lock_user_stacking) e_border_raise(bd);

             /* focus the border */
             e_border_focus_set(bd, 1, 1);

             /* NB: since we skip needless border evals when container layout
              * is called (to save cpu cycles), we need to
              * signal this border that it's focused so that the edj gets
              * updated.
              *
              * This is potentially useless as THIS policy
              * makes all windows borderless anyway, but it's in here for
              * completeness
              e_border_focus_latest_set(bd);
              if (bd->bg_object)
              edje_object_signal_emit(bd->bg_object, "e,state,focused", "e");
              if (bd->icon_object)
              edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");
              e_focus_event_focus_in(bd);
              */
          }
     }
}

static void
_policy_border_move(E_Border *bd, int x, int y)
{
   if (!bd) return;

   /* NB: Qt uses a weird window type called 'VCLSalFrame' that needs to
    * have bd->placed set else it doesn't position correctly...
    * this could be a result of E honoring the icccm request position,
    * not sure */

   e_border_move (bd, x, y);
}

static void
_policy_border_resize(E_Border *bd, int w, int h)
{
   if (!bd) return;

   if (bd->visible)
      _policy_border_add_block_list(bd);

   e_border_resize (bd, w, h);
}

static void
_policy_border_show_below(E_Border *bd)
{
   Eina_List *l;
   E_Border *prev;
   int pos = 0, i;

   //   printf("Show Borders Below: %s %d %d\n",
   //          bd->client.icccm.class, bd->x, bd->y);

   if (!bd) return;

   if (bd->client.icccm.transient_for)
     {
        if ((prev = e_border_find_by_client_window(bd->client.icccm.transient_for)))
          {
             _policy_border_set_focus(prev);
             return;
          }
     }

   /* determine layering position */
   if (bd->layer <= 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   /* Find the windows below this one */
   for (i = pos; i >= 2; i--)
     {
        E_Border *b;

        EINA_LIST_REVERSE_FOREACH(bd->zone->container->layers[i].clients, l, b)
          {
             if (!b) continue;

             /* skip if it's the same border */
             if (b == bd) continue;

             /* skip if it's not on this zone */
             if (b->zone != bd->zone) continue;

             /* skip special borders */
             if (e_illume_border_is_indicator(b)) continue;
             if (e_illume_border_is_keyboard(b)) continue;
             if (e_illume_border_is_quickpanel(b)) continue;
             if (e_illume_border_is_sliding_win(b)) continue;

             if ((bd->fullscreen) || (bd->need_fullscreen))
               {
                  _policy_border_set_focus(b);
                  return;
               }
             else
               {
                  /* need to check x/y position */
                  if (E_CONTAINS(bd->x, bd->y, bd->w, bd->h,
                                 b->x, b->y, b->w, b->h))
                    {
                       _policy_border_set_focus(b);
                       return;
                    }
               }
          }
     }

   /* if we reach here, then there is a problem with showing a window below
    * this one, so show previous window in stack */
   EINA_LIST_REVERSE_FOREACH(_pol_focus_stack, l, prev)
     {
        if (!prev) continue;
        if (prev->zone != bd->zone) continue;
        _policy_border_set_focus(prev);
        return;
     }
}

static void
_policy_zone_layout_update(E_Zone *zone)
{
   Eina_List *l;
   E_Border *bd;

   if (!zone) return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;

        /* skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* skip special windows */
        if (e_illume_border_is_keyboard(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;

        /* signal a changed pos here so layout gets updated */
        bd->changes.pos = 1;
        bd->changed = 1;
     }
}

static void
_policy_zone_layout_indicator(E_Border *bd, E_Illume_Config_Zone *cz)
{
   ILLUME2_TRACE ("[ILLUME2] %s (%d) win = %x\n", __func__, __LINE__, bd->client.win);

   if ((!bd) || (!cz)) return;

   /* grab minimum indicator size */
   //e_illume_border_min_get(bd, NULL, &cz->indicator.size);

   /* no point in doing anything here if indicator is hidden */
   if ((!bd->new_client) && (!bd->visible)) return;

   /* if we are dragging, then skip it for now */
   if (bd->client.illume.drag.drag)
     {
        /* when dragging indicator, we need to trigger a layout update */
        _policy_zone_layout_update(bd->zone);
        return;
     }

   /* lock indicator window from dragging if we need to */
   if ((cz->mode.dual == 1) && (cz->mode.side == 0))
      ecore_x_e_illume_drag_locked_set(bd->client.win, 0);
   else
      ecore_x_e_illume_drag_locked_set(bd->client.win, 1);

   /* make sure it's the required width & height */
   int rotation = _policy_window_rotation_angle_get(bd->client.win);
   if(rotation == -1) return;

   ILLUME2_TRACE ("ILLUME2] INDICATOR'S ANGLE = %d\n", rotation);

   // check indicator's rotation info and then set it's geometry
   if (rotation == 0 || rotation == 180)
     {
        ecore_x_e_illume_indicator_geometry_set(bd->zone->black_win, bd->x, bd->y, bd->w, bd->h);
        ecore_x_e_illume_indicator_geometry_set(bd->zone->container->manager->root, bd->x, bd->y, bd->w, bd->h);
     }
   else
     {
        ecore_x_e_illume_indicator_geometry_set(bd->zone->black_win, bd->zone->x, bd->zone->y, bd->h, bd->w);
        ecore_x_e_illume_indicator_geometry_set(bd->zone->container->manager->root, bd->zone->x, bd->zone->y, bd->h, bd->w);
     }
}

static void
_policy_zone_layout_quickpanel(E_Border *bd)
{
   if (!bd) return;

/*
   int rotation;
   rotation = _policy_window_rotation_angle_get (bd->client.win);
   ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] _policy_zone_layout_quickpanel..(%d) rotation angle = %d\n", __LINE__, rotation);

   if (rotation == 0 || rotation == 180)
     {
        if ((bd->w != bd->zone->w))
          {
             ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] CHANGE QUICK-PANEL SIZE to Portrait..(%d) old (%d, %d)  new (%d, %d)\n", __LINE__, bd->w, bd->h, bd->zone->w, bd->h);
             _policy_border_resize(bd, bd->zone->w, bd->h);
          }
     }
   else
     {
        if ((bd->h != bd->zone->h))
          {
             ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] CHANGE QUICK-PANEL SIZE to Landscape..(%d) old (%d, %d)  new (%d, %d)\n", __LINE__, bd->w, bd->h, bd->w, bd->zone->h);
             _policy_border_resize(bd, bd->w, bd->zone->h);
          }
     }
*/
}

static void
_policy_zone_layout_keyboard(E_Border *bd, E_Illume_Config_Zone *cz)
{
   if ((!bd) || (!cz)) return;

   /* no point in adjusting size or position if it's not visible */
   if (!bd->visible) return;

   /* set layer if needed */
   if (bd->client.icccm.transient_for == 0)
     {
        if (bd->layer != POL_KEYBOARD_LAYER)
           e_border_layer_set(bd, POL_KEYBOARD_LAYER);
     }
}


static void
_policy_zone_layout_fullscreen(E_Border *bd)
{
   //   printf("\tLayout Fullscreen: %s\n", bd->client.icccm.name);

   if (!bd) return;

   /* make sure it's the required width & height */
   if ((bd->w != bd->zone->w) || (bd->h != bd->zone->h))
      _policy_border_resize(bd, bd->zone->w, bd->zone->h);

   /* set layer if needed */
   if (bd->layer != POL_FULLSCREEN_LAYER)
      e_border_layer_set(bd, POL_FULLSCREEN_LAYER);
}

static void
_policy_zone_layout_dialog(E_Border *bd, E_Illume_Config_Zone *cz)
{
   E_Border *parent;
   int mw, mh, nx, ny;

   //   printf("\tLayout Dialog: %s\n", bd->client.icccm.name);

   /* NB: This policy ignores any ICCCM requested positions and centers the
    * dialog on it's parent (if it exists) or on the zone */

   if ((!bd) || (!cz)) return;

   /* no point in adjusting size or position if it's not visible */
   if (!bd->visible) return;

   /* grab minimum size */
   e_illume_border_min_get(bd, &mw, &mh);

   /* make sure it fits in this zone */
   if (mw > bd->zone->w) mw = bd->zone->w;
   if (mh > bd->zone->h) mh = bd->zone->h;

   if (mw < bd->w) mw = bd->w;
   if (mh < bd->h) mh = bd->h;

   /* try to get this dialog's parent if it exists */
   parent = e_illume_border_parent_get(bd);

   /* if we have no parent, or we are in dual mode, then center on zone */
   /* NB: we check dual mode because if we are in dual mode, dialogs tend to
    * be too small to be useful when positioned on the parent, so center
    * on zone. We could check their size first here tho */
   if ((!parent) || (cz->mode.dual == 1))
     {
        /* no parent or dual mode, center on screen */
        nx = (bd->zone->x + ((bd->zone->w - mw) / 2));
        ny = (bd->zone->y + ((bd->zone->h - mh) / 2));
     }
   else
     {
        /* NB: there is an assumption here that the parent has already been
         * layed out on screen. This could be bad. Needs Testing */

        /* make sure we are not larger than the parent window */
        if (mw > parent->w) mw = parent->w;
        if (mh > parent->h) mh = parent->h;

        /* center on parent */
        nx = (parent->x + ((parent->w - mw) / 2));
        ny = (parent->y + ((parent->h - mh) / 2));
     }

   /* make sure it's the required width & height */
   if ((bd->w != mw) || (bd->h != mh))
     {
        _policy_border_resize(bd, mw, mh);
     }

   /* make sure it's in the correct position */
   if ((bd->x != nx) || (bd->y != ny))
      _policy_border_move(bd, nx, ny);

   /* set layer if needed */
   if (bd->layer != POL_DIALOG_LAYER) e_border_layer_set(bd, POL_DIALOG_LAYER);
}

static void
_policy_zone_layout_sliding_win(E_Border *bd, E_Illume_Config_Zone *cz)
{
   /* no point in adjusting size or position if it's not visible */
   if (!bd->visible) return;

   /* set layer if needed */
   if (bd->layer != POL_SLIDING_WIN_LAYER)
      e_border_layer_set(bd, POL_SLIDING_WIN_LAYER);
}

/* policy functions */
void
_policy_border_add(E_Border *bd)
{
   //   printf("Border added: %s\n", bd->client.icccm.class);

   if (!bd) return;

   /* NB: this call sets an atom on the window that specifices the zone.
    * the logic here is that any new windows created can access the zone
    * window by a 'get' call. This is useful for elementary apps as they
    * normally would not have access to the zone window. Typical use case
    * is for indicator & softkey windows so that they can send messages
    * that apply to their respective zone only. Example: softkey sends close
    * messages (or back messages to cycle focus) that should pertain just
    * to it's current zone */
   ecore_x_e_illume_zone_set(bd->client.win, bd->zone->black_win);

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... bd = %x\n", __func__, __LINE__, bd->client.win);
   // 1. create border_info
   E_Illume_Border_Info* bd_info = NULL;
   bd_info = _policy_get_border_info(bd);
   if (bd_info == NULL)
      bd_info = _policy_add_border_info_list (bd);

   if (bd_info)
     {
        bd_info->opaque = _policy_property_window_opaque_get (bd->client.win);
     }

   /* ignore stolen borders. These are typically quickpanel or keyboards */
   if (bd->stolen)
     {
        if (bd->client.illume.quickpanel.quickpanel)
          {
             E_Border* indi_bd;
             E_Illume_Border_Info* indi_bd_info = NULL;

             /* try to get the Indicator on this zone */
             if ((indi_bd = e_illume_border_indicator_get (bd->zone)))
               {
                  if ((indi_bd_info = _policy_get_border_info (indi_bd)))
                    {
                       if (bd_info) bd_info->level = indi_bd_info->level;
                    }
               }
          }
        return;
     }

   /* Add this border to our focus stack if it can accept or take focus */
   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
      _pol_focus_stack = eina_list_append(_pol_focus_stack, bd);

   if (e_illume_border_is_indicator(bd))
      _policy_zone_layout_update(bd->zone);
   else
     {
        /* set focus on new border if we can */
        _policy_border_set_focus(bd);
     }

   if (e_illume_border_is_indicator (bd))
     {
        E_Illume_Config_Zone *cz;
        cz = e_illume_zone_config_get(bd->zone->id);
        if (cz)
          {
             ILLUME2_TRACE ("[ILLUME2] ADD INDICATOR WINDOW... win = %x, Save indicator's size = %d!!!\n", bd->client.win, bd->h);
             cz->indicator.size = bd->h;
          }

        int is_show = 0;
        if (g_active_win)
          {
             E_Border* active_bd = NULL;
             active_bd = e_border_find_by_client_window (g_active_win);
             if (active_bd)
               {
                  is_show = _policy_border_indicator_state_get (active_bd);
                  if (e_illume_border_is_notification (active_bd))
                    {
                       int level;
                       E_Illume_Border_Info* active_bd_info;
                       active_bd_info = _policy_get_border_info (active_bd);
                       if (active_bd_info)
                         {
                            level = active_bd_info->level;
                         }
                       else
                         {
                            level = 250;
                         }
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Update indicator's layer to NOTIFICATON.. level = %d\n", __func__, __LINE__, level);
                       _policy_change_indicator_layer (bd, POL_NOTIFICATION_LAYER, level);
                    }
               }
          }

        if (is_show != 1)
          {
             e_border_hide (bd, 2);
          }
     }
}

void
_policy_border_del(E_Border *bd)
{
   //   printf("Border deleted: %s\n", bd->client.icccm.class);
   E_Illume_XWin_Info* xwin_info;
   Eina_Bool is_rotated_win = EINA_FALSE;

   if (!bd) return;

   /* if this is a fullscreen window, than we need to show indicator window */
   /* NB: we could use the e_illume_border_is_fullscreen function here
    * but we save ourselves a function call this way */
   if ((bd->fullscreen) || (bd->need_fullscreen))
     {
        E_Border *indi_bd;

        /* try to get the Indicator on this zone */
        if ((indi_bd = e_illume_border_indicator_get(bd->zone)))
          {
             /* we have the indicator, show it if needed */
             if (!indi_bd->visible) e_illume_border_show(indi_bd);
          }
     }

   if (e_illume_border_is_sliding_win(bd))
     {
        ecore_x_e_illume_sliding_win_state_set (bd->zone->black_win, 0);
     }

   /* remove from our focus stack */
   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
      _pol_focus_stack = eina_list_remove(_pol_focus_stack, bd);

   if (g_rotated_win == bd->client.win)
     {
        is_rotated_win = EINA_TRUE;
        g_rotated_win = 0;
     }

   xwin_info = _policy_xwin_info_find (bd->win);
   if (xwin_info)
     {
        if (is_rotated_win)
          {
             if (xwin_info->argb && !xwin_info->bd_info->opaque)
               {
                  E_Illume_XWin_Info* next_xwin_info = NULL;
                  next_xwin_info = _policy_find_next_visible_window (xwin_info);
                  L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. next_win:%x\n", __func__, __LINE__, next_xwin_info ? (unsigned int)next_xwin_info->bd_info->border->client.win : (unsigned int)NULL);
                  if (next_xwin_info)
                    {
                       L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. CALL _policy_change_root_angle_by_border_angle!!! win:%x\n", __func__, __LINE__, next_xwin_info->bd_info->border->client.win);
                       _policy_change_root_angle_by_border_angle (next_xwin_info->bd_info->border);
                    }
                  else
                    {
                       L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... CALL _policy_root_angle_set.. win:%x\n", __func__, __LINE__, (unsigned int)NULL);
                       _policy_root_angle_set (bd->zone->container->manager->root, 0);
                    }

               }
          }

        xwin_info->bd_info = NULL;
     }

   _policy_delete_border_info_list (bd);

   if (g_screen_lock_info->is_lock)
      _policy_border_remove_block_list (bd);

   if (e_illume_border_is_indicator(bd))
     {
        E_Illume_Config_Zone *cz;

        /* get the config for this zone */
        cz = e_illume_zone_config_get(bd->zone->id);
        if (cz) cz->indicator.size = 0;
        _policy_zone_layout_update(bd->zone);
     }
   else
     {
        /* show the border below this one */
        _policy_border_show_below(bd);
     }

   if (e_illume_border_is_lock_screen(bd))
     {
        if (_e_illume_cfg->use_mem_trim)
          {
             /* heap and stack trim */
             e_illume_util_mem_trim();
          }
     }
}

void
_policy_border_focus_in(E_Border *bd __UNUSED__)
{
   //   printf("Border focus in: %s\n", bd->client.icccm.name);
}

void
_policy_border_focus_out(E_Border *bd)
{
   //   printf("Border focus out: %s\n", bd->client.icccm.name);

   if (!bd) return;

   /* NB: if we got this focus_out event on a deleted border, we check if
    * it is a transient (child) of another window. If it is, then we
    * transfer focus back to the parent window */
   if (e_object_is_del(E_OBJECT(bd)))
     {
        if (e_illume_border_is_dialog(bd))
          {
             E_Border *parent;

             if ((parent = e_illume_border_parent_get(bd)))
                _policy_border_set_focus(parent);
          }
     }
}

void
_policy_border_activate(E_Border *bd)
{
   //   printf("Border Activate: %s\n", bd->client.icccm.name);
   int level;
   E_Illume_Border_Info* bd_info = NULL;

   if (!bd) return;

   /* NB: stolen borders may or may not need focus call...have to test */
   if (bd->stolen) return;

   /* NB: We cannot use our set_focus function here because it does,
    * occasionally fall through wrt E's focus policy, so cherry pick the good
    * parts and use here :) */

   /* if the border is iconified then uniconify if allowed */
   if ((bd->iconic) && (!bd->lock_user_iconify))
      e_border_uniconify(bd);

   ILLUME2_TRACE ("[ILLUME2] _policy_border_activate.. (%d) ACTIVE WIN = %x\n", __LINE__, bd->client.win);

   if (e_illume_border_is_notification (bd))
     {
        // create border_info
        bd_info = _policy_get_border_info(bd);
        if (bd_info == NULL) return;

        level = _policy_border_get_notification_level (bd->client.win);
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, level);
        _policy_border_update_notification_stack (bd, level, EINA_TRUE);
     }

   /* NB: since we skip needless border evals when container layout
    * is called (to save cpu cycles), we need to
    * signal this border that it's focused so that the edj gets
    * updated.
    *
    * This is potentially useless as THIS policy
    * makes all windows borderless anyway, but it's in here for
    * completeness
    e_border_focus_latest_set(bd);
    if (bd->bg_object)
    edje_object_signal_emit(bd->bg_object, "e,state,focused", "e");
    if (bd->icon_object)
    edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");
    e_focus_event_focus_in(bd);
    */
}

void
_policy_border_post_fetch(E_Border *bd)
{
   //   printf("Border post fetch\n");

   if (!bd) return;

   /* NB: for this policy we disable all remembers set on a border */
   if (bd->remember) e_remember_del(bd->remember);
   bd->remember = NULL;

   /* set this border to borderless */
#ifdef DIALOG_USES_PIXEL_BORDER
   if ((e_illume_border_is_dialog(bd)) && (e_illume_border_parent_get(bd)))
      eina_stringshare_replace(&bd->bordername, "pixel");
   else
      bd->borderless = 1;
#else
   bd->borderless = 1;
#endif

   /* tell E the border has changed */
   bd->client.border.changed = 1;
}


void
_policy_border_post_new_border(E_Border *bd)
{
   int level;
   E_Illume_Border_Info* bd_info = NULL;

   if (bd->new_client)
     {
        if (e_illume_border_is_notification (bd))
          {
             // create border_info
             bd_info = _policy_get_border_info(bd);
             if (bd_info == NULL)
                bd_info = _policy_add_border_info_list (bd);

             level = _policy_border_get_notification_level (bd->client.win);
             L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, level);
             _policy_border_update_notification_stack (bd, level, EINA_TRUE);
          }
     }
}


void
_policy_border_pre_fetch(E_Border *bd)
{
   if (!bd) return;

   /* Below code are same to _e_border_eval0 in e_border.c.
      But we added code to handle notification window */
   if (bd->client.icccm.fetch.transient_for)
     {
        /* TODO: What do to if the transient for isn't mapped yet? */
        E_Border *bd_parent = NULL;

        bd->client.icccm.transient_for = ecore_x_icccm_transient_for_get(bd->client.win);
        if (bd->client.icccm.transient_for)
          {
             bd_parent = e_border_find_by_client_window(bd->client.icccm.transient_for);
          }

        /* If we already have a parent, remove it */
        if (bd->parent)
          {
             bd->parent->transients = eina_list_remove(bd->parent->transients, bd);
             if (bd->parent->modal == bd) bd->parent->modal = NULL;
             bd->parent = NULL;
          }

        if (bd_parent)
          {
             if (bd_parent != bd)
               {
                  bd_parent->transients = eina_list_append(bd_parent->transients, bd);
                  bd->parent = bd_parent;
                  e_border_layer_set(bd, bd->parent->layer);

                  if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
                     bd->parent->modal = bd;

                  if (e_config->focus_setting == E_FOCUS_NEW_DIALOG ||
                      (bd->parent->focused && (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))
                     bd->take_focus = 1;

                  /* handles notification window stack */
                  if (e_illume_border_is_notification (bd_parent) || bd_parent->layer == POL_NOTIFICATION_LAYER)
                    {
                       int level;
                       E_Illume_Border_Info* bd_info;
                       E_Illume_Border_Info* parent_bd_info;

                       bd_info = _policy_get_border_info(bd);
                       if (bd_info == NULL)
                          bd_info = _policy_add_border_info_list (bd);

                       parent_bd_info = _policy_get_border_info (bd_parent);
                       if (parent_bd_info)
                         {
                            level = parent_bd_info->level;
                         }
                       else
                         {
                            level = _policy_border_get_notification_level (bd_parent->client.win);
                         }
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, level);
                       _policy_border_update_notification_stack (bd, level, EINA_TRUE);
                    }
               }
             else
               {
                  if (e_illume_border_is_notification (bd_parent) || bd_parent->layer == POL_NOTIFICATION_LAYER)
                    {
                       int level;
                       E_Illume_Border_Info* bd_info;
                       E_Illume_Border_Info* parent_bd_info;

                       bd_info = _policy_get_border_info(bd);
                       if (bd_info == NULL)
                          bd_info = _policy_add_border_info_list (bd);

                       parent_bd_info = _policy_get_border_info (bd_parent);
                       if (parent_bd_info)
                         {
                            level = parent_bd_info->level;
                         }
                       else
                         {
                            level = _policy_border_get_notification_level (bd_parent->client.win);
                         }
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, level);
                       _policy_border_update_notification_stack (bd, level, EINA_TRUE);
                    }
               }
          }
        bd->client.icccm.fetch.transient_for = 0;
     }
}

void
_policy_border_post_assign(E_Border *bd)
{
   //   printf("Border post assign\n");

   if (!bd) return;

   bd->internal_no_remember = 1;

   /* do not allow client to change these properties */
   bd->lock_client_shade = 1;
   bd->lock_client_maximize = 1;

   /* do not allow the user to change these properties */
   bd->lock_user_location = 1;
   bd->lock_user_size = 1;
   bd->lock_user_shade = 1;

   /* clear any centered states */
   /* NB: this is mainly needed for E's main config dialog */
   bd->client.e.state.centered = 0;

   /* lock the border type so user/client cannot change */
   bd->lock_border = 1;

   if (e_illume_border_is_utility (bd))
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... win(%x) is UTILITY type.. SET REQUEST_POS!!!\n", __func__, __LINE__, bd->client.win);
        bd->client.icccm.request_pos = 1;
     }

}

void
_policy_border_show(E_Border *bd)
{
   if (!bd) return;

   /* make sure we have a name so that we don't handle windows like E's root */
   if (!bd->client.icccm.name) return;

   //   printf("Border Show: %s\n", bd->client.icccm.class);

   /* trap for special windows so we can ignore hides below them */
   if (e_illume_border_is_indicator(bd)) return;
   if (e_illume_border_is_quickpanel(bd)) return;
   if (e_illume_border_is_keyboard(bd)) return;
   if (e_illume_border_is_sliding_win(bd))
     {
        ecore_x_e_illume_sliding_win_state_set (bd->zone->black_win, 1);
        ecore_x_e_illume_sliding_win_geometry_set (bd->zone->black_win, bd->x, bd->y, bd->w, bd->h);
        return;
     }
}

void
_policy_zone_layout(E_Zone *zone)
{
   E_Illume_Config_Zone *cz;
   Eina_List *l;
   E_Border *bd;

   //   printf("Zone Layout: %d\n", zone->id);

   if (!zone) return;

   /* get the config for this zone */
   cz = e_illume_zone_config_get(zone->id);
   if (!cz) return;

   /* loop through border list and update layout */
   E_Illume_Border_Info* bd_info;
   EINA_LIST_FOREACH(e_border_info_list, l, bd_info)
     {
        if (!bd_info) continue;

        bd = bd_info->border;

        /* skip borders that are being deleted */
        if (e_object_is_del(E_OBJECT(bd))) continue;

        /* skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* only update layout for this border if it really needs it */
        if ((!bd->new_client) && (!bd->changes.pos) && (!bd->changes.size) &&
            (!bd->changes.visible) && (!bd->pending_move_resize) &&
            (!bd->need_shape_export) && (!bd->need_shape_merge)) continue;

        /* are we laying out an indicator ? */
        if (e_illume_border_is_indicator(bd))
           _policy_zone_layout_indicator(bd, cz);

        /* are we layout out a quickpanel ? */
        else if (e_illume_border_is_quickpanel(bd))
           _policy_zone_layout_quickpanel(bd);

        /* are we laying out a keyboard ? */
        else if (e_illume_border_is_keyboard(bd))
           _policy_zone_layout_keyboard(bd, cz);

        /* are we laying out a fullscreen window ? */
        /* NB: we could use the e_illume_border_is_fullscreen function here
         * but we save ourselves a function call this way. */
        else if ((bd->fullscreen) || (bd->need_fullscreen))
           _policy_zone_layout_fullscreen(bd);

        /* are we laying out a dialog ? */
        else if (e_illume_border_is_dialog(bd))
           _policy_zone_layout_dialog(bd, cz);

        else if (e_illume_border_is_sliding_win(bd))
           _policy_zone_layout_sliding_win(bd, cz);

        /* must be an app */
        else
          {
             /* are we in single mode ? */
             if (!cz->mode.dual)
                _policy_zone_layout_app_single_new (bd_info, cz);
             else
               {
                  /* we are in dual-mode, check orientation */
                  if (cz->mode.side == 0)
                    {
                       int ty;

                       /* grab the indicator position so we can tell if it
                        * is in a custom position or not (user dragged it) */
                       e_illume_border_indicator_pos_get(bd->zone, NULL, &ty);
                       if (ty <= bd->zone->y)
                          _policy_zone_layout_app_dual_top_new (bd_info, cz);
                       else
                          _policy_zone_layout_app_dual_custom_new (bd_info, cz);
                    }
                  else
                     _policy_zone_layout_app_dual_left_new (bd_info, cz);
               }
          }
     }
}

void
_policy_zone_move_resize(E_Zone *zone)
{
   Eina_List *l;
   E_Border *bd;

   //   printf("Zone move resize\n");

   if (!zone) return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        /* skip borders not on this zone */
        if (bd->zone != zone) continue;

        /* signal a changed pos here so layout gets updated */
        bd->changes.pos = 1;
        bd->changed = 1;
     }
}

void
_policy_zone_mode_change(E_Zone *zone, Ecore_X_Atom mode)
{
   E_Illume_Config_Zone *cz;
   E_Border *bd;

   //   printf("Zone mode change: %d\n", zone->id);

   if (!zone) return;

   /* get the config for this zone */
   cz = e_illume_zone_config_get(zone->id);
   if (!cz) return;

   /* update config with new mode */
   if (mode == ECORE_X_ATOM_E_ILLUME_MODE_SINGLE)
      cz->mode.dual = 0;
   else
     {
        cz->mode.dual = 1;
        if (mode == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_TOP)
           cz->mode.side = 0;
        else if (mode == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_LEFT)
           cz->mode.side = 1;
     }
   e_config_save_queue();

   /* lock indicator window from dragging if we need to */
   bd = e_illume_border_indicator_get(zone);
   if (bd)
     {
        /* only dual-top mode can drag */
        if ((cz->mode.dual == 1) && (cz->mode.side == 0))
          {
             /* only set locked if we need to */
             if (bd->client.illume.drag.locked != 0)
                ecore_x_e_illume_drag_locked_set(bd->client.win, 0);
          }
        else
          {
             /* only set locked if we need to */
             if (bd->client.illume.drag.locked != 1)
                ecore_x_e_illume_drag_locked_set(bd->client.win, 1);
          }
     }

   /* Need to trigger a layout update here */
   _policy_zone_layout_update(zone);
}

void
_policy_zone_close(E_Zone *zone)
{
   E_Border *bd;

   //   printf("Zone close\n");

   if (!zone) return;

   /* make sure we have a focused border */
   if (!(bd = e_border_focused_get())) return;

   /* make sure focused border is on this zone */
   if (bd->zone != zone) return;

   /* close this border */
   e_border_act_close_begin(bd);
}

void
_policy_drag_start(E_Border *bd)
{
   //   printf("Drag start\n");

   if (!bd) return;

   /* ignore stolen borders */
   if (bd->stolen) return;

   /* set property on this border to say we are dragging */
   ecore_x_e_illume_drag_set(bd->client.win, 1);

   /* set property on zone window that a drag is happening */
   ecore_x_e_illume_drag_set(bd->zone->black_win, 1);
}

void
_policy_drag_end(E_Border *bd)
{
   //   printf("Drag end\n");

   if (!bd) return;

   /* ignore stolen borders */
   if (bd->stolen) return;

   /* set property on this border to say we are done dragging */
   ecore_x_e_illume_drag_set(bd->client.win, 0);

   /* set property on zone window that a drag is finished */
   ecore_x_e_illume_drag_set(bd->zone->black_win, 0);
}

void
_policy_focus_back(E_Zone *zone)
{
   Eina_List *l, *fl = NULL;
   E_Border *bd, *fbd;

   if (!zone) return;
   if (eina_list_count(_pol_focus_stack) < 1) return;

   //   printf("Focus back\n");

   EINA_LIST_FOREACH(_pol_focus_stack, l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        fl = eina_list_append(fl, bd);
     }

   if (!(fbd = e_border_focused_get())) return;
   if (fbd->parent) return;

   EINA_LIST_REVERSE_FOREACH(fl, l, bd)
     {
        if ((fbd) && (bd == fbd))
          {
             E_Border *b;

             if ((l->next) && (b = l->next->data))
               {
                  _policy_border_set_focus(b);
                  break;
               }
             else
               {
                  /* we've reached the end of the list. Set focus to first */
                  if ((b = eina_list_nth(fl, 0)))
                    {
                       _policy_border_set_focus(b);
                       break;
                    }
               }
          }
     }
   eina_list_free(fl);
}

void
_policy_focus_forward(E_Zone *zone)
{
   Eina_List *l, *fl = NULL;
   E_Border *bd, *fbd;

   if (!zone) return;
   if (eina_list_count(_pol_focus_stack) < 1) return;

   //   printf("Focus forward\n");

   EINA_LIST_FOREACH(_pol_focus_stack, l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        fl = eina_list_append(fl, bd);
     }

   if (!(fbd = e_border_focused_get())) return;
   if (fbd->parent) return;

   EINA_LIST_FOREACH(fl, l, bd)
     {
        if ((fbd) && (bd == fbd))
          {
             E_Border *b;

             if ((l->next) && (b = l->next->data))
               {
                  _policy_border_set_focus(b);
                  break;
               }
             else
               {
                  /* we've reached the end of the list. Set focus to first */
                  if ((b = eina_list_nth(fl, 0)))
                    {
                       _policy_border_set_focus(b);
                       break;
                    }
               }
          }
     }
   eina_list_free(fl);
}

/* enable/disable composite module - 100320 yigl */
#ifdef COMP_MODULE_CONTROL
static void
_policy_property_composite_module_change(Ecore_X_Event_Window_Property *ev)
{
   int ret, count;
   int enable = 0;
   int current_enabled = 0;
   unsigned char* prop_data = NULL;
   E_Module* comp = NULL;

   ret = ecore_x_window_prop_property_get (ev->win, E_ILLUME_ATOM_COMP_MODULE_ENABLED, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
     {
        memcpy (&enable, prop_data, sizeof (int));
        fprintf( stdout, "[E17-illume2-slp] %s(%d) enable: %s ", __func__, __LINE__, enable ? "true" : "false" );

        comp = e_module_find ("comp-slp");
        if( comp )
          {
             current_enabled = e_module_enabled_get(comp);
             fprintf( stdout, "current: %s ", current_enabled ? "true" : "false" );

             if( current_enabled && !enable )
               {
                  fprintf( stdout, "e_module_disable(comp-slp) " );
                  e_module_disable(comp);
               }
             else if( !current_enabled && enable )
               {
                  fprintf( stdout, "e_module_enable(comp-slp) " );
                  e_module_enable(comp);
               }
             else
               {
                  fprintf( stdout, "skip... " );
               }

             fprintf( stdout, "\n" );
          }
        else
          {
             fprintf( stderr, "\n[E17-illume2-slp] %s(%d) can't find comp module.\n", __func__, __LINE__ );
          }
     }

   if (prop_data) free (prop_data);

}
#endif


static void _policy_property_window_state_change (Ecore_X_Event_Window_Property *event)
{
   E_Border *bd, *indi_bd;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   /* not interested in stolen or invisible borders */
   if ((bd->stolen) || (!bd->visible)) return;

   /* make sure the border has a name or class */
   /* NB: this check is here because some E borders get State Changes
    * but do not have a name/class associated with them. Not entirely sure
    * which ones they are, but I would guess Managers, Containers, or Zones.
    * At any rate, we're not interested in those types of borders */
   if ((!bd->client.icccm.name) || (!bd->client.icccm.class)) return;

   /* NB: If we have reached this point, then it should be a fullscreen
    * border that has toggled fullscreen on/off */

   /* if the window is not active window, then it doesn't need to hande indicator */
   if (g_active_win != bd->client.win) return;

   /* try to get the Indicator on this zone */
   if (!(indi_bd = e_illume_border_indicator_get(bd->zone))) return;

   /* if we are fullscreen, hide the indicator...else we show it */
   /* NB: we could use the e_illume_border_is_fullscreen function here
    * but we save ourselves a function call this way */
   if ((bd->fullscreen) || (bd->need_fullscreen))
     {
        if (indi_bd->visible) e_border_hide(indi_bd, 2);
     }
   else
     {
        int indi_show = _policy_border_indicator_state_get(bd);
        if (indi_show == 1)
          {
             if (!indi_bd->visible) e_border_show(indi_bd);
          }
     }

}

static void _policy_property_indicator_geometry_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   E_Border *bd;
   int x, y, w, h;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   /* get the geometry */
   if (!(bd = e_illume_border_indicator_get(zone))) return;
   x = bd->x;
   y = bd->y;
   w = bd->w;
   h = bd->h;

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (!e_illume_border_is_conformant(bd)) continue;
        /* set indicator geometry on conformant window */
        /* NB: This is needed so that conformant apps get told about
         * the indicator size/position...else they have no way of
         * knowing that the geometry has been updated */
        ecore_x_e_illume_indicator_geometry_set(bd->client.win, x, y, w, h);
     }
}

static void _policy_property_keyboard_geometry_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   E_Border *bd;
   int x, y, w, h;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   ecore_x_e_illume_keyboard_geometry_get(zone->black_win, &x, &y, &w, &h);

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (!e_illume_border_is_conformant(bd)) continue;
        /* set keyboard geometry on conformant window */
        /* NB: This is needed so that conformant apps get told about
         * the keyboard size/position...else they have no way of
         * knowing that the geometry has been updated */
        ecore_x_e_illume_keyboard_geometry_set(bd->client.win, x, y, w, h);
     }
}

static void _policy_property_virtual_keyboard_state_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   Ecore_X_Virtual_Keyboard_State state;
   E_Border *bd;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   state = ecore_x_e_virtual_keyboard_state_get (zone->black_win);

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (!e_illume_border_is_conformant(bd)) continue;
        /* set keyboard state on conformant window */
        /* NB: This is needed so that conformant apps get told about
         * the keyboard state...else they have no way of
         * knowing that the state of keyboard has been updated */
        ecore_x_e_virtual_keyboard_state_set (bd->client.win, state);
     }
}

static void _policy_property_sliding_win_geometry_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   E_Border *bd;
   int x, y, w, h;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   ecore_x_e_illume_sliding_win_geometry_get(zone->black_win, &x, &y, &w, &h);

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (e_illume_border_is_indicator(bd)) continue;
        if (e_illume_border_is_keyboard(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;

        ecore_x_e_illume_sliding_win_geometry_set(bd->client.win, x, y, w, h);
     }
}

static void _policy_property_sliding_win_state_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   E_Border *bd;
   int is_visible;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   is_visible = ecore_x_e_illume_sliding_win_state_get (zone->black_win);

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (e_illume_border_is_indicator(bd)) continue;
        if (e_illume_border_is_keyboard(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;

        ecore_x_e_illume_sliding_win_state_set (bd->client.win, is_visible);
     }
}

static void _policy_property_enlightenment_scale_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *ml;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        Eina_List *cl;
        E_Container *con;

        if (!man) continue;
        if (event->win != man->root) continue;
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             Eina_List *zl;
             E_Zone *zone;

             if (!con) continue;
             EINA_LIST_FOREACH(con->zones, zl, zone)
                _policy_zone_layout_update(zone);
          }
     }
}

static void _policy_property_rotate_win_angle_change (Ecore_X_Event_Window_Property *event)
{
   E_Border* bd;
   E_Illume_XWin_Info* xwin_info;

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. win:%x\n", __func__, __LINE__, event->win);

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   xwin_info = _policy_xwin_info_find (bd->win);
   if (xwin_info)
     {
        if (xwin_info->visibility == E_ILLUME_VISIBILITY_UNOBSCURED)
          {
             L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. CALL _policy_change_root_angle_by_border_angle!!! win:%x\n", __func__, __LINE__, bd->client.win);
             _policy_change_root_angle_by_border_angle (bd);
          }
     }
}

static void _policy_property_indicator_state_change (Ecore_X_Event_Window_Property *event)
{
   E_Border *bd, *indi_bd;
   Ecore_X_Window active_win;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   indi_bd = e_illume_border_indicator_get (bd->zone);
   if (!indi_bd) return;

   active_win = _policy_active_window_get(bd->zone->container->manager->root);
   if (active_win == bd->client.win)
     {
        int indi_show = _policy_border_indicator_state_get(bd);
        if (indi_show == 1)
          {
             ILLUME2_TRACE ("[ILLUME2] Get ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE (%d)   SHOW Indicator... bd = %x\n", __LINE__, event->win);
             e_border_show(indi_bd);

             if (e_illume_border_is_notification (bd))
               {
                  int level;
                  E_Illume_Border_Info* active_bd_info;
                  active_bd_info = _policy_get_border_info (bd);
                  if (active_bd_info)
                    {
                       level = active_bd_info->level;
                    }
                  else
                    {
                       level = 250;
                    }
                  L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, bd->client.win, level);
                  _policy_change_indicator_layer (indi_bd, POL_NOTIFICATION_LAYER, level);
               }
          }
        else if (indi_show == 0)
          {
             ILLUME2_TRACE ("[ILLUME2] Get ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE (%d)  HIDE Indicator... bd = %x\n", __LINE__, event->win);
             e_border_hide(indi_bd, 2);
          }
     }
}

static void _policy_property_active_win_change (Ecore_X_Event_Window_Property *event)
{
   Ecore_X_Window active_win;
   int indi_show;
   E_Border* active_border;
   E_Border* indi_bd;

   active_win = _policy_active_window_get(event->win);
   if (active_win)
     {
        active_border = e_border_find_by_client_window (active_win);
     }
   else
     {
        active_border = NULL;
     }

   /* for active/deactive message */
   if (active_win != g_active_win)
     {
        int active_pid ;
        if (active_border)
           active_pid = active_border->client.netwm.pid;
        else
           active_pid = 0;

        // 1. send deactive event to g_active_win
        ecore_x_client_message32_send (g_active_win, E_ILLUME_ATOM_DEACTIVATE_WINDOW,
                                       ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, active_win, active_pid, g_active_win, g_active_pid, 0);

        // 2. send active event to active_win
        ecore_x_client_message32_send (active_win, E_ILLUME_ATOM_ACTIVATE_WINDOW,
                                       ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, active_win, active_pid, g_active_win, g_active_pid, 0);

        // for debug...
        printf ("\n=========================================================================================\n");
        if (active_border)
          {
             printf ("[WM] Active window is changed.  OLD(win:%x,pid:%d) --> NEW(win:%x,pid:%d,name:%s)\n", g_active_win, g_active_pid, active_win, active_pid, active_border->client.netwm.name);
             LOG(LOG_DEBUG, "LAUNCH", "[WM] Active window is changed. OLD(win:%x,pid:%d) --> NEW(win:%x,pid:%d,name:%s)", g_active_win, g_active_pid, active_win, active_pid, active_border->client.netwm.name);
          }
        else
          {
             printf ("[WM] Active window is changed.  OLD(win:%x,pid:%d) --> NEW(win:%x,pid:%d,name:NULL)\n", g_active_win, g_active_pid, active_win, active_pid);
             LOG(LOG_DEBUG, "LAUNCH", "[WM] Active window is changed. OLD(win:%x,pid:%d) --> NEW(win:%x,pid:%d,name:NULL)", g_active_win, g_active_pid, active_win, active_pid);
          }
        printf ("=========================================================================================\n\n");
        g_active_win = active_win;
        g_active_pid = active_pid;
     }

   if (active_border)
     {
        indi_bd = e_illume_border_indicator_get (active_border->zone);
        if (indi_bd)
          {
             /* if the active window is full screen, then hide indicator */
             if (active_border->fullscreen || active_border->need_fullscreen)
               {
                  e_border_hide (indi_bd, 2);
                  return;
               }

             indi_show = _policy_border_indicator_state_get(active_border);
             if (indi_show == 1)
               {
                  // if border is notification, the indicator's layer is changed
                  if (e_illume_border_is_notification (active_border))
                    {
                       int level;
                       E_Illume_Border_Info* active_bd_info;
                       active_bd_info = _policy_get_border_info (active_border);
                       if (active_bd_info)
                         {
                            level = active_bd_info->level;
                         }
                       else
                         {
                            level = 250;
                         }
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, indi_bd->client.win, level);
                       _policy_change_indicator_layer (indi_bd, POL_NOTIFICATION_LAYER, level);
                    }
                  else
                    {
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, indi_bd->client.win, 50);
                       _policy_change_indicator_layer (indi_bd, POL_INDICATOR_LAYER, 50);
                    }

                  ILLUME2_TRACE ("[ILLUME2] Get ECORE_X_ATOM_NET_ACTIVE_WINDOW (%d)   SHOW Indicator... bd = %x\n", __LINE__, active_win);
                  e_border_show(indi_bd);
               }
             else if (indi_show == 0)
               {
                  ILLUME2_TRACE ("[ILLUME2] Get ECORE_X_ATOM_NET_ACTIVE_WINDOW (%d)  HIDE Indicator... bd = %x\n", __LINE__, active_win);
                  e_border_hide(indi_bd, 2);
               }
          }

        ILLUME2_TRACE ("[ILLUME2] ACTIVE WINDOW... (%d) active win = %x HIDE quickpanel\n", __LINE__, active_win);
        e_illume_quickpanel_hide (active_border->zone, 0);
     }
}

static void _policy_property_win_type_change (Ecore_X_Event_Window_Property *event)
{
   E_Border *bd;
   int level;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   e_hints_window_type_get (bd);
   if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NOTIFICATION)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WINDOW TYPE is CHANGED to NOTIFICATION!!!!! win = %x\n", __func__, __LINE__, bd->client.win);
        if (bd->layer != POL_NOTIFICATION_LAYER)
           e_border_layer_set(bd, POL_NOTIFICATION_LAYER);

        level = _policy_border_get_notification_level (bd->client.win);
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, level);
        _policy_border_update_notification_stack (bd, level, EINA_TRUE);

        if (_policy_active_window_get (bd->zone->container->manager->root) == bd->client.win)
          {
             int show_indi;
             E_Border* indi_bd;

             indi_bd = e_illume_border_indicator_get(bd->zone);
             if (indi_bd)
               {
                  show_indi = _policy_border_indicator_state_get (bd);
                  if (show_indi == 1)
                    {
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, indi_bd->client.win, level);
                       _policy_change_indicator_layer (indi_bd, POL_NOTIFICATION_LAYER, level);
                    }
               }
          }
     }
   else if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WINDOW TYPE is CHANGED to NORMAL!!!!! win = %x\n", __func__, __LINE__, bd->client.win);
        if (bd->layer != POL_APP_LAYER)
          {
             if ( (!e_illume_border_is_quickpanel(bd)) &&
                  (!e_illume_border_is_keyboard(bd)))
               {
                  e_border_layer_set(bd, POL_APP_LAYER);
               }
          }

        if (_policy_active_window_get (bd->zone->container->manager->root) == bd->client.win)
          {
             int show_indi;
             E_Border* indi_bd;

             indi_bd = e_illume_border_indicator_get(bd->zone);
             if (indi_bd)
               {
                  show_indi = _policy_border_indicator_state_get (bd);
                  if (show_indi == 1)
                    {
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, indi_bd->client.win, 50);
                       _policy_change_indicator_layer (indi_bd, POL_INDICATOR_LAYER, 50);
                    }
               }
          }
     }
   else if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UTILITY)
     {
        bd->client.icccm.request_pos = 1;

        if (bd->layer != POL_APP_LAYER)
           e_border_layer_set(bd, POL_APP_LAYER);
     }

}

static void _policy_property_rotate_root_angle_change (Ecore_X_Event_Window_Property *event)
{
   E_Border* bd;
   E_Border* indi_bd;
   int ret;
   int count;
   int angle = 0;
   unsigned char *prop_data = NULL;
   E_Zone* zone;

   ret = ecore_x_window_prop_property_get (event->win, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
      memcpy (&angle, prop_data, sizeof (int));

   if (prop_data) free (prop_data);

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. ROOT ANGLE CHANGED... angle = %d\n", __func__, __LINE__, angle);
   Ecore_X_Window active_win;
   active_win = _policy_active_window_get(event->win);

   bd = e_border_find_by_client_window(active_win);

   if (bd) zone = bd->zone;
   else zone = e_util_container_zone_number_get (0, 0);

   if (zone)
     {
        // send client message to all visible windows
        if (g_root_angle != angle)
          {
             indi_bd = e_illume_border_indicator_get(zone);
             if (indi_bd)
               {
                  _policy_indicator_angle_change (indi_bd, angle);
               }

             Eina_List *l;
             E_Border* bd_temp;

             EINA_LIST_FOREACH(e_border_client_list(), l, bd_temp)
               {
                  if (!bd_temp) continue;
                  if (!bd_temp->visible) continue;

                  L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... win = %x..  SEND client Event with angle = %d\n", __func__, __LINE__, bd_temp->client.win, angle);
                  ecore_x_client_message32_send (bd_temp->client.win, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, angle, g_active_win, 0, 0, 0);
               }

             e_illume_util_hdmi_rotation (event->win, angle);
          }
     }

   g_root_angle = angle;
}

static void _policy_property_notification_level_change (Ecore_X_Event_Window_Property *event)
{
   /*
      0. Check if border is exist or not
      1. Check if a window is notification or not
      2. Get and Set level
      3. Change Stack
    */
   L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... E_ILLUME_ATOM_NOTIFICATION_LEVEL property!!!  win = %x\n", __func__, __LINE__, event->win);

   E_Border* bd;
   int level;

   // 0.
   if (!(bd = e_border_find_by_client_window(event->win)))
     {
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Win (%x) doesn't have border... So return..\n", __func__, __LINE__, event->win);
        return;
     }

   // 1.
   if (!e_illume_border_is_notification (bd))
     {
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Win (%x) gets NOTIFICATION_LEVEL notifiy... But this is NOT notification window... IGNORE!!!!!\n", __func__, __LINE__, bd->client.win);
        return;
     }

   // 2.
   level = _policy_border_get_notification_level (bd->client.win);
   L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Win (%x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, level);

   // 3.
   _policy_border_update_notification_stack (bd, level, EINA_TRUE);

   // 4.
   if (_policy_active_window_get (bd->zone->container->manager->root) == bd->client.win)
     {
        int show_indi;
        E_Border* indi_bd;

        indi_bd = e_illume_border_indicator_get(bd->zone);
        if (indi_bd)
          {
             show_indi = _policy_border_indicator_state_get (bd);
             if (show_indi == 1)
               {
                  L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, indi_bd->client.win, level);
                  _policy_change_indicator_layer (indi_bd, POL_NOTIFICATION_LAYER, level);
               }
          }
     }
}

static void _policy_property_overlay_win_change (Ecore_X_Event_Window_Property *event)
{
   int ret;
   int count;
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get (event->win, E_ILLUME_ATOM_OVERAY_WINDOW, ECORE_X_ATOM_WINDOW, 32, &prop_data, &count);
   if( ret && prop_data )
      memcpy (&_e_overlay_win, prop_data, sizeof (ECORE_X_ATOM_WINDOW));

   if (prop_data) free (prop_data);

   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d)... OVERAY_WINDOW:%x\n", __func__, __LINE__, _e_overlay_win);
   _policy_xwin_info_delete (_e_overlay_win);
}

static int _policy_property_window_opaque_get (Ecore_X_Window win)
{
   int ret;
   int count;
   int is_opaque = 0;
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get (win, E_ILLUME_ATOM_WINDOW_OPAQUE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
      memcpy (&is_opaque, prop_data, sizeof (ECORE_X_ATOM_CARDINAL));

   if (prop_data) free (prop_data);

   return is_opaque;
}

static void _policy_property_window_opaque_change (Ecore_X_Event_Window_Property *event)
{
   E_Border* bd;
   E_Illume_Border_Info* bd_info;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   // get border info
   bd_info = _policy_get_border_info (bd);
   if (!bd_info) return;

   // set current property
   bd_info->opaque = _policy_property_window_opaque_get (event->win);

   // recalculate visibility
   _policy_calculate_visibility (bd_info->border->win);
}

void
_policy_property_change(Ecore_X_Event_Window_Property *event)
{
   //   printf("Property Change\n");

   /* we are interested in state changes here */
   if (event->atom == ECORE_X_ATOM_NET_WM_STATE)
     {
        _policy_property_window_state_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_GEOMETRY)
     {
        _policy_property_indicator_geometry_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_KEYBOARD_GEOMETRY)
     {
        _policy_property_keyboard_geometry_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
     {
        _policy_property_virtual_keyboard_state_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_SLIDING_WIN_GEOMETRY)
     {
        _policy_property_sliding_win_geometry_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_SLIDING_WIN_STATE)
     {
        _policy_property_sliding_win_state_change (event);
     }
   else if (event->atom == ATM_ENLIGHTENMENT_SCALE)
     {
        _policy_property_enlightenment_scale_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE)
     {
        _policy_property_rotate_win_angle_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE)
     {
        _policy_property_indicator_state_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        _policy_property_active_win_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_NET_WM_WINDOW_TYPE)
     {
        _policy_property_win_type_change (event);
     }
   else if (event->atom == E_ILLUME_ATOM_STACK_DISPLAY)
     {
        _policy_border_list_print (event->win);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE)
     {
        _policy_property_rotate_root_angle_change (event);
     }
   else if (event->atom == E_ILLUME_ATOM_NOTIFICATION_LEVEL)
     {
        _policy_property_notification_level_change (event);
     }
   /* for visibility */
   else if (event->atom == E_ILLUME_ATOM_OVERAY_WINDOW)
     {
        _policy_property_overlay_win_change (event);
     }
   else if (event->atom == E_ILLUME_ATOM_WINDOW_OPAQUE)
     {
        _policy_property_window_opaque_change (event);
     }
#if ILLUME_LOGGER_BUILD_ENABLE
   else if (event->atom == E_ILLUME_ATOM_ILLUME_LOG)
     {
        _policy_property_illume_log_change (event);
     }
#endif
   /* enable/disable composite module - 100320 yigl */
#ifdef COMP_MODULE_CONTROL
   else if (event->atom == E_ILLUME_ATOM_COMP_MODULE_ENABLED)
     {
        _policy_property_composite_module_change (event);
     }
#endif
}


void
_policy_border_list_print (Ecore_X_Window win)
{
   Eina_List* border_list;
   Eina_List *l;
   E_Border *bd;
   E_Border* temp_bd = NULL;
   int i, ret, count;
   E_Illume_Print_Info info;
   unsigned char* prop_data = NULL;
   FILE* out;

   info.type = 0;
   memset (info.file_name, 0, 256);

   ret = ecore_x_window_prop_property_get (win, E_ILLUME_ATOM_STACK_DISPLAY, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
      memcpy (&info, prop_data, sizeof (E_Illume_Print_Info));

   if (prop_data) free (prop_data);

   out = fopen (info.file_name, "w+");
   if (out == NULL) out = stderr;

   if ((info.type & PT_STACK) == PT_STACK)
     {
        border_list = e_border_client_list();
        if (border_list)
          {
             i = 1;
             fprintf (out, "--------------------------------BORDER INFO--------------------------------------------------\n" );
             fprintf (out, " No  Border     ClientWin     w     h     x     y   layer  visible  WinName\n" );
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );

             EINA_LIST_FOREACH(border_list, l, bd)
               {
                  if (!bd) continue;
                  if (temp_bd == NULL) temp_bd = bd;

                  fprintf (out, "%3i  0x%07x  0x%07x  %4i  %4i  %4i  %4i  %5i  %5i     %-30s \n",
                          i++, bd->win, bd->client.win, bd->w, bd->h, bd->x, bd->y, bd->layer, bd->visible, bd->client.netwm.name);
               }
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );
          }
        ecore_x_flush();

        E_Illume_Border_Info* bd_info;
        if (e_border_info_list)
          {
             i = 1;
             fprintf (out, "--------------------------------ILLUME BORDER INFO------------------------------------------------------------------\n" );
             fprintf (out, " No  Border     ClientWin     w     h     x     y   isKBD layer level  WinName\n" );
             fprintf (out, "--------------------------------------------------------------------------------------------------------------------\n" );

             EINA_LIST_FOREACH(e_border_info_list, l, bd_info)
               {
                  if (!bd_info) continue;
                  fprintf (out, "%3i  0x%07x  0x%07x  %4i  %4i  %4i  %4i  %3i  %5i %5i   %-30s \n",
                          i++, bd_info->border->win, bd_info->border->client.win, bd_info->border->w, bd_info->border->h, bd_info->border->x, bd_info->border->y,
                          bd_info->border->client.vkbd.vkbd, bd_info->border->layer, bd_info->level, bd_info->border->client.netwm.name);
               }
             fprintf (out, "--------------------------------------------------------------------------------------------------------------------\n" );
          }
        ecore_x_flush();

        if (temp_bd == NULL) goto finish;

        E_Border_List *bl;

        fprintf (out, "\n\n-------------------------------- E17 STACK INFO--------------------------------------------\n" );
        fprintf (out, " No  Border     ClientWin     w     h     x     y   layer  visible  WinName\n" );
        fprintf (out, "---------------------------------------------------------------------------------------------\n" );

        i = 1;
        bl = e_container_border_list_last(temp_bd->zone->container);
        while ((bd = e_container_border_list_prev(bl)))
          {
             fprintf (out, "%3i  0x%07x  0x%07x  %4i  %4i  %4i  %4i  %5i  %5i     %-30s \n",
                     i++, bd->win, bd->client.win, bd->w, bd->h, bd->x, bd->y, bd->layer, bd->visible, bd->client.netwm.name);
          }
        e_container_border_list_free(bl);
        fprintf (out, "---------------------------------------------------------------------------------------------\n\n" );
     }

   /* for visibility */
   if ((info.type & PT_VISIBILITY) == PT_VISIBILITY)
     {
        Eina_Inlist* xwin_info_list;
        E_Illume_XWin_Info *xwin_info;

        xwin_info_list = _e_illume_xwin_info_list;
        if (xwin_info_list)
          {
             i = 1;
             fprintf (out, "--------------------------------BORDER INFO--------------------------------------------------\n" );
             fprintf (out, " No  Win          w     h     x     y   depth  viewable  visibility is_border(Client Win)\n" );
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );

             EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
               {
                  if (xwin_info->bd_info)
                    {
                       if (xwin_info->bd_info->border)
                         {
                            fprintf (out, "%3i  0x%07x  %4i  %4i  %4i  %4i  %5i   %5i      %5i        yes(0x%07x)\n",
                                    i++, xwin_info->id, xwin_info->attr.w, xwin_info->attr.h, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.depth, xwin_info->attr.visible, xwin_info->visibility, xwin_info->bd_info->border->client.win);
                         }
                       else
                         {
                            fprintf (out, "%3i  0x%07x  %4i  %4i  %4i  %4i  %5i   %5i      %5i        no(NULL)\n",
                                    i++, xwin_info->id, xwin_info->attr.w, xwin_info->attr.h, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.depth, xwin_info->attr.visible, xwin_info->visibility);
                         }
                    }
                  else
                    {
                       fprintf (out, "%3i  0x%07x  %4i  %4i  %4i  %4i  %5i   %5i      %5i        no(NULL)\n",
                               i++, xwin_info->id, xwin_info->attr.w, xwin_info->attr.h, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.depth, xwin_info->attr.visible, xwin_info->visibility);
                    }
               }
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );
          }

        ecore_x_flush();
     }

finish:

   fprintf (out, "--------------------------------GLOBAL INFO--------------------------------------------------\n" );
   fprintf (out, "g_rotated_win:%x (g_root_angle:%d)\n", g_rotated_win, g_root_angle);
   fprintf (out, "g_active_win:%x (pid:%d, angle:%d)\n", g_active_win, g_active_pid, _policy_window_rotation_angle_get(g_active_win));
   fprintf (out, "---------------------------------------------------------------------------------------------\n" );

   if (out != stderr)
     {
        fflush (out);
        fclose (out);
     }

   ecore_x_client_message32_send (ecore_x_window_root_first_get(), E_ILLUME_ATOM_STACK_DISPLAY_DONE,
                                  ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                  0, 0, 0, 0, 0);
}


static int
_policy_window_rotation_angle_get(Ecore_X_Window win)
{
   Atom type_ret = 0;
   int ret, size_ret = 0;
   unsigned long num_ret = 0, bytes = 0;
   unsigned char *prop_ret = NULL;
   Ecore_X_Display *dpy;
   int angle;

   dpy = ecore_x_display_get();

   if (!win)
     win = ecore_x_window_root_first_get();

   ret = XGetWindowProperty(dpy, win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE, 0, LONG_MAX,
                            False, ECORE_X_ATOM_CARDINAL, &type_ret, &size_ret,
                            &num_ret, &bytes, &prop_ret);

   if (ret == E_ERROR_CODE_SUCCESS)
     {
        if (prop_ret && num_ret)
          angle = ((int *)prop_ret)[0];
        else
          angle = 0;
     }
   else
     {
        angle = -1;
     }

   if (prop_ret) XFree(prop_ret);

   return angle;
}


static Ecore_X_Window
_policy_active_window_get(Ecore_X_Window root)
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

static int
_policy_border_indicator_state_get(E_Border *bd)
{
   int ret;
   int count;
   int show = -1;
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get (bd->client.win, ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
      memcpy (&show, prop_data, sizeof (int));

   if (prop_data) free (prop_data);

   return show;
}

static void _policy_layout_quickpanel_rotate (E_Illume_Quickpanel* qp, int angle)
{
   E_Border* bd;
   Eina_List *bd_list;
   E_Illume_Quickpanel_Info *panel;

   if (!qp) return;

   int diff, temp;

   // pass 1 - resize window
   EINA_LIST_FOREACH(qp->borders, bd_list, panel)
     {
        if (!panel) continue;
        if (panel->angle == angle) continue;

        if (panel->angle > angle) diff = panel->angle - angle;
        else diff = angle - panel->angle;

        bd = panel->bd;

        if (angle == 0 || angle == 180)
          {
             if (diff == 90 || diff == 270)
               {
                  temp = bd->w;
                  ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Portrait quick panel...(%d) quick win = %x  old (%d, %d)  new (%d, %d)\n", __LINE__, bd->client.win, bd->w, bd->h, bd->zone->w, temp);
                  _policy_border_resize (bd, bd->zone->w, temp);
               }
             else
                ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Portrait quick panel...(%d) quick win = %x..  But size is not change\n", __LINE__, bd->client.win);
          }
        else
          {
             if (diff == 90 || diff == 270)
               {
                  temp = bd->h;
                  ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Landscape quick panel...(%d) quick win = %x  old (%d, %d)  new (%d, %d)\n", __LINE__, bd->client.win, bd->w, bd->h, temp, bd->zone->h);
                  _policy_border_resize (bd, temp, bd->zone->h);
               }
             else
                ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Landscape quick panel...(%d) quick win = %x..  But size is not change\n", __LINE__, bd->client.win);
          }
     }

   // pass 2 - send client message
   EINA_LIST_FOREACH(qp->borders, bd_list, panel)
     {
        if (!panel) continue;
        if (panel->angle == angle) continue;

        if (panel->angle > angle) diff = panel->angle - angle;
        else diff = angle - panel->angle;

        bd = panel->bd;

        ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] SEND CLIENT MESSAGE TO QUICKPANEL!!!!(%d)  win:%x, angle = %d\n", __LINE__, bd->client.win, angle);
        ecore_x_client_message32_send (bd->client.win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                       ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                       angle, 0, 0, 0, 0);

        panel->angle = angle;
     }
}

void
_policy_window_focus_in(Ecore_X_Event_Window_Focus_In *event)
{
   ILLUME2_TRACE("[ILLUME2-FOCUS] _policy_window_focus_in... win = %x\n", event->win);

   E_Border *bd;

   if (e_config->focus_policy == E_FOCUS_CLICK) return;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   if (e_illume_border_is_indicator (bd))
     {
        Eina_List *ml, *cl;
        E_Manager *man;
        E_Container *con;

        EINA_LIST_FOREACH(e_manager_list(), ml, man)
          {
             if (!man) continue;
             EINA_LIST_FOREACH(man->containers, cl, con)
               {
                  E_Border_List *bl;
                  E_Border *temp_bd;

                  // send focus to top-level window.
                  bl = e_container_border_list_last(con);
                  while ((temp_bd = e_container_border_list_prev(bl)))
                    {
                       if (temp_bd->client.icccm.accepts_focus && temp_bd->visible)
                         {
                            /* focus the border */
                            e_border_focus_set(temp_bd, 1, 1);
                            e_container_border_list_free(bl);
                            return;
                         }
                    }
                  e_container_border_list_free(bl);
               }
          }
     }
}


void _policy_border_stack_change (E_Border* bd, E_Border* sibling, int stack_mode)
{
   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... win = %x, sibling = %x, stack mode = %d\n", __func__, __LINE__, bd->client.win, sibling->client.win, stack_mode);

   if (bd->layer != sibling->layer)
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... Sibling layer is different!!! win = %x (layer = %d), sibling = %x (layer = %d)\n", __func__, __LINE__, bd->client.win, bd->layer, sibling->client.win, sibling->layer);
        return;
     }

   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... Restack Window.. win = %x,  sibling = %x, stack_mode = %d\n", __func__, __LINE__, bd->win, sibling->win, stack_mode);
   if (stack_mode == E_ILLUME_STACK_ABOVE)
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... STACK CHANGE with ABOVE... win:%x, above_win:%x\n", __func__, __LINE__, bd->client.win, sibling->client.win);
        e_border_stack_above (bd, sibling);
     }
   else if (stack_mode == E_ILLUME_STACK_BELOW)
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... STACK CHANGE with BELOW... win:%x, below_win:%x\n", __func__, __LINE__, bd->client.win, sibling->client.win);
        e_border_stack_below (bd, sibling);
     }
}


int _policy_atom_init (void)
{
   /* Notification Level Atom */
   E_ILLUME_ATOM_NOTIFICATION_LEVEL = ecore_x_atom_get ("_E_ILLUME_NOTIFICATION_LEVEL");
   if (!E_ILLUME_ATOM_NOTIFICATION_LEVEL)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_NOTIFICATION_LEVEL Atom...\n");
        return 0;
     }

   /* for active/deactive message */
   E_ILLUME_ATOM_ACTIVATE_WINDOW = ecore_x_atom_get ("_X_ILLUME_ACTIVATE_WINDOW");
   if (!E_ILLUME_ATOM_ACTIVATE_WINDOW)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _X_ILLUME_ACTIVATE_WINDOW Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_DEACTIVATE_WINDOW = ecore_x_atom_get ("_X_ILLUME_DEACTIVATE_WINDOW");
   if (!E_ILLUME_ATOM_DEACTIVATE_WINDOW)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _X_ILLUME_DEACTIVATE_WINDOW Atom...\n");
        return 0;
     }

   /* for visibility */
   E_ILLUME_ATOM_OVERAY_WINDOW = ecore_x_atom_get ("_E_COMP_OVERAY_WINDOW");
   if (!E_ILLUME_ATOM_OVERAY_WINDOW)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_COMP_OVERAY_WINDOW Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_STACK_DISPLAY = ecore_x_atom_get ("_E_ILLUME_PRINT_BORDER_WIN_STACK");
   if (!E_ILLUME_ATOM_STACK_DISPLAY)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_PRINT_BORDER_WIN_STACK Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_STACK_DISPLAY_DONE = ecore_x_atom_get ("_E_ILLUME_PRINT_BORDER_WIN_STACK_DONE");
   if (!E_ILLUME_ATOM_STACK_DISPLAY_DONE)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_PRINT_BORDER_WIN_STACK_DONE Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_WINDOW_OPAQUE = ecore_x_atom_get ("_E_ILLUME_WINDOW_REGION_OPAQUE");
   if (!E_ILLUME_ATOM_WINDOW_OPAQUE)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_WINDOW_REGION_OPAQUE Atom...\n");
        return 0;
     }

#ifdef COMP_MODULE_CONTROL
   E_ILLUME_ATOM_COMP_MODULE_ENABLED = ecore_x_atom_get ("_E_COMP_ENABLE");
   if(!E_ILLUME_ATOM_COMP_MODULE_ENABLED)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_COMP_ENABLE Atom...\n");
        return 0;
     }
#endif

#if ILLUME_LOGGER_BUILD_ENABLE
   E_ILLUME_ATOM_ILLUME_LOG = ecore_x_atom_get ("_E_ILLUME_LOG");
   if(!E_ILLUME_ATOM_ILLUME_LOG)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_LOG Atom...\n");
        return 0;
     }
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

   // for screen lock
   E_ILLUME_ATOM_SCREEN_LOCK = ecore_x_atom_get ("_E_COMP_LOCK_SCREEN");
   if(!E_ILLUME_ATOM_SCREEN_LOCK)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_COMP_LOCK_SCREEN Atom...\n");
        return 0;
     }

   return 1;
}


int _policy_init (void)
{
   Eina_List *ml;
   E_Manager *man;

   g_screen_lock_info = (E_Illume_Screen_Lock_Info*) malloc (sizeof (E_Illume_Screen_Lock_Info));
   if (!g_screen_lock_info) return 0;

   g_screen_lock_info->is_lock = 0;
   g_screen_lock_info->man = NULL;
   g_screen_lock_info->blocked_list = NULL;
   g_screen_lock_info->lock_timer = NULL;

   /* for visibility */
   _e_illume_xwin_info_hash = eina_hash_string_superfast_new(NULL);
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        _policy_manage_xwins (man);
     }

   // initialize atom
   if (!_policy_atom_init())
     {
        eina_hash_free (_e_illume_xwin_info_hash);
        free (g_screen_lock_info);
        return 0;
     }

   return 1;
}


void _policy_fin (void)
{
   /* for visibility */
   eina_hash_free (_e_illume_xwin_info_hash);

   if (g_screen_lock_info->blocked_list) eina_list_free (g_screen_lock_info->blocked_list);
   g_screen_lock_info->blocked_list = NULL;

   if (g_screen_lock_info->is_lock == 1)
     {
        _policy_request_screen_unlock (g_screen_lock_info->man);

        g_screen_lock_info->man = NULL;
        g_screen_lock_info->is_lock = 0;
     }

   free (g_screen_lock_info);
}

static int
_policy_compare_cb_border (E_Illume_Border_Info* data1, E_Illume_Border_Info* data2)
{
   if (data1 && data2)
     {
        if (data1->border == data2->border)
           return 0;
     }

   return 1;
}


static E_Illume_Border_Info*
_policy_get_border_info (E_Border* bd)
{
   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... bd = %x, bd's client = %x\n", __func__, __LINE__, bd, bd->client.win);
   E_Illume_Border_Info tmp_win;
   tmp_win.border = bd;
   return (E_Illume_Border_Info*) eina_list_search_unsorted (
      e_border_info_list, EINA_COMPARE_CB(_policy_compare_cb_border), &tmp_win);
}


static E_Illume_Border_Info* _policy_add_border_info_list (E_Border* bd)
{
   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... bd = %x, bd's client = %x\n", __func__, __LINE__, bd, bd->client.win);

   if (e_object_is_del(E_OBJECT(bd))) return NULL;

   E_Illume_Border_Info* bd_info = (E_Illume_Border_Info*) malloc (sizeof (E_Illume_Border_Info));
   if (!bd_info)
     {
        fprintf (stderr, "[ILLUME_SS] Critical Error... Fail to create memory... (%s:%d)\n", __func__, __LINE__);
        return NULL;
     }
   bd_info->pid = bd->client.netwm.pid;
   bd_info->border = bd;

   // set level
   bd_info->level = 50;

   // could find bd_info of ev->stack.. there is no bd_info yet...
   Eina_List *l = NULL;
   E_Illume_Border_Info *temp_bd_info = NULL;

   EINA_LIST_FOREACH(e_border_info_list, l, temp_bd_info)
     {
        if (!temp_bd_info) continue;
        if (bd_info->border->layer >= temp_bd_info->border->layer)
        break;
     }

   if (temp_bd_info)
     {
        e_border_info_list = eina_list_prepend_relative(e_border_info_list, bd_info, temp_bd_info);
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:%x) -> other(win:%x) ]\n", __func__, __LINE__, bd->client.win, temp_bd_info->border->client.win);
     }
   else
     {
        e_border_info_list = eina_list_append (e_border_info_list, bd_info);
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ ---> bd(win:%x) ]\n", __func__, __LINE__, bd->client.win);
     }

   return bd_info;
}


static void _policy_delete_border_info_list (E_Border* bd)
{
   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)...  bd = %x, bd's client = %x\n", __func__, __LINE__, bd, bd->client.win);
   E_Illume_Border_Info* bd_info = _policy_get_border_info (bd);

   if (bd_info == NULL)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... There is no border in the list... bd = %x, bd's client = %x\n", __func__, __LINE__, bd, bd->client.win);
        return;
     }

   e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... remove bd(win:%x)\n", __func__, __LINE__, bd->client.win);
   free (bd_info);
}


static int
_policy_zone_layout_app_layer_check (E_Border* bd)
{
   Ecore_X_Window_Type *types = NULL;
   int num, i, layer;

   if (!bd) return POL_APP_LAYER;

   layer = POL_APP_LAYER;

   num = ecore_x_netwm_window_types_get(bd->client.win, &types);
   if (num)
     {
        i = 0;
        for (i=0; i< num; i++)
          {
             if (types[i] == ECORE_X_WINDOW_TYPE_NOTIFICATION)
                layer = POL_NOTIFICATION_LAYER;
          }

        free (types);
     }

   if (bd->client.netwm.state.stacking == E_STACKING_BELOW)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WIN:%x  is BELOW state.. Change layer to 50\n", __func__, __LINE__, bd->client.win);
        layer = POL_STATE_BELOW_LAYER;
     }
   else if (bd->client.netwm.state.stacking == E_STACKING_ABOVE)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WIN:%x  is ABOVE state.. Change layer to 150\n", __func__, __LINE__, bd->client.win);
        layer = POL_STATE_ABOVE_LAYER;
     }

   return layer;
}


static void
_policy_zone_layout_app_layer_set (E_Border* bd, int new_layer)
{
   if (!bd) return;

   /* if a window sets transient_for property, it sets same layer to parent window */
   if (bd->client.icccm.transient_for != 0)
     {
        E_Border *bd_parent = NULL;
        bd_parent = e_border_find_by_client_window (bd->client.icccm.transient_for);
        if (bd_parent)
          {
             if (bd->layer != bd_parent->layer)
                e_border_layer_set (bd, bd_parent->layer);
          }
     }
   else
     {
        if (bd->layer != new_layer)
           e_border_layer_set(bd, new_layer);
     }
}

static void
_policy_zone_layout_app_single_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone *cz)
{
   E_Border* bd;
   int layer;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_SINGLE... bd_info's border = %x, client win = %x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

   bd = bd_info->border;
   if (!bd)
     {
        fprintf (stderr, "[ILLUME2] fatal error! (%s)  There is no border!\n", __func__);
        return;
     }

   if ((!bd->new_client) && (!bd->visible)) return;

   layer = _policy_zone_layout_app_layer_check (bd);

   /* check if user defined position */
   if (bd->client.icccm.request_pos)
     {
        _policy_zone_layout_app_layer_set (bd, layer);
        return;
     }

   /* resize if needed */
   if ((bd->w != bd->zone->w) || (bd->h != bd->zone->h))
      _policy_border_resize(bd, bd->zone->w, bd->zone->h);

   if ((bd->x != bd->zone->x) || (bd->y != bd->zone->y))
      _policy_border_move(bd, bd->zone->x, bd->zone->y);

   /* set layer if needed */
   _policy_zone_layout_app_layer_set (bd, layer);
}


static void
_policy_zone_layout_app_dual_top_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone *cz)
{
   E_Border* bd;
   E_Border* temp_bd;
   int ny, nh;
   int layer;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_DUAL_TOP... bd_info's border = %x, client win = %x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

   bd = bd_info->border;

   if (!bd || !cz) return;
   if ((!bd->new_client) && (!bd->visible)) return;

   layer = _policy_zone_layout_app_layer_check (bd);

   // check if user defined position
   if (bd->client.icccm.request_pos)
     {
        _policy_zone_layout_app_layer_set (bd, layer);
        return;
     }

   /* set a default Y position */
   ny = (bd->zone->y + cz->indicator.size);
   nh = ((bd->zone->h - cz->indicator.size - cz->softkey.size) / 2);

   /* see if there is a border already there. if so, check placement based on
    * virtual keyboard usage */
   temp_bd = e_illume_border_at_xy_get(bd->zone, bd->zone->x, ny);
   if ((temp_bd) && (temp_bd != bd)) ny = temp_bd->y + nh;

   /* resize if needed */
   if ((bd->w != bd->zone->w) || (bd->h != nh))
      _policy_border_resize(bd, bd->zone->w, nh);

   /* move to correct position (relative to zone) if needed */
   if ((bd->x != bd->zone->x) || (bd->y != ny))
      _policy_border_move(bd, bd->zone->x, ny);

   /* set layer if needed */
   _policy_zone_layout_app_layer_set (bd, layer);
}


static void
_policy_zone_layout_app_dual_left_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone *cz)
{
   E_Border* bd;
   E_Border* temp_bd;
   int ky, kh, nx, nw;
   int layer;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_DUAL_LEFT... bd_info's border = %x, client win = %x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

   bd = bd_info->border;

   if (!bd || !cz) return;
   if ((!bd->new_client) && (!bd->visible)) return;

   layer = _policy_zone_layout_app_layer_check (bd);

   // check if user defined position
   if (bd->client.icccm.request_pos)
     {
        _policy_zone_layout_app_layer_set (bd, layer);
        return;
     }

   /* set some defaults */
   nx = bd->zone->x;
   nw = (bd->zone->w / 2);

   ky = bd->zone->y + cz->indicator.size;
   kh = bd->zone->h - cz->indicator.size - cz->softkey.size;

   /* see if there is a border already there. if so, place at right */
   temp_bd = e_illume_border_at_xy_get(bd->zone, nx, (ky + bd->zone->h / 2));
   if ((temp_bd) && (bd != temp_bd)) nx = temp_bd->x + nw;

   /* resize if needed */
   if ((bd->w != nw) || (bd->h != kh))
      _policy_border_resize(bd, nw, kh);

   /* move to correct position (relative to zone) if needed */
   if ((bd->x != nx) || (bd->y != ky))
     _policy_border_move(bd, nx, ky);

   /* set layer if needed */
   _policy_zone_layout_app_layer_set (bd, layer);
}


static void
_policy_zone_layout_app_dual_custom_new (E_Illume_Border_Info* bd_info, E_Illume_Config_Zone *cz)
{
   E_Border* bd;
   E_Border *app;
   int iy, ny, nh;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_DUAL_CUSTOM... bd_info's border = %x, client win = %x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

   bd = bd_info->border;
   if (!bd)
     {
        fprintf (stderr, "[ILLUME2] fatal error! (%s)  There is no border!\n", __func__);
        return;
     }

   if ((!bd->new_client) && (!bd->visible)) return;

   /* grab indicator position */
   e_illume_border_indicator_pos_get(bd->zone, NULL, &iy);

   /* set a default position */
   ny = bd->zone->y;
   nh = iy;

   app = e_illume_border_at_xy_get(bd->zone, bd->zone->x, bd->zone->y);
   if (app)
     {
        if (bd != app)
          {
             ny = (iy + cz->indicator.size);
             nh = ((bd->zone->y + bd->zone->h) - ny - cz->softkey.size);
          }
     }

   /* make sure it's the required width & height */
   if ((bd->w != bd->zone->w) || (bd->h != nh))
      _policy_border_resize(bd, bd->zone->w, nh);

   /* move to correct position (relative to zone) if needed */
   if ((bd->x != bd->zone->x) || (bd->y != ny))
      _policy_border_move(bd, bd->zone->x, ny);

   /* set layer if needed */
   if (bd->layer != POL_APP_LAYER)
      e_border_layer_set(bd, POL_APP_LAYER);
}

static int _policy_border_get_notification_level (Ecore_X_Window win)
{
   int ret;
   int num;
   int level = 50;
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get (win, E_ILLUME_ATOM_NOTIFICATION_LEVEL, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &num);
   if( ret && prop_data )
      memcpy (&level, prop_data, sizeof (int));

   if (prop_data) free (prop_data);

   return level;
}

static void _update_notification_stack (E_Illume_Border_Info* bd_info, int level)
{
   Eina_List* l;
   E_Border* temp_bd;
   E_Border* below_bd;
   E_Border* upper_bd;
   E_Illume_Border_Info* temp_bd_info;
   E_Illume_Border_Info* below_bd_info;
   E_Illume_Border_Info* upper_bd_info;

   below_bd = NULL;
   upper_bd = NULL;
   below_bd_info = NULL;
   upper_bd_info = NULL;

   EINA_LIST_FOREACH(e_border_info_list, l, temp_bd_info)
     {
        if (!temp_bd_info) continue;

        temp_bd = temp_bd_info->border;

        /* skip borders not on this zone */
        if (temp_bd->layer != POL_NOTIFICATION_LAYER) continue;
        if (temp_bd == bd_info->border) continue;

        if (temp_bd_info->level <= level)
          {
             // find below border
             below_bd = temp_bd;
             below_bd_info = temp_bd_info;

             L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Stack Change!!! (Top) Win (%x) -> Sibling (%x) (Bottom)\n", __func__, __LINE__, bd_info->border->client.win, below_bd->client.win);

             _policy_border_stack_change (bd_info->border, below_bd, E_ILLUME_STACK_ABOVE);

             e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
             e_border_info_list = eina_list_prepend_relative (e_border_info_list, bd_info, below_bd_info);
             L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:%x) -> other(win:%x) ]\n", __func__, __LINE__, bd_info->border->client.win, below_bd_info->border->client.win);
             return;
          }
        else
          {
             upper_bd = temp_bd;
             upper_bd_info = temp_bd_info;
          }
     }

   if (upper_bd_info)
     {
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Stack Change!!! (Top) Sibling (%x) -> Win (%x) (Bottom)\n", __func__, __LINE__, upper_bd->client.win, bd_info->border->client.win);

        _policy_border_stack_change (bd_info->border, upper_bd, E_ILLUME_STACK_BELOW);

        e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
        e_border_info_list = eina_list_append_relative (e_border_info_list, bd_info, upper_bd_info);
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ other(win:%x) -> bd(win:%x) ]\n", __func__, __LINE__, upper_bd_info->border->client.win, bd_info->border->client.win);
     }
   else
     {
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Stack Change!!! Sibling is NULL.. So no operation..\n", __func__, __LINE__);
     }
}


/* find new focus window */
static void
_policy_border_focus_top_stack_set (E_Border* bd)
{
   E_Border *temp_bd;
   int root_w, root_h;

   root_w = bd->zone->w;
   root_h = bd->zone->h;

   E_Border_List *bl;
   bl = e_container_border_list_last(bd->zone->container);
   while ((temp_bd = e_container_border_list_prev(bl)))
     {
        if (!temp_bd) continue;
        if ((temp_bd->x >= root_w) || (temp_bd->y >= root_h)) continue;
        if (((temp_bd->x + temp_bd->w) <= 0) || ((temp_bd->y + temp_bd->h) <= 0)) continue;

        if ((!temp_bd->iconic) && (temp_bd->visible) && (temp_bd->desk == bd->desk) &&
            (temp_bd->client.icccm.accepts_focus || temp_bd->client.icccm.take_focus) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
          {
             if (!temp_bd->focused)
               {
                  /* this border is the top of the latest stack */
                  e_border_focus_set (temp_bd, 1, 1);
               }
             break;
          }
     }
   e_container_border_list_free(bl);
}


static void _policy_border_update_notification_stack (E_Border* bd, int level, Eina_Bool check_focus)
{
   E_Illume_Border_Info* bd_info = NULL;

   if (!bd) return;

   bd_info = _policy_get_border_info(bd);
   if (bd_info == NULL) return;

   L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Update Stack!!! Win (%x). old level = %d  new level = %d\n", __func__, __LINE__, bd->client.win, bd_info->level, level);
   bd_info->level = level;

   /*
      1. find first high notification win
      2. place new win on the found win
    */
   _update_notification_stack (bd_info, level);

   /* for notification level stack - consider transient_for window */
   E_Border *child;
   Eina_List *l;

   if (e_config->transient.raise && e_config->transient.lower)
     {
        EINA_LIST_REVERSE_FOREACH(bd->transients, l, child)
          {
             if (!child) continue;

             /* Don't stack iconic transients. If the user wants these shown,
              * thats another option.
              */
             if ((!child->iconic) && (child->visible))
               {
                  L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, child->client.win, level);
                  _policy_border_update_notification_stack (child, level, EINA_FALSE);
               }
          }
     }

   if (check_focus)
     {
        if (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
           _policy_border_focus_top_stack_set(bd);
     }
}


static E_Illume_Border_Info* _policy_border_find_next_notification_border_info (E_Illume_Border_Info* bd_info)
{
   Eina_List *l;
   E_Illume_Border_Info *temp_bd_info;

   if (!bd_info) return NULL;

   EINA_LIST_FOREACH(e_border_info_list, l, temp_bd_info)
     {
        if (!temp_bd_info) continue;

        if (temp_bd_info == bd_info)
          {
             l = eina_list_next(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }

        if (temp_bd_info->border->layer != POL_NOTIFICATION_LAYER)
           return NULL;
     }

   return NULL;
}


void _policy_border_stack (E_Event_Border_Stack *event)
{
   E_Event_Border_Stack* ev;
   E_Illume_Border_Info* bd_info;
   E_Illume_Border_Info* stack_bd_info;

   ev = event;

   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... bd(win:%x), stack(win:%x), stack type: %d\n", __func__, __LINE__, ev->border->client.win, ev->stack ? (unsigned int)ev->stack->client.win:(unsigned int)NULL, ev->type);

   bd_info = _policy_get_border_info(ev->border);
   if (bd_info == NULL)
     {
        bd_info = _policy_add_border_info_list (ev->border);
        if (!bd_info) return;
     }

   if (ev->stack)
     {
        stack_bd_info = _policy_get_border_info(ev->stack);
     }
   else
     {
        stack_bd_info = NULL;
     }

   if (ev->type == E_STACKING_ABOVE)
     {
        if (ev->stack)
          {
             if (stack_bd_info)
               {
                  e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
                  e_border_info_list = eina_list_prepend_relative (e_border_info_list, bd_info, stack_bd_info);
                  L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:%x) -> stack(win:%x) ]\n", __func__, __LINE__, ev->border->client.win, ev->stack->client.win);

                  if (ev->border->layer == POL_NOTIFICATION_LAYER)
                    {
                       E_Illume_Border_Info* next_bd_info;

                       next_bd_info = _policy_border_find_next_notification_border_info (bd_info);
                       if (next_bd_info)
                         {
                            if (bd_info->level < next_bd_info->level)
                              {
                                 L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... RESTACK... bd(win:%x) level:%d.. stack(win:%x) level:%d..\n", __func__, __LINE__, ev->border->client.win, bd_info->level, ev->stack->client.win, stack_bd_info->level);
                                 _policy_border_stack_change (bd_info->border, next_bd_info->border, E_ILLUME_STACK_BELOW);
                              }
                         }
                    }
               }
             else
               {
                  // could find bd_info of ev->stack.. there is no bd_info yet...
                  Eina_List *l = NULL;
                  E_Illume_Border_Info *temp_bd_info;

                  EINA_LIST_FOREACH(e_border_info_list, l, temp_bd_info)
                    {
                       if (!temp_bd_info) continue;
                       if (bd_info->border->layer >= temp_bd_info->border->layer)
                          break;
                    }

                  if (bd_info != temp_bd_info)
                    {
                       e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
                       e_border_info_list = eina_list_prepend_relative(e_border_info_list, bd_info, temp_bd_info);
                       L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:%x) -> other(win:%x).. No stack(win:%x) info ]\n", __func__, __LINE__, ev->border->client.win, temp_bd_info ? (unsigned int)temp_bd_info->border->client.win:(unsigned int)NULL, ev->stack->client.win);
                    }
               }
          }
        else
          {
             e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
             e_border_info_list = eina_list_append (e_border_info_list, bd_info);
             L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ ---> bd(win:%x) ]\n", __func__, __LINE__, ev->border->client.win);
          }
     }
   else if (ev->type == E_STACKING_BELOW)
     {
        if (ev->stack)
          {
             if (stack_bd_info)
               {
                  e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
                  e_border_info_list = eina_list_append_relative (e_border_info_list, bd_info, stack_bd_info);
                  L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ stack(win:%x) -> bd(win:%x) ]\n", __func__, __LINE__, ev->stack->client.win, ev->border->client.win);

                  if (ev->border->layer == POL_NOTIFICATION_LAYER)
                    {
                       E_Illume_Border_Info* next_bd_info;

                       next_bd_info = _policy_border_find_next_notification_border_info (bd_info);
                       if (next_bd_info)
                         {
                            if (bd_info->level < next_bd_info->level)
                              {
                                 L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... RESTACK... bd(win:%x) level:%d.. stack(win:%x) level:%d..\n", __func__, __LINE__, ev->border->client.win, bd_info->level, ev->stack->client.win, stack_bd_info->level);
                                 _policy_border_stack_change (bd_info->border, next_bd_info->border, E_ILLUME_STACK_BELOW);
                              }
                         }
                    }
               }
             else
               {
                  // could find bd_info of ev->stack.. there is no bd_info yet...
                  Eina_List *l = NULL;
                  E_Illume_Border_Info *temp_bd_info;

                  EINA_LIST_FOREACH(e_border_info_list, l, temp_bd_info)
                    {
                       if (!temp_bd_info) continue;
                       if (bd_info->border->layer >= temp_bd_info->border->layer)
                          break;
                    }

                  if (bd_info != temp_bd_info)
                    {
                       e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
                       e_border_info_list = eina_list_prepend_relative(e_border_info_list, bd_info, temp_bd_info);
                       L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:%x) -> other(win:%x).. No stack(win:%x) info ]\n", __func__, __LINE__, ev->border->client.win, temp_bd_info ? (unsigned int)temp_bd_info->border->client.win:(unsigned int)NULL, ev->stack->client.win);
                    }
               }
          }
        else
          {
             e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
             e_border_info_list = eina_list_prepend (e_border_info_list, bd_info);
             L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:%x) ---> ]\n", __func__, __LINE__, ev->border->client.win);
          }
     }
   else
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... Unknown type... border (%x), win (%x), type = %d\n", __func__, __LINE__, ev->border, ev->border->client.win, ev->type);
     }

   /*restack indicator when a active window stack is changed */
   if (ev->border->client.win == g_active_win)
     {
        E_Border* indi_bd;
        indi_bd = e_illume_border_indicator_get (ev->border->zone);
        if (indi_bd)
          {
             E_Illume_Border_Info* indi_bd_info;
             indi_bd_info = _policy_get_border_info(indi_bd);
             if (indi_bd_info)
               {
                  if (indi_bd->layer == POL_NOTIFICATION_LAYER)
                    {
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:%x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, indi_bd->client.win, indi_bd_info->level);
                       _policy_change_indicator_layer (indi_bd, indi_bd->layer, indi_bd_info->level);
                    }
               }
          }
     }

   ev->border->changes.pos = 1;
   ev->border->changed = 1;

   return;
}


static void _policy_change_quickpanel_layer (E_Illume_Quickpanel* qp, E_Border* indi_bd, int layer, int level)
{
   Eina_List *bd_list;
   E_Illume_Quickpanel_Info *panel;
   E_Illume_Border_Info* bd_info;

   if (!qp) return;

   EINA_LIST_FOREACH(qp->borders, bd_list, panel)
     {
        if (!panel) continue;
        if (e_object_is_del(E_OBJECT(panel->bd))) continue;

        bd_info = _policy_get_border_info(panel->bd);
        if (bd_info)
          {
             bd_info->level = level;
             e_border_stack_below (panel->bd, indi_bd);
          }
     }

   EINA_LIST_FOREACH(qp->hidden_mini_controllers, bd_list, panel)
     {
        if (!panel) continue;
        if (e_object_is_del(E_OBJECT(panel->bd))) continue;

        bd_info = _policy_get_border_info(panel->bd);
        if (bd_info)
          {
             bd_info->level = level;
             e_border_stack_below (panel->bd, indi_bd);
          }
     }

}

/* Recommend to use this function when targer border is viewable, activate and show indicator */
static void _policy_change_indicator_layer (E_Border* indi_bd, int layer, int level)
{
   // the indicator's layer is changed to layer with level
   E_Illume_Border_Info* indi_bd_info;
   E_Illume_Quickpanel* qp;

   indi_bd_info = _policy_get_border_info(indi_bd);
   if (indi_bd_info)
     {
        indi_bd_info->level = level;
     }

   if (indi_bd->layer != layer)
      e_border_layer_set(indi_bd, layer);

   L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (%x) is notification window... level = %d\n", __func__, __LINE__, indi_bd->client.win, level);
   _policy_border_update_notification_stack (indi_bd, level, EINA_FALSE);

   if ((qp = e_illume_quickpanel_by_zone_get(indi_bd->zone)))
     {
        _policy_change_quickpanel_layer (qp, indi_bd, layer, level);
     }
}


/* for visibility */
static void
_policy_send_visibility_notify (Ecore_X_Window win, int visibility)
{
   XEvent event;

   event.type = VisibilityNotify;
   event.xvisibility.display = ecore_x_display_get();
   event.xvisibility.send_event = EINA_TRUE;
   event.xvisibility.state = visibility;
   event.xvisibility.window = win;

   XSendEvent(event.xvisibility.display
      , event.xvisibility.window
      , False
      , VisibilityChangeMask, &event);
}

static void
_policy_calculate_visibility (Ecore_X_Window win)
{
   // 1. calculates window's region until the windos which is placed on the target window
   // 2. set and send state of the target window's visiblity
   // 3. calculate a visibility of window which is placed under the target window
   //     - when a fully obscured window is found. then all rest windows are fully obscured

   Ecore_X_XRegion* region;
   Ecore_X_XRegion* temp_region;
   Ecore_X_Rectangle rect, temp_rect;
   Eina_Inlist* xwin_info_list;
   E_Illume_XWin_Info* xwin_info;
   Eina_Bool placed;
   int is_fully_obscured;
   int old_vis;
   int is_opaque;

   xwin_info = NULL;
   is_opaque = 0;
   rect.x = 0;
   rect.y = 0;
   rect.width = _g_root_width;
   rect.height = _g_root_height;

   L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] Change win:%x... root_width:%d, root_height:%d\n", win, _g_root_width, _g_root_height);

   // create screen size's region
   region = ecore_x_xregion_new();
   ecore_x_xregion_union_rect (region, region, &rect);

   xwin_info_list = _e_illume_xwin_info_list;

   //
   // 1. calculates window's region until the windos which is placed on the target window
   //
   L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] PASS-----1\n");
   if (xwin_info_list)
     {
        EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
          {
             L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] xwin_info(win:%x, (%d, %d, %d x %d)\n", xwin_info->id, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.w, xwin_info->attr.h);
             is_opaque = 0;
             if (xwin_info->id == win) break;

             // skip unmapped window
             if (xwin_info->attr.visible == 0) continue;
             if (xwin_info->bd_info)
               {
                  // skip unvisible border
                  if (xwin_info->bd_info->border && !xwin_info->bd_info->border->visible) continue;

                  // check opaque
                  is_opaque = xwin_info->bd_info->opaque;
               }

             if ((!xwin_info->argb) ||
                 (is_opaque))
               {
                  temp_rect.x = xwin_info->attr.x;
                  temp_rect.y = xwin_info->attr.y;
                  temp_rect.width = xwin_info->attr.w;
                  temp_rect.height = xwin_info->attr.h;

                  temp_region = ecore_x_xregion_new();
                  ecore_x_xregion_union_rect (temp_region, temp_region, &temp_rect);
                  ecore_x_xregion_subtract (region, region, temp_region);
                  ecore_x_xregion_free (temp_region);
               }
          }
     }

   if (xwin_info == NULL)
     {
        L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] BAD.... current border is not in the raise_Stack...\n");
        ecore_x_xregion_free (region);
        return;
     }

   is_fully_obscured = 0;

   //
   // 2. set and send state of the target window's visiblity
   // 3. calculate a visibility of window which is placed under the target window
   //     - when a fully obscured window is found. then all rest windows are fully obscured
   //
   L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] PASS-----2,3\n");
   if (xwin_info_list)
     {
        for (; xwin_info; xwin_info = (EINA_INLIST_GET(xwin_info)->prev ? _EINA_INLIST_CONTAINER(xwin_info, EINA_INLIST_GET(xwin_info)->prev) : NULL))
          {
             is_opaque = 0;
             old_vis = xwin_info->visibility;

             // skip unmapped window
             if (xwin_info->attr.visible == 0) continue;
             if (xwin_info->bd_info)
               {
                  // skip unvisible border
                  if (xwin_info->bd_info->border && !xwin_info->bd_info->border->visible) continue;

                  // check opaque
                  is_opaque = xwin_info->bd_info->opaque;
               }

             if (is_fully_obscured)
               {
                  L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] Win:%x is FULLY OBSCURED...\n", xwin_info->id);

                  /* if temp_bd's visibility is not fully obscured, then set and send event */
                  xwin_info->visibility = E_ILLUME_VISIBILITY_FULLY_OBSCURED;

                  if (old_vis  != xwin_info->visibility)
                    {
                       // send event
                       if (xwin_info->bd_info && xwin_info->bd_info->border)
                         {
                            L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] SEND VISIBILITY NOTIFY (Line:%d)... win:%x (old:%d -> new:%d)\n", __LINE__, xwin_info->bd_info->border->client.win, old_vis, xwin_info->visibility);
                            _policy_send_visibility_notify (xwin_info->bd_info->border->client.win, xwin_info->visibility);
                         }
                    }

                  continue;
               }

             temp_rect.x = xwin_info->attr.x;
             temp_rect.y = xwin_info->attr.y;
             temp_rect.width = xwin_info->attr.w;
             temp_rect.height = xwin_info->attr.h;

             L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] xwin_info(win:%x, (%d, %d, %d x %d)\n", xwin_info->id, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.w, xwin_info->attr.h);
             placed = ecore_x_xregion_rect_contain (region, &temp_rect);
             if (placed == EINA_TRUE)
               {
                  // Unobscured or partially obscured
                  L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] Win:%x is UN-OBSCURED...\n", xwin_info->id);
                  xwin_info->visibility = E_ILLUME_VISIBILITY_UNOBSCURED;
               }
             else
               {
                  // Fully obscured
                  L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] Win:%x is Fully OBSCURED...\n", xwin_info->id);
                  xwin_info->visibility = E_ILLUME_VISIBILITY_FULLY_OBSCURED;
               }

             if (old_vis  != xwin_info->visibility)
               {
                  // send event
                  if (xwin_info->bd_info && xwin_info->bd_info->border)
                    {
                       L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] SEND VISIBILITY NOTIFY (Line:%d)... win:%x (old:%d -> new:%d)\n", __LINE__, xwin_info->bd_info->border->client.win, old_vis, xwin_info->visibility);
                       _policy_send_visibility_notify (xwin_info->bd_info->border->client.win, xwin_info->visibility);

                       if (xwin_info->visibility == E_ILLUME_VISIBILITY_UNOBSCURED)
                         {
                            L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. CALL _policy_change_root_angle_by_border_angle!!! win:%x\n", __func__, __LINE__, xwin_info->bd_info->border->client.win);
                            _policy_change_root_angle_by_border_angle (xwin_info->bd_info->border);
                         }
                    }
               }

             if ((!xwin_info->argb) ||
                 (is_opaque))
               {
                  temp_region = ecore_x_xregion_new();
                  ecore_x_xregion_union_rect (temp_region, temp_region, &temp_rect);
                  ecore_x_xregion_subtract (region, region, temp_region);
                  ecore_x_xregion_free (temp_region);

                  if (ecore_x_xregion_is_empty (region) == EINA_TRUE)
                    {
                       L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] region is empty... Rest Windows are FULLY OBSCURED...\n");
                       is_fully_obscured = 1;
                    }
               }
          }
     }

   ecore_x_xregion_free (region);
}


static E_Illume_XWin_Info*
_policy_xwin_info_find (Ecore_X_Window win)
{
   return eina_hash_find(_e_illume_xwin_info_hash, e_util_winid_str_get(win));
}


static void
_policy_manage_xwins (E_Manager* man)
{
   Ecore_X_Window *windows;
   int wnum;
   int i;

   windows = ecore_x_window_children_get(man->root, &wnum);
   if (windows)
     {
        for (i = 0; i < wnum; i++)
           _policy_xwin_info_add(windows[i]);

        free(windows);
     }

   ecore_x_window_size_get (man->root, &_g_root_width, &_g_root_height);
}


static Eina_Bool
_policy_xwin_info_add (Ecore_X_Window win)
{
   E_Border* bd;

   if (win == _e_overlay_win) return EINA_FALSE;

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find (win);
   if (xwin_info)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x EXIST in the list...\n", __func__, __LINE__, win);
        return EINA_FALSE;
     }

   xwin_info = (E_Illume_XWin_Info*) malloc (sizeof (E_Illume_XWin_Info));
   if (!xwin_info)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] (%s:%d).. Critical Error... Fail to create memory... \n", __func__, __LINE__);
        return EINA_FALSE;
     }

   xwin_info->id = win;
   xwin_info->visibility = E_ILLUME_VISIBILITY_FULLY_OBSCURED;

   if (!ecore_x_window_attributes_get(win, &xwin_info->attr))
     {
        free (xwin_info);
        return EINA_FALSE;
     }

   bd = e_border_find_by_window (win);
   xwin_info->bd_info = _policy_get_border_info(bd);
   xwin_info->argb = ecore_x_window_argb_get (win);

   eina_hash_add(_e_illume_xwin_info_hash, e_util_winid_str_get(xwin_info->id), xwin_info);
   _e_illume_xwin_info_list = eina_inlist_append(_e_illume_xwin_info_list, EINA_INLIST_GET(xwin_info));

   return EINA_TRUE;
}


static Eina_Bool
_policy_xwin_info_delete (Ecore_X_Window win)
{
   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find (win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:%x in the list...\n", __func__, __LINE__, win);
        return EINA_FALSE;
     }

   _e_illume_xwin_info_list = eina_inlist_remove(_e_illume_xwin_info_list, EINA_INLIST_GET(xwin_info));
   eina_hash_del(_e_illume_xwin_info_hash, e_util_winid_str_get(xwin_info->id), xwin_info);

   free (xwin_info);

   return EINA_TRUE;
}


void _policy_window_create (Ecore_X_Event_Window_Create *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x...\n", __func__, __LINE__, event->win);

   _policy_xwin_info_add (event->win);
}


void _policy_window_destroy (Ecore_X_Event_Window_Destroy *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x...\n", __func__, __LINE__, event->win);

   _policy_xwin_info_delete (event->win);
}


void _policy_window_reparent (Ecore_X_Event_Window_Reparent *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x...\n", __func__, __LINE__, event->win);

   if (event->parent == ecore_x_window_root_first_get())
      _policy_xwin_info_add (event->win);
   else
      _policy_xwin_info_delete (event->win);
}


void _policy_window_show (Ecore_X_Event_Window_Show *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x...\n", __func__, __LINE__, event->win);

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find (event->win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:%x in the list...\n", __func__, __LINE__, event->win);
        return ;
     }

   xwin_info->attr.visible = 1;

   _policy_calculate_visibility (event->win);
}


void _policy_window_hide (Ecore_X_Event_Window_Hide *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x...\n", __func__, __LINE__, event->win);

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find (event->win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:%x in the list...\n", __func__, __LINE__, event->win);
        return;
     }

   xwin_info->attr.visible = 0;

   _policy_calculate_visibility (event->win);
}


void _policy_window_configure (Ecore_X_Event_Window_Configure *event)
{
   Eina_Inlist* l;
   E_Illume_XWin_Info* xwin_info;
   E_Illume_XWin_Info* old_above_xwin_info;
   E_Illume_XWin_Info* new_above_xwin_info;
   E_Illume_XWin_Info* temp_xwin_info;
   int check_visibility;
   Ecore_X_Window target_win;

   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:%x...\n", __func__, __LINE__, event->win);

   xwin_info = NULL;
   old_above_xwin_info = NULL;
   new_above_xwin_info = NULL;
   check_visibility = 0;
   target_win = event->win;

   xwin_info = _policy_xwin_info_find (event->win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:%x in the list...\n", __func__, __LINE__, event->win);
        return;
     }

   if ((xwin_info->attr.x != event->x) ||
       (xwin_info->attr.y != event->y) ||
       (xwin_info->attr.w != event->w) ||
       (xwin_info->attr.h != event->h))
      check_visibility = 1;

   xwin_info->attr.x = event->x;
   xwin_info->attr.y = event->y;
   xwin_info->attr.w = event->w;
   xwin_info->attr.h = event->h;

   if ((l = EINA_INLIST_GET(xwin_info)->prev) != NULL)
     {
        old_above_xwin_info = EINA_INLIST_CONTAINER_GET (l, E_Illume_XWin_Info);
     }

   new_above_xwin_info = _policy_xwin_info_find (event->abovewin);

   if (old_above_xwin_info != new_above_xwin_info)
     {
        // find target win
        if (old_above_xwin_info)
          {
             temp_xwin_info = old_above_xwin_info;
             for (; temp_xwin_info; temp_xwin_info = (EINA_INLIST_GET(temp_xwin_info)->prev ? _EINA_INLIST_CONTAINER(temp_xwin_info, EINA_INLIST_GET(temp_xwin_info)->prev) : NULL))
               {
                  if (temp_xwin_info == new_above_xwin_info)
                    {
                       target_win = old_above_xwin_info->id;
                       break;
                    }
               }
          }
        check_visibility = 1;
     }

   _e_illume_xwin_info_list = eina_inlist_remove (_e_illume_xwin_info_list, EINA_INLIST_GET(xwin_info));
   if (new_above_xwin_info)
      _e_illume_xwin_info_list = eina_inlist_append_relative (_e_illume_xwin_info_list, EINA_INLIST_GET(xwin_info), EINA_INLIST_GET(new_above_xwin_info));
   else
      _e_illume_xwin_info_list = eina_inlist_prepend (_e_illume_xwin_info_list, EINA_INLIST_GET(xwin_info));

   if (check_visibility == 1)
      _policy_calculate_visibility (target_win);
}


void _policy_window_configure_request (Ecore_X_Event_Window_Configure_Request *event)
{
   E_Border *bd;
   Ecore_X_Event_Window_Configure_Request *e;

   e = event;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return;

   if (!bd->lock_client_stacking)
     {
        if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE) &&
            (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING))
          {
             if (bd->layer == POL_NOTIFICATION_LAYER)
               {
                  E_Illume_Border_Info* bd_info;
                  bd_info = _policy_get_border_info(bd);
                  if (bd_info)
                    {
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Win (%x) is notification layer... level = %d\n", __func__, __LINE__, bd->client.win, bd_info->level);
                       _policy_border_update_notification_stack (bd, bd_info->level, EINA_TRUE);
                    }
               }

             if (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
               {
                  if (bd->visible && (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus))
                     _policy_border_focus_top_stack_set (bd);
               }
          }
        else if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE)
          {
             if (bd->layer == POL_NOTIFICATION_LAYER)
               {
                  E_Illume_Border_Info* bd_info;
                  bd_info = _policy_get_border_info(bd);
                  if (bd_info)
                    {
                       L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Win (%x) is notification layer... level = %d\n", __func__, __LINE__, bd->client.win, bd_info->level);
                       _policy_border_update_notification_stack (bd, bd_info->level, EINA_TRUE);
                    }
               }

             if (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
               {
                  if (bd->visible && (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus))
                     _policy_border_focus_top_stack_set(bd);
               }
          }
     }
}


#if ILLUME_LOGGER_BUILD_ENABLE
static Eina_Bool
_policy_property_illume_log_change (Ecore_X_Event_Window_Property *ev)
{
   int ret, count;
   unsigned int info_type;
   unsigned char* prop_data = NULL;

   info_type = 0;
   ret = ecore_x_window_prop_property_get (ev->win, E_ILLUME_ATOM_ILLUME_LOG, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
     {
        fprintf(stdout, "[E17-ILLUME2] %s(%d) _illume_logger_type: ",  __func__, __LINE__);

        if (_illume_logger_type == LT_NOTHING) fprintf (stdout, "LT_NOTHING ");
        else if (_illume_logger_type == LT_ALL) fprintf (stdout, "LT_ALL "    );
        else
          {
             if ((_illume_logger_type & LT_STACK) == LT_STACK) fprintf (stdout, "LT_STACK, ");
             if ((_illume_logger_type & LT_NOTIFICATION) == LT_NOTIFICATION) fprintf (stdout, "LT_NOTIFICATION, ");
             if ((_illume_logger_type & LT_NOTIFICATION_LEVEL) == LT_NOTIFICATION_LEVEL) fprintf (stdout, "LT_NOTIFICATION_LEVEL, ");
             if ((_illume_logger_type & LT_VISIBILITY) == LT_VISIBILITY) fprintf (stdout, "LT_VISIBILITY, ");
             if ((_illume_logger_type & LT_LOCK_SCREEN) == LT_LOCK_SCREEN) fprintf (stdout, "LT_LOCK_SCREEN, ");
             if ((_illume_logger_type & LT_ANGLE) == LT_ANGLE) fprintf (stdout, "LT_ANGLE, ");
          }

        fprintf(stdout, "-> ");

        memcpy (&_illume_logger_type, prop_data, sizeof(unsigned int));

        if      (_illume_logger_type == LT_NOTHING ) fprintf (stdout, "LT_NOTHING\n");
        else if (_illume_logger_type == LT_ALL     ) fprintf (stdout, "LT_ALL\n"    );
        else
          {
             if ((_illume_logger_type & LT_STACK) == LT_STACK) fprintf (stdout, "LT_STACK, ");
             if ((_illume_logger_type & LT_NOTIFICATION) == LT_NOTIFICATION) fprintf (stdout, "LT_NOTIFICATION, ");
             if ((_illume_logger_type & LT_NOTIFICATION_LEVEL) == LT_NOTIFICATION_LEVEL) fprintf (stdout, "LT_NOTIFICATION_LEVEL, ");
             if ((_illume_logger_type & LT_VISIBILITY) == LT_VISIBILITY) fprintf (stdout, "LT_VISIBILITY, ");
             if ((_illume_logger_type & LT_LOCK_SCREEN) == LT_LOCK_SCREEN) fprintf (stdout, "LT_LOCK_SCREEN, ");
             if ((_illume_logger_type & LT_ANGLE) == LT_ANGLE) fprintf (stdout, "LT_ANGLE, ");

             fprintf (stdout, "\n");
          }
     }

   if (prop_data) free (prop_data);

   return EINA_TRUE;
}
#endif /* ILLUME_LOGGER_BUILD_ENABLE */


void _policy_window_sync_draw_done (Ecore_X_Event_Client_Message* event)
{
   E_Border* bd;

   if (g_screen_lock_info->is_lock)
     {
        if (!(bd = e_border_find_by_client_window(event->win))) return;

        _policy_border_remove_block_list (bd);
     }
}


void _policy_quickpanel_state_change (Ecore_X_Event_Client_Message* event)
{
   E_Zone* zone;
   E_Illume_Quickpanel *qp;

   if ((zone = e_util_zone_window_find(event->win)))
     {
        if ((qp = e_illume_quickpanel_by_zone_get(zone)))
          {
             if (event->data.l[0] == (int)ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON)
               {
                  _policy_layout_quickpanel_rotate (qp, g_root_angle);
               }
          }
     }
}


static Eina_Bool _policy_screen_lock_timeout (void *data)
{
   L (LT_LOCK_SCREEN, "[ILLUME2][LOCK]  %s(%d)... LOCK SCREEN TIME-OUT!!!\n", __func__, __LINE__);
   E_Manager* man = (E_Manager*)data;

   if (g_screen_lock_info->blocked_list) eina_list_free (g_screen_lock_info->blocked_list);
   g_screen_lock_info->blocked_list = NULL;

   if (g_screen_lock_info->is_lock == 1)
     {
        _policy_request_screen_unlock (man);

        g_screen_lock_info->man = NULL;
        g_screen_lock_info->is_lock = 0;
     }

   return ECORE_CALLBACK_CANCEL;
}


static void _policy_request_screen_lock (E_Manager* man)
{
   L (LT_LOCK_SCREEN, "[ILLUME2][LOCK]  %s(%d)... LOCK SCREEN!!! manager:%p\n", __func__, __LINE__, man);
   if (!man) return;

   if (!g_screen_lock_info->lock_timer)
     {
        g_screen_lock_info->lock_timer = ecore_timer_add (SCREEN_LOCK_TIMEOUT,
                                                          _policy_screen_lock_timeout,
                                                          man);
     }

   if (man->comp && man->comp->func.screen_lock)
      man->comp->func.screen_lock (man->comp->data, man);
}


static void _policy_request_screen_unlock (E_Manager* man)
{
   L (LT_LOCK_SCREEN, "[ILLUME2][LOCK]  %s(%d)... UN-LOCK SCREEN!!! manager:%p\n", __func__, __LINE__, man);
   int is_lock;

   if (g_screen_lock_info->lock_timer)
     {
        ecore_timer_del (g_screen_lock_info->lock_timer);
        g_screen_lock_info->lock_timer = NULL;
     }

   is_lock = 0;
   ecore_x_client_message32_send (man->root, E_ILLUME_ATOM_SCREEN_LOCK, ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, is_lock, 0, 0, 0, 0);

   /* Above code could be replaced with following code later
      if (!man) return;

      if (man->comp && man->comp->func.screen_unlock)
      man->comp->func.screen_unlock (man->comp->data, man);
    */

}


static void  _policy_border_add_block_list (E_Border* bd)
{
   if (!bd) return;

   if (g_screen_lock_info->is_lock == 0)
     {
        g_screen_lock_info->is_lock = 1;
        g_screen_lock_info->man = bd->zone->container->manager;

        _policy_request_screen_lock (g_screen_lock_info->man);
     }

   if (bd->zone->container->manager != g_screen_lock_info->man) return;

   L (LT_LOCK_SCREEN, "[ILLUME2][LOCK]  %s(%d)... ADD to the block list. Win:%x\n", __func__, __LINE__, bd->client.win);
   g_screen_lock_info->blocked_list = eina_list_append (g_screen_lock_info->blocked_list, bd);

}


static void  _policy_border_remove_block_list (E_Border* bd)
{
   if (!bd) return;

   if (bd->zone->container->manager != g_screen_lock_info->man) return;

   L (LT_LOCK_SCREEN, "[ILLUME2][LOCK]  %s(%d)... REMOVE from the block list. Win:%x\n", __func__, __LINE__, bd->client.win);
   g_screen_lock_info->blocked_list = eina_list_remove (g_screen_lock_info->blocked_list, bd);

   if (g_screen_lock_info->blocked_list == NULL)
     {
        _policy_request_screen_unlock (g_screen_lock_info->man);

        g_screen_lock_info->man = NULL;
        g_screen_lock_info->is_lock = 0;
     }
}

static E_Illume_XWin_Info*
_policy_find_next_visible_window (E_Illume_XWin_Info* xwin_info)
{
   E_Illume_XWin_Info* next_xwin_info = NULL;
   E_Border* temp_bd;
   int root_w, root_h;

   root_w = xwin_info->bd_info->border->zone->w;
   root_h = xwin_info->bd_info->border->zone->h;

   next_xwin_info = EINA_INLIST_GET(xwin_info)->prev ? _EINA_INLIST_CONTAINER(xwin_info, EINA_INLIST_GET(xwin_info)->prev) : NULL;

   for (; next_xwin_info; next_xwin_info = (EINA_INLIST_GET(next_xwin_info)->prev ? _EINA_INLIST_CONTAINER(next_xwin_info, EINA_INLIST_GET(next_xwin_info)->prev) : NULL))
     {
        // skip non-border window
        if (!next_xwin_info->bd_info) continue;
        if (!next_xwin_info->bd_info->border) continue;

        temp_bd = next_xwin_info->bd_info->border;

        if ((temp_bd->x >= root_w) || (temp_bd->y >= root_h)) continue;
        if (((temp_bd->x + temp_bd->w) <= 0) || ((temp_bd->y + temp_bd->h) <= 0)) continue;
        if ((temp_bd->iconic) || (!temp_bd->visible)) continue;

        if (e_illume_border_is_indicator(temp_bd)) continue;
        if (e_illume_border_is_keyboard(temp_bd)) continue;
        if (e_illume_border_is_quickpanel(temp_bd)) continue;

        break;
     }

   return next_xwin_info;
}

static void
_policy_root_angle_set (Ecore_X_Window root, Ecore_X_Window win)
{
   int angle;

   if(win)
     {
        angle = _policy_window_rotation_angle_get(win);
        if(angle == -1) return;
     }
   else
      angle = g_root_angle;
   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. old_win:%x -> new_win:%x,  old angle:%d -> new_angle:%d\n", __func__, __LINE__, g_rotated_win, win, g_root_angle, angle);

   g_rotated_win = win;

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. SET ROOT ANGLE... angle:%d\n\n", __func__, __LINE__, angle);
   // set root window property
   ecore_x_window_prop_property_set (root, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &angle, 1);
 }

static void
_policy_change_root_angle_by_border_angle (E_Border* bd)
{
   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... win:%x\n", __func__, __LINE__, bd ? (unsigned int)bd->client.win:(unsigned int)NULL);
   if (!bd) return;

   // ignore the angle of special borders - like indicator, keyboard, quickpanel, etc.
   if (e_illume_border_is_indicator(bd)) return;
   if (e_illume_border_is_keyboard(bd)) return;
   if (e_illume_border_is_quickpanel(bd)) return;

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... CALL _policy_root_angle_set.. win:%x\n", __func__, __LINE__, bd->client.win);
   _policy_root_angle_set (bd->zone->container->manager->root, bd->client.win);
}

static void
_policy_indicator_angle_change (E_Border* indi_bd, int angle)
{
   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... indicator:%x\n", __func__, __LINE__, indi_bd ? (unsigned int)indi_bd->client.win:(unsigned int)NULL);
   if (!indi_bd) return;

   int old_angle = _policy_window_rotation_angle_get (indi_bd->client.win);
   if(old_angle == -1) return;

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... old_angle:%d, new_angle:%d\n", __func__, __LINE__, old_angle, angle);
   if (old_angle != angle)
     {
        int angles[2];
        angles[0] = angle;
        angles[1] = old_angle;
        ecore_x_window_prop_property_set(indi_bd->client.win,
                                         ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                         ECORE_X_ATOM_CARDINAL,
                                         32,
                                         &angles,
                                         2);

        ecore_x_client_message32_send (indi_bd->client.win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                       ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                       angle, 0, 0, 0, 0);

        E_Illume_Quickpanel *qp;
        qp = e_illume_quickpanel_by_zone_get (indi_bd->zone);
        if (qp)
          {
             if (qp->visible)
                _policy_layout_quickpanel_rotate (qp, angle);
          }
     }
}

