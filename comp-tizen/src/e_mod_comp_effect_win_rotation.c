#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_win_rotation.h"
#include <Elementary.h>

typedef struct _E_Comp_Zone_Rotation_Effect_Begin E_Comp_Zone_Rotation_Effect_Begin;
typedef struct _E_Comp_Zone_Rotation_Effect_End   E_Comp_Zone_Rotation_Effect_End;

struct _E_Comp_Effect_Zone_Rotation
{
   E_Comp_Canvas      *canvas;
   Eina_Bool           ready;
   Eina_Bool           run;
   Elm_Transit        *trans_begin;
   Elm_Transit        *trans_end;
   Elm_Transit_Effect *effect_begin;
   Elm_Transit_Effect *effect_end;
};

struct _E_Comp_Zone_Rotation_Effect_Begin
{
   Evas_Object   *o;
   E_Zone        *zone;
   double         src;
   double         target;
   Ecore_X_Image *xim;
   Evas_Object   *img;
   E_Comp_Layer  *ly;
};

struct _E_Comp_Zone_Rotation_Effect_End
{
   Evas_Object *o;
   E_Zone      *zone;
   double       src;
   double       target;
};

/* local subsystem functions */
static Eina_Bool            _angle_get(E_Comp_Win *cw, int *req, int *curr);
static Elm_Transit_Effect *_effect_zone_rot_begin_new(E_Comp_Layer *ly);
static Elm_Transit_Effect *_effect_zone_rot_end_new(Evas_Object *o, E_Zone *zone);
static void                _effect_zone_rot_begin_free(Elm_Transit_Effect *effect, Elm_Transit *transit);
static void                _effect_zone_rot_end_free(Elm_Transit_Effect *effect, Elm_Transit *transit);
static void                _effect_zone_rot_begin_op(Elm_Transit_Effect *effect, Elm_Transit *transit, double progress);
static void                _effect_zone_rot_end_op(Elm_Transit_Effect *effect, Elm_Transit *transit, double progress);

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_comp_effect_win_angle_get(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   int req_angle = -1;
   int cur_angle = -1;
   Eina_Bool res;
   Ecore_X_Window win;
   E_CHECK_RETURN(cw, 0);

   win = e_mod_comp_util_client_xid_get(cw);
   st = e_mod_comp_effect_style_get
      (cw->eff_type,
       E_COMP_EFFECT_KIND_ROTATION);

   if (st == E_COMP_EFFECT_STYLE_NONE)
     return EINA_FALSE;

   res = _angle_get(cw, &req_angle, &cur_angle);
   if (!res)
     return EINA_FALSE;

   cw->angle = cur_angle;
   cw->angle %= 360;

   return EINA_TRUE;
}

EINTERN E_Comp_Effect_Zone_Rotation *
e_mod_comp_effect_zone_rotation_new(E_Comp_Canvas *canvas)
{
   E_Comp_Effect_Zone_Rotation *zr = NULL;
   zr = E_NEW(E_Comp_Effect_Zone_Rotation, 1);
   E_CHECK_RETURN(zr, NULL);

   zr->canvas = canvas;

   return zr;
}

EINTERN void
e_mod_comp_effect_zone_rotation_free(E_Comp_Effect_Zone_Rotation *zr)
{
   E_FREE(zr);
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_begin(E_Comp_Effect_Zone_Rotation *zr)
{
   E_Comp_Layer *ly;
   E_Comp *c = e_mod_comp_util_get();

   E_CHECK_RETURN(c->animatable, EINA_FALSE);

   /* HACK: disabled xv app rotation and lock screen rotation
    * look for full-screen window and then check class and name.
    * if it is an app window which is using xv, rotation effect will be
    * canceled.
    */
   E_Border *bd = e_border_focused_get();
   E_Border *_bd = NULL;
   E_Comp_Win *cw = NULL, *cw2 = NULL;
   if (bd)
     {
        cw = e_mod_comp_win_find(bd->win);
        if ((cw) && (!cw->invalid) && (!cw->input_only) && (cw->visible))
          {
             /* floating mode window can have focus */
             if (!REGION_EQUAL_TO_ZONE(cw, bd->zone))
               {
                  cw2 = e_mod_comp_util_win_normal_get(cw);
                  if ((cw2) && (cw2->bd)) _bd = cw2->bd;
               }
             else
               _bd = bd;

             if (_bd)
               {
                  if ((_bd->client.icccm.name) && (_bd->client.icccm.class))
                    {
                       if ((!strcmp(_bd->client.icccm.name, "camera")) &&
                           (!strcmp(_bd->client.icccm.class, "camera")))
                         {
                            ELB(ELBT_COMP, "SKIP CAMERA", _bd->client.win);
                            return EINA_FALSE;
                         }
                       else if (!strcmp(_bd->client.icccm.name, "video_play"))
                         {
                            ELB(ELBT_COMP, "SKIP VIDEO PLAYER", _bd->client.win);
                            return EINA_FALSE;
                         }
                       if ((!strcmp(_bd->client.icccm.name, "LOCK_SCREEN")) &&
                           (!strcmp(_bd->client.icccm.class, "LOCK_SCREEN")))
                         {
                            ELB(ELBT_COMP, "SKIP LOCK_SCREEN", _bd->client.win);
                            return EINA_FALSE;
                         }
                    }
               }
          }
     }

   ly = e_mod_comp_canvas_layer_get(zr->canvas, "effect");
   if (ly)
     {
        /* TODO: check animation is running */
        if (zr->trans_begin) elm_transit_del(zr->trans_begin);
        if (zr->trans_end) elm_transit_del(zr->trans_end);
        zr->trans_begin = NULL;
        zr->trans_end = NULL;

        zr->trans_begin = elm_transit_add();

        zr->effect_begin = _effect_zone_rot_begin_new(ly);
        if (!zr->effect_begin)
          {
             if (zr->trans_begin) elm_transit_del(zr->trans_begin);
             zr->trans_begin = NULL;
             zr->ready = EINA_FALSE;
             return EINA_FALSE;
          }
        elm_transit_object_add(zr->trans_begin, ly->layout);
        elm_transit_smooth_set(zr->trans_begin, EINA_FALSE);
        elm_transit_duration_set(zr->trans_begin, 0.4f);
        elm_transit_effect_add(zr->trans_begin, _effect_zone_rot_begin_op, zr->effect_begin, _effect_zone_rot_begin_free);
        elm_transit_tween_mode_set(zr->trans_begin, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
        elm_transit_objects_final_state_keep_set(zr->trans_begin, EINA_FALSE);

        e_mod_comp_layer_effect_set(ly, EINA_TRUE);

        zr->ready = EINA_TRUE;
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_end(E_Comp_Effect_Zone_Rotation *zr)
{
   E_Comp_Layer *ly;
   E_Comp *c = e_mod_comp_util_get();

   E_CHECK_RETURN(c->animatable, EINA_FALSE);
   E_CHECK_RETURN(zr->ready, EINA_FALSE);
   E_CHECK_RETURN(zr->trans_begin, EINA_FALSE);

   zr->ready = EINA_FALSE;

   ly = e_mod_comp_canvas_layer_get(zr->canvas, "comp");
   if (ly)
     {
        if (zr->trans_end) elm_transit_del(zr->trans_end);
        zr->trans_end = NULL;

        zr->trans_end = elm_transit_add();
        zr->effect_end = _effect_zone_rot_end_new(ly->layout, zr->canvas->zone);
        elm_transit_object_add(zr->trans_end, ly->layout);
        elm_transit_smooth_set(zr->trans_end, EINA_FALSE);
        elm_transit_duration_set(zr->trans_end, 0.4f);
        elm_transit_effect_add(zr->trans_end, _effect_zone_rot_end_op, zr->effect_end, _effect_zone_rot_end_free);
        elm_transit_tween_mode_set(zr->trans_end, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
        elm_transit_objects_final_state_keep_set(zr->trans_end, EINA_FALSE);

        e_zone_rotation_block_set(zr->canvas->zone, "comp-tizen", EINA_TRUE);

        return EINA_TRUE;
     }

   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_cancel(E_Comp_Effect_Zone_Rotation *zr)
{
   if (zr->trans_begin)
     {
        elm_transit_del(zr->trans_begin);
        zr->trans_begin = NULL;
     }
   if (zr->trans_end)
     {
        elm_transit_del(zr->trans_end);
        zr->trans_end = NULL;
     }
   zr->ready = EINA_FALSE;

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_do(E_Comp_Effect_Zone_Rotation *zr)
{
   E_Comp *c = e_mod_comp_util_get();
   E_CHECK_RETURN(c->animatable, EINA_FALSE);

   elm_transit_go(zr->trans_begin);
   elm_transit_go(zr->trans_end);

   return EINA_TRUE;
}

/* local subsystem functions */
static Eina_Bool
_angle_get(E_Comp_Win *cw,
           int        *req,
           int        *curr)
{
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(req, 0);
   E_CHECK_RETURN(curr, 0);

   *req  = cw->bd->client.e.state.rot.prev;
   *curr = cw->bd->client.e.state.rot.curr;

   return EINA_TRUE;
}

static Elm_Transit_Effect *
_effect_zone_rot_begin_new(E_Comp_Layer *ly)
{
   E_Comp_Zone_Rotation_Effect_Begin *ctx= E_NEW(E_Comp_Zone_Rotation_Effect_Begin, 1);
   E_CHECK_RETURN(ctx, NULL);

   E_Zone *zone = ly->canvas->zone;

   /* capture the screen */
   Ecore_X_Window_Attributes att;
   Ecore_X_Window root = ecore_x_window_root_first_get();
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   if (!ecore_x_window_attributes_get(root, &att))
     {
        E_FREE(ctx);
        return NULL;
     }

   ctx->xim = ecore_x_image_new(zone->w, zone->h, att.visual, att.depth);
   if (!ctx->xim)
     {
        E_FREE(ctx);
        return NULL;
     }

   unsigned int *pix = ecore_x_image_data_get(ctx->xim, NULL, NULL, NULL);;
   ELBF(ELBT_COMP, 0, 0, "%15.15s|root:0x%08x vis:%p depth:%d xim:%p pix:%p", "ZONE_ROT_B_NEW",
        root, att.visual, att.depth, ctx->xim, pix);

   ctx->img = evas_object_image_filled_add(evas_object_evas_get(ly->layout));
   evas_object_image_colorspace_set(ctx->img, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(ctx->img, 0);
   evas_object_image_size_set(ctx->img, zone->w, zone->h);
   evas_object_image_smooth_scale_set(ctx->img, EINA_FALSE);
   evas_object_image_data_set(ctx->img, pix);
   evas_object_image_data_update_add(ctx->img, 0, 0, zone->w, zone->h);

   // why do we neeed these 2? this smells wrong
   if (ecore_x_image_get(ctx->xim, root, 0, 0, 0, 0, zone->w, zone->h))
     {
        pix = ecore_x_image_data_get(ctx->xim, NULL, NULL, NULL);;
        evas_object_image_data_set(ctx->img, pix);
        evas_object_image_data_update_add(ctx->img, 0, 0, zone->w, zone->h);
     }
   evas_object_show(ctx->img);

   e_mod_comp_layer_populate(ly, ctx->img);
   e_layout_child_move(ctx->img, 0, 0);
   e_layout_child_resize(ctx->img, zone->w, zone->h);

   ctx->o = ly->layout;
   ctx->zone = ly->canvas->zone;
   ctx->ly = ly;

   int diff = zone->rot.prev - zone->rot.curr;
   if (diff == 270) diff = -90;
   else if (diff == -270) diff = 90;
   ctx->src = 0.0;
   ctx->target = diff;
   ELBF(ELBT_COMP, 0, 0, "%15.15s|%d->%d pix:%p", "ZONE_ROT_B_NEW", zone->rot.prev, zone->rot.curr, pix);
   return ctx;
}

static Elm_Transit_Effect *
_effect_zone_rot_end_new(Evas_Object *o,
                         E_Zone      *zone)
{
   E_Comp_Zone_Rotation_Effect_End *ctx= E_NEW(E_Comp_Zone_Rotation_Effect_End, 1);
   E_CHECK_RETURN(ctx, NULL);

   ctx->o = o;
   ctx->zone = zone;
   int diff = zone->rot.curr - zone->rot.prev;
   if (diff == 270) diff = -90;
   else if (diff == -270) diff = 90;
   ctx->src = diff;
   ctx->target = 0.0;
   ELBF(ELBT_COMP, 0, 0, "%15.15s|%d->%d", "ZONE_ROTATION_EFFECT_END_NEW", zone->rot.prev, zone->rot.curr);
   return ctx;
}

static void
_effect_zone_rot_begin_free(Elm_Transit_Effect *effect,
                            Elm_Transit        *transit)
{
   E_Comp_Zone_Rotation_Effect_Begin *ctx = (E_Comp_Zone_Rotation_Effect_Begin *)effect;
   if (ctx->xim) ecore_x_image_free(ctx->xim);
   if (ctx->img)
     {
        evas_object_hide(ctx->img);
        e_layout_unpack(ctx->img);
        evas_object_del(ctx->img);
     }
   //evas_object_color_set(ctx->o, 255, 255, 255, 255);

   e_mod_comp_layer_effect_set(ctx->ly, EINA_FALSE);

   E_FREE(ctx);
}

static void
_effect_zone_rot_end_free(Elm_Transit_Effect *effect,
                          Elm_Transit        *transit)
{
   E_Comp_Zone_Rotation_Effect_End *ctx = (E_Comp_Zone_Rotation_Effect_End *)effect;
   e_zone_rotation_block_set(ctx->zone, "comp-tizen", EINA_FALSE);
   evas_object_color_set(ctx->o, 255, 255, 255, 255);
   ELBF(ELBT_COMP, 0, 0, "%15.15s|", "ZONE_ROTATION_EFFECT_END_FREE");
   E_FREE(ctx);
}

static void
_effect_zone_rot_begin_op(Elm_Transit_Effect *effect,
                          Elm_Transit        *transit,
                          double              progress)
{
   E_Comp_Zone_Rotation_Effect_Begin *ctx = (E_Comp_Zone_Rotation_Effect_Begin *)effect;
   double curr = (progress * ctx->target);
   Evas_Coord x, y, w, h;

   double col = 255 - (255 * progress * 1.3);
   if (col <= 0) col = 0;
   evas_object_color_set(ctx->o, col, col, col, col);

   int diff = ctx->zone->h - ctx->zone->w;
   Evas_Coord _x = -1.0 * (diff * progress) / 2;
   Evas_Coord _y = (diff * progress) / 2;
   Evas_Coord _w = ctx->zone->w + (diff * progress);
   Evas_Coord _h = ctx->zone->h - (diff * progress);
   evas_object_move(ctx->o, _x, _y);
   evas_object_resize(ctx->o, _w, _h);
   evas_object_geometry_get(ctx->o, &x, &y, &w, &h);

   Evas_Map *m = evas_map_new(4);
   evas_map_util_points_populate_from_object(m, ctx->o);
   evas_map_util_rotate(m, curr, x + (w/2), y + (h/2));
   evas_object_map_set(ctx->o, m);
   evas_object_map_enable_set(ctx->o, EINA_TRUE);
   evas_map_free(m);

}

static void
_effect_zone_rot_end_op(Elm_Transit_Effect *effect,
                        Elm_Transit        *transit,
                        double              progress)
{
   E_Comp_Zone_Rotation_Effect_End *ctx = (E_Comp_Zone_Rotation_Effect_End *)effect;
   double curr = ((-1.0f * progress * ctx->src) + ctx->src);
   Evas_Coord x, y, w, h;

   double col = 100 + 155 * progress;
   evas_object_color_set(ctx->o, col, col, col, 255);

   evas_object_geometry_get(ctx->o, &x, &y, &w, &h);

   Evas_Map *m = evas_map_new(4);
   evas_map_util_points_populate_from_object(m, ctx->o);
   evas_map_util_rotate(m, curr, x + (w/2), y + (h/2));
   evas_object_map_set(ctx->o, m);
   evas_object_map_enable_set(ctx->o, EINA_TRUE);
   evas_map_free(m);
}
