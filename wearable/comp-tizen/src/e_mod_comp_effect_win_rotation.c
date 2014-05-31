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
   Eina_Bool           xv_use;
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
   Evas_Object   *xv_img;
   Ecore_X_Pixmap xv_pix;
   Eina_Bool      init;
   E_Comp_Layer  *ly;
};

struct _E_Comp_Zone_Rotation_Effect_End
{
   Evas_Object   *o;
   E_Zone        *zone;
   double         src;
   double         target;
   Evas_Object   *xv_img;
   Ecore_X_Pixmap xv_pix;
   Eina_Bool      init;
   Eina_Bool      send_msg;
};

/* local subsystem functions */
static void                _on_trans_begin_end(void *data, Elm_Transit *transit __UNUSED__);
static void                _on_trans_end_end(void *data, Elm_Transit *transit __UNUSED__);
static Eina_Bool           _angle_get(E_Comp_Win *cw, int *req, int *curr);
static Eina_Bool           _effect_zone_rot_bd_check(E_Border *bd);
static Elm_Transit_Effect *_effect_zone_rot_begin_new(E_Comp_Layer *ly, Eina_Bool xv_use);
static Elm_Transit_Effect *_effect_zone_rot_end_new(E_Comp_Layer *ly, E_Zone *zone, Eina_Bool xv_use);
static void                _effect_zone_rot_begin_free(Elm_Transit_Effect *effect, Elm_Transit *transit);
static void                _effect_zone_rot_end_free(Elm_Transit_Effect *effect, Elm_Transit *transit);
static void                _effect_zone_rot_begin_op(Elm_Transit_Effect *effect, Elm_Transit *transit, double progress);
static void                _effect_zone_rot_end_op(Elm_Transit_Effect *effect, Elm_Transit *transit, double progress);
static Eina_Bool           _effect_zone_rot_clear(E_Comp_Effect_Zone_Rotation *zr);

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
   memset(canvas->xv_ready, 0, sizeof(canvas->xv_ready));

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

   e_mod_comp_util_rr_prop_set(ATOM_RR_WM_STATE, "wm.zone.rotation.begin");

   E_CHECK_GOTO(c->animatable, finish);

   E_Comp_Win *cw = NULL, *nocomp_cw = NULL;
   E_Zone *zone = NULL;
   E_Comp_Canvas *canvas = NULL;

   Eina_Bool res = EINA_FALSE;

   Ecore_X_Window root = c->man->root;
   int xv_prop_res = -1;
   unsigned int xv_use = 0;

   canvas = zr->canvas;

   zone = zr->canvas->zone;
   E_CHECK_GOTO(zone, finish);

   /* find nocomp window first because nocomp window doesn't have pixmap */
   nocomp_cw = e_mod_comp_util_win_nocomp_get(c, zone);

   /* Check whether or not it will do xv rotation effect. */
   xv_prop_res =  ecore_x_window_prop_card32_get(root, ATOM_XV_USE, &xv_use, 1);
   if ((xv_prop_res == -1 ) || (!xv_use))
     {
        zr->xv_use = EINA_FALSE;
        canvas->xv_ready[0] = EINA_FALSE;
     }
   else
     {
        if (!_comp_mod->conf->xv_rotation_effect)
          {
             zr->xv_use = EINA_FALSE;
             canvas->xv_ready[0] = EINA_FALSE;
             goto finish;
          }
     }

   /* if it is first entering to effect begin route and xv is running,
    * rotation effect is canceled at first and waits until xv pass pixmap0.
    */
   if ((!zr->xv_use) && (xv_prop_res >= 0) && (xv_use))
     {
        Ecore_X_Pixmap pix0 = 0, pix1 = 0;
        zr->xv_use = EINA_TRUE;
        zr->canvas->xv_ready[0] = EINA_TRUE;

        pix0 = ecore_x_pixmap_new(root, zone->w, zone->h, 24);
        ecore_x_window_prop_xid_set(root, ATOM_XV_PIXMAP0, ECORE_X_ATOM_PIXMAP, &pix0, 1);

        pix1 = ecore_x_pixmap_new(root, zone->w, zone->h, 24);
        ecore_x_window_prop_xid_set(root, ATOM_XV_PIXMAP1, ECORE_X_ATOM_PIXMAP, &pix1, 1);

        ecore_x_client_message32_send(root,
                                      ATOM_XV_BYPASS_TO_PIXMAP0,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      root, 0, 0, 0, 0);

        ecore_x_client_message32_send(root,
                                      ATOM_XV_BYPASS_TO_PIXMAP1,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      root, 0, 0, 0, 0);

        goto finish;
     }

   if (e_mod_comp_effect_image_launch_running_check(c->eff_img))
     goto finish;

   /* look for visible normal window except for given window */
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((cw->invalid) || (cw->input_only)) continue;
        if (!cw->bd) continue;
        if (!cw->bd->zone) continue;
        if (cw->bd->zone != zone) continue;
        if (!E_INTERSECTS(zone->x, zone->y, zone->w, zone->h,
                          cw->x, cw->y, cw->w, cw->h))
          continue;

        /* return nocomp window */
        if ((nocomp_cw) && (nocomp_cw == cw))
          {
             if (TYPE_LOCKSCREEN_CHECK(cw))
               {
                  ELB(ELBT_COMP, "SKIP LOCKSCREEN", e_mod_comp_util_client_xid_get(cw));
                  goto finish;
               }
             else
               {
                  if (cw->bd)
                    {
                       res = _effect_zone_rot_bd_check(cw->bd);
                       E_CHECK_GOTO(res, finish);
                    }

                  Eina_Bool animatable = EINA_FALSE;
                  animatable = e_mod_comp_effect_state_get(cw->eff_type);
                  E_CHECK_GOTO(animatable, finish);
                  break; /* do rotation effect */
               }
          }

        /* check pixmap and compare size with zone */
        if (!((cw->pixmap) && (cw->pw > 0) && (cw->ph > 0) &&
              (cw->dmg_updates >= 1)))
          continue;

        if (REGION_EQUAL_TO_ZONE(cw, zone))
          {
             if (cw->argb)
               {
                  /* except the lock screen window */
                  if (TYPE_LOCKSCREEN_CHECK(cw))
                    {
                       ELB(ELBT_COMP, "SKIP LOCKSCREEN", e_mod_comp_util_client_xid_get(cw));
                       goto finish;
                    }
                  else
                    continue; /* skip alpha window as a default */
               }
             else
               {
                  if (cw->bd)
                    {
                       res = _effect_zone_rot_bd_check(cw->bd);
                       E_CHECK_GOTO(res, finish);
                    }

                  Eina_Bool animatable = EINA_FALSE;
                  animatable = e_mod_comp_effect_state_get(cw->eff_type);
                  E_CHECK_GOTO(animatable, finish);
                  break; /* do rotation effect */
               }
          }
     }

   ly = e_mod_comp_canvas_layer_get(zr->canvas, "effect");
   if (ly)
     {
        /* TODO: check animation is running */
        if (zr->trans_begin)
          {
             elm_transit_del_cb_set(zr->trans_begin, NULL, NULL);
             elm_transit_del(zr->trans_begin);
          }
        if (zr->trans_end)
          {
             elm_transit_del_cb_set(zr->trans_end, NULL, NULL);
             elm_transit_del(zr->trans_end);
          }
        zr->trans_begin = NULL;
        zr->trans_end = NULL;
        zr->trans_begin = elm_transit_add();
        elm_transit_del_cb_set(zr->trans_begin, _on_trans_begin_end, zr);

        zr->effect_begin = _effect_zone_rot_begin_new(ly, zr->xv_use);
        if (!zr->effect_begin)
          {
             if (zr->trans_begin)
               {
                  elm_transit_del_cb_set(zr->trans_begin, NULL, NULL);
                  elm_transit_del(zr->trans_begin);
               }
             zr->trans_begin = NULL;
             zr->ready = EINA_FALSE;
             zr->xv_use = EINA_FALSE;
             goto finish;
          }
        elm_transit_object_add(zr->trans_begin, ly->layout);
        elm_transit_smooth_set(zr->trans_begin, EINA_FALSE);
        elm_transit_duration_set(zr->trans_begin, 0.3f);
        elm_transit_effect_add(zr->trans_begin, _effect_zone_rot_begin_op, zr->effect_begin, _effect_zone_rot_begin_free);
        elm_transit_tween_mode_set(zr->trans_begin, ELM_TRANSIT_TWEEN_MODE_DECELERATE);
        elm_transit_objects_final_state_keep_set(zr->trans_begin, EINA_FALSE);

        e_mod_comp_layer_effect_set(ly, EINA_TRUE);

        zr->ready = EINA_TRUE;
     }

   return EINA_TRUE;

finish:
   e_mod_comp_util_rr_prop_set(ATOM_RR_WM_STATE, "wm.effect.rotation.cancel");
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_end(E_Comp_Effect_Zone_Rotation *zr)
{
   E_Comp_Layer *ly;
   E_Comp *c = e_mod_comp_util_get();
   Eina_Bool xv_use = EINA_FALSE;

   e_mod_comp_util_rr_prop_set(ATOM_RR_WM_STATE, "wm.zone.rotation.end");

   E_CHECK_RETURN(c->animatable, EINA_FALSE);

   if ((!zr->ready) || (!zr->trans_begin))
     {
        zr->canvas->xv_ready[0] = EINA_FALSE;
        return EINA_FALSE;
     }

   if ((zr->canvas->xv_ready[0]) && (!((zr->canvas->xv_ready[1]) && (zr->canvas->xv_ready[2]))))
     return EINA_FALSE;

   zr->ready = EINA_FALSE;
   xv_use = zr->xv_use;
   zr->xv_use = EINA_FALSE;
   memset(zr->canvas->xv_ready, 0, sizeof(zr->canvas->xv_ready));

   ly = e_mod_comp_canvas_layer_get(zr->canvas, "comp");
   if (ly)
     {
        if (zr->trans_end)
          {
             elm_transit_del_cb_set(zr->trans_end, NULL, NULL);
             elm_transit_del(zr->trans_end);
          }
        zr->trans_end = NULL;
        zr->trans_end = elm_transit_add();
        elm_transit_del_cb_set(zr->trans_end, _on_trans_end_end, zr);
        zr->effect_end = _effect_zone_rot_end_new(ly, zr->canvas->zone, xv_use);
        elm_transit_object_add(zr->trans_end, ly->layout);
        elm_transit_smooth_set(zr->trans_end, EINA_FALSE);
        elm_transit_duration_set(zr->trans_end, 0.3f);
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
   e_mod_comp_util_rr_prop_set(ATOM_RR_WM_STATE, "wm.zone.rotation.cancel");
   return _effect_zone_rot_clear(zr);
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_do(E_Comp_Effect_Zone_Rotation *zr)
{
   E_Comp *c = e_mod_comp_util_get();
   E_CHECK_RETURN(c->animatable, EINA_FALSE);

   e_mod_comp_util_rr_prop_set(ATOM_RR_WM_STATE, "wm.effect.rotation.start");

   elm_transit_go(zr->trans_begin);
   elm_transit_go(zr->trans_end);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_zone_rotation_clear(E_Comp_Effect_Zone_Rotation *zr)
{
   return _effect_zone_rot_clear(zr);
}

static Eina_Bool
_effect_zone_rot_clear(E_Comp_Effect_Zone_Rotation *zr)
{
   if (zr->trans_begin)
     {
        elm_transit_del_cb_set(zr->trans_begin, NULL, NULL);
        elm_transit_del(zr->trans_begin);
        zr->trans_begin = NULL;
     }
   if (zr->trans_end)
     {
        elm_transit_del_cb_set(zr->trans_end, NULL, NULL);
        elm_transit_del(zr->trans_end);
        zr->trans_end = NULL;
     }
   zr->ready = EINA_FALSE;
   zr->xv_use = EINA_FALSE;

   return EINA_TRUE;
}

static void
_on_trans_begin_end(void *data,
                        Elm_Transit *transit __UNUSED__)
{
   E_Comp_Effect_Zone_Rotation *zr = data;
   zr->trans_begin = NULL;
   if (zr->trans_begin)
     {
        elm_transit_del_cb_set(zr->trans_begin, NULL, NULL);
        zr->trans_begin = NULL;
     }
}

static void
_on_trans_end_end(void *data,
                        Elm_Transit *transit __UNUSED__)
{
   E_Comp_Effect_Zone_Rotation *zr = data;
   if (zr->trans_end)
     {
        elm_transit_del_cb_set(zr->trans_end, NULL, NULL);
        zr->trans_end = NULL;
     }
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

   *req  = e_border_rotation_prev_angle_get(cw->bd);
   *curr = e_border_rotation_curr_angle_get(cw->bd);

   return EINA_TRUE;
}

/* check before running rotation effect for given normal window.
 * exceptional windows: lock, setup wizard
 * otherwise: do rotation effect
 */
static Eina_Bool
_effect_zone_rot_bd_check(E_Border *bd)
{
   /* except window which does not use the wm rotation */
   if (!bd->client.e.state.rot.app_set)
     {
        ELB(ELBT_COMP, "SKIP ROT_EFFECT app_set:0", bd->client.win);
        return EINA_FALSE;
     }
   /* except window which has the value of preferred rotation */
   else if (bd->client.e.state.rot.preferred_rot != -1)
     {
        ELB(ELBT_COMP, "SKIP ROT_EFFECT preferred_rot", bd->client.win);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}


static Elm_Transit_Effect *
_effect_zone_rot_begin_new(E_Comp_Layer *ly,
                           Eina_Bool xv_use)
{
   E_Comp_Zone_Rotation_Effect_Begin *ctx= E_NEW(E_Comp_Zone_Rotation_Effect_Begin, 1);
   E_CHECK_RETURN(ctx, NULL);

   Evas_Native_Surface ns;

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
   evas_object_image_alpha_set(ctx->img, 1);
   evas_object_render_op_set(ctx->img, EVAS_RENDER_COPY);
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

   if (xv_use)
     {
        int ret = -1;
        int pw = 0, ph = 0;
        ctx->xv_pix = 0;

        ret = ecore_x_window_prop_xid_get(root, ATOM_XV_PIXMAP0, ECORE_X_ATOM_PIXMAP, &ctx->xv_pix, 1);

        if (ret < 0)
          ctx->xv_pix = ecore_x_pixmap_new(root, zone->w, zone->h, 32);

        ecore_x_pixmap_geometry_get(ctx->xv_pix, NULL, NULL, &pw, &ph);

        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_X11;
        ns.data.x11.visual = ecore_x_default_visual_get(ecore_x_display_get(), ecore_x_default_screen_get());
        ns.data.x11.pixmap = ctx->xv_pix;

        ctx->xv_img = evas_object_image_filled_add(ly->canvas->evas);
        evas_object_image_size_set(ctx->xv_img, pw, ph);
        evas_object_image_smooth_scale_set(ctx->xv_img, EINA_FALSE);
        evas_object_image_native_surface_set(ctx->xv_img, &ns);
        evas_object_image_data_update_add(ctx->xv_img, 0, 0, zone->w, zone->h);

        evas_object_show(ctx->xv_img);

        e_layout_pack(ly->layout, ctx->xv_img);
        e_layout_child_move(ctx->xv_img, 0, 0);
        e_layout_child_resize(ctx->xv_img, zone->w, zone->h);
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
_effect_zone_rot_end_new(E_Comp_Layer *ly,
                         E_Zone       *zone,
                         Eina_Bool     xv_use)
{
   E_Comp_Zone_Rotation_Effect_End *ctx= E_NEW(E_Comp_Zone_Rotation_Effect_End, 1);
   E_CHECK_RETURN(ctx, NULL);

   Evas_Native_Surface ns;

   Ecore_X_Window root = ecore_x_window_root_first_get();

   if (xv_use)
     {
        int ret = -1;
        ctx->xv_pix = 0;

        ret = ecore_x_window_prop_xid_get(root, ATOM_XV_PIXMAP1, ECORE_X_ATOM_PIXMAP, &ctx->xv_pix, 1);

        if (ret < 0)
          ctx->xv_pix = ecore_x_pixmap_new(root, zone->w, zone->h, 24);

        ns.version = EVAS_NATIVE_SURFACE_VERSION;
        ns.type = EVAS_NATIVE_SURFACE_X11;
        ns.data.x11.visual = ecore_x_default_visual_get(ecore_x_display_get(), ecore_x_default_screen_get());
        ns.data.x11.pixmap = ctx->xv_pix;

        ctx->xv_img = evas_object_image_filled_add(evas_object_evas_get(ly->layout));
        evas_object_image_size_set(ctx->xv_img, zone->w, zone->h);
        evas_object_image_smooth_scale_set(ctx->xv_img, EINA_FALSE);
        evas_object_image_native_surface_set(ctx->xv_img, &ns);
        evas_object_image_data_update_add(ctx->xv_img, 0, 0, zone->w, zone->h);

//        evas_object_show(ctx->xv_img);

        e_layout_pack(ly->layout, ctx->xv_img);
        e_layout_child_move(ctx->xv_img, 0, 0);
        e_layout_child_resize(ctx->xv_img, zone->w, zone->h);

        ctx->send_msg = EINA_TRUE;
     }

   ctx->o = ly->layout;
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
   if (ctx->xv_img)
     {
        evas_object_image_native_surface_set(ctx->xv_img, NULL);
        evas_object_image_size_set(ctx->xv_img, 1, 1);
        evas_object_image_data_set(ctx->xv_img, NULL);

        if (ctx->xv_pix)
          {
             ecore_x_pixmap_free(ctx->xv_pix);
             ctx->xv_pix = 0;
          }

        evas_object_hide(ctx->xv_img);
        e_layout_unpack(ctx->xv_img);
        evas_object_del(ctx->xv_img);
        ctx->xv_img = NULL;
     }
   if (ctx->img)
     {
        evas_object_hide(ctx->img);
        e_layout_unpack(ctx->img);
        evas_object_del(ctx->img);
        ctx->img = NULL;
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

   if (ctx->xv_img)
     {
        evas_object_image_native_surface_set(ctx->xv_img, NULL);
        evas_object_image_size_set(ctx->xv_img, 1, 1);
        evas_object_image_data_set(ctx->xv_img, NULL);

        if (ctx->xv_pix)
          {
             ecore_x_pixmap_free(ctx->xv_pix);
             ctx->xv_pix = 0;
          }

        evas_object_hide(ctx->xv_img);
        e_layout_unpack(ctx->xv_img);
        evas_object_del(ctx->xv_img);
        ctx->xv_img = NULL;
     }

   if (ctx->send_msg)
     {
        ecore_x_client_message32_send(ecore_x_window_root_first_get(),
                                      ATOM_XV_ROT_EFFECT_DONE,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      ecore_x_window_root_first_get(), 0, 0, 0, 0);
     }

   e_mod_comp_util_rr_prop_set(ATOM_RR_WM_STATE, "wm.effect.rotation.finish");

   ELBF(ELBT_COMP, 0, 0, "%15.15s|", "ZONE_ROTATION_EFFECT_END_FREE");
   E_FREE(ctx);
}

static void
_effect_zone_rot_begin_op(Elm_Transit_Effect *effect,
                          Elm_Transit        *transit,
                          double              progress)
{
   E_Comp_Zone_Rotation_Effect_Begin *ctx = (E_Comp_Zone_Rotation_Effect_Begin *)effect;
   if (progress < 0.0) progress = 0.0;
   double curr = (progress * ctx->target);
   Evas_Coord x, y, w, h;

   double col = 255 - (255 * progress);
   if (col <= 0) col = 0;
   evas_object_color_set(ctx->o, 255, 255, 255, col);

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

   /* to notify beginning of xv rotation effect */
   if ((ctx->xv_img) && (!ctx->init) && (progress >= 0.1))
     {
        E_Comp *c = e_mod_comp_util_get();
        ecore_x_client_message32_send(c->man->root,
                                      ATOM_XV_ROT_EFFECT_BEGIN,
                                      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                      c->man->root, 0, 0, 0, 0);
        ctx->init = EINA_TRUE;
     }


}

static void
_effect_zone_rot_end_op(Elm_Transit_Effect *effect,
                        Elm_Transit        *transit,
                        double              progress)
{
   E_Comp_Zone_Rotation_Effect_End *ctx = (E_Comp_Zone_Rotation_Effect_End *)effect;
   if (progress < 0.0) progress = 0.0;
   double curr = ((-1.0f * progress * ctx->src) + ctx->src);
   Evas_Coord x, y, w, h;

   evas_object_color_set(ctx->o, 255, 255, 255, 255);

   evas_object_geometry_get(ctx->o, &x, &y, &w, &h);

   Evas_Map *m = evas_map_new(4);
   evas_map_util_points_populate_from_object(m, ctx->o);
   evas_map_util_rotate(m, curr, x + (w/2), y + (h/2));
   evas_object_map_set(ctx->o, m);
   evas_object_map_enable_set(ctx->o, EINA_TRUE);
   evas_map_free(m);

   /* to avoid exposing "comp" layer before rotation effect running */
   if ((ctx->xv_img) && (!ctx->init) && (progress >= 0.1))
     {
        evas_object_show(ctx->xv_img);
        ctx->init = EINA_TRUE;
     }
}
