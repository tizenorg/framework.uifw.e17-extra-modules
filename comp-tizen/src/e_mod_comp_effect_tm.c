#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_tm.h"

/* local subsystem functions */
static void      _defer_raise_clear(void);
static Eina_Bool _valid_win_check(E_Comp_Win *cw);
static Eina_Bool _win_size_check(E_Comp_Win *cw);
static Eina_Bool _win_bg_item_init(E_Comp *c);

/* local subsystem globals */

/* externally accessible functions */
EINTERN Eina_Bool
e_mod_comp_effect_tm_bg_show(E_Comp_Win *cw)
{
   E_Comp_Win *_cw;
   E_Comp_Object *co;
   int i = 0, _x = 0;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->animatable, 0);

   _defer_raise_clear();
   e_mod_comp_animation_transfer_list_clear();
   cw->c->switcher_animating = EINA_TRUE;
   cw->c->selected_pos = 0;

   L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s\n", "EFF",
     e_mod_comp_util_client_xid_get(cw), "BG_SHOW_TM");

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        if (!_cw) continue;
        if (!_win_size_check(_cw)) continue;
        if (!_valid_win_check(_cw)) continue;

        i++;

        L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s [%d]\n", "EFF",
          e_mod_comp_util_client_xid_get(_cw), "BG_SHOW_TM", i);

        // move all windows to each stack position
        _x = (i + 1) * (-WINDOW_SPACE) + (_cw->c->man->w);
        co = eina_list_data_get(cw->objs);
        if (!co) continue;
        evas_object_move(co->shadow, _x, 0);

        if (i == 1)
          {
             // top window
             e_mod_comp_effect_signal_add
               (_cw, NULL, "e,state,switcher_top,on", "e");
          }
        else
          {
             // left of top window
             e_mod_comp_effect_signal_add
               (_cw, NULL, "e,state,switcher,on", "e");
          }
     }

   cw->c->switcher = EINA_TRUE;
   cw->c->switcher_obscured = !(e_mod_comp_util_win_visible_get(cw));

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_tm_bg_hide(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->animatable, 0);

   if (cw->c->switcher_obscured) return EINA_TRUE;

   e_mod_comp_animation_transfer_list_clear();
   cw->c->switcher_animating = EINA_TRUE;

   L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s\n", "EFF",
     e_mod_comp_util_client_xid_get(cw), "BG_HIDE_TM");

   _win_bg_item_init(cw->c);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_tm_state_update(E_Comp *c)
{
   E_Comp_Win *_cw;
   E_CHECK_RETURN(c->switcher, 0);
   Eina_Bool res = EINA_FALSE;

   // if task switcher is open, just set defer_raise
   // and do jump effect in _e_mod_comp_window_hide_effect
   EINA_INLIST_REVERSE_FOREACH(c->wins, _cw)
     {
        if (!_cw) continue;
        if (TYPE_TASKMANAGER_CHECK(_cw))
          {
             // just return without visibility update
             // while task switcher is closing
             if (_cw->defer_hide) break;
             if (_cw->c->switcher_animating) break;

             c->switcher_obscured = !(e_mod_comp_util_win_visible_get(_cw));
             res = EINA_TRUE;
             break;
          }
     }

   L(LT_EFFECT, "[COMP] %18.18s tm:%d obscured:%d\n",
     "EFF", c->switcher, c->switcher_obscured);

   return res;
}

EINTERN Eina_Bool
e_mod_comp_effect_tm_raise_above(E_Comp_Win *cw,
                                 E_Comp_Win *cw2)
{
   Eina_Bool raise;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);

   cw->defer_raise = EINA_TRUE;
   raise = e_mod_comp_policy_win_restack_check(cw, cw2);
   E_CHECK_RETURN(raise, 0);

   cw->c->switcher_animating = EINA_TRUE;

   e_mod_comp_animation_transfer_list_clear();

   L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s sel:%d defer_raise:1\n",
     "EFF", e_mod_comp_util_client_xid_get(cw), "RAISE_ABOVE_TM",
     cw->c->selected_pos);

   _win_bg_item_init(cw->c);
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_tm_handler_show_done(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_Comp_Win *_cw;

   cw->c->switcher_animating = EINA_FALSE;

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        if (!_cw) continue;
        if (!_cw->visible) continue;
        e_mod_comp_win_comp_objs_move(_cw, _cw->x, _cw->y);
        e_mod_comp_win_comp_objs_resize(_cw,
                                        _cw->pw + (_cw->border * 2),
                                        _cw->ph + (_cw->border * 2));
        _cw->defer_move_resize = EINA_FALSE;
     }
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_tm_handler_hide_done(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_Comp_Win *_cw;
   if (cw->c->selected_pos > 0) cw->c->selected_pos = 0;

   // task switcher is closed
   cw->c->switcher = EINA_FALSE;
   cw->c->switcher_obscured = EINA_TRUE;
   cw->c->switcher_animating = EINA_FALSE;

   EINA_INLIST_FOREACH(cw->c->wins, _cw)
     {
        if (!_cw) continue;
        if (!_cw->visible) continue;
        if (!_cw->defer_move_resize) continue;
        e_mod_comp_win_comp_objs_move(_cw, _cw->x, _cw->y);
        e_mod_comp_win_comp_objs_resize(_cw,
                                        _cw->pw + (_cw->border * 2),
                                        _cw->ph + (_cw->border * 2));
        _cw->defer_move_resize = EINA_FALSE;
        L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s\n", "EFF",
          e_mod_comp_util_client_xid_get(_cw), "HIDE_DONE_BG_ITEM");
     }

   _defer_raise_clear();

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_effect_tm_handler_raise_above_pre(E_Comp_Win *cw,
                                             E_Comp_Win *cw2)
{
   E_CHECK_RETURN(cw, 0);
   int i = 0;
   E_Comp_Win *_cw;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->man, 0);
   E_CHECK_RETURN(cw->c->switcher, 0);

   e_mod_comp_effect_tm_state_update(cw->c);
   E_CHECK_RETURN((!cw->c->switcher_obscured), 0);

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        if ((_cw->visible) &&
            (!TYPE_INDICATOR_CHECK(cw)) &&
            (!TYPE_TASKMANAGER_CHECK(cw)) &&
            SIZE_EQUAL_TO_ROOT(_cw))
          {
             i++;
             if (_cw == cw)
               {
                  cw->c->selected_pos = i;
                  break;
               }
          }
     }

   L(LT_EFFECT, "[COMP] %18.18s w:0x%08x sel:%d\n", "EFF",
     e_mod_comp_util_client_xid_get(cw), cw->c->selected_pos);

   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append_relative(cw->c->wins,
                                             EINA_INLIST_GET(cw),
                                             EINA_INLIST_GET(cw2));
   return EINA_TRUE;
}

/* local subsystem functions */
static void
_defer_raise_clear(void)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw;
   E_CHECK(c);

   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (!cw) continue;
        if (cw->defer_raise)
          {
             L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s\n",
               "EFF", e_mod_comp_util_client_xid_get(cw),
               "TM_DEFER_RAISE_CLEAR");
             e_mod_comp_done_defer(cw);
          }
     }
   e_mod_comp_effect_disable_stage(c, NULL);
}

static Eina_Bool
_valid_win_check(E_Comp_Win *cw)
{
   E_Comp_Object *co = eina_list_data_get(cw->objs);
   E_CHECK_RETURN(co, 0);
   if (cw->visible &&
       evas_object_visible_get(co->shadow) &&
       evas_object_visible_get(co->img) &&
       !(TYPE_INDICATOR_CHECK(cw)) &&
       !(TYPE_QUICKPANEL_CHECK(cw)) &&
       !(TYPE_TASKMANAGER_CHECK(cw)) &&
       !(STATE_INSET_CHECK(cw)))
     {
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
_win_size_check(E_Comp_Win *cw)
{
   if (SIZE_EQUAL_TO_ROOT(cw))
     return EINA_TRUE;

   if (!TYPE_NORMAL_CHECK(cw))
     return EINA_FALSE;

   cw->animate_hide = EINA_TRUE;
   e_mod_comp_win_comp_objs_hide(cw);
   return EINA_FALSE;
}

static Eina_Bool
_win_bg_item_init(E_Comp *c)
{
   E_Comp_Win *_cw;
   E_Comp_Transfer *tr;
   E_Comp_Object *co;
   int i = 0;

   EINA_INLIST_REVERSE_FOREACH(c->wins, _cw)
     {
        if (!_cw) continue;
        if (!_win_size_check(_cw)) continue;
        if (!_valid_win_check(_cw)) continue;

        i++;

        L(LT_EFFECT, "[COMP] %18.18s w:0x%08x %s [%d/%d]\n", "EFF",
          e_mod_comp_util_client_xid_get(_cw), "ADD_TM_BG_ITEM",
          i, c->selected_pos);

        e_mod_comp_effect_animating_set(c, _cw, EINA_TRUE);

        tr = e_mod_comp_animation_transfer_new();
        E_CHECK_GOTO(tr, error);

        _cw->transfer = tr;
        co = eina_list_data_get(_cw->objs);
        if (!co) continue;
        tr->obj = co->shadow;

        if ((c->selected_pos == 0) ||
            (c->selected_pos == 1))
          {
             // if window is not selected, skip translation stage
             // and do rotation stage
             tr->from = i * (- WINDOW_SPACE) + (c->man->w -100);
             tr->duration = SWITCHER_DURATION_TOP;
             tr->begin_time = ecore_loop_time_get();
             if (i == 1)
               {
                  // top window
                  tr->len = -(tr->from);
                  tr->animator = ecore_animator_add
                    (e_mod_comp_animation_on_rotate_top, _cw);
               }
             else
               {
                  // left of top window
                  tr->len = 0;
                  tr->animator = ecore_animator_add
                    (e_mod_comp_animation_on_rotate_left, _cw);
               }
          }
        else
          {
             // if certain window is selected, do translation stage
             // and rotation stage
             tr->len = (c->selected_pos - 1) * WINDOW_SPACE;
             tr->begin_time = ecore_loop_time_get();
             tr->duration = SWITCHER_DURATION_TRANSLATE;
             tr->animator = ecore_animator_add(e_mod_comp_animation_on_translate, _cw);
             if (i == 1)
               {
                  // top window
                  tr->selected = EINA_TRUE;
                  tr->from = c->selected_pos * (-WINDOW_SPACE) + (c->man->w -100);
               }
             else
               {
                  // left of top window
                  // if left of top window is selected,
                  // other windows's position is adjusted
                  if (c->selected_pos >= i)
                    tr->from = (i-1) * (-WINDOW_SPACE) + (c->man->w -100);
                  else
                    tr->from = i * (-WINDOW_SPACE) + (c->man->w -100);
               }
          }
     }

   return EINA_TRUE;

error:
   e_mod_comp_animation_transfer_list_clear();
   return EINA_FALSE;
}
