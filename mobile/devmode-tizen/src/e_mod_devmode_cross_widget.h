#ifndef __TSP_CROSS_WIDGET_H__
#define __TSP_CROSS_WIDGET_H__

#include <Elementary.h>
#include "e_mod_devmode_color.h"

Evas_Object *tsp_cross_widget_add(Evas_Object *parent);
void         tsp_cross_widget_append_cross(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y);
void         tsp_cross_widget_move_cross(Evas_Object *obj, int idx, Evas_Coord x, Evas_Coord y);
void         tsp_cross_widget_remove_cross(Evas_Object *obj, int idx);
void         tsp_cross_widget_change_color_set(Evas_Object *obj, color_set_e color_set);

#endif /* __TSP_CROSS_WIDGET_H__ */

