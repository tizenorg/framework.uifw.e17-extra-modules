#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_SHARED_TYPES_H
#define E_MOD_MOVE_SHARED_TYPES_H

typedef struct _E_Move                               E_Move;
typedef struct _E_Move_Border                        E_Move_Border;
typedef enum _E_Move_Animation_Direction             E_Move_Animation_Direction;
typedef enum _E_Move_Visibility_State                E_Move_Visibility_State;
typedef enum _E_Move_Indicator_State                 E_Move_Indicator_State;
typedef enum _E_Move_Indicator_Type                  E_Move_Indicator_Type;
typedef enum _E_Move_Fullscreen_Indicator_Show_State E_Move_Fullscreen_Indicator_Show_State;
typedef enum _E_Move_Angle                           E_Move_Angle;
typedef enum _E_Move_Mini_Apptray_State              E_Move_Mini_Apptray_State;

#include "e.h"
#include "e_mod_main.h"
#include "e_mod_move_canvas.h"
#include "e_mod_move_event.h"
#include "e_mod_move_object.h"
#include "e_mod_move_control_object.h"
#include "e_mod_move_border_type.h"
#include "e_mod_move_border_shape_input.h"
#include "e_mod_move_border_contents.h"
#include "e_mod_move_util.h"
#include "e_mod_move_indicator.h"
#include "e_mod_move_apptray.h"
#include "e_mod_move_mini_apptray.h"
#include "e_mod_move_quickpanel.h"
#include "e_mod_move_lockscreen.h"
#include "e_mod_move_taskmanager.h"
#include "e_mod_move_pwlock.h"
#include "e_mod_move_setup_wizard.h"
#include "e_mod_move_flick.h"
#include "e_mod_move_dim_object.h"
#include "e_mod_move_evas_object.h"
#include "e_mod_move_widget_object.h"
#include "e_mod_move_indicator_widget.h"
#include "e_mod_move_mini_apptray_widget.h"

enum _E_Move_Visibility_State
{
   E_MOVE_VISIBILITY_STATE_NONE = 0,
   E_MOVE_VISIBILITY_STATE_VISIBLE,
   E_MOVE_VISIBILITY_STATE_FULLY_OBSCURED
};

enum _E_Move_Indicator_State
{
   E_MOVE_INDICATOR_STATE_NONE = 0,
   E_MOVE_INDICATOR_STATE_ON,
   E_MOVE_INDICATOR_STATE_OFF
};

enum _E_Move_Indicator_Type
{
   E_MOVE_INDICATOR_TYPE_NONE = 0,
   E_MOVE_INDICATOR_TYPE_1,
   E_MOVE_INDICATOR_TYPE_2
};

enum _E_Move_Fullscreen_Indicator_Show_State
{
   E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_NONE = 0,
   E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_ON,
   E_MOVE_FULLSCREEN_INDICATOR_SHOW_STATE_OFF
};

enum _E_Move_Angle
{
   E_MOVE_ANGLE_0 = 0,
   E_MOVE_ANGLE_90,
   E_MOVE_ANGLE_180,
   E_MOVE_ANGLE_270
};

enum _E_Move_Animation_Direction
{
   E_MOVE_ANIMATION_DIRECTION_NONE = 0,
   E_MOVE_ANIMATION_DIRECTION_INSIDE,
   E_MOVE_ANIMATION_DIRECTION_OUTSIDE
};

enum _E_Move_Mini_Apptray_State
{
   E_MOVE_MINI_APPTRAY_STATE_NONE = 0,
   E_MOVE_MINI_APPTRAY_STATE_ON,
   E_MOVE_MINI_APPTRAY_STATE_OFF
};

struct _E_Move
{
   E_Manager                   *man;
   Eina_Inlist                 *borders;
   Eina_List                   *borders_list;
   Eina_Bool                    animating;
   Eina_List                   *canvases; // list of E_Comp_Canvas

   struct {
      double portrait;
      double landscape;
   } indicator_always_region_ratio; // indicator's always region ratio

   struct {
      double portrait;
      double landscape;
   } indicator_quickpanel_region_ratio; // indicator's always region ratio

   struct {
      double portrait;
      double landscape;
   } indicator_apptray_region_ratio; // indicator's always region ratio

   struct {
      double speed;
      double angle;
      double distance;
      double distance_rate;
   } flick_limit; // indicator / quickpanel / apptray flick limit check

   Eina_Bool                           qp_scroll_with_visible_win : 1;
   Eina_Bool                           qp_scroll_with_clipping : 1;
   double                              animation_duration; // apptray / quickpanel move animation duration
   int                                 dim_max_opacity; // dim max opacity
   int                                 dim_min_opacity; // dim min opacity
   Eina_Bool                           elm_indicator_mode : 1; // 1: indicator widget mode / 0: indicator window mode
   E_Move_Indicator_Widget            *indicator_widget; // indicator widget data ( it contains widget object, internal data)
   E_Move_Mini_Apptray_Widget         *mini_apptray_widget; // mini_apptray widget data ( it contains widget object, internal data)
   Eina_Bool                           screen_reader_state : 1; // screen reader state  enabled  or disabled
   Eina_Bool                           setup_wizard_state : 1; // setup wizard state  enabled  or disabled
   int                                 screen_input_block_id; // if id exists, that means screen's input is blocked.

   struct {
      int x;
      int y;
      int w;
      int h;
   } indicator_widget_geometry[4]; //indicator widget's per angle geometry. [0]: angle 0, [1]: angle 90, [2]: angle 180, [3]: angle 270

   Eina_Bool                    apptray_launch_by_flickup : 1; // 1: apptray launch by flick up feature enable. 0: apptray launch by flick up feature disable
   struct {
      int x;
      int y;
      int w;
      int h;
   } mini_apptray_widget_geometry[4]; //mini_apptray widget's per angle geometry. [0]: angle 0, [1]: angle 90, [2]: angle 180, [3]: angle 270

};

struct _E_Move_Border
{
   EINA_INLIST;
   E_Move                                *m; // parent move structure
   E_Border                              *bd; // E17's Border
   Ecore_X_Window                         client_win; // it represents border's client win, it is used when bd is destroyed.
   int                                    x, y, w, h; // geometry
   Eina_List                             *objs; // list of E_Move_Object. ( it represts Compositor's Shadow Object or Mirror Object)
   Eina_List                             *ctl_objs; // list of E_Move_Control_Object. E_Move_Control_Object has E_Move_Event.
   Eina_Bool                              visible : 1; // is visible. if border is visible, Object could move.
   Eina_Bool                              argb : 1; // is argb
   E_Move_Border_Type                     type;
   E_Move_Border_Shape_Input             *shape_input; // it reprents window's input shape mask info
   E_Move_Border_Contents                *contents; // it reprents window's contents region info
   int                                    angle; // window's current angle property
   void                                  *data; // E_Move_Border's internal data ( it could be quickpanel, indicator apptray's internal data.)
   void                                  *anim_data; // E_Move_Border's internal animation data
   E_Move_Flick_Data                     *flick_data;  // E_Move_Object's flick related data
   E_Object_Delfn                        *dfn; // delete function handle for objects being tracked
   Eina_Bool                              inhash : 1; // is in the windows hash
   Eina_Bool                              animate_move : 1; // Quickpanel UX related window check.
   E_Move_Visibility_State                visibility; // Client Window's Visibility
   E_Move_Indicator_State                 indicator_state; // Client Window's Indicator Property State
   E_Move_Indicator_Type                  indicator_type; // Client Window's Indicator Type Property
   E_Move_Fullscreen_Indicator_Show_State fullscreen_indicator_show_state; // Client Window's Full Screen Indicator Show Property State
   E_Move_Panel_Scrollable_State          panel_scrollable_state; // panel (apptray / quickpanel / etc.) 's scrollable state
   E_Move_Mini_Apptray_State              mini_apptray_state; // Client Window's Mini Apptray Property State
};

#endif
#endif
