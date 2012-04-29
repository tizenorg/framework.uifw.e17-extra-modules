#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_SHARED_TYPES_H
#define E_MOD_COMP_SHARED_TYPES_H

typedef struct _E_Comp     E_Comp;
typedef struct _E_Comp_Win E_Comp_Win;

#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_update.h"
#include "e_mod_comp_pixmap_rotation.h"
#include "e_mod_comp_policy.h"
#include "e_mod_comp_animation.h"
#include "e_mod_comp_effect_image_launch.h"
#include "e_mod_comp_effect_screen_capture.h"
#include "e_mod_comp_effect_win_rotation.h"
#include "e_mod_comp_effect_tm.h"
#include "e_mod_comp_effect.h"
#include "e_mod_comp_win_type.h"
#include "e_mod_comp_screen.h"
#include "e_mod_comp_win_shape_input.h"
#include "e_mod_comp_bg_win.h"
#include "e_mod_comp_util.h"

struct _E_Comp
{
   Ecore_X_Window                win; // COW
   Ecore_Evas                   *ee;
   Ecore_X_Window                ee_win; // child window of COW
   Evas                         *evas;
   E_Manager                    *man;
   Eina_Inlist                  *wins;
   Eina_List                    *wins_list;
   Eina_List                    *updates;
   Ecore_Animator               *render_animator;
   Ecore_Job                    *update_job;
   Ecore_Timer                  *new_up_timer;
   int                           animating;
   int                           render_overflow;
   E_Manager_Comp                comp;
   Ecore_X_Window                cm_selection;
   Eina_Bool                     gl : 1;
   Eina_Bool                     grabbed : 1;
   Eina_Bool                     nocomp : 1;
   Eina_Bool                     wins_invalid : 1;
   Eina_Bool                     animatable : 1; // if this value is true then window can show animaton. otherwise, window can not show animation effect.
   Evas_Object                  *fps_bg;
   Evas_Object                  *fps_fg;
   double                        frametimes[122];
   int                           frameskip;
   Eina_Bool                     switcher : 1; // task switcher is open
   Eina_Bool                     switcher_obscured : 1; //task switcher is obscured
   Eina_Bool                     switcher_animating : 1; // task switcher effect is running
   int                           selected_pos; // selected window's position when task switcher is open
   Eina_Bool                     effect_stage : 1;
   Evas_Object                  *bg_img;
   Eina_Bool                     use_bg_img : 1;
   E_Comp_Screen_Lock            lock;
   E_Comp_Screen_Rotation        rotation;
   E_Comp_Effect_Image_Launch   *eff_img; // image launch effect
   E_Comp_Effect_Screen_Capture *eff_cap; // capture effect
   Eina_Bool                     need_shape_merge;
};

struct _E_Comp_Win
{
   EINA_INLIST;

   E_Comp                     *c; // parent compositor
   Ecore_X_Window              win; // raw window - for menus etc.
   E_Border                   *bd; // if its a border - later
   E_Popup                    *pop; // if its a popup - later
   E_Menu                     *menu; // if it is a menu - later
   int                         x, y, w, h; // geometry
   struct {
      int                      x, y, w, h; // hidden geometry (used when its unmapped and re-instated on map)
   } hidden;
   int                         pw, ph; // pixmap w/h
   int                         border; // border width
   Ecore_X_Pixmap              pixmap; // the compositing pixmap
   Ecore_X_Damage              damage; // damage region
   Ecore_X_Visual              vis; // window visual
   int                         depth; // window depth
   Evas_Object                *obj; // composite object
   Evas_Object                *shobj; // shadow object
   Eina_List                  *obj_mirror; // extra mirror objects
   Ecore_X_Image              *xim; // x image - software fallback
   E_Update                   *up; // update handler
   E_Object_Delfn             *dfn; // delete function handle for objects being tracked
   Ecore_X_Sync_Counter        counter; // sync counter for syncronised drawing
   struct {
      int                      version; // version of efl sync
      int                      val; // sync value
      int                      done_count; // draw done event count
   } sync_info;
   Ecore_Timer                *update_timeout; // max time between damage and "done" event
   int                         dmg_updates; // num of damage event updates since a redirect
   Ecore_X_Rectangle          *rects; // shape rects... if shaped :(
   int                         rects_num; // num rects above
   Ecore_X_Pixmap              cache_pixmap; // the cached pixmap (1/nth the dimensions)
   int                         cache_w, cache_h; // cached pixmap size
   int                         update_count; // how many updates have happend to this win
   double                      last_visible_time; // last time window was visible
   double                      last_draw_time; // last time window was damaged
   int                         pending_count; // pending event count
   unsigned int                opacity;  // opacity set with _NET_WM_WINDOW_OPACITY
   char                       *title, *name, *clas, *role; // fetched for override-redirect windowa
   Ecore_X_Window_Type         primary_type; // fetched for override-redirect windowa
   Eina_Bool                   delete_pending : 1; // delete pendig
   Eina_Bool                   hidden_override : 1; // hidden override
   Eina_Bool                   animating : 1; // it's busy animating - defer hides/dels
   Eina_Bool                   force : 1; // force del/hide even if animating
   Eina_Bool                   defer_hide : 1; // flag to get hide to work on deferred hide
   Eina_Bool                   delete_me : 1; // delete me!
   Eina_Bool                   visible : 1; // is visible
   Eina_Bool                   input_only : 1; // is input_only
   Eina_Bool                   override : 1; // is override-redirect
   Eina_Bool                   argb : 1; // is argb
   Eina_Bool                   shaped : 1; // is shaped
   Eina_Bool                   update : 1; // has updates to fetch
   Eina_Bool                   redirected : 1; // has updates to fetch
   Eina_Bool                   shape_changed : 1; // shape changed
   Eina_Bool                   native : 1; // native
   Eina_Bool                   drawme : 1; // drawme flag fo syncing rendering
   Eina_Bool                   invalid : 1; // invalid depth used - just use as marker
   Eina_Bool                   nocomp : 1; // nocomp applied
   Eina_Bool                   animate_hide : 1 ; // if window animation effect is occured, do hide unrelated window. -> use evas_object_hide()
   Eina_Bool                   resize_hide : 1; // if window do resize event received, set this valuse true; and check win_update()
   Eina_Bool                   needpix : 1; // need new pixmap
   Eina_Bool                   needxim : 1; // need new xim
   Eina_Bool                   real_hid : 1; // last hide was a real window unmap
   Eina_Bool                   inhash : 1; // is in the windows hash
   Eina_Bool                   first_show_worked : 1 ; // check for first show of shobj
   Eina_Bool                   show_done : 1 ; // check for show is done
   Eina_Bool                   effect_stage: 1; // check for if background window is hided or not.
   Eina_Bool                   defer_raise; // flag to defer to raise
   Eina_Bool                   defer_move_resize; // flag to defer to move_resize for shobj
   E_Comp_Effect_Type         *eff_type;
   E_Comp_Effect_Win_Rotation *eff_winrot; // image launch effect
   E_Comp_Win_Type             win_type;
   E_Comp_Pixmap_Rotation     *pixrot; // for pixmap rotation
   E_Comp_Transfer            *transfer;
   E_Comp_Win_Shape_Input     *shape_input;
   E_Comp_BG_Win              *bgwin;
   Eina_Bool                   move_lock : 1; // lock / unlock evas_object's move. evas_object represents window.
};

typedef enum _E_Comp_Illume_Window_State
{
   E_COMP_ILLUME_WINDOW_STATE_NORMAL = 0,
   E_COMP_ILLUME_WINDOW_STATE_INSET
} E_Comp_Illume_Window_State;

#endif
#endif
