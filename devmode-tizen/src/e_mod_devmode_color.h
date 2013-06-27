#ifndef __TSP_COLOR_H__
#define __TSP_COLOR_H__

#include "e_mod_devmode.h"

typedef struct
{
   unsigned char r;
   unsigned char g;
   unsigned char b;
   unsigned char a;
} color_t;

void tsp_color_get_trace_color(color_set_e color_set, color_t *normal, color_t *last);
void tsp_color_get_cross_color(color_set_e color_set, color_t *color);

#endif /* __TSP_COLOR_H__ */

