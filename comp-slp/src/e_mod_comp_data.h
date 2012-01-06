#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_DATA_H
#define E_MOD_COMP_DATA_H

#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "e_mod_comp_update.h"
#include "e_mod_comp_rotation.h"
//#include "e_mod_comp_animation.h"
#include "config.h"

typedef struct _E_Comp_Transfer E_Comp_Transfer;
typedef enum _E_Comp_Effect_Type E_Comp_Effect_Type;
typedef enum _E_Comp_Win_Type    E_Comp_Win_Type;
typedef struct _E_Comp     E_Comp;
typedef struct _E_Comp_Win E_Comp_Win;

enum _E_Comp_Effect_Type
{
   COMP_EFFECT_DEFAULT,
   COMP_EFFECT_NONE,
   COMP_EFFECT_CUSTOM0,
   COMP_EFFECT_CUSTOM1,
   COMP_EFFECT_CUSTOM2,
   COMP_EFFECT_CUSTOM3,
   COMP_EFFECT_CUSTOM4,
   COMP_EFFECT_CUSTOM5,
   COMP_EFFECT_CUSTOM6,
   COMP_EFFECT_CUSTOM7,
   COMP_EFFECT_CUSTOM8,
   COMP_EFFECT_CUSTOM9
};
enum _E_Comp_Win_Type
{
   WIN_NORMAL,
   WIN_QUICKPANEL,
   WIN_TASK_MANAGER,
   WIN_LIVE_MAGAZINE,
   WIN_LOCK_SCREEN,
   WIN_INDICATOR,
   WIN_ISF_MAIN,
   WIN_ISF_SUB,
   WIN_TOOLTIP,
   WIN_COMBO,
   WIN_DND,
   WIN_DESKTOP,
   WIN_TOOLBAR,
   WIN_MENU,
   WIN_SPLASH,
   WIN_DROP_DOWN_MENU,
   WIN_NOTIFICATION,
   WIN_UTILITY,
   WIN_POPUP_MENU,
   WIN_DIALOG
};

struct _E_Comp
{
   Ecore_X_Window  win;
   Ecore_Evas     *ee;
   Ecore_X_Window  ee_win;
   Evas           *evas;
   E_Manager      *man;
   Eina_Inlist    *wins;
   Eina_List      *wins_list;
   Eina_List      *updates;
   Ecore_Timer    *lock_screen_timeout; // max time between block and unblock screen event
   Ecore_Animator *render_animator;
   Ecore_Job      *update_job;
   Ecore_Timer    *new_up_timer;
   int             animating;
   int             render_overflow;

   E_Manager_Comp  comp;

   Eina_Bool       gl : 1;
   Eina_Bool       grabbed : 1;
   Eina_Bool       nocomp : 1;
   Eina_Bool       wins_invalid : 1;
   Eina_Bool       lock_screen : 1;
   Eina_Bool       screen_rotation : 1;

   int             screen_w;
   int             screen_h;
   int             screen_ang;

   Evas_Object    *bg_img;
   char           *fake_launch_name;
   char           *fake_launch_image;
   Evas_Object    *fake_img_obj;  // fake image object
   Evas_Object    *fake_img_shobj; // fake image shadow object
   Eina_Bool       fake_launch_state: 1;
   int             fake_img_w;
   int             fake_img_h;
   Ecore_Timer    *fake_launch_timeout; // max time between show, hide fake launch
   Eina_Bool       fake_launch_done: 1; // fake launch edje object got effect done or not.
   Ecore_X_Window  fake_win; // this represent fake effect's real window id.

   Eina_Bool       animatable : 1; // if this valuse is true then window can show animaton. otherwise, window can not show animation effect.

   Evas_Object    *fps_bg;
   Evas_Object    *fps_fg;
   double          frametimes[122];
   int             frameskip;

   Eina_Bool       switcher : 1; // task switcher is open
   Eina_Bool       switcher_obscured : 1; //task switcher is obscured
   Eina_Bool       switcher_animating : 1; //task switcher is animating
   int             selected_pos; // selected window's position when task switcher is open
   Eina_List      *transfer_list;
   Eina_Bool       effect_stage : 1;
   Eina_Bool       use_bg_img : 1;
   Evas_Object    *capture_effect_obj; // capture effect's object
};

struct _E_Comp_Win
{
   EINA_INLIST;

   E_Comp               *c; // parent compositor
   Ecore_X_Window        win; // raw window - for menus etc.
   E_Border             *bd; // if its a border - later
   E_Popup              *pop; // if its a popup - later
   E_Menu               *menu; // if it is a menu - later
   int                   x, y, w, h; // geometry
   struct {
      int                x, y, w, h; // hidden geometry (used when its unmapped and re-instated on map)
   } hidden;
   int                   pw, ph; // pixmap w/h
   int                   border; // border width
   Ecore_X_Pixmap        pixmap; // the compositing pixmap
   Ecore_X_Damage        damage; // damage region
   Ecore_X_Visual        vis; // window visual
   int                   depth; // window depth
   Evas_Object          *obj; // composite object
   Evas_Object          *shobj; // shadow object
   Eina_List            *obj_mirror; // extra mirror objects
   Ecore_X_Image        *xim; // x image - software fallback
   E_Update             *up; // update handler
   E_Object_Delfn       *dfn; // delete function handle for objects being tracked
   Ecore_X_Sync_Counter  counter; // sync counter for syncronised drawing
   Ecore_Timer          *update_timeout; // max time between damage and "done" event
   int                   dmg_updates; // num of damage event updates since a redirect
   Ecore_X_Rectangle    *rects; // shape rects... if shaped :(
   int                   rects_num; // num rects above

   E_Comp_Rotation      *rotobj; // for pixmap rotation
   Ecore_Timer          *win_rot_timeout; // timer for window rotation
   int                   req_ang; // request window rotation angle
   int                   cur_ang; // current window rotation angle
   Ecore_X_Pixmap        cache_pixmap; // the cached pixmap (1/nth the dimensions)
   int                   cache_w, cache_h; // cached pixmap size
   int                   update_count; // how many updates have happend to this win
   double                last_visible_time; // last time window was visible
   double                last_draw_time; // last time window was damaged

   int                   pending_count; // pending event count

   unsigned int          opacity;  // opacity set with _NET_WM_WINDOW_OPACITY
   Eina_Bool             use_opacity;

   char                 *title, *name, *clas, *role; // fetched for override-redirect windowa
   Ecore_X_Window_Type   primary_type; // fetched for override-redirect windowa

   Eina_Bool             delete_pending : 1; // delete pendig
   Eina_Bool             hidden_override : 1; // hidden override

   Eina_Bool             animating : 1; // it's busy animating - defer hides/dels
   Eina_Bool             force : 1; // force del/hide even if animating
   Eina_Bool             defer_hide : 1; // flag to get hide to work on deferred hide
   Eina_Bool             delete_me : 1; // delete me!

   Eina_Bool             visible : 1; // is visible
   Eina_Bool             input_only : 1; // is input_only
   Eina_Bool             override : 1; // is override-redirect
   Eina_Bool             argb : 1; // is argb
   Eina_Bool             shaped : 1; // is shaped
   Eina_Bool             update : 1; // has updates to fetch
   Eina_Bool             redirected : 1; // has updates to fetch
   Eina_Bool             shape_changed : 1; // shape changed
   Eina_Bool             native : 1; // native
   Eina_Bool             drawme : 1; // drawme flag fo syncing rendering
   Eina_Bool             invalid : 1; // invalid depth used - just use as marker
   Eina_Bool             nocomp : 1; // nocomp applied
   Eina_Bool             animate_hide : 1 ; // if window animation effect is occured, do hide unrelated window. -> use evas_object_hide()
   Eina_Bool             animatable : 1; // if this valuse is true then window can show animaton. otherwise, window can not show animation effect.
   Eina_Bool             rotating : 1 ; // for rotating
   Eina_Bool             resize_hide : 1; // if window do resize event received, set this valuse true; and check win_update()
   Eina_Bool             ready_win_rot_effect : 1; // for window rotation effect
   Eina_Bool             win_rot_effect : 1; // for window rotation effect
   Eina_Bool             needpix : 1; // need new pixmap
   Eina_Bool             needxim : 1; // need new xim
   Eina_Bool             real_hid : 1; // last hide was a real window unmap
   Eina_Bool             inhash : 1; // is in the windows hash
   Eina_Bool             first_show_worked : 1 ; // check for first show of shobj
   Eina_Bool             show_done : 1 ; // check for show is done
   //Eina_Bool             ready_hide_effect : 1; // check for hide effect is available
   Eina_Bool             effect_stage: 1; // check for if background window is hided or not.

   E_Comp_Effect_Type    show_effect; // indicate show effect type
   E_Comp_Effect_Type    hide_effect; // indicate hide effect type
   E_Comp_Effect_Type    restack_effect; // indicate restack effect type
   E_Comp_Effect_Type    rotation_effect; // indicate rotation effect type
   E_Comp_Effect_Type    focusin_effect; // indicate focus in effect type
   E_Comp_Effect_Type    focusout_effect; // indicate focus out effect type

   Eina_Bool             defer_raise; // flag to defer to raise
   Eina_Bool             defer_move_resize; // flag to defer to move_resize for shobj
   E_Comp_Win_Type       win_type;
   E_Comp_Transfer      *transfer;
};

#if 0
#define DBG(f, x...) printf(f, ##x)
#else
#define DBG(f, x...)
#endif


#define COMP_LOGGER_BUILD_ENABLE 1

#if COMP_LOGGER_BUILD_ENABLE
# define LT_NOTHING     0x0000
# define LT_EVENT_X     0x0001
# define LT_EVENT_BD    0x0002
# define LT_CREATE      0x0004
# define LT_CONFIGURE   0x0008
# define LT_DAMAGE      0x0010
# define LT_DRAW        0x0020
# define LT_SYNC        0x0040
# define LT_EFFECT      0x0080
# define LT_DUMP        0x0100
# define LT_ALL         0xFFFF

# include <stdarg.h>
# define L( type, fmt, args... ) { \
   if( comp_logger_type & type ) printf( fmt, ##args ); \
}
#else
# define L( ... ) { ; }
#endif /* COMP_LOGGER_BUILD_ENABLE */


#endif
#endif
