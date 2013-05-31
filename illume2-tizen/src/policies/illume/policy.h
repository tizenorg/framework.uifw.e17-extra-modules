#ifndef _POLICY_H
# define _POLICY_H

/* for desktop mode
   for decision of border's location among display devices*/
# define E_ILLUME_BORDER_IS_IN_MOBILE(border)      \
   ((_e_illume_cfg->display_name.mobile) &&        \
    (!strncmp((border)->desk->window_profile,      \
              _e_illume_cfg->display_name.mobile,  \
              strlen(_e_illume_cfg->display_name.mobile))))
# define E_ILLUME_BORDER_IS_IN_DESKTOP(border)     \
   ((_e_illume_cfg->display_name.desktop) &&       \
    (!strncmp((border)->desk->window_profile,      \
              _e_illume_cfg->display_name.desktop, \
              strlen(_e_illume_cfg->display_name.desktop))))
# define E_ILLUME_BORDER_IS_IN_POPSYNC(border)     \
   ((_e_illume_cfg->display_name.popsync) &&       \
    (!strncmp((border)->desk->window_profile,      \
              _e_illume_cfg->display_name.popsync, \
              strlen(_e_illume_cfg->display_name.popsync))))


typedef struct _E_Illume_Border_Info E_Illume_Border_Info;
typedef struct _E_Illume_Screen_Lock_Info E_Illume_Screen_Lock_Info;
typedef struct _E_Illume_XWin_Info E_Illume_XWin_Info;

/* for stack change */
typedef enum _E_Illume_Stack_Mode
{
   E_ILLUME_STACK_ABOVE = 0,
   E_ILLUME_STACK_BELOW = 1,
   E_ILLUME_STACK_TOPIF = 2,
   E_ILLUME_STACK_BOTTOMIF = 3,
   E_ILLUME_STACK_OPPOSITE = 4,
} E_Illume_Stack_Mode;

/* for visibility state */
typedef enum _E_Illume_Visibility_State
{
   E_ILLUME_VISIBILITY_UNOBSCURED = 0, // VisibilityUnobscured
   E_ILLUME_VISIBILITY_PARTIALLY_OBSCURED = 1, // VisibilityPartiallyObscured
   E_ILLUME_VISIBILITY_FULLY_OBSCURED = 2, // VisibilityFullyObscured
} E_Illume_Visibility_State;

/* for notification level */
typedef enum _E_Illume_Notification_Level
{
   E_ILLUME_NOTIFICATION_LEVEL_LOW = 50,
   E_ILLUME_NOTIFICATION_LEVEL_NORMAL = 100,
   E_ILLUME_NOTIFICATION_LEVEL_HIGH = 150,
} E_Illume_Notification_Level;

/* for border information */
struct _E_Illume_Border_Info
{
   int pid;
   int level;
   int opaque;
   Ecore_X_Window_Type win_type;
   struct
     {
        unsigned int need_change : 1;
        int          angle;
        int          direction;
        struct
          {
             unsigned char down : 1;
             unsigned char locked : 1;
             unsigned char resize : 1;
             int           x, y, dx, dy;
             int           w, h;
          } mouse;
     } resize_req;
   Eina_List *handlers;
   E_Border *border;
};

/* for screen lock */
struct _E_Illume_Screen_Lock_Info
{
   int is_lock;
   E_Manager* man;
   Eina_List* blocked_list;
   Ecore_Timer* lock_timer;
};

/* for visibility */
struct _E_Illume_XWin_Info
{
   EINA_INLIST;

   Ecore_X_Window id;
   Ecore_X_Window_Attributes attr;
   int argb;
   int visibility;
   Eina_Bool iconify_by_wm;
   Eina_Bool comp_vis;
   Eina_Bool is_drawed;
   Eina_Bool viewable : 1;             // map state
   E_Illume_Border_Info* bd_info;
};

void _policy_border_add(E_Border *bd);
void _policy_border_del(E_Border *bd);
void _policy_border_focus_in(E_Border *bd);
void _policy_border_focus_out(E_Border *bd);
void _policy_border_activate(E_Border *bd);
void _policy_border_post_fetch(E_Border *bd);
void _policy_border_post_assign(E_Border *bd);
void _policy_border_show(E_Border *bd);
void _policy_border_cb_move(E_Border *bd);
void _policy_zone_layout(E_Zone *zone);
void _policy_zone_move_resize(E_Zone *zone);
void _policy_zone_mode_change(E_Zone *zone, Ecore_X_Atom mode);
void _policy_zone_close(E_Zone *zone);
void _policy_drag_start(E_Border *bd);
void _policy_drag_end(E_Border *bd);
void _policy_focus_back(E_Zone *zone);
void _policy_focus_forward(E_Zone *zone);
void _policy_property_change(Ecore_X_Event_Window_Property *event);

void _policy_window_focus_in(Ecore_X_Event_Window_Focus_In *event);

void _policy_border_stack_change (E_Border* bd, E_Border* sibling, int stack_mode);
void _policy_border_stack (E_Event_Border_Stack *ev);
void _policy_border_zone_set(E_Event_Border_Zone_Set *event);

void _policy_border_post_new_border(E_Border *bd);
void _policy_border_pre_fetch(E_Border *bd);
void _policy_border_new_border(E_Border *bd);
#ifdef _F_BORDER_HOOK_PATCH_
void _policy_border_del_border(E_Border *bd);
#endif

void _policy_window_configure_request (Ecore_X_Event_Window_Configure_Request *event);

void _policy_border_iconify_cb(E_Border *bd);
void _policy_border_uniconify_cb(E_Border *bd);

int _policy_init (void);
void _policy_fin (void);

/* for visibility */
void _policy_window_create (Ecore_X_Event_Window_Create *event);
void _policy_window_destroy (Ecore_X_Event_Window_Destroy *event);
void _policy_window_reparent (Ecore_X_Event_Window_Reparent *event);
void _policy_window_show (Ecore_X_Event_Window_Show *event);
void _policy_window_hide (Ecore_X_Event_Window_Hide *event);
void _policy_window_configure (Ecore_X_Event_Window_Configure *event);

void _policy_window_sync_draw_done (Ecore_X_Event_Client_Message* event);
void _policy_quickpanel_state_change (Ecore_X_Event_Client_Message* event);

void _policy_window_desk_set(Ecore_X_Event_Client_Message *event);

void _policy_window_move_resize_request(Ecore_X_Event_Window_Move_Resize_Request *event);
void _policy_window_state_request(Ecore_X_Event_Window_State_Request *event);

void _policy_module_update(E_Event_Module_Update *event);

void _policy_idle_enterer(void);

void _policy_illume_win_state_change_request(Ecore_X_Event_Client_Message *event);

void _policy_border_hook_rotation_list_add(E_Border *bd);
#endif
