#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "e_mod_comp_data.h"
#include "e_mod_comp_animation.h"
#include "config.h"
#include <utilX.h>

#if COMP_LOGGER_BUILD_ENABLE
extern int comp_logger_type                        ;
extern Ecore_X_Atom ATOM_CM_LOG                    ;
#endif

extern Ecore_X_Atom ATOM_EFFECT_ENABLE             ;
extern Ecore_X_Atom ATOM_EFFECT_DEFAULT            ;
extern Ecore_X_Atom ATOM_EFFECT_NONE               ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM0            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM1            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM2            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM3            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM4            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM5            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM6            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM7            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM8            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM9            ;
extern Ecore_X_Atom ATOM_FAKE_LAUNCH_IMAGE         ;

extern E_Comp_Win       * _e_mod_comp_win_transient_parent_find    (E_Comp_Win * cw);
extern E_Comp_Win       * _e_mod_comp_win_find_background_win      (E_Comp_Win * cw);

extern E_Comp           * _e_mod_comp_find                         (Ecore_X_Window root);
extern void               _e_mod_comp_win_render_queue             (E_Comp_Win *cw);
extern void               _e_mod_comp_send_window_effect_client_state( E_Comp_Win *cw, Eina_Bool effect_state);
extern Eina_Bool          _e_mod_comp_is_application_launch        (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_application_close         (E_Border *bd);
extern E_Comp_Win       * _e_mod_comp_win_find_by_indicator        (E_Comp *c );
extern Eina_Bool          _e_mod_comp_is_isf_main_window           (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_isf_sub_window            (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_utility_window            (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_indicator_window          (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_tooltip_window            (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_combo_window              (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_dnd_window                (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_popup_menu_window         (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_task_manager_window       (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_live_magazine_window       (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_lock_screen_window        (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_menu_window               (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_drop_down_menu_window     (E_Border *bd);
extern Eina_Bool          _e_mod_comp_is_splash_window             (E_Border *bd);
extern E_Comp_Win       * _e_mod_comp_win_find_fake_background_win (E_Comp *c);
extern void               _e_mod_comp_cb_pending_after             (void *data __UNUSED__, E_Manager *man __UNUSED__, E_Manager_Comp_Source *src);
extern Eina_Bool          _e_mod_comp_fake_launch_timeout          (void *data);
extern Eina_Bool          _e_mod_comp_win_check_visible            (E_Comp_Win *cw);
extern Eina_Bool          _e_mod_comp_win_check_visible3           (E_Comp_Win *cw);
extern void               _e_mod_comp_disable_touch_event_block    (E_Comp *c);
extern void               _e_mod_comp_enable_touch_event_block     (E_Comp *c);
extern void               _e_mod_comp_win_inc_animating            (E_Comp_Win *cw);
extern void               _e_mod_comp_win_dec_animating            (E_Comp_Win *cw);
static void               _e_mod_comp_background_show_effect       (E_Comp_Win *cw);
static void               _e_mod_comp_background_hide_effect       (E_Comp_Win *cw);
static void               _e_mod_comp_enable_effect_stage          (E_Comp_Win *actor_cw, E_Comp_Win *actor_cw2);
void                      _e_mod_comp_disable_fake_launch          (E_Comp *c);

void                      _e_mod_comp_window_show_effect           (E_Comp_Win *cw);
void                      _e_mod_comp_window_hide_effect           (E_Comp_Win *cw);
void                      _e_mod_comp_window_restack_effect        (E_Comp_Win *cw, E_Comp_Win *cw2);
void                      _e_mod_comp_disable_effect_stage         (E_Comp *c);
Eina_Bool                 _e_mod_comp_fake_show                    (Ecore_X_Event_Client_Message *ev);
Eina_Bool                 _e_mod_comp_fake_hide                    (Ecore_X_Event_Client_Message *ev );
E_Comp_Effect_Type        _e_mod_comp_get_effect_type              (Ecore_X_Atom *atom);
Eina_Bool                 _e_mod_comp_win_check_visible2           (E_Comp_Win *cw);
void                      _e_mod_comp_background_show_effect_for_switcher( E_Comp_Win *cw );
void                      _e_mod_comp_background_hide_effect_for_switcher( E_Comp_Win *cw );
void                      _e_mod_comp_window_lower_effect          (E_Comp_Win *cw, E_Comp_Win *cw2);

E_Comp_Effect_Type
_e_mod_comp_get_effect_type(Ecore_X_Atom *atom)
{
   if ( *atom == ATOM_EFFECT_DEFAULT )      return  COMP_EFFECT_DEFAULT;
   else if ( *atom == ATOM_EFFECT_NONE )    return  COMP_EFFECT_NONE;
   else if ( *atom == ATOM_EFFECT_CUSTOM0 ) return  COMP_EFFECT_CUSTOM0;
   else if ( *atom == ATOM_EFFECT_CUSTOM1 ) return  COMP_EFFECT_CUSTOM1;
   else if ( *atom == ATOM_EFFECT_CUSTOM2 ) return  COMP_EFFECT_CUSTOM2;
   else if ( *atom == ATOM_EFFECT_CUSTOM3 ) return  COMP_EFFECT_CUSTOM3;
   else if ( *atom == ATOM_EFFECT_CUSTOM4 ) return  COMP_EFFECT_CUSTOM4;
   else if ( *atom == ATOM_EFFECT_CUSTOM5 ) return  COMP_EFFECT_CUSTOM5;
   else if ( *atom == ATOM_EFFECT_CUSTOM6 ) return  COMP_EFFECT_CUSTOM6;
   else if ( *atom == ATOM_EFFECT_CUSTOM7 ) return  COMP_EFFECT_CUSTOM7;
   else if ( *atom == ATOM_EFFECT_CUSTOM8 ) return  COMP_EFFECT_CUSTOM8;
   else if ( *atom == ATOM_EFFECT_CUSTOM9 ) return  COMP_EFFECT_CUSTOM9;
   else                                     return  COMP_EFFECT_DEFAULT;
}

static void
_e_mod_comp_enable_effect_stage(E_Comp_Win *actor_cw, E_Comp_Win *actor_cw2)
{
   E_Comp_Win *find_cw = NULL;
   Eina_Inlist *wins_list;

   if ( ( actor_cw == NULL ) || ( actor_cw2 == NULL ) )  return;

   actor_cw->effect_stage = EINA_TRUE;
   actor_cw->c->effect_stage = EINA_TRUE;

   find_cw = actor_cw;

   while ( (wins_list = EINA_INLIST_GET(find_cw)->prev ) != NULL )
     {
        find_cw = _EINA_INLIST_CONTAINER(find_cw, wins_list );
        if ( find_cw == NULL )
          {
             fprintf( stdout, "[E17-comp] WIN_EFFECT : _e_mod_comp_enable_effect_stage() error ( find_cw == NULL ) ! \n");
             return;
          }

        if ( (find_cw->invalid ) || ( find_cw->input_only ) || ( find_cw->win == actor_cw->win )
             || (find_cw->win == actor_cw2->win ) || ( _e_mod_comp_is_indicator_window( find_cw->bd ) == EINA_TRUE  ) )
          {
             ;
          }
        else
          {
             if (evas_object_visible_get(find_cw->shobj))
               {
                  find_cw->animate_hide = EINA_TRUE; // do hide window which is not related window animation effect.
                  evas_object_hide(find_cw->shobj);
               }
          }
     }
   //show background image when background show animation is emitted.
   evas_object_stack_below(actor_cw->c->bg_img, evas_object_bottom_get(actor_cw->c->evas) );

   if ( (actor_cw->animate_hide == EINA_TRUE )
        && !evas_object_visible_get(actor_cw->shobj) )
     {
        evas_object_show(actor_cw->shobj);
        actor_cw->animate_hide = EINA_FALSE;
     }

   if ( (actor_cw2->animate_hide == EINA_TRUE )
        && !evas_object_visible_get(actor_cw2->shobj) )
     {
        evas_object_show(actor_cw2->shobj);
        actor_cw->animate_hide = EINA_FALSE;
     }

}

static void _e_mod_comp_background_show_effect(E_Comp_Win *cw)
{
   E_Comp_Win *background_cw = NULL;

   background_cw = _e_mod_comp_win_find_background_win(cw);

   if ( background_cw )
     {
        _e_mod_comp_enable_effect_stage(cw, background_cw);

        edje_object_signal_emit(background_cw->shobj, "e,state,background,visible,on", "e");
        L( LT_EFFECT, "[COMP] WIN_EFFECT : Background Show Effect signal Emit -> win:0x%08x\n",background_cw->win);

        _e_mod_comp_win_inc_animating(background_cw);
     }
}

static void _e_mod_comp_background_hide_effect(E_Comp_Win *cw)
{
   E_Comp_Win *background_cw = NULL;

   background_cw = _e_mod_comp_win_find_background_win(cw);

   if ( background_cw )
     {
        _e_mod_comp_enable_effect_stage(cw, background_cw);

        edje_object_signal_emit(background_cw->shobj, "e,state,background,visible,off", "e");
        L( LT_EFFECT, "[COMP] WIN_EFFECT : Background Hide Effect signal Emit -> win:0x%08x\n",background_cw->win);

        _e_mod_comp_win_inc_animating(background_cw);
     }
}

void
_e_mod_comp_background_show_effect_for_switcher( E_Comp_Win *cw )
{
   if(!cw) return;
   if( cw->c->animatable == EINA_FALSE) return;

   if ( cw->c->effect_stage == EINA_TRUE)
     {
        E_Comp_Win *find_cw;
        EINA_INLIST_FOREACH(cw->c->wins, find_cw)
          {
             if (find_cw->defer_raise == EINA_TRUE) _e_mod_comp_done_defer(find_cw);
          }
        _e_mod_comp_disable_effect_stage(cw->c);
     }

   cw->c->switcher = EINA_TRUE;

   int i = 0;
   Evas_Coord x, y, w, h;
   E_Comp_Win *_cw;
   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        if ( (_cw->visible == 1) && (evas_object_visible_get(_cw->shobj) == 1) && (evas_object_visible_get(_cw->obj) == 1)  &&
             (_cw->win_type != WIN_INDICATOR) && (_cw->win_type != WIN_TASK_MANAGER))
          {
             evas_object_geometry_get( _cw->shobj, &x, &y, &w, &h );
             if( w == _cw->c->screen_w && h == _cw->c->screen_h)
               {
                  i++;
                  evas_object_move(_cw->shobj, (i+1) * (-WINDOW_SPACE) + (_cw->c->screen_w), 0); // move all windows to each stack position
                  if(i == 1)
                    {
                       edje_object_signal_emit(_cw->shobj, "e,state,switcher_top,on", "e"); // top window
                       L( LT_EFFECT, "[COMP] WIN_EFFECT : Task Switcher Top signal Emit -> win:0x%08x\n",_cw->win);
                       _e_mod_comp_win_inc_animating(_cw);
                    }
                  else
                    {
                       edje_object_signal_emit(_cw->shobj, "e,state,switcher,on", "e"); // left of top window
                       L( LT_EFFECT, "[COMP] WIN_EFFECT : Task Switcher Left signal Emit -> win:0x%08x\n",_cw->win);
                       _e_mod_comp_win_inc_animating(_cw);
                    }
               }
             else
               {
                  _cw->animate_hide = EINA_TRUE;
                  evas_object_hide(_cw->shobj);
               }
          }
     }
}

void
_e_mod_comp_background_hide_effect_for_switcher( E_Comp_Win *cw )
{
   if(!cw) return;
   if( cw->c->animatable == EINA_FALSE) return;

   if ( cw->c->effect_stage == EINA_TRUE)
     {
        E_Comp_Win *find_cw;
        EINA_INLIST_FOREACH(cw->c->wins, find_cw)
          {
             if (find_cw->defer_raise == EINA_TRUE) _e_mod_comp_done_defer(find_cw);
          }
        _e_mod_comp_disable_effect_stage(cw->c);
     }

   Eina_List *l;
   E_Comp_Transfer *tr;
   EINA_LIST_FOREACH (cw->c->transfer_list, l, tr)
     {
        if (!tr) continue;
        if(tr->animator) ecore_animator_del(tr->animator);
        tr->animator = NULL;
        cw->c->transfer_list = eina_list_remove (cw->c->transfer_list, tr);
        free(tr);
     }
   int i = 0;
   Evas_Coord x, y, w, h;
   E_Comp_Win *_cw;
   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        if ( (_cw->visible == 1) && (evas_object_visible_get(_cw->shobj) == 1) && (evas_object_visible_get(_cw->obj) == 1)  &&
             (_cw->win_type != WIN_INDICATOR) && (_cw->win_type != WIN_TASK_MANAGER))
          {
             evas_object_geometry_get( _cw->shobj, &x, &y, &w, &h );
             if ( w == _cw->c->screen_w && h == _cw->c->screen_h )
               {
                  i++;
                  L( LT_EFFECT, "[COMP] WIN_EFFECT : Task Switcher Hide effect start -> win:0x%08x\n",_cw->win);
                  _e_mod_comp_win_inc_animating(_cw);
                  if(_cw->c->selected_pos == 0 || _cw->c->selected_pos == 1) // if window is not selected, skip translation stage and do rotation stage
                    {
                       E_Comp_Transfer * rotate;
                       rotate = calloc(1, sizeof(E_Comp_Transfer));
                       if (!rotate) return;
                       rotate->from = i * (- WINDOW_SPACE) + (_cw->c->screen_w -100);
                       rotate->obj = _cw->shobj;
                       rotate->duration = SWITCHER_DURATION_TOP;
                       rotate->begin_time = ecore_loop_time_get();
                       rotate->selected = EINA_FALSE;
                       if(i == 1) // top window
                         {
                            rotate->len = -(rotate->from);
                            rotate->animator = ecore_animator_add(on_animate_rotate_top, _cw);
                         }
                       else // left of top window
                         {
                            rotate->len = 0;
                            rotate->animator = ecore_animator_add(on_animate_rotate_left, _cw);
                         }
                       _cw->transfer = rotate;
                       _cw->c->transfer_list = eina_list_append (_cw->c->transfer_list, _cw->transfer);
                    }
                  else // if certain window is selected, do translation stage and rotation stage
                    {
                       E_Comp_Transfer * translate;
                       translate = calloc(1, sizeof(E_Comp_Transfer));
                       if (!translate) return;
                       if(i == 1) // top window
                         {
                            translate->selected = EINA_TRUE;
                            translate->from = cw->c->selected_pos * (-WINDOW_SPACE) + (_cw->c->screen_w -100);
                         }
                       else // left of top window
                         {
                            translate->selected = EINA_FALSE;
                            if(_cw->c->selected_pos >= i) // if left of top window is selected, other windows's position is adjusted
                              translate->from = (i-1) * (-WINDOW_SPACE) + (_cw->c->screen_w -100);
                            else
                              translate->from = i * (-WINDOW_SPACE) + (_cw->c->screen_w -100);
                         }
                       translate->len = (_cw->c->selected_pos - 1) * WINDOW_SPACE;
                       translate->obj = _cw->shobj;
                       translate->begin_time = ecore_loop_time_get();
                       translate->duration = SWITCHER_DURATION_TRANSLATE;
                       translate->animator = ecore_animator_add(on_animate_translate, _cw);
                       _cw->transfer = translate;
                       _cw->c->transfer_list = eina_list_append (_cw->c->transfer_list, _cw->transfer);
                    }
               }
             else
               {
                  _cw->animate_hide = EINA_TRUE;
                  evas_object_hide(_cw->shobj);
               }
          }
     }
}
void
_e_mod_comp_window_show_effect( E_Comp_Win *cw )
{
   if ( cw )
     {
        //cw->ready_hide_effect = EINA_TRUE;
        if ( cw->first_show_worked == EINA_FALSE )
          cw->first_show_worked = EINA_TRUE;

        if ( (cw->c->fake_launch_state == EINA_TRUE)  &&  ( cw->c->fake_win == 0)
             && ( (cw->x == 0)  && (cw->y == 0) && (cw->w == cw->c->man->w) &&  (cw->h == cw->c->man->h) )
             && ( cw->bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL) )
          {
             if ( cw->c->fake_launch_done == EINA_TRUE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,noeffect", "e"); // send_noeffect_signal
                  _e_mod_comp_disable_fake_launch(cw->c);
               }
             else
               {
                  cw->c->fake_win = cw->win;
                  return;
               }
          }
        else
          {
#if 0
             if ( !_e_mod_comp_win_check_visible2(cw) )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,noeffect", "e");
               }
             else
#endif
             if (cw->c->animatable == EINA_FALSE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,noeffect", "e");
               }
             else if ( cw->animatable == EINA_FALSE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,noeffect", "e");
               }
             else if (cw->show_effect == COMP_EFFECT_DEFAULT )
               {
                  if ( _e_mod_comp_is_application_launch(cw->bd) )
                    {
                       _e_mod_comp_background_show_effect(cw);
                    }
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
               }
             else if  ( cw->show_effect ==  COMP_EFFECT_NONE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,noeffect", "e");
               }
             else if  ( cw->show_effect ==  COMP_EFFECT_CUSTOM0 )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,custom0", "e");
               }
             else if  ( cw->show_effect ==  COMP_EFFECT_CUSTOM1 )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on,custom1", "e");
               }
#if 0
               else if ( COMP_EFFECT_CUSTOM0 )
                 {
                    edje_signal_emit (  custom0_effect )
                 }
#endif
             else
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
               }
          }
        L( LT_EFFECT, "[COMP] WIN_EFFECT : Show effect signal Emit -> win:0x%08x\n",cw->win);
        _e_mod_comp_win_inc_animating(cw);

        cw->pending_count++;
        e_manager_comp_event_src_visibility_send(cw->c->man, (E_Manager_Comp_Source *)cw, _e_mod_comp_cb_pending_after, cw->c);

        if (!cw->bd) return;
        if (cw->win_type == WIN_TASK_MANAGER)
          {
             cw->c->switcher_animating = EINA_TRUE;
             _e_mod_comp_background_show_effect_for_switcher(cw);
          }
     }
}

void
_e_mod_comp_window_hide_effect( E_Comp_Win *cw )
{
   if ( cw )
     {
        //cw->ready_hide_effect = EINA_FALSE;
#if 0
        if ( _e_mod_comp_win_check_visible2(cw) )
          {
             if ( cw->animating )
               {
                  return;
               }
             else
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off,noeffect", "e");
               }
          }
#endif
        if ( !_e_mod_comp_win_check_visible3(cw) )
          {
             edje_object_signal_emit(cw->shobj, "e,state,visible,off,noeffect", "e");
          }
        else
          {
             if ( cw->c->animatable == EINA_FALSE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off,noeffect", "e");
               }
             else if ( cw->animatable == EINA_FALSE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off,noeffect", "e");
               }
             else if (cw->hide_effect == COMP_EFFECT_DEFAULT )
               {
                  if ( !cw->animating &&  _e_mod_comp_is_application_close(cw->bd) )
                    {
                       _e_mod_comp_background_hide_effect(cw);
                    }
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off", "e");
               }
             else if ( cw->hide_effect == COMP_EFFECT_NONE )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off,noeffect", "e");
               }
             else if ( cw->hide_effect == COMP_EFFECT_CUSTOM0 )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off,custom0", "e");
               }
             else if ( cw->hide_effect == COMP_EFFECT_CUSTOM1 )
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off,custom1", "e");
               }
#if 0
             else if ( COMP_EFFECT_CUSTOM0 )
               {
                  edje_signal_emit (  custom0_effect )
               }
#endif
             else
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,off", "e");
               }
          }
        L( LT_EFFECT, "[COMP] WIN_EFFECT : Hide effect signal Emit -> win:0x%08x\n",cw->win);
        _e_mod_comp_win_inc_animating(cw);

        cw->pending_count++;
        e_manager_comp_event_src_visibility_send (cw->c->man, (E_Manager_Comp_Source *)cw, _e_mod_comp_cb_pending_after, cw->c);

        if (!cw->bd) return;
        if (cw->win_type == WIN_TASK_MANAGER)
          {
             cw->c->switcher_animating = EINA_TRUE;
             cw->c->switcher_obscured = !_e_mod_comp_win_check_visible(cw);
             if(!cw->c->switcher_obscured)
               {
                  _e_mod_comp_background_hide_effect_for_switcher(cw);
               }
             else
               {
                  L( LT_EFFECT, "[COMP] WIN_EFFECT : Task Manager is obscured. Nothing to do -> win:0x%08x\n",cw->win);
               }
          }
     }
}

void
_e_mod_comp_window_restack_effect(E_Comp_Win *cw,
                                  E_Comp_Win *cw2)
{
   E_Comp_Win *cw3, *anim_cw;
   Eina_Inlist *wins_list;

   if (!cw || !cw2) return;

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Raise Above \n");

   cw3 = _e_mod_comp_win_find_by_indicator(cw->c);
   anim_cw = cw2;

   while (anim_cw->defer_hide
          || !(anim_cw->x == 0 && anim_cw->y == 0
               && anim_cw->w == anim_cw->c->man->w
               && anim_cw->h == anim_cw->c->man->h)
          || _e_mod_comp_is_isf_main_window(anim_cw->bd)
          || _e_mod_comp_is_isf_sub_window(anim_cw->bd)
          || _e_mod_comp_is_utility_window(anim_cw->bd)
          || _e_mod_comp_is_indicator_window(anim_cw->bd)
          || _e_mod_comp_is_tooltip_window(anim_cw->bd)
          || _e_mod_comp_is_combo_window(anim_cw->bd)
          || _e_mod_comp_is_dnd_window(anim_cw->bd)
          || _e_mod_comp_is_popup_menu_window(anim_cw->bd)
          || _e_mod_comp_is_task_manager_window(anim_cw->bd)
          || _e_mod_comp_is_lock_screen_window(anim_cw->bd)
          || _e_mod_comp_is_menu_window(anim_cw->bd)
          || _e_mod_comp_is_drop_down_menu_window(anim_cw->bd)
          || _e_mod_comp_is_splash_window(anim_cw->bd))

     {
        wins_list = EINA_INLIST_GET(anim_cw)->prev;
        if (!wins_list) return;

        anim_cw = _EINA_INLIST_CONTAINER(anim_cw, wins_list);
        if (!anim_cw) return;
     }

   if (!anim_cw->visible) return;

   // if task switcher is open, just set defer_raise
   // and do jump effect in _e_mod_comp_window_hide_effect
   if (cw->c->switcher)
     {
        E_Comp_Win *find_cw;
        EINA_INLIST_REVERSE_FOREACH(cw->c->wins, find_cw)
          {
             if ( find_cw->win_type == WIN_TASK_MANAGER )
               cw->c->switcher_obscured = !_e_mod_comp_win_check_visible(find_cw);
          }
     }
   if (cw->c->switcher && !cw->c->switcher_obscured)
     {
        L(LT_EFFECT,
          "[COMP] WIN_EFFECT : Raise above Emit (Task Switcher) -> win:0x%08x\n",
          cw->win);

        cw->defer_raise = EINA_TRUE;
        if (eina_list_count(cw->c->transfer_list) > 0)
          {
             L(LT_EFFECT,
               "[COMP] WIN_EFFECT : previous Transfer_list is cleaned up. num: %d\n",
               eina_list_count(cw->c->transfer_list));
             Eina_List *l;
             E_Comp_Transfer *tr;
             EINA_LIST_FOREACH (cw->c->transfer_list, l, tr)
               {
                  if (!tr) continue;
                  if(tr->animator) ecore_animator_del(tr->animator);
                  tr->animator = NULL;
                  cw->c->transfer_list = eina_list_remove (cw->c->transfer_list, tr);
                  free(tr);
               }
             int i = 0;
             Evas_Coord x, y, w, h;
             E_Comp_Win *_cw;
             EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
               {
                  if (_cw->visible
                      && evas_object_visible_get(_cw->shobj)
                      && evas_object_visible_get(_cw->obj)
                      && (_cw->win_type != WIN_INDICATOR) && (_cw->win_type != WIN_TASK_MANAGER))
                    {
                       evas_object_geometry_get(_cw->shobj, &x, &y, &w, &h);
                       if (w == _cw->c->screen_w && h == _cw->c->screen_h)
                         {
                            i++;
                            L( LT_EFFECT, "[COMP] WIN_EFFECT : Task Switcher Hide effect start -> win:0x%08x\n",_cw->win);
                            _e_mod_comp_win_inc_animating(_cw);
                            // if window is not selected, skip translation stage
                            // and do rotation stage
                            if (_cw->c->selected_pos == 0 || _cw->c->selected_pos == 1)
                              {
                                 E_Comp_Transfer *rotate;
                                 rotate = calloc(1, sizeof(E_Comp_Transfer));
                                 if (!rotate) return;
                                 rotate->from = i * (- WINDOW_SPACE) + (_cw->c->screen_w -100);
                                 rotate->obj = _cw->shobj;
                                 rotate->duration = SWITCHER_DURATION_TOP;
                                 rotate->begin_time = ecore_loop_time_get();
                                 rotate->selected = EINA_FALSE;
                                 if (i == 1)
                                   {
                                      // top window
                                      rotate->len = -(rotate->from);;
                                      rotate->animator = ecore_animator_add(on_animate_rotate_top, _cw);
                                   }
                                 else
                                   {
                                      // left of top window
                                      rotate->len = 0;
                                      rotate->animator = ecore_animator_add(on_animate_rotate_left, _cw);
                                   }
                                 _cw->transfer = rotate;
                                 _cw->c->transfer_list = eina_list_append(_cw->c->transfer_list, _cw->transfer);
                              }
                            else
                              {
                                 // if certain window is selected,
                                 // do translation stage and rotation stage
                                 E_Comp_Transfer *translate;
                                 translate = calloc(1, sizeof(E_Comp_Transfer));
                                 if (!translate) return;
                                 if(i == 1)
                                   {
                                      // top window
                                      translate->selected = EINA_TRUE;
                                      translate->from = _cw->c->selected_pos * (-WINDOW_SPACE) + (_cw->c->screen_w -100);
                                   }
                                 else
                                   {
                                      // left of top window
                                      translate->selected = EINA_FALSE;
                                      if (_cw->c->selected_pos >= i)
                                        {
                                           // if left of top window is selected,
                                           // other windows's position is adjusted
                                           translate->from = (i-1) * (-WINDOW_SPACE) + (_cw->c->screen_w -100);
                                        }
                                      else
                                        translate->from = i * (-WINDOW_SPACE) + (_cw->c->screen_w -100);
                                   }
                                 translate->len = (_cw->c->selected_pos - 1) * WINDOW_SPACE;
                                 translate->obj = _cw->shobj;
                                 translate->begin_time = ecore_loop_time_get();
                                 translate->duration = SWITCHER_DURATION_TRANSLATE;
                                 translate->animator = ecore_animator_add(on_animate_translate, _cw);
                                 _cw->transfer = translate;
                                 _cw->c->transfer_list = eina_list_append (_cw->c->transfer_list, _cw->transfer);
                              }
                         }
                    }
               }
          }
     }
   else
     {
        if (_e_mod_comp_is_application_launch(cw->bd))
          {
             evas_object_stack_above(cw->shobj, cw2->shobj);
             L(LT_EFFECT,
               "[COMP] WIN_EFFECT : Raise above signal1 Emit -> win:0x%08x\n",
               cw->win);

             _e_mod_comp_enable_effect_stage(anim_cw, cw);

             edje_object_signal_emit(anim_cw->shobj,
                                     "e,state,background,visible,on",
                                     "e");
             _e_mod_comp_win_inc_animating(anim_cw);

             edje_object_signal_emit(cw->shobj,
                                     "e,state,visible,on",
                                     "e");

             _e_mod_comp_win_inc_animating(cw);
          }
        else if (_e_mod_comp_is_application_close(anim_cw->bd))
          {
             cw->defer_raise = EINA_TRUE;
             anim_cw->defer_raise = EINA_TRUE;
             L(LT_EFFECT,
               "[COMP] WIN_EFFECT : Raise above signal2 Emit -> win:0x%08x\n",
               anim_cw->win);

             _e_mod_comp_enable_effect_stage(cw, anim_cw);

             edje_object_signal_emit(anim_cw->shobj,
                                     "e,state,raise_above,off",
                                     "e");
             _e_mod_comp_win_inc_animating(anim_cw);

             edje_object_signal_emit(cw->shobj,
                                     "e,state,background,visible,off",
                                     "e");

             _e_mod_comp_win_inc_animating(cw);
          }
        else
          {
             evas_object_stack_above(cw->shobj, cw2->shobj);

             L(LT_EFFECT,
               "[COMP] WIN_EFFECT : Raise above signal1 Emit -> win:0x%08x\n",
               cw->win);

             edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
             _e_mod_comp_win_inc_animating(cw);
          }
     }

   if (cw3)
     {
        if (!(cw3->invalid) && (cw3->visible))
          {
             // if cw is indicator then, send indicator move effect
             if (cw3->animate_hide
                 && !evas_object_visible_get(cw3->shobj))
               {
                  evas_object_show(cw3->shobj);
                  cw3->animate_hide = EINA_FALSE;
               }
          }
     }

   L(LT_EFFECT,
     "[COMP] WIN_EFFECT : Animation raise above  [0x%x] abv [0x%x]\n",
     cw->win, anim_cw->win);
}

void
_e_mod_comp_disable_effect_stage( E_Comp *c )
{
   E_Comp_Win *find_cw = NULL;

   if ( c == NULL ) return ;

   L( LT_EFFECT, "[COMP] WIN_EFFECT : _e_mod_comp_disable_effect_stage() \n");
   c->effect_stage = EINA_FALSE;
   EINA_INLIST_FOREACH(c->wins, find_cw)
     {
        if ( find_cw->invalid )
          {
             find_cw->animate_hide = EINA_FALSE; // Do recovery state of window visibility ( when was window animation effect occured, unrelated window was hid. )
             continue;
          }
        else if (find_cw->input_only )
          {
             find_cw->animate_hide = EINA_FALSE; // Do recovery state of window visibility ( when was window animation effect occured, unrelated window was hid. )
             continue;
          }
        else if ( find_cw->visible == 0 )
          {
             find_cw->animate_hide = EINA_FALSE; // Do recovery state of window visibility ( when was window animation effect occured, unrelated window was hid. )
             continue;
          }
        else if (  (find_cw->animate_hide == EINA_FALSE) && evas_object_visible_get(find_cw->shobj) )
          {
             continue;
          }
        else if (  (find_cw->first_show_worked == EINA_FALSE ) && !evas_object_visible_get(find_cw->shobj) )
          {
             continue;
          }
        else
          {
             find_cw->animate_hide = EINA_FALSE; // Do recovery state of window visibility ( when was window animation effect occured, unrelated window was hid. )
             evas_object_show(find_cw->shobj);
          }
     }
}

Eina_Bool
_e_mod_comp_fake_show(Ecore_X_Event_Client_Message *ev )
{
   int grab_result;
   unsigned int effect_enable = 0;

   char *fake_img_file = NULL;
   E_Comp *c = NULL;
   E_Comp_Win *cw = NULL;
   E_Comp_Win *background_cw = NULL;
   E_Comp_Win *find_cw = NULL; // for animate_hide window

   if ( ev == NULL ) return ECORE_CALLBACK_PASS_ON;

   c = _e_mod_comp_find(ev->win);
   if (!c) return ECORE_CALLBACK_PASS_ON;
   L( LT_EFFECT, "[COMP] Message - ATOM_FAKE_LAUNCH : Show EFFECT\n");

   effect_enable = 0;
   ecore_x_window_prop_card32_get(c->man->root,  ATOM_EFFECT_ENABLE, &effect_enable, 1);
   if ( effect_enable == 0 )  // if effect_enable is 0 ,then do not show fake image
     return ECORE_CALLBACK_PASS_ON;

   // if previous fake is running, then don't launch fake show
   if ( c->fake_launch_state == EINA_TRUE ) return ECORE_CALLBACK_PASS_ON;

   // grab hardware key when fake launch effect is running
   grab_result = utilx_grab_key(ecore_x_display_get(), c->win , KEY_SELECT, EXCLUSIVE_GRAB);

   if (EXCLUSIVE_GRABBED_ALREADY == grab_result)
     {
        printf("Failed to grab a key (keyname=%s) for EXCLUSIVE_GRAB !\n", KEY_SELECT);
        return ECORE_CALLBACK_PASS_ON;
     }

   grab_result = utilx_grab_key(ecore_x_display_get(), c->win , KEY_VOLUMEUP , EXCLUSIVE_GRAB);
   if (EXCLUSIVE_GRABBED_ALREADY == grab_result)
     {
        printf("Failed to grab a key (keyname=%s) for EXCLUSIVE_GRAB !\n", KEY_VOLUMEUP );
        return ECORE_CALLBACK_PASS_ON;
     }

   grab_result = utilx_grab_key(ecore_x_display_get(), c->win ,KEY_VOLUMEDOWN, EXCLUSIVE_GRAB);
   if (EXCLUSIVE_GRABBED_ALREADY == grab_result)
     {
        printf("Failed to grab a key (keyname=%s) for EXCLUSIVE_GRAB !\n", KEY_VOLUMEDOWN);
        return ECORE_CALLBACK_PASS_ON;
     }

   grab_result = utilx_grab_key(ecore_x_display_get(), c->win , KEY_CAMERA, EXCLUSIVE_GRAB);
   if (EXCLUSIVE_GRABBED_ALREADY == grab_result)
     {
        printf("Failed to grab a key (keyname=%s) for EXCLUSIVE_GRAB !\n", KEY_CAMERA);
        return ECORE_CALLBACK_PASS_ON;
     }

   c->fake_launch_state = EINA_TRUE;
   c->fake_img_obj = evas_object_image_add(c->evas);
   fake_img_file = ecore_x_window_prop_string_get(ev->win, ATOM_FAKE_LAUNCH_IMAGE);
   if (fake_img_file == NULL)
     {
        fprintf(stdout,"[COMP] Message - ATOM_FAKE_LAUNCH_IMAGE NAME is NULL\n");
        c->fake_launch_state = EINA_FALSE;
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_SELECT);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_VOLUMEUP);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_VOLUMEDOWN);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_CAMERA);
        return ECORE_CALLBACK_PASS_ON;
     }

   evas_object_image_file_set(c->fake_img_obj, fake_img_file, NULL );

   if ( evas_object_image_load_error_get (c->fake_img_obj) != EVAS_LOAD_ERROR_NONE )
     {
        printf("Failed to Load Fake Launch Image : %s !\n", fake_img_file);
        c->fake_launch_state = EINA_FALSE;
        free(fake_img_file);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_SELECT);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_VOLUMEUP);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_VOLUMEDOWN);
        utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_CAMERA);
        return ECORE_CALLBACK_PASS_ON;
     }
   free(fake_img_file);

   evas_object_image_size_get(c->fake_img_obj, &(c->fake_img_w), &(c->fake_img_h) );
   evas_object_image_fill_set(c->fake_img_obj, 0, 0, c->fake_img_w, c->fake_img_h);
   evas_object_image_filled_set(c->fake_img_obj, EINA_TRUE);
   if (!edje_object_part_swallow(c->fake_img_shobj, "fake.swallow.content", c->fake_img_obj))
     fprintf( stdout, "[E17-comp] Fake Image didn't swallowed!\n");

   evas_object_show(c->fake_img_obj);

   cw = _e_mod_comp_win_find_by_indicator(c);

   if (cw == NULL)
     {
        fprintf( stdout, "[E17-comp] FAKE_EFFECT : ( indicator window == NULL) !\n");
        evas_object_raise(c->fake_img_shobj);
     }
   else
     {
        evas_object_stack_below(c->fake_img_shobj, cw->shobj);
     }

   // background show effect
   background_cw = _e_mod_comp_win_find_fake_background_win(c);
   if ( background_cw )
     {
        EINA_INLIST_FOREACH(c->wins, find_cw)
          {
             if ( (find_cw->invalid ) || ( find_cw->input_only ) || (find_cw->win == background_cw->win ) || ( _e_mod_comp_is_indicator_window( find_cw->bd ) == EINA_TRUE  ) )
               {
                  continue;
               }
             else
               {
                  if (evas_object_visible_get(find_cw->shobj))
                    {
                       find_cw->animate_hide = EINA_TRUE; // do hide window which is not related window animation effect.
                       evas_object_hide(find_cw->shobj);
                    }
               }
          }
        //show background image when background show animation is emitted.
        evas_object_stack_below(c->bg_img, evas_object_bottom_get(c->evas) );

        background_cw->animate_hide = EINA_FALSE;

        edje_object_signal_emit(background_cw->shobj, "e,state,fake,background,visible,on", "e");
        L( LT_EFFECT, "[COMP] WIN_EFFECT : FAKE Background Show Effect signal Emit -> win:0x%08x\n",background_cw->win);

        _e_mod_comp_win_inc_animating(background_cw);

     }

   // background show effect
   evas_object_show(c->fake_img_shobj);
   edje_object_signal_emit(c->fake_img_shobj, "fake,state,visible,on", "fake");
   L( LT_EFFECT, "[COMP] WIN_EFFECT : Fake Show signal Emit\n");

   //fake hide timer add
   c->fake_launch_timeout = ecore_timer_add( 10.0f , _e_mod_comp_fake_launch_timeout, c);  // 10.0f --> 10 sec

   _e_mod_comp_enable_touch_event_block(c);

   return ECORE_CALLBACK_PASS_ON;
}

Eina_Bool
_e_mod_comp_fake_hide(Ecore_X_Event_Client_Message *ev )
{
   unsigned int effect_enable = 0;

   E_Comp *c = NULL;
   E_Comp_Win *background_cw = NULL;
   E_Comp_Win *find_cw = NULL; // for animate_hide window

   if ( ev == NULL ) return ECORE_CALLBACK_PASS_ON;
   c = _e_mod_comp_find(ev->win);

   if (!c) return ECORE_CALLBACK_PASS_ON;

   effect_enable = 0;
   ecore_x_window_prop_card32_get(c->man->root,  ATOM_EFFECT_ENABLE, &effect_enable, 1);
   if ( effect_enable == 0 )  // if effect_enable is 0 ,then do not show fake image
     return ECORE_CALLBACK_PASS_ON;

   L( LT_EFFECT, "[COMP] Message - ATOM_FAKE_LAUNCH : Hide EFFECT win:0x08%x\n", ev->win);
   if (c->fake_launch_state == EINA_TRUE)
     {
        if (c->fake_launch_timeout) // del fake launch timeout timer
          {
             ecore_timer_del(c->fake_launch_timeout);
             c->fake_launch_timeout  = NULL;
          }

        // background hide effect
        background_cw = _e_mod_comp_win_find_fake_background_win(c);
        if ( background_cw )
          {
             EINA_INLIST_FOREACH(c->wins, find_cw)
               {
                  if ( (find_cw->invalid ) || ( find_cw->input_only ) || (find_cw->win == background_cw->win ) || ( _e_mod_comp_is_indicator_window( find_cw->bd ) == EINA_TRUE  ) )
                    {
                       continue;
                    }
                  else
                    {
                       if (evas_object_visible_get(find_cw->shobj))
                         {
                            find_cw->animate_hide = EINA_TRUE; // do hide window which is not related window animation effect.
                            evas_object_hide(find_cw->shobj);
                         }
                    }
               }
             //show background image when background show animation is emitted.
             evas_object_stack_below(c->bg_img, evas_object_bottom_get(c->evas) );

             background_cw->animate_hide = EINA_FALSE;

             edje_object_signal_emit(background_cw->shobj, "e,state,background,visible,off", "e");
             L( LT_EFFECT, "[COMP] WIN_EFFECT : Background Show Effect signal Emit -> win:0x%08x\n",background_cw->win);

             _e_mod_comp_win_inc_animating(background_cw);

          }
        // background show effect

        edje_object_signal_emit(c->fake_img_shobj, "fake,state,visible,off", "fake");
        L( LT_EFFECT, "[COMP] WIN_EFFECT : Fake Hide signal Emit\n");
     }
   return ECORE_CALLBACK_PASS_ON;
}

void
_e_mod_comp_disable_fake_launch( E_Comp *c)
{
   if ( c )
     {
        if ( c->fake_launch_state == EINA_TRUE )
          {
             c->fake_launch_state = EINA_FALSE;
             c->fake_win = 0;
             c->fake_launch_done = EINA_FALSE;

             _e_mod_comp_disable_touch_event_block(c);

             utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_SELECT);
             utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_VOLUMEUP);
             utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_VOLUMEDOWN);
             utilx_ungrab_key(ecore_x_display_get(), c->win , KEY_CAMERA);

             if (  c->fake_launch_timeout ) // del fake launch timeout timer
               {
                  ecore_timer_del(c->fake_launch_timeout);
                  c->fake_launch_timeout  = NULL;
               }

             evas_object_hide(c->fake_img_shobj);
             edje_object_part_unswallow( c->fake_img_shobj, c->fake_img_obj );
             evas_object_del(c->fake_img_obj);
          }
     }
}

void _e_mod_comp_window_lower_effect(E_Comp_Win *cw, E_Comp_Win *cw2)
{
   cw->defer_raise = EINA_TRUE;

   if ( _e_mod_comp_is_application_close(cw->bd) )
     {
        _e_mod_comp_enable_effect_stage(cw2, cw);
        edje_object_signal_emit(cw2->shobj, "e,state,background,visible,off", "e");
        L( LT_EFFECT, "[COMP] WIN_EFFECT : Background Hide Effect signal Emit -> win:0x%08x\n",cw2->win);
        _e_mod_comp_win_inc_animating(cw2);
     }
   edje_object_signal_emit(cw->shobj, "e,state,raise_above,off", "e");
   L( LT_EFFECT, "[COMP] WIN_EFFECT : Raise Above Hide Effect signal Emit -> win:0x%08x\n",cw->win);
   _e_mod_comp_win_inc_animating(cw);
}
