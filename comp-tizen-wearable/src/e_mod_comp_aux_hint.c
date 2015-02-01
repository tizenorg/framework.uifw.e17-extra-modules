#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"

typedef struct _E_Comp_Aux_Hint_Item E_Comp_Aux_Hint_Item;

struct _E_Comp_Aux_Hint_Item
{
   /* val[0] previous value, val[1] current value */
   union
     {
        Eina_Bool b;
        int       i;
     } val[2];
   Eina_Bool changed;
};

typedef enum
{
   AUX_HINT_NAME_EFFECT_ENABLE =0,
   AUX_HINT_NAME_DIM_BG_ENABLE,
   AUX_HINT_NAME_EFFECT_LAUNCH_ENABLE,
   AUX_HINT_NAME_EFFECT_LAUNCH_POS,
   AUX_HINT_NAME_EFFECT_CLOSE_POS,
   AUX_HINT_NAME_EFFECT_RENDER_COPY
};

static const char *hint_names[] =
{
   "wm.comp.win.effect.enable",
   "wm.comp.win.dim_bg.enable",
   "wm.comp.win.effect.launch.enable",
   "wm.comp.win.effect.launch.pos",
   "wm.comp.win.effect.close.pos",
   "wm.comp.win.render.copy"
};

/* externally accessible functions */
EAPI int
e_mod_comp_aux_hint_init(void)
{
   int i, n;
   n = (sizeof(hint_names) / sizeof(char *));

   Eina_List *l;
   E_Manager *man;
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        for (i = 0; i < n; i++)
          {
             e_hints_aux_hint_supported_add(man->root, hint_names[i]);
          }
     }

   return 1;
}

EAPI int
e_mod_comp_aux_hint_shutdown(void)
{
   return 1;
}

#define _STR_CMP(a, b) (strncmp(a, b, strlen(b)) == 0)

#define _VAL_CHECK(a, b, c, d, e) \
   else if (_STR_CMP(a, b))       \
     {                            \
        c = d;                    \
        e = EINA_TRUE;            \
        send = EINA_TRUE;         \
     }

EAPI void
e_mod_comp_aux_hint_eval(void *data __UNUSED__,
                         void      *data2)
{
   E_Border *bd = (E_Border *)data2;
   E_CHECK(bd);

   E_Comp_Win *cw = e_mod_comp_win_find(bd->win);
   E_CHECK(cw);

   E_Comp_Aux_Hint_Item eff;
   eff.val[0].b = cw->c->effect_funcs.state_get(cw->eff_type);
   eff.val[1].b = EINA_FALSE;
   eff.changed = EINA_FALSE;

   E_Comp_Aux_Hint_Item dim;
   dim.val[0].b = EINA_TRUE; /* TODO: make e,state,dim,on */
   dim.val[1].b = EINA_FALSE;
   dim.changed = EINA_FALSE;

   E_Comp_Aux_Hint_Item launch_eff;
   launch_eff.val[0].i = cw->eff_launch_style;
   launch_eff.val[1].i = 0;
   launch_eff.changed = EINA_FALSE;

   E_Comp_Aux_Hint_Item close_eff;
   close_eff.val[0].i = cw->eff_close_style;
   close_eff.val[1].i = 0;
   close_eff.changed = EINA_FALSE;

   E_Comp_Aux_Hint_Item render_copy;
   render_copy.val[0].i = cw->skip_blend;
   render_copy.val[1].i = 0;
   render_copy.changed = EINA_FALSE;


   E_Border_Aux_Hint *hint;
   Eina_List *l;
   EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
     {
        Eina_Bool send = EINA_FALSE;
        if (!hint->deleted)
          {
             if (_STR_CMP(hint->hint, hint_names[AUX_HINT_NAME_EFFECT_ENABLE]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "0", eff.val[1].b, EINA_FALSE, eff.changed)
                  _VAL_CHECK(hint->val, "1", eff.val[1].b, EINA_TRUE,  eff.changed)
               }
             else if (_STR_CMP(hint->hint, hint_names[AUX_HINT_NAME_DIM_BG_ENABLE]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "0", dim.val[1].b, EINA_FALSE, dim.changed)
                  _VAL_CHECK(hint->val, "1", dim.val[1].b, EINA_TRUE,  dim.changed)
               }
             else if (_STR_CMP(hint->hint, hint_names[AUX_HINT_NAME_EFFECT_LAUNCH_POS]))
               {
                  launch_eff.val[1].i = cw->c->effect_funcs.pos_launch_type_get(hint->val);
                  if (launch_eff.val[0].i != launch_eff.val[1].i)
                    {
                       launch_eff.changed = EINA_TRUE;
                       send = EINA_TRUE;
                    }
               }
             else if (_STR_CMP(hint->hint, hint_names[AUX_HINT_NAME_EFFECT_CLOSE_POS]))
               {
                  close_eff.val[1].i = cw->c->effect_funcs.pos_close_type_get(hint->val);
                  if (close_eff.val[0].i != close_eff.val[1].i)
                    {
                       close_eff.changed = EINA_TRUE;
                       send = EINA_TRUE;
                    }
               }
             else if (_STR_CMP(hint->hint, hint_names[AUX_HINT_NAME_EFFECT_RENDER_COPY]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "0", render_copy.val[1].b, EINA_FALSE, render_copy.changed)
                  _VAL_CHECK(hint->val, "1", render_copy.val[1].b, EINA_TRUE,  render_copy.changed)
               }

          }

        if (hint->changed)
          {
             if (send)
               e_border_aux_hint_reply_send(bd, hint->id);
          }
     }

   if ((eff.changed) && (eff.val[0].b != eff.val[1].b))
     cw->c->effect_funcs.state_set(cw->eff_type, eff.val[1].b);

   if ((dim.changed) && (dim.val[0].b != dim.val[1].b))
     {
        E_Comp_Win_Type type = e_mod_comp_win_type_get(cw);
        if ((type == E_COMP_WIN_TYPE_DIALOG) ||
            (type == E_COMP_WIN_TYPE_NOTIFICATION))
          {
             /* TODO: make e,state,dim,on */
             if (!dim.val[1].b)
               {
                  E_Comp_Object *co;
                  Eina_List *l;
                  EINA_LIST_FOREACH(cw->objs, l, co)
                    {
                       if (!co) continue;
                       if ((co->shadow) && (co->img))
                         {
                            edje_object_signal_emit(co->shadow, "e,state,dim,off", "e");
                         }
                    }
               }
          }
     }

   if (launch_eff.changed)
     {
        //the animation effect builds on cw's edc obj and determinded by signal along the "eff_launch_style" or "eff_close_style" specify
        //and the signal can be manipulated by hint->val
        cw->eff_launch_style = launch_eff.val[1].i;
     }

   if (close_eff.changed)
     {
        cw->eff_close_style = close_eff.val[1].i;
     }

   if (render_copy.changed)
     {
        cw->skip_blend = render_copy.val[1].i;
     }

}
