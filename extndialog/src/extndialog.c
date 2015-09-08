#include "extndialog.h"

#define WIN_PROP_NAME "SYSTEM_POPUP"

static Eina_Bool
_update_locale (const char *domain, const char *dir)
{
   char *lang;
   char *region;

   lang = vconf_get_str(VCONFKEY_LANGSET);

   if (lang)
   {
      setenv("LANG", lang, 1);
      setenv("LC_MESSAGES", lang, 1);
      char *r = setlocale(LC_ALL, "");
      if (r == NULL)
         setlocale(LC_ALL, lang);
      free(lang);
   }

   region = vconf_get_str(VCONFKEY_REGIONFORMAT);
   if (region)
   {
      setenv("LC_CTYPE", region, 1);
      setenv("LC_NUMERIC", region, 1);
      setenv("LC_TIME", region, 1);
      setenv("LC_COLLATE", region, 1);
      setenv("LC_MONETARY", region, 1);
      setenv("LC_PAPER", region, 1);
      setenv("LC_NAME", region, 1);
      setenv("LC_ADDRESS", region, 1);
      setenv("LC_TELEPHONE", region, 1);
      setenv("LC_MEASUREMENT", region, 1);
      setenv("LC_IDENTIFICATION", region, 1);
      free(region);
   }

   if (!bindtextdomain(domain, dir))
      return EINA_FALSE;

   if (!textdomain(domain))
      return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_key_down (void *data, int type, void *event)
{
    Ecore_Event_Key *ev = event;

   if (!ev) return ECORE_CALLBACK_PASS_ON;

    if(strcmp(ev->keyname, KEY_HOME) && strcmp(ev->keyname, KEY_BACK))
        return ECORE_CALLBACK_DONE;

    elm_exit();

    return ECORE_CALLBACK_DONE;
}

static void
_win_del(void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

static int
_popup_add(ExtnDialogData *d_data)
{
   if (!strcmp (d_data->dialog_type, "HDMI-CONVERGENCE"))
      return extndialog_hdmi_popup_add (d_data);
   else
      return extndialog_default_popup_add (d_data);
}

static void
_parse_args(int argc, char **argv, ExtnDialogData *d_data)
{
   const char *param;
   int i;

   _STRING_INIT(2, param);
   snprintf (d_data->dialog_type, DIALOG_STR_LEN, "%s", param);

   _STRING_INIT(3, param);
   snprintf (d_data->dialog_title, DIALOG_STR_LEN, "%s", param);

   for (i = 4; i <= argc && (i - 4) < DIALOG_ARGS_NUM; i++)
   {
      _STRING_INIT(i, param);
      snprintf (d_data->dialog_args[i-4], DIALOG_STR_LEN, "%s", param);
   }
}

char*
extndialog_gettext(const char *s)
{
   if (s == NULL) return "NULL";

   char *p = dgettext (GETTEXT_PACKAGE, s);

#ifdef _ENV_MOBILE_
   if (p && !strcmp(s, p))
      p = dgettext ("sys_string", s);
#endif
#ifdef _ENV_WEARABLE_
   if (!p) return "NULL";
#endif

   return p;
}

static ExtnDialogData g_data;

int
elm_main(int argc, char **argv)
{
   const char *profile = "mobile";
   Ecore_X_Window xwin;
   int rots[4] = { 0, 90, 180, 270 };
   ExtnDialogData *d_data = &g_data;

   _update_locale (GETTEXT_PACKAGE, "/usr/share/locale");

   memset (d_data, 0, sizeof (ExtnDialogData));
   _parse_args (argc, argv, d_data);

   d_data->dialog_win = elm_win_add(NULL, d_data->dialog_type, ELM_WIN_NOTIFICATION);
   if (!d_data->dialog_win) return -1;

   elm_win_title_set(d_data->dialog_win, d_data->dialog_title);
   evas_object_smart_callback_add(d_data->dialog_win, "delete,request", _win_del, NULL);
   elm_win_alpha_set(d_data->dialog_win, EINA_TRUE);
   elm_win_borderless_set(d_data->dialog_win, EINA_TRUE);
   elm_win_profiles_set(d_data->dialog_win, &profile, 1);

   if (_popup_add(d_data))
     {
        evas_object_del (d_data->dialog_win);
        return -1;
     }

   evas_object_show(d_data->dialog_win);

   ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_down, d_data);
   xwin = elm_win_xwindow_get(d_data->dialog_win);
   utilx_grab_key(ecore_x_display_get(), xwin, KEY_HOME, SHARED_GRAB);
   utilx_grab_key(ecore_x_display_get(), xwin, KEY_BACK, SHARED_GRAB);

   ecore_x_icccm_name_class_set(xwin, WIN_PROP_NAME, WIN_PROP_NAME);
   if (elm_win_wm_rotation_supported_get(d_data->dialog_win))
      elm_win_wm_rotation_available_rotations_set(d_data->dialog_win, (const int*)&rots, 4);

   elm_run();

   elm_shutdown();
   return 0;
}
ELM_MAIN()
