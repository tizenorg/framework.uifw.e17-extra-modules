#include "e_illume_private.h"
#include "policy_util.h"
#include "policy.h"

#if 1 // for visibility
#include <X11/Xlib.h>
#endif

#ifdef USE_DLOG
#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17_EXTRA_MODULES"
#endif

/* NB: DIALOG_USES_PIXEL_BORDER is an experiment in setting dialog windows
 * to use the 'pixel' type border. This is done because some dialogs,
 * when shown, blend into other windows too much. Pixel border adds a
 * little distinction between the dialog window and an app window.
 * Disable if this is not wanted */
//#define DIALOG_USES_PIXEL_BORDER 1

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
#define BACKKEY_MODULE_CONTROL
#define DEVMODE_MODULE_CONTROL


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
static void _policy_zone_layout_quickpanel_popup(E_Border *bd);
static void _policy_zone_layout_keyboard(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_fullscreen(E_Border *bd);
static void _policy_zone_layout_dialog(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_splash(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_clipboard(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_apptray(E_Border *bd);

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
static int _policy_notification_level_map(int level);

static void _policy_change_quickpanel_layer (E_Illume_Quickpanel* qp, E_Border* indi_bd, int layer, int level);
static void _policy_change_indicator_layer(E_Border *indi_bd, E_Border *bd, int layer, int level);

/* for property change */
static void _policy_property_window_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_clipboard_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_clipboard_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_geometry_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_enlightenment_scale_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_rotate_win_angle_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_state_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_opacity_change(Ecore_X_Event_Window_Property *event);
static void _policy_property_active_win_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_win_type_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_rotate_root_angle_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_notification_level_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_overlay_win_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_window_opaque_change (Ecore_X_Event_Window_Property *event);
static void _policy_property_illume_window_state_change(Ecore_X_Event_Window_Property *event);
static void _policy_property_indicator_cmd_win_change(Ecore_X_Event_Window_Property *event);
static void _policy_property_active_indicator_win_change(Ecore_X_Event_Window_Property *event);

static void _policy_border_illume_window_state_change(E_Border *bd, unsigned int state);

static Eina_Bool _policy_border_cb_mouse_down(void *data, int type __UNUSED__, void *event);
static Eina_Bool _policy_border_cb_mouse_up(void *data, int type __UNUSED__, void *event);
static Eina_Bool _policy_border_cb_mouse_move(void *data, int type __UNUSED__, void *event);

static int _policy_property_window_opaque_get (Ecore_X_Window win);

static void _policy_border_focus_top_stack_set(E_Border *bd);

/* for debugging */
void _policy_border_list_print (Ecore_X_Window win);

#ifdef COMP_MODULE_CONTROL
static void _policy_property_composite_module_change (Ecore_X_Event_Window_Property *ev);
#endif
#ifdef BACKKEY_MODULE_CONTROL
static void _policy_property_backkey_module_change (Ecore_X_Event_Window_Property *ev);
#endif
#ifdef DEVMODE_MODULE_CONTROL
static void _policy_property_devmode_module_change (Ecore_X_Event_Window_Property *ev);
#endif

#if 1 // for visibility
static void _policy_manage_xwins (E_Manager* man);
static E_Illume_XWin_Info* _policy_xwin_info_find (Ecore_X_Window win);
static Eina_Bool _policy_xwin_info_add (Ecore_X_Window win);
static Eina_Bool _policy_xwin_info_delete (Ecore_X_Window win);

static void _policy_send_visibility_notify (Ecore_X_Window win, int visibility);
static void _policy_calculate_visibility (void);
#endif // visibility

static Eina_Bool _policy_root_angle_set(E_Border *bd);
static void _policy_change_root_angle_by_border_angle (E_Border* bd);
static void _policy_indicator_angle_change (E_Border* indi_bd, int angle);

static void _policy_border_transient_for_group_make(E_Border *bd, Eina_List** list);
static E_Border* _policy_border_transient_for_border_top_get(E_Border *bd);
static void _policy_border_transient_for_layer_set(E_Border *bd, E_Border *parent_bd, int layer);

/* for desktop mode */
static void _policy_zone_layout_app_single_monitor(E_Illume_Border_Info *bd_info, E_Illume_Config_Zone *cz);

/* for controling indicator */
static void _policy_border_indicator_control(E_Border *indi_bd);
static Eina_Bool _policy_border_indicator_state_change(E_Border *indi_bd, E_Border *bd);
static Ecore_X_Window _policy_indicator_cmd_win_get(Ecore_X_Window win);
static Ecore_X_Window _policy_active_indicator_win_get(Ecore_X_Window win);

static void _policy_resize_start(E_Illume_Border_Info *bd_info);
static void _policy_resize_end(E_Illume_Border_Info *bd_info);
static void _policy_border_illume_handlers_add(E_Illume_Border_Info *bd_info);
static void _policy_border_illume_handlers_remove(E_Illume_Border_Info *bd_info);

static void _policy_border_root_angle_control(E_Zone *zone);
static int _policy_border_layer_map(int layer);

/* for visibility */
static void _policy_msg_handler(void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata);

/* for iconify */
static void _policy_border_force_iconify(E_Border *bd);
static void _policy_border_force_uniconify(E_Border *bd);
static void _policy_border_iconify_by_illume(E_Illume_XWin_Info *xwin_info);
static void _policy_border_uniconify_below_borders_by_illume(E_Illume_XWin_Info *xwin_info);
static E_Border* _policy_border_find_below(E_Border *bd);
static void _policy_border_uniconify_below_borders(E_Border *bd);
static void _policy_border_uniconify_top_border(E_Border *bd);

/* for supporting rotation */
static void       _policy_border_dependent_rotation(E_Border *bd, int rotation);
static Eina_Bool _policy_dependent_rotation_check(E_Border *bd, int rotation);
static int _prev_angle_get(Ecore_X_Window win);


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

/* for indicator */
static Ecore_X_Window g_indi_control_win;

 #ifdef COMP_MODULE_CONTROL
static Ecore_X_Atom E_ILLUME_ATOM_COMP_MODULE_ENABLED;
#endif
#ifdef BACKKEY_MODULE_CONTROL
static Ecore_X_Atom E_ILLUME_ATOM_BACKKEY_MODULE_ENABLED;
#endif
#ifdef DEVMODE_MODULE_CONTROL
static Ecore_X_Atom E_ILLUME_ATOM_DEVMODE_MODULE_ENABLED;
#endif

/* for supporting rotation */
static Ecore_X_Atom E_INDICATOR_CMD_WIN;
static Ecore_X_Atom E_ACTIVE_INDICATOR_WIN;

#if 1 // for visibility
static Eina_Hash* _e_illume_xwin_info_hash = NULL;
static Eina_Inlist* _e_illume_xwin_info_list = NULL;
static Ecore_X_Window _e_overlay_win = 0;
static int _g_root_width;
static int _g_root_height;
#endif

/* for visibility */
static E_Msg_Handler *_e_illume_msg_handler = NULL;
static Eina_Bool _e_use_comp = EINA_FALSE;
static Eina_Bool _g_visibility_changed = EINA_FALSE;

/* for supporing rotation */
typedef struct _E_Policy_Rotation_Dependent  E_Policy_Rotation_Dependent;

struct _E_Policy_Rotation_Dependent
{
   Eina_List *list;
   Ecore_X_Window root;

   struct
     {
        E_Border *active_bd;
        Ecore_X_Window cmd_win;
     } refer;

   int ang;
};

static E_Policy_Rotation_Dependent dep_rot =
{
   NULL,
   NULL,
   {NULL, NULL},
   0
};

typedef struct _E_Resizable_Area_Info
{
   int x_dist;
   int y_dist;
   int min_width;
   int min_height;
   int max_width;
   int max_height;
} E_Resizable_Area_Info;

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
   pos = _policy_border_layer_map(bd->layer);

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
             if (e_illume_border_is_keyboard_sub(b)) continue;
             if (e_illume_border_is_quickpanel(b)) continue;
             if (e_illume_border_is_quickpanel_popup(b)) continue;
             if (e_illume_border_is_clipboard(b)) continue;

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
        if (e_illume_border_is_keyboard_sub(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;
        if (e_illume_border_is_quickpanel_popup(bd)) continue;

        /* signal a changed pos here so layout gets updated */
        bd->changes.pos = 1;
        bd->changed = 1;
     }
}

static void
_policy_zone_layout_indicator(E_Border *bd, E_Illume_Config_Zone *cz)
{
   ILLUME2_TRACE ("[ILLUME2] %s (%d) win = 0x%07x\n", __func__, __LINE__, bd->client.win);

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

   if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
     {
        if ((bd->w != bd->zone->w) || (bd->h != bd->zone->h))
          _policy_border_resize(bd, bd->zone->w, bd->zone->h);
     }

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

static void _policy_zone_layout_quickpanel_popup(E_Border *bd)
{
   if (!bd) return;

   // Do something
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
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

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
     resize = EINA_TRUE;

   /* make sure it's in the correct position */
   if ((bd->x != nx) || (bd->y != ny))
     move = EINA_TRUE;

   if (resize && move)
     e_border_move_resize(bd, nx, ny, mw, mh);
   else if (resize)
     _policy_border_resize(bd, mw, mh);
   else if (move)
     _policy_border_move(bd, nx, ny);

   /* set layer if needed */
   if (bd->layer != POL_DIALOG_LAYER) e_border_layer_set(bd, POL_DIALOG_LAYER);
}

static void
_policy_zone_layout_splash(E_Border *bd, E_Illume_Config_Zone *cz)
{
   int mw, mh, nx, ny;
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

   if ((!bd) || (!cz)) return;

   /* no point in adjusting size or position if it's not visible */
   if ((!bd->new_client) && (!bd->visible)) return;

   /* grab minimum size */
   e_illume_border_min_get(bd, &mw, &mh);

   /* make sure it fits in this zone */
   if (mw > bd->zone->w) mw = bd->zone->w;
   if (mh > bd->zone->h) mh = bd->zone->h;

   if (mw < bd->w) mw = bd->w;
   if (mh < bd->h) mh = bd->h;

   nx = (bd->zone->x + ((bd->zone->w - mw) / 2));
   ny = (bd->zone->y + ((bd->zone->h - mh) / 2));

   if ((bd->w != mw) || (bd->h != mh))
     resize = EINA_TRUE;

   if ((bd->x != nx) || (bd->y != ny))
     move = EINA_TRUE;

   if (resize && move)
     e_border_move_resize(bd, nx, ny, mw, mh);
   else if (resize)
     _policy_border_resize(bd, mw, mh);
   else if (move)
     _policy_border_move(bd, nx, ny);
}

static void
_policy_zone_layout_clipboard(E_Border *bd, E_Illume_Config_Zone *cz)
{
   /* no point in adjusting size or position if it's not visible */
   if (!bd->visible) return;

   /* set layer if needed */
   if (bd->layer != POL_CLIPBOARD_LAYER)
      e_border_layer_set(bd, POL_CLIPBOARD_LAYER);
}

static void
_policy_zone_layout_apptray(E_Border *bd)
{
   if (!bd) return;

   if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
     {
        if ((bd->w != bd->zone->w) || (bd->h != bd->zone->h))
          _policy_border_resize(bd, bd->zone->w, bd->zone->h);
     }

   /* set layer if needed */
   if (bd->layer != POL_APPTRAY_LAYER)
      e_border_layer_set(bd, POL_APPTRAY_LAYER);
}

static void
_policy_zone_layout_miniapptray(E_Border *bd)
{
   if (!bd) return;

   if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
     {
        if ((bd->w != bd->zone->w) || (bd->h != bd->zone->h))
          _policy_border_resize(bd, bd->zone->w, bd->zone->h);
     }

   /* set layer if needed */
   if (bd->layer != POL_APPTRAY_LAYER)
      e_border_layer_set(bd, POL_APPTRAY_LAYER);
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

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... bd = 0x%07x\n", __func__, __LINE__, bd->client.win);

   /* ignore stolen borders. These are typically quickpanel or keyboards */
   if (bd->stolen)
     {
        if (e_illume_border_is_quickpanel(bd) ||
            e_illume_border_is_quickpanel_popup(bd))
          {
             E_Border* indi_bd;
             E_Illume_Border_Info* bd_info = NULL;
             E_Illume_Border_Info* indi_bd_info = NULL;

             /* try to get the Indicator on this zone */
             if ((indi_bd = e_illume_border_indicator_get(bd->zone)))
               {
                  if ((indi_bd_info = _policy_get_border_info(indi_bd)))
                    {
                       if ((bd_info = _policy_get_border_info(bd)))
                          bd_info->level = indi_bd_info->level;
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
             ILLUME2_TRACE ("[ILLUME2] ADD INDICATOR WINDOW... win = 0x%07x, Save indicator's size = %d!!!\n", bd->client.win, bd->h);
             cz->indicator.size = bd->h;
          }

        if (_e_illume_cfg->use_indicator_widget)
          {
             L(LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... Control Root Angle.\n", __func__, __LINE__);
             _policy_border_root_angle_control(bd->zone);
          }
        else
          {
             L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
             _policy_border_indicator_control(bd);
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
             if (!indi_bd->visible)
               {
                  L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... Show Indicator (by win: 0x%07x)\n", __func__, __LINE__, bd->client.win);
                  e_illume_border_show(indi_bd);
               }
          }
     }

   if (e_illume_border_is_clipboard(bd))
     {
        ecore_x_e_illume_clipboard_state_set(bd->zone->black_win, ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
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
        if (_e_illume_cfg->use_force_iconify)
          {
             L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... DELETE win:0x%07x\n", __func__, __LINE__, bd->client.win);
             if (!(e_illume_border_is_indicator(bd) ||
                   e_illume_border_is_keyboard(bd) ||
                   e_illume_border_is_keyboard_sub(bd) ||
                   e_illume_border_is_quickpanel(bd) ||
                   e_illume_border_is_quickpanel_popup(bd) ||
                   e_illume_border_is_clipboard(bd) ||
                   e_illume_border_is_app_tray(bd) ||
                   e_illume_border_is_miniapp_tray(bd)))
               {
                  _policy_border_uniconify_below_borders_by_illume(xwin_info);
               }
          }

        if (xwin_info->visibility != E_ILLUME_VISIBILITY_FULLY_OBSCURED)
          {
             if (_e_illume_cfg->use_indicator_widget)
               {
                  if ((bd->w == bd->zone->w) && (bd->h == bd->zone->h))
                    {
                       L(LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... Deleted win:0x%07x. Control Root Angle.\n", __func__, __LINE__, bd->client.win);
                       _policy_border_root_angle_control(bd->zone);
                    }
               }
             else
               {
                  E_Border *indi_bd;
                  indi_bd = e_illume_border_indicator_get(bd->zone);
                  if (indi_bd)
                    {
                       L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
                       _policy_border_indicator_control(indi_bd);
                    }
               }
          }
        xwin_info->bd_info = NULL;
        xwin_info->attr.visible = 0;
        xwin_info->viewable = EINA_FALSE;
        xwin_info->is_drawed = EINA_FALSE;
     }

   E_Illume_Border_Info* bd_info = _policy_get_border_info(bd);
   if (bd_info)
     {
        if (bd_info->resize_req.mouse.down)
          {
             e_grabinput_release(bd->event_win, bd->event_win);

             if (bd_info->resize_req.mouse.resize)
               {
                  Evas_Object *o = (Evas_Object *)e_object_data_get(E_OBJECT(bd));
                  if (o) evas_object_del(o);
               }

             /* set property on zone window that a drag is finished */
             ecore_x_e_illume_drag_set(bd->zone->black_win, 0);

             _policy_border_illume_handlers_remove(bd_info);
          }
     }

   _policy_delete_border_info_list (bd);

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

   // for supporting rotation such as quickpanel
   if (e_illume_border_is_quickpanel(bd) ||
       e_illume_border_is_miniapp_tray(bd) ||
       (bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) ||
       e_illume_border_is_syspopup(bd) ||
       e_illume_border_is_app_selector(bd))
     dep_rot.list = eina_list_remove(dep_rot.list, bd);

   if (dep_rot.refer.active_bd == bd)
     dep_rot.refer.active_bd = NULL;
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
   if (!bd) return;

   /* NB: stolen borders may or may not need focus call...have to test */
   if (bd->stolen) return;

   /* NB: We cannot use our set_focus function here because it does,
    * occasionally fall through wrt E's focus policy, so cherry pick the good
    * parts and use here :) */

   /* if the border is iconified then uniconify if allowed */
   if ((bd->iconic) && (!bd->lock_user_iconify))
      e_border_uniconify(bd);

   ILLUME2_TRACE ("[ILLUME2] _policy_border_activate.. (%d) ACTIVE WIN = 0x%07x\n", __LINE__, bd->client.win);

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
   /* for desktop mode */
   if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
     bd->borderless = 1;
   else if (E_ILLUME_BORDER_IS_IN_DESKTOP(bd))
     {
        if (!bd->client.illume.win_state.state)
          {
             bd->borderless = 0;
          }
     }
#endif

   // for supporting rotation such as quickpanel
   if (e_illume_border_is_quickpanel(bd) ||
       e_illume_border_is_miniapp_tray(bd) ||
       (bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) ||
       e_illume_border_is_syspopup(bd) ||
       e_illume_border_is_app_selector(bd))
     {
        bd->client.e.state.rot.type = E_BORDER_ROTATION_TYPE_DEPENDENT;
        if (!eina_list_data_find(dep_rot.list, bd))
          {
             int ang = _prev_angle_get(bd->client.win);
             int next_ang = 0;

             dep_rot.list = eina_list_append(dep_rot.list, bd);

             if (ang == -1)
               ang = bd->client.e.state.rot.curr;

             if (dep_rot.refer.active_bd) next_ang = dep_rot.refer.active_bd->client.e.state.rot.curr;
             else next_ang = dep_rot.ang;

             if (next_ang != ang)
               e_border_rotation_set(bd, next_ang);
          }
     }

   /* tell E the border has changed */
   bd->client.border.changed = 1;
}


void
_policy_border_post_new_border(E_Border *bd)
{
   int layer = 0;
   E_Illume_Border_Info* bd_info = NULL;

   if (bd->new_client)
     {
        bd_info = _policy_get_border_info(bd);
        if (!bd_info) return;

        bd_info->win_type = bd->client.netwm.type;

        if (e_illume_border_is_notification(bd))
          {
             bd_info->level = _policy_border_get_notification_level(bd->client.win);
             layer = _policy_notification_level_map(bd_info->level);
             e_border_layer_set(bd, layer);
             L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (0x%07x) is notification window... level = %d\n",
                __func__, __LINE__, bd->client.win, bd_info->level);
          }
     }
}

static Eina_Bool
_check_parent_in_transient_for_tree(E_Border *bd, E_Border *parent_bd)
{
   E_Border *ancestor;

   if (!bd || !parent_bd) return EINA_FALSE;

   ancestor = parent_bd;
   while (ancestor->parent)
     {
        if (ancestor->parent == bd)
          {
             // This is very bad. bd and parent_bd are transient_for each other
#ifdef USE_DLOG
             SECURE_SLOGD("[WM] Transient_for Error!!! Win:0x%07x and Parent:0x%07x are transient_for each other.", bd->client.win, parent_bd->client.win);
#endif
             ELBF(ELBT_ILLUME, 0, bd->client.win, "BAD. Transient_for Error. Parent:0x%07x is descendant", parent_bd->client.win);
             return EINA_TRUE;
          }
        ancestor = ancestor->parent;
     }

   return EINA_FALSE;
}

void
_policy_border_pre_fetch(E_Border *bd)
{
#ifdef _F_DEICONIFY_APPROVE_
   Eina_Bool change_parent = EINA_TRUE;
#endif

   if (!bd) return;

   if (bd->new_client)
     {
        unsigned int state;
        state = ecore_x_e_illume_window_state_get(bd->client.win);
        _policy_border_illume_window_state_change(bd, state);
     }

   /* Below code are same to _e_border_eval0 in e_border.c.
      But we added code to handle notification window */
   if (bd->client.icccm.fetch.transient_for)
     {
        /* TODO: What do to if the transient for isn't mapped yet? */
        E_Border *bd_parent = NULL;
        E_Illume_XWin_Info *xwin_info = NULL;
        Eina_Bool transient_each_other;
        Ecore_X_Window transient_for_win;

        if (_e_illume_cfg->use_force_iconify)
          xwin_info = _policy_xwin_info_find(bd->win);

        transient_for_win = ecore_x_icccm_transient_for_get(bd->client.win);
        if (bd->client.icccm.transient_for == transient_for_win)
          {
             ELBF(ELBT_ILLUME, 0, bd->client.win, "Same transient_for:0x%07x. SKIP...", transient_for_win);
             goto transient_fetch_done;
          }

        bd->client.icccm.transient_for = transient_for_win;
        if (bd->client.icccm.transient_for)
          {
             bd_parent = e_border_find_by_client_window(bd->client.icccm.transient_for);
          }

        /* If we already have a parent, remove it */
        if (bd->parent)
          {
#ifdef _F_DEICONIFY_APPROVE_
             if (bd_parent == bd->parent) change_parent = EINA_FALSE;
#endif
             bd->parent->transients = eina_list_remove(bd->parent->transients, bd);
             if (bd->parent->modal == bd) bd->parent->modal = NULL;
             bd->parent = NULL;
          }

        L(LT_TRANSIENT_FOR, "[ILLUME2][TRANSIENT_FOR] %s(%d)... win:0x%07x, parent:0x%07x\n", __func__, __LINE__, bd->client.win, bd->client.icccm.transient_for);
        if (bd_parent)
          {
             transient_each_other = _check_parent_in_transient_for_tree(bd, bd_parent);
             if (!transient_each_other)
               {
                  L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. win:0x%07x(iconic:%d, by_wm:%d), parent:0x%07x(iconic:%d)\n", __func__, __LINE__, bd->client.win, bd->iconic, xwin_info ? xwin_info->iconify_by_wm : -100, bd_parent->client.win, bd_parent->iconic);
                  if (_e_illume_cfg->use_force_iconify)
                    {
                       if (!bd_parent->iconic)
                         {
                            if (xwin_info && xwin_info->iconify_by_wm)
                              {
                                 if (bd->iconic)
                                   {
                                      L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. FORCE UNICONIFY... win:0x%07x\n", __func__, __LINE__, bd->client.win);
                                      _policy_border_force_uniconify(bd);
                                   }
                              }
                         }
                    }

                  if (bd_parent != bd)
                    {
                       bd->parent = bd_parent;
                       _policy_border_transient_for_layer_set(bd, bd->parent, bd->parent->layer);
                       bd_parent->transients = eina_list_append(bd_parent->transients, bd);

                       if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
                         bd->parent->modal = bd;

                       if (e_config->focus_setting == E_FOCUS_NEW_DIALOG ||
                           (bd->parent->focused && (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))
                         bd->take_focus = 1;
                    }
               }
          }

#ifdef _F_DEICONIFY_APPROVE_
        if (change_parent)
          {
             E_Border *ancestor_bd;
             bd->client.e.state.deiconify_approve.render_done = 0;

             ancestor_bd = bd->client.e.state.deiconify_approve.ancestor;
             if ((ancestor_bd) &&
                 (!e_object_is_del(E_OBJECT(ancestor_bd))))
               {
                  ancestor_bd->client.e.state.deiconify_approve.req_list = eina_list_remove(ancestor_bd->client.e.state.deiconify_approve.req_list, bd);
                  bd->client.e.state.deiconify_approve.ancestor = NULL;

                  if ((ancestor_bd->client.e.state.deiconify_approve.req_list == NULL) &&
                      (ancestor_bd->client.e.state.deiconify_approve.render_done))
                    {
                       if (ancestor_bd->client.e.state.deiconify_approve.wait_timer)
                         {
                            ecore_timer_del(ancestor_bd->client.e.state.deiconify_approve.wait_timer);
                            ancestor_bd->client.e.state.deiconify_approve.wait_timer = NULL;
                            e_border_uniconify(ancestor_bd);
                         }
                    }
               }
          }
#endif

transient_fetch_done:
        bd->client.icccm.fetch.transient_for = 0;
     }
}

void
_policy_border_new_border(E_Border *bd)
{
   if (!bd) return;

   if (bd->zone)
     ecore_x_e_illume_zone_set(bd->client.win, bd->zone->black_win);

   _policy_add_border_info_list(bd);
}

#ifdef _F_BORDER_HOOK_PATCH_
void
_policy_border_del_border(E_Border *bd)
{
   if (!bd) return;

   if (bd->new_client)
     {
        _policy_border_del(bd);
     }
}
#endif

void
_policy_border_post_assign(E_Border *bd)
{
   //   printf("Border post assign\n");

   if (!bd) return;
   if (!bd->new_client) return;

   bd->internal_no_remember = 1;

   /* do not allow client to change these properties */
   bd->lock_client_shade = 1;

   if (e_illume_border_is_utility (bd))
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... win(0x%07x) is UTILITY type.. SET REQUEST_POS!!!\n", __func__, __LINE__, bd->client.win);
        bd->client.icccm.request_pos = 1;
     }

   /* do not allow the user to change these properties */
   /* for desktop mode */
   if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
     {
        if (bd->client.illume.win_state.state)
          {
             bd->lock_user_location = 0;
             bd->lock_user_size = 0;
          }
        else
          {
             bd->lock_user_location = 1;
             bd->lock_user_size = 1;
          }
        bd->lock_user_shade = 1;
        if (bd->client.icccm.request_pos == 0)
          {
             bd->placed = 1;
             bd->changes.pos = 1;
          }
     }

   bd->lock_user_shade = 1;

   /* clear any centered states */
   /* NB: this is mainly needed for E's main config dialog */
   bd->client.e.state.centered = 0;

   /* lock the border type so user/client cannot change */
   bd->lock_border = 1;
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
   if (e_illume_border_is_quickpanel_popup(bd)) return;

   if (e_illume_border_is_keyboard(bd))
     {
        int angle;
        angle = _policy_window_rotation_angle_get(bd->client.win);
        if (angle != -1)
          {
             if (angle != g_root_angle)
               {
                  L(LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... win = 0x%07x..  SEND client Event with angle = %d\n", __func__, __LINE__, bd->client.win, g_root_angle);
                  ecore_x_client_message32_send(bd->client.win, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                                                ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, g_root_angle, g_active_win, 0, 0, 0);
               }
          }

        return;
     }

   if (e_illume_border_is_clipboard(bd))
     {
        ecore_x_e_illume_clipboard_state_set(bd->zone->black_win, ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
        ecore_x_e_illume_clipboard_geometry_set(bd->zone->black_win, bd->x, bd->y, bd->w, bd->h);
        return;
     }

   if (eina_list_data_find(dep_rot.list, bd) == bd)
     {
        int next_ang = 0;

        if (dep_rot.refer.active_bd) next_ang = dep_rot.refer.active_bd->client.e.state.rot.curr;
        else next_ang = dep_rot.ang;

        if (bd->client.e.state.rot.curr != next_ang)
          e_border_rotation_set(bd, next_ang);
     }
}

void
_policy_border_cb_move(E_Border *bd)
{
   if (!bd) return;

   if (e_illume_border_is_app_tray(bd) ||
       e_illume_border_is_miniapp_tray(bd))
     {
        _policy_border_focus_top_stack_set(bd);
     }

   return;
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

        else if (e_illume_border_is_quickpanel_popup(bd))
           _policy_zone_layout_quickpanel_popup(bd);

        /* are we laying out a keyboard ? */
        else if (e_illume_border_is_keyboard(bd))
           _policy_zone_layout_keyboard(bd, cz);

        else if (e_illume_border_is_keyboard_sub(bd))
           _policy_zone_layout_keyboard(bd, cz);

        /* are we laying out a fullscreen window ? */
        /* NB: we could use the e_illume_border_is_fullscreen function here
         * but we save ourselves a function call this way. */
        else if ((bd->fullscreen) || (bd->need_fullscreen))
           _policy_zone_layout_fullscreen(bd);

        /* are we laying out a dialog ? */
        else if (e_illume_border_is_dialog(bd))
           _policy_zone_layout_dialog(bd, cz);

        else if (e_illume_border_is_splash(bd))
           _policy_zone_layout_splash(bd, cz);

        else if (e_illume_border_is_clipboard(bd))
           _policy_zone_layout_clipboard(bd, cz);

        else if (e_illume_border_is_app_tray(bd))
           _policy_zone_layout_apptray(bd);

        else if (e_illume_border_is_miniapp_tray(bd))
           _policy_zone_layout_miniapptray(bd);

        /* must be an app */
        else
          {
             /* are we in single mode ? */
             if (!cz->mode.dual)
               {
                  /* for desktop mode */
                  if (E_ILLUME_BORDER_IS_IN_MOBILE(bd))
                    _policy_zone_layout_app_single_new(bd_info, cz);
                  else if (E_ILLUME_BORDER_IS_IN_DESKTOP(bd))
                    _policy_zone_layout_app_single_monitor(bd_info, cz);
                  else
                    _policy_zone_layout_app_single_new(bd_info, cz);
               }
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

   ecore_x_window_size_get (zone->container->manager->root, &_g_root_width, &_g_root_height);

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

   if (!bd->visible) return;

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

static void
_policy_resize_start(E_Illume_Border_Info *bd_info)
{
   E_Manager *m;
   Evas *canvas;
   Evas_Object *o;
   E_Border *bd;
   int nx, ny;
   const char buf[PATH_MAX];

   bd = bd_info->border;

   if (!bd) return;
   if (bd->stolen) return;
   if (!bd->client.illume.win_state.state) return;
   if (!bd_info->resize_req.mouse.down) return;

   bd_info->resize_req.need_change = 0;
   bd_info->resize_req.mouse.locked = 1;
   bd_info->resize_req.mouse.resize = 1;
   int ang = _policy_window_rotation_angle_get(bd->client.win);
   if (ang == -1) ang = 0;
   bd_info->resize_req.angle = ang;

   m = e_manager_current_get();
   if (!m) return;
   canvas = e_manager_comp_evas_get(m);
   if (!canvas) return;

   o = edje_object_add(canvas);
   snprintf(buf, sizeof(buf), "%s/e-module-illume2-tizen.edj", _e_illume_mod_dir);
   evas_object_image_border_center_fill_set(o, EVAS_BORDER_FILL_NONE);
   if(!(edje_object_file_set(o, buf, "new_shadow"))
      || !(bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING))
     {
         o = evas_object_rectangle_add(canvas);                                 
         evas_object_color_set(o, 100, 100, 100, 100);
     }

   nx = bd->x - bd->zone->x;
   ny = bd->y - bd->zone->y;
   evas_object_move(o, nx, ny);
   evas_object_resize(o, bd->w, bd->h);
   e_object_data_set(E_OBJECT(bd), o);
   evas_object_show(o);
}

static void
_policy_resize_end(E_Illume_Border_Info *bd_info)
{
   E_Border *bd;
   bd = bd_info->border;

   if (!bd) return;

   Evas_Object *o = (Evas_Object *)e_object_data_get(E_OBJECT(bd));
   if (o) evas_object_del(o);

   if (bd->stolen) return;
   if (!bd->client.illume.win_state.state) return;
   if (!bd_info->resize_req.mouse.down) return;

   bd_info->resize_req.mouse.locked = 0;
   bd_info->resize_req.mouse.resize = 0;
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
        fprintf( stdout, "[E17-illume2-tizen] %s(%d) enable: %s ", __func__, __LINE__, enable ? "true" : "false" );

        comp = e_module_find ("comp-tizen");
        if( comp )
          {
             current_enabled = e_module_enabled_get(comp);
             fprintf( stdout, "current: %s ", current_enabled ? "true" : "false" );

             if( current_enabled && !enable )
               {
                  fprintf( stdout, "e_module_disable(comp-tizen) " );
                  e_module_disable(comp);
               }
             else if( !current_enabled && enable )
               {
                  fprintf( stdout, "e_module_enable(comp-tizen) " );
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
             fprintf( stderr, "\n[E17-illume2-tizen] %s(%d) can't find comp module.\n", __func__, __LINE__ );
          }
     }

   if (prop_data) free (prop_data);

}
#endif

/* enable/disable backkey module - 130610 seongwon1.cho */
#ifdef BACKKEY_MODULE_CONTROL
static void
_policy_property_backkey_module_change(Ecore_X_Event_Window_Property *ev)
{
   int ret, count;
   int enable = 0;
   int current_enabled = 0;
   unsigned char* prop_data = NULL;
   E_Module* backkey = NULL;

   ret = ecore_x_window_prop_property_get (ev->win, E_ILLUME_ATOM_BACKKEY_MODULE_ENABLED, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
     {
        memcpy (&enable, prop_data, sizeof (int));
        fprintf( stdout, "[E17-illume2-tizen] %s(%d) enable: %s ", __func__, __LINE__, enable ? "true" : "false" );

        backkey = e_module_find ("backkey-tizen");
        if( backkey )
          {
             current_enabled = e_module_enabled_get(backkey);
             fprintf( stdout, "current: %s ", current_enabled ? "true" : "false" );

             if( current_enabled && !enable )
               {
                  fprintf( stdout, "e_module_disable(backkey-tizen) " );
                  e_module_disable(backkey);
               }
             else if( !current_enabled && enable )
               {
                  fprintf( stdout, "e_module_enable(backkey-tizen) " );
                  e_module_enable(backkey);
               }
             else
               {
                  fprintf( stdout, "skip... " );
               }

             fprintf( stdout, "\n" );
          }
        else
          {
             fprintf( stderr, "\n[E17-illume2-tizen] %s(%d) can't find backkey module.\n", __func__, __LINE__ );
             backkey = e_module_new("backkey-tizen");
             if (backkey) e_module_enable(backkey);
          }
     }

   if (prop_data) free (prop_data);

}
#endif


/* enable/disable devmode module - 130610 seongwon1.cho */
#ifdef DEVMODE_MODULE_CONTROL
static void
_policy_property_devmode_module_change(Ecore_X_Event_Window_Property *ev)
{
   int ret, count;
   int enable = 0;
   int current_enabled = 0;
   unsigned char* prop_data = NULL;
   E_Module* devmode = NULL;

   ret = ecore_x_window_prop_property_get (ev->win, E_ILLUME_ATOM_DEVMODE_MODULE_ENABLED, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if( ret && prop_data )
     {
        memcpy (&enable, prop_data, sizeof (int));
        fprintf( stdout, "[E17-illume2-tizen] %s(%d) enable: %s ", __func__, __LINE__, enable ? "true" : "false" );

        devmode = e_module_find ("devmode-tizen");
        if( devmode )
          {
             current_enabled = e_module_enabled_get(devmode);
             fprintf( stdout, "current: %s ", current_enabled ? "true" : "false" );

             if( current_enabled && !enable )
               {
                  fprintf( stdout, "e_module_disable(devmode-tizen) " );
                  e_module_disable(devmode);
               }
             else if( !current_enabled && enable )
               {
                  fprintf( stdout, "e_module_enable(devmode-tizen) " );
                  e_module_enable(devmode);
               }
             else
               {
                  fprintf( stdout, "skip... " );
               }

             fprintf( stdout, "\n" );
          }
        else
          {
             fprintf( stderr, "\n[E17-illume2-tizen] %s(%d) can't find devmode module.\n", __func__, __LINE__ );
             devmode = e_module_new("devmode-tizen");
             if (devmode) e_module_enable(devmode);
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
        if (indi_bd->visible)
          {
             L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... Hide Indicator (by win: 0x%07x)\n", __func__, __LINE__, bd->client.win);
             e_border_hide(indi_bd, 2);
          }
     }
   else
     {
        int indi_show = _policy_border_indicator_state_get(bd);
        if (indi_show == 1)
          {
             if (!indi_bd->visible)
               {
                  L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... Show Indicator (by win: 0x%07x)\n", __func__, __LINE__, bd->client.win);
                  e_border_show(indi_bd);
               }
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

static void _policy_property_clipboard_geometry_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   E_Border *bd;
   int x, y, w, h;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   ecore_x_e_illume_clipboard_geometry_get(zone->black_win, &x, &y, &w, &h);

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (e_illume_border_is_indicator(bd)) continue;
        if (e_illume_border_is_keyboard(bd)) continue;
        if (e_illume_border_is_keyboard_sub(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;
        if (e_illume_border_is_quickpanel_popup(bd)) continue;

#ifdef USE_DLOG
        SECURE_SLOGD("[WM] Clipboard geometry set. win:0x%07x, geo(%d,%d,%d,%d)", bd->client.win, x, y, w, h);
#endif
        ecore_x_e_illume_clipboard_geometry_set(bd->client.win, x, y, w, h);
     }
}

static void _policy_property_clipboard_state_change (Ecore_X_Event_Window_Property *event)
{
   Eina_List *l;
   E_Zone *zone;
   E_Border *bd;
   Ecore_X_Illume_Clipboard_State state;

   /* make sure this property changed on a zone */
   if (!(zone = e_util_zone_window_find(event->win))) return;

   state = ecore_x_e_illume_clipboard_state_get(zone->black_win);

   /* look for conformant borders */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (e_illume_border_is_indicator(bd)) continue;
        if (e_illume_border_is_keyboard(bd)) continue;
        if (e_illume_border_is_keyboard_sub(bd)) continue;
        if (e_illume_border_is_quickpanel(bd)) continue;
        if (e_illume_border_is_quickpanel_popup(bd)) continue;

#ifdef USE_DLOG
        SECURE_SLOGD("[WM] Clipboard state set. win:0x%07x, state:%d", bd->client.win, state);
#endif
        ecore_x_e_illume_clipboard_state_set(bd->client.win, state);
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

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. win:0x%07x\n", __func__, __LINE__, event->win);

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   xwin_info = _policy_xwin_info_find (bd->win);
   if (xwin_info)
     {
        if (xwin_info->visibility == E_ILLUME_VISIBILITY_UNOBSCURED)
          {
             L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. CALL _policy_change_root_angle_by_border_angle!!! win:0x%07x\n", __func__, __LINE__, bd->client.win);
             _policy_change_root_angle_by_border_angle (bd);
          }
     }

   bd->changes.pos = 1;
   bd->changed = 1;
}

static void _policy_property_indicator_state_change (Ecore_X_Event_Window_Property *event)
{
   E_Border *bd, *indi_bd;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   indi_bd = e_illume_border_indicator_get(bd->zone);
   if (!indi_bd) return;

   L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
   _policy_border_indicator_control(indi_bd);
}

static void _policy_property_indicator_opacity_change(Ecore_X_Event_Window_Property *event)
{
   E_Border *bd, *indi_bd;
   Ecore_X_Window active_win;
   Ecore_X_Illume_Indicator_Opacity_Mode mode;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   indi_bd = e_illume_border_indicator_get(bd->zone);
   if (!indi_bd) return;

   active_win = _policy_active_window_get(bd->zone->container->manager->root);
   if (active_win == bd->client.win)
     {
        mode = ecore_x_e_illume_indicator_opacity_get(bd->client.win);
        ecore_x_e_illume_indicator_opacity_send(indi_bd->client.win, mode);
     }
}

static void _policy_active_win_change(E_Illume_XWin_Info *xwin_info, Ecore_X_Window active_win)
{
   E_Border *active_bd = NULL;
   int active_pid;

   if (!xwin_info) return;
   if (!xwin_info->comp_vis) return;

   active_bd = e_border_find_by_client_window(active_win);

   /* for active/deactive message */
   if (active_win != g_active_win)
     {
        if (active_bd)
          active_pid = active_bd->client.netwm.pid;
        else
          active_pid = 0;

        // 1. send deactive event to g_active_win
        ecore_x_client_message32_send(g_active_win, E_ILLUME_ATOM_DEACTIVATE_WINDOW,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, active_win, active_pid, g_active_win, g_active_pid, 0);

        // 2. send active event to active_win
        ecore_x_client_message32_send(active_win, E_ILLUME_ATOM_ACTIVATE_WINDOW,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, active_win, active_pid, g_active_win, g_active_pid, 0);

        // for debug...
        if (active_bd)
          {
#ifdef USE_DLOG
             SECURE_SLOGD("[WM] ACT WIN 0x%07x(%d) -> 0x%07x(%d)", g_active_win, g_active_pid, active_win, active_pid);
#endif
             if ((E_ILLUME_BORDER_IS_IN_DESKTOP(active_bd)) ||
                 (active_bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING))
               {
                  e_border_raise(active_bd);
               }
          }
        else
          {
#ifdef USE_DLOG
             SECURE_SLOGD("[WM] ACT WIN 0x%07x(%d) -> 0x%07x(%d)", g_active_win, g_active_pid, active_win, active_pid);
#endif
          }
        g_active_win = active_win;
        g_active_pid = active_pid;
     }

   if (active_bd)
     {
        if (_e_illume_cfg->use_indicator_widget)
          {
             L(LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... Control Root Angle.\n", __func__, __LINE__);
             _policy_border_root_angle_control(active_bd->zone);
          }
        else
          {
             E_Border *indi_bd;
             indi_bd = e_illume_border_indicator_get(active_bd->zone);
             if (indi_bd)
               {
                  L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, active_bd->client.win);
                  _policy_border_indicator_control(indi_bd);
               }
          }

        ILLUME2_TRACE ("[ILLUME2] ACTIVE WINDOW... (%d) active win = 0x%07x HIDE quickpanel\n", __LINE__, active_win);
        e_illume_quickpanel_hide(active_bd->zone, 0);
     }
}

static void _policy_property_active_win_change (Ecore_X_Event_Window_Property *event)
{
   Ecore_X_Window active_win;
   E_Border* active_bd;
   E_Illume_XWin_Info *xwin_info;

   active_win = _policy_active_window_get(event->win);
   active_bd = e_border_find_by_client_window(active_win);
   if (active_bd)
     {
        xwin_info = _policy_xwin_info_find(active_bd->win);
     }
   else
     {
        xwin_info = _policy_xwin_info_find(active_win);
     }

   _policy_active_win_change(xwin_info, active_win);
}

static void _policy_property_win_type_change (Ecore_X_Event_Window_Property *event)
{
   E_Border *bd;
   E_Border *indi_bd;
   E_Illume_Border_Info* bd_info = NULL;
   int layer = 0;

   if (!(bd = e_border_find_by_client_window(event->win))) return;

   bd_info = _policy_get_border_info(bd);
   if (!bd_info) return;

   e_hints_window_type_get (bd);

   if (bd_info->win_type == bd->client.netwm.type) return;
   bd_info->win_type = bd->client.netwm.type;

   if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NOTIFICATION)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WINDOW TYPE is CHANGED to NOTIFICATION!!!!! win = 0x%07x\n", __func__, __LINE__, bd->client.win);
        bd_info->level = _policy_border_get_notification_level(bd->client.win);
        layer = _policy_notification_level_map(bd_info->level);
        e_border_layer_set(bd, layer);

        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (0x%07x) is notification window... level = %d\n", __func__, __LINE__, bd->client.win, bd_info->level);

        indi_bd = e_illume_border_indicator_get(bd->zone);
        if (indi_bd)
          {
             L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
             _policy_border_indicator_control(indi_bd);
          }

        L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. ICONIFY win:0x%07x And UN-ICONIFY Top win...\n", __func__, __LINE__, bd->client.win);
        _policy_border_uniconify_top_border(bd);
     }
   else if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WINDOW TYPE is CHANGED to NORMAL!!!!! win = 0x%07x\n", __func__, __LINE__, bd->client.win);
        if (bd->layer != POL_APP_LAYER)
          {
             if ( (!e_illume_border_is_quickpanel(bd)) &&
                  (!e_illume_border_is_quickpanel_popup(bd)) &&
                  (!e_illume_border_is_keyboard(bd)) &&
                  (!e_illume_border_is_keyboard_sub(bd)))
               {
                  e_border_layer_set(bd, POL_APP_LAYER);
               }
          }

        indi_bd = e_illume_border_indicator_get(bd->zone);
        if (indi_bd)
          {
             L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
             _policy_border_indicator_control(indi_bd);
          }

        L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. ICONIFY win:0x%07x And UN-ICONIFY Top win...\n", __func__, __LINE__, bd->client.win);
        _policy_border_uniconify_top_border(bd);
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

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. ROOT ANGLE CHANGED... angle = %d. control_win:0x%07x\n", __func__, __LINE__, angle, g_indi_control_win);
   bd = e_border_find_by_client_window(g_indi_control_win);

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
                  if (!bd_temp->visible && !bd_temp->iconic)
                    {
                       if (!e_illume_border_is_keyboard(bd_temp))
                         continue;
                    }

                  L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... win = 0x%07x..  SEND client Event with angle = %d\n", __func__, __LINE__, bd_temp->client.win, angle);
                  ecore_x_client_message32_send (bd_temp->client.win, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                                                 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE, angle, g_indi_control_win, 0, 0, 0);
               }

             if (_e_illume_cfg->use_indicator_widget)
               {
                  E_Illume_Quickpanel *qp;
                  qp = e_illume_quickpanel_by_zone_get(zone);
                  if (qp)
                    _policy_layout_quickpanel_rotate(qp, angle);
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
   L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... E_ILLUME_ATOM_NOTIFICATION_LEVEL property!!!  win = 0x%07x\n", __func__, __LINE__, event->win);

   E_Border* bd;
   E_Border *indi_bd;
   E_Illume_Border_Info *bd_info = NULL;
   int layer = 0;

   // 0.
   if (!(bd = e_border_find_by_client_window(event->win)))
     {
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Win (0x%07x) doesn't have border... So return..\n", __func__, __LINE__, event->win);
        return;
     }

   bd_info = _policy_get_border_info(bd);
   if (!bd_info) return;

   // 1.
   if (!e_illume_border_is_notification (bd))
     {
        L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... Win (0x%07x) gets NOTIFICATION_LEVEL notifiy... But this is NOT notification window... IGNORE!!!!!\n", __func__, __LINE__, bd->client.win);
        return;
     }

   // 2.
   bd_info->level = _policy_border_get_notification_level(bd->client.win);
   layer = _policy_notification_level_map(bd_info->level);

   // 3.
   e_border_layer_set(bd, layer);
   L (LT_NOTIFICATION, "[ILLUME2][NOTIFICATION] %s(%d)... win (0x%07x) is notification window... level = %d\n",
      __func__, __LINE__, bd->client.win, bd_info->level);

   // 4.
   indi_bd = e_illume_border_indicator_get(bd->zone);
   if (indi_bd)
     {
        L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
        _policy_border_indicator_control(indi_bd);
     }

   L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. ICONIFY win:0x%07x And UN-ICONIFY Top win...\n", __func__, __LINE__, bd->client.win);
   _policy_border_uniconify_top_border(bd);
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

   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d)... OVERAY_WINDOW:0x%07x\n", __func__, __LINE__, _e_overlay_win);
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

   // visibility is changed
   L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
   _g_visibility_changed = EINA_TRUE;
}

static void
_policy_property_illume_window_state_change(Ecore_X_Event_Window_Property *event)
{
   E_Border *bd;

   if (!(bd = e_border_find_by_client_window(event->win))) return;
   if (bd->stolen) return;

   unsigned int state = ecore_x_e_illume_window_state_get(event->win);
   _policy_border_illume_window_state_change(bd, state);
}

static void
_policy_border_illume_window_state_change(E_Border *bd, unsigned int state)
{
   E_Border* indi_bd;

   if (!bd) return;
   if (bd->client.illume.win_state.state == state) return;

   ELBF(ELBT_ILLUME, 0, bd->client.win, "SET WIN_STATE %d->%d",
        bd->client.illume.win_state.state, state);

   bd->client.illume.win_state.state = state;
   switch (state)
     {
      case ECORE_X_ILLUME_WINDOW_STATE_FLOATING:
         e_hints_window_state_update(bd, ECORE_X_WINDOW_STATE_ABOVE, ECORE_X_WINDOW_STATE_ACTION_ADD);

         if (_e_illume_cfg->use_force_iconify)
           _policy_border_uniconify_below_borders(bd);

         bd->client.icccm.request_pos = EINA_TRUE;
         bd->lock_user_size = 0;
         bd->lock_user_location = 0;
         bd->borderless = 0;

         if (bd->maximized)
           e_border_unmaximize(bd, E_MAXIMIZE_BOTH);

         indi_bd = e_illume_border_indicator_get(bd->zone);
         if (indi_bd)
           {
              L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
              _policy_border_indicator_control(indi_bd);
           }

        if (!eina_list_data_find(dep_rot.list, bd))
          {
             int ang = _prev_angle_get(bd->client.win);
             int next_ang = 0;

             bd->client.e.state.rot.type = E_BORDER_ROTATION_TYPE_DEPENDENT;
             dep_rot.list = eina_list_append(dep_rot.list, bd);

             if (ang == -1)
               ang = bd->client.e.state.rot.curr;

             if (dep_rot.refer.active_bd) next_ang = dep_rot.refer.active_bd->client.e.state.rot.curr;
             else next_ang = dep_rot.ang;

             if (next_ang != ang)
               e_border_rotation_set(bd, next_ang);
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
           {
             bd->borderless = 0;
           }

         bd->client.icccm.request_pos = EINA_FALSE;

         indi_bd = e_illume_border_indicator_get(bd->zone);
         if (indi_bd)
           {
              L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, bd->client.win);
              _policy_border_indicator_control(indi_bd);
           }

         if (eina_list_data_find(dep_rot.list, bd) == bd)
           {
              bd->client.e.state.rot.type = E_BORDER_ROTATION_TYPE_NORMAL;
              dep_rot.list = eina_list_remove(dep_rot.list, bd);
           }
         break;
     }
   bd->changes.size = 1;
   bd->changes.pos = 1;
   bd->changed = 1;
}

static void
_policy_border_illume_handlers_add(E_Illume_Border_Info *bd_info)
{
   if (!bd_info) return;
   if (bd_info->handlers) return;

   bd_info->handlers = eina_list_append(bd_info->handlers,
                                                 ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                                                         _policy_border_cb_mouse_up, bd_info));
   bd_info->handlers = eina_list_append(bd_info->handlers,
                                                 ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                                                         _policy_border_cb_mouse_move, bd_info));
}

static void
_policy_border_illume_handlers_remove(E_Illume_Border_Info *bd_info)
{
   Ecore_Event_Handler *hdl;

   if (!bd_info) return;
   if (!bd_info->handlers) return;

   EINA_LIST_FREE(bd_info->handlers, hdl)
     ecore_event_handler_del(hdl);
}

static Eina_Bool
_policy_border_cb_mouse_down(void *data,
                             int   type __UNUSED__,
                             void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Illume_Border_Info *bd_info;
   E_Border *bd;

   L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);
   ev = event;
   bd_info = data;
   bd = bd_info->border;

   if (ev->window != bd->event_win &&
       ev->event_window != bd->event_win)
     return ECORE_CALLBACK_PASS_ON;

   if (bd_info->resize_req.mouse.down)
     return ECORE_CALLBACK_PASS_ON;

   bd_info->resize_req.mouse.down = 1;

   bd_info->resize_req.mouse.dx = bd->x - ev->root.x;
   bd_info->resize_req.mouse.dy = bd->y - ev->root.y;

   bd_info->resize_req.mouse.x = bd->x;
   bd_info->resize_req.mouse.y = bd->y;

   e_border_raise(bd);
   ecore_x_mouse_down_send(bd->client.win, ev->x, ev->y, ev->buttons);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_resize_rect_geometry_get(E_Illume_Border_Info *bd_info,
                          Evas_Coord_Rectangle *r,
                          int                   ev_x,
                          int                   ev_y,
                          int                   direction,
                          E_Resizable_Area_Info *area)
{
   E_Border *bd;
   int x = 0, y = 0, w = 0, h = 0;
   int mw = 0, mh = 0;
   int cx = 0, cy = 0;
   int max_size = 0;
   int min_size = 200;

   bd = bd_info->border;

   e_illume_border_min_get(bd, &mw, &mh);
   // min_size is workaround adjustement due to some window's wrong w h after rotation is changed.
   if(mw < min_size) mw = min_size;
   if(mh < min_size) mh = min_size;

   if (direction == ECORE_X_NETWM_DIRECTION_SIZE_BR)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = ev_x - bd->x;          h = ev_y - bd->y;
              break;
           case 90:
              cx = bd->x;               cy = bd->y + bd->h;
              x = bd->x;                 y = ev_y;
              w = ev_x - x;              h = (bd->y + bd->h) - y;
              break;
           case 180:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = ev_x;                  y = ev_y;
              w = (bd->x + bd->w) - x;   h = (bd->y + bd->h) - y;
              break;
           case 270:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = ev_x;                 y = bd->y;
              w  = (bd->x + bd->w) - x;  h = ev_y - y;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_TL)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = ev_x;                  y = ev_y;
              w = (bd->x + bd->w) - x;   h = (bd->y + bd->h) - y;
              break;
           case 90:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = ev_x;                 y = bd->y;
              w  = (bd->x + bd->w) - x;  h = ev_y - y;
              break;
           case 180:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = ev_x - bd->x;          h = ev_y - bd->y;
              break;
           case 270:
              cx = bd->x;               cy = bd->y + bd->h;
              x = bd->x;                 y = ev_y;
              w = ev_x - x;              h = (bd->y + bd->h) - y;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_TR)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x;               cy = bd->y + bd->h;
              x = bd->x;                 y = ev_y;
              w = ev_x - x;              h = (bd->y + bd->h) - y;
              break;
           case 90:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = ev_x;                  y = ev_y;
              w = (bd->x + bd->w) - x;   h = (bd->y + bd->h) - y;
              break;
           case 180:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = ev_x;                 y = bd->y;
              w  = (bd->x + bd->w) - x;  h = ev_y - y;
              break;
           case 270:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = ev_x - bd->x;          h = ev_y - bd->y;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_BL)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = ev_x;                 y = bd->y;
              w  = (bd->x + bd->w) - x;  h = ev_y - y;
              break;
           case 90:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = ev_x - bd->x;          h = ev_y - bd->y;
              break;
           case 180:
              cx = bd->x;               cy = bd->y + bd->h;
              x = bd->x;                 y = ev_y;
              w = ev_x - x;              h = (bd->y + bd->h) - y;
              break;
           case 270:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = ev_x;                  y = ev_y;
              w = (bd->x + bd->w) - x;   h = (bd->y + bd->h) - y;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_R)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = ev_x - bd->x;          h = bd->h;
              break;
           case 90:
              cx = bd->x;               cy = bd->y + bd->h;
              x = bd->x;                 y = ev_y;
              w = bd->w;              h = (bd->y + bd->h) - y;
              break;
           case 180:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = ev_x;                  y = ev_y;
              w = (bd->x + bd->w) - x;   h = bd->h;
              break;
           case 270:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = ev_x;                 y = bd->y;
              w  = bd->w;  h = ev_y - y;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_L)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = ev_x;                  y = bd->y;
              w = (bd->x + bd->w) - x;   h = bd->h;
              break;
           case 90:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = ev_x;                 y = bd->y;
              w  = bd->w;  h = ev_y - y;
              break;
           case 180:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = ev_x - bd->x;          h = bd->h;
              break;
           case 270:
              cx = bd->x;               cy = bd->y + bd->h;
              x = bd->x;                 y = ev_y;
              w = bd->w;              h = (bd->y + bd->h) - y;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_T)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = bd->x;                  y = ev_y;
              w = bd->w;   h = (bd->y + bd->h) - y;
              break;
           case 90:
              cx = bd->x + bd->w;               cy = bd->y;
              x = ev_x;                 y = bd->y;
              w = (bd->x + bd->w) - x;              h = bd->h;
              break;
           case 180:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = bd->w;          h = ev_y - bd->y;
              break;
           case 270:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = bd->x;                 y = bd->y;
              w  = ev_x - bd->x;  h = bd->h;
              break;
           default:
              break;
          }
     }
   else if (direction == ECORE_X_NETWM_DIRECTION_SIZE_B)
     {
        switch (bd_info->resize_req.angle)
          {
           case 0:
              cx = bd->x;               cy = bd->y;
              x = bd->x;                 y = bd->y;
              w = bd->w;          h = ev_y - bd->y;
              break;
           case 90:
              cx = bd->x + bd->w;       cy = bd->y;
              x  = bd->x;                 y = bd->y;
              w  = ev_x - bd->x;  h = bd->h;
              break;
           case 180:
              cx = bd->x + bd->w;       cy = bd->y + bd->h;
              x = bd->x;                  y = ev_y;
              w = bd->w;   h = (bd->y + bd->h) - y;
              break;
           case 270:
              cx = bd->x + bd->w;               cy = bd->y;
              x = ev_x;                 y = bd->y;
              w = (bd->x + bd->w) - x;              h = bd->h;
              break;
           default:
              break;
          }
     }
   else
     {
        //error
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... ERROR... direction(%d) is not defined!!!\n", __func__, __LINE__, direction);
        return;
     }

   if (bd->zone->w > bd->zone->h)
     max_size = bd->zone->h;
   else
     max_size = bd->zone->w;

   if(area)
     {
        area->x_dist = w;
        area->y_dist = h;
        area->min_width = mw;
        area->min_height = mh;
        area->max_width = max_size;
        area->max_height = max_size;
    }

   if (w < mw) w = mw;
   if (h < mh) h = mh;
   if (w > max_size) w = max_size;
   if (h > max_size) h = max_size;

   if ((x + w) > (bd->x + w)) x = cx - w;
   if ((y + h) > (bd->y + h)) y = cy - h;

   if ((cx - x) > max_size)
     x = cx - max_size;
   if ((cy - y) > max_size)
     y = cy - max_size;

   x = x - bd->zone->x;
   y = y - bd->zone->y;

   r->x = x;
   r->y = y;
   r->w = w;
   r->h = h;
}

static Eina_Bool
_policy_border_cb_mouse_up(void *data,
                           int   type __UNUSED__,
                           void *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Illume_Border_Info *bd_info;
   E_Border *bd;

   L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);
   ev = event;
   bd_info = data;
   bd = bd_info->border;

   if (ev->window != bd->event_win &&
       ev->event_window != bd->event_win)
     return ECORE_CALLBACK_PASS_ON;

   if (!bd_info->resize_req.mouse.down)
     return ECORE_CALLBACK_PASS_ON;

   bd->lock_user_location = 0;
   e_grabinput_release(bd->event_win, bd->event_win);

   if (bd_info->resize_req.mouse.resize)
     {
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);
        Evas_Coord_Rectangle r;
        _resize_rect_geometry_get(bd_info, &r, ev->root.x, ev->root.y, bd_info->resize_req.direction, NULL);

        bd_info->resize_req.direction = ECORE_X_NETWM_DIRECTION_CANCEL;
        bd_info->resize_req.mouse.x = r.x + bd->zone->x;
        bd_info->resize_req.mouse.y = r.y + bd->zone->y;
        bd_info->resize_req.mouse.w = r.w;
        bd_info->resize_req.mouse.h = r.h;
        bd_info->resize_req.need_change = 1;
        bd->changes.pos = 1;
        bd->changes.size = 1;
        bd->changed = 1;
        Evas_Object *o = (Evas_Object *)e_object_data_get(E_OBJECT(bd));
        if (o)
          {
             evas_object_del(o);
             printf("[IL2] DEL OBJ in UP\n");
          }
     }
   else
     {
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);
        bd_info->resize_req.mouse.x = ev->root.x + bd_info->resize_req.mouse.dx;
        bd_info->resize_req.mouse.y = ev->root.y + bd_info->resize_req.mouse.dy;
     }

   bd_info->resize_req.mouse.down = 0;
   bd_info->resize_req.mouse.resize = 0;
   bd_info->resize_req.mouse.locked = 0;

   /* set property on this border to say we are done dragging */
   ecore_x_e_illume_drag_set(bd->client.win, 0);

   /* set property on zone window that a drag is finished */
   ecore_x_e_illume_drag_set(bd->zone->black_win, 0);

   ecore_x_window_raise(bd->client.shell_win);
   _policy_border_illume_handlers_remove(bd_info);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_policy_border_cb_mouse_move(void *data,
                             int   type __UNUSED__,
                             void *event)
{
   Ecore_Event_Mouse_Move *ev;
   E_Illume_Border_Info *bd_info;
   E_Border *bd;
   Evas_Object *rect;

   L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);
   ev = event;
   bd_info = data;
   bd = bd_info->border;

   if (ev->window != bd->event_win &&
       ev->event_window != bd->event_win)
     return ECORE_CALLBACK_PASS_ON;

   if (!bd_info->resize_req.mouse.down)
     return ECORE_CALLBACK_PASS_ON;

   if (bd_info->resize_req.mouse.resize)
     {
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... event (x:%d, y:%d, root_x:%d, root_y:%d)\n", __func__, __LINE__, ev->x, ev->y, ev->root.x, ev->root.y);
        Evas_Object *o = (Evas_Object *)e_object_data_get(E_OBJECT(bd));
        if (!o) return ECORE_CALLBACK_PASS_ON;

        Evas_Coord_Rectangle r;
        E_Resizable_Area_Info area;

        _resize_rect_geometry_get(bd_info, &r, ev->root.x, ev->root.y, bd_info->resize_req.direction, &area);
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... x:%d, y:%d, w:%d, h:%d\n", __func__, __LINE__, r.x, r.y, r.w, r.h);

        rect = edje_object_part_object_get(o, "opacity_rect");

        if((r.w <= area.min_width && r.h <= area.min_height)
           || (r.w >= area.max_width && r.h >= area.max_height))
          {
            edje_object_signal_emit(o, "resize,notavail", "illume2");
            evas_object_color_set(rect, 64, 64, 64, 64);
          }
        else
          {
            edje_object_signal_emit(o, "resize,normal", "illume2");
            evas_object_color_set(rect, 255, 255, 255, 255);
          }
        if(area.x_dist < area.min_width && area.y_dist < area.min_height)
          {
            int tmp = 0;
            tmp = ((area.min_width-area.x_dist)>= (area.min_height-area.y_dist))? area.min_width - area.x_dist : area.min_height - area.y_dist;
            if( tmp >= 200)
                tmp = 200;
            tmp /= 20;
            tmp %= 11;
            tmp --;
            tmp = (int)(128*( (double)tmp/10 + 1));
            evas_object_color_set(rect, tmp, tmp, tmp, tmp);
          }
        if (area.x_dist > area.max_width && area.y_dist > area.max_height)
          {
            int tmp = 0;
            tmp = ((area.x_dist - area.max_width) >= (area.y_dist - area.max_height))? area.x_dist - area.max_width : area.y_dist - area.max_height;
            if( tmp >= 200)
               tmp = 200;
            tmp /= 20;
            tmp %= 11;
            tmp --;
            tmp = (int)(128*( (double)tmp/10 + 1));
            evas_object_color_set(rect, tmp, tmp, tmp, tmp);
          }

        evas_object_move(o, r.x, r.y);
        evas_object_resize(o, r.w, r.h);
     }
   else
     {
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);
        bd_info->resize_req.mouse.x = ev->root.x + bd_info->resize_req.mouse.dx;
        bd_info->resize_req.mouse.y = ev->root.y + bd_info->resize_req.mouse.dy;
        bd->changes.pos = 1;
        bd->changed = EINA_TRUE;
     }

   return ECORE_CALLBACK_PASS_ON;
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
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_CLIPBOARD_GEOMETRY)
     {
        _policy_property_clipboard_geometry_change (event);
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_CLIPBOARD_STATE)
     {
        _policy_property_clipboard_state_change (event);
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
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_OPACITY_MODE)
     {
        _policy_property_indicator_opacity_change(event);
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
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE)
     {
        _policy_property_illume_window_state_change(event);
     }
   /* enable/disable composite module - 100320 yigl */
#ifdef COMP_MODULE_CONTROL
   else if (event->atom == E_ILLUME_ATOM_COMP_MODULE_ENABLED)
     {
        _policy_property_composite_module_change (event);
     }
#endif
/* enable/disable backkey module - 130610 seongwon1.cho */
#ifdef BACKKEY_MODULE_CONTROL
   else if (event->atom == E_ILLUME_ATOM_BACKKEY_MODULE_ENABLED)
     {
        _policy_property_backkey_module_change (event);
     }
#endif
/* enable/disable devmode module - 130610 seongwon1.cho */
#ifdef DEVMODE_MODULE_CONTROL
   else if (event->atom == E_ILLUME_ATOM_DEVMODE_MODULE_ENABLED)
     {
        _policy_property_devmode_module_change (event);
     }
#endif
   else if (event->atom == E_INDICATOR_CMD_WIN)
     {
        _policy_property_indicator_cmd_win_change(event);
     }
   else if (event->atom == E_ACTIVE_INDICATOR_WIN)
     {
        _policy_property_active_indicator_win_change(event);
     }
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
             fprintf (out, " No  Border     ClientWin     w     h       x       y   layer  visible  WinName\n" );
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );

             EINA_LIST_FOREACH(border_list, l, bd)
               {
                  if (!bd) continue;
                  if (temp_bd == NULL) temp_bd = bd;

                  fprintf (out, "%3i  0x%07x  0x%07x  %4i  %4i  %6i  %6i  %5i  %5i     %-30s \n",
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
             fprintf (out, " No  Border     ClientWin     w     h       x       y   isKBD layer level desk   WinName\n" );
             fprintf (out, "--------------------------------------------------------------------------------------------------------------------\n" );

             EINA_LIST_FOREACH(e_border_info_list, l, bd_info)
               {
                  E_Zone *zone;
                  E_Border *bd;
                  int x, y;
                  int desk_x=0, desk_y=0;
                  if (!bd_info) continue;

                  bd = bd_info->border;
                  zone = bd->zone;

                  for (x = 0; x < zone->desk_x_count; x++)
                    {
                       for (y = 0; y < zone->desk_y_count; y++)
                         {
                            if (bd->desk == zone->desks[x + zone->desk_x_count * y])
                              {
                                 desk_x = x;
                                 desk_y = y;
                                 break;
                              }
                         }
                    }
                  fprintf (out, "%3i  0x%07x  0x%07x  %4i  %4i  %6i  %6i  %3i  %5i %5i   (%d,%d)  %-30s \n",
                          i++, bd_info->border->win, bd_info->border->client.win, bd_info->border->w, bd_info->border->h, bd_info->border->x, bd_info->border->y,
                          bd_info->border->client.vkbd.vkbd, bd_info->border->layer, bd_info->level, desk_x, desk_y, bd_info->border->client.netwm.name);
               }
             fprintf (out, "--------------------------------------------------------------------------------------------------------------------\n" );
          }
        ecore_x_flush();

        if (temp_bd == NULL) goto finish;

        E_Border_List *bl;

        fprintf (out, "-------------------------------- E17 STACK INFO--------------------------------------------\n" );
        fprintf (out, " No  Border     ClientWin     w     h       x       y   layer  visible  WinName\n" );
        fprintf (out, "---------------------------------------------------------------------------------------------\n" );

        i = 1;
        bl = e_container_border_list_last(temp_bd->zone->container);
        while ((bd = e_container_border_list_prev(bl)))
          {
             fprintf (out, "%3i  0x%07x  0x%07x  %4i  %4i  %6i  %6i  %5i  %5i     %-30s \n",
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
             fprintf (out, " No  Win          w     h       x       y   depth  viewable  visibility comp_vis iconify by_wm is_border(Client Win)\n" );
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );

             EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
               {
                  if (xwin_info->bd_info)
                    {
                       if (xwin_info->bd_info->border)
                         {
                            fprintf (out, "%3i  0x%07x  %4i  %4i  %6i  %6i  %5i   %5i     %5i      %5i   %5i  %5i    yes(0x%07x)\n",
                                    i++, xwin_info->id, xwin_info->attr.w, xwin_info->attr.h, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.depth,
                                    xwin_info->viewable, xwin_info->visibility, xwin_info->comp_vis, xwin_info->bd_info->border->iconic, xwin_info->iconify_by_wm, xwin_info->bd_info->border->client.win);
                         }
                       else
                         {
                            fprintf (out, "%3i  0x%07x  %4i  %4i  %6i  %6i  %5i   %5i     %5i      %5i       0    %3i        no(NULL)\n",
                                    i++, xwin_info->id, xwin_info->attr.w, xwin_info->attr.h, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.depth,
                                    xwin_info->viewable, xwin_info->visibility, xwin_info->comp_vis, xwin_info->iconify_by_wm);
                         }
                    }
                  else
                    {
                       fprintf (out, "%3i  0x%07x  %4i  %4i  %6i  %6i  %5i   %5i      %5i     %5i       0    %3i        no(NULL)\n",
                               i++, xwin_info->id, xwin_info->attr.w, xwin_info->attr.h, xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.depth,
                               xwin_info->viewable, xwin_info->visibility, xwin_info->comp_vis, xwin_info->iconify_by_wm);
                    }
               }
             fprintf (out, "---------------------------------------------------------------------------------------------\n" );
          }

        ecore_x_flush();
     }

finish:

   fprintf (out, "--------------------------------GLOBAL INFO--------------------------------------------------\n" );
   fprintf (out, "g_rotated_win:0x%07x (g_root_angle:%d)\n", g_rotated_win, g_root_angle);
   fprintf (out, "g_active_win:0x%07x (pid:%d, angle:%d)\n", g_active_win, g_active_pid, _policy_window_rotation_angle_get(g_active_win));
   fprintf (out, "g_indi_control_win:0x%07x\n", g_indi_control_win);
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

   if (ret == ECORE_X_ERROR_CODE_SUCCESS)
     {
        if (prop_ret && num_ret)
          {
             angle = ((int *)prop_ret)[0];
             if (angle % 90) angle = -1;
          }
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
   Ecore_X_Illume_Indicator_State state;
   int show;

   state = ecore_x_e_illume_indicator_state_get(bd->client.win);
   if (state == ECORE_X_ILLUME_INDICATOR_STATE_ON)
     show = 1;
   else if (state == ECORE_X_ILLUME_INDICATOR_STATE_OFF)
     show = 0;
   else
     show = -1;

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
   // It caused abnormal size of quickpanel window and abnormal rotation state.
   // disable it for now.
#if 0
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
                  ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Portrait quick panel...(%d) quick win = 0x%07x  old (%d, %d)  new (%d, %d)\n", __LINE__, bd->client.win, bd->w, bd->h, bd->zone->w, temp);
                  _policy_border_resize (bd, bd->zone->w, temp);
               }
             else
                ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Portrait quick panel...(%d) quick win = 0x%07x..  But size is not change\n", __LINE__, bd->client.win);
          }
        else
          {
             if (diff == 90 || diff == 270)
               {
                  temp = bd->h;
                  ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Landscape quick panel...(%d) quick win = 0x%07x  old (%d, %d)  new (%d, %d)\n", __LINE__, bd->client.win, bd->w, bd->h, temp, bd->zone->h);
                  _policy_border_resize (bd, temp, bd->zone->h);
               }
             else
                ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] Landscape quick panel...(%d) quick win = 0x%07x..  But size is not change\n", __LINE__, bd->client.win);
          }
     }
#endif

   // pass 2 - send client message
   EINA_LIST_FOREACH(qp->borders, bd_list, panel)
     {
        if (!panel) continue;
        if (panel->angle == angle) continue;

        if (panel->angle > angle) diff = panel->angle - angle;
        else diff = angle - panel->angle;

        bd = panel->bd;

        ILLUME2_TRACE ("[ILLUME2-QUICKPANEL] SEND CLIENT MESSAGE TO QUICKPANEL!!!!(%d)  win:0x%07x, angle = %d\n", __LINE__, bd->client.win, angle);
        ecore_x_client_message32_send (bd->client.win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                       ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                       angle, 0, 0, 0, 0);

        panel->angle = angle;
     }
}

void
_policy_window_focus_in(Ecore_X_Event_Window_Focus_In *event)
{
   ILLUME2_TRACE("[ILLUME2-FOCUS] _policy_window_focus_in... win = 0x%07x\n", event->win);

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
   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... win = 0x%07x, sibling = 0x%07x, stack mode = %d\n", __func__, __LINE__, bd->client.win, sibling->client.win, stack_mode);

   if (bd->layer != sibling->layer)
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... Sibling layer is different!!! win = 0x%07x (layer = %d), sibling = 0x%07x (layer = %d)\n", __func__, __LINE__, bd->client.win, bd->layer, sibling->client.win, sibling->layer);
        return;
     }

   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... Restack Window.. win = 0x%07x,  sibling = 0x%07x, stack_mode = %d\n", __func__, __LINE__, bd->win, sibling->win, stack_mode);
   if (stack_mode == E_ILLUME_STACK_ABOVE)
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... STACK CHANGE with ABOVE... win:0x%07x, above_win:0x%07x\n", __func__, __LINE__, bd->client.win, sibling->client.win);
        e_border_stack_above (bd, sibling);
     }
   else if (stack_mode == E_ILLUME_STACK_BELOW)
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... STACK CHANGE with BELOW... win:0x%07x, below_win:0x%07x\n", __func__, __LINE__, bd->client.win, sibling->client.win);
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

#ifdef BACKKEY_MODULE_CONTROL
   E_ILLUME_ATOM_BACKKEY_MODULE_ENABLED = ecore_x_atom_get ("_E_BACKKEY_ENABLE");
   if(!E_ILLUME_ATOM_BACKKEY_MODULE_ENABLED)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_BACKKEY_ENABLE Atom...\n");
        return 0;
     }
#endif

#ifdef DEVMODE_MODULE_CONTROL
   E_ILLUME_ATOM_DEVMODE_MODULE_ENABLED = ecore_x_atom_get ("_E_DEVMODE_ENABLE");
   if(!E_ILLUME_ATOM_DEVMODE_MODULE_ENABLED)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_DEVMODE_ENABLE Atom...\n");
        return 0;
     }
#endif

   E_INDICATOR_CMD_WIN = ecore_x_atom_get("_E_INDICATOR_CMD_WIN");
   if (!E_INDICATOR_CMD_WIN)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_INDICATOR_CMD_WIN Atom...\n");
        return 0;
     }

   E_ACTIVE_INDICATOR_WIN = ecore_x_atom_get("_E_ACTIVE_INDICATOR_WIN");
   if (!E_ACTIVE_INDICATOR_WIN)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ACTIVE_INDICATOR_WIN Atom...\n");
        return 0;
     }

   return 1;
}


int _policy_init (void)
{
   Eina_List *ml;
   E_Manager *man;

   /* for visibility */
   _e_illume_xwin_info_hash = eina_hash_string_superfast_new(NULL);
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        _policy_manage_xwins (man);

        dep_rot.root = man->root;
        dep_rot.refer.cmd_win = _policy_indicator_cmd_win_get(dep_rot.root);
        if (dep_rot.refer.cmd_win)
          {
             Ecore_X_Window win;

             win = _policy_active_indicator_win_get(dep_rot.refer.cmd_win);
             dep_rot.refer.active_bd = e_border_find_by_client_window(win);
          }
     }

   // initialize atom
   if (!_policy_atom_init())
     {
        eina_hash_free (_e_illume_xwin_info_hash);
        return 0;
     }

   /* for visibility */
   _e_illume_msg_handler = e_msg_handler_add(_policy_msg_handler, NULL);

   return 1;
}


void _policy_fin (void)
{
   /* for visibility */
   if (_e_illume_msg_handler) e_msg_handler_del(_e_illume_msg_handler);
   eina_hash_free (_e_illume_xwin_info_hash);
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
   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... bd = %x, bd's client = 0x%07x\n", __func__, __LINE__, bd, bd->client.win);
   E_Illume_Border_Info tmp_win;
   tmp_win.border = bd;
   return (E_Illume_Border_Info*) eina_list_search_unsorted (
      e_border_info_list, EINA_COMPARE_CB(_policy_compare_cb_border), &tmp_win);
}


static E_Illume_Border_Info* _policy_add_border_info_list (E_Border* bd)
{
   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... bd = %x, bd's client = 0x%07x\n", __func__, __LINE__, bd, bd->client.win);

   if (e_object_is_del(E_OBJECT(bd))) return NULL;

   E_Illume_Border_Info* bd_info = (E_Illume_Border_Info*) calloc (1, sizeof (E_Illume_Border_Info));
   if (!bd_info)
     {
        fprintf (stderr, "[ILLUME2] Critical Error... Fail to create memory... (%s:%d)\n", __func__, __LINE__);
        return NULL;
     }
   bd_info->pid = bd->client.netwm.pid;
   bd_info->border = bd;
   // set level
   bd_info->level = 50;
   // set opaque
   bd_info->opaque = _policy_property_window_opaque_get(bd->client.win);

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
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:0x%07x) -> other(win:0x%07x) ]\n", __func__, __LINE__, bd->client.win, temp_bd_info->border->client.win);
     }
   else
     {
        e_border_info_list = eina_list_append (e_border_info_list, bd_info);
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ ---> bd(win:0x%07x) ]\n", __func__, __LINE__, bd->client.win);
     }

   return bd_info;
}


static void _policy_delete_border_info_list (E_Border* bd)
{
   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)...  bd = %x, bd's client = 0x%07x\n", __func__, __LINE__, bd, bd->client.win);
   E_Illume_Border_Info* bd_info = _policy_get_border_info (bd);

   if (bd_info == NULL)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... There is no border in the list... bd = %x, bd's client = 0x%07x\n", __func__, __LINE__, bd, bd->client.win);
        return;
     }

   _policy_border_illume_handlers_remove(bd_info);

   e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... remove bd(win:0x%07x)\n", __func__, __LINE__, bd->client.win);
   free (bd_info);
}


static int
_policy_zone_layout_app_layer_check (E_Border* bd)
{
   Ecore_X_Window_Type *types = NULL;
   E_Illume_Border_Info *bd_info;
   int num, i, layer;

   if (!bd) return POL_APP_LAYER;

   bd_info = _policy_get_border_info(bd);
   if (!bd_info) return POL_APP_LAYER;

   layer = POL_APP_LAYER;

   num = ecore_x_netwm_window_types_get(bd->client.win, &types);
   if (num)
     {
        i = 0;
        for (i=0; i< num; i++)
          {
             if (types[i] == ECORE_X_WINDOW_TYPE_NOTIFICATION)
               layer = _policy_notification_level_map(bd_info->level);
          }

        free (types);
     }

   if (bd->client.netwm.state.stacking == E_STACKING_BELOW)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WIN:0x%07x  is BELOW state.. Change layer to 50\n", __func__, __LINE__, bd->client.win);
        layer = POL_STATE_BELOW_LAYER;
     }
   else if (bd->client.netwm.state.stacking == E_STACKING_ABOVE)
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... WIN:0x%07x  is ABOVE state.. Change layer to 150\n", __func__, __LINE__, bd->client.win);
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
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_SINGLE... bd_info's border = %x, client win = 0x%07x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

   bd = bd_info->border;
   if (!bd)
     {
        fprintf (stderr, "[ILLUME2] fatal error! (%s)  There is no border!\n", __func__);
        return;
     }

   if (bd->moving && !bd->client.illume.win_state.state)
     {
        L(LT_AIA, "[ILLUME2][AIA] %s(%d)... Cancel moving... win:0x%07x, bd->moving:%d\n", __func__, __LINE__, bd->client.win, bd->moving);
        e_border_move_cancel();
     }

   if ((!bd->new_client) && (!bd->visible)) return;

   layer = _policy_zone_layout_app_layer_check (bd);

   /* check if user defined position */
   if (bd->client.icccm.request_pos)
     {
        if (bd->client.illume.win_state.state)
          {
             if (bd_info->resize_req.need_change)
               {
                  if ((bd->x != bd_info->resize_req.mouse.x) ||
                      (bd->y != bd_info->resize_req.mouse.y))
                    move = EINA_TRUE;

                  if ((bd->w != bd_info->resize_req.mouse.w) ||
                      (bd->h != bd_info->resize_req.mouse.h))
                    resize = EINA_TRUE;

                  if (move && resize)
                    {
                       e_border_move_resize(bd,
                                            bd_info->resize_req.mouse.x,
                                            bd_info->resize_req.mouse.y,
                                            bd_info->resize_req.mouse.w,
                                            bd_info->resize_req.mouse.h);
                    }
                  else if (move)
                    {
                       _policy_border_move(bd,
                                           bd_info->resize_req.mouse.x,
                                           bd_info->resize_req.mouse.y);
                    }
                  else if (resize)
                    {
                       _policy_border_resize(bd,
                                             bd_info->resize_req.mouse.w,
                                             bd_info->resize_req.mouse.h);
                    }

                  bd_info->resize_req.need_change = 0;
                  L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
               }

             if (bd_info->resize_req.mouse.down &&
                 bd_info->resize_req.mouse.locked)
               {
                  if ((bd->x != bd_info->resize_req.mouse.x) ||
                      (bd->y != bd_info->resize_req.mouse.y))
                    _policy_border_move(bd,
                                        bd_info->resize_req.mouse.x,
                                        bd_info->resize_req.mouse.y);
                  L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
               }
          }

        _policy_zone_layout_app_layer_set (bd, layer);
        return;
     }

   /* resize & move if needed */
   if (bd->client.illume.win_state.state)
     {
        if (bd_info->resize_req.need_change)
          {
             if ((bd->x != bd_info->resize_req.mouse.x) ||
                 (bd->y != bd_info->resize_req.mouse.y))
               move = EINA_TRUE;

             if ((bd->w != bd_info->resize_req.mouse.w) ||
                 (bd->h != bd_info->resize_req.mouse.h))
               resize = EINA_TRUE;

             if (move && resize)
               {
                  e_border_move_resize(bd,
                                       bd_info->resize_req.mouse.x,
                                       bd_info->resize_req.mouse.y,
                                       bd_info->resize_req.mouse.w,
                                       bd_info->resize_req.mouse.h);
               }
             else if (move)
               {
                  _policy_border_move(bd,
                                      bd_info->resize_req.mouse.x,
                                      bd_info->resize_req.mouse.y);
               }
             else if (resize)
               {
                  _policy_border_resize(bd,
                                        bd_info->resize_req.mouse.w,
                                        bd_info->resize_req.mouse.h);
               }

             L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
             bd_info->resize_req.need_change = 0;
          }

        if (bd_info->resize_req.mouse.down &&
            bd_info->resize_req.mouse.locked)
          {
             if ((bd->x != bd_info->resize_req.mouse.x) ||
                 (bd->y != bd_info->resize_req.mouse.y))
               _policy_border_move(bd,
                                   bd_info->resize_req.mouse.x,
                                   bd_info->resize_req.mouse.y);
             L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
          }
     }
   else
     {
        if ((bd->w != bd->zone->w) || (bd->h != bd->zone->h))
          resize = EINA_TRUE;

        if ((bd->x != bd->zone->x) || (bd->y != bd->zone->y))
          move = EINA_TRUE;

        if (resize && move)
          e_border_move_resize(bd, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
        else if (resize)
          _policy_border_resize(bd, bd->zone->w, bd->zone->h);
        else if (move)
          _policy_border_move(bd, bd->zone->x, bd->zone->y);
     }

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
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_DUAL_TOP... bd_info's border = %x, client win = 0x%07x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

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
     resize = EINA_TRUE;

   /* move to correct position (relative to zone) if needed */
   if ((bd->x != bd->zone->x) || (bd->y != ny))
     move = EINA_TRUE;

   if (resize && move)
     e_border_move_resize(bd, bd->zone->x, ny, bd->zone->w, nh);
   else if (resize)
     _policy_border_resize(bd, bd->zone->w, nh);
   else if (move)
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
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_DUAL_LEFT... bd_info's border = %x, client win = 0x%07x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

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
     resize = EINA_TRUE;

   /* move to correct position (relative to zone) if needed */
   if ((bd->x != nx) || (bd->y != ky))
     move = EINA_TRUE;

   if (resize && move)
     e_border_move_resize(bd, nx, ky, nw, kh);
   else if (resize)
     _policy_border_resize(bd, nw, kh);
   else if (move)
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
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

   ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... LAYOUT_DUAL_CUSTOM... bd_info's border = %x, client win = 0x%07x\n", __func__, __LINE__, bd_info->border, bd_info->border->client.win);

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
     resize = EINA_TRUE;

   /* move to correct position (relative to zone) if needed */
   if ((bd->x != bd->zone->x) || (bd->y != ny))
     move = EINA_TRUE;

   if (resize && move)
     e_border_move_resize(bd, bd->zone->x, ny, bd->zone->w, nh);
   else if (resize)
     _policy_border_resize(bd, bd->zone->w, nh);
   else if (move)
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

static int
_policy_notification_level_map(int level)
{
   switch (level)
     {
      case E_ILLUME_NOTIFICATION_LEVEL_LOW:     return POL_NOTIFICATION_LAYER_LOW;
      case E_ILLUME_NOTIFICATION_LEVEL_NORMAL:  return POL_NOTIFICATION_LAYER_NORMAL;
      case E_ILLUME_NOTIFICATION_LEVEL_HIGH:    return POL_NOTIFICATION_LAYER_HIGH;
      default:                                  return POL_NOTIFICATION_LAYER_LOW;
     }
}

/* find new focus window */
static void
_policy_border_focus_top_stack_set (E_Border* bd)
{
   E_Border *temp_bd;
   E_Border *cur_focus;
   E_Border_List *bl;
   int root_w, root_h;

   root_w = bd->zone->w;
   root_h = bd->zone->h;

   cur_focus = e_border_focused_get();

   bl = e_container_border_list_last(bd->zone->container);
   while ((temp_bd = e_container_border_list_prev(bl)))
     {
        if (temp_bd == cur_focus) break;

        if ((temp_bd->x >= root_w) || (temp_bd->y >= root_h)) continue;
        if (((temp_bd->x + temp_bd->w) <= 0) || ((temp_bd->y + temp_bd->h) <= 0)) continue;
        if ((temp_bd != bd) &&
            (temp_bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING)) continue;

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

void _policy_border_stack (E_Event_Border_Stack *event)
{
   E_Event_Border_Stack* ev;
   E_Illume_Border_Info* bd_info;
   E_Illume_Border_Info* stack_bd_info;

   ev = event;

   L (LT_STACK, "[ILLUME2][STACK] %s(%d)... bd(win:0x%07x), stack(win:0x%07x), stack type: %d\n", __func__, __LINE__, ev->border->client.win, ev->stack ? (unsigned int)ev->stack->client.win:(unsigned int)NULL, ev->type);

   bd_info = _policy_get_border_info(ev->border);
   if (!bd_info) return;

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
                  L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:0x%07x) -> stack(win:0x%07x) ]\n", __func__, __LINE__, ev->border->client.win, ev->stack->client.win);
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
                       L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:0x%07x) -> other(win:0x%07x).. No stack(win:0x%07x) info ]\n", __func__, __LINE__, ev->border->client.win, temp_bd_info ? (unsigned int)temp_bd_info->border->client.win:(unsigned int)NULL, ev->stack->client.win);
                    }
               }
          }
        else
          {
             e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
             e_border_info_list = eina_list_append (e_border_info_list, bd_info);
             L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ ---> bd(win:0x%07x) ]\n", __func__, __LINE__, ev->border->client.win);
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
                  L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ stack(win:0x%07x) -> bd(win:0x%07x) ]\n", __func__, __LINE__, ev->stack->client.win, ev->border->client.win);
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
                       L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:0x%07x) -> other(win:0x%07x).. No stack(win:0x%07x) info ]\n", __func__, __LINE__, ev->border->client.win, temp_bd_info ? (unsigned int)temp_bd_info->border->client.win:(unsigned int)NULL, ev->stack->client.win);
                    }
               }
          }
        else
          {
             e_border_info_list = eina_list_remove (e_border_info_list, bd_info);
             e_border_info_list = eina_list_prepend (e_border_info_list, bd_info);
             L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:0x%07x) ---> ]\n", __func__, __LINE__, ev->border->client.win);
          }
     }
   else
     {
        ILLUME2_TRACE ("[ILLUME2-NEW] %s(%d)... Unknown type... border (0x%07x), win (0x%07x), type = %d\n", __func__, __LINE__, ev->border, ev->border->client.win, ev->type);
     }

   /* restack indicator when a active window stack is changed */
   if ((ev->border->client.win == g_active_win) &&
       (ev->border->layer == POL_NOTIFICATION_LAYER))
     {
        E_Border* indi_bd;
        indi_bd = e_illume_border_indicator_get(ev->border->zone);
        if (indi_bd)
          {
             L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... win = 0x%07x..  Control Indicator.\n", __func__, __LINE__, ev->border->client.win);
             _policy_border_indicator_control(indi_bd);
          }
     }

   ev->border->changes.pos = 1;
   ev->border->changed = 1;

   return;
}

void _policy_border_zone_set(E_Event_Border_Zone_Set *event)
{
   E_Event_Border_Zone_Set* ev;
   E_Border *bd;

   ev = event;

   bd = event->border;
   if (!bd) return;

   ecore_x_e_illume_zone_set(bd->client.win, bd->zone->black_win);
}

static void _policy_change_quickpanel_layer (E_Illume_Quickpanel* qp, E_Border* indi_bd, int layer, int level)
{
   Eina_List *bd_list;
   E_Illume_Quickpanel_Info *panel;
   E_Illume_Border_Info* bd_info;

   if (!qp) return;

   if (qp->popup)
     {
        bd_info = _policy_get_border_info(qp->popup->border);
        if (bd_info)
          {
             bd_info->level = level;
             e_border_stack_below (qp->popup->border, indi_bd);
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
}

static void _policy_change_indicator_layer(E_Border *indi_bd, E_Border *bd, int layer, int level)
{
   // the indicator's layer is changed to layer with level
   E_Illume_Border_Info *indi_bd_info;
   E_Illume_Quickpanel *qp;
   int new_noti_layer = 0;

   indi_bd_info = _policy_get_border_info(indi_bd);
   if (indi_bd_info)
     {
        indi_bd_info->level = level;
     }

   if (layer == POL_NOTIFICATION_LAYER)
     {
        new_noti_layer = _policy_notification_level_map(level);

        if (indi_bd->layer != new_noti_layer)
          e_border_layer_set(indi_bd, new_noti_layer);
     }
   else if (indi_bd->layer != layer)
     e_border_layer_set(indi_bd, layer);

   if (bd)
     {
        E_Border *top_bd;
        E_Illume_Border_Info *top_bd_info;

        // check transient_for window
        top_bd = _policy_border_transient_for_border_top_get(bd);
        if (!top_bd) top_bd = bd;

        top_bd_info = _policy_get_border_info(top_bd);
        if (!top_bd_info)
          {
             if ((qp = e_illume_quickpanel_by_zone_get(indi_bd->zone)))
               {
                  _policy_change_quickpanel_layer(qp, indi_bd, layer, level);
               }
             return;
          }

        L (LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... indicator's below win:0x%07x\n", __func__, __LINE__, top_bd->client.win);
        _policy_border_stack_change(indi_bd, top_bd, E_ILLUME_STACK_ABOVE);

        e_border_info_list = eina_list_remove(e_border_info_list, indi_bd_info);
        e_border_info_list = eina_list_prepend_relative(e_border_info_list, indi_bd_info, top_bd_info);
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... changed to [ bd(win:0x%07x) -> other(win:0x%07x) ]\n", __func__, __LINE__, indi_bd->client.win, top_bd->client.win);
     }

   if ((qp = e_illume_quickpanel_by_zone_get(indi_bd->zone)))
     {
        _policy_change_quickpanel_layer(qp, indi_bd, layer, level);
     }
}

static Eina_Bool _policy_border_indicator_state_change(E_Border *indi_bd, E_Border *bd)
{
   E_Illume_Border_Info *bd_info;
   int indi_show;
   int level;

   if (!indi_bd || !bd) return EINA_FALSE;

   indi_show = _policy_border_indicator_state_get(bd);
   if (indi_show == 1)
     {
        L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... Show Indicator (by win: 0x%07x)\n", __func__, __LINE__, bd->client.win);
        e_border_show(indi_bd);

        if ((e_illume_border_is_notification(bd)) ||
            (bd->layer == POL_NOTIFICATION_LAYER))
          {
             bd_info = _policy_get_border_info(bd);
             if (bd_info)
               level = bd_info->level;
             else
               level = 150;

             L(LT_NOTIFICATION, "[ILLUME2][NOTIFICATION]  %s(%d)... Notification Win:0x%07x, Update Indicator's layer to NOTIFICATION.. level = %d\n", __func__, __LINE__, bd->client.win, level);
             _policy_change_indicator_layer(indi_bd, bd, POL_NOTIFICATION_LAYER, level);
          }
        else
          {
             _policy_change_indicator_layer(indi_bd, NULL, POL_NOTIFICATION_LAYER, 50);
          }

        return EINA_TRUE;
     }
   else if (indi_show == 0)
     {
        L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... Hide Indicator (by win: 0x%07x)\n", __func__, __LINE__, bd->client.win);
        e_border_hide(indi_bd, 2);
        return EINA_TRUE;
     }
   else
     {
        return EINA_FALSE;
     }
}

static void
_policy_border_indicator_control(E_Border *indi_bd)
{
   Eina_Inlist *xwin_info_list;
   E_Illume_XWin_Info *xwin_info;
   E_Border *bd;
   Ecore_X_Illume_Indicator_Opacity_Mode mode;

   if (!indi_bd) return;

   xwin_info = NULL;
   xwin_info_list = _e_illume_xwin_info_list;

   if (xwin_info_list)
     {
        EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
          {
             if (xwin_info->visibility != E_ILLUME_VISIBILITY_FULLY_OBSCURED)
               {
                  if (xwin_info->bd_info)
                    {
                       bd = xwin_info->bd_info->border;

                       if (!bd) continue;
                       if (!bd->visible) continue;
                       if (indi_bd == bd) continue;
                       if (indi_bd->zone != bd->zone) continue;
                       if (e_illume_border_is_indicator(bd)) continue;
                       if (e_illume_border_is_keyboard(bd)) continue;
                       if (bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) continue;

                       if (!_policy_border_indicator_state_change(indi_bd, bd))
                         continue;

                       mode = ecore_x_e_illume_indicator_opacity_get(bd->client.win);
                       ecore_x_e_illume_indicator_opacity_send(indi_bd->client.win, mode);

                       _policy_root_angle_set(bd);

                       L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... indicator_control_win = 0x%07x...\n", __func__, __LINE__, bd->client.win);
                       g_indi_control_win = bd->client.win;
                       break;
                    }
               }
          }
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

static Eina_Bool
_policy_check_transient_child_visible(E_Border *bd)
{
   Eina_Bool ret = EINA_FALSE;
   E_Illume_XWin_Info *child_xwin_info = NULL;
   Eina_List *l;
   E_Border *child = NULL;

   EINA_LIST_FOREACH(bd->transients, l, child)
     {
        if (!child) continue;
        if (ret) return ret;

        child_xwin_info = _policy_xwin_info_find(child->win);
        if ((child_xwin_info) &&
            (child_xwin_info->visibility == E_ILLUME_VISIBILITY_UNOBSCURED))
          {
             return EINA_TRUE;
          }

        ret = _policy_check_transient_child_visible(child);
     }

   return ret;
}

static void
_policy_calculate_visibility(void)
{
   // 1. CALCULATES window's region and decide it's visibility.
   // 2. DO (UN)ICONIFY if it's needed.
   // 3. SEND notify about visibility.
   //
   E_Zone *zone = NULL;
   E_Border *bd = NULL, *indi_bd = NULL;
   Eina_Inlist *xwin_info_list = NULL;
   E_Illume_XWin_Info *xwin_info = NULL;
   E_Illume_Border_Info *bd_info = NULL;
   Ecore_X_XRegion *visible_region = NULL;
   Ecore_X_XRegion *win_region = NULL;
   Ecore_X_Rectangle visible_rect, win_rect;
   Eina_Bool is_fully_obscured = EINA_FALSE;
   Eina_Bool is_opaque_win = EINA_FALSE;
   Eina_Bool do_not_iconify = EINA_FALSE;
   Eina_Bool alpha_opaque = EINA_FALSE;
   Eina_Bool obscured_by_alpha_opaque = EINA_FALSE;
   int old_vis = 0;
   int set_root_angle = 0;
   int control_indi = 0;

   if (!_g_visibility_changed) return;
   _g_visibility_changed = EINA_FALSE;

   L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. BEGIN calculate visibility ...\n",  __func__, __LINE__);
   xwin_info_list = _e_illume_xwin_info_list;
   if (!xwin_info_list) return;

   // set the entire visible region as a root geometry
   visible_rect.x = 0;
   visible_rect.y = 0;
   visible_rect.width = _g_root_width;
   visible_rect.height = _g_root_height;

   visible_region = ecore_x_xregion_new();
   if (!visible_region)
     {
        L (LT_VISIBILITY_DETAIL,
           "[ILLUME2][VISIBILITY] BAD.... Creating visible region is failed.\n");
        return;
     }

   ecore_x_xregion_union_rect(visible_region, visible_region, &visible_rect);

   EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
     {
        // skip "input only" window
        if (xwin_info->attr.input_only) continue;

        // skip "unmap" window
        if ((xwin_info->viewable == 0) &&
            (xwin_info->iconify_by_wm == 0)) continue;

        if (!xwin_info->is_drawed) continue;

        // initializing variable
        bd_info = NULL;
        bd = NULL;
        is_opaque_win   = EINA_TRUE;
        do_not_iconify = EINA_FALSE;
        old_vis = xwin_info->visibility;

        bd_info = xwin_info->bd_info;
        if (bd_info) bd = bd_info->border;

        // 1. calculates window's region and decide it's visibility.
        if (is_fully_obscured == EINA_FALSE)
          {
             win_rect.x = xwin_info->attr.x;
             win_rect.y = xwin_info->attr.y;
             win_rect.width = xwin_info->attr.w;
             win_rect.height = xwin_info->attr.h;

             // if it stick out or is bigger than the entire visible region,
             // clip it by the entire visible's geometry.
             E_RECTS_CLIP_TO_RECT(win_rect.x, win_rect.y,
                                  win_rect.width, win_rect.height,
                                  visible_rect.x, visible_rect.y,
                                  (int)(visible_rect.width), (int)(visible_rect.height));

             if (ecore_x_xregion_rect_contain(visible_region, &win_rect))
               {
                  L(LT_VISIBILITY_DETAIL, "[ILLUME2][VISIBILITY] %s(%d)... win:0x%07x Un-OBSCURED.. \n", __func__, __LINE__, xwin_info->id);
                  xwin_info->visibility = E_ILLUME_VISIBILITY_UNOBSCURED;

                  if (bd)
                    {
                       if (bd->client.argb)
                         {
                            if (bd_info && bd_info->opaque)
                              {
                                 alpha_opaque = EINA_TRUE;
                              }
                            else
                              {
                                 is_opaque_win = EINA_FALSE;
                              }
                         }

                       if (bd->client.illume.win_state.state ==
                           ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
                         {
                            is_opaque_win = EINA_FALSE;
                         }
                    }
                  else
                    {
                       if (xwin_info->argb)
                         is_opaque_win = EINA_FALSE;
                    }

                  if (is_opaque_win)
                    {
                       win_region = ecore_x_xregion_new();
                       if (win_region)
                         {
                            ecore_x_xregion_union_rect(win_region, win_region, &win_rect);
                            ecore_x_xregion_subtract(visible_region, visible_region, win_region);
                            ecore_x_xregion_free(win_region);
                            win_region = NULL;

                            if (ecore_x_xregion_is_empty(visible_region))
                              {
                                 is_fully_obscured = EINA_TRUE;
                                 if (alpha_opaque)
                                   {
                                      L(LT_VISIBILITY_DETAIL, "[ILLUME2][VISIBILITY] %s(%d)... OBSCURED by alpha opaque win:0x%07x\n", __func__, __LINE__, xwin_info->id);
                                      obscured_by_alpha_opaque = EINA_TRUE;
                                      alpha_opaque = EINA_FALSE;
                                   }
                              }
                         }
                    }
               }
             else
               {
                  L(LT_VISIBILITY_DETAIL, "[ILLUME2][VISIBILITY] %s(%d)... win:0x%07x Fully OBSCURED.. place on OUTSIDE\n", __func__, __LINE__, xwin_info->id);
                  xwin_info->visibility = E_ILLUME_VISIBILITY_FULLY_OBSCURED;
               }
          }
        else
          {
             L(LT_VISIBILITY_DETAIL, "[ILLUME2][VISIBILITY] %s(%d)... win:0x%07x Fully OBSCURED.. \n", __func__, __LINE__, xwin_info->id);
             xwin_info->visibility = E_ILLUME_VISIBILITY_FULLY_OBSCURED;
          }

        if (!bd) continue;
        if (!E_ILLUME_BORDER_IS_IN_MOBILE(bd)) continue;

        // decide if it's the border that DO NOT iconify.
        if (obscured_by_alpha_opaque)
          {
             do_not_iconify = EINA_TRUE;
          }
        // when this border has transient windows,
        // check out this child's visibility.
        // if there is any child window that is UNOBSCURED,
        // DO NOT iconify this border.
        else if (bd->transients)
          {
             do_not_iconify = _policy_check_transient_child_visible(bd);
          }

        // 2. DO (UN)ICONIFY and send visibility notify if it's needed.
        if (old_vis != xwin_info->visibility)
          {
#ifdef USE_DLOG
             SECURE_SLOGD("[WM] SEND VISIBILITY. win:0x%07x (old:%d -> new:%d)", xwin_info->bd_info->border->client.win, old_vis, xwin_info->visibility);
#endif
             L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] SEND VISIBILITY NOTIFY (Line:%d)... win:0x%07x (old:%d -> new:%d)\n", __LINE__, bd->client.win, old_vis, xwin_info->visibility);
             _policy_send_visibility_notify(bd->client.win, xwin_info->visibility);

             if (xwin_info->visibility == E_ILLUME_VISIBILITY_UNOBSCURED)
               {
                  L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. CALL _policy_change_root_angle_by_border_angle!!! win:0x%07x\n", __func__, __LINE__, xwin_info->bd_info->border->client.win);
                  set_root_angle = 1;

                  if (_e_illume_cfg->use_force_iconify)
                    {
                       if (bd->iconic)
                         {
                            L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. Uniconify by illume.. win:0x%07x (parent:0x%07x)\n", __func__, __LINE__, xwin_info->bd_info->border->client.win, xwin_info->bd_info->border->parent ? xwin_info->bd_info->border->parent->client.win:(unsigned int)NULL);
                            _policy_border_force_uniconify(bd);
                         }
                    }
               }
             else if (xwin_info->visibility == E_ILLUME_VISIBILITY_FULLY_OBSCURED)
               {
                  if (bd->client.win == g_rotated_win)
                    {
                       L(LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. g_rotated_win(0x%07x) is obscured.\n", __func__, __LINE__, xwin_info->bd_info->border->client.win);
                       set_root_angle = 1;
                    }

                  if (_e_illume_cfg->use_force_iconify)
                    {
                       if ((!bd->iconic) && (!do_not_iconify))
                         {
                            L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. Iconify by illume.. win:0x%07x (parent:0x%07x)\n", __func__, __LINE__, xwin_info->bd_info->border->client.win, xwin_info->bd_info->border->parent ? xwin_info->bd_info->border->parent->client.win:(unsigned int)NULL);
                            _policy_border_iconify_by_illume(xwin_info);
                         }
                    }
               }

             control_indi = 1;
             zone = xwin_info->bd_info->border->zone;
          }
        else
          {
             if (xwin_info->visibility == E_ILLUME_VISIBILITY_FULLY_OBSCURED)
               {
                  if (_e_illume_cfg->use_force_iconify)
                    {
                       if (bd->parent && bd->parent->iconic)
                         {
                            if ((!bd->iconic) && (!do_not_iconify))
                              {
                                 L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. Iconify by illume.. win:0x%07x (parent:0x%07x)\n", __func__, __LINE__, xwin_info->bd_info->border->client.win, xwin_info->bd_info->border->parent ? xwin_info->bd_info->border->parent->client.win:(unsigned int)NULL);
                                 _policy_border_iconify_by_illume(xwin_info);
                              }
                         }
                       else if (bd->iconic && do_not_iconify)
                         {
                            L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. Uniconify by illume.. win:0x%07x\n", __func__, __LINE__, xwin_info->bd_info->border->client.win);
                            _policy_border_force_uniconify(bd);
                         }
                    }
               }
          }

        // 3. check if opaque window is ocupied the screen.
        // then we reset the obscured_by_alpha_opaque flag
        if (xwin_info->visibility == E_ILLUME_VISIBILITY_FULLY_OBSCURED)
          {
             if (obscured_by_alpha_opaque && is_opaque_win)
               {
                  if (E_CONTAINS(xwin_info->attr.x, xwin_info->attr.y, xwin_info->attr.w, xwin_info->attr.h,
                                 0, 0, _g_root_width, _g_root_height))
                    {
                       L(LT_VISIBILITY_DETAIL, "[ILLUME2][VISIBILITY] %s(%d)... unset obscured_by_alpha_opaque  win:%x\n", __func__, __LINE__, xwin_info->id);
                       obscured_by_alpha_opaque = EINA_FALSE;
                    }
               }
          }
     }

   if (control_indi)
     {
        if (_e_illume_cfg->use_indicator_widget)
          {
             L(LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... Control Root Angle.\n", __func__, __LINE__);
             _policy_border_root_angle_control(zone);
          }
        else
          {
             indi_bd = e_illume_border_indicator_get(zone);
             if (indi_bd)
               {
                  L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... Control Indicator.\n", __func__, __LINE__);
                  _policy_border_indicator_control(indi_bd);
               }
          }
     }

   L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. END calculate visibility ...\n",  __func__, __LINE__);

   if (visible_region) ecore_x_xregion_free(visible_region);
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
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x EXIST in the list...\n", __func__, __LINE__, win);
        return EINA_FALSE;
     }

   xwin_info = (E_Illume_XWin_Info*) calloc (1, sizeof (E_Illume_XWin_Info));
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

   xwin_info->viewable = xwin_info->attr.viewable;

   bd = e_border_find_by_window (win);
   xwin_info->bd_info = _policy_get_border_info(bd);
   xwin_info->argb = ecore_x_window_argb_get (win);

   if (_e_use_comp) xwin_info->comp_vis = 0;
   else xwin_info->comp_vis = 1;

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
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:0x%07x in the list...\n", __func__, __LINE__, win);
        return EINA_FALSE;
     }

   _e_illume_xwin_info_list = eina_inlist_remove(_e_illume_xwin_info_list, EINA_INLIST_GET(xwin_info));
   eina_hash_del(_e_illume_xwin_info_hash, e_util_winid_str_get(xwin_info->id), xwin_info);

   free (xwin_info);

   return EINA_TRUE;
}


void _policy_window_create (Ecore_X_Event_Window_Create *event)
{
   Ecore_X_Window parent;

   parent = ecore_x_window_parent_get(event->win);
   if (parent != ecore_x_window_root_get(event->win))
     return;

   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x...\n", __func__, __LINE__, event->win);

   _policy_xwin_info_add (event->win);
}


void _policy_window_destroy (Ecore_X_Event_Window_Destroy *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x...\n", __func__, __LINE__, event->win);

   _policy_xwin_info_delete (event->win);
}


void _policy_window_reparent (Ecore_X_Event_Window_Reparent *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x...\n", __func__, __LINE__, event->win);

   if (event->parent == ecore_x_window_root_first_get())
      _policy_xwin_info_add (event->win);
   else
      _policy_xwin_info_delete (event->win);
}


void _policy_window_show (Ecore_X_Event_Window_Show *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x...\n", __func__, __LINE__, event->win);

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find (event->win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:0x%07x in the list...\n", __func__, __LINE__, event->win);
        return ;
     }

   xwin_info->viewable = EINA_TRUE;

   if (xwin_info->comp_vis)
     {
        L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
        _g_visibility_changed = EINA_TRUE;
     }
}


void _policy_window_hide (Ecore_X_Event_Window_Hide *event)
{
   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x...\n", __func__, __LINE__, event->win);

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find (event->win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:0x%07x in the list...\n", __func__, __LINE__, event->win);
        return;
     }

   xwin_info->viewable = EINA_FALSE;

   L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
   _g_visibility_changed = EINA_TRUE;
}


void _policy_window_configure (Ecore_X_Event_Window_Configure *event)
{
   Eina_Inlist* l;
   E_Illume_XWin_Info* xwin_info;
   E_Illume_XWin_Info* old_above_xwin_info;
   E_Illume_XWin_Info* new_above_xwin_info;
   E_Illume_XWin_Info* temp_xwin_info;
   E_Illume_XWin_Info* target_xwin_info;
   int check_visibility;
   int changed_size;
   Ecore_X_Window target_win;

   L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. win:0x%07x...\n", __func__, __LINE__, event->win);

   xwin_info = NULL;
   old_above_xwin_info = NULL;
   new_above_xwin_info = NULL;
   check_visibility = 0;
   changed_size = 0;
   target_win = event->win;

   xwin_info = _policy_xwin_info_find (event->win);
   if (xwin_info == NULL)
     {
        L (LT_XWIN, "[ILLUME2][XWIN] %s(%d).. No win:0x%07x in the list...\n", __func__, __LINE__, event->win);
        return;
     }
   target_xwin_info = xwin_info;

   if ((xwin_info->attr.x != event->x) ||
       (xwin_info->attr.y != event->y))
     {
        check_visibility = 1;
     }

   if ((xwin_info->attr.w != event->w) ||
       (xwin_info->attr.h != event->h))
     {
        changed_size = 1;
        check_visibility = 1;
     }

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
                       target_xwin_info = old_above_xwin_info;
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
     {
        if (target_xwin_info->viewable)
          {
             if (target_xwin_info->comp_vis)
               {
                  if (changed_size)
                    {
                       L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. SIZE is changed... target win:0x%07x\n",  __func__, __LINE__, target_xwin_info->id);
                       target_xwin_info->is_drawed = EINA_FALSE;
                       return;
                    }

                  L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
                  _g_visibility_changed = EINA_TRUE;
               }
             else if (target_xwin_info->iconify_by_wm)
               {
                  L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
                  _g_visibility_changed = EINA_TRUE;
               }
          }
     }
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
             if (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
               {
                  if (bd->visible && (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus))
                     _policy_border_focus_top_stack_set (bd);
               }
          }
        else if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE)
          {
             if (_e_illume_cfg->use_force_iconify)
               {
                  if (e->detail == ECORE_X_WINDOW_STACK_BELOW && !e->abovewin)
                    {
                       L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. Request Lower window... win:0x%07x\n", __func__, __LINE__, e->win);
                       E_Illume_XWin_Info *xwin_info = _policy_xwin_info_find(bd->win);
                       if (xwin_info)
                         {
                            L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. ICONIFY win:0x%07x And UN-ICONIFY Top win...\n", __func__, __LINE__, bd->client.win);
                            _policy_border_uniconify_top_border(bd);
                         }
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

void _policy_window_sync_draw_done(Ecore_X_Event_Client_Message* event)
{
   E_Border* bd;
   E_Illume_XWin_Info *xwin_info;
   Ecore_X_Window win;

   win = event->data.l[0];
   bd = e_border_find_by_client_window(win);
   if (!bd) return;

   xwin_info = _policy_xwin_info_find(bd->win);
   if (!xwin_info) return;

   if (!xwin_info->is_drawed)
     {
        if (xwin_info->comp_vis)
          {
             xwin_info->is_drawed = EINA_TRUE;
             L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
             _g_visibility_changed = EINA_TRUE;
          }
        else if (xwin_info->iconify_by_wm)
          {
             xwin_info->is_drawed = EINA_TRUE;
             L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, event->win);
             _g_visibility_changed = EINA_TRUE;
          }
     }
}

/* Setting window mode requires window stack change and window geometry
 * change. But now, the WM can't control these sequential operations
 * using x property set API which whould be able to overwrite previous
 * value before getting x property by the WM.
 * So we changed ecore_x_e_illume_window_state_set function to use x send
 * message and x sync counter. When the WM receives this message,
 * the WM sets window mode and then increases x sync counter.
 *
 * TODO: We need to make a new protocol to set the window mode!!
 */
void _policy_illume_win_state_change_request (Ecore_X_Event_Client_Message *event)
{
   E_Border *bd = NULL;
   Ecore_X_Atom atom = 0, set = 0;
   unsigned int state = 0;
   Ecore_X_Sync_Counter counter = 0;
   long val = 0;

   if (!event) return;

   bd      = e_border_find_by_client_window(event->win);
   atom    = event->data.l[0];
   counter = event->data.l[1];
   val     = event->data.l[2];

   if (atom == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE_NORMAL)
     {
        state = ECORE_X_ILLUME_WINDOW_STATE_NORMAL;
        set = atom;
     }
   else if (atom == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE_FLOATING)
     {
        state = ECORE_X_ILLUME_WINDOW_STATE_FLOATING;
        set = atom;
     }

   ELBF(ELBT_ILLUME, 0, event->win,
        "GET WIN_STATE_CHANGE_REQ bd:0x%08x(%d->%d) counter:0x%08x val:%d",
        bd ? bd->client.win : (unsigned int)NULL,
        bd ? bd->client.illume.win_state.state : 0,
        state,
        counter, val);

   if (bd)
     _policy_border_illume_window_state_change(bd, state);

   if (set != 0)
     {
        ecore_x_window_prop_atom_set(event->win, ECORE_X_ATOM_E_ILLUME_WINDOW_STATE,
                                     &set, 1);
        ELB(ELBT_ILLUME, "SET WIN_STATE", event->win);
     }

   if (counter) ecore_x_sync_counter_inc(counter, 1);
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

static Eina_Bool
_policy_root_angle_set(E_Border *bd)
{
   int angle;
   Ecore_X_Window root;

   if (bd)
     {
        angle = _policy_window_rotation_angle_get(bd->client.win);
        if (angle == -1) return EINA_FALSE;
        if (!(((bd->w == bd->zone->w) && (bd->h == bd->zone->h)) ||
              ((bd->w == bd->zone->h) && (bd->h == bd->zone->w))))
           return EINA_FALSE;

        g_rotated_win = bd->client.win;
        root = bd->zone->container->manager->root;
     }
   else
     {
        angle = g_root_angle;
        g_rotated_win = 0;
        root = 0;
     }

   if (_e_illume_cfg->use_indicator_widget)
     {
        L(LT_INDICATOR, "[ILLUME2][INDICATOR] %s(%d)... indicator_control_win = 0x%07x...\n", __func__, __LINE__, g_rotated_win);
        g_indi_control_win = g_rotated_win;
     }

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. new_win:0x%07x,  old angle:%d -> new_angle:%d\n", __func__, __LINE__, g_rotated_win, g_root_angle, angle);
   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d).. SET ROOT ANGLE... angle:%d\n\n", __func__, __LINE__, angle);
   // set root window property
   ecore_x_window_prop_property_set(root, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &angle, 1);

   return EINA_TRUE;
}

static Eina_Bool
e_illume_border_is_camera(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if (clas == NULL) return EINA_FALSE;
   if (strncmp(clas,"camera",strlen("camera"))!= 0) return EINA_FALSE;
   if (name == NULL) return EINA_FALSE;
   if (strncmp(name,"camera",strlen("camera"))!= 0) return EINA_FALSE;

   return EINA_TRUE;
}

static void
_policy_change_root_angle_by_border_angle (E_Border* bd)
{
   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... win:0x%07x\n", __func__, __LINE__, bd ? (unsigned int)bd->client.win:(unsigned int)NULL);
   if (!bd) return;

   // ignore the angle of special borders - like indicator, keyboard, quickpanel, etc.
   if (e_illume_border_is_indicator(bd)) return;
   if (e_illume_border_is_keyboard(bd)) return;
   if (e_illume_border_is_quickpanel(bd)) return;
   if (e_illume_border_is_quickpanel_popup(bd)) return;

   if (e_illume_border_is_camera(bd))
     {
        if (dep_rot.refer.active_bd == bd)
          {
             // make rotation request for the dependent windows such as quickpanel
             int ang = _policy_window_rotation_angle_get(bd->client.win);
             if (ang == -1) ang = 0;

             if (dep_rot.ang != ang)
               _policy_border_dependent_rotation(bd, ang);
          }
     }

   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... CALL _policy_root_angle_set.. win:0x%07x\n", __func__, __LINE__, bd->client.win);
   _policy_root_angle_set(bd);
}

static void
_policy_indicator_angle_change (E_Border* indi_bd, int angle)
{
   L (LT_ANGLE, "[ILLUME2][ANGLE] %s(%d)... indicator:0x%07x\n", __func__, __LINE__, indi_bd ? (unsigned int)indi_bd->client.win:(unsigned int)NULL);
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
             _policy_layout_quickpanel_rotate (qp, angle);
          }
     }
}

static void
_policy_border_transient_for_group_make(E_Border  *bd,
                                        Eina_List **list)
{
   E_Border *child;
   Eina_List *l;

   if (!bd) return;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->transient.raise)
     {
        EINA_LIST_FOREACH(bd->transients, l, child)
          {
             if (!child) continue;
             if (!child->iconic)
               {
                  *list = eina_list_prepend(*list, child);
                  _policy_border_transient_for_group_make(child, list);
               }
          }
     }
}

static E_Border *
_policy_border_transient_for_border_top_get(E_Border *bd)
{
   E_Border *top_border = NULL;
   Eina_List *transient_list = NULL;

   _policy_border_transient_for_group_make(bd, &transient_list);

   if (transient_list)
     {
        Eina_List *l = NULL;
        E_Border *temp_bd;
        E_Border *temp_bd2;
        E_Border_List *bl;

        bl = e_container_border_list_last(bd->zone->container);
        while ((temp_bd = e_container_border_list_prev(bl)))
          {
             if (top_border) break;
             if (temp_bd == bd) break;

             EINA_LIST_FOREACH(transient_list, l, temp_bd2)
               {
                  if (temp_bd == temp_bd2)
                    {
                       top_border = temp_bd2;
                       break;
                    }
               }
          }
        e_container_border_list_free(bl);
     }

   L (LT_TRANSIENT_FOR, "[ILLUME2][TRANSIENT] %s(%d).. win:0x%07x, transient_for_top_win:0x%07x\n", __func__, __LINE__, bd->client.win, top_border ? (unsigned int)top_border->client.win:(unsigned int)NULL);

   eina_list_free(transient_list);

   return top_border;
}

static void
_policy_border_transient_for_layer_set(E_Border *bd,
                                       E_Border *parent_bd,
                                       int       layer)
{
   int raise;
   E_Border *top_border;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   L (LT_TRANSIENT_FOR, "[ILLUME2][TRANSIENT] %s(%d).. win:0x%07x, transient_for:0x%07x, layer:%d\n", __func__, __LINE__, bd->client.win, parent_bd ? (unsigned int)parent_bd->client.win:(unsigned int)NULL, layer);

   ecore_x_window_shadow_tree_flush();

   raise = e_config->transient.raise;

   bd->saved.layer = bd->layer;
   bd->layer = layer;
   if (e_config->transient.layer)
     {
        Eina_List *l;
        E_Border *child;

        /* We need to set raise to one, else the child wont
         * follow to the new layer. It should be like this,
         * even if the user usually doesn't want to raise
         * the transients.
         */
        e_config->transient.raise = 1;
        EINA_LIST_FOREACH(bd->transients, l, child)
          {
             if (!child) continue;
             child->layer = layer;
          }
     }

   top_border = _policy_border_transient_for_border_top_get(parent_bd);
   if (top_border)
     {
        if (top_border != bd)
          {
             L (LT_STACK, "[ILLUME2][STACK] %s(%d)... STACK CHANGE with ABOVE... win:0x%07x, above_win:0x%07x\n", __func__, __LINE__, bd->client.win, top_border->client.win);
             e_border_stack_above(bd, top_border);
          }
     }
   else
     {
        L (LT_STACK, "[ILLUME2][STACK] %s(%d)... STACK CHANGE with ABOVE... win:0x%07x, above_win:0x%07x\n", __func__, __LINE__, bd->client.win, parent_bd->client.win);
        e_border_stack_above(bd, parent_bd);
     }

   e_config->transient.raise = raise;
}

/* for desktop mode */
static void
_policy_zone_layout_app_single_monitor(E_Illume_Border_Info* bd_info, E_Illume_Config_Zone *cz)
{
   E_Border* bd;
   int layer;
   Eina_Bool resize = EINA_FALSE;
   Eina_Bool move = EINA_FALSE;

   bd = bd_info->border;
   if (!bd)
     {
        fprintf(stderr, "[ILLUME2] fatal error! (%s)  There is no border!\n", __func__);
        return;
     }

   if ((!bd->new_client) && (!bd->visible)) return;

   layer = _policy_zone_layout_app_layer_check(bd);

   if (bd->new_client)
     {
        int zx = 0, zy = 0, zw = 0, zh = 0;
        int new_x, new_y, new_w, new_h;

        if (!bd->client.icccm.request_pos)
          {
             double delta;
             Eina_List *skiplist = NULL;

             if (bd->zone)
               e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

             // calculate width & height
             if (zw > zh)
               delta = zh / 16.0;
             else
               delta = zw / 16.0;

             delta = delta * 0.66;

             new_w = delta * 9.0;
             new_h = delta * 16.0;

             if (zw > new_w)
               new_x = zx + (rand() % (zw - new_w));
             else
               new_x = zx;
             if (zh > new_h)
               new_y = zy + (rand() % (zh - new_h));
             else
               new_y = zy;

             skiplist = eina_list_append(skiplist, bd);

             e_place_zone_region_smart(bd->zone, skiplist,
                                       bd->x, bd->y, new_w, new_h,
                                       &new_x, &new_y);

             eina_list_free(skiplist);
          }
        else
          {
             if (bd->zone)
               e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

             if (zx > bd->x) new_x = zx;
             else new_x = bd->x;

             if (zy > bd->y) new_y = zy;
             else new_y = bd->y;

             if (zw < bd->w) new_w = zw;
             else new_w = bd->w;

             if (zh < bd->h) new_h = zh;
             else new_h = bd->h;
          }

        if ((bd->x != new_x) || (bd->y != new_y))
          move = EINA_TRUE;

        if ((bd->w != new_w) || (bd->h != new_h))
          resize = EINA_TRUE;

        if (move && resize)
          e_border_move_resize(bd, new_x, new_y, new_w, new_h);
        else if (move)
          _policy_border_move(bd, new_x, new_y);
        else if (resize)
          _policy_border_resize(bd, new_w, new_h);
     }
   else
     {
        /* check if user defined position */
        if (bd->client.icccm.request_pos)
          {
             if (bd->client.illume.win_state.state)
               {
                  if (bd_info->resize_req.need_change)
                    {
                       if ((bd->x != bd_info->resize_req.mouse.x) ||
                           (bd->y != bd_info->resize_req.mouse.y))
                         move = EINA_TRUE;

                       if ((bd->w != bd_info->resize_req.mouse.w) ||
                           (bd->h != bd_info->resize_req.mouse.h))
                         resize = EINA_TRUE;

                       if (move && resize)
                         {
                            e_border_move_resize(bd,
                                                 bd_info->resize_req.mouse.x,
                                                 bd_info->resize_req.mouse.y,
                                                 bd_info->resize_req.mouse.w,
                                                 bd_info->resize_req.mouse.h);
                         }
                       else if (move)
                         {
                            _policy_border_move(bd,
                                                bd_info->resize_req.mouse.x,
                                                bd_info->resize_req.mouse.y);
                         }
                       else if (resize)
                         {
                            _policy_border_resize(bd,
                                                  bd_info->resize_req.mouse.w,
                                                  bd_info->resize_req.mouse.h);
                         }

                       bd_info->resize_req.need_change = 0;
                       L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
                    }

                  if (bd_info->resize_req.mouse.down &&
                      bd_info->resize_req.mouse.locked)
                    {
                       if ((bd->x != bd_info->resize_req.mouse.x) ||
                           (bd->y != bd_info->resize_req.mouse.y))
                         _policy_border_move(bd,
                                             bd_info->resize_req.mouse.x,
                                             bd_info->resize_req.mouse.y);
                       L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
                    }
               }
             _policy_zone_layout_app_layer_set(bd, layer);
             return;
          }

        /* resize & move if needed */
        if (bd->client.illume.win_state.state)
          {
             if (bd_info->resize_req.need_change)
               {
                  if ((bd->x != bd_info->resize_req.mouse.x) ||
                      (bd->y != bd_info->resize_req.mouse.y))
                    move = EINA_TRUE;

                  if ((bd->w != bd_info->resize_req.mouse.w) ||
                      (bd->h != bd_info->resize_req.mouse.h))
                    resize = EINA_TRUE;

                  if (move && resize)
                    {
                       e_border_move_resize(bd,
                                            bd_info->resize_req.mouse.x,
                                            bd_info->resize_req.mouse.y,
                                            bd_info->resize_req.mouse.w,
                                            bd_info->resize_req.mouse.h);
                    }
                  else if (move)
                    {
                       _policy_border_move(bd,
                                           bd_info->resize_req.mouse.x,
                                           bd_info->resize_req.mouse.y);
                    }
                  else if (resize)
                    {
                       _policy_border_resize(bd,
                                             bd_info->resize_req.mouse.w,
                                             bd_info->resize_req.mouse.h);
                    }

                  L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
                  bd_info->resize_req.need_change = 0;
               }

             if (bd_info->resize_req.mouse.down &&
                 bd_info->resize_req.mouse.locked)
               {
                  if ((bd->x != bd_info->resize_req.mouse.x) ||
                      (bd->y != bd_info->resize_req.mouse.y))
                    _policy_border_move(bd,
                                        bd_info->resize_req.mouse.x,
                                        bd_info->resize_req.mouse.y);
                  L(LT_AIA, "[ILLUME2][AIA] %s(%d)... bd move resize... (%d, %d, %d, %d)\n", __func__, __LINE__, bd->x, bd->y, bd->w, bd->h);
               }
          }
     }

   /* set layer if needed */
   _policy_zone_layout_app_layer_set(bd, layer);
}

void _policy_window_move_resize_request(Ecore_X_Event_Window_Move_Resize_Request *event)
{
   E_Border *bd;
   E_Illume_Border_Info *bd_info;
   Ecore_X_Event_Window_Move_Resize_Request *e;

   e = event;
   bd = e_border_find_by_client_window(e->win);

   L(LT_AIA, "[ILLUME2][AIA]  %s(%d)... win:0x%07x, x:%d, y:%d, direction:%d, button:%d, source:%d\n", __func__, __LINE__, e->win, e->x, e->y, e->direction, e->button, e->source);
   if (!bd) return;
   if (e->direction == ECORE_X_NETWM_DIRECTION_MOVE ||
       e->direction == ECORE_X_NETWM_DIRECTION_CANCEL)
     return;

   if (bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
     return;

   bd_info = _policy_get_border_info(bd);
   if (!bd_info) return;

   e_border_resize_cancel();

   if (bd_info->resize_req.mouse.down)
     return;

   L(LT_AIA, "[ILLUME2][AIA] %s(%d)... \n", __func__, __LINE__);

   bd_info->resize_req.mouse.down = 1;
   bd_info->resize_req.mouse.dx = bd->x - e->x;
   bd_info->resize_req.mouse.dy = bd->y - e->y;
   bd_info->resize_req.mouse.x = bd->x;
   bd_info->resize_req.mouse.y = bd->y;

   bd->lock_user_location = 1;

   _policy_border_illume_handlers_add(bd_info);
   ecore_x_window_raise(bd->event_win);
   if (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus)
     e_grabinput_get(bd->event_win, 0, bd->event_win);
   else
     e_grabinput_get(bd->event_win, 0, 0);

   bd_info->resize_req.direction = e->direction;
   _policy_resize_start(bd_info);
}

void _policy_window_state_request(Ecore_X_Event_Window_State_Request *event)
{
   E_Border *bd;
   Ecore_X_Event_Window_State_Request *e;
   int i;
   E_Maximize maximize = 0;

   e = event;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return;

   for (i = 0; i < 2; i++)
     {
        switch (e->state[i])
          {
           case ECORE_X_WINDOW_STATE_MAXIMIZED_VERT:
             if (bd->lock_client_maximize) break;
             maximize |= E_MAXIMIZE_VERTICAL;
             break;

           case ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ:
             if (bd->lock_client_maximize) break;
             maximize |= E_MAXIMIZE_HORIZONTAL;
             break;

           default:
             break;
          }
     }

   if (maximize &= E_MAXIMIZE_BOTH)
     {
        if (e->action == ECORE_X_WINDOW_STATE_ACTION_ADD)
          {
             if (bd->pointer && bd->pointer->type)
               e_pointer_type_pop(bd->pointer, bd, bd->pointer->type);
          }
     }
}

static void _policy_border_root_angle_control(E_Zone *zone)
{
   Eina_Inlist *xwin_info_list;
   E_Illume_XWin_Info *xwin_info;
   E_Border *bd;

   xwin_info = NULL;
   xwin_info_list = _e_illume_xwin_info_list;

   if (xwin_info_list)
     {
        EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
          {
             if (xwin_info->visibility != E_ILLUME_VISIBILITY_FULLY_OBSCURED)
               {
                  if (xwin_info->bd_info)
                    {
                       bd = xwin_info->bd_info->border;

                       if (!bd) continue;
                       if (!bd->visible) continue;
                       if (bd->zone != zone) continue;
                       if (e_illume_border_is_indicator(bd)) continue;
                       if (e_illume_border_is_keyboard(bd)) continue;
                       if (e_illume_border_is_quickpanel(bd)) continue;
                       if (e_illume_border_is_quickpanel_popup(bd)) continue;
                       if (bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) continue;

                       if (_policy_root_angle_set(bd)) break;
                    }
               }
          }
     }
}

static int _policy_border_layer_map(int layer)
{
   int pos = 0;

   if (layer < 0) layer = 0;
   pos = 1 + (layer / 50);
   if (pos > POL_NUM_OF_LAYER) pos = POL_NUM_OF_LAYER;
   return pos;
}

static void
_policy_msg_handler(void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata)
{
   E_Manager *man = (E_Manager *)obj;
   E_Manager_Comp_Source *src = (E_Manager_Comp_Source *)msgdata;

   // handle only comp.manager msg
   if (strncmp(name, "comp.manager", sizeof("comp.manager"))) return;

   if (!strncmp(info, "visibility.src", sizeof("visibility.src")))
     {
        E_Illume_XWin_Info *xwin_info;
        Ecore_X_Window win;
        Ecore_X_Window active_win;
        Ecore_X_Window client_win;
        Eina_Bool visible;

        win = e_manager_comp_src_window_get(man, src);

        xwin_info = _policy_xwin_info_find(win);
        if (!xwin_info) return;

        visible = e_manager_comp_src_visible_get(man, src);
        if (visible)
          {
             xwin_info->is_drawed = EINA_TRUE;
             xwin_info->comp_vis = 1;
             L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, win);
             _g_visibility_changed = EINA_TRUE;

             if (xwin_info->bd_info && xwin_info->bd_info->border)
               client_win = xwin_info->bd_info->border->client.win;
             else
               client_win = win;

             active_win = _policy_active_window_get(ecore_x_window_root_get(win));
             if (active_win == client_win)
               {
                  _policy_active_win_change(xwin_info, active_win);
               }
          }
        else
          {
             xwin_info->comp_vis = 0;
          }
     }
}

void _policy_module_update(E_Event_Module_Update *event)
{
   if ((!strncmp(event->name, "comp-tizen", sizeof("comp-tizen"))) ||
       (!strncmp(event->name, "comp", sizeof("comp"))))
     {
        if (event->enabled)
          {
             // set variable
             _e_use_comp = EINA_TRUE;
          }
        else
          {
             // unset variable
             _e_use_comp = EINA_FALSE;

             // change all variable to 1
             Eina_Inlist* xwin_info_list;
             E_Illume_XWin_Info *xwin_info;

             xwin_info_list = _e_illume_xwin_info_list;
             if (xwin_info_list)
               {
                  EINA_INLIST_REVERSE_FOREACH (xwin_info_list, xwin_info)
                    {
                       xwin_info->comp_vis = EINA_TRUE;
                    }
               }
          }
     }
}

void _policy_border_iconify_cb(E_Border *bd)
{
   if (!_e_illume_cfg->use_force_iconify) return;

   if (!bd) return;
   if (e_object_is_del(E_OBJECT(bd))) return;

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find(bd->win);
   if (xwin_info == NULL) return;

   _policy_border_uniconify_below_borders_by_illume(xwin_info);

   if (xwin_info->visibility != E_ILLUME_VISIBILITY_FULLY_OBSCURED)
     {
        int old_vis = xwin_info->visibility;
        xwin_info->visibility = E_ILLUME_VISIBILITY_FULLY_OBSCURED;
        L (LT_VISIBILITY, "[ILLUME2][VISIBILITY] SEND VISIBILITY NOTIFY (Line:%d)... win:0x%07x (old:%d -> new:%d)\n", __LINE__, xwin_info->bd_info->border->client.win, old_vis, xwin_info->visibility);
#ifdef USE_DLOG
        SECURE_SLOGD("[WM] SEND VISIBILITY. win:0x%07x (old:%d -> new:%d)", xwin_info->bd_info->border->client.win, old_vis, xwin_info->visibility);
#endif
        _policy_send_visibility_notify (xwin_info->bd_info->border->client.win, xwin_info->visibility);
     }

   L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... ICONFIY win:0x%07x (iconify_by_wm:%d)\n", __func__, __LINE__, bd->client.win, xwin_info->iconify_by_wm);
   L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, bd->client.win);
   _g_visibility_changed = EINA_TRUE;

   xwin_info->attr.visible = 0;

   _policy_border_focus_top_stack_set(bd);
}

void _policy_border_uniconify_cb(E_Border *bd)
{
   if (!_e_illume_cfg->use_force_iconify) return;

   if (!bd) return;
   if (e_object_is_del(E_OBJECT(bd))) return;

   E_Illume_XWin_Info* xwin_info = _policy_xwin_info_find(bd->win);
   if (xwin_info == NULL) return;

   L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... UNICONIFY win:0x%07x (iconify_by_wm:%d)\n", __func__, __LINE__, bd->client.win, xwin_info->iconify_by_wm);

   xwin_info->iconify_by_wm = 0;
   xwin_info->attr.visible = 1;

   L(LT_VISIBILITY, "[ILLUME2][VISIBILITY] %s(%d).. visibility is changed... win:0x%07x\n",  __func__, __LINE__, bd->client.win);
   _g_visibility_changed = EINA_TRUE;

   _policy_border_focus_top_stack_set(bd);
}

static void
_policy_border_event_border_iconify_free(void *data __UNUSED__,
                                         void      *ev)
{
   E_Event_Border_Iconify *e;

   e = ev;
   //   e_object_breadcrumb_del(E_OBJECT(e->border), "border_iconify_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}


static void
_policy_border_force_iconify(E_Border *bd)
{
   E_Event_Border_Iconify *ev;
   unsigned int iconic;
   E_Illume_XWin_Info* xwin_info;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->shading) return;
   ecore_x_window_shadow_tree_flush();
   if (!bd->iconic)
     {
        bd->iconic = 1;
        e_border_hide(bd, 0);
        if (bd->fullscreen) bd->desk->fullscreen_borders--;
        edje_object_signal_emit(bd->bg_object, "e,action,iconify", "e");
     }
   iconic = 1;
   e_hints_window_iconic_set(bd);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &iconic, 1);

   ev = E_NEW(E_Event_Border_Iconify, 1);
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
   //   e_object_breadcrumb_add(E_OBJECT(bd), "border_iconify_event");
   ecore_event_add(E_EVENT_BORDER_ICONIFY, ev, _policy_border_event_border_iconify_free, NULL);

   xwin_info = _policy_xwin_info_find(bd->win);
   if (xwin_info)
     {
        xwin_info->iconify_by_wm = 1;
     }

   if (e_config->transient.iconify)
     {
        Eina_List *l;
        E_Border *child;
        E_Illume_XWin_Info* child_xwin_info;

        EINA_LIST_FOREACH(bd->transients, l, child)
          {
             if (!e_object_is_del(E_OBJECT(child)))
               {
                  if (!child->iconic)
                    {
                       L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... Iconify by illume.. child:0x%07x\n", __func__, __LINE__, child->client.win);
                       child_xwin_info = _policy_xwin_info_find(child->win);
                       if (child_xwin_info)
                         {
                            _policy_border_iconify_by_illume(child_xwin_info);
                         }
                    }
               }
          }
     }
   e_remember_update(bd);
}

static void
_policy_border_iconify_by_illume(E_Illume_XWin_Info *xwin_info)
{
   E_Border *bd;

   if (!xwin_info) return;
   if (!xwin_info->bd_info) return;
   if (!xwin_info->bd_info->border) return;

   bd = xwin_info->bd_info->border;
   if (!E_ILLUME_BORDER_IS_IN_MOBILE(bd)) return;
   if (e_object_is_del(E_OBJECT(bd))) return;

   if (bd->parent && (!bd->parent->iconic)) return;

   if (e_illume_border_is_indicator(bd)) return;
   if (e_illume_border_is_keyboard(bd)) return;
   if (e_illume_border_is_keyboard_sub(bd)) return;
   if (e_illume_border_is_quickpanel(bd)) return;
   if (e_illume_border_is_quickpanel_popup(bd)) return;
   if (e_illume_border_is_clipboard(bd)) return;
   if (e_illume_border_is_app_tray(bd)) return;
   if (e_illume_border_is_miniapp_tray(bd)) return;
//   if (bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING) return;

   xwin_info->iconify_by_wm = 1;

   if (!bd->iconic)
     {
        L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... FORCE_ICONIFY win:0x%07x\n", __func__, __LINE__, bd->client.win);
        _policy_border_force_iconify(bd);
     }
}

static void
_policy_border_event_border_uniconify_free(void *data, void *ev)
{
   E_Event_Border_Uniconify *e;

   e = ev;
   //   e_object_breadcrumb_del(E_OBJECT(e->border), "border_uniconify_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void _policy_border_force_uniconify(E_Border *bd)
{
   E_Desk *desk;
   E_Event_Border_Uniconify *ev;
   unsigned int iconic;
   E_Illume_XWin_Info* xwin_info;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   xwin_info = _policy_xwin_info_find(bd->win);
   if (!(xwin_info && xwin_info->iconify_by_wm)) return;

   if (bd->shading) return;
   ecore_x_window_shadow_tree_flush();
   e_border_show(bd);
   if (bd->iconic)
     {
        bd->iconic = 0;
        if (bd->fullscreen) bd->desk->fullscreen_borders++;
        if (e_manager_comp_evas_get(bd->zone->container->manager))
          {
             if (bd->await_hide_event > 0)
               bd->await_hide_event--;
          }
        desk = e_desk_current_get(bd->desk->zone);
        e_border_desk_set(bd, desk);
        edje_object_signal_emit(bd->bg_object, "e,action,uniconify", "e");
     }
   iconic = 0;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &iconic, 1);

   ev = E_NEW(E_Event_Border_Uniconify, 1);
   ev->border = bd;
   e_object_ref(E_OBJECT(bd));
   //   e_object_breadcrumb_add(E_OBJECT(bd), "border_uniconify_event");
   ecore_event_add(E_EVENT_BORDER_UNICONIFY, ev, _policy_border_event_border_uniconify_free, NULL);

   if (e_config->transient.iconify)
     {
        Eina_List *l;
        E_Border *child;
        E_Illume_XWin_Info* child_xwin_info;

        EINA_LIST_FOREACH(bd->transients, l, child)
          {
             if (!e_object_is_del(E_OBJECT(child)))
               {
                  if (child->iconic)
                    {
                       child_xwin_info = _policy_xwin_info_find(child->win);
                       if (child_xwin_info) child_xwin_info->iconify_by_wm = 1;

                       L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. FORCE UNICONIFY... win:0x%07x\n", __func__, __LINE__, child->client.win);
                       _policy_border_force_uniconify(child);
                    }
               }
          }
     }
   e_remember_update(bd);
}

static void
_policy_border_uniconify_below_borders_by_illume(E_Illume_XWin_Info *xwin_info)
{
   E_Border *bd;

   if (!xwin_info) return;
   if (!xwin_info->bd_info) return;

   bd = xwin_info->bd_info->border;

   // 1. check if iconify is caused by visibility change or not
   if (xwin_info->iconify_by_wm) return;

   // 2. check if current bd's visibility is fully-obscured or not
   if (xwin_info->visibility == E_ILLUME_VISIBILITY_FULLY_OBSCURED) return;

   // 3-1. find bd's below window and un-iconify it
   L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... UNICONIFY Below Windows of win:0x%07x\n", __func__, __LINE__, bd->client.win);
   _policy_border_uniconify_below_borders(bd);

   if (!e_object_is_del(E_OBJECT(bd)))
     {
        e_border_lower(bd);
     }
}

static E_Border* _policy_border_find_below(E_Border *bd)
{
   Eina_List *l;
   int i, pos;
   Eina_Bool passed;
   E_Illume_XWin_Info* xwin_info;

   passed = EINA_FALSE;

   /* determine layering position */
   pos = _policy_border_layer_map(bd->layer);

   /* Find the windows below this one */
   for (i = pos; i >= 2; i--)
     {
        E_Border *b;

        EINA_LIST_REVERSE_FOREACH(bd->zone->container->layers[i].clients, l, b)
          {
             if (!b) continue;

             /* skip if it's the same border */
             if (b == bd)
               {
                  passed = EINA_TRUE;
                  continue;
               }

             if ((b->x == b->zone->x) &&
                 (b->y == b->zone->y) &&
                 (b->w == b->zone->w) &&
                 (b->h == b->zone->h) &&
                 (b->h == b->zone->h) &&
                 (b->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_FLOATING) &&
                 (b->visible))
               {
                  xwin_info = _policy_xwin_info_find(b->win);
                  if (xwin_info)
                    {
                       if ((!xwin_info->argb) ||
                           (xwin_info->argb && xwin_info->bd_info->opaque))
                         {
                            L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d)... win:0x%07x, ALREADY FULLY-OBSCURED... by win:0x%07x visible:%d (visibility:%d)\n", __func__, __LINE__, bd->client.win, b->client.win, b->visible, xwin_info->visibility);
                            break;
                         }
                    }
               }

             if (!passed) continue;

             /* skip if it's not on this zone */
             if (b->zone != bd->zone) continue;

             /* skip special borders */
             if (e_illume_border_is_indicator(b)) continue;
             if (e_illume_border_is_keyboard(b)) continue;
             if (e_illume_border_is_keyboard_sub(b)) continue;
             if (e_illume_border_is_quickpanel(b)) continue;
             if (e_illume_border_is_quickpanel_popup(b)) continue;
             if (e_illume_border_is_clipboard(b)) continue;
             if (e_illume_border_is_app_tray(b)) continue;
             if (e_illume_border_is_miniapp_tray(b)) continue;

             return b;
          }
     }

   return NULL;
}

static void _policy_border_uniconify_below_borders(E_Border *bd)
{
   E_Border *below_bd;

   below_bd = _policy_border_find_below(bd);
   if (!below_bd) return;

   E_Illume_XWin_Info* below_xwin_info = _policy_xwin_info_find(below_bd->win);
   if (below_xwin_info == NULL) return;

   if ((below_bd->w != below_bd->zone->w) ||
       (below_bd->h != below_bd->zone->h) ||
       (below_xwin_info->argb))
     {
        if (!(below_xwin_info->bd_info && below_xwin_info->bd_info->opaque))
          _policy_border_uniconify_below_borders(below_bd);
     }

   // 3-2.
   if (below_bd->iconic)
     {
        L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. FORCE UNICONIFY... win:0x%07x\n", __func__, __LINE__, below_bd->client.win);
        _policy_border_force_uniconify(below_bd);
     }
}

static void _policy_border_uniconify_top_border(E_Border *bd)
{
   Eina_List *l;
   int i;
   int zone_w, zone_h;
   E_Illume_XWin_Info* xwin_info;

   if (!_e_illume_cfg->use_force_iconify) return;

   zone_w = bd->zone->w;
   zone_h = bd->zone->h;
   /* determine layering position */

   /* Find the windows below this one */
   for (i = POL_NUM_OF_LAYER; i >= 2; i--)
     {
        E_Border *b;

        EINA_LIST_REVERSE_FOREACH(bd->zone->container->layers[i].clients, l, b)
          {
             if (!b) continue;

             if ((b->x >= zone_w) || (b->y >= zone_h)) continue;
             if (((b->x + b->w) <= 0) || ((b->y + b->h) <= 0)) continue;

             /* skip if it's not on this zone */
             if (b->zone != bd->zone) continue;

             /* skip special borders */
             if (e_illume_border_is_indicator(b)) continue;
             if (e_illume_border_is_keyboard(b)) continue;
             if (e_illume_border_is_keyboard_sub(b)) continue;
             if (e_illume_border_is_quickpanel(b)) continue;
             if (e_illume_border_is_quickpanel_popup(b)) continue;
             if (e_illume_border_is_clipboard(b)) continue;
             if (e_illume_border_is_app_tray(b)) continue;
             if (e_illume_border_is_miniapp_tray(b)) continue;

             if (b->iconic)
               {
                  L(LT_ICONIFY, "[ILLUME2][ICONIFY] %s(%d).. FORCE UNICONIFY... win:0x%07x\n", __func__, __LINE__, b->client.win);
                  _policy_border_force_uniconify(b);
               }

             if ((b->x == b->zone->x) &&
                 (b->y == b->zone->y) &&
                 (b->w == b->zone->w) &&
                 (b->h == b->zone->h))
               {
                  xwin_info = _policy_xwin_info_find(b->win);
                  if (xwin_info)
                    {
                       if (xwin_info->argb && !xwin_info->bd_info->opaque)
                         {
                            continue;
                         }
                       else
                         break;
                    }
                  else
                    break;
               }
          }
     }
   return;
}

/* change the desk of window for popsync.
 * if there is no window in data read from ev->l[0],
 * window that is latest above stack will be changed.
 *   - event->data.l[0] : window ID
 *   - event->data.l[1]
 *     1(default): mobile
 *     2: popsync
 */
void
_policy_window_desk_set(Ecore_X_Event_Client_Message *event)
{
   Ecore_X_Event_Client_Message *ev = event;
   Ecore_X_Window win;
   E_Container *con;
   E_Zone *zone;
   E_Desk *desk = NULL;
   E_Border_List *bl;
   E_Border *bd;
   Eina_Bool one_time = EINA_FALSE;
   char *profile = NULL, *new_profile = NULL;
   unsigned int desk_num;
   int x, y;
   int err_tracer = 0;

   EINA_SAFETY_ON_NULL_RETURN(ev);

   win = ev->data.l[0];
   desk_num = ev->data.l[1];
   if (win) one_time = EINA_TRUE;

   // decide new desk's profile from received data.
   switch (desk_num)
     {
      case 2:
         new_profile = _e_illume_cfg->display_name.popsync;
         break;
      case 1:
      default:
         new_profile = _e_illume_cfg->display_name.mobile;
         break;
     }

   con = e_container_current_get(e_manager_current_get());
   bl = e_container_border_list_last(con);

   // if there exists a ID of window read from data ev->data.l[0],
   // find and set this window's border window, or set latest above border.
   bd = win ? e_border_find_by_client_window(win) :
      e_container_border_list_prev(bl);
   if (!bd) bd = e_border_find_by_window(win);
   if (!bd) goto fin;
   err_tracer++;

   do {
        // skip special window.
        if ((e_illume_border_is_indicator(bd)) ||
            (e_illume_border_is_keyboard(bd)) ||
            (e_illume_border_is_keyboard_sub(bd)) ||
            (e_illume_border_is_quickpanel(bd)) ||
            (e_illume_border_is_quickpanel_popup(bd)) ||
            (e_illume_border_is_clipboard(bd)))
          {
             if (one_time) goto fin;
             continue;
          }

        if ((bd->client.icccm.accepts_focus || bd->client.icccm.take_focus) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NOTIFICATION))
          {
             err_tracer++;
             zone = bd->zone;

             // terminated this function if there is only one desk.
             if ((zone->desk_x_count < 2) && (zone->desk_y_count < 2))
               goto fin;
             err_tracer++;

             // find the corresponsive desk.
             for (x = 0; x < zone->desk_x_count; x++)
               {
                  for (y = 0; y < zone->desk_y_count; y++)
                    {
                       int index = x + (y * zone->desk_x_count);
                       profile = zone->desks[index]->window_profile;

                       if (!strcmp(new_profile, profile))
                         desk = zone->desks[index];
                    }
               }

             if (!desk) goto fin;

             e_border_desk_set(bd, desk);

             // we have to receive the DAMAGE notify from this window.
             // for this, the window has to have visible state as a its property.
             // if not, can not receive DAMAGE notify.
             e_hints_window_visible_set(bd);
             e_hints_window_desktop_set(bd);
             _policy_border_uniconify_below_borders(bd);
             break;
          }
   } while((!one_time) && (bd = e_container_border_list_prev(bl)));

fin:
   e_container_border_list_free(bl);
   if (err_tracer)
     {
        fprintf(stderr,
                "[POPSYNC] ERROR: ");
        switch (err_tracer)
          {
           case 0:
              fprintf(stderr, "Couldn't find border.\n");
              break;
           case 1:
              fprintf(stderr, "Couldn't change desk. No border, or special window.\n");
              break;
           case 2:
              fprintf(stderr, "NO desk to change \n");
              break;
           case 3:
              fprintf(stderr, "Couldn't find desk such a profile.\n");
              break;
           default:
              fprintf(stderr, "Non-defined error\n");
          }
     }
   return;
}

/* for supporting rotation */
void
_policy_border_hook_rotation_list_add(E_Border *bd)
{
   int rotation = 0;
   if (!bd) return;

   rotation = bd->client.e.state.rot.curr;
   _policy_border_dependent_rotation(bd, rotation);
}

static void
_policy_border_dependent_rotation(E_Border *bd, int rotation)
{
   Eina_List *l;
   E_Border *dep_bd = NULL;

   if (!bd) return;
   if (!dep_rot.list) return;
   if (dep_rot.refer.active_bd != bd) return;

   EINA_LIST_FOREACH(dep_rot.list, l, dep_bd)
     {
        if (!dep_bd) continue;
        if (_policy_dependent_rotation_check(dep_bd, rotation))
          {
             ELBF(ELBT_ROT, 0, dep_bd->client.win, "ROT_SET ANG [%d -> %d]",
                  dep_bd->client.e.state.rot.curr, rotation);
             e_border_rotation_set(dep_bd, rotation);
          }
     }
   dep_rot.ang = rotation;
}

static Eina_Bool
_policy_dependent_rotation_check(E_Border *bd, int rotation)
{
   Eina_Bool ret = EINA_FALSE;

   if (!bd) return ret;
   if (!bd->visible) return ret;
   if (bd->client.e.state.rot.curr == rotation) return ret;
   ret = EINA_TRUE;

   return ret;
}

static void
_policy_property_indicator_cmd_win_change(Ecore_X_Event_Window_Property *event)
{
   Ecore_X_Window cmd_win;

   cmd_win = _policy_indicator_cmd_win_get(event->win);

   if (!cmd_win)
     {
        ELB(ELBT_ROT, "ERR! NO INDI_CMD_WIN", event->win);
        return;
     }

   if (dep_rot.refer.cmd_win != cmd_win)
     {
        ELBF(ELBT_ROT, 0, cmd_win,
             "INDICATOR COMMAND WIN: [0x%08x -> 0x%08x]",
             dep_rot.refer.cmd_win, cmd_win);
        dep_rot.refer.cmd_win = cmd_win;
     }
}

static void
_policy_property_active_indicator_win_change(Ecore_X_Event_Window_Property *event)
{
   Ecore_X_Window active_win = 0;
   E_Border *bd = NULL;
   int rotation = 0;

   if (!event) return;

   active_win = _policy_active_indicator_win_get(event->win);
   if (!active_win)
     {
        ELB(ELBT_ROT, "ERR! NO ACTIVE_INDI_WIN", event->win);
        return;
     }

   if ((!dep_rot.refer.active_bd) ||
       (dep_rot.refer.active_bd->client.win != active_win))
     {
        bd = e_border_find_by_client_window(active_win);
        if (!bd)
          {
             ELB(ELBT_ROT, "ERR! NO BD ACTIVE_INDI_WIN", active_win);
             return;
          }

        /* The normal application window is ok to rotate dependent rotation windows.
         * But if the notification window which doesn't have accepts_focus such as volume popup
         * is the active window, then the illume doesn't rotate dependent windows.
         */
        Eina_Bool rot = EINA_FALSE;
        if (e_illume_border_is_notification(bd))
          {
            if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
              {
                 rot = EINA_TRUE;
              }
          }
        else
          {
             rot = EINA_TRUE;
          }

        ELBF(ELBT_ROT, 0, bd->client.win,
             "ROT:%d NOTI:%d ACCEPT_FOCUS:%d TAKE_FOCUS:%d",
             rot,
             e_illume_border_is_notification(bd),
             bd->client.icccm.accepts_focus,
             bd->client.icccm.take_focus);

        if (rot)
          {
             ELBF(ELBT_ROT, 0, active_win,
                  "INDICATOR ACTIVE WIN CHANGED:0x%08x", active_win);
             dep_rot.refer.active_bd = bd;
             rotation = bd->client.e.state.rot.curr;

             _policy_border_dependent_rotation(bd, rotation);
          }
     }
}

static Ecore_X_Window
_policy_indicator_cmd_win_get(Ecore_X_Window win)
{
   Ecore_X_Window cmd_win = NULL;
   unsigned char* prop_data = NULL;
   int ret = 0, count = 0;

   if (win != dep_rot.root) return NULL;

   ret = ecore_x_window_prop_property_get(win, E_INDICATOR_CMD_WIN,
                                          ECORE_X_ATOM_WINDOW, 32,
                                          &prop_data, &count);
   if (ret)
     memcpy (&cmd_win, prop_data, sizeof(ECORE_X_ATOM_WINDOW));

   if (prop_data) free(prop_data);

   return cmd_win;
}

static Ecore_X_Window
_policy_active_indicator_win_get(Ecore_X_Window win)
{
   Ecore_X_Window active_win = NULL;
   unsigned char* prop_data = NULL;
   int ret = 0, count = 0;

   if (win != dep_rot.refer.cmd_win) return NULL;

   ret = ecore_x_window_prop_property_get(win, E_ACTIVE_INDICATOR_WIN,
                                          ECORE_X_ATOM_WINDOW, 32,
                                          &prop_data, &count);
   if (ret)
     memcpy (&active_win, prop_data, sizeof(ECORE_X_ATOM_WINDOW));

   if (prop_data) free(prop_data);

   return active_win;
}

static int
_prev_angle_get(Ecore_X_Window win)
{
   int ret, count = 0, ang = -1;
   unsigned char* data = NULL;

   ret = ecore_x_window_prop_property_get
      (win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
       ECORE_X_ATOM_CARDINAL, 32, &data, &count);

   if ((ret) && (data) && (count))
     ang = ((int *)data)[0];
   if (data) free(data);
   return ang;
}


void
_policy_idle_enterer(void)
{
   _policy_calculate_visibility();
}
