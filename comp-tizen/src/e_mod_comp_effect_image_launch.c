#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_image_launch.h"

struct _E_Comp_Effect_Image_Launch
{
   Eina_Bool       running : 1;
   Eina_Bool       fake_image_show_done : 1; // image launch edje object got effect done or not.
   Evas_Object    *obj;             // image object
   Evas_Object    *shobj;           // image shadow object
   Ecore_Timer    *timeout;         // max time between show, hide image launch
   Ecore_X_Window  win;             // this represent image launch effect's real window id.
   int             w, h;            // width and height of image object
   int             rot;             // rotation angle
   int             indicator_show;  // indicator enable / disable flag
   Evas_Object*    indicator_obj;   // plugin indicator object
   Evas_Object*    layer_obj;       // evas_object between normal layer with float layer
};

/* local subsystem functions */
static void      _show_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _hide_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _launch_timeout(void *data);
static void      _invisible_win_hide(E_Comp_Win *cw, Eina_Bool show);
static E_Comp_Win *_bg_win_get(void);
static E_Comp_Win *_appinapp_bottom_win_get(E_Comp_Win *normal_top);

/* local subsystem globals */

/* externally accessible functions */
EINTERN E_Comp_Effect_Image_Launch *
e_mod_comp_effect_image_launch_new(Evas *e,
                                   int w, int h)
{
   E_Comp_Effect_Image_Launch *eff;
   int ok = 0;

   E_Container *con;
   E_Comp_Win *layers_cw = NULL;
   Eina_List *ll;
   E_Comp_Object *co;
   int layer;

   E_CHECK_RETURN(e, NULL);

   eff = E_NEW(E_Comp_Effect_Image_Launch, 1);
   E_CHECK_RETURN(eff, NULL);

   eff->shobj = edje_object_add(e);
   E_CHECK_GOTO(eff->shobj, error);

   //indicator_comp
   Ecore_Evas *ee;
   ee = ecore_evas_ecore_evas_get(e);
   E_CHECK_GOTO(ee, error);
   eff->indicator_obj = ecore_evas_extn_plug_new(ee);
   E_CHECK_GOTO(eff->indicator_obj, error);

   ok = edje_object_file_set
          (eff->shobj,
          _comp_mod->conf->shadow_file,
          "fake_effect_fade");
   E_CHECK_GOTO(ok, error);

   eff->w = w;
   eff->h = h;

   evas_object_move(eff->shobj, 0, 0);
   evas_object_resize(eff->shobj, w, h);

   eff->obj = evas_object_image_add(e);
   E_CHECK_GOTO(eff->obj, error);

   edje_object_signal_callback_add
     (eff->shobj, "fake,action,hide,done", "fake",
     _hide_done, eff);

   edje_object_signal_callback_add
     (eff->shobj, "fake,action,show,done", "fake",
     _show_done, eff);

   // check fake image object stack (Choice proper layer)
   eff->layer_obj = NULL;
   layer = _comp_mod->conf->fake_launch_layer;
   con = e_container_current_get(e_manager_current_get());
   if ((con) && (layer))
     layers_cw = e_mod_comp_win_find(con->layers[layer].win);

   if (layers_cw)
     {
        EINA_LIST_FOREACH(layers_cw->objs, ll, co)
          {
             if (!co) continue;
             eff->layer_obj = co->shadow;
          }
     }

   return eff;

error:
   e_mod_comp_effect_image_launch_free(eff);
   return NULL;
}

EINTERN void
e_mod_comp_effect_image_launch_free(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK(eff);
   if (eff->shobj) evas_object_del(eff->shobj);
   if (eff->obj) evas_object_del(eff->obj);
   E_FREE(eff);
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_handler_message(Ecore_X_Event_Client_Message *ev)
{
   E_Comp_Effect_Image_Launch *eff;
   E_Comp *c = NULL;
   char *file = NULL;

   E_CHECK_RETURN(ev, 0);

   c = e_mod_comp_find(ev->win);
   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(c->fake_image_launch, 0);

   eff = c->eff_img;
   E_CHECK_RETURN(eff, 0);

   eff->rot = ev->data.l[1]; //rotation value
   eff->indicator_show = ev->data.l[2]; //indicator flag value

   if (ev->data.l[0] == 0)
     {
        e_mod_comp_effect_image_launch_hide(eff);
     }
   else if (ev->data.l[0] == 1)
     {
        file = ecore_x_window_prop_string_get
                 (ev->win, ATOM_IMAGE_LAUNCH_FILE);
        E_CHECK_RETURN(file, 0);

        e_mod_comp_effect_image_launch_show(eff, file);
        E_FREE(file);
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_show(E_Comp_Effect_Image_Launch *eff,
                                    const char *file)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *bg_cw = NULL;
   Eina_Bool grabbed;
   Evas_Load_Error err;
   Eina_Bool res;
   int img_w, img_h;
   Eina_List *ll;
   E_Comp_Object *co;
   E_Comp_Win *bottom_appinapp_cw = NULL;

   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(file, 0);
   E_CHECK_RETURN(c, 0);

   grabbed = e_mod_comp_util_grab_key_set(EINA_TRUE);
   E_CHECK_RETURN(grabbed, 0);

   eff->running = EINA_TRUE;

   //fake image part
   evas_object_image_file_set(eff->obj, file, NULL);
   err = evas_object_image_load_error_get(eff->obj);
   E_CHECK_GOTO(err == EVAS_LOAD_ERROR_NONE, error);

   evas_object_image_size_get(eff->obj, &(img_w), &(img_h));
   evas_object_image_fill_set(eff->obj, 0, 0, img_w, img_h);
   evas_object_image_filled_set(eff->obj, EINA_TRUE);

   evas_object_show(eff->obj);

   res = edje_object_part_swallow
           (eff->shobj, "fake.swallow.content", eff->obj);
   E_CHECK_GOTO(res, error);

   if ((eff->rot == 0) || (eff->rot == 180))//portrait mode
     {
        evas_object_move(eff->shobj, 0, 0);
        evas_object_resize(eff->shobj, eff->w, eff->h);

        //get indicator object
        if (eff->indicator_show)
          {
             if (!ecore_evas_extn_plug_connect(eff->indicator_obj, "elm_indicator_portrait", 0, EINA_FALSE))
               eff->indicator_show = 0;
          }
     }
   else //landscape mode
     {
        evas_object_move(eff->shobj, (eff->w - eff->h)/2, (eff->h - eff->w)/2);
        evas_object_resize(eff->shobj, eff->h, eff->w);

        //get indicator object
        if (eff->indicator_show)
          {
             if (!ecore_evas_extn_plug_connect(eff->indicator_obj, "elm_indicator_landscape", 0, EINA_FALSE))
               eff->indicator_show = 0;
          }
     }

   if (eff->indicator_show)
     {
        evas_object_show(eff->indicator_obj);
        res = edje_object_part_swallow
              (eff->shobj, "fake.swallow.indicator", eff->indicator_obj);
        E_CHECK_GOTO(res, error);
     }

   // background hide effect
   if(c->animatable)
     {
        bg_cw = _bg_win_get();
   if (bg_cw)
     {
        _invisible_win_hide(bg_cw, EINA_TRUE);
     }
     }

   // set position of fake launch image
   if (eff->layer_obj)
     {
        evas_object_stack_above(eff->shobj, eff->layer_obj);
     }
   else
     {
        bottom_appinapp_cw = _appinapp_bottom_win_get(bg_cw);

        if (bottom_appinapp_cw)
          {
             EINA_LIST_FOREACH(bottom_appinapp_cw->objs, ll, co)
               {
                  if (!co) continue;
                  evas_object_stack_below(eff->shobj, co->shadow);
               }
          }
        else
          evas_object_raise(eff->shobj);
     }

   evas_object_show(eff->shobj);

   if (c->animatable)
     {
        switch (eff->rot)
          {
           case 90:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,90,on", "fake");
              break;
           case 180:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,180,on", "fake");
              break;
           case 270:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,270,on", "fake");
              break;
           default:
        e_mod_comp_effect_signal_add
          (NULL, eff->shobj,
          "fake,state,visible,on", "fake");
              break;
          }
     }
   else
     {
        switch (eff->rot)
          {
           case 90:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,90,on,noeffect", "fake");
              break;
           case 180:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,180,on,noeffect", "fake");
              break;
           case 270:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,270,on,noeffect", "fake");
              break;
           default:
        e_mod_comp_effect_signal_add
          (NULL, eff->shobj,
          "fake,state,visible,on,noeffect", "fake");
              break;
          }
     }
   eff->timeout = ecore_timer_add(10.0f, _launch_timeout, eff);
   e_mod_comp_util_screen_input_region_set(EINA_TRUE);

   e_mod_comp_render_queue(c);
   return EINA_TRUE;

error:
   eff->running = EINA_FALSE;
   if (grabbed)
     e_mod_comp_util_grab_key_set(EINA_FALSE);
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_hide(E_Comp_Effect_Image_Launch *eff)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *bg_cw = NULL;
   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(eff, 0);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   // background hide effect
   if(c->animatable)
     {
        bg_cw = _bg_win_get();
   if (bg_cw)
     {
        _invisible_win_hide(bg_cw, EINA_FALSE);
     }
     }

   if (c->animatable)
     {
        switch (eff->rot)
          {
           case 90:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,90,off", "fake");
              break;
           case 180:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,180,off", "fake");
              break;
           case 270:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,270,off", "fake");
              break;
           default:
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off", "fake");
              break;
          }
     }
   else
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off,noeffect", "fake");

   e_mod_comp_render_queue(c);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_window_check(E_Comp_Effect_Image_Launch *eff, E_Comp_Win *cw)
{
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->running, 0);
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);

   if ( REGION_EQUAL_TO_ROOT(cw) && TYPE_NORMAL_CHECK(cw))
     {
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_running_check(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK_RETURN(eff, 0);
   return eff->running;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_fake_show_done_check(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK_RETURN(eff, 0);
   return eff->fake_image_show_done;
}

EINTERN Eina_Bool
e_mod_comp_effect_image_launch_window_set(E_Comp_Effect_Image_Launch *eff,
                                          Ecore_X_Window w)
{
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(w, 0);
   eff->win = w;
   return EINA_TRUE;
}

EINTERN void
e_mod_comp_effect_image_launch_disable(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK(eff);
   E_CHECK(eff->running);

   eff->running = EINA_FALSE;
   eff->fake_image_show_done= EINA_FALSE;
   eff->win = 0;

   e_mod_comp_util_screen_input_region_set(EINA_FALSE);
   e_mod_comp_util_grab_key_set(EINA_FALSE);

   E_Comp *c = e_mod_comp_util_get();
   if (c)
     e_mod_comp_win_shape_input_update(c);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   if (eff->indicator_show)
     {
        edje_object_part_unswallow(eff->shobj, eff->indicator_obj);
        evas_object_hide(eff->indicator_obj);
     }
   edje_object_part_unswallow(eff->shobj, eff->obj);

   evas_object_hide(eff->shobj);
   evas_object_hide(eff->obj);
}

static E_Comp_Win *
_appinapp_bottom_win_get(E_Comp_Win *normal_top)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw = NULL;
   E_Comp_Win *_cw = NULL;
   E_CHECK_RETURN(c, 0);
   Ecore_X_Window win = 0;

   if (normal_top)
     win = normal_top->win;

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if((cw->win) == (win)) break;
        if (!(cw->bd)) continue;

        if(cw->bd->client.illume.win_state.state == ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
          _cw = cw;
     }
   return _cw;
}


static E_Comp_Win *
_bg_win_get(void)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw = NULL;
   E_CHECK_RETURN(c, 0);

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((cw->visible) &&
            (REGION_EQUAL_TO_ROOT(cw)) &&
            (!cw->invalid) && (!cw->input_only) &&
            (TYPE_NORMAL_CHECK(cw)))
          {
             return cw;
          }
        if(TYPE_HOME_CHECK(cw))
          return cw;
     }
   return NULL;
}


static void
_show_done(void *data,
           Evas_Object *obj __UNUSED__,
           const char *emission __UNUSED__,
           const char *source __UNUSED__)
{
   E_Comp_Win *cw;
   E_Comp_Effect_Image_Launch *eff;
   eff = (E_Comp_Effect_Image_Launch *)data;
   E_CHECK(eff);

   eff->fake_image_show_done = EINA_TRUE;

   E_CHECK(eff->win);

   cw = e_mod_comp_win_find(eff->win);
   E_CHECK(cw);

   e_mod_comp_effect_signal_add
     (cw, NULL, "e,state,visible,on,noeffect", "e");
   e_mod_comp_comp_event_src_visibility_send(cw);
   e_mod_comp_effect_image_launch_disable(eff);

   e_mod_comp_win_render_queue(cw);
}

static void
_hide_done(void *data,
           Evas_Object *obj __UNUSED__,
           const char *emission __UNUSED__,
           const char *source __UNUSED__)
{
   E_Comp_Effect_Image_Launch *eff;
   eff = (E_Comp_Effect_Image_Launch *)data;
   E_CHECK(eff);

   e_mod_comp_util_screen_input_region_set(EINA_FALSE);
   e_mod_comp_util_grab_key_set(EINA_FALSE);

   if (eff->indicator_show)
     {
        edje_object_part_unswallow(eff->shobj, eff->indicator_obj);
        evas_object_hide(eff->indicator_obj);
     }
   edje_object_part_unswallow(eff->shobj, eff->obj);
   evas_object_hide(eff->shobj);
   evas_object_hide(eff->obj);
}

static Eina_Bool
_launch_timeout(void *data)
{
   E_Comp_Effect_Image_Launch *eff;
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *bg_cw = NULL;
   eff = (E_Comp_Effect_Image_Launch *)data;

   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(eff, 0);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   eff->running = EINA_FALSE;
   eff->win = 0;
   eff->fake_image_show_done = EINA_FALSE;

   // background hide effect
   bg_cw = _bg_win_get();
   if (bg_cw)
     {
        _invisible_win_hide(bg_cw, EINA_FALSE);
     }

   if (c->animatable)
     {
        switch (eff->rot)
          {
           case 90:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,90,off", "fake");
              break;
           case 180:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,180,off", "fake");
              break;
           case 270:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,270,off", "fake");
              break;
           default:
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off", "fake");
              break;
          }
     }
   else
     e_mod_comp_effect_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off,noeffect", "fake");

   e_mod_comp_render_queue(c);

   return EINA_FALSE;
}

static void
_invisible_win_hide(E_Comp_Win *cw,
                    Eina_Bool show)
{
   E_Comp *c = e_mod_comp_util_get();
   Eina_List *ll;
   E_Comp_Object *co;
   Eina_Bool visible = 0;
   E_Comp_Canvas *canvas;
   E_Comp_Win *_cw = cw;
   Eina_Inlist *l;

   E_CHECK(cw);
   E_CHECK(c);

   while ((l = EINA_INLIST_GET(_cw)->prev) != NULL)
     {
        visible = 0;
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        E_CHECK(_cw);
        if ((!_cw) ||
            (_cw->invalid) ||
            (_cw->input_only) ||
            (_cw->win == cw->win) ||
            TYPE_INDICATOR_CHECK(_cw))
          {
             continue;
          }

        EINA_LIST_FOREACH(_cw->objs, ll, co)
          {
             if (!co) continue;
             if (evas_object_visible_get(co->shadow))
               {
                  visible = 1;
                  break;
               }
          }
        if (!visible) continue;

        // do hide window which is not related window animation effect.
        _cw->animate_hide = EINA_TRUE;
        EINA_LIST_FOREACH(_cw->objs, ll, co)
          {
             if (!co) continue;
             evas_object_hide(co->shadow);
          }
     }

   EINA_LIST_FOREACH(cw->c->canvases, ll, canvas)
     {
        if (!canvas) continue;
        if (canvas->use_bg_img) continue;
        evas_object_lower(canvas->bg_img);
     }

   EINA_LIST_FOREACH(cw->objs, ll, co)
     {
        if (!co) continue;
        if (!cw->hidden_override && cw->show_done)
          evas_object_show(co->shadow);
     }

   cw->animate_hide = EINA_FALSE;
   cw->c->effect_stage = EINA_TRUE;

   if (show)
     {
        if (c->animatable)
          e_mod_comp_effect_signal_add
            (cw, NULL,
            "e,state,background,visible,on", "e");
        else
          e_mod_comp_effect_signal_add
            (cw, NULL,
            "e,state,background,visible,on,noeffect", "e");
     }
   else
     {
        if (c->animatable)
          e_mod_comp_effect_signal_add
            (cw, NULL,
            "e,state,background,visible,off", "e");
        else
          e_mod_comp_effect_signal_add
            (cw, NULL,
            "e,state,background,visible,off,noeffect", "e");
     }
}