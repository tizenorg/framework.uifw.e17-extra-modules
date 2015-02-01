#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_SHARED_TYPES_H
#define E_MOD_COMP_SHARED_TYPES_H

typedef struct _E_Comp                      E_Comp;
typedef struct _E_Comp_Win                  E_Comp_Win;
typedef struct _E_Comp_Effect_Object        E_Comp_Effect_Object;
typedef struct _E_Comp_Effect_Zone_Rotation E_Comp_Effect_Zone_Rotation;
typedef struct _E_Comp_HWComp               E_Comp_HWComp;

#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_update.h"
#include "e_mod_comp_hw_ov_win.h"
#include "e_mod_comp_canvas.h"
#include "e_mod_comp_object.h"
#include "e_mod_comp_policy.h"
#include "e_mod_comp_effect_win_rotation.h"
#include "e_mod_comp_effect.h"
#include "e_mod_comp_win_type.h"
#include "e_mod_comp_screen.h"
#include "e_mod_comp_win_shape_input.h"
#include "e_mod_comp_bg_win.h"
#include "e_mod_comp_util.h"
#include "e_mod_comp_effect_image_launch.h"
#include "e_mod_comp_hwcomp.h"
#include "e_mod_comp_aux_hint.h"

#ifdef _F_USE_EXTN_DIALOG_
# define e_util_dialog_internal e_util_extn_dialog_show
#endif

//structure for passing values to XRR output property
typedef struct _Data_Rrprop
{
   Ecore_X_Window win;
   unsigned int value;
}Data_Rrprop;

struct _E_Comp
{
   Ecore_X_Window                win; // COW
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
   Eina_List                    *canvases; // list of E_Comp_Canvas
   Eina_Bool                     gl : 1;
   Eina_Bool                     grabbed : 1;
   Eina_Bool                     nocomp : 1;
   Eina_Bool                     wins_invalid : 1;

   // added for tizen
   Eina_Bool                     animatable : 1; // if this value is true then window can show animaton. otherwise, window can not show animation effect.
   Eina_Bool                     effect_stage : 1;
   E_Comp_Screen_Lock            lock;
   E_Comp_Screen_Rotation        rotation;
   Eina_Bool                     need_shape_merge;
   Eina_Bool                     use_hw_ov; // use H/W overlay window for primary screen
   Eina_Bool                     keyboard_effect : 1; // True: Compositor Show Keyborad Window Effect / False: Compositor do not show keyboard window effect
   Eina_Bool                     defer_raise_effect : 1; // True : Compositor defer evas object restack on window effect
   Eina_Bool                     fake_image_launch : 1; // True : Enable Fake Image Launch feature
   E_Comp_Win                   *lower_win; // For saving window on backkey(lower) event

   // fake image launch
   E_Comp_Effect_Image_Launch   *eff_img;
   Evas                         *evas;

   Eina_List                    *shape_inputs;
   Eina_List                    *shape_inputs_mo;

   //effect module
   void *effect_handle;

   struct
     {
        Eina_Bool           (*init)(E_Comp *c);
        void                (*shutdown)(E_Comp *c);

        E_Comp_Effect_Type *(*type_new)(void);
        void                (*type_free)(E_Comp_Effect_Type *type);
        Eina_Bool           (*type_setup)(E_Comp_Effect_Type *type, Ecore_X_Window win);
        Eina_Bool           (*state_setup)(E_Comp_Effect_Type *type, Ecore_X_Window win);
        Eina_Bool           (*state_get)(E_Comp_Effect_Type *type);
        void                (*state_set)(E_Comp_Effect_Type *type, Eina_Bool state);
        Eina_Bool           (*style_setup)(E_Comp_Effect_Type *type, Ecore_X_Window win);
        E_Comp_Effect_Style (*style_get)(E_Comp_Effect_Type *type, E_Comp_Effect_Kind kind);

        /* window effect functions */
        void                (*win_show)(E_Comp_Win *cw);
        void                (*win_hide)(E_Comp_Win *cw);
        void                (*win_restack)(E_Comp_Win *cw, Eina_Bool v1, Eina_Bool v2);

        Eina_Bool           (*signal_add)(E_Comp_Win *cw, Evas_Object *o, const char *emission, const char *src);
        Eina_Bool           (*signal_del)(E_Comp_Win *cw, Evas_Object *obj, const char *name);
        Eina_Bool           (*jobs_clean)(E_Comp_Win *cw, Evas_Object *obj, const char *name);
        Eina_Bool           (*signal_flush)(void);
        Eina_Bool           (*animating_set)(E_Comp *c, E_Comp_Win *cw, Eina_Bool set);

        /* effect object functions */
        E_Comp_Effect_Object *(*object_new)(E_Comp_Layer *ly, E_Comp_Win *cw, Eina_Bool recreate);
        void                  (*object_free)(E_Comp_Effect_Object *o);

        void                  (*object_win_set)(E_Comp_Win *cw, const char *emission);
        void                  (*above_wins_set)(E_Comp_Win *cw, Eina_Bool show);

        /* fake image functions */
        E_Comp_Effect_Image_Launch *(*image_launch_new)(Evas *e, int w, int h);
        void                        (*image_launch_free)(E_Comp_Effect_Image_Launch *eff);
        Eina_Bool                   (*image_launch_handler_message)(Ecore_X_Event_Client_Message *ev);
        Eina_Bool                   (*image_launch_show)(E_Comp_Effect_Image_Launch *eff, const char *file);
        Eina_Bool                   (*image_launch_hide)(E_Comp_Effect_Image_Launch *eff);
        Eina_Bool                   (*image_launch_window_check)(E_Comp_Effect_Image_Launch *eff, E_Comp_Win *cw);
        Eina_Bool                   (*image_launch_running_check)(E_Comp_Effect_Image_Launch *eff);
        Eina_Bool                   (*image_launch_fake_show_done_check)(E_Comp_Effect_Image_Launch *eff);
        Eina_Bool                   (*image_launch_window_set)(E_Comp_Effect_Image_Launch *eff, Ecore_X_Window w);
        void                        (*image_launch_disable)(E_Comp_Effect_Image_Launch *eff);

        /* rotation functions */
        Eina_Bool                    (*win_angle_get)(E_Comp_Win *cw);
        E_Comp_Effect_Zone_Rotation *(*zone_rotation_new)(E_Comp_Canvas *canvas);
        void                         (*zone_rotation_free)(E_Comp_Effect_Zone_Rotation *zr);
        Eina_Bool                    (*zone_rotation_begin)(E_Comp_Effect_Zone_Rotation *zr);
        Eina_Bool                    (*zone_rotation_end)(E_Comp_Effect_Zone_Rotation *zr);
        Eina_Bool                    (*zone_rotation_cancel)(E_Comp_Effect_Zone_Rotation *zr);
        Eina_Bool                    (*zone_rotation_do)(E_Comp_Effect_Zone_Rotation *zr);
        Eina_Bool                    (*zone_rotation_clear)(E_Comp_Effect_Zone_Rotation *zr);
     }effect_funcs;
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
   struct
   {
      int                      x, y; // temporary geometry (for save geometry from window was resized to rendering was finished)
   } resizing;
   struct
   {
      int                      x, y, w, h; // hidden geometry (used when its unmapped and re-instated on map)
   } hidden;
   int                         pw, ph; // pixmap w/h
   int                         border; // border width
   Ecore_X_Pixmap              pixmap; // the compositing pixmap
   Ecore_X_Damage              damage; // damage region
   Ecore_X_Visual              vis; // window visual
   int                         depth; // window depth
   Eina_List                  *objs; // list of E_Comp_Object
   E_Update                   *up; // update handler
   E_Object_Delfn             *dfn; // delete function handle for objects being tracked
   Ecore_X_Sync_Counter        counter; // sync counter for syncronised drawing
   Ecore_Timer                *update_timeout; // max time between damage and "done" event
   Ecore_Timer                *ready_timeout;  // max time on show (new window draw) to wait for window contents to be ready if sync protocol not handled. this is fallback.
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
   unsigned char               misses; // number of sync misses
   struct {
      int                      version; // version of efl sync
      int                      val; // sync value
      int                      done_count; // draw done event count
   } sync_info;
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
   Eina_Bool                   drawme : 1; // drawme flag fo syncing rendering
   Eina_Bool                   invalid : 1; // invalid depth used - just use as marker
   Eina_Bool                   nocomp : 1; // nocomp applied
   Eina_Bool                   needpix : 1; // need new pixmap
   Eina_Bool                   real_hid : 1; // last hide was a real window unmap
   Eina_Bool                   inhash : 1; // is in the windows hash
   Eina_Bool                   show_ready : 1;  // is this window ready for its first show
   Eina_Bool                   show_anim : 1; // ran show animation
   Eina_Bool                   use_dri2 : 1;

   Eina_Bool                   animate_hide : 1 ; // if window animation effect is occured, do hide unrelated window. -> use evas_object_hide()
   Eina_Bool                   resize_hide : 1; // if window do resize event received, set this valuse true; and check win_update()
   Eina_Bool                   show_done : 1 ; // check for show is done
   Eina_Bool                   effect_stage: 1; // check for if background window is hided or not.
   Eina_Bool                   defer_raise; // flag to defer to raise
   Eina_Bool                   activate; // flag to check activate
   E_Comp_Effect_Type         *eff_type;
   E_Comp_Win_Type             win_type;
   E_Comp_Win_Shape_Input     *shape_input;
   E_Comp_BG_Win              *bgwin;
   int                         angle; // window's current angle property
   Eina_Bool                   launched : 1; //flag for checking whether launch or not

   // ov object
   Ecore_X_Image              *ov_xim;
   Evas_Object                *ov_obj;
   struct
   {
      int                      x, y, w, h;
   } ov;

   /* transparent rectangle for showing under the H/W ui layer. i.e., video layer */
   struct
   {
      Eina_Bool                use;
      int                      x, y, w, h;
   } transp_rect;

   struct
   {
      Eina_Bool                set_drawable;
      int                      geo_changed;
      int                      resize_pending;
   } hwc;

   // interfacing for 3d stereoscopic mode
   Eina_Bool                   win3d; // flag to set 3D window
};

#endif
#endif
