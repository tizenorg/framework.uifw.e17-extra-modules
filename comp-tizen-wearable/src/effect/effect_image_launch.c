#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_image_launch.h"

#include "effect.h"
#include "effect_image_launch.h"
#include "effect_win_rotation.h"

#define FAKE_TABLE_PATH "/usr/share/themes/LaunchColorTable.xml"
#define APP_DEFINE_GROUP_NAME "effect"

/* local subsystem functions */
static void      _show_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static void      _hide_done(void *data, Evas_Object *obj, const char *emission, const char *source);
static Eina_Bool _launch_timeout(void *data);
/* local subsystem globals */

/* externally accessible functions */
E_Comp_Effect_Image_Launch *
_effect_mod_image_launch_new(Evas *e,
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
#ifdef _E_USE_EFL_ASSIST_
   ea_theme_changeable_ui_enabled_set(EINA_TRUE);

   eff->theme_table = ea_theme_color_table_new(FAKE_TABLE_PATH);
#endif
   return eff;

error:
   _effect_mod_image_launch_free(eff);
   return NULL;
}

void
_effect_mod_image_launch_free(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK(eff);
   if (eff->shobj) evas_object_del(eff->shobj);
   if (eff->obj) evas_object_del(eff->obj);
#ifdef _E_USE_EFL_ASSIST_
   if (eff->theme_table) ea_theme_color_table_free(eff->theme_table);
#endif
   E_FREE(eff);
}

Eina_Bool
_effect_mod_image_launch_handler_message(Ecore_X_Event_Client_Message *ev)
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

   eff->file_type = ev->data.l[0]; //fake effect file type
   eff->rot = ev->data.l[1]; //rotation value
   eff->indicator_show = ev->data.l[2]; //indicator flag value
   eff->theme_type = ev->data.l[3]; //fake effect theme type

   file = ecore_x_window_prop_string_get
        (ev->win, ATOM_IMAGE_LAUNCH_FILE);
   E_CHECK_RETURN(file, 0);

   ELBF(ELBT_COMP, 0, 0,
        "%15.15s| type:%d, ROT:%d, Indi:%d, theme:%d", "FAKE SHOW",
        eff->file_type, eff->rot, eff->indicator_show, eff->theme_type);

   ELBF(ELBT_COMP, 0, 0,
        "%15.15s| File:%s", "FAKE SHOW", file);


   _effect_mod_image_launch_show(eff, file);
   E_FREE(file);

   return EINA_TRUE;
}

Eina_Bool
_effect_mod_image_launch_show(E_Comp_Effect_Image_Launch *eff,
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
   E_CHECK_RETURN(!(eff->running), 0);

   grabbed = e_mod_comp_util_grab_key_set(EINA_TRUE);
   E_CHECK_RETURN(grabbed, 0);

   eff->running = EINA_TRUE;

   bg_cw = e_mod_comp_util_win_normal_get(NULL, EINA_TRUE);
   if (bg_cw)
     {
        E_CHECK_GOTO(!TYPE_LOCKSCREEN_CHECK(bg_cw), error);
        _effect_mod_object_win_set(bg_cw, "e,state,background,visible,on");
     }

   if (eff->file_type == E_FAKE_EFFECT_FILE_TYPE_IMAGE)
     {
        eff->obj = evas_object_image_add(eff->evas);
        E_CHECK_GOTO(eff->obj, error);
        evas_object_image_file_set(eff->obj, file, NULL);
        err = evas_object_image_load_error_get(eff->obj);
        E_CHECK_GOTO(err == EVAS_LOAD_ERROR_NONE, error);

        evas_object_image_size_get(eff->obj, &(img_w), &(img_h));
        evas_object_image_fill_set(eff->obj, 0, 0, img_w, img_h);
        evas_object_image_filled_set(eff->obj, EINA_TRUE);
        evas_object_show(eff->obj);
     }
   else
     {
        char *path = NULL;

        if (file)
          path = strtok(file, ":");
        E_CHECK_GOTO(path, error);

        eff->obj = edje_object_add(eff->evas);
        E_CHECK_GOTO(eff->obj, error);
        edje_object_file_set (eff->obj, path, APP_DEFINE_GROUP_NAME);

        evas_object_move(eff->obj, 0, 0);
        evas_object_resize(eff->obj, eff->w, eff->h);
        evas_object_show(eff->obj);
        evas_object_lower(eff->obj);

#ifdef _E_USE_EFL_ASSIST_
        ea_theme_object_changeable_ui_enabled_set(eff->obj, EINA_TRUE);

        if (eff->theme_table)
          {
             if (eff->theme_type == E_FAKE_EFFECT_THEME_DARK)
               ea_theme_colors_set(eff->theme_table, EA_THEME_STYLE_DARK);
             else if (eff->theme_type == E_FAKE_EFFECT_THEME_LIGHT)
               ea_theme_colors_set(eff->theme_table, EA_THEME_STYLE_LIGHT);
             else if (eff->theme_type == E_FAKE_EFFECT_THEME_DEFAULT)
               ea_theme_colors_set(eff->theme_table, EA_THEME_STYLE_DEFAULT);
          }

        eff->app_table = NULL;
        path = strtok(NULL, ":");
        if (path)
          {
             eff->app_table = ea_theme_color_table_new(path);
             if (eff->app_table)
               {
                  if (eff->theme_type == E_FAKE_EFFECT_THEME_DARK)
                    ea_theme_colors_set(eff->app_table, EA_THEME_STYLE_DARK);
                  else if (eff->theme_type == E_FAKE_EFFECT_THEME_LIGHT)
                    ea_theme_colors_set(eff->app_table, EA_THEME_STYLE_LIGHT);
                  else if (eff->theme_type == E_FAKE_EFFECT_THEME_DEFAULT)
                    ea_theme_colors_set(eff->app_table, EA_THEME_STYLE_DEFAULT);
               }
          }
#endif
     }

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
     _effect_mod_above_wins_set(bg_cw, EINA_TRUE);

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
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,on,from270", "fake");
              break;
           case 180:
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,on,from0", "fake");
              break;
           case 270:
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,on,from90", "fake");
              break;
           default:
        _effect_mod_signal_add
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
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,90,on,noeffect", "fake");
              break;
           case 180:
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,180,on,noeffect", "fake");
              break;
           case 270:
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,270,on,noeffect", "fake");
              break;
           default:
        _effect_mod_signal_add
          (NULL, eff->shobj,
          "fake,state,visible,on,noeffect", "fake");
              break;
          }
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

Eina_Bool
_effect_mod_image_launch_hide(E_Comp_Effect_Image_Launch *eff)
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
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,off,to270", "fake");
              break;
           case 180:
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,off,to0", "fake");
              break;
           case 270:
              _effect_mod_signal_add
                (NULL, eff->shobj,
                "fake,state,visible,off,to90", "fake");
              break;
           default:
     _effect_mod_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off", "fake");
              break;
          }
     }
   else
     _effect_mod_signal_add
       (NULL, eff->shobj,
       "fake,state,visible,off,noeffect", "fake");

   e_mod_comp_render_queue(c);
   return EINA_TRUE;
}

Eina_Bool
_effect_mod_image_launch_window_check(E_Comp_Effect_Image_Launch *eff, E_Comp_Win *cw)
{
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(eff->running, 0);
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bd, 0);

   if ( REGION_EQUAL_TO_ROOT(cw) && TYPE_NORMAL_CHECK(cw))
     {
        return EINA_TRUE;
     }
   else if ((e_config->use_tiled_desk_layout) &&
             (cw->bd->client.e.state.ly.support) &&
             TYPE_NORMAL_CHECK(cw))
     {
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

Eina_Bool
_effect_mod_image_launch_running_check(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK_RETURN(eff, 0);
   return eff->running;
}

Eina_Bool
_effect_mod_image_launch_fake_show_done_check(E_Comp_Effect_Image_Launch *eff)
{
   E_CHECK_RETURN(eff, 0);
   return eff->fake_image_show_done;
}

Eina_Bool
_effect_mod_image_launch_window_set(E_Comp_Effect_Image_Launch *eff,
                                          Ecore_X_Window w)
{
   E_CHECK_RETURN(eff, 0);
   E_CHECK_RETURN(w, 0);
   eff->win = w;
   return EINA_TRUE;
}

void
_effect_mod_image_launch_disable(E_Comp_Effect_Image_Launch *eff)
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

#ifdef _E_USE_EFL_ASSIST_
   if (eff->app_table)
     {
        ea_theme_color_table_free(eff->app_table);
        eff->app_table = NULL;
     }
#endif

   if (canvas)
     ly = e_mod_comp_canvas_layer_get(canvas, "effect");

   if (ly)
     {
        e_mod_comp_layer_effect_set(ly, EINA_FALSE);
        evas_object_hide(ly->layout);
     }
   evas_object_data_del(eff->shobj, "comp.effect_obj.ly");
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

   _effect_mod_signal_add
     (cw, NULL, "e,state,visible,on,noeffect", "e");
   e_mod_comp_comp_event_src_visibility_send(cw);
   _effect_mod_image_launch_disable(eff);

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

   _effect_mod_image_launch_disable(eff);
   e_mod_comp_render_queue(c);

   return EINA_FALSE;
}