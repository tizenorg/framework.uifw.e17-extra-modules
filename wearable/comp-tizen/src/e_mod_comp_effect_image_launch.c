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
   Evas *evas;                       // pointer for saving evas of canvas
   Ecore_Evas *ecore_evas;          // pointer for saving ecore_evas of canvas
};

/* local subsystem functions */
static void      _show_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _hide_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _launch_timeout(void *data);
static Ecore_X_Window _apptray_win_get();

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

   //get ecore_evas and save
   Ecore_Evas *ee;
   ee = ecore_evas_ecore_evas_get(e);
   E_CHECK_GOTO(ee, error);
   eff->ecore_evas = ee;
   eff->evas = e;

   ok = edje_object_file_set
          (eff->shobj,
          _comp_mod->conf->shadow_file,
          "fake_effect_slide");
   E_CHECK_GOTO(ok, error);

   eff->w = w;
   eff->h = h;

   evas_object_move(eff->shobj, 0, 0);
   evas_object_resize(eff->shobj, w, h);

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
   E_Comp_Canvas *canvas;
   E_Comp_Layer *ly;

   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->evas, 0);
   E_CHECK_RETURN(eff->ecore_evas, 0);
   E_CHECK_RETURN(file, 0);
   E_CHECK_RETURN(c, 0);

   ELBF(ELBT_COMP, 0, 0,
        "%15.15s| ROT:%d, Indi:%d, File:%s", "FAKE SHOW",
        eff->rot, eff->indicator_show, file);

   grabbed = e_mod_comp_util_grab_key_set(EINA_TRUE);
   E_CHECK_RETURN(grabbed, 0);

   Ecore_X_Atom ATOM_NET_WM_WINDOW_SHOW;
   Ecore_X_Window apptray_win = 0;
   Eina_Bool open = EINA_FALSE;

   ATOM_NET_WM_WINDOW_SHOW = ecore_x_atom_get("_NET_WM_WINDOW_SHOW");
   if (!ATOM_NET_WM_WINDOW_SHOW) return EINA_FALSE;

   apptray_win = _apptray_win_get();
   if (apptray_win)
     {
        ELBF(ELBT_COMP, 0, 0,
             "%15.15s| Apptray hide", "FAKE SHOW");
        ecore_x_client_message32_send
          (apptray_win, ATOM_NET_WM_WINDOW_SHOW,
           ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
           apptray_win, open, 0, 0, 0);
        ecore_x_sync();
     }


   eff->running = EINA_TRUE;

   bg_cw = e_mod_comp_util_win_normal_get(NULL);
   if (bg_cw)
     {
        E_CHECK_GOTO(!TYPE_LOCKSCREEN_CHECK(bg_cw), error);
        e_mod_comp_effect_object_win_set(bg_cw, "e,state,background,visible,on");
     }

   //fake image part
   eff->obj = evas_object_image_add(eff->evas);
   E_CHECK_GOTO(eff->obj, error);
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

   evas_object_show(eff->shobj);

   canvas = eina_list_nth(c->canvases, 0);
   ly = e_mod_comp_canvas_layer_get(canvas, "effect");
   if (ly)
     {
        evas_object_data_set(eff->shobj, "comp.effect_obj.ly", ly);
        e_mod_comp_layer_populate(ly, eff->shobj);
     }

   if (bg_cw)
     e_mod_comp_effect_above_wins_set(bg_cw, EINA_TRUE);

   eff->indicator_obj = ecore_evas_extn_plug_new(eff->ecore_evas);
   E_CHECK_GOTO(eff->indicator_obj, error);

   if ((eff->rot == 0) || (eff->rot == 180))//portrait mode
     {
        e_layout_child_move(eff->shobj, 0, 0);
        e_layout_child_resize(eff->shobj, eff->w, eff->h);
        //get indicator object
        if (eff->indicator_show)
          {
             if (!ecore_evas_extn_plug_connect(eff->indicator_obj, "elm_indicator_portrait", 0, EINA_FALSE))
               eff->indicator_show = 0;
          }
     }
   else //landscape mode
     {
        e_layout_child_move(eff->shobj, (eff->w - eff->h)/2, (eff->h - eff->w)/2);
        e_layout_child_resize(eff->shobj, eff->h, eff->w);
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

   if (c->animatable)
     {
        switch (eff->rot)
          {
           case 90:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,on,from270", "fake");
              break;
           case 180:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,on,from0", "fake");
              break;
           case 270:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,on,from90", "fake");
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
        e_mod_comp_effect_signal_add
          (NULL, eff->shobj,
          "fake,state,visible,on,noeffect", "fake");
     }
   eff->timeout = ecore_timer_add(4.0f, _launch_timeout, eff);
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
   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(eff, 0);

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   if (c->animatable)
     {
        switch (eff->rot)
          {
           case 90:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,off,to270", "fake");
              break;
           case 180:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,off,to0", "fake");
              break;
           case 270:
              e_mod_comp_effect_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,off,to90", "fake");
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

   ELBF(ELBT_COMP, 0, 0,
     "%15.15s|", "FAKE DISABLE");

   eff->running = EINA_FALSE;
   eff->fake_image_show_done= EINA_FALSE;
   eff->win = 0;

   e_mod_comp_util_screen_input_region_set(EINA_FALSE);
   e_mod_comp_util_grab_key_set(EINA_FALSE);

   E_Comp_Canvas *canvas = NULL;
   E_Comp_Layer *ly = NULL;

   E_Comp *c = e_mod_comp_util_get();
   if (c)
     {
        e_mod_comp_win_shape_input_update(c);
        canvas = eina_list_nth(c->canvases, 0);
     }

   if (eff->timeout)
     ecore_timer_del(eff->timeout);
   eff->timeout = NULL;

   if (eff->indicator_show)
     {
        edje_object_part_unswallow(eff->shobj, eff->indicator_obj);
        evas_object_hide(eff->indicator_obj);
        evas_object_del(eff->indicator_obj);
        eff->indicator_obj = NULL;
     }
   edje_object_part_unswallow(eff->shobj, eff->obj);

   evas_object_hide(eff->shobj);
   evas_object_hide(eff->obj);
   evas_object_del(eff->obj);
   eff->obj = NULL;

   e_layout_unpack(eff->shobj);
   if (canvas)
     ly = e_mod_comp_canvas_layer_get(canvas, "effect");

   if (ly)
     e_mod_comp_layer_effect_set(ly, EINA_FALSE);
   evas_object_data_del(eff->shobj, "comp.effect_obj.ly");
}

static Ecore_X_Window
_apptray_win_get()
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw = NULL;
   E_CHECK_RETURN(c, 0);

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if(TYPE_MINI_APPTRAY_CHECK(cw))
          return cw->win;
     }
   return 0;
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
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Layer *ly = NULL;

   E_Comp *c = e_mod_comp_util_get();

   eff = (E_Comp_Effect_Image_Launch *)data;
   E_CHECK(eff);

   e_mod_comp_util_screen_input_region_set(EINA_FALSE);
   e_mod_comp_util_grab_key_set(EINA_FALSE);

   if (eff->indicator_show)
     {
        edje_object_part_unswallow(eff->shobj, eff->indicator_obj);
        evas_object_hide(eff->indicator_obj);
        eff->indicator_obj = NULL;
     }
   edje_object_part_unswallow(eff->shobj, eff->obj);
   evas_object_hide(eff->shobj);
   evas_object_hide(eff->obj);
   eff->obj = NULL;

   e_layout_unpack(eff->shobj);
   if (c)
     {
        e_mod_comp_win_shape_input_update(c);
        canvas = eina_list_nth(c->canvases, 0);
     }

   if (canvas)
     ly = e_mod_comp_canvas_layer_get(canvas, "effect");

   if (ly)
     e_mod_comp_layer_effect_set(ly, EINA_FALSE);

   evas_object_data_del(eff->shobj, "comp.effect_obj.ly");
}

static Eina_Bool
_launch_timeout(void *data)
{
   E_Comp_Effect_Image_Launch *eff;
   E_Comp *c = e_mod_comp_util_get();
   eff = (E_Comp_Effect_Image_Launch *)data;

   E_CHECK_RETURN(c, 0);
   E_CHECK_RETURN(eff, 0);

   e_mod_comp_effect_image_launch_disable(eff);
   e_mod_comp_render_queue(c);

   return EINA_FALSE;
}
