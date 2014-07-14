#include "e_mod_eom_server.h"
#include "e_mod_eom_dbus.h"
#include "e_mod_eom_config.h"
#include "e_mod_eom_privates.h"
#include "e_mod_eom_scrnconf.h"
#include "e_mod_eom_ownership.h"

#include <sys/ioctl.h>
#include <dri2.h>
#include <xf86drm.h>
#include <exynos_drm.h>

#define STR_ATOM_SCRNCONF_INFO   "_SCRNCONF_INFO"

static EOM e_eom;

static void
_eom_server_get_preferred_size(int sc_output_id, int *preferred_w, int *preferred_h)
{
   if (sc_output_id == SC_EXT_OUTPUT_ID_HDMI)
     {
        *preferred_w = e_eom.hdmi_preferred_w;
        *preferred_h = e_eom.hdmi_preferred_h;
     }
   else if (sc_output_id == SC_EXT_OUTPUT_ID_VIRTUAL)
     {
        *preferred_w = e_eom.virtual_preferred_w;
        *preferred_h = e_eom.virtual_preferred_h;
     }
   else
     {
        *preferred_w = 0;
        *preferred_h = 0;
     }
}

static int
_eom_server_set_edid (int plug, int connection, int extensions, char *edid)
{
   int screen;
   int drm_fd;
   int eventBase, errorBase;
   int dri2Major, dri2Minor;
   char *driverName, *deviceName;
   Display *dpy;
   int ret;
   drm_magic_t magic;

   dpy = ecore_x_display_get ();
   screen = DefaultScreen(dpy);

   /* DRI2 */
   if (!DRI2QueryExtension (dpy, &eventBase, &errorBase))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: DRI2QueryExtension !!\n");
        return 0;
     }

   if (!DRI2QueryVersion (dpy, &dri2Major, &dri2Minor))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: DRI2QueryVersion !!\n");
        return 0;
     }

   if (!DRI2Connect (dpy, RootWindow(dpy, screen), &driverName, &deviceName))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: DRI2Connect !!\n");
        return 0;
     }

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] Open drm device : %s\n", deviceName);

   /* get the drm_fd though opening the deviceName */
   drm_fd = open (deviceName, O_RDWR);
   if (drm_fd < 0)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: cannot open drm device (%s)\n", deviceName);
        free (driverName);
        free (deviceName);
        return 0;
     }

   drmGetMagic(drm_fd, &magic);
   if (!DRI2Authenticate(dpy, RootWindow(dpy, screen), magic))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: DRI2Authenticate (%d)\n", magic);
        free (driverName);
        free (deviceName);
        close (drm_fd);
        return 0;
     }

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] plug: %d, connection: %d, extensions: %d\n",
       plug, connection, extensions);

   struct drm_exynos_vidi_connection vidi;

   if (plug)
     {
        vidi.connection = connection;
        vidi.extensions = extensions;
        vidi.edid = (uint64_t *)edid;
     }
   else
     {
        vidi.connection = connection;
        vidi.extensions = extensions;
        vidi.edid = NULL;
     }

   ret = ioctl (drm_fd, DRM_IOCTL_EXYNOS_VIDI_CONNECTION, &vidi);
   if (ret)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: VIDI_CONNECTION(%s)\n", strerror(errno));
        free (driverName);
        free (deviceName);
        close (drm_fd);
        return 0;
     }

   free (driverName);
   free (deviceName);
   close (drm_fd);

   return 1;
}

static int
_eom_server_list_length (Eina_List *list)
{
   Eina_List *l;
   Eina_Value *v;
   int num = 0;

   if (!list)
      return 0;

   EINA_LIST_FOREACH(list, l, v)
     {
        num++;
     }

   return num;
}

static Eina_List*
_eom_server_method_get_output_ids(const char *client, Eina_List *list)
{
   Eina_List *ret_list = NULL;
   Eina_Value *v;
   int sc_output_id;
   int sc_status;

   sc_output_id = eom_scrnconf_get_output_id ();
   sc_status = eom_scrnconf_get_status ();

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] sc_output_id(%d) sc_status(%d)\n",
          sc_output_id, sc_status);

   if (sc_status == SC_EXT_STATUS_DISCONNECT)
      return NULL;

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_output_id);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static Eina_List*
_eom_server_method_get_output_info(const char *client, Eina_List *list)
{
   Eina_List *ret_list = NULL;
   Eina_Value *v;
   int sc_output_id, sc_type, sc_dispmode;
   char str_res[STR_LEN];

   sc_output_id = eom_scrnconf_get_output_id ();
   sc_type = eom_scrnconf_get_output_type (eom_scrnconf_get_output_id ());
   snprintf (str_res, STR_LEN, "%s", eom_scrnconf_get_res_str ());
   sc_dispmode = eom_scrnconf_get_dispmode ();

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] get output info: %d,%d,%s,%d\n",
        sc_output_id, sc_type, str_res, sc_dispmode);

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_output_id);
   ret_list = eina_list_append (ret_list, v);

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_type);
   ret_list = eina_list_append (ret_list, v);

   v = eina_value_new (EINA_VALUE_TYPE_STRING);
   eina_value_set (v, str_res);
   ret_list = eina_list_append (ret_list, v);

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_dispmode);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static Eina_List*
_eom_server_method_take_ownership(const char *client, Eina_List *list)
{
   int sc_output_id;
   int ret = 0;
   Eina_List *ret_list = NULL;
   Eina_Value *v;

   EOM_CHK_GOTO(_eom_server_list_length(list) == 1, done);

   v = eina_list_nth (list, 0);
   eina_value_get (v, &sc_output_id);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] sc_output_id: %d\n", sc_output_id);

   ret = eom_ownership_take (sc_output_id, (char*)client);

done:

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, ret);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static Eina_List*
_eom_server_method_release_ownership(const char *client, Eina_List *list)
{
   int sc_output_id;
   Eina_Value *v;

   EOM_CHK_RETV(_eom_server_list_length(list) == 1, NULL);

   v = eina_list_nth (list, 0);
   eina_value_get (v, &sc_output_id);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] sc_output_id: %d\n", sc_output_id);

   eom_ownership_release (sc_output_id, (char*)client);

   return NULL;
}

static Eina_List*
_eom_server_method_has_ownership(const char *client, Eina_List *list)
{
   int sc_output_id;
   int ret = 0;
   Eina_List *ret_list = NULL;
   Eina_Value *v;

   EOM_CHK_GOTO(_eom_server_list_length(list) == 1, done);

   v = eina_list_nth (list, 0);
   eina_value_get (v, &sc_output_id);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] sc_output_id: %d\n", sc_output_id);

   ret = eom_ownership_has (sc_output_id, (char*)client);

done:

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, ret);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static Eina_List*
_eom_server_method_set_edid(const char *client, Eina_List *list)
{
   int ret = 0;
   Eina_List *ret_list = NULL;
   Eina_Value *v;
   int plug, connection, extensions;
   Eina_Value_Blob blob;

   EOM_CHK_GOTO(_eom_server_list_length(list) == 4, done);

   v = eina_list_nth (list, 0);
   eina_value_get (v, &plug);

   v = eina_list_nth (list, 1);
   eina_value_get (v, &connection);

   v = eina_list_nth (list, 2);
   eina_value_get (v, &extensions);

   v = eina_list_nth (list, 3);
   eina_value_get (v, &blob);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] plug(%d), connection(%d), extensions(%d), blob.memory(%p,%d)\n",
          plug, connection, extensions, (char*)blob.memory, blob.size);

   ret = _eom_server_set_edid (plug, connection, extensions, (char*)blob.memory);

done:

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, ret);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static Eina_List*
_eom_server_method_set_mode(const char *client, Eina_List *list)
{
   int sc_dispmode = 0;
   int sc_res = 0;
   int sc_stat = 0;
   int sc_output_id = 0;
   int preferred_w = 0, preferred_h = 0;
   Eina_List *ret_list = NULL;
   Eina_Value *v;
   int ret = 0;

   EOM_CHK_GOTO(_eom_server_list_length(list) == 2, done);

   v = eina_list_nth (list, 0);
   eina_value_get (v, &sc_output_id);
   EOM_CHK_GOTO(sc_output_id == eom_scrnconf_get_output_id(), done);

   v = eina_list_nth (list, 1);
   eina_value_get (v, &sc_dispmode);
   EOM_CHK_GOTO (sc_dispmode != eom_scrnconf_get_dispmode(), done);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] sc_output_id(%d), sc_dispmode(%d)\n", sc_output_id, sc_dispmode);

   sc_stat = eom_scrnconf_get_status();
   EOM_CHK_GOTO (sc_stat != SC_EXT_STATUS_DISCONNECT, done);

   if (!eom_ownership_can_set (sc_output_id, (char*)client))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail: not have ownership \n");
        goto done;
     }

   _eom_server_get_preferred_size(sc_output_id, &preferred_w, &preferred_h);
   sc_res = eom_scrnconf_get_default_res(sc_output_id, preferred_w, preferred_h);
   EOM_CHK_GOTO(sc_res != SC_EXT_RES_NULL, done);

   if (!eom_scrnconf_set_dispmode(sc_output_id, sc_dispmode, sc_res))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] fail to get external output \n");
        goto done;
     }

   eom_scrnconf_set_status(SC_EXT_STATUS_ACTIVATE);

   ret = 1;

done:

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, ret);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static Eina_List*
_eom_server_method_eom_cfg(const char *client, Eina_List *list)
{
   eom_config_type_e type = 0;
   Eina_List *ret_list = NULL;
   Eina_Value *v;
   int ret = 0;
   int data;

   EOM_CHK_GOTO(_eom_server_list_length(list) == 2, done);

   v = eina_list_nth (list, 0);
   eina_value_get (v, &type);

   v = eina_list_nth (list, 1);
   eina_value_get (v, &data);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] type(%d), data(%d)\n", type, data);

   if (type == EOM_CONFIG_POPUP)
     {
        Eina_Bool set_popup = EINA_FALSE;
        if (data == 1)
          set_popup = EINA_TRUE;
        else
          set_popup = EINA_FALSE;
        e_eom.isPopUpEnabled = set_popup;
        _e_eom_cfg->ScrnConf.isPopUpEnabled = set_popup;
     }
   else if (type == EOM_CONFIG_DEFAULT_DISPMODE)
     {
        e_eom.default_dispmode = data;
        _e_eom_cfg->ScrnConf.default_dispmode = data;
     }
   else
     goto done;

   eom_config_save();

done:

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, ret);
   ret_list = eina_list_append (ret_list, v);

   return ret_list;
}

static EomDbusMethod methods[] =
{
   {"GetOutputIDs", _eom_server_method_get_output_ids, NULL},
   {"GetOutputInfo", _eom_server_method_get_output_info, NULL},
   {"TakeOwnership", _eom_server_method_take_ownership, NULL},
   {"ReleaseOwnership", _eom_server_method_release_ownership, NULL},
   {"HasOwnership", _eom_server_method_has_ownership, NULL},
   {"SetEdid", _eom_server_method_set_edid, NULL},
   {"SetMode", _eom_server_method_set_mode, NULL},
   {"EomCfg", _eom_server_method_eom_cfg, NULL},
};

#define NUM_METHODS    (sizeof(methods) / sizeof(methods[0]))

static void
_eom_server_init_method(void)
{
   int i;

   for (i = 0; i < NUM_METHODS; i++)
      eom_dbus_add_method (&methods[i]);
}

static Eina_Bool
_eom_server_output_change_cb(void *data, int type, void *ev)
{
   int sc_output_id = 0;
   int sc_res = 0;
   int sc_stat = 0;
   int preferred_w = 0, preferred_h = 0;

   if (type == ECORE_X_EVENT_RANDR_OUTPUT_CHANGE)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] Output Change!: \n");
        Ecore_X_Event_Randr_Output_Change *event = (Ecore_X_Event_Randr_Output_Change *)ev;
        /* available information:
           struct _Ecore_X_Event_Randr_Output_Change
           {
              Ecore_X_Window                  win;
              Ecore_X_Randr_Output            output;
              Ecore_X_Randr_Crtc              crtc;
              Ecore_X_Randr_Mode              mode;
              Ecore_X_Randr_Orientation       orientation;
              Ecore_X_Randr_Connection_Status connection;
              Ecore_X_Render_Subpixel_Order   subpixel_order;
           };
         */
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] Output Connection!: %d (connected = %d, disconnected = %d, unknown %d)\n",
             event->connection, ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED, ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED, ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN);

        /* check status of a output */
        if (event->connection == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED &&
            event->crtc == 0 &&
            event->mode == 0)
          {
             /* if display mode is set by the client message, ignore output conncetion */

             sc_stat = eom_scrnconf_get_status();
             if (sc_stat == SC_EXT_STATUS_CONNECT ||
                 sc_stat == SC_EXT_STATUS_ACTIVATE)
               {
                  //SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] external monitor status is already connected \n");
                  return 1;
               }

             /* set the output of the external monitor */
             sc_output_id = eom_scrnconf_get_output_id_from_xid(event->output);
             EOM_CHK_RETV(sc_output_id != SC_EXT_OUTPUT_ID_NULL, 1);
             eom_scrnconf_set_output_id(sc_output_id);

             /* set the resolution of the external monitor */
             _eom_server_get_preferred_size(sc_output_id, &preferred_w, &preferred_h);
             sc_res = eom_scrnconf_get_default_res(sc_output_id, preferred_w, preferred_h);
             EOM_CHK_GOTO(sc_res != SC_EXT_RES_NULL, fail);
             eom_scrnconf_set_res(sc_res);

             SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] default display mode!: %d (clone = %d, desktop = %d)\n",
                  e_eom.default_dispmode, EOM_OUTPUT_MODE_CLONE, EOM_OUTPUT_MODE_DESKTOP);

             /* set the default display mode of the external monitor */
             if (e_eom.default_dispmode == EOM_OUTPUT_MODE_CLONE ||
                 e_eom.default_dispmode == EOM_OUTPUT_MODE_DESKTOP)
               {
                  if (!eom_scrnconf_set_dispmode(sc_output_id, e_eom.default_dispmode, sc_res))
                    {
                       eom_scrnconf_set_status(SC_EXT_STATUS_CONNECT);
                       /* generate dialog */
                       if (e_eom.isPopUpEnabled)
                         eom_scrnconf_dialog_new(sc_output_id);
                       goto fail;
                    }
                  eom_scrnconf_set_status(SC_EXT_STATUS_ACTIVATE);
               }
             else
               {
                  eom_scrnconf_set_status(SC_EXT_STATUS_CONNECT);
               }

             /* generate dialog */
             if (e_eom.isPopUpEnabled)
               eom_scrnconf_dialog_new(sc_output_id);

             eom_server_send_notify (EOM_NOTIFY_OUTPUT_ADD, sc_output_id);
          }
        else if (event->connection == ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED)
          {
             /* set the output of the external monitor */
             sc_output_id = eom_scrnconf_get_output_id_from_xid(event->output);
             EOM_CHK_RETV(sc_output_id != SC_EXT_OUTPUT_ID_NULL, 1);

             eom_scrnconf_reset(sc_output_id);

             eom_server_send_notify (EOM_NOTIFY_OUTPUT_REMOVE, sc_output_id);

             /* if dialog is still showing, destroy dialog */
             eom_scrnconf_dialog_free();

             eom_scrnconf_container_bg_canvas_visible_set(EINA_FALSE);
          }
        else if (event->connection == ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN)
          {
             /* if dialog is still showing, destroy dialog */
             eom_scrnconf_dialog_free();

             if (e_eom.default_dispmode == EOM_OUTPUT_MODE_DESKTOP)
               {
                  eom_scrnconf_container_bg_canvas_visible_set(EINA_TRUE);
               }
          }
     }

   return EINA_TRUE;

fail:
   eom_scrnconf_reset(sc_output_id);
   return EINA_TRUE;
}


static void
_eom_server_init_output(void)
{
   int i;
   XRROutputInfo *output_info = NULL;
   XRRScreenResources *res = XRRGetScreenResources(ecore_x_display_get(),
                                                   ecore_x_window_root_first_get());

   e_eom.output = 0;

   if (res && (res->noutput != 0))
     {
        for ( i = 0; i < res->noutput; i++ )
          {
             output_info = XRRGetOutputInfo(ecore_x_display_get(), res, res->outputs[i]);
             if (output_info && output_info->name && !strncmp(output_info->name, "LVDS1", 5))
               {
                  e_eom.output = res->outputs[i];
                  SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] LVDS1 was found !\n");
                  XRRFreeOutputInfo(output_info);
                  break;
               }
             else
               {
                  e_eom.output = res->outputs[i];
                  SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] LVDS1 was not found yet !\n");
               }

             if (output_info)
               XRRFreeOutputInfo(output_info);
          }
     }

   if (res)
     XRRFreeScreenResources (res);

   if (!e_eom.output)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] Failed to init output !\n");
     }
}

int
eom_server_send_notify (eom_notify_type_e type, int sc_output_id)
{
   Eina_List *list = NULL;
   Eina_Value *v;
   int sc_type, sc_dispmode;
   char str_res[STR_LEN];

   sc_type = eom_scrnconf_get_output_type (sc_output_id);
   sc_dispmode = eom_scrnconf_get_dispmode ();
   snprintf(str_res, STR_LEN, "%s", eom_scrnconf_get_res_str ());

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, type);
   list = eina_list_append (list, v);

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_output_id);
   list = eina_list_append (list, v);

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_type);
   list = eina_list_append (list, v);

   v = eina_value_new (EINA_VALUE_TYPE_STRING);
   eina_value_set (v, str_res);
   list = eina_list_append (list, v);

   v = eina_value_new (EINA_VALUE_TYPE_INT);
   eina_value_set (v, sc_dispmode);
   list = eina_list_append (list, v);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] send notify: %d,%d,%d,%s,%d\n",
        type, sc_output_id, sc_type, str_res, sc_dispmode);

   eom_dbus_send_signal ("Notify", list);

   if (list)
      eom_dbus_free_list (list);

   return 1;
}

Eina_Bool
eom_server_init(E_Module *m)
{
   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] eom_init!!!!\n");
   memset(&e_eom, 0, sizeof(EOM));

   if (!eom_config_init())
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] Failed to get configuration from %s.cfg file !\n", E_EOM_CFG);
        return EINA_FALSE;
     }

   e_eom.default_dispmode = _e_eom_cfg->ScrnConf.default_dispmode;
   e_eom.isPopUpEnabled = _e_eom_cfg->ScrnConf.isPopUpEnabled;

   setenv("EOM_PRIVATE_CONN", "1", 1);
   eom_dbus_connect();

   eom_scrnconf_init();

   _eom_server_init_output();

   eom_scrnconf_container_bg_canvas_visible_set(EINA_FALSE);

   _eom_server_init_method();

   e_eom.randr_output_handler = ecore_event_handler_add(ECORE_X_EVENT_RANDR_OUTPUT_CHANGE, _eom_server_output_change_cb, NULL);

   if (!e_eom.randr_output_handler)
      SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] Failed to add ECORE_X_EVENT_RANDR_OUTPUT_CHANGE handler\n");

   return EINA_TRUE;
}

void
eom_server_deinit(E_Module *m)
{
   if (e_eom.randr_output_handler)
     {
        ecore_event_handler_del (e_eom.randr_output_handler);
        e_eom.randr_output_handler = NULL;
     }

   eom_ownership_deinit();

   eom_config_shutdown();
}

int
scrnconf_ext_update_get_perperty(Display *dpy, char *str_output_id, char *str_stat, char *str_res, char *str_dispmode)
{
   EOM_CHK_RETV(dpy, 0);

   Window win = DefaultRootWindow(dpy);
   XTextProperty xtp;
   Atom scrnconf_atom = None;
   char *str = NULL;
   int size = 0;

   scrnconf_atom = XInternAtom(dpy, STR_ATOM_SCRNCONF_INFO, False);

   size = strlen(str_output_id) + strlen(str_stat) + strlen(str_res) + strlen(str_dispmode) + 4;

   str = calloc(size, sizeof(char));
   snprintf(str, size, "%s,%s,%s,%s", str_output_id, str_stat, str_res, str_dispmode);

   xtp.value = (unsigned char *)str;
   xtp.format = 8;
   xtp.encoding = XA_STRING;
   xtp.nitems = size;
   XSetTextProperty(dpy, win, &xtp, scrnconf_atom);

   free(str);

   return 1;
}

