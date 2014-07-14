#include "e_mod_eom_server.h"
#include "e_mod_eom_config.h"
#include "e_mod_eom_privates.h"
#include "e_mod_eom_scrnconf.h"
#include "e_mod_eom_clone.h"

#include <Elementary.h>
#include <sys/ioctl.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/Xvproto.h>

#ifndef MAX
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

typedef struct
{
   int x;
   int y;
   int width;
   int height;
   char *profile;

   Evas_Object *win;
   Evas_Object *bg;
   Evas_Object *o;

   Ecore_X_Window xwin;
   XvPortID port;
   GC gc;

   Ecore_Event_Handler *visibility_handler;

   Eina_Bool shown;
} EomCloneInfo;

static EomCloneInfo *g_clone;

static Eina_Bool _eom_clone_grab_port (EomCloneInfo *clone);
static void _eom_clone_ungrab_port (EomCloneInfo *clone);
static Eina_Bool _eom_clone_show (EomCloneInfo *clone);
static void _eom_clone_hide (EomCloneInfo *clone);
static void _eom_clone_free (EomCloneInfo *clone);

static Eina_Bool
_eom_clone_visibility_change (void *data, int type, void *event)
{
   EomCloneInfo *clone = (EomCloneInfo*)data;
   Ecore_X_Event_Window_Visibility_Change *ev = (Ecore_X_Event_Window_Visibility_Change *)event;

   if (!clone || !ev)
      return ECORE_CALLBACK_PASS_ON;

   if (ev->win != clone->xwin)
      return ECORE_CALLBACK_PASS_ON;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] w:0x%08x(clone window:0x%08x) fully_obscured:%d\n",
        ev->win, clone->xwin, ev->fully_obscured);

   if (ev->fully_obscured)
     {
        _eom_clone_hide (clone);
        _eom_clone_ungrab_port (clone);
     }
   else
     {
        if (!_eom_clone_grab_port(clone))
          {
             SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail to grab port\n");
             return ECORE_CALLBACK_PASS_ON;
          }
     
        if (!_eom_clone_show(clone))
          {
             SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail to show clone\n");
             _eom_clone_ungrab_port (clone);
             return ECORE_CALLBACK_PASS_ON;
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_eom_clone_create_window (EomCloneInfo *clone)
{
   Display *dpy = ecore_x_display_get();
   char *profiles[1];
   Ecore_X_Atom win_type_atom;
   Ecore_X_Atom win_type_utility_atom;

   clone->win = elm_win_add(NULL, "clone window", ELM_WIN_BASIC);
   elm_win_title_set(clone->win, "clone window");
   elm_win_borderless_set(clone->win, EINA_TRUE);
   elm_win_alpha_set(clone->win, EINA_FALSE);

   profiles[0] = clone->profile;
   elm_win_profiles_set(clone->win, (const char**)profiles, 1);
   clone->xwin = elm_win_xwindow_get ((const Evas_Object*)clone->win);

   win_type_atom = ecore_x_atom_get("_NET_WM_WINDOW_TYPE"); 
   win_type_utility_atom = ecore_x_atom_get("_NET_WM_WINDOW_TYPE_UTILITY");
   ecore_x_window_prop_property_set (clone->xwin, win_type_atom, XA_ATOM, 32, (void*)&win_type_utility_atom, 1); 

   evas_object_move (clone->win, clone->x, clone->y);
   evas_object_resize (clone->win, clone->width, clone->height);
   evas_object_show (clone->win);

   clone->bg = elm_bg_add (clone->win);
   elm_win_resize_object_add (clone->win, clone->bg);
   evas_object_size_hint_weight_set (clone->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show (clone->bg);

   /* for transparent layer */
   clone->o = evas_object_rectangle_add (evas_object_evas_get(clone->win));
   evas_object_color_set (clone->o, 0, 0, 0, 0);
   evas_object_render_op_set (clone->o, EVAS_RENDER_COPY);

   elm_win_resize_object_add (clone->win, clone->o);
   evas_object_size_hint_weight_set (clone->o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show (clone->o);
   evas_object_move (clone->win, clone->x, clone->y);
   evas_object_resize (clone->win, clone->width, clone->height);

   clone->gc = XCreateGC (dpy, clone->xwin, 0, 0);

   clone->visibility_handler =
      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE,
                              _eom_clone_visibility_change, clone);

   return EINA_TRUE;
}

static Eina_Bool
_eom_clone_grab_port (EomCloneInfo *clone)
{
   Display *dpy = ecore_x_display_get();
   unsigned int   ver, rev, req_base, evt_base, err_base;
   unsigned int   adaptors;
   XvAdaptorInfo *ai = NULL;
   int            i;

   if (XvQueryExtension (dpy, &ver, &rev, &req_base, &evt_base, &err_base) != Success)
      return EINA_FALSE;

   if (XvQueryAdaptors (dpy, DefaultRootWindow (dpy), &adaptors, &ai) != Success)
      return EINA_FALSE;

   if (!ai)
      return EINA_FALSE;

   for (i = 0; i < adaptors; i++)
     {
        int min, max;
        XvPortID port;

        if (!(ai[i].type & XvVideoMask) || !(ai[i].type & XvInputMask))
           continue;

        min = ai[i].base_id;
        max = ai[i].base_id + ai[i].num_ports;

        for (port = min; port < max ; port++)
          {
             if (XvGrabPort (dpy, port, 0) == Success)
               {
                  SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] GrabPort: %ld\n", port);
                  clone->port = port;
                  XvFreeAdaptorInfo (ai);
                  return EINA_TRUE;
               }
          }
     }

   clone->port = 0;
   XvFreeAdaptorInfo (ai);
   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail: XvGrabPort\n");

   return EINA_FALSE;
}

static void
_eom_clone_ungrab_port (EomCloneInfo *clone)
{
   Display *dpy;

   if (clone->port == 0)
      return;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] UngrabPort: %ld\n", clone->port);

   dpy = ecore_x_display_get();
   XvUngrabPort(dpy, clone->port, 0);
   clone->port = 0;
}

static Eina_Bool
_eom_clone_show (EomCloneInfo *clone)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *l_output;
   int lvds_w = 0, lvds_h = 0;
   Display *dpy;

   /* get lvds information */
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, l_output, output_info)
     {
        if (output_info == NULL)
          continue;

        if (!strcmp(output_info->name, "LVDS1"))
          {
             lvds_w = output_info->crtc->current_mode->width;
             lvds_h = output_info->crtc->current_mode->height;
             break;
          }
     }

   if (lvds_w == 0 || lvds_h == 0)
      return EINA_FALSE;

   dpy = ecore_x_display_get();

   /* Put a single frame on framebuffer(underlay) */
   XvPutVideo(dpy, clone->port, clone->xwin, clone->gc,
              0, 0, lvds_w, lvds_h, 0, 0, clone->width, clone->height);

   XSync (dpy, False);

   clone->shown = EINA_TRUE;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] clone shown (win:0x08%x port:%ld)\n", clone->xwin, clone->port);

   return EINA_TRUE;
}

static void
_eom_clone_hide (EomCloneInfo *clone)
{
   Display *dpy;

   if (!clone->shown)
      return;
   if (!clone->port)
      return;
   if (clone->xwin == 0)
      return;

   dpy = ecore_x_display_get();
   XvStopVideo (dpy, clone->port, clone->xwin);

   clone->shown = 0;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] clone hidden\n");
}

static void
_eom_clone_free (EomCloneInfo *clone)
{
   Display *dpy;

   if (!clone)
      return;

   dpy = ecore_x_display_get();

   _eom_clone_hide (clone);
   _eom_clone_ungrab_port (clone);

   if (clone->visibility_handler)
      ecore_event_handler_del(clone->visibility_handler);
   if (clone->gc)
      XFreeGC(dpy, clone->gc);
   if (clone->o)
      evas_object_del(clone->o);
   if (clone->bg)
      evas_object_del(clone->bg);
   if (clone->win)
      evas_object_del(clone->win);

   XSync (dpy, False);

   free (clone);
}

Eina_Bool
eom_clone_start (int x, int y, int width, int height, char *profile)
{
   EomCloneInfo *clone = NULL;

   if (!profile || strlen(profile) == 0)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] no profile\n");
        return EINA_FALSE;
     }

   if (g_clone)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] clone mode already started\n");
        return EINA_TRUE;
     }

   clone = calloc (1, sizeof (EomCloneInfo));
   if (!clone)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail to allocate memory\n");
        goto start_fail;
     }

   clone->x = x;
   clone->y = y;
   clone->width = width;
   clone->height = height;
   clone->profile = strndup (profile, strlen (profile));

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] clone geometry(%d,%d %dx%d)\n",
        clone->x, clone->y, clone->width, clone->height);

   if (!_eom_clone_create_window(clone))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail to create window\n");
        goto start_fail;
     }

   if (!_eom_clone_grab_port(clone))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail to grab port\n");
        goto start_fail;
     }

   if (!_eom_clone_show(clone))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] fail to show clone\n");
        goto start_fail;
     }

   g_clone = clone;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] clone started\n");

   return EINA_TRUE;

start_fail:
   _eom_clone_free (clone);

   return EINA_FALSE;
}

void
eom_clone_stop (void)
{
   if (!g_clone)
      return;

   _eom_clone_free (g_clone);
   g_clone = NULL;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][cln] clone stopped\n");
}
