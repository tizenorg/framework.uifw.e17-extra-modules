#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_MINI_APPTRAY_WIDGET_H
#define E_MOD_MOVE_MINI_APPTRAY_WIDGET_H

struct _E_Move_Mini_Apptray_Widget
{
   Eina_List      *objs; // list of E_Move_Widget_Object. E_Move_Widget_Object has E_Move_Event.
   Ecore_X_Window  win;
   Eina_Bool       mini_apptray_move;
   Evas_Point      pos; // mouse position
   int             input_region_id;
};

typedef struct _E_Move_Mini_Apptray_Widget E_Move_Mini_Apptray_Widget;

EINTERN void e_mod_move_mini_apptray_widget_apply(void); // find mini_apptray widget's target window and apply mini_apptray widget control
EINTERN void e_mod_move_mini_apptray_widget_set(E_Move_Mini_Apptray_Widget *mini_apptray_widget); // set current mini_apptray widget
EINTERN E_Move_Mini_Apptray_Widget *e_mod_move_mini_apptray_widget_get(void); // get current mini_apptray widget
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_target_window_find(Ecore_X_Window *win); // find mini_apptray widget target window
EINTERN E_Move_Mini_Apptray_Widget *e_mod_move_mini_apptray_widget_add(Ecore_X_Window win); // create E_Move_Border related Mini_Apptray_Widget
EINTERN void e_mod_move_mini_apptray_widget_del(E_Move_Mini_Apptray_Widget *mini_apptray_widget); // delete mini_apptray_widget
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_angle_change(Ecore_X_Window win);
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_state_change(Ecore_X_Window win, Eina_Bool state);
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_scrollable_check(void);
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_click_get(E_Move_Mini_Apptray_Widget* mini_apptray_widget);
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_event_clear(E_Move_Mini_Apptray_Widget* mini_apptray_widget);
EINTERN Eina_Bool e_mod_move_mini_apptray_widget_angle_change_post_job(void);
#endif
#endif
