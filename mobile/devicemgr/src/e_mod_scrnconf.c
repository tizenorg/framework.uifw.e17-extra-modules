#include "e.h"
#include "e_randr.h"
#include "e_mod_main.h"
#include "e_mod_scrnconf.h"
#include "scrnconf_devicemgr.h"

#ifdef  _MAKE_ATOM
# undef _MAKE_ATOM
#endif

#define _MAKE_ATOM(a, s)                              \
   do {                                               \
        a = ecore_x_atom_get (s);                      \
        if (!a)                                       \
          SLOG(LOG_DEBUG, "DEVICEMGR",                              \
                   "[E-devmgr] ##s creation failed.\n"); \
   } while (0)

#define STR_XRR_DISPLAY_MODE_PROPERTY "XRR_PROPERTY_DISPLAY_MODE"

/* display mode */
typedef enum
{
   XRR_OUTPUT_DISPLAY_MODE_NULL,           /* null */
   XRR_OUTPUT_DISPLAY_MODE_WB_CLONE,       /* write-back */
} XRROutputPropDisplayMode;

/* screen conf dialog */
static E_Dialog *g_dia = NULL;

/* screen configuration info */
static struct
{
   SC_EXT_OUTPUT           sc_output;
   Utilx_Scrnconf_Dispmode sc_dispmode;
   Utilx_Scrnconf_Status   sc_stat;
   SC_EXT_RES              sc_res;
} sc_ext =
{
   SC_EXT_OUTPUT_NULL,
   UTILX_SCRNCONF_DISPMODE_NULL,
   UTILX_SCRNCONF_STATUS_NULL,
   SC_EXT_RES_NULL,
};

/* mode info */
static struct
{
   int sc_res;
   double refresh;
   Ecore_X_Randr_Mode_Info * mode_info;
} sc_ext_mode [] =
{
   {SC_EXT_RES_1920X1080, 0.0, NULL},
   {SC_EXT_RES_1280X720, 0.0, NULL},
   {SC_EXT_RES_720X480, 0.0, NULL},
   {SC_EXT_RES_720X576, 0.0, NULL},
};

static char *str_output[3] = {
    "null",
    "HDMI1",
    "Virtual1",
};

static char *str_dispmode[3] = {
    "null",
    "CLONE",
    "EXTENDED",
};

static char *str_stat[3] = {
    "null",
    "CONNECT",
    "ACTIVE",
};

static char *str_resolution[5] = {
    "null",
    "1920x1080",
    "1280x720",
    "720x480",
    "720x576",
};

/* Calculates the vertical refresh rate of a mode. */
static double
_cal_vrefresh(Ecore_X_Randr_Mode_Info *mode_info)
{
   double refresh = 0.0;
   double dots = mode_info->hTotal * mode_info->vTotal;
   if (!dots)
       return 0;
   refresh = (mode_info->dotClock + dots/2) / dots;

   if (refresh > 0xffff)
      refresh = 0xffff;

    return refresh;
}

static char *
_get_str_output(int output)
{
    char *str = NULL;

    switch (output)
    {
    case SC_EXT_OUTPUT_HDMI:
        str = str_output[1];
        break;
    case SC_EXT_OUTPUT_VIRTUAL:
        str = str_output[2];
        break;
    default:
        str = str_output[0];
        break;
    }

    return str;
}

static char *
_get_str_dispmode(int dispmode)
{
    char *str = NULL;

    switch (dispmode)
    {
    case UTILX_SCRNCONF_DISPMODE_CLONE:
        str = str_dispmode[1];
        break;
    case UTILX_SCRNCONF_DISPMODE_EXTENDED:
        str = str_dispmode[2];
        break;
    default:
        str = str_dispmode[0];
        break;
    }

    return str;
}
static char *
_get_str_stat(int stat)
{
    char *str = NULL;

    switch (stat)
    {
    case UTILX_SCRNCONF_STATUS_CONNECT:
        str = str_stat[1];
        break;
    case UTILX_SCRNCONF_STATUS_ACTIVE:
        str = str_stat[2];
        break;
    case UTILX_SCRNCONF_STATUS_NULL:
        str = str_stat[0];
        break;
    default:
        str = str_stat[0];
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
_get_resolution_str (char *res_name)
{
    int resolution = SC_EXT_RES_NULL;

    if (!strcmp (res_name, str_resolution[1]))
        resolution = SC_EXT_RES_1920X1080;
    else if (!strcmp (res_name, str_resolution[2]))
        resolution = SC_EXT_RES_1280X720;
    else if (!strcmp (res_name, str_resolution[3]))
        resolution = SC_EXT_RES_720X480;
    else if (!strcmp (res_name, str_resolution[4]))
        resolution = SC_EXT_RES_720X576;
    else
        resolution = SC_EXT_RES_NULL;

    return resolution;
}


#if SCRN_CONF_DEBUG
static void
_debug_possible_crtc (E_Randr_Output_Info *output_info)
{
   E_Randr_Crtc_Info *crtc_info = NULL;
   E_Randr_Output_Info *t_o = NULL;
   Ecore_X_Randr_Mode_Info *t_m = NULL;
   Eina_List *t_l, *l_crtc;

   EINA_LIST_FOREACH (output_info->possible_crtcs, l_crtc, crtc_info)
     {
        if (crtc_info == NULL)
           continue;

        SLOG(LOG_DEBUG, "DEVICEMGR", "[DeviceMgr]: possible crtc = %d\n", crtc_info->xid);

        EINA_LIST_FOREACH (crtc_info->outputs, t_l, t_o)
        {
           SLOG(LOG_DEBUG, "DEVICEMGR", "    output : %s\n", t_o->name);
        }

        EINA_LIST_FOREACH (crtc_info->possible_outputs, t_l, t_o)
          {
             SLOG(LOG_DEBUG, "DEVICEMGR", "    possible output : %s\n", t_o->name);
          }

        EINA_LIST_FOREACH (crtc_info->outputs_common_modes, t_l, t_m)
          {
             if (!t_m->name)
                break;
             SLOG(LOG_DEBUG, "DEVICEMGR", "    outputs common modes : %s\n", t_m->name);
          }

        if (!crtc_info->current_mode)
          {
             SLOG(LOG_DEBUG, "DEVICEMGR", "    no current mode \n");
             crtc_xid = crtc_info->xid;
             SLOG(LOG_DEBUG, "DEVICEMGR", "    crtc_id : %d\n", crtc_info->xid);
          }
        else
          {
             SLOG(LOG_DEBUG, "DEVICEMGR", "    current set mode : %s\n", crtc_info->current_mode->name);
             SLOG(LOG_DEBUG, "DEVICEMGR", "    crtc_id : %d\n", crtc_info->xid);
          }
     }
}
#endif

static E_Randr_Output_Info *
_scrnconf_external_get_output_info_from_output_xid (Ecore_X_Randr_Output output_xid)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info == NULL)
           continue;

        if (output_info->xid == output_xid)
             return output_info;
     }

   return NULL;
}

static E_Randr_Output_Info *
_scrnconf_external_get_output_info_from_output (int output)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *iter;

   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info == NULL)
           continue;

        if (!strcmp(output_info->name, _get_str_output(output)))
             return output_info;
     }

   return NULL;
}

static int
_scrnconf_external_get_output (E_Randr_Output_Info *output_info)
{
   int output = SC_EXT_OUTPUT_NULL;

   if (!strcmp (output_info->name, "HDMI1"))
       output = SC_EXT_OUTPUT_HDMI;
   else if (!strcmp (output_info->name, "Virtual1"))
       output = SC_EXT_OUTPUT_VIRTUAL;
   else
       output = SC_EXT_OUTPUT_NULL;

   return output;
}

static int
_scrnconf_external_set_modes (int sc_output, int num_res, int *resolutions)
{
   E_Randr_Output_Info *output_info = NULL;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   Eina_List *l_mode;
   int res = SC_EXT_RES_NULL;
   double refresh = 0.0;
   int i;

   output_info = _scrnconf_external_get_output_info_from_output (sc_output);
   if (!output_info)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to find output_info from sc_output\n");
        return 0;
     }

   EINA_LIST_FOREACH (output_info->monitor->modes, l_mode, mode_info)
     {
        if (mode_info == NULL)
           continue;

#if 0 //debug
        SLOG(LOG_DEBUG, "DEVICEMGR", "%s(%d): mode_info->name, %s, vrefresh, %f\n"
                , __func__, __LINE__, mode_info->name, _cal_vrefresh(mode_info));
#endif
        res = _get_resolution_str (mode_info->name);
        if (res == SC_EXT_RES_NULL)
            continue;

       for (i = 0; i < num_res; i++)
         {
            if (sc_ext_mode[i].sc_res == res)
              {
                 refresh = _cal_vrefresh (mode_info);
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
        SLOG(LOG_DEBUG, "DEVICEMGR", "[DeviceMgr]: res info:: (%d): %s, %f, %p\n"
                , i, _get_str_resolution(sc_ext_mode[i].sc_res), sc_ext_mode[i].refresh, sc_ext_mode[i].mode_info);

     }
#endif

   return 1;
}

static int
_scrnconf_external_get_setting_info (int output, int resolution, Ecore_X_Randr_Output *output_xid,
        Ecore_X_Randr_Crtc *crtc_xid, Ecore_X_Randr_Mode *mode_xid, int *output_w, int *output_h)
{
   E_Randr_Output_Info *output_info = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL;
   Ecore_X_Randr_Mode_Info *mode_info = NULL;
   Eina_List *l_crtc;
   Eina_Bool found_output = EINA_FALSE;
   int num_res = 0;
   int i;

   output_info = _scrnconf_external_get_output_info_from_output (output);
   if (output_info == NULL)
     {
       SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: fail to get output_info from sc_output\n");
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
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: fail to get mode_info from sc_output\n");
        goto finish;
     }

   *output_w = mode_info->width;
   *output_h = mode_info->height;
   *mode_xid = mode_info->xid;
   found_output = EINA_TRUE;

#if SCRN_CONF_DEBUG
   _debug_possible_crtc (output_info);
#endif

   EINA_LIST_FOREACH (output_info->possible_crtcs, l_crtc, crtc_info)
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

static int
_scrnconf_external_set_extended (int output, int resolution)
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
   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, l_output, output_info)
     {

        if (output_info == NULL)
           continue;

        if (!strcmp (output_info->name, "LVDS1"))
          {
            lvds_x = output_info->crtc->geometry.x;
            lvds_y = output_info->crtc->geometry.y;
            lvds_w = output_info->crtc->current_mode->width;
            lvds_h = output_info->crtc->current_mode->height;
            break;
          }
     }

   if (!_scrnconf_external_get_setting_info (output, resolution, output_xid, &crtc_xid, &mode_xid, &output_w, &output_h))
       goto set_fail;


   /* set the output is right-of lvds output */
   output_x = lvds_w;
   output_y = 0;
   resize_w = lvds_w + output_w;

   if (lvds_h > output_h)
      resize_h = lvds_h;
   else
      resize_h = output_h;

   ecore_x_grab ();

   SLOG(LOG_DEBUG, "DEVICEMGR", "[DeviceMgr]: set screen resize (%d,%d)\n", resize_w, resize_h);
   if (!ecore_x_randr_screen_current_size_set (e_randr_screen_info.root, resize_w, resize_h, 0, 0))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: fail to resize the screen\n");
        goto set_fail;
     }

   SLOG(LOG_DEBUG, "DEVICEMGR", "[DeviceMgr]: set crtc_id, %d output_id, %d mode_id, %d)!\n", crtc_xid, output_xid[0], mode_xid);
   if (!ecore_x_randr_crtc_settings_set (e_randr_screen_info.root, crtc_xid, output_xid, 1, output_x,
                                       output_y, mode_xid, ECORE_X_RANDR_ORIENTATION_ROT_0))
   {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: fail to set x=%d, y=%d, w=%d, h=%d mode_id=%d, crtc_id=%d\n",
                 output_x, output_y, output_w, output_h, mode_xid, crtc_xid);
        goto set_fail;
   }

   e_mod_scrnconf_container_bg_canvas_visible_set(EINA_TRUE);

   ecore_x_ungrab ();

   return 1;

set_fail:
   SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: %s fail to set the extended mode\n", _get_str_output(output));
   ecore_x_ungrab ();

   return 0;
}

static int
_scrnconf_external_set_clone (int output, int resolution)
{
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *l_output;
   int output_w = 0, output_h = 0;
   int lvds_x = 0, lvds_y = 0, lvds_w = 0, lvds_h = 0;
   int resize_w = 0, resize_h = 0;
   Ecore_X_Randr_Crtc crtc_xid = 0;
   Ecore_X_Randr_Output output_xid[1] = {0};
   Ecore_X_Randr_Mode mode_xid = 0;
   Ecore_X_Atom dispmode;
   int value[2];

   /* get lvds information */
   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, l_output, output_info)
     {

        if (output_info == NULL)
           continue;

        if (!strcmp (output_info->name, "LVDS1"))
          {
            lvds_x = output_info->crtc->geometry.x;
            lvds_y = output_info->crtc->geometry.y;
            lvds_w = output_info->crtc->current_mode->width;
            lvds_h = output_info->crtc->current_mode->height;
            break;
          }
     }

   resize_w = lvds_w;
   resize_h = lvds_h;

   if (!_scrnconf_external_get_setting_info (output, resolution, output_xid, &crtc_xid, &mode_xid, &output_w, &output_h))
       goto set_fail;

   ecore_x_grab ();

   SLOG(LOG_DEBUG, "DEVICEMGR", "[DeviceMgr]: set screen resize (%d,%d)\n", resize_w, resize_h);
   if (!ecore_x_randr_screen_current_size_set (e_randr_screen_info.root, resize_w, resize_h, 0, 0))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: fail to resize the screen\n");
        goto set_fail;
     }

   _MAKE_ATOM (dispmode, STR_XRR_DISPLAY_MODE_PROPERTY);

   value[0] = XRR_OUTPUT_DISPLAY_MODE_WB_CLONE;
   value[1] = mode_xid;

   /* no ecore x API for XRRChangeOutputProperty */
   XRRChangeOutputProperty (ecore_x_display_get (), output_xid[0], dispmode, XA_INTEGER, 32,
                           PropModeReplace, (unsigned char *)&value, 2);

   e_mod_scrnconf_container_bg_canvas_visible_set(EINA_FALSE);

   ecore_x_sync ();

   ecore_x_ungrab ();

   return 1;

set_fail:
   SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr]: %s fail to set the clone mode\n", _get_str_output(output));
   ecore_x_ungrab ();

   return 0;
}

static void
_dialog_extended_btn_cb (void *data, E_Dialog *g_dia)
{
   E_Randr_Output_Info *output_info = (E_Randr_Output_Info *)data;
   int sc_output = SC_EXT_OUTPUT_NULL;
   int sc_res = SC_EXT_RES_NULL;
   int num_res = 0;
   int *resolutions  = NULL;

   sc_output = _scrnconf_external_get_output (output_info);
   if (sc_output == SC_EXT_OUTPUT_NULL)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to find sc_output from output_info\n");
        goto set_fail;
     }

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   resolutions = calloc (num_res, sizeof (int));
   if (!resolutions)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to allocate resolutions\n");
        goto set_fail;
     }

   if (!_scrnconf_external_set_modes (sc_output, num_res, resolutions))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to set modes\n");
        goto set_fail;
     }

   sc_res = resolutions[0];

   if (!_scrnconf_external_set_extended (sc_output, sc_res))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to set modes\n");
        goto set_fail;
     }

   /* set display mode */
   sc_ext.sc_dispmode = UTILX_SCRNCONF_DISPMODE_EXTENDED;

   e_mod_scrnconf_external_send_current_status ();

   if (resolutions)
       free (resolutions);

   /* destroy dialog */
   e_mod_scrnconf_external_dialog_free ();
   return;

set_fail:
   if (resolutions)
       free (resolutions);

   /* destroy dialog */
   e_mod_scrnconf_external_dialog_free ();
}

static void
_dialog_clone_btn_cb (void *data, E_Dialog *g_dia)
{
   E_Randr_Output_Info *output_info = (E_Randr_Output_Info *)data;
   int sc_output = SC_EXT_OUTPUT_NULL;
   int sc_res = SC_EXT_RES_NULL;
   int num_res = 0;
   int *resolutions  = NULL;

   sc_output = _scrnconf_external_get_output (output_info);
   if (sc_output == SC_EXT_OUTPUT_NULL)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to find sc_output from output_info\n");
        goto set_fail;
     }

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   resolutions = calloc (num_res, sizeof (int));
   if (!resolutions)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to allocate resolutions\n");
        goto set_fail;
     }

   if (!_scrnconf_external_set_modes (sc_output, num_res, resolutions))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to set modes\n");
        goto set_fail;
     }

   sc_res = resolutions[0];

   if (!_scrnconf_external_set_clone (sc_output, sc_res))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to set clone\n");
        goto set_fail;
     }

   /* set display mode */
   sc_ext.sc_dispmode = UTILX_SCRNCONF_DISPMODE_EXTENDED;

   e_mod_scrnconf_external_send_current_status ();

   if (resolutions)
       free (resolutions);

   /* destroy dialog */
   e_mod_scrnconf_external_dialog_free ();
   return;

set_fail:
   if (resolutions)
       free (resolutions);

   /* destroy dialog */
   e_mod_scrnconf_external_dialog_free ();
}

static void
_dialog_cancle_btn_cb (void *data, E_Dialog *g_dia)
{
   /* destroy dialog */
   e_mod_scrnconf_external_dialog_free ();
}

void
e_mod_scrnconf_external_init ()
{
   /* init scrn conf get property */
   scrnconf_ext_update_get_perperty(ecore_x_display_get (), "null", "null", "null", "null");
}

void
e_mod_scrnconf_external_deinit ()
{}


void
e_mod_scrnconf_external_dialog_new (int output)
{
   E_Manager *man;
   E_Container *con;
   E_Randr_Output_Info *output_info = NULL;

   if (output == SC_EXT_OUTPUT_NULL)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : Unknown output is not supported \n");
        return;
     }

   output_info = _scrnconf_external_get_output_info_from_output (output);
   if (!output_info)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to get output from xid\n");
        return;
     }

   /* set to be null, if dialog already exists */
   if (g_dia != NULL)
      e_mod_scrnconf_external_dialog_free ();

   man = e_manager_current_get ();
   con = e_container_current_get (man);

   if (output == SC_EXT_OUTPUT_HDMI)
     {
        g_dia = e_dialog_new (con, "E", "hdmi mode");
        e_dialog_title_set (g_dia, "\nHDMI Connected !!\n");
        e_dialog_text_set (g_dia, "<hilight>HDMI Connected<hilight><br>Select the screen configuration mode<br>");
     }
   else if (output == SC_EXT_OUTPUT_VIRTUAL)
     {
        g_dia = e_dialog_new (con, "E", "virtual mode");
        e_dialog_title_set (g_dia, "\nVirtual Connected !!\n");
        e_dialog_text_set (g_dia, "<hilight>Virtual Connected<hilight><br>Select the screen configuration mode<br>");
     }

   e_dialog_button_add (g_dia, "CLONE", NULL, _dialog_clone_btn_cb, (void *)output_info);
   e_dialog_button_add (g_dia, "EXTENDED", NULL, _dialog_extended_btn_cb, (void *)output_info);
   e_dialog_button_add (g_dia, "CANCLE", NULL, _dialog_cancle_btn_cb, (void *)output_info);

   e_dialog_resizable_set (g_dia, 1);
   e_win_centered_set (g_dia->win, 1);
   e_dialog_show (g_dia);

   ecore_x_sync ();
}

void
e_mod_scrnconf_external_dialog_free ()
{
   if (g_dia != NULL)
     {
        e_util_defer_object_del (E_OBJECT (g_dia));
        g_dia = NULL;
     }

   ecore_x_sync ();
}

int
e_mod_scrnconf_external_get_output_from_xid (Ecore_X_Randr_Output output_xid)
{
   E_Randr_Output_Info *output_info = NULL;
   int output = SC_EXT_OUTPUT_NULL;

   output_info = _scrnconf_external_get_output_info_from_output_xid (output_xid);
   if (!output_info)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to get output from xid\n");
        return SC_EXT_OUTPUT_NULL;
     }

   output = _scrnconf_external_get_output (output_info);
   if (output == SC_EXT_OUTPUT_NULL)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : Unknown output is not supported \n");
        return SC_EXT_OUTPUT_NULL;
     }

   return output;
}

int
e_mod_scrnconf_external_get_default_res (int sc_output, int preferred_w, int preferred_h)
{
   int sc_res = SC_EXT_RES_NULL;
   int num_res = 0;
   int *resolutions  = NULL;
   Ecore_X_Randr_Mode_Info * mode_info = NULL;
   int i;

   if (sc_output == SC_EXT_OUTPUT_NULL)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : sc_output is unknown\n");
        return 0;
     }

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   resolutions = calloc (num_res, sizeof (int));
   if (!resolutions)
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to allocate resolutions\n");
        goto get_fail;
     }

   if (!_scrnconf_external_set_modes (sc_output, num_res, resolutions))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to set modes\n");
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

   free (resolutions);

   if (sc_res == SC_EXT_RES_NULL)
      SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : default resolution is NULL\n");


   if (sc_output == SC_EXT_OUTPUT_VIRTUAL)
      sc_res = SC_EXT_RES_1280X720;

   return sc_res;

get_fail:
   if (resolutions)
      free (resolutions);

   return SC_EXT_RES_NULL;
}

int
e_mod_scrnconf_external_get_output (void)
{
   return sc_ext.sc_output;
}

int
e_mod_scrnconf_external_set_output (int sc_output)
{
   sc_ext.sc_output = sc_output;

   return 1;
}

int
e_mod_scrnconf_external_get_res (void)
{
  return sc_ext.sc_res;
}

int
e_mod_scrnconf_external_set_res (int sc_res)
{
   sc_ext.sc_res = sc_res;

   return 1;
}

int
e_mod_scrnconf_external_get_status (void)
{
  return sc_ext.sc_stat;
}

int
e_mod_scrnconf_external_set_status (int sc_stat)
{
   sc_ext.sc_stat = sc_stat;

   return 1;
}

int
e_mod_scrnconf_external_set_dispmode (int sc_output, int sc_dispmode, int sc_res)
{
   /* set display mode */
   switch (sc_dispmode)
     {
        case UTILX_SCRNCONF_DISPMODE_CLONE:
           if (!_scrnconf_external_set_clone (sc_output, sc_res))
                goto set_fail;
           sc_ext.sc_dispmode = UTILX_SCRNCONF_DISPMODE_CLONE;
           break;
        case UTILX_SCRNCONF_DISPMODE_EXTENDED:
           if (!_scrnconf_external_set_extended (sc_output, sc_res))
                goto set_fail;
           sc_ext.sc_dispmode = UTILX_SCRNCONF_DISPMODE_EXTENDED;
           break;
        default:
           break;
     }

   return 1;

set_fail:
   SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to set display mode (%d)\n", sc_dispmode);
   return 0;
}

int
e_mod_scrnconf_external_get_dispmode (void)
{
   return sc_ext.sc_dispmode;
}

int
e_mod_scrnconf_external_send_current_status (void)
{
   char * str_output = _get_str_output (sc_ext.sc_output);
   char * str_stat = _get_str_stat (sc_ext.sc_stat);
   char * str_res = _get_str_resolution (sc_ext.sc_res);
   char * str_dispmode = _get_str_dispmode (sc_ext.sc_dispmode);

   if (!scrnconf_ext_update_get_perperty(ecore_x_display_get (), str_output, str_stat, str_res, str_dispmode))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to update get property \n");
        return 0;
     }

   if (!scrnconf_ext_send_status (ecore_x_display_get (), sc_ext.sc_stat, sc_ext.sc_dispmode))
     {
        SLOG(LOG_DEBUG, "DEVICEMGR",  "[DeviceMgr] : fail to send current status \n");
        return 0;
     }

   return 1;
}

int
e_mod_scrnconf_external_reset (int sc_output)
{
   int i;
   int num_res = 0;

   if (sc_output != sc_ext.sc_output ||
       sc_ext.sc_output == SC_EXT_OUTPUT_NULL)
      return 1;

   /* reset scrn conf of a external monitor */
   sc_ext.sc_output = SC_EXT_OUTPUT_NULL;
   sc_ext.sc_dispmode = UTILX_SCRNCONF_DISPMODE_NULL;
   sc_ext.sc_stat = UTILX_SCRNCONF_STATUS_NULL;
   sc_ext.sc_res = SC_EXT_RES_NULL;

   num_res = sizeof (sc_ext_mode) / sizeof (sc_ext_mode[0]);

   /* reset mode list of a external monitor */
   for (i = 0; i < num_res; i++)
     {
        sc_ext_mode[i].refresh = 0.0;
        sc_ext_mode[i].mode_info = NULL;
     }
   return 1;
}

void
e_mod_scrnconf_container_bg_canvas_visible_set(Eina_Bool visible)
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
