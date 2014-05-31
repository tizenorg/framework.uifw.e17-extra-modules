#include "e_mod_devmode.h"
#include "e_mod_devmode_color.h"

void
tsp_color_get_trace_color(color_set_e color_set, color_t *normal, color_t *last)
{
   TSP_CHECK(normal);
   TSP_CHECK(last);

   switch (color_set)
     {
      case COLOR_SET_BLACK:
      case COLOR_SET_BLUE:
        /* WHITE */
        normal->r = 0xff;
        normal->g = 0xff;
        normal->b = 0xff;
        break;

      default:
        /* BLUE */
        normal->r = 0;
        normal->g = 96;
        normal->b = 0xff;
     }
   normal->a = 0xff;

   /* end = RED */
   last->r = 0xff;
   last->g = 0;
   last->b = 0;
   last->a = 0xff;
}

void
tsp_color_get_cross_color(color_set_e color_set, color_t *color)
{
   TSP_CHECK(color);

   switch (color_set)
     {
      case COLOR_SET_BLACK:
      case COLOR_SET_BLUE:
        /* Yellow */
        color->r = 0xff;
        color->g = 0xe4;
        color->b = 0x0;
        break;

      default:
        /* BLUE */
        color->r = 0;
        color->g = 0;
        color->b = 0xff;
     }
   color->a = 0xff;
}

