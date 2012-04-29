#ifndef E_MOD_DATA_H
#define E_MOD_DATA_H

#include "e_mod_move_event.h"
#include "e_mod_move_win_type.h"

typedef struct _E_Move_Win E_Move_Win;
typedef struct _E_Move E_Move;
typedef struct _E_Move_AppTray_Helper E_Move_AppTray_Helper;
typedef struct _E_Move_Key_Helper E_Move_Key_Helper;
typedef struct _E_Move_Block_Helper E_Move_Block_Helper;
typedef struct _E_Move_Border E_Move_Border;
typedef struct _E_Move_TP_Indi E_Move_TP_Indi;
typedef struct _E_Move_Active_Win E_Move_Active_Win;

typedef enum
        {
           MOVE_OFF_STAGE = 0,
           MOVE_ON_STAGE  = 1,
           MOVE_WORK      = 2,
        } E_Move_State;

typedef enum
        {
           LAYOUT_POSITION_X,
           LAYOUT_POSITION_Y
        } QuickPanel_Layout_Position;

struct _E_Move_AppTray_Helper
{
   Ecore_X_Window win;
   Eina_Bool visible;  // app_tray_helper window's state ( show or hide )

   struct geometry
   {
      int x;
      int y;
      int w;
      int h;
   } geo;

   struct _ecore_event
   {
      Ecore_Event_Handler_Cb up; // mouse up callback function
      Ecore_Event_Handler_Cb down; // mouse move callback function
      Ecore_Event_Handler_Cb move; // mouse down callback function
      Eina_Bool              click;
      Eina_Bool              enable;
      Ecore_Event_Handler   *up_handler;
      Ecore_Event_Handler   *down_handler;
      Ecore_Event_Handler   *move_handler;
   } event;

   Evas_Object *obj;
};

struct _E_Move_Key_Helper
{
   Eina_Bool state;  // enable or disable
   Ecore_X_Window win;
   Ecore_Event_Handler *key_down_handler;
};

struct _E_Move_Block_Helper
{
   Eina_Bool state;  // enable or disable
   Ecore_X_Window win;
   Evas_Object  *obj;
};

struct _E_Move_Border
{
    EINA_INLIST;
    E_Border       *bd;
    E_Move_Win_Type win_type;
};

struct _E_Move_TP_Indi
{
   Evas_Object  *obj;
   E_Move_Event *event;
   E_Border     *target_bd;

   Eina_Bool    mouse_clicked;
   struct _input_shape
   {
      int x;
      int y;
      int w;
      int h;
   } input_shape;
   struct _touch
   {
      Eina_Bool visible;
      Ecore_X_Window win;
      Ecore_Event_Handler_Cb ev_up;
      Ecore_Event_Handler   *ev_up_handler;
      Evas_Object *obj;
   } touch;
};

struct _E_Move_Active_Win {
   Ecore_X_Window win;
   E_Border *bd;
   Evas_Object *obj;
   Eina_Bool animatable;
};

struct _E_Move_Win
{
   EINA_INLIST;

   E_Move *m;

   struct _quickpanel_layout
   {
      int position_x;
      int position_y;
   } quickpanel_layout;

   struct _indicator_layout
   {
      int portrait_apptray_input_region_width;
      int landscape_apptray_input_region_height;
   } indicator_layout;

   struct _event_handler
   {
      Evas_Object_Event_Cb  win_mouse_up_cb;
      Evas_Object_Event_Cb  win_mouse_move_cb;
      Evas_Object_Event_Cb  win_mouse_down_cb;
      Evas_Object_Event_Cb  obj_restack_cb;
      Evas_Object_Event_Cb  obj_del_cb;
      Eina_Bool             mouse_clicked;
      int                   window_rotation_angle;
   } event_handler;

   struct _shape_input_mask
   {
      int input_region_x;
      int input_region_y;
      int input_region_w;
      int input_region_h;
   } shape_input_region;

   Evas_Object  *move_object;
   E_Border     *border;

   E_Move_Win_Type win_type;

   struct _window_geometry
   {
      int x;
      int y;
      int w;
      int h;
   } window_geometry;

   E_Move_State state;
   E_Move_Event *event;
   Eina_Bool move_lock;
   Eina_Bool can_unlock;
};

struct _E_Move
{
   Eina_Inlist *wins;
   Eina_Inlist *borders;
   E_Manager   *man;
   E_Move_Win  *quickpanel;
   Eina_Bool   rotation_lock;

   struct _quickpanel_child
   {
      Eina_List *quickpanel_transients;
   } quickpanel_child;

   E_Move_AppTray_Helper *apptray_helper;
   E_Move_Win            *app_tray;

   E_Move_Win            *indicator;

   E_Move_Key_Helper *key_helper;

   E_Move_Block_Helper *block_helper;

   E_Move_State state;

   E_Move_Active_Win active_win;

   E_Move_TP_Indi *tp_indi;
};
#endif // E_MOD_DATA_H
