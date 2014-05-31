#include "e_mod_devmode.h"
#include "e_mod_devmode_main.h"
#include "e_mod_devmode_trace_widget.h"
#include "e_mod_devmode_cross_widget.h"

#define MOVING_NOTI_MIN (10)

static int g_input_max = 0;
static char *g_bg_list_text[COLOR_SET_MAX] = {
   "White",
   "Black",
   "Blue",
   "One pixel dot",
};

#if 0
static void
__tsp_main_view_back_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

static void
__tsp_main_view_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
   TouchInfo *t_info = data;
   TSP_CHECK(t_info);

   tsp_evas_object_del(t_info->popup);
}

static char *
__tsp_main_view_popup_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
   int index = (int)data;

   if (index >= COLOR_SET_MAX)
     {
        TSP_ERROR("Invaid index.. [%d]", index);
        return NULL;
     }

   return g_strdup(g_bg_list_text[index]);
}

void
__tsp_main_view_change_color_set(TouchInfo *t_info, color_set_e color_set)
{
   TSP_CHECK(t_info);

   if (t_info->trace_widget)
     tsp_trace_widget_clear_all_trace(t_info->trace_widget);

   if (t_info->color_set == color_set)
     {
        TSP_WARN("don't need to change color set [%d]", color_set);
        return;
     }

   t_info->color_set = color_set;

   /* change bg & text of edc */
   const char *signal = NULL;
   switch (color_set)
     {
      case COLOR_SET_BLACK:
        signal = "sig_change_bg_black";
        break;

      case COLOR_SET_BLUE:
        signal = "sig_change_bg_blue";
        break;

      case COLOR_SET_DOT:
        signal = "sig_change_bg_dot";
        break;

      default:
        signal = "sig_change_bg_white";
        break;
     }
   edje_object_signal_emit(_EDJ(t_info->layout_main), signal, "c_source");

   if (t_info->trace_widget)
     tsp_trace_widget_change_color_set(t_info->trace_widget, color_set);
   if (t_info->cross_widget)
     tsp_cross_widget_change_color_set(t_info->cross_widget, color_set);
}

static void
__tsp_main_view_popup_gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
   TouchInfo *t_info = data;
   TSP_CHECK(t_info);

   Elm_Object_Item *gl_item = event_info;
   int index = (int)elm_object_item_data_get(gl_item);

   TSP_WARN("BG color set = [%d]", index);

   tsp_evas_object_del(t_info->popup);
   __tsp_main_view_change_color_set(t_info, index);
}

static void
__tsp_main_view_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   TouchInfo *t_info = data;
   TSP_CHECK(t_info);

   t_info->popup = NULL;
}

static void
__tsp_main_view_bg_selector_show(TouchInfo *t_info)
{
   TSP_CHECK(t_info);

   Evas_Object *popup = elm_popup_add(t_info->base);
   TSP_CHECK(popup);
   t_info->popup = popup;

   evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, __tsp_main_view_popup_del_cb, t_info);

   elm_object_style_set(popup, "min_menustyle");
   elm_object_part_text_set(popup, "title,text", "Select BG");
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   Evas_Object *btn = elm_button_add(popup);
   elm_object_text_set(btn, "Cancel");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_smart_callback_add(btn, "clicked", __tsp_main_view_popup_response_cb, t_info);

   Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
   itc->item_style = "1text";
   itc->func.text_get = __tsp_main_view_popup_gl_text_get;
   itc->func.content_get = NULL;
   itc->func.state_get = NULL;
   itc->func.del = NULL;

   Evas_Object *box = elm_box_t_add(popup);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   Evas_Object *genlist = elm_genlist_add(box);
   evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

   int index;
   Elm_Object_Item *item;
   for (index = 0; index < COLOR_SET_MAX; index++) {
        item = elm_genlist_item_append(genlist, itc, (void *)index, NULL, ELM_GENLIST_ITEM_NONE, __tsp_main_view_popup_gl_sel_cb, t_info);
     }

   elm_box_pack_end(box, genlist);
   evas_object_show(genlist);

   /* The height of popup being t_infojusted by application here based on app requirement */
   double scale = elm_config_scale_get();
   evas_object_size_hint_min_set(box, 600 * scale, 440 * scale);
   elm_object_content_set(popup, box);
   evas_object_show(popup);
}

static void
__tsp_main_view_setting_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   TSP_FUNC_ENTER();
   TouchInfo *t_info = data;
   TSP_CHECK(t_info);

   __tsp_main_view_bg_selector_show(t_info);
}

#endif

void
tsp_main_view_create(TouchInfo *t_info)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(t_info);

   t_info->color_set = 0;

   t_info->trace_widget = tsp_trace_widget_add(t_info->base);

   evas_object_move(t_info->trace_widget, 0, 0);
   evas_object_resize(t_info->trace_widget, t_info->width, t_info->height);
   evas_object_show(t_info->trace_widget);

   t_info->cross_widget = tsp_cross_widget_add(t_info->base);

   evas_object_move(t_info->cross_widget, 0, 0);
   evas_object_resize(t_info->cross_widget, t_info->width, t_info->height);
   evas_object_show(t_info->cross_widget);

#if 0
   Evas_Object *back_btn = tsp_widget_button_t_infod(t_info->layout_main, NULL, "exit", NULL, __tsp_main_view_back_btn_clicked_cb, NULL);
   elm_object_part_content_set(t_info->layout_main, "swallow_back_button", back_btn);
   edje_object_signal_callback_t_infod(_EDJ(t_info->layout_main), "sig_setting_btn_clicked", "*", __tsp_main_view_setting_clicked_cb, t_info);
#endif
}

void
tsp_main_view_destroy(TouchInfo *t_info)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(t_info);

   tsp_evas_object_del(t_info->trace_widget);
   tsp_evas_object_del(t_info->cross_widget);
   tsp_evas_object_del(t_info->layout_main);
}

#if 0
static void
__tsp_main_view_update_mouse_count_info(TouchInfo *t_info)
{
   TSP_CHECK(t_info);
   TSP_CHECK(t_info->layout_main);

#if 0
   char *text = g_strdup_printf("P: %d / %d", tsp_trace_widget_get_active_count(t_info->trace_widget), g_input_max);
   edje_object_part_text_set(t_info->layout_main, "info_1", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text = eina_stringshare_printf("P: %d / %d", tsp_trace_widget_get_active_count(t_info->trace_widget), g_input_max);
   if(!text) return;
   edje_object_part_text_set(t_info->layout_main, "info_1", text);
   eina_stringshare_del(text);
#endif
}

static void
__tsp_main_view_update_moving_info(TouchInfo *t_info, tsp_mouse_info_s *info)
{
   TSP_CHECK(t_info);
   TSP_CHECK(t_info->layout_main);

   static tsp_mouse_info_s prev_info = { 0, };

#if 0
   char *text = g_strdup_printf("X: %d", info->x);
   edje_object_part_text_set(t_info->layout_main, "info_2", text);
   SAFE_FREE(text);

   text = g_strdup_printf("Y: %d", info->y);
   edje_object_part_text_set(t_info->layout_main, "info_3", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text = eina_stringshare_printf("X: %d", info->x);
   if(!text) return;
   edje_object_part_text_set(t_info->layout_main, "info_2", text);
   eina_stringshare_del(text);

   Eina_Stringshare *text1 = eina_stringshare_printf("Y: %d", info->y);
   if(!text1) return;
   edje_object_part_text_set(t_info->layout_main, "info_3", text1);
   eina_stringshare_del(text1);
#endif

   unsigned int t = info->timestamp - prev_info.timestamp;
   if (prev_info.timestamp && t)
     {
        int dx = info->x - prev_info.x;
        int dy = info->y - prev_info.y;

        if (dx != 0 || dy != 0)
          {
#if 0
             text = g_strdup_printf("Xv: %.3f", (double)dx / t);
             edje_object_part_text_set(t_info->layout_main, "info_4", text);
             SAFE_FREE(text);

             text = g_strdup_printf("Yv: %.3f", (double)dy / t);
             edje_object_part_text_set(t_info->layout_main, "info_5", text);
             SAFE_FREE(text);
#else
             Eina_Stringshare *text2 = eina_stringshare_printf("Xv: %.3f", (double)dx / t);
             if(!text2) return;
             edje_object_part_text_set(t_info->layout_main, "info_4", text2);
             eina_stringshare_del(text2);

             Eina_Stringshare *text3 = eina_stringshare_printf("Xy: %.3f", (double)dy / t);
             if(!text3) return;
             edje_object_part_text_set(t_info->layout_main, "info_5", text3);
             eina_stringshare_del(text3);
#endif
          }
     }

#if 0
   text = g_strdup_printf("Prs: %.1f", info->pressure);
   edje_object_part_text_set(t_info->layout_main, "info_6", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text4 = eina_stringshare_printf("Prs: %.1f", info->pressure);
   if(!text4) return;
   edje_object_part_text_set(t_info->layout_main, "info_6", text4);
   eina_stringshare_del(text4);
#endif

   memcpy(&prev_info, info, sizeof(tsp_mouse_info_s));
}
#endif

static void
__tsp_main_view_update_mouse_down_info(TouchInfo *t_info, int device, int ex, int ey)
{
   TSP_CHECK(t_info);

#if 0
   char *text = g_strdup_printf("dX: %d", dx);
   edje_object_part_text_set(t_info->layout_main, "info_2", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text = eina_stringshare_printf("Xp: %d", ex);
   if(!text) return;
   edje_object_part_text_set(t_info->layout_main, "info_1", text);
   eina_stringshare_del(text);
#endif
   edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_show_info_1_bg", "c_source");

#if 0
   text = g_strdup_printf("dY: %d", dy);
   edje_object_part_text_set(t_info->layout_main, "info_3", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text2 = eina_stringshare_printf("Yp: %d", ey);
   if(!text2) return;
   edje_object_part_text_set(t_info->layout_main, "info_2", text2);
   eina_stringshare_del(text2);
#endif
   edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_show_info_2_bg", "c_source");
}

static void
__tsp_main_view_update_mouse_up_info(TouchInfo *t_info, int device, int sx, int sy, int ex, int ey)
{
   TSP_CHECK(t_info);

#if 0
   char *text = g_strdup_printf("dX: %d", dx);
   edje_object_part_text_set(t_info->layout_main, "info_2", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text = eina_stringshare_printf("Xr: %d", ex);
   if(!text) return;
   edje_object_part_text_set(t_info->layout_main, "info_3", text);
   eina_stringshare_del(text);
#endif
   //if (abs(ex) > MOVING_NOTI_MIN)
     edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_show_info_3_bg", "c_source");

#if 0
   text = g_strdup_printf("dY: %d", dy);
   edje_object_part_text_set(t_info->layout_main, "info_3", text);
   SAFE_FREE(text);
#else
   Eina_Stringshare *text2 = eina_stringshare_printf("Yr: %d", ey);
   if(!text2) return;
   edje_object_part_text_set(t_info->layout_main, "info_4", text2);
   eina_stringshare_del(text2);
#endif
   //if (abs(ey) > MOVING_NOTI_MIN)
     edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_show_info_4_bg", "c_source");

   Eina_Stringshare *text3 = eina_stringshare_printf("Dx: %d", ex - sx);
   if(!text3) return;
   edje_object_part_text_set(t_info->layout_main, "info_5", text3);
   eina_stringshare_del(text3);

   //if (abs(ey) > MOVING_NOTI_MIN)
     edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_show_info_5_bg", "c_source");

   Eina_Stringshare *text4 = eina_stringshare_printf("Dy: %d", ey - sy);
   if(!text4) return;
   edje_object_part_text_set(t_info->layout_main, "info_6", text4);
   eina_stringshare_del(text4);

   //if (abs(ey) > MOVING_NOTI_MIN)
     edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_show_info_6_bg", "c_source");

}

void
tsp_main_view_mouse_down(TouchInfo *t_info, tsp_mouse_info_s *info)
{
   TSP_CHECK(t_info);
   if (t_info->popup)
     return;

   edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_hide_info_1_bg", "c_source");
   edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_hide_info_2_bg", "c_source");

   int active = 0;
   if (t_info->trace_widget)
     {
        active = tsp_trace_widget_get_active_count(t_info->trace_widget);
        if ( active <= 0)
          {
             tsp_trace_widget_clear_all_trace(t_info->trace_widget);
             g_input_max = 0;
          }

        tsp_trace_widget_start_trace(t_info->trace_widget, info->device, info->x, info->y);
     }

   if (t_info->cross_widget)
     tsp_cross_widget_append_cross(t_info->cross_widget, info->device, info->x, info->y);

   active++;
   if (active > g_input_max)
     g_input_max++;

#if 0
   __tsp_main_view_update_mouse_count_info(t_info);

   __tsp_main_view_update_moving_info(t_info, info);
#else
     __tsp_main_view_update_mouse_down_info(t_info, info->device, info->x, info->y);
#endif
}

void
tsp_main_view_mouse_up(TouchInfo *t_info, tsp_mouse_info_s *info)
{
   TSP_CHECK(t_info);
   if (t_info->popup)
     return;

   int sx = 0;
   int sy = 0;
   int active = 0;

   edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_hide_info_3_bg", "c_source");
   edje_object_signal_emit(_EDJ(t_info->layout_main), "sig_hide_info_4_bg", "c_source");

   if (t_info->trace_widget)
     {
        tsp_trace_widget_get_start_point(t_info->trace_widget, info->device, &sx, &sy);
        //tsp_trace_widget_append_point(t_info->trace_widget, info->device, info->x, info->y);
        tsp_trace_widget_end_trace(t_info->trace_widget, info->device);

        active = tsp_trace_widget_get_active_count(t_info->trace_widget);
     }

   if (t_info->cross_widget)
     tsp_cross_widget_remove_cross(t_info->cross_widget, info->device);
#if 0
   __tsp_main_view_update_mouse_count_info(t_info);
#endif

   if (active == 0)
     __tsp_main_view_update_mouse_up_info(t_info, info->device, sx, sy, info->x, info->y);
}

void
tsp_main_view_mouse_move(TouchInfo *t_info, tsp_mouse_info_s *info)
{
   TSP_CHECK(t_info);
   if (t_info->popup)
     return;

   if (t_info->trace_widget)
     tsp_trace_widget_append_point(t_info->trace_widget, info->device, info->x, info->y);

   if (t_info->cross_widget)
     tsp_cross_widget_move_cross(t_info->cross_widget, info->device, info->x, info->y);

   //__tsp_main_view_update_moving_info(t_info, info);
}

