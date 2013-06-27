#ifndef __TSP_MAIN_VIEW_H__
#define __TSP_MAIN_VIEW_H__

#include "e_mod_devmode.h"

typedef struct
{
   int          device;
   unsigned int timestamp;

   int          x;
   int          y;

   double       pressure;
} tsp_mouse_info_s;

void tsp_main_view_create(TouchInfo *t_info);
void tsp_main_view_destroy(TouchInfo *t_info);
void tsp_main_view_mouse_down(TouchInfo *t_info, tsp_mouse_info_s *info);
void tsp_main_view_mouse_up(TouchInfo *t_info, tsp_mouse_info_s *info);
void tsp_main_view_mouse_move(TouchInfo *t_info, tsp_mouse_info_s *info);

#endif /* __TSP_MAIN_VIEW_H__ */

