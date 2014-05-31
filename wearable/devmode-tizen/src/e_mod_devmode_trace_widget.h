#ifndef __TSP_TRACE_WIDGET_H__
#define __TSP_TRACE_WIDGET_H__

#include <Elementary.h>
#include <stdbool.h>
#include "e_mod_devmode.h"

#ifndef TOUCH_ICON
#define TOUCH_ICON "touch_feedback.png"
#endif

Evas_Object *tsp_trace_widget_add(Evas_Object *parent);
void         tsp_trace_widget_start_trace(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y);
void         tsp_trace_widget_append_point(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y);
void         tsp_trace_widget_end_trace(Evas_Object *obj, int idx);
void         tsp_trace_widget_clear_all_trace(Evas_Object *obj);
int          tsp_trace_widget_get_total_count(Evas_Object *obj);
int          tsp_trace_widget_get_active_count(Evas_Object *obj);
bool         tsp_trace_widget_get_start_point(Evas_Object *obj, int idx, int *x, int *y);
void         tsp_trace_widget_change_color_set(Evas_Object *obj, color_set_e color_set);

#endif /* __TSP_TRACE_WIDGET_H__ */

