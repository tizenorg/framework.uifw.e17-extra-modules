#include "extndialog.h"

#define STR_SCREEN_MIRRORING "IDS_SMR_POP_YOUR_DEVICE_IS_CONNECTED_VIA_HDMI_THE_SCREEN_MIRRORING_CONNECTION_HAS_ENDED"
#define STR_SIDESYNC         "IDS_SMR_POP_YOUR_DEVICE_IS_CONNECTED_VIA_HDMI_THE_SIDESYNC_CONNECTION_HAS_ENDED"
#define STR_OK               "IDS_COM_SK_OK"

static void
close_cb (void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

int
extndialog_hdmi_popup_add (ExtnDialogData *d_data)
{
   Evas_Object *popup;
   Evas_Object *btn;
   char temp[256];

   if (!d_data) return -1;
   if (strlen(d_data->dialog_args[0]) <= 0) return -1;
   if (!d_data->dialog_win) return -1;

   if (!strcmp (d_data->dialog_args[0], "WFD"))
      snprintf (temp, sizeof(temp), "%s", _(STR_SCREEN_MIRRORING));
   else if (!strcmp (d_data->dialog_args[0], "DLNA"))
      snprintf (temp, sizeof(temp), "%s", _(STR_SCREEN_MIRRORING));
   else if (strstr (d_data->dialog_args[0], "SIDESYNC"))
      snprintf (temp, sizeof(temp), "%s", _(STR_SIDESYNC));
   else
     {
        char temp2[256];
        snprintf (temp2, sizeof(temp2), "HDMI connected. Unknown(%s) connection ended", d_data->dialog_args[0]);
        snprintf (temp, sizeof(temp), "%s", _(temp2));
     }

   popup = elm_popup_add (d_data->dialog_win);

   elm_object_style_set(popup, "transparent");
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_text_set (popup, temp);

   btn = elm_button_add (popup);
   elm_object_text_set (btn, _(STR_OK));
   elm_object_part_content_set (popup, "button1", btn);
   evas_object_smart_callback_add (btn, "clicked", close_cb, NULL);

   evas_object_show (btn);
   evas_object_show (popup);

   return 0;
}
