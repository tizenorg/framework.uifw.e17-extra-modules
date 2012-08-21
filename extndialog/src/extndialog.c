#include <Elementary.h>

static void
win_del(void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

static void
close_cb(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_del(data);
   win_del(NULL, NULL, NULL);
}

#define _STRING_INIT(i, s) {                           \
   if (argc >= i) s = eina_stringshare_add(argv[i-1]); \
   else s = eina_stringshare_add("Not specified");     \
}

int
elm_main(int argc, char **argv)
{
   const char *name, *clas, *title, *contents;
   Evas_Object *win, *pop, *bt;

   _STRING_INIT(2, name);
   _STRING_INIT(3, clas);
   _STRING_INIT(4, title);
   _STRING_INIT(5, contents);

   win = elm_win_add(NULL, name, ELM_WIN_NOTIFICATION);
   if (!win) return 0;

   elm_win_title_set(win, clas);
   evas_object_smart_callback_add(win, "delete,request", win_del, NULL);
   elm_win_alpha_set(win, EINA_TRUE);

   pop = elm_popup_add(win);
   elm_object_part_text_set(pop, "title,text", title);
   elm_object_text_set(pop, contents);

   bt = elm_button_add(pop);
   elm_object_text_set(bt, "Close");
   elm_object_part_content_set(pop, "button1", bt);
   evas_object_smart_callback_add(bt, "clicked", close_cb, win);

   evas_object_show(bt);
   evas_object_show(pop);
   evas_object_show(win);

   elm_run();

finish:
   elm_shutdown();
   return 0;
}
ELM_MAIN()
