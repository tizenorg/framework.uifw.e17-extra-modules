#ifndef E_ILLUME_H
# define E_ILLUME_H

#ifdef HAVE_GETTEXT
#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)
#else
#define _(str) (str)
#define d_(str, dom) (str)
#endif

/**
 * @file e_illume.h
 *
 * This header provides the various defines, structures and functions that
 * make writing Illume policies easier.
 *
 * For details on the available functions, see @ref E_Illume_Main_Group.
 *
 * For details on the configuration structure, see @ref E_Illume_Config_Group.
 *
 * For details on the Policy API, see @ref E_Illume_Policy_Group.
 *
 * For details on quickpanels, see @ref E_Illume_Quickpanel_Group.
 */

/* include standard E header */
# include "e.h"

/* define all of our typedefs */
typedef struct _E_Illume_Policy_Api E_Illume_Policy_Api;
typedef struct _E_Illume_Policy E_Illume_Policy;
typedef struct _E_Illume_Config E_Illume_Config;
typedef struct _E_Illume_Config_Zone E_Illume_Config_Zone;
typedef struct _E_Illume_Quickpanel E_Illume_Quickpanel;
typedef struct _E_Illume_Quickpanel_Info E_Illume_Quickpanel_Info;

typedef enum _E_Illume_Quickpanel_Ani_Type
{
   E_ILLUME_QUICKPANEL_ANI_NONE,
   E_ILLUME_QUICKPANEL_ANI_SHOW,
   E_ILLUME_QUICKPANEL_ANI_HIDE
} E_Illume_Quickpanel_Ani_Type;

/**
 * @def E_ILLUME_POLICY_API_VERSION
 * @brief Current version of the Policy API that is supported by the Illume
 * module.
 *
 * @warning Policies not written to match this version will fail to load.
 *
 * @ingroup E_Illume_Policy_Group
 */
# define E_ILLUME_POLICY_API_VERSION 2

/**
 * @brief structure for policy API.
 *
 * @details When Illume tries to load a policy, it will check for the
 * existince of this structure. If it is not found, the policy will fail
 * to load.
 *
 * @warning This structure is required for Illume to load a policy.
 *
 * @ingroup E_Illume_Policy_Group
 */
struct _E_Illume_Policy_Api
{
   int version;
   /**< The version of this policy. */

   const char *name;
   /**< The name of this policy. */
   const char *label;
   /**< The label of this policy that will be displayed in the Policy Selection dialog. */
};

/**
 * @brief structure for policy
 *
 * This structure actually holds the policy functions that Illume will call
 * at the appropriate times.
 *
 * @ingroup E_Illume_Policy_Group
 */
struct _E_Illume_Policy
{
   E_Object e_obj_inherit;

   E_Illume_Policy_Api *api;
   /**< pointer to the @ref E_Illume_Policy_Api structure.
    * @warning Policies are required to implement this or they will fail to
    * load. */

   void *handle;

   struct
     {
        void *(*init) (E_Illume_Policy *p);
        /**< pointer to the function that Illume will call to initialize this
         * policy. Typically, a policy would set the pointers to the functions
         * that it supports in here.
         * @warning Policies are required to implement this function. */

        int (*shutdown) (E_Illume_Policy *p);
        /**< pointer to the function that Illume will call to shutdown this
         * policy. Typically, a policy would do any cleanup that it needs to
         * do in here.
         * @warning Policies are required to implement this function. */

        void (*border_add) (E_Border *bd);
        /**< pointer to the function that Illume will call when a new border
         * gets added. @note This function is optional. */

        void (*border_del) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets
         * deleted. @note This function is optional. */

        void (*border_focus_in) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets
         * focus. @note This function is optional. */

        void (*border_focus_out) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border loses
         * focus. @note This function is optional. */

        void (*border_activate) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets
         * an activate message. @note This function is optional. */

        void (*border_post_fetch) (E_Border *bd);
        /**< pointer to the function that Illume will call when E signals a
         * border post fetch. @note This function is optional. */

        void (*border_post_assign) (E_Border *bd);
        /**< pointer to the function that Illume will call when E signals a
         * border post assign. @note This function is optional. */

        void (*border_show) (E_Border *bd);
        /**< pointer to the function that Illume will call when a border gets
         * shown. @note This function is optional. */

        void (*border_move) (E_Border *bd);

        void (*zone_layout) (E_Zone *zone);
        /**< pointer to the function that Illume will call when a Zone needs
         * to update it's layout. @note This function is optional. */

        void (*zone_move_resize) (E_Zone *zone);
        /**< pointer to the function that Illume will call when a Zone gets
         * moved or resized. @note This function is optional. */

        void (*zone_mode_change) (E_Zone *zone, Ecore_X_Atom mode);
        /**< pointer to the function that Illume will call when the layout
         * mode of a Zone changes. @note This function is optional. */

        void (*zone_close) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has
         * requested a border get closed. This is usually signaled from the
         * Softkey window. @note This function is optional. */

        void (*drag_start) (E_Border *bd);
        /**< pointer to the function that Illume will call when the user has
         * started to drag the Indicator/Softkey windows.
         * @note This function is optional. */

        void (*drag_end) (E_Border *bd);
        /**< pointer to the function that Illume will call when the user has
         * stopped draging the Indicator/Softkey windows.
         * @note This function is optional. */

        void (*focus_back) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has
         * requested to cycle the focused border backwards. This is typically
         * signalled from the Softkey window.
         * @note This function is optional. */

        void (*focus_forward) (E_Zone *zone);
        /**< pointer to the function that Illume will call when the user has
         * requested to cycle the focused border forward. This is typically
         * signalled from the Softkey window.
         * @note This function is optional. */

        void (*property_change) (Ecore_X_Event_Window_Property *event);
        /**< pointer to the function that Illume will call when properties
         * change on a window. @note This function is optional. */

        void (*window_focus_in) (Ecore_X_Event_Window_Focus_In *event);

        void (*border_restack_request) (E_Border* bd, E_Border* sibling, int mode);
        void (*border_stack) (E_Event_Border_Stack* ev);
        void (*border_zone_set) (E_Event_Border_Zone_Set *event);

        void (*border_post_new_border) (E_Border *bd);

        void (*border_pre_fetch) (E_Border *bd);
        void (*border_new_border) (E_Border *bd);
#ifdef _F_BORDER_HOOK_PATCH_
        void (*border_del_border) (E_Border *bd);
#endif
        void (*window_configure_request) (Ecore_X_Event_Window_Configure_Request *event);

        void (*border_iconify_cb) (E_Border *bd);
        void (*border_uniconify_cb) (E_Border *bd);

        /* for visibility */
        void (*window_create) (Ecore_X_Event_Window_Create *event);
        void (*window_destroy) (Ecore_X_Event_Window_Destroy *event);
        void (*window_reparent) (Ecore_X_Event_Window_Reparent *event);
        void (*window_show) (Ecore_X_Event_Window_Show *event);
        void (*window_hide) (Ecore_X_Event_Window_Hide *event);
        void (*window_configure) (Ecore_X_Event_Window_Configure *event);

        void (*window_sync_draw_done) (Ecore_X_Event_Client_Message *event);
        void (*quickpanel_state_change) (Ecore_X_Event_Client_Message *event);

        /* for popsync feature */
        void (*window_desk_set) (Ecore_X_Event_Client_Message *event);

        void (*window_move_resize_request) (Ecore_X_Event_Window_Move_Resize_Request *event);
        void (*window_state_request) (Ecore_X_Event_Window_State_Request *event);

        void (*module_update) (E_Event_Module_Update *event);

        void (*idle_enterer) (void);

        void (*illume_win_state_change_request) (Ecore_X_Event_Client_Message *event);
     } funcs;
};


/**
 * @brief structure for Illume configuration.
 *
 * @ingroup E_Illume_Config_Group
 */
struct _E_Illume_Config
{
   int version;

   struct
     {
        struct
          {
             int duration;
             /**< integer specifying the amount of time it takes for an
              * animation to complete. */
          } vkbd, quickpanel;
     } animation;

   struct
     {
        const char *name;
        /**< the name of the currently active/selected policy. */
        struct
          {
             const char *class;
             /**< The window class to match on */
             const char *name;
             /**< The window name to match on */
             const char *title;
             /**< The window title to match on */
             int type;
             /**< The window type to match on */
             struct
               {
                  int class;
                  /**< flag to indicate if we should match on class */
                  int name;
                  /**< flag to indicate if we should match on name */
                  int title;
                  /**< flag to indicate if we should match on title */
                  int type;
                  /**< flag to indicate if we should match on type */
               } match;
          } vkbd, indicator, softkey, home;
        Eina_List *zones;
     } policy;

   struct
     {
        const char *mobile;
        /**< the name of the type of window to be located mobile. */
        const char *desktop;
        /**< the name of the type of window to be located desktop. */
        const char *popsync;
        /**< the name of the type of window to be located popsync. */
     } display_name;

   Eina_Bool use_mem_trim;
   Eina_Bool use_indicator_widget;
   Eina_Bool use_force_iconify;
   int floating_control_threshold;
};

/**
 * @brief structure for Illume zone configuration.
 *
 * @ingroup E_Illume_Config_Group
 */
struct _E_Illume_Config_Zone
{
   int id;
   struct
     {
        int dual;
        /**< integer specifying whice mode we are in (0 == single application mode, 1 == dual application mode) */
        int side;
        /**< interger specifying if we layout windows in top/bottom or left/right when in dual mode */
     } mode;

   /* NB: These are not configurable by user...just placeholders */
   struct
     {
        int size;
     } vkbd, indicator, softkey;
};

struct _E_Illume_Quickpanel_Info
{
   E_Border *bd;
   int angle;

   /* for mini controller */
   Eina_Bool mini_controller;
};

/**
 * @brief structure for Illume Quickpanels.
 *
 * @ingroup E_Illume_Quickpanel_Group
 */
struct _E_Illume_Quickpanel
{
   E_Object e_obj_inherit;

   E_Zone *zone;
   /**< the current zone on which this quickpanel belongs */
   E_Border *bd;
   /**< the border of this quickpanel */
   Eina_List *borders;
   /**< a list of borders that this quickpanel contains */
   Ecore_Timer *timer;
   Ecore_Animator *animator;
   Ecore_Event_Handler *key_hdl;
   double start, len;
   struct
     {
        int size, isize, adjust, adjust_start, adjust_end, dir;
     } vert, horiz;

   int angle;
   int ani_type;

   unsigned char visible;
   /**< flag to indicate if the quickpanel is currently visible */

   E_Win *popup;
   Ecore_Evas *ee;
   Evas *evas;

   Evas_Coord down_x, down_y;
   int down_adjust;
   Eina_Bool dragging : 1;
   Eina_Bool down : 1;
   Eina_Bool horiz_style : 1;

   Evas_Object *ly_base;
   Eina_Bool hide_trigger;

   int popup_len;
   int item_pos_y;
   int threshold_y, move_x_min;
   double scale;
   E_Border *ind;

   Eina_Bool is_lock;
   Eina_List *hidden_mini_controllers;

   struct
     {
        unsigned char layer :1;
     } changes;
   unsigned char changed :1;

   unsigned int layer;

   E_Border *below_bd;
};


/* define function prototypes that policies can use */
EAPI E_Illume_Config_Zone *e_illume_zone_config_get(int id);

/* general functions */
EAPI Eina_Bool e_illume_border_is_indicator(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_keyboard(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_keyboard_sub(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_dialog(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_splash(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_qt_frame(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_fullscreen(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_conformant(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_quickpanel(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_lock_screen(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_notification(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_utility(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_clipboard(E_Border *bd);

EAPI void e_illume_border_min_get(E_Border *bd, int *w, int *h);
EAPI void e_illume_border_max_get(E_Border *bd, int *w, int *h);
EAPI E_Border *e_illume_border_at_xy_get(E_Zone *zone, int x, int y);
EAPI E_Border *e_illume_border_parent_get(E_Border *bd);
EAPI void e_illume_border_show(E_Border *bd);
EAPI void e_illume_border_hide(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_fixed(E_Border *bd);

/* indicator functions */
EAPI E_Border *e_illume_border_indicator_get(E_Zone *zone);
EAPI void e_illume_border_indicator_pos_get(E_Zone *zone, int *x, int *y);

/* quickpanel functions */
EAPI E_Illume_Quickpanel *e_illume_quickpanel_by_zone_get(E_Zone *zone);
EAPI Eina_List* e_illume_quickpanel_get (void);
EAPI void e_illume_quickpanel_show(E_Zone *zone, int isAni);
EAPI void e_illume_quickpanel_hide(E_Zone *zone, int isAni);
EAPI Eina_Bool e_illume_border_is_quickpanel_popup(E_Border *bd);

/* app tray functions */
EAPI Eina_Bool e_illume_border_is_app_tray(E_Border *bd);
EAPI Eina_Bool e_illume_border_is_miniapp_tray(E_Border *bd);
#endif
