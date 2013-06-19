#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_INDICATOR_WIDGET_H
#define E_MOD_MOVE_INDICATOR_WIDGET_H

struct _E_Move_Indicator_Widget
{
   Eina_List      *objs; // list of E_Move_Widget_Object. E_Move_Widget_Object has E_Move_Event.
   Ecore_X_Window  win;
   Eina_Bool       quickpanel_move;
   Eina_Bool       apptray_move;
   Eina_Bool       move_started;
   Evas_Point      pos; // mouse position
   int             input_region_id;
};

typedef struct _E_Move_Indicator_Widget E_Move_Indicator_Widget;

EINTERN void e_mod_move_indicator_widget_apply(void); // find indicator widget's target window and apply indicator widget control
EINTERN void e_mod_move_indicator_widget_set(E_Move_Indicator_Widget *indi_widget); // set current indicator widget
EINTERN E_Move_Indicator_Widget *e_mod_move_indicator_widget_get(void); // get current indicator widget
EINTERN Eina_Bool e_mod_move_indicator_widget_target_window_find(Ecore_X_Window *win); // find indicator widget target window
EINTERN E_Move_Indicator_Widget *e_mod_move_indicator_widget_add(Ecore_X_Window win); // create E_Move_Border related Indicator_Widget
EINTERN void e_mod_move_indicator_widget_del(E_Move_Indicator_Widget *indi_widget); // delete indicator_widget
EINTERN Eina_Bool e_mod_move_indicator_widget_angle_change(Ecore_X_Window win);
EINTERN Eina_Bool e_mod_move_indicator_widget_state_change(Ecore_X_Window win, Eina_Bool state);
EINTERN Eina_Bool e_mod_move_indicator_widget_type_change(Ecore_X_Window win, E_Move_Indicator_Type type);
EINTERN Eina_Bool e_mod_move_indicator_widget_scrollable_check(void);
EINTERN Eina_Bool e_mod_move_indicator_widget_click_get(E_Move_Indicator_Widget* indi_widget);
EINTERN Eina_Bool e_mod_move_indicator_widget_event_clear(E_Move_Indicator_Widget* indi_widget);
EINTERN Eina_Bool e_mod_move_indicator_widget_angle_change_post_job(void);
#endif
#endif
