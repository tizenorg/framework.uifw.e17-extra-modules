#include "e_mod_comp_animation.h"

void on_animate_done(void *data)
{
   E_Comp_Win *cw = data;

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Animate done -> win:0x%08x\n",
     cw->win);
   if (cw->transfer != NULL)
     {
        L(LT_EFFECT,
          "[COMP] w:0x%08x force win to delete transfer animators. bd:%s\n",
          cw->win, cw->bd ? "O" : "X");
        evas_object_map_enable_set(cw->transfer->obj, EINA_FALSE);
        evas_object_move(cw->transfer->obj, 0,0);

        if(cw->transfer->animator) ecore_animator_del(cw->transfer->animator);
        cw->transfer->animator = NULL;
        cw->c->transfer_list = eina_list_remove(cw->c->transfer_list, cw->transfer);
        free(cw->transfer);
        cw->transfer = NULL;
     }
   _e_mod_comp_done_defer(cw);
}

Eina_Bool
on_animate_rotate_top(void *data)
{
   E_Comp_Transfer *transfer;
   double elapsed_time;

   E_Comp_Win *cw = data;
   if (!(transfer = cw->transfer)) return ECORE_CALLBACK_CANCEL;
   elapsed_time = ecore_loop_time_get() - transfer->begin_time ;
   if (elapsed_time > transfer->duration) elapsed_time = transfer->duration;

   float frame = elapsed_time / transfer->duration;
   evas_object_move(transfer->obj, transfer->from + (transfer->len * frame) , 0);

   Evas_Coord x, y, w, h;
   Evas_Map* map;
   float half_w, half_h;
   float degree;

   map = evas_map_new( 4 );
   if( map == NULL ) return ECORE_CALLBACK_CANCEL;
   evas_map_smooth_set( map, EINA_TRUE );
   evas_map_util_points_populate_from_object_full( map, transfer->obj, 0 );
   evas_object_geometry_get( transfer->obj, &x, &y, &w, &h );
   half_w = (float) w * 0.5;
   half_h = (float) h * 0.5;
   degree = ROTATE_ANGLE_BEGIN + (ROTATE_ANGLE_TOP * frame);
   evas_map_util_3d_rotate( map, 0, degree, 0, x + half_w, y + half_h, 0 );
   evas_map_util_3d_perspective( map, x + half_w, y + half_h, (-500) + 500 * frame, 1000 );
   evas_object_map_enable_set( transfer->obj, EINA_TRUE );
   evas_object_map_set( transfer->obj, map );
   evas_map_free( map );

   if (elapsed_time == transfer->duration)
     {
        if(cw->animating == 1)
          {
             cw->animating = 0;
          }
        on_animate_done(cw);
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}


Eina_Bool
on_animate_rotate_left(void *data)
{
   E_Comp_Transfer *transfer;
   double elapsed_time;

   E_Comp_Win *cw = data;
   if (!(transfer = cw->transfer)) return ECORE_CALLBACK_CANCEL;
   elapsed_time = ecore_loop_time_get() - transfer->begin_time ;
   if (elapsed_time > transfer->duration) elapsed_time = transfer->duration;

   float frame = elapsed_time / transfer->duration;
   evas_object_move(transfer->obj, transfer->from , 0);

   Evas_Coord x, y, w, h;
   Evas_Map* map;
   float half_w, half_h;
   float degree;

   map = evas_map_new( 4 );
   if( map == NULL ) return ECORE_CALLBACK_CANCEL;
   evas_map_smooth_set( map, EINA_TRUE );
   evas_map_util_points_populate_from_object_full( map, transfer->obj, 0 );
   evas_object_geometry_get( transfer->obj, &x, &y, &w, &h );
   half_w = (float) w * 0.5;
   half_h = (float) h * 0.5;
   degree = ROTATE_ANGLE_BEGIN + (ROTATE_ANGLE_LEFT * frame);
   evas_map_util_3d_rotate( map, 0, degree, 0, x + half_w, y + half_h, 0 );
   evas_map_util_3d_perspective( map, x + half_w, y + half_h, -500, 1000 );
   evas_object_map_enable_set( transfer->obj, EINA_TRUE );
   evas_object_map_set( transfer->obj, map );
   evas_map_free( map );

   if (elapsed_time == transfer->duration)
     {
       if(cw->animating == 1)
          {
             cw->animating = 0;
          }
        on_animate_done(cw);
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
on_animate_translate(void *data)
{
   E_Comp_Transfer *transfer;
   double elapsed_time;

   E_Comp_Win *cw = data;
   if (!(transfer = cw->transfer)) return ECORE_CALLBACK_CANCEL;
   elapsed_time = ecore_loop_time_get() - transfer->begin_time ;
   if (elapsed_time > transfer->duration) elapsed_time = transfer->duration;

   float frame = elapsed_time / transfer->duration;
   float translate_x = transfer->from + (transfer->len * frame);
   evas_object_move(transfer->obj, translate_x , 0);

   Evas_Coord x, y, w, h;
   Evas_Map* map;
   float half_w, half_h;
   float degree;

   map = evas_map_new( 4 );
   if( map == NULL ) return ECORE_CALLBACK_CANCEL;
   evas_map_smooth_set( map, EINA_TRUE );
   evas_map_util_points_populate_from_object_full( map, transfer->obj, 0 );
   evas_object_geometry_get( transfer->obj, &x, &y, &w, &h );
   half_w = (float) w * 0.5;
   half_h = (float) h * 0.5;
   degree  = 70.0f;
   evas_map_util_3d_rotate( map, 0, degree, 0, x + half_w, y + half_h, 0 );
   evas_map_util_3d_perspective( map, x + half_w, y + half_h, -500, 1000 );
   evas_object_map_enable_set( transfer->obj, EINA_TRUE );
   evas_object_map_set( transfer->obj, map );
   evas_map_free( map );

   if (elapsed_time == transfer->duration)
     {

        E_Comp_Transfer * rotate;
        rotate = calloc(1, sizeof(E_Comp_Transfer));
        if (!rotate) return ECORE_CALLBACK_CANCEL;
        rotate->obj = transfer->obj;
        rotate->duration = SWITCHER_DURATION_ROTATE;
        rotate->begin_time = ecore_loop_time_get();
        rotate->from = transfer->from + transfer->len;
        if(transfer->selected == EINA_TRUE)
          {
             rotate->len = -(rotate->from);
             rotate->animator = ecore_animator_add(on_animate_rotate_top, cw);
          }
        else
          {
             rotate->len = 0;
             rotate->animator = ecore_animator_add(on_animate_rotate_left, cw);
          }
        if (transfer->animator) ecore_animator_del(transfer->animator);
        transfer->animator = NULL;
        E_Comp *c = _e_mod_comp_get();
        c->transfer_list = eina_list_remove(c->transfer_list, cw->transfer);
        free(cw->transfer);
        cw->transfer = NULL;

        cw->transfer = rotate;
        c->transfer_list = eina_list_append(c->transfer_list, rotate);

        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}
