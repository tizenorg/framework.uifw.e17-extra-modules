#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp.h"

/* local subsystem functions */
static void _e_mod_comp_animation_done(void *data);

/* local subsystem globals */
static Eina_List *transfers = NULL;

/* externally accessible functions */
EINTERN E_Comp_Transfer *
e_mod_comp_animation_transfer_new(void)
{
   E_Comp_Transfer *tr;
   tr = E_NEW(E_Comp_Transfer, 1);
   transfers = eina_list_append(transfers, tr);
   return tr;
}

EINTERN void
e_mod_comp_animation_transfer_free(E_Comp_Transfer *tr)
{
   E_Comp_Win *cw = NULL;
   E_CHECK(tr);
   transfers = eina_list_remove(transfers, tr);
   if (tr->animator)
     ecore_animator_del(tr->animator);
   tr->animator = NULL;
   if (tr->obj)
     {
        cw = evas_object_data_get(tr->obj, "src");
        if (cw) cw->transfer = NULL;
     }
   tr->obj = NULL;
   E_FREE(tr);
}

EINTERN Eina_Bool
e_mod_comp_animation_on_rotate_top(void *data)
{
   E_Comp_Transfer *transfer;
   double elapsed_time;

   E_Comp_Win *cw = data;
   E_CHECK_RETURN(cw, 0);
   if (!(transfer = cw->transfer)) return EINA_FALSE;
   elapsed_time = ecore_loop_time_get() - transfer->begin_time ;
   if (elapsed_time > transfer->duration) elapsed_time = transfer->duration;

   float frame = elapsed_time / transfer->duration;
   evas_object_move(transfer->obj, transfer->from + (transfer->len * frame) , 0);

   Evas_Coord x, y, w, h;
   Evas_Map* map;
   float half_w, half_h;
   float degree;

   map = evas_map_new(4);
   E_CHECK_RETURN(map, 0);
   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, transfer->obj, 0);
   evas_object_geometry_get(transfer->obj, &x, &y, &w, &h);
   half_w = (float) w * 0.5;
   half_h = (float) h * 0.5;
   degree = ROTATE_ANGLE_BEGIN + (ROTATE_ANGLE_TOP * frame);
   evas_map_util_3d_rotate(map, 0, degree, 0, x + half_w, y + half_h, 0);
   evas_map_util_3d_perspective(map, x + half_w, y + half_h, (-500) + 500 * frame, 1000);
   evas_object_map_enable_set(transfer->obj, EINA_TRUE);
   evas_object_map_set(transfer->obj, map);
   evas_map_free(map);

   if (elapsed_time == transfer->duration)
     {
        if (cw->animating == 1)
          {
             cw->animating = 0;
          }
        _e_mod_comp_animation_done(cw);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_animation_on_rotate_left(void *data)
{
   E_Comp_Transfer *transfer;
   double elapsed_time;

   E_Comp_Win *cw = data;
   if (!cw) return EINA_FALSE;
   if (!(transfer = cw->transfer)) return EINA_FALSE;
   elapsed_time = ecore_loop_time_get() - transfer->begin_time ;
   if (elapsed_time > transfer->duration) elapsed_time = transfer->duration;

   float frame = elapsed_time / transfer->duration;
   evas_object_move(transfer->obj, transfer->from , 0);

   Evas_Coord x, y, w, h;
   Evas_Map* map;
   float half_w, half_h;
   float degree;

   map = evas_map_new(4);
   E_CHECK_RETURN(map, 0);
   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, transfer->obj, 0);
   evas_object_geometry_get(transfer->obj, &x, &y, &w, &h);
   half_w = (float) w * 0.5;
   half_h = (float) h * 0.5;
   degree = ROTATE_ANGLE_BEGIN + (ROTATE_ANGLE_LEFT * frame);
   evas_map_util_3d_rotate(map, 0, degree, 0, x + half_w, y + half_h, 0);
   evas_map_util_3d_perspective(map, x + half_w, y + half_h, -500, 1000);
   evas_object_map_enable_set(transfer->obj, EINA_TRUE);
   evas_object_map_set(transfer->obj, map);
   evas_map_free(map);

   if (elapsed_time == transfer->duration)
     {
        cw->animating = 0;
        _e_mod_comp_animation_done(cw);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_animation_on_translate(void *data)
{
   E_Comp_Win *cw;
   E_Comp_Transfer *transfer, *rotate;
   Evas_Coord x, y, w, h;
   Evas_Map* map;
   float half_w, half_h;
   float degree, frame, translate_x;
   double elapsed_time;

   cw = (E_Comp_Win *)data;
   E_CHECK_RETURN(cw, 0);

   transfer = cw->transfer;
   E_CHECK_RETURN(transfer, 0);

   elapsed_time = ecore_loop_time_get() - transfer->begin_time;
   if (elapsed_time > transfer->duration) elapsed_time = transfer->duration;

   frame = elapsed_time / transfer->duration;
   translate_x = transfer->from + (transfer->len * frame);
   evas_object_move(transfer->obj, translate_x , 0);

   map = evas_map_new(4);
   E_CHECK_RETURN(map, 0);

   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, transfer->obj, 0);
   evas_object_geometry_get(transfer->obj, &x, &y, &w, &h);
   half_w = (float) w * 0.5;
   half_h = (float) h * 0.5;
   degree = 70.0f;
   evas_map_util_3d_rotate(map, 0, degree, 0, x + half_w, y + half_h, 0);
   evas_map_util_3d_perspective(map, x + half_w, y + half_h, -500, 1000);
   evas_object_map_enable_set(transfer->obj, EINA_TRUE);
   evas_object_map_set(transfer->obj, map);
   evas_map_free(map);

   if (elapsed_time != transfer->duration) return EINA_TRUE;

   rotate = e_mod_comp_animation_transfer_new();
   E_CHECK_RETURN(rotate, 0);

   rotate->obj = transfer->obj;
   rotate->duration = SWITCHER_DURATION_ROTATE;
   rotate->begin_time = ecore_loop_time_get();
   rotate->from = transfer->from + transfer->len;
   if (transfer->selected)
     {
        rotate->len = -(rotate->from);
        rotate->animator = ecore_animator_add(e_mod_comp_animation_on_rotate_top, cw);
     }
   else
     {
        rotate->len = 0;
        rotate->animator = ecore_animator_add(e_mod_comp_animation_on_rotate_left, cw);
     }
   if (transfer->animator) ecore_animator_del(transfer->animator);
   transfer->animator = NULL;

   e_mod_comp_animation_transfer_free(cw->transfer);
   cw->transfer = rotate;
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_animation_transfer_list_clear(void)
{
   E_Comp_Transfer *tr;
   E_CHECK_RETURN(transfers, 0);
   EINA_LIST_FREE(transfers, tr)
     {
        if (!tr) continue;
        e_mod_comp_animation_transfer_free(tr);
     }
   transfers = NULL;
   return EINA_TRUE;
}

/* local subsystem functions */
static void
_e_mod_comp_animation_done(void *data)
{
   E_Comp_Win *cw = (E_Comp_Win *)data;
   E_CHECK(cw);

   L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s cw->transfer:%p\n",
     "EFF", e_mod_comp_util_client_xid_get(cw),
     "ANIMATE_DONE", cw->transfer);

   E_CHECK_GOTO(cw->transfer, postjob);

   evas_object_map_enable_set(cw->transfer->obj, EINA_FALSE);
   evas_object_move(cw->transfer->obj, 0,0);

   e_mod_comp_animation_transfer_free(cw->transfer);
   cw->transfer = NULL;

postjob:
   e_mod_comp_done_defer(cw);
}
