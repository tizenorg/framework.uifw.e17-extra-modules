#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_CANVAS_H
#define E_MOD_MOVE_CANVAS_H

typedef struct _E_Move_Canvas E_Move_Canvas;

struct _E_Move_Canvas
{
   E_Move           *move;
   E_Zone           *zone;
   Evas             *evas;
   int               x, y, w, h; // geometry
   int               num;
};

EINTERN E_Move_Canvas *e_mod_move_canvas_add(E_Move *m, E_Zone *zone);
EINTERN void           e_mod_move_canvas_del(E_Move_Canvas *canvas);

#endif
#endif
