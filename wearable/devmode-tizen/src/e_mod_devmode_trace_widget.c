#include "e_mod_devmode_trace_widget.h"
#include "e_mod_devmode_log.h"
#include "e_mod_devmode_log.h"
#include "e_mod_devmode_color.h"

typedef struct
{
   int          id;

   Evas_Point   start_point;
   Eina_List   *line_list;
   Eina_List   *point_list;
   Evas_Object *touch_feedback;
} trace_info_s;

typedef struct
{
   Evas_Object *obj;
   struct
   {
      Evas_Coord x;
      Evas_Coord y;
      Evas_Coord w;
      Evas_Coord h;
   } geometry;

   color_t      normal_color;
   color_t      last_color;

   Eina_List   *trace_list;
} trace_widget_smart_data_s;

static void
__tsp_trace_widget_smart_add(Evas_Object *obj)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = calloc(1, sizeof(trace_widget_smart_data_s));
   TSP_CHECK(sd);
   evas_object_smart_data_set(obj, sd);

   sd->obj = obj;

   tsp_color_get_trace_color(0, &sd->normal_color, &sd->last_color);
}

static void
__tsp_trace_widget_smart_del(Evas_Object *obj)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   tsp_trace_widget_clear_all_trace(obj);

   free(sd);
}

static void
__tsp_trace_widget_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   sd->geometry.x = x;
   sd->geometry.y = y;

   evas_object_smart_changed(obj);
}

static void
__tsp_trace_widget_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   sd->geometry.w = w;
   sd->geometry.h = h;

   evas_object_smart_changed(obj);
}

static void
__tsp_trace_widget_smart_show(Evas_Object *obj)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->trace_list);

   Eina_List *l = NULL;
   trace_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->trace_list, l, data) {
        if (data)
          {
             Eina_List *ll = NULL;
             Evas_Object *line = NULL;
             EINA_LIST_FOREACH (data->line_list, ll, line) {
                  if (line)
                    evas_object_show(line);
               }
#if 1
             Eina_List *pl = NULL;
             Evas_Object *point = NULL;
             EINA_LIST_FOREACH (data->point_list, pl, point) {
                  if (point)
                    evas_object_show(point);
               }
#endif
          }
     }
}

static void
__tsp_trace_widget_smart_hide(Evas_Object *obj)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->trace_list);

   Eina_List *l = NULL;
   trace_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->trace_list, l, data) {
        if (data)
          {
             Eina_List *ll = NULL;
             Evas_Object *line = NULL;
             EINA_LIST_FOREACH (data->line_list, ll, line) {
                  if (line)
                    evas_object_hide(line);
               }
#if 1
             Eina_List *pl = NULL;
             Evas_Object *point = NULL;
             EINA_LIST_FOREACH (data->point_list, pl, point) {
                  if (point)
                    evas_object_hide(point);
               }
#endif
          }
     }
}

static void
__tsp_trace_widget_smart_calculate(Evas_Object *obj)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
}

static trace_info_s *
_tsp_trace_widget_find_trace(Eina_List *list, int idx)
{
   TSP_CHECK_NULL(list);

   Eina_List *l = NULL;
   trace_info_s *data = NULL;
   EINA_LIST_FOREACH (list, l, data)
     {
        if (data->id == idx)
          return data;
     }

   return NULL;
}

Evas_Object *
tsp_trace_widget_add(Evas_Object *parent)
{
   TSP_CHECK_NULL(parent);
   Evas_Object *obj = NULL;
   static Evas_Smart_Class sc;
   static Evas_Smart *smart = NULL;
   if (!smart)
     {
        memset(&sc, 0x0, sizeof(Evas_Smart_Class));
        sc.name = "tsp_trace_widget";
        sc.version = EVAS_SMART_CLASS_VERSION;
        sc.add = __tsp_trace_widget_smart_add;
        sc.del = __tsp_trace_widget_smart_del;
        sc.move = __tsp_trace_widget_smart_move;
        sc.resize = __tsp_trace_widget_smart_resize;
        sc.show = __tsp_trace_widget_smart_show;
        sc.hide = __tsp_trace_widget_smart_hide;
        sc.calculate = __tsp_trace_widget_smart_calculate;
        if (!(smart = evas_smart_class_new(&sc))) return NULL;
     }

   obj = evas_object_smart_add(evas_object_evas_get(parent), smart);

   return obj;
}

void
tsp_trace_widget_start_trace(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y)
{
   Evas_Load_Error err;

   char icon_path[PATH_MAX];
   sprintf(icon_path, "%s/%s", e_module_dir_get(devmode_mod), TOUCH_ICON);

   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   if (_tsp_trace_widget_find_trace(sd->trace_list, idx))
     {
        TSP_WARN("[devmode-tizen] already exist trace [%d]", idx);
        return;
     }

   trace_info_s *trace = calloc(1, sizeof(trace_info_s));
   TSP_ASSERT(trace);

   TSP_DEBUG("[devmode-tizen] append at [%d, %d]", x, y);

   trace->id = idx;
   trace->start_point.x = x;
   trace->start_point.y = y;

   Evas *e = evas_object_evas_get(obj);
   TSP_ASSERT(e);

#if 1
   if (trace->touch_feedback == NULL)
     {
        trace->touch_feedback = evas_object_image_add(e);
        evas_object_image_file_set(trace->touch_feedback, icon_path, NULL);
        err = evas_object_image_load_error_get(trace->touch_feedback);
        if (err != EVAS_LOAD_ERROR_NONE)
          TSP_ERROR("[devmode-tizen] Failed to load image [%s]", icon_path);
     }
   else
     TSP_ERROR("[devmode-tizen] trace->touch feedback should be NULL here");
#endif

   sd->trace_list = eina_list_append(sd->trace_list, trace);
}

void
tsp_trace_widget_append_point(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   trace_info_s *trace = _tsp_trace_widget_find_trace(sd->trace_list, idx);
   TSP_CHECK(trace);

   Evas_Point start_point = trace->start_point;

   Eina_List *last_list = eina_list_last(trace->line_list);
   if (last_list)
     {
        Evas_Object *line = eina_list_data_get(last_list);
        if (line)
          {
             evas_object_line_xy_get(line, NULL, NULL, &start_point.x, &start_point.y);
          }
     }
#if 0
   if (start_point.x == x && start_point.y == y)
     {
        return;
     }
#endif

   Evas_Object *line = evas_object_line_add(evas_object_evas_get(obj));
   evas_object_render_op_set(line, EVAS_RENDER_COPY);
   evas_object_line_xy_set(line, start_point.x, start_point.y, x, y);
   evas_object_color_set(line, sd->normal_color.r, sd->normal_color.g, sd->normal_color.b, sd->normal_color.a);
   evas_object_smart_member_add(line, obj);
   trace->line_list = eina_list_append(trace->line_list, line);
   evas_object_show(line);

#if 1
   Evas_Object *point = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_render_op_set(point, EVAS_RENDER_COPY);
   evas_object_resize(point, 4, 4);
   evas_object_move(point, x - 2, y - 2);
   evas_object_color_set(point, 0xFF, 0xFF, 0xFF, 0xFF);
   evas_object_smart_member_add(point, obj);
   trace->point_list = eina_list_append(trace->point_list, point);
   evas_object_show(point);
#endif
#if 1
   if (trace->touch_feedback)
     {
        evas_object_move(trace->touch_feedback, x - 23, y - 23);
        evas_object_image_fill_set(trace->touch_feedback, 0, 0, 46, 46);
        evas_object_resize(trace->touch_feedback, 46, 46);
        evas_object_show(trace->touch_feedback);
     }
#endif
}

static void
__tsp_trace_widget_destroy_trace(trace_info_s *trace)
{
   TSP_CHECK(trace);

   Eina_List *l = NULL;
   Evas_Object *data = NULL;
   EINA_LIST_FOREACH (trace->line_list, l, data)
     {
        tsp_evas_object_del(data);
     }
   eina_list_free(trace->line_list);

#if 1
   EINA_LIST_FOREACH (trace->point_list, l, data)
     {
        tsp_evas_object_del(data);
     }
   eina_list_free(trace->point_list);
#endif

   free(trace);
}

void
tsp_trace_widget_end_trace(Evas_Object *obj, int idx)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   trace_info_s *trace = _tsp_trace_widget_find_trace(sd->trace_list, idx);
   TSP_CHECK(trace);

   Eina_List *last_node = eina_list_last(trace->line_list);
   if (last_node)
     {
        Evas_Object *last_line = eina_list_data_get(last_node);
        if (last_line)
          {
             evas_object_color_set(last_line, sd->last_color.r, sd->last_color.g, sd->last_color.b, sd->last_color.a);
          }
     }

#if 1
   if (trace->touch_feedback)
     {
        evas_object_del(trace->touch_feedback);
        trace->touch_feedback = NULL;
     }

#endif

   trace->id = -1;      // can not draw anymore
}

void
tsp_trace_widget_clear_all_trace(Evas_Object *obj)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->trace_list);

   Eina_List *l = NULL;
   trace_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->trace_list, l, data)
     {
        __tsp_trace_widget_destroy_trace(data);
     }

   eina_list_free(sd->trace_list);
   sd->trace_list = NULL;
}

int
tsp_trace_widget_get_total_count(Evas_Object *obj)
{
   TSP_CHECK_VAL(obj, 0);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK_VAL(sd, 0);
   TSP_CHECK_VAL(sd->trace_list, 0);

   return eina_list_count(sd->trace_list);
}

int
tsp_trace_widget_get_active_count(Evas_Object *obj)
{
   TSP_CHECK_VAL(obj, 0);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK_VAL(sd, 0);
   TSP_CHECK_VAL(sd->trace_list, 0);

   int count = 0;

   Eina_List *l = NULL;
   trace_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->trace_list, l, data)
     {
        if (data->id >= 0)
          count++;
     }

   return count;
}

bool
tsp_trace_widget_get_start_point(Evas_Object *obj, int idx, int *x, int *y)
{
   TSP_CHECK_FALSE(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK_FALSE(sd);
   TSP_CHECK_FALSE(sd->trace_list);

   trace_info_s *trace = _tsp_trace_widget_find_trace(sd->trace_list, idx);
   TSP_CHECK_FALSE(trace);

   *x = trace->start_point.x;
   *y = trace->start_point.y;

   return true;
}

void
tsp_trace_widget_change_color_set(Evas_Object *obj, color_set_e color_set)
{
   TSP_CHECK(obj);
   trace_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   tsp_color_get_trace_color(color_set, &sd->normal_color, &sd->last_color);

   if (sd->trace_list)
     {
        Eina_List *l = NULL;
        trace_info_s *trace = NULL;
        EINA_LIST_FOREACH (sd->trace_list, l, trace) {
             if (trace)
               {
                  Eina_List *line_list = NULL;
                  Evas_Object *line = NULL;
                  EINA_LIST_FOREACH (trace->line_list, line_list, line) {
                       if (eina_list_last(trace->line_list) == line_list)
                         evas_object_color_set(line, sd->last_color.r, sd->last_color.g, sd->last_color.b, sd->last_color.a);
                       else
                         evas_object_color_set(line, sd->normal_color.r, sd->normal_color.g, sd->normal_color.b, sd->normal_color.a);
                    }
               }
          }
     }
}

