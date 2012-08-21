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
   Ecore_Timer *timeout;
   struct {
      int       req;
      int       cur;
   } ang;
};

/* local subsystem functions */
static Eina_Bool _win_rotation_begin(E_Comp_Win *cw, Eina_Bool timeout);
static void      _win_rotation_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _angle_get(Ecore_X_Window win, int *req, int *curr);
static Eina_Bool _begin_timeout(void *data);
static Eina_Bool _end_timeout(void *data);
static Eina_Bool _counter_inc(E_Comp_Win *cw);

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
   if (r->timeout) ecore_timer_del(r->timeout);
   r->timeout = NULL;
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
   Ecore_X_Sync_Counter counter;
   Ecore_X_Window win;
   E_Comp_Effect_Win_Rotation *r;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   L(LT_EVENT_X,
     "[COMP] %31s\n", "PROP_ILLUME_ROT_WND_ANG");

   cw = e_mod_comp_border_client_find(ev->win);
   if (!cw)
     {
        cw = e_mod_comp_win_find(ev->win);
        if (!cw)
          {
             counter = ecore_x_e_comp_sync_counter_get(ev->win);
             ecore_x_e_comp_sync_cancel_send(ev->win);
             if (counter) ecore_x_sync_counter_inc(counter, 1);
             return EINA_FALSE;
          }
     }

   win = e_mod_comp_util_client_xid_get(cw);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_ROTATION);

   if (st == E_COMP_EFFECT_STYLE_NONE)
     {
        _counter_inc(cw);
        return EINA_FALSE;
     }

   res = _angle_get(win, &req_angle, &cur_angle);
   if (!res)
     {
        _counter_inc(cw);
        return EINA_FALSE;
     }

   cw->angle = req_angle;
   cw->angle %= 360;

   if (req_angle == cur_angle)
     {
        _counter_inc(cw);
        return EINA_FALSE;
     }

   effect = e_mod_comp_policy_win_rotation_effect_check(cw);
   if (!effect)
     {
        _counter_inc(cw);
        return EINA_FALSE;
     }

   if (!cw->eff_winrot)
     {
        cw->eff_winrot = e_mod_comp_effect_win_rotation_new();
        if (!cw->eff_winrot)
          {
             _counter_inc(cw);
             return EINA_FALSE;
          }
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
   if (r->timeout) ecore_timer_del(r->timeout);
   r->timeout = ecore_timer_add(4.0f, _begin_timeout, cw);

   _counter_inc(cw);

   L(LT_EVENT_X, "[COMP] %31s %d\n",
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
     "[COMP] %31s timeout:%d\n",
     "win_rot_begin", timeout);

   if (r->timeout)
     {
        ecore_timer_del(r->timeout);
        r->timeout = NULL;
     }

   switch (r->ang.req - r->ang.cur)
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
        _counter_inc(cw);
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

   r->timeout = ecore_timer_add(4.0f, _end_timeout, cw);
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
   if (r->timeout)
     {
        ecore_timer_del(r->timeout);
        r->timeout = NULL;
     }

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
_angle_get(Ecore_X_Window  win,
           int            *req,
           int            *curr)
{
   Eina_Bool res = EINA_FALSE;
   int ret, count;
   int angle[2] = {-1, -1};
   unsigned char* data = NULL;

   E_CHECK_RETURN(win, 0);
   E_CHECK_RETURN(req, 0);
   E_CHECK_RETURN(curr, 0);

   ret = ecore_x_window_prop_property_get
           (win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
           ECORE_X_ATOM_CARDINAL, 32, &data, &count);
   if (ret <= 0)
     {
        if (data) E_FREE(data);
        return res;
     }

   if (ret && data)
     {
        memcpy(&angle, data, sizeof (int)*count);
        if (count == 2) res = EINA_TRUE;
     }

   if (data) E_FREE(data);

   *req  = angle[_WND_REQUEST_ANGLE_IDX];
   *curr = angle[_WND_CURR_ANGLE_IDX];

   if (angle[0] == -1 &&
       angle[1] == -1)
     {
        res = EINA_FALSE;
     }

   L(LT_EVENT_X,
     "[COMP] %31s %d=>%d count:%d res:%s\n",
     "rot_prop_get",
     angle[_WND_CURR_ANGLE_IDX],
     angle[_WND_REQUEST_ANGLE_IDX],
     count, res ? "Ture" : "False");

   return res;
}

static Eina_Bool
_begin_timeout(void *data)
{
   E_Comp_Win *cw = (E_Comp_Win*)data;
   E_CHECK_RETURN(cw, 0);
   fprintf(stderr, "[E17-comp] %s(%d) w:0x%08x\n",
           __func__, __LINE__,
           e_mod_comp_util_client_xid_get(cw));
   _win_rotation_begin(cw, EINA_TRUE);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_end_timeout(void *data)
{
   E_Comp_Win *cw = (E_Comp_Win*)data;
   E_CHECK_RETURN(cw, 0);
   fprintf(stderr,"[E17-comp] %s(%d) w:0x%08x\n",
           __func__, __LINE__,
           e_mod_comp_util_client_xid_get(cw));
   _win_rotation_done((void*)cw, NULL, NULL, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_counter_inc(E_Comp_Win *cw)
{
   if (cw->counter)
     {
        ecore_x_sync_counter_inc(cw->counter, 1);

        L(LT_EVENT_X,
          "[COMP] %31.31s w:0x%08x done:%d val:%d ROTATION\n",
          "INC", e_mod_comp_util_client_xid_get(cw),
          cw->sync_info.done_count, cw->sync_info.val);
     }
   else
     {
        Ecore_X_Window w = e_mod_comp_util_client_xid_get(cw);
        cw->counter = ecore_x_e_comp_sync_counter_get(w);
        if (cw->counter) ecore_x_sync_counter_inc(cw->counter, 1);
     }
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

   res = _angle_get(win, &req_angle, &cur_angle);
   if (!res)
     return EINA_FALSE;

   cw->angle = req_angle;
   cw->angle %= 360;

   return EINA_TRUE;
}
