#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_win_rotation.h"

#define _WND_REQUEST_ANGLE_IDX 0
#define _WND_CURR_ANGLE_IDX    1

struct _E_Comp_Effect_Win_Rotation
{
   Eina_Bool    ready : 1;
   Eina_Bool    run   : 1;
   struct {
      int       req;
      int       cur;
   } ang;
};

/* local subsystem functions */
static Eina_Bool _win_rotation_begin(E_Comp_Win *cw, Eina_Bool timeout);
static void      _win_rotation_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _angle_get(E_Comp_Win *cw, int *req, int *curr);

/* externally accessible functions */
EINTERN E_Comp_Effect_Win_Rotation *
e_mod_comp_effect_win_rotation_new(void)
{
   E_Comp_Effect_Win_Rotation *r;
   r = E_NEW(E_Comp_Effect_Win_Rotation, 1);
   r->ang.req = -1;
   r->ang.cur = -1;
   return r;
}

EINTERN void
e_mod_comp_effect_win_rotation_free(E_Comp_Effect_Win_Rotation *r)
{
   E_CHECK(r);
   E_FREE(r);
}

EINTERN Eina_Bool
e_mod_comp_effect_win_roation_run_check(E_Comp_Effect_Win_Rotation *r)
{
   E_CHECK_RETURN(r, 0);
   return r->run;
}

EINTERN Eina_Bool
e_mod_comp_effect_win_rotation_handler_prop(Ecore_X_Event_Window_Property *ev)
{
   E_Comp_Win *cw = NULL;
   E_Comp_Effect_Style st;
   int req_angle = -1;
   int cur_angle = -1;
   Eina_Bool res, effect;
   Ecore_X_Window win;
   E_Comp_Effect_Win_Rotation *r;
   E_Comp_Canvas *canvas = NULL;
   Eina_List *l;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   L(LT_EVENT_X,
     "COMP|%31s\n", "PROP_ILLUME_ROT_WND_ANG");

   cw = e_mod_comp_border_client_find(ev->win);
   if (!cw)
     {
        cw = e_mod_comp_win_find(ev->win);
        if (!cw) return EINA_FALSE;
     }

   win = e_mod_comp_util_client_xid_get(cw);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_ROTATION);

   if (st == E_COMP_EFFECT_STYLE_NONE)
     {
        return EINA_FALSE;
     }

   res = _angle_get(cw, &req_angle, &cur_angle);
   if (!res) return EINA_FALSE;

   cw->angle = req_angle;
   cw->angle %= 360;

   if (req_angle == cur_angle) return EINA_FALSE;

   effect = e_mod_comp_policy_win_rotation_effect_check(cw);
   if (!effect) return EINA_FALSE;

   if (!cw->eff_winrot)
     {
        cw->eff_winrot = e_mod_comp_effect_win_rotation_new();
        if (!cw->eff_winrot) return EINA_FALSE;
        Eina_List *l;
        E_Comp_Object *co;
        EINA_LIST_FOREACH(cw->objs, l, co)
          {
             if (!co) continue;
             edje_object_signal_callback_add
               (co->shadow, "e,action,window,rotation,done",
               "e", _win_rotation_done, cw);
          }
     }

   r = cw->eff_winrot;
   r->ready = EINA_TRUE;
   r->ang.req = req_angle;
   r->ang.cur = cur_angle;

   if (_comp_mod->conf->nocomp_fs)
     {
        EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
          {
             if (canvas->nocomp.mode != E_NOCOMP_MODE_RUN) continue;
             if ((cw->nocomp) && (cw == canvas->nocomp.cw))
               {
                  L(LT_EVENT_X,
                    "COMP|%31s|new_w:0x%08x|nocomp.cw:0x%08x canvas:%d\n",
                    "ROTATION_HANDLER NOCOMP_END",
                    cw ? e_mod_comp_util_client_xid_get(cw) : 0,
                    e_mod_comp_util_client_xid_get(canvas->nocomp.cw),
                    canvas->num);
                  e_mod_comp_canvas_nocomp_end(canvas);
               }
          }
     }

   L(LT_EVENT_X, "COMP|%31s|%d\n",
     "ready", r->ready);

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_win_rotation_handler_update(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->eff_winrot, 0);
   E_CHECK_RETURN(cw->eff_winrot->ready, 0);

   return _win_rotation_begin(cw, EINA_FALSE);
}

EINTERN Eina_Bool
e_mod_comp_effect_win_rotation_handler_release(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->eff_winrot, 0);
   if (e_mod_comp_effect_win_roation_run_check(cw->eff_winrot))
     _win_rotation_done(cw, NULL, NULL, NULL);
   e_mod_comp_effect_win_rotation_free(cw->eff_winrot);
   cw->eff_winrot = NULL;
   return EINA_TRUE;
}

/* local subsystem functions */
static Eina_Bool
_win_rotation_begin(E_Comp_Win *cw,
                    Eina_Bool timeout)
{
   E_Comp_Effect_Win_Rotation *r;
   Eina_List *l, *ll;
   E_Comp_Canvas *canvas;
   E_Comp_Object *co;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->eff_winrot, 0);

   r = cw->eff_winrot;
   r->ready = EINA_FALSE;

   L(LT_EVENT_X,
     "COMP|%31s|timeout:%d\n",
     "win_rot_begin", timeout);

   switch (r->ang.cur - r->ang.req)
     {
      case -270: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,90",   "e"); break;
      case -180: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,180",  "e"); break;
      case  -90: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,-90",  "e"); break;
      case    0: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,0",    "e"); break;
      case   90: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,90",   "e"); break;
      case  180: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,-180", "e"); break;
      case  270: e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,-90",  "e"); break;
      default  : e_mod_comp_effect_signal_add(cw, NULL, "e,state,window,rotation,0",    "e"); break;
     }

   r->run = EINA_TRUE;

   if (timeout)
     {
        if (!cw->update)
          {
             if (cw->update_timeout)
               {
                  ecore_timer_del(cw->update_timeout);
                  cw->update_timeout = NULL;
               }
             cw->update = 1;
             cw->c->updates = eina_list_append(cw->c->updates, cw);
          }
        cw->drawme = 1;
        e_mod_comp_win_render_queue(cw);
     }

   if (!cw->argb)
     {
        EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
          {
             if (!canvas) continue;
             EINA_LIST_FOREACH(cw->objs, ll, co)
               {
                  if (!co) continue;
                  if (co->canvas == canvas)
                    {
                       evas_object_stack_below(canvas->bg_img,
                                               co->shadow);
                       canvas->use_bg_img = 1;
                    }
               }
          }
     }

   return EINA_TRUE;
}

static void
_win_rotation_done(void        *data,
                   Evas_Object *obj      __UNUSED__,
                   const char  *emission __UNUSED__,
                   const char  *source   __UNUSED__)
{
   E_Comp_Effect_Win_Rotation *r;
   E_Comp_Win *cw = (E_Comp_Win*)data;
   Eina_List *l, *ll;
   E_Comp_Canvas *canvas;
   E_Comp_Object *co;
   E_CHECK(cw);
   E_CHECK(cw->eff_winrot);

   r = cw->eff_winrot;

   if (!cw->show_done) cw->show_done = EINA_TRUE;

   EINA_LIST_FOREACH(cw->c->canvases, l, canvas)
     {
        if (!canvas) continue;
        EINA_LIST_FOREACH(cw->objs, ll, co)
          {
             if (!co) continue;
             if (co->canvas == canvas)
               {
                  evas_object_stack_below(canvas->bg_img,
                                          evas_object_bottom_get(canvas->evas));
                  canvas->use_bg_img = 0;
               }
          }
     }

   r->run = EINA_FALSE;

   e_mod_comp_effect_signal_del(cw, obj, "rotation,done");

   e_mod_comp_done_defer(cw);
}

static Eina_Bool
_angle_get(E_Comp_Win     *cw,
           int            *req,
           int            *curr)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(req, 0);
   E_CHECK_RETURN(curr, 0);

   *req  = cw->bd->client.e.state.rot.prev;
   *curr = cw->bd->client.e.state.rot.curr;

   L(LT_EVENT_X,
     "COMP|%31s|w:0x%08x|%d=>%d\n",
     "rot_prop_get",
     e_mod_comp_util_client_xid_get(cw),
     *req,
     *curr);

   return EINA_TRUE;
}

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

EINTERN void
e_mod_comp_explicit_win_rotation_done(E_Comp_Win *cw)
{
   E_CHECK(cw);
   _win_rotation_done((void*)cw, NULL, NULL, NULL);
}
