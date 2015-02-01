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

static const char *hint_names[] =
{
   "wm.comp.win.effect.enable",
   "wm.comp.win.dim_bg.enable"
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

   E_Border_Aux_Hint *hint;
   Eina_List *l;
   EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
     {
        Eina_Bool send = EINA_FALSE;
        if (!hint->deleted)
          {
             if (_STR_CMP(hint->hint, hint_names[0]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "0", eff.val[1].b, EINA_FALSE, eff.changed)
                  _VAL_CHECK(hint->val, "1", eff.val[1].b, EINA_TRUE,  eff.changed)
               }
             else if (_STR_CMP(hint->hint, hint_names[1]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "0", dim.val[1].b, EINA_FALSE, dim.changed)
                  _VAL_CHECK(hint->val, "1", dim.val[1].b, EINA_TRUE,  dim.changed)
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
}
