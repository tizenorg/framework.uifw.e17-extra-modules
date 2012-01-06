//////////////////////////////////////////////////////////////////////////
//
// TODO (no specific order):
//   1. abstract evas object and compwin so we can duplicate the object N times
//      in N canvases - for winlist, everything, pager etc. too
//   2. implement "unmapped composite cache" -> N pixels worth of unmapped
//      windows to be fully composited. only the most active/recent.
//   3. for unmapped windows - when window goes out of unmapped comp cachew
//      make a miniature copy (1/4 width+height?) and set property on window
//      with pixmap id
//   8. obey transparent property
//   9. shortcut lots of stuff to draw inside the compositor - shelf,
//      wallpaper, efm - hell even menus and anything else in e (this is what
//      e18 was mostly about)
//  10. fullscreen windows need to be able to bypass compositing *seems buggy*
//
//////////////////////////////////////////////////////////////////////////

/* Local Include */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "e_mod_comp_update.h"
#include "e_mod_comp_rotation.h"
#include "e_mod_comp_data.h"
#include "e_mod_comp_animation.h"
#include "config.h"

/* Global Include */
#include <utilX.h>
#include <X11/Xlib.h>
#include <dlog.h>

/* debug infos */
#if COMP_LOGGER_BUILD_ENABLE
typedef struct _Comp_Logger Comp_Logger;
struct _Comp_Logger
{
    int logger_type;
    char logger_dump_file[256];
};
int comp_logger_type                              = LT_NOTHING;
#define STR_ATOM_CM_LOG                           "_E_COMP_LOG"
Ecore_X_Atom ATOM_CM_LOG                          = 0;
#define STR_ATOM_CM_LOG_DUMP_DONE                 "_E_COMP_LOG_DUMP_DONE"
Ecore_X_Atom ATOM_CM_LOG_DUMP_DONE                = 0;
#endif

/* Macros */
#define OVER_FLOW 4
#define _WND_REQUEST_ANGLE_IDX 0
#define _WND_CURR_ANGLE_IDX    1
#define STR_ATOM_FAKE_LAUNCH_IMAGE                 "_E_COMP_FAKE_LAUNCH_IMAGE"
#define STR_ATOM_FAKE_LAUNCH                       "_E_COMP_FAKE_LAUNCH"
#define STR_ATOM_NET_CM_EFFECT_ENABLE              "_NET_CM_EFFECT_ENABLE"
#define STR_ATOM_NET_CM_WINDOW_EFFECT_ENABLE       "_NET_CM_WINDOW_EFFECT_ENABLE"
#define STR_ATOM_NET_CM_WINDOW_EFFECT_CLIENT_STATE "_NET_CM_WINDOW_EFFECT_CLIENT_STATE"
#define STR_ATOM_NET_CM_WINDOW_EFFECT_STATE        "_NET_CM_WINDOW_EFFECT_STATE"
#define STR_ATOM_NET_CM_WINDOW_EFFECT_TYPE         "_NET_CM_WINDOW_EFFECT_TYPE"
#define STR_ATOM_NET_CM_EFFECT_DEFAULT             "_NET_CM_EFFECT_DEFAULT"
#define STR_ATOM_NET_CM_EFFECT_NONE                "_NET_CM_EFFECT_NONE"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM0             "_NET_CM_EFFECT_CUSTOM0"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM1             "_NET_CM_EFFECT_CUSTOM1"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM2             "_NET_CM_EFFECT_CUSTOM2"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM3             "_NET_CM_EFFECT_CUSTOM3"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM4             "_NET_CM_EFFECT_CUSTOM4"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM5             "_NET_CM_EFFECT_CUSTOM5"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM6             "_NET_CM_EFFECT_CUSTOM6"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM7             "_NET_CM_EFFECT_CUSTOM7"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM8             "_NET_CM_EFFECT_CUSTOM8"
#define STR_ATOM_NET_CM_EFFECT_CUSTOM9             "_NET_CM_EFFECT_CUSTOM9"
#define STR_ATOM_OVERAY_WINDOW                     "_E_COMP_OVERAY_WINDOW"
#define STR_ATOM_X_HIBERNATION_STATE               "X_HIBERNATION_STATE"
#define STR_ATOM_X_SCREEN_ROTATION                 "X_SCREEN_ROTATION"
#define STR_ATOM_CM_LOCK_SCREEN                    "_E_COMP_LOCK_SCREEN"
#define STR_ATOM_CM_PIXMAP_ROTATION_SUPPORTED      "_E_COMP_PIXMAP_ROTATION_SUPPORTED"
#define STR_ATOM_CM_PIXMAP_ROTATION_STATE          "_E_COMP_PIXMAP_ROTATION_STATE"
#define STR_ATOM_CM_PIXMAP_ROTATION_BEGIN          "_E_COMP_PIXMAP_ROTATION_BEGIN"
#define STR_ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE     "_E_COMP_PIXMAP_ROTATION_BEGIN_DONE"
#define STR_ATOM_CM_PIXMAP_ROTATION_END            "_E_COMP_PIXMAP_ROTATION_END"
#define STR_ATOM_CM_PIXMAP_ROTATION_END_DONE       "_E_COMP_PIXMAP_ROTATION_END_DONE"
#define STR_ATOM_CM_PIXMAP_ROTATION_REQUEST        "_E_COMP_PIXMAP_ROTATION_REQUEST"
#define STR_ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE   "_E_COMP_PIXMAP_ROTATION_REQUEST_DONE"
#define STR_ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP  "_E_COMP_PIXMAP_ROTATION_RESIZE_PIXMAP"
#define STR_ATOM_CAPTURE_EFFECT                    "_E_COMP_CAPTURE_EFFECT"

/* static global variables */
static Eina_List *handlers = NULL;
static Eina_List *compositors = NULL;
static Eina_Hash *windows = NULL;
static Eina_Hash *borders = NULL;
static Eina_Hash *damages = NULL;
static E_Comp    *_comp = NULL;

/* global variables */
Ecore_X_Atom ATOM_FAKE_LAUNCH                     = 0;
Ecore_X_Atom ATOM_FAKE_LAUNCH_IMAGE               = 0;
Ecore_X_Atom ATOM_EFFECT_ENABLE                   = 0;
Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE            = 0;
Ecore_X_Atom ATOM_WINDOW_EFFECT_CLIENT_STATE      = 0;
Ecore_X_Atom ATOM_WINDOW_EFFECT_STATE             = 0;
Ecore_X_Atom ATOM_WINDOW_EFFECT_TYPE              = 0;
Ecore_X_Atom ATOM_EFFECT_DEFAULT                  = 0;
Ecore_X_Atom ATOM_EFFECT_NONE                     = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM0                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM1                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM2                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM3                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM4                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM5                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM6                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM7                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM8                  = 0;
Ecore_X_Atom ATOM_EFFECT_CUSTOM9                  = 0;
Ecore_X_Atom ATOM_OVERAY_WINDOW                   = 0;
Ecore_X_Atom ATOM_X_HIBERNATION_STATE             = 0;
Ecore_X_Atom ATOM_X_SCREEN_ROTATION               = 0;
Ecore_X_Atom ATOM_CM_LOCK_SCREEN                  = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_SUPPORTED    = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_STATE        = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_BEGIN        = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE   = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_END          = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_END_DONE     = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_REQUEST      = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE = 0;
Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP= 0;
Ecore_X_Atom ATOM_CAPTURE_EFFECT                  = 0;

/* static functions */
static void               _e_mod_comp_set                             (E_Comp *c);
E_Comp                  * _e_mod_comp_get                             (void);
static void               _e_mod_comp_render_queue                    (E_Comp *c);
static void               _e_mod_comp_win_damage                      (E_Comp_Win *cw, int x, int y, int w, int h, Eina_Bool dmg);
static void               _e_mod_comp_win_del                         (E_Comp_Win *cw);
static void               _e_mod_comp_win_real_hide                   (E_Comp_Win *cw);
static void               _e_mod_comp_win_hide                        (E_Comp_Win *cw);
static void               _e_mod_comp_win_configure                   (E_Comp_Win *cw, int x, int y, int w, int h, int border);
static void               _e_mod_comp_screen_lock                     (E_Comp *c);
static void               _e_mod_comp_screen_unlock                   (E_Comp *c);
static Eina_Bool          _e_mod_comp_screen_lock_timeout             (void *data);
static void               _e_mod_comp_screen_lock_func                (void *data, E_Manager *man __UNUSED__);
static void               _e_mod_comp_screen_unlock_func              (void *data, E_Manager *man __UNUSED__);
static Eina_Bool          _e_mod_comp_win_rotation_begin_timeout      (void *data);
static Eina_Bool          _e_mod_comp_win_rotation_end_timeout        (void *data);
static Eina_Bool          _e_mod_comp_win_damage_timeout              (void *data);
static void               _e_mod_comp_win_raise                       (E_Comp_Win *cw);
static void               _e_mod_comp_win_lower                       (E_Comp_Win *cw);
static void               _e_mod_comp_win_unredirect                  (E_Comp_Win *cw);
static E_Comp_Win       * _e_mod_comp_win_find                        (Ecore_X_Window win);
static Eina_Bool          _e_mod_comp_win_add_damage                  (E_Comp_Win *cw, Ecore_X_Damage damage);
static Eina_Bool          _e_mod_comp_win_del_damage                  (E_Comp_Win *cw, Ecore_X_Damage damage);
static E_Comp_Win       * _e_mod_comp_border_client_find              (Ecore_X_Window win);
static void               _e_mod_comp_win_recreate_shobj              (E_Comp_Win *cw);
static void               _e_mod_comp_win_cb_setup                    (E_Comp_Win *cw);
static Eina_Bool          _e_mod_comp_is_next_win_stack               (E_Comp_Win *cw, E_Comp_Win *cw2);
static Eina_Bool          _e_mod_comp_get_edje_error                  (E_Comp_Win *cw);
static void               _e_mod_comp_atom_init                       (void);
static Eina_Bool          _e_mod_comp_cb_update                       (E_Comp *c);
static Ecore_X_Window     _e_mod_comp_win_get_client_xid              (E_Comp_Win *cw);
static Eina_Bool          _e_mod_comp_win_is_border                   (E_Comp_Win *cw);

/* non-static functions */
void                      _e_mod_comp_done_defer                      (E_Comp_Win *cw);
void                      _e_mod_comp_cb_pending_after                (void *data __UNUSED__, E_Manager *man __UNUSED__, E_Manager_Comp_Source *src);
E_Comp                  * _e_mod_comp_find                            (Ecore_X_Window root);
Eina_Bool                 _e_mod_comp_fake_launch_timeout             (void *data);
void                      _e_mod_comp_win_render_queue                (E_Comp_Win *cw);
void                      _e_mod_comp_send_window_effect_client_state (E_Comp_Win *cw, Eina_Bool effect_state);
Eina_Bool                 _e_mod_comp_win_check_visible               (E_Comp_Win *cw);
Eina_Bool                 _e_mod_comp_win_check_visible3              (E_Comp_Win *cw);
E_Comp_Win              * _e_mod_comp_win_transient_parent_find       (E_Comp_Win * cw);
E_Comp_Win              * _e_mod_comp_win_find_background_win         (E_Comp_Win * cw);
E_Comp_Win              * _e_mod_comp_win_find_by_indicator           (E_Comp *c);
E_Comp_Win              * _e_mod_comp_win_find_fake_background_win    (E_Comp *c);
void                      _e_mod_comp_disable_touch_event_block       (E_Comp *c);
void                      _e_mod_comp_enable_touch_event_block        (E_Comp *c);
void                      _e_mod_comp_win_inc_animating               (E_Comp_Win *cw);
void                      _e_mod_comp_win_dec_animating               (E_Comp_Win *cw);
void                      ecore_x_e_comp_dri_buff_flip_supported_set  (Ecore_X_Window root, Eina_Bool enabled); // TODO

/* extern functions */
extern Eina_Bool          _e_mod_comp_is_quickpanel_window            (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_task_manager_window          (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_live_magazine_window         (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_lock_screen_window           (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_indicator_window             (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_isf_main_window              (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_isf_sub_window               (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_normal_window                (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_tooltip_window               (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_combo_window                 (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_dnd_window                   (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_desktop_window               (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_toolbar_window               (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_menu_window                  (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_splash_window                (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_drop_down_menu_window        (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_notification_window          (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_utility_window               (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_popup_menu_window            (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_dialog_window                (E_Border *bd);
extern void               _e_mod_comp_window_effect_policy            (E_Comp_Win *cw);
extern Eina_Bool          _e_mod_comp_window_restack_policy           (E_Comp_Win *cw, E_Comp_Win *cw2);
extern Eina_Bool          _e_mod_comp_window_rotation_policy          (E_Comp_Win *cw);
extern int                _e_mod_comp_shadow_policy                   (E_Comp_Win *cw);
extern void               _e_mod_comp_window_show_effect              (E_Comp_Win *cw);
extern void               _e_mod_comp_window_hide_effect              (E_Comp_Win *cw);
extern void               _e_mod_comp_window_restack_effect           (E_Comp_Win *cw, E_Comp_Win *cw2);
extern void               _e_mod_comp_disable_effect_stage            (E_Comp *c);
extern Eina_Bool          _e_mod_comp_fake_show                       (Ecore_X_Event_Client_Message *ev);
extern Eina_Bool          _e_mod_comp_fake_hide                       (Ecore_X_Event_Client_Message *ev);
extern void               _e_mod_comp_disable_fake_launch             (E_Comp *c);
extern E_Comp_Effect_Type _e_mod_comp_get_effect_type                 (Ecore_X_Atom *atom);
extern void               _e_mod_comp_window_lower_effect             (E_Comp_Win *cw, E_Comp_Win *cw2);

Eina_Bool
_e_mod_comp_capture_effect( Ecore_X_Event_Client_Message *ev )
{
   E_Comp *c = NULL;

   if ( ev == NULL ) return ECORE_CALLBACK_PASS_ON;
   c = _e_mod_comp_find(ev->win);
   if (!c) return ECORE_CALLBACK_PASS_ON;

   evas_object_show(c->capture_effect_obj);
   evas_object_raise(c->capture_effect_obj);
   edje_object_signal_emit(c->capture_effect_obj, "img,state,capture,on", "img");

   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_comp_capture_effect_show_done(void *data,
                                     Evas_Object *obj __UNUSED__,
                                     const char *emission __UNUSED__,
                                     const char *source __UNUSED__)
{
   E_Comp *c = (E_Comp*)data;
   if ( !c ) return;
   evas_object_hide(c->capture_effect_obj);
}

static Eina_Bool
_e_mod_comp_get_edje_error(E_Comp_Win *cw)
{
   int error = EDJE_LOAD_ERROR_NONE;

   if ((error = edje_object_load_error_get(cw->shobj)) != EDJE_LOAD_ERROR_NONE)
     {
        Ecore_X_Window win = _e_mod_comp_win_get_client_xid(cw);
        switch(error)
          {
           case EDJE_LOAD_ERROR_GENERIC:                    fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_GENERIC! win:0x%08x\n",                    win); break;
           case EDJE_LOAD_ERROR_DOES_NOT_EXIST:             fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_DOES_NOT_EXIST! win:0x%08x\n",             win); break;
           case EDJE_LOAD_ERROR_PERMISSION_DENIED:          fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_PERMISSION_DENIED! win:0x%08x\n",          win); break;
           case EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED: fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED! win:0x%08x\n", win); break;
           case EDJE_LOAD_ERROR_CORRUPT_FILE:               fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_CORRUPT_FILE! win:0x%08x\n",               win); break;
           case EDJE_LOAD_ERROR_UNKNOWN_FORMAT:             fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_UNKNOWN_FORMAT! win:0x%08x\n",             win); break;
           case EDJE_LOAD_ERROR_INCOMPATIBLE_FILE:          fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_INCOMPATIBLE_FILE! win:0x%08x\n",          win); break;
           case EDJE_LOAD_ERROR_UNKNOWN_COLLECTION:         fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_UNKNOWN_COLLECTION! win:0x%08x\n",         win); break;
           case EDJE_LOAD_ERROR_RECURSIVE_REFERENCE:        fprintf(stderr, "[E17-comp] EDJE_LOAD_ERROR_RECURSIVE_REFERENCE! win:0x%08x\n",        win); break;
           default:
              break;
          }
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Ecore_X_Window
_e_mod_comp_win_get_client_xid(E_Comp_Win *cw)
{
   if (!cw) return 0;
   if (cw->bd) return cw->bd->client.win;
   else return cw->win;
}

static Eina_Bool
_e_mod_comp_win_is_border(E_Comp_Win *cw)
{
   if (!cw) return EINA_FALSE;
   if (cw->bd) return EINA_TRUE;
   else return EINA_FALSE;
}

static void
_e_mod_comp_fps_update(E_Comp *c)
{
   if (_comp_mod->conf->fps_show)
     {
        if (!c->fps_bg)
          {
             c->fps_bg = evas_object_rectangle_add(c->evas);
             evas_object_color_set(c->fps_bg, 0, 0, 0, 128);
             evas_object_layer_set(c->fps_bg, EVAS_LAYER_MAX);
             evas_object_show(c->fps_bg);

             c->fps_fg = evas_object_text_add(c->evas);
             evas_object_text_font_set(c->fps_fg, "Sans", 10);
             evas_object_text_text_set(c->fps_fg, "???");
             evas_object_color_set(c->fps_fg, 255, 255, 255, 255);
             evas_object_layer_set(c->fps_fg, EVAS_LAYER_MAX);
             evas_object_show(c->fps_fg);
          }
     }
   else
     {
        if (c->fps_fg)
          {
             evas_object_del(c->fps_fg);
             c->fps_fg = NULL;
          }
        if (c->fps_bg)
          {
             evas_object_del(c->fps_bg);
             c->fps_bg = NULL;
          }
     }
}

static void
_e_mod_comp_fps_toggle(void)
{
   if (_comp_mod)
     {
        Eina_List *l;
        E_Comp *c;

        if (_comp_mod->conf->fps_show)
          {
             _comp_mod->conf->fps_show = 0;
             e_config_save_queue();
          }
         else
          {
             _comp_mod->conf->fps_show = 1;
             e_config_save_queue();
          }
         EINA_LIST_FOREACH(compositors, l, c) _e_mod_comp_cb_update(c);
      }
}

char*
_e_mod_comp_get_atom_name(Ecore_X_Atom a)
{
   if (!a) return NULL;

   if      (ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE        && a == ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE       ) return "SYNC_DRAW_DONE";
   else if (ECORE_X_ATOM_E_COMP_SYNC_COUNTER          && a == ECORE_X_ATOM_E_COMP_SYNC_COUNTER         ) return "SYNC_COUNTER";
   else if (ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE && a == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) return "WIN_ROT_ANGLE";
   else if (ECORE_X_ATOM_NET_WM_WINDOW_OPACITY        && a == ECORE_X_ATOM_NET_WM_WINDOW_OPACITY       ) return "WM_WINDOW_OPACITY";
   else if (ATOM_EFFECT_ENABLE                        && a == ATOM_EFFECT_ENABLE                       ) return "CM_EFFECT_ENABLE";
   else if (ATOM_WINDOW_EFFECT_ENABLE                 && a == ATOM_WINDOW_EFFECT_ENABLE                ) return "WINDOW_EFFECT_ENABLE";
   else if (ATOM_WINDOW_EFFECT_CLIENT_STATE           && a == ATOM_WINDOW_EFFECT_CLIENT_STATE          ) return "EFFECT_CLIENT_STATE";
   else if (ATOM_WINDOW_EFFECT_STATE                  && a == ATOM_WINDOW_EFFECT_STATE                 ) return "EFFECT_STATE";
   else if (ATOM_WINDOW_EFFECT_TYPE                   && a == ATOM_WINDOW_EFFECT_TYPE                  ) return "EFFECT_TYPE";
   else if (ATOM_EFFECT_DEFAULT                       && a == ATOM_EFFECT_DEFAULT                      ) return "EFFECT_DEFAULT";
   else if (ATOM_EFFECT_NONE                          && a == ATOM_EFFECT_NONE                         ) return "EFFECT_NONE";
   else if (ATOM_EFFECT_CUSTOM0                       && a == ATOM_EFFECT_CUSTOM0                      ) return "EFFECT_CUSTOM0";
   else if (ATOM_EFFECT_CUSTOM1                       && a == ATOM_EFFECT_CUSTOM1                      ) return "EFFECT_CUSTOM1";
   else if (ATOM_EFFECT_CUSTOM2                       && a == ATOM_EFFECT_CUSTOM2                      ) return "EFFECT_CUSTOM2";
   else if (ATOM_EFFECT_CUSTOM3                       && a == ATOM_EFFECT_CUSTOM3                      ) return "EFFECT_CUSTOM3";
   else if (ATOM_EFFECT_CUSTOM4                       && a == ATOM_EFFECT_CUSTOM4                      ) return "EFFECT_CUSTOM4";
   else if (ATOM_EFFECT_CUSTOM5                       && a == ATOM_EFFECT_CUSTOM5                      ) return "EFFECT_CUSTOM5";
   else if (ATOM_EFFECT_CUSTOM6                       && a == ATOM_EFFECT_CUSTOM6                      ) return "EFFECT_CUSTOM6";
   else if (ATOM_EFFECT_CUSTOM7                       && a == ATOM_EFFECT_CUSTOM7                      ) return "EFFECT_CUSTOM7";
   else if (ATOM_EFFECT_CUSTOM8                       && a == ATOM_EFFECT_CUSTOM8                      ) return "EFFECT_CUSTOM8";
   else if (ATOM_EFFECT_CUSTOM9                       && a == ATOM_EFFECT_CUSTOM9                      ) return "EFFECT_CUSTOM9";
   else if (ATOM_FAKE_LAUNCH_IMAGE                    && a == ATOM_FAKE_LAUNCH_IMAGE                   ) return "FAKE_LAUNCH_IMAGE";
   else if (ATOM_FAKE_LAUNCH                          && a == ATOM_FAKE_LAUNCH                         ) return "FAKE_LAUNCH";
   else if (ATOM_CM_LOCK_SCREEN                       && a == ATOM_CM_LOCK_SCREEN                      ) return "LOCK_SCREEN";
   else if (ATOM_CM_PIXMAP_ROTATION_SUPPORTED         && a == ATOM_CM_PIXMAP_ROTATION_SUPPORTED        ) return "PIX_ROT_SUPPORTED";
   else if (ATOM_CM_PIXMAP_ROTATION_STATE             && a == ATOM_CM_PIXMAP_ROTATION_STATE            ) return "PIX_ROT_STATE";
   else if (ATOM_CM_PIXMAP_ROTATION_BEGIN             && a == ATOM_CM_PIXMAP_ROTATION_BEGIN            ) return "PIX_ROT_BEGIN";
   else if (ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE        && a == ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE       ) return "PIX_ROT_BEGIN_DONE";
   else if (ATOM_CM_PIXMAP_ROTATION_END               && a == ATOM_CM_PIXMAP_ROTATION_END              ) return "PIX_ROT_END";
   else if (ATOM_CM_PIXMAP_ROTATION_END_DONE          && a == ATOM_CM_PIXMAP_ROTATION_END_DONE         ) return "PIX_ROT_END_DONE";
   else if (ATOM_CM_PIXMAP_ROTATION_REQUEST           && a == ATOM_CM_PIXMAP_ROTATION_REQUEST          ) return "PIX_ROT_REQUEST";
   else if (ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE      && a == ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE     ) return "PIX_ROT_REQUEST_DONE";
   else if (ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP     && a == ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP    ) return "PIX_ROT_RESIZE_PIXMAP";
   else if (ATOM_OVERAY_WINDOW                        && a == ATOM_OVERAY_WINDOW                       ) return "OVERAY_WINDOW";
   else if (ATOM_X_HIBERNATION_STATE                  && a == ATOM_X_HIBERNATION_STATE                 ) return "HIBERNATION_STATE";
   else if (ATOM_X_SCREEN_ROTATION                    && a == ATOM_X_SCREEN_ROTATION                   ) return "SCREEN_ROTATION";
#if COMP_LOGGER_BUILD_ENABLE
   else if (ATOM_CM_LOG                               && a == ATOM_CM_LOG                              ) return "CM_LOG";
   else if (ATOM_CM_LOG_DUMP_DONE                     && a == ATOM_CM_LOG_DUMP_DONE                    ) return "CM_LOG_DUMP_DONE";
#endif /* COMP_LOGGER_BUILD_ENABLE */

   return NULL;
}

Eina_Bool
_e_mod_comp_rotation_release(E_Comp_Win *cw)
{
   if (!cw || !cw->rotating || !cw->rotobj) return EINA_FALSE;

   L(LT_EVENT_X,
     "[COMP] %31s w:0x%08x bd:%s release rotobj. animating:%d\n",
     "PIX_ROT", _e_mod_comp_win_get_client_xid(cw),
     cw->bd ? "O" : "X", cw->animating);

   if (cw->animating)
     {
        cw->delete_me = 1;
        return EINA_TRUE;
     }

   cw->rotating = EINA_FALSE;

   Ecore_X_Damage damage;
   damage = e_mod_comp_rotation_get_damage(cw->rotobj);
   if (damage) _e_mod_comp_win_del_damage(cw, damage);

   e_mod_comp_rotation_end(cw->rotobj);
   e_mod_comp_rotation_free(cw->rotobj);
   cw->rotobj = NULL;

   e_mod_comp_rotation_done_send
      (_e_mod_comp_win_get_client_xid(cw),
       ATOM_CM_PIXMAP_ROTATION_END_DONE);

   L(LT_EVENT_X,
     "[COMP] %31s w:0x%08x send END_DONE.\n",
     "PIX_ROT", _e_mod_comp_win_get_client_xid(cw));

   return EINA_TRUE;
}

static int
_e_mod_comp_rotation_handle_message(Ecore_X_Event_Client_Message *ev)
{
   Ecore_X_Atom type = ev->message_type;
   Ecore_X_Window win;
   E_Comp_Win *cw = NULL;

   cw = _e_mod_comp_border_client_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_win_find(ev->win);
        if (!cw) return 0;
     }

   win = ev->win;

   if (type == ATOM_CM_PIXMAP_ROTATION_BEGIN)
     {
        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x bd:%d c:0x%08x cw:%p\n",
          "PIX_ROT", win, _e_mod_comp_win_is_border(cw),
          _e_mod_comp_win_get_client_xid(cw), cw);

        if (cw->rotating || cw->rotobj)
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x invalid BEGIN_REQUEST. rotating:%d rotobj:%p\n",
               "PIX_ROT", ev->win, cw->rotating, cw->rotobj);
             return 0;
          }

        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x BEGIN_REQUEST.\n",
          "PIX_ROT", ev->win);

        cw->rotobj = e_mod_comp_rotation_new();
        if (!cw->rotobj) return 0;

        cw->rotating = EINA_TRUE;

        edje_object_part_unswallow(cw->shobj, cw->obj);
        _e_mod_comp_win_unredirect(cw);

        e_mod_comp_rotation_done_send
           (win, ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE);

        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x send BEGIN_DONE.\n",
          "PIX_ROT", ev->win);
     }
   else if (type == ATOM_CM_PIXMAP_ROTATION_END)
     {
        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x bd:%d c:0x%08x cw:%p\n",
          "PIX_ROT", win, _e_mod_comp_win_is_border(cw),
          _e_mod_comp_win_get_client_xid(cw), cw);

        if (!cw->rotating || !cw->rotobj)
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x invalid END_REQUEST. rotating:%d rotobj:%p\n",
               "PIX_ROT", ev->win, cw->rotating, cw->rotobj);
             return 0;
          }

        _e_mod_comp_win_dec_animating(cw);

        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x end END_REQUEST. release rotobj.\n",
          "PIX_ROT", ev->win);

        _e_mod_comp_rotation_release(cw);
     }
   else if (type == ATOM_CM_PIXMAP_ROTATION_REQUEST)
     {
        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x bd:%d c:0x%08x cw:%p\n",
          "PIX_ROT", win, _e_mod_comp_win_is_border(cw),
          _e_mod_comp_win_get_client_xid(cw), cw);

        if (!cw->rotating || !cw->rotobj)
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x invalid ROTATION_REQUEST. rotating:%d rotobj:%p\n",
               "PIX_ROT", ev->win, cw->rotating, cw->rotobj);
             return 0;
          }

        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x ROTATION_REQUEST.\n",
          "PIX_ROT", ev->win);

        Ecore_X_Damage damage;
        damage = e_mod_comp_rotation_get_damage(cw->rotobj);
        if (damage) _e_mod_comp_win_del_damage(cw, damage);

        if (cw->obj)
          edje_object_part_unswallow(cw->shobj, cw->obj);
        else
          {
             if (!cw->c)
               {
                  L(LT_EVENT_X,
                    "[COMP] %31s w:0x%08x can't create cw->obj. cw->c:NULL\n",
                    "PIX_ROT", ev->win);
                  return 0;
               }

             cw->obj = evas_object_image_filled_add(cw->c->evas);
             evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);

             if (cw->argb)
               evas_object_image_alpha_set(cw->obj, 1);
             else
               evas_object_image_alpha_set(cw->obj, 0);

             evas_object_show(cw->obj);
             evas_object_pass_events_set(cw->obj, 1);

             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x create cw->obj:%p (alpha:%d).\n",
               "PIX_ROT", ev->win, cw->obj, cw->argb);
          }

        if (!e_mod_comp_rotation_request(cw->rotobj,
                                         ev,
                                         cw->c->evas,
                                         cw->shobj,
                                         cw->obj,
                                         cw->vis,
                                         cw->x, cw->y, cw->w, cw->h))
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x e_mod_comp_rotation_request() failed.\n",
               "PIX_ROT", ev->win);
             return 0;
          }

        e_mod_comp_update_resize(cw->up, cw->w, cw->h);
        e_mod_comp_update_add(cw->up, 0, 0, cw->w, cw->h);

        cw->native = 1;

        damage = e_mod_comp_rotation_get_damage(cw->rotobj);
        if (!damage) return 0;
        _e_mod_comp_win_add_damage(cw, damage);
        e_mod_comp_rotation_done_send(win, ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE);

        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x send ROTATION_DONE.\n",
          "PIX_ROT", ev->win);
     }
   else
     return 0;

   return 1;
}

static Eina_Bool
_e_mod_comp_win_add_damage(E_Comp_Win *cw,
                           Ecore_X_Damage damage)
{
   return eina_hash_add(damages, e_util_winid_str_get(damage), cw);
}

static Eina_Bool
_e_mod_comp_win_del_damage(E_Comp_Win *cw,
                           Ecore_X_Damage damage)
{
   return eina_hash_del(damages, e_util_winid_str_get(damage), cw);
}

static Eina_Bool
_e_mod_comp_win_get_prop_angle(Ecore_X_Window win,
                               int *req,
                               int *curr)
{
   Eina_Bool res = EINA_FALSE;
   int ret, count;
   int angle[2] = {-1, -1};
   unsigned char* prop_data = NULL;

   ret = ecore_x_window_prop_property_get(win,
                                          ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
                                          ECORE_X_ATOM_CARDINAL,
                                          32,
                                          &prop_data,
                                          &count);
   if (ret <= 0)
     {
        if (prop_data) free(prop_data);
        return res;
     }
   if (ret && prop_data)
     {
        memcpy (&angle, prop_data, sizeof (int)*count);
        if (count == 2) res = EINA_TRUE;
     }

   if (prop_data) free(prop_data);

   *req  = angle[_WND_REQUEST_ANGLE_IDX];
   *curr = angle[_WND_CURR_ANGLE_IDX];

   if (angle[0] == -1 && angle[1] == -1) res = EINA_FALSE;

   L(LT_EVENT_X, "[COMP] %31s %d=>%d count:%d res:%s\n", \
     "rot_prop_get", \
     angle[_WND_CURR_ANGLE_IDX], \
     angle[_WND_REQUEST_ANGLE_IDX], \
     count,
     res ? "Ture" : "False");

   return res;
}

Eina_Bool
_e_mod_comp_win_check_visible(E_Comp_Win *cw)
{
   Eina_Bool visible = EINA_FALSE;
   E_Comp_Win *check_cw = NULL;

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, check_cw)
     {
        if (check_cw->x == 0 &&
            check_cw->y == 0 &&
            check_cw->w == cw->c->man->w &&
            check_cw->h == cw->c->man->h)
          {
             if (check_cw == cw)
               {
                  visible = EINA_TRUE;
                  break;
               }
             if (!check_cw->visible   ) continue;
             if ( check_cw->defer_hide) continue;
             if ( check_cw->invalid   ) continue;
             if ( check_cw->input_only) continue;
             if ( check_cw->argb      ) continue;
             break;
          }
     }
   return visible;
}

Eina_Bool
_e_mod_comp_win_check_visible2(E_Comp_Win *cw)
{
   E_Comp_Win *find_cw = NULL;
   E_Comp *c = NULL;

   Ecore_X_Region VisibleRegion = 0;
   Ecore_X_Region CurrentRegion = 0;
   Ecore_X_Region ResultRegion = 0;

   Ecore_X_Rectangle *rects = NULL;
   Ecore_X_Rectangle bounds = { 0, 0, 0, 0 };

   int num_rects = 0;

   Ecore_X_Rectangle screen_rect = { 0, 0, cw->c->man->w, cw->c->man->h };
   Ecore_X_Rectangle window_rect;

   Eina_Bool visible = EINA_FALSE;

   c = cw->c;

   if (((!cw->visible)
        || (cw->defer_hide)
        || (cw->invalid)
        || (cw->input_only)
        || (!E_INTERSECTS(0, 0, c->man->w, c->man->h,
                          cw->x, cw->y, cw->w, cw->h))))
     {
        return visible;
     }

   VisibleRegion = ecore_x_region_new(&screen_rect, 1);

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, find_cw)
     {
        if (cw == find_cw)
          {
             rects = NULL;
             num_rects = 0;

             window_rect.x = find_cw->x;
             window_rect.y = find_cw->y;
             window_rect.width = find_cw->w;
             window_rect.height = find_cw->h;
             CurrentRegion = ecore_x_region_new(&window_rect, 1);

             ResultRegion = ecore_x_region_new(&screen_rect, 1);
             // result = visible intersect current
             ecore_x_region_intersect(ResultRegion, VisibleRegion, CurrentRegion);
             rects = ecore_x_region_fetch(ResultRegion, &num_rects, &bounds);

             if (num_rects > 0)
               {
                  visible = EINA_TRUE;
                  break;
               }
             else
               {
                  visible = EINA_FALSE;
                  break;
               }
          }
        else if (((!find_cw->visible)
                  || (find_cw->defer_hide)
                  || (find_cw->invalid)
                  || (find_cw->input_only)
                  || (find_cw->argb) // this code may cause bug.
                  || (!E_INTERSECTS(0, 0, c->man->w, c->man->h,
                                    cw->x, cw->y, cw->w, cw->h))))
          {
             continue;
          }
        else
          {
             rects = NULL;
             num_rects = 0;

             window_rect.x = find_cw->x;
             window_rect.y = find_cw->y;
             window_rect.width = find_cw->w;
             window_rect.height = find_cw->h;

             CurrentRegion = ecore_x_region_new(&window_rect, 1);
             ResultRegion = ecore_x_region_new(&screen_rect, 1);

             // result = visible - current
             ecore_x_region_subtract (ResultRegion, VisibleRegion, CurrentRegion);
             rects = ecore_x_region_fetch (ResultRegion, &num_rects, &bounds);

             if (num_rects == 0)
               {
                  visible = EINA_FALSE;
                  break;
               }
             else
               {
                  ecore_x_region_copy(VisibleRegion, ResultRegion);
                  if (rects) free(rects);
                  ecore_x_region_free(ResultRegion);
                  ecore_x_region_free(CurrentRegion);
               }
          }
     }

     if (rects) free(rects);
     ecore_x_region_free(ResultRegion);
     ecore_x_region_free(CurrentRegion);
     ecore_x_region_free(VisibleRegion);

     return visible;
}

Eina_Bool
_e_mod_comp_win_check_visible3(E_Comp_Win *cw)
{
   Eina_Bool visible = EINA_FALSE;
   E_Comp_Win *check_cw = NULL;

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, check_cw)
     {
        if (check_cw->x == 0 &&
            check_cw->y == 0 &&
            check_cw->w == cw->c->man->w &&
            check_cw->h == cw->c->man->h &&
            !check_cw->input_only &&
            !check_cw->invalid )
          {
             if (check_cw == cw)
               {
                  visible = EINA_TRUE;
                  break;
               }
          }
     }
   return visible;
}

static Eina_Bool
_e_mod_comp_prop_window_rotation(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   int req_angle = -1;
   int cur_angle = -1;

   L(LT_EVENT_X,
     "[COMP] %31s\n", "PROP_ILLUME_ROT_WND_ANG");

   cw = _e_mod_comp_border_client_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_win_find(ev->win);
        if (!cw)
          {
             Ecore_X_Sync_Counter counter = ecore_x_e_comp_sync_counter_get(ev->win);
             ecore_x_e_comp_sync_cancel_send(ev->win);
             if (counter) ecore_x_sync_counter_inc(counter, 1);
             return EINA_FALSE;
          }
     }

   // if window's rotation effect type is NONE,
   // then do not show rotation effect.
   // just return from here
   if (cw->rotation_effect == COMP_EFFECT_NONE)
     {
        if (cw->counter)
          ecore_x_sync_counter_inc(cw->counter, 1);
        else
          {
             cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
             if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
          }
        return EINA_FALSE;
     }

   if (!_e_mod_comp_win_get_prop_angle(_e_mod_comp_win_get_client_xid(cw),
                                       &req_angle,
                                       &cur_angle))
     {
        if (cw->counter)
          ecore_x_sync_counter_inc(cw->counter, 1);
        else
          {
             cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
             if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
          }
        return EINA_FALSE;
     }

   if (req_angle == cur_angle)
     {
        // candidate window of keypad requests rotation with same angles.
        if (cw->counter)
          ecore_x_sync_counter_inc(cw->counter, 1);
        else
          {
             cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
             if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
          }
        return EINA_FALSE;
     }

   if (_e_mod_comp_window_rotation_policy(cw))
     {
       cw->req_ang = req_angle;
       cw->cur_ang = cur_angle;
       cw->ready_win_rot_effect = EINA_TRUE;
       if (cw->win_rot_timeout)
         {
            ecore_timer_del(cw->win_rot_timeout);
            cw->win_rot_timeout = NULL;
         }
       cw->win_rot_timeout = ecore_timer_add(4.0f,
                                             _e_mod_comp_win_rotation_begin_timeout,
                                             cw);
       if (cw->counter)
         ecore_x_sync_counter_inc(cw->counter, 1);
       else
         {
            cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
            if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
         }

       L(LT_EVENT_X,
         "[COMP] %31s:%p\n",
         "win_rot_timeout",
         cw->win_rot_timeout);
      }
    else
      {
         cw->req_ang = req_angle;
         cw->cur_ang = req_angle;
         cw->ready_win_rot_effect = EINA_FALSE;
         if (cw->counter)
           ecore_x_sync_counter_inc(cw->counter, 1);
         else
           {
              cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
              if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
           }
      }

   L(LT_EVENT_X,
     "[COMP] %31s %d\n",
     "ready_effect",
     cw->ready_win_rot_effect);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_win_rotation_begin(E_Comp_Win *cw, Eina_Bool timeout)
{
   cw->ready_win_rot_effect = EINA_FALSE;

   L(LT_EVENT_X,
     "[COMP] %31s timeout:%d\n",
     "win_rot_begin", timeout);

   if (cw->win_rot_timeout)
     {
        ecore_timer_del(cw->win_rot_timeout);
        cw->win_rot_timeout = NULL;
     }

   if (cw->rotation_effect == COMP_EFFECT_NONE) return EINA_TRUE;
   else
     {
        switch (cw->req_ang - cw->cur_ang)
          {
             case -270: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,90",   "e"); break;
             case -180: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,180",  "e"); break;
             case  -90: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,-90",  "e"); break;
             case    0: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,0",    "e"); break;
             case   90: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,90",   "e"); break;
             case  180: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,-180", "e"); break;
             case  270: edje_object_signal_emit(cw->shobj, "e,state,window,rotation,-90",  "e"); break;
             default  : edje_object_signal_emit(cw->shobj, "e,state,window,rotation,0",    "e"); break;
          }
     }
   cw->win_rot_effect = 1;
   _e_mod_comp_win_inc_animating(cw);

   if (timeout)
     {
        if (!cw->update)
          {
             if (cw->update_timeout)
               {
                  ecore_timer_del(cw->update_timeout);
                  cw->update_timeout = NULL;
               }
             cw->update = 1;
             cw->c->updates = eina_list_append(cw->c->updates, cw);
          }
        cw->drawme = 1;
        _e_mod_comp_win_render_queue(cw);
        if (cw->counter)
          ecore_x_sync_counter_inc(cw->counter, 1);
        else
          {
             cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
             if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
          }
     }

   // TODO: remove argb check code for the ticker window
   if (!cw->argb)
     {
        evas_object_stack_below(cw->c->bg_img, cw->shobj);
        cw->c->use_bg_img = 1;
     }

   cw->win_rot_timeout = ecore_timer_add(4.0f,
                                         _e_mod_comp_win_rotation_end_timeout,
                                         cw);

   return EINA_TRUE;
}

static void
_e_mod_comp_win_rotation_done(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   E_Comp_Win *cw = (E_Comp_Win*)data;
   if (!cw) return;

   if (cw->win_rot_timeout)
     {
        ecore_timer_del(cw->win_rot_timeout);
        cw->win_rot_timeout = NULL;
     }

   if ( !cw->show_done ) cw->show_done = EINA_TRUE;
   evas_object_stack_below(cw->c->bg_img, evas_object_bottom_get(cw->c->evas));
   cw->c->use_bg_img = 0;
   cw->win_rot_effect = 0;

   L(LT_EVENT_X,
     "[COMP] %31s %s w:0x%08x\n",
     "EFFECT", "WIN_ROT_DONE",
     _e_mod_comp_win_get_client_xid(cw));

   _e_mod_comp_done_defer(cw);
}

static Eina_Bool
_e_mod_comp_win_rotation_release(E_Comp_Win *cw)
{
   cw->ready_win_rot_effect = EINA_FALSE;
   cw->req_ang = 0;
   cw->cur_ang = 0;

   if (cw->win_rot_timeout)
     {
        ecore_timer_del(cw->win_rot_timeout);
        cw->win_rot_timeout = NULL;
     }
   return EINA_TRUE;
}

void
_e_mod_comp_send_window_effect_client_state(E_Comp_Win *cw,
                                            Eina_Bool state)
{
   Ecore_X_Window win = 0;
   long d[5] = { 0L, 0L, 0L, 0L, 0L };
   if (!cw) return;

   win = _e_mod_comp_win_get_client_xid(cw);
   if (!win) return;

   if (state) d[0] = 1L;
   else d[1] = 1L;

   ecore_x_client_message32_send
      (win, ATOM_WINDOW_EFFECT_CLIENT_STATE,
          StructureNotifyMask, d[0], d[1], d[2], d[3], d[4]);
}

static Eina_Bool
_e_mod_comp_is_next_win_stack(E_Comp_Win *cw,
                              E_Comp_Win *cw2)
{
   Eina_Inlist *wins_list;
   if (!cw || !cw2) return EINA_FALSE;

   while ((wins_list = EINA_INLIST_GET(cw)->prev) != NULL)
     {
        cw = _EINA_INLIST_CONTAINER(cw, wins_list);
        if (cw == cw2)
          {
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

E_Comp_Win *
_e_mod_comp_win_find_by_indicator(E_Comp *c)
{
   E_Comp_Win *cw;
   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (_e_mod_comp_is_indicator_window(cw->bd)) return cw;
     }
   return NULL;
}

#ifdef  _MAKE_ATOM
# undef _MAKE_ATOM
#endif

#define _MAKE_ATOM(a, s)                              \
   do {                                               \
        a = ecore_x_atom_get(s);                      \
        if (!a)                                       \
          fprintf(stderr,                             \
                  "[E-comp] ##s creation failed.\n"); \
   } while(0)

static void
_e_mod_comp_atom_init(void)
{
   _MAKE_ATOM(ATOM_FAKE_LAUNCH_IMAGE,                STR_ATOM_FAKE_LAUNCH_IMAGE                );
   _MAKE_ATOM(ATOM_FAKE_LAUNCH,                      STR_ATOM_FAKE_LAUNCH                      );
   _MAKE_ATOM(ATOM_EFFECT_ENABLE,                    STR_ATOM_NET_CM_EFFECT_ENABLE             );
   _MAKE_ATOM(ATOM_WINDOW_EFFECT_ENABLE,             STR_ATOM_NET_CM_WINDOW_EFFECT_ENABLE      );
   _MAKE_ATOM(ATOM_WINDOW_EFFECT_CLIENT_STATE,       STR_ATOM_NET_CM_WINDOW_EFFECT_CLIENT_STATE);
   _MAKE_ATOM(ATOM_WINDOW_EFFECT_STATE,              STR_ATOM_NET_CM_WINDOW_EFFECT_STATE       );
   _MAKE_ATOM(ATOM_WINDOW_EFFECT_TYPE,               STR_ATOM_NET_CM_WINDOW_EFFECT_TYPE        );
   _MAKE_ATOM(ATOM_EFFECT_DEFAULT,                   STR_ATOM_NET_CM_EFFECT_DEFAULT            );
   _MAKE_ATOM(ATOM_EFFECT_NONE,                      STR_ATOM_NET_CM_EFFECT_NONE               );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM0,                   STR_ATOM_NET_CM_EFFECT_CUSTOM0            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM1,                   STR_ATOM_NET_CM_EFFECT_CUSTOM1            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM2,                   STR_ATOM_NET_CM_EFFECT_CUSTOM2            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM3,                   STR_ATOM_NET_CM_EFFECT_CUSTOM3            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM4,                   STR_ATOM_NET_CM_EFFECT_CUSTOM4            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM5,                   STR_ATOM_NET_CM_EFFECT_CUSTOM5            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM6,                   STR_ATOM_NET_CM_EFFECT_CUSTOM6            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM7,                   STR_ATOM_NET_CM_EFFECT_CUSTOM7            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM8,                   STR_ATOM_NET_CM_EFFECT_CUSTOM8            );
   _MAKE_ATOM(ATOM_EFFECT_CUSTOM9,                   STR_ATOM_NET_CM_EFFECT_CUSTOM9            );
   _MAKE_ATOM(ATOM_OVERAY_WINDOW,                    STR_ATOM_OVERAY_WINDOW                    );
   _MAKE_ATOM(ATOM_X_HIBERNATION_STATE,              STR_ATOM_X_HIBERNATION_STATE              );
   _MAKE_ATOM(ATOM_CM_LOCK_SCREEN,                   STR_ATOM_CM_LOCK_SCREEN                   );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_SUPPORTED,     STR_ATOM_CM_PIXMAP_ROTATION_SUPPORTED     );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_STATE,         STR_ATOM_CM_PIXMAP_ROTATION_STATE         );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_BEGIN,         STR_ATOM_CM_PIXMAP_ROTATION_BEGIN         );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE,    STR_ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE    );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_END,           STR_ATOM_CM_PIXMAP_ROTATION_END           );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_END_DONE,      STR_ATOM_CM_PIXMAP_ROTATION_END_DONE      );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_REQUEST,       STR_ATOM_CM_PIXMAP_ROTATION_REQUEST       );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE,  STR_ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE  );
   _MAKE_ATOM(ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP, STR_ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP );
   _MAKE_ATOM(ATOM_X_SCREEN_ROTATION,                STR_ATOM_X_SCREEN_ROTATION                );
   _MAKE_ATOM(ATOM_CAPTURE_EFFECT,                   STR_ATOM_CAPTURE_EFFECT                   );
#if COMP_LOGGER_BUILD_ENABLE
   _MAKE_ATOM(ATOM_CM_LOG,                           STR_ATOM_CM_LOG                           );
   _MAKE_ATOM(ATOM_CM_LOG_DUMP_DONE,                 STR_ATOM_CM_LOG_DUMP_DONE                 );
#endif
}

void
_e_mod_comp_cb_pending_after(void *data __UNUSED__,
                             E_Manager *man __UNUSED__,
                             E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   cw->pending_count--;
   if (!cw->delete_pending) return;
   if (cw->pending_count == 0)
     {
        free(cw);
     }
}

static void
_e_mod_comp_set(E_Comp *c)
{
   if (_comp)
     {
        fprintf(stderr,
                "[E17-comp] %s(%d) E_Comp setup failed.\n",
                __func__, __LINE__);
        return;
     }
   _comp = c;
}

E_Comp *
_e_mod_comp_get(void)
{
   return _comp;
}

static void
_e_mod_comp_dump_cw_stack(Eina_Bool to_file,
                          const char *name)
{
   E_Comp *c;
   E_Comp_Win *cw;
   double val;
   const char *file, *group;
   int x, y, w, h, i = 1;
   FILE *fs = stderr;

   c = _e_mod_comp_get();
   if (!c) return;

   if ((to_file) && (name))
     {
        fs = fopen(name, "w");
        if (!fs)
          {
             fprintf(stderr, "can't open %s file.\n", name);
             fs = stderr;
             to_file = EINA_FALSE;
          }
     }

   fprintf(fs, "B-------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " No   WinID        shobj    x    y    w    h   ex   ey   ew   eh W S O    clipper         shower        swallow         group      st \n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " c->bg_img : %s 0x%08x\n", evas_object_visible_get(c->bg_img) ? "O" : "X", (unsigned int)c->bg_img);
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        edje_object_file_get(cw->shobj, &file, &group);
        evas_object_geometry_get(cw->shobj, &x, &y, &w, &h);
        fprintf(fs,
                " %2d 0x%07x 0x%08x %4d %4d %4d %4d %4d %4d %4d %4d %s %s %s %10.10s %.1f %10.10s %.1f %10.10s %.1f %15.15s %d\n",
                i,
                _e_mod_comp_win_get_client_xid(cw),
                (unsigned int)cw->shobj,
                cw->x, cw->y, cw->w, cw->h,
                x, y, w, h,
                cw->visible ? "O" : "X",
                evas_object_visible_get(cw->shobj) ? "O" : "X",
                evas_object_visible_get(cw->obj) ? "O" : "X",
                edje_object_part_state_get(cw->shobj, "clipper", &val), val,
                edje_object_part_state_get(cw->shobj, "shower", &val), val,
                edje_object_part_state_get(cw->shobj, "e.swallow.content", &val), val,
                group, cw->effect_stage);
        i++;
     }

   i = 1;
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if (!cw) break;
        file = group = NULL;
        edje_object_file_get(cw->shobj,
                             &file, &group);
        fprintf(fs,
                " %2d 0x%07x %s\n",
                i, _e_mod_comp_win_get_client_xid(cw),
                file);
        i++;
     }

   i = 1;
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " No     WinID      shobj        obj    found_o    x    y    w    h   ex   ey   ew   eh | W S O\n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   Evas_Object *o = evas_object_top_get(c->evas);
   Eina_Bool found = 0;
   Ecore_X_Window cid = 0;
   char *wname = NULL, *wclas = NULL;
   int pid = 0;
   while (o)
     {
        EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
          {
             if (!cw) break;
             if (cw->shobj == o)
               {
                  found = 1;
                  cid = _e_mod_comp_win_get_client_xid(cw);
                  ecore_x_icccm_name_class_get(cid, &wname, &wclas);
                  ecore_x_netwm_pid_get(cid, &pid);
                  break;
               }
          }

        evas_object_geometry_get(o, &x, &y, &w, &h);

        if (found && cw->visible)
          {
             fprintf(fs,
                     " %2d 0x%07x 0x%08x 0x%08x 0x%08x %4d,%4d %4dx%4d %4d,%4d %4dx%4d | %s %s %s %4d, %s, %s\n",
                     i,
                     _e_mod_comp_win_get_client_xid(cw),
                     (unsigned int)cw->shobj,
                     (unsigned int)cw->obj,
                     (unsigned int)o,
                     cw->x, cw->y, cw->w, cw->h,
                     x, y, w, h,
                     cw->visible ? "O" : "X",
                     evas_object_visible_get(cw->shobj) ? "O" : "X",
                     evas_object_visible_get(o) ? "O" : "X",
                     pid, wname, wclas);
          }
        else if (o == c->bg_img)
          {
             fprintf(fs,
                     " %2d %31.31s 0x%08x %19.19s %4d,%4d %4dx%4d |     %s <-- bg_img\n",
                     i, " ", (unsigned int)o, " ", x, y, w, h, evas_object_visible_get(o) ? "O" : "X");
          }
        else if (evas_object_visible_get(o))
          {
             if (found)
               {
                  fprintf(fs,
                          " %2d 0x%07x 0x%08x 0x%08x 0x%08x %4d,%4d %4dx%4d %4d,%4d %4dx%4d | %s %s %s %4d, %s, %s\n",
                          i,
                          _e_mod_comp_win_get_client_xid(cw),
                          (unsigned int)cw->shobj,
                          (unsigned int)cw->obj,
                          (unsigned int)o,
                          cw->x, cw->y, cw->w, cw->h,
                          x, y, w, h,
                          cw->visible ? "O" : "X",
                          evas_object_visible_get(cw->shobj) ? "O" : "X",
                          evas_object_visible_get(o) ? "O" : "X",
                          pid, wname, wclas);
               }
             else
               {
                  fprintf(fs,
                          " %2d %31.31s 0x%08x %19.19s %4d,%4d %4dx%4d |     %s\n",
                          i, " ", (unsigned int)o, " ", x, y, w, h,
                          evas_object_visible_get(o) ? "O" : "X");
               }
          }

        o = evas_object_below_get(o);

        if (wname) free(wname);
        if (wclas) free(wclas);
        wname = wclas = NULL;
        pid = cid = 0;
        found = 0;
        i++;
     }
   fprintf(fs, "E-------------------------------------------------------------------------------------------------------------------------------------\n");
   fflush(fs);

   if (to_file) fclose(fs);
}

static E_Comp_Win *
_e_mod_comp_fullscreen_check(E_Comp *c)
{
   E_Comp_Win *cw;
   if (!c->wins) return NULL;
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((!cw->visible) || (cw->input_only) || (cw->invalid))
          continue;
        if ((cw->x == 0) && (cw->y == 0)
            && ((cw->x + cw->w) >= c->man->w)
            && ((cw->y + cw->h) >= c->man->h)
            && (!cw->argb) && (!cw->shaped))
          {
             return cw;
          }
        return NULL;
     }
   return NULL;
}

static inline Eina_Bool
_e_mod_comp_shaped_check(int w,
                         int h,
                         const Ecore_X_Rectangle *rects,
                         int num)
{
   if ((!rects) || (num < 1)) return EINA_FALSE;
   if (num > 1) return EINA_TRUE;
   if ((rects[0].x == 0) && (rects[0].y == 0) &&
       ((int)(rects[0].width) == w) && ((int)(rects[0].height) == h))
     return EINA_FALSE;
   return EINA_TRUE;
}

static inline Eina_Bool
_e_mod_comp_win_shaped_check(const E_Comp_Win *cw,
                             const Ecore_X_Rectangle *rects,
                             int num)
{
   return _e_mod_comp_shaped_check(cw->w, cw->h, rects, num);
}

static void
_e_mod_comp_win_shape_rectangles_apply(E_Comp_Win *cw,
                                       const Ecore_X_Rectangle *rects,
                                       int num)
{
   Eina_List *l;
   Evas_Object *o;
   int i;

   if (!_e_mod_comp_win_shaped_check(cw, rects, num))
     {
        rects = NULL;
     }
   if (rects)
     {
        unsigned int *pix, *p;
        unsigned char *spix, *sp;
        int w, h, px, py;

        evas_object_image_size_get(cw->obj, &w, &h);
        if ((w > 0) && (h > 0))
          {
             if (cw->native)
               {
                  fprintf(stderr,
                          "BUGGER: shape with native surface. cw=%p\n",
                          cw);
                  return;
               }

             evas_object_image_native_surface_set(cw->obj, NULL);
             evas_object_image_alpha_set(cw->obj, 1);
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
                  evas_object_image_alpha_set(o, 1);
               }
             pix = evas_object_image_data_get(cw->obj, 1);
             if (pix)
               {
                  spix = calloc(w * h, sizeof(unsigned char));
                  if (spix)
                    {
                       for (i = 0; i < num; i++)
                         {
                            int rx, ry, rw, rh;

                            rx = rects[i].x; ry = rects[i].y;
                            rw = rects[i].width; rh = rects[i].height;
                            E_RECTS_CLIP_TO_RECT(rx, ry, rw, rh, 0, 0, w, h);
                            sp = spix + (w * ry) + rx;
                            for (py = 0; py < rh; py++)
                              {
                                 for (px = 0; px < rw; px++)
                                   {
                                      *sp = 0xff; sp++;
                                   }
                                 sp += w - rw;
                              }
                         }
                       sp = spix;
                       p = pix;
                       for (py = 0; py < h; py++)
                         {
                            for (px = 0; px < w; px++)
                              {
                                 unsigned int mask, imask;
                                 mask = ((unsigned int)(*sp)) << 24;
                                 imask = mask >> 8;
                                 imask |= imask >> 8;
                                 imask |= imask >> 8;
                                 *p = mask | (*p & imask);
                                 sp++;
                                 p++;
                              }
                         }
                       free(spix);
                    }
                  evas_object_image_data_set(cw->obj, pix);
                  evas_object_image_data_update_add(cw->obj, 0, 0, w, h);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_set(o, pix);
                       evas_object_image_data_update_add(o, 0, 0, w, h);
                    }
               }
          }
     }
   else
     {
        if (cw->shaped)
          {
             unsigned int *pix, *p;
             int w, h, px, py;

             evas_object_image_size_get(cw->obj, &w, &h);
             if ((w > 0) && (h > 0))
               {
                  if (cw->native)
                    {
                       fprintf(stderr,
                               "BUGGER: shape with native surface? cw=%p\n",
                               cw);
                       return;
                    }

                  evas_object_image_alpha_set(cw->obj, 0);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_alpha_set(o, 1);
                    }
                  pix = evas_object_image_data_get(cw->obj, 1);
                  if (pix)
                    {
                       p = pix;
                       for (py = 0; py < h; py++)
                         {
                            for (px = 0; px < w; px++)
                              *p |= 0xff000000;
                         }
                    }
                  evas_object_image_data_set(cw->obj, pix);
                  evas_object_image_data_update_add(cw->obj, 0, 0, w, h);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_set(o, pix);
                       evas_object_image_data_update_add(o, 0, 0, w, h);
                    }

               }
          }
        // dont need to fix alpha chanel as blending
        // should be totally off here regardless of
        // alpha channel content
     }
}

static void
_e_mod_comp_win_free_xim(E_Comp_Win *cw)
{
   if (cw->xim)
     {
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
     }
}

static void
_e_mod_comp_win_update(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;
   E_Update_Rect *r;
   int i;

   if (_comp_mod->conf->grab) ecore_x_grab();
   cw->update = 0;

   if (cw->rotating)
     {
        if (cw->needpix)
          {
             cw->needpix = 0;
             cw->pw = cw->w;
             cw->ph = cw->h;
          }

        if (cw->resize_hide) cw->resize_hide = EINA_FALSE;

        Eina_Bool res;
        res = e_mod_comp_rotation_update(cw->rotobj,
                                         cw->up,
                                         cw->x, cw->y,
                                         cw->w, cw->h,
                                         cw->border);
        if (res)
          {
             if ((cw->visible) && (cw->dmg_updates > 0))
               {
                  Evas_Object *shobj = e_mod_comp_rotation_get_shobj(cw->rotobj);
                  if (shobj)
                    {
                       if (!evas_object_visible_get(shobj))
                         {
                            evas_object_show(cw->obj);
                            evas_object_show(shobj);
                            e_mod_comp_rotation_show_effect(cw->rotobj);
                            _e_mod_comp_win_inc_animating(cw);

                         }
                       else if(e_mod_comp_rotation_angle_is_changed(cw->rotobj))
                         {
                            e_mod_comp_rotation_request_effect(cw->rotobj);
                            _e_mod_comp_win_inc_animating(cw);
                         }
                    }
               }
          }
        if (_comp_mod->conf->grab) ecore_x_ungrab();
        return;
     }

   if (cw->argb)
     {
        if (cw->rects)
          {
             free(cw->rects);
             cw->rects = NULL;
             cw->rects_num = 0;
          }
     }
   else
     {
        if (cw->shape_changed)
          {
             if (cw->rects)
               {
                  free(cw->rects);
                  cw->rects = NULL;
                  cw->rects_num = 0;
               }
             ecore_x_pixmap_geometry_get(cw->win, NULL, NULL, &(cw->w), &(cw->h));
             cw->rects = ecore_x_window_shape_rectangles_get(cw->win, &(cw->rects_num));
             if (cw->rects)
               {
                  int int_w, int_h;
                  for (i = 0; i < cw->rects_num; i++)
                    {
                       int_w = (int)cw->rects[i].width;
                       int_h = (int)cw->rects[i].height;
                       E_RECTS_CLIP_TO_RECT(cw->rects[i].x,
                                            cw->rects[i].y,
                                            int_w,
                                            int_h,
                                            0, 0, cw->w, cw->h);
                       cw->rects[i].width = (unsigned int)int_w;
                       cw->rects[i].height = (unsigned int)int_h;
                    }
               }
             if (!_e_mod_comp_win_shaped_check(cw, cw->rects, cw->rects_num))
               {
                  free(cw->rects);
                  cw->rects = NULL;
                  cw->rects_num = 0;
               }
             if ((cw->rects) && (!cw->shaped)) cw->shaped = 1;
             else if ((!cw->rects) && (cw->shaped)) cw->shaped = 0;
          }
     }

   if ((cw->needpix) && (cw->dmg_updates <= 0))
     {
        if (_comp_mod->conf->grab) ecore_x_ungrab();
        return;
     }

   if ((!cw->pixmap) || (cw->needpix))
     {
        Ecore_X_Pixmap pm;
        pm = ecore_x_composite_name_window_pixmap_get(cw->win);
        if (pm)
          {
             Ecore_X_Pixmap oldpm;

             cw->needpix = 0;
             if (cw->xim) cw->needxim = 1;
             oldpm = cw->pixmap;
             cw->pixmap = pm;

             if (cw->pixmap)
               {
                  ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
                  // pixmap's size is not equal with window's size case
                  if (!((cw->pw == cw->w) && (cw->ph == cw->h)))
                    {
                        L(LT_EFFECT,
                          "[COMP] pixmap is not prepared yet!! win: 0x%07x \n",
                          cw->win);
                        cw->pw = cw->w;
                        cw->ph = cw->h;
                        cw->pixmap = oldpm;
                        cw->needpix = 1;
                        ecore_x_pixmap_free(pm);
                        if (_comp_mod->conf->grab) ecore_x_ungrab();
                        return;
                    }
               }
             else
               {
                  cw->pw = 0;
                  cw->ph = 0;
               }
             if ((cw->pw <= 0) || (cw->ph <= 0))
               {
                  if (cw->native)
                    {
                       evas_object_image_native_surface_set(cw->obj, NULL);
                       cw->native = 0;
                       EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                         {
                            evas_object_image_native_surface_set(o, NULL);
                         }
                    }
                  if (cw->pixmap)
                    {
                       ecore_x_pixmap_free(cw->pixmap);
                       cw->pixmap = 0;
                    }
                  cw->pw = 0;
                  cw->ph = 0;
               }
             ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
             cw->native = 0;
             e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
             e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
             if (oldpm)
               {
                  if (cw->native)
                    {
                       cw->native = 0;
                       if (!((cw->pw > 0) && (cw->ph > 0)))
                         {
                            evas_object_image_native_surface_set(cw->obj, NULL);
                            EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                              {
                                 evas_object_image_native_surface_set(o, NULL);
                              }
                         }
                    }
                  ecore_x_pixmap_free(oldpm);
               }
          }
     }

   if (!((cw->pw > 0) && (cw->ph > 0)))
     {
        if (_comp_mod->conf->grab) ecore_x_ungrab();
        return;
     }

   // update obj geometry when task switcher is not open
   // or task switcher is open and new window is added
   cw->defer_move_resize = EINA_FALSE;
   if ( !cw->c->switcher_animating ||
        !cw->c->switcher ||
       (cw->c->switcher && !cw->first_show_worked) ||
       (cw->c->switcher && cw->win_type == WIN_INDICATOR))
     {
        evas_object_move(cw->shobj, cw->x, cw->y);
        evas_object_resize(cw->shobj,
                      cw->pw + (cw->border * 2),
                      cw->ph + (cw->border * 2));
     }
   else
     {
        cw->defer_move_resize = EINA_TRUE;
     }

   if ((cw->c->gl)
       && (_comp_mod->conf->texture_from_pixmap)
       && (!cw->shaped)
       && (!cw->rects))
     {
        evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_size_set(o, cw->pw, cw->ph);
          }
        if (!cw->native)
          {
             Evas_Native_Surface ns;

             ns.version = EVAS_NATIVE_SURFACE_VERSION;
             ns.type = EVAS_NATIVE_SURFACE_X11;
             ns.data.x11.visual = cw->vis;
             ns.data.x11.pixmap = cw->pixmap;
             evas_object_image_native_surface_set(cw->obj, &ns);
             cw->native = 1;
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, &ns);
               }
          }

        r = e_mod_comp_update_rects_get(cw->up);
        if (r)
          {
             e_mod_comp_update_clear(cw->up);
             for (i = 0; r[i].w > 0; i++)
               {
                  int x, y, w, h;
                  x = r[i].x; y = r[i].y;
                  w = r[i].w; h = r[i].h;
                  evas_object_image_data_update_add(cw->obj, x, y, w, h);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_update_add(o, x, y, w, h);
                    }

               }
             free(r);
          }
        else
          cw->update = 1;
     }
   else
     {
        if (cw->native)
          {
             evas_object_image_native_surface_set(cw->obj, NULL);
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
               }
             cw->native = 0;
          }
        if (cw->needxim)
          {
             cw->needxim = 0;
             if (cw->xim)
               {
                  evas_object_image_size_set(cw->obj, 1, 1);
                  evas_object_image_data_set(cw->obj, NULL);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_size_set(o, 1, 1);
                       evas_object_image_data_set(o, NULL);
                    }
                  ecore_x_image_free(cw->xim);
                  cw->xim = NULL;
               }
          }
        if (!cw->xim)
          {
             if ((cw->xim = ecore_x_image_new(cw->pw, cw->ph, cw->vis, cw->depth)))
               e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
          }
        r = e_mod_comp_update_rects_get(cw->up);
        if (r)
          {
             Eina_Bool get_image_failed = 0;
             if (cw->xim)
               {
                  unsigned int *pix;
                  pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                  evas_object_image_data_set(cw->obj, pix);
                  evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_set(o, pix);
                       evas_object_image_size_set(o, cw->pw, cw->ph);
                    }

                  e_mod_comp_update_clear(cw->up);
                  for (i = 0; r[i].w > 0; i++)
                    {
                       int x, y, w, h;
                       x = r[i].x; y = r[i].y;
                       w = r[i].w; h = r[i].h;
                       if (!ecore_x_image_get(cw->xim, cw->pixmap, x, y, x, y, w, h))
                         {
                            e_mod_comp_update_add(cw->up, x, y, w, h);
                            cw->update = 1;
                            get_image_failed = 1;
                         }
                       else
                         {
                            pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                            evas_object_image_data_set(cw->obj, pix);
                            evas_object_image_data_update_add(cw->obj, x, y, w, h);
                            EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                              {
                                 evas_object_image_data_set(o, pix);
                                 evas_object_image_data_update_add(o, x, y, w, h);
                              }
                         }
                    }
               }
             free(r);

             if (!get_image_failed)
               {
                  if (cw->shaped)
                    _e_mod_comp_win_shape_rectangles_apply(cw,
                                                           cw->rects,
                                                           cw->rects_num);
                  else
                    {
                       if (cw->shape_changed)
                         _e_mod_comp_win_shape_rectangles_apply(cw,
                                                                cw->rects,
                                                                cw->rects_num);
                    }
               }
             cw->shape_changed = 0;
          }
        else
          cw->update = 1;
     }

   if (cw->resize_hide)
     {
        evas_object_show(cw->shobj);
        cw->resize_hide = EINA_FALSE;
     }

   if ((!cw->rotating)
       && (!cw->update)
       && (cw->visible)
       && (cw->dmg_updates >= 1))
     {
        if (!evas_object_visible_get(cw->shobj)
            && !(cw->animate_hide))
          {
             if (!cw->hidden_override)
               evas_object_show(cw->shobj);

             L(LT_EFFECT,
               "[COMP] WIN_EFFECT : Show signal Emit -> win:0x%08x\n",
               cw->win);

             _e_mod_comp_window_show_effect(cw);
          }
     }

   if (cw->ready_win_rot_effect)
     _e_mod_comp_win_rotation_begin(cw, EINA_FALSE);

   if (_comp_mod->conf->grab) ecore_x_ungrab();
}

static void
_e_mod_comp_pre_swap(void *data,
                     Evas *e __UNUSED__)
{
   E_Comp *c = data;
   L(LT_EVENT_X, "[COMP]    %15.15s\n", "SWAP");
   if (_comp_mod->conf->grab)
     {
        if (c->grabbed)
          {
             ecore_x_ungrab();
             c->grabbed = 0;
          }
     }
}

static Eina_Bool
_e_mod_comp_cb_delayed_update_timer(void *data)
{
   E_Comp *c = data;
   _e_mod_comp_render_queue(c);
   c->new_up_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_comp_cb_update(E_Comp *c)
{
   Eina_List *l;
   Evas_Object *o;
   E_Comp_Win *cw;
   Eina_List *new_updates = NULL; // for failed pixmap fetches - get them next frame
   Eina_List *update_done = NULL;

   c->update_job = NULL;
   if (c->nocomp) goto nocomp;
   if (_comp_mod->conf->grab)
     {
        ecore_x_grab();
        ecore_x_sync();
        c->grabbed = 1;
     }
   EINA_LIST_FREE(c->updates, cw)
     {
        if (!cw) continue;
        if (_comp_mod->conf->efl_sync)
          {
             if (((cw->counter) && (cw->drawme)) || (!cw->counter))
               {
                  _e_mod_comp_win_update(cw);
                  if (cw->drawme)
                    update_done = eina_list_append(update_done, cw);
                  cw->drawme = 0;
               }
             else
               cw->update = 0;
          }
        else
          _e_mod_comp_win_update(cw);
        if (cw->update)
          new_updates = eina_list_append(new_updates, cw);
     }

   _e_mod_comp_fps_update(c);
   if (_comp_mod->conf->fps_show)
     {
        char buf[128];
        double fps = 0.0, t, dt;
        int i;
        Evas_Coord x = 0, y = 0, w = 0, h = 0;
        E_Zone *z;

        t = ecore_time_get();

        if (_comp_mod->conf->fps_average_range < 1)
          _comp_mod->conf->fps_average_range = 30;
        else if (_comp_mod->conf->fps_average_range > 120)
          _comp_mod->conf->fps_average_range = 120;

        dt = t - c->frametimes[_comp_mod->conf->fps_average_range - 1];

        if (dt > 0.0) fps = (double)_comp_mod->conf->fps_average_range / dt;
        else fps = 0.0;

        if (fps > 0.0) snprintf(buf, sizeof(buf), "FPS: %1.1f", fps);
        else snprintf(buf, sizeof(buf), "N/A");

        for (i = 121; i >= 1; i--) c->frametimes[i] = c->frametimes[i - 1];
        c->frametimes[0] = t;
        c->frameskip++;

        if (c->frameskip >= _comp_mod->conf->fps_average_range)
          {
             c->frameskip = 0;
             evas_object_text_text_set(c->fps_fg, buf);
          }

        evas_object_geometry_get(c->fps_fg, NULL, NULL, &w, &h);

        w += 8;
        h += 8;
        z = e_util_zone_current_get(c->man);

        if (z)
          {
            switch (_comp_mod->conf->fps_corner)
             {
              case 3: // bottom-right
               x = z->x + z->w - w;
               y = z->y + z->h - h;
               break;
              case 2: // bottom-left
               x = z->x;
               y = z->y + z->h - h;
               break;
              case 1: // top-right
               x = z->x + z->w - w;
               y = z->y;
               break;
              default: // 0 // top-left
               x = z->x;
               y = z->y;
               break;
             }
          }
        evas_object_move(c->fps_bg, x, y);
        evas_object_resize(c->fps_bg, w, h);
        evas_object_move(c->fps_fg, x + 4, y + 4);
     }

   if (_comp_mod->conf->lock_fps)
     {
        ecore_evas_manual_render(c->ee);
     }
   if (_comp_mod->conf->efl_sync)
     {
        EINA_LIST_FREE(update_done, cw)
          {
             if (!cw) continue;
             ecore_x_sync_counter_inc(cw->counter, 1);
          }
     }
   if (_comp_mod->conf->grab)
     {
        if (c->grabbed)
          {
             c->grabbed = 0;
             ecore_x_ungrab();
          }
     }
   if (new_updates)
     {
        if (c->new_up_timer) ecore_timer_del(c->new_up_timer);
        c->new_up_timer =
          ecore_timer_add(0.001, _e_mod_comp_cb_delayed_update_timer, c);
     }
   c->updates = new_updates;
   if (!c->animating) c->render_overflow--;

nocomp:
   cw = _e_mod_comp_fullscreen_check(c);
   if (cw)
     {
        if (_comp_mod->conf->nocomp_fs)
          {
             if (!c->nocomp)
               {
                  printf("NOCOMP!\n");
                  printf("kill comp %x\n", cw->win);
                  c->nocomp = 1;
                  c->render_overflow = OVER_FLOW;
                  ecore_x_window_hide(c->win);
                  cw->nocomp = 1;
                  if (cw->redirected)
                    {
                       printf("^^^^ undirect1 %x\n", cw->win);
                       ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
                       cw->redirected = 0;
                       cw->pw = 0;
                       cw->ph = 0;
                    }
                  if (cw->pixmap)
                    {
                       ecore_x_pixmap_free(cw->pixmap);
                       cw->pixmap = 0;
                       cw->pw = 0;
                       cw->ph = 0;
                       ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
                    }
                  if (cw->xim)
                    {
                       evas_object_image_size_set(cw->obj, 1, 1);
                       evas_object_image_data_set(cw->obj, NULL);
                       EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                         {
                            evas_object_image_size_set(o, 1, 1);
                            evas_object_image_data_set(o, NULL);
                         }
                       ecore_x_image_free(cw->xim);
                       cw->xim = NULL;
                    }
                  if (cw->damage)
                    {
                       Ecore_X_Region parts;

                       eina_hash_del(damages, e_util_winid_str_get(cw->damage), cw);
                       parts = ecore_x_region_new(NULL, 0);
                       ecore_x_damage_subtract(cw->damage, 0, parts);
                       ecore_x_region_free(parts);
                       ecore_x_damage_free(cw->damage);
                       cw->damage = 0;
                    }
                  if (cw->update_timeout)
                    {
                       ecore_timer_del(cw->update_timeout);
                       cw->update_timeout = NULL;
                    }
                  if (cw->update)
                    {
                       cw->update = 0;
                       cw->c->updates = eina_list_remove(cw->c->updates, cw);
                    }
                  if (cw->counter)
                    {
                       printf("nosync\n");
                       Ecore_X_Window _win = _e_mod_comp_win_get_client_xid(cw);
                       ecore_x_e_comp_sync_cancel_send(_win);
                       ecore_x_sync_counter_inc(cw->counter, 1);
                    }
                  _e_mod_comp_render_queue(c);
               }
          }
     }
   else
     {
        if (c->nocomp)
          {
             printf("COMP!\n");
             c->nocomp = 0;
             c->render_overflow = OVER_FLOW;
             ecore_x_window_show(c->win);
             EINA_INLIST_FOREACH(c->wins, cw)
               {
                  if (!cw->nocomp) continue;
                  cw->nocomp = 0;
                  printf("restore comp %x --- %x\n", cw->win, cw->pixmap);
                  if (cw->pixmap) ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
                  cw->pw = 0;
                  cw->ph = 0;
                  cw->native = 0;
                  if (!cw->damage)
                    {
                       cw->damage = ecore_x_damage_new
                         (cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
                       eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
                    }
                  if (!cw->redirected)
                    {
                       printf("^^^^ redirect2 %x\n", cw->win);
                       printf("  redr\n");
                       ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
                       cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
                       if (cw->pixmap)
                         ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
                       else
                         {
                            cw->pw = 0;
                            cw->ph = 0;
                         }
                       printf("  %x %ix%i\n", cw->pixmap, cw->pw, cw->ph);
                       if ((cw->pw <= 0) || (cw->ph <= 0))
                         {
                            ecore_x_pixmap_free(cw->pixmap);
                            cw->pixmap = 0;
                         }
                       ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
                       cw->redirected = 1;
                       cw->dmg_updates = 0;
                       e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
                       e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
                    }
                  if (cw->visible)
                    {
                       if (!cw->hidden_override) evas_object_show(cw->shobj);
                       // no need for effect
                       cw->pending_count++;
                       e_manager_comp_event_src_visibility_send
                          (cw->c->man, (E_Manager_Comp_Source *)cw,
                              _e_mod_comp_cb_pending_after, cw->c);

                    }
                  _e_mod_comp_win_render_queue(cw);
                  if (cw->counter)
                    {
                       Ecore_X_Window _win = _e_mod_comp_win_get_client_xid(cw);
                       ecore_x_e_comp_sync_begin_send(_win);
                    }
               }
          }
     }

   if (c->render_overflow <= 0)
     {
        c->render_overflow = 0;
        if (c->render_animator) c->render_animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_mod_comp_cb_job(void *data)
{
   _e_mod_comp_cb_update(data);
}

static Eina_Bool
_e_mod_comp_cb_animator(void *data)
{
   return _e_mod_comp_cb_update(data);
}

static void
_e_mod_comp_render_queue(E_Comp *c)
{
   if (_comp_mod->conf->lock_fps)
     {
        if (c->render_animator)
          {
             c->render_overflow = OVER_FLOW;
             return;
          }
        c->render_animator = ecore_animator_add(_e_mod_comp_cb_animator, c);
     }
   else
     {
        if (c->update_job)
          {
             ecore_job_del(c->update_job);
             c->update_job = NULL;
             c->render_overflow = 0;
          }
        c->update_job = ecore_job_add(_e_mod_comp_cb_job, c);
     }
}

void
_e_mod_comp_win_render_queue(E_Comp_Win *cw)
{
   _e_mod_comp_render_queue(cw->c);
}

E_Comp *
_e_mod_comp_find(Ecore_X_Window root)
{
   Eina_List *l;
   E_Comp *c;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        if (c->man->root == root) return c;
     }
   return NULL;
}

static E_Comp_Win *
_e_mod_comp_win_find(Ecore_X_Window win)
{
   return eina_hash_find(windows, e_util_winid_str_get(win));
}

static E_Comp_Win *
_e_mod_comp_border_client_find(Ecore_X_Window win)
{
   return eina_hash_find(borders, e_util_winid_str_get(win));
}

static E_Comp_Win *
_e_mod_comp_win_damage_find(Ecore_X_Damage damage)
{
   return eina_hash_find(damages, e_util_winid_str_get(damage));
}

static Eina_Bool
_e_mod_comp_win_is_borderless(E_Comp_Win *cw)
{
   if (!cw->bd) return 1;
   if ((cw->bd->client.border.name) &&
       (!strcmp(cw->bd->client.border.name, "borderless")))
     return 1;
   return 0;
}

Eina_Bool
_e_mod_comp_fake_launch_timeout(void *data)
{
   E_Comp *c = (E_Comp*)data;
   E_Comp_Win *background_cw = NULL;
   E_Comp_Win *find_cw = NULL;

   if (!c->fake_launch_state) return 0;

   if (c->fake_launch_timeout)
     {
        ecore_timer_del(c->fake_launch_timeout);
        c->fake_launch_timeout  = NULL;
     }

   c->fake_launch_state = EINA_FALSE;
   c->fake_win = 0;
   c->fake_launch_done = EINA_FALSE;

   // background hide effect
   background_cw = _e_mod_comp_win_find_fake_background_win(c);
   if (background_cw)
     {
        EINA_INLIST_FOREACH(c->wins, find_cw)
          {
             if ((find_cw->invalid)
                 || (find_cw->input_only)
                 || (find_cw->win == background_cw->win)
                 || _e_mod_comp_is_indicator_window(find_cw->bd))
               {
                 continue;
               }
             else
               {
                 if (evas_object_visible_get(find_cw->shobj))
                   {
                      // do hide window which is not related window animation effect.
                      find_cw->animate_hide = EINA_TRUE;
                      evas_object_hide(find_cw->shobj);
                   }
               }
          }

        //show background image when background show animation is emitted.
        evas_object_stack_below(c->bg_img, evas_object_bottom_get(c->evas));

        background_cw->animate_hide = EINA_FALSE;

        edje_object_signal_emit(background_cw->shobj,
                                "e,state,background,visible,off",
                                "e");
        L(LT_EFFECT,
          "[COMP] WIN_EFFECT : Background Show Effect signal Emit -> win:0x%08x\n",
          background_cw->win);

        _e_mod_comp_win_inc_animating(background_cw);
     }

   // background show effect
   edje_object_signal_emit(c->fake_img_shobj,
                           "fake,state,visible,off",
                           "fake");
   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Fake Hide signal Emit\n");

   return 0;
}

static void
_e_mod_comp_screen_lock(E_Comp *c)
{
   if (!_comp_mod || !c) return;
   if (!_comp_mod->conf->use_lock_screen) return;
   if (c->lock_screen == 1) return;
   if (c->lock_screen_timeout) return;

   if (!_comp_mod->conf->lock_fps)
     ecore_evas_manual_render_set(c->ee, 1);

   c->lock_screen_timeout = ecore_timer_add(_comp_mod->conf->max_lock_screen_time,
                                            _e_mod_comp_screen_lock_timeout,
                                            c);
   c->lock_screen = 1;
}

static void
_e_mod_comp_screen_unlock(E_Comp *c)
{
   E_Comp_Win *cw;

   if (!_comp_mod || !c) return;
   if (!_comp_mod->conf->use_lock_screen) return;
   if (c->lock_screen == 0) return;
   if (c->lock_screen_timeout)
     {
        ecore_timer_del(c->lock_screen_timeout);
        c->lock_screen_timeout = NULL;
     }

   /////////////////////////////////////////////////////////
   // To ensure the correct screen appearance
   // 1. remove all cw->update_timeout
   // 2. clear c->update_job
   // 3. clear c->updates using _e_mod_comp_cb_update()
   // 4. swap buffer
   /////////////////////////////////////////////////////////

   // remove all cw->update_timeout
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((!cw->visible) || (cw->input_only) || (cw->invalid))
           continue;
        if (cw->counter && cw->update_timeout)
          {
             cw->update = 0;
             _e_mod_comp_win_damage_timeout((void*)cw);
          }
     }

   // clear c->update_job
   if (c->update_job)
     {
        ecore_job_del(c->update_job);
        c->update_job = NULL;
        c->render_overflow = 0;
     }

   // clear c->updates
   if (c->updates) _e_mod_comp_cb_update(c);

   ecore_evas_manual_render(c->ee);

   if (!_comp_mod->conf->lock_fps)
     ecore_evas_manual_render_set(c->ee, 0);

   c->lock_screen = 0;
}

static Eina_Bool
_e_mod_comp_screen_lock_timeout(void *data)
{
   E_Comp *c = (E_Comp*)data;
   _e_mod_comp_screen_unlock(c);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_comp_win_rotation_begin_timeout(void *data)
{
   E_Comp_Win *cw = (E_Comp_Win*)data;
   if (!cw) return EINA_FALSE;

   fprintf(stderr, "[E17-comp] %s(%d) w:0x%08x\n",
           __func__, __LINE__,
           _e_mod_comp_win_get_client_xid(cw));

   _e_mod_comp_win_rotation_begin(cw, EINA_TRUE);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_comp_win_rotation_end_timeout(void *data)
{
   E_Comp_Win *cw = (E_Comp_Win*)data;
   if (!cw) return EINA_FALSE;

   fprintf(stderr,"[E17-comp] %s(%d) w:0x%08x\n",
           __func__, __LINE__,
           _e_mod_comp_win_get_client_xid(cw));

   _e_mod_comp_win_rotation_done((void*)cw,
                                 NULL,
                                 NULL,
                                 NULL);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_comp_win_damage_timeout(void *data)
{
   E_Comp_Win *cw = data;

   if (!cw->update)
     {
        if (cw->update_timeout)
          {
             ecore_timer_del(cw->update_timeout);
             cw->update_timeout = NULL;
          }
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   cw->drawme = 1;
   _e_mod_comp_win_render_queue(cw);
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_comp_object_del(void *data,
                       void *obj)
{
   E_Comp_Win *cw = data;

   _e_mod_comp_win_render_queue(cw);

   _e_mod_comp_rotation_release(cw);

   if (obj == cw->bd)
     {
        if (cw->counter)
          {
             Ecore_X_Window _w = _e_mod_comp_win_get_client_xid(cw);
             ecore_x_e_comp_sync_cancel_send(_w);
             ecore_x_sync_counter_inc(cw->counter, 1);
          }
        if (cw->bd) eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
        cw->bd = NULL;
        evas_object_data_del(cw->shobj, "border");
     }
   else if (obj == cw->pop)
     {
        cw->pop = NULL;
        evas_object_data_del(cw->shobj, "popup");
     }
   else if (obj == cw->menu)
     {
        cw->menu = NULL;
        evas_object_data_del(cw->shobj, "menu");
     }
   if (cw->dfn)
     {
        e_object_delfn_del(obj, cw->dfn);
        cw->dfn = NULL;
     }
}

void
_e_mod_comp_done_defer(E_Comp_Win *cw)
{
   _e_mod_comp_win_dec_animating(cw);

   if (cw->defer_raise == EINA_TRUE)
     {
        L(LT_EFFECT,
          "[COMP] w:0x%08x force win to raise. bd:%s\n",
          _e_mod_comp_win_get_client_xid(cw),
          cw->bd ? "O" : "X");

        E_Comp_Win *_cw;
        EINA_INLIST_FOREACH(cw->c->wins, _cw)
          {
             evas_object_raise(_cw->shobj);
             if (cw->c->use_bg_img && _cw->win_rot_effect)
               {
                  evas_object_stack_below(cw->c->bg_img, _cw->shobj);
               }
          }
        cw->defer_raise = EINA_FALSE;
        edje_object_signal_emit(cw->shobj,
                                "e,state,raise_above_post,on",
                                "e");
     }
   cw->force = 1;
   if (cw->defer_hide)
     {
        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x force win to hide. bd:%s\n",
          "EDJ_DONE", _e_mod_comp_win_get_client_xid(cw),
          cw->bd ? "O" : "X");

        _e_mod_comp_win_hide(cw);
     }
   cw->force = 1;
   if (cw->delete_me)
     {
        L(LT_EVENT_X,
          "[COMP] %31s w:0x%08x force win to delete. bd:%s\n",
          "EDJ_DONE", _e_mod_comp_win_get_client_xid(cw),
          cw->bd ? "O" : "X");

        _e_mod_comp_win_del(cw);
     }
   else cw->force = 0;
}

static void
_e_mod_comp_show_done(void *data,
                      Evas_Object *obj __UNUSED__,
                      const char *emission __UNUSED__,
                      const char *source __UNUSED__)
{
   E_Comp_Win *cw = data;

   LOG(LOG_DEBUG, "LAUNCH", "[e17:Application:Launching:done]");
   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Show done -> win:0x%08x\n",
     cw->win);

   if ((cw->win_type == WIN_TASK_MANAGER)
       && (cw->c->switcher == EINA_TRUE))
     {
        int i = 0;
        Evas_Coord x, y, w, h;
        E_Comp_Win *_cw;
        cw->c->switcher_animating = EINA_FALSE;
        EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
          {
             if ((_cw->visible == 1)
                 && evas_object_visible_get(_cw->shobj)
                 && evas_object_visible_get(_cw->obj)
                 && !_e_mod_comp_is_indicator_window(_cw->bd))
               {
                  evas_object_geometry_get(_cw->shobj,
                                           &x, &y, &w, &h);
                  if (w == _cw->c->screen_w
                      && h == _cw->c->screen_h)
                    {
                       i++;
                       // return all windows to original position
                       evas_object_move(_cw->shobj, 0, 0);
                    }
                  if ( _cw->defer_move_resize )
                    {
                       evas_object_move(_cw->shobj, _cw->x, _cw->y );
                       evas_object_resize(_cw->shobj, _cw->pw + (_cw->border * 2),
                                                      _cw->ph + (_cw->border * 2));
                       _cw->defer_move_resize = EINA_FALSE;
                    }
               }
          }
        _e_mod_comp_disable_effect_stage(_e_mod_comp_get());
     }
   cw->show_done = EINA_TRUE;
   _e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_hide_done(void *data,
                      Evas_Object *obj __UNUSED__,
                      const char *emission __UNUSED__,
                      const char *source __UNUSED__)
{
   E_Comp_Win *cw = data;

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Hide done -> win:0x%08x\n",
     cw->win);

   if ((cw->win_type == WIN_TASK_MANAGER)
       && (cw->c->switcher))
     {
        E_Comp_Win *_cw;
        cw->c->switcher_animating = EINA_FALSE;
        if (cw->c->selected_pos > 0) cw->c->selected_pos = 0;
        // task switcher is closed
        cw->c->switcher = EINA_FALSE;
        _e_mod_comp_disable_effect_stage(_e_mod_comp_get());

        EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
          {
             if ( (_cw->visible == 1)
                  && evas_object_visible_get(_cw->shobj)
                  && !_e_mod_comp_is_indicator_window(_cw->bd)
                  && (_cw->defer_move_resize == EINA_TRUE) )
               {
                   evas_object_move(_cw->shobj, _cw->x, _cw->y );
                   evas_object_resize(_cw->shobj, _cw->pw + (_cw->border * 2),
                                                  _cw->ph + (_cw->border * 2));
                   _cw->defer_move_resize = EINA_FALSE;
               }
          }
     }

   cw->show_done = EINA_FALSE;
   _e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_raise_above_hide_done(void *data,
                                  Evas_Object *obj __UNUSED__,
                                  const char *emission __UNUSED__,
                                  const char *source __UNUSED__)
{
   E_Comp_Win *cw = data;
   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Raise Above Hide done -> win:0x%08x\n",
     cw->win);
   if (!cw) return;
   _e_mod_comp_disable_effect_stage(_e_mod_comp_get());
   _e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_fake_show_done(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   E_Comp *c = data;
   E_Comp_Win* cw = NULL;

   L(LT_EFFECT,
     "[COMP] FAKE_EFFECT : FAKE Show done\n");

   c->fake_launch_done = EINA_TRUE;
   if ((c->fake_win != 0)
       && (c->fake_launch_state))
     {
        cw = _e_mod_comp_win_find(c->fake_win);
        if (cw)
          {
             // send_noeffect_signal
             edje_object_signal_emit(cw->shobj,
                                     "e,state,visible,on,noeffect",
                                     "e");
             _e_mod_comp_disable_fake_launch(cw->c);
          }
      }
}

static void
_e_mod_comp_fake_hide_done(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   E_Comp *c = data;

   L(LT_EFFECT,
     "[COMP] FAKE_EFFECT : Hide done \n");

   _e_mod_comp_disable_touch_event_block(c);

   evas_object_hide(c->fake_img_shobj);
   edje_object_part_unswallow(c->fake_img_shobj, c->fake_img_obj);
   evas_object_del(c->fake_img_obj);

   utilx_ungrab_key(ecore_x_display_get(), c->win, KEY_SELECT);
   utilx_ungrab_key(ecore_x_display_get(), c->win, KEY_VOLUMEUP);
   utilx_ungrab_key(ecore_x_display_get(), c->win, KEY_VOLUMEDOWN);
   utilx_ungrab_key(ecore_x_display_get(), c->win, KEY_CAMERA);
}

static void
_e_mod_comp_background_show_done(void *data,
                                 Evas_Object *obj __UNUSED__,
                                 const char *emission __UNUSED__,
                                 const char *source __UNUSED__)
{
   E_Comp_Win *cw = data;

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Background Show done -> win:0x%08x\n",
     cw->win);

   cw->effect_stage = EINA_FALSE;
   _e_mod_comp_disable_effect_stage(cw->c);

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Background Show done (Background show animate_hide window) -> win:0x%08x\n",
     cw->win);

   _e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_background_hide_done(void *data,
                                 Evas_Object *obj __UNUSED__,
                                 const char *emission __UNUSED__,
                                 const char *source __UNUSED__)
{
   E_Comp_Win *cw = data;

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Background Hide done -> win:0x%08x\n",
     cw->win);

   cw->effect_stage = EINA_FALSE;
   _e_mod_comp_disable_effect_stage(cw->c);

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Background Hide done (Background hide animate_hide window) -> win:0x%08x\n",
     cw->win);

   _e_mod_comp_done_defer(cw);
}

// if Border is not existed then, return itself.
// otherwise, return itself or parent.
E_Comp_Win *
_e_mod_comp_win_transient_parent_find(E_Comp_Win *cw)
{
   Ecore_X_Window transient_parent;
   E_Comp_Win *parent_cw = NULL;
   E_Border *bd = NULL;
   if (cw->bd)
     {
        bd = cw->bd;
        do {
             transient_parent = bd->win;
             bd = bd->parent;
        } while (bd);

        parent_cw = _e_mod_comp_win_find(transient_parent);
        return parent_cw;
     }
   return cw;
}

E_Comp_Win *
_e_mod_comp_win_find_background_win(E_Comp_Win *cw)
{
   Eina_Inlist *wins_list;
   E_Comp_Win *find_cw = cw;
   if (!find_cw) return NULL;

   while ((wins_list = EINA_INLIST_GET(find_cw)->prev) != NULL)
     {
        find_cw = _EINA_INLIST_CONTAINER(find_cw, wins_list);
        if ((find_cw->w == find_cw->c->man->w) &&
            (find_cw->h == find_cw->c->man->h) &&
            (find_cw->x == 0) &&
            (find_cw->y == 0) &&
            (find_cw->visible) &&
            (find_cw->invalid == EINA_FALSE) &&
            (find_cw->input_only == EINA_FALSE))
          {
             return find_cw;
          }
     }
   return NULL;
}

E_Comp_Win *
_e_mod_comp_win_find_fake_background_win(E_Comp *c)
{
   E_Comp_Win *find_cw = NULL;
   if (!c) return NULL;

   EINA_INLIST_REVERSE_FOREACH(c->wins, find_cw)
     {
        if ((find_cw->w == c->man->w) &&
            (find_cw->h == c->man->h) &&
            (find_cw->x == 0) &&
            (find_cw->y == 0) &&
            (find_cw->visible) &&
            (find_cw->invalid == EINA_FALSE) &&
            (find_cw->input_only == EINA_FALSE))
          {
             return find_cw;
          }
     }
   return NULL;
}

static void
_e_mod_comp_win_sync_setup(E_Comp_Win *cw,
                           Ecore_X_Window win)
{
   if (!_comp_mod->conf->efl_sync) return;
   if (cw->bd)
     {
        if (_e_mod_comp_win_is_borderless(cw)
            || (_comp_mod->conf->loose_sync))
          cw->counter = ecore_x_e_comp_sync_counter_get(win);
        else
          {
             ecore_x_e_comp_sync_cancel_send(win);
             cw->counter = 0;
          }
     }
   else
     cw->counter = ecore_x_e_comp_sync_counter_get(win);

   if (cw->counter)
     {
        if (cw->bd)
          {
             E_Comp_Win *client_cw = _e_mod_comp_win_find(win);
             if (client_cw &&
                 client_cw->counter == cw->counter)
               {
                  ecore_x_sync_counter_inc(cw->counter, 1);
                  return;
               }
          }

        ecore_x_e_comp_sync_begin_send(win);
        ecore_x_sync_counter_inc(cw->counter, 1);
     }
}

static void
_e_mod_comp_win_shadow_setup(E_Comp_Win *cw)
{
   Evas_Object *o;
   Eina_List *l;
   int ok = 0;
   char buf[PATH_MAX];

   evas_object_image_smooth_scale_set(cw->obj, _comp_mod->conf->smooth_windows);
   EINA_LIST_FOREACH(cw->obj_mirror, l, o)
     {
        evas_object_image_smooth_scale_set(o, _comp_mod->conf->smooth_windows);
     }

   if (_comp_mod->conf->shadow_file)
     {
        ok = _e_mod_comp_shadow_policy(cw);
     }

   if (!ok)
     {
        fprintf(stdout,
                "[E17-comp] EDC Animation isn't loaded! win:0x%08x %s(%d) file:%s\n",
                cw->win, __func__, __LINE__, _comp_mod->conf->shadow_file);

        _e_mod_comp_get_edje_error(cw);

        if (_comp_mod->conf->shadow_style)
          {
             snprintf(buf, sizeof(buf), "e/comp/%s", _comp_mod->conf->shadow_style);
             ok = e_theme_edje_object_set(cw->shobj,
                                          "base/theme/borders",
                                          buf);
          }
        if (!ok)
          {
             ok = e_theme_edje_object_set(cw->shobj,
                                          "base/theme/borders",
                                          "e/comp/default");
          }
     }
   // fallback to local shadow.edj - will go when default theme supports this
   if (!ok)
     {
        fprintf(stdout,
                "[E17-comp] EDC Animation isn't loaded! win:0x%08x %s(%d)\n",
                cw->win, __func__, __LINE__);
        _e_mod_comp_get_edje_error(cw);
        snprintf(buf, sizeof(buf), "%s/shadow.edj", e_module_dir_get(_comp_mod->module));
        ok = edje_object_file_set(cw->shobj, buf, "shadow");
     }
   if (!edje_object_part_swallow(cw->shobj,
                                 "e.swallow.content",
                                 cw->obj))
     {
        fprintf(stdout,
                "[E17-comp] Window pixmap didn't swalloed! win:0x%08x %s(%d)\n",
                cw->win, __func__, __LINE__);
     }

   _e_mod_comp_get_edje_error(cw);
   edje_object_signal_emit(cw->shobj,
                           "e,state,shadow,off",
                           "e");

   if (cw->bd)
     {
        if (cw->bd->focused)
          edje_object_signal_emit(cw->shobj,
                                  "e,state,focus,on",
                                  "e");
        if (cw->bd->client.icccm.urgent)
          edje_object_signal_emit(cw->shobj,
                                  "e,state,urgent,on",
                                  "e");
     }
}

static Evas_Object *
_e_mod_comp_win_mirror_add(E_Comp_Win *cw)
{
   Evas_Object *o;

   if (!cw->c) return NULL;

   o = evas_object_image_filled_add(cw->c->evas);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   cw->obj_mirror = eina_list_append(cw->obj_mirror, o);

   if ((cw->pixmap) && (cw->pw > 0) && (cw->ph > 0))
     {
        unsigned int *pix;
        Eina_Bool alpha;
        int w, h;

        alpha = evas_object_image_alpha_get(cw->obj);
        evas_object_image_size_get(cw->obj, &w, &h);

        evas_object_image_alpha_set(o, alpha);

        if (cw->shaped)
          {
             pix = evas_object_image_data_get(cw->obj, 0);
             evas_object_image_data_set(o, pix);
             evas_object_image_size_set(o, w, h);
             evas_object_image_data_set(o, pix);
             evas_object_image_data_update_add(o, 0, 0, w, h);
          }
        else
          {
             if (cw->native)
               {
                  Evas_Native_Surface ns;

                  ns.version = EVAS_NATIVE_SURFACE_VERSION;
                  ns.type = EVAS_NATIVE_SURFACE_X11;
                  ns.data.x11.visual = cw->vis;
                  ns.data.x11.pixmap = cw->pixmap;
                  evas_object_image_size_set(o, w, h);
                  evas_object_image_native_surface_set(o, &ns);
                  evas_object_image_data_update_add(o, 0, 0, w, h);
               }
             else
               {
                  pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                  evas_object_image_data_set(o, pix);
                  evas_object_image_size_set(o, w, h);
                  evas_object_image_data_set(o, pix);
                  evas_object_image_data_update_add(o, 0, 0, w, h);
               }
          }
        evas_object_image_size_set(o, w, h);
        evas_object_image_data_update_add(o, 0, 0, w, h);
    }

   return o;
}


static E_Comp_Win *
_e_mod_comp_win_add(E_Comp *c,
                    Ecore_X_Window win)
{
   Ecore_X_Window_Attributes att;
   E_Comp_Win *cw;

   cw = calloc(1, sizeof(E_Comp_Win));
   if (!cw) return NULL;

   cw->win = win;
   cw->c = c;
   cw->bd = e_border_find_by_window(cw->win);
   cw->resize_hide = EINA_FALSE;
   cw->animate_hide = EINA_FALSE;
   cw->animatable = EINA_TRUE;
   cw->first_show_worked = EINA_FALSE;
   cw->show_done = EINA_FALSE;
   cw->effect_stage = EINA_FALSE;
   cw->win_type = WIN_NORMAL;
   cw->defer_move_resize = EINA_FALSE;
   //cw->ready_hide_effect = EINA_FALSE;

   _e_mod_comp_window_effect_policy(cw);

   if (_comp_mod->conf->grab) ecore_x_grab();
   if (cw->bd)
     {
        eina_hash_add(borders, e_util_winid_str_get(cw->bd->client.win), cw);
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->bd),
                                     _e_mod_comp_object_del, cw);

        if      (_e_mod_comp_is_normal_window(cw->bd)        ) cw->win_type = WIN_NORMAL;
        else if (_e_mod_comp_is_quickpanel_window(cw->bd)    ) cw->win_type = WIN_QUICKPANEL;
        else if (_e_mod_comp_is_task_manager_window(cw->bd)  ) cw->win_type = WIN_TASK_MANAGER;
        else if (_e_mod_comp_is_live_magazine_window(cw->bd) ) cw->win_type = WIN_LIVE_MAGAZINE;
        else if (_e_mod_comp_is_lock_screen_window(cw->bd)   ) cw->win_type = WIN_LOCK_SCREEN;
        else if (_e_mod_comp_is_indicator_window(cw->bd)     ) cw->win_type = WIN_INDICATOR;
        else if (_e_mod_comp_is_isf_main_window(cw->bd)      ) cw->win_type = WIN_ISF_MAIN;
        else if (_e_mod_comp_is_isf_sub_window(cw->bd)       ) cw->win_type = WIN_ISF_SUB;
        else if (_e_mod_comp_is_tooltip_window(cw->bd)       ) cw->win_type = WIN_TOOLTIP;
        else if (_e_mod_comp_is_combo_window(cw->bd)         ) cw->win_type = WIN_COMBO;
        else if (_e_mod_comp_is_dnd_window(cw->bd)           ) cw->win_type = WIN_DND;
        else if (_e_mod_comp_is_desktop_window(cw->bd)       ) cw->win_type = WIN_DESKTOP;
        else if (_e_mod_comp_is_toolbar_window(cw->bd)       ) cw->win_type = WIN_TOOLBAR;
        else if (_e_mod_comp_is_menu_window(cw->bd)          ) cw->win_type = WIN_MENU;
        else if (_e_mod_comp_is_splash_window(cw->bd)        ) cw->win_type = WIN_SPLASH;
        else if (_e_mod_comp_is_drop_down_menu_window(cw->bd)) cw->win_type = WIN_DROP_DOWN_MENU;
        else if (_e_mod_comp_is_notification_window(cw->bd)  ) cw->win_type = WIN_NOTIFICATION;
        else if (_e_mod_comp_is_utility_window(cw->bd)       ) cw->win_type = WIN_UTILITY;
        else if (_e_mod_comp_is_popup_menu_window(cw->bd)    ) cw->win_type = WIN_POPUP_MENU;
        else if (_e_mod_comp_is_dialog_window(cw->bd)        ) cw->win_type = WIN_DIALOG;
     }
   else
     {
        cw->pop = e_popup_find_by_window(cw->win);
        if (cw->pop)
          cw->dfn = e_object_delfn_add(E_OBJECT(cw->pop),
                                       _e_mod_comp_object_del, cw);
        else
          {
             cw->menu = e_menu_find_by_window(cw->win);
             if (cw->menu)
               cw->dfn = e_object_delfn_add(E_OBJECT(cw->menu),
                                            _e_mod_comp_object_del, cw);
             else
               {
                  char *netwm_title = NULL;
                  cw->title = ecore_x_icccm_title_get(cw->win);
                  if (ecore_x_netwm_name_get(cw->win, &netwm_title))
                    {
                       if (cw->title) free(cw->title);
                       cw->title = netwm_title;
                    }
                  ecore_x_icccm_name_class_get(cw->win, &cw->name, &cw->clas);
                  cw->role = ecore_x_icccm_window_role_get(cw->win);
                  if (!ecore_x_netwm_window_type_get(cw->win, &cw->primary_type))
                    cw->primary_type = ECORE_X_WINDOW_TYPE_UNKNOWN;
               }
          }
     }
   // fixme: could use bd/pop/menu for this too
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   if (!ecore_x_window_attributes_get(cw->win, &att))
     {
        free(cw);
        if (_comp_mod->conf->grab) ecore_x_ungrab();
        return NULL;
     }

   if ((!att.input_only) &&
       ((att.depth != 24) && (att.depth != 32)))
     {
        printf("WARNING: window 0x%x not 24/32bpp -> %ibpp\n",
               cw->win, att.depth);
        cw->invalid = 1;
     }
   cw->input_only = att.input_only;
   cw->override = att.override;
   cw->vis = att.visual;
   cw->depth = att.depth;
   cw->argb = ecore_x_window_argb_get(cw->win);

   eina_hash_add(windows, e_util_winid_str_get(cw->win), cw);
   cw->inhash = 1;
   if ((!cw->input_only) && (!cw->invalid))
     {
        Ecore_X_Rectangle *rects;
        int num;

        cw->damage = ecore_x_damage_new
          (cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
        cw->shobj = edje_object_add(c->evas);
        if (!cw->c->use_bg_img)
          {
             evas_object_stack_below(cw->c->bg_img, evas_object_bottom_get(cw->c->evas));
          }
        cw->obj = evas_object_image_filled_add(c->evas);
        evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);

        if (cw->argb) evas_object_image_alpha_set(cw->obj, 1);
        else evas_object_image_alpha_set(cw->obj, 0);

        _e_mod_comp_win_shadow_setup(cw);
        _e_mod_comp_win_cb_setup(cw);

        evas_object_show(cw->obj);
        ecore_x_window_shape_events_select(cw->win, 1);
        rects = ecore_x_window_shape_rectangles_get(cw->win, &num);
        if (rects)
          {
             int i;
             if (rects)
               {
                  int int_w, int_h;
                  for (i = 0; i < num; i++)
                    {
                       int_w = (int)rects[i].width;
                       int_h = (int)rects[i].height;
                       E_RECTS_CLIP_TO_RECT(rects[i].x, rects[i].y,
                                            int_w, int_h,
                                            0, 0, att.w, att.h);
                       rects[i].width = (unsigned int)int_w;
                       rects[i].height = (unsigned int)int_h;
                    }
               }
             if (!_e_mod_comp_shaped_check(att.w, att.h, rects, num))
               {
                  free(rects);
                  rects = NULL;
               }
             if (rects)
               {
                  cw->shape_changed = 1;
                  free(rects);
               }
          }
        if (cw->bd) evas_object_data_set(cw->shobj, "border", cw->bd);
        else if (cw->pop) evas_object_data_set(cw->shobj, "popup", cw->pop);
        else if (cw->menu) evas_object_data_set(cw->shobj, "menu", cw->menu);

        evas_object_pass_events_set(cw->obj, 1);

        cw->pending_count++;
        e_manager_comp_event_src_add_send
           (cw->c->man, (E_Manager_Comp_Source *)cw,
               _e_mod_comp_cb_pending_after, cw->c);
     }
   else
     {
        cw->shobj = evas_object_rectangle_add(c->evas);
        evas_object_color_set(cw->shobj, 0, 0, 0, 0);
     }
   evas_object_pass_events_set(cw->shobj, 1);
   evas_object_data_set(cw->shobj, "win",
                        (void *)((unsigned long)cw->win));
   evas_object_data_set(cw->shobj, "src", cw);

   c->wins_invalid = 1;
   c->wins = eina_inlist_append(c->wins, EINA_INLIST_GET(cw));
   cw->up = e_mod_comp_update_new();
   e_mod_comp_update_tile_size_set(cw->up, 32, 32);
   // for software:
   e_mod_comp_update_policy_set
     (cw->up, E_UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH);
   if (((!cw->input_only) && (!cw->invalid)) && (cw->override))
     {
        cw->redirected = 1;
        cw->dmg_updates = 0;
     }
   if (_comp_mod->conf->grab) ecore_x_ungrab();
   return cw;
}

static void
_e_mod_comp_win_del(E_Comp_Win *cw)
{
   int pending_count;
   Eina_List *l;
   Evas_Object *o;

   // while win_hide animation is progressing, at that time win_del is called,
   // background window effect is may not work fully.
   // so, explicit call disable effect stage function.
   if (cw->effect_stage)
     _e_mod_comp_disable_effect_stage(cw->c);

   if (cw->animating)
     {
        cw->c->animating--;
        cw->animating = 0;
        _e_mod_comp_send_window_effect_client_state(cw, EINA_FALSE);
     }

   if ((!cw->input_only) && (!cw->invalid))
     {
        cw->pending_count++;
        e_manager_comp_event_src_del_send
           (cw->c->man, (E_Manager_Comp_Source *)cw,
               _e_mod_comp_cb_pending_after, cw->c);
     }

   e_mod_comp_update_free(cw->up);
   _e_mod_comp_rotation_release(cw);

   // release window rotation stuffs
   _e_mod_comp_win_rotation_release(cw);

   if (cw->rects)
     {
        free(cw->rects);
        cw->rects = NULL;
     }
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (cw->dfn)
     {
        if (cw->bd)
          {
             eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
             e_object_delfn_del(E_OBJECT(cw->bd), cw->dfn);
             cw->bd = NULL;
          }
        else if (cw->pop)
          {
             e_object_delfn_del(E_OBJECT(cw->pop), cw->dfn);
             cw->pop = NULL;
          }
        else if (cw->menu)
          {
             e_object_delfn_del(E_OBJECT(cw->menu), cw->dfn);
             cw->menu = NULL;
          }
        cw->dfn = NULL;
     }
   if (cw->pixmap)
     {
        if (cw->native)
          {
             cw->native = 0;
             evas_object_image_native_surface_set(cw->obj, NULL);
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
               }
          }

        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
        _e_mod_comp_win_free_xim(cw);
     }
   if (cw->redirected)
     {
        cw->redirected = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   if (cw->update)
     {
        cw->update = 0;
        cw->c->updates = eina_list_remove(cw->c->updates, cw);
     }
   if (cw->obj_mirror)
     {
        Evas_Object *_o;
        EINA_LIST_FREE(cw->obj_mirror, _o)
          {
             if (!_o) continue;
             if (cw->xim) evas_object_image_data_set(_o, NULL);
             evas_object_del(_o);
          }
     }
   if (cw->xim)
     {
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
     }
   if (cw->obj)
     {
        evas_object_del(cw->obj);
        cw->obj = NULL;
     }
   if (cw->shobj)
     {
        evas_object_del(cw->shobj);
        cw->shobj = NULL;
     }
   if (cw->inhash)
     eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);
   if (cw->damage)
     {
        Ecore_X_Region parts;

        eina_hash_del(damages, e_util_winid_str_get(cw->damage), cw);
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }
   if (cw->title) free(cw->title);
   if (cw->name) free(cw->name);
   if (cw->clas) free(cw->clas);
   if (cw->role) free(cw->role);
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   pending_count = cw->pending_count;
   memset(cw, 0, sizeof(E_Comp_Win));
   cw->pending_count = pending_count;
   cw->delete_pending = 1;
   if (cw->pending_count > 0) return;
   free(cw);
}

static void
_e_mod_comp_win_show(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;

   // if win_hide was showed then immediatly win_show() function is called. case.
   if (cw->defer_hide == 1) cw->defer_hide = 0;
   if (cw->visible) return;
   cw->visible = 1;
   _e_mod_comp_win_configure(cw,
                             cw->hidden.x, cw->hidden.y,
                             cw->w, cw->h,
                             cw->border);
   if ((cw->input_only) || (cw->invalid)) return;

   if (cw->bd)
     _e_mod_comp_win_sync_setup(cw, cw->bd->client.win);
   else
     _e_mod_comp_win_sync_setup(cw, cw->win);

   if (cw->real_hid)
     {
        cw->real_hid = 0;
        if (cw->native)
          {
             evas_object_image_native_surface_set(cw->obj, NULL);
             cw->native = 0;
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
               }
          }
        if (cw->pixmap)
          {
             ecore_x_pixmap_free(cw->pixmap);
             cw->pixmap = 0;
             cw->pw = 0;
             cw->ph = 0;
             ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
          }
        if (cw->xim)
          {
             evas_object_image_size_set(cw->obj, 1, 1);
             evas_object_image_data_set(cw->obj, NULL);
             ecore_x_image_free(cw->xim);
             cw->xim = NULL;
             _e_mod_comp_win_free_xim(cw);
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_size_set(o, 1, 1);
                  evas_object_image_data_set(o, NULL);
               }
          }
        if (cw->redirected)
          {
             cw->redirected = 0;
             cw->pw = 0;
             cw->ph = 0;
          }
     }

   if ((!cw->redirected) || (!cw->pixmap))
     {
        if (!cw->pixmap) cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
        L(LT_EVENT_X,
          "[COMP]\t\tSHOW ecore_x_composite_name_window_pixmap_get() pixmap:0x%08x after redirect\n",
          cw->pixmap);
        if (cw->pixmap)
          ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
        else
          {
             cw->pw = 0;
             cw->ph = 0;
          }
        if ((cw->pw <= 0) || (cw->ph <= 0))
          {
             if (cw->pixmap)
               {
                  ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
                  cw->needpix = 1;
                  L(LT_EVENT_X,
                    "[COMP]\t\tSHOW free pixmap: 0x%08x invalid pixmap size.\n",
                    cw->pixmap);
               }
          }
        cw->redirected = 1;

        e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
        e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);

        evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_size_set(o, cw->pw, cw->ph);
          }

        L(LT_EVENT_X,
          "[COMP]\t\tSHOW redirect win pixmap:0x%08x %3dx%3d\n",
          cw->pixmap, cw->pw, cw->ph);

        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
     }

   if (cw->dmg_updates >= 1)
     {
        if ((cw->c->gl) && (_comp_mod->conf->texture_from_pixmap))
          {
             if (!cw->pixmap)
               cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);

             if (cw->pixmap)
               {
                  ecore_x_pixmap_geometry_get(cw->pixmap,
                                              NULL, NULL,
                                              &(cw->pw), &(cw->ph));
               }
             else
               {
                  cw->pw = 0;
                  cw->ph = 0;
               }

             if ((cw->pw <= 0) || (cw->ph <= 0))
               {
                  if (cw->pixmap)
                  ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
                  cw->needpix = 1;
                  L(LT_EVENT_X,
                    "[COMP]\t\tSHOW free pixmap: 0x%08x invalid pixmap size.\n",
                    cw->pixmap);
               }
             else
               {
                  ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
                  evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_size_set(o, cw->pw, cw->ph);
                    }
                  if (!cw->native)
                    {
                       Evas_Native_Surface ns;
                       ns.version = EVAS_NATIVE_SURFACE_VERSION;
                       ns.type = EVAS_NATIVE_SURFACE_X11;
                       ns.data.x11.visual = cw->vis;
                       ns.data.x11.pixmap = cw->pixmap;
                       evas_object_image_native_surface_set(cw->obj, &ns);

                       EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                         {
                            evas_object_image_native_surface_set(o, &ns);
                         }

                       evas_object_image_data_update_add(cw->obj, 0, 0, cw->pw, cw->ph);
                       cw->native = 1;
                    }
               }
            }

          if (cw->pixmap)
            {
                  cw->defer_hide = 0;
                  if (!cw->hidden_override)
                    {
                       if ( cw->defer_move_resize )
                         {
                            evas_object_move(cw->shobj, cw->x, cw->y );
                            evas_object_resize(cw->shobj, cw->pw + (cw->border * 2),
                                                          cw->ph + (cw->border * 2));
                            cw->defer_move_resize = EINA_FALSE;
                         }
                       evas_object_show(cw->shobj);
                    }
                  L(LT_EVENT_X,
                    "[COMP]\t\tSHOW evas_object_show()\n");
                  L(LT_EFFECT,
                    "[COMP] WIN_EFFECT : Show signal Emit -> win:0x%08x\n",
                    cw->win);
                  _e_mod_comp_window_show_effect(cw);
            }
     }
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_unredirect(E_Comp_Win *cw)
{
   if (!cw->visible) return;
   if ((cw->input_only) || (cw->invalid)) return;

   if (cw->obj)
     {
        evas_object_hide(cw->obj);
        evas_object_del(cw->obj);
        cw->obj = NULL;
     }
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (cw->native)
     {
        evas_object_image_native_surface_set(cw->obj, NULL);
        cw->native = 0;
     }
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   if (cw->xim)
     {
        evas_object_image_size_set(cw->obj, 1, 1);
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
     }
   if (cw->redirected)
     {
        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
     }
   if (cw->damage)
     {
        _e_mod_comp_win_del_damage(cw, cw->damage);
        ecore_x_damage_subtract(cw->damage, 0, 0);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }
}

static void
_e_mod_comp_win_real_hide(E_Comp_Win *cw)
{
   if (cw->bd)
     {
        _e_mod_comp_win_hide(cw);
        return;
     }
   cw->real_hid = 1;
   _e_mod_comp_win_hide(cw);
}

static void
_e_mod_comp_win_hide(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;

   if ((!cw->visible) && (!cw->defer_hide)) return;
   cw->visible = 0;
   if ((cw->input_only) || (cw->invalid)) return;

   /* release window rotation stuffs */
   _e_mod_comp_win_rotation_release(cw);

   if (cw->rotating)
     {
        if (!cw->force)
          {
             cw->defer_hide = 1;
             e_mod_comp_rotation_end_effect(cw->rotobj);

             _e_mod_comp_win_inc_animating(cw);

             cw->pending_count++;

             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x hide rotobj. bd:%s force:%d defer_hide:%d\n",
               "PIX_ROT", _e_mod_comp_win_get_client_xid(cw),
               cw->bd ? "O" : "X", cw->force, cw->defer_hide);
             return;
          }
        else
          {
             cw->animating = 0;
             cw->defer_hide = 0;
             _e_mod_comp_rotation_release(cw);
          }
     }

   if (cw->force == 0)
     {
        cw->defer_hide = 1;
        _e_mod_comp_window_hide_effect(cw);
        return;
     }
   cw->defer_hide = 0;
   cw->force = 0;
   evas_object_hide(cw->shobj);

   Ecore_X_Window _w = _e_mod_comp_win_get_client_xid(cw);
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (_comp_mod->conf->keep_unmapped)
     {
        if (_comp_mod->conf->send_flush) ecore_x_e_comp_flush_send(_w);
        if (_comp_mod->conf->send_dump) ecore_x_e_comp_dump_send(_w);
        return;
     }

   if (cw->native)
     {
        evas_object_image_native_surface_set(cw->obj, NULL);
        cw->native = 0;
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_native_surface_set(o, NULL);
          }
     }
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
     }
   if (cw->xim)
     {
        evas_object_image_size_set(cw->obj, 1, 1);
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_size_set(o, 1, 1);
             evas_object_image_data_set(o, NULL);
          }
     }
   if (cw->redirected)
     {
        cw->redirected = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   _e_mod_comp_win_render_queue(cw);
   if (_comp_mod->conf->send_flush) ecore_x_e_comp_flush_send(_w);
   if (_comp_mod->conf->send_dump) ecore_x_e_comp_dump_send(_w);
}

static void
_e_mod_comp_win_raise_above(E_Comp_Win *cw,
                            E_Comp_Win *cw2)
{
   Eina_Bool show_stack_before = EINA_FALSE;
   Eina_Bool show_stack_after = EINA_FALSE;
   Eina_Bool real_show_stack_effect = EINA_TRUE;

   E_Comp_Win *lower_background = NULL;

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Raise Above [0x%x] <- [0x%x]\n",
     _e_mod_comp_win_get_client_xid(cw),
     _e_mod_comp_win_get_client_xid(cw2));

   if (cw->c->switcher)
     {
        int i = 0;
        Evas_Coord x, y, w, h;
        E_Comp_Win *find_cw;
        EINA_INLIST_REVERSE_FOREACH(cw->c->wins, find_cw)
          {
             if (find_cw->visible
                 && evas_object_visible_get(find_cw->shobj)
                 && evas_object_visible_get(find_cw->obj)
                 && (find_cw->win_type != WIN_INDICATOR)
                 && (find_cw->win_type != WIN_TASK_MANAGER))
               {
                  evas_object_geometry_get( find_cw->shobj, &x, &y, &w, &h );
                  if ( w == find_cw->c->screen_w && h == find_cw->c->screen_h )
                    {
                       i++;
                       if (find_cw == cw)
                         {
                            cw->c->selected_pos = i;
                            break;
                         }
                    }
               }
          }
     }

   cw->c->wins_invalid = 1;

   lower_background = _e_mod_comp_win_find_background_win(cw);
   //real show effect check 1
   show_stack_before = _e_mod_comp_is_next_win_stack(cw, cw2);

   cw->c->wins = eina_inlist_remove(cw->c->wins,
                                    EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append_relative(cw->c->wins,
                                             EINA_INLIST_GET(cw),
                                             EINA_INLIST_GET(cw2));
   //real show effect check 2
   show_stack_after = _e_mod_comp_is_next_win_stack(cw, cw2);

   if (show_stack_before == show_stack_after)
     real_show_stack_effect = EINA_FALSE;

   // indicate slide up and down, save current position, size, color
   if ( ( _e_mod_comp_window_restack_policy(cw, cw2) == EINA_TRUE)
       && ( real_show_stack_effect == EINA_TRUE)
       && ( cw->first_show_worked == EINA_TRUE) )
     {
        _e_mod_comp_window_restack_effect(cw, cw2);
     }
   else if ( ( _e_mod_comp_window_restack_policy(cw, lower_background) == EINA_TRUE )
       && ( real_show_stack_effect == EINA_FALSE)
       && ( cw->show_done == EINA_TRUE)
       && ( _e_mod_comp_win_check_visible2(cw) == EINA_FALSE ) )
  //     && ( cw->ready_hide_effect == EINA_TRUE) )
     {
        _e_mod_comp_window_lower_effect(cw, lower_background);
     }
   else
     {
        evas_object_stack_above(cw->shobj, cw2->shobj);

        // for no-effect window
        if ( (cw->visible == EINA_TRUE)
             && (cw->first_show_worked == EINA_TRUE ) )
          {
             edje_object_signal_emit(cw->shobj,
                                     "e,state,visible,on,noeffect",
                                     "e");
             L(LT_EFFECT,
               "[COMP] WIN_EFFECT : Raise above(for no-effect window) signal Emit -> win:0x%08x\n",
               cw->win);
             _e_mod_comp_win_inc_animating(cw);
          }
     }

   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
      (cw->c->man, (E_Manager_Comp_Source *)cw,
       _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_raise(E_Comp_Win *cw)
{
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append(cw->c->wins, EINA_INLIST_GET(cw));

   evas_object_raise(cw->shobj);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_lower(E_Comp_Win *cw)
{
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_prepend(cw->c->wins, EINA_INLIST_GET(cw));

   evas_object_lower(cw->shobj);
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
      (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_configure(E_Comp_Win *cw,
                          int x, int y,
                          int w, int h,
                          int border)
{
   if (cw->rotating && cw->rotobj)
     {
        if (!((w == cw->w) && (h == cw->h)))
          {
             Ecore_X_Window win = _e_mod_comp_win_get_client_xid(cw);

             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x bd:%s resize.\n",
               "PIX_ROT", win, cw->bd ? "O" : "X");

             _e_mod_comp_win_recreate_shobj(cw);
             e_mod_comp_rotation_done_send(win, ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP);

             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x send ROTATION_RESIZE_PIXMAP.\n",
               "PIX_ROT", win);
          }
     }

   if (!((w == cw->w) && (h == cw->h)))
     {
        cw->w = w;
        cw->h = h;
        cw->needpix = 1;
        cw->dmg_updates = 0;
     }

   if (!cw->visible)
     {
        cw->hidden.x = x;
        cw->hidden.y = y;
        cw->border = border;
     }
   else
     {
        if (!((x == cw->x) && (y == cw->y)))
          {
             cw->x = x;
             cw->y = y;
             if (!cw->needpix)
               evas_object_move(cw->shobj, cw->x, cw->y);
          }
        cw->hidden.x = x;
        cw->hidden.y = y;
     }

   if (cw->border != border)
     {
        cw->border = border;
        evas_object_resize(cw->shobj,
                           cw->pw + (cw->border * 2),
                           cw->ph + (cw->border * 2));
     }
   cw->hidden.w = cw->w;
   cw->hidden.h = cw->h;
   if ((cw->input_only) || (cw->invalid) || (cw->needpix)) return;
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
      (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_damage(E_Comp_Win *cw,
                       int x, int y,
                       int w, int h,
                       Eina_Bool dmg)
{
   if ((cw->input_only) || (cw->invalid)) return;
   if ((dmg) && (cw->rotating))
     {
        e_mod_comp_rotation_damage(cw->rotobj);
        cw->dmg_updates++;
     }
   else if ((dmg) && (cw->damage))
     {
        Ecore_X_Region parts;
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        cw->dmg_updates++;
     }
   e_mod_comp_update_add(cw->up, x, y, w, h);
   if (dmg)
     {
        if (cw->counter)
          {
             if (!cw->update_timeout)
               cw->update_timeout = ecore_timer_add
                   (ecore_animator_frametime_get() * 600,
                    _e_mod_comp_win_damage_timeout, cw);
             return;
          }
     }

   /* to fix noise window appearing when non-EFL's window is showing,
    * add a check code that checks and skips first damage.
    * first damage is generated by the X server when copying image of the
    * root window. - gl77.lee 110208
    */
   if ((dmg)
       && !(cw->rotating)
       && (cw->dmg_updates <= 1))
     {
        if (!(cw->needpix))
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x bd:%s skip first damage.\n",
               "X_DAMAGE", _e_mod_comp_win_get_client_xid(cw),
               cw->bd ? "O" : "X");
             return;
          }
     }

   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_reshape(E_Comp_Win *cw)
{
   if (cw->shape_changed) return;
   cw->shape_changed = 1;
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   e_mod_comp_update_add(cw->up, 0, 0, cw->w, cw->h);
   _e_mod_comp_win_render_queue(cw);
}

static Eina_Bool
_e_mod_comp_create(void *data __UNUSED__,
                   int type __UNUSED__,
                   void *event)
{
   Ecore_X_Event_Window_Create *ev = event;
   E_Comp_Win *cw;
   E_Comp *c = _e_mod_comp_find(ev->parent);
   if (!c) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_find(ev->win)) return ECORE_CALLBACK_PASS_ON;
   if (c->win == ev->win) return ECORE_CALLBACK_PASS_ON;
   if (c->ee_win == ev->win) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "X_CREATE", ev->win);
   cw = _e_mod_comp_win_add(c, ev->win);
   if (cw)
     _e_mod_comp_win_configure(cw,
                               ev->x, ev->y,
                               ev->w, ev->h,
                               ev->border);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_destroy(void *data __UNUSED__,
                    int type __UNUSED__,
                    void *event)
{
   Ecore_X_Event_Window_Destroy *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_is_border(cw)) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p\n",
     "X_DESTROY", ev->win, _e_mod_comp_win_is_border(cw),
     _e_mod_comp_win_get_client_xid(cw), cw);
   if (cw->animating) cw->delete_me = 1;
   else _e_mod_comp_win_del(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_show(void *data __UNUSED__,
                 int type __UNUSED__,
                 void *event)
{
   Ecore_X_Event_Window_Show *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->visible) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_is_border(cw)) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p\n",
     "X_SHOW", ev->win, _e_mod_comp_win_is_border(cw),
     _e_mod_comp_win_get_client_xid(cw), cw);
   _e_mod_comp_win_show(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_hide(void *data __UNUSED__,
                 int type __UNUSED__,
                 void *event)
{
   Ecore_X_Event_Window_Hide *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (!cw->visible) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_is_border(cw)) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p\n",
     "X_HIDE", ev->win, _e_mod_comp_win_is_border(cw),
     _e_mod_comp_win_get_client_xid(cw), cw);
   _e_mod_comp_win_real_hide(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_reparent(void *data __UNUSED__,
                     int type __UNUSED__,
                     void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p TO rw:0x%08x\n",
     "X_REPARENT", ev->win, _e_mod_comp_win_is_border(cw),
     _e_mod_comp_win_get_client_xid(cw), cw, ev->parent);
   if (ev->parent != cw->c->man->root)
     {
        L(LT_EVENT_X, "[COMP] %31s w:0x%08x\n", "DEL", ev->win);
        _e_mod_comp_win_del(cw);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_configure(void *data __UNUSED__,
                      int type __UNUSED__,
                      void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p %4d %4d %3dx%3d\n",
     "X_CONFIGURE", ev->win, _e_mod_comp_win_is_border(cw),
     _e_mod_comp_win_get_client_xid(cw), cw, ev->x, ev->y, ev->w, ev->h);
   if (ev->abovewin == 0)
     {
        if (EINA_INLIST_GET(cw)->prev)
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x\n",
               "LOWER", ev->win);

             _e_mod_comp_win_lower(cw);
          }
     }
   else
     {
        E_Comp_Win *cw2 = _e_mod_comp_win_find(ev->abovewin);
        if (cw2)
          {
             E_Comp_Win *cw3 = (E_Comp_Win *)(EINA_INLIST_GET(cw)->prev);
             if (cw3 != cw2)
               {
                  L(LT_EVENT_X,
                    "[COMP] %31s above_w:0x%08x\n",
                    "RAISE_ABOVE", ev->abovewin);

                  _e_mod_comp_win_raise_above(cw, cw2);
               }
          }
     }

  if (!((cw->x == ev->x) && (cw->y == ev->y)) &&
      ((cw->w == ev->w) && (cw->h == ev->h)) &&
      _e_mod_comp_win_is_border(cw))
    {
       return ECORE_CALLBACK_PASS_ON;
    }

   if (!((cw->x == ev->x) && (cw->y == ev->y) &&
         (cw->w == ev->w) && (cw->h == ev->h) &&
         (cw->border == ev->border)))
     {
        _e_mod_comp_win_configure(cw,
                                  ev->x, ev->y,
                                  ev->w, ev->h,
                                  ev->border);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_stack(void *data __UNUSED__,
                  int type __UNUSED__,
                  void *event)
{
   Ecore_X_Event_Window_Stack *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p\n",
     "X_STACK", ev->win, _e_mod_comp_win_is_border(cw), _e_mod_comp_win_get_client_xid(cw), cw);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (ev->detail == ECORE_X_WINDOW_STACK_ABOVE) _e_mod_comp_win_raise(cw);
   else _e_mod_comp_win_lower(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_prop_effect_state(Ecore_X_Event_Window_Property *ev __UNUSED__)
{
   E_Comp *c;
   unsigned int val = 0;
   int ret = -1;

   c = _e_mod_comp_get();
   if (!c)
     {
        fprintf(stderr,
                "[E17-comp] _e_mod_comp_prop_effect_state failed.\n");
        return EINA_FALSE;
     }

   ret = ecore_x_window_prop_card32_get(c->man->root,
                                        ATOM_EFFECT_ENABLE,
                                        &val, 1);
   if (ret == -1) return EINA_FALSE;

   L(LT_EVENT_X,
     "[COMP] %31s propval:%d\n",
     "PROP_COMP_EFFECT_STATE", val);

   if (val != 0)
     {
        c->animatable = EINA_TRUE;
        _comp_mod->conf->default_window_effect = 1;
        e_config_domain_save("module.comp-slp",
                             _comp_mod->conf_edd,
                             _comp_mod->conf);
     }
   else
     {
        c->animatable = EINA_FALSE;
        _comp_mod->conf->default_window_effect = 0;
        e_config_domain_save("module.comp-slp",
                             _comp_mod->conf_edd,
                             _comp_mod->conf);
     }

   L(LT_EVENT_X,
     "[COMP] %31s c->animatable:%d\n",
     "ATOM_WINDOW_EFFECT_ENABLE",
     c->animatable);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_window_effect_state(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   unsigned int val = 0;
   int ret = -1;

   ret = ecore_x_window_prop_card32_get(ev->win,
                                        ATOM_WINDOW_EFFECT_ENABLE,
                                        &val,
                                        1);
   if (ret == -1) return EINA_FALSE;

   L(LT_EVENT_X,
     "[COMP] %31s propval:%d\n",
     "PROP_COMP_EFFECT_STATE", val);

   cw = _e_mod_comp_win_find(ev->win);
   if (cw) cw->animatable = val;
   else
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        if (!cw) return EINA_FALSE;
        cw->animatable = val;
     }

   L(LT_EVENT_X,
     "[COMP] %31s cw->animatable:%d\n",
     "ATOM_WINDOW_EFFECT_ENABLE",
     cw->animatable);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_prop_effect_style(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   Ecore_X_Atom *atoms = NULL;
   int num = 0;

   num = ecore_x_window_prop_atom_list_get(ev->win,
                                           ATOM_WINDOW_EFFECT_TYPE,
                                           &atoms);

   if (num != 6)
     {
        if (num > 0) free(atoms);
        return EINA_FALSE;
     }

   L(LT_EVENT_X,
     "[COMP] %31s \n",
     "PROP_COMP_EFFECT_STYLE");

   cw = _e_mod_comp_win_find(ev->win);
   if (cw)
     {
        cw->show_effect     = _e_mod_comp_get_effect_type(&atoms[0]);
        cw->hide_effect     = _e_mod_comp_get_effect_type(&atoms[1]);
        cw->restack_effect  = _e_mod_comp_get_effect_type(&atoms[2]);
        cw->rotation_effect = _e_mod_comp_get_effect_type(&atoms[3]);
        cw->focusin_effect  = _e_mod_comp_get_effect_type(&atoms[4]);
        cw->focusout_effect = _e_mod_comp_get_effect_type(&atoms[5]);
     }
   else
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        if (!cw)
          {
             if (num > 0) free(atoms);
             return EINA_FALSE;
          }
        cw->show_effect     = _e_mod_comp_get_effect_type(&atoms[0]);
        cw->hide_effect     = _e_mod_comp_get_effect_type(&atoms[1]);
        cw->restack_effect  = _e_mod_comp_get_effect_type(&atoms[2]);
        cw->rotation_effect = _e_mod_comp_get_effect_type(&atoms[3]);
        cw->focusin_effect  = _e_mod_comp_get_effect_type(&atoms[4]);
        cw->focusout_effect = _e_mod_comp_get_effect_type(&atoms[5]);
     }

   if (num > 0) free(atoms);

   return EINA_TRUE;
}


#if COMP_LOGGER_BUILD_ENABLE
static Eina_Bool
_e_mod_comp_prop_log(Ecore_X_Event_Window_Property *ev)
{
   int ret = -1;
   int count;
   unsigned char* prop_data = NULL;
   Comp_Logger comp_log_info;

   comp_log_info.logger_type = 0;
   memset(comp_log_info.logger_dump_file, '\0', 256 * sizeof(char) );

   ret = ecore_x_window_prop_property_get(ev->win,
                                          ATOM_CM_LOG,
                                          ECORE_X_ATOM_CARDINAL,
                                          32,
                                          &prop_data,
                                          &count);
   if (ret == -1)
     {
        if (prop_data) free(prop_data);
        return EINA_FALSE;
     }
   if ((ret > 0) && prop_data)
     {
        fprintf(stdout,
                "[E17-comp] %s(%d) comp_logger_type: ",
                __func__, __LINE__);

        if      (comp_logger_type == LT_NOTHING) fprintf(stdout, "LT_NOTHING ");
        else if (comp_logger_type == LT_ALL    ) fprintf(stdout, "LT_ALL "    );
        else                                     fprintf(stdout, "%d ", comp_logger_type);

        fprintf(stdout, "-> ");

        memcpy(&comp_log_info, prop_data, sizeof(Comp_Logger) );
        comp_logger_type = comp_log_info.logger_type;

        if      (comp_logger_type == LT_NOTHING ) fprintf(stdout, "LT_NOTHING\n");
        else if (comp_logger_type == LT_ALL     ) fprintf(stdout, "LT_ALL\n"    );
        else                                      fprintf(stdout, "%d\n", comp_logger_type);

        if (comp_logger_type == LT_CREATE)
          {
            _e_mod_comp_dump_cw_stack(EINA_FALSE, NULL);
            _e_mod_comp_fps_toggle();
          }
        if ( ( comp_logger_type == LT_DUMP )
             && ( strlen(comp_log_info.logger_dump_file) > 0 ) )
          {
            _e_mod_comp_dump_cw_stack(EINA_TRUE, comp_log_info.logger_dump_file);

            ecore_x_client_message32_send (ecore_x_window_root_first_get(),
                                           ATOM_CM_LOG_DUMP_DONE,
                                           ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                           0, 0, 0, 0, 0);
          }
        if(prop_data) free(prop_data);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
#endif /* COMP_LOGGER_BUILD_ENABLE */

static Eina_Bool
_e_mod_comp_prop_opacity(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   unsigned int val = 0;
   int ret = -1;

   cw = _e_mod_comp_win_find(ev->win);
   if (!cw)
     {
        cw = _e_mod_comp_border_client_find(ev->win);
        if (!cw) return EINA_FALSE;
     }

   ret = ecore_x_window_prop_card32_get(cw->win,
                                        ECORE_X_ATOM_NET_WM_WINDOW_OPACITY,
                                        &val, 1);
   if (ret == -1) return EINA_FALSE;

   cw->use_opacity = EINA_TRUE;
   cw->opacity = (val >> 24);
   evas_object_color_set(cw->shobj,
                         cw->opacity,
                         cw->opacity,
                         cw->opacity,
                         cw->opacity);
   return EINA_TRUE;

}

static Eina_Bool
_e_mod_comp_prop_sync_counter(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   cw = _e_mod_comp_win_find(ev->win);
   if (!cw) cw = _e_mod_comp_border_client_find(ev->win);

   Ecore_X_Sync_Counter counter = ecore_x_e_comp_sync_counter_get(ev->win);
   if (cw)
     {
        if (cw->counter != counter)
          {
             Ecore_X_Window _w = _e_mod_comp_win_get_client_xid(cw);
             if (cw->counter)
               {
                  ecore_x_e_comp_sync_cancel_send(_w);
                  ecore_x_sync_counter_inc(cw->counter, 1);
               }
             cw->counter = counter;
             if (cw->counter)
               {
                  ecore_x_sync_counter_inc(cw->counter, 1);
                  ecore_x_e_comp_sync_begin_send(_w);
               }
          }
     }
   else
     {
        if (counter) ecore_x_sync_counter_inc(counter, 1);
     }
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_comp_property(void *data __UNUSED__,
                     int type __UNUSED__,
                     void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;
   Ecore_X_Atom a = 0;
   if (!ev) return ECORE_CALLBACK_PASS_ON;
   if (!ev->atom) return ECORE_CALLBACK_PASS_ON;
   if (!_e_mod_comp_get_atom_name(ev->atom)) return ECORE_CALLBACK_PASS_ON;
   a = ev->atom;

   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x atom:%s\n",
     "X_PROPERTY", ev->win,
     _e_mod_comp_get_atom_name(a));

   if      (a == ECORE_X_ATOM_E_COMP_SYNC_COUNTER         ) _e_mod_comp_prop_sync_counter(ev);
   else if (a == ATOM_EFFECT_ENABLE                       ) _e_mod_comp_prop_effect_state(ev);
   else if (a == ATOM_WINDOW_EFFECT_ENABLE                ) _e_mod_comp_prop_window_effect_state(ev);
   else if (a == ATOM_WINDOW_EFFECT_TYPE                  ) _e_mod_comp_prop_effect_style(ev);
   else if (a == ECORE_X_ATOM_NET_WM_WINDOW_OPACITY       ) _e_mod_comp_prop_opacity(ev);
   else if (a == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) _e_mod_comp_prop_window_rotation(ev);
#if COMP_LOGGER_BUILD_ENABLE
   else if (a == ATOM_CM_LOG                              ) _e_mod_comp_prop_log(ev);
#endif /* COMP_LOGGER_BUILD_ENABLE */

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_msg_sync_draw_done(Ecore_X_Event_Client_Message *ev)
{
   E_Comp_Win *cw = NULL;
   int v = 0, w = 0, h = 0;
   cw = _e_mod_comp_border_client_find(ev->data.l[0]);
   v = ev->data.l[1];
   w = ev->data.l[2];
   h = ev->data.l[3];
   if (cw)
     {
        if (!cw->bd) return EINA_FALSE;
        if ((Ecore_X_Window)(ev->data.l[0]) != cw->bd->client.win) return EINA_FALSE;
     }
   else
     {
        cw = _e_mod_comp_win_find(ev->data.l[0]);
        if (!cw || (ev->data.l[0] != (int)cw->win))
          {
             Ecore_X_Sync_Counter counter = ecore_x_e_comp_sync_counter_get(ev->win);
             ecore_x_e_comp_sync_cancel_send(ev->win);
             if (counter) ecore_x_sync_counter_inc(counter, 1);
             L(LT_EVENT_X,
               "[COMP] ev:%15.15s w:0x%08x type:%s !cw v%d %03dx%03d\n",
               "X_CLIENT_MSG", ev->win, "SYNC_DRAW_DONE", v, w, h);
             return EINA_FALSE;
          }
     }
   if (!cw->counter)
     {
        cw->counter = ecore_x_e_comp_sync_counter_get(_e_mod_comp_win_get_client_xid(cw));
        if (cw->counter)
          {
             ecore_x_sync_counter_inc(cw->counter, 1);
             ecore_x_e_comp_sync_begin_send(_e_mod_comp_win_get_client_xid(cw));
          }
        L(LT_EVENT_X,
          "[COMP] ev:%15.15s w:0x%08x type:%s !cw->counter v%d %03dx%03d\n",
          "X_CLIENT_MSG", _e_mod_comp_win_get_client_xid(cw),
          "SYNC_DRAW_DONE", v, w, h);
        return EINA_FALSE;
     }

   if (!cw->update)
     {
        if (cw->update_timeout)
          {
             ecore_timer_del(cw->update_timeout);
             cw->update_timeout = NULL;
          }
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   cw->drawme = 1;

   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x type:%s v%d %03dx%03d\n",
     "X_CLIENT_MSG", _e_mod_comp_win_get_client_xid(cw),
     "SYNC_DRAW_DONE", v, w, h);

   _e_mod_comp_win_render_queue(cw);
   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_hib_leave(void)
{
   E_Comp *c = _e_mod_comp_get();
   Config *cfg = NULL;

   if (!c || !_comp_mod)
     {
        fprintf(stderr,
                "[E17-comp] _e_mod_comp_hib_leave failed.\n");
        return ECORE_CALLBACK_PASS_ON;
     }

   cfg = e_config_domain_load("module.comp-slp",
                              _comp_mod->conf_edd);
   if (!cfg)
     return ECORE_CALLBACK_PASS_ON;

   if (cfg->default_window_effect != c->animatable)
     {
        if (cfg->default_window_effect)
          {
             c->animatable = EINA_TRUE;
             _comp_mod->conf->default_window_effect = 1;
          }
        else
          {
             c->animatable = EINA_FALSE;
             _comp_mod->conf->default_window_effect = 0;
          }
        ecore_x_window_prop_card32_set
           (c->man->root, ATOM_EFFECT_ENABLE,
           (unsigned int *)(&(_comp_mod->conf->default_window_effect)), 1);
     }

   if (cfg->shadow_file)
     eina_stringshare_del(cfg->shadow_file);
   if (cfg->shadow_style)
     eina_stringshare_del(cfg->shadow_style);

   free(cfg);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_message(void *data __UNUSED__,
                    int type __UNUSED__,
                    void *event)
{
   Ecore_X_Event_Client_Message *ev = event;

   if (ev->format != 32)
     return ECORE_CALLBACK_PASS_ON;

   if (ev->message_type == ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE)
     {
        _e_mod_comp_msg_sync_draw_done(ev);
        return ECORE_CALLBACK_PASS_ON;
     }

   if (_e_mod_comp_rotation_handle_message(ev))
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   if (_comp_mod->conf->use_lock_screen)
     {
        if (ev->message_type == ATOM_CM_LOCK_SCREEN)
          {
             E_Comp *c = _e_mod_comp_find(ev->win);
             if (!c)
               return ECORE_CALLBACK_PASS_ON;
             if (ev->data.l[0] == 1)
               _e_mod_comp_screen_lock(c);
             else if (ev->data.l[0] == 0)
               _e_mod_comp_screen_unlock(c);
             return ECORE_CALLBACK_PASS_ON;
          }
     }

   if ((ev->message_type == ATOM_FAKE_LAUNCH)
       && (ev->data.l[0] == 1))
     {
        return _e_mod_comp_fake_show(ev);
     }

   if ((ev->message_type == ATOM_FAKE_LAUNCH)
       && (ev->data.l[0] == 0))
     {
        return _e_mod_comp_fake_hide(ev);
     }

   if ((ev->message_type == ATOM_X_HIBERNATION_STATE)
       && (ev->data.l[0] == 0))
     {
        return _e_mod_comp_hib_leave();
     }

   if (ev->message_type == ATOM_CAPTURE_EFFECT)
     {
        return _e_mod_comp_capture_effect(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_shape(void *data __UNUSED__,
                  int type __UNUSED__,
                  void *event)
{
   Ecore_X_Event_Window_Shape *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (ev->type != ECORE_X_SHAPE_BOUNDING) return ECORE_CALLBACK_PASS_ON;
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p\n",
     "X_SHAPE", ev->win, _e_mod_comp_win_is_border(cw), _e_mod_comp_win_get_client_xid(cw), cw);
   _e_mod_comp_win_reshape(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_damage(void *data __UNUSED__,
                   int type __UNUSED__,
                   void *event)
{
   Ecore_X_Event_Damage *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_damage_find(ev->damage);

   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p %4d %4d %3dx%3d\n",
     "X_DAMAGE", ev->drawable, _e_mod_comp_win_is_border(cw),
     _e_mod_comp_win_get_client_xid(cw), cw,
     ev->area.x, ev->area.y, ev->area.width, ev->area.height);

   if (!cw)
     {
        L(LT_EVENT_X,
          "[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p %4d %4d %3dx%3d ERR1\n",
          "X_DAMAGE", ev->drawable, _e_mod_comp_win_is_border(cw),
          _e_mod_comp_win_get_client_xid(cw), cw,
          ev->area.x, ev->area.y, ev->area.width, ev->area.height);

        cw = _e_mod_comp_border_client_find(ev->drawable);

        if (!cw)
          {
             L(LT_EVENT_X,"[COMP] ev:%15.15s w:0x%08x bd:%d c:0x%08x cw:%p %4d %4d %3dx%3d ERR2\n",
               "X_DAMAGE", ev->drawable, _e_mod_comp_win_is_border(cw),
               _e_mod_comp_win_get_client_xid(cw), cw,
               ev->area.x, ev->area.y, ev->area.width, ev->area.height);
             return ECORE_CALLBACK_PASS_ON;
          }
     }

   _e_mod_comp_win_damage(cw,
                          ev->area.x, ev->area.y,
                          ev->area.width, ev->area.height, 1);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_damage_win(void *data __UNUSED__,
                       int type __UNUSED__,
                       void *event)
{
   Ecore_X_Event_Window_Damage *ev = event;
   Eina_List *l;
   E_Comp *c;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        if (ev->win == c->ee_win)
          {
             // expose on comp win - init win or some other bypass win did it
             _e_mod_comp_render_queue(c);
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_randr(void *data __UNUSED__,
                  int type __UNUSED__,
                  __UNUSED__ void *event)
{
   Eina_List *l;
   E_Comp *c;

   L(LT_EVENT_X,
     "[COMP] ev:%15.15s\n",
     "E_CONTNR_RESIZE");

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c) continue;
        ecore_evas_resize(c->ee,
                          c->man->w,
                          c->man->h);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_add(void *data __UNUSED__,
                   int type __UNUSED__,
                   void *event)
{
   E_Event_Border_Add *ev = event;
   E_Comp_Win *cw;
   E_Comp* c = _e_mod_comp_find(ev->border->zone->container->manager->root);
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_ADD", ev->border->win);
   if (!c) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_find(ev->border->win)) return ECORE_CALLBACK_PASS_ON;
   if (c->win == ev->border->win) return ECORE_CALLBACK_PASS_ON;
   if (c->ee_win == ev->border->win) return ECORE_CALLBACK_PASS_ON;
   cw = _e_mod_comp_win_add(c, ev->border->win);
   if (cw)
     _e_mod_comp_win_configure(cw,
                               ev->border->x,
                               ev->border->y,
                               ev->border->w,
                               ev->border->h,
                               ev->border->client.initial_attributes.border);

   if ( ev->border->internal && ev->border->visible )
     _e_mod_comp_win_show(cw);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_del(void *data __UNUSED__,
                   int type __UNUSED__,
                   void *event)
{
   E_Event_Border_Remove *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_DEL", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->bd == ev->border) _e_mod_comp_object_del(cw, ev->border);
   if (cw->animating) cw->delete_me = 1;
   else _e_mod_comp_win_del(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_show(void *data __UNUSED__,
                    int type __UNUSED__,
                    void *event)
{
   E_Event_Border_Show *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_SHOW", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_show(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_hide(void *data __UNUSED__,
                    int type __UNUSED__,
                    void *event)
{
   E_Event_Border_Hide *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_HIDE", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (!cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_hide(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_move(void *data __UNUSED__,
                    int type __UNUSED__,
                    void *event)
{
   E_Event_Border_Move *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;

   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x   cw->x: %d, cw->y: %d, cw->visible: %d, \
     cw->hidden.x: %d, cw->hidden.y: %d, ev->x: %d,  ev->y: %d\n",
     "BD_MOVE", ev->border->win, cw->x, cw->y, cw->visible,
     cw->hidden.x, cw->hidden.y, ev->border->x, ev->border->y);

   if (!((cw->x == ev->border->x) && (cw->y == ev->border->y))
       && cw->visible)
     {
        _e_mod_comp_win_configure(cw,
                                  ev->border->x,
                                  ev->border->y,
                                  ev->border->w,
                                  ev->border->h,
                                  0);
     }
   else if (!((cw->hidden.x == ev->border->x) && (cw->hidden.y == ev->border->y))
            && !cw->visible)
     {
        _e_mod_comp_win_configure(cw,
                                  ev->border->x,
                                  ev->border->y,
                                  ev->border->w,
                                  ev->border->h,
                                  0);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_resize(void *data __UNUSED__,
                      int type __UNUSED__,
                      void *event)
{
   E_Event_Border_Resize *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_X,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_RESIZE", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if ((cw->w == ev->border->w) && (cw->h == ev->border->h))
     return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_configure(cw,
                             cw->x, cw->y,
                             ev->border->w, ev->border->h,
                             cw->border);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_iconify(void *data __UNUSED__,
                       int type __UNUSED__,
                       void *event)
{
   E_Event_Border_Iconify *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_BD,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_ICONIFY", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: special iconfiy anim
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_uniconify(void *data __UNUSED__,
                         int type __UNUSED__,
                         void *event)
{
   E_Event_Border_Uniconify *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_BD,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_UNICONIFY", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: special uniconfiy anim
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_urgent_change(void *data __UNUSED__,
                             int type __UNUSED__,
                             void *event)
{
   E_Event_Border_Urgent_Change *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_BD,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_URGENTCHANGE", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->bd->client.icccm.urgent)
     edje_object_signal_emit(cw->shobj, "e,state,urgent,on", "e");
   else
     edje_object_signal_emit(cw->shobj, "e,state,urgent,off", "e");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_focus_in(void *data __UNUSED__,
                        int type __UNUSED__,
                        void *event)
{
   E_Event_Border_Focus_In *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_BD,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_FOCUS_IN", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   edje_object_signal_emit(cw->shobj, "e,state,focus,on", "e");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_focus_out(void *data __UNUSED__,
                         int type __UNUSED__,
                         void *event)
{
   E_Event_Border_Focus_Out *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_BD,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_FOCUS_OUT", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   edje_object_signal_emit(cw->shobj, "e,state,focus,off", "e");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_property(void *data __UNUSED__,
                        int type __UNUSED__,
                        void *event)
{
   E_Event_Border_Property *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   L(LT_EVENT_BD,
     "[COMP] ev:%15.15s w:0x%08x\n",
     "BD_PROPERTY", ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: other properties?
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_key_down(void *data __UNUSED__,
                     int type __UNUSED__,
                     void *event)
{
   Ecore_Event_Key *ev = event;

   if ((!strcmp(ev->keyname, "Home")) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_ALT))
     {
        if (_comp_mod)
          {
             _e_mod_config_free(_comp_mod->module);
             _e_mod_config_new(_comp_mod->module);
             e_config_save();
             e_module_disable(_comp_mod->module);
             e_config_save();
             e_sys_action_do(E_SYS_RESTART, NULL);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Evas *
_e_mod_comp_evas_get_func(void *data,
                          E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   return c->evas;
}

static void
_e_mod_comp_update_func(void *data,
                        E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   _e_mod_comp_render_queue(c);
}

static const Eina_List *
_e_mod_comp_src_list_get_func(void *data,
                              E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   E_Comp_Win *cw;

   if (!c->wins) return NULL;
   if (c->wins_invalid)
     {
        c->wins_invalid = 0;
        if (c->wins_list) eina_list_free(c->wins_list);
        c->wins_list = NULL;
        EINA_INLIST_FOREACH(c->wins, cw)
          {
             if ((cw->shobj) && (cw->obj))
                c->wins_list = eina_list_append(c->wins_list, cw);
          }
     }
   return c->wins_list;
}

static Evas_Object *
_e_mod_comp_src_image_get_func(void *data __UNUSED__,
                               E_Manager *man __UNUSED__,
                               E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return NULL;
   return cw->obj;
}

static Evas_Object *
_e_mod_comp_src_shadow_get_func(void *data __UNUSED__,
                                E_Manager *man __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return NULL;
   return cw->shobj;
}

static Evas_Object *
_e_mod_comp_src_image_mirror_add_func(void *data __UNUSED__,
                                      E_Manager *man __UNUSED__,
                                      E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return NULL;
   return _e_mod_comp_win_mirror_add(cw);
}

static Eina_Bool
_e_mod_comp_src_visible_get_func(void *data __UNUSED__,
                                 E_Manager *man __UNUSED__,
                                 E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->visible;
}

static void
_e_mod_comp_src_hidden_set_func(void *data __UNUSED__,
                                E_Manager *man __UNUSED__,
                                E_Manager_Comp_Source *src,
                                Eina_Bool hidden)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return;
   if (cw->hidden_override == hidden) return;
   cw->hidden_override = hidden;
   if (cw->bd)
     e_border_comp_hidden_set(cw->bd,
                              cw->hidden_override);
   if (cw->visible)
     {
        if (cw->hidden_override)
          evas_object_hide(cw->shobj);
        else
          evas_object_show(cw->shobj);
     }
   else
     {
        if (cw->hidden_override)
          evas_object_hide(cw->shobj);
     }
}

static Eina_Bool
_e_mod_comp_src_hidden_get_func(void *data __UNUSED__,
                                E_Manager *man __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->hidden_override;
}

static void
_e_mod_comp_screen_lock_func(void *data,
                             E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   _e_mod_comp_screen_lock(c);
}

static void
_e_mod_comp_screen_unlock_func(void *data,
                               E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   _e_mod_comp_screen_unlock(c);
}

static int
_e_mod_comp_get_screen_angle(Ecore_X_Window root)
{
   int ret = -1, ang = 0;
   unsigned int val = 0;
   Ecore_X_Display* dpy = ecore_x_display_get();
   if (!dpy) return 0;
   if (!ATOM_X_SCREEN_ROTATION) return 0;

   ret = ecore_x_window_prop_card32_get(root, ATOM_X_SCREEN_ROTATION, &val, 1);
   if (ret == -1) return 0;

   switch (val)
     {
      case  1: ang =   0; break;
      case  2: ang =  90; break;
      case  4: ang = 180; break;
      case  8: ang = 270; break;
      default: ang =   0; break;
     }

   return ang;
}

static E_Comp *
_e_mod_comp_add(E_Manager *man)
{
   E_Comp *c;
   Ecore_X_Window *wins;
   Ecore_X_Window_Attributes att;
   int i, num;
   int ok = 0;

   c = calloc(1, sizeof(E_Comp));
   if (!c) return NULL;

   if (_comp_mod->conf->vsync)
     {
        e_util_env_set("__GL_SYNC_TO_VBLANK", "1");
     }
   else
     {
        e_util_env_set("__GL_SYNC_TO_VBLANK", NULL);
     }

   ecore_x_e_comp_sync_supported_set(man->root, _comp_mod->conf->efl_sync);

   c->man = man;
   c->win = ecore_x_composite_render_window_enable(man->root);
   if (!c->win)
     {
        e_util_dialog_internal
          (_("Compositor Error"),
           _("Your screen does not support the compositor<br>"
             "overlay window. This is needed for it to<br>"
             "function."));
        free(c);
        return NULL;
     }

   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(c->win, &att);

   if ((att.depth != 24) && (att.depth != 32))
     {
        e_util_dialog_internal
          (_("Compositor Error"),
           _("Your screen is not in 24/32bit display mode.<br>"
             "This is required to be your default depth<br>"
             "setting for the compositor to work properly."));
        ecore_x_composite_render_window_disable(c->win);
        free(c);
        return NULL;
     }

   if (c->man->num == 0) e_alert_composite_win = c->win;

   if (_comp_mod->conf->engine == E_EVAS_ENGINE_GL_X11)
     {
        int opt[20];
        int opt_i = 0;

        c->screen_w = man->w;
        c->screen_h = man->h;
        c->screen_ang = _e_mod_comp_get_screen_angle(c->man->root);

        if (0 != c->screen_ang)
          {
             c->screen_rotation = EINA_TRUE;
             if (0 != (c->screen_ang % 180))
               {
                  c->screen_w = man->h;
                  c->screen_h = man->w;
               }
          }

        if (_comp_mod->conf->indirect)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_INDIRECT;
             opt_i++;
             opt[opt_i] = 1;
             opt_i++;
          }
        if (_comp_mod->conf->vsync)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_VSYNC;
             opt_i++;
             opt[opt_i] = 1;
             opt_i++;
          }
        if (opt_i > 0)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_NONE;
             c->ee = ecore_evas_gl_x11_options_new
               (NULL, c->win, 0, 0, c->screen_w, c->screen_h, opt);
          }
        if (!c->ee)
          c->ee = ecore_evas_gl_x11_new(NULL, c->win, 0, 0, c->screen_w, c->screen_h);
        if (c->ee)
          {
             c->gl = 1;
             ecore_evas_gl_x11_pre_post_swap_callback_set
               (c->ee, c, _e_mod_comp_pre_swap, NULL);
          }
     }
   if (!c->ee)
     c->ee = ecore_evas_software_x11_new(NULL, c->win, 0, 0, man->w, man->h);

   ecore_evas_comp_sync_set(c->ee, 0);
   ecore_evas_manual_render_set(c->ee, _comp_mod->conf->lock_fps);
   c->evas = ecore_evas_get(c->ee);
   ecore_evas_show(c->ee);

   if (c->screen_rotation)
     ecore_evas_rotation_with_resize_set(c->ee, c->screen_ang);

   c->ee_win = ecore_evas_window_get(c->ee);
   ecore_x_screen_is_composited_set(c->man->num, c->ee_win);

   ecore_x_e_comp_dri_buff_flip_supported_set
     (c->man->root, _comp_mod->conf->dri_buff_flip);

   ecore_x_composite_redirect_subwindows
      (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);

   wins = ecore_x_window_children_get(c->man->root, &num);
   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             E_Comp_Win *cw;
             int x, y, w, h, border;
             char *wname = NULL, *wclass = NULL;

             ecore_x_icccm_name_class_get(wins[i], &wname, &wclass);
             if ((man->initwin == wins[i]) ||
                 ((wname) && (wclass) && (!strcmp(wname, "E")) &&
                  (!strcmp(wclass, "Init_Window"))))
               {
                  free(wname);
                  free(wclass);
                  ecore_x_window_reparent(wins[i], c->win, 0, 0);
                  ecore_x_sync();
                  continue;
               }
             if (wname) free(wname);
             if (wclass) free(wclass);
             wname = wclass = NULL;
             cw = _e_mod_comp_win_add(c, wins[i]);
             if (!cw) continue;
             ecore_x_window_geometry_get(cw->win, &x, &y, &w, &h);
             border = ecore_x_window_border_width_get(cw->win);
             if (wins[i] == c->win) continue;
             _e_mod_comp_win_configure(cw, x, y, w, h, border);
             if (ecore_x_window_visible_get(wins[i]))
               _e_mod_comp_win_show(cw);
          }
        free(wins);
     }

   ecore_x_window_key_grab(c->man->root,
                           "Home",
                           ECORE_EVENT_MODIFIER_SHIFT |
                           ECORE_EVENT_MODIFIER_CTRL |
                           ECORE_EVENT_MODIFIER_ALT, 0);


   c->bg_img = evas_object_rectangle_add(c->evas);
   evas_object_color_set(c->bg_img, 0, 0, 0, 255);

   evas_object_stack_below(c->bg_img, evas_object_bottom_get(c->evas));
   evas_object_show(c->bg_img);
   evas_object_move(c->bg_img, 0, 0);
   evas_object_resize(c->bg_img, c->man->w, c->man->h); // resize to root window's width, height

   c->fake_img_shobj = NULL;
   c->fake_img_shobj = edje_object_add(c->evas);
   ok = edje_object_file_set(c->fake_img_shobj, _comp_mod->conf->shadow_file, "fake_effect_twist");
   if (!ok)
     {
        fprintf(stdout,
                "[E17-comp] FAKE IMG EDC Animation isn't loaded!\n");
     }

   edje_object_signal_callback_add(c->fake_img_shobj,
                                   "fake,action,hide,done",
                                   "fake",
                                   _e_mod_comp_fake_hide_done, c);

   evas_object_move(c->fake_img_shobj, 0, 0);
   evas_object_resize(c->fake_img_shobj, c->man->w, c->man->h);
   c->fake_launch_state = EINA_FALSE;

   c->fake_launch_timeout  = NULL;
   c->fake_win = 0;
   c->fake_launch_done = EINA_FALSE;

   edje_object_signal_callback_add(c->fake_img_shobj,
                                   "fake,action,show,done",
                                   "fake",
                                   _e_mod_comp_fake_show_done, c);

   c->capture_effect_obj = NULL;
   c->capture_effect_obj = edje_object_add(c->evas);
   ok = edje_object_file_set(c->capture_effect_obj, _comp_mod->conf->shadow_file, "capture_effect");
   if (!ok)
     {
        fprintf(stdout,
                "[E17-comp] CAPTURE EFFECT EDC Animation isn't loaded!\n");
     }
   edje_object_signal_callback_add(c->capture_effect_obj,
                                   "img,capture,show,done",
                                   "img",
                                   _e_mod_comp_capture_effect_show_done, c);

   evas_object_move(c->capture_effect_obj, 0, 0);
   evas_object_resize(c->capture_effect_obj, c->man->w, c->man->h);

   c->switcher_animating = EINA_FALSE;

   c->comp.data                      = c;
   c->comp.func.evas_get             = _e_mod_comp_evas_get_func;
   c->comp.func.update               = _e_mod_comp_update_func;
   c->comp.func.src_list_get         = _e_mod_comp_src_list_get_func;
   c->comp.func.src_image_get        = _e_mod_comp_src_image_get_func;
   c->comp.func.src_shadow_get       = _e_mod_comp_src_shadow_get_func;
   c->comp.func.src_image_mirror_add = _e_mod_comp_src_image_mirror_add_func;
   c->comp.func.src_visible_get      = _e_mod_comp_src_visible_get_func;
   c->comp.func.src_hidden_set       = _e_mod_comp_src_hidden_set_func;
   c->comp.func.src_hidden_get       = _e_mod_comp_src_hidden_get_func;
   c->comp.func.screen_lock          = _e_mod_comp_screen_lock_func;
   c->comp.func.screen_unlock        = _e_mod_comp_screen_unlock_func;

   e_manager_comp_set(c->man, &(c->comp));

   return c;
}

static void
_e_mod_comp_del(E_Comp *c)
{
   E_Comp_Win *cw;

   e_manager_comp_set(c->man, NULL);

   ecore_x_window_key_ungrab(c->man->root,
                             "Home",
                             ECORE_EVENT_MODIFIER_SHIFT |
                             ECORE_EVENT_MODIFIER_CTRL |
                             ECORE_EVENT_MODIFIER_ALT, 0);
   if (c->grabbed)
     {
        c->grabbed = 0;
        ecore_x_ungrab();
     }
   if (c->fps_fg)
     {
        evas_object_del(c->fps_fg);
        c->fps_fg = NULL;
     }
   if (c->fps_bg)
     {
        evas_object_del(c->fps_bg);
        c->fps_bg = NULL;
     }
   ecore_x_screen_is_composited_set(c->man->num, 0);
   while (c->wins)
     {
        cw = (E_Comp_Win *)(c->wins);
        if (cw->counter)
          {
             ecore_x_sync_counter_free(cw->counter);
             cw->counter = 0;
          }
        cw->force = 1;
        _e_mod_comp_win_hide(cw);
        cw->force = 1;
        _e_mod_comp_win_del(cw);
     }
   ecore_evas_free(c->ee);
   ecore_x_composite_unredirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
   ecore_x_composite_render_window_disable(c->win);
   if (c->man->num == 0) e_alert_composite_win = 0;
   if (c->render_animator) ecore_animator_del(c->render_animator);
   if (c->new_up_timer) ecore_timer_del(c->new_up_timer);
   if (c->update_job) ecore_job_del(c->update_job);
   if (c->wins_list) eina_list_free(c->wins_list);

   ecore_x_e_comp_dri_buff_flip_supported_set(c->man->root, 0);

   ecore_x_e_comp_sync_supported_set(c->man->root, 0);
   free(c);
}

Eina_Bool
e_mod_comp_init(void)
{
   Eina_List *l;
   E_Manager *man;
   E_Comp    *comp = NULL;
   unsigned int effect_enable = 0;
   unsigned int window_effect_state = 0;

   windows = eina_hash_string_superfast_new(NULL);
   borders = eina_hash_string_superfast_new(NULL);
   damages = eina_hash_string_superfast_new(NULL);

   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,    _e_mod_comp_create,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,   _e_mod_comp_destroy,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW,      _e_mod_comp_show,             NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE,      _e_mod_comp_hide,             NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT,  _e_mod_comp_reparent,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, _e_mod_comp_configure,        NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK,     _e_mod_comp_stack,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,  _e_mod_comp_property,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,   _e_mod_comp_message,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,     _e_mod_comp_shape,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_DAMAGE_NOTIFY,    _e_mod_comp_damage,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DAMAGE,    _e_mod_comp_damage_win,       NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,           _e_mod_comp_key_down,         NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,       _e_mod_comp_randr,            NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ADD,             _e_mod_comp_bd_add,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE,          _e_mod_comp_bd_del,           NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_SHOW,            _e_mod_comp_bd_show,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_HIDE,            _e_mod_comp_bd_hide,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_MOVE,            _e_mod_comp_bd_move,          NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_RESIZE,          _e_mod_comp_bd_resize,        NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ICONIFY,         _e_mod_comp_bd_iconify,       NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY,       _e_mod_comp_bd_uniconify,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_URGENT_CHANGE,   _e_mod_comp_bd_urgent_change, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN,        _e_mod_comp_bd_focus_in,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT,       _e_mod_comp_bd_focus_out,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_PROPERTY,        _e_mod_comp_bd_property,      NULL));

   _e_mod_comp_atom_init();

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Comp *c;
        c = _e_mod_comp_add(man);
        if (c) compositors = eina_list_append(compositors, c);
        if (c) _e_mod_comp_set(c);
     }

   comp = _e_mod_comp_get();
   if (comp)
     {
        if (_comp_mod->conf->default_window_effect)
          effect_enable = 1;
        else
          effect_enable = 0;

        ecore_x_window_prop_card32_set(comp->man->root,
                                       ATOM_EFFECT_ENABLE,
                                       &effect_enable, 1);
        ecore_x_window_prop_card32_set(comp->man->root,
                                       ATOM_WINDOW_EFFECT_STATE,
                                       &window_effect_state, 1);
        ecore_x_window_prop_property_set(comp->man->root,
                                         ATOM_OVERAY_WINDOW,
                                         ECORE_X_ATOM_WINDOW,
                                         32, &comp->win, 1);
     }

   ecore_animator_frametime_set(1.0f/60.0f);
   ecore_x_sync();
   return 1;
}

void
e_mod_comp_shutdown(void)
{
   E_Comp *c;

   EINA_LIST_FREE(compositors, c) _e_mod_comp_del(c);

   E_FREE_LIST(handlers, ecore_event_handler_del);

   eina_hash_free(damages);
   eina_hash_free(windows);
   eina_hash_free(borders);

   _e_mod_comp_set(NULL);
}

void
e_mod_comp_shadow_set(void)
{
   Eina_List *l;
   E_Comp *c;

   EINA_LIST_FOREACH(compositors, l, c)
     {
        E_Comp_Win *cw;
        if (!c) continue;
        ecore_evas_manual_render_set(c->ee, _comp_mod->conf->lock_fps);
        EINA_INLIST_FOREACH(c->wins, cw)
          {
             if ((cw->shobj) && (cw->obj))
               {
                  _e_mod_comp_win_shadow_setup(cw);
                  if (cw->visible)
                    {
                       L(LT_EFFECT,
                         "[COMP] WIN_EFFECT : Show signal Emit (e_mod_comp_shadow_set()) -> win:0x%08x\n",
                         cw->win);

                       if (cw->animatable)
                         edje_object_signal_emit(cw->shobj,
                                                 "e,state,visible,on",
                                                 "e");
                       else
                         edje_object_signal_emit(cw->shobj,
                                                 "e,state,visible,on,noeffect",
                                                 "e");

                       _e_mod_comp_win_inc_animating(cw);
                       cw->pending_count++;
                       e_manager_comp_event_src_visibility_send
                         (cw->c->man, (E_Manager_Comp_Source *)cw,
                         _e_mod_comp_cb_pending_after, cw->c);
                    }
               }
          }
     }
}

E_Comp *
e_mod_comp_manager_get(E_Manager *man)
{
   return _e_mod_comp_find(man->root);
}

E_Comp_Win *
e_mod_comp_win_find_by_window(E_Comp *c,
                              Ecore_X_Window win)
{
   E_Comp_Win *cw;

   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (cw->win == win) return cw;
     }
   return NULL;
}

E_Comp_Win *
e_mod_comp_win_find_by_border(E_Comp *c,
                              E_Border *bd)
{
   E_Comp_Win *cw;

   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (cw->bd == bd) return cw;
     }
   return NULL;
}

E_Comp_Win *
e_mod_comp_win_find_by_popup(E_Comp *c,
                             E_Popup *pop)
{
   E_Comp_Win *cw;

   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (cw->pop == pop) return cw;
     }
   return NULL;
}

E_Comp_Win *
e_mod_comp_win_find_by_menu(E_Comp *c,
                            E_Menu *menu)
{
   E_Comp_Win *cw;

   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (cw->menu == menu) return cw;
     }
   return NULL;
}

Evas_Object *
e_mod_comp_win_evas_object_get(E_Comp_Win *cw)
{
   if (!cw) return NULL;
   return cw->obj;
}

Evas_Object *
e_mod_comp_win_mirror_object_add(Evas *e __UNUSED__,
                                 E_Comp_Win *cw __UNUSED__)
{
   return NULL;
}

static void
_e_mod_comp_win_cb_setup(E_Comp_Win *cw)
{
   edje_object_signal_callback_add(cw->shobj, "e,action,show,done",            "e", _e_mod_comp_show_done,            cw);
   edje_object_signal_callback_add(cw->shobj, "e,action,hide,done",            "e", _e_mod_comp_hide_done,            cw);
   edje_object_signal_callback_add(cw->shobj, "e,action,background,show,done", "e", _e_mod_comp_background_show_done, cw);
   edje_object_signal_callback_add(cw->shobj, "e,action,background,hide,done", "e", _e_mod_comp_background_hide_done, cw);
   edje_object_signal_callback_add(cw->shobj, "e,action,window,rotation,done", "e", _e_mod_comp_win_rotation_done,    cw);
   edje_object_signal_callback_add(cw->shobj, "e,action,raise_above_hide,done","e", _e_mod_comp_raise_above_hide_done,cw);
}

static void
_e_mod_comp_win_recreate_shobj(E_Comp_Win *cw)
{
   /* backup below obj */
   Eina_Bool bottom = EINA_FALSE;

   Evas_Object *below_obj = evas_object_below_get(cw->shobj);
   if (!below_obj)
     {
        if (evas_object_bottom_get(cw->c->evas) == cw->shobj)
          {
             L(LT_EVENT_X,
               "[COMP] %31s w:0x%08x bd:%s shobj is bottom.\n",
               "PIX_ROT", _e_mod_comp_win_get_client_xid(cw),
               cw->bd ? "O" : "X");
             bottom = EINA_TRUE;
          }
     }

   if (cw->obj)
     {
        evas_object_hide(cw->obj);
        evas_object_del(cw->obj);
        cw->obj = NULL;
     }
   if (cw->shobj)
     {
        evas_object_hide(cw->obj);
        evas_object_del(cw->shobj);
        cw->shobj = NULL;
     }

   cw->shobj = edje_object_add(cw->c->evas);
   cw->obj = evas_object_image_filled_add(cw->c->evas);
   evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);

   if (cw->argb)
     evas_object_image_alpha_set(cw->obj, 1);
   else
     evas_object_image_alpha_set(cw->obj, 0);

   _e_mod_comp_win_shadow_setup(cw);
   _e_mod_comp_win_cb_setup(cw);

   evas_object_show(cw->obj);
   evas_object_pass_events_set(cw->obj, 1);
   evas_object_pass_events_set(cw->shobj, 1);

   /* restore stack */
   if (bottom)
     below_obj = evas_object_below_get(cw->shobj);

   evas_object_stack_above(cw->shobj, below_obj);
   L(LT_EVENT_X,
     "[COMP] %31s w:0x%08x bd:%s shobj restore stack.\n",
     "PIX_ROT", _e_mod_comp_win_get_client_xid(cw),
     cw->bd ? "O" : "X");
}

void
_e_mod_comp_enable_touch_event_block(E_Comp *c)
{
   if (!c) return;
   if (!c->win) return;

   ecore_x_window_shape_input_rectangle_set(c->win, 0, 0, c->man->w, c->man->h );
}

void
_e_mod_comp_disable_touch_event_block(E_Comp *c)
{
   if (!c) return;
   if (!c->win) return;

   ecore_x_window_shape_input_rectangle_set(c->win, -1, -1, 1, 1 );
}

void
_e_mod_comp_win_inc_animating(E_Comp_Win *cw)
{
   if (!cw->animating)
     {
        cw->c->animating++;
        // send current effect status is starting.
        _e_mod_comp_send_window_effect_client_state(cw, EINA_TRUE);
        cw->animating = 1;
     }
   else
     {
        // if previous effect is not done case
        cw->c->animating--;
        cw->animating = 0;
        // send previous effect status is done
        _e_mod_comp_send_window_effect_client_state(cw, EINA_FALSE);
        cw->c->animating++;
        cw->animating = 1;
        // send current effect status is starting.
        _e_mod_comp_send_window_effect_client_state(cw, EINA_TRUE);
     }
   _e_mod_comp_win_render_queue(cw);
}

void
_e_mod_comp_win_dec_animating(E_Comp_Win *cw)
{
   if (cw->animating) cw->c->animating--;
   cw->animating = 0;

   _e_mod_comp_send_window_effect_client_state(cw, EINA_FALSE);
   _e_mod_comp_win_render_queue(cw);
}
