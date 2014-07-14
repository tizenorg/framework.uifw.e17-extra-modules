#include <e.h>
#include <e_randr.h>

#include "e_mod_main.h"
#include "e_mod_eom_scrnconf.h"
#include "e_mod_eom_server.h"
#include "e_mod_eom_clone.h"
#include "e_mod_eom_privates.h"

#ifdef  _MAKE_ATOM
# undef _MAKE_ATOM
#endif

#define _MAKE_ATOM(a, s)                          \
  do {                                            \
       a = ecore_x_atom_get(s);                   \
       if (!a)                                    \
         SLOG(LOG_DEBUG, "EOM",                   \
              "[e_eom][srn] ##s creation failed.\n");  \
    } while (0)

#define STR_XRR_DISPLAY_MODE_PROPERTY "XRR_PROPERTY_DISPLAY_MODE"

/* display mode */
typedef enum
{
   XRR_OUTPUT_DISPLAY_MODE_NULL, /* null */
   XRR_OUTPUT_DISPLAY_MODE_WB_CLONE, /* write-back */
} XRROutputPropDisplayMode;

/* screen conf dialog */
static E_Dialog *g_dia = NULL;

static struct
{
   Ecore_X_Randr_Crtc crtc;
   Ecore_X_Randr_Mode mode;
   Ecore_X_Randr_Output output;
} connected_info;

/* screen configuration info */
static struct
{
   SC_EXT_OUTPUT_ID     sc_output_id;
   eom_output_mode_e    sc_dispmode;
   SC_EXT_STATUS        sc_stat;
   SC_EXT_RES           sc_res;
} sc_ext =
{
   SC_EXT_OUTPUT_ID_NULL,
   EOM_OUTPUT_MODE_NONE,
   SC_EXT_STATUS_DISCONNECT,
   SC_EXT_RES_NULL,
};

/* mode info */
static struct
{
   int                      sc_res;
   double                   refresh;
   Ecore_X_Randr_Mode_Info *mode_info;
} sc_ext_mode [] =
{
   {SC_EXT_RES_1920X1080, 0.0, NULL},
   {SC_EXT_RES_1280X720, 0.0, NULL},
   {SC_EXT_RES_720X480, 0.0, NULL},
   {SC_EXT_RES_720X576, 0.0, NULL},
};

static char *str_output_id[3] = {
   "null",
   "HDMI1",
   "Virtual1",
};

static char *str_resolution[5] = {
   "null",
   "1920x1080",
   "1280x720",
   "720x480",
   "720x576",
};

static Ecore_Event_Handler *randr_crtc_handler;

/* Calculates the vertical refresh rate of a mode. */
static double
_cal_vrefresh(Ecore_X_Randr_Mode_Info *mode_info)
{
   double refresh = 0.0;
   double dots = mode_info->hTotal * mode_info->vTotal;
   if (!dots)
     return 0;
   refresh = (mode_info->dotClock + dots / 2) / dots;

   if (refresh > 0xffff)
     refresh = 0xffff;

   return refresh;
}

static char *
_get_str_output(int sc_output_id)
{
   char *str = NULL;

   switch (sc_output_id)
     {
      case SC_EXT_OUTPUT_ID_HDMI:
        str = str_output_id[1];
        break;

      case SC_EXT_OUTPUT_ID_VIRTUAL:
        str = str_output_id[2];
        break;

      default:
        str = str_output_id[0];
        break;
     }

   return str;
}

static char *
_get_str_resolution(int resolution)
{
   char *str = NULL;

   switch (resolution)
     {
      case SC_EXT_RES_1920X1080:
        str = str_resolution[1];
        break;

      case SC_EXT_RES_1280X720:
        str = str_resolution[2];
        break;

      case SC_EXT_RES_720X480:
        str = str_resolution[3];
        break;

      case SC_EXT_RES_720X576:
        str = str_resolution[4];
        break;

      default:
        str = str_resolution[0];
        break;
     }

   return str;
}

static int
_get_resolution_str(char *res_name)
{
   int resolution = SC_EXT_RES_NULL;

   if (!strcmp(res_name, str_resolution[1]))
     resolution = SC_EXT_RES_1920X1080;
   else if (!strcmp(res_name, str_resolution[2]))
     resolution = SC_EXT_RES_1280X720;
   else if (!strcmp(res_name, str_resolution[3]))
     resolution = SC_EXT_RES_720X480;
   else if (!strcmp(res_name, str_resolution[4]))
     resolution = SC_EXT_RES_720X576;
   else
     resolution = SC_EXT_RES_NULL;

   return resolution;
}

#if SCRN_CONF_DEBUG
static void
_debug_possible_crtc(E_Randr_Output_Info *output_info)
{
   E_Randr_Crtc_Info *crtc_info = NULL;
   E_Randr_Output_Info *t_o = NULL;
   Ecore_X_Randr_Mode_Info *t_m = NULL;
   Eina_List *t_l, *l_crtc;

   EINA_LIST_FOREACH(output_info->possible_crtcs, l_crtc, crtc_info)
     {
        if (crtc_info == NULL)
          continue;

        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] possible crtc = %d\n", crtc_info->xid);

        EINA_LIST_FOREACH(crtc_info->outputs, t_l, t_o)
          {
             SLOG(LOG_DEBUG, "EOM", "    output : %s\n", t_o->name);
          }

        EINA_LIST_FOREACH(crtc_info->possible_outputs, t_l, t_o)
          {
             SLOG(LOG_DEBUG, "EOM", "    possible output : %s\n", t_o->name);
          }

        EINA_LIST_FOREACH(crtc_info->outputs_common_modes, t_l, t_m)
          {
             if (!t_m->name)
               break;
             SLOG(LOG_DEBUG, "EOM", "    outputs common modes : %s\n", t_m->name);
          }

        if (!crtc_info->current_mode)
          {
             SLOG(LOG_DEBUG, "EOM", "    no current mode \n");
             crtc_xid = crtc_info->xid;
             SLOG(LOG_DEBUG, "EOM", "    crtc_id : %d\n", crtc_info->xid);
          }
        else
          {
             SLOG(LOG_DEBUG, "EOM", "    current set mode : %s\n", crtc_info->current_mode->name);
             SLOG(LOG_DEBUG, "EOM", "    crtc_id : %d\n", crtc_info->xid);
          }
     }
}

#endif

static E_Randr_Output_Info *
_eom_scrnconf_get_output_info_from_output_xid(Ecore_X_Randr_Output output_xid)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info == NULL)
          continue;

        if (output_info->xid == output_xid)
          return output_info;
     }

   return NULL;
}

static E_Randr_Output_Info *
_eom_scrnconf_get_output_info_from_output(int sc_output_id)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info == NULL)
          continue;

        if (!strcmp(output_info->name, _get_str_output(sc_output_id)))
          return output_info;
     }

   return NULL;
}

static int
_eom_scrnconf_get_output(E_Randr_Output_Info *output_info)
{
   int sc_output_id = SC_EXT_OUTPUT_ID_NULL;

   if (!strcmp(output_info->name, "HDMI1"))
     sc_output_id = SC_EXT_OUTPUT_ID_HDMI;
   else if (!strcmp(output_info->name, "Virtual1"))
     sc_output_id = SC_EXT_OUTPUT_ID_VIRTUAL;
   else
     sc_output_id = SC_EXT_OUTPUT_ID_NULL;

   return sc_output_id;
}

static int
_eom_scrnconf_set_modes(int sc_output_id, int num_res, int *resolutions)
{
   E_Randr_Output_Info *output_info = NULL;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   Eina_List *l_mode;
   int res = SC_EXT_RES_NULL;
   double refresh = 0.0;
   int i;

   output_info = _eom_scrnconf_get_output_info_from_output(sc_output_id);
   if (!output_info)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to get output_info from sc_output_id\n");
        return 0;
     }

   EINA_LIST_FOREACH(output_info->monitor->modes, l_mode, mode_info)
     {
        if (mode_info == NULL)
          continue;

#if 0 //debug
        SLOG(LOG_DEBUG, "EOM", "%s(%d): mode_info->name, %s, vrefresh, %f\n"
             , __func__, __LINE__, mode_info->name, _cal_vrefresh(mode_info));
#endif
        res = _get_resolution_str(mode_info->name);
        if (res == SC_EXT_RES_NULL)
          continue;

        for (i = 0; i < num_res; i++)
          {
             if (sc_ext_mode[i].sc_res == res)
               {
                  refresh = _cal_vrefresh(mode_info);
                  if (refresh > sc_ext_mode[i].refresh)
                    {
                       sc_ext_mode[i].refresh = refresh;
                       sc_ext_mode[i].mode_info = mode_info;
                    }
               }
          }
     }

   for (i = 0; i < num_res; i++)
     {
        resolutions[i] = sc_ext_mode[i].sc_res;
     }

#if 0 // debug
   for (i = 0; i < num_res; i++)
     {
        SLOG(LOG_DEBUG, "EOM", "[EOM]: res info:: (%d): %s, %f, %p\n"
             , i, _get_str_resolution(sc_ext_mode[i].sc_res), sc_ext_mode[i].refresh, sc_ext_mode[i].mode_info);
     }
#endif

   return 1;
}

static int
_eom_scrnconf_get_setting_info(int sc_output_id, int resolution, Ecore_X_Randr_Output *output_xid,
                               Ecore_X_Randr_Crtc *crtc_xid, Ecore_X_Randr_Mode *mode_xid, int *output_w, int *output_h)
{
   E_Randr_Output_Info *output_info = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   Eina_List *l_crtc;
   Eina_Bool found_output = EINA_FALSE;
   int num_res = 0;
   int i;

   output_info = _eom_scrnconf_get_output_info_from_output(sc_output_id);
   if (output_info == NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to get output_info from sc_output_id\n");
        goto finish;
     }

   output_xid[0] = output_info->xid;

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   /* find mode info */
   for (i = 0; i < num_res; i++)
     {
        if (sc_ext_mode[i].sc_res == resolution)
          {
             mode_info = sc_ext_mode[i].mode_info;
             break;
          }
     }

   if (mode_info == NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to get mode_info from sc_output_id\n");
        goto finish;
     }

   *output_w = mode_info->width;
   *output_h = mode_info->height;
   *mode_xid = mode_info->xid;
   found_output = EINA_TRUE;

#if SCRN_CONF_DEBUG
   _debug_possible_crtc(output_info);
#endif

   EINA_LIST_FOREACH(output_info->possible_crtcs, l_crtc, crtc_info)
     {
        if (crtc_info == NULL)
          continue;

        if (!crtc_info->current_mode)
          {
             *crtc_xid = crtc_info->xid;
          }
     }

   if (!*crtc_xid)
     *crtc_xid = output_info->crtc->xid;

finish:

   return found_output;
}

static Eina_Bool
_eom_scrnconf_crtc_change_cb (void *data, int type, void *ev)
{
   Ecore_X_Event_Randr_Crtc_Change *e = (Ecore_X_Event_Randr_Crtc_Change *)ev;

   if (type != ECORE_X_EVENT_RANDR_CRTC_CHANGE)
      return EINA_TRUE;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] crtc changed(win:%d crtc:%d mode:%d orientation:%d geo(%d,%d %dx%d)\n",
        e->win, e->crtc, e->mode, e->orientation, e->geo.x, e->geo.y, e->geo.w, e->geo.h);

   if (sc_ext.sc_dispmode != EOM_OUTPUT_MODE_CLONE)
      return EINA_TRUE;

   if (!e || e->crtc != connected_info.crtc)
      return EINA_TRUE;

   if (!eom_clone_start (e->geo.x, e->geo.y, e->geo.w, e->geo.h, "desktop"))
      goto set_fail;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] %s success to start the clone mode\n", _get_str_output(sc_ext.sc_output_id));

   return EINA_TRUE;

set_fail:
   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] %s fail to start the clone mode\n", _get_str_output(sc_ext.sc_output_id));

   return EINA_TRUE;
}

static void
_eom_scrnconf_prepare_dispmode_change (void)
{
    ecore_x_grab();
}

static void
_eom_scrnconf_finish_dispmode_change (void)
{
    ecore_x_ungrab();
}

static int
_eom_scrnconf_set_desktop(int sc_output_id, int resolution)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *l_output;
   int lvds_x = 0, lvds_y = 0, lvds_w = 0, lvds_h = 0;
   int output_x = 0, output_y = 0, output_w = 0, output_h = 0;
   int resize_w = 0, resize_h = 0;
   Ecore_X_Randr_Crtc crtc_xid = 0;
   Ecore_X_Randr_Output output_xid[1] = {0};
   Ecore_X_Randr_Mode mode_xid = 0;

   /* get lvds information */
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, l_output, output_info)
     {
        if (output_info == NULL)
          continue;

        if (!strcmp(output_info->name, "LVDS1"))
          {
             lvds_x = output_info->crtc->geometry.x;
             lvds_y = output_info->crtc->geometry.y;
             lvds_w = output_info->crtc->current_mode->width;
             lvds_h = output_info->crtc->current_mode->height;
             break;
          }
     }

   if (!_eom_scrnconf_get_setting_info(sc_output_id, resolution, output_xid, &crtc_xid, &mode_xid, &output_w, &output_h))
     goto set_fail;

   /* set the output is right-of lvds output */
   output_x = lvds_w;
   output_y = 0;
   resize_w = lvds_w + output_w;

   if (lvds_h > output_h)
     resize_h = lvds_h;
   else
     resize_h = output_h;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] set screen resize (%d,%d)\n", resize_w, resize_h);
   if (!ecore_x_randr_screen_current_size_set(e_randr_screen_info.root, resize_w, resize_h, 0, 0))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to resize the screen\n");
        goto set_fail;
     }

   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] set crtc_id, %d output_id, %d mode_id, %d)!\n", crtc_xid, output_xid[0], mode_xid);
   if (!ecore_x_randr_crtc_settings_set(e_randr_screen_info.root, crtc_xid, output_xid, 1, output_x,
                                        output_y, mode_xid, ECORE_X_RANDR_ORIENTATION_ROT_0))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set x=%d, y=%d, w=%d, h=%d mode_id=%d, crtc_id=%d\n",
             output_x, output_y, output_w, output_h, mode_xid, crtc_xid);
        goto set_fail;
     }

   connected_info.crtc = crtc_xid;
   connected_info.mode = mode_xid;
   connected_info.output = output_xid[0];

   eom_scrnconf_container_bg_canvas_visible_set(EINA_TRUE);

   return 1;

set_fail:
   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] %s fail to set the desktop mode\n", _get_str_output(sc_output_id));

   return 0;
}

static int
_eom_scrnconf_set_clone(int sc_output_id, int sc_res)
{
   if (!_eom_scrnconf_set_desktop(sc_output_id, sc_res))
      goto set_fail;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] %s success to set the clone mode\n", _get_str_output(sc_output_id));
   return 1;

set_fail:
   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] %s fail to set the clone mode\n", _get_str_output(sc_output_id));
   return 0;
}

static void
_eom_scrnconf_dialog_desktop_cb(void *data, E_Dialog *g_dia)
{
   E_Randr_Output_Info *output_info = (E_Randr_Output_Info *)data;
   int sc_output_id = SC_EXT_OUTPUT_ID_NULL;
   int sc_res = SC_EXT_RES_NULL;
   int num_res = 0;
   int *resolutions = NULL;

   sc_output_id = _eom_scrnconf_get_output(output_info);
   if (sc_output_id == SC_EXT_OUTPUT_ID_NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to find sc_output_id from output_info\n");
        goto set_fail;
     }

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   resolutions = calloc(num_res, sizeof (int));
   if (!resolutions)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to allocate resolutions\n");
        goto set_fail;
     }

   if (!_eom_scrnconf_set_modes(sc_output_id, num_res, resolutions))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set modes\n");
        goto set_fail;
     }

   sc_res = resolutions[0];

   _eom_scrnconf_prepare_dispmode_change();

   if (!_eom_scrnconf_set_desktop(sc_output_id, sc_res))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set modes\n");
        _eom_scrnconf_finish_dispmode_change();
        goto set_fail;
     }

   _eom_scrnconf_finish_dispmode_change();

   /* set display mode */
   sc_ext.sc_dispmode = EOM_OUTPUT_MODE_DESKTOP;

   eom_server_send_notify (EOM_NOTIFY_MODE_CHANGED, sc_output_id);

   if (resolutions)
     free(resolutions);

   /* destroy dialog */
   eom_scrnconf_dialog_free();

   return;

set_fail:
   if (resolutions)
     free(resolutions);

   /* destroy dialog */
   eom_scrnconf_dialog_free();
}

static void
_eom_scrnconf_dialog_clone_cb(void *data, E_Dialog *g_dia)
{
   E_Randr_Output_Info *output_info = (E_Randr_Output_Info *)data;
   int sc_output_id = SC_EXT_OUTPUT_ID_NULL;
   int sc_res = SC_EXT_RES_NULL;
   int num_res = 0;
   int *resolutions = NULL;

   sc_output_id = _eom_scrnconf_get_output(output_info);
   if (sc_output_id == SC_EXT_OUTPUT_ID_NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to find sc_output_id from output_info\n");
        goto set_fail;
     }

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   resolutions = calloc(num_res, sizeof (int));
   if (!resolutions)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to allocate resolutions\n");
        goto set_fail;
     }

   if (!_eom_scrnconf_set_modes(sc_output_id, num_res, resolutions))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set modes\n");
        goto set_fail;
     }

   sc_res = resolutions[0];

   _eom_scrnconf_prepare_dispmode_change();

   if (!_eom_scrnconf_set_clone(sc_output_id, sc_res))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set clone\n");
        _eom_scrnconf_finish_dispmode_change();
        goto set_fail;
     }

   _eom_scrnconf_finish_dispmode_change();

   /* set display mode */
   sc_ext.sc_dispmode = EOM_OUTPUT_MODE_CLONE;

   eom_server_send_notify (EOM_NOTIFY_MODE_CHANGED, sc_output_id);

   if (resolutions)
     free(resolutions);

   /* destroy dialog */
   eom_scrnconf_dialog_free();

   return;

set_fail:
   if (resolutions)
     free(resolutions);

   /* destroy dialog */
   eom_scrnconf_dialog_free();
}

static void
_eom_scrnconf_dialog_cancle_cb(void *data, E_Dialog *g_dia)
{
   /* destroy dialog */
   eom_scrnconf_dialog_free();
}

/*
   void
   eom_scrnconf_init ()
   {
   scrnconf_ext_update_get_perperty("null", "null", "null", "null");
   }

   void
   eom_scrnconf_deinit ()
   {}
 */

void
eom_scrnconf_dialog_new(int sc_output_id)
{
   E_Manager *man;
   E_Container *con;
   E_Randr_Output_Info *output_info = NULL;

   if (sc_output_id == SC_EXT_OUTPUT_ID_NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] Unknown output is not supported \n");
        return;
     }

   output_info = _eom_scrnconf_get_output_info_from_output(sc_output_id);
   if (!output_info)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to get output_info from sc_output_id\n");
        return;
     }

   /* set to be null, if dialog already exists */
   if (g_dia != NULL)
     eom_scrnconf_dialog_free();

   man = e_manager_current_get();
   con = e_container_current_get(man);

   if (sc_output_id == SC_EXT_OUTPUT_ID_HDMI)
     {
        g_dia = e_dialog_new(con, "E", "hdmi mode");
        e_dialog_title_set(g_dia, "\nHDMI Connected !!\n");
        e_dialog_text_set(g_dia, "<hilight>HDMI Connected<hilight><br>Select the screen configuration mode<br>");
     }
   else if (sc_output_id == SC_EXT_OUTPUT_ID_VIRTUAL)
     {
        g_dia = e_dialog_new(con, "E", "virtual mode");
        e_dialog_title_set(g_dia, "\nVirtual Connected !!\n");
        e_dialog_text_set(g_dia, "<hilight>Virtual Connected<hilight><br>Select the screen configuration mode<br>");
     }

   e_dialog_button_add(g_dia, "CLONE", NULL, _eom_scrnconf_dialog_clone_cb, (void *)output_info);
   e_dialog_button_add(g_dia, "DESKTOP", NULL, _eom_scrnconf_dialog_desktop_cb, (void *)output_info);
   e_dialog_button_add(g_dia, "CANCLE", NULL, _eom_scrnconf_dialog_cancle_cb, (void *)output_info);

   e_dialog_resizable_set(g_dia, 1);
   e_win_centered_set(g_dia->win, 1);
   e_dialog_show(g_dia);

   ecore_x_sync();
}

void
eom_scrnconf_init()
{
   /* init scrn conf get property */
   scrnconf_ext_update_get_perperty(ecore_x_display_get(), "null", "null", "null", "null");

   if (randr_crtc_handler)
      ecore_event_handler_del (randr_crtc_handler);

   randr_crtc_handler = ecore_event_handler_add(ECORE_X_EVENT_RANDR_CRTC_CHANGE,
                                                _eom_scrnconf_crtc_change_cb, NULL);
}

void
eom_scrnconf_deinit()
{
   if (randr_crtc_handler)
     {
        ecore_event_handler_del (randr_crtc_handler);
        randr_crtc_handler = NULL;
     }
}

void
eom_scrnconf_dialog_free()
{
   if (g_dia != NULL)
     {
        e_util_defer_object_del(E_OBJECT(g_dia));
        g_dia = NULL;
     }

   ecore_x_sync();
}

int
eom_scrnconf_get_output_id_from_xid(Ecore_X_Randr_Output output_xid)
{
   E_Randr_Output_Info *output_info = NULL;
   int sc_output_id = SC_EXT_OUTPUT_ID_NULL;

   output_info = _eom_scrnconf_get_output_info_from_output_xid(output_xid);
   if (!output_info)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to get output from xid\n");
        return SC_EXT_OUTPUT_ID_NULL;
     }

   sc_output_id = _eom_scrnconf_get_output(output_info);
   if (sc_output_id == SC_EXT_OUTPUT_ID_NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] Unknown output is not supported \n");
        return SC_EXT_OUTPUT_ID_NULL;
     }

   return sc_output_id;
}

int
eom_scrnconf_get_default_res(int sc_output_id, int preferred_w, int preferred_h)
{
   int sc_res = SC_EXT_RES_NULL;
   int num_res = 0;
   int *resolutions = NULL;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   int i;

   if (sc_output_id == SC_EXT_OUTPUT_ID_NULL)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] sc_output_id is unknown\n");
        return 0;
     }

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   resolutions = calloc(num_res, sizeof (int));
   if (!resolutions)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to allocate resolutions\n");
        goto get_fail;
     }

   if (!_eom_scrnconf_set_modes(sc_output_id, num_res, resolutions))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set modes\n");
        goto get_fail;
     }

   /* find the preferred resolution of the virtual output */
   if (preferred_w > 0 && preferred_h > 0)
     {
        for (i = 0; i < num_res; i++)
          {
             mode_info = sc_ext_mode[i].mode_info;

             if (!mode_info)
               continue;

             if (sc_ext_mode[i].mode_info->width == preferred_w &&
                 sc_ext_mode[i].mode_info->height == preferred_h)
               {
                  sc_res = sc_ext_mode[i].sc_res;
                  break;
               }
          }
     }
   else
     {
        for (i = 0; i < num_res; i++)
          {
             mode_info = sc_ext_mode[i].mode_info;

             if (!mode_info)
               continue;

             sc_res = sc_ext_mode[i].sc_res;
             break;
          }
     }

   free(resolutions);

   if (sc_res == SC_EXT_RES_NULL)
     SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] default resolution is NULL\n");

   if (sc_output_id == SC_EXT_OUTPUT_ID_VIRTUAL)
     sc_res = SC_EXT_RES_1280X720;

   return sc_res;

get_fail:
   if (resolutions)
     free(resolutions);

   return SC_EXT_RES_NULL;
}

int
eom_scrnconf_get_output_id(void)
{
   return sc_ext.sc_output_id;
}

int
eom_scrnconf_set_output_id(int sc_output_id)
{
   sc_ext.sc_output_id = sc_output_id;

   return 1;
}

int
eom_scrnconf_get_res(void)
{
   return sc_ext.sc_res;
}

char*
eom_scrnconf_get_res_str(void)
{
    return _get_str_resolution (sc_ext.sc_res);
}

int
eom_scrnconf_set_res(int sc_res)
{
   sc_ext.sc_res = sc_res;

   return 1;
}

int
eom_scrnconf_get_status(void)
{
   return sc_ext.sc_stat;
}

int
eom_scrnconf_set_status(int sc_stat)
{
   sc_ext.sc_stat = sc_stat;

   return 1;
}

int
eom_scrnconf_get_output_type(int sc_output_id)
{
   if (sc_output_id == SC_EXT_OUTPUT_ID_HDMI)
     return EOM_OUTPUT_TYPE_HDMIA;
   else if (sc_output_id == SC_EXT_OUTPUT_ID_VIRTUAL)
     return EOM_OUTPUT_TYPE_VIRTUAL;

   return EOM_OUTPUT_TYPE_Unknown;
}

int
eom_scrnconf_set_dispmode(int sc_output_id, int sc_dispmode, int sc_res)
{
   _eom_scrnconf_prepare_dispmode_change();

   /* set display mode */
   switch (sc_dispmode)
     {
      case EOM_OUTPUT_MODE_CLONE:
        if (!_eom_scrnconf_set_clone(sc_output_id, sc_res))
          goto set_fail;
        sc_ext.sc_dispmode = EOM_OUTPUT_MODE_CLONE;
        break;

      case EOM_OUTPUT_MODE_DESKTOP:
        if (sc_ext.sc_dispmode == EOM_OUTPUT_MODE_CLONE)
           eom_clone_stop ();
        if (!_eom_scrnconf_set_desktop(sc_output_id, sc_res))
          goto set_fail;

        sc_ext.sc_dispmode = EOM_OUTPUT_MODE_DESKTOP;
        break;

      default:
        break;
     }

   _eom_scrnconf_finish_dispmode_change();

   return 1;

set_fail:
   _eom_scrnconf_finish_dispmode_change();
   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] fail to set display mode (%d)\n", sc_dispmode);
   return 0;
}

int
eom_scrnconf_get_dispmode(void)
{
   return sc_ext.sc_dispmode;
}

int
eom_scrnconf_reset(int sc_output_id)
{
   int i;
   int num_res = 0;

   if (sc_output_id != sc_ext.sc_output_id ||
       sc_ext.sc_output_id == SC_EXT_OUTPUT_ID_NULL)
     return 1;

   if (sc_ext.sc_dispmode == EOM_OUTPUT_MODE_CLONE)
      eom_clone_stop ();

   /* reset scrn conf of a external monitor */
   sc_ext.sc_output_id = SC_EXT_OUTPUT_ID_NULL;
   sc_ext.sc_dispmode = EOM_OUTPUT_MODE_NONE;
   sc_ext.sc_stat = SC_EXT_STATUS_DISCONNECT;
   sc_ext.sc_res = SC_EXT_RES_NULL;

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   /* reset mode list of a external monitor */
   for (i = 0; i < num_res; i++)
     {
        sc_ext_mode[i].refresh = 0.0;
        sc_ext_mode[i].mode_info = NULL;
     }

   connected_info.crtc = 0;
   connected_info.mode = 0;
   connected_info.output = 0;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][srn] reset scrnconf\n");

   return 1;
}

void
eom_scrnconf_container_bg_canvas_visible_set(Eina_Bool visible)
{
   E_Manager *man = NULL;
   E_Container *con = NULL;
   Eina_List *m, *c;

   EINA_LIST_FOREACH(e_manager_list(), m, man)
     {
        EINA_LIST_FOREACH(man->containers, c, con)
          {
             if (visible)
               ecore_evas_show(con->bg_ecore_evas);
             else
               ecore_evas_hide(con->bg_ecore_evas);
          }
     }
}
