#include <Evas.h>
#include "e_mod_devmode_cross_widget.h"
#include "e_mod_devmode_log.h"
#include "e_mod_devmode_define.h"
#include "e_mod_devmode_color.h"

typedef struct
{
   int          id;
   Evas_Coord   x;
   Evas_Coord   y;
   Evas_Object *h_line;
   Evas_Object *v_line;
} cross_info_s;

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

   color_t      color;

   Eina_List   *cross_list;
} cross_widget_smart_data_s;

static void
__tsp_cross_widget_smart_add(Evas_Object *obj)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = calloc(1, sizeof(cross_widget_smart_data_s));
   TSP_CHECK(sd);
   evas_object_smart_data_set(obj, sd);

   sd->obj = obj;
   tsp_color_get_cross_color(0, &sd->color);
}

static void
__tsp_cross_widget_smart_del(Evas_Object *obj)
{
   TSP_FUNC_ENTER();
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   if (sd->cross_list)
     {
        Eina_List *l = NULL;
        cross_info_s *data = NULL;
        EINA_LIST_FOREACH (sd->cross_list, l, data)
          {
             tsp_evas_object_del(data->h_line);
             tsp_evas_object_del(data->v_line);
             free(data);
          }
        eina_list_free(sd->cross_list);
        sd->cross_list = NULL;
     }

   free(sd);
}

static void
__tsp_cross_widget_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   sd->geometry.x = x;
   sd->geometry.y = y;

   evas_object_smart_changed(obj);
}

static void
__tsp_cross_widget_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   sd->geometry.w = w;
   sd->geometry.h = h;

   evas_object_smart_changed(obj);
}

static void
__tsp_cross_widget_smart_show(Evas_Object *obj)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->cross_list);

   Eina_List *l = NULL;
   cross_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->cross_list, l, data)
     {
        if (data->h_line)
          evas_object_show(data->h_line);
        if (data->v_line)
          evas_object_show(data->v_line);
     }
}

static void
__tsp_cross_widget_smart_hide(Evas_Object *obj)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->cross_list);

   Eina_List *l = NULL;
   cross_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->cross_list, l, data)
     {
        if (data->h_line)
          evas_object_hide(data->h_line);
        if (data->v_line)
          evas_object_hide(data->v_line);
     }
}

static void
__tsp_cross_widget_smart_calculate(Evas_Object *obj)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->cross_list);

   Eina_List *l = NULL;
   cross_info_s *data = NULL;
   EINA_LIST_FOREACH (sd->cross_list, l, data)
     {
        tsp_cross_widget_move_cross(obj, data->id, data->x, data->y);
     }
}

Evas_Object *
tsp_cross_widget_add(Evas_Object *parent)
{
   TSP_CHECK_NULL(parent);
   Evas_Object *obj = NULL;
   static Evas_Smart_Class sc;
   static Evas_Smart *smart = NULL;
   if (!smart)
     {
        memset(&sc, 0x0, sizeof(Evas_Smart_Class));
        sc.name = "tsp_cross_widget";
        sc.version = EVAS_SMART_CLASS_VERSION;
        sc.add = __tsp_cross_widget_smart_add;
        sc.del = __tsp_cross_widget_smart_del;
        sc.move = __tsp_cross_widget_smart_move;
        sc.resize = __tsp_cross_widget_smart_resize;
        sc.show = __tsp_cross_widget_smart_show;
        sc.hide = __tsp_cross_widget_smart_hide;
        sc.calculate = __tsp_cross_widget_smart_calculate;
        if (!(smart = evas_smart_class_new(&sc))) return NULL;
     }

   obj = evas_object_smart_add(evas_object_evas_get(parent), smart);

   return obj;
}

static cross_info_s *
_tsp_cross_widget_find_cross(Eina_List *list, int idx)
{
   TSP_CHECK_NULL(list);

   Eina_List *l = NULL;
   cross_info_s *data = NULL;
   EINA_LIST_FOREACH (list, l, data)
     {
        if (data->id == idx)
          return data;
     }

   return NULL;
}

void
tsp_cross_widget_append_cross(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   if (_tsp_cross_widget_find_cross(sd->cross_list, idx))
     {
        TSP_WARN("alret_infoy exist cross [%d]", idx);
        return;
     }

   cross_info_s *cross = calloc(1, sizeof(cross_info_s));
   TSP_ASSERT(cross);

   TSP_DEBUG("append at [%d, %d]", x, y);

   cross->id = idx;
   cross->h_line = evas_object_line_add(evas_object_evas_get(obj));
   evas_object_color_set(cross->h_line, sd->color.r, sd->color.g, sd->color.b, sd->color.a);
   evas_object_smart_member_add(cross->h_line, obj);

   cross->v_line = evas_object_line_add(evas_object_evas_get(obj));
   evas_object_color_set(cross->v_line, sd->color.r, sd->color.g, sd->color.b, sd->color.a);
   evas_object_smart_member_add(cross->v_line, obj);
   evas_object_show(cross->v_line);

   sd->cross_list = eina_list_append(sd->cross_list, cross);
   tsp_cross_widget_move_cross(obj, idx, x, y);

   evas_object_show(cross->h_line);
}

void
tsp_cross_widget_move_cross(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->cross_list);

   cross_info_s *cross = _tsp_cross_widget_find_cross(sd->cross_list, idx);
   if (!cross)
     {
        tsp_cross_widget_append_cross(obj, idx, x, y);
        return;
     }

   TSP_CHECK(cross->h_line);
   TSP_CHECK(cross->v_line);

   Evas_Coord sx = sd->geometry.x;
   Evas_Coord sy = sd->geometry.y;
   Evas_Coord ex = sd->geometry.x + sd->geometry.w;
   Evas_Coord ey = sd->geometry.y + sd->geometry.h;

   evas_object_line_xy_set(cross->h_line, sx, y, ex, y);
   evas_object_line_xy_set(cross->v_line, x, sy, x, ey);

   cross->x = x;
   cross->y = y;
}

void
tsp_cross_widget_remove_cross(Evas_Object *obj, int idx)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);
   TSP_CHECK(sd->cross_list);

   cross_info_s *cross = _tsp_cross_widget_find_cross(sd->cross_list, idx);
   TSP_CHECK(cross);

   tsp_evas_object_del(cross->v_line);
   tsp_evas_object_del(cross->h_line);

   sd->cross_list = eina_list_remove(sd->cross_list, cross);
   SAFE_FREE(cross);
}

void
tsp_cross_widget_change_color_set(Evas_Object *obj, color_set_e color_set)
{
   TSP_CHECK(obj);
   cross_widget_smart_data_s *sd = evas_object_smart_data_get(obj);
   TSP_CHECK(sd);

   tsp_color_get_cross_color(color_set, &sd->color);

   if (sd->cross_list)
     {
        Eina_List *l = NULL;
        cross_info_s *cross = NULL;
        EINA_LIST_FOREACH (sd->cross_list, l, cross) {
             if (cross)
               {
                  if (cross->h_line)
                    evas_object_color_set(cross->h_line, sd->color.r, sd->color.g, sd->color.b, sd->color.a);
                  if (cross->v_line)
                    evas_object_color_set(cross->v_line, sd->color.r, sd->color.g, sd->color.b, sd->color.a);
               }
          }
     }
}

