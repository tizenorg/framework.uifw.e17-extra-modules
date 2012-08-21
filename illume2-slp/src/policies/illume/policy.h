#ifndef _POLICY_H
# define _POLICY_H

/* define layer values here so we don't have to grep through code to change */
/* layer level 5 (201~) */
# define POL_NOTIFICATION_LAYER 240
# define POL_INDICATOR_LAYER 240

/* layer level 4 (151~200) */
# define POL_QUICKPANEL_LAYER 160

/* layer level 3 (101~150) */
# define POL_STATE_ABOVE_LAYER 150
# define POL_ACTIVATE_LAYER 145
# define POL_FULLSCREEN_LAYER 140
# define POL_DIALOG_LAYER 120
# define POL_SPLASH_LAYER 120
# define POL_SOFTKEY_LAYER 110

/* layer level 2 (51~100) */
# define POL_CLIPBOARD_LAYER 100
# define POL_KEYBOARD_LAYER 100
# define POL_CONFORMANT_LAYER 100
# define POL_APP_LAYER 100
# define POL_HOME_LAYER 90

/* layer level 1 (1~50) */
# define POL_STATE_BELOW_LAYER 50

/* for desktop mode */
typedef enum _E_Illume_Zone_Id
{
   E_ILLUME_ZONE_ID_PHONE = 0,
   E_ILLUME_ZONE_ID_MONITOR = 1,
} E_Illume_Zone_Id;

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

/* for border information */
struct _E_Illume_Border_Info
{
   int pid;
   int level;
   int opaque;
   E_Border* border;
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
void _policy_resize_start(E_Border *bd);
void _policy_resize_end(E_Border *bd);

void _policy_window_focus_in(Ecore_X_Event_Window_Focus_In *event);

void _policy_border_stack_change (E_Border* bd, E_Border* sibling, int stack_mode);
void _policy_border_stack (E_Event_Border_Stack *ev);

void _policy_border_post_new_border(E_Border *bd);
void _policy_border_pre_fetch(E_Border *bd);
void _policy_border_new_border(E_Border *bd);

void _policy_window_configure_request (Ecore_X_Event_Window_Configure_Request *event);

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

void _policy_window_move_resize_request(Ecore_X_Event_Window_Move_Resize_Request *event);
#endif
