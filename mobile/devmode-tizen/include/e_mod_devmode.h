
#ifndef __TSP_TRACE_TEST_H__
#define __TSP_TRACE_TEST_H__

#include <e.h>
#include <Elementary.h>
#include "e_mod_devmode_log.h"
#include "e_mod_devmode_define.h"

typedef enum
{
   COLOR_SET_WHITE = 0,
   COLOR_SET_BLACK,
   COLOR_SET_BLUE,
   COLOR_SET_DOT,
   COLOR_SET_MAX
} color_set_e;

typedef struct
{
   void        *module;
   Eina_List   *cons;
   Eina_List   *zones;
   Evas        *canvas;
   Evas_Object *base;
   Evas_Object *layout_main;

   Evas_Object *cross_widget;
   Evas_Object *trace_widget;

   Evas_Object *popup;
   int          x;
   int          y;
   int          width;
   int          height;
   color_set_e  color_set;
} TouchInfo;

extern E_Module *devmode_mod;

EAPI extern E_Module_Api e_modapi;

#endif /* __TSP_TRACE_TEST_H__ */

