#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_SPLIT_MODE_INDICATOR_WIDGET_H
#define E_MOD_MOVE_SPLIT_MODE_INDICATOR_WIDGET_H

typedef struct _E_Move_Split_Mode_Indicator_Widget E_Move_Split_Mode_Indicator_Widget;
typedef struct _E_Move_Split_Mode_Indicator_Animation_Data E_Move_Split_Mode_Indicator_Animation_Data;

struct _E_Move_Split_Mode_Indicator_Widget
{
   Eina_List                                  *objs; // list of E_Move_Widget_Object. E_Move_Widget_Object has E_Move_Event.
   Eina_List                                  *plugin_objs; // list of Evas_Plugin_Object.
   Eina_List                                  *indi_bg_objs; // list of Evas_Object. (Indicator's Background)
   Ecore_X_Window                              win;
   Eina_Bool                                   plugin_objs_show; // first flick makes plugin_objs visible.
   Eina_Bool                                   apptray_move;
   Eina_Bool                                   quickpanel_move;
   Eina_Bool                                   move_started;
   Eina_Bool                                   event_forwarding_off;
   Evas_Point                                  pos; // mouse position
   int                                         input_region_id;
   Ecore_Timer                                *plugin_objs_auto_hide_timer; // plugin indicator object auto hide timer
   int                                         angle; // split mode indicator's angle
   E_Move_Split_Mode_Indicator_Animation_Data *anim_data; // indicator show/hide animation data

   struct {
     int x;
     int y;
     int w;
     int h;
   } plugin_objs_geometry;
};

EINTERN void e_mod_move_split_mode_indicator_widget_apply(void); // find indicator widget's target window and apply indicator widget control
EINTERN void e_mod_move_split_mode_indicator_widget_set(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget); // set current split_mode_indicator widget
EINTERN E_Move_Split_Mode_Indicator_Widget *e_mod_move_split_mode_indicator_widget_get(void); // get current split_mode_indicator widget
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_target_window_find(Ecore_X_Window *win); // find split_mode_indicator widget target window
EINTERN E_Move_Split_Mode_Indicator_Widget *e_mod_move_split_mode_indicator_widget_add(Ecore_X_Window win); // create E_Move_Border related Indicator_Widget
EINTERN void e_mod_move_split_mode_indicator_widget_del(E_Move_Split_Mode_Indicator_Widget *split_mode_indi_widget); // delete indicator_widget
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_angle_change(Ecore_X_Window win);
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_state_change(Ecore_X_Window win, Eina_Bool state);
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_scrollable_check(void);
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_click_get(E_Move_Split_Mode_Indicator_Widget* split_mode_indi_widget);
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_event_clear(E_Move_Split_Mode_Indicator_Widget* split_mode_indi_widget);
EINTERN Eina_Bool e_mod_move_split_mode_indicator_widget_angle_change_post_job(void);
EINTERN void      e_mod_move_split_mode_indicator_widget_zone_rot_change(void);
EINTERN void      e_mod_move_split_mode_indicator_widget_event_forward_off_set(Eina_Bool set);
#endif
#endif
