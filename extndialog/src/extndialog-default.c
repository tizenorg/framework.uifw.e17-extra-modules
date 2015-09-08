#include "extndialog.h"

static void
close_cb (void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

int
extndialog_default_popup_add(ExtnDialogData *d_data)
{
   Evas_Object *pop, *bt;

   if (!d_data) return -1;
   if (strlen(d_data->dialog_args[0]) <= 0) return -1;
   if (strlen(d_data->dialog_args[1]) <= 0) return -1;

   pop = elm_popup_add(d_data->dialog_win);
   elm_object_part_text_set(pop, "title,text", d_data->dialog_args[0]);
   elm_object_text_set(pop, d_data->dialog_args[1]);
   elm_popup_align_set(pop, ELM_NOTIFY_ALIGN_FILL, 1.0);

   bt = elm_button_add(pop);
   elm_object_text_set(bt, "Close");
   elm_object_part_content_set(pop, "button1", bt);
   evas_object_smart_callback_add(bt, "clicked", close_cb, NULL);

   evas_object_show(bt);
   evas_object_show(pop);

   return 0;
}
